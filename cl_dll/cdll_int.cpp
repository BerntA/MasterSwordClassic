/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
//  cdll_int.c
//
// this implementation handles the linking of the engine to the DLL
//

#include "hud.h"
#include "cl_util.h"
#include "netadr.h"
#include "vgui_schememanager.h"
#include "logfile.h"

//#define LOG_ALLEXPORTS //more exports in entity.cpp


#ifdef LOG_ALLEXPORTS
	#define logfileopt logfile
#else
	#define logfileopt NullFile
#endif

extern "C"
{
#include "pm_shared.h"
}

#include <string.h>
#include "hud_servers.h"
#include "vgui_int.h"
#include "interface.h"
#include "voice_status.h"

#define DLLEXPORT __declspec( dllexport )

cl_enginefunc_t gEngfuncs;
CHud gHUD;
TeamFortressViewport *gViewPort = NULL;

// IMAGE-SPACE GLOW - Thothie TWHL JUN2010_22 - see comments in CLRender.cpp
extern void InitScreenGlow(void);
extern void RenderScreenGlow(void);

void InitInput (void);
void EV_HookEvents( void );
void IN_Commands( void );
/*
========================== 
    Initialize

Called when the DLL is first loaded.
==========================
*/
extern "C" 
{
int		DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
int		DLLEXPORT HUD_VidInit( void );
void	DLLEXPORT HUD_Init( void );
int		DLLEXPORT HUD_Redraw( float flTime, int intermission );
int		DLLEXPORT HUD_UpdateClientData( client_data_t *cdata, float flTime );
void	DLLEXPORT HUD_Reset ( void );
void	DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server );
void	DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove );
char	DLLEXPORT HUD_PlayerMoveTexture( char *name );
int		DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
int		DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void	DLLEXPORT HUD_Frame( double time );
void	DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking);
void	DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf );
}

/*
================================
HUD_GetHullBounds

  Engine calls this to enumerate player collision hulls, for prediction.  Return 0 if the hullnumber doesn't exist.
================================
*/
int DLLEXPORT HUD_GetHullBounds( int hullnumber, float *mins, float *maxs )
{
	int iret = 0;
	DBG_INPUT;

	startdbg;
	dbg( "Begin" );

	switch ( hullnumber )
	{
	case 0:				// Normal player
		mins = Vector(-16, -16, -36);
		maxs = Vector(16, 16, 36);
		iret = 1;
		break;
	case 1:				// Crouched player
		mins = Vector(-16, -16, -18 );
		maxs = Vector(16, 16, 18 );
		iret = 1;
		break;
	case 2:				// Point based hull
		mins = Vector( 0, 0, 0 );
		maxs = Vector( 0, 0, 0 );
		iret = 1;
		break;
	}

	logfile << "[HUD_GetHullBounds: Complete]" << endl;

	enddbg;
	return iret;
}

/*
================================
HUD_ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int	DLLEXPORT HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size )
{
	DBG_INPUT;
	startdbg;
	logfileopt << "HUD_ConnectionlessPacket\r\n";
	// Parse stuff from args
	int max_buffer_size = *response_buffer_size;

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	logfileopt << "HUD_ConnectionlessPacket END\r\n";
	enddbg;
	return 0;
}

void DLLEXPORT HUD_PlayerMoveInit( struct playermove_s *ppmove )
{
	DBG_INPUT;
	startdbg;
	dbg( "Begin" );
	PM_Init( ppmove );
	enddbg;
}

char DLLEXPORT HUD_PlayerMoveTexture( char *name )
{
	DBG_INPUT;
	char ret;
	startdbg;
	ret = PM_FindTextureType( name );
	enddbg;
	return ret;
}

void DLLEXPORT HUD_PlayerMove( struct playermove_s *ppmove, int server )
{
	DBG_INPUT;
	startdbg;

	dbg( "Begin" );
	//Half-life sets the dead flag if player healh is < 1.  In MS its < 0
	PM_Move( ppmove, server );
	enddbg;
}

int DLLEXPORT Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion )
{
	DBG_INPUT;
	startdbg;

	gEngfuncs = *pEnginefuncs;

	if (iVersion != CLDLL_INTERFACE_VERSION)
		return 0;

	memcpy(&gEngfuncs, pEnginefuncs, sizeof(cl_enginefunc_t));

	EV_HookEvents();

	logfile << "[DLLEXPORT Initialize: Complete]" << endl;

	enddbg;
	return 1;
}


/*
==========================
	HUD_VidInit

Called when the game initializes
and whenever the vid_mode is changed
so the HUD can reinitialize itself.
==========================
*/

int DLLEXPORT HUD_VidInit( void )
{
	DBG_INPUT;
	startdbg;

	dbg( "Call gHUD.VidInit" );
	gHUD.VidInit();

	dbg( "Call VGui_Startup" );
	VGui_Startup();

	dbg( "Call Glow" );
	// IMAGE-SPACE GLOW - Thothie TWHL JUN2010_22 - see comments in tri.cpp
    InitScreenGlow();

	logfile << "[HUD_VidInit: Complete]" << endl;

	enddbg;

	return 1;
}

/*
==========================
	HUD_Init

Called whenever the client connects
to a server.  Reinitializes all 
the hud variables.
==========================
*/

void DLLEXPORT HUD_Init( void )
{
	DBG_INPUT;
	startdbg;

	logfile << "[HUD_Init: InitInput]" << endl;
	dbg( "Call InitInput" );
	InitInput();

	logfile << "[HUD_Init: gHUD.Init]" << endl;
	dbg( "Call gHUD.Init" );
	gHUD.Init();

	logfile << "[HUD_Init: Scheme_Init]" << endl;
	dbg( "Call Scheme_Init" );
	Scheme_Init();

	logfile << "[HUD_Init: Complete]" << endl;

	enddbg;
}


/*
==========================
	HUD_Redraw

called every screen frame to
redraw the HUD.
===========================
*/

int DLLEXPORT HUD_Redraw( float time, int intermission )
{
	DBG_INPUT;
	startdbg;

	dbg( "Call gHUD.Redraw" );

	logfileopt << "HUD_Redraw...";

    // IMAGE-SPACE GLOW - Thothie TWHL JUN2010_22 - see comments in tri.cpp
    RenderScreenGlow();

	gHUD.Redraw( time, intermission );

	//logfileopt << "END\r\n";
	enddbg;
	return 1;
}


/*
==========================
	HUD_UpdateClientData

called every time shared client
dll/engine data gets changed,
and gives the cdll a chance
to modify the data.

returns 1 if anything has been changed, 0 otherwise.
==========================
*/

int DLLEXPORT HUD_UpdateClientData(client_data_t *pcldata, float flTime )
{
	int ret = 0;
	DBG_INPUT;
	startdbg;

	dbg( "Call IN_Commands" );
	IN_Commands();

	dbg( "Call gHUD.UpdateClientData" );
	ret = gHUD.UpdateClientData(pcldata, flTime );

	enddbg;
	return ret;
}

/*
==========================
	HUD_Reset

Called at start and end of demos to restore to "non"HUD state.
==========================
*/

void DLLEXPORT HUD_Reset( void )
{
	DBG_INPUT;
	startdbg;
	dbg( "Call VidInit" );

	gHUD.VidInit();

	logfile << "[HUD_Reset: Complete]" << endl;

	enddbg;
}

/*
==========================
HUD_Frame

Called by engine every frame that client .dll is loaded
==========================
*/

void DLLEXPORT HUD_Frame( double time )
{
	DBG_INPUT;
	startdbg;

	dbg( "Call ServersThink" );
	ServersThink( time );

	dbg( "Call ServersThink DONE" );

	enddbg;
}

/*
==========================
HUD_VoiceStatus

Called when a player starts or stops talking.
==========================
*/

void DLLEXPORT HUD_VoiceStatus(int entindex, qboolean bTalking)
{
	DBG_INPUT;
	startdbg;

	dbg( "Call GetClientVoiceMgr()->UpdateSpeakerStatus" );
	GetClientVoiceMgr()->UpdateSpeakerStatus(entindex, bTalking);

	enddbg;
}

/*
==========================
HUD_DirectorEvent

Called when a director event message was received
==========================
*/

void DLLEXPORT HUD_DirectorMessage( int iSize, void *pbuf )
{
	DBG_INPUT;
	startdbg;

	dbg( "Call gHUD.m_Spectator.DirectorMessage" );
	gHUD.m_Spectator.DirectorMessage( iSize, pbuf );

	enddbg;

}

