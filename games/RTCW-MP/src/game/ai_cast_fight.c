/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//===========================================================================
//
// Name:			ai_cast_fight.c
// Function:		Wolfenstein AI Character Fighting/Combat
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
#include "../game/be_ai_weap.h"
#include "../botai/botai.h"          //bot ai interface

#include "ai_cast.h"

/*
Support routines for the Decision Making layer.
*/

// FIXME: go through here and convert all weapon/character parameters to #define's
// and move them to a seperate header file for easy modification

/*
=================
AICast_StateChange

  returns qfalse if scripting has denied the action
=================
*/
qboolean AICast_StateChange( cast_state_t *cs, aistateEnum_t newaistate ) {
	gentity_t *ent;
	int result, scriptIndex;
	aistateEnum_t oldstate;

	ent = &g_entities[cs->entityNum];

	oldstate = cs->aiState;
	cs->aiState = newaistate;

	// if moving from query mode, kill the anim and pausetime
	if ( oldstate == AISTATE_QUERY ) {
		// stop playing the animation
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;
		cs->pauseTime = 0;
	}

	// if moving to combat mode, default back to normal movetype (fast)
	if ( newaistate == AISTATE_COMBAT ) {
		cs->movestate = MS_DEFAULT;
		cs->movestateType = MSTYPE_NONE;
	}

	scriptIndex = cs->scriptCallIndex;

	// check scripting to see if this event should be ignored (no anim or handling)
	cs->aiFlags &= ~AIFL_DENYACTION;
	AICast_ScriptEvent( cs, "statechange", va( "%s %s", animStateStr[oldstate].string, animStateStr[newaistate].string ) );

	if ( !( cs->aiFlags & AIFL_DENYACTION ) ) {
		// if no script was found, try enemysight
		if ( newaistate == AISTATE_COMBAT && cs->scriptCallIndex == scriptIndex ) {   // no script was found, so default back to enemysight
			AICast_ScriptEvent( cs, "enemysight", g_entities[cs->bs->enemy].aiName );
			if ( !( cs->aiFlags & AIFL_DENYACTION ) ) {
				G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].sightSoundScript ) );
			}

			if ( cs->aiFlags & AIFL_DENYACTION ) {
				// don't run any dynamic handling or default anims
				return qfalse;
			}
		}

		// look for an animation
		result = BG_AnimScriptStateChange( &ent->client->ps, newaistate, oldstate );

		if ( result > 0 ) {
			// pause while the animation plays
			cs->pauseTime = level.time + result;
		}
	}

	// set query mode fields
	if ( newaistate == AISTATE_QUERY ) {
		cs->queryStartTime = level.time;
		if ( cs->queryCountValidTime < level.time ) {
			cs->queryCount = 0;
		} else {
			cs->queryCount++;
		}
		cs->queryCountValidTime = level.time + 60000;   // one minute
		switch ( cs->queryCount ) {
		case 0:
			cs->queryAlertSightTime = level.time + 1000;
			break;
		case 1:
			cs->queryAlertSightTime = level.time + 500;
			break;
		default:
			cs->queryAlertSightTime = -1;   // IMMEDIATE COMBAT MODE
			break;
		}
	}

	return qtrue;
}

/*
=================
AICast_ScanForEnemies

  returns the number of enemies visible, filling the "enemies" list before exiting

  if we only found queryEnemies (possibly hostile, but not sure) while relaxed,
  then a negative count is returned
=================
*/
int AICast_ScanForEnemies( cast_state_t *cs, int *enemies ) {
	int i, j, enemyCount, queryCount, friendlyAlertCount;
	static float distances[MAX_CLIENTS];
	static int sortedEnemies[MAX_CLIENTS];
	float lastDist;
	int best, oldEnemy, oldPauseTime;
	cast_state_t *ocs;

	if ( cs->castScriptStatus.scriptAttackEnt >= 0 ) {
		if ( g_entities[cs->castScriptStatus.scriptAttackEnt].health <= 0 ) {
			cs->castScriptStatus.scriptAttackEnt = -1;
		} else {
			// if we are not in combat mode, then an enemy should trigger a state change straight to combat mode
			if ( cs->aiState < AISTATE_COMBAT ) {
				AICast_StateChange( cs, AISTATE_COMBAT );   // just go straight to combat mode
			}
			enemies[0] = cs->castScriptStatus.scriptAttackEnt;
			return 1;
		}
	}

	if ( cs->castScriptStatus.scriptNoAttackTime >= level.time ) {
		return qfalse;
	}

	if ( cs->noAttackTime >= level.time ) {
		return qfalse;
	}

	if ( cs->castScriptStatus.scriptNoSightTime >= level.time ) {
		return qfalse;
	}

	enemyCount = 0;
	queryCount = 0;
	friendlyAlertCount = 0;

	// while we're here, may as well check for some baddies
	for ( i = 0; i < g_maxclients.integer; i++ )
	{
		if ( g_entities[i].inuse ) {
			// try not to commit suicide
			if ( i != cs->bs->entitynum ) {
				if ( AICast_EntityVisible( cs, i, qfalse ) ) {
					// how should we deal with them?
					if ( ( g_entities[i].health > 0 ) && AICast_HostileEnemy( cs, i ) ) { // visible and a baddy!
						enemies[enemyCount] = i;
						enemyCount++;
						queryCount = 0;
						friendlyAlertCount = 0;
					} else if ( !enemyCount && ( g_entities[i].health > 0 ) && AICast_QueryEnemy( cs, i ) && ( cs->vislist[i].flags & AIVIS_PROCESS_SIGHTING ) ) {
						enemies[queryCount] = i;
						queryCount++;
						friendlyAlertCount = 0;
					} else if ( !queryCount && !enemyCount && ( cs->vislist[i].flags & AIVIS_INSPECT ) ) {
						enemies[friendlyAlertCount] = i;
						friendlyAlertCount++;
					}
					// the sighting has been processed
					cs->vislist[i].flags &= ~AIVIS_PROCESS_SIGHTING;
				}
			}
		}
	}

	if ( !enemyCount ) {
		if ( queryCount ) {
			enemyCount = queryCount;
		} else if ( friendlyAlertCount ) {
			enemyCount = friendlyAlertCount;
		}
	}

	if ( !enemyCount ) {  // nothing worth doing anything about
		// look for audible events that we should investigate
		if ( cs->audibleEventTime && cs->audibleEventTime < level.time && cs->audibleEventTime > level.time - 2000 ) {
			return -4;
		}
		// look for bullet impacts that have occured recently
		if ( cs->bulletImpactTime && cs->bulletImpactTime < level.time && cs->bulletImpactTime > level.time - 1000 ) {
			return -3;
		}
		return 0;
	}

	// sort the enemies by distance
	for ( i = 0; i < enemyCount; i++ ) {
		distances[i] = Distance( cs->bs->origin, g_entities[enemies[i]].client->ps.origin );
		if ( !distances[i] ) {
			G_Printf( "WARNING: zero distance between enemies:\n%s at %s, %s at %s\n", g_entities[cs->entityNum].aiName, vtos( cs->bs->origin ), g_entities[enemies[i]].aiName, vtos( g_entities[enemies[i]].client->ps.origin ) );
			distances[i] = 999998;  // try to ignore them (HACK)
		}
	}
	for ( j = 0; j < enemyCount; j++ ) {
		lastDist = 999999;
		best = -1;
		for ( i = 0; i < enemyCount; i++ ) {
			if ( distances[i] && distances[i] < lastDist ) {
				lastDist = distances[i];
				best = i;
			}
		}
		if ( best < 0 ) {
			G_Error( "error sorting enemies by distance\n" );
		}
		sortedEnemies[j] = enemies[best];
		distances[best] = -1;
	}
	memcpy( enemies, sortedEnemies, sizeof( int ) * enemyCount );

	// if we are not in combat mode, then an enemy should trigger a state change straight to combat mode
	if ( !queryCount && !friendlyAlertCount && enemyCount && cs->aiState < AISTATE_COMBAT ) {
		// if only one enemy, face them while making the transition
		oldEnemy = cs->bs->enemy;
		if ( enemyCount == 1 ) {
			cs->bs->enemy = enemies[0];
			AICast_AimAtEnemy( cs );
			// leave it set so StateChange() can use the name for the script calls
		}
		AICast_StateChange( cs, AISTATE_COMBAT );   // just go straight to combat mode
		cs->bs->enemy = oldEnemy;
	}

	// if we are in relaxed state, and we see a query enemy, then go into query mode
	if ( queryCount ) {
		if ( cs->aiState == AISTATE_RELAXED ) {
			// go into query mode
			if ( AICast_StateChange( cs, AISTATE_QUERY ) ) {
				cs->bs->enemy = enemies[0]; // lock onto the closest potential enemy
				return -1;
			}
			return 0;   // scripting obviously doesn't want us to progress from relaxed just yet
		}
		// else ignore the query mode, since we are already above relaxed mode
		return 0;
	}
	if ( friendlyAlertCount ) {
		// call a script event
		oldPauseTime = cs->scriptPauseTime;
		if ( g_entities[enemies[0]].health <= 0 ) {
			AICast_ForceScriptEvent( cs, "inspectbodystart", g_entities[enemies[0]].aiName );
			if ( cs->aiFlags & AIFL_DENYACTION ) {
				// ignore this friendly
				cs->vislist[i].flags |= AIVIS_INSPECTED;
				return 0;
			}
		}
		//
		if ( cs->aiState < AISTATE_COMBAT ) {
			// go into alert mode, and return this entity so we can inspect it or something,
			// but let dynamic AI sort out what it wants to do
			if ( cs->aiState == AISTATE_ALERT || AICast_StateChange( cs, AISTATE_ALERT ) ) {
				// only return the entity if they are in combat mode or dead
				ocs = AICast_GetCastState( enemies[0] );
				if ( ( g_entities[enemies[0]].health <= 0 ) || ( ocs->aiState >= AISTATE_COMBAT ) ) {
					return -2;
				}
			}
			return 0;   // scripting failed or they're not worth physically inspecting
		}
		// ignore the friendly, we have our hands full
		return 0;
	}

	// must be hostile enemy(s) found, so return them
	return enemyCount;
}

/*
==================
AICast_EntityVisible
==================
*/
qboolean AICast_EntityVisible( cast_state_t *cs, int enemynum, qboolean directview ) {
	cast_visibility_t *vis;
	int last_visible;
	int reactionTime;
	float dist;

	if ( enemynum >= MAX_CLIENTS ) {
		return qtrue;           // FIXME: do a visibility calculation on non-client entities?

	}
	vis = &cs->vislist[enemynum];

	if ( !vis->visible_timestamp && !vis->real_visible_timestamp ) {
		return qfalse;  // they are not visible at all

	}
	if ( directview ) {
		last_visible = vis->real_visible_timestamp;
	} else {
		last_visible = vis->visible_timestamp;
	}

	reactionTime = (int)( 1000 * cs->attributes[REACTION_TIME] );
	if ( cs->startAttackCount > 1 ) {
		// we recently saw them, so we are more "aware" of their presence
		reactionTime /= 2;
	}

	// if they are close, we should react faster
	if ( cs->bs && enemynum == cs->bs->enemy ) {
		dist = cs->enemyDist;
	} else {
		dist = VectorDistance( g_entities[cs->entityNum].client->ps.origin, cs->vislist[enemynum].visible_pos );
	}
	if ( dist < 384 ) {
		reactionTime *= 0.5 + 0.5 * ( dist / 384 );
	}

	if ( vis->notvisible_timestamp < ( level.time - reactionTime ) ) {
		// make sure we've seen them since we've last not seen them (since the visibility checking is spread amongst server frames)
		if ( vis->notvisible_timestamp < last_visible ) {
			return qtrue;
		}
	}

	// we can't directly see them, but if they've just left our sight, pretend we can see them for another second or so

	if ( !directview && last_visible ) {
		if ( vis->notvisible_timestamp > last_visible ) {
			if ( vis->notvisible_timestamp < ( last_visible + 5000 ) ) {
				return qtrue;
			}
		}
	}


	return qfalse;
}

/*
==================
AICast_HostileEnemy

  returns qtrue if the entity is hostile
==================
*/
qboolean AICast_HostileEnemy( cast_state_t *cs, int enemynum ) {
	// if we hate them, they are an enemy
	if ( cs->vislist[enemynum].flags & AIVIS_ENEMY ) {
		return qtrue;
	} else {
		return qfalse;
	}
}

/*
==================
AICast_QueryEnemy

  returns qtrue if the entity can become hostile (they hurt us or we recognize them)
==================
*/
qboolean AICast_QueryEnemy( cast_state_t *cs, int enemynum ) {

	if ( g_entities[cs->entityNum].aiTeam != g_entities[enemynum].aiTeam ) {
		if ( g_entities[cs->entityNum].aiTeam == AITEAM_MONSTER || g_entities[enemynum].aiTeam == AITEAM_MONSTER ) {
			// monsters hate all non-monsters and vice-versa
			return qtrue;
		} else {
			// neutral's can only possibly become hostile if they hurt us, otherwise they are assumed to be harmless
			if ( g_entities[cs->entityNum].aiTeam == AITEAM_NEUTRAL || g_entities[enemynum].aiTeam == AITEAM_NEUTRAL ) {
				return qfalse;  // one of us is neutral, assume harmless
			}
			return qtrue;
		}
	} else {    // same team
		return qfalse;  // be cool bitch
	}

}

/*
==================
AICast_SameTeam

  the player is always team 1, AI's default to team 0

  MONSTER's hate everyone else except other MONSTER's

  NEUTRAL's are cool with everyone that hasn't hurt them
==================
*/
qboolean AICast_SameTeam( cast_state_t *cs, int enemynum ) {

	if ( g_entities[cs->entityNum].aiTeam != g_entities[enemynum].aiTeam ) {
		if ( g_entities[cs->entityNum].aiTeam == AITEAM_NEUTRAL || g_entities[enemynum].aiTeam == AITEAM_NEUTRAL ) {
			// if we hate them, they are an enemy
			if ( cs->vislist[enemynum].flags & AIVIS_ENEMY ) {
				return qfalse;
			} else {
				return qtrue;
			}
		} else {
			return qfalse;  // they are an enemy
		}
	} else {
		return qtrue;   // be cool bitch
	}

}

/*
==================
AICast_WeaponRange
==================
*/
float AICast_WeaponRange( cast_state_t *cs, int weaponnum ) {
	switch ( weaponnum ) {
	case WP_GAUNTLET:
		return 32 - 4;      // to be safe
	case WP_TESLA:
		return ( TESLA_RANGE * 0.9 ) - 50;  // allow for bounding box
	case WP_FLAMETHROWER:
		return ( FLAMETHROWER_RANGE * 0.6 ) - 50;   // allow for bounding box
	case WP_PANZERFAUST:
	case WP_ROCKET_LAUNCHER:
		return 8000;

	case WP_SPEARGUN:   //----(SA)
	case WP_SPEARGUN_CO2:
		return 300;

	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
		return 800;
	case WP_MONSTER_ATTACK1:
		switch ( cs->aiCharacter ) {
		case AICHAR_WARZOMBIE:
			return 44;
		case AICHAR_LOPER:  // close attack, head-butt, fist
		case AICHAR_SEALOPER:
			return 60;
		case AICHAR_BLACKGUARD:
			return BLACKGUARD_MELEE_RANGE;
		case AICHAR_STIMSOLDIER3:
			return TESLA_RANGE;
		case AICHAR_ZOMBIE: // zombie flaming attack
			//return (FLAMETHROWER_RANGE*0.6) - 50;	// allow for bounding box
			return ZOMBIE_FLAME_RADIUS - 100;     // get well within range before starting
		case AICHAR_REJECTX:
			return REJECT_MELEE_RANGE;
		}
		break;
	case WP_MONSTER_ATTACK2:
		switch ( cs->aiCharacter ) {
		case AICHAR_ZOMBIE: // zombie spirit attack
			return 800;
		case AICHAR_LOPER:  // loper leap attack
			return 256;
		}
		break;
	case WP_MONSTER_ATTACK3:
		switch ( cs->aiCharacter ) {
		case AICHAR_LOPER:  // loper ground attack
			return LOPER_GROUND_RANGE;
		case AICHAR_WARZOMBIE:  // warzombie defense
			return 2000;
		}
		break;

		// Rafael added these changes as per Mikes request
	case WP_MAUSER:
	case WP_GARAND:
	case WP_SNIPERRIFLE:
	case WP_SNOOPERSCOPE:
		return 8000;
		break;


	}
	// default range
	return 3000;
}

/*
==================
AICast_CheckAttack_real
==================
*/
qboolean AICast_CheckAttack_real( cast_state_t *cs, int enemy, qboolean allowHitWorld ) {
	//float points;
	vec3_t forward, right, start, end, dir, up, angles;
	weaponinfo_t wi;
	trace_t trace;
	float traceDist;
	static vec3_t smins = {-6, -6, -6}, smaxs = {6, 6, 6};
	static vec3_t fmins = {-30, -30, -24}, fmaxs = {30, 30, 24};
	float *mins, *maxs;
	float halfHeight;
	int traceMask;
	int fuzzyCount, i;
	gentity_t *ent, *enemyEnt;
	float dist;
	int passEnt;
	int weapnum;
	//
	if ( enemy < 0 ) {
		return qfalse;
	}
	ent = &g_entities[cs->entityNum];
	enemyEnt = &g_entities[enemy];
	//
	if ( cs->bs ) {
		weapnum = cs->bs->weaponnum;
	} else {
		weapnum = ent->client->ps.weapon;
	}
	//
	if ( !weapnum ) {
		return qfalse;
	}
	//
	// don't attack while in air (like on a ladder)
	if ( !ent->waterlevel && ent->client->ps.groundEntityNum == ENTITYNUM_NONE && !ent->active ) {
		// stim is allowed to fire while in air for flying attack
		if ( !ent->client->ps.powerups[PW_FLIGHT] ) {
			return qfalse;
		}
	}
	//
	if ( ent->health <= 0 ) {
		return qfalse;
	}
	// can't attack without any ammo
	if ( cs->bs ) {
		if ( !AICast_GotEnoughAmmoForWeapon( cs, cs->bs->weaponnum ) ) {
			return qfalse;
		}
	}
	// special case: femzombie can always use portal lightning attack when available
	if ( cs->aiCharacter == AICHAR_FEMZOMBIE && weapnum == WP_MONSTER_ATTACK1 ) {
		return qtrue;
	}
	// special case: warzombie should play laughing anim at first sight
	if ( cs->aiCharacter == AICHAR_WARZOMBIE && weapnum == WP_MONSTER_ATTACK2 ) {
		return qtrue;
	}
	//
	//if the enemy isn't directly visible
	if ( !allowHitWorld && cs->vislist[enemy].real_visible_timestamp != cs->vislist[enemy].real_update_timestamp ) {
		return qfalse;
	}
	//
	//get the weapon info (FIXME: hard-code the weapon info?)
	memset( &wi, 0, sizeof( weaponinfo_t ) );
	//
	traceMask = MASK_SHOT;  // FIXME: assign mask's to different weapons
	//end point aiming at
	if ( !ent->active ) {
		//get the start point shooting from
		VectorCopy( enemyEnt->r.currentOrigin, start );
		start[2] += enemyEnt->client->ps.viewheight;
		VectorCopy( ent->r.currentOrigin, end );
		end[2] += ent->client->ps.viewheight;
		VectorSubtract( start, end, dir );
		vectoangles( dir, angles );
		AngleVectors( angles, forward, right, up );
		CalcMuzzlePoint( &g_entities[cs->entityNum], weapnum, forward, right, up, start );

		traceDist = AICast_WeaponRange( cs, weapnum );
		switch ( weapnum ) {
		case WP_GAUNTLET:
			mins = NULL;
			maxs = NULL;
			break;
		case WP_DYNAMITE:
		case WP_DYNAMITE2:
		case WP_PANZERFAUST:
		case WP_ROCKET_LAUNCHER:
		case WP_GRENADE_LAUNCHER:
		case WP_GRENADE_PINEAPPLE:
		case WP_PROX:
			traceMask = MASK_MISSILESHOT;
			mins = smins;
			maxs = smaxs;
			break;
		case WP_FLAMETHROWER:
			mins = fmins;
			maxs = fmaxs;
			break;
		default:
			mins = smins;
			maxs = smaxs;
			break;
		}
		passEnt = cs->entityNum;

		// don't try too far
		dist = Distance( ent->r.currentOrigin, enemyEnt->r.currentOrigin );
		if ( traceDist > dist ) {
			traceDist = dist;
			fuzzyCount = 6;
		} else if ( dist > traceDist + 32 ) {
			return qfalse;
		} else {
			fuzzyCount = 0;
		}
	} else {
		gentity_t *mg42;
		// we are mounted on a weapon
		mg42 = &g_entities[cs->mountedEntity];
		VectorCopy( enemyEnt->r.currentOrigin, start );
		start[2] += enemyEnt->client->ps.viewheight;
		VectorCopy( mg42->r.currentOrigin, end );
		VectorSubtract( start, end, dir );
		vectoangles( dir, angles );
		AngleVectors( angles, forward, right, up );

		VectorCopy( mg42->r.currentOrigin, start );
		VectorMA( start, 16, forward, start );
		VectorMA( start, 16, up, start );
		// snap to integer coordinates for more efficient network bandwidth usage
		SnapVector( start );

		traceDist = 8192;
		mins = NULL;
		maxs = NULL;
		if ( mg42->mg42BaseEnt >= 0 ) {
			passEnt = mg42->mg42BaseEnt;
		} else {
			passEnt = cs->entityNum;
		}

		// don't try too far
		dist = Distance( start, enemyEnt->r.currentOrigin );
		if ( ( traceDist - dist ) > 0 ) {
			traceDist = dist;
			fuzzyCount = 6;
		} else if ( ( dist - traceDist ) > 32 ) {
			return qfalse;
		} else {
			fuzzyCount = 0;
		}
	}

	for ( i = 0; i <= fuzzyCount; i++ ) {
		VectorMA( start, traceDist, forward, end );

		// fuzzy end point
		if ( i > 0 ) {
			VectorMA( end, enemyEnt->r.maxs[0] * 0.9 * (float)( ( i % 2 ) * 2 - 1 ), right, end );
			halfHeight = ( enemyEnt->r.maxs[2] - enemyEnt->r.mins[2] ) / 2.0;
			end[2] = ( enemyEnt->r.currentOrigin[2] + enemyEnt->r.mins[2] ) + halfHeight;
			VectorMA( end, halfHeight * 0.9 * ( ( (float)( ( i - 1 ) - ( ( i - 1 ) % 2 ) ) / 2 - 1.0 ) ), up, end );
		}

		if ( allowHitWorld && !trap_InPVS( start, end ) ) {
			// not possibly attackable
			continue;
		}

		trap_Trace( &trace, start, mins, maxs, end, passEnt, traceMask );
		if ( trace.fraction == 1.0 ) {
			//return qfalse;
			continue;
		}
		//if won't hit the enemy
		if ( trace.entityNum != enemy ) {
			if ( !allowHitWorld ) {
				continue;
			}

			if ( trace.startsolid ) {
				continue;
			}

			//if the entity is a client
			if ( trace.entityNum >= 0 && trace.entityNum < MAX_CLIENTS ) {
				//if a teammate is hit
				if ( AICast_SameTeam( cs, trace.entityNum ) ) {
					return qfalse;
				}
			}
			//if the projectile does a radial damage
			if ( cs->bs->weaponnum == WP_ROCKET_LAUNCHER || cs->bs->weaponnum == WP_PANZERFAUST ) {
				if ( Distance( trace.endpos, g_entities[enemy].s.pos.trBase ) > 120 ) {
					continue;
				}
				//FIXME: check if a teammate gets radial damage
			}
		}
		// will successfully hit enemy
		return qtrue;
	}
	//
	return qfalse;
}

/*
==================
AICast_CheckAttackAtPos
==================
*/
qboolean AICast_CheckAttackAtPos( int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld ) {
	gentity_t   *ent;
	vec3_t savepos;
	int saveview;
	qboolean rval;
	cast_state_t *cs;

	cs = AICast_GetCastState( entnum );
	ent = &g_entities[cs->bs->entitynum];

	VectorCopy( ent->r.currentOrigin, savepos );
	VectorCopy( pos, ent->r.currentOrigin );

	saveview = ent->client->ps.viewheight;
	if ( ducking ) {
		if ( ent->client->ps.viewheight != ent->client->ps.crouchViewHeight ) {
			ent->client->ps.viewheight = ent->client->ps.crouchViewHeight;
		}
	} else {
		if ( ent->client->ps.viewheight != ent->client->ps.standViewHeight ) {
			ent->client->ps.viewheight = ent->client->ps.standViewHeight;
		}
	}

	rval = AICast_CheckAttack_real( cs, enemy, allowHitWorld );

	VectorCopy( savepos, ent->r.currentOrigin );
	ent->client->ps.viewheight = saveview;

	return rval;
}

/*
==================
AICast_CheckAttack

  optimization, uses the cache to avoid possible duplicate calls with same world paramaters
==================
*/
qboolean AICast_CheckAttack( cast_state_t *cs, int enemy, qboolean allowHitWorld ) {
	if ( cs->bs ) {
		if (    ( cs->checkAttackCache.time == level.time )
				&&  ( cs->checkAttackCache.enemy == enemy )
				&&  ( cs->checkAttackCache.weapon == cs->bs->weaponnum )
				&&  ( cs->checkAttackCache.allowHitWorld == allowHitWorld ) ) {
			//G_Printf( "checkattack cache hit\n" );
			return ( cs->checkAttackCache.result );
		} else {
			cs->checkAttackCache.allowHitWorld = allowHitWorld;
			cs->checkAttackCache.enemy = enemy;
			cs->checkAttackCache.time = level.time;
			cs->checkAttackCache.weapon = cs->bs->weaponnum;
			return ( cs->checkAttackCache.result = AICast_CheckAttack_real( cs, enemy, allowHitWorld ) );
		}
	} else {
		return AICast_CheckAttack_real( cs, enemy, allowHitWorld );
	}
}

/*
==================
AICast_UpdateBattleInventory
==================
*/
void AICast_UpdateBattleInventory( cast_state_t *cs, int enemy ) {
	vec3_t dir;
	int i;

	if ( enemy >= 0 ) {
		VectorSubtract( cs->vislist[cs->bs->enemy].visible_pos, cs->bs->origin, dir );
		cs->enemyHeight = (int) dir[2];
		cs->enemyDist = (int) VectorLength( dir );
	}

	// stock up ammo that should never run out
	for ( i = 0; i < MAX_WEAPONS; i++ ) {
		if ( g_entities[cs->bs->entitynum].client->ps.ammo[ BG_FindAmmoForWeapon( i )] > 800 ) {
			//g_entities[cs->bs->entitynum].client->ps.ammo[ BG_FindAmmoForWeapon(i)] = 999;
			Add_Ammo( &g_entities[cs->entityNum], cs->bs->weaponnum, 999, qfalse );
		}
	}

	BotAI_GetClientState( cs->entityNum, &( cs->bs->cur_ps ) );

}

/*
==============
AICast_WeaponWantScale
==============
*/
float AICast_WeaponWantScale( cast_state_t *cs, int weapon ) {
	switch ( weapon ) {
	case WP_GAUNTLET:
		return 0.1;
	case WP_FLAMETHROWER:
		return 2.0;     // if we have this up close, definately use it
	default:
		return 1.0;
	}
}

/*
==============
AICast_GotEnoughAmmoForWeapon
==============
*/
qboolean AICast_GotEnoughAmmoForWeapon( cast_state_t *cs, int weapon ) {
	gentity_t *ent;
	int ammo, clip;

	ent = &g_entities[cs->entityNum];
	ammo = ent->client->ps.ammo[BG_FindAmmoForWeapon( weapon )];
	clip = ent->client->ps.ammoclip[BG_FindClipForWeapon( weapon )];

	// TODO!! check some kind of weapon list that holds the minimum requirements for each weapon
	switch ( weapon ) {
	case WP_GAUNTLET:
		return qtrue;
	default:
		return (qboolean)( ( clip >= ammoTable[weapon].uses ) || ( ammo >= ammoTable[weapon].uses ) );    //----(SA)
	}
}

/*
==============
AICast_WeaponUsable

  This is used to prevent weapons from being selected, even if they have ammo.

  This can be used to add a delay between firing for special attacks, or make certain
  that certain weapons are only selected within a certain range or under certain conditions.

  NOTE: that monster_attack2 will always override monster_attack1 if both are usable
==============
*/
qboolean AICast_WeaponUsable( cast_state_t *cs, int weaponNum ) {
	int delay, oldweap, hitclient;
	float dist = -1;
	gentity_t *ent, *grenade;

	if ( cs->bs->enemy >= 0 ) {
		dist = Distance( cs->bs->origin, g_entities[cs->bs->enemy].s.pos.trBase );
	}

	oldweap = cs->bs->weaponnum;
	ent = &g_entities[cs->entityNum];
	delay = -1;

	// just return qfalse if this weapon isn't ready for use
	switch ( weaponNum ) {
		// don't attempt to lob a grenade more than this often, since we will abort a grenade
		// throw if it's not safe, we shouldn't keep switching back too quickly
	case WP_DYNAMITE:
	case WP_DYNAMITE2:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
		if ( cs->bs->enemy < 0 ) {
			return qfalse;
		}
		delay = 5000;
		if ( dist > 0 && dist < 200 ) {
			return qfalse;
		}
		if ( cs->weaponFireTimes[weaponNum] < level.time - delay ) {
			// make sure it's safe
			CalcMuzzlePoints( ent, weaponNum );
			grenade = weapon_grenadelauncher_fire( ent, weaponNum );
			hitclient = AICast_SafeMissileFire( grenade, grenade->nextthink - level.time, cs->bs->enemy, g_entities[cs->bs->enemy].s.pos.trBase, cs->entityNum, NULL );
			G_FreeEntity( grenade );
			if ( hitclient > -1 ) {
				return qtrue;
			} else {
				return qfalse;  // it's not safe
			}
		}
		break;
	case WP_ROCKET_LAUNCHER:
		switch ( cs->aiCharacter ) {
		case AICHAR_STIMSOLDIER2:
			return qfalse;  // stime only uses Rocket Launcher in jumping attack
			//delay = 1000 + (cs->weaponFireTimes[weaponNum]%2000);	// somewhat randomize it so it's less predictable
			//break;
		}
		break;
	case WP_TESLA:
		switch ( cs->aiCharacter ) {
		case AICHAR_STIMSOLDIER3:
			if ( dist < 0 || dist >= TESLA_RANGE ) {
				return qfalse;
			}
		}
		break;
	case WP_MONSTER_ATTACK1:
		switch ( g_entities[cs->entityNum].aiCharacter ) {
		case AICHAR_ZOMBIE: // zombie flaming attack
			delay = 4000;
			if ( dist < 0 ) { // || dist < 128) {
				return qfalse;
			}
			if ( dist > 1200 ) {
				return qfalse;
			}
			if ( cs->bs->enemy < 0 ) {
				return qfalse;
			}
			//if (cs->vislist[cs->bs->enemy].notvisible_timestamp > level.time - 500) {
			//	return qfalse;
			//}
			break;

			// melee attacks are always available
		case AICHAR_LOPER:
		case AICHAR_WARZOMBIE:
		case AICHAR_REJECTX:
		case AICHAR_SEALOPER:
			return qtrue;   // always usable

		case AICHAR_STIMSOLDIER2:
			delay = 7000;
			if ( dist < 0 || dist < 300 ) {
				return qfalse;
			}
			break;
		case AICHAR_STIMSOLDIER3:   // stim flying tesla attack
			delay = 7000;
			if ( dist < 0 || dist < 300 ) {
				return qfalse;
			}
			break;
		case AICHAR_BLACKGUARD:
			delay = 2000;
			if ( dist < 0 || dist > BLACKGUARD_MELEE_RANGE ) {
				return qfalse;
			}
			break;
		default:
			delay = -1;
			break;
		}
		break;
	case WP_MONSTER_ATTACK2:
		switch ( g_entities[cs->entityNum].aiCharacter ) {
		case AICHAR_WARZOMBIE:
			delay = 9999999;
			break;
		case AICHAR_ZOMBIE:
			delay = 6000;
			// zombie "flying spirit" attack
			if ( dist < 0 || dist < 64 ) {
				return qfalse;
			}
			if ( dist > 1200 ) {
				return qfalse;
			}
			if ( cs->bs->enemy < 0 ) {
				return qfalse;
			}
			if ( cs->vislist[cs->bs->enemy].notvisible_timestamp > level.time - 500 ) {
				return qfalse;
			}
			break;
		case AICHAR_LOPER:  // loper leap attack
			delay = 3500;
			if ( dist < 200 ) {
				return qfalse;
			}
			break;
		case AICHAR_FEMZOMBIE:  // hand lightning
			delay = 2500;
			break;
		default:
			delay = -1;
			break;
		}
		break;
	case WP_MONSTER_ATTACK3:
		switch ( g_entities[cs->entityNum].aiCharacter ) {
		case AICHAR_LOPER:  // loper ground zap
			delay = 3500;
			if ( dist < 0 || dist > LOPER_GROUND_RANGE ) {
				return qfalse;
			}
			break;
		case AICHAR_WARZOMBIE:  // warzombie defense
			delay = 2000;
			if ( dist < 0 || dist > 2000 ) {
				return qfalse;
			}
			break;
		default:
			delay = -1;
			break;
		}
		break;
	default:
		delay = -1;
	}
	//
	return ( !cs->weaponFireTimes[weaponNum] || ( cs->weaponFireTimes[weaponNum] < level.time - delay ) );
}

/*
==============
AICast_ChooseWeapon
==============
*/
void AICast_ChooseWeapon( cast_state_t *cs, qboolean battleFunc ) {
	int i;
	int *ammo;
	float wantScale, bestWantScale;

	BotAI_GetClientState( cs->entityNum, &( cs->bs->cur_ps ) );
	ammo = cs->bs->cur_ps.ammo;
	bestWantScale = 0.0;
	// read back in the weapon that we are really using
	//cs->bs->weaponnum = cs->bs->cur_ps.weapon;

	if ( cs->bs->cur_ps.weaponstate == WEAPON_RAISING ||
		 cs->bs->cur_ps.weaponstate == WEAPON_DROPPING ) {
		return;
	}

// disabled this, makes grenade guy keep trying to throw a grenade he doesn't have
//	if (cs->bs->cur_ps.weaponDelay || cs->bs->cur_ps.weaponTime)
//		return;

	if ( cs->bs->weaponnum && ( cs->castScriptStatus.scriptFlags & SFL_NOCHANGEWEAPON ) ) {
		if ( AICast_GotEnoughAmmoForWeapon( cs, cs->bs->weaponnum ) && AICast_WeaponUsable( cs, cs->bs->weaponnum ) ) {
			return;
		} else {
			cs->castScriptStatus.scriptFlags &= ~SFL_NOCHANGEWEAPON;
		}
	}

	// choose the best weapon to fight with
	for ( i = 0; i < MAX_WEAPONS; i++ ) {
		if ( i == WP_GRENADE_LAUNCHER || i == WP_GRENADE_PINEAPPLE ) {
			continue;   // never choose grenades at will, only when going into grenade flush mode
		}

		if ( !battleFunc && ( i == WP_MONSTER_ATTACK1 ) && cs->aifuncAttack1 ) {
			continue;   // only choose this weapon from within AIFunc_BattleStart()
		}
		if ( !battleFunc && ( i == WP_MONSTER_ATTACK2 ) && cs->aifuncAttack2 ) {
			continue;   // only choose this weapon from within AIFunc_BattleStart()
		}
		if ( !battleFunc && ( i == WP_MONSTER_ATTACK3 ) && cs->aifuncAttack3 ) {
			continue;   // only choose this weapon from within AIFunc_BattleStart()
		}

		if ( COM_BitCheck( cs->bs->cur_ps.weapons, i ) ) {
			// check that our ammo is enough
			if ( !AICast_GotEnoughAmmoForWeapon( cs, i ) ||
				 !AICast_WeaponUsable( cs, i ) ) {
				continue;
			}
			// get the wantScale for this weapon given the current circumstances (0.0 - 1.0)
			wantScale = AICast_WeaponWantScale( cs, i );
			//
			if ( wantScale >= bestWantScale ) {
				cs->bs->weaponnum = i;
				bestWantScale = wantScale;
			}
		}
	}
}

/*
==================
AICast_Aggression

  Check all possible reasons why we shouldn't attack, returning a value from 1.0 (fully willing)
  to 0.0 (please don't hurt me).
==================
*/
float AICast_Aggression( cast_state_t *cs ) {
	bot_state_t *bs;
	float scale, dist;
	int painTime;
	int     *ammo;

	bs = cs->bs;

	// if we are out of ammo, we should never chase
	ammo = cs->bs->cur_ps.ammo;
	if ( g_entities[cs->entityNum].aiTeam != AITEAM_MONSTER ) {
		if ( !AICast_GotEnoughAmmoForWeapon( cs, cs->bs->weaponnum ) ) {
			return 0;
		}
	}

	// start fully willing to attack
	scale = 1.0;

	//if the enemy is located way higher
	//if (cs->enemyHeight > 200)
	//	scale -= (cs->enemyHeight)/800.0;

	//if very low on health
	if ( bs->cur_ps.stats[STAT_HEALTH] < 50 ) {
		scale -= ( 1.0 - cs->attributes[AGGRESSION] ) * ( 1.0 - ( (float)bs->cur_ps.stats[STAT_HEALTH] / 50.0 ) );
	}

	// if they've recently hit us, factor that in, so we get scared off by being
	// damaged, but later return once we've regained our confidence
	painTime = 15000 - (int)( 10000.0 * cs->attributes[AGGRESSION] * cs->attributes[AGGRESSION] );
	if ( cs->lastPain + painTime > level.time ) {
		scale -= 3 * ( 1.0 - cs->attributes[AGGRESSION] )   * ( (float)( cs->lastPain + painTime - level.time ) / (float)painTime );
	}

	// if we just rolled, stay out of view if we jumped behind cover
	painTime = 10000 - (int)( 10000.0 * cs->attributes[AGGRESSION] * cs->attributes[AGGRESSION] );
	if ( cs->battleRollTime + painTime > level.time ) {
		scale -= 2 * ( 1.0 - cs->attributes[AGGRESSION] )   * ( (float)( cs->battleRollTime + painTime - level.time ) / (float)painTime );
	}

	// gain in confidence the further we are away
	if ( cs->bs->enemy >= 0 ) {
		dist = Distance( cs->bs->origin, g_entities[bs->enemy].s.pos.trBase );
		//if (dist > 512) {
		scale += ( dist - 800.0 ) / ( 8000.0 );
		//}
	}

	// if our weapon is reloading, we should hide
	if ( cs->bs->cur_ps.weaponTime > 0 ) {
		scale -= ( (float)cs->bs->cur_ps.weaponTime / 1000.0 );
	}

	scale *= cs->attributes[AGGRESSION];

	// this should increase the chances of an ambush attack
	if ( ( ( level.time + 2000 * g_entities[cs->entityNum].aiTeam ) % ( 4000 + 500 * g_entities[cs->entityNum].aiTeam ) ) > 4000 ) {
		scale += 0.3;
	}

	if ( scale < 0 ) {
		scale = 0;
	}

	return scale;
}

/*
==================
AICast_WantsToChase
==================
*/
int AICast_WantsToChase( cast_state_t *cs ) {
	int     *ammo;
	ammo = cs->bs->cur_ps.ammo;
	if ( g_entities[cs->entityNum].aiTeam != AITEAM_MONSTER ) {
		if ( !AICast_GotEnoughAmmoForWeapon( cs, cs->bs->weaponnum ) ) {
			return qfalse;
		}
	}
	if ( cs->attributes[AGGRESSION] == 1.0 ) {
		return qtrue;
	}
	if ( AICast_Aggression( cs ) > 0.6 ) {
		return qtrue;
	}
	return qfalse;
}

/*
==================
AICast_WantsToTakeCover
==================
*/
int AICast_WantsToTakeCover( cast_state_t *cs, qboolean attacking ) {
	float aggrScale;
	int     *ammo;

	ammo = cs->bs->cur_ps.ammo;
	if ( g_entities[cs->entityNum].aiTeam != AITEAM_MONSTER ) {
		if ( !cs->bs->weaponnum ) {
			return qtrue;
		}
		if ( !AICast_GotEnoughAmmoForWeapon( cs, cs->bs->weaponnum ) ) {
			return qtrue;
		}
	}
	if ( cs->attributes[AGGRESSION] == 1.0 ) {
		return qfalse;
	}
	// if currently attacking, we should stick around if not getting hurt
	if ( attacking ) {
		aggrScale = 1.2;
	} else { aggrScale = 0.8 /*+ 0.4 * random()*/;}
	//
	// if currently following someone, we should be more aggressive
	if ( cs->leaderNum >= 0 ) {
		aggrScale *= 3;
	}
	//
	// Dodge enemy aim?
	if ( cs->attributes[AGGRESSION] < 1.0 && attacking && ( cs->bs->enemy >= 0 ) && ( g_entities[cs->bs->enemy].client->ps.weapon ) && ( cs->attributes[TACTICAL] > 0.5 ) && ( cs->aiFlags & AIFL_ROLL_ANIM ) && ( VectorLength( cs->bs->cur_ps.velocity ) < 1 ) ) {
		vec3_t aim, enemyVec;
		// are they aiming at us?
		AngleVectors( g_entities[cs->bs->enemy].client->ps.viewangles, aim, NULL, NULL );
		VectorSubtract( cs->bs->origin, g_entities[cs->bs->enemy].r.currentOrigin, enemyVec );
		VectorNormalize( enemyVec );
		// if they are looking at us, we should avoid them
		if ( DotProduct( aim, enemyVec ) > 0.97 ) {
			//G_Printf("%s: I'm in danger, I should probably avoid\n", g_entities[cs->entityNum].aiName);
			aggrScale *= 0.6;
		}
	}
	//
	// FIXME: instead of a constant, call a "attack danger"
	// function, so we only attack if our aggression is greater than
	// the danger
	if ( AICast_Aggression( cs ) * aggrScale < 0.4 ) {
		//G_Printf("%s: run for your life!\n", g_entities[cs->entityNum].aiName);
		return qtrue;
	}
	//
	return qfalse;
}

/*
==================
AICast_CombatMove
==================
*/
bot_moveresult_t AICast_CombatMove( cast_state_t *cs, int tfl ) {
	bot_state_t *bs;
	float attack_skill, croucher, dist;
	vec3_t forward, backward; //, up = {0, 0, 1};
	bot_moveresult_t moveresult;
	bot_goal_t goal;

	bs = cs->bs;

	//get the enemy entity info
	memset( &moveresult, 0, sizeof( bot_moveresult_t ) );
	//
	attack_skill = cs->attributes[ATTACK_SKILL];
	croucher = ( cs->attributes[ATTACK_CROUCH] > 0.1 );

	//initialize the movement state
	BotSetupForMovement( bs );
	//direction towards the enemy
	VectorSubtract( cs->vislist[cs->bs->enemy].visible_pos, bs->origin, forward );
	//the distance towards the enemy
	dist = VectorNormalize( forward );
	VectorNegate( forward, backward );
	//
	// do we have somewhere we are trying to get to?
	if ( cs->combatGoalTime > level.time ) {
		if ( VectorLength( cs->combatGoalOrigin ) > 1 ) {
			//create the chase goal
			goal.areanum = BotPointAreaNum( cs->combatGoalOrigin );
			VectorCopy( cs->combatGoalOrigin, goal.origin );

			// if we are really close, stop going for it
			// FIXME: a better way of doing this, so we don't stop short of the goal?
			if ( ( dist = Distance( goal.origin, cs->bs->origin ) ) < 32 ) {
				if ( cs->combatGoalTime > level.time + 3000 ) {
					cs->combatGoalTime = level.time + 2000 + rand() % 1000;
					cs->combatSpotDelayTime = level.time + 4000 + rand() % 3000;
				}
				VectorClear( cs->combatGoalOrigin );
			} else {
				aicast_predictmove_t move;
				//
				AICast_MoveToPos( cs, cs->combatGoalOrigin, -1 );
				cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, 32 );
				//
				// if we are going to move out of view very soon, stop moving
				AICast_PredictMovement( cs, 1, 0.8, &move, &cs->bs->lastucmd, -1 );
				//
				if ( move.numtouch || !AICast_CheckAttackAtPos( cs->entityNum, cs->bs->enemy, move.endpos, qfalse, qfalse ) ) {
					// abort the manouver, reached a good spot
					cs->combatGoalTime = 0;
					cs->combatSpotAttackCount = cs->startAttackCount;
				}
			}

			// if we are there, and the enemy can see us, but we cant hit them, abort immediately
		} else if ( !AICast_CheckAttack( cs, cs->bs->enemy, qfalse ) &&
					AICast_VisibleFromPos( cs->vislist[cs->bs->enemy].visible_pos, cs->bs->enemy, cs->bs->origin, cs->entityNum, qfalse ) ) {
			cs->combatGoalTime = 0;
			cs->combatSpotAttackCount = cs->startAttackCount;
		}
	} else {    // look for a good position to move to?
		if (    (   ( cs->attributes[CAMPER] < random() )
					&&  ( cs->takeCoverTime < level.time )
					&&  ( cs->combatSpotAttackCount < cs->startAttackCount )
					&&  ( cs->combatSpotDelayTime < level.time ) ) ) {

			if (    ( cs->attributes[TACTICAL] > 0.3 + random() * 0.5 )
					&&  trap_AAS_RT_GetHidePos( cs->bs->origin, cs->bs->entitynum, cs->bs->areanum, cs->vislist[cs->bs->enemy].visible_pos, bs->enemy, BotPointAreaNum( cs->vislist[cs->bs->enemy].visible_pos ), cs->combatGoalOrigin ) ) {
				cs->combatGoalTime = level.time + 10000;                // give us plenty of time to get there
				//cs->combatSpotAttackCount = cs->startAttackCount + 3;	// don't keep moving around to different positions on our own
				cs->combatSpotDelayTime = level.time + 3000 + rand() % 3000;
			} else {
				// don't look again until we've moved
				//cs->combatSpotAttackCount = cs->startAttackCount;
				cs->combatSpotDelayTime = level.time + 3000 + rand() % 3000;
			}
		}
	}
	//
	return moveresult;
}

/*
==================
AICast_WeaponSway

  Some weapons should be "sprayed" around a bit while firing
==================
*/
void AICast_WeaponSway( cast_state_t *cs, vec3_t ofs ) {
	VectorClear( ofs );
	switch ( cs->bs->weaponnum ) {
	case WP_MONSTER_ATTACK1:
		if ( cs->aiCharacter != AICHAR_ZOMBIE ) {
			break;      // only allow flaming zombie beyond here
		}
	case WP_FLAMETHROWER:
		ofs[PITCH] = ( 3.0 + 4.0 * sin( ( (float)level.time / 320.0 ) ) ) * sin( ( (float)level.time / 500.0 ) );
		ofs[YAW] = ( 6.0 + 8.0 * sin( ( (float)level.time / 250.0 ) ) ) * sin( ( (float)level.time / 400.0 ) );
		ofs[ROLL] = 0;
		break;
	case WP_VENOM:
		ofs[PITCH] = 2 * (float)cos( ( level.time / 200 ) );
		ofs[YAW] = 10 * (float)sin( ( level.time / 150 ) ) * (float)sin( ( level.time / 100 ) );
		ofs[ROLL] = 0;
		break;
	}
}

/*
==================
AICast_AimAtEnemy
==================
*/
qboolean AICast_AimAtEnemy( cast_state_t *cs ) {
	bot_state_t *bs;
	float aim_skill, aim_accuracy;
	vec3_t dir, bestorigin, start, enemyOrg;
	vec3_t mins = {-4,-4,-4}, maxs = {4, 4, 4};
	bsp_trace_t trace;
	float dist;
	cast_visibility_t *vis;

	//
	if ( cs->castScriptStatus.scriptNoAttackTime >= ( level.time + 500 ) ) {
		return qfalse;
	}
	//
	if ( cs->noAttackTime >= level.time ) {
		return qfalse;
	}
	//
	bs = cs->bs;
	//
	//if the bot has no enemy
	if ( bs->enemy < 0 ) {
		return qfalse;
	}
	//
	aim_skill = cs->attributes[AIM_SKILL];
	aim_accuracy = AICast_GetAccuracy( cs->entityNum );
	if ( aim_accuracy <= 0 ) {
		aim_accuracy = 0.0001;
	}

	// StimSoldier is very good at firing Rocket Launcher
	if ( cs->aiCharacter == AICHAR_STIMSOLDIER2 && cs->bs->weaponnum == WP_ROCKET_LAUNCHER ) {
		aim_skill = 1;
		aim_accuracy = 1;
	}

	//get the weapon information

	//get the enemy entity information
	vis = &cs->vislist[bs->enemy];
	if ( vis->visible_timestamp < vis->lastcheck_timestamp ) {
		// use our last visible position of them
		if ( vis->real_visible_timestamp == vis->lastcheck_timestamp ) {
			VectorCopy( vis->real_visible_pos, bestorigin );
		} else {
			VectorCopy( vis->visible_pos, bestorigin );
		}
	} else {
		// we can see them, if this weapon isn't a direct-hit weapon (bullets),
		// then predict where they are going to be
		if ( cs->bs->weaponnum == WP_GRENADE_LAUNCHER || cs->bs->weaponnum == WP_GRENADE_PINEAPPLE ) {
			aicast_predictmove_t move;
			AICast_PredictMovement( AICast_GetCastState( cs->bs->enemy ), 1, 1.0, &move, &g_entities[bs->enemy].client->pers.cmd, -1 );
			VectorCopy( move.endpos, bestorigin );
		} else {    // they are visible, use actual position
			VectorCopy( g_entities[bs->enemy].client->ps.origin, bestorigin );
		}
	}

	bestorigin[2] += g_entities[bs->enemy].client->ps.viewheight;
	//get the start point shooting from
	//NOTE: the x and y projectile start offsets are ignored
	VectorCopy( bs->origin, start );
	start[2] += bs->cur_ps.viewheight;
	//
	VectorCopy( cs->vislist[bs->enemy].visible_pos, enemyOrg );
	//
	// grenade hack: aim grenades at their feet if they are close
	if ( cs->bs->weaponnum == WP_GRENADE_LAUNCHER || cs->bs->weaponnum == WP_GRENADE_PINEAPPLE ) {
		if ( Distance( start, bestorigin ) < 180 ) {
			bestorigin[2] = enemyOrg[2] + g_entities[bs->enemy].r.mins[2] + crandom() * 20;
		} else if ( Distance( start, bestorigin ) > 400 ) { // aim up a bit for distance
			bestorigin[2] += 12 + Distance( start, bestorigin ) / 50 + crandom() * 20;
		}
	}
	dist = Distance( bs->eye, bestorigin );
	// rocket launcher should aim ahead
	if ( bs->weaponnum == WP_ROCKET_LAUNCHER || bs->weaponnum == WP_PANZERFAUST ) {
		VectorMA( bestorigin, aim_skill * aim_skill * ( dist / 900 ), g_entities[bs->enemy].client->ps.velocity, bestorigin );
	}
	//
	BotAI_Trace( &trace, start, mins, maxs, bestorigin, bs->entitynum, MASK_SHOT );
	//if the enemy is NOT hit
	if ( trace.fraction <= 1 && trace.ent != bs->enemy ) {
		if ( bs->weaponnum == WP_ROCKET_LAUNCHER || bs->weaponnum == WP_PANZERFAUST ) {
			if ( Distance( trace.endpos, bestorigin ) > 100 ) {
				// remove the prediction
				VectorMA( bestorigin, aim_skill * aim_skill * ( dist / 900 ), g_entities[bs->enemy].client->ps.velocity, bestorigin );
				// aim downwards
				bestorigin[2] -= 16;
			}
		} else {
			bestorigin[2] += 16;
		}
	}
	//
	// adjust accuracy with distance if upclose, so we don't start firing the wrong direction
/*
	scale = 1;
	switch (cs->bs->weaponnum) {
	// these weapons don't do random offsetting
	case WP_FLAMETHROWER:
		break;
	default:
		{
			float scale;

			if (dist < 256)
				scale *= (dist / 256);
			else
				scale *= 1.0;

			bestorigin[0] += scale * 96 * sin((float)level.time/(200.0 + (40.0*((cs->entityNum+3)%4)))) * (1 - aim_accuracy);
			bestorigin[1] += scale * 96 * cos((float)level.time/(220.0 + (36.0*((cs->entityNum+1)%5)))) * (1 - aim_accuracy);
			bestorigin[2] += scale * 48 * sin((float)level.time/(210.0 + (32.0*((cs->entityNum+2)%6)))) * (1 - aim_accuracy);
		}
		break;
	}
*/
	// if the enemy is moving, they are harder to hit
	if ( dist > 256 ) {
		VectorMA( bestorigin, ( 0.3 + 0.7 * ( 1 - aim_accuracy ) ) * 0.4 * sin( (float)level.time / ( 500.0 + ( 100.0 * ( ( cs->entityNum + 3 ) % 4 ) ) ) ), g_entities[bs->enemy].client->ps.velocity, bestorigin );
	}
	//get aim direction
	VectorSubtract( bestorigin, bs->eye, dir );
	//set the ideal view angles
	vectoangles( dir, bs->ideal_viewangles );

	return qtrue;   // do real aim checking after we've moved the angles
}

/*
==================
AICast_CanMoveWhileFiringWeapon
==================
*/
qboolean AICast_CanMoveWhileFiringWeapon( int weaponnum ) {
	switch ( weaponnum ) {
	case WP_MAUSER:
	case WP_GARAND:
	case WP_SNIPERRIFLE:    //----(SA)	added
	case WP_SNOOPERSCOPE:   //----(SA)	added
	case WP_FG42SCOPE:      //----(SA)	added
	case WP_PANZERFAUST:
	case WP_ROCKET_LAUNCHER:
		return qfalse;
	default:
		return qtrue;
	}
}

/*
================
AICast_RandomTriggerRelease
================
*/
qboolean AICast_RandomTriggerRelease( cast_state_t *cs ) {
	// some characters override all weapon settings for trigger release
	switch ( cs->aiCharacter ) {
	case AICHAR_BLACKGUARD:     // this is here since his "ready" frame is different to his firing frame, so it looks wierd to keep swapping between them
	case AICHAR_STIMSOLDIER1:
	case AICHAR_STIMSOLDIER2:
	case AICHAR_STIMSOLDIER3:
		return qfalse;
		break;
	}

	switch ( cs->bs->weaponnum ) {
	case WP_MP40:
	case WP_VENOM:
		//case WP_FLAMETHROWER:
		return qtrue;
	default:
		return qfalse;
	}
}

/*
==================
AICast_ProcessAttack

  NOTE: this should always be called after the movement has been processed
==================
*/
void AICast_ProcessAttack( cast_state_t *cs ) {
	bot_state_t *bs;

	// if our enemy is dead, stop attacking them
	if ( cs->bs->enemy >= 0 && g_entities[cs->bs->enemy].health <= 0 ) {
		return;
	}
	//
	if ( cs->castScriptStatus.scriptNoAttackTime >= level.time ) {
		return;
	}
	//
	if ( cs->noAttackTime >= level.time ) {
		return;
	}
	// select a weapon
	AICast_ChooseWeapon( cs, qfalse );
	//
	bs = cs->bs;
	// check for not firing while moving flag, if present, abort attack if any movement has been issued
	if ( !AICast_CanMoveWhileFiringWeapon( cs->bs->weaponnum ) ) {
		// if we are moving, don't fire
		bot_input_t bi;
		if ( cs->bs->cur_ps.weaponTime > 200 ) {
			// if we recently fired, don't let us move for a bit
			cs->speedScale = 0;
			AICast_AimAtEnemy( cs );    // keep looking at them regardless
		}
		// if we're trying to move somewhere, don't let us shoot, until we've arrived
		trap_EA_GetInput( bs->client, (float) level.time / 1000, &bi );
		if (    ( cs->castScriptStatus.scriptNoMoveTime < level.time ) &&
				(   ( bi.actionflags & ACTION_MOVEFORWARD ) ||
					( bi.actionflags & ACTION_MOVEBACK ) ||
					( bi.actionflags & ACTION_MOVELEFT ) ||
					( bi.actionflags & ACTION_MOVERIGHT ) ||
					( bi.speed ) ) ) {
			return;
		}
	}
	//
	// if we are stuck in this position, we should duck if we can't hit them
	if ( !AICast_CheckAttack( cs, bs->enemy, qfalse ) ) {
		// we should duck if the enemy is shooting at us, and we can't hit them
		if ( cs->attributes[ATTACK_CROUCH] && ( cs->castScriptStatus.scriptNoMoveTime >= level.time ) ) {
			if ( !AICast_CheckAttackAtPos( cs->entityNum, bs->enemy, cs->bs->origin, qfalse, qfalse ) ) {
				cs->bs->attackcrouch_time = trap_AAS_Time() + 2;
			} else {
				cs->bs->attackcrouch_time = 0;  // we can attack them if we stand, so go for it
			}
		}
		return;
	}
	//
	//aim at the enemy
	if ( !AICast_AimAtEnemy( cs ) ) {
		// not fully facing them yet
		return;
	}
	//
	// release the trigger every now and then
	if ( AICast_RandomTriggerRelease( cs ) && cs->triggerReleaseTime < ( level.time - 500 ) ) {
		if ( rand() % 5 == 0 ) {
			cs->triggerReleaseTime = level.time + 100 + rand() % 100;
			return;
		}
	}
	//
	if ( cs->triggerReleaseTime > level.time ) {
		return;
	}
	//
	// FIXME: handle fire-on-release weapons?
	trap_EA_Attack( bs->client );
	//
	bs->flags |= BFL_ATTACKED;

}

/*
==============
AICast_GetTakeCoverPos
==============
*/
qboolean AICast_GetTakeCoverPos( cast_state_t *cs, int enemyNum, vec3_t enemyPos, vec3_t returnPos ) {
	cs->crouchHideFlag = qfalse;
	//
	if ( cs->castScriptStatus.scriptNoMoveTime > level.time ) {
		return qfalse;
	}
	//
	cs->lastGetHidePos = level.time;
	//
	// can we just crouch?
	if (    ( cs->bs->attackcrouch_time < trap_AAS_Time() )
			&&  ( enemyNum < aicast_maxclients )
			&&  AICast_CheckAttackAtPos( cs->entityNum, enemyNum, cs->bs->origin, qfalse, qfalse )
			&&  !AICast_CheckAttackAtPos( cs->entityNum, enemyNum, cs->bs->origin, qtrue, qfalse ) ) {

		// do a more thorough check to see if the enemy can see us if we crouch
		vec3_t omaxs;
		gentity_t *ent;
		qboolean visible;

		ent = &g_entities[cs->entityNum];
		VectorCopy( ent->r.maxs, omaxs );
		ent->r.maxs[2] = ent->client->ps.crouchMaxZ + 4;    // + 4 to be safe

		visible = AICast_VisibleFromPos( g_entities[enemyNum].r.currentOrigin, enemyNum, cs->bs->origin, cs->entityNum, qfalse );

		ent->r.maxs[2] = omaxs[2];

		if ( !visible ) {
			VectorCopy( enemyPos, cs->takeCoverEnemyPos );
			VectorCopy( cs->bs->origin, returnPos );
			cs->crouchHideFlag = qtrue;
			return qtrue;
		}
	}
	// look for a hiding spot
	if ( trap_AAS_RT_GetHidePos( cs->bs->origin, cs->bs->entitynum, cs->bs->areanum, enemyPos, enemyNum, BotPointAreaNum( enemyPos ), returnPos ) ) {
		return qtrue;
	}
	// if we are hiding from a dangerous entity, try and avoid it
	if ( cs->dangerEntity == enemyNum && cs->dangerEntityValidTime > level.time ) {
		if ( cs->dangerLastGetAvoid > level.time - 750 ) {
			return qtrue;
		} else if ( AICast_GetAvoid( cs, NULL, cs->takeCoverPos, qtrue, cs->dangerEntity ) ) {
			cs->dangerLastGetAvoid = level.time;
			return qtrue;
		}
	}
	//
	return qfalse;
}

/*
==============
AICast_AIDamageOK
==============
*/
qboolean AICast_AIDamageOK( cast_state_t *cs, cast_state_t *ocs ) {
	if ( cs->castScriptStatus.scriptFlags & SFL_NOAIDAMAGE ) {
		return qfalse;
	} else {

		if ( cs->aiCharacter == AICHAR_LOPER && ocs->aiCharacter == AICHAR_LOPER ) {
			return qfalse;
		}

		return qtrue;
	}
}

/*
===============
AICast_RecordWeaponFire

  used for scripting, so we know when the weapon has been fired
===============
*/
void AICast_RecordWeaponFire( gentity_t *ent ) {
	cast_state_t *cs;
	float range;

	cs = AICast_GetCastState( ent->s.number );
	cs->lastWeaponFired = level.time;
	cs->lastWeaponFiredWeaponNum = ent->client->ps.weapon;
	VectorCopy( ent->r.currentOrigin, cs->lastWeaponFiredPos );

	cs->weaponFireTimes[cs->lastWeaponFiredWeaponNum] = level.time;

	// do sighting
	range = AICast_GetWeaponSoundRange( cs->lastWeaponFiredWeaponNum );

	AICast_AudibleEvent( cs->entityNum, cs->lastWeaponFiredPos, range );
	//AICast_SightSoundEvent( cs, range );
	/*
	if (!cs->sightSoundTime || cs->sightSoundRange < range) {
		cs->sightSoundRange = range;
		cs->sightSoundTime = level.time;
	}
	*/

	if ( cs->bs ) {   // real player's don't need to play AI sounds
		AIChar_AttackSound( cs );
	}
}

/*
===============
AICast_GetWeaponSoundRange
===============
*/
float AICast_GetWeaponSoundRange( int weapon ) {
	// NOTE: made this a case, that way changing the ordering of weapons won't cause problems, as it would
	// with an array lookup

	switch ( weapon ) {
	case    WP_NONE:
		return 0;
	case    WP_KNIFE:
	case    WP_KNIFE2:
	case    WP_GAUNTLET:
	case    WP_SPEARGUN:
	case    WP_SPEARGUN_CO2:
	case    WP_STEN:
	case    WP_SILENCER:
		return 64;
	case    WP_GRENADE_LAUNCHER:
	case    WP_GRENADE_PINEAPPLE:
	case    WP_PROX:    //----(SA)	added
		return 1500;
	case    WP_SNIPERRIFLE:
		return 1500;
	case    WP_SNOOPERSCOPE:
		return 128;
	case    WP_LUGER:
	case    WP_COLT:
	case    WP_AKIMBO:
		return 700;

	case    WP_MONSTER_ATTACK1:
	case    WP_MONSTER_ATTACK2:
	case    WP_MONSTER_ATTACK3:
		// TODO: case for each monster
		return 1000;

	case    WP_GARAND:
	case    WP_MP40:
	case    WP_THOMPSON:
		return 1000;

	case    WP_BAR:
	case    WP_BAR2:
		return 2000;    //----(SA)	added

	case    WP_FG42:
	case    WP_FG42SCOPE:
		return 1500;

	case    WP_MAUSER:  //----(SA)	change for DM
		return 1500;

	case    WP_DYNAMITE:
	case    WP_DYNAMITE2:
		return 3000;

	case    WP_PANZERFAUST:
	case    WP_ROCKET_LAUNCHER:
	case    WP_VENOM:
	case    WP_VENOM_FULL:
	case    WP_FLAMETHROWER:
	case    WP_CROSS:
	case    WP_TESLA:
		return 1000;
	}

	G_Error( "AICast_GetWeaponSoundRange: unknown weapon index: %i\n", weapon );
	return 0;   // shutup the compiler
}

/*
===============
AICast_StopAndAttack

  returns qtrue if they should go back to a battle state to attack,
  qfalse if they should keep chasing while they attack (like the Zombie)
===============
*/
qboolean AICast_StopAndAttack( cast_state_t *cs ) {
	float dist = -1;

	if ( cs->bs->enemy >= 0 ) {
		dist = Distance( cs->bs->origin, g_entities[cs->bs->enemy].r.currentOrigin );
	}
/*
	switch (cs->bs->weaponnum) {

		// removed

	}
*/
	return qtrue;
}

/*
===============
AICast_GetAccuracy
===============
*/
float AICast_GetAccuracy( int entnum ) {
	#define AICAST_VARIABLE_ACC_ENABLED     1
	#define AICAST_ACC_VISTIME  4000
	#define AICAST_ACC_SCALE    0.4
	cast_state_t *cs;
	float acc;

	cs = AICast_GetCastState( entnum );
	// the more they stay in our sights, the more accurate we get
	acc = cs->attributes[AIM_ACCURACY];

	if ( AICAST_VARIABLE_ACC_ENABLED ) {
		if ( cs->bs->enemy >= 0 ) {
			if ( cs->vislist[cs->bs->enemy].real_notvisible_timestamp < level.time - AICAST_ACC_VISTIME ) {
				acc += AICAST_ACC_SCALE;
			} else {
				acc += AICAST_ACC_SCALE * ( (float)( level.time - cs->vislist[cs->bs->enemy].real_notvisible_timestamp ) / (float)( AICAST_ACC_VISTIME ) );
			}

			if ( acc > 1.0 ) {
				acc = 1.0;
			} else if ( acc < 0.0 ) {
				acc = 0.0;
			}
		}
	}
	return ( acc );
}

/*
==============
AICast_WantToRetreat
==============
*/
qboolean AICast_WantToRetreat( cast_state_t *cs ) {
	int     *ammo;

	ammo = cs->bs->cur_ps.ammo;
	if ( g_entities[cs->entityNum].aiTeam != AITEAM_MONSTER ) {
		if ( !cs->bs->weaponnum ) {
			return qtrue;
		}
		if ( !AICast_GotEnoughAmmoForWeapon( cs, cs->bs->weaponnum ) ) {
			return qtrue;
		}
	}

	if ( cs->attributes[AGGRESSION] >= 1.0 ) {
		return qfalse;
	}

	if  ( cs->leaderNum < 0 ) {
		if  (   ( cs->attributes[TACTICAL] > 0.11 + random() * 0.5 ) &&
				(   ( cs->bs->cur_ps.weaponTime > 500 ) ||
					(   ( cs->takeCoverTime < level.time - 100 ) &&
					( AICast_WantsToTakeCover( cs, qtrue ) ) ) ) ) {
			return qtrue;
		}
	}
	//
	return qfalse;
}

/*
==============
AICast_SafeMissileFire

  checks to see if firing the missile will be successful, neutral, or dangerous to us or a friendly
==============
*/
int AICast_SafeMissileFire( gentity_t *ent, int duration, int enemyNum, vec3_t enemyPos, int selfNum, vec3_t endPos ) {
	int rval;
	vec3_t org;
	gentity_t   *trav;

	if ( !G_PredictMissile( ent, duration, org, qtrue ) ) {
		// not point firing, since it won't explode
		return 0;
	}

	if ( endPos ) {
		VectorCopy( org, endPos );
	}

	// at end of life, so do radius damage
	rval = ( Distance( org, enemyPos ) < ent->splashRadius ) && AICast_VisibleFromPos( org, ent->s.number, enemyPos, enemyNum, qfalse );
	if ( rval ) {
		// don't hurt ourselves
		// disabled, don't worry about us, we can get out the way in time (we hope!)
		//if (Distance( org, g_entities[selfNum].r.currentOrigin ) < ent->splashRadius*1.5)
		//	return -1;
		// make sure we don't injure a friendly
		for ( trav = g_entities; trav < g_entities + g_maxclients.integer; trav++ ) {
			if ( !trav->inuse ) {
				continue;
			}
			if ( !trav->client ) {
				continue;
			}
			if ( trav->health <= 0 ) {
				continue;
			}
			if ( trav->s.number == selfNum ) {
				continue;
			}
			if ( AICast_SameTeam( AICast_GetCastState( selfNum ), trav->s.number ) ) {
				if ( Distance( org, trav->r.currentOrigin ) < ent->splashRadius ) {
					return -1;
				}
			}
		}
	}
	// if it overshot the mark
	if ( !rval && Distance( g_entities[ent->r.ownerNum].r.currentOrigin, org ) > Distance( g_entities[ent->r.ownerNum].r.currentOrigin, enemyPos ) ) {
		return -2;  // so the AI can try aiming down a bit next time
	}
	//
	return rval;
}

/*
=============
AICast_CheckDangerousEntity

  check to see if the given entity can harm an AI character, if so, informs them
  appropriately
=============
*/
void AICast_CheckDangerousEntity( gentity_t *ent, int dangerFlags, float dangerDist, float tacticalLevel, float aggressionLevel, qboolean hurtFriendly ) {
	vec3_t org, fwd, vec;
	cast_state_t *cs, *dcs;
	gentity_t *trav;
	int i, endTime;
	float dist;
	//
	//
	if ( dangerFlags & DANGER_MISSILE ) {
		// predict where the entity will explode
		if ( !( endTime = G_PredictMissile( ent, ent->nextthink - level.time, org, qtrue ) ) ) {
			return; // missile won't explode, so no danger
		}
	} else {
		// just avoid it for a bit, then forget it
		endTime = level.time + 1000;
		VectorCopy( ent->r.currentOrigin, org );
	}
	if ( dangerFlags & DANGER_CLIENTAIM ) {
		AngleVectors( ent->client->ps.viewangles, fwd, NULL, NULL );
	}
	//
	if ( ent->client ) {
		dcs = AICast_GetCastState( ent->s.number );
	} else {
		dcs = NULL;
	}
	//
	// see if this will hurt anyone
	for ( trav = g_entities, cs = AICast_GetCastState( 0 ), i = 0; i < level.numPlayingClients; cs++, trav++ ) {
		if ( !trav->inuse || !trav->client ) {
			continue;
		}
		i++;    // found a connected client
		if ( trav == ent ) {  // don't be afraid of ourself
			continue;
		}
		if ( trav->health <= 0 ) {
			continue;
		}
		if ( !cs->bs ) {  // not an AI, they should look out for themselves
			continue;
		}
		if ( cs->castScriptStatus.scriptNoSightTime >= level.time ) {
			continue;       // absolutely no sight (or hear) information allowed
		}
		if ( !hurtFriendly && ent->s.number < MAX_CLIENTS && AICast_SameTeam( cs, ent->s.number ) ) {
			continue;   // trust that friends will not hurt us
		}
		if ( ( dangerFlags & DANGER_FLAMES ) && ( cs->aiFlags & AIFL_NO_FLAME_DAMAGE ) ) {
			continue;   // venom not effected by flames
		}
		if ( cs->attributes[TACTICAL] < tacticalLevel ) { // not smart enough
			continue;
		}
		if ( cs->aiState >= AISTATE_COMBAT && cs->attributes[AGGRESSION] > aggressionLevel ) { // we are too aggressive to worry about being hurt by this
			continue;
		}
		// if they are below alert mode, and the danger is not in FOV, then ignore it
		if ( cs->aiState < AISTATE_ALERT ) {
			vec3_t ang, dir;
			VectorSubtract( ent->r.currentOrigin, cs->bs->origin, dir );
			VectorNormalize( dir );
			vectoangles( dir, ang );
			if ( !AICast_InFieldOfVision( cs->bs->viewangles, cs->attributes[FOV], ang ) ) {
				// can't see it
				continue;
			}
		}
		if (    ent->client &&
				( !cs->vislist[ent->s.number].visible_timestamp || ( cs->vislist[ent->s.number].visible_timestamp < level.time - 3000 ) ) ) {
			//	(!dcs->vislist[trav->s.number].visible_timestamp || (dcs->vislist[trav->s.number].visible_timestamp < level.time - 3000))))
			continue;   // not aware of them, and they're not aware of us
		}
		// are they in danger?
		if ( cs->dangerEntityValidTime < level.time + 50 ) {
			VectorSubtract( cs->bs->cur_ps.origin, org, vec );
			dist = VectorLength( vec );
			if ( dist < dangerDist ) {
				if ( dangerFlags & DANGER_CLIENTAIM ) {
					// also check aiming
					if ( DotProduct( vec, fwd ) < ( dist * 0.95 - 100 ) ) {
						continue;
					}
				}
				//
				cs->aiFlags &= ~AIFL_DENYACTION;
				AICast_ScriptEvent( cs, "avoiddanger", ent->classname );
				if ( cs->aiFlags & AIFL_DENYACTION ) {
					continue;
				}
				cs->dangerEntity = ent->s.number;
				VectorCopy( org, cs->dangerEntityPos );
				cs->dangerEntityValidTime = endTime + 50;
				cs->dangerDist = dangerDist * 1.5;    // when we hide from it, get a good distance away
				cs->dangerEntityTimestamp = level.time;
			}
		}
	}
}


qboolean AICast_HasFiredWeapon( int entNum, int weapon ) {
	if ( AICast_GetCastState( entNum )->weaponFireTimes[weapon] ) {
		return qtrue;
	}

	return qfalse;
}

qboolean AICast_AllowFlameDamage( int entNum ) {
	// DHM - Nerve :: caststates are not initialized in multiplayer
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		return qtrue;
	}
	// dhm

	if ( caststates[entNum].aiFlags & AIFL_NO_FLAME_DAMAGE ) {
		return qfalse;
	}
	return qtrue;
}

/*
=============
AICast_ProcessBullet
=============
*/
void AICast_ProcessBullet( gentity_t *attacker, vec3_t start, vec3_t end ) {
	gentity_t   *tent;
	int i;
	float dist;
	vec3_t vProj, vDir, dir;
	cast_state_t *cs;

	VectorSubtract( end, start, dir );

	// RF, AI should hear this pass by them really closely, or hitting a wall close by
	for ( cs = caststates, tent = g_entities, i = 0; i < level.maxclients; i++, tent++, cs++ ) {
		if ( !tent->inuse ) {
			continue;
		}
		if ( tent == attacker ) {
			continue;
		}
		if ( tent->aiInactive ) {
			continue;
		}
		if ( tent->health <= 0 ) {
			continue;
		}
		if ( cs->castScriptStatus.scriptNoSightTime > level.time ) {
			continue;
		}
		if ( !( tent->r.svFlags & SVF_CASTAI ) ) {
			continue;
		}
		if ( cs->aiState >= AISTATE_COMBAT ) { // RF add	// already fighting, not interested in bullet impacts
			continue;
		}
		if ( cs->bulletImpactIgnoreTime > level.time ) {
			continue;
		}
		dist = Distance( tent->client->ps.origin, end );
		if ( dist <= cs->attributes[INNER_DETECTION_RADIUS] ) {
			// close enough to hear/see the impact?
			// first check pvs
			if ( !trap_InPVS( tent->client->ps.origin, end ) ) {
				continue;
			}
			// heard it
			goto heard;
		}
		// are they within radius of the bullet path to hear it travel through the air?
		ProjectPointOntoVector( tent->client->ps.origin, start, end, vProj );
		VectorSubtract( vProj, start, vDir );
		if ( DotProduct( vDir, dir ) < 0 ) {  // they are behind the path of the bullet
			continue;
		}
		if ( Distance( vProj, tent->client->ps.origin ) > 0.5 * cs->attributes[INNER_DETECTION_RADIUS] ) {
			continue;
		}
heard:
		// call the script event
		AICast_ScriptEvent( cs, "bulletimpact", "" );
		if ( cs->aiFlags & AIFL_DENYACTION ) {
			continue;   // ignore the bullet
		}
		//
		cs->bulletImpactTime = level.time + 200 + rand() % 300;   // random reaction delay;
		VectorCopy( start, cs->bulletImpactStart );
		VectorCopy( end, cs->bulletImpactEnd );
	}
}

/*
================
AICast_AudibleEvent
================
*/
void AICast_AudibleEvent( int srcnum, vec3_t pos, float range ) {
	int i;
	cast_state_t *cs, *scs;
	gentity_t *ent, *sent;

	// DHM - Nerve :: caststates are not initialized in multiplayer
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		return;
	}
	// dhm

	sent = &g_entities[srcnum];
	scs = AICast_GetCastState( srcnum );

	for ( ent = g_entities, cs = caststates, i = 0; i < level.maxclients; i++, cs++, ent++ ) {
		if ( !cs->bs ) {
			continue;
		}
		//if (cs->aiState >= AISTATE_COMBAT)
		//	continue;
		if ( ent == sent ) {
			continue;
		}
		if ( cs->castScriptStatus.scriptNoSightTime > level.time ) {
			continue;
		}
		if ( ent->health <= 0 ) {
			continue;
		}
		// if within range, and this sound was made by an enemy
		if ( scs->aiState < AISTATE_COMBAT && !AICast_QueryEnemy( cs, srcnum ) ) {
			continue;
		}
		if ( Distance( pos, ent->s.pos.trBase ) > range ) {
			continue;
		}
		// we heard it
		cs->audibleEventTime = level.time + 200 + rand() % 300;   // random reaction delay
		VectorCopy( pos, cs->audibleEventOrg );
		cs->audibleEventEnt = ent->s.number;
	}
}
