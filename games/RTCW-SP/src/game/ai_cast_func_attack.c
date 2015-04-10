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
// ZOMBIE SPECIAL ATTACKS
//
//=================================================================================

/*
============
AIFunc_ZombieFlameAttack()

  Zombie "Flaming Bats" attack.

  NOTE: this actually uses the EFFECT3 slot for client-side effects (others are taken)
============
*/

#define ZOMBIE_FLAME_DURATION       4000

char *AIFunc_ZombieFlameAttack( cast_state_t *cs ) {
	bot_state_t *bs;
	gentity_t *ent;
	//
	ent = &g_entities[cs->entityNum];
	bs = cs->bs;
	//
	ent->s.onFireEnd = level.time + 2000;
	//
	if ( ent->health < 0 ) {
		ent->s.onFireEnd = 0;
		return AIFunc_DefaultStart( cs );
	}
	//
	if ( cs->enemyNum < 0 ) {
		ent->s.onFireEnd = level.time + 1500;
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;
		return AIFunc_DefaultStart( cs );
	}
/*	disabled, keep going so they cant come back for the easy kill
	//
	// if we can't see them anymore, abort immediately
	if (cs->vislist[cs->enemyNum].real_visible_timestamp != cs->vislist[cs->enemyNum].real_update_timestamp) {
		ent->s.onFireEnd = level.time + 1500;
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;
		return AIFunc_DefaultStart( cs );
	}
*/
	// if outside range, move closer
	if ( VectorDistance( cs->bs->origin, cs->vislist[cs->enemyNum].visible_pos ) > ZOMBIE_FLAME_RADIUS ) {
		ent->s.onFireEnd = level.time + 1500;
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;
		return AIFunc_DefaultStart( cs );
	}
	// we are firing this weapon, so record it
	cs->weaponFireTimes[WP_MONSTER_ATTACK1] = level.time;
	// once an attack has started, only abort once the player leaves our view, or time runs out
	if ( cs->thinkFuncChangeTime < level.time - ZOMBIE_FLAME_DURATION ) {

		// finish this attack
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;
		return AIFunc_DefaultStart( cs );

	} else {

		//ent->client->ps.torsoTimer = 400;
		//ent->client->ps.legsTimer = 400;

		// draw the client-side effect
		ent->client->ps.eFlags |= EF_MONSTER_EFFECT3;

		// inform the client of our enemies position
		//VectorCopy( g_entities[cs->enemyNum].client->ps.origin, ent->s.origin2 );
		//ent->s.origin2[2] += g_entities[cs->enemyNum].client->ps.viewheight;

		// keep facing them
		AICast_AimAtEnemy( cs );

		// look slightly downwards since animation is facing upwards slightly
		cs->ideal_viewangles[PITCH] += 20;
	}
	//
	//
	return NULL;
}

char *AIFunc_ZombieFlameAttackStart( cast_state_t *cs ) {
	gentity_t *ent;
	//
	ent = &g_entities[cs->entityNum];
	ent->s.otherEntityNum2 = cs->enemyNum;
	ent->s.effect3Time = level.time;
	//
	// dont turn
	cs->ideal_viewangles[YAW] = cs->viewangles[YAW];
	//cs->ideal_viewangles[PITCH] = -45;	// look upwards
	// start the flame
	ent->s.onFireStart = level.time;
	ent->s.onFireEnd = level.time + ZOMBIE_FLAME_DURATION;
	//
	// set the correct animation
	BG_PlayAnimName( &ent->client->ps, "both_attack1", ANIM_BP_BOTH, qtrue, qfalse, qtrue );
	ent->client->ps.legsTimer = ZOMBIE_FLAME_DURATION;
	ent->client->ps.torsoTimer = ZOMBIE_FLAME_DURATION;
	//
	cs->aifunc = AIFunc_ZombieFlameAttack;
	return "AIFunc_ZombieFlameAttack";
}



/*
============
AIFunc_ZombieAttack2()

  Zombie "Evil Spirit" attack.

  Character draws the light from surrounding walls (expanding negative light) and builds
  up to the release of a flying translucent skull with trail effect (and beady eyes).

  Spirit should track it's enemy slightly, inflicting lots of damage, removing sprint bar,
  and effecting sight temporarily.

  Speed of spirit is effected by skill level, higher skill = faster speed

  Spirits inflicting AI soldiers should kill instantly, removing all flesh from the
  soldier's face (draw skull under head model, then fade head model away over a short period).
============
*/
extern void weapon_zombiespirit( gentity_t *ent, gentity_t *missile );

#define ZOMBIE_SPIRIT_BUILDUP_TIME      6000    // last for this long
#define ZOMBIE_SPIRIT_FADEOUT_TIME      1000
#define ZOMBIE_SPIRIT_DLIGHT_RADIUS_MAX 256
#define ZOMBIE_SPIRIT_FIRE_INTERVAL     1000

int lastZombieSpiritAttack;

char *AIFunc_ZombieAttack2( cast_state_t *cs ) {
	bot_state_t *bs;
	gentity_t *ent;
	//
	ent = &g_entities[cs->entityNum];
	bs = cs->bs;
	//
	if ( cs->enemyNum < 0 ) {
		return AIFunc_DefaultStart( cs );
	}
	//
	// if we can't see them anymore, abort immediately
	if ( cs->vislist[cs->enemyNum].real_visible_timestamp != cs->vislist[cs->enemyNum].real_update_timestamp ) {
		return AIFunc_DefaultStart( cs );
	}
	//
	lastZombieSpiritAttack = level.time;
	// we are firing this weapon, so record it
	cs->weaponFireTimes[WP_MONSTER_ATTACK2] = level.time;
	// once an attack has started, only abort once the player leaves our view, or time runs out
	if ( cs->thinkFuncChangeTime < level.time - ZOMBIE_SPIRIT_BUILDUP_TIME ) {
		// if enough time has elapsed, finish this attack
		if ( level.time > cs->thinkFuncChangeTime + ZOMBIE_SPIRIT_BUILDUP_TIME + ZOMBIE_SPIRIT_FADEOUT_TIME ) {
			return AIFunc_DefaultStart( cs );
		}
	} else {

		// set torso to the correct animation
		// TODO
		//ent->client->ps.torsoTimer = 300;	// leave enough time to cancel if we stop coming in here, but stay in the anim if we come back next thing

		// draw the client-side effect
		ent->client->ps.eFlags |= EF_MONSTER_EFFECT;

		// inform the client of our enemies position
		VectorCopy( g_entities[cs->enemyNum].client->ps.origin, ent->s.origin2 );
		ent->s.origin2[2] += g_entities[cs->enemyNum].client->ps.viewheight;
	}
	//
	//
	return NULL;
}

char *AIFunc_ZombieAttack2Start( cast_state_t *cs ) {
	gentity_t *ent;
	//
	// don't allow 2 consecutive spirit attacks at once
	if ( lastZombieSpiritAttack <= level.time && lastZombieSpiritAttack > level.time - 1000 ) {
		return NULL;
	}
	lastZombieSpiritAttack = level.time;
	//
	ent = &g_entities[cs->entityNum];
	ent->s.otherEntityNum2 = cs->enemyNum;
	ent->s.effect1Time = level.time;
	//
	// dont turn
	cs->ideal_viewangles[YAW] = cs->viewangles[YAW];
	// set torso to the correct animation
	// TODO
	//ent->client->ps.torsoAnim =
	//	( ( ent->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | BOTH_SALUTE;	// FIXME: need a specific anim for this
	//
	cs->aifunc = AIFunc_ZombieAttack2;
	return "AIFunc_ZombieAttack2";
}


/*
================
AIFunc_ZombieMelee
================
*/

int zombieHitDamage[5] = {
	16,
	16,
	16,
	12,
	20
};

#define NUM_ZOMBIE_ANIMS    5
int zombieHitTimes[NUM_ZOMBIE_ANIMS][3] = { // up to three hits per attack
	{ANIMLENGTH( 11,20 ),-1},
	{ANIMLENGTH( 9,20 ),-1},
	{ANIMLENGTH( 9,20 ),-1},
	{ANIMLENGTH( 8,20 ),ANIMLENGTH( 16,20 ),-1},
	{ANIMLENGTH( 8,20 ),ANIMLENGTH( 15,20 ),ANIMLENGTH( 24,20 )},
};

char *AIFunc_ZombieMelee( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	int hitDelay = -1, anim;
	trace_t *tr;
	cast_state_t *ecs = AICast_GetCastState( cs->enemyNum );
	aicast_predictmove_t move;
	float enemyDist;

	if ( !ent->client->ps.torsoTimer ) {
		return AIFunc_DefaultStart( cs );
	}

	if ( ecs ) {

		anim = ( ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT ) - BG_AnimationIndexForString( "attack1", cs->entityNum );
		if ( anim < 0 || anim >= NUM_ZOMBIE_ANIMS ) {
			// animation interupted
			return AIFunc_DefaultStart( cs );
		}
		if ( zombieHitTimes[anim][cs->animHitCount] >= 0 && cs->animHitCount < 3 ) {

			if ( !cs->animHitCount ) {
				hitDelay = zombieHitTimes[anim][cs->animHitCount];
			} else {
				hitDelay = zombieHitTimes[anim][cs->animHitCount] - zombieHitTimes[anim][cs->animHitCount - 1];
			}

			// check for inflicting damage
			if ( level.time - cs->weaponFireTimes[cs->weaponNum] > hitDelay ) {
				// do melee damage
				if ( ( tr = CheckMeleeAttack( ent, AICast_WeaponRange( cs, cs->weaponNum ) + 4.0, qfalse ) ) && ( tr->entityNum == cs->enemyNum ) ) {
					G_Damage( &g_entities[tr->entityNum], ent, ent, vec3_origin, tr->endpos,
							  zombieHitDamage[anim], 0, MOD_GAUNTLET );
					G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[STAYSOUNDSCRIPT] ) );
				} else {
					G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[FOLLOWSOUNDSCRIPT] ) );
				}
				cs->weaponFireTimes[cs->weaponNum] = level.time;
				cs->animHitCount++;
			}
		}
		// face them
		AICast_AimAtEnemy( cs );
		if ( !ent->client->ps.legsTimer || zombieHitTimes[anim][cs->animHitCount] < 0 || cs->animHitCount >= 3 ) {
			// if they are outside range, move forward
			AICast_PredictMovement( ecs, 2, 0.5, &move, &g_entities[cs->enemyNum].client->pers.cmd, -1 );
			enemyDist = Distance( move.endpos, cs->bs->origin );
			enemyDist -= g_entities[cs->enemyNum].r.maxs[0];
			enemyDist -= ent->r.maxs[0];
			if ( /*anim != 4 &&*/ ( enemyDist > 16 ) ) {    // we can get closer
				if ( ent->client->ps.legsTimer ) {
					ent->client->ps.legsTimer = 0;      // allow legs us to move
					if ( cs->castScriptStatus.scriptNoMoveTime < level.time + 200 ) { // dont move until the legs are done lerping out of attack anim
						cs->castScriptStatus.scriptNoMoveTime = level.time + 200;
					}
				}
				if ( !ent->client->ps.legsTimer && cs->castScriptStatus.scriptNoMoveTime < level.time ) {
					trap_EA_MoveForward( cs->entityNum );
				}
			}
		}

	}

	return NULL;
}

/*
================
AIFunc_ZombieMeleeStart
================
*/
char *AIFunc_ZombieMeleeStart( cast_state_t *cs ) {
	gentity_t *ent;

	ent = &g_entities[cs->entityNum];
	cs->weaponFireTimes[cs->weaponNum] = level.time;
	cs->animHitCount = 0;

	// face them
	AICast_AimAtEnemy( cs );

	// audible sound
	AIChar_AttackSound( cs );

	// play an anim
	BG_UpdateConditionValue( cs->entityNum, ANIM_COND_WEAPON, cs->weaponNum, qtrue );
	BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_FIREWEAPON, qfalse, qtrue );

	cs->aifunc = AIFunc_ZombieMelee;
	return "AIFunc_ZombieMelee";
}

//=================================================================================
//
// LOPER MELEE ATTACK
//
//=================================================================================

#define NUM_LOPERMELEE_ANIMS    2
int loperHitTimes[NUM_LOPERMELEE_ANIMS] = {
	( ANIMLENGTH( 3,15 ) ),
	( ANIMLENGTH( 6,15 ) )
};

#define LOPER_MELEE_DAMAGE          20
#define LOPER_MELEE_RANGE           48

/*
===============
AIFunc_LoperAttack1()

  Loper's close range melee attack
===============
*/
char *AIFunc_LoperAttack1( cast_state_t *cs ) {
	trace_t *tr;
	gentity_t *ent;
	int anim;
	//
	ent = &g_entities[cs->entityNum];
	//
	// draw the client-side lightning effect
	//ent->client->ps.eFlags |= EF_MONSTER_EFFECT;
	//
	// have we inflicted the damage?
	if ( cs->weaponFireTimes[WP_MONSTER_ATTACK1] > cs->thinkFuncChangeTime ) {
		// has the animation finished?
		if ( !ent->client->ps.legsTimer ) {
			return AIFunc_DefaultStart( cs );
		}
		return NULL;    // just wait for anim to finish
	}
	// ready to inflict damage?
	anim = ( ent->client->ps.legsAnim & ~ANIM_TOGGLEBIT ) - BG_AnimationIndexForString( "legs_extra", cs->entityNum );
	if ( cs->thinkFuncChangeTime < level.time - loperHitTimes[anim] ) {
		// check for damage
		// TTimo: gcc: suggests () around assignment used as truth value
		if ( ( tr = CheckMeleeAttack( &g_entities[cs->entityNum], LOPER_MELEE_RANGE, qfalse ) ) ) {
			G_Damage( &g_entities[tr->entityNum], ent, ent, vec3_origin, tr->endpos,
					  LOPER_MELEE_DAMAGE, 0, MOD_LOPER_HIT );
			// sound
			if ( anim == 0 ) {
				G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[ORDERSDENYSOUNDSCRIPT] ) );
			} else {
				G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[MISC1SOUNDSCRIPT] ) );
			}
		}
		cs->weaponFireTimes[WP_MONSTER_ATTACK1] = level.time;
	}

	return NULL;
}


char *AIFunc_LoperAttack1Start( cast_state_t *cs ) {
	gentity_t *ent;
	//
	ent = &g_entities[cs->entityNum];
	// face them
	AICast_AimAtEnemy( cs );
	// start the animation
	if ( rand() % 2 ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[FOLLOWSOUNDSCRIPT] ) );
		BG_PlayAnimName( &ent->client->ps, "legs_extra", ANIM_BP_LEGS, qtrue, qfalse, qtrue );
	} else {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[STAYSOUNDSCRIPT] ) );
		BG_PlayAnimName( &ent->client->ps, "legs_extra2", ANIM_BP_LEGS, qtrue, qfalse, qtrue );
	}
	//
	cs->aifunc = AIFunc_LoperAttack1;
	return "AIFunc_LoperAttack1";
}

//=================================================================================
//
// LOPER LEAP ATTACK
//
//=================================================================================

#define LOPER_LEAP_ANIM             LEGS_EXTRA3
#define LOPER_LEAP_FPS              15

// update for a version of the loper new today (6/26)
//#define	LOPER_LEAP_FRAME_COUNT		4
#define LOPER_LEAP_FRAME_COUNT      10
#define LOPER_LEAP_DURATION         ( LOPER_LEAP_FRAME_COUNT*( 1000 / LOPER_LEAP_FPS ) )


#define LOPER_LAND_ANIM             LEGS_EXTRA4
#define LOPER_LAND_FPS              15

// update for a version of the loper new today (6/26)
//#define	LOPER_LAND_FRAME_COUNT		17
#define LOPER_LAND_FRAME_COUNT      21
#define LOPER_LAND_DURATION         ( LOPER_LAND_FRAME_COUNT*( 1000 / LOPER_LAND_FPS ) )


#define LOPER_LEAP_DAMAGE           10
#define LOPER_LEAP_DELAY            100 // note, this needs to be something, or they'll think they're done before they even started
#define LOPER_LEAP_RANGE            200
#define LOPER_LEAP_VELOCITY_START   750.0
#define LOPER_LEAP_VELOCITY_END     1050.0
#define LOPER_LEAP_VELOCITY_Z       300
#define LOPER_LEAP_LAND_MOMENTUM    250

/*
===============
AIFunc_LoperAttack2()

  Loper's leaping long range attack
===============
*/
char *AIFunc_LoperAttack2( cast_state_t *cs ) {
	gentity_t *ent;
	vec3_t vec;
	qboolean onGround = qfalse;
	//
	ent = &g_entities[cs->entityNum];
	//
	// are we waiting to inflict damage?
	if ( ( cs->enemyNum >= 0 ) && ( cs->weaponFireTimes[WP_MONSTER_ATTACK2] < level.time - 50 ) &&
		 ( cs->bs->cur_ps.groundEntityNum == ENTITYNUM_NONE ) ) {
		// ready to inflict damage?
		if ( cs->thinkFuncChangeTime < level.time - LOPER_LEAP_DELAY ) {
			// check for damage
			if ( VectorDistance( cs->bs->origin, g_entities[cs->enemyNum].client->ps.origin ) < LOPER_LEAP_RANGE ) {
				// draw the client-side lightning effect
				ent->client->ps.eFlags |= EF_MONSTER_EFFECT;
				// do the damage
				G_Damage( &g_entities[cs->enemyNum], ent, ent, vec3_origin, cs->bs->origin, LOPER_LEAP_DAMAGE, 0, MOD_LOPER_LEAP );
				G_Sound( &g_entities[cs->entityNum], level.loperZapSound );
				cs->weaponFireTimes[WP_MONSTER_ATTACK2] = level.time;
			}
		}
	}
	//
	// landed?
	if ( cs->bs->cur_ps.groundEntityNum != ENTITYNUM_NONE ) {
		onGround = qtrue;
	} else {    // predict a landing
		aicast_predictmove_t move;
		float changeTime;
		AICast_PredictMovement( cs, 1, 0.2, &move, &cs->lastucmd, cs->enemyNum );
		if ( move.groundEntityNum != ENTITYNUM_NONE ) {
			onGround = qtrue;
		}
		//
		// adjust velocity
		VectorCopy( cs->loperLeapVel, vec );
		vec[2] = 0;
		VectorNormalize( vec );
		changeTime = 2.0 * ( 0.001 * ( level.time - cs->thinkFuncChangeTime ) );
		if ( changeTime > 1.0 ) {
			changeTime = 1.0;
		}
		VectorScale( vec, LOPER_LEAP_VELOCITY_START + changeTime * ( LOPER_LEAP_VELOCITY_END - LOPER_LEAP_VELOCITY_START ), vec );
		g_entities[cs->entityNum].s.pos.trDelta[0] = vec[0];
		g_entities[cs->entityNum].s.pos.trDelta[1] = vec[1];
	}
	//
	if ( onGround || ( cs->aiFlags & AIFL_LAND_ANIM_PLAYED ) ) {
		// if we just started the attack recently, we probably haven't had a chance to get airborne yet
		if ( cs->thinkFuncChangeTime < level.time - LOPER_LEAP_DELAY ) {
			// loper is back on ground, wait for animation to play out
			if ( !( cs->aiFlags & AIFL_LAND_ANIM_PLAYED ) ) {
				BG_PlayAnimName( &ent->client->ps, "legs_extra4", ANIM_BP_LEGS, qtrue, qfalse, qtrue );
				//
				cs->aiFlags |= AIFL_LAND_ANIM_PLAYED;
				// TODO:play the landing thud
			}
			//
			if ( ent->client->ps.legsTimer < 800 ) {  // we're done
				ent->client->ps.legsTimer = 0;
				return AIFunc_DefaultStart( cs );
			}
			// keep moving slightly in our facing direction to simulate landing momentum
			AngleVectors( cs->viewangles, vec, NULL, NULL );
			trap_EA_Move( cs->entityNum, vec, ( (float)ent->client->ps.legsTimer / (float)LOPER_LAND_DURATION ) * (float)LOPER_LEAP_LAND_MOMENTUM );
			return NULL;
		}
	}
	ent->client->ps.legsTimer = 500;    // stay on this until landing
	return NULL;
}

char *AIFunc_LoperAttack2Start( cast_state_t *cs ) {
	gentity_t *ent;
	vec3_t vec, avec;
	//
	ent = &g_entities[cs->entityNum];
	//
	if ( cs->enemyNum < 0 ) {
		return AIFunc_DefaultStart( cs );
	}
	// face them
	AICast_AimAtEnemy( cs );
	// if not facing them yet, wait
	VectorSubtract( g_entities[cs->enemyNum].client->ps.origin, cs->bs->origin, vec );
	VectorNormalize( vec );
	AngleVectors( cs->viewangles, avec, NULL, NULL );
	if ( DotProduct( vec, avec ) < 0.9 ) {
		//cs->aifunc = AIFunc_LoperAttack2Start;
		return NULL;
	}
	// OK, start the animation
	BG_PlayAnimName( &ent->client->ps, "legs_extra3", ANIM_BP_LEGS, qtrue, qfalse, qtrue );
	ent->client->ps.legsTimer = 500;    // stay on this until landing
	// send us hurtling towards our enemy
	VectorScale( vec, LOPER_LEAP_VELOCITY_START, vec );
	vec[2] = LOPER_LEAP_VELOCITY_Z;
	VectorCopy( vec, ent->client->ps.velocity );
	VectorCopy( vec, cs->loperLeapVel );
	//
	cs->aiFlags &= ~AIFL_LAND_ANIM_PLAYED;
	// play the sound
	// TODO
	//
	cs->aifunc = AIFunc_LoperAttack2;
	return "AIFunc_LoperAttack2";
}

//=================================================================================
//
// LOPER GROUND ATTACK
//
//=================================================================================

#define LOPER_GROUND_ANIM               LEGS_EXTRA5
#define LOPER_GROUND_FPS                10

#define LOPER_GROUND_FRAME_COUNT        8
#define LOPER_GROUND_DURATION           ( LOPER_GROUND_FRAME_COUNT*( 1000 / LOPER_GROUND_FPS ) )

#define LOPER_GROUND_DAMAGE             20

/*
===============
AIFunc_LoperAttack3()

  Loper's ground electrical attack
===============
*/
char *AIFunc_LoperAttack3( cast_state_t *cs ) {
	gentity_t *ent;
	qboolean hitClient = qfalse;
	//
	ent = &g_entities[cs->entityNum];
	//
	// done with this attack?
	if ( !ent->client->ps.legsTimer ) {
		cs->pauseTime = level.time + 600;   // don't move until effect is done
		ent->client->ps.legsTimer = 600;    // stay down until effect is done
		return AIFunc_DefaultStart( cs );
	}
	// ready to inflict damage?
	if ( cs->thinkFuncChangeTime < level.time - 900 ) {
		//
		// draw the client-side lightning effect
		ent->client->ps.eFlags |= EF_MONSTER_EFFECT3;
		//ent->s.effect3Time = level.time + 500;//cs->thinkFuncChangeTime + LOPER_GROUND_DELAY - 200;
		//
		// are we waiting to inflict damage?
		if ( cs->weaponFireTimes[WP_MONSTER_ATTACK3] < level.time - 100 ) {
			// check for damage
			hitClient = G_RadiusDamage( cs->bs->origin, ent, LOPER_GROUND_DAMAGE, LOPER_GROUND_RANGE, ent, MOD_LOPER_GROUND );
			//
			cs->weaponFireTimes[WP_MONSTER_ATTACK3] = level.time;
			// TODO: client-side visual effect
			// TODO: throw them backwards (away from us)
		} else {
			hitClient = qtrue;  // so we don't abort
		}
		//
		if ( !hitClient && ( ent->client->ps.legsTimer > 400 ) && cs->thinkFuncChangeTime < ( level.time - 1000 ) ) { // we're done with this attack
			cs->pauseTime = level.time + 400;   // don't move until effect is done
			ent->client->ps.legsTimer = 400;    // stay down until effect is done
			return AIFunc_DefaultStart( cs );
		}
	}
	//
	return NULL;
}

char *AIFunc_LoperAttack3Start( cast_state_t *cs ) {
	gentity_t *ent;
	//
	ent = &g_entities[cs->entityNum];
	//
	// face them
	AICast_AimAtEnemy( cs );
	// play the animation
	BG_PlayAnimName( &ent->client->ps, "legs_extra5", ANIM_BP_LEGS, qtrue, qfalse, qtrue );
	//
	// play the buildup sound
	// TODO
	//
	cs->aifunc = AIFunc_LoperAttack3;
	return "AIFunc_LoperAttack3";
}

//=================================================================================
//
// STIM SOLDIER FLYING ATTACK
//
//=================================================================================

#define STIMSOLDIER_FLYJUMP_ANIM            LEGS_EXTRA1
#define STIMSOLDIER_FLYJUMP_FPS             15
#define STIMSOLDIER_FLYJUMP_FRAME_COUNT     28
#define STIMSOLDIER_FLYJUMP_DURATION        ( STIMSOLDIER_FLYJUMP_FRAME_COUNT*( 1000 / STIMSOLDIER_FLYJUMP_FPS ) )
#define STIMSOLDIER_FLYJUMP_DELAY           ( STIMSOLDIER_FLYJUMP_DURATION + 3000 )

// hover plays continuously
#define STIMSOLDIER_FLYHOVER_ANIM           LEGS_EXTRA2
#define STIMSOLDIER_FLYHOVER_FPS            5

#define STIMSOLDIER_FLYLAND_ANIM            LEGS_LAND
#define STIMSOLDIER_FLYLAND_FPS             15
#define STIMSOLDIER_FLYLAND_FRAME_COUNT     14
#define STIMSOLDIER_FLYLAND_DURATION        ( STIMSOLDIER_FLYLAND_FRAME_COUNT*( 1000 / STIMSOLDIER_FLYLAND_FPS ) )

#define STIMSOLDIER_STARTJUMP_DELAY         ( STIMSOLDIER_FLYJUMP_DURATION*0.5 )

char *AIFunc_StimSoldierAttack1( cast_state_t *cs ) {
	gentity_t   *ent;
	vec3_t vec;
	static vec3_t up = {0,0,1};
	//
	ent = &g_entities[cs->entityNum];
	cs->weaponFireTimes[WP_MONSTER_ATTACK1] = level.time;
	// face them
	AICast_AimAtEnemy( cs );
	//
	// are we done with this attack?
	if ( cs->thinkFuncChangeTime < level.time - STIMSOLDIER_FLYJUMP_DELAY ) {
		// have we hit the ground yet?
		if ( ent->s.groundEntityNum != ENTITYNUM_NONE ) {
			// we are on something, have we started the landing animation?
			if ( !( cs->aiFlags & AIFL_LAND_ANIM_PLAYED ) ) {
				ent->client->ps.legsAnim =
					( ( ent->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | STIMSOLDIER_FLYLAND_ANIM;
				ent->client->ps.legsTimer = STIMSOLDIER_FLYLAND_DURATION;   // stay down until attack is finished
				//
				cs->noAttackTime = level.time + STIMSOLDIER_FLYLAND_DURATION;
				cs->aiFlags |= AIFL_LAND_ANIM_PLAYED;
			} else {
				if ( !ent->client->ps.legsTimer ) {   // animation has finished, resume AI
					return AIFunc_DefaultStart( cs );
				}
			}
		} else {
			// still flying
		}
		return NULL;
	}
	//
	// are we ready to start flying?
	if ( cs->thinkFuncChangeTime < ( level.time - STIMSOLDIER_STARTJUMP_DELAY ) ) {
		if ( !ent->client->ps.powerups[PW_FLIGHT] ) {
			// play a special ignition sound?
		}
		ent->client->ps.powerups[PW_FLIGHT] = 1;    // let them fly
		ent->s.loopSound = level.stimSoldierFlySound;
		ent->client->ps.eFlags |= EF_MONSTER_EFFECT;    // client-side stim engine effect
		if ( ent->s.effect1Time != ( cs->thinkFuncChangeTime + STIMSOLDIER_STARTJUMP_DELAY ) ) {
			ent->s.effect1Time = ( cs->thinkFuncChangeTime + STIMSOLDIER_STARTJUMP_DELAY );
			// start the hovering animation
			ent->client->ps.legsAnim =
				( ( ent->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | STIMSOLDIER_FLYHOVER_ANIM;
		}
		// give us some upwards velocity?
		if ( cs->thinkFuncChangeTime > level.time - STIMSOLDIER_FLYJUMP_DURATION * 0.9 ) {
			trap_EA_Move( cs->entityNum, up, 300 );
			//trap_EA_Jump(cs->entityNum);
			VectorCopy( cs->bs->origin, cs->stimFlyAttackPos );
		} else {
			// attack them
			//
			// if we can't attack, abort
			if ( AICast_CheckAttack( cs, cs->enemyNum, qfalse ) ) {
				// apply weapons..
				trap_EA_Attack( cs->entityNum );
			}
			// we're done here
			cs->thinkFuncChangeTime = -9999;
		}
	} else {
		// still on ground, so move forward to account for stepping animation
		AngleVectors( cs->viewangles, vec, NULL, NULL );
		trap_EA_Move( cs->entityNum, vec, 300 );
	}
	//
	if ( ent->client->ps.legsTimer < 1000 ) {
		ent->client->ps.legsTimer = 1000;   // stay down until effect is done
	}
	//
	return NULL;
}

char *AIFunc_StimSoldierAttack1Start( cast_state_t *cs ) {
	gentity_t   *ent;
	//static vec3_t mins={-96,-96,0}, maxs={96,96,72};
	vec3_t pos, dir;
	trace_t tr;
	//
	cs->weaponFireTimes[cs->weaponNum] = level.time;
	ent = &g_entities[cs->entityNum];
	//
	// face them
	AICast_AimAtEnemy( cs );
	// first, check if this is a good place to start the flying attack
	AngleVectors( cs->ideal_viewangles, dir, NULL, NULL );
	VectorMA( cs->bs->origin, 300, dir, pos );
	pos[2] += 128;
	trap_Trace( &tr, cs->bs->origin, cs->bs->cur_ps.mins, cs->bs->cur_ps.maxs, pos, cs->entityNum, ent->clipmask );
	if ( tr.startsolid || tr.allsolid ) {
		return NULL;    // not a good place
	}
	// check we can attack them from there
	// select our special weapon (rocket launcher or tesla)
	if ( COM_BitCheck( cs->bs->cur_ps.weapons, WP_PANZERFAUST ) ) {
		cs->weaponNum = WP_PANZERFAUST;
	} else if ( COM_BitCheck( cs->bs->cur_ps.weapons, WP_TESLA ) ) {
		cs->weaponNum = WP_TESLA;
	} else {    // no weapon?
		G_Error( "stim soldier tried special jump attack without a tesla or rocket launcher\n" );
	}
	if ( !AICast_CheckAttackAtPos( cs->entityNum, cs->enemyNum, pos, qfalse, qfalse ) ) {
		AICast_ChooseWeapon( cs, qfalse );
		return NULL;
	}
	// play the animation
	ent->client->ps.legsAnim =
		( ( ent->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | STIMSOLDIER_FLYJUMP_ANIM;
	ent->client->ps.legsTimer = STIMSOLDIER_FLYJUMP_DELAY;  // stay down until attack is finished
	//
	cs->aiFlags &= ~AIFL_LAND_ANIM_PLAYED;
	// play the buildup sound
	// TODO
	//
	cs->aifunc = AIFunc_StimSoldierAttack1;
	return "AIFunc_StimSoldierAttack1";
}

//=================================================================================
//
// STIM SOLDIER DUAL MACHINEGUN ATTACK
//
//=================================================================================

char *AIFunc_StimSoldierAttack2( cast_state_t *cs ) {
	return NULL;
}

char *AIFunc_StimSoldierAttack2Start( cast_state_t *cs ) {
	gentity_t   *ent;
	//
	cs->weaponFireTimes[cs->weaponNum] = level.time;
	ent = &g_entities[cs->entityNum];
	//
	// face them
	AICast_AimAtEnemy( cs );
	// TODO!
	G_Printf( "TODO: stim dual machinegun attack\n" );
	//
	cs->aifunc = AIFunc_StimSoldierAttack2;
	return "AIFunc_StimSoldierAttack2";
}

//=================================================================================
//
// BLACK GUARD MELEE KICK ATTACK
//
//=================================================================================

#define BLACKGUARD_KICK_DELAY   300
#define BLACKGUARD_KICK_RANGE   BLACKGUARD_MELEE_RANGE + 16
#define BLACKGUARD_KICK_DAMAGE  25

char *AIFunc_BlackGuardAttack1( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	trace_t *tr;
	vec3_t fwd;

	if ( !ent->client->ps.legsTimer ) {
		return AIFunc_DefaultStart( cs );
	}
	//
	if ( cs->enemyNum < 0 ) {
		return NULL;
	}
	// time for the melee?
	if ( !( cs->aiFlags & AIFL_MISCFLAG1 ) ) {
		// face them
		AICast_AimAtEnemy( cs );
		// ready for damage?
		if ( cs->thinkFuncChangeTime < level.time - BLACKGUARD_KICK_DELAY ) {
			cs->aiFlags |= AIFL_MISCFLAG1;
			// keep checking for impact status
			tr = CheckMeleeAttack( ent, BLACKGUARD_KICK_RANGE, qfalse );
			// do melee damage?
			if ( tr && ( tr->entityNum == cs->enemyNum ) ) {
				AngleVectors( cs->viewangles, fwd, NULL, NULL );
				G_Damage( &g_entities[tr->entityNum], ent, ent, fwd, tr->endpos, BLACKGUARD_KICK_DAMAGE, 0, MOD_GAUNTLET );
				// throw them in direction of impact
				fwd[2] = 0.5;
				VectorMA( g_entities[cs->enemyNum].client->ps.velocity, 300, fwd, g_entities[cs->enemyNum].client->ps.velocity );
			}
		}
	}

	return NULL;
}

char *AIFunc_BlackGuardAttack1Start( cast_state_t *cs ) {
	gentity_t   *ent = &g_entities[cs->entityNum];
	//
	cs->weaponFireTimes[cs->weaponNum] = level.time;
	// face them
	AICast_AimAtEnemy( cs );
	// audible sound
	AIChar_AttackSound( cs );
	// start the animation
	BG_PlayAnimName( &ent->client->ps, "kick", ANIM_BP_BOTH, qtrue, qfalse, qtrue );
	// clear flags
	cs->aiFlags &= ~( AIFL_MISCFLAG1 | AIFL_MISCFLAG2 );
	//
	cs->aifunc = AIFunc_BlackGuardAttack1;
	return "AIFunc_BlackGuardAttack1";
}


//=================================================================================
//
// REJECT X-CREATURE
//
//	Attacks are: backhand slap, blowtorch (small flamethrower)
//
//=================================================================================

////// Backhand attack (slap)

/*
==============
AIFunc_RejectAttack1
==============
*/
char *AIFunc_RejectAttack1( cast_state_t *cs ) {
	return NULL;
}

/*
==============
AIFunc_RejectAttack1Start
==============
*/
char *AIFunc_RejectAttack1Start( cast_state_t *cs ) {
	gentity_t *ent;

	ent = &g_entities[cs->entityNum];
	ent->s.effect1Time = level.time;
	cs->ideal_viewangles[YAW] = cs->viewangles[YAW];
	cs->aifunc = AIFunc_RejectAttack1;
	return "AIFunc_RejectAttack1";
}


//=================================================================================
//
// WARRIOR ZOMBIE
//
// Standing melee attacks
//
//=================================================================================

int warriorHitDamage[5] = {
	16,
	16,
	16,
	12,
	20
};

#define NUM_WARRIOR_ANIMS   5
int warriorHitTimes[NUM_WARRIOR_ANIMS][3] = {   // up to three hits per attack
	{ANIMLENGTH( 10,20 ),-1},
	{ANIMLENGTH( 15,20 ),-1},
	{ANIMLENGTH( 18,20 ),-1},
	{ANIMLENGTH( 15,20 ),-1},
	{ANIMLENGTH( 14,20 ),-1},
};

/*
================
AIFunc_WarriorZombieMelee
================
*/
char *AIFunc_WarriorZombieMelee( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];
	int hitDelay = -1, anim;
	trace_t *tr;
	cast_state_t *ecs = AICast_GetCastState( cs->enemyNum );
	aicast_predictmove_t move;
	float enemyDist;

	if ( !ent->client->ps.torsoTimer ) {
		return AIFunc_DefaultStart( cs );
	}
	//
	if ( cs->enemyNum < 0 ) {
		return NULL;
	}
	if ( ecs ) {

		anim = ( ent->client->ps.torsoAnim & ~ANIM_TOGGLEBIT ) - BG_AnimationIndexForString( "attack1", cs->entityNum );
		if ( anim < 0 || anim >= NUM_WARRIOR_ANIMS ) {
			// animation interupted
			return AIFunc_DefaultStart( cs );
		}
		if ( warriorHitTimes[anim][cs->animHitCount] >= 0 && cs->animHitCount < 3 ) {

			if ( !cs->animHitCount ) {
				hitDelay = warriorHitTimes[anim][cs->animHitCount];
			} else {
				hitDelay = warriorHitTimes[anim][cs->animHitCount] - warriorHitTimes[anim][cs->animHitCount - 1];
			}

			// check for inflicting damage
			if ( level.time - cs->weaponFireTimes[cs->weaponNum] > hitDelay ) {
				// do melee damage
				if ( ( tr = CheckMeleeAttack( ent, 44, qfalse ) ) && ( tr->entityNum == cs->enemyNum ) ) {
					G_Damage( &g_entities[tr->entityNum], ent, ent, vec3_origin, tr->endpos,
							  warriorHitDamage[anim], 0, MOD_GAUNTLET );
					G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[STAYSOUNDSCRIPT] ) );
				} else {
					G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].soundScripts[FOLLOWSOUNDSCRIPT] ) );
				}
				cs->weaponFireTimes[cs->weaponNum] = level.time;
				cs->animHitCount++;
			}
		}
		// face them
		AICast_AimAtEnemy( cs );
		if ( anim < 3 ) { // back handed-swinging, dont allow legs to move
			// if they are outside range, move forward
			AICast_PredictMovement( ecs, 2, 0.5, &move, &g_entities[cs->enemyNum].client->pers.cmd, -1 );
			enemyDist = Distance( move.endpos, cs->bs->origin );
			enemyDist -= g_entities[cs->enemyNum].r.maxs[0];
			enemyDist -= ent->r.maxs[0];
			if ( enemyDist > 16 ) {   // we can get closer
				if ( ent->client->ps.legsTimer ) {
					ent->client->ps.legsTimer = 0;      // allow legs us to move
					if ( cs->castScriptStatus.scriptNoMoveTime < level.time + 200 ) { // dont move until the legs are done lerping out of attack anim
						cs->castScriptStatus.scriptNoMoveTime = level.time + 200;
					}
				}
				if ( cs->castScriptStatus.scriptNoMoveTime < level.time ) {
					trap_EA_MoveForward( cs->entityNum );
				}
			}
		}
	}

	return NULL;
}

/*
================
AIFunc_WarriorZombieMeleeStart
================
*/
char *AIFunc_WarriorZombieMeleeStart( cast_state_t *cs ) {
	gentity_t *ent;

	ent = &g_entities[cs->entityNum];
	ent->s.effect1Time = level.time;
	cs->ideal_viewangles[YAW] = cs->viewangles[YAW];
	cs->weaponFireTimes[cs->weaponNum] = level.time;
	cs->animHitCount = 0;

	// face them
	AICast_AimAtEnemy( cs );

	// audible sound
	AIChar_AttackSound( cs );

	// play an anim
	BG_UpdateConditionValue( cs->entityNum, ANIM_COND_WEAPON, cs->weaponNum, qtrue );
	BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_FIREWEAPON, qfalse, qtrue );

	// stop charging
	BG_UpdateConditionValue( cs->entityNum, ANIM_COND_CHARGING, 0, qfalse );
	ent->flags &= ~FL_WARZOMBIECHARGE;

	cs->aifunc = AIFunc_WarriorZombieMelee;
	return "AIFunc_WarriorZombieMelee";
}

// Warrior "sight" animation
/*
================
AIFunc_WarriorZombieSight
================
*/
char *AIFunc_WarriorZombieSight( cast_state_t *cs ) {
	gentity_t *ent = &g_entities[cs->entityNum];

	if ( !ent->client->ps.torsoTimer ) {
		return AIFunc_DefaultStart( cs );
	}
	return NULL;
}

/*
================
AIFunc_WarriorZombieSightStart
================
*/
char *AIFunc_WarriorZombieSightStart( cast_state_t *cs ) {
	gentity_t *ent;

// RF, disabled
	return NULL;

	ent = &g_entities[cs->entityNum];
	cs->ideal_viewangles[YAW] = cs->viewangles[YAW];
	cs->weaponFireTimes[cs->weaponNum] = level.time;

	// face them
	AICast_AimAtEnemy( cs );

	// anim
	BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_FIRSTSIGHT, qfalse, qtrue );
	//BG_PlayAnimName( &ent->client->ps, "first_sight", ANIM_BP_BOTH, qtrue, qfalse, qtrue );

	cs->aifunc = AIFunc_WarriorZombieSight;
	return "AIFunc_WarriorZombieSight";
}

/*
================
AIFunc_WarriorZombieDefense
================
*/
char *AIFunc_WarriorZombieDefense( cast_state_t *cs ) {
	gentity_t *ent, *enemy;
	vec3_t enemyDir, vec;
	float dist;

	ent = &g_entities[cs->entityNum];

	if ( !( ent->flags & FL_DEFENSE_GUARD ) ) {
		if ( cs->weaponFireTimes[cs->weaponNum] < level.time - 100 ) {
			return AIFunc_DefaultStart( cs );
		}
		return NULL;
	}

	if ( ( cs->enemyNum < 0 ) || ( cs->dangerEntityValidTime >= level.time ) ) {
		ent->flags &= ~FL_DEFENSE_GUARD;
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;
		return NULL;
	}

	enemy = &g_entities[cs->enemyNum];

	if ( cs->thinkFuncChangeTime < level.time - 1500 ) {
		// if we cant see them
		if ( !AICast_EntityVisible( cs, cs->enemyNum, qtrue ) ) {
			ent->flags &= ~FL_DEFENSE_GUARD;
			ent->client->ps.torsoTimer = 0;
			ent->client->ps.legsTimer = 0;
			return NULL;
		}

		// if our enemy isn't using a dangerous weapon
		if ( enemy->client->ps.weapon < WP_LUGER || enemy->client->ps.weapon > WP_CLASS_SPECIAL ) {
			ent->flags &= ~FL_DEFENSE_GUARD;
			ent->client->ps.torsoTimer = 0;
			ent->client->ps.legsTimer = 0;
			return NULL;
		}

		// if our enemy isn't looking right at us, abort
		VectorSubtract( ent->client->ps.origin, enemy->client->ps.origin, vec );
		dist = VectorNormalize( vec );
		if ( dist > 512 ) {
			dist = 512;
		}
		AngleVectors( enemy->client->ps.viewangles, enemyDir, NULL, NULL );
		if ( DotProduct( vec, enemyDir ) < ( 0.98 - 0.2 * ( dist / 512 ) ) ) {
			ent->flags &= ~FL_DEFENSE_GUARD;
			ent->client->ps.torsoTimer = 0;
			ent->client->ps.legsTimer = 0;
			return NULL;
		}
	}

	cs->weaponFireTimes[cs->weaponNum] = level.time;

	if ( !ent->client->ps.torsoTimer ) {
		ent->flags &= ~FL_DEFENSE_GUARD;
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;
		return NULL;
	}

	// face them
	AICast_AimAtEnemy( cs );
	// crouching position, use smaller bounding box
	trap_EA_Crouch( cs->bs->client );

	return NULL;
}

/*
================
AIFunc_WarriorZombieDefenseStart
================
*/
char *AIFunc_WarriorZombieDefenseStart( cast_state_t *cs ) {
	gentity_t *ent, *enemy;
	vec3_t enemyDir, vec;
	float dist;
	static int lastWarriorDefense;

	if ( lastWarriorDefense <= level.time && lastWarriorDefense > level.time - 3000 ) {
		return NULL;    // dont all go into defense at once
	}
	lastWarriorDefense = level.time;

	ent = &g_entities[cs->entityNum];
	enemy = &g_entities[cs->enemyNum];

	// if our enemy isn't using a dangerous weapon
	if ( enemy->client->ps.weapon < WP_LUGER || enemy->client->ps.weapon > WP_CLASS_SPECIAL ) {
		return NULL;
	}

	// if we are doing a goto
	if ( cs->followEntity >= 0 ) {
		return NULL;
	}

	// if our enemy isn't looking right at us, abort
	VectorSubtract( ent->client->ps.origin, enemy->client->ps.origin, vec );
	dist = VectorNormalize( vec );
	if ( dist > 512 ) {
		dist = 512;
	}
	if ( dist < 128 ) {
		return NULL;
	}
	AngleVectors( enemy->client->ps.viewangles, enemyDir, NULL, NULL );
	if ( DotProduct( vec, enemyDir ) < ( 0.98 - 0.2 * ( dist / 512 ) ) ) {
		return NULL;
	}

	cs->weaponFireTimes[cs->weaponNum] = level.time;

	// face them
	AICast_AimAtEnemy( cs );

	// anim
	BG_UpdateConditionValue( cs->entityNum, ANIM_COND_WEAPON, cs->weaponNum, qtrue );
	BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_FIREWEAPON, qfalse, qtrue );
	ent->client->ps.torsoTimer = 3000;
	ent->client->ps.legsTimer = 3000;

	ent->flags |= FL_DEFENSE_GUARD;

	// when they come out of defense mode, go into charge mode
	BG_UpdateConditionValue( cs->entityNum, ANIM_COND_CHARGING, 1, qfalse );
	ent->flags |= FL_WARZOMBIECHARGE;

	cs->aifunc = AIFunc_WarriorZombieDefense;
	return "AIFunc_WarriorZombieDefense";
}
