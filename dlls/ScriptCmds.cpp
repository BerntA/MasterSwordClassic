/* 
	Master Sword - A Half-life Total Conversion
	Script.cpp - Parses & Executes script event files for NPCs or weapons 
*/

#include "inc_weapondefs.h"
#include "animation.h"
#include "soundent.h"
#include "Script.h"
#include "Stats/Races.h"
#include "Stats/statdefs.h"
#include "../MSShared/Titles.h"
#include "../MSShared/ScriptedEffects.h"
#include "logfile.h"
#include "pm_defs.h"

#ifndef VALVE_DLL
	#include "../cl_dll/MasterSword/CLRender.h"
	#include "../cl_dll/MasterSword/CLGlobal.h"
#else
	#include "SVGlobals.h"
	#include "../MSShared/Global.h"
#endif


#undef SCRIPTVAR
#define SCRIPTVAR GetVar								//A script-wide or global variable
#define ERROR_MISSING_PARMS MSErrorConsoleText( "ExecuteScriptCmd", UTIL_VarArgs("Script: %s, %s - not enough parameters!\n", m.ScriptFile.c_str(), Cmd.Name().c_str()) )

#define VecMultiply( a, b ) Vector( a[0] * b[0], a[1] * b[1], a[2] * b[2] ) 
void Player_UseStamina( float flAddAmt );
extern "C" playermove_t *pmove;

void CScript::Script_Setup( )
{
	if( !m_GlobalCmds.size() )
	{
		//m_GlobalCmds.add( scriptcmdname_t( "see", true ) );
		m_GlobalCmds.add( scriptcmdname_t( "if", true ) );		//The old if
		m_GlobalCmds.add( scriptcmdname_t( "if()", true ) );	//The new if
		m_GlobalCmds.add( scriptcmdname_t( "else", true ) );	//Really just part of if()

		m_GlobalCmds.add( scriptcmdname_t( "debugprint" ) );
		m_GlobalCmds.add( scriptcmdname_t( "dbg" ) );
		m_GlobalCmds.add( scriptcmdname_t( "name" ) );
		m_GlobalCmds.add( scriptcmdname_t( "name_prefix" ) );
		m_GlobalCmds.add( scriptcmdname_t( "name_unique" ) );
		m_GlobalCmds.add( scriptcmdname_t( "desc" ) );
		m_GlobalCmds.add( scriptcmdname_t( "weight" ) );
		m_GlobalCmds.add( scriptcmdname_t( "size" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setbbox" ) );
		m_GlobalCmds.add( scriptcmdname_t( "gravity" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setangle" ) );
		m_GlobalCmds.add( scriptcmdname_t( "movetype" ) );
		m_GlobalCmds.add( scriptcmdname_t( "callevent" ) );
		m_GlobalCmds.add( scriptcmdname_t( "callexternal" ) );
		m_GlobalCmds.add( scriptcmdname_t( "calleventtimed" ) );
		m_GlobalCmds.add( scriptcmdname_t( "calleventloop" ) );
		m_GlobalCmds.add( scriptcmdname_t( "incvar" ) );
		m_GlobalCmds.add( scriptcmdname_t( "decvar" ) );
		m_GlobalCmds.add( scriptcmdname_t( "inc" ) );
		m_GlobalCmds.add( scriptcmdname_t( "dec" ) );
		m_GlobalCmds.add( scriptcmdname_t( "add" ) );
		m_GlobalCmds.add( scriptcmdname_t( "subtract" ) );
		m_GlobalCmds.add( scriptcmdname_t( "multiply" ) );
		m_GlobalCmds.add( scriptcmdname_t( "divide" ) );
		m_GlobalCmds.add( scriptcmdname_t( "mod" ) );
		m_GlobalCmds.add( scriptcmdname_t( "vectoradd" ) );
		m_GlobalCmds.add( scriptcmdname_t( "vectormultiply" ) );
		m_GlobalCmds.add( scriptcmdname_t( "vectorscale" ) );
		m_GlobalCmds.add( scriptcmdname_t( "vectorset" ) );
		m_GlobalCmds.add( scriptcmdname_t( "stradd" ) );
		m_GlobalCmds.add( scriptcmdname_t( "capvar" ) );
		m_GlobalCmds.add( scriptcmdname_t( "volume" ) );
		m_GlobalCmds.add( scriptcmdname_t( "usetrigger" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setmodelbody" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setmodelskin" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setmodel" ) );
		m_GlobalCmds.add( scriptcmdname_t( "playanim" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setsolid" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setalive" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setprop" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setfollow" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setvar" ) );		//Exectuted at loadtime and runtime
		m_GlobalCmds.add( scriptcmdname_t( "setvarg" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setvard" ) );		//Exectuted at runtime only
		m_GlobalCmds.add( scriptcmdname_t( "const" ) );
		m_GlobalCmds.add( scriptcmdname_t( "local" ) );
		m_GlobalCmds.add( scriptcmdname_t( "token.add" ) );
		m_GlobalCmds.add( scriptcmdname_t( "token.del" ) );
		m_GlobalCmds.add( scriptcmdname_t( "repeatdelay" ) );
		m_GlobalCmds.add( scriptcmdname_t( "givehp" ) );
		m_GlobalCmds.add( scriptcmdname_t( "givemp" ) );
		m_GlobalCmds.add( scriptcmdname_t( "createnpc" ) );
		m_GlobalCmds.add( scriptcmdname_t( "createitem" ) );
		m_GlobalCmds.add( scriptcmdname_t( "registerrace" ) );
		m_GlobalCmds.add( scriptcmdname_t( "registertitle" ) );
		m_GlobalCmds.add( scriptcmdname_t( "registerdefaults" ) );
		m_GlobalCmds.add( scriptcmdname_t( "registereffect" ) );
		m_GlobalCmds.add( scriptcmdname_t( "registertexture" ) );
		m_GlobalCmds.add( scriptcmdname_t( "deleteme" ) );
		m_GlobalCmds.add( scriptcmdname_t( "deleteent" ) );
		m_GlobalCmds.add( scriptcmdname_t( "expiretime" ) );
		m_GlobalCmds.add( scriptcmdname_t( "effect" ) );
		m_GlobalCmds.add( scriptcmdname_t( "cleffect" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setvelocity" ) );
		m_GlobalCmds.add( scriptcmdname_t( "addvelocity" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setorigin" ) );
		m_GlobalCmds.add( scriptcmdname_t( "addorigin" ) );
		m_GlobalCmds.add( scriptcmdname_t( "clientevent" ) );
		m_GlobalCmds.add( scriptcmdname_t( "removescript" ) );
		m_GlobalCmds.add( scriptcmdname_t( "playsound" ) );
		m_GlobalCmds.add( scriptcmdname_t( "playrandomsound" ) );
		m_GlobalCmds.add( scriptcmdname_t( "sound.play3d" ) );
		m_GlobalCmds.add( scriptcmdname_t( "sound.pm_play" ) );
		m_GlobalCmds.add( scriptcmdname_t( "sound.setvolume" ) );
		m_GlobalCmds.add( scriptcmdname_t( "applyeffect" ) );
		m_GlobalCmds.add( scriptcmdname_t( "storeentity" ) );
		m_GlobalCmds.add( scriptcmdname_t( "precachefile" ) );
		m_GlobalCmds.add( scriptcmdname_t( "playermessage" ) );
		m_GlobalCmds.add( scriptcmdname_t( "consolemsg" ) );
		m_GlobalCmds.add( scriptcmdname_t( "infomsg" ) );
		m_GlobalCmds.add( scriptcmdname_t( "returndata" ) );
		m_GlobalCmds.add( scriptcmdname_t( "emitsound" ) );
		m_GlobalCmds.add( scriptcmdname_t( "companion" ) );
		m_GlobalCmds.add( scriptcmdname_t( "helptip" ) );
		m_GlobalCmds.add( scriptcmdname_t( "quest" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setcallback" ) );		
		m_GlobalCmds.add( scriptcmdname_t( "drop_to_floor" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setwearpos" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setgaitspeed" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setexpstat" ) );
		m_GlobalCmds.add( scriptcmdname_t( "drainstamina" ) );
		m_GlobalCmds.add( scriptcmdname_t( "setenv" ) );
		m_GlobalCmds.add( scriptcmdname_t( "respawn" ) );
		m_GlobalCmds.add( scriptcmdname_t( "npcmove" ) );
	}
}

//Param1 = Property name
//Param2 = Extra data
msstring_ref CBaseEntity::GetProp( CBaseEntity *pTarget, msstring &FullParams, msstringlist &Params )
{
	if( !pTarget ) pTarget = this;
	CGenericItem *pItem = pTarget->IsMSItem() ? (CGenericItem *)pTarget : NULL;
	CMSMonster *pMonster = pTarget->IsMSMonster() ? (CMSMonster *)pTarget : NULL;
	CBasePlayer *pPlayer = pTarget->IsPlayer() ? (CBasePlayer *)pTarget : NULL;
	IScripted *pScripted = pTarget->GetScripted();

	bool fSuccess = false;
	int Stat = -1;
	static msstring Return;
	msstring &Prop = FullParams;

	if( Prop == "name" )				return pTarget->DisplayName();
	else if( Prop == "id" )				return EntToString(pTarget);
	#ifdef VALVE_DLL
		//Client can't use entity.index.  Only player.index (handled later under player)
		else if( Prop == "index" ) RETURN_INT( pTarget->entindex() )
	#endif
	else if( Prop == "exists" )			fSuccess = true;
	else if( Prop == "alive" || Prop == "isalive" )			fSuccess = pTarget->IsAlive() ? true : false;
	else if( Prop == "hp" )				RETURN_FLOAT( pTarget->pev->health )
	else if( Prop == "gravity" )		RETURN_FLOAT( pTarget->pev->gravity )
	else if( Prop == "height" )			RETURN_FLOAT( pTarget->pev->maxs.z - pTarget->pev->mins.z )
	else if( Prop == "speed" )			RETURN_FLOAT( pTarget->pev->velocity.Length() )
	else if( Prop == "speed2D" )		RETURN_FLOAT( pTarget->pev->velocity.Length2D() )
	else if( Prop == "forwardspeed" )
	{
		Vector vForward;
		EngineFunc::MakeVectors( pTarget->pev->v_angle, vForward, NULL, NULL );
		RETURN_FLOAT( DotProduct(pTarget->pev->velocity, vForward) );
	}
	else if( Prop == "ducking" )		fSuccess = FBitSet(pTarget->pev->flags,FL_DUCKING) ? true : false;
	else if( Prop == "onground" )		fSuccess = FBitSet(pTarget->pev->flags,FL_ONGROUND) ? true : false;
	else if( Prop == "inwater" )		fSuccess = FBitSet(pTarget->pev->flags,FL_INWATER) ? true : false;
	else if( Prop == "invincible" )		fSuccess = FBitSet(pTarget->pev->flags,FL_GODMODE) ? true : false;
	else if( Prop == "waterlevel" )		RETURN_INT( pTarget->pev->waterlevel )
	else if( Prop == "anim.current_frame" )		RETURN_FLOAT( pTarget->pev->frame )
	else if( Prop == "anim.max_frames" )		RETURN_FLOAT( 255.0f )
	else if( Prop.starts_with("origin") )			RETURN_POSITION( "origin", pTarget->pev->origin )
	else if( Prop.starts_with("origin_center") )	RETURN_POSITION( "origin_center", pTarget->Center() )
	else if( Prop == "dist" || Prop == "dist2D" || Prop == "range" || Prop == "range2D" )
	{
		float Dist = (Prop == "range" || Prop == "dist" ) ? (pTarget->pev->origin - pev->origin).Length() : (pTarget->pev->origin - pev->origin).Length2D();
		RETURN_FLOAT( Dist )
	}
	else if( Prop.starts_with("eyepos") )	RETURN_POSITION( "eyepos", pTarget->EyePosition() )
	else if( Prop.starts_with("velocity") )	RETURN_POSITION( "velocity", pTarget->pev->velocity )
	else if( Prop.starts_with("angles") )
	{
		RETURN_ANGLE( "angles", pTarget->pev->angles )
		RETURN_POSITION( "angles", pTarget->pev->angles )
	}
	else if( Prop.starts_with("viewangles") )
	{
		RETURN_ANGLE( "viewangles", pTarget->pev->v_angle )
		RETURN_POSITION( "viewangles", pTarget->pev->v_angle )
	}
	else if( Prop == "target" )
	{
		CBaseEntity *pPlayerTarget = pTarget->RetrieveEntity( ENT_TARGET ) ;
		return pPlayerTarget ? EntToString(pPlayerTarget) : "0";
	}
	else if( Prop == "isplayer" ) fSuccess = pTarget->IsPlayer() ? true : false;

	else if( pScripted )
	{
		if( Prop == "scriptvar" ) return pScripted->GetFirstScriptVar( Params.size() >= 3 ? Params[2] : "" );
	
		if( pItem )
		{
			if( Prop == "name.full" )			return SPEECH::ItemName( pItem );
			else if( Prop == "name.full.capital" )	return SPEECH::ItemName( pItem, true );
			else if( Prop == "is_item" )			fSuccess = true;
			else if( Prop == "is_projectile" )		fSuccess = pItem->ProjectileData ? true : false;
			else if( Prop == "is_drinkable" )		fSuccess = pItem->DrinkData ? true : false;
			else if( Prop == "is_spell" )			fSuccess = pItem->SpellData ? true : false;
			else if( Prop == "attacking"  )			fSuccess = pItem->CurrentAttack ? true : false;
			else if( Prop == "inhand"  )			fSuccess = (pItem->m_Location == ITEMPOS_HANDS) ? true : false;
			else if( Prop == "is_worn"  )		fSuccess = pItem->IsWorn( ) ? true : false;
			else if( Prop == "hand_index"  ) RETURN_INT( pItem->m_Hand )
			else if( Prop == "owner"  )		 fSuccess = pItem->m_pOwner ? true : false;
			else if( Prop == "container.open" ) fSuccess = pItem->Container_IsOpen( ) ? true : false;
			else if( Prop == "container.items" ) RETURN_INT( pItem->Container_ItemCount() )
			#ifndef VALVE_DLL
				else if( Prop == "viewmodel"  )		return pItem->m_ViewModel;
				else if( Prop == "viewmodel.id" )  RETURN_INT( pItem->GetViewModelID() );
			#endif
		}
		else if( pMonster )
		{
			if( Prop == "name.full" ) return SPEECH::NPCName( pMonster );
			else if( Prop == "name.full.capital" )	return SPEECH::NPCName( pMonster, true );
			else if( Prop == "race"  )		return pMonster->m_Race;
			else if( Prop == "mp" )		RETURN_FLOAT( pMonster->m_MP )
			else if( Prop == "maxhp" )	RETURN_FLOAT( pMonster->MaxHP() )
			else if( Prop == "maxmp" )	RETURN_FLOAT( pMonster->MaxMP() )
			else if( Prop == "height" )	RETURN_FLOAT( pMonster->m_Height )
			else if( Prop == "movedest.origin" )	return VecToString( pMonster->m_MoveDest.Origin );
			else if( Prop == "movedest.prox" )		RETURN_FLOAT( pMonster->m_MoveDest.Proximity )
			else if( Prop == "moveprox" )			RETURN_FLOAT( pMonster->GetDefaultMoveProximity() )
			else if( Prop == "relationship" && Params.size() >= 3 )
			{
				CBaseEntity *pOtherEntity = RetrieveEntity( Params[2] );
				int RelationShip = pMonster->IRelationship(pOtherEntity);
				if( RelationShip == RELATIONSHIP_NO ) return "neutral";
				else if( RelationShip > RELATIONSHIP_NO ) return "ally";
				else if( RelationShip < RELATIONSHIP_NO ) return "enemy";
			}
			else if( pPlayer )
			{
				if( Prop == "gender" )	return (pPlayer->m_Gender == GENDER_MALE) ? "male" : "female";
				else if( Prop == "jumping" )	fSuccess = FBitSet( pPlayer->m_StatusFlags, PLAYER_MOVE_JUMPING ) ? true : false;
	#ifndef VALVE_DLL
				else if( Prop == "stamina" )		RETURN_FLOAT( pPlayer->Stamina )
				else if( Prop == "stamina.ratio" )	RETURN_FLOAT( pPlayer->Stamina/pPlayer->MaxStamina() )
	#endif
				else if( Prop == "stamina.max" )	RETURN_FLOAT( pPlayer->MaxStamina() )
				//else if( pPlayer && Prop == "sitting" )	fSuccess = FBitSet( pPlayer->m_StatusFlags, PLAYER_MOVE_SITTING ) ? true : false;
				else if( Prop == "anim.type" ) { if( pPlayer->m_pAnimHandler ) RETURN_INT( pPlayer->m_pAnimHandler->GetID() ) else fSuccess = false; }
				else if( Prop == "anim.uselegs" ) { if( pPlayer->m_pAnimHandler ) fSuccess = pPlayer->m_pAnimHandler->UseGait; }
				else if( Prop == "torso_anim" ) return pPlayer->m_szAnimTorso;
				else if( Prop == "legs_anim" ) return pPlayer->m_szAnimLegs;
				#ifdef VALVE_DLL
					else if( Prop == "in_attack_stance" ) fSuccess = pPlayer->IsInAttackStance();
				#endif
				else if( Prop == "currentitem" ) fSuccess = pPlayer->ActiveItem( ) ? true : false;
				else if( Prop == "currentitem.anim_torso" ) { if( pPlayer->ActiveItem( ) ) return pPlayer->ActiveItem()->m_AnimExt; else fSuccess = false; }
				else if( Prop == "currentitem.anim_legs" )  { if( pPlayer->ActiveItem( ) ) return pPlayer->ActiveItem()->m_AnimExtLegs; else fSuccess = false; }
				#ifdef VALVE_DLL
					else if( Prop.starts_with("skillavg") ) pPlayer->m_ScoreInfoCache.SkillLevel;  //Use the cached value for speed.  skillavg might get checked 
																									//quite a few times per frame.  Don't waste time re-computing with SkillAvg()
					else if( Prop.starts_with("title") ) return GetPlayerTitle( pPlayer->m_ScoreInfoCache.TitleIndex ); 
				#else
					else if( Prop.starts_with("skillavg") ) pPlayer->SkillAvg( );  //use the current one for the client
					else if( Prop.starts_with("title") ) return pPlayer->GetTitle( ); 
				#endif
				else if( Prop.starts_with("stat.") )
				{
					int Max = 100;

					if( Prop.contains(".max") ) RETURN_INT( Max )
					else
					{
						msstring Stat = Prop.substr( 5 ).thru_char(".");
						int iStat = GetNatStatByName( Stat );
						if( iStat > -1 )
						{
							int Amount = pPlayer->GetNatStat( iStat );

							if( Prop.contains(".ratio") ) RETURN_FLOAT_PRECISION( Amount / (float)Max )
							else RETURN_INT( Amount );
						}
						else ALERT( at_console, "Player stat %s doesn't exist!\n", Stat.c_str() );
					}
				}
				else if( Prop.starts_with("skill.") )
				{
					int SubSkill = -1;
					if( Prop.contains(".prof") )			SubSkill = 0;
					else if( Prop.contains(".balance") )	SubSkill = 1;
					else if( Prop.contains(".power") )		SubSkill = 2;

					else if( Prop.contains(".fire") )			SubSkill = 0;	//Magic
					else if( Prop.contains(".ice") )			SubSkill = 1;
					else if( Prop.contains(".lightning") )		SubSkill = 2;
					else if( Prop.contains(".summoning") )		SubSkill = 3;
					else if( Prop.contains(".protection") )		SubSkill = 4;
					else if( Prop.contains(".divination") )		SubSkill = 5;
					else if( Prop.contains(".affliction") )		SubSkill = 6;

					int Max = (SubSkill > -1) ? (int)STATPROP_MAX_VALUE : (int)STAT_MAX_VALUE;

					if( Prop.contains(".max") ) RETURN_INT( Max )
					else
					{
						msstring Skill = Prop.substr( 6 ).thru_char(".");
						int Stat = GetSkillStatByName( Skill );
						if( Stat > -1 )
						{
							int Amount;
							if( SubSkill > -1 ) Amount = pPlayer->GetSkillStat( Skill, SubSkill );
							else Amount = pPlayer->GetSkillStat( Stat );

							if( Prop.contains(".ratio") ) RETURN_FLOAT_PRECISION( Amount / (float)Max )
							else RETURN_INT( Amount );
						}
						else ALERT( at_console, "Player skill %s doesn't exist!\n", Skill.c_str() );
					}
				}
			}
		}
	}
	else return "¯NA¯";

	return fSuccess ? "1" : "0";
}

bool CScript::Script_ExecuteCmd( SCRIPT_EVENT &Event, scriptcmd_t &Cmd, msstringlist &Params ) 
{
	msstring sTemp;
	bool ConditionalCmd = Cmd.m_Conditional;
	bool ConditionsMet = false;

	bool fParentSuccess = false;
	if( m.pScriptedEnt )
		fParentSuccess = m.pScriptedInterface->Script_ExecuteCmd( this, Event, Cmd, Params );

	if( fParentSuccess )
		return fParentSuccess;

	//******************************* IF (cond) ***************************
	//Usage: if <varname> equals <value>
	//Usage: if <varname> isnot <value>
	if( Cmd.Name() == "var" ||
		Cmd.Name() == "if" || 
		Cmd.Name() == "if()" )
	{
		ConditionalCmd = true;
		if( Cmd.Params() == 1 )
		{
			msstring Value = Params[0];

			bool Opposite = false;
			if( Params[0][0] == '!' )
			{
				Opposite = true;
				Value = SCRIPTVAR( Params[0].substr( 1 ) );	//The '!' interferes with the default variable resolution, so remove it and resolve the variable again
			}
			
			ConditionsMet = atoi(Value) ? true : false;
			ConditionsMet = ConditionsMet ^ Opposite;
		}
		else if( Cmd.Params() >= 3 )
		{
			int iCompareType = 0;

			msstring &CompareParam = Params[1];

			if( CompareParam == "equals"	) iCompareType = 0;
			else if( CompareParam == "isnot" ) iCompareType = 1;
			else if( CompareParam == "<"	) iCompareType = 2;
			else if( CompareParam == ">"	) iCompareType = 3;
			else if( CompareParam == "<="	) iCompareType = 4;
			else if( CompareParam == ">="	) iCompareType = 5;
			else if( CompareParam == "=="	) iCompareType = 6;
			else if( CompareParam == "!="	) iCompareType = 7;

			msstring &CompareFromParam = Params[0], &CompareToParam = Params[2];
			if( iCompareType == 0 )
				ConditionsMet = FStrEq(CompareFromParam,CompareToParam)? true : false;
			else if( iCompareType == 1 )
				ConditionsMet = !FStrEq(CompareFromParam,CompareToParam) ? true : false;
			else
			{
				float flFromValue = GetNumeric(CompareFromParam);
				float flToValue = GetNumeric(CompareToParam);
				if( iCompareType == 2 ) 
					ConditionsMet = (flFromValue < flToValue);
				else if( iCompareType == 3 ) 
					ConditionsMet = (flFromValue > flToValue);
				else if( iCompareType == 4 ) 
					ConditionsMet = (flFromValue <= flToValue);
				else if( iCompareType == 5 ) 
					ConditionsMet = (flFromValue >= flToValue);
				else if( iCompareType == 6 ) 
					ConditionsMet = (flFromValue == flToValue);
				else if( iCompareType == 7 ) 
					ConditionsMet = (flFromValue != flToValue);
			}
		}
		else
		{
			ConditionalCmd = false;	//If I get a parameter error, just fall through like a normal command
			ERROR_MISSING_PARMS;
		}
	}


	//Commands
	//--------

	//******************************  SETVAR  *******************************
	else if( Cmd.Name() == "setvar" || Cmd.Name() == "setvarg" || Cmd.Name() == "local" ) 
	{
		if( Params.size() >= 2 )
		{
			msstring_ref VarName = Cmd.m_Params[1];
			msstring_ref VarValue = Params[1];
			if( Params.size() >= 3 )	//Add strings together
			{
				sTemp = "";
				 for (int i = 0; i < Params.size()-1; i++) 
					sTemp += Params[i+1];
				VarValue = sTemp;
			}
			if( Cmd.Name() == "local" )
				Event.SetLocal( VarName, VarValue );
			else if( Cmd.Name() == "setvarg" )
				SetVar( VarName, VarValue, true );
			else
				SetVar( VarName, VarValue, Event );
		}
		else ERROR_MISSING_PARMS;
	}
	//**************************** INC/DEC VAR ******************************
	else if( Cmd.Name() == "incvar" || Cmd.Name() == "decvar" ||
		     Cmd.Name() == "inc" || Cmd.Name() == "dec" ||
		     Cmd.Name() == "add" || Cmd.Name() == "subtract" ||
		     Cmd.Name() == "multiply" || Cmd.Name() == "divide" ||
			 Cmd.Name() == "mod" ) 
	{
		if( Params.size() >= 2 )
		{
			int Operation = 0;
			if( Cmd.Name() == "decvar" || Cmd.Name() == "dec" || Cmd.Name() == "subtract" ) Operation = 1;
			else if( Cmd.Name() == "multiply" ) Operation = 2;
			else if( Cmd.Name() == "divide" ) Operation = 3;
			else if( Cmd.Name() == "mod" ) Operation = 4;

			float flValue = 0;
			flValue = atof(Params[0]);

			float Amount = atof(Params[1]);
			if( !Operation ) flValue += Amount;
			else if( Operation == 1 ) flValue -= Amount;
			else if( Operation == 2 ) flValue *= Amount;
			else if( Operation == 3 && Amount ) flValue /= Amount;
			else if( Operation == 4 && Amount ) flValue = (int)flValue % (int)Amount;

			if( Params.size() <= 2 )
				//No precision specified, assume 2 digits
				SetVar( Cmd.m_Params[1], UTIL_VarArgs("%.2f",flValue), Event );
			else
				SetVar( Cmd.m_Params[1], UTIL_VarArgs("%f",flValue), Event );	//used to be %.2f, but I decided to keep the precision
		}
		else ERROR_MISSING_PARMS;
	}
	//********************************* VECTORADD ***************************
	else if( Cmd.Name() == "vectoradd" ) {	// <vec> <addvec>
		if( Params.size() >= 2 )			// <vec> <changevec> <addvec>
		{									// <vec> <x/y/z> <addvalue>
			Vector Result;
			if( Params[1] == "x" )		Result = StringToVec( Params[0] ) + Vector(atof(Params[2]),0,0);
			else if( Params[1] == "y" ) Result = StringToVec( Params[0] ) + Vector(0,atof(Params[2]),0);
			else if( Params[1] == "z" ) Result = StringToVec( Params[0] ) + Vector(0,0,atof(Params[2]));
			else
			{
				if( Params.size() < 3 ) Result = StringToVec( Params[0] ) + StringToVec( Params[1] );
				else Result = StringToVec( Params[1] ) + StringToVec( Params[2] );
			}
			
			SetVar( Cmd.m_Params[1], VecToString(Result), Event );
		}
		else ERROR_MISSING_PARMS;
	}
	//******************************* VECTORSCALE ***************************
	else if( Cmd.Name() == "vectormultiply" 	// <vec> <multvec>
		|| Cmd.Name() == "vectorscale" ) {		// <vec> <changevec> <multvec>
		if( Params.size() >= 2 )				// <vec> <x/y/z> <multvalue>
		{
			Vector Result;
			if( Params[1] == "x" )		Result = VecMultiply( StringToVec( Params[0] ), Vector(atof(Params[2]),0,0) );
			else if( Params[1] == "y" ) Result = VecMultiply( StringToVec( Params[0] ), Vector(0,atof(Params[2]),0) );
			else if( Params[1] == "z" ) Result = VecMultiply( StringToVec( Params[0] ), Vector(0,0,atof(Params[2])) );
			else
			{
				if( Params.size() < 3 ) 
				{
					if( isdigit(Params[1][0]) ) Result = StringToVec( Params[0] ) * atof( Params[1] );
					else Result = VecMultiply( StringToVec( Params[0] ), StringToVec( Params[1] ) );
				}
				else 
				{
					if( isdigit(Params[1][0]) ) Result = StringToVec( Params[1] ) * atof( Params[2] );
					else Result = VecMultiply( StringToVec( Params[1] ), StringToVec( Params[2] ) );
				}
			}
			
			SetVar( Cmd.m_Params[1], VecToString(Result), Event );
		}
		else ERROR_MISSING_PARMS;
	}
	//********************************* VECTORSET ***************************
	else if( Cmd.Name() == "vectorset" ) {	// <vec> <x/y/z> <newvalue>
		if( Params.size() >= 2 )
		{
			Vector ModifyVec = StringToVec( Params[0] );
			float *Prop = NULL;
			if( Params[1] == "x" )		Prop = &ModifyVec.x;
			else if( Params[1] == "y" ) Prop = &ModifyVec.y;
			else if( Params[1] == "z" ) Prop = &ModifyVec.z;
			else MSErrorConsoleText( "CGenericItem::ExecuteScriptCmd", UTIL_VarArgs("Script: %s, %s - '%s' not a valid vector coordinate!\n", m.ScriptFile.c_str(), Cmd.Name().c_str(), Params[1].c_str() ));

			if( Prop ) *Prop = atof(Params[2]);
			
			SetVar( Cmd.m_Params[1], VecToString(ModifyVec), Event );
		}
		else ERROR_MISSING_PARMS;
	}
	//********************************* STRADD *****************************
	else if( Cmd.Name() == "stradd" ) {
		if( Params.size() >= 3 ) SetVar( Cmd.m_Params[1], Params[1] + Params[2], Event );
		else if( Params.size() >= 2 ) SetVar( Cmd.m_Params[1], Params[0] + Params[1], Event );

		else ERROR_MISSING_PARMS;
	}
	//****************************** TOKEN.ADD/DEL *************************
	else if( Cmd.Name() == "token.add") {
		if( Params.size() >= 2 )
		{
			msstring &Token = Params[0];
			msstring &TokenAdd = Params[1];
			
			if( Token.len() )	Token += ";";

			Token += TokenAdd;
			SetVar( Cmd.m_Params[1], Token, Event );
		}
		else ERROR_MISSING_PARMS;
	}
	else if( Cmd.Name() == "token.del") {
		if( Params.size() >= 2 )
		{
			static msstringlist Tokens;
			Tokens.clearitems();
			int DelItem = atoi(Params[1]);
			msstring TokenStr;

			TokenizeString( Params[0], Tokens );

			if( DelItem >= 0 && DelItem < (signed)Tokens.size() )
			{
				Tokens.erase( DelItem );

				 for (int i = 0; i < Tokens.size(); i++) 
				{
					if( TokenStr.len() )	TokenStr += ";";
					TokenStr += Tokens[i];
				}

				SetVar( Cmd.m_Params[1], TokenStr, Event );
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//******************************** CAPVAR ******************************
	else if( Cmd.Name() == "capvar" ) {
		if( Params.size() >= 3 )
		{
			float flValue = atof(Params[0]);
			if( flValue < atof(Params[1]) )	SetVar( Cmd.m_Params[1], Params[1], Event );
			if( flValue > atof(Params[2]) )	SetVar( Cmd.m_Params[1], Params[2], Event );
		}
		else ERROR_MISSING_PARMS;
	}
	//********************************* DEBUGPRINT *************************
	else if( Cmd.Name() == "debugprint" || Cmd.Name() == "dbg" ) {
		 for (int i = 0; i < Params.size(); i++) 
			sTemp += (i ? msstring(" ") : msstring("")) + Params[i];
	
		msstring_ref LocationString = "Server";
#ifndef VALVE_DLL
		LocationString = "Client";
#endif
		Print( "* Script Debug (%s): %s - %s\n", LocationString, m.pScriptedEnt ? m.pScriptedEnt->DisplayName() : "(No Entity)", sTemp.c_str() );
	}
	//****************************** CALLEVENT *****************************
	else if( Cmd.Name() == "callevent" || Cmd.Name() == "calleventtimed" || Cmd.Name() == "callexternal" || Cmd.Name() == "calleventloop" ) {
		if( Params.size() >= 1 )
		{
			float Delay = 0;
			IScripted *pScripted = NULL;
			msstring_ref EventName = "<none>";
			size_t NextParm = 0;
			int Loops = 1;
			msstringlist Parameters;
			enum calleventype_e { CE_NORMAL, CE_EXTERNAL, CE_EXTERNAL_ALL, CE_LOOP } Type = CE_NORMAL;

			if( Cmd.Name() == "callexternal" )
			{
				if( Params[0] == "all" )
				{
					Type = CE_EXTERNAL_ALL;
				}
				else
				{
					CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity( Params[NextParm] );
					if( pEntity ) pScripted = pEntity->GetScripted();
					Type = CE_EXTERNAL;
				}
				NextParm++;
			}
			else if( Cmd.Name() == "calleventloop" )
			{
				Loops = atoi(Params[NextParm]);
				NextParm++;
				Type = CE_LOOP;
			}

			if( Params.size() == NextParm+1 )		//callevent [entity] <eventname>
				EventName = Params[NextParm];

			else if( Params.size() > NextParm+1 )
			{ 
				if( isdigit(Params[NextParm][0]) && Type != CE_EXTERNAL_ALL ) //CE_EXTERNAL_ALL doesn't allow timed calls
				{
					Delay = atof(Params[NextParm]);
					EventName = Params[(++NextParm)++]; //callevent [entity] <delay> <eventname> x x x
				}
				else EventName = Params[NextParm++];		//callevent [entity] <eventname> x x x

				if( Params.size() > NextParm )
				{ 
					foreach( i, (Params.size() - NextParm) )		//Parameters to pass
						Parameters.add( Params[i+NextParm] );
				}
				
			}

			if( Delay )
			{
				if( Type == CE_EXTERNAL ) { if( pScripted ) pScripted->CallScriptEventTimed( EventName, Delay ); }	//Need the braces for logic
				else
				{
					SCRIPT_EVENT *seEvent = EventByName( EventName );
					if( seEvent ) CallEventTimed( EventName, Delay );
					else MSErrorConsoleText( "ExecuteScriptCmd", UTIL_VarArgs("Script: %s, callevent (timed) - event %s NOT FOUND!\n", m.ScriptFile.c_str(), EventName) );
				}
			}
			else
			{
				if( Type == CE_EXTERNAL_ALL )
					CallScriptEventAll( EventName, Parameters.size() ? &Parameters : NULL );
				else if( Type == CE_EXTERNAL )
					{ if( pScripted ) pScripted->CallScriptEvent( EventName, Parameters.size() ? &Parameters : NULL ); }	//Need the braces for logic
				else if( strcmp(EventName,Event.Name) )	//Can't call myself recursively
				{
					int SaveIteration = m.m_Iteration;
					 for (int i = 0; i < Loops; i++) 
					{
						m.m_Iteration = i;
						RunScriptEventByName( EventName, Parameters.size() ? &Parameters : NULL );
					}
					m.m_Iteration = SaveIteration;
				}
			}


		}
		else ERROR_MISSING_PARMS;
	}
	//****************************** REPEATDELAY **************************
	else if( Cmd.Name() == "repeatdelay" ) {
		if( Params.size() >= 1 )
		{
			Event.fRepeatDelay = atof(Params[0]);
			Event.fNextExecutionTime = gpGlobals->time + Event.fRepeatDelay;
		}
		else ERROR_MISSING_PARMS;
	}
	//********************************* NAME ******************************
	else if( Cmd.Name() == "name" ) {
		if( Params.size() >= 1 )
		{
			if( m.pScriptedEnt && !m.pScriptedEnt->IsPlayer() )	//Don't rename players
			{
				 for (int i = 0; i < Params.size(); i++) 
					sTemp += (i ? msstring(" ") : msstring("")) + Params[i];

				int barloc = 0;
				msstring Prefix, Name = sTemp;
				if( (barloc = sTemp.find( "|" )) != msstring_error )
				{
					Prefix = sTemp.substr( 0, barloc );
					Name = sTemp.substr( barloc + 1 );

					m.pScriptedEnt->DisplayPrefix = GetScriptVar(Prefix);
				}

				if( !m.pScriptedEnt->pev->netname )
				{
					//m.pScriptedEnt->pev->netname = ALLOC_STRING( msstring("¯") + Name );
					m.pScriptedEnt->m_NetName = msstring("¯") + Name;
					m.pScriptedEnt->pev->netname = MAKE_STRING(m.pScriptedEnt->m_NetName.c_str());
				}
				m.pScriptedEnt->m_DisplayName = GetScriptVar(Name);
			}
		}
		else ERROR_MISSING_PARMS;
	}
	else if( Cmd.Name() == "name_prefix" ) {
		if( Params.size() >= 1 )
			{ if( m.pScriptedEnt ) m.pScriptedEnt->DisplayPrefix = Params[0]; }

		else ERROR_MISSING_PARMS;
	}
	else if( Cmd.Name() == "name_unique" ) {
		if( Params.size() >= 1 )
		{ 
			if( m.pScriptedEnt )
			{
				//m.pScriptedEnt->pev->netname = ALLOC_STRING( msstring("¯") + Params[0] );
						m.pScriptedEnt->m_NetName = msstring("¯") + Params[0];
						m.pScriptedEnt->pev->netname =  MAKE_STRING(m.pScriptedEnt->m_NetName.c_str());
			}
		}	//Need braces

		else ERROR_MISSING_PARMS;
	}
	//********************************* DESC *****************************
	else if( Cmd.Name() == "desc" ) 
	{
		if( Params.size() >= 1 )
		{
			 for (int i = 0; i < Params.size(); i++) 
				{ if( i ) sTemp += " ";  sTemp += Params[i]; }

			if( m.pScriptedEnt ) m.pScriptedEnt->DisplayDesc = sTemp;
		}
		else ERROR_MISSING_PARMS;
	}
	//******************************* SIZE *******************************
	else if( Cmd.Name() == "size" ) {
		if( Params.size() >= 1 )
			m.pScriptedEnt->m_Volume = atof(Params[0]);
		else ERROR_MISSING_PARMS;
	}
	//****************************** WEIGHT ******************************
	else if( Cmd.Name() == "weight" ) {
		if( Params.size() >= 1 )
			m.pScriptedEnt->m_Weight = atof(Params[0]);
		else ERROR_MISSING_PARMS;
	}
	//****************************** GRAVITY *****************************
	else if( Cmd.Name() == "gravity" ) {
		if( Params.size() >= 1 )
			m.pScriptedEnt->pev->gravity = max(atof(Params[0]),0.001f);
		else ERROR_MISSING_PARMS;
	}
	//****************************** SETBBOX *****************************
	else if( Cmd.Name() == "setbbox" ) {
		//"npcsize" - Uses width and height
		//[mins] [maxs] - Uses specified mins and maxs
		#ifdef VALVE_DLL
			if( Params.size() >= 1 )
			{
				if( m.pScriptedEnt )
					if( Params.size() >= 2 )
					{
						m.pScriptedEnt->pev->mins = StringToVec(Params[0]);
						m.pScriptedEnt->pev->maxs = StringToVec(Params[1]);
						UTIL_SetSize( m.pScriptedEnt->pev, m.pScriptedEnt->pev->mins, m.pScriptedEnt->pev->maxs );
					}
					else {
						CMSMonster *pMonster = m.pScriptedEnt->IsMSMonster() ? (CMSMonster *)m.pScriptedEnt : NULL;
						if( pMonster )
						{
							if( Params[0] == "animsize" )
							{
								if( pMonster->pev->model )
								{
									Vector v_Mins, v_Maxs;
									pMonster->ExtractBbox( 0, v_Mins, v_Maxs );
									UTIL_SetSize( pMonster->pev, v_Mins, v_Maxs );
								}
							}
							else if( Params[0] == "npcsize" )
								UTIL_SetSize( pMonster->pev, Vector(-pMonster->m_Width/2,-pMonster->m_Width/2,0), Vector(pMonster->m_Width/2,pMonster->m_Width/2,pMonster->m_Height) );
						}
					}
			}
			else ERROR_MISSING_PARMS;
		#endif
	}
	//****************************** SETSOLID ****************************
	else if( Cmd.Name() == "setsolid" ) 
	{
		CBaseEntity *pEntity = m.pScriptedEnt;
		msstring Setting = Params[0];
		if( Params.size() >= 2 )
		{
			pEntity = RetrieveEntity( Params[0] );
			Setting = Params[1];
		}

		if( Params.size() >= 1 )
		{
			if( pEntity )
			{
				if( Setting == "none" )			pEntity->pev->solid = SOLID_NOT;
				else if( Setting == "box" )		pEntity->pev->solid = SOLID_BBOX;
				else if( Setting == "slidebox" )	pEntity->pev->solid = SOLID_SLIDEBOX;
				else if( Setting == "trigger" )	pEntity->pev->solid = SOLID_TRIGGER;
				UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//****************************** SETALIVE ****************************
	else if( Cmd.Name() == "setalive" ) {
		if( Params.size() >= 1 )
		{
			if( m.pScriptedEnt )
				if( atoi(Params[0]) )
				{
					m.pScriptedEnt->pev->deadflag	= DEAD_NO;
					m.pScriptedEnt->pev->solid		= SOLID_BBOX;
					m.pScriptedEnt->pev->movetype	= MOVETYPE_STEP;
					UTIL_SetOrigin( m.pScriptedEnt->pev, m.pScriptedEnt->pev->origin );
					m.pScriptedEnt->SetThink( NULL );
				}
				else m.pScriptedEnt->pev->deadflag = DEAD_DEAD;
		}
		else ERROR_MISSING_PARMS;
	}
	//****************************** SETPROP ****************************
	else if( Cmd.Name() == "setprop" ) {
		if( Params.size() >= 3 )
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
			if( pEntity )
			{
				msstring &PropName = Params[1];
				msstring &PropValue = Params[2];
				int Int = atoi(PropValue);
				float Float = atof(PropValue);
				Vector vVector = StringToVec(PropValue);

				if( PropName == "speed" )			pEntity->pev->speed = Float;
				else if( PropName == "skin" )		pEntity->pev->skin = Int;
				else if( PropName == "modelindex" )	pEntity->pev->modelindex = Int;
				else if( PropName == "model" )		pEntity->pev->model = ALLOC_STRING(PropValue);
				else if( PropName == "movetype" )	pEntity->pev->movetype = Int;
				else if( PropName == "solid" )		pEntity->pev->solid = Int;
				else if( PropName == "frame" )		pEntity->pev->frame = Float;
				else if( PropName == "framerate" )	pEntity->pev->framerate = Float;
				else if( PropName == "classname" )	pEntity->pev->classname = ALLOC_STRING(PropValue);
				else if( PropName == "basevelocity" )	pEntity->pev->basevelocity = vVector;
				else if( PropName == "avelocity" )	pEntity->pev->avelocity = vVector;
				else if( PropName == "velocity" )	pEntity->pev->velocity = vVector;
				else if( PropName == "movedir" )	pEntity->pev->movedir = vVector;
				else if( PropName == "ltime" )		pEntity->pev->ltime = Float;
				else if( PropName == "nextthink" )	pEntity->pev->nextthink = Float;
				else if( PropName == "friction" )	pEntity->pev->friction = Float;
				else if( PropName == "frame" )	pEntity->pev->frame = Float;
				else if( PropName == "animtime" )	pEntity->pev->animtime = Float;
				else if( PropName == "sequence" )	pEntity->pev->sequence = Int;
				else if( PropName == "playerclass" )	pEntity->pev->playerclass = Int;
				else if( PropName == "target" )		pEntity->pev->target = ALLOC_STRING(PropValue);
				else if( PropName == "targetname" )	pEntity->pev->targetname = ALLOC_STRING(PropValue);
				else if( PropName == "netname" )	pEntity->pev->netname = ALLOC_STRING(PropValue);
				else if( PropName == "view_ofs" )	pEntity->pev->view_ofs = vVector;
				else if( PropName == "deadflag" )	pEntity->pev->deadflag = Int;
				else if( PropName == "health" )		pEntity->pev->health = Float;
				else if( PropName == "aiment" )
				{
					CBaseEntity *pEntity = StringToEnt(PropValue);
					if( pEntity )
						pEntity->pev->aiment = pEntity->edict();
				}
				else if( PropName == "controller0" ) pEntity->pev->controller[0] = Int;
				else if( PropName == "controller1" ) pEntity->pev->controller[1] = Int;
				else if( PropName == "controller2" ) pEntity->pev->controller[2] = Int;
				else if( PropName == "controller3" ) pEntity->pev->controller[3] = Int;
				else if( PropName == "blending0" ) pEntity->pev->blending[0] = Int;
				else if( PropName == "blending1" ) pEntity->pev->blending[1] = Int;
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//**************************** SETFOLLOW *****************************
	else if( Cmd.Name() == "setfollow" ) {
		if( Params.size() >= 1 )
		{
			if( m.pScriptedEnt )
			{
				if( Params[0] == "none" )
				{
					m.pScriptedEnt->pev->movetype = MOVETYPE_STEP;
					m.pScriptedEnt->SetFollow( NULL );
				}
				else if( Params.size() >= 2 )
				{
					CBaseEntity *pTarget = m.pScriptedEnt->RetrieveEntity( Params[0] );
					if( pTarget )
					{
						int FollowFlags = 0;
						if( Params[1].find("align_bottom") != msstring_error ) SetBits( FollowFlags, ENT_EFFECT_FOLLOW_ALIGN_BOTTOM );
						//if( Params[1].find("face_host") != msstring_error ) SetBits( FollowFlags, ENT_EFFECT_FOLLOW_FACE_HOST );

						m.pScriptedEnt->SetFollow( pTarget, FollowFlags );
						m.pScriptedEnt->pev->movetype = MOVETYPE_NONE;
					}
				}
				else ERROR_MISSING_PARMS;
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//******************************** SETANGLE **************************
	else if( Cmd.Name() == "setangle" ) {
		if( Params.size() >= 1 )
		{
			if( m.pScriptedEnt )
			{
				msstring AngToModify = Params[0];
				bool SetFace = false;

				if( AngToModify == "velocity" )
					m.pScriptedEnt->pev->angles = UTIL_VecToAngles(m.pScriptedEnt->pev->velocity);

				else if( Params.size() >= 2 )
				{
					if( (AngToModify == "add_view.x" || AngToModify == "add_view.pitch") )
						m.pScriptedEnt->pev->v_angle.x += -atof(Params[1]);
					else if( (AngToModify == "add_view.y" || AngToModify == "add_view.yaw") )
						m.pScriptedEnt->pev->v_angle.y += atof(Params[1]);
					else if( (AngToModify == "add_view.z" || AngToModify == "add_view.roll") )
						m.pScriptedEnt->pev->v_angle.z += atof(Params[1]);

					else if( (AngToModify == "view.x" || AngToModify == "view.pitch") )
						m.pScriptedEnt->pev->v_angle.x = -atof(Params[1]);
					else if( (AngToModify == "view.y" || AngToModify == "view.yaw") )
						m.pScriptedEnt->pev->v_angle.y = atof(Params[1]);
					else if( (AngToModify == "view.z" || AngToModify == "view.roll") )
						m.pScriptedEnt->pev->v_angle.z = atof(Params[1]);
					else if( AngToModify == "view" )
					{
						CBaseEntity *pTarget = m.pScriptedEnt->RetrieveEntity( Params[1] );
						if( pTarget ) m.pScriptedEnt->pev->v_angle = UTIL_VecToAngles((pTarget->EyePosition() - m.pScriptedEnt->Center()).Normalize());
						else m.pScriptedEnt->pev->v_angle = StringToVec(Params[1]);
						m.pScriptedEnt->pev->v_angle *= -1;
					}
					else if( AngToModify == "view_origin" )
					{
						m.pScriptedEnt->pev->v_angle = UTIL_VecToAngles((StringToVec(Params[1]) - m.pScriptedEnt->Center()).Normalize());
						m.pScriptedEnt->pev->v_angle *= -1;
					}


					else if( (AngToModify == "face.x" || AngToModify == "face.pitch") )
						{ m.pScriptedEnt->pev->angles.x = atof(Params[1]); SetFace = true; }
					else if( (AngToModify == "face.y" || AngToModify == "face.yaw") )
						{ m.pScriptedEnt->pev->angles.y = atof(Params[1]); SetFace = true; }
					else if( (AngToModify == "face.z" || AngToModify == "face.roll") )
						{ m.pScriptedEnt->pev->angles.z = atof(Params[1]); SetFace = true; }

					else if( AngToModify == "face" )
					{
						CBaseEntity *pTarget = m.pScriptedEnt->RetrieveEntity( Params[1] );
						if( pTarget ) m.pScriptedEnt->pev->angles = UTIL_VecToAngles((pTarget->EyePosition() - m.pScriptedEnt->Center()).Normalize());
						else m.pScriptedEnt->pev->angles = StringToVec(Params[1]);
						SetFace = true;
					}
					else if( AngToModify == "face_origin" )
					{
						m.pScriptedEnt->pev->angles = UTIL_VecToAngles((StringToVec(Params[1]) - m.pScriptedEnt->Center()).Normalize());
						SetFace = true;
					}
				}

				if( SetFace )		//If I set face angles, make the view angles match up with the face angles
				{
					 m.pScriptedEnt->pev->v_angle = m.pScriptedEnt->pev->angles;
					 m.pScriptedEnt->pev->v_angle.x *= -1;
				}

			}
		}
		else ERROR_MISSING_PARMS;
	}
	//******************************** MOVETYPE ***************************
	else if( Cmd.Name() == "movetype" ) {
	#ifdef VALVE_DLL
		if( Params.size() >= 1 )
		{
			if( Params[0] == "projectile" )
				m.pScriptedEnt->MSMoveType = MOVETYPE_ARROW;
			else 
				m.pScriptedEnt->MSMoveType = MOVETYPE_NORMAL;
		}
		else ERROR_MISSING_PARMS;
	#endif
	}
	//******************************* SETMODEL ****************************
	else if( Cmd.Name() == "setmodel" )
	{
	#ifdef VALVE_DLL
		if( Params.size() >= 1 )
		{
			if( Params[0] == "none" )
				SetBits( m.pScriptedEnt->pev->effects, EF_NODRAW );
			else 
			{
				sTemp = "models/";
				sTemp += Params[0];
				m.pScriptedEnt->m_ModelName = sTemp;
				m.pScriptedEnt->pev->model = MAKE_STRING( m.pScriptedEnt->m_ModelName.c_str() );
				SET_MODEL( m.pScriptedEnt->edict(), STRING(m.pScriptedEnt->pev->model) );

				ClearBits( m.pScriptedEnt->pev->effects, EF_NODRAW );
			}
		}
		else ERROR_MISSING_PARMS;
	#endif
	}	
	//****************************** SETMODELBODY ************************
	else if( Cmd.Name() == "setmodelbody" ) {
	#ifdef VALVE_DLL
		if( Params.size() >= 2 )
			SetBodygroup( GET_MODEL_PTR( ENT(m.pScriptedEnt->pev) ), m.pScriptedEnt->pev, atoi(Params[0]), atoi(Params[1]) );

		else ERROR_MISSING_PARMS;
	#endif
	}
	//****************************** SETMODELSKIN ************************
	else if( Cmd.Name() == "setmodelskin" ) {
	#ifdef VALVE_DLL
		if( Params.size() >= 1 )
			m.pScriptedEnt->pev->skin = atoi(Params[0]);
		else ERROR_MISSING_PARMS;
	#endif
	}
	//*************************** STOREENTITY ******************************
	else if( Cmd.Name() == "storeentity" ) {
		if( Params.size() >= 1 )
		{
			if( m.pScriptedEnt )
			{
				int EntType = EntityNameToType( Params[0] );
				if( EntType > -1 )
				{
					CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity( Params[1] );
					if( pEntity )
						m.pScriptedEnt->StoreEntity( pEntity, (enttype_e)EntType );
				}
				else ALERT( at_console, "Erorr  (%s), %s - '%s' not a valid entity type!\n", m.ScriptFile.c_str(), Cmd.Name().c_str(), Params[0].c_str() );
				
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//****************************** SETWEARPOS ****************************
	else if( Cmd.Name() == "setwearpos" ) {
		if( Params.size() >= 1 )
		{
			if( m.pScriptedEnt && m.pScriptedEnt->IsPlayer() )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)m.pScriptedEnt;
				if( Params[0] == "clearall" )
				{
					pPlayer->m_WearPositions.clearitems( );
				}
				else if( Params.size() >= 2 )
				{
					wearpos_t *pWearPos = NULL;
					 for (int i = 0; i < pPlayer->m_WearPositions.size(); i++) 
					{
						if( Params[0] != pPlayer->m_WearPositions[i].Name )
							continue;

						pWearPos = &pPlayer->m_WearPositions[i];
						break;
					}

					if( !pWearPos )	//Create new position
					{
						pPlayer->m_WearPositions.add( wearpos_t( Params[0], atoi(Params[1])) );
					}
					else	//Change max slots in position
						pWearPos->MaxAmt = atoi(Params[1]);
				}
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//****************************** SETGAITSPEED ****************************
	else if( Cmd.Name() == "setgaitspeed" ) {
		if( Params.size() >= 1 )
		{
			if( m.pScriptedEnt && m.pScriptedEnt->IsPlayer() )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)m.pScriptedEnt;
				pPlayer->m_GaitFramerateGauge = atof(Params[0]);
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//****************************** VOLUME ******************************
	else if( Cmd.Name() == "volume" ) {
		if( Params.size() >= 1 )
		{
			m.pScriptedEnt->SndVolume = atof(Params[0]) * 0.1;
			if( m.pScriptedEnt->SndVolume < 0 ) m.pScriptedEnt->SndVolume = 0;
			if( m.pScriptedEnt->SndVolume > 1 ) m.pScriptedEnt->SndVolume = 1;
		}
		else ERROR_MISSING_PARMS;
	}
	//******************************** USETRIGGER ***************************
	else if( Cmd.Name() == "usetrigger" ) {
	#ifdef VALVE_DLL
		if( Params.size() >= 1 )
		{
			 for (int i = 0; i < Params.size(); i++) 
				FireTargets( Params[i], m.pScriptedEnt, m.pScriptedEnt, USE_TOGGLE, 0 );
		}
		else ERROR_MISSING_PARMS;
	#endif
	}
	//******************************** GIVEHP / GIVEMP ***************************
	else if( Cmd.Name() == "givehp" || Cmd.Name() == "givemp" ) {
	#ifdef VALVE_DLL
		if( Params.size() >= 1 )
			//Must be in braces for logic
		{ 
			CBaseEntity *pTarget = m.pScriptedEnt;
			msstring_ref Amt = Params[0];
			if( Params.size() >= 2 ) { pTarget = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL; Amt = Params[1]; }

			if( pTarget ) pTarget->Give( Cmd.Name() == "givehp" ? GIVE_HP : GIVE_MP, atof(Amt) ); 
		}

		else ERROR_MISSING_PARMS;
	#endif
	}
	//******************************** DRAINSTAMINA ***************************
	else if( Cmd.Name() == "drainstamina" ) 
	{
			if( Params.size() >= 2 )
			{
				float Amt = atof(Params[1]);
				#ifdef VALVE_DLL
					CBaseEntity *pTarget = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
					if( pTarget && pTarget->IsPlayer( ) )
					{
						MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pTarget->pev );
							WRITE_BYTE( 5 );
							WRITE_SHORT( Amt );
						MESSAGE_END();

					}
				#else
					Player_UseStamina( Amt );
				#endif
			}
			else ERROR_MISSING_PARMS;
	}
	//******************************** DROP_TO_FLOOR ***************************
	else if( Cmd.Name() == "drop_to_floor" ) {
	#ifdef VALVE_DLL
		CBaseEntity *pTarget = (Params.size() >= 1) ? RetrieveEntity( Params[0] ) : m.pScriptedEnt;

		if( pTarget ) DROP_TO_FLOOR( pTarget->edict() );
	#endif
	}
	//******************************** REGISTERRACE ***************************
	else if( Cmd.Name() == "registerrace" ) {
#ifdef VALVE_DLL
		race_t NewRace;
		NewRace.Name = SCRIPTVAR("reg.race.name");
		TokenizeString( SCRIPTVAR("reg.race.enemies"), NewRace.Enemies );
		TokenizeString( SCRIPTVAR("reg.race.allies"), NewRace.Allies );

		CRaceManager::AddRace( NewRace );		
#endif
	}
	//******************************** REGISTERTITLE ***************************
	else if( Cmd.Name() == "registertitle" ) 
	{
		if( Params.size() >= 1 )
		{
			if( Params.size() < 2 )
			{	//Default title
				CTitleManager::DefaultTitle.Name = Params[0];
				CTitleManager::DefaultTitle.MinLevel = 0;
			}
			else
			{	//Specific title
				title_t NewTitle;
				NewTitle.Name = Params[0];
				NewTitle.MinLevel = atoi(GetScriptVar("TITLE_MINSKILL"));
				static msstringlist Skills;
				Skills.clearitems( );
				TokenizeString( Params[1], Skills );
				bool SkillSuccess = true;
				 for (int s = 0; s < Skills.size(); s++) 
				{
					int Skill = GetSkillStatByName(SCRIPTVAR(Skills[s]));
					if( Skill == -1 )
					{ 
						SkillSuccess = false; 
						break;
					}
					NewTitle.SkillsReq.add( Skill );
				}

				if( SkillSuccess )
					CTitleManager::AddTitle( NewTitle );
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//******************************** REGISTERDEFAULTS ***************************
	else if( Cmd.Name() == "registerdefaults" ) 
	{
		//Reset old
		MSGlobals::DefaultWeapons.clearitems( );
		MSGlobals::DefaultFreeItems.clearitems( );

		TokenizeString( SCRIPTVAR("reg.newchar.weaponlist"), MSGlobals::DefaultWeapons );
		TokenizeString( SCRIPTVAR("reg.newchar.freeitems"), MSGlobals::DefaultFreeItems );
		MSGlobals::DefaultGold = atoi( SCRIPTVAR("reg.newchar.gold") );
		MSGlobals::DefaultSpawnBoxModel = SCRIPTVAR("reg.hud.spawnbox");
		#ifdef VALVE_DLL
			PRECACHE_MODEL( MSGlobals::DefaultSpawnBoxModel );
		#else
			MSCLGlobals::DefaultHUDCharAnims.Idle_Weapon = SCRIPTVAR("reg.hud.char.active_weapon");
			MSCLGlobals::DefaultHUDCharAnims.Idle_NoWeapon = SCRIPTVAR("reg.hud.char.active_noweap");
			MSCLGlobals::DefaultHUDCharAnims.Fidget = SCRIPTVAR("reg.hud.char.figet");
			MSCLGlobals::DefaultHUDCharAnims.Highlighted = SCRIPTVAR("reg.hud.char.highlight");
			MSCLGlobals::DefaultHUDCharAnims.Uploading = SCRIPTVAR("reg.hud.char.upload");
			MSCLGlobals::DefaultHUDCharAnims.Inactive = SCRIPTVAR("reg.hud.char.inactive");

			MSCLGlobals::DefaultHUDSounds.QuickSlot_Select = SCRIPTVAR("reg.hud.quickslot.select");
			MSCLGlobals::DefaultHUDSounds.QuickSlot_Confirm = SCRIPTVAR("reg.hud.quickslot.confirm");
			MSCLGlobals::DefaultHUDSounds.QuickSlot_Assign = SCRIPTVAR("reg.hud.quickslot.assign");

			MSCLGlobals::DefaultHUDCoords.ItemDesc_X = atof(SCRIPTVAR("reg.hud.desctext.x"));
			MSCLGlobals::DefaultHUDCoords.ItemDesc_Y = atof(SCRIPTVAR("reg.hud.desctext.y"));
		#endif
	}
	//******************************** REGISTEREFFECT ***************************
	else if( Cmd.Name() == "registereffect" ) {
		#ifdef VALVE_DLL
			globalscripteffect_t Effect;

			Effect.m_Name = SCRIPTVAR("reg.effect.name");
			Effect.m_ScriptName = SCRIPTVAR("reg.effect.script");

			msstring Flags = SCRIPTVAR("reg.effect.flags");

			Effect.m_Flags = SCRIPTEFFECT_NORMAL;
			if( Flags.contains("player_action") )	SetBits( Effect.m_Flags, SCRIPTEFFECT_PLAYERACTION );
			if( Flags.contains("nostack") )			SetBits( Effect.m_Flags, SCRIPTEFFECT_NOSTACK );

			CGlobalScriptedEffects::RegisterEffect( Effect );
		#endif
	}
		//******************************** REGISTERTEXTURE ***************************
	else if( Cmd.Name() == "registertexture" ) {
		#ifndef VALVE_DLL
			
			mstexture_t NewTexture;
			clrmem( NewTexture );

			//Load settings
			NewTexture.Name = SCRIPTVAR( "reg.texture.name" );

			msstringlist ColorParts;
			NewTexture.IsReflective = atoi(SCRIPTVAR("reg.texture.reflect")) ? true : false;
			NewTexture.IsWater = atoi(SCRIPTVAR("reg.texture.water")) ? true : false;

			//Reflection settings
			NewTexture.Mirror.Blending = atoi(SCRIPTVAR("reg.texture.reflect.blend")) ? true : false;
			TokenizeString( SCRIPTVAR("reg.texture.reflect.color"), ColorParts );
			 for (int i = 0; i < ColorParts.size(); i++) 
			{
				if( i == 4 ) break;	//Too many elements specified - a color only has 4 elements
				NewTexture.Mirror.Color[i] = atof(ColorParts[i]);
			}
			NewTexture.Mirror.Blending = atoi(SCRIPTVAR("reg.texture.reflect.blend")) ? true : false;
			NewTexture.Mirror.Range = atof(SCRIPTVAR("reg.texture.reflect.range"));
			if( VarExists("reg.texture.reflect.world") )	//Check var existence, because the default is "1"
				NewTexture.Mirror.NoWorld = !atoi(SCRIPTVAR("reg.texture.reflect.world"));
			if( VarExists("reg.texture.reflect.ents") )
				NewTexture.Mirror.NoEnts = !atoi(SCRIPTVAR("reg.texture.reflect.ents"));

			MSCLGlobals::Textures.add( NewTexture );
		#endif
	}
	//****************************** DELETEENT ********************************
	else if( Cmd.Name() == "deleteme" || Cmd.Name() == "deleteent" ) 
	{
		if( m.pScriptedEnt )
			if( Cmd.Name() == "deleteme" )
			{
				if( !m.pScriptedEnt->IsPlayer() )	//Don't allow a crash by deleting players
					m.pScriptedEnt->DelayedRemove( );
			}
			else if( Params.size() >= 1 )
			{
				
				CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity( Params[0] );
				if( pEntity && !pEntity->IsPlayer())
				{
					#ifdef VALVE_DLL
						if( Params.size() >= 2 ) 
						{
							if( Params[1] == "fade" )
							{
								if( Params.size() >= 3 )
									pEntity->SUB_FadeOut( atoi(Params[2]) );
								else
									pEntity->SUB_StartFadeOut( );
							}
						}
						else pEntity->DelayedRemove( );				
					#endif
				}
			}

	}
	//************************** REMOVESCRIPT **************************
	else if( Cmd.Name() == "removescript" ) 
	{
		m.RemoveNextFrame = true;
	}
	//************************** CREATENPC ****************************
	else if( Cmd.Name() == "createnpc" || Cmd.Name() == "createitem" ) 
	{	//Parameters: <script name> <position> [PARAM1] [PARAM2] etc...
		#ifdef VALVE_DLL
			if( Params.size() >= 2 )
			{
				Vector Position = StringToVec( Params[1] );

				CBaseEntity *pEntity = NULL;
				IScripted *pScript = NULL;

				if( Cmd.Name() == "createnpc" )
				{
					CMSMonster *NewMonster = (CMSMonster *)GET_PRIVATE(CREATE_NAMED_ENTITY(MAKE_STRING("ms_npc")));
					pEntity = NewMonster;
					if( NewMonster ) {
						NewMonster->pev->origin = Position;
						NewMonster->Spawn( Params[0] );	
						pScript = NewMonster;
					}
				}
				else
				{
					CGenericItem *pNewItem = NewGenericItem( Params[0] );
					pEntity = pNewItem; pScript = pNewItem;
					if( pNewItem )
						pNewItem->pev->origin = Position;
				}

				if( pEntity )
				{
					pEntity->StoreEntity( m.pScriptedEnt, ENT_CREATIONOWNER );
					if( m.pScriptedEnt ) 
						m.pScriptedEnt->StoreEntity( pEntity, ENT_LASTCREATED );

					if( pScript )
					{
						//Eveything starting from param 3 is passed to the created entity as PARAM1 PARAM2, etc.
						static msstringlist Params2;
						Params2.clearitems( );
						 for (int i = 0; i < Params.size()-2; i++) 
							Params2.add( Params[i+2] );

						pScript->CallScriptEvent( "game_dynamically_created", &Params2 );
					}
				}
			}
			else ERROR_MISSING_PARMS;
		#endif
	}
	//********************************* EFFECT ***************************
	else if( Cmd.Name() == "effect" ) 
	{
		#ifdef VALVE_DLL
			ScriptedEffect( Params );
		#endif
	}
	//********************************* CLEFFECT ***************************
	else if( Cmd.Name() == "cleffect" ) 
	{
		#ifndef VALVE_DLL
			CLScriptedEffect( Params );
		#endif
	}
	//********************** SETVELOCITY / ADDVELOCITY *******************
	else if( Cmd.Name() == "setvelocity" ||
		     Cmd.Name() == "addvelocity" ) 
	{	//Parameters: <target> <velocity>
		if( Params.size() >= 2 )
			if( m.pScriptedEnt )
			{
				CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity( Params[0] );
				if( pEntity )
				{
					if( Cmd.Name() == "setvelocity" ) pEntity->pev->velocity = StringToVec( Params[1] );
					else pEntity->pev->velocity += StringToVec( Params[1] );
				}
			}

		else ERROR_MISSING_PARMS;
	}
	//*********************** SETORIGIN / ADDORIGIN *********************
	else if( Cmd.Name() == "setorigin" ||
		     Cmd.Name() == "addorigin" ) 
	{	//Parameters: <target> <origin>
		if( Params.size() >= 2 )
			if( m.pScriptedEnt )
			{
				CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity( Params[0] );
				if( pEntity )
				{
					if( Cmd.Name() == "setorigin" ) pEntity->pev->origin = StringToVec( Params[1] );
					else pEntity->pev->origin += StringToVec( Params[1] );
				}
			}

		else ERROR_MISSING_PARMS;
	}
	//*************************** CLIENTEVENT ****************************
	else if( Cmd.Name() == "clientevent" ) 
	{	//<new/persist> <target> <scriptname> [param1] [param2] etc..
		//<update> <target> <scriptid> <eventname> [param1] [param2] etc..
		//<remove> <target> <scriptid>
		#ifdef VALVE_DLL
		if( Params.size() >= 3 )
		{
			bool Persistent = false;
			msstring MsgType = Params[0];
			if( Params[0] == "persist" )
			{
				MsgType = "new";
				Persistent = true;
			}

			scriptsendcmd_t SendCmd;

			SendCmd.ScriptName = Params[2];
			SendCmd.UniqueID = (MsgType == "new") ? ++m_gLastSendID : atoi(SendCmd.ScriptName);
			SendCmd.MsgType = MsgType;
			SendCmd.MsgTarget = Params[1];
			if( Params.size() >= 4 ) foreach( p, Params.size() - 3 ) SendCmd.Params.add( Params[p+3] );
			SendScript( SendCmd );
			if( Persistent ) m.PersistentSendCmds.add( SendCmd );
			else if( Params[0] == "remove" )
			{
				//Stop event from persisting after "clientevent remove" is called on it
				 for (int e = 0; e < m.PersistentSendCmds.size(); e++) 
					if( m.PersistentSendCmds[e].UniqueID == SendCmd.UniqueID )
					{
						m.PersistentSendCmds.erase( e );
						break;
					}
			}
		}
		else ERROR_MISSING_PARMS;
		#endif
	}
	//*************************** APPLYEFFECT ****************************
	else if( Cmd.Name() == "applyeffect" ) 
	{	//<target> <effect name> [param1] [param2]...
		#ifdef VALVE_DLL
		if( Params.size() >= 2 )
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
			if( pEntity && pEntity->GetScripted() )
			{
				IScripted *pScripted = pEntity->GetScripted();

				msstringlist Parameters;
				 for (int i = 0; i < Params.size()-2; i++) 
					Parameters.add( Params[i+2] );
				CGlobalScriptedEffects::ApplyEffect( Params[1], pScripted, pEntity, &Parameters );
			}
		}
		else ERROR_MISSING_PARMS;
		#endif
	}
	//**************************** PLAYSOUND ***************************
	else if( Cmd.Name() == "playsound" || Cmd.Name() == "playrandomsound" ) {
		if( Params.size() >= 2 )
		{
			//Old way: playsound <chan> <sound>
			//New way: playsound <chan> <volume> <sound>
			if( m.pScriptedEnt )
			{
				int iChannel = atoi(Params[0]);
				int NextParm = 1;
				float Volume = -1;

				if( isdigit(Params[1][0]) )
				{
					Volume = atof(Params[1]) / 10.0f;
					NextParm++;
				}

				msstring_ref pszSound = ((signed)Params.size() > NextParm) ? Params[NextParm] : "common/null.wav";

				if( Cmd.Name() == "playrandomsound" )
					pszSound = Params[NextParm + RANDOM_LONG( 0, Params.size() - (Volume > -1 ? 3 : 2) )];

				if( !FStrEq(pszSound,"none") ) //skip over 'none'
					if( Volume > -1 )
					{
						if( Volume ) EMIT_SOUND( m.pScriptedEnt->edict(), iChannel, pszSound, Volume, ATTN_NORM );
						else EMIT_SOUND( m.pScriptedEnt->edict(), iChannel, "common/null.wav", 0.001f, ATTN_NORM );
						//else 
						//	STOP_SOUND( m.pScriptedEnt->edict(), iChannel, pszSound );
					}
					else
						EMIT_SOUND( m.pScriptedEnt->edict(), iChannel, pszSound, m.pScriptedEnt->SndVolume, ATTN_NORM );

			}

		}
		else ERROR_MISSING_PARMS;
	}
	//**************************** SOUND.PLAY3D ***************************
	else if( Cmd.Name() == "sound.play3d" ) 
	{
			if( Params.size() >= 3 )
			{	//<sound> <volume> <origin> [attenuation]
				float Attn = Params.size() >= 4 ? atof(Params[3]) : ATTN_NORM;
				
				float Volume = atof(Params[1]) / 10;

				EngineFunc::Shared_PlaySound3D( Params[0], Volume, StringToVec(Params[2]), Attn );
			}
			else ERROR_MISSING_PARMS;
	}
	//**************************** SOUND.PM_PLAY ***************************
	else if( Cmd.Name() == "sound.pm_play" ) 
	{
		//Can only be called from game_playermove
		//Plays a PM sound - On client, plays sound locally
		//                   On server, plays sound for all OTHER players
		#ifndef VALVE_DLL
			if( Params.size() >= 3 )
			{	//<sound> <volume> <channel>
				pmove->PM_PlaySound( atoi(Params[2]), Params[0], atof(Params[1]), ATTN_NORM, 0, PITCH_NORM );
			}
			else ERROR_MISSING_PARMS;
		#endif
	}			

	//**************************** SOUND.SETVOLUME ***************************
	else if( Cmd.Name() == "sound.setvolume" ) {
		if( Params.size() >= 3 )
		{	//<chan> <sound> <volume>
			if( m.pScriptedEnt )
			{
				EMIT_SOUND_DYN( m.pScriptedEnt->edict(), atoi(Params[0]), Params[1], atof(Params[2])/ 10.0f, ATTN_NORM, SND_CHANGE_PITCH |SND_CHANGE_VOL, PITCH_NORM);
				//EMIT_SOUND_DYN( m.pScriptedEnt->edict(), CHAN_STATIC, Params[1], 
				//	atof(Params[2])/ 10.0f, ATTN_STATIC, SND_CHANGE_PITCH | SND_CHANGE_VOL, PITCH_NORM);
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//************************** PRECACHEFILE **************************
	else if( Cmd.Name() == "precachefile" ) {
		if( Params.size() >= 1 )
		{
			msstring &ScriptName = Params[0];
			bool PrecacheOnly = true;
			if( Params.size() >=2 )
			{
				if( Params[0] == "[full]" ) PrecacheOnly = false;
				ScriptName = Params[1];
			}

			CScript TempScript;
			bool fSuccess = TempScript.Spawn( ScriptName, NULL, NULL, PrecacheOnly );
			if( fSuccess ) { if( !PrecacheOnly ) TempScript.RunScriptEventByName( "game_precache" ); }
			else ALERT( at_console, "Script Erorr (%s), %s - '%s' possible file not found!\n", m.ScriptFile.c_str(), Cmd.Name().c_str(), ScriptName.c_str() );
		}
		else ERROR_MISSING_PARMS;
	}
	//****************************** PLAYERMESSAGE / CONSOLEMSG *********************
	else if( Cmd.Name() == "playermessage" || Cmd.Name() == "consolemsg" ) {
		if( Params.size( ) >= 2 )
		{
			CBaseEntity *pEntity = RetrieveEntity( Params[0] );
			if( pEntity && pEntity->IsPlayer( ) )
			{
				 for (int i = 0; i < Params.size()-1; i++) 
				{
					if( i ) sTemp += " ";
					sTemp += Params[i+1];
				}
				if( Cmd.Name() == "playermessage" )
				{
					if( sTemp.len() ) sTemp += "\n";
					((CBasePlayer *)pEntity)->SendInfoMsg( sTemp );
				}
				else if( Cmd.Name() == "consolemsg" )
					ClientPrint( pEntity->pev, at_console, sTemp );
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//********************************* INFOMSG ************************************
	else if( Cmd.Name() == "infomsg" ) {
		if( Params.size( ) >= 3 )
		{
			bool SendToAll = false;
			if( Params[0] == "all" ) SendToAll = true; 
	
			CBaseEntity *pEntity = NULL;
			if( !SendToAll )
				#ifdef VALVE_DLL
					pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
				#else
					pEntity = (CBaseEntity *)&player;
				#endif

			if( SendToAll || (pEntity && pEntity->IsPlayer( )) )
			{
				msstring Title = Params[1];
				 for (int i = 0; i < Params.size()-2; i++) 
				{
					if( i ) sTemp += " ";
					sTemp += Params[i+2];
				}

				#ifdef VALVE_DLL
					if( SendToAll )
						SendHUDMsgAll( Title, sTemp );
					else
				#endif
						((CBasePlayer *)pEntity)->SendHUDMsg( Title, sTemp );
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//********************************* RESPAWN ************************************
	else if( Cmd.Name() == "respawn" ) {
		if( Params.size( ) >= 1 )
		{
			#ifdef VALVE_DLL

				static mslist<CBasePlayer *> PlayerList;
				PlayerList.clearitems( );

				if( Params[0] == "all" )
				{			
					for( int i = 1; i <= gpGlobals->maxClients; i++ )
					{
						CBaseEntity *pEntity = UTIL_PlayerByIndex( i );
						if( pEntity ) PlayerList.add( (CBasePlayer *)pEntity );
					}
				}
				else
				{
					CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
					if( pEntity && pEntity->IsPlayer( ) )
						PlayerList.add( (CBasePlayer *)pEntity );
				}

				 for (int i = 0; i < PlayerList.size(); i++) 
					PlayerList[i]->MoveToSpawnSpot( );
			#endif	
		}
		else ERROR_MISSING_PARMS;
	}
	//****************************** RETURNDATA ***********************
	else if( Cmd.Name() == "returndata" ) {
		if( Params.size( ) >= 1 )
		{
			if( m.pScriptedInterface )
			{
				if( m.pScriptedInterface->m_ReturnData[0] ) m.pScriptedInterface->m_ReturnData += ";";
				 for (int i = 0; i < Params.size(); i++) 
				{
					if( i ) m.pScriptedInterface->m_ReturnData += " ";
					m.pScriptedInterface->m_ReturnData += Params[i];
				}
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//**************************** EMITSOUND *************************
	else if( Cmd.Name() == "emitsound" ) { //<srcentity> <origin> <volume> <duration> <type> [danger radius]
		#ifdef VALVE_DLL
			if( Params.size( ) >= 5 )
			{
				if( m.pScriptedEnt )
				{
					CBaseEntity *pEntity = m.pScriptedEnt->RetrieveEntity( Params[0] );
					if( pEntity )
					{
						Vector Origin = StringToVec(Params[1]);
						float Volume = atof(Params[2]);
						float Duration = atof(Params[3]);
						msstring &SoundType = Params[4];
						float DangerRadius = atof(Params[5]);
			
						CSoundEnt::InsertSound( pEntity, SoundType, Origin, Volume, Duration, DangerRadius );
					}
				}
			}
			else ERROR_MISSING_PARMS;
		#endif
	}
	//*************************** COMPANION ****************************
	else if( Cmd.Name() == "companion" ) 
	{	//<add/remove> <target companion> <target player>
		#ifdef VALVE_DLL
		if( Params.size() >= 3 )
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[2] ) : NULL;
			if( pEntity && pEntity->IsPlayer() )
			{
				CBaseEntity *pCompanionEnt = m.pScriptedEnt->RetrieveEntity( Params[1] );
				if( m.pScriptedEnt && pCompanionEnt->IsMSMonster() )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					CMSMonster *pCompanion = (CMSMonster *)pCompanionEnt;

					if( Params[0] == "add" )
					{
						bool CanAdd = true;
						 for (int i = 0; i < pPlayer->m_Companions.size(); i++) 
							if( pPlayer->m_Companions[i].Entity.Entity() == pCompanion )
							{
								CanAdd = false;	//Already exist on player's list
								break;
							}

						if( CanAdd )
						{
							companion_t NewCompanion;
							NewCompanion.Active = true;
							NewCompanion.Entity = m.pScriptedEnt;
							NewCompanion.ScriptName = pCompanion->m_ScriptName;
							pPlayer->m_Companions.add( NewCompanion );

							pCompanion->StoreEntity( pPlayer, ENT_OWNER );
							pPlayer->CallScriptEvent( "game_companion_added" );
							pCompanion->CallScriptEvent( "game_companion_added" );
						}
					}
					else //"remove"
					{
						 for (int i = 0; i < pPlayer->m_Companions.size(); i++) 
							if( pPlayer->m_Companions[i].Entity.Entity() == pCompanion )
							{
								pPlayer->m_Companions.erase( i );
								break;
							}

						pCompanion->StoreEntity( NULL, ENT_OWNER );
						pCompanion->CallScriptEvent( "game_companion_removed" );
						pPlayer->CallScriptEvent( "game_companion_removed" );
					}
				}
			}
		}
		else ERROR_MISSING_PARMS;
		#endif
	}
	//*************************** HELPTIP ****************************
	else if( Cmd.Name() == "helptip" ) 
	{
		//<target> <tipname> <title> <text>
		if( Params.size() >= 4 )
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
			if( pEntity && pEntity->IsPlayer() )
			{
				CBasePlayer *pPlayer = (CBasePlayer *)pEntity;

				MSGlobals::Buffer[0] = 0;
				 for (int i = 0; i < Params.size()-3; i++) 
					strcat( MSGlobals::Buffer, Params[i+3] );
				pPlayer->SendHelpMsg( Params[1], Params[2], MSGlobals::Buffer );
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//**************************** QUEST *****************************
	else if( Cmd.Name() == "quest" ) 
	{
		#ifdef VALVE_DLL
			//<set/unset> <target> <name> <data>
			if( Params.size() >= 3 )
			{
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[1] ) : NULL;
				if( pEntity && pEntity->IsPlayer() )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					msstring &Action = Params[0];
					msstring &Name = Params[2];
					msstring &Data = Params.size() >= 4 ? Params[3] : NULL;

					bool SetData = true;
					if( Action == "unset" ) SetData = false;

					if( !SetData || Params.size() >= 4 )
						pPlayer->SetQuest( SetData, Name, Data );
				}
			}
			else ERROR_MISSING_PARMS;
		#endif
	}
	//*************************** SETCALLBACK ****************************
	else if( Cmd.Name() == "setcallback" ) 
	{
		//<type> <enable/disable>
		if( Params.size() >= 2 )
		{
			if( m.pScriptedEnt )
			{
				if( m.pScriptedEnt->GetScripted() )
				{
					CScriptedEnt *pScriptedEnt = (CScriptedEnt *)m.pScriptedEnt;
					msstring &Type = Params[0];
					msstring &Setting = Params[1];

					bool NewSetting = false;
					if( Setting == "enable" ) NewSetting = true;

					if( Type.contains("touch") ) pScriptedEnt->m_HandleTouch = NewSetting;
					if( Type.contains("think") ) pScriptedEnt->m_HandleThink = NewSetting;
					if( Type.contains("blocked") ) pScriptedEnt->m_HandleBlocked = NewSetting;
					if( Type.contains("render") ) m.m_HandleRender = NewSetting;
				}
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//******************************* SETEXPSTAT *************************
	else if( Cmd.Name() == "setexpstat" ) {
		if( Params.size() >= 2 )
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
			if( pEntity && pEntity->IsMSMonster() )
			{
				CMSMonster *pMonster = (CMSMonster *)pEntity;

				int StatVar = -1, PropVar = -1;
				GetStatIndices( Params[1], StatVar, PropVar );
				if( StatVar > -1 )
				{
					pMonster->m_UseExpStat = true;
					pMonster->m_ExpStat = StatVar;
					pMonster->m_ExpProp = PropVar;
				}
			}
		}
		else ERROR_MISSING_PARMS;
	}
	//**************************** SETENV *****************************
	else if( Cmd.Name() == "setenv" ) 
	{
		#ifndef VALVE_DLL
			//<prop> <value>
			if( Params.size() >= 2 )
			{
				msstring &Prop = Params[0];
				msstring &Value = Params[1];
				if( Prop == "sky.texture" )			CEnvMgr::ChangeSkyTexture( Value );
				else if( Prop == "lightgamma" )		CEnvMgr::SetLightGamma( atof( Value ) );
				else if( Prop == "maxviewdist" )	CEnvMgr::m_MaxViewDistance = atof( Value );
				else if( Prop == "fog.enabled" )	CEnvMgr::m_Fog.Enabled = atoi( Value ) ? true : false;
				else if( Prop == "fog.color" )		CEnvMgr::m_Fog.Color = StringToVec( Value );
				else if( Prop == "fog.density" )	CEnvMgr::m_Fog.Density = atof( Value );
				else if( Prop == "fog.start" )		CEnvMgr::m_Fog.Start = atof( Value );
				else if( Prop == "fog.end" )		CEnvMgr::m_Fog.End = atof( Value );
				else if( Prop == "fog.type" )
				{
					if( Value == "exp" )		CEnvMgr::m_Fog.Type = 0x0800;	//GL_EXP
					else if( Value == "exp2" )	CEnvMgr::m_Fog.Type = 0x0801;	//GL_EXP2
					else						CEnvMgr::m_Fog.Type = 0x2601;	//GL_LINEAR
				}
				else if( Prop == "screen.tint" )	CEnvMgr::ChangeTint( StringToColor(Value) );
			}
			else ERROR_MISSING_PARMS;
		#endif
	}
	//************************** TRACELINE ***************************
	else if( Cmd.Name() == "traceline" ) 
	{
		//<startpos> <endpos> <flags> <ignoreent>
		if( Params.size() >= 3 )
		{
			Vector StartPos = StringToVec(Params[0]);
			Vector EndPos = StringToVec(Params[1]);
			msstring &Flags = Params[2];
			CBaseEntity *pIgnoreEnt = 
			#ifdef VALVE_DLL
				(Params.size() >= 4) ? StringToEnt( Params[3] ) : NULL;
			#else
				NULL;
			#endif

			int iFlags = 0;
			#ifdef VALVE_DLL
				if( Flags.contains("normal" ) )				iFlags = dont_ignore_monsters;
				else if( Flags.contains("worldonly" ) )		SetBits( iFlags, ignore_monsters );
				else if( Flags.contains("ignoreglass" ) )	SetBits( iFlags, missile );			//missile is just a hack to indicate ignore_glass
				else if( Flags.contains("simplebox" ) )		SetBits( gpGlobals->trace_flags, FTRACE_SIMPLEBOX );
			#else
				if( Flags.contains("normal" ) )			iFlags = 0;
				if( Flags.contains("worldonly" ) )		SetBits( iFlags, PM_WORLD_ONLY );
				if( Flags.contains("ignoreglass" ) )	SetBits( iFlags, PM_GLASS_IGNORE );
				if( Flags.contains("simplebox" ) )		SetBits( iFlags, PM_STUDIO_BOX );
			#endif

			sharedtrace_t Tr;
			EngineFunc::Shared_TraceLine( StartPos, EndPos, iFlags, Tr, 0, iFlags, pIgnoreEnt ? pIgnoreEnt->edict() : NULL );
			ClearBits( gpGlobals->trace_flags, FTRACE_SIMPLEBOX );
						
			SetVar( "game.trace.hit", (Tr.Fraction < 1 || Tr.HitEnt) ? "1" : 0, Event );
			SetVar( "game.trace.fraction", UTIL_VarArgs("%f",Tr.Fraction), Event );
			#ifdef VALVE_DLL
				CBaseEntity *pHitEnt = Tr.HitEnt ? MSInstance(INDEXENT(Tr.HitEnt)) : NULL;
				SetVar( "game.trace.hitent", pHitEnt ? EntToString(pHitEnt) : "none", Event );
			#else
				SetVar( "game.trace.hitent", UTIL_VarArgs("%i",Tr.HitEnt), Event );
			#endif
			SetVar( "game.trace.endpos", VecToString(Tr.EndPos), Event );
				
		}
		else ERROR_MISSING_PARMS;
	}	
	//**************************** NPCMOVE *****************************
	else if( Cmd.Name() == "npcmove" ) 
	{
		#ifdef VALVE_DLL
			if( Params.size() >= 1 )
			{
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
				if( pEntity && pEntity->IsMSMonster() )
				{
					CMSMonster *pMonster = (CMSMonster *)pEntity;

					moveexec_t MoveExec;
					clrmem( MoveExec );

					MoveExec.vecStart = pMonster->pev->origin;
					MoveExec.vecEnd = StringToVec(GetVar( "reg.npcmove.endpos" ));
					MoveExec.fTestMove = atoi(GetVar( "reg.npcmove.testonly" )) ? true : false;
					if( Params.size() >= 2 ) MoveExec.pTarget = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[1] ) : RetrieveEntity( Params[1] );


					bool Success = pMonster->MoveExecute( MoveExec );

					Event.SetVar( "game.ret.npcmove.success", Success ? "1" : "0"  );
					Event.SetVar( "game.ret.npcmove.dist", UTIL_VarArgs("%.2f",MoveExec.Return_DistMoved) );
					Event.SetVar( "game.ret.npcmove.hitstep", MoveExec.Return_HitStep ? "1" : "0" );
				}
			}
			else ERROR_MISSING_PARMS;
		#endif
	}
	else return false;


	//Return true to continue event execution, or to prevent subsequent 'else' statmements from occuring
	return ConditionalCmd ? ConditionsMet : true;
}
