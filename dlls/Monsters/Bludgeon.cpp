#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "../MSItemDefs.h"
#include "../Weapons/Weapons.h"

#include "../Items/Shields/shieldglobal.h"
#include "MSmonster.h"
#include "Script.h"
#include "../MSUtils.h"

extern int iBeam; //Global cuz this can be shared across players

#define INTERVAL_ATTACK_LOW	.7
#define INTERVAL_ATTACK_HIGH	1.2
#define INTERVAL_WALK		1
#define INTERVAL_SWITCHTARGET1	4.0 //Target is found
#define INTERVAL_SWITCHTARGET2	3.0 //No target found
#define INTERVAL_LOOK	5

class CBludgeon : public CMSMonster {
public:
	EHANDLE m_LastEnemy;
	float LookTime,		//Time until you next check around for enemies
		FindTargetTime, //Time until you next check for the best enemy to fight
		MoveTime,		//Time until you update your movement path
		LostEnemyTime,	//Time until you forget about an enemy that's gone
		AttackTime,		//Time until you can next attack
		AttackLandTime,	//Time until your attack actually tracelines and hits
//		TurnRate,		//Rate of turning (unused)
		SequenceTime;	//Time that this sequence ends

	BOOL bAttacking,
		bCanChangeVel;  //FALSE if velocity is temporarily being skewed (like jumping)

	Vector AttackAngle;

	void Attack( );		//Initiate the attack
	void AttackLand( ); //Land the attack later
	void FinishAttack( ); //Finish the attack when the sequence is finished
	void StopWalking( );
	void Growl( );
	void DeathSound( );

//	Overridden Functions
	void Spawn( );
	void Precache( );
	void Think( );
	int BloodColor() { return BLOOD_COLOR_RED; }
	int Classify( ) { return CLASS_MONSTER; }	
	int IRelationship ( CBaseEntity *pTarget );
	BOOL HasHumanGibs( void ) { return TRUE; }
	float WalkSpeed( );
	float RunSpeed( );
	void Move( );		  //Walk or run around idly or towards the enemy
	int	TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	float BaseDamage( );
	void HandleAnimEvent( MonsterEvent_t *pEvent );
	void Jump( );
};

//LINK_ENTITY_TO_CLASS( monster_bludgeon, CBludgeon );

/*enum { //Bludgeon Animations
};*/

#define BLUDGEON_MODEL "models/monsters/bludgeon.mdl"
#define BLUDGEON_GROWL1 "bullchicken/bc_attackgrowl.wav"
#define BLUDGEON_GROWL2 "bullchicken/bc_attackgrowl2.wav"
#define BLUDGEON_GROWL3 "bullchicken/bc_attackgrowl3.wav"
#define BLUDGEON_DIE1 "bullchicken/bc_die1.wav"
#define BLUDGEON_DIE2 "bullchicken/bc_pain3.wav"
#define BLUDGEON_HITWORLD1 "debris/wood2.wav"
#define BLUDGEON_HITWORLD2 "debris/wood3.wav"
#define BLUDGEON_HITMONSTER1 "weapons/cbar_hitbod1.wav"
#define BLUDGEON_HITMONSTER2 "weapons/cbar_hitbod2.wav"
#define BLUDGEON_HITMONSTER3 "weapons/cbar_hitbod3.wav"
#define BLUDGEON_SWING1		 "weapons/cbar_miss1.wav"

 
void CBludgeon::Spawn( ) {
	Precache( );
	pev->model = MAKE_STRING( BLUDGEON_MODEL );
	SET_MODEL( edict(), STRING(pev->model) );
	
	pev->movetype = MOVETYPE_STEP;//MOVETYPE_FLY
	pev->solid = SOLID_BBOX;
	pev->classname = MAKE_STRING("bludgeon");
	pev->gravity = 4;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 72));

	pev->flags			|= FL_MONSTER|FL_MONSTERCLIP;
	pev->takedamage		= DAMAGE_AIM;
	pev->max_health		= 30; //200
	pev->health			= pev->max_health;
	pev->deadflag		= DEAD_NO;
	
	Gold = 50;
	m_hEnemy = NULL;
	m_LastEnemy = NULL;
	m_EnemyListNum = 0;
	bCanChangeVel = TRUE;
	bAttacking = FALSE;
	AttackTime = 0; AttackLandTime = 0;
	NodeCancelTime = 0; NextNodeTime = 0;
	StepSize = 24;
	JumpHeight = 64;
	WalkProximity = (pev->maxs - pev->mins).Length2D();
	RunProximity = WalkProximity + 25;
	AttackProximity = WalkProximity + 32;//48
//	TurnRate = .8;//.7
	m_flFieldOfView = .5235; // +/-30 degrees in radians
	pev->view_ofs = Vector( 0, 0, 72 );
	StopWalking( );
	SetAnimation( );
//	pev->sequence = LookupActivity( ACT_IDLE );
//	ResetSequenceInfo( );

	pev->nextthink = gpGlobals->time + 0.1;
	CMSMonster::Spawn( );
}
void CBludgeon::Precache( ) {
	PRECACHE_MODEL( BLUDGEON_MODEL );
	PRECACHE_SOUND( BLUDGEON_GROWL1 );
	PRECACHE_SOUND( BLUDGEON_GROWL2 );
	PRECACHE_SOUND( BLUDGEON_GROWL3 );
	PRECACHE_SOUND( BLUDGEON_DIE1 );
	PRECACHE_SOUND( BLUDGEON_DIE2 );
	PRECACHE_SOUND( BLUDGEON_HITWORLD1 );
	PRECACHE_SOUND( BLUDGEON_HITWORLD2 );
	PRECACHE_SOUND( BLUDGEON_HITMONSTER1 );
	PRECACHE_SOUND( BLUDGEON_HITMONSTER2 );
	PRECACHE_SOUND( BLUDGEON_SWING1 );
}
void CBludgeon::Think( ) {

	if( m_pfnThink ) { (this->*m_pfnThink)(); return; }
	if( !IsAlive() ) return;
	
	int OldActivity = m_Activity;

	StudioFrameAdvance( );
	if( pev->health < 0 ) return;

	if( m_hEnemy == NULL || !m_hEnemy->IsAlive() || m_hEnemy->pev->flags&FL_NOTARGET ||gpGlobals->time > FindTargetTime ) {
		EHANDLE ehPrevEnemy = m_hEnemy;
		// find the current best target... this may change in the middle of combat
		m_hEnemy = BestVisibleEnemy( );
		if( m_hEnemy != NULL ) {
//			ALERT( at_console, "m_hEnemy != NULL\n" );
			if( ehPrevEnemy != NULL && ehPrevEnemy.Get() != m_hEnemy.Get() ) m_LastEnemy = ehPrevEnemy;
			//If this is a new target, growl
			if( ehPrevEnemy == NULL || ehPrevEnemy.Get() != m_hEnemy.Get() ) Growl( );
			MoveTime = 0;
			FindTargetTime = gpGlobals->time + INTERVAL_SWITCHTARGET1;
		}
		else if( m_LastEnemy != NULL ) {
//			ALERT( at_console, "m_LastEnemy\n" );
			if( m_LastEnemy->IsAlive() && FVisible(m_LastEnemy) ) m_hEnemy = m_LastEnemy;
			FindTargetTime = gpGlobals->time + INTERVAL_SWITCHTARGET1;
		}
		else if( ehPrevEnemy != NULL ) {
//			ALERT( at_console, "ehPrevEnemy\n" );
			if( ehPrevEnemy->IsAlive() ) m_hEnemy = ehPrevEnemy;
			FindTargetTime = gpGlobals->time + INTERVAL_SWITCHTARGET1;
		}
		if( m_hEnemy == NULL ) { //m_hEnemy is still invalid
//			ALERT( at_console, "m_hEnemy == NULL\n" );
			//StopWalking( );
			FindTargetTime = gpGlobals->time + INTERVAL_SWITCHTARGET2;
		}
	}
	if( gpGlobals->time >= LookTime && (m_hEnemy == NULL || !m_hEnemy->IsAlive()) ) {
		Look( 1024 ); //was 512
		// Look around me for targets if I don't have one
//		if( m_hEnemy ) { if( m_hEnemy->IsAlive() ) 
			LookTime = gpGlobals->time + INTERVAL_LOOK;
//		else LookTime = gpGlobals->time + 3.0;
	}
//	ALERT( at_console, "Done.\n" );
	if( gpGlobals->time >= AttackTime ) Attack( );
	if( gpGlobals->time >= AttackLandTime ) AttackLand( );
	if( bAttacking ) FinishAttack( );
	if( gpGlobals->time >= MoveTime ) Move( );

	//Some Movement rules that always take place 
	//[gravity]
//	if( !FBitSet(pev->flags,FL_ONGROUND) ) {
/*		TraceResult tr;
		UTIL_TraceLine(pev->origin, pev->origin + Vector(0,0,-2), dont_ignore_monsters, dont_ignore_glass, edict(), &tr);
		if( tr.flFraction >= 1.0 ) pev->movetype = MOVETYPE_STEP;
		else pev->movetype = MOVETYPE_FLY;*/
//	}
//	else pev->movetype = MOVETYPE_FLY;

	//pev->angles = UTIL_VecToAngles( pev->velocity );

	Step(); //Walk up steps and the such
	Jump(); //Jump if neccesary

	//AtferMath
	//[Animation to play]
	SetAnimation( );
	DispatchAnimEvents( );

	pev->angles.x = 0;
	pev->angles.z = 0;
	if( m_flGroundSpeed ) {
		//if( m_flGroundSpeed < 0 ) m_flGroundSpeed = -m_flGroundSpeed;
		if( bCanChangeVel ) {
			UTIL_MakeVectors( pev->angles );
			pev->velocity = gpGlobals->v_forward * m_flGroundSpeed * SpeedMultiplier;
		}
	}
	//if( FBitSet(pev->flags,FL_ONGROUND) ) pev->velocity.z = 0;

	CMSMonster::Think( );
	pev->nextthink = gpGlobals->time + 0.1;
	//UTIL_BloodDrips( pev->origin, g_vecZero, BloodColor(), 80 );
}
float CBludgeon::WalkSpeed( ) { return 1.5; }
float CBludgeon::RunSpeed( ) { return 3; }
int CBludgeon::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) {
	if( RANDOM_LONG(0,1) ) Growl( );
	return CMSMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}
float CBludgeon::BaseDamage( ) { return RANDOM_LONG(1,3); }
void CBludgeon::Attack( ) {
	if( AttackLandTime || m_Activity == ACT_HOP  ) return;
	if ( m_hEnemy == NULL || !m_hEnemy->IsAlive() ) return;
	Vector m_vecTarget = m_hEnemy->EyePosition() - EyePosition();
	//Are we close enough?				
	if( m_vecTarget.Length() > AttackProximity ) return;
	StopWalking( );
	pev->angles = pev->v_angle = UTIL_VecToAngles(m_vecTarget);
	pev->angles.z = 0;

	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, BLUDGEON_SWING1, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 65 + RANDOM_FLOAT(0, 40) );

	m_Activity = ACT_MELEE_ATTACK1;
	pev->sequence = 0; //Causes it to play this animation NOW no matter what

	SequenceTime = 256/(m_flFrameRate * pev->framerate);  //Relative time

	bAttacking = TRUE;
	AttackLandTime = gpGlobals->time + .1;
	AttackTime = AttackLandTime + RANDOM_FLOAT(INTERVAL_ATTACK_LOW,INTERVAL_ATTACK_HIGH);
	SequenceTime += gpGlobals->time; //Absolute time
}
void CBludgeon::AttackLand( ) {
	if( !AttackLandTime ) return;
	
	TraceResult tr;
	Vector vecSrc, vecEnd;
	switch( m_Activity ) {
		case ACT_MELEE_ATTACK1:
			Vector vRand = Vector(RANDOM_FLOAT(-15,15),RANDOM_FLOAT(-15,15),RANDOM_FLOAT(-15,15));
			UTIL_MakeVectors (pev->angles);
			vecSrc = pev->origin + gpGlobals->v_forward*10 + pev->view_ofs - Vector(0,0,30);
			pev->punchangle = g_vecZero;
			UTIL_MakeVectors (pev->v_angle+pev->punchangle+vRand);
			vecEnd = vecSrc + gpGlobals->v_forward*60;
	}

	G_MakeSHIELDs( );
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, edict(), &tr );
	G_RemoveSHIELDs( );

	if ( tr.flFraction < 1.0 ) {
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
		if( pEntity ) {
			int iTemp;
			if (pEntity->Classify() == CLASS_NONE || pEntity->Classify() == CLASS_MACHINE) {
				//Hit the world
				if( (iTemp = RANDOM_LONG(0,1)) == 0 )
					EMIT_SOUND_DYN( edict(), CHAN_WEAPON, BLUDGEON_HITWORLD1, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 55 + RANDOM_FLOAT(0, 40) );
				else if( iTemp == 1 )
					EMIT_SOUND_DYN( edict(), CHAN_WEAPON, BLUDGEON_HITWORLD2, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 55 + RANDOM_FLOAT(0, 40) );				
				if( pEntity->MSProperties()&ITEM_SHIELD )				
					pEntity->DamageDecal( BaseDamage() );
			}
			else { //Hit something organic
				if( (iTemp = RANDOM_LONG(0,1)) == 0 )
					EMIT_SOUND_DYN( edict(), CHAN_WEAPON, BLUDGEON_HITMONSTER1, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 55 + RANDOM_FLOAT(0, 40) );
				else if( iTemp == 1 )
					EMIT_SOUND_DYN( edict(), CHAN_WEAPON, BLUDGEON_HITMONSTER2, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 55 + RANDOM_FLOAT(0, 40) );				
				//ALERT( at_console, "Hit: %s.\n", STRING(pEntity->pev->classname) );
			}

			ClearMultiDamage( );
			pEntity->TraceAttack(pev, pev, BaseDamage(), gpGlobals->v_forward, &tr, DMG_CLUB ); 
			ApplyMultiDamage( pev, pev );		
			float flMult = BaseDamage() * 10;
			pEntity->pev->punchangle.x += RANDOM_FLOAT(-1,1) * flMult;
			pEntity->pev->punchangle.y += RANDOM_FLOAT(-1,1) * flMult;
			pEntity->pev->punchangle.z += RANDOM_FLOAT(-1,1) * flMult;
		}
	}
	AttackLandTime = 0;
//	BeamEffect( vecSrc, vecEnd, iBeam, 0, 0, 100, 1, 0, 255, 255,255, 255, 20 );
}
void CBludgeon::FinishAttack( ) {
	if( !bAttacking ) return;
	if( !AttackLandTime && gpGlobals->time > SequenceTime ) {
		bAttacking = FALSE;
	}
}
void CBludgeon::Move( ) {
	if( bAttacking || m_Activity == ACT_HOP) return;
	bCanChangeVel = TRUE;
	Vector m_vecTarget, ExactAngle; //ExactAngle: The vector m_vecTarget as an angle
	Activity OldActivity = m_Activity;
	TraceResult tr;
	Vector VecDest;
	BOOL bCloseEnough;  //Close enough to our target to stop walking

	if ( m_hEnemy != NULL && m_hEnemy->IsAlive() ) {
		//Track the enemy
		bCloseEnough = FALSE;
		if (FVisible( m_hEnemy )) {
			m_vecTarget = m_hEnemy->EyePosition() - EyePosition();
			//ALERT( at_console, "Length: %f, Close: %f\n", m_vecTarget.Length2D(), Proximity );
			m_vecEnemyLKP = m_hEnemy->EyePosition();
		}
		else if( m_vecEnemyLKP ) m_vecTarget = m_vecEnemyLKP - EyePosition();
		
		//Are we close enough?		
		if( m_vecTarget.Length2D() < WalkProximity ) {
			pev->angles.y = UTIL_VecToAngles(m_vecTarget.Normalize( )).y;
			StopWalking( );
			bCloseEnough = TRUE;
			return;
		}
		else if( m_vecTarget.Length2D() < RunProximity ) {
			m_Activity = ACT_WALK;
			SpeedMultiplier = WalkSpeed();
		}
		else { m_Activity = ACT_RUN; SpeedMultiplier = RunSpeed(); }

		//This must be done AFTER we check m_vecTarget's length
		m_vecTarget = m_vecTarget.Normalize( );

/*		UTIL_MakeVectors( pev->angles );
		//UNDONE --> only turn a certain amount each frame (TurnRate in radians)
		if( DotProduct(gpGlobals->v_right, m_vecTarget) > 
			DotProduct(-gpGlobals->v_right, m_vecTarget) ) flTemp = 1;
		else flTemp = -1;

		flRemainingTurn = 1-DotProduct(gpGlobals->v_forward.Make2D(), m_vecTarget.Make2D());
*///		pev->avelocity = Vector( 0, 180 * (flRemainingTurn*1) * -flTemp, 0 );
//		ALERT( at_console, "1: %f, 2: %f\n", 1-DotProduct(gpGlobals->v_forward.Make2D(), m_vecTarget.Make2D()), TurnRate );
		ExactAngle = UTIL_VecToAngles(m_vecTarget);
		//Get an angle in the middle
		pev->angles.y = pev->angles.y + UTIL_AngleDiff(ExactAngle.y, pev->angles.y)/3;
	}
	else CMSMonster::Move( ); //No enemy, walk around randomly (here m_vecEnemyLKP is the random place to walk)

	MoveTime = gpGlobals->time + INTERVAL_WALK/2;
}
void CBludgeon::HandleAnimEvent( MonsterEvent_t *pEvent ) {
	LastEvent = *pEvent;
	float z;
//	ALERT( at_console, "event %i\n", LastEvent.event );
	switch( LastEvent.event ) {
		case 400:  //Animation Event 400 - Jump
			UTIL_MakeVectors( pev->angles );
			pev->velocity = gpGlobals->v_forward * 250;
			pev->velocity.z = 450;
			break;
		case 401:  //Animation Event 401 - Move forward in Jump
			z = pev->velocity.z;
			UTIL_MakeVectors( pev->angles );
			pev->velocity = gpGlobals->v_forward * 250;
			pev->velocity.z = z;
			break;
		case 500:  //Animation Event 500 - Stop forward/backward movement
			bCanChangeVel = FALSE;
			pev->velocity.x = 0;
			pev->velocity.y = 0;
			break;
		default:
			CBaseAnimating::HandleAnimEvent( pEvent );
			break;
	}
}
void CBludgeon::StopWalking( ) {
	switch( m_Activity ) {
		case ACT_WALK:
			m_Activity = ACT_IDLE;
			break;
	}
	pev->velocity = g_vecZero;
	m_flGroundSpeed = 0;
	SpeedMultiplier = 0;
}
void CBludgeon::Jump( ) {
	Activity OldActivity = m_Activity;
	if( m_Activity != ACT_HOP && m_hEnemy != NULL ) {
		Vector m_vecTarget = m_hEnemy->EyePosition() - EyePosition();
		//Are we too close?				
		if( m_vecTarget.Length() <= AttackProximity ) return;
	}

	CMSMonster::Jump( );

	if( m_Activity == ACT_HOP && OldActivity != m_Activity) {
		UTIL_MakeVectors( pev->angles );
		bCanChangeVel = FALSE;
		//pev->velocity = -gpGlobals->v_forward * 40;
	}
}
int CBludgeon::IRelationship ( CBaseEntity *pTarget )
{
	if( pTarget->pev->classname == pev->classname ) return R_AL;
	return CMSMonster::IRelationship( pTarget );
}
void CBludgeon::Growl( ) {
	if( !IsAlive() ) return;
	int iTemp;

	if( (iTemp = RANDOM_LONG(0,3)) == 0 )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, BLUDGEON_GROWL1, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 85 + RANDOM_FLOAT(0, 20) );
	else if( iTemp == 1 )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, BLUDGEON_GROWL2, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 85 + RANDOM_FLOAT(0, 20) );
	else if( iTemp == 2 )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, BLUDGEON_GROWL3, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 85 + RANDOM_FLOAT(0, 20) );
}
void CBludgeon::DeathSound( ) {
	int iTemp;

	if( (iTemp = RANDOM_LONG(0,1)) == 0 )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, BLUDGEON_DIE1, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 85 + RANDOM_FLOAT(0, 20) );
	else if( iTemp == 1 )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, BLUDGEON_DIE2, RANDOM_FLOAT(0.85, 1.0), ATTN_NORM, 0, 85 + RANDOM_FLOAT(0, 20) );				
}
