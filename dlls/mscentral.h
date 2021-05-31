#include <ctime> // MiB 06DEC_2014 - Include required for central server time

class MSCentral
{
public:
	static void WriteFNFile( msstring FileName, msstring line , msstring mode, int lineNum = -1 ); //MiB Feb2008a
	static void ReadFNFile( msstring FileName, msstring EntString ); //MiB FEB2008a
	static bool Enabled( );
	static void RetrieveInfo( );
	static void RetrieveChar( const char *AuthID, int CharNum );
	static void StoreChar( const char *AuthID, int CharNum, const char *Data, int Size );
	static void RemoveChar( const char *AuthID, int CharNum );
	static void DoTransaction( msstringlist &Params );
	static void SaveChar( const char *AuthID, int CharNum, const char *Data, int Size, bool IsNewChar = false );
	static void Think( );
	static void Startup( );
	static void NewLevel( );
	static void GameEnd( );

	static void Print( const char *szFormat, ... );

	static bool m_Online,					//Only give offline warning message the first time a connection fails...
			    m_CachedOnline;
	static float m_TimeRetryConnect;		//Keep trying to connect if offline
	static float m_TimeBeforeLevelChange;	//Keep track of this, and offset pending sends appropriately when the new level loads
	static time_t m_CentralTime; // MiB 06DEC_2014 - Central server's time for holiday events and such
	static time_t m_CentralTimeLastRecd; // MiB 06DEC_2014 - Time we last received central server's time, in case we don't get an update for awhile
	static char m_NetworkName[256];
	static char m_MOTD[4096];
};

#define CENTRAL_FILEPREFIX "central_"
