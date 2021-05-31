#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "logfile.h"
CMSStream logfile;
CMSStream NullFile;

void *MSCopyClassMemory( void *pDest, void *pSource, size_t Length )
{
	long lFirstPtr = *(long *)pDest;
	void *pReturn = CopyMemory( pDest, pSource, Length );
	*(long *)pDest = lFirstPtr;
	return pReturn;
}

void *MSZeroClassMemory( void *pDest, size_t Length )
{
	long lFirstPtr = *(long *)pDest;
	void *pReturn = ZeroMemory( pDest, Length );
	*(long *)pDest = lFirstPtr;
	return pReturn;
}

#ifndef VALVE_DLL
	#include "../cl_dll/MasterSword/MSGlobal.h"
#endif
CBaseEntity *MSInstance( entvars_t *pev )
{
	if( !pev ) return NULL;
#ifdef VALVE_DLL
	CBaseEntity *pEnt = GetClassPtr((CBaseEntity *)pev); 
#else
	//In the client DLL, edict() (our parameter) == pev
	CBaseEntity *pEnt = PrivData((entvars_t *)pev);
#endif
	return pEnt; 
}
CBaseEntity *MSInstance( edict_t *pent )
{
	if( !pent ) return NULL;
#ifdef VALVE_DLL
	CBaseEntity *pEnt = (CBaseEntity *)GET_PRIVATE(pent); 
#else
	//In the client DLL, edict() (our parameter) == pev
	CBaseEntity *pEnt = PrivData((entvars_t *)pent);
#endif
	return pEnt; 
}

int iBeam;

Vector GetHighBone( entvars_t *pev, int Bone ) {
	//Bones positions above the waist are way off so this tries to get it close
	//Y needs to be rotated +/- 120 degrees and the
	//engine swaps the X and Z values so you have to compensate for that
//	ALERT( at_console, "x: %f\n", pev->angles.x );
	Vector vTemp1 = Vector(0,0,0);
	float yrot = 124;
	float oldx = pev->angles.x;
	float oldz = pev->angles.z;
	if( oldx < 0 ) pev->angles.x *= 1.2;
	else pev->angles.x /= -16;
	
	pev->angles.y += yrot;
	if( oldx < 0 ) pev->angles.z = -oldx*1.5;
	else pev->angles.z = -oldx;

	GET_BONE_POSITION( ENT(pev), Bone, vTemp1, NULL );
	pev->angles.x = oldx;
	pev->angles.y -= yrot;
	pev->angles.z = oldz;

	return vTemp1; 
}
void BeamEffect( float SRCx, float SRCy, float SRCz, float DESTx,
				float DESTy, float DESTz, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed ) {
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, NULL );
		WRITE_BYTE( TE_BEAMPOINTS);
		WRITE_COORD( SRCx ); WRITE_COORD( SRCy ); WRITE_COORD( SRCz );
		WRITE_COORD( DESTx ); WRITE_COORD( DESTy ); WRITE_COORD( DESTz );
		WRITE_SHORT( sprite );
		WRITE_BYTE( startframe ); // startframe
		WRITE_BYTE( framerate ); // framerate
		WRITE_BYTE( life );		//life
		WRITE_BYTE( width );	// width
		WRITE_BYTE( noise );	// noise
		WRITE_BYTE( r ); WRITE_BYTE( g ); WRITE_BYTE( b );
		WRITE_BYTE( brightness );	// brightness
		WRITE_BYTE( ispeed );		// speed
	MESSAGE_END();
}
void BeamEffect( Vector vStart, Vector vEnd, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed ) {
	BeamEffect( vStart.x, vStart.y, vStart.z, vEnd.x, vEnd.y, vEnd.z, 
		sprite, startframe,framerate, life, width, noise, r, g, b, 
		brightness, ispeed );
}

BOOL UTIL_IsPointWithinEntity( Vector &vPoint, CBaseEntity *pEntity ) {
	if ( vPoint.x > pEntity->pev->absmax.x ||
		 vPoint.y > pEntity->pev->absmax.y ||
		 vPoint.z > pEntity->pev->absmax.z ||
		 vPoint.x < pEntity->pev->absmin.x ||
		 vPoint.y < pEntity->pev->absmin.y ||
		 vPoint.z < pEntity->pev->absmin.z )
		 return FALSE;
	return TRUE;
}
float absf( float num ) {
	if( num < 0 ) return -num;
	return num;
}
/*int power( int exponent, int basenum ) {
	int i, ireturn = 1;
	for( i = 0; i < exponent; i++ ) ireturn *= basenum;
	return ireturn;
}*/
int numofdigits( int x ) {
	int idigits = 1;
	while( x >= pow(10,idigits) && idigits < 256) idigits++;
	return idigits;
}
void SpriteEffect( CBaseEntity *pEntity, int Effect, char *cSprite ) {
	int iSprite = PRECACHE_MODEL( cSprite );
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );

		WRITE_BYTE( Effect );
		WRITE_SHORT(pEntity->entindex());	// entity
		WRITE_SHORT( iSprite );	// sprite model
		WRITE_BYTE( 20 ); // life
		WRITE_BYTE( 5 );  // width
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 224 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness

	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)
}


//Needed to support sound.cpp
#include "talkmonster.h"
float	CTalkMonster::g_talkWaitTime = 0;		// time delay until it's ok to speak: used so that two NPCs don't talk at once
