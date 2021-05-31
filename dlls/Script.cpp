/* 
	Master Sword - A Half-life Total Conversion
	Script.cpp - Parses & Executes script event files for NPCs or weapons 
*/

#include "inc_weapondefs.h"

#include "Script.h"
#include "../MSShared/GroupFile.h"
#ifdef VALVE_DLL
	#include "SVGlobals.h"
	#include "../MSShared/Global.h"
	bool GetModelBounds( CBaseEntity *pEntity, Vector Bounds[2] );

#else
	#include "../cl_dll/MasterSword/CLGlobal.h"
	#include "../cl_dll/hud.h"
	#include "../cl_dll/cl_util.h"
	#include "../cl_dll/MasterSword/HUDScript.h"
#endif
#include "../engine/studio.h"
#include "logfile.h"

#undef SCRIPTVAR
#define SCRIPTVAR GetVar								//A script-wide or global variable
#define SCRIPTCONST( a ) SCRIPTVAR(GetConst(a))			//A const, script-wide, or global variable - loadtime only
#define GETCONST_COMPATIBLE( a ) ( a[0] == '$' ? GetConst(a) : SCRIPTCONST(a) )			//Loadtime - Only parse it as a var if it's not a $parser
int CountPlayers( void );
bool FindSkyHeight( Vector Origin, float &SkyHeight );
msstring_ref PM_GetValue( msstringlist &Params );
bool GetModelBounds( void *pModel, Vector Bounds[2] );


scriptcmdname_list CScript::m_GlobalCmds;				//Shared list of script commands
mslist<scriptvar_t> CScript::m_gVariables;				//Global script variables
ulong CScript::m_gLastSendID = SCRIPT_ID_START;			//Server: ID of last script sent to a client
														//Client: ID of next script to be created
ulong CScript::m_gLastLightID;							//ID of last dynamic light

bool BreakUpLine( msstring &Line, msstring &outParserName, msstringlist &outParams )
{
	if( Line[0] != '$' ) //Example: $cansee(ent_lastseen,ATTACK_RANGE)
		return true;	 //Was not a parser, ignore

	//'P' here stands for parenthesis
	size_t StartP = Line.findchar( "(" );
	if( StartP == msstring_error )
		return false;

	outParserName = Line.substr( 0, StartP );

	StartP++;
	int PCount = 1;
	int LastParamEnd = StartP;

	//Use a PCount to keep track of open and close parenthesis and determine the internal parameters
	//PCount starts at 1
	for( int i = StartP; i < (signed)Line.len(); i++ )
	{
		char Char = Line[i];
		if( Char == '(' ) PCount++;						//Increase counter when open P found
		else if( Char == ')' ) PCount--;				//Decrease counter when close P found

		if( (Char == ',' && PCount == 1) ||				//Found a comma, save parameter
			(Char == ')' && PCount == 0) )				//Found the closing P, save parameter
		{ 
			msstring Temp = Line.substr( LastParamEnd, i-LastParamEnd );
			outParams.add( Temp );

			LastParamEnd = i+1;
		}

		if( !PCount )										//If this is the closing P, then stop here
			break;
	}

	if( PCount != 0 )	//Parenthesis did not close correctly
		return false;

	//Parenthesis closed correctly
	return true;
}

//Script Manager - Loads and Unloads groupfile when neccesary
class ScriptMgr
{
public:
	static CGroupFile m_GroupFile;
	static int m_TotalScripts;

	static void RegisterScript( CScript *NewScript )
	{
		if( !m_GroupFile.IsOpen( ) )
		{
			char cGroupFilePath[MAX_PATH];
			sprintf( cGroupFilePath, "%s/dlls/sc.dll", EngineFunc::GetGameDir() );
			m_GroupFile.Open( cGroupFilePath );
		}

		m_TotalScripts++;
	}
	static void UnRegisterScript( CScript *NewScript )
	{
		m_TotalScripts--;
		if( m_TotalScripts == 0 )
		{
			m_GroupFile.Close( );
		}
	}
};
CGroupFile ScriptMgr::m_GroupFile;
int ScriptMgr::m_TotalScripts = 0;


msstring_ref CScript::GetConst( msstring_ref Text )
{
	static msstring ReturnString;
	if( !strcmp(Text,"$currentscript" ) )
	{
		ReturnString = (msstring_ref)m.ScriptFile;
		ReturnString = ReturnString.thru_substr( SCRIPT_EXT );	//Remove the file extention
		return ReturnString;
	}
	else if( !strcmp(Text,"$currentmapscript" ) )
	{
		ReturnString = MSGlobals::MapName;
		ReturnString += "/map_startup";
		return ReturnString;
	}
	

	if( Text[0] == '!' )	//Not operator in if() statements
	{
		ReturnString = msstring("!") + GetConst( &Text[1] );
		return ReturnString;
	}

	//Handle the constant values within an interpreted parameter (Starts with '$')
	if( Text[0] == '$' ) //Example: $cansee(ent_lastseen,ATTACK_RANGE)
	{
		msstring Line = Text;
		msstring ParserName;
		msstringlist Params;

		if( BreakUpLine( Line, ParserName, Params ) )
		{
			msstring StackReturn = ParserName;		//This string must be on the stack, so recursive parsing doesn't screw up
			StackReturn += "(";
			 for (int i = 0; i < Params.size(); i++) 
			{
				StackReturn += GetConst(Params[i]);
				if( i != Params.size()-1 ) StackReturn += ",";
				else StackReturn += ")";
			}

			ReturnString = StackReturn;		//Transfer from stack to global mem, for return
			return ReturnString;
		}
		else
			MSErrorConsoleText( __FUNCTION__, UTIL_VarArgs("Script: %s, \"%s\" - Mismatched Parenthesis!\n", m.ScriptFile.c_str(), Text) );
	}
	else
		 for (int i = 0; i < m_Constants.size(); i++) 
			if( m_Constants[i].Name == Text )
				return m_Constants[i].Value;

	return Text;
}

bool GetString( char *Return, const char *sentence, int start, char *endchars )
{
	//Quickie function to return the next CMD or parameter in a script string
	int i = 0, n, iPosition = start;
	strcpy( Return, "" );
	while( 1 ) {
		for( n = 0; (unsigned)n < strlen(endchars); n++ )
			if( sentence[iPosition] == endchars[n] ) return true;
		if( sentence[iPosition] == 0 ) return false;
		Return[i] = sentence[iPosition]; Return[i+1] = '\0';
		iPosition++; i++;
	}
	return true;
}


scriptvar_t *IVariables::FindVar( msstring_ref Name )
{
	//Check local variables
	 for (int i = 0; i < m_Variables.size(); i++) 
		if( FStrEq(Name,m_Variables[i].Name) ) return &m_Variables[i];
	return NULL;
}
msstring_ref IVariables::GetVar( msstring_ref Name )
{
	scriptvar_t *pScriptvar = FindVar( Name );
	if( pScriptvar ) return pScriptvar->Value;

	return Name;
}
scriptvar_t *IVariables::SetVar( msstring_ref Name, msstring_ref Value )
{
	scriptvar_t *pScriptvar = FindVar( Name );
	if( pScriptvar ) { pScriptvar->Value = Value; return pScriptvar; }

	return &m_Variables.add( scriptvar_t( Name, Value ) );
}

// Find an event by its name...
SCRIPT_EVENT *CScript::EventByName( const char *pszEventName ) 
{
	 for (int i = 0; i < m.Events.size(); i++) 
	{
		SCRIPT_EVENT &seEvent = m.Events[i];
		if( !seEvent.Name || seEvent.Name != pszEventName )
			continue;

		return &seEvent;
	}

	return NULL;
}
//Variable Handlers
scriptvar_t *CScript::FindVar( const char *pszName )
{
	scriptvar_t *pScriptvar = IVariables::FindVar( pszName );
	if( pScriptvar ) return pScriptvar;

	//Check global variables
	 for (int i = 0; i < m_gVariables.size(); i++) 
		if( FStrEq(pszName,m_gVariables[i].Name) ) return &m_gVariables[i];

	return NULL;
}
bool CScript::VarExists( msstring_ref pszText )
{
	return ( GetVar( pszText ) != pszText );
}

msstring_ref CScript::GetVar( msstring_ref pszText ) 
{
	static char cReturn[1024];

	msstring FullName = pszText;
	msstring Name;
	static msstring Return;
	Return[0] = 0;
	if( m.CurrentEvent ) pszText = m.CurrentEvent->GetLocal( pszText );

	if( pszText[0] == '$' )
	{
		msstring ParserName;
		msstringlist Params;			//Must be allocated each time, because this funtion is called recursively
		BreakUpLine( FullName, ParserName, Params );

		 for (int i = 0; i < Params.size(); i++) 
			Params[i] = SCRIPTVAR( Params[i] );

		//Handle entity-specific parser
		if( m.pScriptedInterface && m.pScriptedInterface->GetScriptVar( ParserName, Params, this, Return ) )
			return Return;


		//Handle parser
		if( ParserName.starts_with("$rand") )  //random number
		{
			if( ParserName[5] == 'f' ) //float version
				RETURN_FLOAT( RANDOM_FLOAT( atof(Params[0]), atof(Params[1]) ) )
			else  //int version
				RETURN_INT( RANDOM_LONG( atoi(Params[0]), atoi(Params[1]) ) )
		}
		else if( ParserName == "$int" )	//convert to integer
		{
			if( Params.size() >= 1 ) 
			{
				Return = atoi(Params[0]);
				return Return;
			}
			else return "0";
		}
		else if( ParserName == "$float" )	//convert to float  <precision> <value>
		{
			if( Params.size() >= 2 )
			{
				Return = msstring("%.") + atoi(Params[0]) + "f";
				sprintf( cReturn, Return, atof(Params[1]) );
				return cReturn;
			}
			else return "0";
		}
		else if( ParserName == "$neg" )	//Make negative
		{
			if( Params.size() >= 1 ) 
				RETURN_FLOAT( (-atof(Params[0])) )
			else return "0";
		}
		else if( ParserName == "$stradd" )	//Add strings together
		{
			 for (int i = 0; i < Params.size(); i++) 
				Return += Params[i];

			return Return;
		}
		else if( ParserName == "$get_token_amt" )	//Get number of tokens in string
		{
			if( Params.size() >= 1 ) 
			{
				static msstringlist Tokens;
				Tokens.clearitems();

				TokenizeString( Params[0], Tokens );

				RETURN_INT( Tokens.size() );
			}
			else return "0";
		}
		else if( ParserName == "$get_token" )	//Get token
		{
			if( Params.size() >= 2 ) 
			{
				static msstringlist Tokens;
				Tokens.clearitems();
				int GetItem = atoi(Params[1]);

				msstring TokenString = GetVar(Params[0]);
				TokenizeString( TokenString, Tokens );

				if( GetItem >= 0 && GetItem < (signed)Tokens.size() )
				{
					Return = Tokens[GetItem];
					return Return;
				}
				else return "0";
			}
			else return "0";
		}
		else if( ParserName == "$vec" )	//Create Vector
		{
			if( Params.size() >= 3 )
				Return = msstring("(") + Params[0] + "," + Params[1] + "," + Params[2] + ")";
			else return "0";
			return Return;
		}
		else if( ParserName.starts_with("$vec.") )	//Get vector coord
		{
			msstring CoordName = ParserName.substr(5).thru_char( "(" );
			if( Params.size() == 1 )
			{
				Vector Vec = StringToVec( Params[0] );
				if( CoordName == "x" || CoordName == "pitch" ) RETURN_FLOAT( Vec.x )
				else if( CoordName == "y" || CoordName == "yaw" ) RETURN_FLOAT( Vec.y )
				else if( CoordName == "z" || CoordName == "roll" ) RETURN_FLOAT( Vec.z )
			}
			else if( Params.size() >= 3 )
			{
				Return = "0";
				if( CoordName == "x" || CoordName == "pitch" ) Return = Params[0];
				else if( CoordName == "y" || CoordName == "yaw" ) Return = Params[1];
				else if( CoordName == "z" || CoordName == "roll" ) Return = Params[2];
				return Return;
			}
		}
		else if( ParserName == "$anglediff" )	//Get angle diff
		{
			if( Params.size() >= 2 )
			{
				float DestAngle = atof(Params[0]); 
				float SrcAngle = atof(Params[1]);
				RETURN_FLOAT( EngineFunc::AngleDiff( DestAngle, SrcAngle ) );
			}
		}
		else if( ParserName == "$dist" )		//dist between two points
		{
			if( Params.size() >= 2 )
			{
				Vector Start = StringToVec( Params[0] );
				Vector End = StringToVec( Params[1] );
				float Dist = (Start-End).Length( );
				RETURN_FLOAT( Dist );
			}
			else return "0";
		}
		else if( ParserName == "$dir" )		//dir from point1 to point2
		{
			if( Params.size() >= 2 )
			{
				Vector Start = StringToVec( Params[1] );
				Vector End = StringToVec( Params[0] );
				Vector Dir = (Start-End).Normalize( );
				RETURN_VECTOR( Dir );
			}
			else return "0";
		}
		else if( ParserName == "$veclen" )		//Length of the vector
		{
			if( Params.size() >= 1 )
			{
				Vector Start = StringToVec( Params[0] );
				RETURN_FLOAT( Start.Length() );
			}
			else return "0";
		}
		else if( ParserName == "$veclen2D" )		//Length of the vector (2D)
		{
			if( Params.size() >= 1 )
			{
				Vector Start = StringToVec( Params[0] );
				RETURN_FLOAT( Start.Length2D() );
			}
			else return "0";
		}		
		else if( ParserName.starts_with("$within_cone") ||		//Whether a point is within a defined cone
				 ParserName.starts_with("$cone_dot") )			//Dot product from defined cone
		{
			if( Params.size() >= 4 )	//point, cone origin, cone angles, cone fov
			{
				Vector PointOrigin = StringToVec( Params[0] );
				Vector ConeOrigin = StringToVec( Params[1] );
				Vector ConeAngles = StringToVec( Params[2] );
				float ConeFOV = cosf(atof( Params[3] ) / 2.0f);

				Vector vForward;
				EngineFunc::MakeVectors( ConeAngles, vForward, NULL, NULL );

				Vector	vec2LOS = PointOrigin - ConeOrigin;

				if( ParserName == "$within_cone2D" ||
					ParserName == "$cone_dot2D" ) { vec2LOS.z = 0; vForward.z = 0; }

				vec2LOS = vec2LOS.Normalize();
				float flDot = DotProduct( vec2LOS, vForward );

				if( ParserName.starts_with("$within_cone") ) 
				{
					if( flDot >= ConeFOV ) RETURN_TRUE
					else RETURN_FALSE
				}
				else RETURN_FLOAT( flDot );
			}
			else return "0";
		}
		else if( ParserName == "$relpos" )
		{
			msstring PosString;
			Vector Angle, StartPos;
			if( m.pScriptedEnt && Params.size() >= 1 )
			{
				if( Params[0][0] != '(' )	//(x,y,z)
				{
					if( m.pScriptedEnt )
					{
						StartPos = (m.pScriptedEnt->pev->modelindex ?  m.pScriptedEnt->Center( ) : m.pScriptedEnt->pev->origin),
						Angle = m.pScriptedEnt->pev->angles;
						PosString = FullName.substr(7);
					}
				}
				else	//(pitch,yaw,roll),(x,y,z)
				{
					Angle = StringToVec(Params[0]);
					PosString = Params[1];
					StartPos = g_vecZero;
				}

				Vector Pos = StartPos + GetRelativePos( Angle, StringToVec(PosString) );
				//sprintf( cReturn, "(%.2f,%.2f,%.2f)", Pos.x, Pos.y, Pos.z );
				RETURN_VECTOR( Pos )
			}
			else return "0";
		}
		else if( ParserName == "$relvel" )
		{
			if( m.pScriptedEnt && Params.size() >= 1 )
			{
				Vector Angle, RelVel;
				if( Params[0][0] != '(' )	//(x,y,z)
				{
					Angle = ( (m.pScriptedEnt->IsPlayer() || FBitSet(m.pScriptedEnt->pev->flags,FL_FLY|FL_SWIM) ) ? m.pScriptedEnt->pev->v_angle : m.pScriptedEnt->pev->angles);
					RelVel = StringToVec( &FullName[7] );
				}
				else	//(pitch,yaw,roll),(x,y,z)
				{
					Angle = StringToVec( Params[0] );
					RelVel = StringToVec( Params[1] );
				}

				Vector vForward, vRight, vUp;
				EngineFunc::MakeVectors( Angle, vForward, vRight, vUp );

				Vector Final = vRight * RelVel.x + vForward * RelVel.y + vUp * RelVel.z;

				RETURN_VECTOR( Final )
			}
			else return "0";
		}
		else if( ParserName == "$anim_exists" )
		{
			Name = FullName.substr( 13 );
			Name = Name.thru_char( ")" );
			if( m.pScriptedEnt->IsMSMonster() && ((CBaseAnimating *)m.pScriptedEnt)->LookupSequence( GetScriptVar(Name) ) > -1 )
				return "1";
			else return "0";
		}
		else if( ParserName == "$get_quest_data" )	//( <target>, <questname> )
		{
			#ifdef VALVE_DLL
				msstring &Name = Params[1];
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;

				if( pEntity && pEntity->IsPlayer( ) )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					 for (int i = 0; i < pPlayer->m_Quests.size(); i++) 
						if( pPlayer->m_Quests[i].Name == Name )
							return pPlayer->m_Quests[i].Data;
				}
			#endif
			return "0";
		}
		else if( ParserName == "$item_exists" )	//( <target>, <itemname>, <flags> )
		{
			#ifdef VALVE_DLL
				msstring &Name = Params[1];
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;

				if( pEntity && pEntity->IsPlayer( ) )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					getitem_t ItemDesc;
					clrmem( ItemDesc );

					ItemDesc.Name = Params[1];
					msstring &Flags = Params[2];

					if( Flags.contains("nohands") ) ItemDesc.IgnoreHands = true;
					if( Flags.contains("noworn") ) ItemDesc.IgnoreWornItems = true;
					if( Flags.contains("notinpack") ) ItemDesc.IgnoreInsideContainers = true;
					if( Flags.contains("partialname") ) ItemDesc.CheckPartialName = true;

					bool Found = pPlayer->GetItem( ItemDesc );
					if( Found ) RETURN_TRUE
				}
			#endif
			return "0";
		}
		/*else if( ParserName == "$gold_exists" )	//( <target>, <amount> )
		{
			#ifdef VALVE_DLL
				msstring &Name = Params[1];
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;

				if( pEntity && pEntity->IsPlayer( ) )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					if( (int)pPlayer->m_Gold > atoi(Params[1]) ) RETURN_TRUE
				}
			#endif
			return "0";
		}*/
		else if( ParserName == "$get_sky_height" )
		{
			if( Params.size() >= 1 )
			{
				float SkyHeight = 0;
				bool FoundSky = FindSkyHeight( StringToVec( GetScriptVar( Params[0] ) ), SkyHeight );
				if( FoundSky ) RETURN_FLOAT( SkyHeight );
				return "none";
			}
		}
		else if( ParserName == "$get_ground_height" )
		{
			if( Params.size() >= 1 )
			{
				Vector StartOrigin = StringToVec( GetScriptVar( Params[0] ) );
				Vector EndOrigin = StartOrigin + Vector(0,0,-8092);

				sharedtrace_t Tr;
				EngineFunc::Shared_TraceLine( StartOrigin, EndOrigin, ignore_monsters, Tr, 0, PM_GLASS_IGNORE|PM_WORLD_ONLY, NULL );

				if( Tr.Fraction < 1.0 )
					RETURN_FLOAT( Tr.EndPos.z )
				return "none";
			}
		}
		else if( ParserName == "$get_skill_ratio" )
		{
			if( Params.size() >= 3 )
			{
				float Ratio = atof( Params[0] );
				float Min = atof( Params[1] ), Max = atof( Params[2] );

				if( Params.size() >= 4 )
				{
					if( Params[3] == "inversed" ) Ratio = 1-Ratio;
				}

				float Range = Max - Min;
				float Value = Min + Range * Ratio;
				RETURN_FLOAT( Value )
			}
		}
		else if( ParserName == "$get"					//$get( <entity>, <property>, [options] )
				  || ParserName == "$get_by_idx" ) 		//$get( <entity idx>, <property>, [options] )
		{
			if( Params.size() >= 2 )
			{
				msstring &TargetVar = Params[0];
				msstring &ValueVar = Params[1];

				if( ValueVar == "is_entity_type" ) return TargetVar.find(ENT_PREFIX) != msstring_error ? "1" : "0";

				if( m.pScriptedEnt )
				{
					CBaseEntity *pEntity = NULL;
					if( FullName.starts_with("$get_by_idx(") )
						pEntity = MSInstance( INDEXENT(atoi(TargetVar)) );
					else
						pEntity = m.pScriptedEnt->RetrieveEntity( TargetVar );

					if( pEntity )
						return (Return = m.pScriptedEnt->GetProp( pEntity, ValueVar, Params ));
				}
			}
			return "0";
		}
		else if( ParserName == "$getcl" )				//$getcl( <client entity idx>, <property> )
		{
			#ifndef VALVE_DLL
				if( Params.size() >= 2 )
				{
					cl_entity_t *pclEntity = NULL;
					int EntIdx = atoi(Params[0]);
					if( EntIdx > 0 )
					{
						 for (int i = 0; i < CLPERMENT_TOTAL; i++) 
							if( EntIdx == MSCLGlobals::CLViewEntities[i].index )
								{ pclEntity = &MSCLGlobals::CLViewEntities[i]; break; }

						if( !pclEntity ) pclEntity = gEngfuncs.GetEntityByIndex( EntIdx );
					}
					if( pclEntity ) return (Return = CLGetEntProp( pclEntity, Params ));
				}
				return "0";
			#endif
		}
		else if( ParserName == "$get_by_name" )
		{
			#ifdef VALVE_DLL
			if( Params.size() >= 1 )
			{
				CBaseEntity *pEntity = UTIL_FindEntityByString( NULL, "netname", msstring("¯") + Params[0] );
				if( pEntity ) return EntToString(pEntity);
			}
			#endif
			return "0";
		}
	}
	else if( FullName.starts_with("const.") )
	{
		msstring PropName = FullName.substr( 6 );

		if( PropName.starts_with("movetype.") )	//eventually move sound to here
		{
			PropName = PropName.substr( 9 );
			if( PropName == "none" )				RETURN_INT( MOVETYPE_NONE )
			else if( PropName == "walk" )			RETURN_INT( MOVETYPE_WALK )
			else if( PropName == "step" )			RETURN_INT( MOVETYPE_STEP )
			else if( PropName == "fly" )			RETURN_INT( MOVETYPE_FLY )
			else if( PropName == "toss" )			RETURN_INT( MOVETYPE_TOSS )
			else if( PropName == "push" )			RETURN_INT( MOVETYPE_PUSH )
			else if( PropName == "noclip" )			RETURN_INT( MOVETYPE_NOCLIP )
			else if( PropName == "flymissle" )		RETURN_INT( MOVETYPE_FLYMISSILE )
			else if( PropName == "bounce" )			RETURN_INT( MOVETYPE_BOUNCE )
			else if( PropName == "bouncemissle" )	RETURN_INT( MOVETYPE_BOUNCEMISSILE )
			else if( PropName == "follow" )			RETURN_INT( MOVETYPE_FOLLOW )
			else if( PropName == "pushstep" )		RETURN_INT( MOVETYPE_PUSHSTEP )
		}
		else if( PropName.starts_with("snd.") )	//eventually move sound to here
		{
			PropName = PropName.substr( 4 );
			if( PropName == "auto_channel" )		RETURN_INT( CHAN_AUTO )
			else if( PropName == "weapon" )			RETURN_INT( CHAN_WEAPON )
			else if( PropName == "voice" )			RETURN_INT( CHAN_VOICE )
			else if( PropName == "item" )			RETURN_INT( CHAN_ITEM )
			else if( PropName == "body" )			RETURN_INT( CHAN_BODY )
			else if( PropName == "stream" )			RETURN_INT( CHAN_STREAM )
			else if( PropName == "static" )			RETURN_INT( CHAN_STATIC )
			else if( PropName == "fullvol" || PropName == "maxvol" )	RETURN_INT( 10 )
			else if( PropName == "slientvol" || PropName == "novol" )	RETURN_INT( 0 )
		}
		else if( PropName.starts_with("localplayer.") )	//eventually move sound to here
		{
			PropName = PropName.substr( 12 );
			if( PropName == "scriptID" )		RETURN_INT( PLAYER_SCRIPT_ID )
		}
	}
	else if( FullName.starts_with("game.") )
	{
		Name = FullName.substr( 5 );

		bool fSuccess = false, ValidCmd = true;
		bool IsServer = 
		#ifdef VALVE_DLL
				true;
		#else
				false;
		#endif

		if( Name == "time" ) RETURN_FLOAT( gpGlobals->time )
		else if( Name == "frametime" ) RETURN_FLOAT( gpGlobals->frametime )
		#ifdef VALVE_DLL
			else if( Name == "players" )  RETURN_INT( CountPlayers() )
		#endif
		else if( Name.starts_with("map.") )
		{
			bool Type[2] = { false };
			msstring Prop = Name.substr( 4 );
			if( Prop == "name" )			return MSGlobals::MapName;
			else if( Prop == "skyname" )	return EngineFunc::CVAR_GetString( "sv_skyname" );
			/*else if( (Type[0] = Prop.starts_with("absmin")) || (Type[1] = Prop.starts_with("absmax")) )	
			{
				Vector Bounds[2];
				#ifdef VALVE_DLL
					CBaseEntity *pWorld = MSInstance( ENT( 0 ) );
					
					if( pWorld ) 
					{

						GetModelBounds( GET_MODEL_PTR( pWorld->edict() ), Bounds );
					}
					else RETURN_ZERO;
				#else
					cl_entity_t *pclWorld = gEngfuncs.GetEntityByIndex( 0 );
					if( pclWorld )
					{
						if( Type[0] ) Bounds[0] = pclWorld->origin + pclWorld->curstate.mins;
						if( Type[1] ) Bounds[1] = pclWorld->origin + pclWorld->curstate.maxs;
					}
					else RETURN_ZERO;
				#endif

				if( Type[0] ) RETURN_POSITION( "absmin", Bounds[0] )
				if( Type[1] ) RETURN_POSITION( "absmax", Bounds[1] )
			}*/
		}
		else if( Name == "clientside" )  fSuccess = !IsServer;
		else if( Name == "serverside" )  fSuccess = IsServer;
		else if( Name.starts_with("script.") )
		{
			msstring Property = Name.substr( 7 );
			if( Property == "last_sent_id" )		RETURN_INT( m_gLastSendID )
			else if( Property == "last_light_id" )  RETURN_INT( m_gLastLightID )
			else if( Property == "iteration" )		RETURN_INT( m.m_Iteration )
		}
		else if( Name.starts_with("sound.") )
		{
			//ONLY STILL HERE FOR BACKWARDS COMPATIBILITY
			//MOVED TO --> game.const.snd.x
			msstring SoundProp = Name.substr( 6 );
			if( SoundProp == "auto_channel" )			RETURN_INT( CHAN_AUTO )
			else if( SoundProp == "weapon" )			RETURN_INT( CHAN_WEAPON )
			else if( SoundProp == "voice" )				RETURN_INT( CHAN_VOICE )
			else if( SoundProp == "item" )				RETURN_INT( CHAN_ITEM )
			else if( SoundProp == "body" )				RETURN_INT( CHAN_BODY )
			else if( SoundProp == "stream" )			RETURN_INT( CHAN_STREAM )
			else if( SoundProp == "static" )			RETURN_INT( CHAN_STATIC )
			else if( SoundProp == "fullvol" || SoundProp == "maxvol" )	RETURN_INT( 10 )
			else if( SoundProp == "slientvol" || SoundProp == "novol" )	RETURN_INT( 0 )
		}
		else
		{
			static msstringlist Params;
			Params.clearitems();
			TokenizeString( Name, Params, "." );
			Name = Params[0];
			msstring FullProp = &FullName[ 5 + Name.len()+1 ];
			msstring_ref Value = RETURN_NOTHING_STR;
			if( ( Name == "entity" || Name == "monster" || Name == "player" || Name == "item" || Name == "container" ) && m.pScriptedEnt )
			{
				Value = m.pScriptedEnt->GetProp( m.pScriptedEnt, FullProp, Params );
			}
			else if( Name == "pmove" )
			{
				Value = PM_GetValue( Params );
			}
			#ifndef VALVE_DLL
			else if( Name == "localplayer" )
			{
				
				msstring &Prop = FullProp;
				if( Prop == "index" ) RETURN_INT( MSCLGlobals::GetLocalPlayerIndex( ) )
				else if( Prop == "thirdperson" ) RETURN_INT( (MSCLGlobals::CamThirdPerson ? 1 : 0) )
				else if( Prop == "viewmodel.left.id"  || Prop == "viewmodel.0.id" ) RETURN_INT( player.Hand( 0 ) ? player.Hand( 0 )->GetViewModelID( ) : -1 )
				else if( Prop == "viewmodel.right.id" || Prop == "viewmodel.1.id" ) RETURN_INT( player.Hand( 1 ) ? player.Hand( 1 )->GetViewModelID( ) : -1 )
				else if( Prop == "viewmodel.active.id" ) RETURN_INT( player.ActiveItem( ) ? player.ActiveItem( )->GetViewModelID( ) : -1 )
			}
			else if( Name == "tempent" ) 
				Value = CLGetCurrentTempEntProp( FullProp );
			#endif

			else ValidCmd = false;

			if( ValidCmd )
			{
				if( strcmp(Value,RETURN_NOTHING_STR) ) return Value;
				else ValidCmd = false;
			}
		}
		
		if( ValidCmd )
			return fSuccess ? "1" : "0";
	}
	else if( FullName[0] == '\'' && FullName[FullName.len()-1] == '\'' )	//String literal.  Don't try to resolve it
	{
		Return = FullName.substr(1).thru_char( "'" );
		return Return;
	}
	scriptvar_t *pScriptvar = FindVar( pszText );
	if( pScriptvar ) return pScriptvar->Value;

	return pszText;
}

Vector CScript::StringToVec( msstring_ref String )
{
	msstring Inside = SCRIPTVAR( msstring(String).thru_char( ")" ) );

	msstring X = SCRIPTVAR(Inside.substr(1).thru_char( " ," ));
	Inside = msstring( Inside.findchar_str( " ," ) ).skip( " ," );
	msstring Y = SCRIPTVAR(Inside.thru_char( " ," ));
	Inside = msstring( Inside.findchar_str( " ," ) ).skip( " ," );
	msstring Z = SCRIPTVAR(Inside.thru_char( " )" ));

	return Vector( atof(X), atof(Y), atof(Z) );
}


scriptvar_t *CScript::SetVar( const char *pszVarName, const char *pszValue, bool fGlobal ) 
{
	scriptvar_t *pScriptvar = FindVar( pszVarName );
	if( pScriptvar )	//Found existing variable
		pScriptvar->Value = pszValue;
	else
	{					//Create a new variable
		scriptvar_t NewVar( pszVarName, pszValue );
		if( !fGlobal ) { IVariables::SetVar( pszVarName, pszValue ); }
		else m_gVariables.add( NewVar );
	}
	return pScriptvar;
}
scriptvar_t *CScript::SetVar( const char *pszVarName, const char *pszValue, SCRIPT_EVENT &Event ) 
{
	scriptvar_t *pScriptvar = Event.FindVar( pszVarName );
	if( pScriptvar )	//Found existing local variable
		pScriptvar->Value = pszValue;
	else pScriptvar = SetVar( pszVarName, pszValue ); //Didn't find a local variable, so set it as a regular variable
	
	return pScriptvar;
}
scriptvar_t *CScript::SetVar( const char *pszVarName, int iValue, bool fGlobal ) {
	char ctemp[64];
	itoa( iValue, ctemp, 10 );
	return SetVar( pszVarName, ctemp, fGlobal );
}
scriptvar_t *CScript::SetVar( const char *pszVarName, float flValue, bool fGlobal ) {
	char ctemp[64];
	_gcvt( flValue, 10, ctemp );
	return SetVar( pszVarName, ctemp, fGlobal );
}
// Called when creating a new item from a global template
void CScript::CopyAllData( CScript *pDestScript, CBaseEntity *pScriptedEnt, IScripted *pScriptedInterface ) {
	//I am the source script

	pDestScript->m = m;
	pDestScript->m.pScriptedEnt = pScriptedEnt;
	pDestScript->m.pScriptedInterface = pScriptedInterface;
	//pDestScript->m.Events = m.Events;

	//Copy Variables
	int variables = m_Variables.size();
	 for (int i = 0; i < variables; i++) 
		pDestScript->m_Variables.add( m_Variables[i] );
}
float GetNumeric( const char *pszText ) {
	return atof(pszText);
}
msstring_ref CScript::GetScriptVar( msstring_ref VarName )
{
	return SCRIPTVAR( VarName );
}


CScript::CScript( )  
{ 
	clrmem( m ); 
	Script_Setup( );

	ScriptMgr::RegisterScript( this );
}
CScript::~CScript( ) 
{
	ScriptMgr::UnRegisterScript( this );

	m_Variables.clear( );
	m_Constants.clear( );
}


bool CScript::Spawn( string_i Filename, CBaseEntity *pScriptedEnt, IScripted *pScriptedInterface, bool PrecacheOnly, bool Casual )
{
	//Keep track of all #included files... don't allow #including the same file twice
	//Update: A script can specify when it wants to allow duplicate includes
	if( !m.AllowDupInclude )
		 for (int i = 0; i < m_Dependencies.size(); i++) 
			if( !stricmp(m_Dependencies[i],Filename) )
				return true;	//Return true here, so its a 'fake' successful.  This should only happen on #includes
	m_Dependencies.add( Filename );

	//Localize these for later reference
	m.pScriptedEnt = pScriptedEnt;
	m.pScriptedInterface = pScriptedInterface;
	m.PrecacheOnly = PrecacheOnly;
	m.ScriptFile = Filename;
	m.DefaultScope = EVENTSCOPE_SHARED;
	m.AllowDupInclude = false;
	m.Included = m_Dependencies.size() >= 2;
	bool fReturn = false;


	//This should always be true for non-dev builds, because it must use the script library
	char *ScriptData;
	msstring ScriptName = m.ScriptFile;
	ScriptName += SCRIPT_EXT;

	#ifndef SCRIPT_LOCKDOWN
		char cScriptFile[MAX_PATH];
		sprintf( cScriptFile, "scripts/%s", ScriptName.c_str() );

		int iFileSize;
		byte *pMemFile = LOAD_FILE_FOR_ME( cScriptFile, &iFileSize );
		if( pMemFile ) {
			ScriptData = msnew(char[iFileSize+1]);
			memcpy( ScriptData, pMemFile, iFileSize );
			ScriptData[iFileSize] = 0;
			FREE_FILE( pMemFile );
		}
		else
		{
	#endif

			unsigned long ScriptSize;
			if( ScriptMgr::m_GroupFile.ReadEntry( ScriptName, NULL, ScriptSize ) )
			{
				ScriptData = msnew(char[ScriptSize+1]);
				ScriptMgr::m_GroupFile.ReadEntry( ScriptName, (byte *)ScriptData, ScriptSize );
				ScriptData[ScriptSize] = 0;
			}
			else 
			{					
				if( !Casual )
				{
					#ifndef SCRIPT_LOCKDOWN
						#ifdef VALVE_DLL
							CSVGlobals::LogScript( ScriptName, m.pScriptedEnt, m_Dependencies.size(), m.PrecacheOnly, false );
						#endif
							ALERT( at_console, "Script file: \"%s\" NOT FOUND!\n", ScriptName.c_str() );
					#else
						#ifndef RELEASE_LOCKDOWN
							logfile << "FATAL ERROR: Script not found: " << ScriptName.c_str() << endl;
							MessageBox( NULL, msstring("Script not found: ") + ScriptName + "\r\n\r\nThis is probably caused by a script using #include on a non-existant script.", "FIX THIS QUICK!", MB_OK );
						#else
							//In the release build, this is a fatal error
							//SERVER_COMMAND( "exit\n" ); This crashes the game, currently
							exit( 0 );
						#endif
					#endif
				}
				return false;
			}
	#ifndef SCRIPT_LOCKDOWN
		}
	#endif

	#ifdef VALVE_DLL
		CSVGlobals::LogScript( ScriptName, m.pScriptedEnt, m_Dependencies.size(), m.PrecacheOnly, true );
	#endif

	if( m.pScriptedInterface ) m.pScriptedInterface->Script_Setup( );
	fReturn = ParseScriptFile( ScriptData );	//Parse events

	delete ScriptData;  //Deallocate script data

	RunScriptEventByName( "game_precache" );	//Run precache event
	
	return fReturn;
}

void CScript::RunScriptEvents( bool fOnlyRunNamedEvents )
{
	//Run script events
	//~ Runs unnamed events or named events that were specified with calleventtimed ~
	int events = m.Events.size();
	 for (int i = 0; i < events; i++) 
	{
		SCRIPT_EVENT &Event = m.Events[i];

		//Skip unnamed events when running named events only
		if( !Event.Name && fOnlyRunNamedEvents )
			continue;

		//Run an event, if it's time
		mslist<float> CachedExecutions;

		if( Event.fNextExecutionTime > -1 && gpGlobals->time >= Event.fNextExecutionTime )
		{
			CachedExecutions.add( Event.fNextExecutionTime );
			Event.fNextExecutionTime = -1;
		}


		 for (int e = 0; e < Event.TimedExecutions.size(); e++) 
		{
			if( gpGlobals->time < Event.TimedExecutions[e] )
				continue;

			CachedExecutions.add( Event.TimedExecutions[e] );

			Event.TimedExecutions.erase( e-- );
		}

		 for (int e = 0; e < CachedExecutions.size(); e++) 
			Script_ExecuteEvent( Event );
	}
}
void CScript::RunScriptEventByName( msstring_ref pszEventName, msstringlist *Parameters ) 
{
	SCRIPT_EVENT *CurrentEvent = m.CurrentEvent; //Save the event currently executing

	//Run every event with this name
	 for (int i = 0; i < m.Events.size(); i++) 
	{
		SCRIPT_EVENT &seEvent = m.Events[i];
		if( !seEvent.Name || seEvent.Name != pszEventName )
			continue;

		Script_ExecuteEvent( seEvent, Parameters );
	}

	m.CurrentEvent = CurrentEvent;	//Restore the current event
}

bool CScript::ParseScriptFile( const char *pszScriptData )
{
	startdbg;

	dbg( "Begin" );
	if( !m.ScriptFile.len() || !pszScriptData )
		return false;

#ifdef MSDEBUG
	ALERT( at_console, "Loading Script file: %s...\n", STRING(ScriptFile) );
#endif

	SCRIPT_EVENT *CurrentEvent = NULL;			//Current event
	scriptcmd_list *CurrentCmds = NULL;			//Current command list within event
	mslist<scriptcmd_list *> ParentCmds;		//List of all my parent command lists, top to bottom
	int LineNum = 1;
	char cSpaces[128];
	while( *pszScriptData ) 
	{
		char cBuf[768];
		cBuf[0] = 0;
		if( GetString( cBuf, pszScriptData, 0, "\r\n" ) )
			pszScriptData += strlen( cBuf );
		else pszScriptData += strlen( cBuf );

		// skip over the carriage return
		if( sscanf( pszScriptData, "%2[\r\n]", cSpaces ) > 0 )
			pszScriptData += strlen(cSpaces);

		#define BufferPos &cBuf[ofs]

		int ofs = 0;
		do
		{
			//Read the spaces at the beginning of the line, if any
			if( sscanf( BufferPos, "%[ \t]", cSpaces ) > 0 )
				ofs += strlen(cSpaces);

			//Exclude the end-of-line comments
			char *pszCommentsStart = strstr(cBuf,"//");
			if( pszCommentsStart )
				*pszCommentsStart = 0;

			//If its a comment or empty line, break out
			if( !*BufferPos || sscanf( BufferPos, "%[/]%[/]", cSpaces, cSpaces ) > 0 )
				break;

			//Parse this line and store any commands
			//ParseLine() updates CurrentEvent
			int ret = ParseLine( BufferPos, LineNum, &CurrentEvent, &CurrentCmds, ParentCmds );
		}
		while( 0 );
		
		LineNum++;
	}

	if( MSGlobals::IsServer && m.ScriptFile == "items/smallarms_rknife" )
		int stop = 0;

	if( MSGlobals::IsServer && m.ScriptFile == "items/bows_longbow" )
		int stop = 0;

	enddbg( "CSript::ParseScriptFile()" );
//  Uncomment to print out all this function gathered from the script file
//#ifndef VALVE_DLL
	/*SCRIPT_EVENT *pEvent = seFirstEvent;
	CHAR *pszPtr;
	ALERT( at_console, "\nScript: %s\n", pScriptedEnt->pev?STRING(pScriptedEnt->pev->classname):"new script" );
	while( pEvent ) {
		pszPtr = pEvent->Action;
		if( pEvent->Name ) ALERT( at_console, "Event: %s\n", STRING(pEvent->Name) );
		else ALERT( at_console, "Event: Unnamed\n" );
		for( unsigned int i = 0; i < strlen(pszPtr); i++ ) {
			if( pszPtr[i] == '\r' ) ALERT( at_console, "R" );
			else if( pszPtr[i] == '\n' ) ALERT( at_console, "N" );
			else ALERT( at_console, "%c", pszPtr[i] );
		}
		ALERT( at_console, "\n" );
		pEvent = pEvent->NextEvent;
	}*/
//#endif
	return true;
}
int CScript::ParseLine( const char *pszCommandLine /*in*/, int LineNum /*in*/, SCRIPT_EVENT **pCurrentEvent /*in/out*/, 
					   scriptcmd_list **pCurrentCmds /*in/out*/, mslist<scriptcmd_list *> &ParentCmds /*in/out*/ )
{
	startdbg;

	dbg( "Begin" );

	SCRIPT_EVENT *CurrentEvent = *pCurrentEvent;
	scriptcmd_list &CurrentCmds = **pCurrentCmds;
	
	char TestCommand[128];
	char cBuffer[512];
	int LineOfs = 0, TmpLineOfs = 0;

	#define CmdLine &pszCommandLine[LineOfs]
	#define CmdLineTmp &pszCommandLine[TmpLineOfs]

	dbg( pszCommandLine );

	//Read the first word of the line
	if( sscanf( pszCommandLine, "%s", TestCommand ) <= 0 )
		return 0;

	LineOfs += strlen(TestCommand);

	//Read spaces
	if( sscanf( CmdLine, "%[ \t\n\r]", cBuffer ) > 0 )
		LineOfs += strlen(cBuffer);

	TmpLineOfs = LineOfs;
	msstring Line = CmdLineTmp;
	//ALERT( at_console, "CommandFound: %s\n", TestCommand );

	//Check if this word is a pre-command.
	if( !stricmp(TestCommand,	"{"		) )
	{
		if( !*pCurrentEvent )
		{
			//Create a new Event
			eventscope_e esScope = m.DefaultScope;
			msstring Name;
			bool Override = false;

			msstring Param = Line.thru_char( SKIP_STR );

			//Read options and the event name (all of it is optional and can be in any order)
			while( Param.len() )
			{
				if( Param == "[client]" )		esScope = EVENTSCOPE_CLIENT;
				else if( Param == "[server]" )	esScope = EVENTSCOPE_SERVER;
				else if( Param == "[shared]" )	esScope = EVENTSCOPE_SHARED;
				else if( Param == "[override]" ) Override = true;
				else Name = Param;
		
				Line = Line.substr( Param.len() ).skip( SKIP_STR );
				Param = Line.thru_char( SKIP_STR );
			}

			SCRIPT_EVENT Event;
			clrmem( Event );
			
			Event.fNextExecutionTime = -1;
			Event.fRepeatDelay = -1;
			
			if( Name.len() )
			{
				//Named event.  Don't run until called
				Event.Name = Name;
			}
			//Unnamed event.  Run once and let repeatdelay decide if it gets run after that
			else Event.fNextExecutionTime = 0;		//Run at least once

			Event.Scope = esScope;

			if( Name.len() && Override )						//Delete all previous occurances of this event
				 for (int i = 0; i < m.Events.size(); i++) 
					if( Name == m.Events[i].Name )
						{ m.Events.erase( i ); i--; }

			m.Events.add( Event );
			*pCurrentEvent = &m.Events[m.Events.size()-1];
			*pCurrentCmds = &m.Events[m.Events.size()-1].Commands;
		}
		else
		{
			if( !ParentCmds.size() )
			{
				MSErrorConsoleText( __FUNCTION__, UTIL_VarArgs("Script: %s, Line: %i - %s \"{\" following non conditional command!\n", m.ScriptFile.c_str(), LineNum, TestCommand, cBuffer) );
				return 0;
			}

			//Entering the group of commands within a conditional statement
			CurrentCmds.m_SingleCmd = false;	//Allow multiple commands in this list
		}
		return 1;
	}
	else if( !stricmp(TestCommand,	"#include"		) )
	{	//#include [scope] <name> [allowduplicate]
		//Include another script file
		msstring FileName = Line.thru_char( SKIP_STR );
		eventscope_e Scope = EVENTSCOPE_SHARED;
		bool Casual = false;
	
		if( FileName[0] == '[' )	//#include [scope][casual] <name>
		{
			if( FileName.contains("[server]") ) Scope = EVENTSCOPE_SERVER;
			else if( FileName.contains("[client]") ) Scope = EVENTSCOPE_CLIENT;
			if( FileName.contains("[casual]") ) 
				Casual = true;

			Line = Line.substr( FileName.len() ).skip( SKIP_STR );
			FileName = Line.thru_char( SKIP_STR );
		}

		FileName = GetConst(FileName);

		//Check the scope of this include.  Scope == EVENTSCOPE_SHARED means include the file on both
		if( (Scope == EVENTSCOPE_SHARED) || MSGlobals::IsServer == (Scope==EVENTSCOPE_SERVER) )
		{
			msstring AllowDup = msstring( Line.substr( FileName.len() ) ).skip( SKIP_STR ).thru_char( SKIP_STR );

			string_i CurrentScriptFile = m.ScriptFile;
			bool AllowDupInclude = m.AllowDupInclude;
			m.AllowDupInclude = AllowDup.find( "allowduplicate" ) != msstring_error;
			bool fSucces = Spawn( FileName, m.pScriptedEnt, m.pScriptedInterface, m.PrecacheOnly, Casual );
			m.ScriptFile = CurrentScriptFile;
			m.AllowDupInclude = AllowDupInclude;
			if( !fSucces && !Casual )
			{
				MessageBox( NULL, msstring("Script: ") + m.ScriptFile + " Tried to include non-existant script: " + FileName + "\r\n\r\nThis is a fatal error in the public build.", "FIX THIS QUICK!", MB_OK );
				ALERT( at_console, "Script: %s, Line: %i - %s \"%s\" failed!  Possible File Not Found.\n", m.ScriptFile.c_str(), LineNum, TestCommand, FileName.c_str() );
			}
		}
		return 1;
	}
	else if( !stricmp(TestCommand,	"#scope"		) )
	{
		msstring Scope = Line.thru_char( SKIP_STR );
		if( Scope == "client" )		m.DefaultScope = EVENTSCOPE_CLIENT;
		else if( Scope == "server" )	m.DefaultScope = EVENTSCOPE_SERVER;
		else if( Scope == "shared" )	m.DefaultScope = EVENTSCOPE_SHARED;
		else ALERT( at_console, "Script: %s, Line: %i - %s \"%s\" - Not valid!.\n", m.ScriptFile.c_str(), LineNum, TestCommand, Scope.c_str() );
		return 1;
	}

	if( !CurrentEvent ) 
		{ ALERT( at_console, "Script: %s, Line: %i Missing {\n", m.ScriptFile.c_str(), LineNum ); return 0; }

	//KeepCmd - This is for pre-commands that also function as normal commands
	//			The pre-commands below require that we be inside of an event
	bool KeepCmd = false;

	if( !stricmp(TestCommand,	"}"		) )
	{
		if( !ParentCmds.size() )
		{
			//Done with event
			*pCurrentEvent = NULL;
			*pCurrentCmds = NULL;

			//If Event wasn't in the right scope, delete it now
			if( m.Events.size( ) && m.Events[m.Events.size()-1].Scope ==
				#ifdef VALVE_DLL
					EVENTSCOPE_CLIENT
				#else
					EVENTSCOPE_SERVER
				#endif
					)
			{
				CurrentEvent = NULL;
				m.Events.erase( m.Events.size() - 1 );
			}
		}
		else
		{
			//Done with conditional code block
			//return control to the parent command list
			*pCurrentCmds = ParentCmds[ParentCmds.size()-1];
			ParentCmds.erase( ParentCmds.size() - 1 );
		}
	}
	else if( !stricmp(msstring(TestCommand).substr(0,2),"if") )
	{
		//This could be a new-style if or an old-style if
		if( !strstr(TestCommand,"(") && *CmdLineTmp != '(' ) //Old: if var == var
			KeepCmd = true;
		else
		{
			/*
				New:	if( var == var ) command
				if( var == var )
				{ 
					command1
					command2
				} 
			*/

			//Create the command
			scriptcmd_t &ScriptCmd = CurrentCmds.m_Cmds.add( scriptcmd_t( "if()", true ) );	//Change the command name to "if()
			ScriptCmd.m_NewConditional = true;

			//Add all the command's parameters
			if( *CmdLineTmp == '(' ) TmpLineOfs++;											//Go past the '('
			msstring ParamStr = msstring( CmdLineTmp ).skip( SKIP_STR );					//Skip spaces
			 for (int i = 0; i < 3; i++) 
			{
				ScriptCmd.m_Params.add( GetConst(ParamStr.thru_char( SKIP_STR )) );			//Save the next parameter - Resolve Contants but not variables
				ParamStr = msstring( ParamStr.findchar_str( SKIP_STR ) ).skip( SKIP_STR );	//Skip over the parameter's text and any spaces
				if( !i && ParamStr[0] == ')' )																				
					break;																	//Compare parameter was ')' -- this if statement only has one parameter Ex: if( var ) command
				
			}

			if( ParamStr[0] == ')' )
			{
				ParentCmds.add( *pCurrentCmds );											//Store the current commands list
				*pCurrentCmds = &ScriptCmd.m_IfCmds;										//Set the new parent command list to my true statment child list
				(*pCurrentCmds)->m_SingleCmd = true;										//Default to one command only.  If I hit a '{' first, then allow a block of commands

				//Check if there are any commands at the end of the if line and parse them under this if statement
				ParamStr = msstring( ParamStr.findchar_str( SKIP_STR ) ).skip( SKIP_STR );	//Skip the ')' and any spaces
				
				if( !ParamStr[0] || ParamStr[0] == ')' )
					return 2;  //Return 2 so any parent command knows I'm not done yet
				else
				{
					//There is a command at the end of this line
					if( ParseLine( ParamStr, LineNum, pCurrentEvent, pCurrentCmds, ParentCmds ) == 2 )	//Parse the command at the end of the line
						return 2;	//I found a conditional at the end of this line that isn't done yet... so let it finish parsing
					//...I found a non-conditional or a conditional which only had one line... it drop out and escape to my parent
				}
			}
			else 
				MSErrorConsoleText( "SCript::ParseLine()", msstring("Script: ") + m.ScriptFile.c_str() + " Line: " + LineNum + " - if() statement missing ')'!\n" );
		}
	}
	else if( !stricmp(TestCommand,	"else"			) )
	{
		scriptcmd_t &ParentCmd = CurrentCmds.m_Cmds[CurrentCmds.m_Cmds.size()-1];
		if( ParentCmd.m_Conditional )
		{
			ParentCmd.m_AddingElseCmds = true;											//If I reach a '{', I know its for else commands and not if commands

			ParentCmds.add( *pCurrentCmds );											//Store the current commands list
			*pCurrentCmds = &ParentCmd.m_ElseCmds.add( scriptcmd_list() );				//Set the new parent command list to my new else child list
			(*pCurrentCmds)->m_SingleCmd = true;										//Default to one command only.  If I hit a '{' first, then allow a block of commands

			if( *CmdLineTmp && !isspace( *CmdLineTmp ) )
			{
				//There is a command at the end of this line
				if( ParseLine( CmdLineTmp, LineNum, pCurrentEvent, pCurrentCmds, ParentCmds ) == 2 )	//Parse the command at the end of the line
					return 2;	//I found a conditional at the end of this line that isn't done yet... so let it finish parsing
				//...I found a non-conditional or a conditional which only had one line... it drop out and escape to my parent
			}
			else
				return 2; //Return 2 so any parent command knows I'm not done yet
		}
		else MSErrorConsoleText( "CSript::ParseLine", UTIL_VarArgs("Script: %s, Line: %i - %s \"else\" following non conditional command!\n", m.ScriptFile.c_str(), LineNum, TestCommand, cBuffer) );
	}
	else if( !stricmp(TestCommand,	"eventname"		) )
	{
		//Set a name for the current event
		sscanf(CmdLineTmp, "%s", cBuffer );
		CurrentEvent->Name = cBuffer;
		CurrentEvent->fNextExecutionTime = -1;
		CurrentEvent->fRepeatDelay = -1;
	}
	else if( !stricmp(TestCommand,	"repeatdelay"		) )
	{
		//Set a delay timer for the current event
		sscanf( CmdLineTmp, "%s", cBuffer );
		CurrentEvent->fRepeatDelay = atof(SCRIPTCONST(cBuffer));
		CurrentEvent->fNextExecutionTime = gpGlobals->time + CurrentEvent->fRepeatDelay;
		KeepCmd = true;
	}
	else if( !stricmp(TestCommand,	"setvar"		) ||
			 !stricmp(TestCommand,	"const"			) ||
			 !stricmp(TestCommand,	"const_ovrd"	) )
	{
		if( CurrentEvent->Scope != 
			#ifdef VALVE_DLL
				EVENTSCOPE_CLIENT
			#else
				EVENTSCOPE_SERVER
			#endif
				)
		{
			//Set variable value.  "setvarg" sets global variable
			msstring VarName = Line.thru_char( SKIP_STR );

			Line = Line.substr( VarName.len() ).skip( SKIP_STR );
			msstring VarValue;
			if( Line[0] == '"' ) VarValue = Line.substr(1).thru_char( "\"" );		//Starting quote found, read until the next quote
			else VarValue = Line.thru_char( SKIP_STR );				//No quotes

			VarValue = msstring(GETCONST_COMPATIBLE(VarValue));				//Resolve both constants and variables -- TODO: Remove the variables - should be constants only
			if( !stricmp(TestCommand,"setvar" ) )
			{
				SetVar( VarName, VarValue, !stricmp(TestCommand, "setvarg") ? true : false );
				KeepCmd = true;
			}
			else {
				//'const_ovrd' re-sets the constant even if already set
				bool AddConst = true;

				if( stricmp(TestCommand, "const" ) )			//Only check this if not 'const_ovrd'		
					 for (int i = 0; i < m_Constants.size(); i++) 				
						if( m_Constants[i].Name == VarName )
							{ AddConst = false; break; }		//Constant aready exists
				if( AddConst )
					m_Constants.add( scriptvar_t( VarName, VarValue ) );//Create new constant
			}
		}
	}
	else if( !stricmp(TestCommand,	"removeconst"		) )
	{
		msstring VarName = Line.thru_char( SKIP_STR );

		foreach( i, m_Constants.size() )						//Update constant
			if( m_Constants[i].Name == VarName )
				{ 
					m_Constants.erase( i ); 
					break; }
	}
	else if( !stricmp(TestCommand,	"setvard"		) )
	{
		//Really just a setvar that skips the loadtime execution
		strcpy( TestCommand, "setvar" );
		KeepCmd = true;
	}
	/*else if( !stricmp(TestCommand,	"precachefile"		) )
	{
		//precache another script file
		sscanf( CmdLineTmp, "%[^\t \r\n]", cBuffer );
		strcpy( cBuffer, SCRIPTCONST(cBuffer) );
		strcat( cBuffer, SCRIPT_EXT );
		string_t CurrentScriptFile = m.ScriptFile;
		CScript TempScript;
		bool fSucces = TempScript.Spawn( ALLOC_STRING(cBuffer), NULL, NULL, true );
		if( !fSucces )
			ALERT( at_console, "Script: %s, Line: %i - %s \"%s\" failed!  Possible File Not Found.\n", m.ScriptFile.c_str(), LineNum, TestCommand, cBuffer );
		return 1;
	}*/
	else if( !stricmp(TestCommand,	"setmodel"			) || 
			 !stricmp(TestCommand,	"setviewmodel"		) || 
			 !stricmp(TestCommand,	"setworldmodel"		) || 
			 !stricmp(TestCommand,	"setpmodel"			) || 
			 !stricmp(TestCommand,	"setshield"			) || 
			 !stricmp(TestCommand,	"attachsprite"		) || 
			 !stricmp(TestCommand,	"playsound"			) || 
			 !stricmp(TestCommand,	"playsoundcl"		) || 
			 !stricmp(TestCommand,	"playrandomsound"	) ||
			 !stricmp(TestCommand,	"sound.play3d"		) ||
			 !stricmp(TestCommand,	"precache"			) ||
			 !stricmp(TestCommand,	"say"				) 
		    )
	{
	#ifdef VALVE_DLL
		char cSpaces[256] = "";
		msstringlist Resources;
		enum { pctype_model, pctype_sound, pctype_sprite } Precachetype;

		int ResourceIdx = 0;
		char *pSearchLine = "%s";
		cBuffer[0] = 0;
		int SndType = 0;
		if( !stricmp(TestCommand,"sound.play3d") ) SndType = 1;	//Sound name is first parameter

		while( sscanf( CmdLineTmp, pSearchLine, cSpaces, cBuffer ) > 0 ) //The first time preceding spaces aren't checked and the result is in cSpaces
		{
			TmpLineOfs += strlen(cSpaces) + strlen(cBuffer);
			if( !ResourceIdx ) strcpy( cBuffer, cSpaces );		//The first parameter ends up in cSpaces.... move it to cBuffer
			bool SkipFirst = (!SndType && strstr(TestCommand,"sound")) ? true : false;


			if( !SkipFirst || ResourceIdx )						//Sounds skip the first parameter
			{
				msstring Resolved = SCRIPTCONST(cBuffer);		//For now, resolve both consts and vars
																//Later, once the scripts have been updated remove vars
				if( !strcmp(TestCommand,"say") )
				{
					Resolved = Resolved.thru_char("[");		//Special case for 'say'.  Get the sound separate from the delay
					if( Resolved == "*" ) { Resolved = ""; ALERT( at_console, "%s Warning: Old 'say' command using '*' as sound name\n", m.ScriptFile.c_str() ); }
					if( Resolved.len() ) Resolved = msstring("npc/") + Resolved + ".wav";
				}
				if( Resolved.len() && strstr(Resolved,".") )						
					Resources.add( Resolved );
			}
			
			ResourceIdx++;

			if( strstr(TestCommand,"sprite") || strstr(TestCommand,"model") || strstr(TestCommand,"setshield") || SndType == 1 ) break;	//If a sprite or model, only use the first parameter
			pSearchLine = "%[ \t\r\n]%s";
		}

		 for (int i = 0; i < Resources.size(); i++) 
		{
			msstring &FileName = Resources[i];

			if( FileName == "none" ) continue;

			if( FileName.len() < 4 )
			{
				Print( "Found precache with name too small: %s\n", FileName.c_str() );
				continue;
			}

			msstring Extension = &FileName[FileName.len()-4];
			if( Extension == ".wav" ) Precachetype = pctype_sound;
			else if( Extension == ".mdl" ) 
				Precachetype = pctype_model;
			else if( Extension == ".spr" ) 
				Precachetype = pctype_sprite;
			else { /*Print( "Found unknown preache: %s\n", FileName.c_str() );*/ continue; }

			msstring Dirname;
			Dirname = (Precachetype==pctype_model) ? "models/" : (Precachetype==pctype_sprite) ? "sprites/" : "";
			msstring Fullpath = Dirname + FileName;

			//char *pszDLLString = (char *)STRING(ALLOC_STRING(Fullpath));
			//pszDLLString = Fullpath;
			static msstring Precaches[16384] = { "" };
			static int PrecachesTotal = 0;

			char *pszGlobalPointer = NULL;			//Precaches MUST be global pointers.  Can't use stack memory
			 for (int p = 0; p < PrecachesTotal; p++) 
				if( Fullpath == Precaches[p] )
					{ 
						pszGlobalPointer = Precaches[p]; 
						break;
					}
			if( !pszGlobalPointer )
				pszGlobalPointer = (Precaches[PrecachesTotal++] = Fullpath).c_str();

			//if( Fullpath.contains( "human" ) )
			//	int stop = 0;

			switch( Precachetype )
			{
				case pctype_sound: 
					PRECACHE_SOUND( pszGlobalPointer );  break;
				case pctype_sprite: 
				case pctype_model: 
					PRECACHE_MODEL( pszGlobalPointer );  break;
			}
		}
	#endif
		if( stricmp(TestCommand, "precache") ) //Don't keep the precache command
			KeepCmd = true;
	}
	else KeepCmd = true; //Normal command

	if( CurrentEvent &&
		CurrentEvent->Scope ==		//Don't keep any commands if in the wrong scope.
		#ifdef VALVE_DLL			//This also prevents 'not found' errors from server-only commands found on the client
			EVENTSCOPE_CLIENT
		#else
			EVENTSCOPE_SERVER
		#endif
			) KeepCmd = false;

	if( !KeepCmd || m.PrecacheOnly )
		goto DontKeepCommand;	//Don't keep this command
	
	{
		//Check if this word is a command
		scriptcmdname_t Command( TestCommand );
		bool fFoundCmd = false;

		 for (int icommand = 0; icommand < m_GlobalCmds.size(); icommand++) 
		{
			scriptcmdname_t &Checkcmd = m_GlobalCmds[icommand];
			if( !stricmp(TestCommand,Checkcmd.m_Name) )
			{
				Command = Checkcmd;
				fFoundCmd = true;
				break;
			}
		}

		//Create the command
		scriptcmd_t &ScriptCmd = scriptcmd_t( Command.m_Name, Command.m_Conditional );
		if( !fFoundCmd )
		{
			//First word was not a command

			//I have an owner, try his parseline function
			int iReturn = m.pScriptedInterface ? m.pScriptedInterface->Script_ParseLine( this, pszCommandLine, ScriptCmd ) : 0;
				
			if( iReturn <= 0 )
			{
				//Owner entity didn't recognize command
				if( m.PrecacheOnly )
					ALERT( at_console, "Script: %s, Line: %i - Command \"%s\" NOT FOUND!\n", m.ScriptFile.c_str(), LineNum, TestCommand );
				return 0;
			}

		}

		//Add all the command's parameters
		while( sscanf( CmdLine, "%s", cBuffer ) > 0 )
		{
			LineOfs += strlen(cBuffer);

			//Stop at end-of-line comments
			if( !strncmp(cBuffer, "//", 2 ) ) 
				break;
		
			if( cBuffer[0] == '"' )
			{
				LineOfs -= strlen(cBuffer);		//Bring the offset back to the quote
				LineOfs++;						//Set the offset to the next char
				if( sscanf( CmdLine, "%[^\"]", cBuffer ) > 0 ) //Read until the next quote
				{
					LineOfs += strlen(cBuffer);	//Move up the offset to span the quoted parameter
					LineOfs++;					//Skip the closing quote
				}
				else	//Error: No closing quotes
					MSErrorConsoleText( "", UTIL_VarArgs("Script: %s, Line: %i - \"%s\" Closing quotations NOT FOUND!\n", m.ScriptFile.c_str(), LineNum, cBuffer) );
			}

			ScriptCmd.m_Params.add( GetConst(cBuffer) );	//Resolve constants, but not variables

			if( sscanf( CmdLine, "%[ \t\r\n]", cBuffer ) > 0 ) LineOfs += strlen(cBuffer);
		}

		//Add the command to the bunch
		CurrentCmds.m_Cmds.add( ScriptCmd );
	}

DontKeepCommand:
	if( (*pCurrentCmds) && (*pCurrentCmds)->m_SingleCmd )
	{
		//This is the first line after a conditional and we didn't encounter an opening brace.
		//The conditional only gets this one command... we now return control to the parent command list
		if( ParentCmds.size() )
		{
			*pCurrentCmds = ParentCmds[ParentCmds.size()-1];
			ParentCmds.erase( ParentCmds.size()-1 );
		}
		else MSErrorConsoleText( "", UTIL_VarArgs("Script: %s, Line: %i - Conditional command returned to parent cmd list but the parent list wasn't found!\n", m.ScriptFile.c_str(), LineNum, cBuffer) );
	}

	enddbg( "CScript::ParseLine()" );

	return 1;
}

// Executes a single script event
bool CScript::Script_ExecuteEvent( SCRIPT_EVENT &Event, msstringlist *Parameters )
{
	m.CurrentEvent = &Event;
	if( m.pScriptedEnt ) m.pScriptedInterface->Script_ExecuteEvent( this, Event );

	//-1 means don't ever play again
	//Event.fNextExecutionTime = -1;

	//Auto-repeat if repeatdelay is set
	//if( Event.fRepeatDelay > -1 ) Event.fNextExecutionTime = gpGlobals->time + Event.fRepeatDelay;

	//Setup variables for the event
	Script_SetupEvent( Event, Parameters );

	//Execute the commands
	bool fReturn = Script_ExecuteCmds( Event, Event.Commands );

	Event.m_Variables.clearitems( ); //Erase all local variables
	Event.Params = NULL;

	m.CurrentEvent = NULL;
	return fReturn;
}
bool CScript::Script_SetupEvent( SCRIPT_EVENT &Event, msstringlist *Parameters )
{
	Event.Params = Parameters;

	//Setup option parameter variables
	if( Parameters )
		 for (int i = 0; i < (*Parameters).size(); i++) 
			Event.SetLocal( msstring("PARAM") + int(i+1), (*Parameters)[i] );

	return m.pScriptedInterface ? m.pScriptedInterface->Script_SetupEvent( this, Event ) : true;
}
bool CScript::Script_ExecuteCmds( SCRIPT_EVENT &Event, scriptcmd_list &Cmdlist )
{
	 for (int c = 0; c < Cmdlist.m_Cmds.size(); c++) 
	{
		scriptcmd_t &Cmd = Cmdlist.m_Cmds[c];

		//Convert the variable parameters
		msstringlist Params;
		 for (int icmd = 0; icmd < Cmd.m_Params.size()-1; icmd++) 
			Params.add( GetVar(Event.GetLocal(Cmd.m_Params[icmd+1])) );

		// if Script_ExecuteCmd returns false, break execution of the event
		if( Script_ExecuteCmd( Event, Cmd, Params ) )
		{
			if( Cmd.m_Conditional )
				//This command was a conditional command and the conditions were met, execute the child commands
				Script_ExecuteCmds( Event, Cmd.m_IfCmds );
		}
		else if( Cmd.m_Conditional )
		{
			if( !Cmd.m_NewConditional ) 
				break;		//Old if command.  Breaks event execution on failure

			foreach( e, Cmd.m_ElseCmds.size() )								//Parse all else statements
				if( Script_ExecuteCmds( Event, Cmd.m_ElseCmds[e] ) ) break;	//As soon as one returns true, don't parse any more

			if( Cmdlist.m_Cmds.size() == 1 )	//This is an event with only one conditional statment or
				return false;		//An else if statement.  This statment failed so return false to try the remaining else statements
		}
	}

	return true;
}
void CScript::SendScript( scriptsendcmd_t &SendCmd )
{
	#ifdef VALVE_DLL
	bool TargetOk = true;
	if( g_netmsg[NETMSG_CLDLLFUNC] ) //g_netmsgs aren't initialized until the player is spawned... but this may be called earlier from the world.script
	{
		enum sendtype_e { ST_NEW = 0, ST_UPDATE, ST_REMOVE } Type = (SendCmd.MsgType == "remove") ? ST_REMOVE : SendCmd.MsgType == "new" ? ST_NEW : ST_UPDATE;
		if( SendCmd.MsgTarget == "all" )
		{
			MESSAGE_BEGIN( MSG_ALL, g_netmsg[NETMSG_CLDLLFUNC], NULL );
		}
		else if( SendCmd.MsgTarget == "all_in_sight" )
		{
			if( m.pScriptedEnt )
				MESSAGE_BEGIN( MSG_PVS, g_netmsg[NETMSG_CLDLLFUNC], m.pScriptedEnt->pev->origin );
			else TargetOk = false;
		}
		else
		{
			CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( SendCmd.MsgTarget ) : NULL;
			if( pEntity && pEntity->IsNetClient() )
				MESSAGE_BEGIN( MSG_ONE, g_netmsg[NETMSG_CLDLLFUNC], NULL, pEntity->pev );
			else TargetOk = false;
		}

		if( TargetOk )
		{
				WRITE_BYTE( 17 );
				WRITE_BYTE( Type ); //0 == Add | 1 == Update | 2 = Remove
				WRITE_LONG( SendCmd.UniqueID );
				if( Type == ST_NEW ) WRITE_STRING( SendCmd.ScriptName );
				int Parameters = SendCmd.Params.size();
				WRITE_BYTE( Parameters );
				 for (int i = 0; i < Parameters; i++) 
					WRITE_STRING( SendCmd.Params[i] );
			MESSAGE_END();
		}
	}
	#endif
}
CBaseEntity *CScript::RetrieveEntity( msstring_ref Name )
{
	return m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Name ) : StringToEnt( Name );
}
void CScript::CallEventTimed( msstring_ref EventName, float Delay )
{
	float Time = gpGlobals->time + Delay;
	 for (int e = 0; e < m.Events.size(); e++) 
	{
		SCRIPT_EVENT &seEvent = m.Events[e];
		if( !seEvent.Name || seEvent.Name != EventName )
			continue;

		seEvent.TimedExecutions.add( Time );
	}
}


//IScripted
//*********

IScripted::IScripted( ) 
{ 
	m_pScriptCommands = NULL; 
}
void IScripted::Deactivate( ) 
{
	 for (int i = 0; i < m_Scripts.size(); i++) 
		{ Script_Remove( i ); i--; }
		
	m_Scripts.clear( );		//explicitly delete the list, to reclaim the memory

}
CScript *IScripted::Script_Add( string_i ScriptName, CBaseEntity *pEntity )
{
	//Adds a new script to the list
	CScript *Script = msnew CScript( );

	bool fSuccess = Script->Spawn( ScriptName, pEntity, this );

	if( fSuccess ) m_Scripts.add( Script );
	else 
		{ if( Script ) delete Script; Script = NULL; }
	
	return Script;
}
CScript *IScripted::Script_Get( string_i ScriptName )
{
	 for (int i = 0; i < m_Scripts.size(); i++) 
		if( !strcmp(m_Scripts[i]->m.ScriptFile,ScriptName) )
			return m_Scripts[i];
	return NULL;
}
void IScripted::Script_Remove( int idx )
{
	delete m_Scripts[idx];
	m_Scripts.erase( idx );
}

int IScripted::Script_ParseLine( CScript *Script, msstring_ref pszCommandLine, scriptcmd_t &Cmd ) 
{
	//Script is checking if MSMonster sees this line as a command
	char TestCommand[MSSTRING_SIZE];

	//Read the first word of the line
	if( sscanf( (const char *)pszCommandLine, " %s", TestCommand ) <= 0 )
		return 0;

	if( m_pScriptCommands )
		 for (int icommand = 0; icommand < m_pScriptCommands->size(); icommand++) 
		{
			scriptcmdname_t &Checkcmd = (*m_pScriptCommands)[icommand];
			if( !stricmp(TestCommand,Checkcmd.m_Name.c_str()) )
				return Checkcmd.m_Conditional ? 2 : 1;
		}

	return 0;
}
void IScripted::RunScriptEvents( bool fOnlyRunNamedEvents )
{
	 for (int i = 0; i < m_Scripts.size(); i++) 
	{
		CScript *Script = m_Scripts[i];
		if( !Script->m.RemoveNextFrame )
			Script->RunScriptEvents( fOnlyRunNamedEvents );
		else
		{
			delete Script;

			m_Scripts.erase( i );
			i--;
		}

	}
}
void IScripted::CallScriptEventTimed( msstring_ref EventName, float Delay )
{ 
	 for (int i = 0; i < m_Scripts.size(); i++) 
		m_Scripts[i]->CallEventTimed( EventName, Delay );
}
void IScripted::CallScriptEvent( msstring_ref EventName, msstringlist *Parameters )
{
	m_ReturnData[0] = 0;
	 for (int i = 0; i < m_Scripts.size(); i++) 
		m_Scripts[i]->RunScriptEventByName( EventName, Parameters );
}
void IScripted::Script_InitHUD( CBasePlayer *pPlayer )
{
	 for (int i = 0; i < m_Scripts.size(); i++) 
	{
		CScript *Script = m_Scripts[i];
		 for (int e = 0; e < Script->m.PersistentSendCmds.size(); e++) 
			Script->SendScript( Script->m.PersistentSendCmds[e] );
	}
}
msstring_ref IScripted::GetFirstScriptVar( msstring_ref VarName )
{
	//Only use the first script!
	if( !m_Scripts.size( ) )
		return VarName;

	return m_Scripts[0]->GetVar( VarName );
}
void IScripted::SetScriptVar( msstring_ref VarName, msstring_ref Value ) { if( m_Scripts.size( ) ) m_Scripts[0]->SetVar( VarName, Value ); }
void IScripted::SetScriptVar( msstring_ref VarName, int iValue )		 { if( m_Scripts.size( ) ) m_Scripts[0]->SetVar( VarName, iValue ); }
void IScripted::SetScriptVar( msstring_ref VarName, float flValue )		 { if( m_Scripts.size( ) ) m_Scripts[0]->SetVar( VarName, flValue ); }


msstring_ref SCRIPT_EVENT::GetLocal( msstring_ref Name )
{ 
	if( !stricmp(Name,"game.event.params") )
		return Params ? UTIL_VarArgs( "%i", Params->size() ) : "0";

	return GetVar( Name ); 
}

//Calls every scripted entity
//Also calls world.script
void CScript::CallScriptEventAll( msstring_ref EventName, msstringlist *Parameters )
{
	#ifdef VALVE_DLL
		edict_t		*pEdict = NULL;
		CBaseEntity *pEntity = NULL;
		IScripted *pScripted = NULL;

		for( int i = 1; i < gpGlobals->maxEntities; i++ )
		{
			pEdict = g_engfuncs.pfnPEntityOfEntIndex( i );

			if( !pEdict || pEdict->free )	// Not in use
				continue;

			pEntity = MSInstance( pEdict );
			if( !pEntity ) continue;		//No private data

			pScripted = pEntity->GetScripted( );
			if( !pScripted ) continue;		//Not scripted

			pScripted->CallScriptEvent( EventName, Parameters );
		}
	#else
		gHUD.m_HUDScript->CallScriptEvent( EventName, Parameters );
	#endif

	if( MSGlobals::GameScript )
		MSGlobals::GameScript->CallScriptEvent( EventName, Parameters );
}

bool GetModelBounds( void *pModel, Vector Bounds[2] )
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pModel;
	if( !pstudiohdr )
		return false;

	return true;
}