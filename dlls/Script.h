/*

	Script.h - Script file implementation
	
*/


class CEventList : public mslist<SCRIPT_EVENT *>			//This class was created so I can store Events as pointers, but still access them as
{															//dereferenced objects
public:
	CEventList( ) : mslist<SCRIPT_EVENT *>( ) { }
	SCRIPT_EVENT &add( SCRIPT_EVENT &Event ) { SCRIPT_EVENT *pEvent = msnew(SCRIPT_EVENT); *pEvent = Event; mslist<SCRIPT_EVENT *>::add( pEvent ); return *pEvent; }
	void erase( int idx ) 
	{ 
		SCRIPT_EVENT *pEvent = mslist<SCRIPT_EVENT *>::operator [] ( idx ); 
		mslist<SCRIPT_EVENT *>::erase( idx ); 
		delete pEvent; 
	}
	SCRIPT_EVENT &operator [] ( const int idx ) const { return *mslist<SCRIPT_EVENT *>::operator [] ( idx ); }

	CEventList &operator = ( const CEventList &OtherList )
	{
		clear( );
		 for (int i = 0; i < OtherList.size(); i++) 
			add( OtherList[i] );

		return *this;
	}

	~CEventList( ) { clear( ); }

	void clear( )
	{
		while( size() )
			erase( 0 );
	}
};

class CScript : public IVariables
{
public:

	//If you ever add/change a member variable, make sure that 
	//CopyAllData() will copy it correctly to new ents

	struct props
	{
		string_i				ScriptFile;
		CBaseEntity				*pScriptedEnt;
		IScripted				*pScriptedInterface;
		CEventList				Events;
		//Not copied
		SCRIPT_EVENT			*CurrentEvent;
		bool					PrecacheOnly;
		bool					RemoveNextFrame;
		eventscope_e			DefaultScope;
		bool					AllowDupInclude;	//Allow duplicate include files
		bool					Included;			//Whether this script file is included by another
		mslist<scriptsendcmd_t> PersistentSendCmds;	//Client event command that get transmitted to each joining player
		ulong					UniqueID;			//My unique ID.  Usually set for client-side scripts that get updated by server
		bool					m_HandleRender;		//Handle certain callbacks
		ulong					m_Iteration;		//Current iteration, when called from calleventloop
	} m;

	mslist<scriptvar_t>			m_Constants;	//Local constants
	static mslist<scriptvar_t>	m_gVariables;	//Global variables
	msstringlist				m_Dependencies;	//Dependencies used by the current script.  Check that I can't #include a file twice
	static ulong				m_gLastSendID;	//ID of last script sent to a client
	static ulong				m_gLastLightID;	//ID of last dynamic light

	bool Spawn( string_i Filename, CBaseEntity *pScriptedEnt, IScripted *pScriptedInterface, bool PrecacheOnly = false, bool Casual = false );
	void RunScriptEvents( bool fOnlyRunNamedEvents = false );	//Runs all events
	void RunScriptEventByName( msstring_ref pszEventName, msstringlist *Parameters = NULL );	//Run one named event
	bool ParseScriptFile( const char *pszScriptData );
	SCRIPT_EVENT *EventByName( const char *pszEventName );
	msstring_ref GetConst( msstring_ref pszText );
	scriptvar_t *FindVar( const char *pszName );
	msstring_ref GetVar( msstring_ref pszText );
	bool VarExists( msstring_ref pszText );
	scriptvar_t *SetVar( const char *pszVarName, const char *pszValue, bool fGlobal = false );
	scriptvar_t *SetVar( const char *pszVarName, const char *pszValue, SCRIPT_EVENT &Event );
	scriptvar_t *SetVar( const char *pszVarName, int iValue, bool fGlobal = false );
	scriptvar_t *SetVar( const char *pszVarName, float flValue, bool fGlobal = false );
	Vector StringToVec( msstring_ref String );
	void CopyAllData( CScript *pDestScript, CBaseEntity *pScriptedEnt, IScripted *pScriptedInterface );
	int ParseLine( const char *pszCommandLine /*in*/, int LineNum /*in*/, SCRIPT_EVENT **pCurrentEvent /*in/out*/, scriptcmd_list **pCurrentCmds /*in/out*/, mslist<scriptcmd_list *> &ParentCmds /*in/out*/ );
	void SendScript( scriptsendcmd_t &SendCmd );	//Send script to client
	CBaseEntity *RetrieveEntity( msstring_ref Name );
	void CallEventTimed( msstring_ref EventName, float Delay );
	
	//IScripted - Don't make CScript a part of IScripted or they allocate each other in a recursive loop
	//			  These functions are just imitating IScripted

	static scriptcmdname_list m_GlobalCmds;
	static void Script_Setup( );
	static void CallScriptEventAll( msstring_ref EventName, msstringlist *Parameters );
	static void CallScriptPlayers( msstring_ref EventName, msstringlist *Parameters ); //Thothie - JUN2007a

	int Script_ParseLine( msstring_ref *pszCommandLine, scriptcmd_t &Cmd );
	bool Script_SetupEvent( SCRIPT_EVENT &Event, msstringlist *Parameters );
	bool Script_ExecuteEvent( SCRIPT_EVENT &Event, msstringlist *Parameters = NULL );
	bool Script_ExecuteCmds( SCRIPT_EVENT &Event, scriptcmd_list &Cmds );
	bool Script_ExecuteCmd( SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params );
	msstring_ref GetScriptVar( msstring_ref EventName );

	void ScriptedEffect( msstringlist &Params );
	void CLScriptedEffect( msstringlist &Params );
#ifndef VALVE_DLL
	msstring_ref CLGetCurrentTempEntProp( msstring &Prop );
	msstring_ref CLGetEntProp( struct cl_entity_s *pclEntity, msstringlist &Params );
	msstring_ref CLGetBeamProp( int beamid, msstringlist &Params ); //DEC2014_09 Thothie - beam_update
#endif

	CScript( );
	~CScript( );
};

#undef SCRIPTVAR
#define SCRIPTVAR GetFirstScriptVar
bool GetString( char *Return, const char *sentence, int start, char *endchars );
void ReplaceChar( char *pString, char org, char dest );
float GetNumeric( const char *pszText );

enum scriptconatiner_e {
	MS_SCRIPT_UKNOWN,	//GenericItemPrecache hasn't been called yet, so we dont know
	MS_SCRIPT_LIBRARY,	//Scripts loaded from sc.dll
	MS_SCRIPT_DIR		//Scripts loaded from /scripts
};
struct globalscriptinfo_t
{
	scriptconatiner_e Container; 
	const char *ContainerName;
};
extern globalscriptinfo_t *g_MSScriptInfo;
#define FILE_DEV_ITEMLIST "scripts/items.txt"
#define FILE_ITEMLIST "items.txt"

#ifdef VALVE_DLL
	#define SCRIPT_ID_START 0		//Server: ID of last script sent to a client
#else
	#define SCRIPT_ID_START 10000	//Client: ID of next script to be created
#endif

														
