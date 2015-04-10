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
// Name:			ai_cast_funcs.c
// Function:		Wolfenstein AI Character Decision Making
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

//=================================================================================
//
// Helga (in zombie form), the first boss
//
//=================================================================================

void AICast_FZombie_StartLightning( gentity_t *ent ) {
	ent->AIScript_AlertEntity = NULL;
	AIFunc_FZombie_LightningAttackStart( AICast_GetCastState( ent->s.number ) );
}

/*
===============
AIFunc_FZombie_Idle
===============
*/
char *AIFunc_FZombie_Idle( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	//
	if ( cs->thinkFuncChangeTime < level.time - PORTAL_FEMZOMBIE_SPAWNTIME ) {
		// HACK, make them aware of the player
		cs->castScriptStatus.scriptNoSightTime = 0;
		AICast_UpdateVisibility( &g_entities[cs->entityNum], AICast_FindEntityForName( "player" ), qfalse, qtrue );
		ent->s.time2 = 0;   // turn spawning effect off
		// allow us to be informed to start the portal lightning
		ent->AIScript_AlertEntity = AICast_FZombie_StartLightning;
		return AIFunc_DefaultStart( cs );
	}
	//
	return NULL;
}

/*
===============
AIFunc_FZombie_IdleStart
===============
*/
char *AIFunc_FZombie_IdleStart( cast_state_t *cs ) {
	cs->aifunc = AIFunc_FZombie_Idle;
	return "AIFunc_FZombie_Idle";
}

/*
===============
AICast_FZombie_EndLightning
===============
*/
void AICast_FZombie_EndLightning( gentity_t *ent ) {
	ent->s.effect2Time = level.time;
	// allow us to be informed to start the portal lightning
	ent->AIScript_AlertEntity = AICast_FZombie_StartLightning;
}

/*
===============
AIFunc_FZombie_LightningAttack

  The big portal lightning effect. While this is going on, the FemZombie will climb
  across the walls like an insect.
===============
*/
char *AIFunc_FZombie_LightningAttack( cast_state_t *cs ) {
	bot_state_t *bs;
	gentity_t *ent, *marker, *trav;
	// TTimo gcc: 'best' might be used uninitialized in this function
	gentity_t *best = NULL;
	qboolean move;
	float bestdist, dist;
	vec3_t axis[3];
	cast_state_t    *ecs;
	//
	ent = &g_entities[cs->entityNum];
	bs = cs->bs;
	trav = AICast_FindEntityForName( "player" );
	if ( !trav ) {
		return NULL;        // huh?
	}
	cs->bs->enemy = trav->s.number;
	ecs = AICast_GetCastState( cs->bs->enemy );
	//
	// we should show a big lightning effect and then die once we've given the player enough time to get to us
	//
	ent->s.effect1Time = cs->thinkFuncChangeTime;
	ent->client->ps.eFlags |= EF_MONSTER_EFFECT;
	//
	// TODO: might be cool to have the head move around a bit faster (like an insect?)
	g_entities[cs->entityNum].client->ps.eFlags |= EF_HEADLOOK;
	//
	// turn on flight only if we need it this frame
	ent->client->ps.powerups[PW_FLIGHT] = 0;
	//
	// has this effect finished?
	if ( ent->s.effect2Time ) {

		// we are in climb-down mode as lightning subsides
		if ( ent->client->ps.groundEntityNum == ENTITYNUM_WORLD ) {

			return AIFunc_DefaultStart( cs );

		} else {    // climb down

			if ( cs->followEntity == -1 ) {   // find the closest marker and go there

				// assume we are on the upper wall, and have just reached a marker
				trav = NULL;
				// TTimo assignment used as truth value
				while ( ( trav = G_Find( trav, FOFS( classname ), "ai_marker" ) ) ) {
					if ( trav->count == ent->count && trav->targetname ) {
						cs->followEntity = trav->s.number;
						return AIFunc_FZombie_LightningAttack( cs );  // think again
					}
				}
				if ( !trav ) {
					G_Error( "AIFunc_FZombie_LightningAttack: unable to find matching wall marker for count = %i", ent->count );
				}

			} else if ( cs->followEntity == -2 ) {    // at the base of wall, rotate around then dismount

				ent->client->ps.eFlags |= EF_FORCED_ANGLES; // face angles exactly
				cs->bs->ideal_viewangles[ROLL] = 0;

				if ( fabs( cs->bs->viewangles[ROLL] ) < 5 ) { // we are done, dismount and get outta here
					ent->client->ps.powerups[PW_FLIGHT] = 0;
					return AIFunc_DefaultStart( cs );
				}

				ent->client->ps.powerups[PW_FLIGHT] = 1;    // stay here
				return NULL;
			}

		}

	}

	if ( cs->followEntity >= 0 ) {    // move towards our current goal

		marker = &g_entities[cs->followEntity];

		ent->count = marker->count;

		if ( !ent->s.effect2Time && marker->targetname ) {        // ground marker

			dist = VectorDistance( cs->bs->origin, marker->s.origin );
			if ( dist < 4 ) {
				// we made it there, find the corresponding wall marker
				trav = NULL;
				// TTimo gcc: suggest parentheses around assignment used as truth value
				while ( ( trav = G_Find( trav, FOFS( classname ), "ai_marker" ) ) ) {
					if ( trav->count == marker->count && !trav->targetname ) {
						cs->followEntity = trav->s.number;
						return AIFunc_FZombie_LightningAttack( cs );  // think again
					}
				}
				if ( !trav ) {
					G_Error( "AIFunc_FZombie_LightningAttack: unable to find matching wall marker for count = %i", marker->count );
				}
			}

			cs->followSlowApproach = qtrue;
			AICast_MoveToPos( cs, marker->s.origin, cs->followEntity );

			// should we slow down?
			cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, 8 );

			// check for a movement we should be making
			if ( cs->obstructingTime > level.time ) {
				AICast_MoveToPos( cs, cs->obstructingPos, -1 );
				cs->movestate = MS_WALK;
				cs->movestateType = MSTYPE_TEMPORARY;
			}

		} else {                        // in-air

			if ( !ent->s.effect2Time && ( ecs->aiFlags & AIFL_ROLL_ANIM ) ) {
				// hurt player if they are visible from portal
				ent->client->ps.eFlags |= EF_MONSTER_EFFECT2;
			}

			dist = VectorDistance( cs->bs->origin, marker->s.origin );
			if ( dist < 20 ) {
				// we made it there, stop
				if ( ent->s.effect2Time && marker->targetname ) {
					cs->followEntity = -2;
				} else {
					cs->followEntity = -1;
				}
				return NULL;
			}

			// climb the walls

			ent->client->ps.powerups[PW_FLIGHT] = 1;    // let them fly
			ent->client->ps.eFlags |= EF_FORCED_ANGLES; // face angles exactly

			if ( ( ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT ) != BOTH_CLIMB ) {
				ent->client->ps.torsoAnim =
					( ( ent->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_CLIMB;
				ent->client->ps.legsAnim =
					( ( ent->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_CLIMB;
			}
			ent->client->ps.torsoTimer = 500;
			ent->client->ps.legsTimer = 500;

			// should we slow down?
			cs->speedScale = AICast_SpeedScaleForDistance( cs, dist, 32 );

			// use angles of the marker, but ROLL so we point upwards towards the marker
			// fwd is the marker angles, up is the vec, right is the cross product
			AngleVectors( marker->s.angles, axis[0], NULL, NULL );
			VectorSubtract( marker->s.origin, cs->bs->origin, axis[2] );
			VectorNormalize( axis[2] );
			//if (ent->s.effect2Time && marker->targetname) {	// walk downwards
			//VectorInverse( axis[2] );
			//	VectorSet( axis[2], 0, 0, 1 );
			//}
			CrossProduct( axis[0], axis[2], axis[1] );
			VectorInverse( axis[1] );
			AxisToAngles( axis, cs->bs->ideal_viewangles );

			// movement
			//if (ent->s.effect2Time && marker->targetname) {	// walk downwards
			//	trap_EA_MoveDown(cs->entityNum);
			//} else {
			trap_EA_Jump( cs->entityNum );
			//}
			trap_EA_Move( cs->entityNum, axis[0], 20 );  // move towards the wall so we stay attached to it
		}

	} else {    // we are motionless on the wall.. keep checking to see if we should head somewhere else

		if ( !ent->s.effect2Time && ( ecs->aiFlags & AIFL_ROLL_ANIM ) ) {
			// hurt player if they are visible from portal
			ent->client->ps.eFlags |= EF_MONSTER_EFFECT2;
		}

		ent->client->ps.powerups[PW_FLIGHT] = 1;    // let them fly
		ent->client->ps.eFlags |= EF_FORCED_ANGLES; // face angles exactly

		move = qfalse;

		if ( cs->lastPain >= cs->lastThink ) {    // we've been injured, move
			cs->lastPain = 0;
			move = qtrue;
		} else if ( AICast_VisibleFromPos( g_entities[cs->bs->enemy].client->ps.origin, cs->bs->enemy, cs->bs->origin, cs->entityNum, qfalse ) ) {
			move = qtrue;
		}

		if ( move ) { // we need to start moving again
			trav = NULL;
			bestdist = -1;
			while ( ( trav = G_Find( trav, FOFS( classname ), "ai_marker" ) ) ) {
				if ( trav->targetname ) { // floor marker
					continue;
				}
				if ( VectorDistance( cs->bs->origin, trav->s.origin ) < 48 ) {
					continue;
				}
				if ( ( trav->count < 10 ) == ( ent->count < 10 ) ) {    // this marker is on our wall
					// if this marker no visible from the enemy
					if ( !AICast_VisibleFromPos( g_entities[cs->bs->enemy].client->ps.origin, cs->bs->enemy, trav->s.origin, cs->entityNum, qfalse ) ) {
						cs->followEntity = trav->s.number;
						break;
					}
					dist = VectorDistance( trav->s.origin, g_entities[cs->bs->enemy].client->ps.origin );
					if ( bestdist < 0 || bestdist < dist ) {
						best = trav;
						bestdist = dist;
					}
				}
			}
			if ( !trav ) {
				if ( !best ) {
					G_Error( "AIFunc_FZombie_LightningAttack: unable to find matching wall marker for count = %i", ent->count );
				}
				cs->followEntity = best->s.number;
			}
			//
			return AIFunc_FZombie_LightningAttack( cs );  // think again
		}

	}
	//
	return NULL;
}

/*
===============
AIFunc_FZombie_LightningAttackStart
===============
*/
char *AIFunc_FZombie_LightningAttackStart( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum], *marker, *best;
	float bestdist, dist;
	//
	ent->AIScript_AlertEntity = AICast_FZombie_EndLightning;    // scripting will tell us to stop
	ent->s.effect2Time = 0;
	//
	// find the closest ai_marker on the ground
	marker = NULL;
	best = NULL;
	bestdist = -1;
	// TTimo gcc: suggest parentheses around assignment used as truth value
	while ( ( marker = G_Find( marker, FOFS( classname ), "ai_marker" ) ) ) {
		if ( !marker->targetname || ( Q_stricmp( marker->targetname, "zfloor" ) != 0 ) ) {
			continue;
		}
		dist = VectorDistance( marker->s.origin, cs->bs->origin );
		if ( bestdist >= 0 && ( bestdist < dist ) ) {
			continue;
		}
		// closer
		best = marker;
		bestdist = dist;
	}
	//
	if ( !best ) {
		G_Error( "AIFunc_FZombie_LightningAttackStart: unable to find a close ai_marker with targetname = \"zfloor\"" );
	}
	cs->followEntity = best->s.number;
	//
	cs->aifunc = AIFunc_FZombie_LightningAttack;
	return "AIFunc_FZombie_LightningAttack";
}

/*
===============
AIFunc_FZombie_HandLightningAttack

  Shoots lightning out at the player from her hands
===============
*/
#define FEMZOMBIE_HANDATTACK_DURATION   3400

char *AIFunc_FZombie_HandLightningAttack( cast_state_t *cs ) {
	bot_state_t *bs;
	gentity_t *ent;
	//
	ent = &g_entities[cs->entityNum];
	bs = cs->bs;
	cs->weaponFireTimes[WP_MONSTER_ATTACK2] = level.time;
	//
	if ( cs->aiFlags & AIFL_LAND_ANIM_PLAYED ) {  // stop the effects
		if ( !ent->client->ps.torsoTimer ) {
			// are we ready to do the big portal lightning effect?
			if ( AICast_GotEnoughAmmoForWeapon( cs, WP_MONSTER_ATTACK1 ) ) {
				return AIFunc_FZombie_LightningAttackStart( cs );
			} else {
				return AIFunc_BattleChaseStart( cs );
			}
		}
		return NULL;
	}
	//
	// face them and do the effect
	AICast_AimAtEnemy( cs );
	if ( ent->client->ps.torsoTimer < FEMZOMBIE_HANDATTACK_DURATION - 1000 ) {
		ent->client->ps.eFlags |= EF_MONSTER_EFFECT3;
		ent->s.otherEntityNum = bs->enemy;
		//
		if ( ent->client->ps.torsoTimer < 400 || cs->bs->cur_ps.ammo[BG_FindAmmoForWeapon( WP_MONSTER_ATTACK1 )] || !AICast_EntityVisible( cs, bs->enemy, qtrue ) || !AICast_CheckAttack( cs, bs->enemy, qfalse ) ) {
			// finish this attack
			ent->client->ps.torsoAnim =
				( ( ent->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_ATTACK5;
			ent->client->ps.torsoTimer = 300;
			cs->aiFlags |= AIFL_LAND_ANIM_PLAYED;
		}
	}
	//
	return NULL;
}

/*
===============
AIFunc_FZombie_HandLightningAttackStart
===============
*/
char *AIFunc_FZombie_HandLightningAttackStart( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	//
	ent->client->ps.torsoAnim =
		( ( ent->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_ATTACK4;
	ent->client->ps.torsoTimer = FEMZOMBIE_HANDATTACK_DURATION;
	//
	cs->aiFlags &= ~AIFL_LAND_ANIM_PLAYED;
	ent->s.effect3Time = level.time;
	cs->aifunc = AIFunc_FZombie_HandLightningAttack;
	return "AIFunc_FZombie_handLightningAttack";
}

//=================================================================================
//
// Helga (in normal form), the first boss
//
//=================================================================================

/*
===============
AICast_Helga_Alert

  Special code hooks for helga scripting
===============
*/
void AICast_Helga_Alert( gentity_t *ent ) {
	cast_state_t *cs = AICast_GetCastState( ent->s.number );

	if ( !ent->s.effect2Time ) {
		ent->s.eFlags |= EF_MONSTER_EFFECT2;
		ent->s.effect2Time = level.time;
	} else if ( !( cs->aiFlags & AIFL_LAND_ANIM_PLAYED ) ) {
		cs->aiFlags |= AIFL_LAND_ANIM_PLAYED;   // stop the effects
	} else {
		// we are no longer in the game
		ent->aiInactive = qtrue;
		trap_UnlinkEntity( ent );
	}
}

/*
===============
AIFunc_Helga_Idle
===============
*/
char *AIFunc_Helga_Idle( cast_state_t *cs ) {
	bot_state_t *bs;
	gentity_t *ent;
	//
	ent = &g_entities[cs->entityNum];
	bs = cs->bs;
	//
	if ( cs->aiFlags & AIFL_LAND_ANIM_PLAYED ) {
		return NULL;
	}
	// we should show a big lightning effect and then die once we've given the player enough time to get to us
	//
	ent->s.effect1Time = cs->thinkFuncChangeTime;
	ent->client->ps.eFlags |= EF_MONSTER_EFFECT;
	//
	// are we in lightning death mode?
	if ( ent->s.effect2Time && !( cs->aiFlags & AIFL_LAND_ANIM_PLAYED ) ) {
		ent->client->ps.eFlags |= EF_MONSTER_EFFECT2;
	}
	//
	return NULL;
}

/*
===============
AIFunc_Helga_IdleStart
===============
*/
char *AIFunc_Helga_IdleStart( cast_state_t *cs ) {
	gentity_t *ent;
	//
	ent = &g_entities[cs->entityNum];
	// special alertEntity function so scripting can initialize the death routine
	ent->AIScript_AlertEntity = AICast_Helga_Alert;
	ent->s.effect2Time = 0;
	//
	cs->aiFlags &= ~AIFL_LAND_ANIM_PLAYED;
	//
	cs->aifunc = AIFunc_Helga_Idle;
	return "AIFunc_Helga_Idle";
}

//===================================================================

/*
==============
AIFunc_FlameZombie_Portal
==============
*/
char *AIFunc_FlameZombie_Portal( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	//
	if ( cs->thinkFuncChangeTime < level.time - PORTAL_ZOMBIE_SPAWNTIME ) {
		// HACK, make them aware of the player
		AICast_UpdateVisibility( &g_entities[cs->entityNum], AICast_FindEntityForName( "player" ), qfalse, qtrue );
		ent->s.time2 = 0;   // turn spawning effect off
		return AIFunc_DefaultStart( cs );
	}
	//
	return NULL;
}

/*
==============
AIFunc_FlameZombie_PortalStart
==============
*/
char *AIFunc_FlameZombie_PortalStart( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	//
	ent->s.time2 = level.time + 200;    // hijacking this for portal spawning effect
	//
	// play a special animation
	ent->client->ps.torsoAnim =
		( ( ent->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_EXTRA1;
	ent->client->ps.legsAnim =
		( ( ent->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_EXTRA1;
	ent->client->ps.torsoTimer = PORTAL_ZOMBIE_SPAWNTIME - 200;
	ent->client->ps.legsTimer = PORTAL_ZOMBIE_SPAWNTIME - 200;
	//
	cs->thinkFuncChangeTime = level.time;
	//
	cs->aifunc = AIFunc_FlameZombie_Portal;
	return "AIFunc_FlameZombie_Portal";
}
