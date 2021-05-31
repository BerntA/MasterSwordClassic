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
	#include "MSCentral.h"
	bool GetModelBounds( CBaseEntity *pEntity, Vector Bounds[2] );

#else
	#include "../cl_dll/MasterSword/CLGlobal.h"
	#include "../cl_dll/hud.h"
	#include "../cl_dll/cl_util.h"
	#include "../cl_dll/MasterSword/HUDScript.h"
#endif
#include "../engine/studio.h"
#include "logfile.h"
#include "time.h"

//[MiB] - for checking if the "Cheat Engine.exe" process is running
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <psapi.h>
#pragma comment(lib, "Psapi")
//[/MiB]

#undef SCRIPTVAR
#define SCRIPTVAR GetVar								//A script-wide or global variable
#define SCRIPTCONST( a ) SCRIPTVAR(GetConst(a))			//A const, script-wide, or global variable - loadtime only
#define GETCONST_COMPATIBLE( a ) ( a[0] == '$' ? GetConst(a) : SCRIPTCONST(a) )			//Loadtime - Only parse it as a var if it's not a $parser
//int CountPlayers( void );
bool FindSkyHeight( Vector Origin, float &SkyHeight );
bool UnderSky( Vector Origin ); //Thothie AUG2010_03
msstring_ref PM_GetValue( msstringlist &Params );
bool GetModelBounds( void *pModel, Vector Bounds[2] );


scriptcmdname_list CScript::m_GlobalCmds;				//Shared list of script commands
mslist<scriptvar_t> CScript::m_gVariables;				//Global script variables
mslist<scriptarray_t> CScript::GlobalScriptArrays;		// MiB JUN2010_25
ulong CScript::m_gLastSendID = SCRIPT_ID_START;			//Server: ID of last script sent to a client
														//Client: ID of next script to be created
ulong CScript::m_gLastLightID;							//ID of last dynamic light
//mslist<scriptfile_t> filesOpen; //MiB Feb2008a File I/O - moved elswhere?

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
		if ( MSGlobals::DevModeEnabled )
		{
			ReturnString = "test_scripts/";
			ReturnString += MSGlobals::MapName;
			ReturnString += "/map_startup";
		}
		else
		{
			ReturnString = MSGlobals::MapName;
			ReturnString += "/map_startup";
		}
		return ReturnString;
	}
	else if( !strcmp(Text,"$currentmap_npc_externals" ) ) //Thothie NOV2014_09 - adding for casual addition of additional map-exclusive scripts
	{
		if ( MSGlobals::DevModeEnabled )
		{
			ReturnString = "test_scripts/";
			ReturnString += MSGlobals::MapName;
			ReturnString += "/npc_externals";
		}
		else
		{
			ReturnString = MSGlobals::MapName;
			ReturnString += "/npc_externals";
		}
		return ReturnString;
	}
	else if( !strcmp(Text,"$currentmap_player_externals" ) ) //Thothie NOV2014_09 - adding for casual addition of additional map-exclusive scripts
	{
		if ( MSGlobals::DevModeEnabled )
		{
			ReturnString = "test_scripts/";
			ReturnString += MSGlobals::MapName;
			ReturnString += "/player_externals";
		}
		else
		{
			ReturnString = MSGlobals::MapName;
			ReturnString += "/player_externals";
		}
		return ReturnString;
	}
	else if( !strcmp(Text,"$currentmap_game_master" ) ) //Thothie NOV2014_09 - adding for casual addition of additional map-exclusive scripts
	{
		if ( MSGlobals::DevModeEnabled )
		{
			ReturnString = "test_scripts/";
			ReturnString += MSGlobals::MapName;
			ReturnString += "/game_master";
		}
		else
		{
			ReturnString = MSGlobals::MapName;
			ReturnString += "/game_master";
		}
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

		//Thothie NOV2014_22 - reorginizing this sequence from most common to least
		//$get, followed by math, followed by string ops, followed by everything else
//=======================================================================================
//Very High Priority Functions
//commonly called, often in rapid sucsession
//=======================================================================================
		//Handle parser
		//priority: very high, scope: shared
		if( ParserName.starts_with("$rand") )  //random number
		{
			if( ParserName[5] == 'f' ) //float version
				RETURN_FLOAT( RANDOM_FLOAT( atof(Params[0]), atof(Params[1]) ) )
			else  //int version
				RETURN_INT( RANDOM_LONG( atoi(Params[0]), atoi(Params[1]) ) )
		}
#ifdef VALVE_DLL
		//priority: very high, scope: server
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

					//Thothie adding invincible prediction (fail)
					//if ( ValueVar == "invincible" ) return pEntity->pev->flags,FL_GODMODE ? "1" : "0";

					if( pEntity )
						return (Return = m.pScriptedEnt->GetProp( pEntity, ValueVar, Params ));
				}
			}
			return "0";
		}
#endif
		//priority: very high, scope: shared
		else if( ParserName == "$neg" )	//Make negative
		{
			if( Params.size() >= 1 ) 
				RETURN_FLOAT( (-atof(Params[0])) )
			else return "0";
		}
		//priority: very high, scope: shared
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
		//priority: very high, scope: shared
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
		//priority: very high, scope: shared
		else if( ParserName == "$get_find_token" )	//Thothie DEC2007a - save a common complex scripting step
		{
			//$get_find_token(<token_string>,<search_string>)
			//- returns idx of found string, or -1 if not found
			//- search is exact, and case senstive - may make flags for partial matches later
			if( Params.size() >= 2 ) 
			{
				static msstringlist Tokens;
				Tokens.clearitems();
				msstring &TokenAdd = Params[1];

				msstring TokenString = GetVar(Params[0]);
				TokenizeString( TokenString, Tokens );

				int token_found_at = -1;

				 for (int i = 0; i < Tokens.size(); i++) 
				{
					if ( Tokens[i] == TokenAdd ) token_found_at = i;
				}
				
				RETURN_INT( token_found_at );
			}
			else return "-1";
		}
		//priority: very high, scope: shared
		else if( ParserName == "$vec" )	//Create Vector
		{
			if( Params.size() >= 3 )
				Return = msstring("(") + Params[0] + "," + Params[1] + "," + Params[2] + ")";
			else return "0";
			return Return;
		}
		//priority: very high, scope: shared
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
		//priority: very high, scope: shared
		else if( ParserName == "$anglediff" )	//Get angle diff
		{
			if( Params.size() >= 2 )
			{
				float DestAngle = atof(Params[0]); 
				float SrcAngle = atof(Params[1]);
				RETURN_FLOAT( EngineFunc::AngleDiff( DestAngle, SrcAngle ) );
			}
		}
		//priority: very high, scope: shared
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
		//priority: very high, scope: shared
		else if( ParserName == "$dist2D" )		//Thothie JAN2013_06 - dist between two points, on 2D plane
		{
			if( Params.size() >= 2 )
			{
				Vector Start = StringToVec( Params[0] );
				Vector End = StringToVec( Params[1] );
				float Dist = (Start - End).Length2D();
				RETURN_FLOAT( Dist );
			}
			else return "0";
		}
		//priority: very high, scope: shared
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
		//Thothie MAR 2008a
		//- $angles(<start_origin>,<end_origin>) – returns the 2d angle between two vectors
		//- worried atan2 maybe CPU intensive. :/
		//priority: very high, scope: shared
		else if( ParserName == "$angles" )
		{
			if( Params.size() >= 2 )
			{
				Vector Start = StringToVec( Params[0] );
				Vector End = StringToVec( Params[1] );
				float dy = End.y - Start.y;
				float dx = End.x - Start.x;
				float rang = atan2(dy,dx); //gets radians
				float xang = rang * 57.295779; //back to degrees
				RETURN_FLOAT(xang)
			}
			else return "0";
		}
		//priority: very high, scope: shared (ish)
		else if( ParserName == "$angles3d" )
		{
			Vector Start = StringToVec( Params[1] );
			Vector End = StringToVec( Params[0] );
			Vector out_vector;
			//Thoth - FEB2009_12 - Need this function too often, especially client side
			//$angles3d(<viewer_origin>,<origin_to_face>) - returns angles required for a vector to face a location
			//engine function fast, but not very accurate and doesn't work client side (returns normalized) - will need to flip pitch
			out_vector = UTIL_VecToAngles((Start-End).Normalize());
			RETURN_VECTOR( out_vector )
			/*
			// FUCK THIS FUCKING FUNCTION ><
			//FAIL Thoth JAN2010_22 - steelin from the Quake source code so weez can do this client side
			//- meh, think this is an entirely different function - maybe I can just vectormultiply the results of $dir(vec,vec) for same result?
			Vector Vec_Org = StringToVec( Params[1] );
			Vector Vec_Face = StringToVec( Params[0] );
			Vector out_vector;
			float lforward, lyaw, lpitch;
			if ( Vec_Org.y == 0 && Vec_Org.x == 0 )
			{
				lyaw = 0;
				if ( Vec_Org.z > 0 )
					lpitch = 90;
				else
					lpitch = 270;
			}
			else
			{
				if (Vec_Org.x)
					lyaw = (atan2(Vec_Org.y, Vec_Org.x) * 180 / M_PI);
				else if (Vec_Org.y > 0)
					lyaw = 90;
				else
					lyaw = 270;

				if (lyaw < 0)
					lyaw += 360;

				lforward = sqrt(Vec_Org.x*Vec_Org.x + Vec_Org.y*Vec_Org.y);
				lpitch = (atan2(Vec_Org.y, lforward) * 180 / M_PI);
				if (lpitch < 0)
					lpitch += 360;
			}

			Vec_Face.x = -lpitch;
			Vec_Face.y = lyaw;
			Vec_Face.z = 0;

			out_vector = (Vec_Org-Vec_Face).Normalize();
			*/
		}
		//priority: very high, scope: shared
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
		//priority: very high, scope: shared (ish)
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
		//priority: very high, scope: shared
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
		//priority: very high, scope: shared
		else if( ParserName == "$get_skill_ratio" || ParserName == "$ratio" )
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
		//priority: very high, scope: shared
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

//=======================================================================================
//High Priority Functions
//fairly commonly called, sometimes rapidly
//=======================================================================================
#ifdef VALVE_DLL
		//Thothie DEC2007a
		//$get_tsphere(<search_type>,<radius>,[source_origin])
		//Same as in sphere but returns all targets in a token string
		//(original $get_insphere shoulda worked like this, but I ain't thought of it then
		//cant search for specific ents with this, so old $get_insphere still not useless
		//<search_type> can be: any, player, monster, enemy, or ally
		//priority: high, scope: server
		else if( ParserName.starts_with("$get_tsphere") )
		{
			msstring &Name = Params[0];
			msstring thoth_token_string;
			msstring ent_str;
			int str_limit = 214;
			float thoth_boxsize = atof(Params[1]);
			//float neg_boxsize = thoth_boxsize * -1;
			CBaseEntity *pList[255], *pEnt = NULL;
			Vector StartPos;
			if ( Params.size() == 2 ) StartPos = m.pScriptedEnt->pev->origin;
			else if ( Params.size() >= 3 ) StartPos = StringToVec(Params[2]);
			int count = UTIL_MonstersInSphere( pList, 255, StartPos, thoth_boxsize);
	
			//ALERT( at_aiconsole, "Searching through %i ents\n", count );					

			 for (int i = 0; i < count; i++) 
			{
				if ( (int)thoth_token_string.len() > str_limit ) break; //outta room

				pEnt = pList[i];

				if ( !pEnt->IsAlive() ) continue; //dont count anything dead

				if ( m.pScriptedEnt->entindex() == pEnt->entindex() ) continue; //dont count self

				if( !stricmp("player",Name) && pEnt->IsPlayer() )
				{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
				}

				if( !stricmp("monster",Name) && pEnt->IsMSMonster() && !pEnt->IsPlayer() )
				{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
				}

				if( !stricmp("any",Name) && !pEnt->IsMSItem() )
				{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
				}

				if ( !strcmp("enemy",Name) )
				{
					CMSMonster *pMonster = (CMSMonster *)pEnt;
					int my_relate = pMonster->IRelationship(m.pScriptedEnt);
					if ( my_relate == -3 && pMonster->m_Race )
					{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
					}
				}

				if ( !strcmp("ally",Name) )
				{
					CMSMonster *pMonster = (CMSMonster *)pEnt;
					int my_relate = pMonster->IRelationship(m.pScriptedEnt);
					if ( my_relate == 1 && pMonster->m_Race )
					{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
					}
				}
			}
			if ( thoth_token_string.len() > 0 ) return thoth_token_string.c_str();
			else return "none";
		}
#endif
		//priority: high, scope: shared
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
		//Thothie AUG2010_03
		//$get_under_sky(<origin>) - returns "1" if point is under sky texture
		//priority: high, scope: shared
		else if( ParserName == "$get_under_sky" )
		{
			if( Params.size() >= 1 )
			{
				bool FoundSky = UnderSky( StringToVec( GetScriptVar(Params[0]) ) );
				return FoundSky ? "1" : "0";
			}
		}
#ifdef VALVE_DLL
		//MiB Mar2008a
		//$can_damage(<target_ent>,[targetting_ent])
		//If targetting_ent isn't provided, the calling ent is assumed to be targetting
		//This command is mostly for easier PVP checks, but has other uses
		//priority: high, scope: server
		else if( ParserName == "$can_damage" )
		{

			if( Params.size() >= 1 )
			{
				CBaseEntity *ThisEnt = Params.size() >= 2 ? m.pScriptedEnt->RetrieveEntity( Params[1] ) : m.pScriptedEnt;
				CBaseEntity *ThatEnt = m.pScriptedEnt->RetrieveEntity( Params[0] );

				if( ThisEnt && ThatEnt )
				{
					RETURN_INT( ThisEnt->CanDamage( ThatEnt ) ? 1 : 0 )
				}
			}
			RETURN_INT(0)
		}
#endif
		//NOV2014_20 Thothie - Math macro
		//$math("add|multiply|subtract|divide",<amt|var>,<amt|var>)
		//returns result, does not modify vars
		//todo: add more functions
		//priority: high, scope: shared
		else if ( ParserName.starts_with("$math") )
		{
			if ( Params.size() < 2 )
			{
				MSErrorConsoleText( "ExecuteScriptCmd", UTIL_VarArgs("Script: %s, %s - not enough parameters!\n", m.ScriptFile.c_str(), ParserName.c_str()) );
				return "0";
			}
			
			msstring op_type = Params[0];
			bool legit_command = false;

			float maths1 = atof(Params[1].c_str());
			float maths2 = atof(Params[2].c_str());

			float end_result;

			if ( op_type.starts_with("add") )
			{
				legit_command = true;
				end_result = maths1 + maths2;
			}
			else if ( op_type.starts_with("subtract") )
			{
				legit_command = true;
				end_result = maths1 - maths2;
			}
			else if ( op_type.starts_with("multiply") )
			{
				legit_command = true;
				end_result = (maths1 * maths2);
			}
			else if ( op_type.starts_with("divide") )
			{
				legit_command = true;
				if ( maths1 == 0 || maths2 == 0 ) end_result = 0; //ya know, ya'd think after all these decades, computers would stop trying to do this automatically
				else end_result = (maths1 / maths2);
			}

			if ( !legit_command )
			{
				MSErrorConsoleText( "ExecuteScriptCmd", UTIL_VarArgs("Script: %s, %s - %s is an invalid operation!\n", m.ScriptFile.c_str(), ParserName.c_str(), op_type.c_str()) );
				return "0";
			}

			RETURN_FLOAT(end_result);
		}
		// MiB JAN2010_26	Scripted array functions
		// MiB JUN2010_25 Added global functionality
		//priority: high, scope: uncertain
		else if ( ParserName.starts_with( "$get_array" ) || ParserName.starts_with( "$g_get_array" ) )
		{
			/*
				Return [ERROR_NO_ARRAY] when the given array can't be found.
				Return [ERROR_MISSING_PARAMS] if not given enough Parameters.
				$get_array(<name>,<idx>)							Returns <name>[idx]		( -1 for bad index )
				$get_arrayfind(<name>,<search_str>,[start_idx])		Returns index of first instance of search_str after <name>[idx] ( -1 if not found )
				$get_array_amt(<name>)								Returns number of elements in <name>   ( -1 if couldn't find array )
			*/

			if ( Params.size() >= 1 )
			{
				msstring ArrName = Params[0];
				int idx = -1;
				mslist<scriptarray_t> *ArrayList = ParserName.starts_with( "$g_get_array" ) ? &GlobalScriptArrays : &m.pScriptedEnt->scriptedArrays;
				 for (int i = 0; i < ArrayList->size(); i++) 
				{
					if ( (*ArrayList)[i].Name == ArrName )
					{
						idx = i;
						break;
					}
				}

				if ( idx == -1 )
				{
					//Thothie AUG2013_28 - dis not work like it says it work in description above (fixing)
					if ( ParserName.ends_with( "amt" ) )
					{
						return "-1";
					}
					else
					{
						if ( Params[1] == "exists" ) //Thothie NOV2014_16 simplify
						{
							return "0";
						}
						else
						{
							return "[ERROR_NO_ARRAY]";
						}
					}
				}
				if ( Params[1] == "exists" ) return "1"; //Thothie NOV2014_16 simplify

				if ( ParserName.ends_with( "array" ) )
				{
					if ( Params.size() >= 2 )
					{
						int subIdx = atoi( Params[1].c_str() );
						if ( subIdx > -1 && (*ArrayList)[idx].Vals.size() > (unsigned)subIdx )
							return (*ArrayList)[idx].Vals[subIdx];
					}
				}
				else if ( ParserName.ends_with( "find" ) )
				{
					if ( Params.size() >= 2 )
					{
						msstring srch = Params[1];
						int start_idx = Params.size() >= 3 ? atoi ( Params[2].c_str() ) : 0;
						start_idx = max( start_idx, 0 );
						for ( int i = start_idx ; (unsigned) i < ((*ArrayList)[idx].Vals.size() - start_idx) ; ++i )
							if ( (*ArrayList)[idx].Vals[i] == srch )
								RETURN_INT( i );

						RETURN_INT( -1 );
					}
					else
						return "[ERROR_MISSING_PARAMS]";

				}
				else if ( ParserName.ends_with( "amt" ) )
					RETURN_INT( (*ArrayList)[idx].Vals.size() )
			}
			else 
				return "[ERROR_MISSING_PARAMS]";
		}
#ifdef VALVE_DLL
		//Thothie JAN2013_02 scriptflags
		//$get_scriptflag(<target>,<type>,type_exists) - returns 1 if <type> found
		//$get_scriptflag(<target>,<type>,type_first) - returns first value of <type> found or "none"
		//$get_scriptflag(<target>,<type>,type_count) - returns the # of flags of this <type> that exist on the <target> or "none"
		//$get_scriptflag(<target>,<type>,type_value) - returns total value of all <type> flags combined, or "none"
		//$get_scriptflag(<target>,<type>,type_array) - returns name of array, created on <target>, with all values of <type> or "none"
		//$get_scriptflag(<target>,<name>,name_exists) - returns 1 if <name> found
		//$get_scriptflag(<target>,<name>,name_value) - returns value if <name> found or "none"
		//$get_scriptflag(<target>,<name>,name_type) - returns type if <name> found or "none"
		//$get_scriptflag(<target>,listall) - list all flags [debuggary,developer build only]
		//- do not use quotes around <name> or <type>
		//- this could be used to return the total effect of cold resistance for multiple items, for example
		//priority: high, scope: server
		if( ParserName.starts_with("$get_scriptflag") )
		{
			bool sf_add_values = false;
			bool sf_get_by_name = false;
			bool sf_get_first_value = false;
			bool sf_get_value_by_name = false;
			bool sf_get_type_by_name = false;
			bool sf_count_values = false;
			bool sf_array_values = false;
			
			msstring sf_not_found_msg;
			bool sf_found = false;
			bool sf_setup_array = false;
			float sf_total_values = 0;
			msstring sf_array_name = "ARRAY_";
			static msstringlist Parameters;

			//we may add other function options here later
			if ( Params[2] == "totalvalue" || Params[2] == "type_value" )
			{
				sf_not_found_msg = "none";
				sf_add_values = true;
			}
			else if ( Params[2] == "name_exists" )
			{
				sf_not_found_msg = "0";
				sf_get_by_name = true;
			}
			else if ( Params[2] == "first_value" || Params[2] == "type_first" )
			{
				sf_not_found_msg = "none";
				sf_get_first_value = true;
			}
			else if ( Params[2] == "name_value" )
			{
				sf_not_found_msg = "none";
				sf_get_by_name = true;
				sf_get_value_by_name = true;
			}
			else if ( Params[2] == "name_type" )
			{
				sf_not_found_msg = "none";
				sf_get_by_name = true;
				sf_get_type_by_name = true;
			}
			else if ( Params[2] == "type_count" )
			{
				sf_not_found_msg = "none";
				sf_add_values = true;
				sf_count_values = true;
			}
			else if ( Params[2] == "type_array" )
			{
				sf_not_found_msg = "none";
				sf_add_values = true;
				sf_array_values = true;
				msstring arrayext = Params[1];
				sf_array_name += _strupr(arrayext.c_str());
			}
			else if ( Params[2] == "type_exists" )
			{
				sf_not_found_msg = "0";
			}

			CBaseEntity *pEntity = RetrieveEntity(Params[0]);
			IScripted *iScripted = pEntity ? pEntity->GetScripted() : NULL;
			if ( pEntity && iScripted )
			{
#if !TURN_OFF_ALERT
				if ( Params[1] == "listall" )
				{
					Print( "Scriptflags for %s:\n", pEntity->m_DisplayName.c_str() );
					 for (int i = 0; i < pEntity->m_scriptflags.names.size(); i++) 
					{
						Print( "# %i type %s name %s val %s exp %s\n",i, pEntity->m_scriptflags.types[i].c_str(), pEntity->m_scriptflags.names[i].c_str(), pEntity->m_scriptflags.values[i].c_str(), pEntity->m_scriptflags.expiretimes[i].c_str() );
					}
					return "0";
				}
#endif
				 for (int i = 0; i < pEntity->m_scriptflags.names.size(); i++) 
				{
					if ( !sf_get_by_name )
					{
						if ( pEntity->m_scriptflags.types[i] == Params[1] )
						{
							if ( !sf_add_values )
							{
								if ( !sf_get_first_value )
								{
									return "1"; //type_exists
								}
								else
								{
									msstring sTemp = pEntity->m_scriptflags.values[i];
									return sTemp.c_str(); //value_first
								}
								break;
							}
							else
							{
								if ( sf_array_values )
								{
									//type_array
									sf_found = true;

									if ( !sf_setup_array )
									{
										sf_setup_array = true;
										Util_ScriptArray(pEntity,"create",sf_array_name.c_str(),"0");
									}
									
									Util_ScriptArray(pEntity,"add",sf_array_name.c_str(),pEntity->m_scriptflags.values[i].c_str());
								}
								else if ( sf_count_values )
								{
									sf_found = true;
									sf_total_values = sf_total_values + 1;
								}
								else
								{
									sf_found = true;
									sf_total_values = sf_total_values + atof(pEntity->m_scriptflags.values[i].c_str());
								}
							}
						}
					} //end get by value

					if ( sf_get_by_name ) //would else, but simplify for later additions
					{
						if ( pEntity->m_scriptflags.names[i] == Params[1] )
						{
							sf_found = true;
							if ( sf_get_type_by_name )
							{
								return pEntity->m_scriptflags.types[i].c_str();
							}
							else if ( sf_get_value_by_name )
							{
								return pEntity->m_scriptflags.values[i].c_str();
							}
							else
							{
								return "1";
							}
							break;
						} 
					} //end get by name
				} //end big ass loop

				//no point in else if's here, as these all return, so save some nests
				if ( !sf_found )
				{
					return sf_not_found_msg.c_str();
				}
				else
				{
					//either type_value or type_count or type_array
					if ( !sf_array_values )
					{
						RETURN_FLOAT(sf_total_values)
					}
					else
					{
						//Print("DEBUG: $get_scriptflags value_array %f\n",sf_total_values);
						return sf_array_name.c_str();
					}
				}
			}
			else
			{
				MSErrorConsoleText( "scriptflags", UTIL_VarArgs("$get_scriptflags target entity not found in %s\n",m.ScriptFile.c_str()) );
			}
		}
#endif
		//priority: high, scope: shared
		else if( ParserName == "$quote" )	//Encase string in quotes Thothie FEB2008a (or return a ")
		{
			if( Params.size() >= 1 )
			{
				msstring out_str = "\"";

				 for (int i = 0; i < Params.size(); i++) 
				{
					if( i ) out_str += " ";
					out_str += Params[i];
				}
				out_str += "\"";
				return out_str;
			}
			else
			{
				return "\"";
			}
		}
		//priority: high, scope: shared
		else if( ParserName == "$lcase" )	//Make lower case - Thothie
		{
			if( Params.size() >= 1 )
			{
				char toconv[256];
				strcpy( toconv, Params[0] );
				return _strlwr( toconv );
			}
			else
			{
				return "0";
			}
		}
		//MiB Feb2008a
		//$len(<var>) - Returns the string length of <var>
		//priority: high, scope: shared
		else if( ParserName == "$len" )
		{
			if( Params.size() >= 1 )
				RETURN_INT( Params[0].len() )
			else
				return "0";
		}
		//Thothie $get_traceline(<vec>,<vec>,[flag],[ignore_ent])
		//flags: worldonly returns end pos, ent returns ent hit
		//- Traceline does not work, so attempting to make this way instead
		//priority: high, scope: shared
		else if( ParserName.starts_with("$get_traceline") )
		{
			Vector StartPos = StringToVec(Params[0]);
			Vector EndPos = StringToVec(Params[1]);
			msstring &Flags = Params[2];

			bool thoth_return_contents = false;

			CBaseEntity *pIgnoreEnt = 
#ifdef VALVE_DLL
				(Params.size() >= 4) ? StringToEnt( Params[3] ) : NULL;
#else
				NULL;
#endif

			int iFlags = 0;
#ifdef VALVE_DLL
			if( Flags.contains("ent" ) || Flags.contains("tracebox" ) )				iFlags = dont_ignore_monsters;
			else if( Flags.contains("worldonly") ) SetBits( iFlags, ignore_monsters );
			else if( Flags.contains("contents") )
			{
				thoth_return_contents = true;
				SetBits( iFlags, ignore_monsters );
			}
#else
			if( Flags.contains("ent" ) )			iFlags = 0;
			if( Flags.contains("worldonly" ) )
			{
				//Thothie SEP2011_09 - took some tweaking to get cl traces to actually be worldonly
				//iFlags = 65536;
				//iFlags = 0;
				//NONE OF THIS SHIT WORKS!!!
				//- effing client trace ALWAYS traces hitboxes, no matter what I try
				SetBits( iFlags, PM_WORLD_ONLY ); //doesn't work
				//SetBits( iFlags, PM_STUDIO_IGNORE ); //same as PM_WORLD_ONLY
				//SetBits( iFlags, PM_GLASS_IGNORE ); //doesn't actually ignore glass, but when combined with ignore studio, does a proper world trace *sometimes*
			}
			else if( Flags.contains("contents") )
			{
				thoth_return_contents = true;
				SetBits( iFlags, PM_WORLD_ONLY );
			}
			else if( Flags.contains("clworld") )
			{
				iFlags = 32; //SEP2011_16 - thothie's special clworld flag
			}
#endif

			sharedtrace_t Tr;

			EngineFunc::Shared_TraceLine( StartPos, EndPos, iFlags, Tr, 0, iFlags, pIgnoreEnt ? pIgnoreEnt->edict() : NULL );

			if ( Flags.contains("worldonly" ) || Flags.contains("tracebox") ) 
			{
				return VecToString(Tr.EndPos);
			}
			else if ( !Flags.contains("contents") ) 
			{
				//Thothie AUG2010_05 (comment only)
				//This probably won't work right client side
				//can we work a version that would return hit model indexes?
				CBaseEntity *pHitEnt = Tr.HitEnt ? MSInstance(INDEXENT(Tr.HitEnt)) : NULL;
				//msstring dbg_result = pHitEnt ? EntToString(pHitEnt) : VecToString(Tr.EndPos);
				return pHitEnt ? EntToString(pHitEnt) : VecToString(Tr.EndPos);
			}
			else if ( Flags.contains("contents") ) 
			{
				//Thothie AUG2010_05
				//This doesn't seem to be working right due to the world only flag
				//probably need to work into a differnet function
				//as this also needs to return if the entire trace is empty
				msstring thoth_contents_string;
				//Thothie AUG2013_22 - start pos, then end pos, silly
				int thoth_start_contents = EngineFunc::Shared_PointContents( StartPos );
				int thoth_end_contents = EngineFunc::Shared_PointContents( EndPos );
				if ( thoth_start_contents == CONTENTS_EMPTY ) thoth_contents_string="empty;";
				else if ( thoth_start_contents == CONTENTS_SOLID ) thoth_contents_string="solid;";
				else if ( thoth_start_contents == CONTENTS_WATER ) thoth_contents_string="water;";
				else if ( thoth_start_contents == CONTENTS_SLIME ) thoth_contents_string="slime;";
				else if ( thoth_start_contents == CONTENTS_LAVA ) thoth_contents_string="lava;";
				else if ( thoth_start_contents == CONTENTS_SKY ) thoth_contents_string="sky;";
				else if ( thoth_start_contents == 0 ) thoth_contents_string.append("unknown");

				if ( thoth_end_contents == CONTENTS_EMPTY ) thoth_contents_string.append("empty");
				else if ( thoth_end_contents == CONTENTS_SOLID ) thoth_contents_string.append("solid");
				else if ( thoth_end_contents == CONTENTS_WATER ) thoth_contents_string.append("water");
				else if ( thoth_end_contents == CONTENTS_SLIME ) thoth_contents_string.append("slime");
				else if ( thoth_end_contents == CONTENTS_LAVA ) thoth_contents_string.append("lava");
				else if ( thoth_end_contents == CONTENTS_SKY ) thoth_contents_string.append("sky");
				else if ( thoth_end_contents == 0 ) thoth_contents_string.append("unknown");
				return thoth_contents_string.c_str();
			}
		} //end Thothie $get_traceline
		//Thothie AUG2010_22
		//$get_contents(<origin>) - return contents value of origin
		//AUG2013_22 Thothie - (wow, three years later) - This is always returning solid (-2) on some maps, but only server side
		//priority: high, scope: shared
		else if ( ParserName == "$get_contents" )
		{
			Vector thothie_vec = StringToVec(Params[0]);
			int thothie_result = EngineFunc::Shared_PointContents( StringToVec(Params[0]) );
			msstring thothie_return = "unknown";
			if ( thothie_result == CONTENTS_EMPTY ) thothie_return = "empty";
			else if ( thothie_result == CONTENTS_SOLID ) thothie_return = "solid";
			else if ( thothie_result == CONTENTS_WATER ) thothie_return = "water";
			else if ( thothie_result == CONTENTS_SLIME ) thothie_return = "slime";
			else if ( thothie_result == CONTENTS_LAVA ) thothie_return = "lava";
			else if ( thothie_result == CONTENTS_SKY ) thothie_return = "sky";
			return thothie_return.c_str();
		}
#ifndef VALVE_DLL
		//priority: high, scope: client
		else if( ParserName == "$getcl" )				//$getcl( <client entity idx>, <property> )
		{
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
		}
#endif
#ifdef VALVE_DLL
		//$get_takedmg(<target>,<type>) FEB2009 Thothie
		//priority: high, scope: server
		else if( ParserName.starts_with("$get_takedmg") )
		{
			CMSMonster *pTarget = (CMSMonster *) RetrieveEntity( Params[0] );
			if ( !pTarget ) return "-1"; //FEB2009
			if ( Params[1] == "all" ) RETURN_FLOAT( pTarget->m.GenericTDM ); //MAR2010_03
			 for (int i = 0; i < pTarget->m.TakeDamageModifiers.size(); i++) 
			{
				CMSMonster::takedamagemodifier_t &TDM = pTarget->m.TakeDamageModifiers[i];
				msstring read_dmgtype = TDM.DamageType;
				//Print("Reading_Takedmg %s\n", read_dmgtype.c_str() );
				if ( read_dmgtype.contains(Params[1]) )	RETURN_FLOAT( TDM.modifier )
			}
            return "1.0";
		}
#endif
#ifdef VALVE_DLL
		//MiB Apr2008a
		//$sort_entlist(<ent_token_list>,<var_to_sort_by>)
		//Can sort by distance, hp, maxhp, etc (full list below)
		//priority: high, scope: server
		else if( ParserName.starts_with("$sort_entlist") )
		{
			if( Params.size() >= 2 )
			{
				static msstringlist Tokens;
				Tokens.clearitems();
				TokenizeString( Params[0], Tokens );

				msstring SortedList;

				msstring compParam = Params[1];

				int size = (signed) Tokens.size();
				 for (int i = 0; i < size; i++) 
				{
					CMSMonster *pZeroEnt = (CMSMonster *) m.pScriptedEnt->RetrieveEntity( Tokens[0] );

					int curIdx = 0;

					float curVal = 0;
					if ( compParam == "range" )
					{
						if( Params.size() == 2 )
						{
							curVal = ( pZeroEnt->pev->origin - m.pScriptedEnt->pev->origin ).Length();
						}
						else
						{
							curVal = ( pZeroEnt->pev->origin - StringToVec(Params[2]) ).Length();
						}
					}
					else if ( compParam == "hp" ) curVal = pZeroEnt->m_HP;
					else if ( compParam == "maxhp") curVal = pZeroEnt->m_MaxHP;
					else if ( compParam == "mp" ) curVal = pZeroEnt->m_MP;
					else if ( compParam == "maxmp") curVal = pZeroEnt->m_MaxMP;

					 for (int j = 0; j < Tokens.size()-1; j++) 
					{
						CMSMonster *pCurEnt = (CMSMonster *) m.pScriptedEnt->RetrieveEntity( Tokens[j+1] );
						float compVal =  0;

						if ( compParam == "range" )
						{
							if( Params.size() == 2 )
							{
								compVal = ( pCurEnt->pev->origin - m.pScriptedEnt->pev->origin ).Length();
							}
							else
							{
								compVal = ( pCurEnt->pev->origin - StringToVec(Params[2]) ).Length();
							}
						}
						else if ( compParam == "hp" ) compVal = pCurEnt->m_HP;
						else if ( compParam == "maxhp") compVal = pCurEnt->m_MaxHP;
						else if ( compParam == "mp" ) compVal = pCurEnt->m_MP;
						else if ( compParam == "maxmp") compVal = pCurEnt->m_MaxMP;

						if ( compVal < curVal )
						{
							curIdx = j;
							curVal = compVal;
						}
					}

					SortedList += Tokens[curIdx] + ";";
					Tokens.erase( curIdx );
				}

				return SortedList;
			}
			else
				RETURN_INT(-1)
		}
#endif

//=======================================================================================
//Moderate Priority Functions
//fairly commonly called, but rarely rapidly
//=======================================================================================
#ifdef VALVE_DLL
		//priority: moderate, scope: server
		else if( ParserName == "$get_by_name" )
		{
			if( Params.size() >= 1 )
			{
				CBaseEntity *pEntity = UTIL_FindEntityByString( NULL, "netname", msstring("¯") + Params[0] );
				if( pEntity ) return EntToString(pEntity);
			}
			return "0";
		}
#endif
		//AUG2013_27 Thothie - new function function
		//$func(<eventname>,[params...])
		//calls eventname, replaces itself with return value
		//return doesn't actually stop code execution - if multiple returns are encountered, items will be tokenized
		//priority: moderate, scope: shared
		else if( ParserName == "$func" )
		{
			msstring func_event = Params[0];
			msstringlist OutParams;
			OutParams.clearitems( );
			if ( Params.size() > 1 )
			{
				 for (int i = 0; i < Params.size(); i++) 
				{
					if ( i >= 1 )
					{
						OutParams.add( Params[i] );
					}
				}
			}

			m.pScriptedInterface->CallScriptEvent( func_event.c_str(), &OutParams );

			if( m.pScriptedInterface->m_ReturnData.len() )
			{
				return m.pScriptedInterface->m_ReturnData.c_str();
			}
			else
			{
				return "0";
			}
		}
#ifdef VALVE_DLL
		//priority: moderate, scope: server
		else if( ParserName == "$anim_exists" )
		{
			Name = FullName.substr( 13 );
			Name = Name.thru_char( ")" );
			if( m.pScriptedEnt->IsMSMonster() && ((CBaseAnimating *)m.pScriptedEnt)->LookupSequence( GetScriptVar(Name) ) > -1 )
				return "1";
			else return "0";
		}
#endif
		//Thothie NOV2014_08
		//$within_box(<target|origin>,<testbox_org|0>,<testbox_mins>,<testbox_maxs>)
		//tests if target's bounding box intersects with test box area, or if origin is inside test box
		//if testbox_org is zero, origin is calced from mins/maxs, and abs min/max must be supplied
		//priority: moderate, scope: shared
		else if( ParserName.starts_with("$within_box") )
		{
			Vector tOrg; //target origin
			Vector tMins; //target box absmin
			Vector tMaxs; //target box absmax
            Vector bOrg = StringToVec(Params[1]); //test box origin
			Vector bMins = StringToVec(Params[2]); //test box mins
			Vector bMaxs = StringToVec(Params[3]); //test box maxes
			if ( bOrg == g_vecZero )
			{
				//g_vecZero assumes a brush box from which the absmins/absmaxes were pulled, find center
				bOrg.x = bMins.x+((bMaxs.x - bMins.x)*0.5);
				bOrg.y = bMins.y+((bMaxs.y - bMins.y)*0.5);
				bOrg.z = bMins.z+((bMaxs.z - bMins.z)*0.5);
				ALERT( at_console, "Warning: $within_box test origin is 0 - attempting to find center via boundries (%f,%f,%f) and assuming absmins/absmaxs\n",bOrg.x,bOrg.y,bOrg.z );
			}
			else
			{
				bMins = bOrg - StringToVec(Params[2]);
				bMaxs = bOrg + StringToVec(Params[3]);
			}

			if ( !Params[0].starts_with("(") )
			{
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;
				if ( pEntity )
				{
					tOrg =  pEntity->pev->origin;
					tMins = pEntity->pev->absmin;
					tMaxs = pEntity->pev->absmax;
				}
				else
				{
					ALERT( at_console, "Warning: $within_box got invalid entity" );
					return "-1";
				}
			}
			else
			{
				tOrg = StringToVec(Params[0]);
				tMins = tOrg;
				tMaxs = tOrg;
			}

			if ( bMins.x > tMaxs.x ||
				bMins.y > tMaxs.y ||
				bMins.z > tMaxs.z ||
				bMaxs.x < tMins.x ||
				bMaxs.y < tMins.y ||
				bMaxs.z < tMins.z )
			{
				return "0";
			}
			else
			{
				return "1";
			}
		}
		//priority: moderate, scope: shared
		else if( ParserName == "$stradd" )	//Add strings together
		{
			 for (int i = 0; i < Params.size(); i++) 
				Return += Params[i];

			return Return;
		}
		//priority: moderate, scope: shared
		else if( ParserName == "$int" )	//convert to integer
		{
			if( Params.size() >= 1 ) 
			{
				Return = atoi(Params[0]);
				return Return;
			}
			else return "0";
		}
		//MiB Feb2008a
		//$search_string(<var>,<search_string>,[start]) - Returns the position of <search_string> in var, or -1 if not found
		//priority: moderate, scope: shared
		else if( ParserName == "$search_string" )
		{
			if( Params.size() >= 2)
			{
				int start = Params.size() >= 3 ? atoi( Params[2] ) : 0;
				RETURN_INT( Params[0].find( Params[1],  start ) )
			}
			else
				return "0";
		}
		// $timestamp MiB Feb2008a
		// Returns a timestamp in the format "Thu Jan 03 17:40:10 
		// - thothie notes: time format will vary with region
		// - for logging
		//priority: moderate, scope: shared
		else if( ParserName == "$timestamp" )
		{
			time_t Time;
			time( &Time );
			msstring_ref TimeString = ctime( &Time );
			msstring out_timestr = TimeString;
			out_timestr = out_timestr.substr(0, out_timestr.len() - 1 );
			if ( Params.size() > 0 ) out_timestr += Params[0];
			if( TimeString )
				return out_timestr.c_str();
			else
				return "NO_TIME_AVAILABLE";
		}
#ifdef VALVE_DLL
		//Thothie - lost a comment block here :/
		//$get_insphere(<search_type>,<radius>,[source_origin])
		//returns first detected ent in sphere
		//(Needs to be changed to return nearest instead)
		//<search_type> can be: id, player, monster, or any
		//priority: moderate, scope: server
		//rarely used now, but tends to be called rapidly when it is - more or less defunct
		else if( ParserName.starts_with("$get_insphere") )
		{
			msstring &Name = Params[0];
			float thoth_boxsize = atof(Params[1]);
			//float neg_boxsize = thoth_boxsize * -1;
			CBaseEntity *pList[255], *pEnt = NULL;
			Vector StartPos;
			if ( Params.size() == 2 ) StartPos = m.pScriptedEnt->pev->origin;
			else if ( Params.size() >= 3 ) StartPos = StringToVec(Params[2]);
			int count = UTIL_MonstersInSphere( pList, 255, StartPos, thoth_boxsize);
			bool spec_search = false;

			CBaseEntity *pSpecificEnt = RetrieveEntity(Name);

			if ( !Name.starts_with("player") && !Name.starts_with("monster") && !Name.starts_with("any") ) spec_search = true;

			//ALERT( at_aiconsole, "Searching through %i ents\n", count );					

			 for (int i = 0; i < count; i++) 
			{
				pEnt = pList[i];

				if ( !pEnt->IsAlive() ) continue; //dont count anything dead

				if ( m.pScriptedEnt->entindex() == pEnt->entindex() ) continue; //dont count self

				if ( spec_search )
				{
					if ( pSpecificEnt->entindex() == pEnt->entindex() ) return "1";
					continue;
				}

				if( !stricmp("player",Name) && pEnt->IsPlayer() )
					return EntToString(pEnt);

				if( !stricmp("monster",Name) && pEnt->IsMSMonster() && !pEnt->IsPlayer() )
					return EntToString(pEnt);

				if( !stricmp("any",Name) )
					return EntToString(pEnt);

				//This doesn't work here - works in $get_tsphere though :(
				if ( !strcmp("enemy",Name) )
				{
					CMSMonster *pMonster = (CMSMonster *)pEnt;
					int my_relate = pMonster->IRelationship(m.pScriptedEnt);
					if ( my_relate == -3 ) return EntToString(pEnt);
				}

				if ( !strcmp("ally",Name) )
				{
					CMSMonster *pMonster = (CMSMonster *)pEnt;
					int my_relate = pMonster->IRelationship(m.pScriptedEnt);
					if ( my_relate == 1 ) return EntToString(pEnt);
				}
			}
			return "0";
		}
#endif
#ifdef VALVE_DLL
		//$get_tbox() - same as $get_tsphere, but uses box for optimization when you don't need to be accurate
		//- tends to be rapidly used where it is, but rarely used, and more or less defunct
		//- should probably merge with $get_tsphere so the functions are consistent
		//priority: moderate, scope: server
		else if( ParserName.starts_with("$get_tbox") )
		{
			msstring &Name = Params[0];
			msstring thoth_token_string;
			msstring ent_str;
			int str_limit = 214;
			float thoth_boxsize = atof(Params[1]);
			Vector thoth_delta = Vector( thoth_boxsize, thoth_boxsize, thoth_boxsize );
			//float neg_boxsize = thoth_boxsize * -1;
			CBaseEntity *pList[255], *pEnt = NULL;
			Vector StartPos;
			if ( Params.size() == 2 ) StartPos = m.pScriptedEnt->pev->origin;
			else if ( Params.size() >= 3 ) StartPos = StringToVec(Params[2]);
			int count = UTIL_EntitiesInBox( pList, 255, StartPos - thoth_delta, StartPos + thoth_delta, FL_CLIENT|FL_MONSTER );
	
			//ALERT( at_aiconsole, "Searching through %i ents\n", count );					

			 for (int i = 0; i < count; i++) 
			{
				if ( (int)thoth_token_string.len() > str_limit ) continue;

				pEnt = pList[i];

				if ( !pEnt->IsAlive() ) continue; //dont count anything dead

				if ( m.pScriptedEnt->entindex() == pEnt->entindex() ) continue; //dont count self

				if( !stricmp("player",Name) && pEnt->IsPlayer() )
				{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
				}

				if( !stricmp("monster",Name) && pEnt->IsMSMonster() && !pEnt->IsPlayer() )
				{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
				}

				if( !stricmp("any",Name) && !pEnt->IsMSItem() )
				{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
				}

				if ( !strcmp("enemy",Name) )
				{
					CMSMonster *pMonster = (CMSMonster *)pEnt;
					int my_relate = pMonster->IRelationship(m.pScriptedEnt);
					if ( my_relate == -3 && pMonster->m_Race )
					{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
					}
				}

				if ( !strcmp("ally",Name) )
				{
					CMSMonster *pMonster = (CMSMonster *)pEnt;
					int my_relate = pMonster->IRelationship(m.pScriptedEnt);
					if ( my_relate == 1 && pMonster->m_Race )
					{
						ent_str = EntToString(pEnt);
						int total_len = thoth_token_string.len() + ent_str.len();
						if ( total_len < str_limit )
						{
							thoth_token_string += ent_str;
							thoth_token_string += ";";
						}
					}
				}
			}
			if ( thoth_token_string.len() > 0 ) return thoth_token_string.c_str();
			else return "none";
		}
#endif
		//MiB Feb2008a
		//$mid(<var>,<start>,<length>) - Returns a substring of <var> starting at <start> and going <length> characters
		//priority: moderate, scope: shared
		else if( ParserName == "$mid" )
		{
			if( Params.size() >= 3 )
			{
				int start = atoi( Params[1] );
				int length = atoi( Params[2] );
				return Params[0].substr( start, length );
			}
			else
				return "0";
		}
		//MiB Feb2008a
		//$right(<var>,<length>) - Returns a substring of size <length> from the right side of <var>
		//priority: moderate, scope: shared
		else if( ParserName == "$right" || ParserName == "$left" )
		{
			if( Params.size() >= 2 )
			{
				int length = atoi(Params[1]); //MAR2008a Thothie - the -1 here fubared this
				length = max( 0 , min(length, (signed)Params[0].len() ) ); 
				if( ParserName == "$left" )
					return Params[0].substr( 0 , length );
				else
					return Params[0].substr( Params[0].len() - length);
			}
			else 
				return "0";
		}
		//MiB Feb2008a
		//$replace(<var>,<replace_string>,<start>) - Overwrites a section of <var> with <replace_string> starting at <start>
		//$insert(<var>,<replace_string>,<start>) - Same, but doesn't overwrite, obviously.
		//priority: moderate, scope: shared
		else if( ParserName == "$replace" || ParserName == "$insert" )
		{
			if( Params.size() >= 3 )
			{
				msstring var = Params[0];
				msstring rstr = Params[1];
				int start = atoi( Params[2] );

				msstring temp = var.substr( 0 , start );
				temp += rstr;
				if( ParserName == "$replace" )
				{
					if( temp.len() < var.len() )
						temp += var.substr( temp.len() );
				}
				else
				{
					temp += var.substr( start );
				}
				return temp;
			}
			else
				return "0";
		}
#ifndef VALVE_DLL
		//Thothie - CL side tsphere
		//$getcl_tsphere(<origin>,<radius>,[flags]) - returns indexes of all ents in area as token string
		//priority: moderate, scope: client
		//this is rapidly called, but only used in one or two scripts
		else if( ParserName.starts_with("$getcl_tsphere") )
		{
			Vector scanpos = StringToVec(Params[0]);
			float scanrange = atof(Params[1]);
			msstring out_str;

			int numents = 1024; //only returnign worldspawn ents - MSCLGlobals::m_ClEntites.size();
			bool flag_justplayers = false;
			bool flag_incbrushes = false;
			Vector Start;
			Vector End;
			float Dist;
			msstring sTemp;

			if ( Params[2].contains("players") ) flag_justplayers = true;
			if ( Params[2].contains("addbrushes") ) flag_incbrushes = true;
			for ( int idx = 1; idx < numents; idx++ )
			{
				cl_entity_t *pclEntity = gEngfuncs.GetEntityByIndex( idx );
				if ( !pclEntity ) continue;
				if ( !pclEntity->model ) continue;
				if ( !pclEntity->index ) continue;
				if ( pclEntity->origin == g_vecZero ) continue; //brush model with no origin - can't get distance
				if ( !flag_incbrushes )
				{
					sTemp = UTIL_VarArgs("%s",pclEntity->model);
					if ( sTemp.contains("*") ) continue;
				}
				if ( flag_justplayers )
				{
					if ( !pclEntity->player ) continue;
				}
				Start = scanpos;
				End = pclEntity->origin;
				Dist = (Start-End).Length( );
				//Print("$getcl_tsphere: %i %s (%f vs %f) \n",pclEntity->index, pclEntity->model, Dist, scanrange);
				if ( Dist <= scanrange )
				{
					out_str += UTIL_VarArgs("%i;",pclEntity->index);
				}
			}
			if ( out_str.len() > 0 ) return out_str.c_str();
			else return "none";
		}
#endif

//=======================================================================================
//Low Priority Functions
//rarely called, and never rapidly
//=======================================================================================
#ifdef VALVE_DLL
		//priority: low, scope: server
		else if( ParserName == "$item_exists" )	//( <target>, <itemname>, <flags> )
		{
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
					if( Flags.contains("onlyhands") ) //MIB MAR2008a
					{
						ItemDesc.IgnoreHands = false; 
						ItemDesc.IgnoreWornItems = true; 
						ItemDesc.IgnoreInsideContainers = true; 
					}

					bool Found = pPlayer->GetItem( ItemDesc );
					if( Found ) 
					{
						if ( Flags.contains("remove") )
						{
							//Thothie hack - attempting to remove items remotely (can't remove items from players with removeitem command, so need this.)
							//- this be a hack, since we're using a return to do a method, one side effect is item doesn't vanish proper on the client
							pPlayer->RemoveItem( ItemDesc.retItem );
						}
						if ( Flags.contains("name") )
						{
							//Thothie hack - more hax, I'm sure way to get an item's proper name from the item list without spawning it
							//- however, I dun think I have time to figure it out before the next release. This at least works without issue.
							CGenericItem *pItem = NewGenericItem(Params[1]);
							pItem->DelayedRemove( );
							return pItem->DisplayName();
							//attempt fail:
							//return CGenericItemMgr::GetItemDisplayName( Params[1], false, true, 1 );
						}
						else
						RETURN_TRUE
					}
				}
			return "0";
		}
#endif
		//priority: low, scope: shared
		else if( ParserName == "$veclen2D" )		//Length of the vector (2D)
		{
			if( Params.size() >= 1 )
			{
				Vector Start = StringToVec( Params[0] );
				RETURN_FLOAT( Start.Length2D() );
			}
			else return "0";
		}
#ifdef VALVE_DLL
		//priority: low, scope: server
		else if( ParserName == "$get_quest_data" )	//( <target>, <questname> )
		{
				msstring &Name = Params[1];
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;

				if( pEntity && pEntity->IsPlayer( ) )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					 for (int i = 0; i < pPlayer->m_Quests.size(); i++) 
						if( pPlayer->m_Quests[i].Name == Name )
							return pPlayer->m_Quests[i].Data;
				}
			return "0";
		}
#endif
		//MiB JUL2010_02   $get_skillname(<skill>)
		// Turns "skill.bluntarms" to "Blunt Arms" (skill. is optional)
		//priority: low, scope: shared
		else if( ParserName.starts_with("$get_skillname") )
		{
			if ( Params.size() >= 1 )
			{
				msstring Skill = Params[0].starts_with("skill.") ? Params[0].substr(6) : Params[0];
				return GetSkillName( GetSkillStatByName( Skill.c_str() ) );
			}
		}
		//MiB Feb2008a
		//$cap_first(<var>) - Capitalize the first character of <var>
		//priority: low, scope: shared
		else if( ParserName == "$cap_first" )
		{
			if( Params.size() >= 1 )
			{
				msstring sen = Params[0];
				msstring f = sen;
				f = strupr( f );
				sen[0] = f[0];
				return sen;
			}
			else
				return "0";
		}
		//MiB Feb2008a
		//$string_upto(<search>,<string>,[start]) - Return everything up to <search>
		//$string_from(<search>,<string>,[start]) - Return everything after <search>
		//priority: low, scope: shared
		else if( ParserName == "$string_upto" || ParserName == "$string_from" )
		{
			if( Params.size() >= 2 )
			{
				int start = Params.size() >= 3? atoi( Params[2] ) : 0;
				msstring temp = Params[0].thru_substr( Params[1] , start );
				if( temp != Params[0] ) //This is the case if the search string is at the beginning of the string
				{
					if( ParserName == "$string_upto" )
						return temp;
					else
					{
						if( temp.len() + Params[1].len() >= Params[0].len() - 1 )
							return "";
						return Params[0].substr( temp.len() + Params[1].len());
					}
				}
				else if( Params[0].starts_with( Params[1] ) )
				{
					if( ParserName == "$string_upto" )
						return "";
					return Params[0].substr( Params[1].len() );
				}
			}

			return "0";
		}
#ifdef VALVE_DLL
		//MiB DEC2007a - return stats of an attack
		//$get_attackprop(<target>,<attack_idx>,<property>)
		//priority: low, scope: server
		else if( ParserName.starts_with("$get_attackprop") )
		{
			if( Params.size() >= 2 )
			{
				CGenericItem *pItem = (CGenericItem *) RetrieveEntity( Params[0] );
				int attackNum = atoi(Params[1]);
				attackdata_t &AttData = pItem->m_Attacks[attackNum];
				msstring &PropName = Params[2];
				
				if( PropName == "type" ) return AttData.sDamageType;
				else if( PropName == "range" ) RETURN_FLOAT( AttData.flRange )
				else if( PropName == "dmg" ) RETURN_FLOAT( AttData.flDamage )
				else if( PropName == "dmg.range" ) RETURN_FLOAT( AttData.flDamageRange )
				else if( PropName == "dmg.type" ) return AttData.sDamageType;
				else if( PropName == "dmg.multi" ) RETURN_FLOAT(AttData.f1DmgMulti)
				else if( PropName == "aoe.range" ) RETURN_FLOAT( AttData.flDamageAOERange )
				else if( PropName == "aoe.falloff" ) RETURN_FLOAT( AttData.flDamageAOEAttn )
				else if( PropName == "energydrain" ) RETURN_FLOAT( AttData.flEnergy )
				else if( PropName == "mpdrain" ) RETURN_FLOAT( AttData.flMPDrain )
				//else if( PropName == "stat" ) return STRING( AttData.
				else if( PropName == "stat.prof" ) RETURN_INT( AttData.StatProf )
				else if( PropName == "stat.balance" ) RETURN_INT( AttData.StatBalance )
				else if( PropName == "stat.power" ) RETURN_INT( AttData.StatPower )
				else if( PropName == "stat.exp" ) RETURN_INT( AttData.StatExp )
				else if( PropName == "noise" ) RETURN_FLOAT( AttData.flNoise )
				else if( PropName == "callback" ) return AttData.CallbackName;
				else if( PropName == "ofs.startpos" ) return STRING( VecToString( AttData.StartOffset ) );
				else if( PropName == "ofs.aimang" ) return STRING( VecToString( AttData.AimOffset ) );
				else if( PropName == "priority" ) RETURN_INT( AttData.iPriority )
				else if( PropName == "keys" ) return AttData.ComboKeys[i];
				else if( PropName == "dmg.ignore" ) return AttData.NoDamage ? "1":"0";
				else if( PropName == "hitchance" ) RETURN_FLOAT( AttData.flAccuracyDefault )
				else if( PropName == "delay.end" ) RETURN_FLOAT( AttData.tDuration )
				else if( PropName == "delay.strike" ) RETURN_FLOAT( AttData.tLandDelay )
				else if( PropName == "dmg.noautoaim" ) return AttData.NoAutoAim ? "1":"0";
				else if( PropName == "ammodrain" ) RETURN_INT( AttData.iAmmoDrain )
				else if( PropName == "hold_min&max" )
				{
					sprintf(Return, "%.2f;%.2f", AttData.tProjMinHold, AttData.tMaxHold );
					return Return;
				}
				else if( PropName == "projectile" ) return STRING( AttData.sProjectileType );
				else if( PropName == "COF" )
				{
					sprintf(Return, "%.2f;%.2f", AttData.flAccuracyDefault, AttData.flAccBest );
					return Return;
				} 
				else if( PropName == "delay.end" ) return STRING( AttData.tDuration );
				else return "ILLEGAL_PROPERTY";
			}
			else 
				return "-1";
		}
#endif
		//Thothie - JUNE2007 $get_cvar(<cvar_name>)
		//priority: low, scope: shared
		else if( ParserName.starts_with("$get_cvar") )
		{
			msstring thoth_cvar_returnb = Params[0];
			return EngineFunc::CVAR_GetString(thoth_cvar_returnb.c_str());
		}
		//Thothie NOV2014_22 comment: wut? Was wondeirng why this didn't seem to work...
		//dox says this returns the distance between two vectors, but that doesn't look like what it is doing...
		//priority: low, scope: shared
		else if( ParserName == "$veclen" )		//Length of the vector
		{
			if( Params.size() >= 1 )
			{
				Vector Start = StringToVec( Params[0] );
				RETURN_FLOAT( Start.Length() );
			}
			else return "0";
		}
		//MiB Feb2008a
		//$filesize(file) - returns size of file
		//- for map verification mostly, atm
		//priority: low, scope: server
		else if( ParserName == "$filesize" )  
		{
			if( Params.size() >= 1 ) 
			{ 
				char cfileName[MAX_PATH];
				#ifdef VALVE_DLL
					GET_GAME_DIR( cfileName );
				#else
					strcpy( cfileName, gEngfuncs.pfnGetGameDirectory( ) );
				#endif

				//GET_GAME_DIR( cfileName );
				msstring fileName = cfileName;
				fileName += "/";
				fileName += Params[0];
				ifstream file;
				file.open( fileName.c_str(), ios_base::in );
				if( file.is_open() )
				{
					file.seekg( 0 , ios_base::end );
					long fileSize = file.tellg();  
					file.close();
					RETURN_INT( fileSize );
				}
				else RETURN_INT(-1)
			}
		}
//=======================================================================================
//Very Low Priority Functions
//called maybe once per map or player join
//=======================================================================================
#ifdef VALVE_DLL
		//Thothie $get_lastmap(<target>) - retrieve lastmap player was on
		//priority: very low, scope: server
		else if( ParserName.starts_with("$get_lastmap") )	//( <target> )
		{
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;

				if( pEntity && pEntity->IsPlayer( ) )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					return pPlayer->m_cEnterMap;
				}
			return "[unknown]";
		}
#endif
#ifdef VALVE_DLL
		//Thothie $get_map_valid(<target>) - retrieve map status player was on
		//priority: very low, scope: server
		else if( ParserName.starts_with("$get_map_legit") )	//( <target> )
		{
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;

				if( pEntity && pEntity->IsPlayer( ) )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					if ( pPlayer->m_MapStatus == INVALID_MAP ) return "0";
					if ( pPlayer->m_MapStatus != INVALID_MAP ) return "1";
				}
			return "[unknown]";
		}
#endif
		//Thothie $get_time(month|day|year|version) - used to verify MS.DLL / holiday events
		//priority: very low, scope: server
		else if( ParserName.starts_with("$get_time") )	//( <target> )
		{
			msstring &Flags = Params[0];
			/*
			 //always returns 0, should return 0-11 :\
			struct tm *timeinfo = (struct tm *)malloc(sizeof(struct tm));
			
			if( Flags.contains("month" ) )
			{
				int gtmonth = timeinfo->tm_mon;
				ALERT( at_console, "Pulled %i %d %f %s", gtmonth,gtmonth,gtmonth,gtmonth);
				return UTIL_VarArgs("%i",gtmonth);
			}*/

			if( Flags.contains("version" ) )
			{
				return EngineFunc::CVAR_GetString("ms_version"); //Thothie JUN2007a - changed this to work proper (broken ver was added JAN2007b)
			}
		}
#ifdef VALVE_DLL
		//Thothie $get_jointype(<target>) - retrieve map status player was on
		//priority: very low, scope: server
		else if( ParserName.starts_with("$get_jointype") )	//( <target> )
		{
				CBaseEntity *pEntity = m.pScriptedEnt ? m.pScriptedEnt->RetrieveEntity( Params[0] ) : NULL;

				if( pEntity && pEntity->IsPlayer( ) )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)pEntity;
					const char *JoinTypeText = "unknown";
					if( pPlayer->m_CharacterState == CHARSTATE_UNLOADED ) JoinTypeText = "Spectate";
					else
					{
						switch( pPlayer->m_JoinType )
						{
							case JN_TRAVEL:		JoinTypeText = "Travel"; break;
							case JN_STARTMAP:	JoinTypeText = "Start Map"; break;
							case JN_VISITED:	JoinTypeText = "Previously Visited"; break;
							case JN_ELITE:		JoinTypeText = "GM"; break;
							case JN_NOTALLOWED: JoinTypeText = "Not Allowed"; break;
						}
					}
					return JoinTypeText; // pPlayer->m_JoinType
				}
			return "[unknown]";
		}
#endif
#ifdef VALVE_DLL
		//Thothie - JAN2008a - attempting to verify map existance to allow future map vote system 
		//- $map_exists(<mapname>) - sans the .bsp
		//priority: very low, scope: server
		else if( ParserName == "$map_exists" )
		{
			return ( IS_MAP_VALID(Params[0]) ? "1" : "0" );
		}
#endif
#ifdef VALVE_DLL
		//MiB Feb2008a – “Read” a line from a file
		//- $get_fileline(<file>,[line])
		//priority: very low, scope: server
		else if( ParserName == "$get_fileline" )
		{
			if( Params.size() >= 1 )
			{
				msstring fileName = Params[0];

				bool found = false;
				if( m.pScriptedEnt->filesOpen.size() >= 1 )
				{
					 for (int i = 0; i < m.pScriptedEnt->filesOpen.size(); i++) 
					{
						//Check to see if we already have this file open
						if( m.pScriptedEnt->filesOpen[i].fileName == fileName )
						{
							found = true;
							if( Params.size() >= 2 )
								return m.pScriptedEnt->filesOpen[i].ScriptFile_ReadLine( atoi( Params[1] )); //Read specific line
							else
								return m.pScriptedEnt->filesOpen[i].ScriptFile_ReadLine(); //Read next line

							break;
						}
					}
				}
				
				if( !found ) //The file wasn't found
				{
					scriptfile_t file;
					file = fileName; //Open the file
					m.pScriptedEnt->filesOpen.add( file ); //Add it to the list of opened files
					if( Params.size() >= 2 )
						return m.pScriptedEnt->filesOpen[ m.pScriptedEnt->filesOpen.size() - 1 ].ScriptFile_ReadLine( atoi( Params[1] ) );
					else
						return m.pScriptedEnt->filesOpen[ m.pScriptedEnt->filesOpen.size() - 1 ].ScriptFile_ReadLine();
				}
			}
		}
#endif
#ifdef VALVE_DLL
		//MiB Feb2008a
		//$get_fnfileline(<fileName>) - If the file is ready to read, returns the line, otherwise warns the script that it's not ready to be read
		//priority: very low, scope: server
		else if( ParserName == "$get_fnfileline" )
		{
			if( Params.size() >= 1 )
			{
				msstring fileName = Params[0];
				 for (int i = 0; i < m.pScriptedEnt->filesOpenFN.size(); i++) 
				{
					if( m.pScriptedEnt->filesOpenFN[i].fileName == fileName )
					{
						if( m.pScriptedEnt->filesOpenFN[i].readyForRead )
						{
							if( Params.size() >= 2 )
								return m.pScriptedEnt->filesOpenFN[i].ScriptFile_ReadLine( atoi( Params[1] )); //Read specific line
							else
								return m.pScriptedEnt->filesOpenFN[i].ScriptFile_ReadLine(); //Read next line
						}
						else
						{
							return "[NOT_READY_FOR_READ]";
						}
					}
				}

				//We've made it here, thus we don't have the file yet
				scriptfile_t File;
				File.fileName = fileName;
				File.readyForRead = false;
				m.pScriptedEnt->filesOpenFN.add( File );

				MSCentral::ReadFNFile( fileName, EntToString(m.pScriptedEnt) );

				return "[NOT_READY_FOR_READ]";

			}
			else return 0;
		}
#endif
		//priority: very low, scope: shared
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
		//priority: very low, scope: shared
		else if( ParserName == "$ucase" )	//Make upper case - Thothie
		{
			if( Params.size() >= 1 )
			{
				char toconv[256];
				strcpy( toconv, Params[0] );
				return _strupr( toconv );
			}
			else
			{
				return "0";
			}
		}
		//priority: very low, scope: shared
		else if( ParserName == "$num" )	//Strip non number bits Thothie FEB2008a
		{
			//$num(<string>,[extras]) - strips non-digits except characters found in [extras]
			//eg. $num(SOME_INPUT,".")
			if( Params.size() >= 1 )
			{
				//yes, this is a shit way of doing this, but I'm in a rush...
				msstring num_conv = Params[0];
				msstring out_str;
				int in_length = num_conv.len();
				char *cc;
				bool char_legit;
				msstring extra_list;
				bool check_extras = false;
				if ( Params.size() > 1 )
				{
					extra_list = Params[1];
					check_extras = true;
				}
				for( int i = 0; i <= in_length; i++ )
				{
					char_legit = false;
					cc = num_conv.substr(i,1).c_str();
					if ( isdigit( cc[0] ) > 0 ) char_legit = true;
					if ( check_extras )
					{
						if ( extra_list.contains(cc) ) char_legit = true;
					}
					if ( char_legit ) out_str += cc;
				}
				return out_str.c_str();
			}
			else
			{
				return "0";
			}
		}
		//priority: very low, scope: shared
		else if( ParserName == "$alphanum" )
		{
			//$alphanum(<string>,[extras]) - strips non-alphanumeric except characters found in [extras]
			//eg. $alphanum(SOME_INPUT," ")
			if( Params.size() >= 1 )
			{
				//yes, this is a shit way of doing this, but I'm in a rush...
				msstring num_conv = Params[0];
				msstring out_str;
				int in_length = num_conv.len();
				char *cc;
				bool char_legit;
				msstring extra_list;
				bool check_extras = false;
				if ( Params.size() > 1 )
				{
					extra_list = Params[1];
					check_extras = true;
				}
				for( int i = 0; i <= in_length; i++ )
				{
					char_legit = false;
					cc = num_conv.substr(i,1).c_str();
					if ( isalnum( cc[0] ) > 0 ) char_legit = true;
					if ( check_extras )
					{
						if ( extra_list.contains(cc) ) char_legit = true;
					}
					if ( char_legit ) out_str += cc;
				}
				return out_str.c_str();
			}
			else
			{
				return "0";
			}
		}
		//MiB FEB2010_05 - Return the max/min value of a given list
		//$max(<1>,[2]..[n])
		//$min(<1>,[2]..[n])
		//Thothie NOV2014 comments: We've yet to use this for anything, but nice to have, I suppose
		//priority: very low, scope: shared
		else if( ParserName == "$max" || ParserName == "$min" )
		{
			if ( Params.size() >= 1 )
			{
				float best = atof( Params[0].c_str() );
				bool max = ParserName == "$max";
				 for (int i = 0; i < Params.size(); i++) 
					best = max ? max( best , atof( Params[i].c_str() ) ) : min( best, atof( Params[i].c_str() ) ) ;
				RETURN_FLOAT( best );
			}
		}
#ifndef VALVE_DLL
		//MiB FEB2010_02
		//$get_local_prop(<property>)
		//Returns a property about the client player. Difference between this and $getcl is that it accesses
		//CBasePlayer properties of the client, too. Could also add the cl_entity_t properties of $getcl if 
		//one wants to save time/processing speed.
		//priority: very low, scope: client
		else if( ParserName == "$get_local_prop" )
		{
			if ( Params.size() >= 1 )
			{
				msstring Prop = Params[0];

				if ( Prop == "gender" ) return player.m_Gender == GENDER_MALE ? "male" : "female";
				else if ( Prop == "race" ) return player.m_Race;
			}
			else
				return "0";
		}
#endif
	} //end if starwith $
	//============================================================================
	//============================================================================
	//============================================================================
	//============================================================================
	//============================================================================
	//^ big marjer while I sort priorities above
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
	else if( FullName.starts_with("CheatEngineCheck") )
	{
		CheckIfUsingCE();
		//ALERT( at_console, "Checking memory encryption integrity..." );
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

		if( Name == "time" )
		{
			RETURN_FLOAT( gpGlobals->time )
		}
		else if( Name.starts_with("time.") ) 
		{ 
			// game.time.* functions 
			// MiB OCT2007a 
				time_t curTime; 
				time( &curTime ); 

				struct tm* TheTime = localtime( &curTime ); 

		                                
				msstring dayInfo = ctime( &curTime ); 
				int theReturn = -1; 
				int day = TheTime->tm_mday;         // Correct day of the MONTH 
				int month = TheTime->tm_mon + 1;   // Zero indexed month 
				int year = TheTime->tm_year + 1900;   // Zero indexed year (starting with 1900) 
				int DoW = TheTime->tm_wday + 1;      // Zero indexed Day of Week (0 == Sunday) 
		        int thoth_hour = TheTime->tm_hour;		//hour (24 format?)
				int thoth_minute = TheTime->tm_min;		//minute (past the hour?)

				if( Name.starts_with("time.day") )         theReturn = day; 
				else if( Name.starts_with("time.minute") )   theReturn = thoth_minute; 
				else if( Name.starts_with("time.hour") )   theReturn = thoth_hour; 
				else if( Name.starts_with("time.year") )   theReturn = year; 
				else if( Name.starts_with("time.month") )   theReturn = month; 
				else if( Name.starts_with("time.year") )   theReturn = year; 
				else if( Name.starts_with("time.dow") )      theReturn = DoW; 
				else if( Name.contains(".since") ) //game.time.*.since or game.time.minutes 
				{ 
		            
					//Create NewYears as a new time 

					time_t NewYears; 
					struct tm * timeinfo; 

					time( &NewYears ); 

					timeinfo = localtime( &NewYears ); 
		            
					timeinfo->tm_hour = 0;   // 12.XX.XX AM 
					timeinfo->tm_min = 0;   // 12.00.XX AM 
					timeinfo->tm_sec = 0;   // 12.00.00 AM 

					timeinfo->tm_mon = 0;   // January XX 
					timeinfo->tm_mday = 1;   // January 01 NOT ZERO INDEXED 
		            

					timeinfo->tm_yday = 0;   // January 1 "Days since Jan 1" - redundant, but if one works and the other doesn't, it's worth it 
		            
					NewYears = mktime( timeinfo ); 

					//***timeinfo->tm_year = 100;// Y2K -> 2000 - 1900 
					// RAWR - Thought you said Y2K >_< Anywhoo, here if you want it 

					// Extra flags that you can mess with if ya want. 
					// timeinfo->tm_isdst;   //DaylightSavingsTime flag - it IS an int 
					// timeinfo->tm_wday;   //[0-6] "Days since Sunday" 
		            
					//End Creation 


					int dif = (int) difftime( curTime, NewYears ); // Seconds since new years 

					// Now output the time since New Years in seconds, minutes, hours, or days. 

					if( Name.contains( "seconds" ) )   //game.time.seconds.since 
						theReturn = dif; 
					else if( Name.contains( "minutes" ) ) //game.time.minutes.since or game.time.minutes 
						theReturn = dif/60; 
					else if( Name.contains( "hours" ) ) //game.time.hours.since 
						theReturn = dif/60/60; 
					else if( Name.contains( "days" ) ) //game.time.days.since 
						theReturn = dif/60/60/24; 
		            
				} 

				RETURN_INT( theReturn ); 
			} 
			else if( Name == "frametime" ) RETURN_FLOAT( gpGlobals->frametime )
#ifdef VALVE_DLL
			else if( Name == "maxlevel" )
			{
				RETURN_INT( CHAR_LEVEL_CAP );
			}
			//Thothie FEB2013_24 - consolidating all "game.players.xxx" functions into one conditional, and adding noafk
			else if( Name.starts_with("players") )
			{
				//Thothie JUN2007a - make sure game.players does not return bots
				//Thothie NOV2014_09 - using new centralized checking
				if ( Name.contains("totalhp") ) RETURN_FLOAT( UTIL_TotalHP() )
				else if ( Name.contains("avghp") ) RETURN_FLOAT( UTIL_AvgHP() )
				else if ( Name.contains("playersnb") || Name.contains("noafk") ) RETURN_INT( UTIL_NumActivePlayers() )
				else
				{
					RETURN_INT( UTIL_NumPlayers() )
				}
			}
#endif
		//seems we commented a little too far in JUL2013a (fixed)
		//Thothie JAN2013_10 - Dynamic music system (tabs be fubar here for some reason)
		else if( Name.starts_with("map.music.") )
		{
			msstring Prop = Name.substr( 10 );
			if( Prop == "idle.file" ) return MSGlobals::map_music_idle_file;
			else if( Prop == "idle.length" ) return MSGlobals::map_music_idle_length;
			else if( Prop == "combat.file" ) return MSGlobals::map_music_combat_file;
			else if( Prop == "combat.length" ) return MSGlobals::map_music_combat_length;
		}
		else if( Name.starts_with("map.") )
		{
			bool Type[2] = { false };
			msstring Prop = Name.substr( 4 );
			if( Prop == "name" ) return MSGlobals::MapName;
			else if( Prop == "skyname" ) return EngineFunc::CVAR_GetString( "sv_skyname" );
			else if( Prop == "title" )
			{
				msstring thoth_tstring = MSGlobals::MapTitle;
				if ( thoth_tstring.len() > 1 ) return MSGlobals::MapTitle;
				else return "0";

			}
			else if( Prop == "weather" ) //Thothie SEP2007a
			{
				msstring thoth_weather = MSGlobals::MapWeather;
				if ( thoth_weather.len() > 1 ) return MSGlobals::MapWeather;
				else return "0";
			}
			else if( Prop == "desc" ) //Thothie SEP2007a, updated OCT2007a
			{
				msstring thoth_tstring = MSGlobals::MapDesc;
				if ( thoth_tstring.len() > 1 ) return MSGlobals::MapDesc;
				else return "0";
			}
			else if( Prop == "maxviewdistance" ) //Thothie SEP2007a, updated OCT2007a
			{
				RETURN_FLOAT(MSGlobals::maxviewdistance)
			}
			else if( Prop == "hpwarn" ) //Thothie SEP2007a, updated OCT2007a
			{
				msstring thoth_tstring = MSGlobals::HPWarn;
				if ( thoth_tstring.len() > 1 ) return MSGlobals::HPWarn;
				else return "0";
			}
		} //end if map.
		else if( Name == "debug" ) return EngineFunc::CVAR_GetString("developer"); //Thothie MAR2007b - so we can disable client-side debugs from the script level (failed at the .dll level)
		else if( Name == "developer" ) return EngineFunc::CVAR_GetString("developer");  //ditto, alias
		else if( Name == "pvp" )
		{
			//Thothie FEB2008a - having it return the global rule, rather than cvar, for better dynamics
			return MSGlobals::PKAllowed ? "1" : "0";
			//return EngineFunc::CVAR_GetString("ms_pklevel"); //Thothie MAR2007b - the long awaited script PVP awareness
		}
		else if( Name == "central" ) return EngineFunc::CVAR_GetString("ms_central_enabled"); //Thothie MAY2007a - the long awaited FN awareness
		else if( Name == "revision" ) return EngineFunc::CVAR_GetString("ms_version"); //Thothie MAY2007a - the long awaited script side version control
		else if( Name == "clientside" )  fSuccess = !IsServer;
		else if( Name == "serverside" )  fSuccess = IsServer;
		else if( Name.starts_with("cvar.") )
		{
			msstring thoth_cvar_return = Name.substr( 5 );
			return EngineFunc::CVAR_GetString(thoth_cvar_return.c_str());
		}
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
				else if( Prop == "viewangles" ) RETURN_VECTOR(player.pev->v_angle) //Thothie JAN2013_08 - viewangles, sorta, only returns yaw
				else if( Prop == "eyepos" ) RETURN_VECTOR(player.EyePosition()) //Thothie JAN2013_08 eyepos - sorta, returns pos on model
				else if( Prop == "thirdperson" ) RETURN_INT( (MSCLGlobals::CamThirdPerson ? 1 : 0) )
				else if( Prop == "viewmodel.left.id"  || Prop == "viewmodel.0.id" ) RETURN_INT( player.Hand( 0 ) ? player.Hand( 0 )->GetViewModelID( ) : -1 )
				else if( Prop == "viewmodel.right.id" || Prop == "viewmodel.1.id" ) RETURN_INT( player.Hand( 1 ) ? player.Hand( 1 )->GetViewModelID( ) : -1 )
				else if( Prop == "viewmodel.active.id" ) RETURN_INT( player.ActiveItem( ) ? player.ActiveItem( )->GetViewModelID( ) : -1 )
				else if( Prop == "canattack" ) RETURN_INT( FBitSet(player.m_StatusFlags,PLAYER_MOVE_NOATTACK) ? 0 : 1 )
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
	} //end if game.
	//NOV2014_22 comment: seems we shoulda checked this a LOT earlier:
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
	//pScriptedEnt->ScriptFName = Filename; //MiB DEC2007a - scriptname prop - phayle
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

	//MiB JUL2010_13 - *EXT/ means to external script folder. Lets mappers script.
	//- Thothie - not intuitive, switching to test_scripts/ - same folder scripts are to be placed in.
	bool MapperScript = ScriptName.starts_with("test_scripts/") && MSGlobals::DevModeEnabled;
	if ( MapperScript )
	{
		ScriptName = ScriptName.substr(13); //Get rid of the *EXT/
		m.ScriptFile = ScriptName.thru_char(".").c_str();
	}

	#ifndef SCRIPT_LOCKDOWN
		char cScriptFile[MAX_PATH];
		sprintf( cScriptFile, "test_scripts/%s", ScriptName.c_str() );

		int iFileSize;
		byte *pMemFile = LOAD_FILE_FOR_ME( cScriptFile, &iFileSize );
		if( pMemFile ) 
		{
			ScriptData = msnew(char[iFileSize+1]);
			memcpy( ScriptData, pMemFile, iFileSize );
			ScriptData[iFileSize] = 0;
			FREE_FILE( pMemFile );
		}
		else
	#endif
		{
			unsigned long ScriptSize;
			if ( MapperScript && !MSGlobals::CentralEnabled )
			{
				char cScriptFile[MAX_PATH], cGameDir[MAX_PATH];
				#ifdef VALVE_DLL
					GET_GAME_DIR( cGameDir );
				#else
					strcpy( cGameDir, gEngfuncs.pfnGetGameDirectory( ) );
				#endif
				sprintf( cScriptFile, "test_scripts/%s", ScriptName.c_str() ); //Thothie FEB2010_06 - attempting to fix other folks not being able to use test_scripts folder

				int iFileSize;
				byte *pMemFile = LOAD_FILE_FOR_ME( cScriptFile, &iFileSize );
				if( pMemFile )
				{
					ScriptData = msnew(char[iFileSize+1]);
					memcpy( ScriptData, pMemFile, iFileSize );
					ScriptData[iFileSize] = 0;
					FREE_FILE( pMemFile );
				}
			}
			else if( ScriptMgr::m_GroupFile.ReadEntry( ScriptName, NULL, ScriptSize ) )
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
							ALERT( at_notice, "Script file: \"%s\" NOT FOUND!\n", ScriptName.c_str() ); //thothie - moved to at_notice in hopes of getting bogus script reports
					#else
						#ifndef RELEASE_LOCKDOWN
#ifdef VALVE_DLL
							logfile << "ERROR: Script not found: " << ScriptName.c_str() << endl;
							MessageBox( NULL, msstring("Script not found: ") + ScriptName + "\r\n\r\nThis is probably caused by a script using #include on a non-existant script.", "FIX THIS QUICK!", MB_OK );
#endif
						#else
							//In the release build, this is a fatal error
							//SERVER_COMMAND( "exit\n" ); This crashes the game, currently
	//server side so we can retain ability to add server side only scripts
#ifdef VALVE_DLL
						MessageBox( NULL, msstring("Script not found: ") + ScriptName, "MAP SCRIPT ERROR", MB_OK ); //Thothie - JUN2007 Trying to get script bugs to report
						//exit( 0 ); //MAR2008a Thothie - making non-fatal so it can report multiple
#endif
						#endif
					#endif
				}
				return false;
			}
		}

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
	startdbg;
	dbg( "Proc_Events" );
	//Run script events
	//~ Runs unnamed events or named events that were specified with calleventtimed ~
	int events = m.Events.size();
	 for (int i = 0; i < events; i++) 
	{
		SCRIPT_EVENT &Event = m.Events[i];
		//Skip unnamed events when running named events only
		msstring thoth_event_name = (m.ScriptFile.c_str());
		thoth_event_name.append("->");
		thoth_event_name.append(Event.Name);
		dbg( thoth_event_name );
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
	enddbg;
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
			 !stricmp(TestCommand,	"const"			) )
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

#if !TURN_OFF_ALERT
			//Thothie JUN2013_08 - check for conflicts in developer builds as we go
			msstring testvar = VarName;
			msstring testvar_type;
			msstring testvar_scope = "preload";
            if ( !stricmp(TestCommand,"setvar") ) testvar_type = "setvar";
			if ( !stricmp(TestCommand,"const") ) testvar_type = "const";
			conflict_check(testvar,testvar_type,testvar_scope);
#endif

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
			else
			{
				bool AddConst = true;
				
				 for (int i = 0; i < m_Constants.size(); i++) 
				{
					if( m_Constants[i].Name == VarName )
					{
						AddConst = false;
						break;
					}
				}

				if( AddConst )
				{
                    m_Constants.add( scriptvar_t( VarName, VarValue ) );//Create new constant
				}
				//else if( !stricmp(TestCommand, "const_ovrd") ) m_Constants[i].Value = VarValue; //NOV2014_18 - no good here, runs at load time
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
#if !TURN_OFF_ALERT
			//Thothie JUN2013_08 - check for conflicts in developer builds as we go
			msstring tVarName = Line.thru_char( SKIP_STR );
			msstring testvar = tVarName;
			msstring testvar_type = "setvard";
			msstring testvar_scope = "preload";
			conflict_check(testvar,testvar_type,testvar_scope);
#endif
		strcpy( TestCommand, "setvar" );
		KeepCmd = true;
	}
	//this fails, as it sometimes runs at load time (not sure why)
	else if( !stricmp(TestCommand,	"const_ovrd"		) )
	{
		//Thothie NOV2014_18 - fixing const_ovrd to only work during run time
		msstring VarName = Line.thru_char( SKIP_STR );

		Line = Line.substr( VarName.len() ).skip( SKIP_STR );
		msstring VarValue;
		if( Line[0] == '"' ) VarValue = Line.substr(1).thru_char( "\"" );		//Starting quote found, read until the next quote
		else VarValue = Line.thru_char( SKIP_STR );				//No quotes

		VarValue = msstring(GETCONST_COMPATIBLE(VarValue));

		//keep command, but don't use until run time
		//HOW DO THIS!?
		if ( !m.PrecacheOnly )
		{
			Print("DEBUG: const_ovrd to replace %s with %s\n",VarName.c_str(),VarValue.c_str());
			 for (int i = 0; i < m_Constants.size(); i++) 
			{
				Print("DEBUG: const_ovrd checking %s vs %s\n",m_Constants[i].Name.c_str(),VarName.c_str());
				if ( m_Constants[i].Name == VarName )
				{
					m_Constants[i].Value = VarValue;
					Print("DEBUG: const_ovrd found! %s is now %s\n",m_Constants[i].Name.c_str(),m_Constants[i].Value.c_str());
					break;
				}
			}
		}
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
			 !stricmp(TestCommand,	"svplaysound"			) || 
			 //!stricmp(TestCommand,	"playsoundcl"		) ||  //never used
			 !stricmp(TestCommand,	"svplayrandomsound"	) ||
			 !stricmp(TestCommand,	"svsound.play3d"		) ||
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
		if( !stricmp(TestCommand,"sound.play3d") || !stricmp(TestCommand,"svsound.play3d") ) SndType = 1;	//Sound name is first parameter

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
					//Thothie JUN2007b - see thoth_pissed
					//- Well, this here explains why I couldn't stop it from parsing into waves
					Resolved = Resolved.thru_char("[");		//Special case for 'say'.  Get the sound separate from the delay
					if( Resolved == "*" ) { Resolved = ""; ALERT( at_console, "%s Warning: Old 'say' command using '*' as sound name\n", m.ScriptFile.c_str() ); }
					if( Resolved.len() )
					{
						if ( !Resolved.contains("RND_SAY") )
							Resolved = msstring("npc/") + Resolved + ".wav"; //still total h4x, but there's no good way to fix this without changing how the command works
                	}
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
				Print( "Found precache with name too small: %s in %s\n", FileName.c_str(),m.ScriptFile.c_str() );
				continue;
			}

			msstring Extension = &FileName[FileName.len()-4];
			//Thothie - Fail
			/*if ( !FileName.contains("items") )
			{
				int thoth_pre = gpGlobals->PreCount;
				thoth_pre++;
				gpGlobals->PreCount = thoth_pre;
			}*/

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
					if ( stricmp(TestCommand,"precache") ) PRECACHE_SOUND( pszGlobalPointer ); //Thothie MAR2012_27 - no longer precahing sounds from here, using client side sounds wherever possible
					break;
				case pctype_sprite:
					PRECACHE_MODEL( pszGlobalPointer );	break;
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
//Thothie JUN2007a, allows callexternal on all players, via "callexternal players [delay] <event> <params...>"
void CScript::CallScriptPlayers( msstring_ref EventName, msstringlist *Parameters )
{
		#ifdef VALVE_DLL
		//edict_t		*pEdict = NULL;
		//CBaseEntity *pEntity = NULL;
		
		IScripted *pScripted = NULL;

		for( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity *pEntity = UTIL_PlayerByIndex( i );
			CBasePlayer *pPlayer = (CBasePlayer *)pEntity;;
			if ( !pPlayer ) continue;

			pScripted = pPlayer->GetScripted( );
			if( !pScripted ) continue;		//Not scripted

			pScripted->CallScriptEvent( EventName, Parameters );
		}
	//#else
	//gHUD.m_HUDScript->CallScriptEvent( EventName, Parameters );*/
	/*if( MSGlobals::GameScript )
		MSGlobals::GameScript->CallScriptEvent( EventName, Parameters );*/
	#endif
}

//Thothie MAR2012_27 - send a client event to all CL player scripts
//equiv of: clientevent update all const.localplayer.scriptID <event> [params...]
void CScript::ClCallScriptPlayers( msstring_ref EventName, msstringlist *Parameters )
{
#ifdef VALVE_DLL
	if( g_netmsg[NETMSG_CLDLLFUNC] ) //g_netmsgs aren't initialized until the player is spawned... but this may be called earlier from the world.script
	{
		MESSAGE_BEGIN( MSG_ALL, g_netmsg[NETMSG_CLDLLFUNC], NULL );

		WRITE_BYTE( 17 );
		WRITE_BYTE( 1 ); //0 == Add | 1 == Update | 2 = Remove
		WRITE_LONG( -2 );
		int nParameters = Parameters->size();
		WRITE_BYTE( nParameters + 1 );
		WRITE_STRING( EventName );
		 for (int i = 0; i < nParameters; i++) 
			WRITE_STRING( (*Parameters)[i] );
		MESSAGE_END();
	}
#endif
}

//MAR2012_28 - Client Message Sound Function
//ClXPlaySoundAll( pszSound, m.pScriptedEnt->pev->origin, iChannel, m.pScriptedEnt->SndVolume, sAttn, sPitch );
//gEngfuncs.pEventAPI->EV_PlaySound( 0, *(Vector *)&Origin, CHAN_AUTO, Sound, Volume, Attn, 0, 100 );
void CScript::ClXPlaySoundAll( msstring_ref sSample, const Vector &Origin, int sChannel, float sVolume, float sAttn, int sPitch )
{
#ifdef VALVE_DLL
	if( g_netmsg[NETMSG_CLDLLFUNC] ) //g_netmsgs aren't initialized until the player is spawned... but this may be called earlier from the world.script
	{
		MESSAGE_BEGIN( MSG_ALL, g_netmsg[NETMSG_CLXPLAY], NULL );
		WRITE_STRING( sSample );
		WRITE_COORD( Origin.x );
		WRITE_COORD( Origin.y );
		WRITE_COORD( Origin.z );
		WRITE_BYTE( sChannel );
		WRITE_COORD( sVolume );
		WRITE_COORD( sAttn );
		WRITE_COORD( sPitch );
		MESSAGE_END();
	}
#endif
}


bool GetModelBounds( void *pModel, Vector Bounds[2] )
{
	studiohdr_t *pstudiohdr = (studiohdr_t *)pModel;
	if( !pstudiohdr )
		return false;

	return true;
}


//[MiB NOV2007a]
void CheckIfUsingCE()
{
	// Get the list of process identifiers.

	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return;

	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.

	for ( i = 0; i < cProcesses; i++ )
		if( aProcesses[i] != 0 )
			CheckProcess( aProcesses[i] );
}

void CheckProcess( DWORD processID )
{
	TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");

	// Get a handle to the process.

	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
								PROCESS_VM_READ,
								FALSE, processID );

	// Get the process name.

	if (NULL != hProcess )
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
			&cbNeeded) )
		{
			GetModuleBaseName( hProcess, hMod, szProcessName, 
							sizeof(szProcessName)/sizeof(TCHAR) );
		}
	}

	//MessageBox(NULL,UTIL_VarArgs("Checking Procs: %s",szProcessName),"DEBUG",MB_OK|MB_ICONEXCLAMATION); 

	// Check to see if this process is the "Cheat Engine.exe" process (Yes, that's actually what they named it.
	//if( strcmp( szProcessName, "Cheat Engine.exe" ) == 0 )
	//char toconv[256];
	//strcpy( toconv, Params[0] );
	//return _strlwr( toconv );

	char toconv[256];
	strcpy( toconv, szProcessName );
	msstring thoth_proc = _strlwr( toconv );

	if( thoth_proc.contains("cheat engine.exe") || thoth_proc.contains("artmoney.exe") || thoth_proc.contains("prjredux") )
	{
		// If it is
		// Do the things with the fucking up the cheater's game.
		// [THOTH] 
		MessageBox(NULL,"Memory area not secure. Cannot continue.","ERROR",MB_OK|MB_ICONEXCLAMATION); 
		exit (-1);
	}

#ifdef VALVE_DLL
	//Thothie FEB2008a - handle restarter
	if( thoth_proc.contains("restarter.exe") )
	{
		char cFileName[512];
		sprintf( cFileName, "%s/%s", EngineFunc::GetGameDir(), "heartbeat.txt" );
		std::remove(cFileName);
	}
#endif VALVE_DLL

	CloseHandle( hProcess );
}
//[/MiB]

#if !TURN_OFF_ALERT
//Thothie JAN2013
void CScript::conflict_check ( msstring testvar, msstring testvar_type, msstring testvar_scope)
{
	bool cc_found = false;
	bool cc_check_against_const = false;
	bool cc_check_against_var = false;
	bool cc_check_against_global = false;
	msstring cc_conflict_rep = testvar_type;

	if ( testvar_type == "setvar" || testvar_type == "setvard" )
	{
		cc_check_against_const = true;
		cc_check_against_global = true;
	}
	else if ( testvar_type == "const" )
	{
		cc_check_against_var = true;
		cc_check_against_global = true;
	}
	else if ( testvar_type == "setvarg" )
	{
		cc_check_against_var = true;
		cc_check_against_const = true;
	}
	else if ( testvar_type == "local" )
	{
		cc_check_against_var = true;
		cc_check_against_const = true;
		cc_check_against_global = true;
	}

	if ( cc_check_against_const )
	{
		 for (int i = 0; i < m_Constants.size(); i++) 
		{
			if ( m_Constants[i].Name == testvar )
			{
				cc_conflict_rep.append("==const");
				cc_found = true;
			}
		}
	}
	if ( cc_check_against_var )
	{
		 for (int i = 0; i < m_Variables.size(); i++) 
		{
			if ( m_Variables[i].Name == testvar )
			{
				cc_conflict_rep.append("==setvar");
				cc_found = true;
			}
		}
	}
	if ( cc_check_against_global )
	{
		 for (int i = 0; i < m_gVariables.size(); i++) 
		{
			if ( m_gVariables[i].Name == testvar )
			{
				cc_conflict_rep.append("==setvarg");
				cc_found = true;
			}

		}
	}

	if ( cc_found )
	{
		msstring out_error = UTIL_VarArgs( "CONFLICT_ERROR! [%s:%s]:(%s) %s %s\n", testvar_scope.c_str(), m.ScriptFile.c_str(), cc_conflict_rep.c_str(), testvar_type.c_str(), testvar.c_str() );
		logfile << out_error.c_str();
		//be nice to be able to return the top script here, but buggers up if I try to pull the ent to do so
		Print("%s",out_error.c_str());
		MSErrorConsoleText( "",out_error.c_str() );
		/*
		if ( m.pScriptedEnt )
		{
			CBaseEntity *pEntity = m.pScriptedEnt;
			IScripted *pScripted = (pEntity) ? pEntity->GetScripted() : NULL;
			if ( pScripted )
			{
				cc_error_text.append( UTIL_VarArgs( "[%s]", pScripted->m_Scripts[0]->m.ScriptFile.c_str() ) );
			}
			else
			{
				cc_error_text.append( UTIL_VarArgs( "[*%s]", m.ScriptFile.c_str() ) );
			}
		}
		else
		{
			cc_error_text.append( UTIL_VarArgs( "[*%s]", m.ScriptFile.c_str() ) );
		}
		logfile << "Conflict_error*: " << cc_error_text.c_str() << " : " << VarName.c_str() << "\n";
		//Print("Conflict_error: %s with %s in %s:\n",cc_error_text.c_str(),VarName.c_str(), m.ScriptFile.c_str() );
		//MSErrorConsoleText( "", UTIL_VarArgs("Conflict_error: %s with %s in %s:\n",cc_error_text.c_str(),VarName.c_str(), m.ScriptFile.c_str()) );
		*/
	}
}
#endif