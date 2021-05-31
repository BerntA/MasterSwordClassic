//Client-side only file

#ifdef NOEXTERN
	#include <stdarg.h>
	char* UTIL_VarArgs( char *format, ... ) { //from Valve
		va_list		argptr;
		static char		string[1024];
		
		va_start (argptr, format);
		vsprintf (string, format,argptr);
		va_end (argptr);

		return (char *)string;	
	}
#else 
	extern int numofdigits( int x );
	extern char *UTIL_VarArgs( char *format, ... );

#endif //NOEXTERN

#ifdef EXTDLL_H
	//Important - our local global stuff
	struct MSCLGlobals_s {
	// Pool of client side entities/entvars_t
		entvars_t	*entvar1;
		int			num_ents;
	};
	extern MSCLGlobals_s *MS_CLGlobals;

	inline CBaseEntity *PrivData( entvars_t *pev ) 
		{ return (CBaseEntity *)pev->pContainingEntity; }
#endif

void MSGlobalInitCL( );
void MSGlobalCleanUp( );

#include <string.h>
#ifndef SF_TRIG_PUSH_ONCE //This just means FStrEq is already defined since util.h was included
	inline BOOL FStrEq(const char*sz1, const char*sz2) //from Valve
			{ return (strcmp(sz1, sz2) == 0); }
#endif

void MSGlobalThink( );
