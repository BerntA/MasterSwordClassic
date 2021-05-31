#include "statdefs.h"
#include "Stats.h"
#include "Classes.h"
#include "../Global/MSGlobals.h"

//Classes
CClass g_Warrior, g_Ranger, g_Wizard, g_Cleric, g_Rogue;

CClass::CClass( ) {
	NaturalMulti = new(CStat[NATURAL_MAX_STATS]);
	SkillMulti = new(unsigned char[SKILL_MAX_STATS]);
}
CClass::~CClass( ) {
	delete NaturalMulti;
	delete SkillMulti;
}
#define SetNatStat( a, b, c ) pClass->NaturalStats[a] = b; \
							  pClass->NaturalMulti[a] = c
#define SetSkillStat( a, b, c ) pClass->SkillStats[a] = b; \
							  pClass->SkillMulti[a] = c
#define SetSkillStat2( a, b, c ) pClass->SkillStats[a].StatValue[0] = b; \
							  pClass->SkillMulti[a] = c
void InitializeGlobalClasses( ) {
	CClass *pClass;
	//Class stats should be crafted to ADD ON to the race stats
	//They should thus be relatively low
	//80 Points for each class!! -- UNDONE

	//Warrior
	pClass = &g_Warrior;
	pClass->id = CLASS_WARRIOR;
	pClass->Name = "Warrior";
	pClass->MaleModel = pClass->FemaleModel = "";

	SetNatStat( NATURAL_STR, 20, 100 );
	SetNatStat( NATURAL_DEX, 0, 30 );
	SetNatStat( NATURAL_INT, 0, 25 );
	SetNatStat( NATURAL_FIT, 10, 50 );
	SetNatStat( NATURAL_AWR, 0, 25 );

	//50-55 points for skill stats.  Keeps SkillAvg the same for all
	SetSkillStat( SKILL_SWORDSMANSHIP,	15, 100 );
	SetSkillStat( SKILL_SMALLARMS,		0,    0 );
	SetSkillStat( SKILL_AXEHANDLING,	15, 100 );
	SetSkillStat( SKILL_BLUNTARMS,		15, 100 );
	SetSkillStat( SKILL_ARCHERY,		0,    3 );
//	SetSkillStat( SKILL_SHIELDHANDLING, 10, 100 );
//	SetSkillStat( SKILL_TWOHANDED,		5,  100 );
//	SetSkillStat( SKILL_MULTIHANDED,	5,  100 );
	SetSkillStat2( SKILL_PARRY,		    10, 150 );
//	SetSkillStat2( SKILL_THEFT,			 0,   0 );

	//SetSkillStat2 == Stat that only uses one of the stat types (SKILL)

	//Ranger
	pClass = &g_Ranger;
	pClass->id = CLASS_RANGER;
	pClass->Name = "Ranger";
	pClass->MaleModel = pClass->FemaleModel = "";

	SetNatStat( NATURAL_STR, 5, 25 );
	SetNatStat( NATURAL_DEX, 15, 50 );
	SetNatStat( NATURAL_INT, 10, 50 );
	SetNatStat( NATURAL_FIT, 5, 50 );
	SetNatStat( NATURAL_AWR, 20, 25 );

	//50-55 points for skill stats.  Keeps SkillAvg the same for all
	SetSkillStat( SKILL_SWORDSMANSHIP,	5,   20 );
	SetSkillStat( SKILL_SMALLARMS,		20,  50 );
	SetSkillStat( SKILL_AXEHANDLING,	0,   10 );
	SetSkillStat( SKILL_BLUNTARMS,		0,   10 );
	SetSkillStat( SKILL_ARCHERY,		20,  75 );
//	SetSkillStat( SKILL_SHIELDHANDLING, 5,  100 );
//	SetSkillStat( SKILL_TWOHANDED,		0,  100 );
//	SetSkillStat( SKILL_MULTIHANDED,	10, 100 );
	SetSkillStat2( SKILL_PARRY,			5,   50 );
//	SetSkillStat2( SKILL_THEFT,			0,   20 );


	//Rogue
	pClass = &g_Rogue;
	pClass->id = CLASS_ROGUE;
	pClass->Name = "Rogue";
	pClass->MaleModel = pClass->FemaleModel = "";

	SetNatStat( NATURAL_STR, 5, 25 );
	SetNatStat( NATURAL_DEX, 20, 50 );
	SetNatStat( NATURAL_INT, 5, 100 );
	SetNatStat( NATURAL_FIT, 0, 50 );
	SetNatStat( NATURAL_AWR, 5, 25 );

	//50-55 points for skill stats.  Keeps SkillAvg the same for all
	SetSkillStat( SKILL_SWORDSMANSHIP,	0,  25 );
	SetSkillStat( SKILL_SMALLARMS,		25, 100 );
	SetSkillStat( SKILL_AXEHANDLING,	0,  10 );
	SetSkillStat( SKILL_BLUNTARMS,		0,  15 );
	SetSkillStat( SKILL_ARCHERY,		0,  25 );
//	SetSkillStat( SKILL_SHIELDHANDLING, 5,  50 );
//	SetSkillStat( SKILL_TWOHANDED,		0, 100 );
//	SetSkillStat( SKILL_MULTIHANDED,	5, 100 );
	SetSkillStat2( SKILL_PARRY,			10, 75 );
//	SetSkillStat2( SKILL_THEFT,			15,100 );

		//Wizard
/*	pClass = &g_Wizard;
	pClass->id = CLASS_WIZARD;
	pClass->Name = "Wizard";
	pClass->MaleModel = pClass->FemaleModel = "";

	pClass->NaturalStats[NATURAL_STR] = -3;
	pClass->NaturalStats[NATURAL_DEX] = 0;
	pClass->NaturalStats[NATURAL_INT] = 7;
	pClass->NaturalStats[NATURAL_WIS] = 5;
	pClass->NaturalStats[NATURAL_FIT] = 2;

	pClass->SkillStats[SKILL_SWORDSMANSHIP] = -4;
	pClass->SkillStats[SKILL_SMALLARMS] = 0;
	pClass->SkillStats[SKILL_AXEHANDLING] = -5;
	pClass->SkillStats[SKILL_ARCHERY] = -3;
	pClass->SkillStats[SKILL_SHIELDHANDLING] = -5;
	pClass->SkillStats[SKILL_TWOHANDED] = -5;
	pClass->SkillStats[SKILL_MULTIHANDED] = 0;
	pClass->SkillStats[SKILL_PARRY] = 10;
	pClass->SkillStats[SKILL_SPELLPREPARING] = 5;
	pClass->SkillStats[SKILL_SPELLCASTING] = 5;
	pClass->SkillStats[SKILL_SWIMMING] = 0;
	pClass->SkillStats[SKILL_THEIVERY] = 0;

	//Cleric
	pClass = &g_Cleric;
	pClass->id = CLASS_CLERIC;
	pClass->Name = "Cleric";
	pClass->MaleModel = pClass->FemaleModel = "";

	pClass->NaturalStats[NATURAL_STR] = -4;
	pClass->NaturalStats[NATURAL_DEX] = -4;
	pClass->NaturalStats[NATURAL_INT] = 6;
	pClass->NaturalStats[NATURAL_WIS] = 3;
	pClass->NaturalStats[NATURAL_FIT] = 5;

	pClass->SkillStats[SKILL_SWORDSMANSHIP] = -5;
	pClass->SkillStats[SKILL_SMALLARMS] = 5;
	pClass->SkillStats[SKILL_AXEHANDLING] = -5;
	pClass->SkillStats[SKILL_ARCHERY] = 5;
	pClass->SkillStats[SKILL_SHIELDHANDLING] = -5;
	pClass->SkillStats[SKILL_TWOHANDED] = -5;
	pClass->SkillStats[SKILL_MULTIHANDED] = 5;
	pClass->SkillStats[SKILL_PARRY] = 10;
	pClass->SkillStats[SKILL_SPELLPREPARING] = 5;
	pClass->SkillStats[SKILL_SPELLCASTING] = 3;
	pClass->SkillStats[SKILL_SWIMMING] = 0;
	pClass->SkillStats[SKILL_THEIVERY] = 3;*/
}

CClass *GetClassByID( int m_ID )
{
	switch( m_ID )
	{
	case CLASS_WARRIOR:
		return &g_Warrior;
	case CLASS_RANGER:
		return &g_Ranger;
	case CLASS_ROGUE:
		return &g_Rogue;
	}
	return 0;
}