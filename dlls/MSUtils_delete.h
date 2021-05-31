BOOL UTIL_IsPointWithinEntity( Vector &vPoint, CBaseEntity *pEntity );
float absf( float num );
void SpriteEffect( CBaseEntity *pEntity, int Effect, char *cSprite );
void BeamEffect( Vector vStart, Vector vEnd, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed );
void BeamEffect( float SRCx, float SRCy, float SRCz, float DESTx,
				float DESTy, float DESTz, int sprite, int startframe,
				int framerate, int life, int width, int noise,
				int r, int g, int b, int brightness, int ispeed );
int numofdigits( int x );
Vector GetHighBone( entvars_t *pev, int Bone );
CBaseEntity *MSInstance( edict_t *pent );
CBaseEntity *MSInstance( entvars_t *pev );
void *MSCopyClassMemory( void *pDest, void *pSource, size_t Length );
void *MSZeroClassMemory( void *pDest, size_t Length );

#define MACRO_CREATEITEM( item ) (CBaseEntity *)GET_PRIVATE(CREATE_NAMED_ENTITY(MAKE_STRING(item)))
#define foreach( var, max ) for( int var = 0; var < (signed)max; var++ )	//variable declared
#define Foreach( var, max ) for( var = 0; var < (signed)max; var++ )		//variable not declared
#define clrmem( a ) memset( &a, 0, sizeof(a) );