//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: VGUI scoreboard
//
// $Workfile:     $
// $Date: 2004/11/07 01:06:28 $
//
//-----------------------------------------------------------------------------
// $Log: vgui_ScorePanel.cpp,v $
// Revision 1.6  2004/11/07 01:06:28  dogg
// New Event Console
//
// Revision 1.5  2004/10/16 11:47:01  dogg
// no message
//
// Revision 1.4  2004/10/14 12:15:31  dogg
// no message
//
// Revision 1.3  2004/10/13 20:21:51  dogg
// Big update
// Netcode re-arranged
//
// Revision 1.2  2004/09/08 03:20:54  dogg
// no message
//
//
// $NoKeywords: $
//=============================================================================


#include<VGUI_LineBorder.h>

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"

//Master Sword
#include "parsemsg.h"
#include "MSDLLHeaders.h"
#include "Player/player.h"
#include "Stats\Statdefs.h"
#include "../MSShared/Global.h"

hud_player_info_t	 g_PlayerInfoList[MAX_PLAYERS+1];	 // player info from the engine
extra_player_info_t  g_PlayerExtraInfo[MAX_PLAYERS+1];   // additional player info sent directly to the client dll
team_info_t			 g_TeamInfo[MAX_TEAMS+1];
int					 g_IsSpectator[MAX_PLAYERS+1];

int HUD_IsGame( const char *game );
int EV_TFC_IsAllyTeam( int iTeam1, int iTeam2 );

// Scoreboard dimensions
#define SBOARD_TITLE_SIZE_Y			YRES(24)
#define SBOARD_HEADER_SIZE_Y		YRES(40)
#define SBOARD_TABLE_X				XRES(16)
#define SBOARD_TABLE_Y				(SBOARD_TITLE_SIZE_Y + SBOARD_HEADER_SIZE_Y)

#define SBOARD_TEAM_CELL_SIZE_Y		YRES(20)
#define SBOARD_CELL_SIZE_Y			YRES(13)

// Column sizes
#define CSIZE_ICON				XRES(18)
#define CSIZE_NAME				CSIZE_ICON + XRES(120)
#define CSIZE_PARTY				CSIZE_NAME + XRES(90)
#define CSIZE_TITLE				CSIZE_PARTY + XRES(80)
#define CSIZE_SKILL				CSIZE_TITLE + XRES(50)

/*#define SMALL_CSIZE_NAME		XRES(124)					
#define SMALL_CSIZE_CLASS		SMALL_CSIZE_NAME + XRES(60)	
#define SMALL_CSIZE_KILLS		SMALL_CSIZE_CLASS + XRES(64)
#define SMALL_CSIZE_DEATHS		SMALL_CSIZE_KILLS + XRES(70)*/

#define TEAM_NO				0
#define TEAM_YES			1
#define TEAM_UNASSIGNED		2
#define TEAM_SPECTATORS		3

enum {
	TEAM_JOIN = 0,
	//TEAM_UNASSIGNED = TEAM_JOIN,
	TEAM_SPEC,
};
char *DefaultTeamName[] =
{
	"No Team",
	"Yes Team",
	"Joining...",
	"Spectators"
};

// Team Colors used in the scoreboard
int ScoreColorsBG[5][3] =
{
	{ 0, 0, 0 },
	{ 66, 114, 247 },
	{ 220, 51, 38 },
	{ 236, 212, 48 },
	{ 68, 199, 42 },
};

int ScoreColorsFG[5][3] =
{
	{ 255, 255, 255 },
	{ 170, 193, 251 },
	{ 215, 151, 146 },
	{ 227, 203, 46 },
	{ 143, 215, 142 },
};

//-----------------------------------------------------------------------------
// Purpose: Set different cell heights for Teams and Players
//-----------------------------------------------------------------------------
int ScoreTablePanel::getCellTall(int row)
{
	if ( m_iIsATeam[row] )
		return SBOARD_TEAM_CELL_SIZE_Y;

	return SBOARD_CELL_SIZE_Y;
}
ScoreTablePanel::ScoreTablePanel(int x,int y,int wide,int tall,int columnCount) : TablePanel(x,y,wide,tall,columnCount)
{
	setCellEditingEnabled(false);
	
	m_pLabel = new Label( "", 0, 0, wide, tall );
	m_pLabel->setFont( Scheme::sf_primary2 );

	m_pIcon[0] = new CImageDelayed( "hud_trans", false, false, 0, 0, 16, 16 );
}


//-----------------------------------------------------------------------------
// Purpose: Render each of the cells in the Table
//-----------------------------------------------------------------------------
Panel* ScoreTablePanel::getCellRenderer(int column,int row,bool columnSelected,bool rowSelected,bool cellSelected)
{
	char sz[128];
	//hud_player_info_t *pl_info = NULL;
	team_info_t *team_info = NULL;

	if ( m_iIsATeam[row] == TEAM_YES )
	{
		// Get the team's data
		team_info = &g_TeamInfo[ m_iSortedRows[row] ];

		// White text for team names
		m_pLabel->setFgColor(Scheme::sc_white);

		// Set background color
		m_pLabel->setBgColor( ScoreColorsBG[ team_info->teamnumber ][0], ScoreColorsBG[ team_info->teamnumber ][1], ScoreColorsBG[ team_info->teamnumber ][2], 128 );
	}
	else if ( m_iIsATeam[row] == TEAM_UNASSIGNED || m_iIsATeam[row] == TEAM_SPECTATORS )
	{
		// White text for team names
		m_pLabel->setFgColor(Scheme::sc_white);

		// Set background color
		m_pLabel->setBgColor( 0,0,0, 255 );
	}
	else
	{
		//Fill out row-based information (colors)
		//---------------------------------------

		// Grey text for player names
//		m_pLabel->setFgColor( ScoreColorsFG[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber ][0], ScoreColorsFG[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber ][1], ScoreColorsFG[ g_PlayerExtraInfo[ m_iSortedRows[row] ].teamnumber ][2], 128 );
		m_pLabel->setFgColor( ScoreColorsFG[ 0 ][0], ScoreColorsFG[ 0 ][1], ScoreColorsFG[ 0 ][2], 128 );

		// Get the player's data
		hud_player_info_t &Info = g_PlayerInfoList[ m_iSortedRows[row] ];
		extra_player_info_t &ExtraInfo = g_PlayerExtraInfo[ m_iSortedRows[row] ];

		// Set background color
		if ( Info.thisplayer ) // if it is their name, draw it a different color
		{
			// Highlight this player
			m_pLabel->setFgColor( Scheme::sc_white );
			m_pLabel->setBgColor( ScoreColorsBG[ 0 ][0], ScoreColorsBG[ 0 ][1], ScoreColorsBG[ 0 ][2], 196 );
		}
		else if ( m_iSortedRows[row] == m_iLastKilledBy && m_fLastKillTime && m_fLastKillTime > gHUD.m_flTime )
		{
			// Killer's name
			m_pLabel->setBgColor( 255,0,0, 255 - ((float)15 * (float)(m_fLastKillTime - gHUD.m_flTime)) );
		}
		else
		{
			m_pLabel->setBgColor( 0,0,0, 255 );
		}


		if( MSGlobals::CurrentVote.fActive && PlayerVotedYes(m_iSortedRows[row]) )
		{
			//Player voted yes to a current vote.  Change the background color to green
			m_pLabel->setBgColor( 0, 90, 0, 255-90 );
		}

		//Fill out column-based information (text)
		//----------------------------------------

		bool bShowClass = false;
		bool fLoading = false;
		if( !Info.thisplayer &&
			&ExtraInfo.SkillAve < 0 )
			fLoading = true;

		switch (column)
		{
		case 0:
			if( FBitSet(ExtraInfo.Flags,(1<<1)) )
				return m_pIcon[0];					//Standing in transition
			else
				break;
		case 1:
			{
				if( Info.thisplayer )
					sprintf(sz, "  %s", STRING(player.DisplayName));
				sprintf(sz, "  %s", Info.name);
			}
			break;
		case 2:
			{
				if( fLoading ) strcpy( sz, "Loading..." );
				else if( !&ExtraInfo.teamname[0] )
					strcpy( sz, "None" );
				else sprintf(sz, "%s",  &ExtraInfo.teamname );
			}
			break;
		case 3:
			{
				/*if( Info.thisplayer )
					sprintf(sz, "%s", player.GetTitle( ) );
				else
				{
					if( fLoading ) strcpy( sz, "N/A" );
					else
						sprintf(sz, "%s",  GetPlayerTitle( ExtraInfo.Title ) );
				}*/
				strcpy( sz, ExtraInfo.Title );
			}
			break;
		case 4:
			{
				if( fLoading ) strcpy( sz, "N/A" );
				else
					//strcpy( sz, "" );
					sprintf(sz, "%d", ExtraInfo.SkillAve );
			}
			break;
		case 5:
			{
				sprintf(sz, "%d", Info.ping );
			}
			break;
		default:
			strcpy(sz, "");
		}
	}

	// Align 
	if (column <= 2)
	{
		if ( m_iIsATeam[row] )
			m_pLabel->setContentAlignment( vgui::Label::a_southwest );
		else
			m_pLabel->setContentAlignment( vgui::Label::a_west );
	}
	else
	{
		if ( m_iIsATeam[row] )
			m_pLabel->setContentAlignment( vgui::Label::a_south );
		else
			m_pLabel->setContentAlignment( vgui::Label::a_center );
	}

	// Fill out with the correct data
	if ( m_iIsATeam[row] )
	{
		m_pLabel->setText("");
	}
	else
	{
		//m_pLabel->setText( "" );
		m_pLabel->setText( sz );
	}

	return m_pLabel;
}

//-----------------------------------------------------------------------------
// Purpose: Create the ScoreBoard panel
//-----------------------------------------------------------------------------
ScorePanel::ScorePanel(int x,int y,int wide,int tall) : Panel(x,y,wide,tall)
{
	setBorder( new LineBorder( Color(255 * 0.7,170 * 0.7,0,0) ) );
	setBgColor( 0,0,0, 100 );

	m_pTitleLabel = new Label( "  SCORES", 0,0, wide, SBOARD_TITLE_SIZE_Y );
	m_pTitleLabel->setBgColor( Scheme::sc_primary2 );
	m_pTitleLabel->setFgColor( Scheme::sc_primary1 );
	m_pTitleLabel->setContentAlignment( vgui::Label::a_west );
	m_pTitleLabel->setParent(this);

	_headerPanel = new HeaderPanel( SBOARD_TABLE_X, SBOARD_TITLE_SIZE_Y, wide - (SBOARD_TABLE_X * 2), SBOARD_HEADER_SIZE_Y);
	_headerPanel->setParent(this);

	// BUGBUG: This isn't working. gHUD.m_Teamplay hasn't been initialized yet.
	//if ( gHUD.m_Teamplay )
	//	_headerPanel->addSectionPanel( new CLabelHeader( CHudTextMessage::BufferedLocaliseTextString( "#TEAMS" ), true) );
	//else
	_headerPanel->addSectionPanel( new CLabelHeader( "" ) );
	_headerPanel->addSectionPanel( new CLabelHeader( "Name", true) );

	_headerPanel->addSectionPanel( new CLabelHeader( "Party", true) );

	_headerPanel->addSectionPanel( new CLabelHeader( "Title" ) );
	_headerPanel->addSectionPanel( new CLabelHeader( "Skill Level" ) );
	_headerPanel->addSectionPanel( new CLabelHeader( "Ping" ) );
	_headerPanel->setBgColor( 0,0,0, 255 );

	// Need to special case 400x300, because the titles just wont fit otherwise
	/*if ( ScreenWidth == 400 )
	{
		_headerPanel->setSliderPos( 1, SMALL_CSIZE_CLASS );
		_headerPanel->setSliderPos( 2, SMALL_CSIZE_KILLS );
		_headerPanel->setSliderPos( 3, SMALL_CSIZE_DEATHS );
		_headerPanel->setSliderPos( 4, wide - (SBOARD_TABLE_X * 2) - 1 );
	}
	else
	{*/
	int Columns = 0;
	_headerPanel->setSliderPos( Columns++, CSIZE_ICON );
	_headerPanel->setSliderPos( Columns++, CSIZE_NAME );
	_headerPanel->setSliderPos( Columns++, CSIZE_PARTY );
	_headerPanel->setSliderPos( Columns++, CSIZE_TITLE );
	_headerPanel->setSliderPos( Columns++, CSIZE_SKILL );
	_headerPanel->setSliderPos( Columns++, wide - (SBOARD_TABLE_X * 2) - 1 );
	//}

	_tablePanel = new ScoreTablePanel( SBOARD_TABLE_X, SBOARD_TABLE_Y, wide - (SBOARD_TABLE_X * 2), tall - SBOARD_TABLE_Y, Columns );
	_tablePanel->setParent(this);
	_tablePanel->setHeaderPanel(_headerPanel);
	_tablePanel->setBgColor( 0,0,0, 255 );

	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void ScorePanel::Initialize( void )
{
	// Clear out scoreboard data
	_tablePanel->m_iLastKilledBy = 0;
	_tablePanel->m_fLastKillTime = 0;
	m_iPlayerNum = 0;
	m_iNumTeams = 0;
	memset( g_PlayerExtraInfo, 0, sizeof g_PlayerExtraInfo );
	memset( g_TeamInfo, 0, sizeof g_TeamInfo );
}

//-----------------------------------------------------------------------------
// Purpose: Set the size of the header and table when this panel's size changes
//-----------------------------------------------------------------------------
void ScorePanel::setSize(int wide,int tall)
{
	Panel::setSize(wide,tall);

	_headerPanel->setBounds(0,0,wide,SBOARD_HEADER_SIZE_Y);
	_tablePanel->setBounds(0,20,wide,tall - SBOARD_HEADER_SIZE_Y);
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the internal scoreboard data
//-----------------------------------------------------------------------------
void ScorePanel::Update()
{
	// Set the title
	if (gViewPort->m_szServerName)
	{
		char sz[MAX_SERVERNAME_LENGTH + 16];
		sprintf(sz, "  SCORES: %s", gViewPort->m_szServerName );
		m_pTitleLabel->setText(sz);
	}

	_tablePanel->m_iRows = 0;
	gViewPort->GetAllPlayersInfo();

	// Clear out sorts
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		_tablePanel->m_iSortedRows[i] = 0;
		_tablePanel->m_iIsATeam[i] = TEAM_NO;
		_tablePanel->m_bHasBeenSorted[i] = false;
	}

	// If it's not teamplay, sort all the players. Otherwise, sort the teams.
	//if ( !gHUD.m_Teamplay )
		SortPlayers( 0, NULL );
	//else
	//	SortTeams();
}

//-----------------------------------------------------------------------------
// Purpose: Sort all the teams
//-----------------------------------------------------------------------------
void ScorePanel::SortTeams()
{
	// clear out team scores
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		if ( !g_TeamInfo[i].scores_overriden )
			g_TeamInfo[i].frags = g_TeamInfo[i].deaths = 0;
		g_TeamInfo[i].ping = g_TeamInfo[i].packetloss = 0;
	}

	// recalc the team scores, then draw them
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( g_PlayerInfoList[i].name == NULL )
			continue; // empty player slot, skip

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

		// find what team this player is in
		for ( int j = 1; j <= m_iNumTeams; j++ )
		{
			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}
		if ( j > m_iNumTeams )  // player is not in a team, skip to the next guy
			continue;

		/*if ( !g_TeamInfo[j].scores_overriden )
		{
			g_TeamInfo[j].frags += g_PlayerExtraInfo[i].frags;
			g_TeamInfo[j].deaths += g_PlayerExtraInfo[i].deaths;
		}*/

		g_TeamInfo[j].ping += g_PlayerInfoList[i].ping;
		g_TeamInfo[j].packetloss += g_PlayerInfoList[i].packetloss;

		if ( g_PlayerInfoList[i].thisplayer )
			g_TeamInfo[j].ownteam = TRUE;
		else
			g_TeamInfo[j].ownteam = FALSE;

		// Set the team's number (used for team colors)
		g_TeamInfo[j].teamnumber = g_PlayerExtraInfo[i].teamnumber;
	}

	// find team ping/packetloss averages
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].already_drawn = FALSE;

		if ( g_TeamInfo[i].players > 0 )
		{
			g_TeamInfo[i].ping /= g_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
			g_TeamInfo[i].packetloss /= g_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
		}
	}

	// Draw the teams
	while ( 1 )
	{
		int highest_frags = -99999; int lowest_deaths = 99999;
		int best_team = 0;

		for ( i = 1; i <= m_iNumTeams; i++ )
		{
			if ( g_TeamInfo[i].players < 1 )
				continue;

			if ( !g_TeamInfo[i].already_drawn && g_TeamInfo[i].frags >= highest_frags )
			{
				if ( g_TeamInfo[i].frags > highest_frags || g_TeamInfo[i].deaths < lowest_deaths )
				{
					best_team = i;
					lowest_deaths = g_TeamInfo[i].deaths;
					highest_frags = g_TeamInfo[i].frags;
				}
			}
		}

		// draw the best team on the scoreboard
		if ( !best_team )
			break;

		// Put this team in the sorted list
		_tablePanel->m_iSortedRows[ _tablePanel->m_iRows ] = best_team;
		_tablePanel->m_iIsATeam[ _tablePanel->m_iRows ] = TEAM_YES;
		g_TeamInfo[best_team].already_drawn = TRUE;  // set the already_drawn to be TRUE, so this team won't get sorted again
		_tablePanel->m_iRows++;

		// Now sort all the players on this team
		SortPlayers( 0, g_TeamInfo[best_team].name );
	}

	// Now add all the spectators
	SortPlayers( TEAM_SPECTATORS, NULL );

	// Now add all the players who aren't in a team yet
	SortPlayers( TEAM_UNASSIGNED, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: Sort a list of players
//-----------------------------------------------------------------------------
void ScorePanel::SortPlayers( int iTeam, char *team )
{
	bool bCreatedTeam = false;

	// draw the players, in order,  and restricted to team if set
	while ( 1 )
	{
		// Find the top ranking player
		int highest_frags = -99999;	int lowest_deaths = 99999;
		int best_player;
		best_player = 0;

		for ( int i = 1; i < MAX_PLAYERS; i++ )
		{
			if ( _tablePanel->m_bHasBeenSorted[i] == false && g_PlayerInfoList[i].name && g_PlayerExtraInfo[i].SkillAve >= highest_frags )
			{
				cl_entity_t *ent = gEngfuncs.GetEntityByIndex( i );

				//if ( ent && ((iTeam == TEAM_SPECTATORS && g_IsSpectator[i] != 0) || (iTeam != TEAM_SPECTATORS && !(team && stricmp(g_PlayerExtraInfo[i].teamname, team)))) )  
				if( ent )
				{
					extra_player_info_t *pl_info = &g_PlayerExtraInfo[i];
					if ( pl_info->SkillAve > highest_frags )
					{
						best_player = i;
						//lowest_deaths = pl_info->deaths;
						highest_frags = pl_info->SkillAve;
					}
				}
			}
		}

		if ( !best_player )
			break;

		// If we haven't created the Team yet, do it first
		if( !bCreatedTeam && iTeam )
		{
			_tablePanel->m_iIsATeam[ _tablePanel->m_iRows ] = TEAM_NO;//iTeam;
			_tablePanel->m_iRows++;

			bCreatedTeam = true;
		}

		// Put this player in the sorted list
		_tablePanel->m_iSortedRows[ _tablePanel->m_iRows ] = best_player;
		_tablePanel->m_bHasBeenSorted[ best_player ] = true;
		_tablePanel->m_iRows++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate the existing teams in the match
//-----------------------------------------------------------------------------
void ScorePanel::RebuildTeams()
{
	return;
	// clear out player counts from teams
	for ( int i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].players = 0;
	}

	// rebuild the team list
	gViewPort->GetAllPlayersInfo();
	m_iNumTeams = 0;
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( g_PlayerInfoList[i].name == NULL )
			continue;

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

		// is this player in an existing team?
		for ( int j = 1; j <= m_iNumTeams; j++ )
		{
			if ( g_TeamInfo[j].name[0] == '\0' )
				break;

			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}

		if ( j > m_iNumTeams )
		{ // they aren't in a listed team, so make a new one
			// search through for an empty team slot
			for ( int j = 1; j <= m_iNumTeams; j++ )
			{
				if ( g_TeamInfo[j].name[0] == '\0' )
					break;
			}
			m_iNumTeams = max( j, m_iNumTeams );

			strncpy( g_TeamInfo[j].name, g_PlayerExtraInfo[i].teamname, MAX_TEAM_NAME );
			g_TeamInfo[j].players = 0;
		}

		g_TeamInfo[j].players++;
	}

	// clear out any empty teams
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		if ( g_TeamInfo[i].players < 1 )
			memset( &g_TeamInfo[i], 0, sizeof(team_info_t) );
	}

	// Update the scoreboard
	Update();
}

//-----------------------------------------------------------------------------
// Purpose: Setup highlights for player names in scoreboard
//-----------------------------------------------------------------------------
void ScorePanel::DeathMsg( int killer, int victim )
{
	// if we were the one killed,  or the world killed us, set the scoreboard to indicate suicide
	if ( victim == m_iPlayerNum || killer == 0 )
	{
		_tablePanel->m_iLastKilledBy = killer ? killer : m_iPlayerNum;
		_tablePanel->m_fLastKillTime = gHUD.m_flTime + 10;	// display who we were killed by for 10 seconds

		if ( killer == m_iPlayerNum )
			_tablePanel->m_iLastKilledBy = m_iPlayerNum;
	}
}
void ScorePanel::MSG_ScoreInfo( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	short	cl = READ_BYTE();
	byte	Flags = READ_BYTE();
	msstring_ref sTitle = READ_STRING();
	//byte	iTitle = READ_BYTE();
	short	iSkillave = READ_SHORT();

	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		strcpy( g_PlayerExtraInfo[cl].Title, sTitle );
		g_PlayerExtraInfo[cl].SkillAve = iSkillave;
		g_PlayerExtraInfo[cl].teamnumber = 0;//teamnumber;
		g_PlayerExtraInfo[cl].Flags = Flags;

		gViewPort->UpdateOnPlayerInfo();
	}
}

