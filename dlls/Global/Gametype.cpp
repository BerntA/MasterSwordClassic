#include "extdll.h"
#include "util.h"
#include "../cbase.h"
#include "../Player/player.h"
#include "MSGlobals.h"
#include "../Stats/statdefs.h"
// Entity that determines the Game type

/*class CBasePlayerSpawnHuman : public CBaseEntity {
	void Spawn( ) { pev->classname = MAKE_STRING("playerspawn_human"); }
};
class CBasePlayerSpawnDwarf : public CBaseEntity {
	void Spawn( ) { pev->classname = MAKE_STRING("playerspawn_dwarf"); }
};
class CBasePlayerSpawnElf : public CBaseEntity {
	void Spawn( ) { pev->classname = MAKE_STRING("playerspawn_elf"); }
};
*/
DLL_GLOBAL gametype_e GameType;
DLL_GLOBAL int	bGameTypeInitialized = FALSE;

LINK_ENTITY_TO_CLASS( playerspawn_human, CBaseEntity );
LINK_ENTITY_TO_CLASS( playerspawn_dwarf, CBaseEntity );
LINK_ENTITY_TO_CLASS( playerspawn_elf, CBaseEntity );


BOOL IsSpawnPointValid( CBaseEntity *pPlayer, CBaseEntity *pSpot );

CBaseEntity *GetAssaultSpawnSpot( CBasePlayer *pPlayer ) {
	CBaseEntity *pFirstSpot, *pSpot = NULL;
	char *SpawnSpotName;
	int RaceID, n = 0;
	
	if( !pPlayer->Race ) RaceID = RANDOM_LONG(0,RACE_NUM-1);
	else RaceID = pPlayer->Race->id;

	if( RaceID == RACE_HUMAN ) SpawnSpotName = "playerspawn_human";
	else if( RaceID == RACE_DWARF ) SpawnSpotName = "playerspawn_dwarf";
	else if( RaceID == RACE_ELF ) SpawnSpotName = "playerspawn_elf";

		
	pFirstSpot = UTIL_FindEntityByClassname( pSpot, SpawnSpotName );
	if( FNullEnt(pFirstSpot) ) {
		ALERT( at_console, "COULD NOT FIND ONE %s entity to SPAWN!!\n", SpawnSpotName );
		return NULL;
	}

	while ( pSpot != pFirstSpot ) {
		// check if pSpot is valid
		if( pSpot && IsSpawnPointValid(pPlayer, pSpot) && RANDOM_LONG(0,1) ) break;
		n++;
		if( n > 255 ) {
			ALERT( at_console, "RACE SPAWN ERROR: Couldn't loop back to first %s\n", SpawnSpotName );
			return NULL;
		}
		// increment pSpot
		if( !pSpot ) pSpot = UTIL_FindEntityByClassname( pFirstSpot, SpawnSpotName );
		else pSpot = UTIL_FindEntityByClassname( pSpot, SpawnSpotName );
	} 

	if( pSpot == pFirstSpot && !IsSpawnPointValid(pPlayer, pSpot) ) {
		//No valid spots except the first and somebody's standing there...
		//!!Broken!, the traceline below doesn't account for the player thats blocking my spawn
		ALERT( at_console, "Telefrag!\n" );
		int i = 0;
		BOOL bCanBreak = FALSE;
		TraceResult tr;
		while( !bCanBreak && i < 100 ) {
			float fAngle = RANDOM_FLOAT(0,359);
			Vector vDir, vecSrc, vecEnd;
			UTIL_MakeVectorsPrivate( Vector(0,fAngle,0), vDir, NULL, NULL );
			vecSrc = pSpot->pev->origin; vecEnd = pSpot->pev->origin + vDir * 250;
			UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pSpot->edict(), &tr );
			if ( tr.flFraction >= 1.0 ) bCanBreak = TRUE;
			i++;
		}
		if( bCanBreak ) {
			ALERT( at_console, "Avoided a Telefrag!\n" );
			pSpot = GetClassPtr((CBaseEntity *)NULL);
			pSpot->pev->origin = tr.vecEndPos;
			pSpot->pev->flags |= FL_KILLME;
			pSpot->pev->effects |= EF_NODRAW;
		}
		else ALERT( at_console, "Sorry, there's no where to spawn!!!\n" );
	}
	return pSpot;
}