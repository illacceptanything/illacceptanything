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

/*
 * name:		ai_cast_funcs.c
 *
 * desc:		Wolfenstein AI Character Decision Making
 *
*/


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
This file contains the generic thinking states for the characters.

Different types of movement or behaviour will be represented by
a seperate thinking function, which may or may not pass control
over to a new behaviour function.

If control is passed onto a new function, the string name of the
current function is returned, mostly for debugging purposes.

!!! NOTE: control must not be passed to a new AI func from outside of
this file. A new AI func must only be called from within another AI func.

This gives us the ability to keep all code related to sections of AI
self-contained, so adding new features to the AI will be less likely to
step on other areas of AI.
*/

static int enemies[MAX_CLIENTS], numEnemies;

// this is used to prevent try/abort/try/abort/etc grenade flush behaviour
static int lastGrenadeFlush = 0;

#define AICAST_LEADERDIST_MAX   240     // try and stay at least this close to them when nothing else to do
#define AICAST_LEADERDIST_MIN   64      // get this close if we have a clear line of sight to them

char *AIFunc_BattleChase( cast_state_t *cs );
char *AIFunc_Battle( cast_state_t *cs );

static bot_moveresult_t *moveresult;

/*
============
AIFunc_Restore()

  restores the last aifunc that was backed up
============
*/
char *AIFunc_Restore( cast_state_t *cs ) {
	// if the old aifunc was BattleChase, set it back to Battle, in case we have found a good position
	if ( cs->oldAifunc == AIFunc_BattleChase ) {
		cs->oldAifunc = AIFunc_Battle;
	}
	cs->aifunc = cs->oldAifunc;
	return cs->aifunc( cs );
}

/*
============
AICast_GetRandomViewAngle()
============
*/
float AICast_GetRandomViewAngle( cast_state_t *cs, float tracedist ) {
	int cnt, passent, contents_mask;
	vec3_t vec, dir, start, end;
	trace_t trace;
	float bestdist, bestyaw;

	cnt = 0;
	VectorClear( vec );
	//
	VectorCopy( cs->bs->origin, start );
	start[2] += cs->bs->cur_ps.viewheight;
	//
	passent = cs->entityNum;
	contents_mask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_WATER | CONTENTS_SLIME;
//	contents_mask = CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WATER;
	bestdist = 0;
	bestyaw = 0;
	//
	while ( cnt++ < 4 )
	{
		vec[YAW] = random() * 360.0;
		//
		AngleVectors( vec, dir, NULL, NULL );
		VectorMA( start, tracedist, dir, end );
		//
		trap_Trace( &trace, start, NULL, NULL, end, passent, contents_mask );
		//
		if ( trace.fraction >= 1 ) {
			return vec[YAW];
		} else if ( !bestdist || bestdist < trace.fraction ) {
			bestdist = trace.fraction;
			bestyaw = vec[YAW];
		}
	}
	//
	if ( bestdist ) {
		return bestyaw;
	}
	// just return their current direction
	return cs->ideal_viewangles[YAW];
}

/*
============
AICast_MoveToPos()

  returns a pointer to the moveresult it used to make the move, so we can investigate it
  outside of this function
============
*/
bot_moveresult_t *AICast_MoveToPos( cast_state_t *cs, vec3_t pos, int entnum ) {
	bot_goal_t goal;
	vec3_t /*target,*/ dir;
	static bot_moveresult_t lmoveresult;
	int tfl;
	bot_state_t *bs;
	float dist;

//int pretime = Sys_MilliSeconds();

	moveresult = NULL;

	if ( cs->castScriptStatus.scriptNoMoveTime > level.time ) {
		return NULL;
	}
	if ( cs->pauseTime > level.time ) {
		return NULL;
	}
	//
	bs = cs->bs;
	tfl = cs->travelflags;
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		tfl |= TFL_SLIME;
	}
	//
	//create the chase goal
	memset( &goal, 0, sizeof( goal ) );
	goal.entitynum = entnum;
	if ( entnum >= 0 && entnum < level.maxclients && caststates[entnum].lastValidAreaTime[cs->aasWorldIndex] > level.time - 100 ) {
		goal.areanum = caststates[entnum].lastValidAreaNum[cs->aasWorldIndex];
	} else {
		goal.areanum = BotPointAreaNum( pos );
		if ( entnum >= 0 && entnum < level.maxclients ) {
			if ( !goal.areanum ) {
				// use the last valid area
				goal.areanum = caststates[entnum].lastValidAreaNum[cs->aasWorldIndex];
			}
		}
	}
	VectorCopy( pos, goal.origin );
	VectorSet( goal.mins, -8, -8, -8 );
	VectorSet( goal.maxs, 8, 8, 8 );
	if ( entnum > -1 && entnum == cs->followEntity && !cs->followSlowApproach ) {
		goal.flags |= GFL_NOSLOWAPPROACH;   // just speed right passed it
	}
	//
	// debugging, show the route
	if ( aicast_debug.integer == 2 && ( g_entities[cs->entityNum].aiName && !strcmp( aicast_debugname.string, g_entities[cs->entityNum].aiName ) ) ) {
		trap_AAS_RT_ShowRoute( cs->bs->origin, cs->bs->areanum, goal.areanum );
	}
	//
	//initialize the movement state
	BotSetupForMovement( bs );
	//if this is a slow moving creature, don't use avoidreach
	if ( cs->attributes[RUNNING_SPEED] < 100 ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
	} else if ( !VectorCompare( cs->lastMoveToPosGoalOrg, pos ) ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		VectorCopy( pos, cs->lastMoveToPosGoalOrg );
	}
	//move towards the goal
	if ( !( cs->aiFlags & AIFL_EXPLICIT_ROUTING ) || ( entnum < 0 ) || Q_strcasecmp( g_entities[entnum].classname, "ai_marker" ) ) {
		// use AAS routing
		trap_BotMoveToGoal( &lmoveresult, bs->ms, &goal, tfl );
		//if the movement failed
		if ( lmoveresult.failure ) {

			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", lmoveresult.traveltype);
			// clear all movement
			trap_EA_Move( cs->entityNum, vec3_origin, 0 );

		} else {

			if ( entnum > 0 && goal.areanum && entnum >= 0 && entnum < level.maxclients ) {   // NOTE: dont do this for the player
				// save this destination point
				caststates[entnum].lastValidAreaNum[cs->aasWorldIndex] = goal.areanum;
				caststates[entnum].lastValidAreaTime[cs->aasWorldIndex] = level.time;
			}

			if ( lmoveresult.flags & ( MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
				VectorCopy( lmoveresult.ideal_viewangles, cs->ideal_viewangles );
				VectorCopy( cs->ideal_viewangles, cs->viewlock_viewangles );
				cs->aiFlags |= AIFL_VIEWLOCKED;
			} else if ( !( cs->bFlags & BFL_ATTACKED ) )       { // if we are attacking, don't change angles
				bot_input_t bi;

				trap_EA_GetInput( bs->client, 0.1, &bi );
				if ( VectorLength( lmoveresult.movedir ) < 0.5 ) {
					VectorSubtract( goal.origin, bs->origin, dir );
					vectoangles( dir, cs->ideal_viewangles );
				} else {
					// use our velocity if we are moving
					if ( VectorNormalize2( cs->bs->cur_ps.velocity, dir ) > 1 ) {
						vectoangles( dir, cs->ideal_viewangles );
					} else {
						vectoangles( lmoveresult.movedir, cs->ideal_viewangles );
					}
				}
				cs->ideal_viewangles[2] *= 0.5;
				// look towards our future direction (like looking around a corner as we approach it)
				if ( !( cs->aiFlags & AIFL_WALKFORWARD ) && ( lmoveresult.flags & MOVERESULT_FUTUREVIEW ) ) {
					if ( AngleDifference( cs->ideal_viewangles[1], lmoveresult.ideal_viewangles[1] ) > 45 ) {
						cs->ideal_viewangles[1] -= 45;
					} else if ( AngleDifference( cs->ideal_viewangles[1], lmoveresult.ideal_viewangles[1] ) < -45 ) {
						cs->ideal_viewangles[1] += 45;
					} else {
						cs->ideal_viewangles[1] = lmoveresult.ideal_viewangles[1];
					}
					cs->ideal_viewangles[1] = AngleNormalize360( cs->ideal_viewangles[1] );
					cs->ideal_viewangles[0] = lmoveresult.ideal_viewangles[0];
					cs->ideal_viewangles[0] = 0.5 * AngleNormalize180( cs->ideal_viewangles[0] );
				}
			}

		}

	} else {    // manual routing towards markers

		VectorSubtract( pos, cs->bs->origin, dir );
		if ( ( dist = VectorNormalize( dir ) ) < 64 ) {
			trap_EA_Move( cs->entityNum, dir, 100.0 + 300.0 * ( dist / 64.0 ) );
		} else {
			trap_EA_Move( cs->entityNum, dir, 400 );
		}

		// look towards the marker also
		vectoangles( dir, cs->ideal_viewangles );
		cs->ideal_viewangles[2] *= 0.5;

	}
	// this must go last so we face the direction we avoid move
	AICast_Blocked( cs, &lmoveresult, qfalse, &goal );

//G_Printf("MoveToPos: %i ms\n", -pretime + Sys_MilliSeconds() );
/*
// debug, print movement info
if(0) // (SA) added to hide the print
{
bot_input_t bi;

trap_EA_GetInput(cs->bs->client, (float) level.time / 1000, &bi);
G_Printf("spd: %i\n", (int)bi.speed );
}
*/
	return ( moveresult = &lmoveresult );
}

/*
============
AICast_SpeedScaleForDistance()
============
*/
float AICast_SpeedScaleForDistance( cast_state_t *cs, float startdist, float idealDist ) {
#define PREDICT_TIME_WALK   0.2
#define PREDICT_TIME_CROUCH 0.2
#define PREDICT_TIME_RUN    0.3
	float speed, dist;

	dist = startdist - idealDist;
	if ( dist < 1 ) {
		dist = 1;
	}

	// if walking
	if ( cs->movestate == MS_WALK ) {
		speed = cs->attributes[WALKING_SPEED];
		if ( speed * PREDICT_TIME_WALK > dist ) {
			return 0.2 + 0.8 * ( dist / ( speed * PREDICT_TIME_WALK ) );
		} else {
			return 1.0;
		}
	} else
	// if crouching
	if ( cs->movestate == MS_CROUCH || cs->attackcrouch_time > level.time ) {
		speed = cs->attributes[CROUCHING_SPEED];
		if ( speed * PREDICT_TIME_CROUCH > dist ) {
			return 0.3 + 0.7 * ( dist / ( speed * PREDICT_TIME_CROUCH ) );
		} else {
			return 1.0;
		}
	} else
	// running
	{
		speed = cs->attributes[RUNNING_SPEED];
		if ( speed * PREDICT_TIME_RUN > dist ) {
			return 0.2 + 0.8 * ( dist / ( speed * PREDICT_TIME_RUN ) );
		} else {
			return 1.0;
		}
	}
}

/*
============
AICast_SpecialFunc
============
*/
void AICast_SpecialFunc( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	gentity_t *enemy = NULL;

	if ( cs->enemyNum >= 0 ) {
		enemy = &g_entities[cs->enemyNum];
	}

	switch ( cs->aiCharacter ) {
	case AICHAR_WARZOMBIE:
		// disable defense unless we want it
		ent->flags &= ~FL_DEFENSE_CROUCH;
		// if we are pursuing the player from a distance, use our "crouch moving defense"
		if (    ( enemy )
				&&  ( cs->vislist[cs->enemyNum].real_visible_timestamp > level.time - 5000 )
				&&  ( Distance( cs->bs->origin, enemy->s.pos.trBase ) > 200 )
				&&  ( Distance( cs->bs->origin, enemy->s.pos.trBase ) < 600 )
				&&  ( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE )
				//&&	(infront( ent, enemy ))
				&&  ( infront( enemy, ent ) ) ) {
			// crouch
			trap_EA_Crouch( cs->entityNum );
			// enable defense pose
			ent->flags |= FL_DEFENSE_CROUCH;
		}
		break;
	case AICHAR_HELGA:
		// if she has recently finished a spirit attack, go into charge mode
		if ( ( cs->weaponFireTimes[WP_MONSTER_ATTACK2] && ( cs->weaponFireTimes[WP_MONSTER_ATTACK2] > level.time - 12000 ) ) ||
			 ( cs->weaponFireTimes[WP_MONSTER_ATTACK1] && ( cs->weaponFireTimes[WP_MONSTER_ATTACK1] > level.time - 6000 ) ) ) {
			BG_UpdateConditionValue( cs->entityNum, ANIM_COND_CHARGING, 1, qfalse );
			cs->actionFlags &= ~CASTACTION_WALK;
		} else {    // not charging
			BG_UpdateConditionValue( cs->entityNum, ANIM_COND_CHARGING, 0, qfalse );
		}
		//
		if ( ent->health <= 0 && ent->takedamage ) {
			if ( ent->client->ps.torsoTimer < 500 ) {
				// blow up
				GibEntity( ent, 0 );
				ent->takedamage = qfalse;
				ent->r.contents = 0;
				ent->health = GIB_HEALTH - 1;
			}
		}
		break;
	case AICHAR_HEINRICH:
		if (    ( ent->health <= 0.25 * cs->attributes[STARTING_HEALTH] )
				||  ( cs->weaponFireTimes[WP_MONSTER_ATTACK1] > level.time - 6000 ) // walk for period after attack
				||  ( cs->weaponFireTimes[WP_MONSTER_ATTACK1] % 8000 < 3000 ) ) {   // dont run constantly
			cs->actionFlags |= CASTACTION_WALK;
		} else {    // charging
			cs->actionFlags &= ~CASTACTION_WALK;
		}
		// allow running while attacking
		if ( ent->client->ps.torsoTimer && !ent->client->ps.legsTimer ) {
			cs->actionFlags &= ~CASTACTION_WALK;
		}
		//
		if ( ent->health <= 0 && ent->takedamage ) {
			if ( ent->client->ps.torsoTimer < 500 ) {
				// blow up
				GibEntity( ent, 0 );
				ent->takedamage = qfalse;
				ent->r.contents = 0;
				ent->health = GIB_HEALTH - 1;
			}
			// blow up other warriors left around
			if ( !ent->takedamage || ( ent->count2 < level.time && ent->client->ps.torsoTimer < 4000 ) ) {
				int i;
				gentity_t *trav;
				for ( i = 0, trav = g_entities; i < level.maxclients; i++, trav++ ) {
					if ( !trav->inuse ) {
						continue;
					}
					if ( trav->aiCharacter != AICHAR_WARZOMBIE ) {
						continue;
					}
					if ( trav->aiInactive ) {
						continue;
					}
					if ( trav->health <= 0 ) {
						continue;
					}
					// blow it up, set some delay
					G_Damage( trav, ent, ent, NULL, NULL, 99999, 0, MOD_CRUSH );
					if ( ent->takedamage ) {
						ent->count2 = level.time + 200 + rand() % 1500;
					}
				}
			}
		}
		break;
	case AICHAR_ZOMBIE:
		if ( COM_BitCheck( cs->bs->cur_ps.weapons, WP_MONSTER_ATTACK1 ) ) { // flaming zombie, run
			BG_UpdateConditionValue( cs->entityNum, ANIM_COND_CHARGING, 1, qfalse );
		}
		break;
	}
}

/*
============
AIFunc_Idle()

  The cast AI is standing around, contemplating the meaning of life
============
*/
char *AIFunc_Idle( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];

	// we are in an idle state, looking for something to do

	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// shit??
		}
		// go to a position that cannot be seen from the dangerPos
		cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
		cs->attackcrouch_time = 0;
		return AIFunc_AvoidDangerStart( cs );
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}
	//
	// do we need to go to our leader?
	if ( cs->leaderNum >= 0 && Distance( cs->bs->origin, g_entities[cs->leaderNum].r.currentOrigin ) > MAX_LEADER_DIST ) {
		return AIFunc_ChaseGoalStart( cs, cs->leaderNum, AICAST_LEADERDIST_MAX, qtrue );
	}
	//
	// look for things we should attack
	numEnemies = AICast_ScanForEnemies( cs, enemies );
	if ( numEnemies == -1 ) {     // query mode
		return NULL;
	} else if ( numEnemies == -2 )     { // inspection may be required
		char *retval;
		// TTimo: gcc: suggest () around assignment used as truth value
		if ( ( retval = AIFunc_InspectFriendlyStart( cs, enemies[0] ) ) ) {
			return retval;
		}
	} else if ( numEnemies == -3 )     { // bullet impact
		if ( cs->aiState < AISTATE_COMBAT ) {
			return AIFunc_InspectBulletImpactStart( cs );
		}
	} else if ( numEnemies == -4 )     { // audible event
		if ( cs->aiState < AISTATE_COMBAT ) {
			return AIFunc_InspectAudibleEventStart( cs, cs->audibleEventEnt );
		}
	} else if ( numEnemies > 0 )     {
		int i;

		cs->enemyNum = -1;
		// choose an enemy
		for ( i = 0; i < numEnemies; i++ ) {
			if ( Distance( cs->bs->origin, cs->vislist[enemies[i]].visible_pos ) > 16 ) { // if we are really close to the last place we saw them, no point trying to attack, since we'll just end up back here
				if ( cs->enemyNum < 0 ) {
					cs->enemyNum = enemies[i];
				} else if ( AICast_CheckAttack( cs, enemies[i], qfalse ) ) {
					cs->enemyNum = enemies[i];
					return AIFunc_BattleStart( cs );
				}
			}
		}
		if ( cs->enemyNum >= 0 ) {
			if ( ( ( cs->leaderNum < 0 ) || ( cs->thinkFuncChangeTime < level.time - 3000 ) ) && AICast_WantsToChase( cs ) ) {  // don't leave our leader as soon as we get to them
				return AIFunc_BattleStart( cs );
			} else if ( AICast_EntityVisible( AICast_GetCastState( cs->enemyNum ), cs->entityNum, qtrue ) || AICast_CheckAttack( AICast_GetCastState( cs->enemyNum ), cs->entityNum, qfalse ) ) {
				// if we are tactical enough, look for a hiding spot
				if ( !( cs->leaderNum >= 0 ) && cs->attributes[TACTICAL] > 0.4 && cs->attributes[AGGRESSION] < 1.0 ) {
					// they can see us, and we want to hide from them
					if ( AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].visible_pos, cs->takeCoverPos ) ) {
						// go to a position that cannot be seen from the last place we saw the enemy, and wait there for some time
						cs->takeCoverTime = level.time + 2000 + rand() % 3000;
						return AIFunc_BattleTakeCoverStart( cs );
					}
				}
				// attack them if nothing else to do, since they can attack us here
				return AIFunc_BattleStart( cs );
			} else if ( cs->leaderNum < 0 ) {     // we should pursue if no leader, and not wanting to hide
				return AIFunc_BattleStart( cs );
			} else {
				// they can't see us anyway, so ignore them
				cs->lastEnemy = cs->enemyNum;   // at least face them if they come to get us
				cs->enemyNum = -1;
				// crouching makes us look like we are hiding, which is what we are doing
				if ( cs->attributes[ATTACK_CROUCH] > 0.5 ) {
					cs->attackcrouch_time = level.time + 1000;
				}
			}
		}
	}
	//
	// if we are in combat mode, then we should relax, since we dont have an enemy
	if ( cs->aiState >= AISTATE_COMBAT ) {
		AICast_StateChange( cs, AISTATE_ALERT );
	}
	//
	// if we couldn't find anything, see if our previous enemy is still around, if so, go find them
	// this is an attempt to prevent guys from running away to hide from something, never to
	// be seen again. They shouldn't really "forget" that they are indeed soldiers.
	if ( !( cs->leaderNum >= 0 ) && cs->lastEnemy >= 0 && g_entities[cs->lastEnemy].health > 0 && cs->vislist[cs->lastEnemy].real_visible_timestamp < level.time - 5000 &&
		 cs->takeCoverTime < level.time - 5000 ) {
		cs->enemyNum = cs->lastEnemy;   // just go to the place we last saw them
		return AIFunc_BattleStart( cs );
	}
	//
	// if we've recently been in a fight, keep looking around, so we don't look stupid
	if ( cs->lastEnemy >= 0 ) {
		// we have been in a battle, so face our enemy in anticipation of their return
		if ( ent->aiTeam != AITEAM_ALLIES ) {
			vec3_t dir;
			//
			//VectorSubtract( cs->vislist[cs->lastEnemy].visible_pos, cs->bs->origin, dir );
			// hack, use their real angles, prevent them from looking dumb when the player returns
			VectorSubtract( g_entities[cs->lastEnemy].s.pos.trBase, cs->bs->origin, dir );
			if ( VectorLength( dir ) < 1 ) {
				cs->ideal_viewangles[PITCH] = 0;
			} else {
				VectorNormalize( dir );
				vectoangles( dir, cs->ideal_viewangles );
				cs->ideal_viewangles[PITCH] = AngleNormalize180( cs->ideal_viewangles[PITCH] ) * 0.5;
			}
		} else if ( cs->attributes[TACTICAL] && cs->nextIdleAngleChange < level.time )     {
			// wait a second before changing again
			if ( ( cs->nextIdleAngleChange + 3000 ) < level.time ) {

				// FIXME: This could be changed to use some AAS sampling, which would:
				//
				//	Given a src area, pick a random dest area which is visible from that area
				//	and return it's position, which we'd then use to set the next view angles
				//
				//	This would result in more efficient, more realistic behaviour, since they'd
				//	also use PITCH angles to look at areas above/below them

				cs->idleYaw = AICast_GetRandomViewAngle( cs, 512 );

				if ( abs( AngleDelta( cs->idleYaw, cs->ideal_viewangles[YAW] ) ) < 45 ) {
					cs->nextIdleAngleChange = level.time + 1000 + rand() % 2500;
				} else { // do really fast
					cs->nextIdleAngleChange = level.time + 500;
				}

				// adjust with time
				cs->idleYawChange = AngleDelta( cs->idleYaw, cs->ideal_viewangles[YAW] );
				/// ((float)(cs->nextIdleAngleChange - level.time) / 1000.0);

				cs->ideal_viewangles[PITCH] = 0;
			}
		} else if ( cs->idleYawChange )     {
			cs->idleYawChange = AngleDelta( cs->idleYaw, cs->ideal_viewangles[YAW] );
			cs->ideal_viewangles[YAW] = AngleMod( cs->ideal_viewangles[YAW] + ( cs->idleYawChange * cs->bs->thinktime ) );
		}
	}

	// check for a movement we should be making
	if ( cs->obstructingTime > level.time ) {
		AICast_MoveToPos( cs, cs->obstructingPos, -1 );
		if ( cs->movestate != MS_CROUCH ) {
			cs->movestate = MS_WALK;
		}
		cs->movestateType = MSTYPE_TEMPORARY;
	}

	// set head look flag if no enemy
	if ( cs->enemyNum < 0 && cs->attributes[TACTICAL] >= 0.5 && !( cs->aiFlags & AIFL_NO_HEADLOOK ) ) {
		g_entities[cs->entityNum].client->ps.eFlags |= EF_HEADLOOK;
	}

	// reload?
	AICast_IdleReload( cs );

	return NULL;
}

/*
============
AIFunc_IdleStart()
============
*/
char *AIFunc_IdleStart( cast_state_t *cs ) {
	g_entities[cs->entityNum].flags &= ~FL_AI_GRENADE_KICK;
	// stop following
	cs->followEntity = -1;
	// if our enemy has just died, inspect the body
	if ( cs->enemyNum >= 0 ) {
		if ( g_entities[cs->entityNum].aiTeam == AITEAM_NAZI && g_entities[cs->enemyNum].health <= 0 ) {
			return AIFunc_InspectBodyStart( cs );
		} else {
			cs->enemyNum = -1;
		}
	}
	// make sure we don't avoid any areas when we start again
	trap_BotInitAvoidReach( cs->bs->ms );

	// randomly choose idle animation
//----(SA)	try always using the 'casual' stand on spawn and change to crouching one when 'alerted'
	if ( cs->aiFlags & AIFL_STAND_IDLE2 ) {
//		if (rand()%2 || (cs->lastEnemy < 0 && cs->aiFlags & AIFL_TALKING))
		g_entities[cs->entityNum].client->ps.eFlags |= EF_STAND_IDLE2;
//		else
//			g_entities[cs->entityNum].client->ps.eFlags &= ~EF_STAND_IDLE2;
	}

	cs->aifunc = AIFunc_Idle;
	return "AIFunc_Idle";
}

/*
============
AIFunc_InspectFriendly()
============
*/
char *AIFunc_InspectFriendly( cast_state_t *cs ) {
	gentity_t   *followent, *ent;
	bot_state_t *bs;
	vec3_t destorg;
	float dist;
	qboolean moved = qfalse;

	ent = &g_entities[cs->entityNum];

	// if we have an enemy, attack now!
	if ( cs->enemyNum >= 0 ) {
		return AIFunc_BattleStart( cs );
	}

	cs->followEntity = cs->inspectNum;
	cs->followDist = 64;

	cs->scriptPauseTime = level.time + 4000;    // wait for at least this long before resuming any scripted walking, etc

	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// go to a position that cannot be seen from the dangerPos
			cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
			cs->attackcrouch_time = 0;
			cs->castScriptStatus.scriptGotoId = -1;
			cs->movestate = MS_DEFAULT;
			cs->movestateType = MSTYPE_NONE;
			return AIFunc_AvoidDangerStart( cs );
		}
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}

	followent = &g_entities[cs->followEntity];

	// if the entity is not ready yet
	if ( !followent->inuse ) {
		// if it's a connecting client, wait
		if (    cs->followEntity < MAX_CLIENTS
				&&  (   ( followent->client && followent->client->pers.connected == CON_CONNECTING )
						|| ( level.time < 3000 ) ) ) {
			return AIFunc_ChaseGoalIdleStart( cs, cs->followEntity, cs->followDist );
		} else    // stop following it
		{
			AICast_EndChase( cs );
			return AIFunc_IdleStart( cs );
		}
	}

	if ( followent->client ) {
		VectorCopy( followent->client->ps.origin, destorg );
	} else {
		VectorCopy( followent->r.currentOrigin, destorg );
	}

	// they are ready, are they inside range? FIXME: make configurable
	dist = Distance( destorg, cs->bs->origin );
	if ( !( dist < cs->followDist && ( ent->waterlevel || ( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE ) ) ) ) {
		//
		// go to them
		//
		bs = cs->bs;

		// set this flag so we know when we;ve just reached them
		cs->aiFlags |= AIFL_MISCFLAG1;

		// move straight to them if we can
		if (    !moved &&
				( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE || g_entities[cs->entityNum].waterlevel > 1 ) ) {
			aicast_predictmove_t move;
			vec3_t dir;
			bot_input_t bi;
			usercmd_t ucmd;
			trace_t tr;
			qboolean simTest = qfalse;

			if ( cs->attributes[RUNNING_SPEED] < 120 ) {
				simTest = qtrue;
			}

			if ( !simTest ) {
				// trace will eliminate most unsuccessful paths
				trap_Trace( &tr, cs->bs->origin, NULL /*g_entities[cs->entityNum].r.mins*/, NULL /*g_entities[cs->entityNum].r.maxs*/, followent->r.currentOrigin, cs->entityNum, g_entities[cs->entityNum].clipmask );
				if ( tr.entityNum == cs->followEntity || tr.fraction == 1 ) {
					simTest = qtrue;
				}
			}

			if ( simTest ) {
				// try walking straight to them
				VectorSubtract( followent->r.currentOrigin, cs->bs->origin, dir );
				VectorNormalize( dir );
				if ( !ent->waterlevel ) {
					dir[2] = 0;
				}
				//trap_EA_Move(cs->entityNum, dir, 400);
				trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi );
				VectorCopy( dir, bi.dir );
				bi.speed = 400;
				bi.actionflags = 0;
				AICast_InputToUserCommand( cs, &bi, &ucmd, bs->cur_ps.delta_angles );
				AICast_PredictMovement( cs, 10, 0.8, &move, &ucmd, cs->followEntity );

				if ( move.stopevent == PREDICTSTOP_HITENT ) { // success!
					trap_EA_Move( cs->entityNum, dir, 400 );  // set the movement
					vectoangles( dir, cs->ideal_viewangles );
					cs->ideal_viewangles[2] *= 0.5;
					moved = qtrue;
				} else {    // clear movement
					//trap_EA_Move(cs->entityNum, dir, 0);
				}
			}
		}
		//
		if ( !moved ) {
			// use AAS routing
			moveresult = AICast_MoveToPos( cs, followent->r.currentOrigin, cs->followEntity );
			// if we cant get there, face the path to the enemy
			if ( !moveresult || moveresult->failure ) {
				// if we can get a visible target, then face it
				if ( !( cs->aiFlags & AIFL_MISCFLAG2 ) ) {
					if ( trap_AAS_GetRouteFirstVisPos( followent->r.currentOrigin, cs->bs->origin, cs->travelflags, cs->takeCoverEnemyPos ) ) {
						cs->aiFlags |= AIFL_MISCFLAG2;
					} else {
						// if it failed, just use their origin for now, but keep checking
						VectorCopy( followent->r.currentOrigin, cs->takeCoverEnemyPos );
					}
				}
				VectorSubtract( cs->takeCoverEnemyPos, cs->bs->origin, destorg );
				VectorNormalize( destorg );
				vectoangles( destorg, cs->ideal_viewangles );
			}
		}

		// should we slow down?
		if ( cs->followDist && cs->followSlowApproach ) {
			cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, cs->followDist );
		}
/*
		// check for a movement we should be making
		if (cs->obstructingTime > level.time)
		{
			AICast_MoveToPos( cs, cs->obstructingPos, -1 );
			if (cs->movestate != MS_CROUCH) {
				cs->movestate = MS_WALK;
			}
			cs->movestateType = MSTYPE_TEMPORARY;
		}
*/
	} else if ( cs->aiFlags & AIFL_MISCFLAG1 ) {
		cs->aiFlags &= ~AIFL_MISCFLAG1;
		if ( g_entities[cs->inspectNum].health <= 0 ) {
			// call a script event
			cs->aiFlags &= ~AIFL_DENYACTION;
			AICast_ForceScriptEvent( cs, "inspectbodyend", g_entities[cs->inspectNum].aiName );
			if ( cs->aiFlags & AIFL_DENYACTION ) {
				// relinguish control back to scripting
				return AIFunc_DefaultStart( cs );
			}
		} else {    // force a visibility update so we get their vis also
			AICast_UpdateVisibility( ent, &g_entities[cs->inspectNum], qtrue, qtrue );
		}
	}

	{
		int numEnemies;
		//
		// look for things we should attack
		numEnemies = AICast_ScanForEnemies( cs, enemies );
		if ( numEnemies == -1 ) { // query mode
			return NULL;
		} else if ( numEnemies == -2 )     { // inspection
			// only override current objective if we are inspecting a dead guy, and the new inspect target is fighting someone
			if ( ( g_entities[cs->inspectNum].health <= 0 ) && ( g_entities[enemies[0]].health > 0 ) ) {
				return AIFunc_InspectFriendlyStart( cs, enemies[0] );
			}
		}
		// RF, disabled this, if we are interrupted, scripts might not work right, and anyway, this is only a bullet, not as if it's a dead guy or anything
		//else if (numEnemies == -3)	// bullet impact
		//{
		//	if (cs->aiState < AISTATE_COMBAT) {
		//		return AIFunc_InspectBulletImpactStart( cs );
		//	}
		//}
		else if ( numEnemies > 0 ) {
			int i;

			cs->enemyNum = enemies[0];  // just attack the first one
			// override with a visible enemy
			for ( i = 1; i < numEnemies; i++ ) {
				if ( AICast_CheckAttack( cs, enemies[i], qfalse ) ) {
					cs->enemyNum = enemies[i];
					break;
				} else if ( cs->enemyNum < 0 ) {
					cs->lastEnemy = enemies[i];
				}
			}

			return AIFunc_BattleStart( cs );
		}
	}

	if ( cs->nextIdleAngleChange < level.time ) {
		// wait a second before changing again
		if ( ( cs->nextIdleAngleChange + 3000 ) < level.time ) {

			// FIXME: This could be changed to use some AAS sampling, which would:
			//
			//	Given a src area, pick a random dest area which is visible from that area
			//	and return it's position, which we'd then use to set the next view angles
			//
			//	This would result in more efficient, more realistic behaviour, since they'd
			//	also use PITCH angles to look at areas above/below them

			cs->idleYaw = AICast_GetRandomViewAngle( cs, 512 );

			if ( abs( AngleDelta( cs->idleYaw, cs->ideal_viewangles[YAW] ) ) < 45 ) {
				cs->nextIdleAngleChange = level.time + 1000 + rand() % 2500;
			} else { // do really fast
				cs->nextIdleAngleChange = level.time + 500;
			}

			// adjust with time
			cs->idleYawChange = AngleDelta( cs->idleYaw, cs->ideal_viewangles[YAW] );
			/// ((float)(cs->nextIdleAngleChange - level.time) / 1000.0);

			cs->ideal_viewangles[PITCH] = 0;
		}
	} else if ( cs->idleYawChange )     {
		cs->idleYawChange = AngleDelta( cs->idleYaw, cs->ideal_viewangles[YAW] );
		cs->ideal_viewangles[YAW] = AngleMod( cs->ideal_viewangles[YAW] + ( cs->idleYawChange * cs->bs->thinktime ) );
	}

	// set head look flag if no enemy
	if ( cs->enemyNum < 0 && cs->attributes[TACTICAL] >= 0.5 && !( cs->aiFlags & AIFL_NO_HEADLOOK ) ) {
		g_entities[cs->entityNum].client->ps.eFlags |= EF_HEADLOOK;
	}

	// reload?
	AICast_IdleReload( cs );

	return NULL;
}

/*
============
AIFunc_InspectFriendlyStart
============
*/
char *AIFunc_InspectFriendlyStart( cast_state_t *cs, int entnum ) {
	cast_state_t *ocs;

	ocs = AICast_GetCastState( entnum );

	// we are about to deal with the request for inspection
	cs->vislist[entnum].flags &= ~AIVIS_INSPECT;
	cs->scriptPauseTime = level.time + 4000;    // wait for at least this long before resuming any scripted walking, etc

	//
	cs->aiFlags &= ~AIFL_MISCFLAG2;

	if ( ocs->aiState >= AISTATE_COMBAT || g_entities[entnum].health <= 0 ) {
		// mark this character as having been inspected
		cs->vislist[entnum].flags |= AIVIS_INSPECTED;
	}

	// what should we do? wait here? hide? go see them?

	// if dead, go see them
	if ( g_entities[entnum].health <= 0 ) {
		cs->inspectNum = entnum;
		cs->aifunc = AIFunc_InspectFriendly;
		return "AIFunc_InspectFriendlyStart";
	}

	// not dead, so call scripting event
	AICast_ForceScriptEvent( cs, "inspectfriendlycombatstart", g_entities[entnum].aiName );
	if ( cs->aiFlags & AIFL_DENYACTION ) {
		// ignore this friendly forever and ever amen
		cs->vislist[entnum].flags |= AIVIS_INSPECTED;
		return NULL;
	}

	// if they are in combat, then act according to aggressiveness
	if ( ocs->aiState >= AISTATE_COMBAT ) {
		if ( cs->attributes[AGGRESSION] < 0.3 ) {
			if ( AICast_GetTakeCoverPos( cs, entnum, g_entities[entnum].client->ps.origin, cs->takeCoverPos ) ) {
				cs->takeCoverTime = level.time + 10000; // hide for 10 seconds
				cs->scriptPauseTime = cs->takeCoverTime;
				// crouch there if possible
				if ( cs->attributes[ATTACK_CROUCH] > 0.1 ) {
					cs->attackcrouch_time = level.time + 3000;
				}
				return AIFunc_BattleTakeCoverStart( cs );
			}
		}
	}

	// if still around, then we need to go to them
	cs->inspectNum = entnum;
	cs->aifunc = AIFunc_InspectFriendly;
	return "AIFunc_InspectFriendly";
}

/*
============
AIFunc_InspectBulletImpact()
============
*/
char *AIFunc_InspectBulletImpact( cast_state_t *cs ) {
	gentity_t *ent;
	vec3_t v1;
	gclient_t *client;
	//
	client = &level.clients[cs->entityNum];
	//
	ent = &g_entities[cs->entityNum];
	//
	cs->bulletImpactIgnoreTime = level.time + 800;
	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// go to a position that cannot be seen from the dangerPos
			cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
			cs->attackcrouch_time = 0;
			cs->castScriptStatus.scriptGotoId = -1;
			cs->movestate = MS_DEFAULT;
			cs->movestateType = MSTYPE_NONE;
			return AIFunc_AvoidDangerStart( cs );
		}
	}
	// wait until we are looking at the impact
	if ( cs->aiFlags & AIFL_MISCFLAG2 ) {
		// pause any scripting
		cs->scriptPauseTime = level.time + 1000;
		// look at bullet impact
		VectorSubtract( cs->bulletImpactEnd, cs->bs->origin, v1 );
		VectorNormalize( v1 );
		vectoangles( v1, cs->ideal_viewangles );
		//
		// if we are facing that direction, we've looked at the impact point
		if ( fabs( cs->ideal_viewangles[YAW] - cs->viewangles[YAW] ) < 1 ) {
			cs->aiFlags &= ~AIFL_MISCFLAG2;
		}
		return NULL;
	} else if ( cs->aiFlags & AIFL_MISCFLAG1 ) {
		// clear the flag now
		cs->aiFlags &= ~AIFL_MISCFLAG1;
		// start looking back at bullet
		VectorSubtract( cs->bulletImpactStart, cs->bs->origin, v1 );
		VectorNormalize( v1 );
		vectoangles( v1, cs->ideal_viewangles );
		if ( cs->aiState < AISTATE_ALERT ) {
			// change to alert state
			if ( !AICast_StateChange( cs, AISTATE_ALERT ) ) {
				if ( cs->lastEnemy < 0 && cs->enemyNum < 0 ) {
					// look back at our original angles
					VectorCopy( ent->s.angles, cs->ideal_viewangles );
				}
				// stop doing whatever we are doing, and return control to scripting
				cs->scriptPauseTime = 0;
				return AIFunc_IdleStart( cs );
			}
			// make sure we didnt change thinkfunc
			if ( cs->aifunc != AIFunc_InspectBulletImpact ) {
				//G_Error( "scripting passed control out of AIFunc_InspectBulletImpact(), this is bad" );
				return NULL;
			}
		}
		// pause any scripting
		if ( ent->client->ps.legsTimer ) {
			cs->scriptPauseTime = level.time + ent->client->ps.legsTimer;
		} else { // just wait for a few seconds looking at the source
			cs->scriptPauseTime = level.time + 3500;
		}
	}
	// are we done?
	if ( cs->scriptPauseTime < level.time ) {
		if ( cs->lastEnemy < 0 && cs->enemyNum < 0 ) {
			// look back at our original angles
			VectorCopy( ent->s.angles, cs->ideal_viewangles );
		}
		return AIFunc_IdleStart( cs );
	}
	//
	// reload?
	AICast_IdleReload( cs );
	//
	// check for enemies
	{
		int numEnemies;
		//
		// look for things we should attack
		numEnemies = AICast_ScanForEnemies( cs, enemies );
		if ( numEnemies == -2 ) { // inspection
			// only override current objective if we are inspecting a dead guy, and the new inspect target is fighting someone
			if ( ( g_entities[cs->inspectNum].health <= 0 ) && ( g_entities[enemies[0]].health > 0 ) ) {

				return AIFunc_InspectFriendlyStart( cs, enemies[0] );
			}
		} else if ( numEnemies > 0 )     {
			int i;

			cs->enemyNum = enemies[0];  // just attack the first one
			// override with a visible enemy
			for ( i = 1; i < numEnemies; i++ ) {
				if ( AICast_CheckAttack( cs, enemies[i], qfalse ) ) {
					cs->enemyNum = enemies[i];
					break;
				} else if ( cs->enemyNum < 0 ) {
					cs->lastEnemy = enemies[i];
				}
			}

			return AIFunc_BattleStart( cs );
		}
	}
	//
	return NULL;
}

/*
============
AIFunc_InspectBulletImpactStart()
============
*/
char *AIFunc_InspectBulletImpactStart( cast_state_t *cs ) {
	int oldScriptIndex;
	// set the impact timer so we ignore bullets while inspecting this one
	cs->bulletImpactIgnoreTime = level.time + 5000;
	// pause any scripting
	cs->scriptPauseTime = level.time + 1000;
	// set this so we know if we've started the trace back to the bullet origin
	cs->aiFlags |= AIFL_MISCFLAG1;
	cs->aiFlags |= AIFL_MISCFLAG2;
	//
	// call the script event
	oldScriptIndex = cs->scriptCallIndex;
	AICast_ScriptEvent( cs, "bulletimpactsound", "" );
	if ( oldScriptIndex == cs->scriptCallIndex ) {
		// no script event, so call the animation script
		BG_AnimScriptEvent( &g_entities[cs->entityNum].client->ps, ANIM_ET_BULLETIMPACT, qfalse, qtrue );
	}
	//
	// if the origin is not visible, set the bullet origin to the closest visible area from the src
	if ( !trap_InPVS( cs->bulletImpactStart, cs->bs->origin ) ) {
		// if it fails, then just look at the source
		trap_AAS_GetRouteFirstVisPos( g_entities[cs->bulletImpactEntity].s.pos.trBase, cs->bs->origin, cs->travelflags, cs->bulletImpactStart );
	}
	//
	cs->aifunc = AIFunc_InspectBulletImpact;
	return "AIFunc_InspectBulletImpact";
}

/*
============
AIFunc_InspectAudibleEvent()
============
*/
char *AIFunc_InspectAudibleEvent( cast_state_t *cs ) {
	gentity_t   *ent;
	bot_state_t *bs;
	vec3_t destorg, vec;
	float dist;
	qboolean moved = qfalse;

	ent = &g_entities[cs->entityNum];

	// if we have an enemy, attack now!
	if ( cs->enemyNum >= 0 ) {
		return AIFunc_BattleStart( cs );
	}

	cs->followDist = 64;

	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// go to a position that cannot be seen from the dangerPos
			cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
			cs->attackcrouch_time = 0;
			cs->castScriptStatus.scriptGotoId = -1;
			cs->movestate = MS_DEFAULT;
			cs->movestateType = MSTYPE_NONE;
			return AIFunc_AvoidDangerStart( cs );
		}
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}

	// are we just looking for now?
	if ( cs->aiFlags & AIFL_MISCFLAG2 ) {
		if ( cs->scriptPauseTime <= level.time ) {
			return AIFunc_DefaultStart( cs );
		}
		return NULL;
	}

	VectorCopy( cs->audibleEventOrg, destorg );

	// they are ready, are they inside range? FIXME: make configurable
	dist = Distance( destorg, cs->bs->origin );
	if ( !( dist < cs->followDist && ( ent->waterlevel || ( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE ) ) ) ) {
		//
		// go to them
		//
		bs = cs->bs;

		// set this flag so we know when we;ve just reached them
		cs->aiFlags |= AIFL_MISCFLAG1;

		// if not overly aggressive, pursue with caution
		if ( cs->attributes[AGGRESSION] <= 0.8 ) {
			cs->movestate = MS_CROUCH;
			cs->movestateType = MSTYPE_TEMPORARY;
		}

		// move straight to them if we can
		if (    !moved &&
				( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE || g_entities[cs->entityNum].waterlevel > 1 ) ) {
			aicast_predictmove_t move;
			vec3_t dir;
			bot_input_t bi;
			usercmd_t ucmd;
			trace_t tr;
			qboolean simTest = qfalse;

			if ( cs->attributes[RUNNING_SPEED] < 120 ) {
				simTest = qtrue;
			}

			if ( !simTest ) {
				// trace will eliminate most unsuccessful paths
				trap_Trace( &tr, cs->bs->origin, NULL /*g_entities[cs->entityNum].r.mins*/, NULL /*g_entities[cs->entityNum].r.maxs*/, destorg, cs->entityNum, g_entities[cs->entityNum].clipmask );
				if ( tr.fraction == 1 ) {
					simTest = qtrue;
				}
			}

			if ( simTest ) {
				// try walking straight to them
				gentity_t *gent;
				//
				gent = G_Spawn();
				VectorCopy( destorg, gent->r.currentOrigin );
				//
				VectorSubtract( destorg, cs->bs->origin, dir );
				VectorNormalize( dir );
				if ( !ent->waterlevel ) {
					dir[2] = 0;
				}
				//trap_EA_Move(cs->entityNum, dir, 400);
				trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi );
				VectorCopy( dir, bi.dir );
				bi.speed = 400;
				bi.actionflags = 0;
				AICast_InputToUserCommand( cs, &bi, &ucmd, bs->cur_ps.delta_angles );
				AICast_PredictMovement( cs, 10, 0.8, &move, &ucmd, gent->s.number );
				//
				if ( move.stopevent == PREDICTSTOP_HITENT ) { // success!
					trap_EA_Move( cs->entityNum, dir, 400 );
					vectoangles( dir, cs->ideal_viewangles );
					cs->ideal_viewangles[2] *= 0.5;
					moved = qtrue;
				} else {    // clear movement
					//trap_EA_Move(cs->entityNum, dir, 0);
				}
				//
				G_FreeEntity( gent );
			}
		}
		//
		if ( !moved ) {
			// use AAS routing
			moveresult = AICast_MoveToPos( cs, destorg, -1 );
			// if we cant get there, do something else
			if ( moveresult && moveresult->failure ) {

				// if we can get a visible target, then face it
				if ( trap_AAS_GetRouteFirstVisPos( cs->audibleEventOrg, cs->bs->origin, cs->travelflags, destorg ) ) {
					cs->aiFlags |= AIFL_MISCFLAG2;
					VectorSubtract( destorg, cs->bs->origin, destorg );
					VectorNormalize( destorg );
					vectoangles( destorg, cs->ideal_viewangles );
					return NULL;
				}

				if ( cs->lastEnemy < 0 && cs->enemyNum < 0 ) {
					// look back at our original angles
					VectorCopy( ent->s.angles, cs->ideal_viewangles );
				}
				return AIFunc_DefaultStart( cs );
			} else if ( !moveresult ) {   // face it?
				if ( trap_InPVS( destorg, cs->bs->origin ) ) {
					VectorSubtract( destorg, cs->bs->origin, vec );
					VectorNormalize( vec );
					vectoangles( vec, cs->ideal_viewangles );
				}
			}
		}

		// should we slow down?
		if ( cs->followDist && cs->followSlowApproach ) {
			cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, cs->followDist );
		}
/*
		// check for a movement we should be making
		if (cs->obstructingTime > level.time)
		{
			AICast_MoveToPos( cs, cs->obstructingPos, -1 );
			if (cs->movestate != MS_CROUCH) {
				cs->movestate = MS_WALK;
			}
			cs->movestateType = MSTYPE_TEMPORARY;
		}
*/
	} else if ( cs->aiFlags & AIFL_MISCFLAG1 ) {
		cs->aiFlags &= ~AIFL_MISCFLAG1;
		// call a script event
		cs->aiFlags &= ~AIFL_DENYACTION;
		AICast_ForceScriptEvent( cs, "inspectsoundend", g_entities[cs->audibleEventEnt].aiName );
		if ( cs->aiFlags & AIFL_DENYACTION ) {
			// relinguish control back to scripting
			return AIFunc_DefaultStart( cs );
		}
	} else {
		// look around randomly
		if ( cs->battleHuntViewTime < level.time ) {
			cs->battleHuntViewTime = level.time + 700 + rand() % 1000;
			// set a random viewangle
			cs->ideal_viewangles[YAW] = AngleMod( cs->ideal_viewangles[YAW] + ( 45.0 + random() * 45.0 ) * ( 2 * ( rand() % 2 ) - 1 ) );
		}
		//
		if ( cs->scriptPauseTime < level.time ) {
			// we're done waiting around here
			if ( cs->lastEnemy < 0 && cs->enemyNum < 0 ) {
				// look back at our original angles
				VectorCopy( ent->s.angles, cs->ideal_viewangles );
			}
			return AIFunc_DefaultStart( cs );
		}
	}

	{
		int numEnemies;
		//
		// look for things we should attack
		numEnemies = AICast_ScanForEnemies( cs, enemies );
		if ( numEnemies == -1 ) { // query mode
			return NULL;
		} else if ( numEnemies == -2 )     { // inspection
			// only override current objective if we are inspecting a dead guy, and the new inspect target is fighting someone
			if ( ( g_entities[cs->inspectNum].health <= 0 ) && ( g_entities[enemies[0]].health > 0 ) ) {
				return AIFunc_InspectFriendlyStart( cs, enemies[0] );
			}
		} else if ( numEnemies == -4 )     { // NEW audible event
			//if (cs->aiState < AISTATE_COMBAT) {
			return AIFunc_InspectAudibleEventStart( cs, cs->audibleEventEnt );
			//}
		} else if ( numEnemies > 0 )     {
			int i;

			cs->enemyNum = enemies[0];  // just attack the first one
			// override with a visible enemy
			for ( i = 1; i < numEnemies; i++ ) {
				if ( AICast_CheckAttack( cs, enemies[i], qfalse ) ) {
					cs->enemyNum = enemies[i];
					break;
				} else if ( cs->enemyNum < 0 ) {
					cs->lastEnemy = enemies[i];
				}
			}

			return AIFunc_BattleStart( cs );
		}
	}

	// set head look flag if no enemy
	if ( cs->enemyNum < 0 && cs->attributes[TACTICAL] >= 0.5 && !( cs->aiFlags & AIFL_NO_HEADLOOK ) ) {
		g_entities[cs->entityNum].client->ps.eFlags |= EF_HEADLOOK;
	}

	// reload?
	AICast_IdleReload( cs );

	return NULL;
}

/*
============
AIFunc_InspectAudibleEventStart
============
*/
char *AIFunc_InspectAudibleEventStart( cast_state_t *cs, int entnum ) {
	cast_state_t *ocs;
	int oldScriptIndex;

	ocs = AICast_GetCastState( entnum );

	// we have now processed the audible event (whether we act on it or not)
	cs->audibleEventTime = -9999;

	// trigger a script event, which has the ability to deny the request
	oldScriptIndex = cs->scriptCallIndex;
	AICast_ForceScriptEvent( cs, "inspectsoundstart", g_entities[cs->audibleEventEnt].aiName );
	if ( cs->aiFlags & AIFL_DENYACTION ) {
		return NULL;
	}

	// if not in alert mode, go there now
	if ( cs->aiState < AISTATE_ALERT ) {
		AICast_StateChange( cs, AISTATE_ALERT );
	}

	if ( oldScriptIndex == cs->scriptCallIndex ) {
		BG_AnimScriptEvent( &g_entities[cs->entityNum].client->ps, ANIM_ET_INSPECTSOUND, qfalse, qtrue );
	}

	// pause the scripting for now
	cs->scriptPauseTime = level.time + 4000;    // wait for at least this long before resuming any scripted walking, etc

	// set this when we decide to just look, rather than pursue
	cs->aiFlags &= ~AIFL_MISCFLAG2;

	// what should we do? wait here? hide? go see them?

	// if dead, go see them
	if ( g_entities[entnum].health <= 0 ) {
		cs->inspectNum = entnum;
		cs->aifunc = AIFunc_InspectFriendly;
		return "AIFunc_InspectFriendlyStart";
	}

	// if they are in combat, then act according to aggressiveness
	if ( ocs->aiState >= AISTATE_COMBAT ) {
		if ( cs->attributes[AGGRESSION] < 0.3 ) {
			if ( AICast_GetTakeCoverPos( cs, entnum, g_entities[entnum].client->ps.origin, cs->takeCoverPos ) ) {
				cs->takeCoverTime = level.time + 10000; // hide for 10 seconds
				cs->scriptPauseTime = cs->takeCoverTime;
				// crouch there if possible
				if ( cs->attributes[ATTACK_CROUCH] > 0.1 ) {
					cs->attackcrouch_time = level.time + 3000;
				}
				return AIFunc_BattleTakeCoverStart( cs );
			}
		}
	}

	cs->aifunc = AIFunc_InspectAudibleEvent;
	return "AIFunc_InspectAudibleEvent";
}

/*
============
AIFunc_ChaseGoalIdle()
============
*/
char *AIFunc_ChaseGoalIdle( cast_state_t *cs ) {
	gentity_t   *followent;
	vec3_t dir;

	if ( cs->followEntity < 0 ) {
		AICast_EndChase( cs );
		return AIFunc_IdleStart( cs );
	}

	followent = &g_entities[cs->followEntity];

	// CHECK: will this interfere with scripting?
	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// go to a position that cannot be seen from the dangerPos
			cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
			cs->attackcrouch_time = 0;
			return AIFunc_AvoidDangerStart( cs );
		}
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}
	//
	// if the player is not ready yet, wait
	if ( !followent->inuse ) {
		return NULL;
	}

	// has the scripting stopped asking us to pursue this goal?
	if ( cs->followIsGoto && ( cs->followTime < level.time ) ) {
		return AIFunc_Idle( cs );
	}

	// they are ready, are they outside range?
	if ( Distance( followent->r.currentOrigin, cs->bs->origin ) > cs->followDist ) {
		return AIFunc_ChaseGoalStart( cs, cs->followEntity, cs->followDist, qtrue );
	}

	// check for a movement we should be making
	if ( cs->obstructingTime > level.time ) {
		AICast_MoveToPos( cs, cs->obstructingPos, -1 );
		cs->speedScale = cs->attributes[WALKING_SPEED] / cs->attributes[RUNNING_SPEED];
	}
	// if we have an enemy, fire if they're visible
	else if ( cs->enemyNum >= 0 ) {
		//attack the enemy if possible
		AICast_ProcessAttack( cs );
	}
	// if we had an enemy recently, face them
	else if ( cs->lastEnemy >= 0 ) {
		vec3_t dir;
		//
		VectorSubtract( cs->vislist[cs->lastEnemy].visible_pos, cs->bs->origin, dir );
		if ( VectorLength( dir ) < 1 ) {
			cs->ideal_viewangles[PITCH] = 0;
		} else {
			VectorNormalize( dir );
			vectoangles( dir, cs->ideal_viewangles );
		}
		// reload?
		AICast_IdleReload( cs );
	} else if ( followent->client )     {
		// face them
		VectorSubtract( followent->r.currentOrigin, cs->bs->origin, dir );
		dir[2] += followent->client->ps.viewheight - g_entities[cs->bs->entitynum].client->ps.viewheight;
		VectorNormalize( dir );
		vectoangles( dir, cs->ideal_viewangles );
	}

	// look for things we should attack
	numEnemies = AICast_ScanForEnemies( cs, enemies );
	if ( numEnemies == -1 ) { // query mode
		return NULL;
	} else if ( numEnemies == -2 )     { // inspection may be required
		char *retval;
		// TTimo: gcc: suggest () around assignment used as truth value
		if ( ( retval = AIFunc_InspectFriendlyStart( cs, enemies[0] ) ) ) {
			return retval;
		}
	} else if ( numEnemies == -3 )     { // bullet impact
		if ( cs->aiState < AISTATE_COMBAT ) {
			return AIFunc_InspectBulletImpactStart( cs );
		}
	} else if ( numEnemies == -4 )     { // audible event
		if ( cs->aiState < AISTATE_COMBAT ) {
			return AIFunc_InspectAudibleEventStart( cs, cs->audibleEventEnt );
		}
	} else if ( numEnemies > 0 )     {
		cs->enemyNum = enemies[0];  // just attack the first one
	}

	// set head look flag if no enemy
	if ( cs->enemyNum < 0 && cs->attributes[TACTICAL] >= 0.5 && !( cs->aiFlags & AIFL_NO_HEADLOOK ) ) {
		g_entities[cs->entityNum].client->ps.eFlags |= EF_HEADLOOK;
	}

	return NULL;
}

/*
============
AIFunc_ChaseGoalIdleStart()
============
*/
char *AIFunc_ChaseGoalIdleStart( cast_state_t *cs, int entitynum, float reachdist ) {
	// make sure we don't avoid any areas when we start again
	trap_BotInitAvoidReach( cs->bs->ms );

	// if we are following someone, always use the default (ready for action) anim
	if ( entitynum < MAX_CLIENTS ) {
		g_entities[cs->entityNum].client->ps.eFlags &= ~EF_STAND_IDLE2;
	} else {
		// randomly choose idle animation
//----(SA)	try always using the 'casual' stand on spawn and change to crouching one when 'alerted'
		if ( cs->aiFlags & AIFL_STAND_IDLE2 ) {
//			if (cs->lastEnemy < 0)
			g_entities[cs->entityNum].client->ps.eFlags |= EF_STAND_IDLE2;
//			else
//				g_entities[cs->entityNum].client->ps.eFlags &= ~EF_STAND_IDLE2;
		}
	}

	cs->followEntity = entitynum;
	cs->followDist = reachdist;
	cs->aifunc = AIFunc_ChaseGoalIdle;
	return "AIFunc_ChaseGoalIdle";
}

/*
============
AIFunc_ChaseGoal()
============
*/
char *AIFunc_ChaseGoal( cast_state_t *cs ) {
	gentity_t   *followent, *ent;
	bot_state_t *bs;
	vec3_t destorg;
	float dist;
	qboolean moved = qfalse;

	ent = &g_entities[cs->entityNum];

	if ( cs->followEntity < 0 ) {
		AICast_EndChase( cs );
		return AIFunc_IdleStart( cs );
	}

	// CHECK: will this mess with scripting?
	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// go to a position that cannot be seen from the dangerPos
			cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
			cs->attackcrouch_time = 0;
			cs->castScriptStatus.scriptGotoId = -1;
			cs->movestate = MS_DEFAULT;
			cs->movestateType = MSTYPE_NONE;
			return AIFunc_AvoidDangerStart( cs );
		}
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}

	followent = &g_entities[cs->followEntity];

	// if the entity is not ready yet
	if ( !followent->inuse ) {
		// if it's a connecting client, wait
		if (    cs->followEntity < MAX_CLIENTS
				&&  (   ( followent->client && followent->client->pers.connected == CON_CONNECTING )
						|| ( level.time < 3000 ) ) ) {
			return AIFunc_ChaseGoalIdleStart( cs, cs->followEntity, cs->followDist );
		} else    // stop following it
		{
			AICast_EndChase( cs );
			return AIFunc_IdleStart( cs );
		}
	}

	// has the scripting stopped asking us to pursue this goal?
	if ( cs->followIsGoto && ( cs->followTime < level.time ) ) {
		return AIFunc_IdleStart( cs );
	}

	if ( followent->client ) {
		VectorCopy( followent->client->ps.origin, destorg );
	} else {
		VectorCopy( followent->r.currentOrigin, destorg );
	}

	// they are ready, are they inside range? FIXME: make configurable
	dist = Distance( destorg, cs->bs->origin );
	if ( cs->followSlowApproach && dist < cs->followDist && ( ent->waterlevel || ( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE ) ) ) {
		// if this is a scripted GOTO, stop following now
		if ( cs->followEntity == cs->castScriptStatus.scriptGotoEnt ) {
			AICast_EndChase( cs );
			return AIFunc_IdleStart( cs );
		}
		// if we have reached our leader
		else
		{
			if ( cs->followEntity == cs->leaderNum ) {
				if ( dist < AICAST_LEADERDIST_MIN ) {
					AICast_EndChase( cs );
					return AIFunc_IdleStart( cs );
				} else {
					trace_t tr;
					// if we have a clear line to our leader, move closer, since there may be others following also
					trap_Trace( &tr, cs->bs->origin, cs->bs->cur_ps.mins, cs->bs->cur_ps.maxs, g_entities[cs->followEntity].r.currentOrigin, cs->entityNum, g_entities[cs->entityNum].clipmask );
					if ( tr.entityNum != cs->followEntity ) {
						AICast_EndChase( cs );
						return AIFunc_IdleStart( cs );
					}
					// if we have crouching ability, then use it while we are just moving closer
					if ( cs->attributes[ATTACK_CROUCH] > 0.1 ) {
						cs->attackcrouch_time = level.time + 1000;
					}
				}
			} else
			{
				return AIFunc_ChaseGoalIdleStart( cs, cs->followEntity, cs->followDist );
			}
		}
	}
	//
	// go to them
	//
	bs = cs->bs;
	//
	// RF, disabled this, MIKE sees dead people
	//if (followent->client && followent->health <= 0) {
	//	AICast_EndChase( cs );
	//	return AIFunc_IdleStart(cs);
	//}

	// move straight to them if we can
	if (    !moved &&
			( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE || g_entities[cs->entityNum].waterlevel > 1 ) ) {
		aicast_predictmove_t move;
		vec3_t dir;
		bot_input_t bi;
		usercmd_t ucmd;
		trace_t tr;
		qboolean simTest = qfalse;
		float frameTime = 0.8, goaldist;

		if ( cs->attributes[RUNNING_SPEED] < 120 ) {
			simTest = qtrue;
		}

		if ( !simTest ) {
			// trace will eliminate most unsuccessful paths
			trap_Trace( &tr, cs->bs->origin, NULL /*g_entities[cs->entityNum].r.mins*/, NULL /*g_entities[cs->entityNum].r.maxs*/, followent->r.currentOrigin, cs->entityNum, g_entities[cs->entityNum].clipmask );
			if ( tr.entityNum == cs->followEntity || tr.fraction == 1 ) {
				simTest = qtrue;
			}
		}

		if ( simTest ) {
			// try walking straight to them
			VectorSubtract( followent->r.currentOrigin, cs->bs->origin, dir );
			if ( !ent->waterlevel ) {
				dir[2] = 0;
			}
			goaldist = VectorNormalize( dir );
			//trap_EA_Move(cs->entityNum, dir, 400);
			trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi );
			VectorCopy( dir, bi.dir );
			bi.speed = 400;
			bi.actionflags = 0;
			AICast_InputToUserCommand( cs, &bi, &ucmd, bs->cur_ps.delta_angles );
			AICast_PredictMovement( cs, 10, frameTime, &move, &ucmd, cs->followEntity );

			if ( move.stopevent == PREDICTSTOP_HITENT ) { // success!
				// make sure we didnt spend a lot of time sliding along an obstacle
				if ( ( move.frames * frameTime ) < ( 1.0 + ( goaldist / ( bs->cur_ps.speed * bs->cur_ps.runSpeedScale ) ) ) ) {
					trap_EA_Move( cs->entityNum, dir, 400 );
					vectoangles( dir, cs->ideal_viewangles );
					cs->ideal_viewangles[2] *= 0.5;
					moved = qtrue;
				}
			}
			if ( !moved ) {
				//trap_EA_Move(cs->entityNum, dir, 0);
			}
		}
	}
	//
	if ( !moved ) {
		// use AAS routing
		moveresult = AICast_MoveToPos( cs, followent->r.currentOrigin, cs->followEntity );
		if ( moveresult && moveresult->failure ) {
			// shit?
		}
	}

	// should we slow down?
	if ( cs->followDist && cs->followSlowApproach && cs->followDist < 48 ) {
		cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, cs->followDist );
	}

	// check for a movement we should be making
	if ( cs->obstructingTime > level.time ) {
		AICast_MoveToPos( cs, cs->obstructingPos, -1 );
		if ( cs->movestate != MS_CROUCH ) {
			cs->movestate = MS_WALK;
		}
		cs->movestateType = MSTYPE_TEMPORARY;
	}

	// if we have an enemy, fire if they're visible
	if ( cs->enemyNum >= 0 ) { //attack the enemy if possible
		AICast_ProcessAttack( cs );
	} else {
		int numEnemies;
		//
		// look for things we should attack
		numEnemies = AICast_ScanForEnemies( cs, enemies );
		if ( numEnemies == -1 ) { // query mode
			return NULL;
		} else if ( numEnemies == -2 )     { // inspection may be required
			char *retval;
			// TTimo: gcc: suggest () around assignment used as truth value
			if ( ( retval = AIFunc_InspectFriendlyStart( cs, enemies[0] ) ) ) {
				return retval;
			}
		} else if ( numEnemies == -3 )     { // bullet impact
			if ( cs->aiState < AISTATE_COMBAT ) {
				return AIFunc_InspectBulletImpactStart( cs );
			}
		} else if ( numEnemies == -4 )     { // audible event
			if ( cs->aiState < AISTATE_COMBAT ) {
				return AIFunc_InspectAudibleEventStart( cs, cs->audibleEventEnt );
			}
		} else if ( numEnemies > 0 )     {
			int i;

			cs->enemyNum = enemies[0];  // just attack the first one
			// override with a visible enemy
			for ( i = 1; i < numEnemies; i++ ) {
				if ( AICast_CheckAttack( cs, enemies[i], qfalse ) ) {
					cs->enemyNum = enemies[i];
					break;
				} else if ( cs->enemyNum < 0 ) {
					cs->lastEnemy = enemies[i];
				}
			}
		}
		// reload?
		AICast_IdleReload( cs );
	}

	// set head look flag if no enemy
	if ( cs->enemyNum < 0 && cs->attributes[TACTICAL] >= 0.5 && !( cs->aiFlags & AIFL_NO_HEADLOOK ) ) {
		g_entities[cs->entityNum].client->ps.eFlags |= EF_HEADLOOK;
	}

	return NULL;

}

/*
============
AIFunc_ChaseGoalStart()
============
*/
char *AIFunc_ChaseGoalStart( cast_state_t *cs, int entitynum, float reachdist, qboolean slowApproach ) {
	cs->followEntity = entitynum;
	cs->followDist = reachdist;
	cs->followIsGoto = qfalse;
	cs->followSlowApproach = slowApproach;
	cs->aifunc = AIFunc_ChaseGoal;
	return "AIFunc_ChaseGoal";
}

/*
============
AIFunc_DoorMarker()
============
*/
char *AIFunc_DoorMarker( cast_state_t *cs ) {
	gentity_t   *followent, *door;
	bot_state_t *bs;
	vec3_t destorg;
	float dist;
	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// shit??
		}
		// go to a position that cannot be seen from the dangerPos
		cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
		cs->attackcrouch_time = 0;
		return AIFunc_AvoidDangerStart( cs );
	}

	followent = &g_entities[cs->doorMarker];

	// if the entity is not ready yet
	if ( !followent->inuse ) {
		cs->doorMarkerTime = 0;
		//return AIFunc_DefaultStart( cs );
		return AIFunc_Restore( cs );
	}

	// if the door is open or idle
	door = &g_entities[cs->doorEntNum];
	if (    ( !door->key ) &&
			( door->s.apos.trType == TR_STATIONARY && door->s.pos.trType == TR_STATIONARY ) ) {
		cs->doorMarkerTime = 0;
		//return AIFunc_DefaultStart( cs );
		return AIFunc_Restore( cs );
	}

	// if we have an enemy, fire if they're visible
	if ( cs->enemyNum >= 0 ) { //attack the enemy if possible
		AICast_ProcessAttack( cs );
	}

	// they are ready, are they inside range? FIXME: make configurable
	dist = Distance( destorg, cs->bs->origin );
	if ( dist < 12 ) {
		// check for a movement we should be making
		if ( cs->obstructingTime > level.time ) {
			AICast_MoveToPos( cs, cs->obstructingPos, -1 );
		}
		// if the door is locked, resume
		if ( followent->key ) {
			return AIFunc_Restore( cs );
		}
		return NULL;
	}

	// go to it
	//
	bs = cs->bs;
	//
	moveresult = AICast_MoveToPos( cs, followent->r.currentOrigin, followent->s.number );
	// if we cant get there, forget it
	if ( moveresult && moveresult->failure ) {
		return AIFunc_Restore( cs );
	}
	// should we slow down?
	if ( cs->followDist ) {
		cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, cs->followDist );
	}
	// reload?
	AICast_IdleReload( cs );
	return NULL;

}

/*
============
AIFunc_DoorMarkerStart()
============
*/
char *AIFunc_DoorMarkerStart( cast_state_t *cs, int doornum, int markernum ) {
	cs->doorEntNum = doornum;
	cs->doorMarker = markernum;
	cs->oldAifunc = cs->aifunc;
	cs->aifunc = AIFunc_DoorMarker;
	return "AIFunc_DoorMarker";
}

/*
=============
AIFunc_BattleRoll()
=============
*/
char *AIFunc_BattleRoll( cast_state_t *cs ) {
	gclient_t *client = &level.clients[cs->entityNum];
	vec3_t dir;
	//
	// record the time
	cs->lastRollMove = level.time;
	client->ps.eFlags |= EF_NOSWINGANGLES;
	//
	if ( !client->ps.torsoTimer ) {
		if ( cs->battleRollTime < level.time ) {
			return AIFunc_Restore( cs );
		} else {
			// attack?
			if ( cs->enemyNum >= 0 ) {
				AICast_ProcessAttack( cs );
			}
		}
	}
	if ( g_entities[cs->entityNum].health <= 0 ) {
		return AIFunc_DefaultStart( cs );
	}
	//
	trap_EA_Crouch( cs->entityNum );
	cs->attackcrouch_time = level.time + 500;
	// all characters so far only move during the first second of animation
	if ( cs->thinkFuncChangeTime > level.time - 1000 ) {
		// just move in the direction of our ideal_viewangles
		AngleVectors( cs->ideal_viewangles, dir, NULL, NULL );
		trap_EA_Move( cs->entityNum, dir, 300 );
		// we are crouching, move a little faster than normal
		cs->speedScale = 1.5;
	} else if ( cs->takeCoverTime > level.time ) {
		//
		// if we are taking Cover, use this position, if it's bad, we'll just look for a better spot once we're done here
		VectorCopy( cs->bs->origin, cs->takeCoverPos );
	} else if ( cs->enemyNum >= 0 ) {
		//
		// start turning towards our enemy
		AICast_ProcessAttack( cs );
	}
	//
	return NULL;
}

/*
=============
AIFunc_BattleRollStart()
=============
*/
char *AIFunc_BattleRollStart( cast_state_t *cs, vec3_t vec ) {
	int duration;
//	gclient_t *client = &level.clients[cs->entityNum];
	//
	// backup the current thinkfunc, so we can return to it when done
	cs->oldAifunc = cs->aifunc;
	//
	// face the direction of movement
	vectoangles( vec, cs->ideal_viewangles );
	// do the roll
	duration = BG_AnimScriptEvent( &g_entities[cs->entityNum].client->ps, ANIM_ET_ROLL, qfalse, qtrue );
	//
	if ( duration < 0 ) { // it failed
		return NULL;
	}
	// add some duration to make sure it fully plays out
	duration += 100;
	g_entities[cs->entityNum].client->ps.legsTimer = duration;
	g_entities[cs->entityNum].client->ps.torsoTimer = duration;
	//
	cs->noAttackTime = level.time + duration - 200;
	// set the duration
	cs->battleRollTime = level.time + duration;
	// move into crouch position
	//cs->attackcrouch_time = level.time + (duration) + 1000;
	// record the time
	cs->lastRollMove = level.time;
	//
	// make sure we move this frame
	AIFunc_BattleRoll( cs );
	//
	cs->aifunc = AIFunc_BattleRoll;
	return "AIFunc_BattleRoll";
}

/*
=============
AIFunc_BattleDiveStart()
=============
*/
char *AIFunc_BattleDiveStart( cast_state_t *cs, vec3_t vec ) {
	int duration;
//	gclient_t *client = &level.clients[cs->entityNum];
	//
	// backup the current thinkfunc, so we can return to it when done
	cs->oldAifunc = cs->aifunc;
	//
	// face the direction of movement
	vectoangles( vec, cs->ideal_viewangles );
	// force crouching anim
	BG_UpdateConditionValue( cs->entityNum, ANIM_COND_CROUCHING, qtrue, qfalse );
	// do the roll
	duration = BG_AnimScriptEvent( &g_entities[cs->entityNum].client->ps, ANIM_ET_DIVE, qfalse, qtrue );
	//
	if ( duration < 0 ) { // it failed
		return NULL;
	}
	//
	cs->noAttackTime = level.time + duration - 200;
	// set the duration
	cs->battleRollTime = level.time + duration;
	// move into crouch position
	//cs->attackcrouch_time = level.time + (duration) + 1000;
	// record the time
	cs->lastRollMove = level.time;
	//
	// make sure we move this frame
	AIFunc_BattleRoll( cs );
	//
	cs->aifunc = AIFunc_BattleRoll;
	return "AIFunc_BattleRoll";
}

/*
=============
AIFunc_FlipMove()
=============
*/
char *AIFunc_FlipMove( cast_state_t *cs ) {
	gclient_t *client = &level.clients[cs->entityNum];
	vec3_t dir;
	//
	if ( !client->ps.torsoTimer ) {
		cs->attackcrouch_time = 0;
		return AIFunc_Restore( cs );
	}
	if ( g_entities[cs->entityNum].health <= 0 ) {
		return AIFunc_DefaultStart( cs );
	}
	//
	// just move in the direction of our ideal_viewangles
	AngleVectors( cs->ideal_viewangles, dir, NULL, NULL );
	trap_EA_Move( cs->entityNum, dir, 400 );
	// if we are crouching, move a little faster than normal
	if ( cs->attackcrouch_time > level.time ) {
		cs->speedScale = 1.5;
	}
	//
	return NULL;
}

/*
=============
AIFunc_FlipMoveStart()
=============
*/
char *AIFunc_FlipMoveStart( cast_state_t *cs, vec3_t vec ) {
	int duration;
//	gclient_t *client = &level.clients[cs->entityNum];
	//
	// backup the current thinkfunc, so we can return to it when done
	cs->oldAifunc = cs->aifunc;
	//
	// record the time
	cs->lastRollMove = level.time;
	// face the direction of movement
	vectoangles( vec, cs->ideal_viewangles );
	cs->noAttackTime = level.time + 1200;
	// do the roll
	duration = BG_AnimScriptEvent( &g_entities[cs->entityNum].client->ps, ANIM_ET_ROLL, qfalse, qfalse );
	//
	if ( duration < 0 ) { // it failed
		return NULL;
	}
	// move into crouch position
	cs->attackcrouch_time = level.time + 800;
	//
	// make sure we move this frame
	AIFunc_FlipMove( cs );
	//
	cs->aifunc = AIFunc_FlipMove;
	return "AIFunc_FlipMove";
}

/*
=============
AIFunc_BattleHunt()
=============
*/
char *AIFunc_BattleHunt( cast_state_t *cs ) {
	const float chaseDist = 32;
	gentity_t   *followent, *ent;
	bot_state_t *bs;
	vec3_t destorg;
	qboolean moved = qfalse;
//	gclient_t *client = &level.clients[cs->entityNum];
	char *rval;
	float dist;
	cast_state_t *ocs;
	int     *ammo, i;

	ent = &g_entities[cs->entityNum];

	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// shit??
		}
		// go to a position that cannot be seen from the dangerPos
		cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
		cs->attackcrouch_time = 0;
		return AIFunc_AvoidDangerStart( cs );
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}
	//
	bs = cs->bs;
	//
	if ( cs->enemyNum < 0 ) {
		return AIFunc_IdleStart( cs );
	}
	//
	ocs = AICast_GetCastState( cs->enemyNum );
	//
	if ( cs->aiFlags & AIFL_ATTACK_CROUCH ) {
		cs->attackcrouch_time = level.time + 1000;
	}
	//
	followent = &g_entities[cs->enemyNum];
	//
	// if the entity is not ready yet
	if ( !followent->inuse ) {
		// if it's a connecting client, wait
		if ( !(   ( cs->enemyNum < MAX_CLIENTS )
				  && (   ( followent->client && followent->client->pers.connected == CON_CONNECTING )
						 || ( level.time < 3000 ) ) ) ) { // they don't exist anymore, stop attacking
			cs->enemyNum = -1;
		}

		return AIFunc_IdleStart( cs );
	}
	//
	// if we can see them, go back to an attack state
	AICast_ChooseWeapon( cs, qtrue );   // enable special weapons, if we cant get them, change back
	if (    AICast_EntityVisible( cs, cs->enemyNum, qtrue ) // take into account reaction time
			&&  AICast_CheckAttack( cs, cs->enemyNum, qfalse )
			&&  cs->obstructingTime < level.time ) {
		if ( AICast_StopAndAttack( cs ) ) {
			// TTimo: gcc: suggest () around assignment used as truth value
			if ( ( rval = AIFunc_BattleStart( cs ) ) ) {
				return rval;
			}
		} else {    // just attack them now and keep chasing
			AICast_ProcessAttack( cs );
		}
		AICast_ChooseWeapon( cs, qfalse );
	} else
	{
		int numEnemies, shouldAttack;

		AICast_ChooseWeapon( cs, qfalse );

		ammo = cs->bs->cur_ps.ammo;
		shouldAttack = qfalse;
		numEnemies = AICast_ScanForEnemies( cs, enemies );
		if ( numEnemies == -1 ) { // query mode
			return NULL;
		} else if ( numEnemies == -2 )     { // inspection may be required
			char *retval;
			if ( cs->aiState < AISTATE_COMBAT ) {
				// TTimo: gcc: suggest () around assignment used as truth value
				if ( ( retval = AIFunc_InspectFriendlyStart( cs, enemies[0] ) ) ) {
					return retval;
				}
			}
		} else if ( numEnemies == -3 )     { // bullet impact
			if ( cs->aiState < AISTATE_COMBAT ) {
				return AIFunc_InspectBulletImpactStart( cs );
			}
		} else if ( numEnemies == -4 )     { // audible event
			if ( cs->aiState < AISTATE_COMBAT ) {
				return AIFunc_InspectAudibleEventStart( cs, cs->audibleEventEnt );
			}
		} else if ( AICast_GotEnoughAmmoForWeapon( cs, cs->bs->cur_ps.weapon ) )     {
			if ( numEnemies > 0 ) {
				// default to the first known enemy, overwrite if we find a clearer shot
				cs->enemyNum = enemies[0];
				//
				for ( i = 0; i < numEnemies; i++ ) {
					if ( AICast_CheckAttack( cs, enemies[i], qfalse ) || AICast_CheckAttack( AICast_GetCastState( enemies[i] ), cs->entityNum, qfalse ) ) {
						cs->enemyNum = enemies[i];
						shouldAttack = qtrue;
						break;
					} else if ( cs->enemyNum < 0 ) {
						cs->lastEnemy = enemies[i];
					}
				}
				// note: next frame we'll process this new enemy an begin an attack if necessary
			}
		}
		AICast_ChooseWeapon( cs, qfalse );
	}

	// have we spent enough time in combat mode?
	if ( cs->aiState == AISTATE_COMBAT ) {
		if ( cs->vislist[cs->enemyNum].visible_timestamp < level.time - COMBAT_TIMEOUT ) {
			AICast_StateChange( cs, AISTATE_ALERT );
		}
	}

	// while hunting, use crouch mode if possible
	if ( cs->attributes[ATTACK_CROUCH] >= 0.1 ) {
		cs->attackcrouch_time = level.time + 1000;
	}

	if ( cs->battleHuntPauseTime ) {
		if ( cs->battleHuntPauseTime < level.time ) {
			// pausetime has expired, so go into ambush mode
			if ( AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].chase_marker[cs->battleChaseMarker], cs->takeCoverPos ) ) {
				// wait in ambush, for them to return
				VectorCopy( cs->vislist[cs->enemyNum].chase_marker[cs->battleChaseMarker], cs->combatGoalOrigin );
				return AIFunc_BattleAmbushStart( cs );
			}
			// couldn't find a spot, so just stay here?
			VectorCopy( cs->bs->origin, cs->combatGoalOrigin );
			VectorCopy( cs->bs->origin, cs->takeCoverPos );
			return AIFunc_BattleAmbushStart( cs );
		} else {
			// stay here, looking around
			if ( cs->battleHuntViewTime < level.time ) {
				cs->battleHuntViewTime = level.time + 700 + rand() % 1000;
				// set a random viewangle
				cs->ideal_viewangles[YAW] = AngleMod( cs->ideal_viewangles[YAW] + ( 45.0 + random() * 45.0 ) * ( 2 * ( rand() % 2 ) - 1 ) );
				cs->ideal_viewangles[PITCH] = 0;
			}
		}
	} else {
		// cycle through markers
		VectorCopy( cs->vislist[cs->enemyNum].chase_marker[cs->battleChaseMarker], destorg );
		if ( ( dist = Distance( destorg, cs->bs->origin ) ) < chaseDist ) {
			if ( cs->battleChaseMarker == ( cs->vislist[cs->enemyNum].chase_marker_count - 1 ) ) {
				cs->battleHuntPauseTime = level.time + 4000;
				cs->battleHuntViewTime = level.time + 1000;
			} else {
				cs->battleChaseMarker += cs->battleChaseMarkerDir;
				if ( cs->battleChaseMarker > cs->vislist[cs->enemyNum].chase_marker_count ) {
					cs->battleChaseMarkerDir *= -1;
					cs->battleChaseMarker = cs->vislist[cs->enemyNum].chase_marker_count - 1;
				}
				if ( cs->battleChaseMarker < 0 ) {
					cs->battleChaseMarkerDir = 1;
					cs->battleChaseMarker = 0;
				}
			}
		}
		//
		if ( cs->battleHuntPauseTime < level.time ) {
			// just go to them
			if ( !moved && cs->leaderNum < 0 ) {
				moveresult = AICast_MoveToPos( cs, destorg, cs->enemyNum );
				if ( moveresult && moveresult->failure ) {    // no path, so go back to idle behaviour
					// try to go to ambush mode
					cs->enemyNum = -1;
					return AIFunc_DefaultStart( cs );
				} else {
					moved = qtrue;
				}
				// slow down real close to the goal, so we don't go passed it
				cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, chaseDist );
			}
		}
	}

	// reload?
	AICast_IdleReload( cs );

	return NULL;
}

/*
=============
AIFunc_BattleHuntStart()
=============
*/
char *AIFunc_BattleHuntStart( cast_state_t *cs ) {
	cs->combatGoalTime = 0;
	cs->battleChaseMarker = 0;
	cs->battleChaseMarkerDir = 1;
	cs->battleHuntPauseTime = 0;
	//
	cs->aifunc = AIFunc_BattleHunt;
	return "AIFunc_BattleHunt";
}

/*
=============
AIFunc_BattleAmbush()
=============
*/
char *AIFunc_BattleAmbush( cast_state_t *cs ) {
	bot_state_t *bs;
	vec3_t destorg, vec;
	float dist, moveDist;
	int enemies[MAX_CLIENTS], numEnemies, i;
	qboolean shouldAttack, idleYaw;
	aicast_predictmove_t move;
	int     *ammo;
	vec3_t dir;
//	gclient_t	*client = &level.clients[cs->entityNum];
	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// go to a position that cannot be seen from the dangerPos
			cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
			cs->attackcrouch_time = 0;
			return AIFunc_AvoidDangerStart( cs );
		}
		// if not found, then keep trying, hopefully a spot will free up soon so we can run the hidepos function
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}

	// we need to move towards it
	bs = cs->bs;
	//
	// note: removing this will cause problems down below!
	if ( cs->enemyNum < 0 ) {
		return AIFunc_IdleStart( cs );
	}
	//
	// have we spent enough time in combat mode?
	if ( cs->aiState == AISTATE_COMBAT ) {
		if ( cs->vislist[cs->enemyNum].visible_timestamp < level.time - COMBAT_TIMEOUT ) {
			AICast_StateChange( cs, AISTATE_ALERT );
		}
	}
	// while hunting, use crouch mode if possible
	if ( cs->attributes[ATTACK_CROUCH] >= 0.1 ) {
		cs->attackcrouch_time = level.time + 2000;
	}
	//
	VectorCopy( cs->takeCoverPos, destorg );
	VectorSubtract( destorg, cs->bs->origin, vec );
	vec[2] *= 0.2;
	dist = VectorLength( vec );
	//
	// update the chase marker
	if ( cs->vislist[cs->enemyNum].chase_marker_count > 0 ) {
		VectorCopy( cs->vislist[cs->enemyNum].chase_marker[cs->vislist[cs->enemyNum].chase_marker_count - 1], cs->combatGoalOrigin );
	}
	//
	// look for things we should attack
	// if we are out of ammo, we shouldn't bother trying to attack (and we should keep hiding)
	ammo = cs->bs->cur_ps.ammo;
	shouldAttack = qfalse;
	numEnemies = AICast_ScanForEnemies( cs, enemies );

	// we shouldnt be interrupted from BattleAmbush mode, so try to handle these without interference
	if ( numEnemies == -1 ) { // query mode
		// ...
	} else if ( numEnemies == -2 )     { // inspection may be required
		cs->vislist[enemies[0]].flags |= AIVIS_INSPECTED;   // they have been notified
		cs->vislist[enemies[0]].flags &= ~AIVIS_INSPECT;    // they have been notified
	} else if ( numEnemies == -3 )     { // bullet impact
		// ...
	} else if ( numEnemies == -4 )     { // audible event
		// ...
	} else if ( numEnemies > 0 )     {

		if ( AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum ) ) {
			// default to the first known enemy, overwrite if we find a clearer shot
			cs->enemyNum = enemies[0];
			//
			for ( i = 0; i < numEnemies; i++ ) {
				// if (we can get them from here) or (they can get us, AND we have stopped going to our ambush spot)
				if ( ( AICast_EntityVisible( cs, enemies[i], qfalse ) && AICast_CheckAttack( cs, enemies[i], qfalse ) ) ||
					 ( ( VectorLength( cs->takeCoverPos ) < 1 || dist <= 8 ) && ( AICast_EntityVisible( AICast_GetCastState( enemies[i] ), cs->entityNum, qfalse ) || AICast_CheckAttack( AICast_GetCastState( enemies[i] ), cs->entityNum, qfalse ) || AICast_EntityVisible( AICast_GetCastState( enemies[i] ), cs->entityNum, qtrue ) ) ) ) {
					cs->enemyNum = enemies[i];
					return AIFunc_BattleStart( cs );
				} else if ( cs->enemyNum < 0 ) {
					cs->lastEnemy = enemies[i];
				} else if ( AICast_EntityVisible( cs, enemies[i], qfalse ) ) {
					bot_input_t bi_back;
					// try and move to them, if successful, then start chasing
					trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi_back );
					if ( AICast_MoveToPos( cs, g_entities[enemies[i]].client->ps.origin, enemies[i] ) ) {
						if ( !moveresult->failure ) {
							cs->enemyNum = enemies[i];
							return AIFunc_BattleChaseStart( cs );
						}
					} else {
						trap_EA_ResetInput( cs->entityNum, &bi_back );
					}
				}
			}
		} else {
			AICast_ChooseWeapon( cs, qfalse );
			//
			if ( !AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum ) ) {
				// NO AMMO LEFT!!
				// hide?
				if ( AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].visible_pos, cs->takeCoverPos ) ) {
					// go to a position that cannot be seen from the last place we saw the enemy, and wait there for some time
					cs->takeCoverTime = level.time + 2000 + rand() % 3000;
					return AIFunc_BattleTakeCoverStart( cs );
				}
			}
		}

	}
	//
	// keep hiding forever
	cs->takeCoverTime = level.time + 1000;
	//
	memset( &move, 0, sizeof( move ) );
	//
	// are we close enough to the goal?
	if ( VectorLength( cs->takeCoverPos ) > 1 && dist > 8 && ( cs->obstructingTime < level.time ) /*&& !shouldAttack*/ ) {
		const float simTime = 0.8;
		float enemyDist;
		//
		// we haven't reached it yet, make sure we at least wait there for a few seconds after arriving
		cs->takeCoverTime = level.time + 2000 + rand() % 2000;
		//
		moveresult = AICast_MoveToPos( cs, destorg, -1 );
		if ( moveresult ) {
			//if the movement failed
			if ( moveresult->failure ) {
				//reset the avoid reach, otherwise bot is stuck in current area
				trap_BotResetAvoidReach( bs->ms );
				// couldn't get there, so stop trying to get there
				VectorClear( cs->takeCoverPos );
				dist = 0;
			}
			//
			if ( moveresult->blocked ) {
				// abort the TakeCover
				VectorClear( cs->takeCoverPos );
				dist = 0;
			}
		}
		//
		// NOTE: this is also used by hidepos prediction (below)
		// if we are going to bump into something soon, abort it
		AICast_PredictMovement( cs, 1, simTime, &move, &cs->lastucmd, -1 );
		enemyDist = Distance( cs->bs->origin, g_entities[cs->enemyNum].s.pos.trBase );
		VectorSubtract( move.endpos, cs->bs->origin, vec );
		moveDist = VectorNormalize( vec );
		//
		if (    ( move.numtouch && move.touchents[0] < aicast_maxclients )    // hit something
				// or moved closer to the enemy
				||  (   ( enemyDist < 128 )
						&&  ( ( enemyDist - 1 ) > ( Distance( move.endpos, g_entities[cs->enemyNum].s.pos.trBase ) ) ) ) ) {
			// abort the manouver
			VectorClear( cs->takeCoverPos );
			dist = 0;
		}
		//
		// we should slow down on approaching the destination point
		else if ( dist < 64 ) {
			cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, 0 );
		}

		// don't actually hide, check if we are about to, so we can hide right here
		if ( !( cs->aiFlags & AIFL_MISCFLAG1 ) ) {
			if ( move.numtouch || !AICast_VisibleFromPos( move.endpos, cs->entityNum, cs->combatGoalOrigin, cs->enemyNum, qfalse ) ) {
				// abort the manouver, reached a good spot
				cs->aiFlags |= AIFL_MISCFLAG1;
				VectorCopy( cs->bs->origin, cs->takeCoverPos );
			}
		}

	} else {
		//
		// check for a movement we should be making
		if ( cs->obstructingTime > level.time ) {
			AICast_MoveToPos( cs, cs->obstructingPos, -1 );
		}
		// if we have some enemies that we can attack immediately (without going anywhere to chase them)
		if ( shouldAttack ) {
			return AIFunc_BattleStart( cs );
		}
		// do we need to go to our leader?
		else if ( cs->leaderNum >= 0 && Distance( cs->bs->origin, g_entities[cs->leaderNum].r.currentOrigin ) > MAX_LEADER_DIST ) {
			// wait until we've been hiding for long enough
			if ( level.time > cs->takeCoverTime ) {
				return AIFunc_ChaseGoalStart( cs, cs->leaderNum, AICAST_LEADERDIST_MAX, qtrue );
			}
		}
		// else, crouch while we hide
		if ( cs->attributes[ATTACK_CROUCH] > 0.1 || cs->crouchHideFlag ) {
			cs->attackcrouch_time = level.time + 2000;
		}
	}
	//
	if ( !( cs->aiFlags & AIFL_WALKFORWARD ) || !VectorLength( cs->bs->cur_ps.velocity ) ) {
		idleYaw = qtrue;
		// if we know who we are hiding from, face them (hack, so they dont face stupid directions)
		if ( cs->enemyNum >= 0 ) {
			VectorSubtract( g_entities[cs->enemyNum].s.pos.trBase, cs->bs->origin, dir );
			vectoangles( dir, cs->ideal_viewangles );
			idleYaw = qfalse;
		} else if ( cs->lastEnemy >= 0 ) {
			VectorSubtract( g_entities[cs->lastEnemy].s.pos.trBase, cs->bs->origin, dir );
			vectoangles( dir, cs->ideal_viewangles );
			idleYaw = qfalse;
		}
		// if we can see the place we are hiding from, look at it
		if ( idleYaw  && AICast_VisibleFromPos( cs->bs->origin, cs->entityNum, cs->combatGoalOrigin, cs->lastEnemy, qfalse ) ) {
			// face the position we are retreating from
			VectorSubtract( cs->combatGoalOrigin, cs->bs->origin, dir );
			dir[2] = 0;
			if ( VectorNormalize( dir ) > 4 ) {
				idleYaw = qfalse;
				vectoangles( dir, cs->ideal_viewangles );
			}

		}
		//
		if ( idleYaw ) {  // look around randomly (but not straight into walls)

			if ( cs->nextIdleAngleChange < level.time ) {
				// wait a second before changing again
				if ( ( cs->nextIdleAngleChange + 3000 ) < level.time ) {

					// FIXME: This could be changed to use some AAS sampling, which would:
					//
					//	Given a src area, pick a random dest area which is visible from that area
					//	and return it's position, which we'd then use to set the next view angles
					//
					//	This would result in more efficient, more realistic behaviour, since they'd
					//	also use PITCH angles to look at areas above/below them

					cs->idleYaw = AICast_GetRandomViewAngle( cs, 512 );

					if ( abs( AngleDelta( cs->idleYaw, cs->ideal_viewangles[YAW] ) ) < 45 ) {
						cs->nextIdleAngleChange = level.time + 1000 + rand() % 2500;
					} else { // do really fast
						cs->nextIdleAngleChange = level.time + 500;
					}

					// adjust with time
					cs->idleYawChange = AngleDelta( cs->idleYaw, cs->ideal_viewangles[YAW] );
					/// ((float)(cs->nextIdleAngleChange - level.time) / 1000.0);

					cs->ideal_viewangles[PITCH] = 0;
				}
			} else if ( cs->idleYawChange )     {
				cs->idleYawChange = AngleDelta( cs->idleYaw, cs->ideal_viewangles[YAW] );
				cs->ideal_viewangles[YAW] = AngleMod( cs->ideal_viewangles[YAW] + ( cs->idleYawChange * cs->bs->thinktime ) );
			}

		}
	}
	//
	if ( !cs->crouchHideFlag && cs->enemyNum < 0 ) {  // no enemy, and no need to crouch, so stop crouching
		if ( cs->attackcrouch_time > level.time + 1000 ) {
			cs->attackcrouch_time = level.time + 1000;
		}
	}

	// reload?
	AICast_IdleReload( cs );

	return NULL;
}

/*
=============
AIFunc_BattleAmbushStart()
=============
*/
char *AIFunc_BattleAmbushStart( cast_state_t *cs ) {
	if ( !AICast_CanMoveWhileFiringWeapon( cs->weaponNum ) ) {
		// always run to the cover point
		cs->attackcrouch_time = 0;
	} else if ( cs->attackcrouch_time > level.time + 1000 ) {
		cs->attackcrouch_time = level.time + 1000;
	}

	//
	// start a crouch attack?
	if ( cs->attributes[ATTACK_CROUCH] > 0.1 && cs->attackcrouch_time >= level.time ) {
		// continue
		cs->attackcrouch_time = level.time + 1000;
	}
	// if we arent crouching, start crouching soon after we start retreating
	if ( cs->attributes[ATTACK_CROUCH] > 0.1 ) {
		cs->aiFlags |= AIFL_ATTACK_CROUCH;
	} else {
		cs->aiFlags &= ~AIFL_ATTACK_CROUCH;
	}

	// miscflag1 used to set predicted point as our goal, so we dont keep setting this over and over
	cs->aiFlags &= ~AIFL_MISCFLAG1;

	cs->aifunc = AIFunc_BattleAmbush;
	return "AIFunc_BattleAmbush";
}

/*
============
AIFunc_BattleChase()
============
*/
char *AIFunc_BattleChase( cast_state_t *cs ) {
	const float chaseDist = 32;
	gentity_t   *followent, *ent;
	bot_state_t *bs;
	vec3_t destorg;
	qboolean moved = qfalse;
	gclient_t *client = &level.clients[cs->entityNum];
	char *rval;
	float dist;
	cast_state_t *ocs;

	ent = &g_entities[cs->entityNum];

	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// shit??
		}
		// go to a position that cannot be seen from the dangerPos
		cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
		cs->attackcrouch_time = 0;
		return AIFunc_AvoidDangerStart( cs );
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}

	bs = cs->bs;
	//
	if ( cs->enemyNum < 0 ) {
		return AIFunc_IdleStart( cs );
	}
	//
	// Retreat?
	if ( AICast_WantToRetreat( cs ) ) {
		if  ( AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].visible_pos, cs->takeCoverPos ) ) {
			// go to a position that cannot be seen from the last place we saw the enemy, and wait there for some time
			cs->takeCoverTime = level.time + 2000 + rand() % 3000;
			return AIFunc_BattleTakeCoverStart( cs );
		}
	}
	//
	ocs = AICast_GetCastState( cs->enemyNum );
	//
	if ( cs->aiFlags & AIFL_ATTACK_CROUCH ) {
		if ( cs->attackcrouch_time > level.time || ( cs->thinkFuncChangeTime < level.time - 1000 ) ) {
			cs->attackcrouch_time = level.time + 1000;
		}
	}
	//
	followent = &g_entities[cs->enemyNum];
	//
	// if the entity is not ready yet
	if ( !followent->inuse ) {
		// if it's a connecting client, wait
		if ( !(   ( cs->enemyNum < MAX_CLIENTS )
				  && (   ( followent->client && followent->client->pers.connected == CON_CONNECTING )
						 || ( level.time < 3000 ) ) ) ) { // they don't exist anymore, stop attacking
			cs->enemyNum = -1;
		}

		return AIFunc_IdleStart( cs );
	}
	//
	// if we can see them, go back to an attack state
	AICast_ChooseWeapon( cs, qtrue );   // enable special weapons, if we cant get them, change back
	if (    AICast_EntityVisible( cs, cs->enemyNum, qtrue ) // take into account reaction time
			&&  AICast_CheckAttack( cs, cs->enemyNum, qfalse )
			&&  cs->obstructingTime < level.time ) {
		if ( AICast_StopAndAttack( cs ) ) {
			// TTimo: gcc: suggest () around assignment used as truth value
			if ( ( rval = AIFunc_BattleStart( cs ) ) ) {
				return rval;
			}
		} else {    // just attack them now and keep chasing
			AICast_ProcessAttack( cs );
		}
		AICast_ChooseWeapon( cs, qfalse );
	} else
	{
		AICast_ChooseWeapon( cs, qfalse );
		// not visible, go to their previously visible position
		/*
		if (!cs->vislist[cs->enemyNum].visible_timestamp || Distance( bs->origin, cs->vislist[cs->enemyNum].visible_pos ) < 16)
		{
			// we're done attacking, go back to default state, which in turn will recall previous state
			//
			return AIFunc_DefaultStart( cs );
		}
		*/
	}
	//
	// find the chase position
	if ( followent->client ) {
		// go to the last visible position
		VectorCopy( cs->vislist[cs->enemyNum].visible_pos, destorg );
		// if we have reached it, go into hunt mode
		if ( ( dist = Distance( destorg, cs->bs->origin ) ) < chaseDist ) {
			// if we haven't been hunted for a while, do so
			if ( ocs->lastBattleHunted < level.time - 5000 ) {
				ocs->lastBattleHunted = level.time;
				return AIFunc_BattleHuntStart( cs );
			}
			// otherwise, go into ambush mode
			if ( AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].real_visible_pos, cs->takeCoverPos ) ) {
				VectorCopy( cs->vislist[cs->enemyNum].real_visible_pos, cs->combatGoalOrigin );
				return AIFunc_BattleAmbushStart( cs );
			}
			// couldn't find a spot, so just stay here?
			VectorCopy( cs->bs->origin, cs->combatGoalOrigin );
			VectorCopy( cs->bs->origin, cs->takeCoverPos );
			return AIFunc_BattleAmbushStart( cs );
		}
	} else    // assume we know where other entities are
	{
		VectorCopy( followent->s.pos.trBase, destorg );
		dist = Distance( cs->bs->origin, destorg );
	}
	//
	// if the enemy is inside a CONTENTS_DONOTENTER brush, and we are close enough, stop chasing them
	if ( AICast_EntityVisible( cs, cs->enemyNum, qtrue ) && VectorDistance( cs->bs->origin, destorg ) < 384 ) {
		if ( trap_PointContents( destorg, cs->enemyNum ) & ( CONTENTS_DONOTENTER | CONTENTS_DONOTENTER_LARGE ) ) {
			// just stay here, and hope they move out of the brush without finding a spot where they can hit us but we can't hit them
			return NULL;
		}
	}
	//
	// is there someone else we can go for instead?
	numEnemies = AICast_ScanForEnemies( cs, enemies );
	if ( numEnemies == -1 ) { // query mode
		return NULL;
	} else if ( numEnemies == -2 )     { // inspection may be required
		char *retval;
		// TTimo: gcc: suggest () around assignment used as truth value
		if ( ( retval = AIFunc_InspectFriendlyStart( cs, enemies[0] ) ) ) {
			return retval;
		}
	}
	AICast_ChooseWeapon( cs, qtrue );   // enable special weapons, if we cant get them, change back
	if ( numEnemies > 0 ) {
		int i;
		for ( i = 0; i < numEnemies; i++ ) {
			if ( enemies[i] != cs->enemyNum && AICast_CheckAttack( cs, enemies[i], qfalse ) ) {
				cs->enemyNum = enemies[i];
				return AIFunc_BattleStart( cs );
			}
		}
	}
	AICast_ChooseWeapon( cs, qfalse );

	//
	// if we only recently saw them, face them
	//
	/* RF: disabled 9/19/01, characters like boss2 supersoldier are forced to walk backwards and look wierd
	if (cs->vislist[cs->enemyNum].visible_timestamp > level.time - 3000) {
		AICast_AimAtEnemy( cs );	// be ready for an attack if they become visible again
		//if (cs->attributes[ATTACK_CROUCH] > 0.1) {	// crouching for combat
		//	cs->attackcrouch_time = level.time + 1000;
		//}
	}
	*/

	//
	// Lob a Grenade?
	// if we haven't thrown a grenade in a bit, go into "grenade flush mode"
	if ( ( lastGrenadeFlush > level.time || lastGrenadeFlush < level.time - 5000 ) &&
		 ( cs->aiState >= AISTATE_COMBAT ) &&
		 ( cs->castScriptStatus.castScriptEventIndex < 0 ) &&
		 ( cs->startGrenadeFlushTime < level.time - 3000 ) &&
		 ( COM_BitCheck( cs->bs->cur_ps.weapons, WP_GRENADE_LAUNCHER ) ) &&
		 ( AICast_GotEnoughAmmoForWeapon( cs, WP_GRENADE_LAUNCHER ) ) &&
		 ( cs->weaponFireTimes[WP_GRENADE_LAUNCHER] < level.time - (int)( 1000 + aicast_skillscale * 1000 ) ) &&
		 ( ( cs->weaponNum == WP_GRENADE_LAUNCHER ) || !( cs->castScriptStatus.scriptFlags & SFL_NOCHANGEWEAPON ) ) &&
		 ( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].visible_pos ) > 100 ) &&
		 ( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].visible_pos ) < 1400 ) ) {
		// try and flush them out with a grenade
		//G_Printf("pineapple?\n");
		return AIFunc_GrenadeFlushStart( cs );
	} else if ( ( lastGrenadeFlush > level.time || lastGrenadeFlush < level.time - 5000 ) &&
				( cs->aiState >= AISTATE_COMBAT ) &&
				( cs->castScriptStatus.castScriptEventIndex < 0 ) &&
				( cs->startGrenadeFlushTime < level.time - 3000 ) &&
				( COM_BitCheck( cs->bs->cur_ps.weapons, WP_GRENADE_PINEAPPLE ) ) &&
				( AICast_GotEnoughAmmoForWeapon( cs, WP_GRENADE_PINEAPPLE ) ) &&
				( cs->weaponFireTimes[WP_GRENADE_PINEAPPLE] < level.time - (int)( 1000 + aicast_skillscale * 1000 ) ) &&
				( ( cs->weaponNum == WP_GRENADE_PINEAPPLE ) || !( cs->castScriptStatus.scriptFlags & SFL_NOCHANGEWEAPON ) ) &&
				( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].visible_pos ) > 100 ) &&
				( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].visible_pos ) < 1400 ) ) {
		// try and flush them out with a grenade
		//G_Printf("pineapple?\n");
		return AIFunc_GrenadeFlushStart( cs );
	}
	//
	// Flaming Zombie? Shoot flames while running
	if ( ( cs->aiCharacter == AICHAR_ZOMBIE ) &&
		 ( IS_FLAMING_ZOMBIE( ent->s ) ) &&
		 ( fabs( cs->ideal_viewangles[YAW] - cs->viewangles[YAW] ) < 5 ) ) {
		if ( fabs( sin( ( level.time + cs->entityNum * 314 ) / 1000 ) * cos( ( level.time + cs->entityNum * 267 ) / 979 ) ) < 0.5 ) {
			ent->s.time = level.time + 800;
		}
	}
	// reload?
	AICast_IdleReload( cs );

	if ( dist < chaseDist ) {
		return NULL;
	}

	//
	// go to them
	//
	// ...........................................................
	// Do the movement..
	//
	// move straight to them if we can
	if (    !moved && cs->leaderNum < 0 &&
			( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE || g_entities[cs->entityNum].waterlevel > 1 ) &&
			AICast_EntityVisible( cs, cs->enemyNum, qtrue ) ) {
		aicast_predictmove_t move;
		vec3_t dir;
		bot_input_t bi;
		usercmd_t ucmd;
		trace_t tr;

		// trace will eliminate most unsuccessful paths
		trap_Trace( &tr, cs->bs->origin, NULL, NULL, followent->r.currentOrigin, cs->entityNum, g_entities[cs->entityNum].clipmask );
		if ( tr.entityNum == followent->s.number ) {
			// try walking straight to them
			VectorSubtract( followent->r.currentOrigin, cs->bs->origin, dir );
			VectorNormalize( dir );
			if ( !ent->waterlevel ) {
				dir[2] = 0;
			}
			//trap_EA_Move(cs->entityNum, dir, 400);
			trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi );
			VectorCopy( dir, bi.dir );
			bi.speed = 400;
			bi.actionflags = 0;
			AICast_InputToUserCommand( cs, &bi, &ucmd, bs->cur_ps.delta_angles );
			AICast_PredictMovement( cs, 5, 2.0, &move, &ucmd, cs->enemyNum );

			if ( move.stopevent == PREDICTSTOP_HITENT ) { // success!
				trap_EA_Move( cs->entityNum, dir, 400 );
				// RF, if we are really close, we might be stuck on a corner, so randomly move sideways
				if ( ( VectorLength( followent->client->ps.velocity ) < 50 ) && ( dist < 10 + ( sqrt( cs->bs->cur_ps.maxs[0] * cs->bs->cur_ps.maxs[0] * 8.0 ) / 2.0 + sqrt( followent->client->ps.maxs[0] * followent->client->ps.maxs[0] * 8.0 ) / 2.0 ) ) ) {
					// if the box trace is unsuccessful
					trap_Trace( &tr, cs->bs->origin, cs->bs->cur_ps.mins, cs->bs->cur_ps.maxs, followent->r.currentOrigin, cs->entityNum, g_entities[cs->entityNum].clipmask );
					if ( tr.entityNum != followent->s.number ) {
						if ( level.time % 6000 < 2000 ) {
							trap_EA_MoveRight( cs->entityNum );
						} else {
							trap_EA_MoveLeft( cs->entityNum );
						}
					}
				}
				vectoangles( dir, cs->ideal_viewangles );
				cs->ideal_viewangles[2] *= 0.5;
				moved = qtrue;
			} else {    // clear movement
				//trap_EA_Move(cs->entityNum, dir, 0);
			}
		}
	}
	//
	// if they are visible, but not attackable, look for a spot where we can attack them, and head
	// for there. This should prevent AI's getting stuck in a bunch.
	if ( !moved && cs->weaponNum >= WP_LUGER && cs->weaponNum <= WP_AKIMBO && cs->attributes[TACTICAL] >= 0.1 ) {
		//
		// check for another movement we should be making
		if ( cs->obstructingTime > level.time ) {
			AICast_MoveToPos( cs, cs->obstructingPos, -1 );
			moved = qtrue;
		}
		//
		if ( cs->leaderNum >= 0 ) {
			if ( cs->combatGoalTime < level.time ) {
				if ( cs->attackSpotTime < level.time ) {
					cs->attackSpotTime = level.time + 500 + rand() % 500;
					if ( trap_AAS_FindAttackSpotWithinRange( cs->entityNum, cs->leaderNum, cs->enemyNum, MAX_LEADER_DIST, AICAST_TFL_DEFAULT, cs->combatGoalOrigin ) ) {
						cs->combatGoalTime = level.time + 2000;
					}
				}
			}
			if ( cs->combatGoalTime > level.time ) {
				if ( Distance( cs->combatGoalOrigin, g_entities[cs->leaderNum].r.currentOrigin ) > MAX_LEADER_DIST ) {
					// go find a new combatSpot
					cs->combatGoalTime = 0;
				} else {
					// go to the combat spot
					moveresult = AICast_MoveToPos( cs, cs->combatGoalOrigin, -1 );
					if ( moveresult && moveresult->failure ) {    // no path, so go back to idle behaviour
						cs->combatGoalTime = 0;
					} else {
						moved = qtrue;
						if ( Distance( cs->bs->origin, cs->combatGoalOrigin ) < 32 ) {
							cs->combatGoalTime = 0;
						}
					}
				}
			} else {
				// we can't find a way to get to our enemy, so go back to our leader if outside range
				// do we need to go to our leader?
				if ( Distance( cs->bs->origin, g_entities[cs->leaderNum].r.currentOrigin ) > MAX_LEADER_DIST ) {
					return AIFunc_ChaseGoalStart( cs, cs->leaderNum, AICAST_LEADERDIST_MAX, qtrue );
				}
			}
		} else {
			if ( cs->combatGoalTime < level.time ) {
				if ( cs->attackSpotTime < level.time ) {
					cs->attackSpotTime = level.time + 500 + rand() % 500;
					if ( trap_AAS_FindAttackSpotWithinRange( cs->entityNum, cs->entityNum, cs->enemyNum, 512, AICAST_TFL_DEFAULT, cs->combatGoalOrigin ) ) {
						cs->combatGoalTime = level.time + 2000;
					}
				}
			}
			if ( cs->combatGoalTime > level.time ) {
				// go to the combat spot
				moveresult = AICast_MoveToPos( cs, cs->combatGoalOrigin, -1 );
				if ( moveresult && moveresult->failure ) {    // no path, so go back to idle behaviour
					cs->combatGoalTime = 0;
				} else {
					moved = qtrue;
					if ( Distance( cs->bs->origin, cs->combatGoalOrigin ) < 32 ) {
						cs->combatGoalTime = 0;
						cs->attackSpotTime = level.time + 12000;    // dont go to another combatspot for some time, prevent repetitive behaviour
					}
				}
			}
		}
	}
	// just go to them
	if ( !moved && cs->leaderNum < 0 ) {
		moveresult = AICast_MoveToPos( cs, destorg, cs->enemyNum );
		if ( moveresult && moveresult->failure ) {    // no path, so try and hude from them
			// pausetime has expired, so go into ambush mode
			if ( AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].real_visible_pos, cs->takeCoverPos ) ) {
				// wait in ambush, for them to return
				VectorCopy( cs->bs->origin, cs->combatGoalOrigin );
				return AIFunc_BattleAmbushStart( cs );
			}
			// HACK, help lopers get out of bad spots
			if ( cs->aiCharacter == AICHAR_LOPER ) {
				cs->weaponFireTimes[WP_MONSTER_ATTACK2] = 0;
			}
			// couldn't find a spot, so just stay here?
			if ( cs->bs->areanum ) {  // if our area is valid
				VectorCopy( cs->bs->origin, cs->combatGoalOrigin );
				VectorCopy( cs->bs->origin, cs->takeCoverPos );
				return AIFunc_BattleAmbushStart( cs );
			}
		} else {
			moved = qtrue;
		}
	}
	//
	// slow down real close to the goal, so we don't go passed it
	cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, chaseDist );
	//
	// ...........................................................
	// speed up over some time
	#define BATTLE_CHASE_ACCEL_TIME     300
	if ( ( cs->attributes[RUNNING_SPEED] > 170 ) && ( cs->weaponNum != WP_GAUNTLET ) && ( level.time < ( cs->startBattleChaseTime + BATTLE_CHASE_ACCEL_TIME ) ) ) {
		float ideal;

		ideal = 0.5 + 0.5 * ( 1.0 - ( (float)( ( cs->startBattleChaseTime + BATTLE_CHASE_ACCEL_TIME ) - level.time ) / BATTLE_CHASE_ACCEL_TIME ) );
		if ( ideal < cs->speedScale ) {
			cs->speedScale = ideal;
		}
	}
	//
	// if we are going to reach them soon, predict the attack
	{
		float simTime = 1.5;
		aicast_predictmove_t move;
		float moveDist;
		vec3_t vec;
		//
		if ( cs->weaponNum == WP_GAUNTLET ) {
			simTime = 0.5;
		}
		//
		AICast_PredictMovement( cs, 1, simTime, &move, &cs->lastucmd, cs->enemyNum );
		VectorSubtract( move.endpos, cs->bs->origin, vec );
		moveDist = VectorNormalize( vec );
		//
		if ( cs->weaponNum == WP_GAUNTLET ) {
			if ( move.stopevent == PREDICTSTOP_HITENT ) {
				AICast_AimAtEnemy( cs );
				trap_EA_Attack( bs->client );
				cs->bFlags |= BFL_ATTACKED;
			}
		}
		//
		// do we went to play a diving animation into a cover position?
		else if (   ( ( cs->attributes[TACTICAL] > 0.85 ) && ( cs->aiFlags & AIFL_ROLL_ANIM ) && !client->ps.torsoTimer && !client->ps.legsTimer && ( cs->lastRollMove < level.time - 800 ) && ( move.numtouch == 0 ) && ( moveDist > simTime * cs->attributes[RUNNING_SPEED] * 0.98 ) && move.groundEntityNum == ENTITYNUM_WORLD ) &&
					( AICast_CheckAttackAtPos( cs->entityNum, cs->enemyNum, move.endpos, cs->attackcrouch_time > level.time, qfalse ) ) ) {
			cs->takeCoverTime = 0;
			return AIFunc_BattleRollStart( cs, vec );
		}
		//
		else if ( cs->aiFlags & AIFL_FLIP_ANIM && cs->lastRollMove < level.time - 800 && !client->ps.torsoTimer && cs->castScriptStatus.castScriptEventIndex < 0 && move.numtouch == 0 && moveDist > simTime * cs->attributes[RUNNING_SPEED] * 0.9 && move.groundEntityNum == ENTITYNUM_WORLD && cs->attackcrouch_time < level.time ) {
			int destarea, simarea, starttravel, simtravel;
			// if we'll be closer after the move, proceed
			destarea = BotPointAreaNum( destorg );
			simarea = BotPointAreaNum( move.endpos );
			starttravel = trap_AAS_AreaTravelTimeToGoalArea( cs->bs->areanum, cs->bs->origin, destarea, cs->travelflags );
			simtravel = trap_AAS_AreaTravelTimeToGoalArea( simarea, move.endpos, destarea, cs->travelflags );
			if ( simtravel < starttravel ) {
				return AIFunc_FlipMoveStart( cs, vec );
			}
		}
		// slow down? so we don't go too far from behind the obstruction which is protecting us
		else if ( !( cs->aiFlags & AIFL_WALKFORWARD ) && ( VectorDistance( cs->bs->origin, g_entities[cs->enemyNum].s.pos.trBase ) < AICast_WeaponRange( cs, cs->weaponNum ) ) &&
				  ( cs->obstructingTime < level.time ) && ( cs->attributes[TACTICAL] > 0.1 ) &&
				  ( AICast_VisibleFromPos( cs->vislist[cs->enemyNum].visible_pos, cs->enemyNum, move.endpos, cs->entityNum, qfalse ) ) ) {
			// start a crouch attack?
			//if (cs->attributes[ATTACK_CROUCH] > 0.1) {
			//	cs->attackcrouch_time = level.time + 3000;
			//else
			cs->attackcrouch_time = 0;
			if ( cs->bs->cur_ps.viewheight > cs->bs->cur_ps.crouchViewHeight && cs->attributes[RUNNING_SPEED] * cs->speedScale > 120 ) {
				cs->speedScale = 120.0 * cs->attributes[RUNNING_SPEED];
			}
			// also face them, ready for the attack
			if ( cs->attributes[RUNNING_SPEED] > 140 ) {
				AICast_AimAtEnemy( cs );
			}
			/* disabled, causes them to use up ammo in the clip before they get visible
			if ((cs->castScriptStatus.scriptNoAttackTime < level.time) && (cs->noAttackTime < level.time)) {
				// if we are using a bullet weapon, start firing now
				switch (cs->weaponNum) {
				case WP_MP40:
				case WP_VENOM:
				case WP_THOMPSON:
				case WP_STEN:	//----(SA)	added
					trap_EA_Attack(cs->entityNum);
				}
			}
			*/
		}
	}

	// reload?
	AICast_IdleReload( cs );

	return NULL;
}

/*
============
AIFunc_BattleChaseStart()
============
*/
char *AIFunc_BattleChaseStart( cast_state_t *cs ) {
	cs->startBattleChaseTime = level.time;
	cs->combatGoalTime = 0;
	cs->battleChaseMarker = -99;
	cs->battleChaseMarkerDir = 1;
	// don't wait too long before taking cover, if we just aborted one
	if ( cs->takeCoverTime > level.time ) {
		cs->takeCoverTime = level.time + 1500 + rand() % 500;
	}
	//
	// start a crouch attack?
	if ( cs->attributes[ATTACK_CROUCH] > 0.1 ) {
		cs->aiFlags |= AIFL_ATTACK_CROUCH;
	} else {
		cs->aiFlags &= ~AIFL_ATTACK_CROUCH;
	}
	//
	cs->aifunc = AIFunc_BattleChase;
	return "AIFunc_BattleChase";
}

/*
============
AIFunc_AvoidDanger()
============
*/
char *AIFunc_AvoidDanger( cast_state_t *cs ) {
	bot_state_t *bs;
	vec3_t destorg, vec;
	float dist;
	int enemies[MAX_CLIENTS], numEnemies, i;
	qboolean shouldAttack;
	gentity_t *ent;
	trace_t tr;
	vec3_t end;
	gentity_t *danger;

	// we need to move towards it
	bs = cs->bs;
	ent = g_entities + cs->entityNum;
	//
	// TODO: if we are on fire, play the correct torso animation
	if ( ent->s.onFireEnd > level.time ) {
		// set the animation, and a short timer, but long enough to last until the next frame
		//if (g_cheats.integer) G_Printf( "TODO: torso onfire animation\n" );
	}
	//
	// is the danger gone?
	if ( cs->dangerEntityValidTime < level.time ) {
		return AIFunc_DefaultStart( cs );
	}
	//
	// special case: if it's a grenade, and it's going to land near us with some time left before it
	// explodes, try and kick it back
	//
	danger = &g_entities[cs->dangerEntity];
	if ( ent->s.onFireEnd < level.time ) {
		if ( ( danger->s.weapon == WP_GRENADE_LAUNCHER || danger->s.weapon == WP_GRENADE_PINEAPPLE ) &&
			 ( danger->nextthink - level.time > 1500 ) &&
			 ( level.lastGrenadeKick < level.time - 3000 ) &&
			 ( cs->aiFlags & AIFL_CATCH_GRENADE ) &&
			 !( danger->flags & FL_AI_GRENADE_KICK ) ) {
			// if it was thrown by a friend of ours, leave it alone
			if ( !AICast_SameTeam( cs, danger->r.ownerNum ) ) {
				if ( G_PredictMissile( danger, danger->nextthink - level.time, cs->takeCoverPos, qfalse ) ) {
					// make sure it's a valid position, and drop it down to the ground
					cs->takeCoverPos[2] += -ent->r.mins[2] + 12;
					VectorCopy( cs->takeCoverPos, end );
					end[2] -= 90;
					trap_Trace( &tr, cs->takeCoverPos, ent->r.mins, ent->r.maxs, end, cs->entityNum, MASK_SOLID );
					VectorCopy( tr.endpos, cs->takeCoverPos );
					if ( !tr.startsolid && ( tr.fraction < 1.0 ) &&
						 VectorDistance( cs->bs->origin, cs->takeCoverPos ) < cs->attributes[RUNNING_SPEED] * 0.0004 * ( danger->nextthink - level.time - 2000 ) ) {

						// check for a clear path to the grenade
						trap_Trace( &tr, cs->bs->origin, ent->r.mins, ent->r.maxs, cs->takeCoverPos, cs->entityNum, MASK_SOLID );

						if ( VectorDistance( tr.endpos, cs->takeCoverPos ) < 8 ) {
							danger->flags |= FL_AI_GRENADE_KICK;
							ent->flags |= FL_AI_GRENADE_KICK;
							level.lastGrenadeKick = level.time;
							return AIFunc_GrenadeKickStart( cs );   // we should decide our course of action within this start function (dive or return grenade)
						}
					}
				}
			}
			// if it's really close to us, and we're heading for it, may as well pick it up
			if ( VectorLength( danger->s.pos.trDelta ) < 10 && VectorDistance( danger->r.currentOrigin, cs->bs->origin ) < 128 &&
				 ( level.lastGrenadeKick < level.time - 3000 ) &&
				 ( cs->aiFlags & AIFL_CATCH_GRENADE ) ) {
				vec3_t vec;
				VectorSubtract( danger->r.currentOrigin, cs->bs->origin, vec );
				if ( DotProduct( vec, cs->bs->velocity ) > 0 ) {
					danger->flags |= FL_AI_GRENADE_KICK;
					ent->flags |= FL_AI_GRENADE_KICK;
					level.lastGrenadeKick = level.time;
					return AIFunc_GrenadeKickStart( cs );   // we should decide our course of action within this start function (dive or return grenade)
				}
			}
		}
	}
	//
	if ( g_entities[cs->dangerEntity].inuse ) {
		// is our current destination still safe?
		if ( Distance( cs->dangerEntityPos, cs->takeCoverPos ) < cs->dangerDist &&
			 AICast_VisibleFromPos( cs->dangerEntityPos, cs->dangerEntity, cs->takeCoverPos, cs->entityNum, qfalse ) ) {
			//G_Printf("current coverPos is dangerous, looking for a better place..\n" );
			if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
				// just run away from it ???
			}
		}
	} else {
		// the entity isn't here anymore, so stop hiding
		cs->dangerEntityValidTime = -1;
		return AIFunc_DefaultStart( cs );
	}
	//
	VectorCopy( cs->takeCoverPos, destorg );
	VectorSubtract( destorg, cs->bs->origin, vec );
	vec[2] *= 0.2;
	dist = VectorLength( vec );
	//
	shouldAttack = qfalse;
	if ( ent->s.onFireEnd < level.time ) {
		// look for things we should attack
		numEnemies = AICast_ScanForEnemies( cs, enemies );
		if ( numEnemies > 0 ) {
			// default to the first known enemy, overwrite if we find a clearer shot
			cs->enemyNum = enemies[0];
			//
			for ( i = 0; i < numEnemies; i++ ) {
				if ( AICast_CheckAttack( cs, enemies[i], qfalse ) || AICast_CheckAttack( AICast_GetCastState( enemies[i] ), cs->entityNum, qfalse ) ) {
					cs->enemyNum = enemies[i];
					shouldAttack = qtrue;
					break;
				} else if ( cs->enemyNum < 0 ) {
					cs->lastEnemy = enemies[i];
				}
			}
		}
	}
	//
	// if we are now safe from the danger, stop running away
	if ( cs->dangerEntity >= MAX_CLIENTS && Distance( cs->dangerEntityPos, cs->bs->origin ) > cs->dangerDist * 1.5 ) {
		// don't move, wait for danger to pass
	} else
	// are we close enough to the goal?
	if ( dist > 8 ) {
		moveresult = AICast_MoveToPos( cs, destorg, -1 );
		if ( moveresult ) {
			//if the movement failed
			if ( moveresult->failure || moveresult->blocked ) {
				//reset the avoid reach, otherwise bot is stuck in current area
				trap_BotResetAvoidReach( bs->ms );
				if ( g_entities[cs->dangerEntity].inuse ) {
					// find a better spot?
					AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos );
				} else {
					VectorCopy( cs->bs->origin, cs->takeCoverPos );
				}
			}
		}
		if ( ent->s.onFireEnd < level.time ) {
			// slow down real close to the goal, so we don't go passed it
			cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, 0 );
		}
		//
		// pretend we can still see them while we run to our hide pos, this way they are less likely
		// to forget about their enemy once they get there
		if ( ent->s.onFireEnd < level.time && cs->enemyNum >= 0 && cs->vislist[cs->enemyNum].real_visible_timestamp && ( cs->vislist[cs->enemyNum].real_visible_timestamp > level.time - 10000 ) ) {
			AICast_UpdateVisibility( &g_entities[cs->entityNum], &g_entities[cs->enemyNum], qfalse, cs->vislist[cs->enemyNum].real_visible_timestamp == cs->vislist[cs->enemyNum].lastcheck_timestamp );
		}

	} else {
		// set our origin as the hidepos, that way if we are still in danger, we should find a better spot
		VectorCopy( cs->bs->origin, cs->takeCoverPos );
		// check for a movement we should be making
		if ( cs->obstructingTime > level.time ) {
			AICast_MoveToPos( cs, cs->obstructingPos, -1 );
		}

		// if we are on fire, never stop
		if ( ent->s.onFireEnd > level.time ) {
			VectorCopy( cs->bs->origin, cs->dangerEntityPos );
			cs->dangerEntityValidTime = level.time + 10000;
		}

	}
	//
	// if we should be attacking something on our way
	if ( shouldAttack ) {
		//attack the enemy if possible
		AICast_ProcessAttack( cs );
	} else { //if (dist < 48) {
		// if we've recently been in a fight, look towards the enemy
		if ( cs->lastEnemy >= 0 ) {
			// if we only just recently saw them, face that direction
			if ( cs->vislist[cs->lastEnemy].visible_timestamp > ( level.time - 20000 ) ) {
				vec3_t dir;
				//
				VectorSubtract( cs->vislist[cs->lastEnemy].visible_pos, cs->bs->origin, dir );
				VectorNormalize( dir );
				vectoangles( dir, cs->ideal_viewangles );
			}
		}
	}

	// reload?
	AICast_IdleReload( cs );

	return NULL;
}

/*
============
AIFunc_AvoidDangerStart()
============
*/
char *AIFunc_AvoidDangerStart( cast_state_t *cs ) {
	//
	//if (!AICast_CanMoveWhileFiringWeapon( cs->weaponNum )) {
	// always run to the cover point
	cs->attackcrouch_time = 0;
	//}
	// make sure we move if we are allowed (scripting will overwrite this if necessary)
	cs->castScriptStatus.scriptNoMoveTime = 0;
	// resume following once danger has gone
	cs->castScriptStatus.scriptGotoId = -1;
	//
	cs->aifunc = AIFunc_AvoidDanger;
	return "AIFunc_AvoidDanger";
}

/*
============
AIFunc_BattleTakeCover()
============
*/
char *AIFunc_BattleTakeCover( cast_state_t *cs ) {
	bot_state_t *bs;
	vec3_t destorg, vec;
	float dist, moveDist;
	int enemies[MAX_CLIENTS], numEnemies, i;
	qboolean shouldAttack;
	aicast_predictmove_t move;
	int     *ammo;
	gclient_t   *client = &level.clients[cs->entityNum];
	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// shit??
		}
		// go to a position that cannot be seen from the dangerPos
		cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
		cs->attackcrouch_time = 0;
		return AIFunc_AvoidDangerStart( cs );
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}

	// we need to move towards it
	bs = cs->bs;
	//
	// note: removing this will cause problems down below!
	if ( cs->enemyNum < 0 ) {
		return AIFunc_IdleStart( cs );
	}
	//
	if ( VectorLength( cs->takeCoverPos ) < 1 ) {
		dist = 0;
	} else {
		VectorCopy( cs->takeCoverPos, destorg );
		VectorSubtract( destorg, cs->bs->origin, vec );
		vec[2] *= 0.2;
		dist = VectorLength( vec );
	}
	//
	// look for things we should attack
	// if we are out of ammo, we shouldn't bother trying to attack (and we should keep hiding)
	ammo = cs->bs->cur_ps.ammo;
	shouldAttack = qfalse;
	numEnemies = AICast_ScanForEnemies( cs, enemies );
	if ( numEnemies == -1 ) { // query mode
		return NULL;
	} else if ( numEnemies == -2 )     { // inspection may be required
		char *retval;
		// TTimo: gcc: suggest () around assignment used as truth value
		if ( ( retval = AIFunc_InspectFriendlyStart( cs, enemies[0] ) ) ) {
			return retval;
		}
	} else if ( numEnemies == -3 )     { // bullet impact
		if ( cs->aiState < AISTATE_COMBAT ) {
			return AIFunc_InspectBulletImpactStart( cs );
		}
	} else if ( numEnemies == -4 )     { // audible event
		if ( cs->aiState < AISTATE_COMBAT ) {
			return AIFunc_InspectAudibleEventStart( cs, cs->audibleEventEnt );
		}
	} else if ( numEnemies > 0 )     {

		if ( AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum ) ) {
			// default to the first known enemy, overwrite if we find a clearer shot
			cs->enemyNum = enemies[0];
			//
			for ( i = 0; i < numEnemies; i++ ) {
				if ( AICast_CheckAttack( cs, enemies[i], qfalse ) || AICast_CheckAttack( AICast_GetCastState( enemies[i] ), cs->entityNum, qfalse ) ||
					 AICast_EntityVisible( AICast_GetCastState( enemies[i] ), cs->entityNum, qtrue ) ) {
					if ( ( cs->aiFlags & AIFL_WALKFORWARD ) || ( dist <= 12 ) ) {
						// we are at our hidepos, abort!
						cs->enemyNum = enemies[i];
						return AIFunc_BattleStart( cs );
					} else {
						shouldAttack = qtrue;   // fire at them as we go
					}
				} else if ( cs->enemyNum < 0 ) {
					cs->lastEnemy = enemies[i];
				}
			}

		} else {
			AICast_ChooseWeapon( cs, qfalse );
			//
			if ( dist <= 12 ) {
				if ( !AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum ) ) {
					// NO AMMO LEFT!!
					// hide?
					if ( AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].visible_pos, cs->takeCoverPos ) ) {
						// go to a position that cannot be seen from the last place we saw the enemy, and wait there for some time
						cs->takeCoverTime = level.time + 2000 + rand() % 3000;
						//return AIFunc_BattleTakeCoverStart( cs );
					}
				}
			}
		}

	}
	//
	//if (!shouldAttack)
	// always do this check, if our destination sucks, abort it
	{
		// if the enemy can see our hide position, find a better spot
		if ( AICast_VisibleFromPos( cs->vislist[cs->enemyNum].visible_pos, cs->enemyNum, cs->takeCoverPos, bs->entitynum, qfalse ) ) {
			if ( !AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].visible_pos, cs->takeCoverPos ) ) {
				// shit!! umm.. try and fire?
				return AIFunc_BattleStart( cs );
			} else {    // recalc distance
				VectorCopy( cs->takeCoverPos, destorg );
				VectorSubtract( destorg, cs->bs->origin, vec );
				vec[2] *= 0.2;
				dist = VectorLength( vec );
			}
		} else if ( dist < 8 )     {
			// if they can see us, find a better spot
			if ( AICast_EntityVisible( AICast_GetCastState( cs->enemyNum ), cs->entityNum, qtrue ) || AICast_CheckAttack( AICast_GetCastState( cs->enemyNum ), cs->entityNum, qfalse ) ) {
				if ( !AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].visible_pos, cs->takeCoverPos ) ) {
					// shit!! umm.. try and fire?
					return AIFunc_BattleStart( cs );
				} else {    // recalc distance
					VectorCopy( cs->takeCoverPos, destorg );
					VectorSubtract( destorg, cs->bs->origin, vec );
					vec[2] *= 0.2;
					dist = VectorLength( vec );
				}
			}
		}
		//cs->takeCoverTime = level.time + 1000;
	}
	//
	// pretend we can still see them while we run to our hide pos, this way they are less likely
	// to forget about their enemy once they get there
// DISABLED: doesn't work well with new AI system
	//if (cs->enemyNum >= 0 && cs->vislist[cs->enemyNum].real_visible_timestamp && (cs->vislist[cs->enemyNum].real_visible_timestamp > level.time - 2000)) {
	//	AICast_UpdateVisibility( &g_entities[cs->entityNum], &g_entities[cs->enemyNum], qfalse, cs->vislist[cs->enemyNum].real_visible_timestamp == cs->vislist[cs->enemyNum].lastcheck_timestamp );
	//}
	//
	memset( &move, 0, sizeof( move ) );
	//
	// are we close enough to the goal?
	if ( VectorLength( cs->takeCoverPos ) > 1 && dist > 8 ) {
		const float simTime = 1.5;
		float enemyDist;
		//
		// we haven't reached it yet, make sure we at least wait there for a few seconds after arriving
		cs->takeCoverTime = level.time + 2000 + rand() % 2000;
		//
		moveresult = AICast_MoveToPos( cs, destorg, -1 );
		if ( moveresult ) {
			//if the movement failed
			if ( moveresult->failure ) {
				//reset the avoid reach, otherwise bot is stuck in current area
				trap_BotResetAvoidReach( bs->ms );
				// couldn't get there, so stop trying to get there
				VectorClear( cs->takeCoverPos );
				dist = 0;
			}
			//
			if ( moveresult->blocked ) {
				// abort the TakeCover
				VectorClear( cs->takeCoverPos );
				dist = 0;
			}
		}
		//
		// if we are going to bump into something soon, abort it
		AICast_PredictMovement( cs, 1, simTime, &move, &cs->lastucmd, -1 );
		enemyDist = Distance( cs->bs->origin, g_entities[cs->enemyNum].s.pos.trBase );
		VectorSubtract( move.endpos, cs->bs->origin, vec );
		moveDist = VectorNormalize( vec );
		//
		if (    ( move.numtouch && move.touchents[0] < aicast_maxclients )    // hit something
				// or moved closer to the enemy
				||  (   ( enemyDist < 128 )
						&&  ( ( enemyDist - 1 ) > ( Distance( move.endpos, g_entities[cs->enemyNum].s.pos.trBase ) ) ) ) ) {
			// abort the manouver
			VectorClear( cs->takeCoverPos );
			dist = 0;
		}
		//
		// do we want to play a rolling animation into a cover position?
		else if (   ( cs->aiFlags & AIFL_DIVE_ANIM && !client->ps.torsoTimer && cs->castScriptStatus.castScriptEventIndex < 0 && cs->lastRollMove < level.time - 800 && move.numtouch == 0 && ( moveDist > simTime * cs->attributes[RUNNING_SPEED] * 0.98 ) && move.groundEntityNum == ENTITYNUM_WORLD ) &&
					( shouldAttack && !AICast_VisibleFromPos( g_entities[cs->enemyNum].s.pos.trBase, cs->enemyNum, move.endpos, cs->entityNum, qfalse ) ) ) {
			VectorClear( cs->takeCoverPos );    // stay there when done rolling
			return AIFunc_BattleDiveStart( cs, vec );
		}
		//
		// we should slow down on approaching the destination point
		else if ( dist < 64 ) {
			cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, 0 );
		}
		//
		// if they cant see us, then stay here
		if ( !( cs->aiFlags & AIFL_MISCFLAG1 ) && !AICast_VisibleFromPos( cs->vislist[cs->enemyNum].real_visible_pos, cs->enemyNum, move.endpos, cs->entityNum, qfalse )
			 &&  !AICast_VisibleFromPos( cs->vislist[cs->enemyNum].real_visible_pos, cs->enemyNum, cs->bs->origin, cs->entityNum, qfalse )
			 &&  trap_AAS_PointAreaNum( move.endpos ) ) { // make sure the endpos is in a valid area
			VectorCopy( move.endpos, cs->takeCoverPos );
			dist = 0;
			cs->aiFlags |= AIFL_MISCFLAG1;  // dont do this again
		}
		//
		if ( cs->aiFlags & AIFL_FLIP_ANIM && cs->lastRollMove < level.time - 800 && !client->ps.torsoTimer && cs->castScriptStatus.castScriptEventIndex < 0 && move.numtouch == 0 && moveDist > simTime * cs->attributes[RUNNING_SPEED] * 0.9 && move.groundEntityNum == ENTITYNUM_WORLD && cs->attackcrouch_time < level.time ) {
			int destarea, simarea, starttravel, simtravel;
			// if we'll be closer after the move, proceed
			destarea = BotPointAreaNum( destorg );
			simarea = BotPointAreaNum( move.endpos );
			starttravel = trap_AAS_AreaTravelTimeToGoalArea( cs->bs->areanum, cs->bs->origin, destarea, cs->travelflags );
			simtravel = trap_AAS_AreaTravelTimeToGoalArea( simarea, move.endpos, destarea, cs->travelflags );
			if ( simtravel < starttravel ) {
				return AIFunc_FlipMoveStart( cs, vec );
			}
		}
		// set crouching status
		//if (dist && (cs->thinkFuncChangeTime < level.time - 2000) && (cs->crouchHideFlag || cs->aiFlags & AIFL_ATTACK_CROUCH)) {
		if ( cs->crouchHideFlag || ( ( cs->thinkFuncChangeTime < level.time - 2000 ) && ( cs->aiFlags & AIFL_ATTACK_CROUCH ) ) ) {
			cs->attackcrouch_time = level.time + 1000;
		}

	} else {
		//
		// have we been Taking Cover for enough time?
		if ( level.time > cs->takeCoverTime ) {
			return AIFunc_DefaultStart( cs );
		}
		//
		// check for a movement we should be making
		if ( cs->obstructingTime > level.time ) {
			VectorClear( cs->takeCoverPos );
			AICast_MoveToPos( cs, cs->obstructingPos, -1 );
		}
		// if we have some enemies that we can attack immediately (without going anywhere to chase them)
		if ( shouldAttack ) {
			return AIFunc_BattleStart( cs );
		}
		// if we have some enemies in sight, but they can't attack us, flee if possible, otherwise if we are not afraid, go attack them
		else if ( numEnemies ) {

			// are they reloading? if so we should attack!
			if (    g_entities[cs->entityNum].client->ps.weaponDelay < 100
					&&  g_entities[cs->enemyNum].client->ps.weaponDelay > 1100 ) {
				if ( AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum ) && AICast_WeaponUsable( cs, cs->weaponNum ) ) {
					return AIFunc_BattleStart( cs );
				}
			}

			// we can't hit them and they cant hit us, so dont bother doing anything

			//if (!AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].visible_pos, cs->takeCoverPos )) {
			//if (!AICast_WantsToTakeCover(cs, qfalse))
			//return AIFunc_BattleStart( cs );
			//}
		}
		// do we need to go to our leader?
		else if ( cs->leaderNum >= 0 && Distance( cs->bs->origin, g_entities[cs->leaderNum].r.currentOrigin ) > MAX_LEADER_DIST ) {
			// wait until we've been hiding for long enough
			if ( level.time > cs->takeCoverTime ) {
				return AIFunc_ChaseGoalStart( cs, cs->leaderNum, AICAST_LEADERDIST_MAX, qtrue );
			}
		}

		// else, crouch while we hide
		if ( cs->attributes[ATTACK_CROUCH] > 0.1 || cs->crouchHideFlag ) {
			cs->attackcrouch_time = level.time + 1000;
		}
	}
	//
	// if we should be attacking something on our way
	if ( shouldAttack ) {
		vec3_t vec, dir;
		float dist;
		//
		// if they are close, and we're heading for them, we should abort this manouver
		VectorSubtract( g_entities[cs->enemyNum].client->ps.origin, bs->origin, vec );
		if ( ( dist = VectorNormalize( vec ) ) < 256 ) {
			VectorCopy( bs->velocity, vec );
			vec[2] = 0;
			if ( VectorNormalize2( vec, dir ) > 20 ) {    // we are moving
				if ( DotProduct( dir, vec ) > 0.4 ) {
					// abort
					return AIFunc_BattleStart( cs );
				}
			}
		}
		//
		// if the enemy can see our hide position, abort the manouver
		if ( ( cs->thinkFuncChangeTime < level.time - 1000 ) && ( AICast_VisibleFromPos( g_entities[cs->enemyNum].client->ps.origin, cs->enemyNum, cs->takeCoverPos, bs->entitynum, qfalse ) ) ) {
			// abort
			return AIFunc_BattleStart( cs );
		}
		//
		// if we are tactical and can crouch, do so
		if ( !move.numtouch && ( cs->thinkFuncChangeTime < level.time - 2000 ) && ( dist > 128 ) && cs->attributes[TACTICAL] > 0.4 && cs->attributes[ATTACK_CROUCH] > 0.1 &&
			 ( cs->attackcrouch_time >= level.time ) ) {
			cs->attackcrouch_time = level.time + 1000;
		}
		//
		//attack the enemy if possible
		AICast_ProcessAttack( cs );
		//
	} else /*if (dist < 48)*/ {
		// if we've recently been in a fight, look towards the enemy
		if ( cs->enemyNum >= 0 ) {
			AICast_AimAtEnemy( cs );
		} else if ( cs->lastEnemy >= 0 )     {
			// if we are not moving, face them
			if ( VectorLength( cs->bs->cur_ps.velocity ) < 50 ) {
				vec3_t dir;
				//
				VectorSubtract( cs->vislist[cs->lastEnemy].visible_pos, cs->bs->origin, dir );
				VectorNormalize( dir );
				vectoangles( dir, cs->ideal_viewangles );
			}
		} else if ( !cs->crouchHideFlag )     { // no enemy, and no need to crouch, so stop crouching
			//if (cs->attackcrouch_time > level.time + 1000) {
			//	cs->attackcrouch_time = level.time + 1000;
			//}
		}
		// reload?
		AICast_IdleReload( cs );
	}

	return NULL;
}

/*
============
AIFunc_BattleTakeCoverStart()
============
*/
char *AIFunc_BattleTakeCoverStart( cast_state_t *cs ) {
// debugging
#ifdef DEBUG
//	if (cs->attributes[AGGRESSION] >= 1.0)
//		AICast_Printf( 0, "AI taking cover with full aggression!\n" );
#endif

	if ( !AICast_CanMoveWhileFiringWeapon( cs->weaponNum ) ) {
		// always run to the cover point
		cs->attackcrouch_time = 0;
		cs->aiFlags &= ~AIFL_ATTACK_CROUCH;
	} else {
		// if we arent crouching, start crouching soon after we start retreating
		if ( cs->attributes[ATTACK_CROUCH] > 0.1 ) {
			cs->aiFlags |= AIFL_ATTACK_CROUCH;
		} else {
			cs->aiFlags &= ~AIFL_ATTACK_CROUCH;
		}
		cs->attackcrouch_time = 0;
	}

	// miscflag1 used to set predicted point as our goal, so we dont keep setting this over and over
	cs->aiFlags &= ~AIFL_MISCFLAG1;

	cs->aifunc = AIFunc_BattleTakeCover;
	return "AIFunc_BattleTakeCover";
}

/*
============
AIFunc_GrenadeFlush()
============
*/
char *AIFunc_GrenadeFlush( cast_state_t *cs ) {
	vec3_t dir;
	gentity_t   *followent, *grenade, *ent;
	bot_state_t *bs;
	vec3_t destorg, endPos;
	qboolean moved = qfalse;
	int hitclient;
	//qboolean attacked = qfalse; // TTimo: unused
	float dist, oldyaw;
	int grenadeType;
	int     *ammo;

	bs = cs->bs;
	ent = &g_entities[cs->entityNum];
	//
	// if we are throwing the grenade, keep holding down fire

	ammo = cs->bs->cur_ps.ammo;
	if ( AICast_GotEnoughAmmoForWeapon( cs, WP_GRENADE_LAUNCHER ) ) {
		grenadeType = WP_GRENADE_LAUNCHER;
	} else if ( AICast_GotEnoughAmmoForWeapon( cs, WP_GRENADE_PINEAPPLE ) ) {
		grenadeType = WP_GRENADE_PINEAPPLE;
	} else { // not enough ammo, abort
		return AIFunc_DefaultStart( cs );
	}

	// (SA) probably read the fweapon from t
	if ( cs->grenadeFlushFiring ) {
		// are we still moving?
		if ( VectorLength( cs->bs->cur_ps.velocity ) ) {
			// keep waiting
			// pause for a bit, so the grenade comes out correctly
			cs->lockViewAnglesTime = level.time + 1200;
			if ( cs->castScriptStatus.scriptNoMoveTime < level.time + 1200 ) {
				cs->castScriptStatus.scriptNoMoveTime = level.time + 1200;
			}
			return NULL;
		}
		if ( cs->weaponFireTimes[cs->grenadeFlushFiring] < cs->thinkFuncChangeTime ) {
			// have we switched weapons?
			if ( cs->weaponNum != cs->grenadeFlushFiring ) {
				// damn
				hitclient = -1;
			} else {
				// keep checking it's ok
				CalcMuzzlePoints( ent, grenadeType );
				// fire a dummy grenade
				grenade = weapon_grenadelauncher_fire( ent, WP_GRENADE_LAUNCHER );
				// check to see what will happen
				hitclient = AICast_SafeMissileFire( grenade, grenade->nextthink - level.time, cs->enemyNum, destorg, cs->entityNum, endPos );
				// kill the grenade
				G_FreeEntity( grenade );
				if ( hitclient != 1 ) {   // it wont hit them, abort
					hitclient = -1;     // a miss is as bad as a friendly kill
				}
			}
			if ( hitclient == -1 ) {  // doh
				//G_Printf("aborted grenade\n");
				cs->castScriptStatus.scriptNoMoveTime = 0;
				cs->lockViewAnglesTime = 0;
				AICast_ChooseWeapon( cs, qfalse );
				return AIFunc_DefaultStart( cs );
			}
			if ( !cs->bs->cur_ps.grenadeTimeLeft ) {
				// hold fire button down
				trap_EA_Attack( bs->client );
				cs->bFlags |= BFL_ATTACKED;
			}
			cs->lockViewAnglesTime = level.time + 500;
			return NULL;
		}
		// the grenade/pineapple has been released!
		cs->lockViewAnglesTime = -1;
		cs->startGrenadeFlushTime = level.time + 2000 + rand() % 2000;    // dont throw one again for a bit
		return AIFunc_DefaultStart( cs );
	}
	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// shit??
		}
		// go to a position that cannot be seen from the dangerPos
		cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
		cs->attackcrouch_time = 0;
		return AIFunc_AvoidDangerStart( cs );
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}
	//
	if ( cs->weaponNum && ( cs->castScriptStatus.scriptFlags & SFL_NOCHANGEWEAPON ) ) {
		return AIFunc_IdleStart( cs );
	}
	//
	if ( cs->enemyNum < 0 ) {
		return AIFunc_IdleStart( cs );
	}
	//
	// if we have started a script, abort the grenade flush
	if ( cs->castScriptStatus.castScriptEventIndex >= 0 ) {
		return AIFunc_IdleStart( cs );
	}
	// trying for too long?
	if ( cs->startGrenadeFlushTime < level.time - 4000 ) {
		cs->startGrenadeFlushTime = level.time;
		return AIFunc_IdleStart( cs );
	}
	//
	followent = &g_entities[cs->enemyNum];
	//
	// if the entity is not ready yet
	if ( !followent->inuse ) {
		// if it's a connecting client, wait
		if ( !(   ( cs->enemyNum < MAX_CLIENTS )
				  && (   ( followent->client && followent->client->pers.connected == CON_CONNECTING )
						 || ( level.time < 3000 ) ) ) ) { // they don't exist anymore, stop attacking
			cs->enemyNum = -1;
		}
		return AIFunc_IdleStart( cs );
	}
	//
	// if we can see them, go back to an attack state after some time
	if (    AICast_CheckAttack( cs, cs->enemyNum, qfalse )
			&&  cs->obstructingTime < level.time ) { // give us some time to throw the grenade, otherwise go back to attack state
												   //if ((cs->grenadeFlushEndTime > 0 && cs->grenadeFlushEndTime < level.time)) {
		//G_Printf("aborting, enemy is attackable\n");
		return AIFunc_BattleStart( cs );
		//} else if (cs->grenadeFlushEndTime < 0) {
		//	cs->grenadeFlushEndTime = level.time + 1500;
		//}
		//attack the enemy if possible
		//AICast_ProcessAttack( cs );
		//attacked = qtrue;
	} else {
		// not visible, go to their previously visible position
		if ( !cs->vislist[cs->enemyNum].visible_timestamp || Distance( bs->origin, cs->vislist[cs->enemyNum].real_visible_pos ) < 16 ) {
			// we're done attacking, go back to default state, which in turn will recall previous state
			//
			// face the direction they currently are from this position (bit of a hack, but it looks best)
			VectorSubtract( g_entities[cs->enemyNum].client->ps.origin, cs->vislist[cs->enemyNum].visible_pos, dir );
			vectoangles( dir, cs->ideal_viewangles );
			//
			//G_Printf("aborting, reached visible pos\n");
			return AIFunc_DefaultStart( cs );
		}
	}

	// is there someone else we can go for instead?
	numEnemies = AICast_ScanForEnemies( cs, enemies );
	if ( numEnemies == -1 ) { // query mode
		return NULL;
	} else if ( numEnemies == -2 )     { // inspection may be required
		char *retval;
		// TTimo: gcc: suggest () around assignment used as truth value
		if ( ( retval = AIFunc_InspectFriendlyStart( cs, enemies[0] ) ) ) {
			return retval;
		}
	} else if ( !( cs->bFlags & BFL_ATTACKED ) && numEnemies > 0 )       {
		int i;
		for ( i = 0; i < numEnemies; i++ ) {
			if ( enemies[i] != cs->enemyNum && AICast_CheckAttack( cs, enemies[i], qfalse ) ) {
				cs->enemyNum = enemies[i];
				//G_Printf("aborting, other enemy\n");
				return AIFunc_BattleStart( cs );
			}
		}
	}

	if ( followent->client ) { // go to the last visible position
		VectorCopy( cs->vislist[cs->enemyNum].visible_pos, destorg );
	} else    // assume we know where other entities are
	{
		VectorCopy( followent->s.pos.trBase, destorg );
	}
	//
	dist = VectorDistance( destorg, cs->bs->origin );
	//
	if ( cs->vislist[cs->enemyNum].lastcheck_timestamp > cs->vislist[cs->enemyNum].real_visible_timestamp ||
		 dist > 128 ) {
		//
		// go to them
		//
		if ( followent->client && followent->health <= 0 ) {
			cs->enemyNum = -1;
			//G_Printf("aborting, enemy dead\n");
			return AIFunc_DefaultStart( cs );
		}
		//
		// ...........................................................
		// Do the movement..
		//
		// move straight to them if we can
		if ( ( cs->leaderNum < 0 ) &&
			 ( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE || g_entities[cs->entityNum].waterlevel > 1 ) ) {
			aicast_predictmove_t move;
			vec3_t dir;
			bot_input_t bi;
			usercmd_t ucmd;
			trace_t tr;

			// trace will eliminate most unsuccessful paths
			trap_Trace( &tr, cs->bs->origin, g_entities[cs->entityNum].r.mins, g_entities[cs->entityNum].r.maxs, followent->r.currentOrigin, cs->entityNum, g_entities[cs->entityNum].clipmask );
			if ( tr.entityNum == followent->s.number ) {
				// try walking straight to them
				VectorSubtract( followent->r.currentOrigin, cs->bs->origin, dir );
				VectorNormalize( dir );
				if ( !ent->waterlevel ) {
					dir[2] = 0;
				}
				//trap_EA_Move(cs->entityNum, dir, 400);
				trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi );
				VectorCopy( dir, bi.dir );
				bi.speed = 400;
				bi.actionflags = 0;
				AICast_InputToUserCommand( cs, &bi, &ucmd, bs->cur_ps.delta_angles );
				AICast_PredictMovement( cs, 5, 2.0, &move, &ucmd, cs->enemyNum );

				if ( move.stopevent == PREDICTSTOP_HITENT ) { // success!
					trap_EA_Move( cs->entityNum, dir, 400 );
					vectoangles( dir, cs->ideal_viewangles );
					cs->ideal_viewangles[2] *= 0.5;
					moved = qtrue;
				} else {    // clear movement
					//trap_EA_Move(cs->entityNum, dir, 0);
				}
			}
		}
		// just go to them
		if ( !moved ) {
			moveresult = AICast_MoveToPos( cs, destorg, cs->enemyNum );
			if ( moveresult && moveresult->failure ) {    // no path, so go back to idle behaviour
				cs->enemyNum = -1;
				//G_Printf("aborting, movement failure\n");
				return AIFunc_DefaultStart( cs );
			} else {
				moved = qtrue;
			}
		}
	}
	//
	// ...........................................................
	// if we throw a grenade from here, will it get their last visible position?
	//
	CalcMuzzlePoints( ent, grenadeType );
	// fire a dummy grenade
	grenade = weapon_grenadelauncher_fire( ent, WP_GRENADE_LAUNCHER );
	// check to see what will happen
	hitclient = AICast_SafeMissileFire( grenade, grenade->nextthink - level.time, cs->enemyNum, destorg, cs->entityNum, endPos );
	// kill the grenade
	G_FreeEntity( grenade );
	//if (!attacked)
	//	cs->weaponNum = grenadeType;	// select grenade launcher
	// set our angles for the next frame
	oldyaw = cs->ideal_viewangles[YAW];
	AICast_AimAtEnemy( cs );
	// if we can't see them, keep facing our movement dir, but use the pitch information
	if ( !AICast_EntityVisible( cs, cs->enemyNum, qtrue ) ) {
		cs->ideal_viewangles[YAW] = oldyaw;
	}

	if ( hitclient == 1 ) {
		// it will hit their last visible position
		// give us some time to aim and adjust
		if ( cs->thinkFuncChangeTime < level.time - 200 ) {
			cs->bFlags |= BFL_ATTACKED;
			cs->weaponNum = grenadeType;    // select grenade launcher
			cs->grenadeFlushFiring = cs->weaponNum;
			// pause for a bit, so the grenade comes out correctly
			cs->lockViewAnglesTime = level.time + 1200;
			if ( cs->castScriptStatus.scriptNoMoveTime < level.time + 1200 ) {
				cs->castScriptStatus.scriptNoMoveTime = level.time + 1200;
			}
			return NULL;
		}
	} else if ( hitclient == -1 ) {
		// hit a friendly
		cs->startGrenadeFlushTime = level.time + 3000;  // don't try again for a while
		//G_Printf("aborting, too dangerous\n");
		return AIFunc_DefaultStart( cs );
	} else if ( hitclient == -2 ) {
		// went too far, so angle down a bit
		cs->ideal_viewangles[PITCH] += 15 * random();
	} else {
		if ( cs->thinkFuncChangeTime < level.time - 200 ) {
			// if it went reasonably close to them, but safe from us, then fire away
			if ( Distance( endPos, cs->bs->origin ) > 100 + Distance( endPos, g_entities[cs->enemyNum].r.currentOrigin ) ) {
				trap_EA_Attack( bs->client );
				cs->bFlags |= BFL_ATTACKED;
				cs->weaponNum = grenadeType;    // select grenade launcher
				cs->grenadeFlushFiring = cs->weaponNum;
				// pause for a bit, so the grenade comes out correctly
				cs->lockViewAnglesTime = level.time + 1200;
				if ( cs->castScriptStatus.scriptNoMoveTime < level.time + 1200 ) {
					cs->castScriptStatus.scriptNoMoveTime = level.time + 1200;
				}
				return NULL;
			}
		}
		cs->ideal_viewangles[PITCH] += -10 * random();
	}
	//
	return NULL;
}

/*
============
AIFunc_GrenadeFlushStart()
============
*/
char *AIFunc_GrenadeFlushStart( cast_state_t *cs ) {
	lastGrenadeFlush = level.time; // + rand()%2000;
	cs->startGrenadeFlushTime = level.time;
	cs->grenadeFlushEndTime = -1;
	cs->lockViewAnglesTime = 0;
	cs->combatGoalTime = 0;
	cs->grenadeFlushFiring = qfalse;
	// don't wait too long before taking cover, if we just aborted one
	if ( cs->takeCoverTime > level.time + 1000 ) {
		cs->takeCoverTime = level.time + 500 + rand() % 500;
	}
	//
	cs->aifunc = AIFunc_GrenadeFlush;
	return "AIFunc_GrenadeFlush";
}

/*
============
AIFunc_BattleMG42()
============
*/
char *AIFunc_BattleMG42( cast_state_t *cs ) {
	bot_state_t *bs;
	gentity_t *mg42, *ent;
	vec3_t angles, vec, bestangles;
	qboolean unmount = qfalse;

	mg42 = &g_entities[cs->mountedEntity];
	ent = &g_entities[cs->entityNum];
	bs = cs->bs;

	// have we dismounted the MG42?
	if ( !ent->active ) {
		return AIFunc_DefaultStart( cs );
	}

	// are we waiting to dismount
	if ( cs->aiFlags & AIFL_DISMOUNTING ) {
		// face straight forward
		VectorCopy( mg42->s.angles, cs->ideal_viewangles );
		// only dismount when facing forwards
		if ( fabs( AngleDifference( mg42->s.angles[YAW], cs->viewangles[YAW] ) ) < 10 ) {
			// try and unmount
			Cmd_Activate_f( ent );
		}
		return NULL;
	}

/* RF, disabled this since the gun will unmount them when it is destroyed
	// if the mg42 is dead, dismount
	if (mg42->health <= 0) {
		cs->aiFlags |= AIFL_DISMOUNTING;
		AICast_ScriptEvent( cs, "forced_mg42_unmount", "destroyed" );
		return NULL;
	}
*/
	// if enemy is dead, stop attacking them
	if ( g_entities[cs->enemyNum].health <= 0 ) {
		cs->enemyNum = -1;
	}

	//if no enemy, or our current enemy isn't attackable, look for a better enemy
	if ( cs->enemyNum >= 0 ) {
		if ( cs->vislist[cs->enemyNum].real_visible_timestamp && cs->vislist[cs->enemyNum].real_visible_timestamp > ( level.time - 5000 ) ) {
			VectorSubtract( cs->vislist[cs->enemyNum].real_visible_pos, mg42->r.currentOrigin, vec );
		} else if ( cs->vislist[cs->enemyNum].visible_timestamp && cs->vislist[cs->enemyNum].visible_timestamp > ( level.time - 5000 ) ) {
			VectorSubtract( cs->vislist[cs->enemyNum].visible_pos, mg42->r.currentOrigin, vec );
		} else { // just aim straight forward
			AngleVectors( mg42->s.angles, vec, NULL, NULL );
		}

		VectorNormalize( vec );
		vectoangles( vec, angles );
		angles[PITCH] = AngleNormalize180( angles[PITCH] );
		VectorCopy( angles, bestangles );
	}

	// check for enemy outside harc
	if (    cs->enemyNum < 0 ||
			!AICast_CheckAttack( cs, cs->enemyNum, qfalse ) ||
			( fabs( AngleDifference( angles[YAW], mg42->s.angles[YAW] ) ) > mg42->harc ) ||
			( angles[PITCH] < 0 && angles[PITCH] + 5 < -mg42->varc ) ||
			( angles[PITCH] > 0 && angles[PITCH] - 5 > 5.0 ) ) {
		qboolean shouldAttack;

		// look for a better enemy
		numEnemies = AICast_ScanForEnemies( cs, enemies );
		shouldAttack = qfalse;
		if ( numEnemies > 0 ) {
			int i;
			// default to the first known enemy, overwrite if we find a clearer shot
			cs->enemyNum = enemies[0];
			//
			// unmount unless we find an enemy within harc
			unmount = qtrue;
			//
			for ( i = 0; i < numEnemies; i++ ) {
				/*
				if (!cs->vislist[enemies[i]].real_visible_timestamp ||
					(cs->vislist[enemies[i]].real_visible_timestamp < level.time - 2000)) {
					// we can't see them, ignore them
					continue;
				}
				*/

				// if they are in the range
				if ( cs->vislist[enemies[i]].real_visible_timestamp > ( level.time - 5000 ) ) {
					VectorSubtract( cs->vislist[enemies[i]].real_visible_pos, mg42->r.currentOrigin, vec );
				} else {
					VectorSubtract( cs->vislist[enemies[i]].visible_pos, mg42->r.currentOrigin, vec );
				}
				VectorNormalize( vec );
				vectoangles( vec, angles );
				angles[PITCH] = AngleNormalize180( angles[PITCH] );
				if ( !(  ( fabs( AngleDifference( angles[YAW], mg42->s.angles[YAW] ) ) > mg42->harc ) ||
						 ( angles[YAW] < 0 && angles[YAW] + 2 < -mg42->varc ) ||
						 ( angles[YAW] > 0 && angles[YAW] - 2 > 5.0 ) ) ) {
					//
					// found someone inside harc, so dont unmount
					unmount = qfalse;
					//
					if ( AICast_CheckAttack( cs, enemies[i], qfalse ) ) {
						VectorCopy( angles, bestangles );
						cs->enemyNum = enemies[i];
						shouldAttack = qtrue;
						break;
					} else if ( AICast_CheckAttack( cs, enemies[i], qtrue ) ) {
						// keep firing at anything behind solids, in case they find a position where they can shoot us, but our checkattack() doesn't find a clear shot
						VectorCopy( angles, bestangles );
						cs->enemyNum = enemies[i];
						shouldAttack = qtrue;
					}
				}
			}
		}

		if ( !shouldAttack ) {
			// keep firing at anything behind solids, in case they find a position where they can shoot us, but our checkattack() doesn't find a clear shot
			if ( cs->enemyNum < 0 || !AICast_CheckAttack( cs, cs->enemyNum, qtrue ) ||
				 (   !cs->vislist[cs->enemyNum].real_visible_timestamp ||
					 ( cs->vislist[cs->enemyNum].real_visible_timestamp < level.time - 2000 ) ) ) {
				// face straight forward
				cs->ideal_viewangles[PITCH] = 0;
				return NULL;
			}
		}

		// if we had possible enemies, but couldnt find one to attack, then dismount now
		if ( unmount ) {
			AICast_ScriptEvent( cs, "forced_mg42_unmount", NULL );
			if ( !( cs->aiFlags & AIFL_DENYACTION ) ) {
				cs->aiFlags |= AIFL_DISMOUNTING;
				return NULL;
			}
		}
	}
	//
	// hold down fire, and track them
	//
	// TODO: play a special "holding mg42" torso animation
	//
	VectorCopy( angles, cs->ideal_viewangles );
	if ( cs->triggerReleaseTime < level.time ) {
		trap_EA_Attack( bs->client );
		cs->bFlags |= BFL_ATTACKED;

		if ( cs->triggerReleaseTime < level.time - 3000 ) {
			cs->triggerReleaseTime = level.time + 700 + rand() % 700;
		}
	}
	//
	return NULL;
}

/*
============
AIFunc_BattleMG42Start()
============
*/
char *AIFunc_BattleMG42Start( cast_state_t *cs ) {
	cs->aiFlags &= ~AIFL_DISMOUNTING;
	//
	cs->aifunc = AIFunc_BattleMG42;
	return "AIFunc_BattleMG42";
}

/*
============
AIFunc_InspectBody()

  go up to the enemy, and have a good look at them, randomly taunt them
============
*/
char *AIFunc_InspectBody( cast_state_t *cs ) {
	bot_state_t *bs;
	vec3_t destorg, enemyOrg;
	//
	// stop crouching
	cs->attackcrouch_time = 0;
	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// shit??
		}
		// go to a position that cannot be seen from the dangerPos
		cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
		cs->attackcrouch_time = 0;
		return AIFunc_AvoidDangerStart( cs );
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}
	//
	// if running a script
	if ( cs->castScriptStatus.castScriptEventIndex >= 0 ) {
		cs->enemyNum = -1;
		return AIFunc_IdleStart( cs );
	}
	//
	bs = cs->bs;
	//
	if ( cs->enemyNum < 0 ) {
		return AIFunc_IdleStart( cs );
	}
	//
	// look for things we should attack
	numEnemies = AICast_ScanForEnemies( cs, enemies );
	if ( numEnemies == -1 ) { // query mode
		return NULL;
	} else if ( numEnemies == -2 )     { // inspection may be required
		char *retval;
		// TTimo: gcc: suggest () around assignment used as truth value
		if ( ( retval = AIFunc_InspectFriendlyStart( cs, enemies[0] ) ) ) {
			return retval;
		}
	} else if ( numEnemies == -3 )     { // bullet impact
		if ( cs->aiState < AISTATE_COMBAT ) {
			return AIFunc_InspectBulletImpactStart( cs );
		}
	} else if ( numEnemies == -4 )     { // audible event
		if ( cs->aiState < AISTATE_COMBAT ) {
			return AIFunc_InspectAudibleEventStart( cs, cs->audibleEventEnt );
		}
	} else if ( numEnemies > 0 )     {
		cs->enemyNum = enemies[0];  // just attack the first one
		return AIFunc_BattleStart( cs );
	}
	//
	VectorCopy( cs->vislist[cs->enemyNum].visible_pos, enemyOrg );
	if ( ( cs->inspectBodyTime < 0 ) && ( Distance( cs->bs->origin, enemyOrg ) > 64 ) ) {
		// if they were gibbed, don't go all the way
		if ( g_entities[cs->enemyNum].health < GIB_HEALTH && ( Distance( cs->bs->origin, enemyOrg ) < 180 ) ) {
			cs->inspectBodyTime = level.time + 1000 + rand() % 1000;
			trap_EA_Gesture( cs->entityNum );
			G_AddEvent( &g_entities[cs->entityNum], EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[cs->aiCharacter].soundScripts[ORDERSSOUNDSCRIPT] ) );
		}
		// walk to them
		if ( cs->movestate != MS_CROUCH ) {
			cs->movestate = MS_WALK;
		}
		cs->movestateType = MSTYPE_TEMPORARY;
		//
		moveresult = AICast_MoveToPos( cs, enemyOrg, -1 );
		//if the movement failed
		if ( moveresult && ( moveresult->failure || moveresult->blocked ) ) {
			//reset the avoid reach, otherwise bot is stuck in current area
			trap_BotResetAvoidReach( bs->ms );
			// couldn't get there, so stop trying to get there
			cs->enemyNum = -1;
			return AIFunc_IdleStart( cs );
		}
		if ( Distance( cs->bs->origin, enemyOrg ) < 180 ) {
			// look down at them
			VectorSubtract( enemyOrg, cs->bs->origin, destorg );
			destorg[2] -= 20;
			VectorNormalize( destorg );
			vectoangles( destorg, cs->ideal_viewangles );
		}
	} else if ( cs->inspectBodyTime < 0 ) {
		// just reached them
		cs->inspectBodyTime = level.time + 1000 + rand() % 1000;
		trap_EA_Gesture( cs->entityNum );
		G_AddEvent( &g_entities[cs->entityNum], EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[cs->aiCharacter].soundScripts[ORDERSSOUNDSCRIPT] ) );
	} else if ( cs->inspectBodyTime < level.time ) {
		vec3_t vec;
		VectorSubtract( cs->startOrigin, cs->bs->origin, vec );
		vec[2] = 0;
		// ready to go back to start position
		if ( VectorLength( vec ) > 64 ) {
			if ( cs->movestate != MS_CROUCH ) {
				cs->movestate = MS_WALK;
			}
			cs->movestateType = MSTYPE_TEMPORARY;
			moveresult = AICast_MoveToPos( cs, cs->startOrigin, -1 );
			//if the movement failed
			if ( moveresult && ( moveresult->failure || moveresult->blocked ) ) {
				//reset the avoid reach, otherwise bot is stuck in current area
				trap_BotResetAvoidReach( bs->ms );
				// couldn't get there, so stop trying to get there
				cs->enemyNum = -1;
				return AIFunc_IdleStart( cs );
			}
			// stay looking at them for a bit after starting to walk back
			if ( cs->inspectBodyTime + 750 > level.time ) {
				// look down at them
				VectorSubtract( enemyOrg, cs->bs->origin, destorg );
				destorg[2] -= 20;
				VectorNormalize( destorg );
				vectoangles( destorg, cs->ideal_viewangles );
			}
		} else {
			cs->attackSNDtime = level.time;
			cs->enemyNum = -1;
			return AIFunc_IdleStart( cs );
		}
	}
	//
	return NULL;
}

/*
============
AIFunc_InspectBodyStart()
============
*/
char *AIFunc_InspectBodyStart( cast_state_t *cs ) {
	static int lastInspect;
	//
	// if an inspection was already started not long ago, forget it
	if ( lastInspect <= level.time && lastInspect > level.time - 1000 ) {
		cs->inspectBodyTime = 1;    // go back to start position
	} else {
		lastInspect = level.time;
		cs->inspectBodyTime = -1;
	}
	cs->aifunc = AIFunc_InspectBody;
	return "AIFunc_InspectBody";
}

/*
============
AIFunc_GrenadeKick()
============
*/
char *AIFunc_GrenadeKick( cast_state_t *cs ) {
	bot_state_t *bs;
	vec3_t destorg, vec;
	float dist, speed;
	int enemies[MAX_CLIENTS], numEnemies, i;
	qboolean shouldAttack;
	int     *ammo;
	gentity_t *danger;
	gentity_t *ent;
	vec3_t end;
	trace_t tr;
	vec3_t dir;
	int weapon;

	// !!! NOTE: the only way control should pass out of here, is by calling AIFunc_DefaultStart()

	ent = &g_entities[cs->entityNum];
	danger = &g_entities[cs->dangerEntity];

	weapon = cs->grenadeKickWeapon;

	// just to be sure, give us the grenade launcher
	//ent->client->ps.weapons |= (1 << weapon);

	//
	// NOTE: ignore all danger, since we are trying to solve the situation anyway
	//
	// we need to move towards it
	bs = cs->bs;
	//
	// are we throwing it back?
	if ( cs->grenadeFlushFiring ) {
		// wait until the animation is done
		if ( !ent->client->ps.legsTimer ) {
			return AIFunc_DefaultStart( cs );
		}
		// wait till its finished
		return NULL;
		/*
		cs->weaponNum = weapon;	// select grenade launcher
		//
		if (cs->weaponFireTimes[weapon] < cs->thinkFuncChangeTime) {
			if (!cs->bs->cur_ps.grenadeTimeLeft) {
				// hold fire button down
				AICast_AimAtEnemy( cs );
				trap_EA_Attack(bs->client);
				cs->bFlags |= BFL_ATTACKED;
			}
			//
			return NULL;
		}
		// the grenade has been released!
		//
		// modify the explode time
		g_entities[ent->grenadeFired].nextthink = ent->grenadeExplodeTime;
		if (g_entities[ent->grenadeFired].nextthink < level.time + 200) {	// cut them some slack
			g_entities[ent->grenadeFired].nextthink = level.time + 200 + rand()%500;
		}
		// make sure no-one tries to throw this back again (hot potatoe syndrome)
		g_entities[ent->grenadeFired].flags |= FL_AI_GRENADE_KICK;
		//
		cs->grenadeFlushFiring = qfalse;
		cs->lockViewAnglesTime = -1;
		cs->startGrenadeFlushTime = level.time + 2000 + rand()%2000;	// dont throw one again for a bit
		level.lastGrenadeKick = level.time;
		return AIFunc_DefaultStart( cs );
		*/
	}
	//
/*
	// have we caught the grenade?
	if (!(ent->flags & FL_AI_GRENADE_KICK)) {
		// select grenades
		cs->weaponNum = weapon;	// select grenade launcher
		AICast_AimAtEnemy( cs );
		// hold fire
		trap_EA_Attack(bs->client);
		cs->bFlags |= BFL_ATTACKED;
		cs->grenadeFlushFiring = qtrue;
		//
		return NULL;
	}
*/
	//
	// is it about to explode in our face?
	if ( level.time > danger->nextthink - (int)( 2.0 * VectorDistance( cs->bs->origin, danger->r.currentOrigin ) ) ) {
		// abort!!
		if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// shit??
		}
		// go to a position that cannot be seen from the dangerPos
		cs->takeCoverTime = danger->nextthink + 1000;
		cs->attackcrouch_time = 0;
		level.lastGrenadeKick = level.time;
		return AIFunc_AvoidDangerStart( cs );
	}
	//
	/*
	// are we close enough to start crouching?
	if (danger->s.pos.trDelta[2] < 40 && VectorDistance( danger->r.currentOrigin, cs->bs->origin ) < 48 && (danger->r.currentOrigin[2] < cs->bs->origin[2]) &&
		VectorLength(danger->s.pos.trDelta) < 40) {
		// crouch to pick it up
		cs->attackcrouch_time = level.time + 300;
	}
	*/
	cs->attackcrouch_time = 0;  // animation is played from standing start
	//
	// are we close enough to pick it up?
	if ( /*cs->grenadeGrabFlag <= 0 || */
		( danger->s.pos.trDelta[2] < 20 && VectorDistance( danger->r.currentOrigin, cs->bs->origin ) < 48 && ( danger->r.currentOrigin[2] < cs->bs->origin[2] ) &&
		  VectorLength( danger->s.pos.trDelta ) < 50 ) ) {
		//
		// we have a choice here, either pick up and return, or just kick it
//		if ((cs->grenadeGrabFlag == -1) || (cs->grenadeGrabFlag == qtrue && level.time > danger->nextthink - 2000)) {	// kick

		// play the kick anim
		if ( cs->grenadeGrabFlag == qtrue ) {
			AICast_AimAtEnemy( cs );
			// play the kick anim
			BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_KICKGRENADE, qfalse, qtrue );
			cs->grenadeGrabFlag = -1;
			// stop the grenade from moving away
			danger->s.pos.trDelta[0] = 0;
			danger->s.pos.trDelta[1] = 0;
			if ( danger->s.pos.trDelta[2] > 0 ) {
				danger->s.pos.trDelta[2] = 0;
			}
		} else if ( ent->client->ps.legsTimer < 800 ) {
			// send the grenade on its way
			cs->grenadeFlushFiring = qtrue;
			AngleVectors( cs->viewangles, dir, NULL, NULL );
			dir[2] = 0.4;
			VectorNormalize( dir );
			speed = 400;
			if ( cs->enemyNum >= 0 ) {
				speed = 1.5 * VectorDistance( danger->r.currentOrigin, g_entities[cs->enemyNum].r.currentOrigin );
				if ( speed > 650 ) {
					speed = 650;
				}
			}
			VectorScale( dir, speed, danger->s.pos.trDelta );
			danger->r.ownerNum = ent->s.number;     // we are now the owner, let it pass through us
			danger->s.pos.trTime = level.time - 50;         // move a bit on the very first frame
			VectorCopy( danger->r.currentOrigin, danger->s.pos.trBase );
			danger->s.pos.trType = TR_GRAVITY;
			SnapVector( danger->s.pos.trDelta );                // save net bandwidth
		}
/*
		} else { // throw

			if (cs->grenadeGrabFlag == qtrue) {
				AICast_AimAtEnemy( cs );
				// play the pickup anim
				BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_PICKUPGRENADE, qfalse, qtrue );
				cs->grenadeGrabFlag = qfalse;
				// stop the grenade from moving away
				danger->s.pos.trDelta[0] = 0;
				danger->s.pos.trDelta[1] = 0;
				if (danger->s.pos.trDelta[2] > 0) {
					danger->s.pos.trDelta[2] = 0;
				}
			} else if (ent->client->ps.legsTimer < 400) {
				// send the grenade on its way
				cs->grenadeFlushFiring = qtrue;
				AngleVectors( cs->viewangles, dir, NULL, NULL );
				dir[2] = 0.4;
				VectorNormalize( dir );
				speed = 500;
				if (cs->enemyNum >= 0) {
					speed = 2*VectorDistance(danger->r.currentOrigin, g_entities[cs->enemyNum].r.currentOrigin);
					if (speed > 650)
						speed = 650;
				}
				VectorScale( dir, speed, danger->s.pos.trDelta );
				trap_LinkEntity( danger );
			} else if (ent->client->ps.legsTimer < 800) {
				// stop showing the grenade
				trap_UnlinkEntity( danger );
			}
		}
*/
		//
		return NULL;
	}
	//
	cs->grenadeGrabFlag = qtrue;    // we must play the anim before we can grab it
	//
	// is the danger gone?
	if ( level.time > cs->dangerEntityValidTime || !danger->inuse ) {
		return AIFunc_DefaultStart( cs );
	}
	//
	// update the predicted position of the grenade
	if ( G_PredictMissile( danger, danger->nextthink - level.time, cs->takeCoverPos, qfalse ) ) {
		// make sure it's a valid position, and drop it down to the ground
		cs->takeCoverPos[2] += -ent->r.mins[2] + 8;
		VectorCopy( cs->takeCoverPos, end );
		end[2] -= 80;
		trap_Trace( &tr, cs->takeCoverPos, ent->r.mins, ent->r.maxs, end, cs->entityNum, MASK_SOLID );
		if ( tr.startsolid ) {    // not a valid position, abort
			level.lastGrenadeKick = level.time;
			return AIFunc_DefaultStart( cs );
		}
		VectorCopy( tr.endpos, cs->takeCoverPos );
	} else {    // prediction failed, so use current position
		VectorCopy( danger->r.currentOrigin, cs->takeCoverPos );
		cs->takeCoverPos[2] += 16;  // lift it off the floor
	}

	VectorCopy( cs->takeCoverPos, destorg );
	VectorSubtract( destorg, cs->bs->origin, vec );
	//vec[2] *= 0.2;
	dist = VectorLength( vec );
	//
	// look for things we should attack
	// if we are out of ammo, we shouldn't bother trying to attack
	ammo = cs->bs->cur_ps.ammo;
	shouldAttack = qfalse;
	numEnemies = 0;
	if ( AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum ) ) {
		numEnemies = AICast_ScanForEnemies( cs, enemies );
		if ( numEnemies == -1 ) { // query mode
			return NULL;
		}
		if ( numEnemies == -2 ) { // inspection may be required
			char *retval;
			// TTimo: gcc: suggest () around assignment used as truth value
			if ( ( retval = AIFunc_InspectFriendlyStart( cs, enemies[0] ) ) ) {
				return retval;
			}
		}
		if ( numEnemies > 0 ) {
			// default to the first known enemy, overwrite if we find a clearer shot
			cs->enemyNum = enemies[0];
			//
			for ( i = 0; i < numEnemies; i++ ) {
				if ( AICast_CheckAttack( cs, enemies[i], qfalse ) || AICast_CheckAttack( AICast_GetCastState( enemies[i] ), cs->entityNum, qfalse ) ) {
					cs->enemyNum = enemies[i];
					shouldAttack = qtrue;
					break;
				} else if ( cs->enemyNum < 0 ) {
					cs->lastEnemy = enemies[i];
				}
			}
		}
	}
	//
	// are we close enough to the goal?
	if ( dist > 12 ) { // not close enough
					  //
		moveresult = AICast_MoveToPos( cs, destorg, -1 );
		if ( moveresult ) {
			//if the movement failed
			if ( moveresult->failure ) {
				//reset the avoid reach, otherwise bot is stuck in current area
				trap_BotResetAvoidReach( bs->ms );
				// couldn't get there, so stop trying to get there
				level.lastGrenadeKick = level.time;
				return AIFunc_DefaultStart( cs );
			}
			//
			if ( moveresult->blocked ) {  // abort if we get blocked at any point
				level.lastGrenadeKick = level.time;
				return AIFunc_DefaultStart( cs );
			}
		}
		// we should slow down on approaching it
		cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, 0 );
	}
/*	//
	// if we are close, put our weapon away and get ready to catch it
	if (VectorDistance( danger->r.currentOrigin, cs->bs->origin ) < 128) {
		// put weapon away, select grenades
		// FIXME: does this fail if we don't have any grenades?
		cs->weaponNum = weapon;	// select grenade launcher
		shouldAttack = qfalse;	// don't attack until we've caught it
	}
*/
	// if we should be attacking something on our way
	if ( shouldAttack ) {
		vec3_t vec, dir;

		//attack the enemy if possible
		AICast_ProcessAttack( cs );
		//
		// if they are close, and we're heading for them, we should abort this manouver
		VectorSubtract( g_entities[cs->enemyNum].client->ps.origin, bs->origin, vec );
		if ( VectorNormalize( vec ) < 64 ) {
			if ( VectorNormalize2( bs->velocity, dir ) > 20 ) {   // we are moving
				if ( DotProduct( dir, vec ) > 0 ) {
					// abort
					level.lastGrenadeKick = level.time;
					return AIFunc_DefaultStart( cs );
				}
			}
		}
	} else {
		// face the direction that the grenade is coming
		VectorSubtract( danger->r.currentOrigin, cs->bs->origin, dir );
		dir[2] = 0;
		VectorNormalize( dir );
		vectoangles( dir, cs->ideal_viewangles );
	}

	return NULL;
}

/*
=============
AIFunc_GrenadeKickStart()
=============
*/
char *AIFunc_GrenadeKickStart( cast_state_t *cs ) {
	gentity_t *danger;
	gentity_t *ent;
	//gentity_t *trav;
	//int numFriends, i;

	//G_Printf( "Excuse me, you dropped something\n" );

	ent = &g_entities[cs->entityNum];
	danger = &g_entities[cs->dangerEntity];
	// should we dive onto the grenade?
	/*
	if (danger->s.pos.trDelta[2] < 30) {
		// count the number of friends near us
		numFriends = 0;
		for (i=0, trav=g_entities; i<aicast_maxclients; i++, trav++) {
			if (!trav->inuse)
				continue;
			if (trav->aiInactive)
				continue;
			if (trav->health <= 0)
				continue;
			if (!AICast_SameTeam( cs, i ))
				continue;
			if (VectorDistance( cs->takeCoverPos, trav->r.currentOrigin ) > 200)
				continue;
			numFriends++;
		}
		// if there are enough friends around, and we have a clear path to the position, sacrifice ourself!
		if (numFriends > 2) {
			trace_t tr;
			trap_Trace( &tr, cs->bs->origin, ent->r.mins, ent->r.maxs, cs->takeCoverPos, cs->entityNum, MASK_SOLID );
			if (tr.fraction == 1.0 && !tr.startsolid) {
				return AIFunc_GrenadeDiveStart( cs );
			}
		}
	}
	*/
	//
	// we have decided to kick or throw the grenade away
	cs->grenadeKickWeapon = danger->s.weapon;
	cs->grenadeFlushFiring = qfalse;
	cs->aifunc = AIFunc_GrenadeKick;
	return "AIFunc_GrenadeKick";
}

/*
============
AIFunc_Battle()
============
*/
char *AIFunc_Battle( cast_state_t *cs ) {
	bot_moveresult_t moveresult;
	int tfl;
	bot_state_t *bs;
	gentity_t *ent, *enemy;

	ent = &g_entities[cs->entityNum];
	enemy = &g_entities[cs->enemyNum];

	// if we are not in combat mode, then go there now!
	if ( cs->aiState < AISTATE_COMBAT ) {
		AICast_StateChange( cs, AISTATE_COMBAT );   // just go straight to combat mode
	}
	//
	// do we need to avoid a danger?
	if ( cs->dangerEntityValidTime >= level.time ) {
		if ( !AICast_GetTakeCoverPos( cs, cs->dangerEntity, cs->dangerEntityPos, cs->takeCoverPos ) ) {
			// shit??
		}
		// go to a position that cannot be seen from the dangerPos
		cs->takeCoverTime = cs->dangerEntityValidTime + 1000;
		cs->attackcrouch_time = 0;
		return AIFunc_AvoidDangerStart( cs );
	}
	//
	// are we waiting for a door?
	if ( cs->doorMarkerTime > level.time - 100 ) {
		return AIFunc_DoorMarkerStart( cs, cs->doorMarkerDoor, cs->doorMarkerNum );
	}
	//
	// do we need to go to our leader?
	if ( cs->leaderNum >= 0 && Distance( cs->bs->origin, g_entities[cs->leaderNum].r.currentOrigin ) > MAX_LEADER_DIST ) {
		return AIFunc_ChaseGoalStart( cs, cs->leaderNum, AICAST_LEADERDIST_MAX, qtrue );
	}
	bs = cs->bs;
	//if no enemy
	if ( cs->enemyNum < 0 ) {
		// go back to whatever our default action is
		return AIFunc_DefaultStart( cs );
	}
	//
	if ( enemy->health <= 0 ) {
		// go back to whatever our default action is
		if ( g_entities[cs->entityNum].aiTeam == AITEAM_NAZI ) {
			return AIFunc_InspectBodyStart( cs );
		} else {
			return AIFunc_DefaultStart( cs );
		}
	}
	//
	// if we are not in a good attacking position, we should chase
	if ( !AICast_StopAndAttack( cs ) ) {
		return AIFunc_BattleChaseStart( cs );
	}
	//
	// if the enemy is no longer visible
	if (    ( cs->bs->cur_ps.weaponTime < 100 )   // if reloading, don't chase until ready
			&&  ( cs->castScriptStatus.scriptNoMoveTime < level.time )
			&&  ( /*!AICast_EntityVisible( cs, cs->enemyNum, qtrue ) ||*/ !AICast_CheckAttack( cs, cs->enemyNum, qfalse ) ) ) {

		// if we are in a void, then try to avoid so we get out of it
		if ( !cs->bs->areanum ) {
			if ( cs->obstructingTime >= level.time ) {
				// move there
				trap_EA_Move( cs->entityNum, cs->takeCoverPos, 200 );
			} else if ( AICast_GetAvoid( cs, NULL, cs->takeCoverPos, qtrue, cs->enemyNum ) ) {
				VectorSubtract( cs->takeCoverPos, cs->bs->origin, cs->takeCoverPos );
				if ( VectorNormalize( cs->takeCoverPos ) > 60 ) {
					cs->obstructingTime = level.time + 1000 + rand() % 600;
				}
				return NULL;
			}
		} else
		// if we are heading for a combatGoal, give us some time to get there
		if ( cs->combatGoalTime > level.time ) {
			if ( cs->combatGoalTime > level.time + 3000 ) {
				cs->combatGoalTime = level.time + 2000 + rand() % 1000;
				cs->combatSpotDelayTime = level.time + 4000 + rand() % 3000;
			}
		} else
		if ( cs->leaderNum >= 0 ) {
			// chase them, nothing else to do
			return AIFunc_BattleChaseStart( cs );
		} else
		// if we weren't moving, it is likely they have dodged back behind something, ready to duck out and take another
		// shot. so, we could fool them by hiding from the position we last saw them from, in the hope that when they
		// return to fire at us, we won't be in their sight.
		if (    cs->attributes[TACTICAL] > 0.3
				&&  cs->attributes[AGGRESSION] < 1.0
				&&  cs->attributes[AGGRESSION] < ( random() + 0.5 * cs->attributes[TACTICAL] )
				&&  ( cs->takeCoverTime < level.time )
				&&  AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].real_visible_pos, cs->takeCoverPos ) ) {
			// start taking cover
			cs->takeCoverTime = level.time + 2000 + rand() % 4000;    // only move a little bit
			//cs->attackcrouch_time = 0;		// get out of here real quick
			return AIFunc_BattleTakeCoverStart( cs );
		} else
		// if we haven't thrown a grenade in a bit, go into "grenade flush mode"
		if ( ( lastGrenadeFlush > level.time || lastGrenadeFlush < level.time - 7000 ) &&
			 ( cs->aiState >= AISTATE_COMBAT ) &&
			 ( cs->castScriptStatus.castScriptEventIndex < 0 ) &&
			 (   (   ( COM_BitCheck( cs->bs->cur_ps.weapons, WP_GRENADE_LAUNCHER ) ) &&
					 ( AICast_GotEnoughAmmoForWeapon( cs, WP_GRENADE_LAUNCHER ) ) &&
					 ( cs->weaponFireTimes[WP_GRENADE_LAUNCHER] < level.time - (int)( aicast_skillscale * 3000 ) ) ) ||
				 (   ( COM_BitCheck( cs->bs->cur_ps.weapons, WP_GRENADE_PINEAPPLE ) ) &&
					( AICast_GotEnoughAmmoForWeapon( cs, WP_GRENADE_PINEAPPLE ) ) &&
					( cs->weaponFireTimes[WP_GRENADE_PINEAPPLE] < level.time - (int)( aicast_skillscale * 3000 ) ) ) ) &&
			 !( cs->weaponNum && ( cs->castScriptStatus.scriptFlags & SFL_NOCHANGEWEAPON ) ) &&
			 ( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].real_visible_pos ) > 100 ) &&
			 ( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].real_visible_pos ) < 1200 ) &&
			 ( AICast_WantsToChase( cs ) ) ) {
			// try and flush them out with a grenade
			//G_Printf("get outta there..\n");
			return AIFunc_GrenadeFlushStart( cs );
		} else
		// not visible, should we chase them?
		if ( AICast_WantsToChase( cs ) ) {
			// chase them
			return AIFunc_BattleChaseStart( cs );
		} else
		// Take Cover?
		if (    AICast_WantsToTakeCover( cs, qfalse )
				&&  ( cs->takeCoverTime < level.time )
				&&  AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].real_visible_pos, cs->takeCoverPos ) ) {
			// go to a position that cannot be seen from the last place we saw the enemy, and wait there for some time
			cs->takeCoverTime = level.time + 4000 + rand() % 2000;
			//cs->attackcrouch_time = 0;
			return AIFunc_BattleTakeCoverStart( cs );
		} else
		{
			// chase them, nothing else to do
			return AIFunc_BattleChaseStart( cs );
		}
	}
	// if we are obstructing someone else, move out the way
	if ( cs->obstructingTime > level.time ) {
		// setup a combatgoal in the obstructionYaw direction
		//cs->combatGoalTime = level.time + 10;
		//VectorCopy( cs->obstructingPos, cs->combatGoalOrigin );
		AICast_MoveToPos( cs, cs->obstructingPos, -1 );
		// if not crouching, walk instead of running
		cs->speedScale = cs->attributes[WALKING_SPEED] / cs->attributes[RUNNING_SPEED];
	}
	// if the enemy is really close, avoid them
	else if (   ( cs->obstructingTime < ( level.time - 500 + rand() % 300 ) ) &&
				( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].real_visible_pos ) < 100 ) ) {
		if ( AICast_GetAvoid( cs, NULL, cs->obstructingPos, qtrue, cs->enemyNum ) ) {
			cs->obstructingTime = level.time + 500;
		} else {
			cs->obstructingTime = level.time - 1;   // wait a bit before trying again
		}
	}
	//
	// setup for the fight
	//
	tfl = cs->travelflags;
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		tfl |= TFL_SLIME;
	}
	//
	/*
	moveresult = AICast_CombatMove(cs, tfl);
	//if the movement failed
	if (moveresult.failure) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach(bs->ms);
		// reset the combatgoal
		cs->combatGoalTime = 0;
	} else if (cs->combatGoalTime > level.time && VectorLength(cs->bs->cur_ps.velocity)) {	// crouch if moving?
		if (cs->attributes[ATTACK_CROUCH] > 0.1) {
			AICast_RequestCrouchAttack( cs, cs->bs->origin, 0.5 );
		}
	}
	*/
	// if we are crouching, don't stay down for too long after we finish fighting
	if ( cs->aiFlags & AIFL_ATTACK_CROUCH ) {
		if ( cs->attackcrouch_time > level.time || ( cs->thinkFuncChangeTime < level.time - 1000 ) ) {
			cs->attackcrouch_time = level.time + 1000;
		}
	} else {
		cs->attackcrouch_time = 0;  // only set it if we need it
	}
	//
	AICast_Blocked( cs, &moveresult, qfalse, NULL );
	//
	// Retreat?
	if ( cs->castScriptStatus.scriptNoMoveTime < level.time && AICast_WantToRetreat( cs ) ) {
		if  ( AICast_GetTakeCoverPos( cs, cs->enemyNum, cs->vislist[cs->enemyNum].visible_pos, cs->takeCoverPos ) ) {
			// go to a position that cannot be seen from the last place we saw the enemy, and wait there for some time
			cs->takeCoverTime = level.time + 2000 + rand() % 3000;
			return AIFunc_BattleTakeCoverStart( cs );
		}
	}
	//
	// Lob a Grenade?
	// if we haven't thrown a grenade in a bit, go into "grenade flush mode"
	if ( ( lastGrenadeFlush > level.time || lastGrenadeFlush < level.time - 7000 ) &&
		 ( cs->aiState >= AISTATE_COMBAT ) &&
		 ( cs->castScriptStatus.castScriptEventIndex < 0 ) &&
		 ( cs->startGrenadeFlushTime < level.time - 3000 ) &&
		 ( COM_BitCheck( cs->bs->cur_ps.weapons, WP_GRENADE_LAUNCHER ) ) &&
		 ( AICast_GotEnoughAmmoForWeapon( cs, WP_GRENADE_LAUNCHER ) ) &&
		 ( cs->weaponFireTimes[WP_GRENADE_LAUNCHER] < level.time - (int)( aicast_skillscale * 3000 ) ) &&
		 ( ( cs->weaponNum == WP_GRENADE_LAUNCHER ) || !( cs->castScriptStatus.scriptFlags & SFL_NOCHANGEWEAPON ) ) &&
		 ( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].real_visible_pos ) > 100 ) &&
		 ( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].real_visible_pos ) < 2000 ) ) {
		// try and flush them out with a grenade
		//G_Printf("pineapple?\n");
		return AIFunc_GrenadeFlushStart( cs );
	}
	if ( ( lastGrenadeFlush > level.time || lastGrenadeFlush < level.time - 7000 ) &&
		 ( cs->aiState >= AISTATE_COMBAT ) &&
		 ( cs->castScriptStatus.castScriptEventIndex < 0 ) &&
		 ( cs->startGrenadeFlushTime < level.time - 3000 ) &&
		 ( COM_BitCheck( cs->bs->cur_ps.weapons, WP_GRENADE_PINEAPPLE ) ) &&
		 ( AICast_GotEnoughAmmoForWeapon( cs, WP_GRENADE_PINEAPPLE ) ) &&
		 ( cs->weaponFireTimes[WP_GRENADE_PINEAPPLE] < level.time - (int)( aicast_skillscale * 3000 ) ) &&
		 ( ( cs->weaponNum == WP_GRENADE_PINEAPPLE ) || !( cs->castScriptStatus.scriptFlags & SFL_NOCHANGEWEAPON ) ) &&
		 ( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].real_visible_pos ) > 100 ) &&
		 ( Distance( cs->bs->origin, cs->vislist[cs->enemyNum].real_visible_pos ) < 2000 ) ) {
		// try and flush them out with a grenade
		//G_Printf("pineapple?\n");
		return AIFunc_GrenadeFlushStart( cs );
	}
	//
	// Dodge enemy aim?
	if (    ( cs->attributes[AGGRESSION] < 1.0 ) &&
			( ent->client->ps.weapon ) &&
			( ent->client->ps.groundEntityNum == ENTITYNUM_WORLD ) &&
			( !cs->lastRollMove || cs->lastRollMove < level.time - 4000 ) &&
			( cs->attributes[TACTICAL] > 0.5 ) && ( cs->aiFlags & AIFL_ROLL_ANIM ) &&
			( VectorLength( cs->bs->cur_ps.velocity ) < 1 ) ) {
		vec3_t aim, enemyVec, right;
		// are they aiming at us?
		AngleVectors( enemy->client->ps.viewangles, aim, right, NULL );
		VectorSubtract( cs->bs->origin, enemy->r.currentOrigin, enemyVec );
		VectorNormalize( enemyVec );
		// if they are looking at us, we should avoid them
		if ( DotProduct( aim, enemyVec ) > 0.97 ) {
			aicast_predictmove_t move;
			vec3_t dir;
			bot_input_t bi, bi_back;
			usercmd_t ucmd;
			float simTime = 0.8;

			cs->lastRollMove = level.time;

			trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi_back );
			trap_EA_ResetInput( cs->entityNum, NULL );
			if ( level.time % 200 < 100 ) {
				VectorNegate( right, dir );
			} else { VectorCopy( right, dir );}
			trap_EA_Move( cs->entityNum, dir, 400 );
			trap_EA_GetInput( cs->entityNum, (float) level.time / 1000, &bi );
			VectorCopy( dir, bi.dir );
			AICast_InputToUserCommand( cs, &bi, &ucmd, bs->cur_ps.delta_angles );
			AICast_PredictMovement( cs, 4, simTime / 4, &move, &ucmd, cs->enemyNum );

			trap_EA_ResetInput( cs->entityNum, &bi_back );

			if ( move.groundEntityNum == ENTITYNUM_WORLD &&
				 VectorDistance( move.endpos, cs->bs->origin ) > simTime * cs->attributes[RUNNING_SPEED] * 0.8 ) {
				// good enough
				if ( AICast_CheckAttackAtPos( cs->entityNum, cs->enemyNum, move.endpos, cs->bs->cur_ps.viewheight == cs->bs->cur_ps.crouchViewHeight, qfalse ) ) {
					cs->takeCoverTime = 0;
					return AIFunc_BattleRollStart( cs, dir );
				}
			}
		}
	}
	//
	// reload?
	if ( ( cs->bs->cur_ps.weaponstate != WEAPON_RELOADING ) && ( cs->bs->cur_ps.ammoclip[BG_FindClipForWeapon( cs->bs->cur_ps.weapon )] < (int)( ammoTable[cs->bs->cur_ps.weapon].uses ) ) ) {
		if ( AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum ) ) {
			trap_EA_Reload( cs->entityNum );
		} else {    // no ammo, switch?
			AICast_ChooseWeapon( cs, qfalse );
			if ( cs->weaponNum == WP_NONE ) {
				// no ammo, get out of here
				return AIFunc_DefaultStart( cs );
			}
			if ( !AICast_GotEnoughAmmoForWeapon( cs, cs->weaponNum ) ) {
				// no ammo, get out of here
				return AIFunc_DefaultStart( cs );
			}
		}
	} else {
		//attack the enemy if possible
		AICast_ProcessAttack( cs );
	}
	//
	return NULL;
}

/*
============
AIFunc_BattleStart()
============
*/
char *AIFunc_BattleStart( cast_state_t *cs ) {
	char *rval;
	int lastweap;
	// make sure we don't avoid any areas when we start again
	trap_BotInitAvoidReach( cs->bs->ms );
	// wait some time before taking cover again
	cs->takeCoverTime = level.time + 300 + rand() % ( 2000 + (int)( 2000.0 * cs->attributes[AGGRESSION] ) );
	// wait some time before going to a combat spot
	cs->combatSpotDelayTime = level.time + 1500 + rand() % 2500;
	//
	// start a crouch attack?
	if ( ( random() * 3.0 + 1.0 < cs->attributes[ATTACK_CROUCH] )
		 && AICast_RequestCrouchAttack( cs, cs->bs->origin, 0.0 ) ) {
		cs->aiFlags |= AIFL_ATTACK_CROUCH;
	} else {
		cs->attackcrouch_time = 0;
		cs->aiFlags &= ~AIFL_ATTACK_CROUCH;
	}
	//
	cs->lastEnemy = cs->enemyNum;
	cs->startAttackCount++;
	cs->crouchHideFlag = qfalse;
	//
	// get out of talking state
	cs->aiFlags &= ~AIFL_TALKING;
	//
	//update the attack inventory values
	AICast_UpdateBattleInventory( cs, cs->enemyNum );
	//
	// if we have a special attack, call the correct AI routine
recheck:
	rval = NULL;
	// ignore special attacks until we are facing our enemy
	if ( fabs( AngleDifference( cs->ideal_viewangles[YAW], cs->viewangles[YAW] ) ) < 10 ) {
		// select a weapon
		AICast_ChooseWeapon( cs, qtrue );
		//
		if ( ( cs->weaponNum == WP_MONSTER_ATTACK1 ) && cs->aifuncAttack1 ) {
			if ( AICast_CheckAttack( cs, cs->enemyNum, qfalse ) ) {
				rval = cs->aifuncAttack1( cs );
			} else {
				rval = AIFunc_BattleChaseStart( cs );
			}
		} else if ( ( cs->weaponNum == WP_MONSTER_ATTACK2 ) && cs->aifuncAttack2 ) {
			if ( AICast_CheckAttack( cs, cs->enemyNum, qfalse ) ) {
				rval = cs->aifuncAttack2( cs );
			} else {
				rval = AIFunc_BattleChaseStart( cs );
			}
		} else if ( ( cs->weaponNum == WP_MONSTER_ATTACK3 ) && cs->aifuncAttack3 ) {
			if ( AICast_CheckAttack( cs, cs->enemyNum, qfalse ) ) {
				rval = cs->aifuncAttack3( cs );
			} else {
				rval = AIFunc_BattleChaseStart( cs );
			}
		}
		//
		if ( !rval && cs->weaponNum >= WP_MONSTER_ATTACK1 && cs->weaponNum <= WP_MONSTER_ATTACK3 ) {
			// don't use this weapon again for a while
			cs->weaponFireTimes[cs->weaponNum] = level.time;
			// select a different weapon
			lastweap = cs->weaponNum;
			AICast_ChooseWeapon( cs, qfalse );  // qfalse so we don't choose a special weapon
			if ( cs->weaponNum == lastweap ) {
				return NULL;
			}
			// try again
			goto recheck;
		}
	} else {    // normal weapons
		// select a weapon
		AICast_ChooseWeapon( cs, qfalse );
		rval = NULL;
	}
	//
	if ( !rval ) {    // use the generic battle routine for all "normal" weapons
		if ( cs->weaponNum >= WP_MONSTER_ATTACK1 && cs->weaponNum <= WP_MONSTER_ATTACK3 ) {
			// monster attacks are not allowed to go into the normal battle mode
			return NULL;
		} else {
			cs->aifunc = AIFunc_Battle;
			return "AIFunc_Battle";
		}
	}
	//
	// we decided to start a special monster attack
	return rval;
}

/*
============
AIFunc_DefaultStart()
============
*/
char *AIFunc_DefaultStart( cast_state_t *cs ) {
	qboolean first = qfalse;
	char    *rval = NULL;
	//
	if ( cs->aiFlags & AIFL_JUST_SPAWNED ) {
		first = qtrue;
		cs->aiFlags &= ~AIFL_JUST_SPAWNED;
	}
	//
	switch ( cs->aiCharacter ) {
	case AICHAR_ZOMBIE:
		// portal zombie, requires spawning effect
		if ( first && ( g_entities[cs->entityNum].spawnflags & 4 ) ) {
			return AIFunc_FlameZombie_PortalStart( cs );
		}
		break;
	}
	//
	// if they have an enemy, then pursue
	if ( cs->enemyNum >= 0 && ( cs->aifunc != AIFunc_Battle ) ) {   // make sure we haven't just come from there
		rval = AIFunc_BattleStart( cs );
	}
	//
	if ( !rval ) {
		return AIFunc_IdleStart( cs );
	}
	//
	return rval;
}
