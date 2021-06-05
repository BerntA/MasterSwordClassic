//
//	Communicates from the server DLL to the Central Server
//  Sends and Retrieves character files
//

//#define HAS_CENTRAL_ENABLED // ENABLE WHEN CENTRAL IS OPEN SRC?

#include "MSDllHeaders.h"
#include "MSCentral.h"
#include "Global.h"
#include "Player.h"
#include "SVGlobals.h"
#include "MSNetCode.h"
#include "logfile.h"

typedef unsigned char byte;
typedef unsigned long ulong;
typedef unsigned short ushort;

#include <string>
#include "winsock2.h"
using namespace std;

CBasePlayer *GetPlayer(const char *AuthID)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CBaseEntity *pEntity = UTIL_PlayerByIndex(i);

		if (!pEntity || !pEntity->IsPlayer())
			continue;

		CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
		if (pPlayer->AuthID() != AuthID)
			continue;

		return pPlayer;
	}

	return NULL;
}

#ifdef HAS_CENTRAL_ENABLED
#include "../MSCentral/MSCentralTransaction.h"
#endif

void MSCentral_Thread_Transaction(void *vParameters);
bool MSCentral::m_Online = false;
bool MSCentral::m_CachedOnline = false;
float MSCentral::m_TimeRetryConnect = 0;
float MSCentral::m_TimeBeforeLevelChange = 0;
char MSCentral::m_NetworkName[256];
char MSCentral::m_MOTD[4096];
// MiB 06DEC_2014 - Central server's time for holiday events and such
time_t MSCentral::m_CentralTime;
time_t MSCentral::m_CentralTimeLastRecd;

struct pendingupd_t
{
	msstring AuthID;
	int CharNum;
	float TimeUpdate;
	char *Data;
	int DataLen;
};

const char *GetWSAErrorCodeString()
{
	switch (WSAGetLastError())
	{
	case WSANOTINITIALISED:
		return "A successful WSAStartup call must occur before using this function.";
	case WSAENETDOWN:
		return "The network subsystem has failed. (Is LAN/Internet available?)";
	case WSAEADDRINUSE:
		return "The socket's local address is already in use and the socket was not marked to allow address reuse with SO_REUSEADDR. This error usually occurs when executing bind, but could be delayed until this function if the bind was to a partially wildcard address (involving ADDR_ANY) and if a specific address needs to be committed at the time of this function.";
	case WSAECONNREFUSED:
		return "Connection refused";
	default:
		return "Unknown error code";
	}
}

mslist<pendingupd_t> g_PendingUpdates;

int FNMapSynch;
msstring LastSentMap;

#ifdef HAS_CENTRAL_ENABLED
class CTransaction_DLL : public CTransaction
{
public:
	void Read(const char *FileName, const char *EntString);										   //MiB FEB2008a - FNfile I/O
	void Write(const char *FileName, const char *line, const char *EntString, const char *Handle); //MiB Feb2008a
	bool Connect(const char *Address);
	void Think();
	void RetrieveInfo();
	void RetrieveChar(const char *AuthID, int CharNum);
	void StoreChar(const char *AuthID, int CharNum, const char *Data, int DataLen);
	void RemoveChar(const char *AuthID, int CharNum);
	void Begin();
	void UpdateTimeOut() { m_TimeOut = gpGlobals->time + 5.0f; }
	CTransaction_DLL()
	{
		m_BufferLen = 0;
		m_Buffer = msnew char[BUFFER_SIZE];
		m_Disconnected = false;
		m_TimeOut = 0;
		m_CommandSent = m_CommandCompleted = false;
	}
	~CTransaction_DLL()
	{
		if (m_Buffer)
			delete m_Buffer;
		m_Buffer = NULL;
	}

	//Scoped
	//virtual void HandleMsg( msg_t &Msg );
	virtual void ReceivedChar(const char *AuthID, int CharNum, char *Data, int DataLen);
	virtual void Error_FileNotFound(msg_e MsgID, const char *AuthID, int CharNum);
	virtual void HandleMsg(msg_t &Msg);
	virtual void Disconnected();

	msstringlist m_Params;
	char *m_Buffer;
	size_t m_BufferLen;
	float m_TimeOut;
	bool m_CommandSent, m_CommandCompleted;

	static mslist<CTransaction_DLL *> m_Transactions;
};

mslist<CTransaction_DLL *> CTransaction_DLL::m_Transactions;
#endif // HAS_CENTRAL_ENABLED

void MSCentral::SaveChar(const char *AuthID, int CharNum, const char *Data, int Size, bool IsNewChar)
{
	//A character was saved.  Queue a Central Server update, if not done already

	for (int i = 0; i < g_PendingUpdates.size(); i++)
	{
		pendingupd_t &Pending = g_PendingUpdates[i];
		if (Pending.AuthID == AuthID && Pending.CharNum == CharNum)
		{
			//Already pending... just update the data and go
			delete Pending.Data;
			Pending.Data = new char[Size];
			Pending.DataLen = Size;
			memcpy(Pending.Data, Data, Pending.DataLen);
			return;
		}
	}

	pendingupd_t &Pending = g_PendingUpdates.add(pendingupd_t());
	Pending.AuthID = AuthID;
	Pending.CharNum = CharNum;
	Pending.TimeUpdate = IsNewChar ? gpGlobals->time : (gpGlobals->time + CVAR_GET_FLOAT("ms_central_pulse"));
	Pending.Data = new char[Size];
	Pending.DataLen = Size;
	memcpy(Pending.Data, Data, Pending.DataLen);
}

void MSCentral::Think()
{
	if (!MSCentral::Enabled())
		return;

	startdbg;

	dbg("MSCentral - changelevelevel delay");

	float Offset = 0;
	if (gpGlobals->time < m_TimeBeforeLevelChange)
	{
		//Changed levels - Time is now lower than before.  Go through and adjust each Pending Update
		Offset = m_TimeBeforeLevelChange;
	}
	m_TimeBeforeLevelChange = gpGlobals->time;

	dbg("MSCentral - Updates");

	for (int i = 0; i < g_PendingUpdates.size(); i++)
	{
		pendingupd_t &Pending = g_PendingUpdates[i];

		if (Offset)
			Pending.TimeUpdate -= Offset;

		if (gpGlobals->time >= Pending.TimeUpdate)
		{
			StoreChar(Pending.AuthID, Pending.CharNum, Pending.Data, Pending.DataLen);
			g_PendingUpdates.erase(i--);
		}
	}

	dbg("MSCentral - Transaction");

#ifdef HAS_CENTRAL_ENABLED
	for (int i = 0; i < CTransaction_DLL::m_Transactions.size(); i++)
	{
		CTransaction_DLL &Trans = *CTransaction_DLL::m_Transactions[i];
		if (!Trans.m_Disconnected)
			Trans.Think();
		else
		{
			delete &Trans;
			CTransaction_DLL::m_Transactions.erase(i--);
		}
	}
#endif // HAS_CENTRAL_ENABLED

	dbg("MSCentral - Cached");

	if (m_CachedOnline != m_Online)
	{
		if (m_Online)
		{
			//Server came back online.

			//Notify the server admin
			Print("Central Server connection back online.");
			ALERT(at_aiconsole, "Central Server Connection Restored"); //thothie

			//Reload the char list of anyone still waiting to join
			for (int i = 1; i <= gpGlobals->maxClients; i++)
			{
				CBaseEntity *pEntity = UTIL_PlayerByIndex(i);

				if (!pEntity || !pEntity->IsPlayer())
					continue;

				CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
				if (pPlayer->m_CharacterState != CHARSTATE_UNLOADED)
					continue;

				for (int c = 0; c < pPlayer->m_CharInfo.size(); c++)  // Reset the char states
					if (pPlayer->m_CharInfo[c].Status == CDS_LOADING) //So that preloadchar will send out
						pPlayer->m_CharInfo[c].Status = CDS_UNLOADED; //another request

				pPlayer->PreLoadChars();
			}

			//Thothie - too spammy
			//Notify players
			/*
			MESSAGE_BEGIN( MSG_ALL, g_netmsg[NETMSG_CLDLLFUNC] );
			WRITE_BYTE( 6 );
			WRITE_BYTE( 1 );
			WRITE_BYTE( 2 );
			WRITE_STRING( MSCentral::m_NetworkName );
			MESSAGE_END();
			*/

			//Thothie JUN2008a - MS Central Icon
			//bugger this, stupid, and message 91 - better to use the cvar
			/*
			CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
			if ( pGameMasterEnt )
			{
			IScripted *pGMScript = pGameMasterEnt->GetScripted();
			msstringlist Parameters;
			Parameters.add( "1" );
			pGMScript->CallScriptEvent( "game_fn_connected" , &Parameters );
			}
			*/
		}
		else
		{
			//Notify the server admin
			Print("Central Server connection broken.");
			ALERT(at_aiconsole, "Central Server Connection Broken"); //thothie
			MSCentral::m_TimeRetryConnect = 0;

			//Thothie - too spammy
			//Notify players
			/*
			 MESSAGE_BEGIN( MSG_ALL, g_netmsg[NETMSG_CLDLLFUNC] );
			 WRITE_BYTE( 6 );
			 WRITE_BYTE( 2 );
			 WRITE_BYTE( MSCentral::m_Online );
			 MESSAGE_END();
			 */

			//Thothie JUN2008a - MS Central Icon

			//Quicker to have the GM read the ms_central_online cvar, I suspect
			/*
			CBaseEntity *pGameMasterEnt = UTIL_FindEntityByString( NULL, "netname", msstring("�") + "game_master" );
			if ( pGameMasterEnt )
			{
			IScripted *pGMScript = pGameMasterEnt->GetScripted();
			msstringlist Parameters;
			Parameters.add( "0" );
			pGMScript->CallScriptEvent( "game_fn_connected" , &Parameters );
			}*/
		}

		m_CachedOnline = m_Online;
	}

	dbg("MSCentral - Retry Connect");

	float test = MSCentral::m_TimeRetryConnect;
	if (!MSCentral::m_Online && gpGlobals->time > MSCentral::m_TimeRetryConnect)
	{
		RetrieveInfo(); //Try to retrieve Central Network info.
		//Realy just a filler message to get the connection re-established

		MSCentral::m_TimeRetryConnect = gpGlobals->time + CVAR_GET_FLOAT("ms_central_pulse");
		//CVAR_SET_FLOAT( "ms_central_online", 0 );
	}

	CVAR_SET_FLOAT("ms_central_online", MSCentral::m_Online ? 1 : 0);

	enddbg;
}

bool MSCentral::Enabled()
{
	//Thothie attempting to prevent FN upload sploit
	return MSGlobals::CentralEnabled && !MSGlobals::IsLanGame && MSGlobals::ServerSideChar;
	//return CVAR_GET_FLOAT("ms_central_enabled") && !MSGlobals::IsLanGame && MSGlobals::ServerSideChar;
}
void MSCentral::Startup()
{
	clrmem(MSCentral::m_NetworkName);
	clrmem(MSCentral::m_MOTD);
	MSCentral::m_CachedOnline = false;
	MSCentral::m_Online = false;
	MSCentral::m_TimeRetryConnect = 0;
}
void MSCentral::NewLevel()
{
	MSCentral::m_CachedOnline = false;
	MSCentral::m_Online = false;
	MSCentral::m_TimeRetryConnect = 0;
}
void MSCentral::GameEnd()
{
#ifdef HAS_CENTRAL_ENABLED
	//Stop all transactions
	for (int i = 0; i < CTransaction_DLL::m_Transactions.size(); i++)
	{
		CTransaction_DLL &Trans = *CTransaction_DLL::m_Transactions[i];
		Trans.Disconnect();
		CTransaction_DLL::m_Transactions.erase(i--);
	}
#endif // HAS_CENTRAL_ENABLED
}

void MSCentral::RetrieveChar(const char *AuthID, int CharNum)
{
	//A character needs to be loaded.  Check if a save for this char is pending.  If so, used the data
	//to be saved.  If not, load as normal

	for (int i = 0; i < g_PendingUpdates.size(); i++)
	{
		pendingupd_t &Pending = g_PendingUpdates[i];
		if (Pending.AuthID == AuthID && Pending.CharNum == CharNum)
		{
			//Save is pending... use the pending data instead of requesting from the central server
			CBasePlayer *pPlayer = GetPlayer(AuthID);
			if (pPlayer)
				pPlayer->m_CharInfo[CharNum].AssignChar(CharNum, LOC_CENTRAL, Pending.Data, Pending.DataLen, pPlayer);

			return;
		}
	}

	msstringlist Params;
	Params.add("retr");
	Params.add(AuthID);
	Params.add(msstring() = CharNum);

	DoTransaction(Params);
}

void MSCentral::WriteFNFile(msstring FileName, msstring Line, msstring mode, int lineNum)
{
	msstringlist Params;
	Params.clear();
	Params.add("write");

	Params.add("");	 //AuthID  -
	Params.add("0"); //CharNum - These are both "required", but not used for this function

	Params.add(FileName);
	Params.add(Line);
	Params.add(mode);
	Params.add(msstring() = lineNum);
	DoTransaction(Params);
}

//MiB FEB2008a
void MSCentral::ReadFNFile(msstring FileName, msstring EntString)
{
	msstringlist Params;
	Params.clear();
	Params.add("read");

	Params.add("");	 //AuthID  -
	Params.add("0"); //CharNum - These are both required, but not used for this function

	Params.add(FileName);
	Params.add(EntString);

	DoTransaction(Params);
}

void MSCentral::StoreChar(const char *AuthID, int CharNum, const char *Data, int DataLen)
{
	msstringlist Params;
	Params.add("stor");
	Params.add(AuthID);
	Params.add(msstring() = CharNum);
	Params.add(msstring() = (int)Data);
	Params.add(msstring() = DataLen);

	DoTransaction(Params);
}
void MSCentral::RemoveChar(const char *AuthID, int CharNum)
{
	msstringlist Params;
	Params.add("dele");
	Params.add(AuthID);
	Params.add(msstring() = CharNum);

	DoTransaction(Params);
}
void MSCentral::RetrieveInfo()
{
	msstringlist Params;
	Params.add("info");

	DoTransaction(Params);
}

//Start a Central Server transaction
void MSCentral::DoTransaction(msstringlist &Params)
{
	if (!Enabled())
		return;

#ifdef HAS_CENTRAL_ENABLED
	CTransaction_DLL &Trans = *CTransaction_DLL::m_Transactions.add(msnew CTransaction_DLL);
	Trans.m_Params.add(CVAR_GET_STRING("ms_central_addr"));
	for (int i = 0; i < Params.size(); i++)
		Trans.m_Params.add(Params[i]);
	Trans.Begin();
#endif
}

void MSCentral::Print(const char *szFormat, ...)
{
	va_list argptr;
	static char string[1024];

	va_start(argptr, szFormat);
	vsnprintf(string, sizeof(string), szFormat, argptr);
	va_end(argptr);

	UTIL_LogPrintf(msstring("Central Server: ") + string + "\n");
}

#ifdef HAS_CENTRAL_ENABLED

void CTransaction_DLL::Begin()
{
	startdbg;
	dbg("Begin Thread");

	msstring &Addr = m_Params[0];
	msstring &Cmd = m_Params[1];

	if (Cmd == "retr")
	{
		//Update the status of this character to 'pending'
		msstring &AuthID = m_Params[2];
		int CharNum = atoi(m_Params[3]);
		CBasePlayer *pPlayer = GetPlayer(AuthID);
		if (pPlayer)
			pPlayer->Central_UpdateChar(CharNum, CDS_LOADING);
	}

	if (!Connect(Addr))
		return;

	enddbg;
}

bool CTransaction_DLL::Connect(const char *ServerAddress)
{
	sockaddr_in SockAddr;

	if (!CNetCode::ResolveAddress(ServerAddress, CENTRAL_DEFAULT_PORT, SockAddr))
	{
		msstring Addr = msstring(ServerAddress).thru_char(":");
		MSCentral::Print("Cannot resolve address: %s", Addr.c_str());
		return false;
	}

	m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//	int Result = SOCKET_ERROR;
	//	foreach( i, 3 )
	//	if( (Result = connect( m_Socket, (sockaddr *)&SockAddr, sizeof(sockaddr) )) != SOCKET_ERROR )
	//		break;

	//Disable socket blocking
	SetSocketBlocking(m_Socket, false);

	//Send connect message
	int Result = connect(m_Socket, (sockaddr *)&SockAddr, sizeof(sockaddr));

	if (!Result || (Result == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK))
	{
		UpdateTimeOut();
		return true;
	}
	else
	{
		MSCentral::m_Online = false;

		MSCentral::Print("Connect error: %s", GetWSAErrorCodeString());

		Disconnect();
		return false;
	}
}

void CTransaction_DLL::RetrieveInfo()
{
	retrinfo_t Msg;
	Msg.Msg = MSG_RETRINFO;
	Msg.Length = sizeof(Msg);

	send(m_Socket, (const char *)&Msg, sizeof(Msg), 0);
}

void CTransaction_DLL::RetrieveChar(const char *AuthID, int CharNum)
{
	retrchar_t MsgRetrChar;
	MsgRetrChar.Msg = MSG_RETRFILE;
	MsgRetrChar.Length = sizeof(MsgRetrChar);
	strncpy(MsgRetrChar.AuthID, AuthID, sizeof(MsgRetrChar.AuthID));
	MsgRetrChar.CharNum = CharNum;

	send(m_Socket, (const char *)&MsgRetrChar, sizeof(MsgRetrChar), 0);
}

void CTransaction_DLL::StoreChar(const char *AuthID, int CharNum, const char *Data, int DataLen)
{
	startdbg;
	SendChar(AuthID, CharNum, Data, DataLen);
	enddbg;
}

//MiB Feb2008a
void CTransaction_DLL::Write(const char *FileName, const char *line, const char *Mode, const char *lineNum)
{
	fnfilewrite_t FileMsg;
	strncpy(FileMsg.FileName, FileName, sizeof(FileMsg.FileName));
	strncpy(FileMsg.line, line, sizeof(FileMsg.line));
	FileMsg.type = Mode[0];
	FileMsg.lineNum = atoi(lineNum);

	FileMsg.Msg = MSG_WRITEFNFILE;
	FileMsg.Length = sizeof(FileMsg);

	send(m_Socket, (const char *)&FileMsg, sizeof(FileMsg), 0);
}

//MiB Feb2008a
void CTransaction_DLL::Read(const char *FileName, const char *EntString)
{
	fnfileread_t FileMsg;
	strncpy(FileMsg.FileName, FileName, sizeof(FileMsg.FileName));
	strncpy(FileMsg.EntString, EntString, sizeof(FileMsg.EntString));
	FileMsg.Msg = MSG_READFNFILE;
	FileMsg.Length = sizeof(FileMsg);

	send(m_Socket, (const char *)&FileMsg, sizeof(FileMsg), 0);
}

void CTransaction_DLL::RemoveChar(const char *AuthID, int CharNum)
{
	deletechar_t MsgDeleChar;
	MsgDeleChar.Msg = MSG_DELEFILE;
	MsgDeleChar.Length = sizeof(MsgDeleChar);
	strncpy(MsgDeleChar.AuthID, AuthID, sizeof(MsgDeleChar.AuthID));
	MsgDeleChar.CharNum = CharNum;

	send(m_Socket, (const char *)&MsgDeleChar, sizeof(MsgDeleChar), 0);
}

void CTransaction_DLL::ReceivedChar(const char *AuthID, int CharNum, char *Data, int DataLen)
{
	m_CommandCompleted = true;

	CBasePlayer *pPlayer = GetPlayer(AuthID);
	if (!pPlayer)
		return;

	pPlayer->Central_ReceivedChar(CharNum, Data, DataLen);
}
void CTransaction_DLL::Error_FileNotFound(msg_e MsgID, const char *AuthID, int CharNum)
{
	m_CommandCompleted = true;

	CBasePlayer *pPlayer = GetPlayer(AuthID);
	if (!pPlayer)
		return;

	pPlayer->Central_UpdateChar(CharNum, CDS_NOTFOUND);
}

void CTransaction_DLL::HandleMsg(msg_t &Msg)
{
	// MiB 06DEC_2014 - Sanity check, if messages arrived out of order
	if (Msg.CentralServerTime > MSCentral::m_CentralTime)
	{
		MSCentral::m_CentralTime = Msg.CentralServerTime; // Set the time to the one from the server
		MSCentral::m_CentralTimeLastRecd = time(NULL);	  // Set the time we received it to now
	}

	switch (Msg.Msg)
	{
	case MSG_INFO:
	{
		networkinfo_t &MsgInfo = (networkinfo_t &)Msg;
		strncpy(MSCentral::m_NetworkName, MsgInfo.NetworkName, sizeof(MSCentral::m_NetworkName));
		strncpy(MSCentral::m_MOTD, MsgInfo.MOTD, sizeof(MSCentral::m_MOTD));

		//The command went through, I'm online.
		//This is the command that determines online status.  Any failed command can bring the server
		//offline.  Then the server keeps retrying this command until it succeeds - bringing the
		//server back online.
		MSCentral::m_Online = true;
		CVAR_SET_FLOAT("ms_central_online", 1);

		SendDisconnect();
		break;
	}
	case MSG_CHAR_SUCCESS: //Returned from Central Server after I send
						   //MSG_STOR or MSG_DELE
	{
		charsuccess_t &MsgIn = (charsuccess_t &)Msg;
		CBasePlayer *pPlayer = GetPlayer(MsgIn.AuthID);
		if (pPlayer && pPlayer->m_CharacterState == CHARSTATE_UNLOADED)
			pPlayer->PreLoadChars(MsgIn.CharNum);

		SendDisconnect();
		break;
	}

	case MSG_READFNFILE: //MiB Feb2008a
	{
		fnfileread_t &ReadFile = (fnfileread_t &)Msg;
		CBaseEntity *pEnt = StringToEnt(ReadFile.EntString);
		msstring tempFileName = "654nkoaeru90234aegaweg";
		bool fileExists = ReadFile.fileExists == 1;
		if (fileExists)
		{
			char cFileName[256];
			 _snprintf(cFileName, sizeof(cFileName),  "%s/%s",  EngineFunc::GetGameDir(),  tempFileName.c_str() );
			ofstream tempOut;

			tempOut.open(cFileName);
			tempOut << ReadFile.Data;
			tempOut.close();
		}

		for (int i = 0; i < pEnt->filesOpenFN.size(); i++)
		{
			if (pEnt->filesOpenFN[i].fileName == ReadFile.FileName && !pEnt->filesOpenFN[i].readyForRead)
			{
				if (fileExists)
				{
					pEnt->filesOpenFN[i] = tempFileName;
					pEnt->filesOpenFN[i].fileName = ReadFile.FileName;
					pEnt->filesOpenFN[i].readyForRead = true;
				}
				else
				{
					pEnt->filesOpenFN[i].nofile = true;
					pEnt->filesOpenFN[i].readyForRead = true;
				}

				break;
			}
		}
		std::remove(tempFileName);
		break;
	}
	case MSG_ERROR:
	{
		//MIB JAN2010_15 FN Updates
		error_t &e = (error_t &)Msg;

		if (e.flags & ERR_BADMAP)
			FNMapSynch = -1;

		if (e.flags & ERR_POPUP)
		{
			MessageBox(NULL, e.error_msg, "Error from Central Server", MB_OK | MB_ICONEXCLAMATION);
		}

		//Always make this the last one!
		//MIB JAN2010_21
		if (e.flags & ERR_CRASH)
		{
			//Write "edana.bsp" to the crashed.cfg so auto restarters don't
			//keep looping an illegal map
			char cFileName[512];
			 _snprintf(cFileName, sizeof(cFileName),  "%s/crashed.cfg",  EngineFunc::GetGameDir() );

			std::remove(cFileName);

			ofstream Out;
			Out.open(cFileName);
			Out << "edana.bsp // written from MSC" << endl;
			Out.close();

			exit(-1);
		}
	}
	}
	CTransaction::HandleMsg(Msg);
}

void CTransaction_DLL::Think()
{
	startdbg;
	//Once connected, send first command  (after password is sent)
	if (!m_CommandSent)
	{
		dbg("Send First Command");
		fd_set FDSet;
		dbg("Clear Mem");
		clrmem(FDSet);
		dbg("FD_SET");
		FD_SET(m_Socket, &FDSet);
		dbg("Setup TimeVal");
		timeval TimeVal;
		clrmem(TimeVal);

		//Wait until the connection is established before sending anything...
		//Keep retrying until either the connection goes through or I time out
		if (select(0, NULL, &FDSet, NULL, &TimeVal) > 0)
		{
			m_CommandSent = true;
			msstring &Addr = m_Params[0];
			msstring &Cmd = m_Params[1];
			msstring &AuthID = m_Params[2];

			dbg("Read Charnum");
			int CharNum = atoi(m_Params[3]);

			dbg("Assemble Pass");
			m_Validated = true;
			password_t MsgPassword;
			clrmem(MsgPassword);
			MsgPassword.Length = sizeof(password_t);
			MsgPassword.Msg = MSG_PASSWORD_NEW;
			//Thothie JUN2007a
			//- sneaky method of FN version verification
			//- append password with ms_version
			//- maintain password on fn
			dbg("Verify Pass");
			msstring thoth_fnpass = CVAR_GET_STRING("ms_central_pass");
			//msstring thoth_fnpass_ext = CVAR_GET_STRING("ms_version");
			msstring thoth_fnpass_ext = "FEB2015c"; //if you need to lie for a month, comment the previous line, and add here
			thoth_fnpass.append(":");
			thoth_fnpass.append(thoth_fnpass_ext);
			//original january 2006 line:
			//strncpy( MsgPassword.Password, CVAR_GET_STRING("ms_central_pass"), sizeof(MsgPassword.Password) );
			strncpy(MsgPassword.Password, thoth_fnpass.c_str(), sizeof(MsgPassword.Password));

			//MiB FN Updates JAN2010_15
			//Reset our synch-level on map change.
			if (LastSentMap != MSGlobals::MapName)
				FNMapSynch = 0;

			if (FNMapSynch == 0)
			{
				LastSentMap = MSGlobals::MapName;
				//strcpy( MsgPassword.mapName , MSGlobals::MapName.c_str() );
				 _snprintf(MsgPassword.mapName, sizeof(MsgPassword.mapName),  "%s.bsp",  MSGlobals::MapName.c_str() );
				char cfileName[MAX_PATH];
				GET_GAME_DIR(cfileName);
				msstring fileName = (msstring(cfileName) + "/maps/") + MSGlobals::MapName + ".bsp";
				ifstream file;
				file.open(fileName.c_str(), ios_base::in);
				file.seekg(0, ios_base::end);
				MsgPassword.mapSize = file.tellg();
				file.close();

				FNMapSynch = 1;
			}
			//Attempting some optimization here. Stops FN from reading through the map file every few seconds per server.
			else if (FNMapSynch == 1) //FN Told us our map was good.
				 strncpy(MsgPassword.mapName,  "MAP_VERIFIED", sizeof(MsgPassword.mapName) );
			else if (FNMapSynch == -1) //FN Told us our map was bad.
				 strncpy(MsgPassword.mapName,  "BAD_MAP", sizeof(MsgPassword.mapName) );

			dbg("Socket Send");
			send(m_Socket, (const char *)&MsgPassword, sizeof(MsgPassword), 0);

			msstring thoth_debug = "FNCommand: ";
			thoth_debug.append(Cmd.c_str());

			dbg(thoth_debug);
			if (Cmd == "info")
			{
				//Retreieve info about the central server
				RetrieveInfo();
			}
			else if (Cmd == "retr")
			{
				//Retreieve a character file from the central server
				RetrieveChar(AuthID, CharNum);
			}
			else if (Cmd == "stor")
			{
				//Store a character on the central server
				const char *Data = (const char *)atoi(m_Params[4]);
				int DataLen = atoi(m_Params[5]);
				StoreChar(AuthID, CharNum, Data, DataLen);
				delete Data;
			}
			else if (Cmd == "dele")
			{
				//Remove a character file from the central server
				RemoveChar(AuthID, CharNum);
			}
			else if (Cmd == "write") //MiB Feb2008a
			{
				msstring FileName = m_Params[4];
				msstring Line = m_Params[5];
				msstring Mode = m_Params[6];
				msstring lineNum = m_Params[7];
				Write(FileName, Line, Mode, lineNum);
			}
			else if (Cmd == "read") //MiB Feb2008a
			{
				msstring FileName = m_Params[4];
				msstring EntString = m_Params[5];
				Read(FileName, EntString);
			}
		}
	}

	//Loop and receive data
	dbg("Receive Data");
	int DataLen = recv(m_Socket, &m_Buffer[m_BufferLen], 4096, 0);

	dbg("DataLen");
	if (DataLen != SOCKET_ERROR)
	{
		if (m_BufferLen + DataLen <= BUFFER_SIZE) //Data too big for buffer
		{

			m_BufferLen += DataLen;

			dbg("Handle Buffer");
			while (m_BufferLen >= sizeof(msg_t) && m_BufferLen >= ((msg_t *)m_Buffer)->Length)
			{
				msg_t &Msg = *(msg_t *)m_Buffer;
				size_t Length = Msg.Length;
				HandleMsg(Msg);
				if (m_BufferLen > Length)
					memmove(m_Buffer, &m_Buffer[Length], m_BufferLen - Length);
				m_BufferLen -= Length;
				if (m_Disconnected)
					break;
			}
		}
	}

	dbg("Disconnect on timeout");
	if (gpGlobals->time > m_TimeOut)
	{
		//Couldn't compelete command.  Assume connection is broken
		MSCentral::m_Online = false;

		Disconnect();
	}

	enddbg;
}
void CTransaction_DLL::Disconnected()
{
	if (m_CommandSent)
	{
		if (m_Params[1] == "retr" && !m_CommandCompleted)
		{
			CBasePlayer *pPlayer = GetPlayer(m_Params[2]);
			if (!pPlayer)
				return;

			pPlayer->Central_UpdateChar(atoi(m_Params[3]), CDS_ERROR);
		}
	}
}

void CTransaction::GetCharFileName(const char *AuthID, int iCharacter, string &FileName)
{
	char cFileName[MAX_PATH];

	//Server
	//Print("CHAR_SAVE_DEBUG [MScentral]: %s %#i\n",cFileName,iCharacter+1); //MAR2010_08
	_snprintf(cFileName, MAX_PATH,  "%s/save/%s%s_%i.char",  EngineFunc::GetGameDir(),  CENTRAL_FILEPREFIX,  AuthID,  iCharacter + 1 );
	ReplaceChar(cFileName, ':', '-');

	FileName = cFileName;
}

#endif // HAS_CENTRAL_ENABLED