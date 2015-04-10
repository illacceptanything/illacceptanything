/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//===========================================================================
//
// Name:			ai_cast_script_actions.c
// Function:		Wolfenstein AI Character Scripting
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "../game/g_local.h"
#include "../game/q_shared.h"
#include "../game/botlib.h"      //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../botai/botai.h"          //bot ai interface

#include "ai_cast.h"

/*
Contains the code to handle the various commands available with an event script.

These functions will return true if the action has been performed, and the script
should proceed to the next item on the list.
*/

/*
===============
AICast_NoAttackIfNotHurtSinceLastScriptAction

  Not an actual command, this is just used by the script code
===============
*/
void AICast_NoAttackIfNotHurtSinceLastScriptAction( cast_state_t *cs ) {
	if ( cs->castScriptStatus.scriptNoAttackTime > level.time ) {
		return;
	}

	// if not moving, we should attack
	if ( VectorLength( cs->bs->velocity ) < 10 ) {
		return;
	}

	// if our enemy is in the direction we are moving, don't hold back
	if ( cs->enemyNum >= 0 && cs->castScriptStatus.scriptGotoEnt >= 0 ) {
		vec3_t v;

		VectorSubtract( g_entities[cs->enemyNum].r.currentOrigin, cs->bs->origin, v );
		if ( DotProduct( cs->bs->velocity, v ) > 0 ) {
			return;
		}
	}

	if ( cs->lastPain < cs->castScriptStatus.castScriptStackChangeTime ) {
		cs->castScriptStatus.scriptNoAttackTime = level.time + FRAMETIME;
	}
}

/*
===============
AICast_ScriptAction_GotoMarker

  syntax: gotomarker <targetname> [firetarget [noattack]] [nostop] OR runtomarker <targetname> [firetarget [noattack]] [nostop]
===============
*/
qboolean AICast_ScriptAction_GotoMarker( cast_state_t *cs, char *params ) {
#define SCRIPT_REACHGOAL_DIST   8
	char    *pString, *token;
	gentity_t *ent;
	vec3_t vec, org;
	int i, diff;
	qboolean slowApproach;

	ent = NULL;

	// if we are avoiding danger, then wait for the danger to pass
	if ( cs->castScriptStatus.scriptGotoId < 0 && cs->dangerEntityValidTime > level.time ) {
		return qfalse;
	}
	// if we are in a special func, then wait
	if ( cs->aiFlags & AIFL_SPECIAL_FUNC ) {
		return qfalse;
	}

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI scripting: gotomarker must have an targetname\n" );
	}

	// if we already are going to the marker, just use that, and check if we're in range
	if ( cs->castScriptStatus.scriptGotoEnt >= 0 && cs->castScriptStatus.scriptGotoId == cs->thinkFuncChangeTime ) {
		ent = &g_entities[cs->castScriptStatus.scriptGotoEnt];
		if ( ent->targetname && !Q_strcasecmp( ent->targetname, token ) ) {
			// if we're not slowing down, then check for passing the marker, otherwise check distance only
			VectorSubtract( ent->r.currentOrigin, cs->bs->origin, vec );
			//
			if ( cs->followSlowApproach && VectorLength( vec ) < cs->followDist ) {
				cs->followTime = 0;
				AIFunc_IdleStart( cs );   // resume normal AI
				return qtrue;
			} else if ( !cs->followSlowApproach && VectorLength( vec ) < 64 /*&& DotProduct(cs->bs->cur_ps.velocity, vec) < 0*/ )       {
				cs->followTime = 0;
				AIFunc_IdleStart( cs );   // resume normal AI
				return qtrue;
			} else
			{
				// do we have a firetarget ?
				token = COM_ParseExt( &pString, qfalse );
				if ( !token[0] || !Q_stricmp( token,"nostop" ) ) {
					AICast_NoAttackIfNotHurtSinceLastScriptAction( cs );
				} else {    // yes we do
					// find this targetname
					ent = G_Find( NULL, FOFS( targetname ), token );
					if ( !ent ) {
						ent = AICast_FindEntityForName( token );
						if ( !ent ) {
							G_Error( "AI Scripting: gotomarker cannot find targetname \"%s\"\n", token );
						}
					}
					// set the view angle manually
					BG_EvaluateTrajectory( &ent->s.pos, level.time, org );
					VectorSubtract( org, cs->bs->origin, vec );
					VectorNormalize( vec );
					vectoangles( vec, cs->ideal_viewangles );
					// noattack?
					token = COM_ParseExt( &pString, qfalse );
					if ( !token[0] || Q_stricmp( token,"noattack" ) ) {
						qboolean fire = qtrue;
						// if it's an AI, and they aren't visible, dont shoot
						if ( ent->r.svFlags & SVF_CASTAI ) {
							if ( cs->vislist[ent->s.number].real_visible_timestamp != cs->vislist[ent->s.number].lastcheck_timestamp ) {
								fire = qfalse;
							}
						}
						if ( fire ) {
							for ( i = 0; i < 2; i++ ) {
								diff = abs( AngleDifference( cs->viewangles[i], cs->ideal_viewangles[i] ) );
								if ( diff < 20 ) {
									// dont reload prematurely
									cs->noReloadTime = level.time + 1000;
									// force fire
									trap_EA_Attack( cs->bs->client );
									//
									cs->bFlags |= BFL_ATTACKED;
									// dont reload prematurely
									cs->noReloadTime = level.time + 200;
								}
							}
						}
					}
				}
				cs->followTime = level.time + 500;
				return qfalse;
			}
		} else
		{
			ent = NULL;
		}
	}

	// find the ai_marker with the given "targetname"
	// TTimo: gcc: suggest () around assignment used as truth value
	while ( ( ent = G_Find( ent, FOFS( classname ), "ai_marker" ) ) )
	{
		if ( ent->targetname && !Q_strcasecmp( ent->targetname, token ) ) {
			break;
		}
	}

	if ( !ent ) {
		G_Error( "AI Scripting: can't find ai_marker with \"targetname\" = \"%s\"\n", token );
	}

	if ( Distance( cs->bs->origin, ent->r.currentOrigin ) < SCRIPT_REACHGOAL_DIST ) { // we made it
		return qtrue;
	}

	cs->castScriptStatus.scriptNoMoveTime = 0;
	cs->castScriptStatus.scriptGotoEnt = ent->s.number;
	//
	// slow approach to the goal?
	if ( !params || !strstr( params," nostop" ) ) {
		slowApproach = qtrue;
	} else {
		slowApproach = qfalse;
	}
	//
	AIFunc_ChaseGoalStart( cs, ent->s.number, ( slowApproach ? SCRIPT_REACHGOAL_DIST : 32 ), slowApproach );
	cs->followIsGoto = qtrue;
	cs->followTime = 0x7fffffff;    // make sure it gets through for the first frame
	cs->castScriptStatus.scriptGotoId = cs->thinkFuncChangeTime;

	AICast_NoAttackIfNotHurtSinceLastScriptAction( cs );
	return qfalse;
}

/*
===============
AICast_ScriptAction_WalkToMarker

  syntax: walktomarker <targetname> [firetarget [noattack]] [nostop]
===============
*/
qboolean AICast_ScriptAction_WalkToMarker( cast_state_t *cs, char *params ) {
	// if we are avoiding danger, then wait for the danger to pass
	if ( cs->castScriptStatus.scriptGotoId < 0 && cs->dangerEntityValidTime > level.time ) {
		return qfalse;
	}
	// if we are in a special func, then wait
	if ( cs->aiFlags & AIFL_SPECIAL_FUNC ) {
		return qfalse;
	}
	if ( !AICast_ScriptAction_GotoMarker( cs, params ) || ( !strstr( params, " nostop" ) && VectorLength( cs->bs->cur_ps.velocity ) ) ) {
		cs->movestate = MS_WALK;
		cs->movestateType = MSTYPE_TEMPORARY;
		AICast_NoAttackIfNotHurtSinceLastScriptAction( cs );
		return qfalse;
	}

	return qtrue;
}


/*
===============
AICast_ScriptAction_CrouchToMarker

  syntax: crouchtomarker <targetname> [firetarget [noattack]] [nostop]
===============
*/
qboolean AICast_ScriptAction_CrouchToMarker( cast_state_t *cs, char *params ) {
	// if we are avoiding danger, then wait for the danger to pass
	if ( cs->castScriptStatus.scriptGotoId < 0 && cs->dangerEntityValidTime > level.time ) {
		return qfalse;
	}
	// if we are in a special func, then wait
	if ( cs->aiFlags & AIFL_SPECIAL_FUNC ) {
		return qfalse;
	}
	if ( !AICast_ScriptAction_GotoMarker( cs, params ) || ( !strstr( params, " nostop" ) && VectorLength( cs->bs->cur_ps.velocity ) ) ) {
		cs->movestate = MS_CROUCH;
		cs->movestateType = MSTYPE_TEMPORARY;
		AICast_NoAttackIfNotHurtSinceLastScriptAction( cs );
		return qfalse;
	}

	return qtrue;
}

/*
===============
AICast_ScriptAction_GotoCast

  syntax: gotocast <ainame> [firetarget [noattack]] OR runtocast <ainame> [firetarget [noattack]]
===============
*/
qboolean AICast_ScriptAction_GotoCast( cast_state_t *cs, char *params ) {
#define SCRIPT_REACHCAST_DIST   64
	char    *pString, *token;
	gentity_t *ent;
	vec3_t vec, org;
	int i, diff;

	ent = NULL;

	// if we are avoiding danger, then wait for the danger to pass
	if ( cs->castScriptStatus.scriptGotoId < 0 && cs->dangerEntityValidTime > level.time ) {
		return qfalse;
	}
	// if we are in a special func, then wait
	if ( cs->aiFlags & AIFL_SPECIAL_FUNC ) {
		return qfalse;
	}

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI scripting: gotocast must have an ainame\n" );
	}

	// if we already are going to the marker, just use that, and check if we're in range
	if ( cs->castScriptStatus.scriptGotoEnt >= 0 && cs->castScriptStatus.scriptGotoId == cs->thinkFuncChangeTime ) {
		ent = &g_entities[cs->castScriptStatus.scriptGotoEnt];
		if ( ent->targetname && !Q_strcasecmp( ent->targetname, token ) ) {
			if ( Distance( cs->bs->origin, ent->r.currentOrigin ) < cs->followDist ) {
				cs->followTime = 0;
				AIFunc_IdleStart( cs );   // resume normal AI
				return qtrue;
			} else
			{
				// do we have a firetarget ?
				token = COM_ParseExt( &pString, qfalse );
				if ( !token[0] ) {
					AICast_NoAttackIfNotHurtSinceLastScriptAction( cs );
				} else {    // yes we do
					// find this targetname
					ent = G_Find( NULL, FOFS( targetname ), token );
					if ( !ent ) {
						ent = AICast_FindEntityForName( token );
						if ( !ent ) {
							G_Error( "AI Scripting: gotomarker cannot find targetname \"%s\"\n", token );
						}
					}

					// set the view angle manually
					BG_EvaluateTrajectory( &ent->s.pos, level.time, org );
					VectorSubtract( org, cs->bs->origin, vec );
					VectorNormalize( vec );
					vectoangles( vec, cs->ideal_viewangles );
					// noattack?
					token = COM_ParseExt( &pString, qfalse );
					if ( !token[0] || Q_stricmp( token,"noattack" ) ) {
						qboolean fire = qtrue;
						// if it's an AI, and they aren't visible, dont shoot
						if ( ent->r.svFlags & SVF_CASTAI ) {
							if ( cs->vislist[ent->s.number].real_visible_timestamp != cs->vislist[ent->s.number].lastcheck_timestamp ) {
								fire = qfalse;
							}
						}
						if ( fire ) {
							for ( i = 0; i < 2; i++ ) {
								diff = abs( AngleDifference( cs->viewangles[i], cs->ideal_viewangles[i] ) );
								if ( diff < 20 ) {
									// dont reload prematurely
									cs->noReloadTime = level.time + 1000;
									// force fire
									trap_EA_Attack( cs->bs->client );
									//
									cs->bFlags |= BFL_ATTACKED;
									// dont reload prematurely
									cs->noReloadTime = level.time + 200;
								}
							}
						}
					}
				}
				cs->followTime = level.time + 500;
				return qfalse;
			}
		} else
		{
			ent = NULL;
		}
	}

	// find the cast/player with the given "name"
	ent = AICast_FindEntityForName( token );
	if ( !ent ) {
		G_Error( "AI Scripting: can't find AI cast with \"ainame\" = \"%s\"\n", token );
	}

	if ( Distance( cs->bs->origin, ent->r.currentOrigin ) < SCRIPT_REACHCAST_DIST ) { // we made it
		return qtrue;
	}

	if ( !ent ) {
		G_Error( "AI Scripting: can't find ai_marker with \"targetname\" = \"%s\"\n", token );
	}

	cs->castScriptStatus.scriptNoMoveTime = 0;
	cs->castScriptStatus.scriptGotoEnt = ent->s.number;
	//
	AIFunc_ChaseGoalStart( cs, ent->s.number, SCRIPT_REACHCAST_DIST, qtrue );
	cs->followTime = 0x7fffffff;
	AICast_NoAttackIfNotHurtSinceLastScriptAction( cs );
	cs->castScriptStatus.scriptGotoId = cs->thinkFuncChangeTime;

	return qfalse;
}

/*
===============
AICast_ScriptAction_WalkToCast

  syntax: walktocast <ainame> [firetarget [noattack]]
===============
*/
qboolean AICast_ScriptAction_WalkToCast( cast_state_t *cs, char *params ) {
	// if we are avoiding danger, then wait for the danger to pass
	if ( cs->castScriptStatus.scriptGotoId < 0 && cs->dangerEntityValidTime > level.time ) {
		return qfalse;
	}
	// if we are in a special func, then wait
	if ( cs->aiFlags & AIFL_SPECIAL_FUNC ) {
		return qfalse;
	}
	if ( !AICast_ScriptAction_GotoCast( cs, params ) ) {
		cs->movestate = MS_WALK;
		cs->movestateType = MSTYPE_TEMPORARY;
		AICast_NoAttackIfNotHurtSinceLastScriptAction( cs );
		return qfalse;
	}

	return qtrue;
}


/*
===============
AICast_ScriptAction_CrouchToCast

  syntax: crouchtocast <ainame> [firetarget [noattack]]
===============
*/
qboolean AICast_ScriptAction_CrouchToCast( cast_state_t *cs, char *params ) {
	// if we are avoiding danger, then wait for the danger to pass
	if ( cs->castScriptStatus.scriptGotoId < 0 && cs->dangerEntityValidTime > level.time ) {
		return qfalse;
	}
	// if we are in a special func, then wait
	if ( cs->aiFlags & AIFL_SPECIAL_FUNC ) {
		return qfalse;
	}
	if ( !AICast_ScriptAction_GotoCast( cs, params ) ) {
		cs->movestate = MS_CROUCH;
		cs->movestateType = MSTYPE_TEMPORARY;
		AICast_NoAttackIfNotHurtSinceLastScriptAction( cs );
		return qfalse;
	}

	return qtrue;
}


/*
==============
AICast_ScriptAction_AbortIfLoadgame
==============
*/
qboolean AICast_ScriptAction_AbortIfLoadgame( cast_state_t *cs, char *params ) {
	char loading[4];

	trap_Cvar_VariableStringBuffer( "savegame_loading", loading, sizeof( loading ) );

	if ( strlen( loading ) > 0 && atoi( loading ) != 0 ) {
		// abort the current script
		cs->castScriptStatus.castScriptStackHead = cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].stack.numItems;
	}

	return qtrue;
}


/*
=================
AICast_ScriptAction_Wait

  syntax: wait <duration/FOREVER> [moverange] [facetarget]

  moverange defaults to 200, allows some monouverability to avoid fire or attack
=================
*/
qboolean AICast_ScriptAction_Wait( cast_state_t *cs, char *params ) {
	char    *pString, *token, *facetarget;
	int duration;
	float moverange;
	float dist;
	gentity_t *ent;
	vec3_t org, vec;

	// if we are in a special func, then wait
	if ( cs->aiFlags & AIFL_SPECIAL_FUNC ) {
		return qfalse;
	}
	// EXPERIMENTAL: if they are on fire, or avoiding danger, let them loose until it passes (or they die)
	if ( cs->dangerEntityValidTime > level.time ) {
		cs->castScriptStatus.scriptNoMoveTime = -1;
		return qfalse;
	}

	if ( ( cs->castScriptStatus.scriptFlags & SFL_FIRST_CALL ) && cs->bs ) {
		// first call, init the waitPos
		VectorCopy( cs->bs->origin, cs->castScriptStatus.scriptWaitPos );
	}

	// get the duration
	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI scripting: wait must have a duration\n" );
	}
	if ( !Q_stricmp( token, "forever" ) ) {
		duration = level.time + 10000;
	} else {
		duration = atoi( token );
	}

	// if this is for the player, don't worry about enforcing the moverange
	if ( !cs->bs ) {
		return ( cs->castScriptStatus.castScriptStackChangeTime + duration < level.time );
	}

	token = COM_ParseExt( &pString, qfalse );
	// if this token is a number, then assume it is the moverange, otherwise we have a default moverange with a facetarget
	moverange = -999;
	facetarget = NULL;
	if ( token[0] ) {
		if ( toupper( token[0] ) >= 'A' && toupper( token[0] ) <= 'Z' ) {
			facetarget = token;
		} else {    // we found a moverange
			moverange = atof( token );
			token = COM_ParseExt( &pString, qfalse );
			if ( token[0] ) {
				facetarget = token;
			}
		}
	}

	// default to no moverange
	//if (moverange == -999) {
	//	moverange = 200;
	//}

	if ( moverange != 0 ) {       // default to 200 if no range given
		if ( moverange > 0 ) {
			dist = Distance( cs->bs->origin, cs->castScriptStatus.scriptWaitPos );
			// if we are able to move, and have an enemy
			if (    ( cs->castScriptStatus.scriptWaitMovetime < level.time )
					&&  ( cs->enemyNum >= 0 ) ) {

				// if we can attack them, or they can't attack us, stay here
				// TTimo: gcc: warning: suggest parentheses around && within ||
				if (    AICast_CheckAttack( cs, cs->enemyNum, qfalse )
						||  (   !AICast_EntityVisible( AICast_GetCastState( cs->enemyNum ), cs->entityNum, qfalse )
								&&  !AICast_CheckAttack( AICast_GetCastState( cs->enemyNum ), cs->entityNum, qfalse ) ) ) {
					cs->castScriptStatus.scriptNoMoveTime = level.time + 200;
				}

			}
			// if outside range, move towards waitPos
			if ( ( !cs->bs || !cs->bs->cur_ps.legsTimer ) && ( ( ( cs->castScriptStatus.scriptWaitMovetime > level.time ) && ( dist > 32 ) ) || ( dist > moverange ) ) ) {
				cs->castScriptStatus.scriptNoMoveTime = 0;
				AICast_MoveToPos( cs, cs->castScriptStatus.scriptWaitPos, 0 );
				if ( dist > 64 ) {
					cs->castScriptStatus.scriptWaitMovetime = level.time + 600;
				}
			} else
			// if we are reloading, look for somewhere to hide
			if ( cs->castScriptStatus.scriptWaitHideTime > level.time || cs->bs->cur_ps.weaponTime > 500 ) {
				if ( cs->castScriptStatus.scriptWaitHideTime < level.time ) {
					// look for a hide pos within the wait range

				}
				cs->castScriptStatus.scriptWaitHideTime = level.time + 500;
			}
		}
	} else {
		cs->castScriptStatus.scriptNoMoveTime = cs->castScriptStatus.castScriptStackChangeTime + duration;
	}

	// do we have a facetarget ?
	if ( facetarget ) {   // yes we do
		// find this targetname
		ent = G_Find( NULL, FOFS( targetname ), facetarget );
		if ( !ent ) {
			ent = AICast_FindEntityForName( facetarget );
			if ( !ent ) {
				G_Error( "AI Scripting: wait cannot find targetname \"%s\"\n", token );
			}
		}
		// set the view angle manually
		BG_EvaluateTrajectory( &ent->s.pos, level.time, org );
		VectorSubtract( org, cs->bs->origin, vec );
		VectorNormalize( vec );
		vectoangles( vec, cs->ideal_viewangles );
	}

	return ( cs->castScriptStatus.castScriptStackChangeTime + duration < level.time );
}

/*
=================
AICast_ScriptAction_Trigger

  syntax: trigger <ainame> <trigger>

  Calls the specified trigger for the given ai character
=================
*/
qboolean AICast_ScriptAction_Trigger( cast_state_t *cs, char *params ) {
	gentity_t *ent;
	char *pString, *token;
	int oldId;

	// get the cast name
	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI scripting: trigger must have a name and an identifier\n" );
	}

	ent = AICast_FindEntityForName( token );
	if ( !ent ) {
		ent = G_Find( &g_entities[MAX_CLIENTS], FOFS( scriptName ), token );
		if ( !ent ) {
			if ( trap_Cvar_VariableIntegerValue( "developer" ) ) {
				G_Printf( "AI Scripting: can't find AI cast with \"ainame\" = \"%s\"\n", params );
			}
			return qtrue;
		}
	}

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI scripting: trigger must have a name and an identifier\n" );
	}

	oldId = cs->castScriptStatus.scriptId;
	if ( ent->client ) {
		AICast_ScriptEvent( AICast_GetCastState( ent->s.number ), "trigger", token );
	} else {
		G_Script_ScriptEvent( ent, "trigger", token );
	}

	// if the script changed, return false so we don't muck with it's variables
	return ( oldId == cs->castScriptStatus.scriptId );
}

/*
===================
AICast_ScriptAction_FollowCast

  syntax: followcast <ainame>
===================
*/
qboolean AICast_ScriptAction_FollowCast( cast_state_t *cs, char *params ) {
	gentity_t *ent;

	// find the cast/player with the given "name"
	ent = AICast_FindEntityForName( params );
	if ( !ent ) {
		G_Error( "AI Scripting: can't find AI cast with \"ainame\" = \"%s\"\n", params );
	}

	AIFunc_ChaseGoalStart( cs, ent->s.number, 64, qtrue );

	return qtrue;
};

/*
================
AICast_ScriptAction_PlaySound

  syntax: playsound <soundname OR scriptname>

  Currently only allows playing on the VOICE channel, unless you use a sound script (yay)
================
*/
qboolean AICast_ScriptAction_PlaySound( cast_state_t *cs, char *params ) {
	if ( !params ) {
		G_Error( "AI Scripting: syntax error\n\nplaysound <soundname OR scriptname>\n" );
	}

	G_AddEvent( &g_entities[cs->bs->entitynum], EV_GENERAL_SOUND, G_SoundIndex( params ) );

	// assume we are talking
	cs->aiFlags |= AIFL_TALKING;

	// randomly choose idle animation
	if ( cs->aiFlags & AIFL_STAND_IDLE2 ) {
		if ( cs->lastEnemy < 0 && cs->aiFlags & AIFL_TALKING ) {
			g_entities[cs->entityNum].client->ps.eFlags |= EF_STAND_IDLE2;
		} else {
			g_entities[cs->entityNum].client->ps.eFlags &= ~EF_STAND_IDLE2;
		}
	}

	return qtrue;
}

/*
=================
AICast_ScriptAction_NoAttack

  syntax: noattack <duration>
=================
*/
qboolean AICast_ScriptAction_NoAttack( cast_state_t *cs, char *params ) {
	if ( !params ) {
		G_Error( "AI Scripting: syntax error\n\nnoattack <duration>\n" );
	}

	cs->castScriptStatus.scriptNoAttackTime = level.time + atoi( params );

	return qtrue;
}

/*
=================
AICast_ScriptAction_Attack

  syntax: attack [ainame]

  Resumes attacking after a noattack was issued

  if ainame is given, we will attack only that entity as long as they are alive
=================
*/
qboolean AICast_ScriptAction_Attack( cast_state_t *cs, char *params ) {
	gentity_t *ent;

	cs->castScriptStatus.scriptNoAttackTime = 0;

	// if we have specified an aiName, then we should attack only this person
	if ( params ) {
		ent = AICast_FindEntityForName( params );
		if ( !ent ) {
			G_Error( "AI Scripting: \"attack\" command unable to find aiName \"%s\"", params );
		}
		cs->castScriptStatus.scriptAttackEnt = ent->s.number;
		cs->enemyNum = ent->s.number;
	} else {
		cs->castScriptStatus.scriptAttackEnt = -1;
	}

	return qtrue;
}

/*
=================
AICast_ScriptAction_PlayAnim

  syntax: playanim <animation> [legs/torso/both] [HOLDFRAME] [numLoops/FOREVER] [target]

  NOTE: any new animations that are needed by the scripting system, will need to be added here
=================
*/
qboolean AICast_ScriptAction_PlayAnim( cast_state_t *cs, char *params ) {
	char *pString, *token, tokens[3][MAX_QPATH];
	int i, endtime, duration, numLoops;
	gclient_t *client;
	gentity_t *ent;
	vec3_t org, vec;
	qboolean forever = qfalse, setAngles = qfalse;
	qboolean holdframe = qfalse;

	pString = params;

	client = &level.clients[cs->entityNum];

	if ( level.animScriptData.modelInfo[level.animScriptData.clientModels[cs->entityNum] - 1]->version > 1 ) {    // new (scripted) model

		// read the name
		token = COM_ParseExt( &pString, qfalse );
		if ( !token || !token[0] ) {
			G_Error( "AI Scripting: syntax error\n\nplayanim <animation> <legs/torso/both>\n" );
		}
		Q_strncpyz( tokens[0], token, sizeof( tokens[0] ) );
		Q_strlwr( tokens[0] );

		// read the body part
		token = COM_ParseExt( &pString, qfalse );
		if ( !token || !token[0] ) {
			G_Error( "AI Scripting: syntax error\n\nplayanim <animation> <legs/torso/both>\n" );
		}
		Q_strncpyz( tokens[1], token, sizeof( tokens[1] ) );
		Q_strlwr( tokens[1] );

		// read the HOLDFRAME (optional)
		token = COM_ParseExt( &pString, qfalse );
		if ( token && token[0] && !Q_strcasecmp( token, "holdframe" ) ) {
			holdframe = qtrue;
			// read the numLoops (optional)
			token = COM_ParseExt( &pString, qfalse );
		}
		// token is parsed above
		if ( token && token[0] ) {
			if ( !Q_strcasecmp( token, "forever" ) ) {
				forever = qtrue;
				numLoops = -1;
				// read the target (optional)
				token = COM_ParseExt( &pString, qfalse );
			} else {
				numLoops = atoi( token );
				if ( !numLoops ) {    // must be the target, so set loops to 1
					numLoops = 1;
				} else {
					// read the target (optional)
					token = COM_ParseExt( &pString, qfalse );
				}
			}

			if ( token && token[0] ) {
				// find this targetname
				ent = G_Find( NULL, FOFS( targetname ), token );
				if ( !ent ) {
					ent = AICast_FindEntityForName( token );
					if ( !ent ) {
						G_Error( "AI Scripting: playanim cannot find targetname \"%s\"\n", token );
					}
				}
				// set the view angle manually
				BG_EvaluateTrajectory( &ent->s.pos, level.time, org );
				VectorSubtract( org, cs->bs->origin, vec );
				VectorNormalize( vec );
				vectoangles( vec, cs->ideal_viewangles );
				VectorCopy( cs->ideal_viewangles, cs->castScriptStatus.playanim_viewangles );
				setAngles = qtrue;
			}

		} else {
			numLoops = 1;
		}

		if ( cs->castScriptStatus.scriptFlags & SFL_FIRST_CALL ) {
			// first time in here, play the anim
			duration = BG_PlayAnimName( &( client->ps ), tokens[0], BG_IndexForString( tokens[1], animBodyPartsStr, qfalse ), qtrue, qfalse, qtrue );
			if ( numLoops == -1 ) {
				cs->scriptAnimTime = 0x7fffffff;    // maximum time allowed
			} else {
				cs->scriptAnimTime = level.time + ( numLoops * duration );
			}
			if ( !strcmp( tokens[1], "torso" ) ) {
				cs->scriptAnimNum = client->ps.torsoAnim & ~ANIM_TOGGLEBIT;
				// adjust the duration according to numLoops
				if ( numLoops > 1 ) {
					client->ps.torsoTimer += duration * numLoops;
				} else if ( numLoops == -1 ) {
					client->ps.torsoTimer = 9999;
				}
			} else {    // dont move
				cs->scriptAnimNum = client->ps.legsAnim & ~ANIM_TOGGLEBIT;
				cs->castScriptStatus.scriptNoMoveTime = cs->scriptAnimTime;
				// lock the viewangles
				if ( !cs->castScriptStatus.playAnimViewlockTime || cs->castScriptStatus.playAnimViewlockTime < level.time ) {
					VectorCopy( cs->ideal_viewangles, cs->castScriptStatus.playanim_viewangles );
				}
				cs->castScriptStatus.playAnimViewlockTime = cs->scriptAnimTime;
				// adjust the duration according to numLoops
				if ( numLoops > 1 ) {
					client->ps.legsTimer += duration * ( numLoops - 1 );
					if ( !strcmp( tokens[1], "both" ) ) {
						client->ps.torsoTimer += duration * ( numLoops - 1 );
					}
				} else if ( numLoops == -1 ) {
					client->ps.legsTimer = 9999;
					if ( !strcmp( tokens[1], "both" ) ) {
						client->ps.torsoTimer = 9999;
					}
				}
			}
		} else {
			if ( holdframe ) {
				// make sure it doesn't stop before the next command can be processed
				if ( !strcmp( tokens[1], "torso" ) ) {
					if ( client->ps.torsoTimer < 400 ) {
						client->ps.torsoTimer = 400;
					}
				} else if ( !strcmp( tokens[1], "legs" ) ) {
					if ( client->ps.legsTimer < 400 ) {
						client->ps.legsTimer = 400;
					}
				} else if ( !strcmp( tokens[1], "both" ) ) {
					if ( client->ps.torsoTimer < 400 ) {
						client->ps.torsoTimer = 400;
					}
					if ( client->ps.legsTimer < 400 ) {
						client->ps.legsTimer = 400;
					}
				}
			}
			// keep it looping if forever
			if ( forever ) {
				if ( !strcmp( tokens[1], "torso" ) ) {
					client->ps.torsoTimer = 9999;
				} else {
					client->ps.legsTimer = 9999;
					if ( !strcmp( tokens[1], "both" ) ) {
						client->ps.torsoTimer = 9999;
					}
				}
				return qfalse;
			}
			// wait for the anim to stop playing
			if ( cs->scriptAnimTime <= level.time ) {
				return qtrue;
			}
		}

		return qfalse;

	} else {    // old model

		for ( i = 0; i < 3; i++ ) {
			token = COM_ParseExt( &pString, qfalse );
			if ( !token || !token[0] ) {
				//G_Error("AI Scripting: syntax error\n\nplayanim <animation> <pausetime> [legs/torso/both]\n");
				G_Printf( "AI Scripting: syntax error\n\nplayanim <animation> <pausetime> <legs/torso/both>\n" );
				return qtrue;
			} else {
				Q_strncpyz( tokens[i], token, sizeof( tokens[i] ) );
			}
		}

		Q_strlwr( tokens[2] );

		endtime = cs->castScriptStatus.castScriptStackChangeTime + atoi( tokens[1] );
		duration = endtime - level.time + 200;  // so animations don't run out before starting a next animation
		if ( duration > 2000 ) {
			duration = 2000;
		}

		cs->scriptAnimTime = level.time;

		if ( duration < 200 ) {
			return qtrue;   // done playing animation

		}
		// call the anims directly based on the animation token

		//if (cs->castScriptStatus.castScriptStackChangeTime == level.time) {
		for ( i = 0; i < MAX_ANIMATIONS; i++ ) {
			if ( !Q_strcasecmp( tokens[0], animStrings[i] ) ) {
				if ( !Q_strcasecmp( tokens[2],"torso" ) ) {
					if ( ( client->ps.torsoAnim & ~ANIM_TOGGLEBIT ) != i ) {
						client->ps.torsoAnim = ( ( client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | i;
					}
					client->ps.torsoTimer = duration;
				} else if ( !Q_strcasecmp( tokens[2],"legs" ) ) {
					if ( ( client->ps.legsAnim & ~ANIM_TOGGLEBIT ) != i ) {
						client->ps.legsAnim = ( ( client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | i;
					}
					client->ps.legsTimer = duration;
				} else if ( !Q_strcasecmp( tokens[2],"both" ) ) {
					if ( ( client->ps.torsoAnim & ~ANIM_TOGGLEBIT ) != i ) {
						client->ps.torsoAnim = ( ( client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | i;
					}
					client->ps.torsoTimer = duration;
					if ( ( client->ps.legsAnim & ~ANIM_TOGGLEBIT ) != i ) {
						client->ps.legsAnim = ( ( client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | i;
					}
					client->ps.legsTimer = duration;
				} else {
					G_Printf( "AI Scripting: syntax error\n\nplayanim <animation> <pausetime> <legs/torso/both>\n" );
				}
				break;
			}
		}
		if ( i == MAX_ANIMATIONS ) {
			G_Printf( "AI Scripting: playanim has unknown or invalid animation \"%s\"\n", tokens[0] );
		}
		//}

		if ( !strcmp( tokens[2], "torso" ) ) {
			cs->scriptAnimNum = client->ps.torsoAnim & ~ANIM_TOGGLEBIT;
		} else {
			cs->scriptAnimNum = client->ps.legsAnim & ~ANIM_TOGGLEBIT;
		}

		if ( cs->castScriptStatus.scriptNoMoveTime < level.time + 300 ) {
			cs->castScriptStatus.scriptNoMoveTime = level.time + 300;
		}
		if ( cs->castScriptStatus.scriptNoAttackTime < level.time + 300 ) {
			cs->castScriptStatus.scriptNoAttackTime = level.time + 300;
		}
		return qfalse;
	}
};

/*
=================
AICast_ScriptAction_ClearAnim

  stops any animation that is currently playing
=================
*/
qboolean AICast_ScriptAction_ClearAnim( cast_state_t *cs, char *params ) {
	gclient_t *client;

	client = &level.clients[cs->entityNum];

	client->ps.torsoTimer = 0;
	client->ps.legsTimer = 0;

	// let us move again
	cs->castScriptStatus.scriptNoMoveTime = 0;

	return qtrue;
}

/*
=================
AICast_ScriptAction_SetAmmo

  syntax: setammo <pickupname> <count>
=================
*/
qboolean AICast_ScriptAction_SetAmmo( cast_state_t *cs, char *params ) {
	char *pString, *token;
	int weapon;
	int i;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: setammo without ammo identifier\n" );
	}

	weapon = WP_NONE;

	for ( i = 1; bg_itemlist[i].classname; i++ )
	{
		//----(SA)	first try the name they see in the editor, then the pickup name
		if ( !Q_strcasecmp( token, bg_itemlist[i].classname ) ) {
			weapon = bg_itemlist[i].giTag;
			break;
		}

		if ( !Q_strcasecmp( token, bg_itemlist[i].pickup_name ) ) {
			weapon = bg_itemlist[i].giTag;
			break;
		}
	}

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: setammo without ammo count\n" );
	}

	if ( weapon != WP_NONE ) {
		// give them the ammo

		if ( atoi( token ) ) {
			int amt;
			amt = atoi( token );
			if ( amt > 50 + ammoTable[BG_FindAmmoForWeapon( weapon )].maxammo ) {
				amt = 999;  // unlimited
			}
			Add_Ammo( &g_entities[cs->entityNum], weapon, amt, qtrue );
		} else {
			// remove ammo for this weapon
			g_entities[cs->entityNum].client->ps.ammo[BG_FindAmmoForWeapon( weapon )] = 0;
			g_entities[cs->entityNum].client->ps.ammoclip[BG_FindClipForWeapon( weapon )] = 0;
		}

	} else {

		if ( g_cheats.integer ) {
			G_Printf( "--SCRIPTER WARNING-- AI Scripting: setammo: unknown ammo \"%s\"", params );
		}
		return qfalse;  // (SA) temp as scripts transition to new names

	}

	return qtrue;
};

/*
=================
AICast_ScriptAction_SetClip

  syntax: setclip <weapon name> <count>

=================
*/
qboolean AICast_ScriptAction_SetClip( cast_state_t *cs, char *params ) {
	char *pString, *token;
	int weapon;
	int i;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: setclip without weapon identifier\n" );
	}

	weapon = WP_NONE;

	for ( i = 1; bg_itemlist[i].classname; i++ )
	{
		//----(SA)	first try the name they see in the editor, then the pickup name
		if ( !Q_strcasecmp( token, bg_itemlist[i].classname ) ) {
			weapon = bg_itemlist[i].giTag;
			break;
		}

		if ( !Q_strcasecmp( token, bg_itemlist[i].pickup_name ) ) {
			weapon = bg_itemlist[i].giTag;
			break;
		}
	}

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: setclip without ammo count\n" );
	}

	if ( weapon != WP_NONE ) {

		int spillover = atoi( token ) - ammoTable[weapon].maxclip;

		if ( spillover > 0 ) {
			// there was excess, put it in storage and fill the clip
			g_entities[cs->entityNum].client->ps.ammo[BG_FindAmmoForWeapon( weapon )] += spillover;
			g_entities[cs->entityNum].client->ps.ammoclip[BG_FindClipForWeapon( weapon )] = ammoTable[weapon].maxclip;
		} else {
			// set the clip amount to the exact specified value
			g_entities[cs->entityNum].client->ps.ammoclip[weapon] = atoi( token );
		}

	} else {
//		G_Printf( "--SCRIPTER WARNING-- AI Scripting: setclip: unknown weapon \"%s\"", params );
		return qfalse;  // (SA) temp as scripts transition to new names
//		G_Error( "AI Scripting: setclip: unknown weapon \"%s\"", params );
	}

	return qtrue;
};



/*
==============
AICast_ScriptAction_SuggestWeapon
==============
*/
qboolean AICast_ScriptAction_SuggestWeapon( cast_state_t *cs, char *params ) {
	int weapon;
	int i;
	//int		suggestedweaps = 0; // TTimo: unused

	weapon = WP_NONE;

	for ( i = 1; bg_itemlist[i].classname; i++ )
	{
		//----(SA)	first try the name they see in the editor, then the pickup name
		if ( !Q_strcasecmp( params, bg_itemlist[i].classname ) ) {
			weapon = bg_itemlist[i].giTag;
			break;
		}

		if ( !Q_strcasecmp( params, bg_itemlist[i].pickup_name ) ) {
			weapon = bg_itemlist[i].giTag;
			break;
		}
	}

	if ( weapon != WP_NONE ) {
		G_AddEvent( &g_entities[cs->entityNum], EV_SUGGESTWEAP, weapon );
	} else {
		G_Error( "AI Scripting: suggestweapon: unknown weapon \"%s\"", params );
	}

	return qtrue;

}


/*
=================
AICast_ScriptAction_SelectWeapon

  syntax: selectweapon <pickupname>
=================
*/
qboolean AICast_ScriptAction_SelectWeapon( cast_state_t *cs, char *params ) {
	int weapon;
	int i;

	weapon = WP_NONE;

	for ( i = 1; bg_itemlist[i].classname; i++ )
	{
		//----(SA)	first try the name they see in the editor, then the pickup name
		if ( !Q_strcasecmp( params, bg_itemlist[i].classname ) ) {
			weapon = bg_itemlist[i].giTag;
			break;
		}

		if ( !Q_strcasecmp( params, bg_itemlist[i].pickup_name ) ) {
			weapon = bg_itemlist[i].giTag;
			break;
		}
	}

	if ( weapon != WP_NONE ) {
		if ( cs->bs ) {
			cs->weaponNum = weapon;
		}
		cs->castScriptStatus.scriptFlags |= SFL_NOCHANGEWEAPON;

		g_entities[cs->entityNum].client->ps.weapon = weapon;
		g_entities[cs->entityNum].client->ps.weaponstate = WEAPON_READY;

		if ( !cs->aiCharacter ) {  // only do this for player
			//vmCvar_t cvar;
			//
			g_entities[cs->entityNum].client->ps.weaponTime = 750;  // (SA) HACK: FIXME: TODO: delay to catch initial weapon reload
			// tell it which weapon to use after spawning in
			//trap_Cvar_Register( &cvar, "cg_loadWeaponSelect", "0", CVAR_ROM );
			//trap_Cvar_Set( "cg_loadWeaponSelect", va("%i", g_entities[cs->entityNum].client->ps.weapon ) );
		}

	} else {
//		G_Printf( "--SCRIPTER WARNING-- AI Scripting: selectweapon: unknown weapon \"%s\"", params );
//		return qfalse;	// (SA) temp as scripts transition to new names
		G_Error( "AI Scripting: selectweapon: unknown weapon \"%s\"", params );
	}

	return qtrue;
};



//----(SA)	added
/*
==============
AICast_ScriptAction_GiveArmor
	syntax: setarmor <amount>

==============
*/
qboolean AICast_ScriptAction_SetArmor( cast_state_t *cs, char *params ) {
	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: setarmor requires an armor value" );
	}

	g_entities[cs->entityNum].client->ps.stats[STAT_ARMOR] += atoi( params );

	return qtrue;
}

/*
==============
AICast_ScriptAction_GiveArmor

	syntax: givearmor <pickupname>

	will probably be more like:
		syntax: givearmor <type> <amount>
==============
*/
qboolean AICast_ScriptAction_GiveArmor( cast_state_t *cs, char *params ) {
	int i;
	gitem_t     *item = 0;

	for ( i = 1; bg_itemlist[i].classname; i++ ) {
		//----(SA)	first try the name they see in the editor, then the pickup name
		if ( !Q_strcasecmp( params, bg_itemlist[i].classname ) ) {
			item = &bg_itemlist[i];
		}

		if ( !Q_strcasecmp( params, bg_itemlist[i].pickup_name ) ) {
			item = &bg_itemlist[i];
		}
	}

	if ( !item ) { // item not found
		G_Error( "AI Scripting: givearmor%s, unknown item", params );
	}

	if ( item->giType == IT_ARMOR ) {
		g_entities[cs->entityNum].client->ps.stats[STAT_ARMOR] += item->quantity;
		if ( g_entities[cs->entityNum].client->ps.stats[STAT_ARMOR] > 100 ) {
			g_entities[cs->entityNum].client->ps.stats[STAT_ARMOR] = 100;
		}
	}

	return qtrue;
}
//----(SA)	end



/*
=================
AICast_ScriptAction_GiveWeapon

  syntax: giveweapon <pickupname>
=================
*/
qboolean AICast_ScriptAction_GiveWeapon( cast_state_t *cs, char *params ) {
	int weapon;
	int i;
	gentity_t   *ent = &g_entities[cs->entityNum];

	weapon = WP_NONE;

	for ( i = 1; bg_itemlist[i].classname; i++ )
	{
		//----(SA)	first try the name they see in the editor, then the pickup name
		if ( !Q_strcasecmp( params, bg_itemlist[i].classname ) ) {
			weapon = bg_itemlist[i].giTag;
			break;
		}

		if ( !Q_strcasecmp( params, bg_itemlist[i].pickup_name ) ) {
			weapon = bg_itemlist[i].giTag;
		}
	}

	if ( weapon == WP_COLT ) {
		// if you had the colt already, now you've got two!
		if ( COM_BitCheck( g_entities[cs->entityNum].client->ps.weapons, WP_COLT ) ) {
			weapon = WP_AKIMBO;
		}
	}

	if ( weapon != WP_NONE ) {
		COM_BitSet( g_entities[cs->entityNum].client->ps.weapons, weapon );

//----(SA)	some weapons always go together (and they share a clip, so this is okay)
		if ( weapon == WP_GARAND ) {
			COM_BitSet( g_entities[cs->entityNum].client->ps.weapons, WP_SNOOPERSCOPE );
		}
		if ( weapon == WP_SNOOPERSCOPE ) {
			COM_BitSet( g_entities[cs->entityNum].client->ps.weapons, WP_GARAND );
		}
		if ( weapon == WP_FG42 ) {
			COM_BitSet( g_entities[cs->entityNum].client->ps.weapons, WP_FG42SCOPE );
		}
		if ( weapon == WP_SNIPERRIFLE ) {
			COM_BitSet( g_entities[cs->entityNum].client->ps.weapons, WP_MAUSER );
		}
//----(SA)	end

		// monsters have full ammo for their attacks
		// knife gets infinite ammo too
		if ( !Q_strncasecmp( params, "monsterattack", 13 ) || weapon == WP_KNIFE ) {
			g_entities[cs->entityNum].client->ps.ammo[BG_FindAmmoForWeapon( weapon )] = 999;
			Fill_Clip( &g_entities[cs->entityNum].client->ps, weapon );      //----(SA)	added
		}
		// conditional flags
		if ( ent->aiCharacter == AICHAR_ZOMBIE ) {
			if ( COM_BitCheck( ent->client->ps.weapons, WP_MONSTER_ATTACK1 ) ) {
				cs->aiFlags |= AIFL_NO_FLAME_DAMAGE;
				SET_FLAMING_ZOMBIE( ent->s, 1 );
			}
		}
	} else {
		G_Error( "AI Scripting: giveweapon %s, unknown weapon", params );
	}

	return qtrue;
};

/*
=================
AICast_ScriptAction_TakeWeapon

  syntax: takeweapon <pickupname>
=================
*/
qboolean AICast_ScriptAction_TakeWeapon( cast_state_t *cs, char *params ) {
	int weapon;
	int i;

	weapon = WP_NONE;

	if ( !Q_stricmp( params, "all" ) ) {

		// clear out all weapons
		memset( g_entities[cs->entityNum].client->ps.weapons, 0, sizeof( g_entities[cs->entityNum].client->ps.weapons ) );
		memset( g_entities[cs->entityNum].client->ps.ammo, 0, sizeof( g_entities[cs->entityNum].client->ps.ammo ) );
		memset( g_entities[cs->entityNum].client->ps.ammoclip, 0, sizeof( g_entities[cs->entityNum].client->ps.ammoclip ) );
		cs->weaponNum = WP_NONE;

	} else {


		for ( i = 1; bg_itemlist[i].classname; i++ )
		{
			//----(SA)	first try the name they see in the editor, then the pickup name
			if ( !Q_strcasecmp( params, bg_itemlist[i].classname ) ) {
				weapon = bg_itemlist[i].giTag;
				break;
			}

			if ( !Q_strcasecmp( params, bg_itemlist[i].pickup_name ) ) {
				weapon = bg_itemlist[i].giTag;
				break;
			}
		}

		if ( weapon != WP_NONE ) {
			qboolean clear;

			if ( weapon == WP_AKIMBO ) {
				// take both the colt /and/ the akimbo weapons when 'akimbo' is specified
				COM_BitClear( g_entities[cs->entityNum].client->ps.weapons, WP_COLT );
			} else if ( weapon == WP_COLT ) {
				// take 'akimbo' first if it's there, then take 'colt'
				if ( COM_BitCheck( g_entities[cs->entityNum].client->ps.weapons, WP_AKIMBO ) ) {
					weapon = WP_AKIMBO;
				}
			}

			//
			COM_BitClear( g_entities[cs->entityNum].client->ps.weapons, weapon );
			// also remove the ammo for this weapon
			// but first make sure we dont have any other weapons that use the same ammo
			clear = qtrue;
			for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
				if ( BG_FindAmmoForWeapon( weapon ) != BG_FindAmmoForWeapon( i ) ) {
					continue;
				}
				if ( COM_BitCheck( g_entities[cs->entityNum].client->ps.weapons, i ) ) {
					clear = qfalse;
				}
			}
			if ( clear ) {
// (SA) temp only.  commented out for pistol guys in escape1
//				g_entities[cs->entityNum].client->ps.ammo[BG_FindAmmoForWeapon(weapon)] = 0;
			}
		} else {
			G_Error( "AI Scripting: takeweapon %s, unknown weapon", params );
		}

	}

	if ( !g_entities[cs->entityNum].client->ps.weapons ) {
		if ( cs->bs ) {
			cs->weaponNum = WP_NONE;
		} else {
			g_entities[cs->entityNum].client->ps.weapon = WP_NONE;
		}
	}

	return qtrue;
};



//----(SA)	added

/*
==============
AICast_ScriptAction_GiveInventory
==============
*/
qboolean AICast_ScriptAction_GiveInventory( cast_state_t *cs, char *params ) {
	int i;
	gitem_t     *item = 0;

	for ( i = 1; bg_itemlist[i].classname; i++ ) {
		//----(SA)	first try the name they see in the editor, then the pickup name
		if ( !Q_strcasecmp( params, bg_itemlist[i].classname ) ) {
			item = &bg_itemlist[i];
		}

		if ( !Q_strcasecmp( params, bg_itemlist[i].pickup_name ) ) {
			item = &bg_itemlist[i];
		}
	}

	if ( !item ) { // item not found
		G_Error( "AI Scripting: giveinventory %s, unknown item", params );
	}

	if ( item->giType == IT_KEY ) {
		g_entities[cs->entityNum].client->ps.stats[STAT_KEYS] |= ( 1 << item->giTag );
	} else if ( item->giType == IT_HOLDABLE )      {
		// (SA) TODO
	}

	return qtrue;
};


//----(SA)	end



/*
=================
AICast_ScriptAction_Movetype

  syntax: movetype <walk/run/crouch/default>

  Sets this character's movement type, will exist until another movetype command is called
=================
*/
qboolean AICast_ScriptAction_Movetype( cast_state_t *cs, char *params ) {
	if ( !Q_strcasecmp( params, "walk" ) ) {
		cs->movestate = MS_WALK;
		cs->movestateType = MSTYPE_PERMANENT;
		return qtrue;
	}
	if ( !Q_strcasecmp( params, "run" ) ) {
		cs->movestate = MS_RUN;
		cs->movestateType = MSTYPE_PERMANENT;
		return qtrue;
	}
	if ( !Q_strcasecmp( params, "crouch" ) ) {
		cs->movestate = MS_CROUCH;
		cs->movestateType = MSTYPE_PERMANENT;
		return qtrue;
	}
	if ( !Q_strcasecmp( params, "default" ) ) {
		cs->movestate = MS_DEFAULT;
		cs->movestateType = MSTYPE_NONE;
		return qtrue;
	}

	return qtrue;
}
/*
=================
AICast_ScriptAction_AlertEntity

  syntax: alertentity <targetname>
=================
*/
qboolean AICast_ScriptAction_AlertEntity( cast_state_t *cs, char *params ) {
	gentity_t   *ent;

	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: alertentity without targetname\n" );
	}

	// find this targetname
	ent = G_Find( NULL, FOFS( targetname ), params );
	if ( !ent ) {
		ent = G_Find( NULL, FOFS( aiName ), params ); // look for an AI
		if ( !ent || !ent->client ) { // accept only AI for aiName check
			G_Error( "AI Scripting: alertentity cannot find targetname \"%s\"\n", params );
		}
	}

	// call this entity's AlertEntity function
	if ( !ent->AIScript_AlertEntity ) {
		if ( !ent->client && ent->use && !Q_stricmp( ent->classname, "ai_trigger" ) ) {
			ent->use( ent, NULL, NULL );
			return qtrue;
		}

		if ( aicast_debug.integer ) {
			G_Printf( "AI Scripting: alertentity \"%s\" (classname = %s) doesn't have an \"AIScript_AlertEntity\" function\n", params, ent->classname );
		}
		//G_Error( "AI Scripting: alertentity \"%s\" (classname = %s) doesn't have an \"AIScript_AlertEntity\" function\n", params, ent->classname );
		return qtrue;
	}

	ent->AIScript_AlertEntity( ent );

	return qtrue;
}

/*
=================
AICast_ScriptAction_SaveGame

  NOTE: only use this command in "player" scripts, not for AI

  syntax: savegame
=================
*/
qboolean AICast_ScriptAction_SaveGame( cast_state_t *cs, char *params ) {
	char *pString, *saveName;
	pString = params;

	if ( cs->bs ) {
		G_Error( "AI Scripting: savegame attempted on a non-player" );
	}

//----(SA)	check for parameter
	saveName = COM_ParseExt( &pString, qfalse );
	if ( !saveName[0] ) {
		G_SaveGame( NULL ); // save the default "current" savegame
	} else {
		G_SaveGame( saveName );
	}
//----(SA)	end

	return qtrue;
}

/*
=================
AICast_ScriptAction_FireAtTarget

  syntax: fireattarget <targetname> [duration]
=================
*/
qboolean AICast_ScriptAction_FireAtTarget( cast_state_t *cs, char *params ) {
	gentity_t   *ent;
	vec3_t vec, org, src;
	char *pString, *token;
	float diff;
	int i;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: fireattarget without a targetname\n" );
	}

	if ( !cs->bs ) {
		G_Error( "AI Scripting: fireattarget called for non-AI character\n" );
	}

	// find this targetname
	ent = G_Find( NULL, FOFS( targetname ), token );
	if ( !ent ) {
		ent = AICast_FindEntityForName( token );
		if ( !ent ) {
			G_Error( "AI Scripting: fireattarget cannot find targetname/aiName \"%s\"\n", token );
		}
	}

	// if this is our first call for this fireattarget, record the ammo count
	if ( cs->castScriptStatus.scriptFlags & SFL_FIRST_CALL ) {
		cs->lastWeaponFired = 0;
	}
	// make sure we don't move or shoot while turning to our target
	if ( cs->castScriptStatus.scriptNoAttackTime < level.time ) {
		cs->castScriptStatus.scriptNoAttackTime = level.time + 500;
	}
	// dont reload prematurely
	cs->noReloadTime = level.time + 1000;
	// don't move while firing at all
	//if (cs->castScriptStatus.scriptNoMoveTime < level.time) {
	cs->castScriptStatus.scriptNoMoveTime = level.time + 500;
	//}
	// let us move our view, whether it looks bad or not
	cs->castScriptStatus.playAnimViewlockTime = 0;
	// set the view angle manually
	BG_EvaluateTrajectory( &ent->s.pos, level.time, org );
	VectorCopy( cs->bs->origin, src );
	src[2] += cs->bs->cur_ps.viewheight;
	VectorSubtract( org, src, vec );
	VectorNormalize( vec );
	vectoangles( vec, cs->ideal_viewangles );
	for ( i = 0; i < 2; i++ ) {
		diff = abs( AngleDifference( cs->bs->cur_ps.viewangles[i], cs->ideal_viewangles[i] ) );
		if ( VectorCompare( vec3_origin, ent->s.pos.trDelta ) ) {
			if ( diff ) {
				return qfalse;  // not facing yet
			}
		} else {
			if ( diff > 25 ) {    // allow some slack when target is moving
				return qfalse;
			}
		}
	}

	// force fire
	trap_EA_Attack( cs->bs->client );
	//
	cs->bFlags |= BFL_ATTACKED;
	//
	// if we haven't fired yet
	if ( !cs->lastWeaponFired ) {
		return qfalse;
	}
	//
	// do we need to stay and fire for a duration?
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		return qtrue;   // no need to wait around
	}
	// only return true if we've been firing for long enough
	return ( ( cs->castScriptStatus.castScriptStackChangeTime + atoi( token ) ) < level.time );
}

/*
=================
AICast_ScriptAction_GodMode

  syntax: godmode <on/off>
=================
*/
qboolean AICast_ScriptAction_GodMode( cast_state_t *cs, char *params ) {
	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: godmode requires an on/off specifier\n" );
	}

	if ( !Q_stricmp( params, "on" ) ) {
		g_entities[cs->bs->entitynum].flags |= FL_GODMODE;
	} else if ( !Q_stricmp( params, "off" ) ) {
		g_entities[cs->bs->entitynum].flags &= ~FL_GODMODE;
	} else {
		G_Error( "AI Scripting: godmode requires an on/off specifier\n" );
	}

	return qtrue;
}

/*
=================
AICast_ScriptAction_Accum

  syntax: accum <buffer_index> <command> <paramater>

  Commands:

	accum <n> inc <m>
	accum <n> abort_if_less_than <m>
	accum <n> abort_if_greater_than <m>
	accum <n> abort_if_not_equal <m>
	accum <n> abort_if_equal <m>
	accum <n> set <m>
	accum <n> random <m>
	accum <n> bitset <m>
	accum <n> bitreset <m>
	accum <n> abort_if_bitset <m>
	accum <n> abort_if_not_bitset <m>
=================
*/
qboolean AICast_ScriptAction_Accum( cast_state_t *cs, char *params ) {
	char *pString, *token, lastToken[MAX_QPATH];
	int bufferIndex;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: accum without a buffer index\n" );
	}

	bufferIndex = atoi( token );
	if ( bufferIndex >= MAX_SCRIPT_ACCUM_BUFFERS ) {
		G_Error( "AI Scripting: accum buffer is outside range (0 - %i)\n", MAX_SCRIPT_ACCUM_BUFFERS );
	}

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: accum without a command\n" );
	}

	Q_strncpyz( lastToken, token, sizeof( lastToken ) );
	token = COM_ParseExt( &pString, qfalse );

	if ( !Q_stricmp( lastToken, "inc" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		cs->scriptAccumBuffer[bufferIndex] += atoi( token );
	} else if ( !Q_stricmp( lastToken, "abort_if_less_than" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( cs->scriptAccumBuffer[bufferIndex] < atoi( token ) ) {
			// abort the current script
			cs->castScriptStatus.castScriptStackHead = cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_greater_than" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( cs->scriptAccumBuffer[bufferIndex] > atoi( token ) ) {
			// abort the current script
			cs->castScriptStatus.castScriptStackHead = cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_equal" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( cs->scriptAccumBuffer[bufferIndex] != atoi( token ) ) {
			// abort the current script
			cs->castScriptStatus.castScriptStackHead = cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_equal" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( cs->scriptAccumBuffer[bufferIndex] == atoi( token ) ) {
			// abort the current script
			cs->castScriptStatus.castScriptStackHead = cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "bitset" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		cs->scriptAccumBuffer[bufferIndex] |= ( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "bitreset" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		cs->scriptAccumBuffer[bufferIndex] &= ~( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "abort_if_bitset" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( cs->scriptAccumBuffer[bufferIndex] & ( 1 << atoi( token ) ) ) {
			// abort the current script
			cs->castScriptStatus.castScriptStackHead = cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_bitset" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( !( cs->scriptAccumBuffer[bufferIndex] & ( 1 << atoi( token ) ) ) ) {
			// abort the current script
			cs->castScriptStatus.castScriptStackHead = cs->castScriptEvents[cs->castScriptStatus.castScriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "set" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		cs->scriptAccumBuffer[bufferIndex] = atoi( token );
	} else if ( !Q_stricmp( lastToken, "random" ) ) {
		if ( !token[0] ) {
			G_Error( "AI Scripting: accum %s requires a parameter\n", lastToken );
		}
		cs->scriptAccumBuffer[bufferIndex] = rand() % atoi( token );
	} else {
		G_Error( "AI Scripting: accum %s: unknown command\n", params );
	}

	return qtrue;
}

/*
=================
AICast_ScriptAction_SpawnCast

  syntax: spawncast <classname> <targetname> <ainame>

  <targetname> is the entity marker which has the position and angles of where we want the new
  cast AI to spawn
=================
*/
qboolean AICast_ScriptAction_SpawnCast( cast_state_t *cs, char *params ) {
//	char	*pString, *token;
//	char	*classname;
//	gentity_t	*targetEnt, *newCast;

	G_Error( "AI Scripting: spawncast is no longer functional. Use trigger_spawn instead.\n" );
	return qfalse;
/*
	if (!params[0]) {
		G_Error( "AI Scripting: spawncast without a classname\n" );
	}

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error( "AI Scripting: spawncast without a classname\n" );
	}

	classname = G_Alloc( strlen(token)+1 );
	Q_strncpyz( classname, token, strlen(token)+1 );

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error( "AI Scripting: spawncast without a targetname\n" );
	}

	targetEnt = G_Find( NULL, FOFS(targetname), token );
	if (!targetEnt) {
		G_Error( "AI Scripting: cannot find targetname \"%s\"\n", token );
	}

	token = COM_ParseExt( &pString, qfalse );
	if (!token[0]) {
		G_Error( "AI Scripting: spawncast without an ainame\n" );
	}

	newCast = G_Spawn();
	newCast->classname = classname;
	VectorCopy( targetEnt->s.origin, newCast->s.origin );
	VectorCopy( targetEnt->s.angles, newCast->s.angles );
	newCast->aiName = G_Alloc( strlen(token)+1 );
	Q_strncpyz( newCast->aiName, token, strlen(token)+1 );

	if (!G_CallSpawn( newCast )) {
		G_Error( "AI Scripting: spawncast for unknown entity \"%s\"\n", newCast->classname );
	}

	return qtrue;
*/
}

/*
=================
AICast_ScriptAction_MissionFailed

  syntax: missionfailed  <time>
=================
*/
qboolean AICast_ScriptAction_MissionFailed( cast_state_t *cs, char *params ) {
	char    *pString, *token;
	int time = 6, mof = 0;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );   // time
	if ( token && token[0] ) {
		time = atoi( token );
	}

	token = COM_ParseExt( &pString, qfalse );   // mof (means of failure)
	if ( token && token[0] ) {
		mof = atoi( token );
	}

	// play mission fail music
	trap_SendServerCommand( -1, "mu_play sound/music/l_failed_1.wav 0\n" );
	// clear queue so it'll be quiet after failed stinger
	trap_SetConfigstring( CS_MUSIC_QUEUE, "" );

	// fade all sound out
	trap_SendServerCommand( -1, va( "snd_fade 0 %d", time * 1000 ) );

	if ( mof < 0 ) {
		mof = 0;
	}
	trap_SendServerCommand( -1, va( "cp missionfail%d", mof ) );

	// reload the current savegame, after a delay
	trap_SetConfigstring( CS_SCREENFADE, va( "1 %i %i", level.time + 250, time * 1000 ) );
//	reloading = RELOAD_FAILED;
	trap_Cvar_Set( "g_reloading", va( "%d", RELOAD_FAILED ) );

	level.reloadDelayTime = level.time + 1000 + time * 1000;

	return qtrue;
}

/*
=================
AICast_ScriptAction_ObjectivesNeeded

  syntax: objectivesneeded <num_objectives>
=================
*/
qboolean AICast_ScriptAction_ObjectivesNeeded( cast_state_t *cs, char *params ) {
	char *pString, *token;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: objectivesneeded requires a num_objectives identifier\n" );
	}

	level.numObjectives = atoi( token );

	return qtrue;
}


/*
=================
AICast_ScriptAction_ObjectiveMet

  syntax: objectivemet <num_objective>
  also (for backwards compaiblity): missionsuccess <num_objective> [nodisplay]
=================
*/
qboolean AICast_ScriptAction_ObjectiveMet( cast_state_t *cs, char *params ) {
	gentity_t   *player;
	vmCvar_t cvar;
	int lvl;
	char *pString, *token;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: missionsuccess requires a num_objective identifier\n" );
	}

	player = AICast_FindEntityForName( "player" );
	// double check that they are still alive
	if ( player->health <= 0 ) {
		return qfalse;  // hold the script here

	}
	lvl = atoi( token );

// if you've already got it, just return.  don't need to set 'yougotmail'
	if ( player->missionObjectives & ( 1 << ( lvl - 1 ) ) ) {
		return qtrue;
	}

	player->missionObjectives |= ( 1 << ( lvl - 1 ) );  // make this bitwise

	//set g_objective<n> cvar
	trap_Cvar_Register( &cvar, va( "g_objective%i", lvl ), "1", CVAR_ROM );
	// set it to make sure
	trap_Cvar_Set( va( "g_objective%i", lvl ), "1" );

	token = COM_ParseExt( &pString, qfalse );
	if ( token[0] ) {
		if ( Q_strcasecmp( token,"nodisplay" ) ) {   // unknown command
			G_Error( "AI Scripting: missionsuccess with unknown parameter: %s\n", token );
		}
	} else {    // show on-screen information
		trap_Cvar_Set( "cg_youGotMail", "2" ); // set flag to draw icon
	}

	return qtrue;
}

/*
=================
AICast_ScriptAction_NoAIDamage

  syntax: noaidamage <on/off>
=================
*/
qboolean AICast_ScriptAction_NoAIDamage( cast_state_t *cs, char *params ) {
	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: noaidamage requires an on/off specifier\n" );
	}

	if ( !Q_stricmp( params, "on" ) ) {
		cs->castScriptStatus.scriptFlags |= SFL_NOAIDAMAGE;
	} else if ( !Q_stricmp( params, "off" ) ) {
		cs->castScriptStatus.scriptFlags &= ~SFL_NOAIDAMAGE;
	} else {
		G_Error( "AI Scripting: noaidamage requires an on/off specifier\n" );
	}
	return qtrue;
}

/*
=================
AICast_ScriptAction_Print

  syntax: print <text>

  Mostly for debugging purposes
=================
*/
qboolean AICast_ScriptAction_Print( cast_state_t *cs, char *params ) {
	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: print requires some text\n" );
	}

	G_Printf( "(AI) %s-> %s\n", g_entities[cs->entityNum].aiName, params );
	return qtrue;
}

/*
=================
AICast_ScriptAction_FaceTargetAngles

  syntax: facetargetangles <targetname>

  The AI will face the same direction that the target entity is facing
=================
*/
qboolean AICast_ScriptAction_FaceTargetAngles( cast_state_t *cs, char *params ) {
	gentity_t   *targetEnt;

	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: facetargetangles requires a targetname\n" );
	}

	targetEnt = G_Find( NULL, FOFS( targetname ), params );
	if ( !targetEnt ) {
		G_Error( "AI Scripting: cannot find targetname \"%s\"\n", params );
	}

	VectorCopy( targetEnt->s.angles, cs->ideal_viewangles );

	return qtrue;
}

/*
===================
AICast_ScriptAction_ResetScript

	causes any currently running scripts to abort, in favour of the current script
===================
*/
qboolean AICast_ScriptAction_ResetScript( cast_state_t *cs, char *params ) {
	gclient_t *client;

	client = &level.clients[cs->entityNum];

	// stop any anim from playing
	if ( client->ps.torsoTimer && ( client->ps.torsoTimer > ( level.time - cs->scriptAnimTime ) ) ) {
		if ( ( client->ps.torsoAnim & ~ANIM_TOGGLEBIT ) == cs->scriptAnimNum ) {
			client->ps.torsoTimer = 0;
		}
	}
	if ( client->ps.legsTimer && ( client->ps.legsTimer > ( level.time - cs->scriptAnimTime ) ) ) {
		if ( ( client->ps.legsAnim & ~ANIM_TOGGLEBIT ) == cs->scriptAnimNum ) {
			client->ps.legsTimer = 0;
		}
	}

	// stop playing voice channel sounds
	G_AddEvent( &g_entities[cs->bs->entitynum], EV_GENERAL_SOUND, G_SoundIndex( "Blank" ) );

	cs->castScriptStatus.scriptNoMoveTime = 0;
	cs->castScriptStatus.playAnimViewlockTime = 0;
	// stop following anything that we don't need to be following
	cs->followEntity = -1;
	if ( cs->castScriptStatus.scriptFlags & SFL_FIRST_CALL ) {
		return qfalse;
	}

	// make sure zoom is off
	cs->aiFlags &= ~AIFL_ZOOMING;
	g_entities[cs->entityNum].client->ps.eFlags &= ~EF_CIG; //----(SA)	added

	return qtrue;
}

/*
===================
AICast_ScriptAction_Mount

	syntax: mount <targetname>

  Used to an AI to mount the MG42
===================
*/
qboolean AICast_ScriptAction_Mount( cast_state_t *cs, char *params ) {
	gentity_t   *targetEnt, *ent;
	vec3_t vec;
	float dist;

	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: mount requires a targetname\n" );
	}

	targetEnt = G_Find( NULL, FOFS( targetname ), params );
	if ( !targetEnt ) {
		G_Error( "AI Scripting: cannot find targetname \"%s\"\n", params );
	}

	VectorSubtract( targetEnt->r.currentOrigin, cs->bs->origin, vec );
	dist = VectorNormalize( vec );
	vectoangles( vec, cs->ideal_viewangles );

	if ( dist > 40 ) {
		// walk towards it
		trap_EA_Move( cs->entityNum, vec, 80 );
		return qfalse;
	}

	if ( !targetEnt->takedamage ) {
		// the gun has been destroyed
		return qtrue;
	}

	// if we are facing it, start holding activate
	if ( fabs( cs->ideal_viewangles[YAW] - cs->viewangles[YAW] ) < 10 ) {
		ent = &g_entities[cs->entityNum];
		Cmd_Activate_f( ent );
		// did we mount it?
		if ( ent->active && targetEnt->r.ownerNum == ent->s.number ) {
			cs->mountedEntity = targetEnt->s.number;
			AIFunc_BattleMG42Start( cs );
			return qtrue;
		}
	}

	return qfalse;
}

/*
===================
AICast_ScriptAction_Unmount

	syntax: unmount

  Stop using their current mounted entity
===================
*/
qboolean AICast_ScriptAction_Unmount( cast_state_t *cs, char *params ) {
	gentity_t   *ent, *mg42;

	ent = &g_entities[cs->entityNum];
	mg42 = &g_entities[cs->mountedEntity];

	if ( !ent->active ) {
		return qtrue;   // nothing mounted, just skip this command
	}
	// face straight forward
	VectorCopy( mg42->s.angles, cs->ideal_viewangles );
	// try and unmount
	Cmd_Activate_f( ent );
	if ( !ent->active ) {
		return qtrue;
	}
	// waiting to unmount still
	return qfalse;
}

/*
====================
AICast_ScriptAction_SavePersistant

	syntax: savepersistant <next_mapname>

  Saves out the data that should be retained between certain levels. Not
  calling this routine before changing levels, is the equivalent of resetting
  the player's inventory/health/etc.

  The <next_mapname> is used to identify the next map we don't
  accidentally read in persistant data that was intended for a different map.
====================
*/
qboolean AICast_ScriptAction_SavePersistant( cast_state_t *cs, char *params ) {
	G_SavePersistant( params );
	return qtrue;
}



/*
==============
AICast_ScriptAction_Teleport
==============
*/
qboolean AICast_ScriptAction_Teleport( cast_state_t *cs, char *params ) {
	gentity_t   *dest;

	dest =  G_PickTarget( params );
	if ( !dest ) {
		G_Error( "AI Scripting: couldn't find teleporter destination: '%s'\n", params );
	}

	TeleportPlayer( &g_entities[cs->entityNum], dest->s.origin, dest->s.angles );

	return qtrue;
}



extern void G_EndGame( void );

/*
==============
AICast_ScriptAction_EndGame
==============
*/
qboolean AICast_ScriptAction_EndGame( cast_state_t *cs, char *params ) {
	G_EndGame();
	return qtrue;
}




/*
====================
AICast_ScriptAction_ChangeLevel

	syntax: changelevel <mapname> [exitTime] <persistent> <silent>

  Issues an spdevmap/spmap to the consol.
  Optionally add
	"persistent" if you want the player to keep their inventory through the transition.
	"silent" if you want it to not play the mission success music

====================
*/
qboolean AICast_ScriptAction_ChangeLevel( cast_state_t *cs, char *params ) {
	int i;
	char *pch, *pch2, *newstr;
	gentity_t   *player;
	qboolean silent = qfalse, endgame = qfalse, savepersist = qfalse;
	int exitTime = 8000;

	player = AICast_FindEntityForName( "player" );
	// double check that they are still alive
	if ( player->health <= 0 ) {
		return qtrue;   // get out of here

	}
	// don't process if already changing
//	if(reloading)
	if ( g_reloading.integer ) {
		return qtrue;
	}

	// save persistent data if required
	newstr = va( params );
	pch = strstr( newstr, " persistent" ); // (SA) whoops, this was mis-spelled
	if ( pch ) {
		pch = strstr( newstr, " " );
		*pch = '\0';
		savepersist = qtrue;
	}

	//
	newstr = va( params );
	pch = strstr( newstr, " silent" );
	if ( pch ) {
		pch = strstr( newstr, " " );
		*pch = '\0';
		silent = qtrue;
	}

	// make sure we strip any params after the mapname
	newstr = va( params );
	pch = strstr( newstr, " " );
	if ( pch ) {
		*( pch++ ) = '\0';
		//
		// see if there is a mission_level specified
		pch2 = strstr( pch, " " );
		if ( pch2 ) { // kill the space if exists
			*pch2 = '\0';
		}

		if ( atoi( pch ) ) { // there's a 'time' specified
			exitTime = atoi( pch );
		}
	}

	if ( !Q_stricmp( newstr, "gamefinished" ) ) { // 'gamefinished' is keyword for 'exit to credits'
		endgame = qtrue;
	}

	if ( !endgame ) {

		// check for missing objectives
		for ( i = 0; i < level.numObjectives; i++ ) {
			if ( !( player->missionObjectives & ( 1 << i ) ) ) {
				trap_SendServerCommand( -1, "cp objectivesnotcomplete" );
				return qtrue;
			}
		}

		if ( savepersist ) {
			G_SavePersistant( newstr ); // save persistent data if required

		}
	}


	if ( !silent && !endgame ) {
		trap_SendServerCommand( -1, "mu_play sound/music/l_complete_1.wav 0\n" );   // play mission success music
	}
	trap_SetConfigstring( CS_MUSIC_QUEUE, "" );  // don't try to start anything.  no level load music

	trap_SetConfigstring( CS_SCREENFADE, va( "1 %i %i", level.time + 250, 750 + exitTime ) ); // fade out screen

	trap_SendServerCommand( -1, va( "snd_fade 0 %d", 1000 + exitTime ) ); //----(SA)	added

	// load the next map, after a delay
	level.reloadDelayTime = level.time + 1000 + exitTime;
	trap_Cvar_Set( "g_reloading", va( "%d", RELOAD_NEXTMAP_WAITING ) );

	if ( endgame ) {
		trap_Cvar_Set( "g_reloading", va( "%d", RELOAD_ENDGAME ) );
		return qtrue;
	}

	Q_strncpyz( level.nextMap, newstr, sizeof( level.nextMap ) );

	//if (g_cheats.integer)
	//	trap_SendConsoleCommand( EXEC_APPEND, va("spdevmap %s\n", newstr) );
	//else
	//	trap_SendConsoleCommand( EXEC_APPEND, va("spmap %s\n", newstr ) );

	return qtrue;
}

/*
==================
AICast_ScriptAction_FoundSecret
==================
*/
qboolean AICast_ScriptAction_FoundSecret( cast_state_t *cs, char *params ) {
	gentity_t *player = AICast_FindEntityForName( "player" );
//	level.numSecretsFound++;
	player->numSecretsFound++;
	trap_SendServerCommand( -1, "cp secretarea" );
	G_SendMissionStats();
	return qtrue;
}

/*
==================
AICast_ScriptAction_NoSight

  syntax: nosight <duration>
==================
*/
qboolean AICast_ScriptAction_NoSight( cast_state_t *cs, char *params ) {
	if ( !params ) {
		G_Error( "AI Scripting: syntax error\n\nnosight <duration>\n" );
	}

	cs->castScriptStatus.scriptNoSightTime = level.time + atoi( params );

	return qtrue;
}

/*
==================
AICast_ScriptAction_Sight

  syntax: sight
==================
*/
qboolean AICast_ScriptAction_Sight( cast_state_t *cs, char *params ) {
	cs->castScriptStatus.scriptNoSightTime = 0;
	return qtrue;
}

/*
==================
AICast_ScriptAction_NoAvoid

  syntax: noavoid
==================
*/
qboolean AICast_ScriptAction_NoAvoid( cast_state_t *cs, char *params ) {
	cs->aiFlags |= AIFL_NOAVOID;
	return qtrue;
}

/*
==================
AICast_ScriptAction_Avoid

  syntax: avoid
==================
*/
qboolean AICast_ScriptAction_Avoid( cast_state_t *cs, char *params ) {
	cs->aiFlags &= ~AIFL_NOAVOID;
	return qtrue;
}

/*
==================
AICast_ScriptAction_Attrib

  syntax: attrib <attribute> <value>
==================
*/
qboolean AICast_ScriptAction_Attrib( cast_state_t *cs, char *params ) {
	char    *pString, *token;
	int i;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: attrib <attribute> <value>" );
	}

	for ( i = 0; i < AICAST_MAX_ATTRIBUTES; i++ ) {
		if ( !Q_strcasecmp( token, castAttributeStrings[i] ) ) {
			// found a match, read in the value
			token = COM_ParseExt( &pString, qfalse );
			if ( !token[0] ) {
				G_Error( "AI_Scripting: syntax: attrib <attribute> <value>" );
			}
			// set the attribute
			cs->attributes[i] = atof( token );
			break;
		}
	}

	return qtrue;
}

/*
=================
AICast_ScriptAction_DenyAction

  syntax: deny
=================
*/
qboolean AICast_ScriptAction_DenyAction( cast_state_t *cs, char *params ) {
	cs->aiFlags |= AIFL_DENYACTION;
	return qtrue;
}

/*
=================
AICast_ScriptAction_LightningDamage
=================
*/
qboolean AICast_ScriptAction_LightningDamage( cast_state_t *cs, char *params ) {
	Q_strlwr( params );
	if ( !Q_stricmp( params, "on" ) ) {
		cs->aiFlags |= AIFL_ROLL_ANIM;  // hijacking this since the player doesn't use it
	} else {
		cs->aiFlags &= ~AIFL_ROLL_ANIM;
	}
	return qtrue;
}

/*
=================
AICast_ScriptAction_Headlook
=================
*/
qboolean AICast_ScriptAction_Headlook( cast_state_t *cs, char *params ) {
	char    *pString, *token;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: headlook <ON/OFF>" );
	}
	Q_strlwr( token );

	if ( !Q_stricmp( token, "on" ) ) {
		cs->aiFlags &= ~AIFL_NO_HEADLOOK;
	} else if ( !Q_stricmp( token, "off" ) ) {
		cs->aiFlags |= AIFL_NO_HEADLOOK;
	} else {
		G_Error( "AI_Scripting: syntax: headlook <ON/OFF>" );
	}

	return qtrue;
}

/*
=================
AICast_ScriptAction_BackupScript

  backs up the current state of the scripting, so we can restore it later and resume
  were we left off (useful if player gets in our way)
=================
*/
qboolean AICast_ScriptAction_BackupScript( cast_state_t *cs, char *params ) {

	if ( !( cs->castScriptStatus.scriptFlags & SFL_WAITING_RESTORE ) ) {
		cs->castScriptStatusBackup = cs->castScriptStatusCurrent;
		cs->castScriptStatus.scriptFlags |= SFL_WAITING_RESTORE;
	}

	return qtrue;
}

/*
=================
AICast_ScriptAction_RestoreScript

  restores the state of the scripting to the previous backup
=================
*/
qboolean AICast_ScriptAction_RestoreScript( cast_state_t *cs, char *params ) {

	cs->castScriptStatus = cs->castScriptStatusBackup;

	// make sure we restart any goto's
	cs->castScriptStatus.scriptGotoId = -1;
	cs->castScriptStatus.scriptGotoEnt = -1;

	return qfalse;  // dont continue scripting until next frame
}

/*
=================
AICast_ScriptAction_StateType

  set the current state for this character
=================
*/
qboolean AICast_ScriptAction_StateType( cast_state_t *cs, char *params ) {

	if ( !Q_stricmp( params, "alert" ) ) {
		cs->aiState = AISTATE_ALERT;
	} else if ( !Q_stricmp( params, "relaxed" ) ) {
		cs->aiState = AISTATE_RELAXED;
	}

	return qtrue;
}

/*
================
AICast_ScriptAction_KnockBack

  syntax: knockback [ON/OFF]
================
*/
qboolean AICast_ScriptAction_KnockBack( cast_state_t *cs, char *params ) {

	char    *pString, *token;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: knockback <ON/OFF>" );
	}
	Q_strlwr( token );

	if ( !Q_stricmp( token, "on" ) ) {
		g_entities[cs->entityNum].flags &= ~FL_NO_KNOCKBACK;
	} else if ( !Q_stricmp( token, "off" ) ) {
		g_entities[cs->entityNum].flags |= FL_NO_KNOCKBACK;
	} else {
		G_Error( "AI_Scripting: syntax: knockback <ON/OFF>" );
	}

	return qtrue;

}

/*
================
AICast_ScriptAction_Zoom

  syntax: zoom [ON/OFF]
================
*/
qboolean AICast_ScriptAction_Zoom( cast_state_t *cs, char *params ) {

	char    *pString, *token;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: zoom <ON/OFF>" );
	}
	Q_strlwr( token );

	// give them the inventory item
	g_entities[cs->entityNum].client->ps.stats[STAT_KEYS] |= ( 1 << INV_BINOCS );

	if ( !Q_stricmp( token, "on" ) ) {
		cs->aiFlags |= AIFL_ZOOMING;
	} else if ( !Q_stricmp( token, "off" ) ) {
		cs->aiFlags &= ~AIFL_ZOOMING;
	} else {
		G_Error( "AI_Scripting: syntax: zoom <ON/OFF>" );
	}

	return qtrue;

}

/*
===================
AICast_ScriptAction_StartCam

  syntax: startcam <camera filename>
===================
*/
qboolean ScriptStartCam( cast_state_t *cs, char *params, qboolean black ) {
	char *pString, *token;
	gentity_t *ent;

	ent = &g_entities[cs->entityNum];

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_Cam: filename parameter required\n" );
	}

	// turn off noclient flag
	ent->r.svFlags &= ~SVF_NOCLIENT;

	// issue a start camera command to the client
	trap_SendServerCommand( cs->entityNum, va( "startCam %s %d", token, (int)black ) );

	return qtrue;
}

qboolean AICast_ScriptAction_StartCam( cast_state_t *cs, char *params ) {
	return ScriptStartCam( cs, params, qfalse );
}
qboolean AICast_ScriptAction_StartCamBlack( cast_state_t *cs, char *params ) {
	return ScriptStartCam( cs, params, qtrue );
}


//----(SA)	added
qboolean AICast_ScriptAction_StopCamBlack( cast_state_t *cs, char *params ) {
	trap_SendServerCommand( cs->entityNum, "stopCamblack" );
	return qtrue;
}

qboolean AICast_ScriptAction_StopCam( cast_state_t *cs, char *params ) {
	trap_SendServerCommand( cs->entityNum, "stopCam" );
	return qtrue;
}
//----(SA)	end


//----(SA)	added
qboolean AICast_ScriptAction_Cigarette( cast_state_t *cs, char *params ) {

	char    *pString, *token;
	gentity_t *ent;

	ent = &g_entities[cs->entityNum];

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: cigarette <ON/OFF>" );
	}
	Q_strlwr( token );

	if ( !Q_stricmp( token, "on" ) ) {
		g_entities[cs->entityNum].client->ps.eFlags |= EF_CIG;
	} else if ( !Q_stricmp( token, "off" ) ) {
		g_entities[cs->entityNum].client->ps.eFlags &= ~EF_CIG;
	} else {
		G_Error( "AI_Scripting: syntax: cigarette <ON/OFF>" );
	}

	return qtrue;
}
//----(SA)	end

/*
=================
AICast_ScriptAction_Parachute

  syntax: parachute [ON/OFF]
=================
*/
qboolean AICast_ScriptAction_Parachute( cast_state_t *cs, char *params ) {

	char    *pString, *token;
	gentity_t *ent;

	ent = &g_entities[cs->entityNum];

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: parachute <ON/OFF>" );
	}
	Q_strlwr( token );

	if ( !Q_stricmp( token, "on" ) ) {
		ent->flags |= FL_PARACHUTE;
	} else if ( !Q_stricmp( token, "off" ) ) {
		ent->flags &= ~FL_PARACHUTE;
	} else {
		G_Error( "AI_Scripting: syntax: parachute <ON/OFF>" );
	}

	return qtrue;

}

/*
=================
AICast_ScriptAction_EntityScriptName
=================
*/
qboolean AICast_ScriptAction_EntityScriptName( cast_state_t *cs, char *params ) {
	trap_Cvar_Set( "g_scriptName", params );
	return qtrue;
}


/*
=================
AICast_ScriptAction_AIScriptName
=================
*/
qboolean AICast_ScriptAction_AIScriptName( cast_state_t *cs, char *params ) {
	trap_Cvar_Set( "ai_scriptName", params );
	return qtrue;
}

/*
=================
AICast_ScriptAction_SetHealth
=================
*/
qboolean AICast_ScriptAction_SetHealth( cast_state_t *cs, char *params ) {
	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: sethealth requires a health value" );
	}

	g_entities[cs->entityNum].health = atoi( params );
	g_entities[cs->entityNum].client->ps.stats[STAT_HEALTH] = atoi( params );

	return qtrue;
}

/*
=================
AICast_ScriptAction_NoTarget

  syntax: notarget ON/OFF
=================
*/
qboolean AICast_ScriptAction_NoTarget( cast_state_t *cs, char *params ) {
	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: notarget requires ON or OFF as parameter" );
	}

	if ( !Q_strcasecmp( params, "ON" ) ) {
		g_entities[cs->entityNum].flags |= FL_NOTARGET;
	} else if ( !Q_strcasecmp( params, "OFF" ) ) {
		g_entities[cs->entityNum].flags &= ~FL_NOTARGET;
	} else {
		G_Error( "AI Scripting: notarget requires ON or OFF as parameter" );
	}

	return qtrue;
}

/*
==================
AICast_ScriptAction_Cvar
==================
*/
qboolean AICast_ScriptAction_Cvar( cast_state_t *cs, char *params ) {
	vmCvar_t cvar;
	char    *pString, *token;
	char cvarName[MAX_QPATH];

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: cvar <cvarName> <cvarValue>" );
	}
	Q_strncpyz( cvarName, token, sizeof( cvarName ) );

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: cvar <cvarName> <cvarValue>" );
	}

	if ( !strcmp( cvarName, "objective" ) ) {
		G_Printf( "WARNING: 'objective' cvar set from script.  Do not set directly.  Use 'missionsuccess <num>'\n" );
		return qtrue;
	}

	trap_Cvar_Register( &cvar, cvarName, token, CVAR_ROM );
	// set it to make sure
	trap_Cvar_Set( cvarName, token );
	return qtrue;
}



//----(SA)	added some music interface

/*
==================
AICast_ScriptAction_MusicStart

==================
*/
qboolean AICast_ScriptAction_MusicStart( cast_state_t *cs, char *params ) {
	char    *pString, *token;
	char cvarName[MAX_QPATH];
	int fadeupTime = 0;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: mu_start <musicfile> <fadeuptime>" );
	}
	Q_strncpyz( cvarName, token, sizeof( cvarName ) );

	token = COM_ParseExt( &pString, qfalse );
	if ( token[0] ) {
		fadeupTime = atoi( token );
	}

	trap_SendServerCommand( cs->entityNum, va( "mu_start %s %d", cvarName, fadeupTime ) );

	return qtrue;
}

/*
==================
AICast_ScriptAction_MusicPlay

==================
*/
qboolean AICast_ScriptAction_MusicPlay( cast_state_t *cs, char *params ) {
	char    *pString, *token;
	char cvarName[MAX_QPATH];
	int fadeupTime = 0;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: mu_play <musicfile> [fadeup time]" );
	}
	Q_strncpyz( cvarName, token, sizeof( cvarName ) );

	trap_SendServerCommand( cs->entityNum, va( "mu_play %s %d", cvarName, fadeupTime ) );

	return qtrue;
}


/*
==================
AICast_ScriptAction_MusicStop
==================
*/
qboolean AICast_ScriptAction_MusicStop( cast_state_t *cs, char *params ) {
	char    *pString, *token;
	int fadeoutTime = 0;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( token[0] ) {
		fadeoutTime = atoi( token );
	}

	trap_SendServerCommand( cs->entityNum, va( "mu_stop %i", fadeoutTime ) );

	return qtrue;
}


/*
==================
AICast_ScriptAction_MusicFade
==================
*/
qboolean AICast_ScriptAction_MusicFade( cast_state_t *cs, char *params ) {
	char    *pString, *token;
	float targetvol;
	int fadetime;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: mu_fade <targetvol> <fadetime>" );
	}
	targetvol = atof( token );

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: mu_fade <targetvol> <fadetime>" );
	}
	fadetime = atoi( token );

	trap_SendServerCommand( cs->entityNum, va( "mu_fade %f %i", targetvol, fadetime ) );

	return qtrue;
}


/*
==================
AICast_ScriptAction_MusicQueue
==================
*/
qboolean AICast_ScriptAction_MusicQueue( cast_state_t *cs, char *params ) {
	char    *pString, *token;
	char cvarName[MAX_QPATH];

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: mu_queue <musicfile>" );
	}
	Q_strncpyz( cvarName, token, sizeof( cvarName ) );

	trap_SetConfigstring( CS_MUSIC_QUEUE, cvarName );

	return qtrue;
}


/*
=================
AICast_ScriptAction_ExplicitRouting

  syntax: explicit_routing <on/off>
=================
*/

qboolean AICast_ScriptAction_ExplicitRouting( cast_state_t *cs, char *params ) {
	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: explicit_routing requires an on/off specifier\n" );
	}

	if ( !Q_stricmp( params, "on" ) ) {
		cs->aiFlags |= AIFL_EXPLICIT_ROUTING;
	} else if ( !Q_stricmp( params, "off" ) ) {
		cs->aiFlags &= ~AIFL_EXPLICIT_ROUTING;
	} else {
		G_Error( "AI Scripting: explicit_routing requires an on/off specifier\n" );
	}

	return qtrue;
}

/*
=================
AICast_ScriptAction_LockPlayer

  syntax: lockplayer <ON/OFF>
=================
*/
qboolean AICast_ScriptAction_LockPlayer( cast_state_t *cs, char *params ) {
	gentity_t *ent;

	ent = &g_entities[cs->entityNum];

	if ( !params || !params[0] ) {
		G_Error( "AI Scripting: lockplayer requires an on/off specifier\n" );
	}

	if ( !Q_stricmp( params, "on" ) ) {
		ent->client->ps.pm_flags |= PMF_IGNORE_INPUT;
	} else if ( !Q_stricmp( params, "off" ) ) {
		ent->client->ps.pm_flags &= ~PMF_IGNORE_INPUT;
	} else {
		G_Error( "AI Scripting: lockplayer requires an on/off specifier\n" );
	}

	return qtrue;
}

/*
==================
AICast_ScriptAction_AnimCondition

  syntax: anim_condition <condition> <string>
==================
*/
qboolean AICast_ScriptAction_AnimCondition( cast_state_t *cs, char *params ) {
	char    *pString, *token;
	char condition[MAX_QPATH];

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: anim_condition <condition> <string>" );
	}
	Q_strncpyz( condition, token, sizeof( condition ) );
	//
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI_Scripting: syntax: anim_condition <condition> <string>" );
	}
	//
	BG_UpdateConditionValueStrings( cs->entityNum, condition, token );
	//
	return qtrue;
}

/*
================
AICast_ScriptAction_PushAway

  syntax: pushaway <ainame>
================
*/
qboolean AICast_ScriptAction_PushAway( cast_state_t *cs, char *params ) {
	gentity_t *pushed;
	vec3_t v, ang, f, r;

	if ( !params || !params[0] ) {
		G_Error( "AI_Scripting: syntax: pushaway <ainame>" );
	}
	// find them
	pushed = AICast_FindEntityForName( params );
	if ( !pushed ) {
		G_Error( "AI_Scripting: pushaway: cannot find \"%s\"", params );
	}
	// calc the vecs
	VectorSubtract( pushed->s.pos.trBase, cs->bs->origin, v );
	VectorNormalize( v );
	vectoangles( v, ang );
	AngleVectors( ang, f, r, NULL );
	// push them away and to the side
	VectorScale( f, 200, v );
	VectorMA( v, ( level.time % 5000 < 2500 ? 200 : 200 ), r, v );
	VectorAdd( pushed->client->ps.velocity, v, pushed->client->ps.velocity );
	//
	return qtrue;
}

/*
==================
AICast_ScriptAction_CatchFire
==================
*/
qboolean AICast_ScriptAction_CatchFire( cast_state_t *cs, char *params ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	//
	ent->s.onFireEnd = level.time + 99999;  // make sure it goes for longer than they need to die
	ent->flameBurnEnt = ENTITYNUM_WORLD;
	// add to playerState for client-side effect
	ent->client->ps.onFireStart = level.time;
	//
	return qtrue;
}
