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
// Name:			ai_cast_think.c
// Function:		Wolfenstein AI Character Thinking
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
The lowest level of Cast AI thinking.
*/

/*
============
AICast_ProcessAIFunctions
============
*/
void AICast_ProcessAIFunctions( cast_state_t *cs, float thinktime ) {
	int i;
	char    *funcname;

	//check for air
	BotCheckAir( cs->bs );
	//if the cast has no ai function
	if ( !cs->aifunc ) {
		AIFunc_DefaultStart( cs );
	}
	//
	// call AI funcs for this cast
	//
	AICast_DBG_InitAIFuncs();
	//
	// only allow looping in debug mode (since it's much slower)
	for ( i = 0; i < ( aicast_debug.integer ? MAX_AIFUNCS : 1 ); i++ )
	{
		if ( !( funcname = cs->aifunc( cs ) ) ) {
			break;
		} else {
			trap_BotResetAvoidReach( cs->bs->ms );    // reset avoidreach
			cs->thinkFuncChangeTime = level.time;
			AICast_DBG_AddAIFunc( cs, funcname );
		}
	}
	//
	//if the cast executed too many AI functions
	//
	if ( aicast_debug.integer && i >= MAX_AIFUNCS ) {
		AICast_DBG_ListAIFuncs( cs, 10 );   // print the last 10 funcs called
	}
}


/*
==============
AICast_ChangeViewAngles
==============
*/
void AICast_ChangeViewAngles( cast_state_t *cs, float thinktime ) {
	float diff, factor, maxchange, anglespeed;
	int i;
	bot_state_t *bs;

	bs = cs->bs;
	//
	// restore locked viewangles if required
	if ( cs->aiFlags & AIFL_VIEWLOCKED ) {
		VectorCopy( cs->viewlock_viewangles, cs->ideal_viewangles );
	} else {
		// check for playanim angles
		if ( cs->castScriptStatus.playAnimViewlockTime >= level.time ) {
			// check to make sure we are still playing a legs animation
			if ( !g_entities[cs->entityNum].client->ps.legsTimer ) {
				cs->castScriptStatus.playAnimViewlockTime = 0;
			} else {
				VectorCopy( cs->castScriptStatus.playanim_viewangles, cs->ideal_viewangles );
			}
		}
	}
	//
	if ( cs->ideal_viewangles[PITCH] > 180 ) {
		cs->ideal_viewangles[PITCH] -= 360;
	}
	//
	maxchange = cs->attributes[YAW_SPEED]; //300;
	if ( cs->aiState >= AISTATE_COMBAT ) {
		factor = 2.0;
		maxchange *= 2.0;
	} else {
		factor = 0.7;
	}
	//
	if ( cs->lockViewAnglesTime < level.time ) {
		maxchange *= thinktime;
		for ( i = 0; i < 3; i++ ) {
			diff = fabs( AngleDifference( cs->viewangles[i], cs->ideal_viewangles[i] ) );
			anglespeed = diff * factor;
			if ( cs->aiState >= AISTATE_COMBAT ) {
				if ( anglespeed < cs->attributes[YAW_SPEED] ) {
					anglespeed = cs->attributes[YAW_SPEED];
				}
			}
			if ( thinktime != 9999.0f ) {
				if ( anglespeed > maxchange ) {
					anglespeed = maxchange;
				}
			}
			cs->viewangles[i] = BotChangeViewAngle( cs->viewangles[i],
													cs->ideal_viewangles[i], anglespeed );
			//BotAI_Print(PRT_MESSAGE, "ideal_angles %f %f\n", cs->ideal_viewangles[0], cs->ideal_viewangles[1], cs->ideal_viewangles[2]);`
			//cs->viewangles[i] = cs->ideal_viewangles[i];
		}
	}
	if ( cs->viewangles[PITCH] > 180 ) {
		cs->viewangles[PITCH] -= 360;
	}
	//elementary action: view
	trap_EA_View( bs->client, cs->viewangles );
}


/*
==============
AICast_InputToUserCommand
==============
*/
static int serverTime;
void AICast_InputToUserCommand( cast_state_t *cs, bot_input_t *bi, usercmd_t *ucmd, int delta_angles[3] ) {
	vec3_t angles, forward, right, up;
	short temp;
	int j;
	signed char movechar;
	gentity_t *ent;

	ent = &g_entities[cs->entityNum];

	//clear the whole structure
	memset( ucmd, 0, sizeof( usercmd_t ) );
	//
	//Com_Printf("dir = %f %f %f speed = %f\n", bi->dir[0], bi->dir[1], bi->dir[2], bi->speed);
	//the duration for the user command in milli seconds
	ucmd->serverTime = serverTime;
	//crouch/movedown
	if ( aiDefaults[cs->aiCharacter].attributes[ATTACK_CROUCH] ) {    // only crouch if this character is physically able to
		if ( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE && bi->actionflags & ACTION_CROUCH ) {
			ucmd->upmove -= 127;
		}
	}
	//
	// actions not effected by script pausing
	//
	// set zoom button
	if ( cs->aiFlags & AIFL_ZOOMING ) {
		ucmd->wbuttons |= WBUTTON_ZOOM;
	}
	//set the buttons
	if ( bi->actionflags & ACTION_ATTACK ) {
		vec3_t ofs;
		// don't fire if we are not facing the right direction yet
		if ( ( cs->triggerReleaseTime < level.time ) &&
			 (   ( cs->lockViewAnglesTime >= level.time ) ||
				 ( fabs( AngleDifference( cs->ideal_viewangles[YAW], cs->viewangles[YAW] ) ) < 20 ) ) &&
			 // check for radid luger firing by skilled users (release fire between shots)
			 ( ( ( level.time + cs->entityNum * 500 ) / 2000 ) % 2 || !( rand() % ( 1 + g_gameskill.integer ) ) || ( cs->attributes[ATTACK_SKILL] < 0.5 ) || ( cs->weaponNum != WP_LUGER ) || ( cs->bs->cur_ps.weaponTime == 0 ) || ( cs->bs->cur_ps.releasedFire ) ) ) {
			ucmd->buttons |= BUTTON_ATTACK;
			// do some swaying around for some weapons
			AICast_WeaponSway( cs, ofs );
			VectorAdd( bi->viewangles, ofs, bi->viewangles );
		}
	}
	//
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT( bi->viewangles[PITCH] );
	ucmd->angles[YAW] = ANGLE2SHORT( bi->viewangles[YAW] );
	ucmd->angles[ROLL] = ANGLE2SHORT( bi->viewangles[ROLL] );
	//subtract the delta angles
	for ( j = 0; j < 3; j++ ) {
		temp = ucmd->angles[j] - delta_angles[j];
		ucmd->angles[j] = temp;
	}


//----(SA)	modified slightly for DM/DK
	ucmd->weapon = bi->weapon;
	//
	// relaxed mode show no weapons
	if ( cs->aiState <= AISTATE_QUERY ) {
		if ( WEAPS_ONE_HANDED & ( 1 << ucmd->weapon ) ) { // one-handed wepons don't draw, others do
			ucmd->weapon = WP_NONE;
		}
	}
//----(SA)	end

	//
	if ( bi->actionflags & ACTION_GESTURE ) {
		ucmd->buttons |= BUTTON_GESTURE;
	}
	if ( bi->actionflags & ACTION_RELOAD ) {
		ucmd->wbuttons |= WBUTTON_RELOAD;
	}
	//
	// if we are locked down, don't do anything
	//
	if ( cs->pauseTime > level.time ) {
		return;
	}
	//
	// if scripted pause, no movement
	//
	if ( cs->castScriptStatus.scriptNoMoveTime > level.time ) {
		return;
	}
	//
	// if viewlock, wait until we are facing ideal angles before we move
	if ( cs->aiFlags & AIFL_VIEWLOCKED ) {
		if ( fabs( AngleDifference( cs->ideal_viewangles[YAW], cs->viewangles[YAW] ) ) > 10 ) {
			return;
		}
	}
	//
	if ( bi->actionflags & ACTION_DELAYEDJUMP ) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//
	// only move if we are in combat or we are facing where our ideal angles
	if ( bi->speed ) {
		if ( ( !( cs->aiFlags & AIFL_WALKFORWARD ) && cs->enemyNum >= 0 ) || ( ( ucmd->forwardmove >= 0 ) && fabs( AngleNormalize180( AngleDifference( cs->ideal_viewangles[YAW], cs->viewangles[YAW] ) ) ) < 60 ) ) {
			//NOTE: movement is relative to the REAL view angles
			//get the horizontal forward and right vector
			//get the pitch in the range [-180, 180]
			if ( bi->dir[2] ) {
				angles[PITCH] = bi->viewangles[PITCH];
			} else { angles[PITCH] = 0;}
			angles[YAW] = bi->viewangles[YAW];
			angles[ROLL] = 0;
			AngleVectors( angles, forward, right, up );
			//bot input speed is in the range [0, 400]
			bi->speed = bi->speed * 127 / 400;
			//set the view independent movement
			ucmd->forwardmove = DotProduct( forward, bi->dir ) * bi->speed;
			ucmd->rightmove = DotProduct( right, bi->dir ) * bi->speed;

			// RF, changed this to fix stim soldier flying attack
			if ( !ucmd->upmove ) { // only change it if we don't already have an upmove set
				ucmd->upmove = DotProduct( up, bi->dir ) * bi->speed;
			}
			//if (!ucmd->upmove)	// only change it if we don't already have an upmove set
			//	ucmd->upmove = abs(forward[2]) * bi->dir[2] * bi->speed;
		}
	}
	//
	//normal keyboard movement
	if ( cs->actionFlags & CASTACTION_WALK ) {
		movechar = 70;
	} else {
		movechar = 127;
	}
	if ( bi->actionflags & ACTION_MOVEFORWARD ) {
		ucmd->forwardmove = movechar;
	}
	if ( !( cs->aiFlags & AIFL_WALKFORWARD ) || ( !cs->bs->cur_ps.groundEntityNum || cs->bs->cur_ps.groundEntityNum == ENTITYNUM_NONE ) ) {   // only do other movements if we are allowed to
		if ( bi->actionflags & ACTION_MOVEBACK ) {
			ucmd->forwardmove = -movechar;
		}
		if ( bi->actionflags & ACTION_MOVELEFT ) {
			ucmd->rightmove = -movechar;
		}
		if ( bi->actionflags & ACTION_MOVERIGHT ) {
			ucmd->rightmove = movechar;
		}
	}
	// prevent WALKFORWARD AI from moving backwards
	if ( cs->aiFlags & AIFL_WALKFORWARD ) {
		if ( ucmd->forwardmove < 0 ) {
			ucmd->forwardmove = 0;
		}
	}
	//jump/moveup
	if ( bi->actionflags & ACTION_JUMP ) {
		ucmd->upmove = 127;                                 // JUMP always takes preference over ducking
	}
	if ( bi->actionflags & ACTION_MOVEDOWN ) {
		ucmd->upmove = -127;                                    // JUMP always takes preference over ducking
	}
	if ( bi->actionflags & ACTION_MOVEUP ) {
		ucmd->upmove = 127;                                     // JUMP always takes preference over ducking
	}
	//
	//Com_Printf("forward = %d right = %d up = %d\n", ucmd.forwardmove, ucmd.rightmove, ucmd.upmove);
}


/*
==============
AICast_UpdateInput
==============
*/
void AICast_UpdateInput( cast_state_t *cs, int time ) {
	bot_input_t bi;
	bot_state_t *bs;
	int j;
	float speed;

	bs = cs->bs;

	//add the delta angles to the bot's current view angles
	for ( j = 0; j < 3; j++ ) {
		cs->viewangles[j] = AngleMod( cs->viewangles[j] + SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}
	//
	AICast_ChangeViewAngles( cs, (float) time / 1000 );
	//
	if ( cs->pauseTime > level.time ) {
		trap_EA_View( bs->client, cs->viewangles );
		trap_EA_GetInput( bs->client, (float) time / 1000, &bi );
		AICast_InputToUserCommand( cs, &bi, &cs->lastucmd, bs->cur_ps.delta_angles );
		g_entities[cs->bs->entitynum].client->ps.pm_flags &= ~PMF_RESPAWNED;
		//
		//subtract the delta angles
		for ( j = 0; j < 3; j++ ) {
			cs->viewangles[j] = AngleMod( cs->viewangles[j] - SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
		}
		//
		return;
	}
	//
	trap_EA_GetInput( bs->client, (float) time / 1000, &bi );
	//
	// restrict the speed according to the character and their current speedScale
	// HACK, don't slow down while crouching
	if ( bi.actionflags & ACTION_CROUCH && cs->speedScale < 1.0 ) {
		cs->speedScale = 1.0;
	}
	//
	// check some Cast AI specific movement flags
	if ( cs->actionFlags & CASTACTION_WALK ) {
		if ( cs->speedScale > ( cs->attributes[WALKING_SPEED] / cs->attributes[RUNNING_SPEED] ) ) {
			cs->speedScale = ( cs->attributes[WALKING_SPEED] / cs->attributes[RUNNING_SPEED] );
		}
	}
	// don't ever let the speed get too low
	if ( cs->speedScale < 0.25 ) {
		cs->speedScale = 0.25;
	}
	if ( cs->speedScale > 1.2 ) {
		cs->speedScale = 1.2;
	}
	//
	speed = cs->speedScale * cs->attributes[RUNNING_SPEED];
	//
	//if (speed <= (cs->attributes[WALKING_SPEED] + (cs->attributes[WALKING_SPEED] + 50 < cs->attributes[RUNNING_SPEED] ? 50 : -1)))	// do a fast shuffle if slightly over walking speed
	if ( speed <= cs->attributes[WALKING_SPEED] ) {
		cs->actionFlags |= CASTACTION_WALK;
	}
	//
	// we use 300 here, because the default player speed is 300, so Cast AI's can't move faster than that
	if ( ( bi.speed / 400.0 ) > ( speed / 300.0 ) ) {
		bi.speed = 400.0 * ( speed / 300.0 );
		if ( bi.speed > 400.0 ) {
			bi.speed = 400.0;   // just in case, we should never exceed this
		}
	}
	//
	// do a fast shuffle if slightly over walking speed
	if ( bi.speed <= ( 400.0 / 300.0 ) * ( cs->attributes[WALKING_SPEED] + ( cs->attributes[WALKING_SPEED] + 50 < cs->attributes[RUNNING_SPEED] ? 50 : -1 ) ) ) {
		cs->actionFlags |= CASTACTION_WALK;
	}
	//
	AICast_InputToUserCommand( cs, &bi, &cs->lastucmd, bs->cur_ps.delta_angles );
	//
	// check some Cast AI specific movement flags
	if ( cs->actionFlags & CASTACTION_WALK ) {
		cs->lastucmd.buttons |= BUTTON_WALKING; // play the walking animation
	}
	//
	//subtract the delta angles
	for ( j = 0; j < 3; j++ ) {
		cs->viewangles[j] = AngleMod( cs->viewangles[j] - SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}
	//
	// make sure the respawn flag is disabled (causes problems after multiple "map xxx" commands)
	g_entities[cs->bs->entitynum].client->ps.pm_flags &= ~PMF_RESPAWNED;
	// set the aiState
	g_entities[cs->bs->entitynum].client->ps.aiState = cs->aiState;
}

/*
============
AICast_Think

  entry point for all cast AI
============
*/
void AICast_Think( int client, float thinktime ) {
	gentity_t       *ent;
	cast_state_t    *cs;
	int i;
	int animIndex;
	animation_t     *anim;

//	if (saveGamePending || (strlen( g_missionStats.string ) > 2 )) {
//		return;
//	}

	//
	// get the cast ready for processing
	//
	cs = AICast_GetCastState( client );
	ent = &g_entities[client];
	//
	// make sure we are using the right AAS data for this entity (one's that don't get set will default to the player's AAS data)
	trap_AAS_SetCurrentWorld( cs->aasWorldIndex );
	//
	// make sure we have a valid navigation system
	//
	if ( !trap_AAS_Initialized() ) {
		return;
	}
	//
	trap_EA_ResetInput( client, NULL );
	cs->aiFlags &= ~AIFL_VIEWLOCKED;
	cs->aiFlags &= ~AIFL_SPECIAL_FUNC;
	//cs->weaponNum = ent->client->ps.weapon;
	//
	// turn off flags that are set each frame if needed
	ent->client->ps.eFlags &= ~( EF_NOSWINGANGLES | EF_MONSTER_EFFECT | EF_MONSTER_EFFECT2 | EF_MONSTER_EFFECT3 );
	// conditional flags
	if ( ent->aiCharacter == AICHAR_ZOMBIE ) {
		if ( COM_BitCheck( ent->client->ps.weapons, WP_MONSTER_ATTACK1 ) ) {
			cs->aiFlags |= AIFL_NO_FLAME_DAMAGE;
			SET_FLAMING_ZOMBIE( ent->s, 1 );
		} else {
			SET_FLAMING_ZOMBIE( ent->s, 0 );
		}
	}
	//
	// update bounding box
	AIChar_SetBBox( ent, cs, qtrue );
	//
	// set/disable these each frame as required
	//ent->r.svFlags |= SVF_BROADCAST;
	ent->client->ps.eFlags &= ~EF_FORCE_END_FRAME;
	//origin of the cast
	VectorCopy( ent->client->ps.origin, cs->bs->origin );
	//eye coordinates of the cast
	VectorCopy( ent->client->ps.origin, cs->bs->eye );
	cs->bs->eye[2] += ent->client->ps.viewheight;
	//get the area the cast is in
	cs->bs->areanum = BotPointAreaNum( cs->bs->origin );
	if ( cs->bs->areanum ) {
		cs->lastValidAreaNum[cs->aasWorldIndex] = cs->bs->areanum;
		cs->lastValidAreaTime[cs->aasWorldIndex] = level.time;
	}
	// if we're dead, do special stuff only
	if ( ent->health <= 0 || cs->revivingTime || cs->rebirthTime ) {
		//
		if ( cs->revivingTime && cs->revivingTime < level.time ) {
			// start us thinking again
			ent->client->ps.pm_type = PM_NORMAL;
			cs->revivingTime = 0;
		}
		//
		if ( cs->rebirthTime && cs->rebirthTime < level.time ) {
			vec3_t mins, maxs;
			int touch[10], numTouch;
			float oldmaxZ;

			oldmaxZ = ent->r.maxs[2];

			// make sure the area is clear
			AIChar_SetBBox( ent, cs, qfalse );

			VectorAdd( ent->r.currentOrigin, ent->r.mins, mins );
			VectorAdd( ent->r.currentOrigin, ent->r.maxs, maxs );
			trap_UnlinkEntity( ent );

			numTouch = trap_EntitiesInBox( mins, maxs, touch, 10 );

			if ( numTouch ) {
				for ( i = 0; i < numTouch; i++ ) {
					//if (!g_entities[touch[i]].client || g_entities[touch[i]].r.contents == CONTENTS_BODY)
					if ( g_entities[touch[i]].r.contents & MASK_PLAYERSOLID ) {
						break;
					}
				}
				if ( i == numTouch ) {
					numTouch = 0;
				}
			}

			if ( numTouch == 0 ) {    // ok to spawn

				// give them health when they start reviving, so we won't gib after
				// just a couple shots while reviving
				ent->health =
					ent->client->ps.stats[STAT_HEALTH] =
						ent->client->ps.stats[STAT_MAX_HEALTH] =
							( ( cs->attributes[STARTING_HEALTH] - 50 ) > 30 ? ( cs->attributes[STARTING_HEALTH] - 50 ) : 30 );

				ent->r.contents = CONTENTS_BODY;
				ent->clipmask = MASK_PLAYERSOLID | CONTENTS_MONSTERCLIP;
				ent->takedamage = qtrue;
				ent->waterlevel = 0;
				ent->watertype = 0;
				ent->flags = 0;
				ent->die = AICast_Die;
				ent->client->ps.eFlags &= ~EF_DEAD;
				ent->s.eFlags &= ~EF_DEAD;

				cs->rebirthTime = 0;
				cs->deathTime = 0;

				ent->client->ps.eFlags &= ~EF_DEATH_FRAME;
				ent->client->ps.eFlags &= ~EF_FORCE_END_FRAME;
				ent->client->ps.eFlags |= EF_NO_TURN_ANIM;

				// play the revive animation
				cs->revivingTime = level.time + BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_REVIVE, qfalse, qtrue );;
			} else {
				// can't spawn yet, so set bbox back, and wait
				ent->r.maxs[2] = oldmaxZ;
				ent->client->ps.maxs[2] = ent->r.maxs[2];
			}
			trap_LinkEntity( ent );
		}
		// ZOMBIE should set effect flag if really dead
		if ( cs->aiCharacter == AICHAR_ZOMBIE && !ent->r.contents ) {
			ent->client->ps.eFlags |= EF_MONSTER_EFFECT2;
		}
		//
		if ( ent->health > GIB_HEALTH && cs->deathTime && cs->deathTime < ( level.time - 1000 ) ) {
			//ent->r.svFlags &= ~SVF_BROADCAST;
			if ( !ent->client->ps.torsoTimer && !ent->client->ps.legsTimer ) {
				ent->client->ps.eFlags |= EF_FORCE_END_FRAME;
			}
			// sink?
			if ( cs->deadSinkStartTime ) {
				ent->s.effect3Time = cs->deadSinkStartTime;
				// if they are gone
				if ( cs->deadSinkStartTime + DEAD_SINK_DURATION < level.time ) {
					trap_DropClient( cs->entityNum, "" );
					return;
				}
			}
			// if we've been dead for a while, stop head-checking
			if ( cs->deathTime < ( level.time - 5000 ) ) {
				ent->flags |= FL_NO_HEADCHECK;
			}
		}
		//
		// do some special handling for this character
		AICast_SpecialFunc( cs );
		//
		// no more thinking required
		return;
	}
	//
	// set some anim conditions
	if ( cs->secondDeadTime ) {
		BG_UpdateConditionValue( cs->entityNum, ANIM_COND_SECONDLIFE, qtrue, qfalse );
	} else {
		BG_UpdateConditionValue( cs->entityNum, ANIM_COND_SECONDLIFE, qfalse, qfalse );
	}
	// set health value
	if ( ent->health <= 0.25 * cs->attributes[STARTING_HEALTH] ) {
		BG_UpdateConditionValue( cs->entityNum, ANIM_COND_HEALTH_LEVEL, 2, qfalse );
	} else if ( ent->health <= 0.5 * cs->attributes[STARTING_HEALTH] ) {
		BG_UpdateConditionValue( cs->entityNum, ANIM_COND_HEALTH_LEVEL, 1, qfalse );
	} else {
		BG_UpdateConditionValue( cs->entityNum, ANIM_COND_HEALTH_LEVEL, 0, qfalse );
	}
	// set enemy position
	if ( cs->enemyNum >= 0 ) {
		if ( infront( ent, &g_entities[cs->enemyNum] ) ) {
			BG_UpdateConditionValue( ent->s.number, ANIM_COND_ENEMY_POSITION, POSITION_INFRONT, qtrue );
		} else {
			BG_UpdateConditionValue( ent->s.number, ANIM_COND_ENEMY_POSITION, POSITION_BEHIND, qtrue );
		}
	} else {
		BG_UpdateConditionValue( ent->s.number, ANIM_COND_ENEMY_POSITION, POSITION_UNUSED, qtrue );
	}
	// set defense pose
	if ( ent->flags & FL_DEFENSE_CROUCH ) {
		BG_UpdateConditionValue( ent->s.number, ANIM_COND_DEFENSE, qtrue, qfalse );
	} else {
		BG_UpdateConditionValue( ent->s.number, ANIM_COND_DEFENSE, qfalse, qfalse );
	}
	//
	cs->speedScale = 1.0;           // reset each frame, set if required
	cs->actionFlags = 0;            // FIXME: move this to a Cast AI movement init function!
	//retrieve the current client state
	BotAI_GetClientState( client, &( cs->bs->cur_ps ) );
	//
	// setup movement speeds for the given state
	// walking
	animIndex = BG_GetAnimScriptAnimation( cs->entityNum, ent->client->ps.aiState, ANIM_MT_WALK );
	if ( animIndex >= 0 ) {
		anim = BG_GetAnimationForIndex( cs->entityNum, animIndex );
		cs->attributes[WALKING_SPEED] = anim->moveSpeed;
	}
	// crouching
	animIndex = BG_GetAnimScriptAnimation( cs->entityNum, ent->client->ps.aiState, ANIM_MT_WALKCR );
	if ( animIndex >= 0 ) {
		anim = BG_GetAnimationForIndex( cs->entityNum, animIndex );
		cs->attributes[CROUCHING_SPEED] = anim->moveSpeed;
	}
	// running
	animIndex = BG_GetAnimScriptAnimation( cs->entityNum, ent->client->ps.aiState, ANIM_MT_RUN );
	if ( animIndex >= 0 ) {
		anim = BG_GetAnimationForIndex( cs->entityNum, animIndex );
		cs->attributes[RUNNING_SPEED] = anim->moveSpeed;
	}
	// update crouch speed scale
	ent->client->ps.crouchSpeedScale = cs->attributes[CROUCHING_SPEED] / cs->attributes[RUNNING_SPEED];
	//
	// only enable headlook if we want to this frame
	ent->client->ps.eFlags &= ~EF_HEADLOOK;
	if ( cs->enemyNum >= 0 ) {
		ent->client->ps.eFlags &= ~EF_STAND_IDLE2;  // never use alt idle if fighting
	}
	//
	// check for dead leader
	if ( cs->leaderNum >= 0 && g_entities[cs->leaderNum].health <= 0 ) {
		cs->leaderNum = -1;
	}
	//
#if 0
	// HACK for village2, if they are stuck, find a good position (there is a friendly guy placed inside a table)
	{
		trace_t tr;
		vec3_t org;
		trap_Trace( &tr, cs->bs->cur_ps.origin, cs->bs->cur_ps.mins, cs->bs->cur_ps.maxs, cs->bs->cur_ps.origin, cs->entityNum, CONTENTS_SOLID );
		while ( tr.startsolid ) {
			VectorCopy( cs->bs->cur_ps.origin, org );
			org[0] += 96 * crandom();
			org[1] += 96 * crandom();
			org[2] += 16 * crandom();
			trap_Trace( &tr, org, cs->bs->cur_ps.mins, cs->bs->cur_ps.maxs, org, cs->entityNum, CONTENTS_SOLID );
			G_SetOrigin( &g_entities[cs->entityNum], org );
			VectorCopy( org, g_entities[cs->entityNum].client->ps.origin );
		}
	}
#endif
	//add the delta angles to the cast's current view angles
	for ( i = 0; i < 3; i++ ) {
		cs->viewangles[i] = AngleMod( cs->viewangles[i] + SHORT2ANGLE( cs->bs->cur_ps.delta_angles[i] ) );
	}
	//
	//increase the local time of the cast
	cs->bs->ltime += thinktime;
	//
	cs->bs->thinktime = thinktime;
	// clear flags each frame
	cs->bFlags = 0;
	//
	// check enemy health
	if ( cs->enemyNum >= 0 && g_entities[cs->enemyNum].health <= 0 ) {
		cs->enemyNum = -1;
	}
	//
	// if the previous movetype was temporary, set it back
	if ( cs->movestateType == MSTYPE_TEMPORARY ) {
		cs->movestate = MS_DEFAULT;
		cs->movestateType = MSTYPE_NONE;
	}
	// crouching?
	if (    ( cs->attackcrouch_time >= level.time ) /*&&
			((cs->lastAttackCrouch > level.time - 500) || (cs->thinkFuncChangeTime < level.time - 1000))*/) {
		// if we are not moving, and we are firing, always stand, unless we are allowed to crouch + fire
		if ( ( cs->lastucmd.forwardmove || cs->lastucmd.rightmove ) || ( cs->lastWeaponFired < level.time - 2000 ) || ( cs->aiFlags & AIFL_ATTACK_CROUCH ) ) {
			cs->lastAttackCrouch = level.time;
			trap_EA_Crouch( cs->bs->client );
		}
	}
	//
	//if (cs->enemyNum >= 0) {
	//update the attack inventory values
	AICast_UpdateBattleInventory( cs, cs->enemyNum );
	//}
	//
	// if we don't have ammo for the current weapon, get rid of it
	if ( !( COM_BitCheck( cs->bs->cur_ps.weapons, cs->weaponNum ) ) || !AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum ) ) {
		// select a weapon
		AICast_ChooseWeapon( cs, qfalse );
		// if still no ammo, select a blank weapon
		//if (!AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum )) {
		//	cs->weaponNum = WP_NONE;
		//}
	}
	//
	// in query mode, we do special handling (pause scripting, check for transition to alert/combat, etc)
	if ( cs->aiState == AISTATE_QUERY ) {
		AICast_QueryThink( cs );
	} else if ( cs->pauseTime < level.time )     {
		// do the thinking
		AICast_ProcessAIFunctions( cs, thinktime );
		//
		// make sure the correct weapon is selected
		trap_EA_SelectWeapon( cs->bs->client, cs->weaponNum );
		//
		// process current script if it exists
		cs->castScriptStatusCurrent = cs->castScriptStatus;
		AICast_ScriptRun( cs, qfalse );
		//
		// do some special handling for this character
		AICast_SpecialFunc( cs );
	}
	//
	// any anim playing on legs should prevent turn anims being played in the cgame
	if ( ent->client->ps.legsTimer ) {
		ent->client->ps.eFlags |= EF_NO_TURN_ANIM;
	} else {
		ent->client->ps.eFlags &= ~EF_NO_TURN_ANIM;
	}
	//
	// set special movestate if necessary
	if ( cs->movestateType != MSTYPE_NONE ) {
		switch ( cs->movestate ) {
		case MS_WALK:
			cs->actionFlags |= CASTACTION_WALK;
			break;
		case MS_CROUCH:
			trap_EA_Crouch( cs->entityNum );
			break;
		default:
			break;
		}
	}
	//
	// set our weapon in the old structure, in case it's needed elsewhere (?)
	cs->bs->weaponnum = cs->weaponNum;
	// see if we were recently firing
	if ( cs->lastWeaponFired && cs->lastWeaponFired > level.time - 2000 ) {
		ent->client->ps.eFlags |= EF_RECENTLY_FIRING;
	} else {
		ent->client->ps.eFlags &= ~EF_RECENTLY_FIRING;
	}
	//
	//subtract the delta angles
	for ( i = 0; i < 3; i++ ) {
		cs->viewangles[i] = AngleMod( cs->viewangles[i] - SHORT2ANGLE( cs->bs->cur_ps.delta_angles[i] ) );
	}
}

/*
============
AICast_StartFrame

  Think any clients that need thinking
============
*/
void CopyToBodyQue( gentity_t *ent );

void AICast_StartFrame( int time ) {
	int i, elapsed, count, clCount;
	cast_state_t    *cs;
	int castcount;
	static int lasttime, lastthink;
	static vmCvar_t aicast_disable;
	gentity_t *ent;

	if ( trap_Cvar_VariableIntegerValue( "savegame_loading" ) ) {
		return;
	}

	if ( saveGamePending ) {
		return;
	}

	// if waiting at intermission, don't think
	if ( strlen( g_missionStats.string ) > 1 ) {
		return;
	}

	if ( !aicast_disable.handle ) {
		trap_Cvar_Register( &aicast_disable, "aicast_disable", "0", CVAR_CHEAT );
	} else
	{
		trap_Cvar_Update( &aicast_disable );
		if ( aicast_disable.integer ) {
			return;
		}
	}

	trap_Cvar_Update( &aicast_debug );
	trap_Cvar_Update( &aicast_debugname );
	trap_Cvar_Update( &aicast_scripts );

	// no need to think during the intermission
	if ( level.intermissiontime ) {
		return;
	}
	//
	// make sure the AAS gets updated
	trap_BotLibStartFrame( (float) time / 1000 );
	//
	//
	elapsed = time - lasttime;
	if ( elapsed == 0 ) {
		return;         // no time has elapsed
	}

//G_Printf( "AI startframe: %i\n", time );

	if ( elapsed < 0 ) {
		elapsed = 0;
		lasttime = time;
	}
	// don't let the SIGHTING framerate drop below 10 (too much sighting to process at once)
	if ( elapsed > 100 ) {
		elapsed = 100;
	}
	AICast_SightUpdate( (int)( (float)SIGHT_PER_SEC * ( (float)elapsed / 1000 ) ) );
	//
	// update the player's area, only update if it's valid
	for ( i = 0; i < 2; i++ ) {
		trap_AAS_SetCurrentWorld( i );
		castcount = BotPointAreaNum( g_entities[0].s.pos.trBase );
		if ( castcount ) {
			caststates[0].lastValidAreaNum[i] = castcount;
			caststates[0].lastValidAreaTime[i] = level.time;
		}
	}
	//
	count = 0;
	castcount = 0;
	clCount = 0;
	//
	if ( ++lastthink > level.maxclients ) {
		lastthink = 0;
	}
	//update the AI characters
	for ( i = lastthink, ent = &g_entities[lastthink]; clCount < level.numPlayingClients && count < aicast_maxthink; i++, ent++ )
	{
		if ( i >= level.maxclients ) {
			// rewind back to the start
			i = 0;
			ent = g_entities;
		}
		//
		lastthink = i;
		if ( !ent->inuse ) {
			continue;
		}
		if ( ent->client ) {
			clCount++;
		}
		//
		cs = AICast_GetCastState( i );
		// is this a cast AI?
		if ( cs->bs ) {
			if ( ent->aiInactive == qfalse ) {
				//
				elapsed = time - cs->lastThink;
				//
				// if they're moving/firing think every frame
				if ( ( elapsed && cs->scriptAnimTime && cs->scriptAnimTime >= ( level.time - 1000 ) ) ||
					 (   ( elapsed >= 50 ) &&
						 (   (   (   ( !VectorCompare( ent->client->ps.velocity, vec3_origin ) ) ||
									 ( cs->enemyNum >= 0 ) ||
									 ( cs->aiState >= AISTATE_COMBAT ) ||
									 ( cs->vislist[0].visible_timestamp && cs->vislist[0].visible_timestamp > level.time - 4000 ) ||
									 ( ent->client->buttons ) ||
									 ( elapsed >= aicast_thinktime ) ) /*&&
								(count <= aicast_maxthink)*/) ||
							( elapsed >= aicast_thinktime * 2 ) ) ) ) {
					// make it think now
					AICast_Think( i, (float)elapsed / 1000 );
					// did they drop?
					if ( !cs->bs || !cs->bs->inuse ) {
						break;  // get out of here, to be safe
					}
					cs->lastThink = time + rand() % 20;   // randomize this slightly to spread out thinks during high framerates
					//
					// only count live guys
					if ( ent->health > 0 ) {
						count++;
					}
				}
				// check for any debug info updates
				AICast_DebugFrame( cs );
			} else if ( cs->aiFlags & AIFL_WAITINGTOSPAWN ) {
				// check f the space is clear yet
				ent->AIScript_AlertEntity( ent );
			}
			//
			// see if we've checked all cast AI's
			if ( ++castcount >= numcast ) {
				break;
			}
		}
	}
	//
	lasttime = time;
}

/*
============
AICast_StartServerFrame

  Do movements, sighting, etc
============
*/
void AICast_StartServerFrame( int time ) {
	int i, elapsed, count, clCount, activeCount;
	cast_state_t    *cs;
	int castcount;
	static int lasttime;
	static vmCvar_t aicast_disable;
	gentity_t *ent;
	cast_state_t *pcs;
	qboolean highPriority;
	int oldLegsTimer;

	if ( trap_Cvar_VariableIntegerValue( "savegame_loading" ) ) {
		return;
	}

	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		return;
	}

	if ( saveGamePending ) {
		return;
	}

	// if waiting at intermission, don't think
	if ( strlen( g_missionStats.string ) > 1 ) {
		return;
	}

	if ( !aicast_disable.handle ) {
		trap_Cvar_Register( &aicast_disable, "aicast_disable", "0", CVAR_CHEAT );
	} else
	{
		trap_Cvar_Update( &aicast_disable );
		if ( aicast_disable.integer ) {
			return;
		}
	}

	trap_Cvar_Update( &aicast_debug );

	// no need to think during the intermission
	if ( level.intermissiontime ) {
		return;
	}
	//
	// make sure the AAS gets updated
	trap_BotLibStartFrame( (float) time / 1000 );
	//
	//
	elapsed = time - lasttime;
	if ( elapsed == 0 ) {
		return;         // no time has elapsed
	}

	pcs = AICast_GetCastState( 0 );

//G_Printf( "AI startserverframe: %i\n", time );

	AICast_AgePlayTime( 0 );

	if ( elapsed < 0 ) {
		elapsed = 0;
		lasttime = time;
	}
	// don't let the framerate drop below 10
	if ( elapsed > 100 ) {
		elapsed = 100;
	}
	//
	// process player's current script if it exists
	AICast_ScriptRun( AICast_GetCastState( 0 ), qfalse );
	//
	//AICast_SightUpdate( (int)((float)SIGHT_PER_SEC * ((float)elapsed / 1000)) );
	//
	count = 0;
	castcount = 0;
	clCount = 0;
	activeCount = 0;
	//
	//update the AI characters
	for ( i = 0, ent = g_entities; i < level.maxclients /*&& clCount < level.numPlayingClients*/; i++, ent++ )
	{
		//if (ent->inuse && ent->client)
		//	clCount++;
		//
		cs = AICast_GetCastState( i );
		// is this a cast AI?
		if ( cs->bs ) {
			if ( ent->aiInactive == qfalse && ent->inuse ) {
				//
				elapsed = level.time - cs->lastMoveThink;
				if ( cs->lastThink && elapsed > 0 ) {
					highPriority = qfalse;
					if ( ent->health > 0 ) {
						highPriority = qtrue;
					} else if ( cs->deathTime > ( level.time - 5000 ) ) {
						highPriority = qtrue;
					}
					//
					if ( highPriority ) {
						activeCount++;
					}
					//
					// optimization, if they're not in the player's PVS, and they aren't trying to move, then don't bother thinking
					if (    ( highPriority && ( elapsed > 300 ) )
							||  ( g_entities[0].client && g_entities[0].client->cameraPortal )
							||  ( highPriority && ( cs->vislist[0].visible_timestamp == cs->vislist[0].lastcheck_timestamp ) )
							||  ( highPriority && ( pcs->vislist[cs->entityNum].visible_timestamp == pcs->vislist[cs->entityNum].lastcheck_timestamp ) )
							||  ( VectorLength( ent->client->ps.velocity ) > 0 )
							||  ( highPriority && ( cs->lastucmd.forwardmove || cs->lastucmd.rightmove || cs->lastucmd.upmove > 0 || cs->lastucmd.buttons || cs->lastucmd.wbuttons ) )
							// !!NOTE: always allow thinking if in PVS, otherwise bosses won't gib, and dead guys might not push away from clipped walls
							||  ( trap_InPVS( cs->bs->origin, g_entities[0].s.pos.trBase ) ) ) { // do pvs check last, since it's the most expensive to call
						oldLegsTimer = ent->client->ps.legsTimer;
						//
						// send it's movement commands
						//
						serverTime = time;
						AICast_UpdateInput( cs, elapsed );
						trap_BotUserCommand( cs->bs->client, &( cs->lastucmd ) );
						cs->lastMoveThink = level.time;
						//
						// check for anim changes that may require us to stay still
						//
						if ( oldLegsTimer < ent->client->ps.legsTimer && ent->client->ps.groundEntityNum == ENTITYNUM_WORLD ) {
							// dont move until they are finished
							if ( cs->castScriptStatus.scriptNoMoveTime < level.time + ent->client->ps.legsTimer ) {
								cs->castScriptStatus.scriptNoMoveTime = level.time + ent->client->ps.legsTimer;
							}
						}
					}
				}
			} else {
				trap_UnlinkEntity( ent );
			}
			//
			// see if we've checked all cast AI's
			if ( ++castcount >= numcast ) {
				break;
			}
		}
	}
	//
	lasttime = time;
	//
	if ( aicast_debug.integer == 3 ) {
		G_Printf( "AI Active Count: %i\n", activeCount );
	}
}

/*
==============
AICast_PredictMovement

  Simulates movement over a number of frames, returning the end position
==============
*/
void AICast_PredictMovement( cast_state_t *cs, int numframes, float frametime, aicast_predictmove_t *move, usercmd_t *ucmd, int checkHitEnt ) {
	int frame, i;
	playerState_t ps;
	pmove_t pm;
	trace_t tr;
	vec3_t end, startHitVec, thisHitVec, lastOrg, projPoint;
	qboolean checkReachMarker;
	gentity_t   *ent = &g_entities[cs->entityNum];
	bot_input_t bi;

//int pretime = Sys_MilliSeconds();
//G_Printf("PredictMovement: %f duration, %i frames\n", frametime, numframes );

	if ( cs->bs ) {
		ps = cs->bs->cur_ps;
		trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi );
	} else {
		ps = g_entities[cs->entityNum].client->ps;
	}

	ps.eFlags |= EF_DUMMY_PMOVE;

	move->stopevent = PREDICTSTOP_NONE;

	if ( checkHitEnt >= 0 && !Q_stricmp( g_entities[checkHitEnt].classname, "ai_marker" ) ) {
		checkReachMarker = qtrue;
		VectorSubtract( g_entities[checkHitEnt].r.currentOrigin, ps.origin, startHitVec );
		VectorCopy( ps.origin, lastOrg );
	} else {
		checkReachMarker = qfalse;
	}

	// don't let the frametime be too high
//	while (frametime > 0.2) {
//		numframes *= 2;
//		frametime /= 2;
//	}

	for ( frame = 0; frame < numframes; frame++ )
	{
		memset( &pm, 0, sizeof( pm ) );
		pm.ps = &ps;
		pm.cmd = *ucmd;
		pm.oldcmd = *ucmd;
		pm.ps->commandTime = 0;
		pm.cmd.serverTime = (int)( 1000.0 * frametime );
		pm.tracemask = g_entities[cs->entityNum].clipmask; //MASK_PLAYERSOLID;

		pm.trace = trap_TraceCapsule; //trap_Trace;
		pm.pointcontents = trap_PointContents;
		pm.debugLevel = qfalse;
		pm.noFootsteps = qtrue;
		// RF, not needed for prediction
		//pm.noWeapClips = qtrue;	// (SA) AI's ignore weapon clips

		// perform a pmove
		Pmove( &pm );

		if ( checkHitEnt >= 0 ) {
			// if we've hit the checkent, abort
			if ( checkReachMarker ) {
				VectorSubtract( g_entities[checkHitEnt].r.currentOrigin, pm.ps->origin, thisHitVec );
				if ( DotProduct( startHitVec, thisHitVec ) < 0 ) {
					// project the marker onto the movement vec, and check distance
					ProjectPointOntoVector( g_entities[checkHitEnt].r.currentOrigin, lastOrg, pm.ps->origin, projPoint );
					if ( VectorDistance( g_entities[checkHitEnt].r.currentOrigin, projPoint ) < 8 ) {
						move->stopevent = PREDICTSTOP_HITENT;
						goto done;
					}
				}
				// use this position as the base for the next test
				//VectorCopy( thisHitVec, startHitVec );
				VectorCopy( pm.ps->origin, lastOrg );
			}
			// if we didnt reach the marker, then check for something that blocked us
			for ( i = 0; i < pm.numtouch; i++ ) {
				if ( pm.touchents[i] == pm.ps->groundEntityNum ) {
					continue;
				}
				if ( pm.touchents[i] == checkHitEnt ) {
					move->stopevent = PREDICTSTOP_HITENT;
					goto done;
				} else if ( pm.touchents[i] < MAX_CLIENTS ||
							( pm.touchents[i] != ENTITYNUM_WORLD && ( g_entities[pm.touchents[i]].s.eType != ET_MOVER || g_entities[pm.touchents[i]].moverState != MOVER_POS1 ) ) ) {
					// we have hit another entity, so abort
					move->stopevent = PREDICTSTOP_HITCLIENT;
					goto done;
				} else if ( !Q_stricmp( g_entities[pm.touchents[i]].classname, "script_mover" ) ) {
					// avoid script_mover's
					move->stopevent = PREDICTSTOP_HITCLIENT;
					goto done;
				}
			}
			// if we are trying to get to something, we should keep adjusting our input to head towards them
			if ( cs->bs && checkHitEnt >= 0 ) {
				// keep walking straight to them
				VectorSubtract( g_entities[checkHitEnt].r.currentOrigin, pm.ps->origin, bi.dir );
				if ( !ent->waterlevel ) {
					bi.dir[2] = 0;
				}
				VectorNormalize( bi.dir );
				bi.actionflags = 0;
				bi.speed = 400;
				AICast_InputToUserCommand( cs, &bi, ucmd, ps.delta_angles );
			}
		}
	}

done:

	// hack, if we are above ground, chances are it's because we only did one frame, and gravity isn't applied until
	// after the frame, so try and drop us down some
	if ( move->groundEntityNum == ENTITYNUM_NONE ) {
		VectorCopy( move->endpos, end );
		end[2] -= 32;
		trap_Trace( &tr, move->endpos, pm.mins, pm.maxs, end, pm.ps->clientNum, pm.tracemask );
		if ( !tr.startsolid && !tr.allsolid && tr.fraction < 1 ) {
			VectorCopy( tr.endpos, pm.ps->origin );
			pm.ps->groundEntityNum = tr.entityNum;
		}
	}

	// copy off the results
	VectorCopy( pm.ps->origin, move->endpos );
	move->frames = frame;
	//move->presencetype = cs->bs->presencetype;
	VectorCopy( pm.ps->velocity, move->velocity );
	move->numtouch = pm.numtouch;
	memcpy( move->touchents, pm.touchents, sizeof( pm.touchents ) );
	move->groundEntityNum = pm.ps->groundEntityNum;

//G_Printf("PredictMovement: %i ms\n", -pretime + Sys_MilliSeconds() );
}

/*
============
AICast_GetAvoid
============
*/
qboolean AICast_GetAvoid( cast_state_t *cs, bot_goal_t *goal, vec3_t outpos, qboolean reverse, int blockEnt ) {
	float yaw, oldyaw, distmoved, bestmoved, bestyaw;
	vec3_t bestpos;
	aicast_predictmove_t castmove;
	usercmd_t ucmd;
	qboolean enemyVisible;
	float angleDiff;
	int starttraveltime = 0, besttraveltime, traveltime;         // TTimo: init
	int invert;
	float inc;
	qboolean averting = qfalse;
	float maxYaw, simTime;
	static int lastTime;
	//
	// if we are in the air, no chance of avoiding
	if ( cs->bs->cur_ps.groundEntityNum == ENTITYNUM_NONE && g_entities[cs->entityNum].waterlevel <= 1 ) {
		return qfalse;
	}
	//
	if ( cs->lastAvoid > level.time - rand() % 500 ) {
		return qfalse;
	}
	cs->lastAvoid = level.time + 50 + rand() % 500;
	//
	if ( lastTime == level.time ) {
		return qfalse;
	}
	lastTime = level.time;

	// if they have an enemy, and can currently see them, don't move out of their view
	enemyVisible =  ( cs->enemyNum >= 0 ) &&
				   ( AICast_CheckAttack( cs, cs->enemyNum, qfalse ) );
	//
	// look for a good direction to move out of the way
	bestmoved = 0;
	bestyaw = 360;
	besttraveltime = 9999999;
	if ( goal ) {
		starttraveltime = trap_AAS_AreaTravelTimeToGoalArea( cs->bs->areanum, cs->bs->origin, goal->areanum, cs->travelflags );
	}
	memcpy( &ucmd, &cs->lastucmd, sizeof( usercmd_t ) );
	ucmd.forwardmove = 127;
	ucmd.rightmove = 0;
	ucmd.upmove = 0;
	if ( cs->dangerEntity >= 0 && cs->dangerEntityValidTime >= level.time ) {
		averting = qtrue;
	} else if ( !goal ) {
		averting = qtrue;   // not heading for a goal, so we must be getting out of someone's way
	}
	//
	maxYaw = 0;
	simTime = 1.2;
	//
	if ( averting ) {
		// avoiding danger, go anywhere!
		angleDiff = 300;
		inc = 60;
		invert = 1;
	} else {
		if ( level.time % 1000 < 500 ) {
			invert = 1;
		} else {
			invert = -1;
		}
		angleDiff = 140;
		inc = 35;
	}
	if ( blockEnt > aicast_maxclients ) {
		maxYaw = angleDiff;
		simTime = 0.5;
	}
	//
	for ( yaw = -angleDiff * invert; yaw*invert <= maxYaw; yaw += inc * invert ) {
		if ( !averting && !yaw ) {
			continue;
		}
		oldyaw = cs->bs->cur_ps.viewangles[YAW];
		cs->bs->cur_ps.viewangles[YAW] += yaw + reverse * 180;
		//
		ucmd.angles[YAW] = ANGLE2SHORT( AngleMod( cs->bs->cur_ps.viewangles[YAW] ) );
		//
		AICast_PredictMovement( cs, 5, 0.4, &castmove, &ucmd, -1 );
		// if we have a danger entity, try and get away from it at all costs
		if ( cs->dangerEntity >= 0 && cs->dangerEntityValidTime >= level.time ) {
			distmoved = Distance( castmove.endpos, cs->dangerEntityPos );
		} else if ( goal ) {
			//distmoved = 99999 - trap_AAS_AreaTravelTimeToGoalArea( BotPointAreaNum(castmove.endpos), castmove.endpos, goal->areanum, cs->travelflags );
			distmoved = 99999 - Distance( castmove.endpos, goal->origin );
		} else {
			distmoved = Distance( castmove.endpos, cs->bs->cur_ps.origin );
		}
		if (    ( distmoved > bestmoved )
				//&&	((cs->bs->origin[2] - castmove.endpos[2]) < 64)	// allow up, but not down (falling)
				&&  ( castmove.groundEntityNum != ENTITYNUM_NONE ) ) {
			// they all passed, check any other stuff
			if ( !enemyVisible || AICast_CheckAttackAtPos( cs->entityNum, cs->enemyNum, castmove.endpos, qfalse, qfalse ) ) {
				if ( !goal || ( traveltime = trap_AAS_AreaTravelTimeToGoalArea( BotPointAreaNum( castmove.endpos ), castmove.endpos, goal->areanum, cs->travelflags ) ) < ( starttraveltime + 200 ) ) {
					bestyaw = yaw;
					bestmoved = distmoved;
					besttraveltime = traveltime;
					VectorCopy( castmove.endpos, bestpos );
				}
			}
		}
		//
		cs->bs->cur_ps.viewangles[YAW] = oldyaw;
	}
	//
	if ( bestmoved > 0 ) {
		VectorCopy( bestpos, outpos );
		return qtrue;
	} else {
		return qfalse;
	}

//G_Printf("GetAvoid: %i ms\n", -pretime + Sys_MilliSeconds() );
}

/*
============
AICast_Blocked
============
*/
void AICast_Blocked( cast_state_t *cs, bot_moveresult_t *moveresult, int activate, bot_goal_t *goal ) {
	vec3_t pos, dir;
	aicast_predictmove_t move;
	usercmd_t ucmd;
	bot_input_t bi;
	cast_state_t *ocs;
	int i, blockEnt = -1;
	bot_goal_t ogoal;

	if ( cs->blockedAvoidTime < level.time ) {
		if ( cs->blockedAvoidTime < level.time - 300 ) {
			if ( VectorCompare( cs->bs->cur_ps.velocity, vec3_origin ) && !cs->lastucmd.forwardmove && !cs->lastucmd.rightmove ) {
				// not moving, don't bother checking
				cs->blockedAvoidTime = level.time - 1;
				return;
			}
			// are we going to hit someone soon?
			trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi );
			AICast_InputToUserCommand( cs, &bi, &ucmd, cs->bs->cur_ps.delta_angles );
			AICast_PredictMovement( cs, 1, 0.6, &move, &ucmd, ( goal && goal->entitynum > -1 ) ? goal->entitynum : cs->entityNum );

			// blocked if we hit a client (or non-stationary mover) other than our enemy or goal
			if ( move.stopevent != PREDICTSTOP_HITCLIENT ) {
				// not blocked
				cs->blockedAvoidTime = level.time - 1;
				return;
			}

			// if we stopped passed our goal, ignore it
			if ( goal ) {
				if ( VectorDistance( cs->bs->origin, goal->origin ) < VectorDistance( cs->bs->origin, move.endpos ) ) {
					vec3_t v1, v2;
					VectorSubtract( goal->origin, cs->bs->origin, v1 );
					VectorSubtract( goal->origin, move.endpos, v2 );
					VectorNormalize( v1 );
					VectorNormalize( v2 );
					if ( DotProduct( v1, v2 ) < 0 ) {
						// we went passed the goal, so assume we can reach it
						cs->blockedAvoidTime = level.time - 1;
						return;
					}
				}
			}

			// try and get them to move, in case we can't get around them
			blockEnt = -1;
			for ( i = 0; i < move.numtouch; i++ ) {
				if ( move.touchents[i] >= MAX_CLIENTS ) {
					if ( !Q_stricmp( g_entities[move.touchents[i]].classname, "script_mover" ) ) {
						// avoid script_mover's
						blockEnt = move.touchents[i];
					}
					// if we are close to the impact point, then avoid this entity
					else if ( VectorDistance( cs->bs->origin, move.endpos ) < 10 ) {
						//G_Printf("AI (%s) avoiding %s\n", g_entities[cs->entityNum].aiName, g_entities[move.touchents[i]].classname );
						blockEnt = move.touchents[i];
					}
					continue;
				}
				//
				ocs = AICast_GetCastState( move.touchents[i] );
				if ( !ocs->bs ) {
					blockEnt = move.touchents[i];
				}
				// reject this blocker if we are following or going to them
				else if ( cs->followEntity != ocs->entityNum ) {
					// if they are moving away from us already, let them go
					if ( VectorLength( ocs->bs->cur_ps.velocity ) > 10 ) {
						vec3_t v1, v2;

						VectorSubtract( ocs->bs->origin, cs->bs->origin, v2 );
						VectorNormalize( v2 );
						VectorNormalize2( ocs->bs->cur_ps.velocity, v1 );

						if ( DotProduct( v1, v2 ) > 0.0 ) {
							continue;
						}
					}
					//
					// if they recently were asked to avoid us, then they're probably not listening
					if ( ocs->obstructingTime > level.time - 500 ) {
						blockEnt = move.touchents[i];
					}
					//
					// if they are not avoiding, ignore
					if ( !( ocs->aiFlags & AIFL_NOAVOID ) ) {
						continue;
					}
					//
					// they should avoid us
					if ( ocs->leaderNum >= 0 ) {
						ogoal.entitynum = ocs->leaderNum;
						VectorCopy( g_entities[ocs->leaderNum].r.currentOrigin, ogoal.origin );
						if ( AICast_GetAvoid( ocs, &ogoal, ocs->obstructingPos, qfalse, cs->entityNum ) ) {
							// give them time to move somewhere else
							ocs->obstructingTime = level.time + 1000;
						} else {
							// make sure they don't call GetAvoid() for another few frames to let others avoid also
							ocs->obstructingTime = level.time - 1;
							blockEnt = move.touchents[i];
						}
					} else {
						if ( AICast_GetAvoid( ocs, NULL, ocs->obstructingPos, qfalse, cs->entityNum ) ) {
							// give them time to move somewhere else
							ocs->obstructingTime = level.time + 1000;
						} else {
							// make sure they don't call GetAvoid() for another few frames to let others avoid also
							ocs->obstructingTime = level.time - 1;
							blockEnt = move.touchents[i];
						}
					}
				}
			}

		} else {
			return;
		}

		if ( blockEnt < 0 ) {
			// nothing found to be worth avoding
			cs->blockedAvoidTime = level.time - 1;
			return;
		}

		// something is blocking our path
		if ( g_entities[blockEnt].aiName && g_entities[blockEnt].client && cs->bs
			 && VectorDistance( cs->bs->origin, g_entities[blockEnt].r.currentOrigin ) < 128 ) {
			int oldId = cs->castScriptStatus.scriptId;
			AICast_ScriptEvent( cs, "blocked", g_entities[blockEnt].aiName );
			if ( ( oldId != cs->castScriptStatus.scriptId ) || ( cs->aiFlags & AIFL_DENYACTION ) ) {
				// the script has changed, so assume the scripting is handling the avoidance
				return;
			}
		}

		// avoid geometry and props, but assume clients will get out the way
		if ( /*blockEnt > MAX_CLIENTS &&*/ AICast_GetAvoid( cs, goal, pos, qfalse, blockEnt ) ) {
			VectorSubtract( pos, cs->bs->cur_ps.origin, dir );
			VectorNormalize( dir );
			cs->blockedAvoidYaw = vectoyaw( dir );
			if ( blockEnt >= MAX_CLIENTS ) {
				cs->blockedAvoidTime = level.time + 100 + rand() % 200;
			} else {
				cs->blockedAvoidTime = level.time + 300 + rand() % 400;
			}
		} else {
			cs->blockedAvoidTime = level.time - 1;    // don't look again for another few frames
			return;
		}
	}

	VectorClear( pos );
	pos[YAW] = cs->blockedAvoidYaw;
	AngleVectors( pos, dir, NULL, NULL );

	if ( moveresult->flags & MOVERESULT_ONTOPOFOBSTACLE ) {
		trap_EA_Jump( cs->bs->entitynum );
	}

	trap_EA_Move( cs->bs->entitynum, dir, 200 ); //400);

	vectoangles( dir, cs->ideal_viewangles );
	cs->ideal_viewangles[2] *= 0.5;
}

/*
================
AICast_EvaluatePmove

  Avoidance after the event (leaders instruct AI's to get out the way, AI's instruct other non-moving AI's to get out the way)
================
*/
void AICast_EvaluatePmove( int clientnum, pmove_t *pm ) {
	cast_state_t    *cs, *ocs;
	int i, ent;
	bot_goal_t ogoal;

	//vec3_t pos, dir;
	cs = AICast_GetCastState( clientnum );
	// make sure we are using the right AAS data for this entity (one's that don't get set will default to the player's AAS data)
	trap_AAS_SetCurrentWorld( cs->aasWorldIndex );

	// NOTE: this is only enabled for real clients, so their followers get out of their way
	//if (cs->bs)
	//	return;

	// look through the touchent's to see if we've bumped into something we should avoid, or react to
	for ( i = 0; i < pm->numtouch; i++ )
	{
		// mark the time, so they can deal with the obstruction in their own think functions
		cs->blockedTime = level.time;

		if ( pm->touchents[i] == pm->ps->groundEntityNum ) {
			continue;
		}

		// if they are an AI Cast, inform them of our disposition, and hope that they are reasonable
		// enough to assist us in our desire to move beyond our current position
		if ( pm->touchents[i] < aicast_maxclients ) {
			if ( !AICast_EntityVisible( cs, pm->touchents[i], qtrue ) ) {
				continue;
			}

			// if we are inspecting the body, abort if we touch anything
			if ( cs->bs && cs->enemyNum >= 0 && g_entities[cs->enemyNum].health <= 0 ) {
				cs->enemyNum = -1;
			}

			// anything we touch, should see us
			AICast_UpdateVisibility( &g_entities[pm->touchents[i]], &g_entities[cs->entityNum], qfalse, qtrue );

			ocs = AICast_GetCastState( pm->touchents[i] );
			if (    ( ocs->bs ) &&
					( AICast_SameTeam( cs, ocs->entityNum ) ) &&
					( !( ocs->aiFlags & AIFL_NOAVOID ) ) &&
					( ( ocs->leaderNum == cs->entityNum ) || ( VectorLength( ocs->bs->velocity ) < 5 ) ) &&
					( ocs->obstructingTime < ( level.time + 100 ) ) ) {
				// if they are moving away from us already, let them go
				if ( VectorLength( ocs->bs->cur_ps.velocity ) > 10 ) {
					vec3_t v1, v2;

					VectorSubtract( ocs->bs->origin, g_entities[clientnum].client->ps.velocity, v2 );
					VectorNormalize( v2 );
					VectorNormalize2( ocs->bs->cur_ps.velocity, v1 );

					if ( DotProduct( v1, v2 ) > 0.0 ) {
						continue;
					}
				}
				if ( ocs->leaderNum >= 0 ) {
					VectorCopy( g_entities[ocs->leaderNum].r.currentOrigin, ogoal.origin );
					ogoal.areanum = BotPointAreaNum( ogoal.origin );
					ogoal.entitynum = ocs->leaderNum;
					if ( ocs->bs && AICast_GetAvoid( ocs, &ogoal, ocs->obstructingPos, qfalse, cs->entityNum ) ) { // give them time to move somewhere else
						ocs->obstructingTime = level.time + 1000;
					}
				} else {
					if ( ocs->bs && AICast_GetAvoid( ocs, NULL, ocs->obstructingPos, qfalse, cs->entityNum ) ) { // give them time to move somewhere else
						ocs->obstructingTime = level.time + 1000;
					}
				}
			}
		} else if ( cs->bs ) {
			// if we are blocked by a brush entity, see if we can activate it
			ent = pm->touchents[i];
			if ( g_entities[ent].s.modelindex > 0 && g_entities[ent].s.eType == ET_MOVER ) {
				//find the bsp entity which should be activated in order to remove
				//the blocking entity

				if ( !g_entities[ent].isProp
					 && Q_stricmp( g_entities[ent].classname, "func_static" )
					 && Q_stricmp( g_entities[ent].classname, "func_button" )
					 && Q_stricmp( g_entities[ent].classname, "func_tram" ) ) {
					G_Activate( &g_entities[ent], &g_entities[cs->entityNum] );
				}

			}
		}
	}
}

/*
==============
AICast_RequestCrouchAttack
==============
*/
qboolean AICast_RequestCrouchAttack( cast_state_t *cs, vec3_t org, float time ) {
	if ( cs->attributes[ATTACK_CROUCH] > 0 && AICast_CheckAttackAtPos( cs->entityNum, cs->enemyNum, org, qtrue, qfalse ) ) {
		if ( time ) {
			cs->attackcrouch_time = level.time + (int)( time * 1000 );
		}
		return qtrue;
	}

	return qfalse;
}

/*
==============
AICast_QueryThink
==============
*/
void AICast_QueryThink( cast_state_t *cs ) {
	gentity_t *ent;
	qboolean visible;
	cast_state_t *ocs;
	vec3_t vec;

	ent = &g_entities[cs->entityNum];
	ocs = AICast_GetCastState( cs->enemyNum );

	// never crouch while in this state (by choice anyway)
	cs->attackcrouch_time = 0;

	// look at where we last (thought we) saw them
	VectorSubtract( cs->vislist[cs->enemyNum].visible_pos, cs->bs->origin, vec );
	VectorNormalize( vec );
	vectoangles( vec, cs->ideal_viewangles );

	// are they visible now?
	visible = AICast_VisibleFromPos( cs->bs->origin, cs->entityNum, g_entities[cs->enemyNum].r.currentOrigin, cs->enemyNum, qfalse );

	// make sure we dont process the sighting of this enemy by going into query mode again, without them being visible again after we leave here
	cs->vislist[cs->enemyNum].flags &= ~AIVIS_PROCESS_SIGHTING;

	// look towards where we last saw them
	AICast_AimAtEnemy( cs );

	// if visible and alert time has expired, go POSTAL
	if ( ( cs->queryAlertSightTime < 0 ) || ( ( cs->queryAlertSightTime < level.time ) && visible ) ) {
		if ( !cs->queryAlertSightTime ) {
			// set the "short reaction" condition
			BG_UpdateConditionValue( cs->entityNum, ANIM_COND_SHORT_REACTION, qtrue, qfalse );
		}
		AICast_StateChange( cs, AISTATE_COMBAT );
		BG_UpdateConditionValue( cs->entityNum, ANIM_COND_SHORT_REACTION, qfalse, qfalse );
		AIFunc_BattleStart( cs );
		return;
	}

	// if they've fired since the start of the query mode, go POSTAL
	if ( ocs->lastWeaponFired > cs->queryStartTime ) {
		// set the "short reaction" condition
		BG_UpdateConditionValue( cs->entityNum, ANIM_COND_SHORT_REACTION, qtrue, qfalse );
		AICast_StateChange( cs, AISTATE_COMBAT );
		BG_UpdateConditionValue( cs->entityNum, ANIM_COND_SHORT_REACTION, qfalse, qfalse );
		AIFunc_BattleStart( cs );
		return;
	}

	// if not visible, then kill the Lock On timer
	if ( ( cs->queryAlertSightTime > 0 ) && !visible ) {
		cs->queryAlertSightTime = 0;
	}

	// if the query has expired, go back to relaxed
	if ( !ent->client->ps.legsTimer ) {
		AICast_StateChange( cs, AISTATE_RELAXED );
	}
}

/*
================
AICast_DeadClipWalls
================
*/
void AICast_DeadClipWalls( cast_state_t *cs ) {
/*
	//animation_t *anim;
	orientation_t or;
	vec3_t	src, vel;
	trace_t	tr;

	// get the death animation we are currently playing
	//anim = BG_GetAnimationForIndex( cs->entityNum, (cs->bs->cur_ps.torsoAnim & ~ANIM_TOGGLEBIT) );

	// find the head position
	trap_GetTag( cs->entityNum, "tag_head", &or );
	// move up a tad
	or.origin[2] += 3;

	// trace from the base of our bounding box, to the head
	VectorCopy( cs->bs->origin, src );
	src[2] -= cs->bs->cur_ps.mins[2] + 3;
	trap_Trace( &tr, src, vec3_origin, vec3_origin, or.origin, cs->entityNum, MASK_SOLID );

	// if we hit something, move away from it
	if (!tr.startsolid && !tr.allsolid && tr.fraction < 1.0) {
		VectorScale( tr.plane.normal, 80, vel );
		vel[2] = 0;
		VectorAdd( g_entities[cs->entityNum].client->ps.velocity, vel, g_entities[cs->entityNum].client->ps.velocity );
	}
*/
}

/*
==================
AICast_IdleReload
==================
*/
void AICast_IdleReload( cast_state_t *cs ) {
	if ( AICast_NoReload( cs->entityNum ) ) {
		return;
	}
	if ( cs->noReloadTime >= level.time ) {
		return;
	}
	if ( !( ( cs->bs->cur_ps.ammoclip[BG_FindClipForWeapon( cs->bs->cur_ps.weapon )] < (int)( 0.75 * ammoTable[cs->bs->cur_ps.weapon].maxclip ) ) && cs->bs->cur_ps.ammo[BG_FindAmmoForWeapon( cs->bs->cur_ps.weapon )] ) ) {
		return;
	}
	//
	trap_EA_Reload( cs->entityNum );
}