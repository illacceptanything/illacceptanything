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



// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

// Rafael gameskill
int bg_pmove_gameskill_integer;
// done

// JPW NERVE -- added because I need to check single/multiplayer instances and branch accordingly
#ifdef CGAMEDLL
extern vmCvar_t cg_gameType;
#endif
#ifdef GAMEDLL
extern vmCvar_t g_gametype;
#endif

// JPW NERVE -- stuck this here so it can be seen client & server side
float Com_GetFlamethrowerRange( void ) {
#ifdef CGAMEDLL
	if ( cg_gameType.integer != GT_SINGLE_PLAYER ) {
		return 2500; // multiplayer range is longer for balance
	} else {
		return 1250; // single player range remains unchanged
	}
#endif
#ifdef GAMEDLL
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		return 2500;
	} else {
		return 1250;
	}
#endif
}
// jpw

pmove_t     *pm;
pml_t pml;

// movement parameters
float pm_stopspeed = 100;
//float	pm_duckScale = 0.25;

//----(SA)	modified
float pm_waterSwimScale   = 0.50;
float pm_waterSprintSwimScale = 0.70;
float pm_waterWadeScale   = 0.70;
float pm_slagSwimScale    = 0.30;
float pm_slagWadeScale    = 0.70;

float pm_accelerate       = 10;
float pm_airaccelerate    = 1;
float pm_wateraccelerate  = 4;
float pm_slagaccelerate   = 2;
float pm_flyaccelerate    = 8;

float pm_friction         = 6;
float pm_waterfriction    = 1;
float pm_slagfriction     = 1;
float pm_flightfriction   = 3;
float pm_ladderfriction   = 14;
float pm_spectatorfriction = 5.0f;

//----(SA)	end

int c_pmove = 0;


/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent ) {
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum ) {
	int i;

	if ( entityNum == ENTITYNUM_WORLD ) {
		return;
	}
	if ( pm->numtouch == MAXTOUCH ) {
		return;
	}

	// see if it is already added
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		if ( pm->touchents[ i ] == entityNum ) {
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
==============
PM_StartWeaponAnim
==============
*/
static void PM_StartWeaponAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}

	if ( pm->ps->weapAnimTimer > 0 ) {
		return;
	}

	if ( pm->cmd.weapon == WP_NONE ) {
		return;
	}

	pm->ps->weapAnim = ( ( pm->ps->weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
}

static void PM_ContinueWeaponAnim( int anim ) {
	if ( pm->cmd.weapon == WP_NONE ) {
		return;
	}

	if ( ( pm->ps->weapAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->ps->weapAnimTimer > 0 ) {
		return;     // a high priority animation is running
	}
	PM_StartWeaponAnim( anim );
}

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float backoff;
	float change;
	int i;

	backoff = DotProduct( in, normal );

	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}

/*
========================
PM_ExertSound

plays random exertion sound when sprint key is press
========================
*/
static void PM_ExertSound( void ) {
	int rval;
	static int oldexerttime = 0;
	static int oldexertcnt = 0;

	if ( pm->cmd.serverTime > oldexerttime + 500 ) {
		oldexerttime = pm->cmd.serverTime;
	} else {
		return;
	}

	rval = rand() % 3;

	if ( oldexertcnt != rval ) {
		oldexertcnt = rval;
	} else {
		oldexertcnt++;
	}

	if ( oldexertcnt > 2 ) {
		oldexertcnt = 0;
	}

	if ( oldexertcnt == 1 ) {
		PM_AddEvent( EV_EXERT2 );
	} else if ( oldexertcnt == 2 ) {
		PM_AddEvent( EV_EXERT3 );
	} else {
		PM_AddEvent( EV_EXERT1 );
	}
}


/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction( void ) {
	vec3_t vec;
	float   *vel;
	float speed, newspeed, control;
	float drop;

	vel = pm->ps->velocity;

	VectorCopy( vel, vec );
	if ( pml.walking ) {
		vec[2] = 0; // ignore slope movement
	}

	speed = VectorLength( vec );
	if ( speed < 1 ) {
		vel[0] = 0;
		vel[1] = 0;     // allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 ) {
		if ( pml.walking && !( pml.groundTrace.surfaceFlags & SURF_SLICK ) ) {
			// if getting knocked back, no friction
			if ( !( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) ) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control * pm_friction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel ) {
		if ( pm->watertype == CONTENTS_SLIME ) { //----(SA)	slag
			drop += speed * pm_slagfriction * pm->waterlevel * pml.frametime;
		} else {
			drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
		}
	}

	// apply flying friction
	if ( pm->ps->powerups[PW_FLIGHT] ) {
		drop += speed * pm_flightfriction * pml.frametime;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR ) {
		drop += speed * pm_spectatorfriction * pml.frametime;
	}

	// apply ladder strafe friction
	if ( pml.ladder ) {
		drop += speed * pm_ladderfriction * pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if ( newspeed < 0 ) {
		newspeed = 0;
	}
	newspeed /= speed;

	vel[0] = vel[0] * newspeed;
	vel[1] = vel[1] * newspeed;
	vel[2] = vel[2] * newspeed;
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( vec3_t wishdir, float wishspeed, float accel ) {
#if 1
	// q2 style
	int i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct( pm->ps->velocity, wishdir );
	addspeed = wishspeed - currentspeed;
	if ( addspeed <= 0 ) {
		return;
	}
	accelspeed = accel * pml.frametime * wishspeed;
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}

	// Ridah, variable friction for AI's
	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		accelspeed *= ( 1.0 / pm->ps->friction );
	}
	if ( accelspeed > addspeed ) {
		accelspeed = addspeed;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		pm->ps->velocity[i] += accelspeed * wishdir[i];
	}
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t wishVelocity;
	vec3_t pushDir;
	float pushLen;
	float canPush;

	VectorScale( wishdir, wishspeed, wishVelocity );
	VectorSubtract( wishVelocity, pm->ps->velocity, pushDir );
	pushLen = VectorNormalize( pushDir );

	canPush = accel * pml.frametime * wishspeed;
	if ( canPush > pushLen ) {
		canPush = pushLen;
	}

	VectorMA( pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
#endif
}



/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd ) {
	int max;
	float total;
	float scale;

	if ( pm->ps->aiChar && !( pm->ps->eFlags & EF_DUMMY_PMOVE ) ) {
		// restrict AI character movements (don't strafe or run backwards as fast as they can run forwards)
		if ( cmd->forwardmove < -64.0 ) {
			cmd->forwardmove = -64.0;
		}
		if ( cmd->rightmove > 64.0 ) {
			cmd->rightmove = 64.0;
		} else if ( cmd->rightmove < -64.0 ) {
			cmd->rightmove = -64.0;
		}
	}

	max = abs( cmd->forwardmove );
	if ( abs( cmd->rightmove ) > max ) {
		max = abs( cmd->rightmove );
	}
	if ( abs( cmd->upmove ) > max ) {
		max = abs( cmd->upmove );
	}
	if ( !max ) {
		return 0;
	}

	total = sqrt( cmd->forwardmove * cmd->forwardmove
				  + cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );
	scale = (float)pm->ps->speed * max / ( 127.0 * total );

	if ( pm->cmd.buttons & BUTTON_SPRINT && pm->ps->sprintTime > 50 ) {
		scale *= pm->ps->sprintSpeedScale;
	} else {
		scale *= pm->ps->runSpeedScale;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		scale *= 3;
	}

// JPW NERVE -- half move speed if heavy weapon is carried
// this is the counterstrike way of doing it -- ie you can switch to a non-heavy weapon and move at
// full speed.  not completely realistic (well, sure, you can run faster with the weapon strapped to your
// back than in carry position) but more fun to play.  If it doesn't play well this way we'll bog down the
// player if the own the weapon at all.
//
// added #ifdef for game/cgame to project so we can get correct g_gametype variable and only do this in
// multiplayer if necessary
#ifdef CGAMEDLL
	if ( cg_gameType.integer != GT_SINGLE_PLAYER )
#endif
#ifdef GAMEDLL
	if ( g_gametype.integer != GT_SINGLE_PLAYER )
#endif
	{
		if ( ( pm->ps->weapon == WP_VENOM ) || ( pm->ps->weapon == WP_PANZERFAUST ) ) {
			scale *= 0.5;
		}
	}
// jpw

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs reletive
to the facing dir
================
*/
static void PM_SetMovementDir( void ) {
// Ridah, changed this for more realistic angles (at the cost of more network traffic?)
#if 1
	float speed;
	vec3_t moved;
	int moveyaw;

	VectorSubtract( pm->ps->origin, pml.previous_origin, moved );

	if (    ( pm->cmd.forwardmove || pm->cmd.rightmove )
			&&  ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
			&&  ( speed = VectorLength( moved ) )
			&&  ( speed > pml.frametime * 5 ) ) { // if moving slower than 20 units per second, just face head angles
		vec3_t dir;

		VectorNormalize2( moved, dir );
		vectoangles( dir, dir );

		moveyaw = (int)AngleDelta( dir[YAW], pm->ps->viewangles[YAW] );

		if ( pm->cmd.forwardmove < 0 ) {
			moveyaw = (int)AngleNormalize180( moveyaw + 180 );
		}

		if ( abs( moveyaw ) > 75 ) {
			if ( moveyaw > 0 ) {
				moveyaw = 75;
			} else
			{
				moveyaw = -75;
			}
		}

		pm->ps->movementDir = (signed char)moveyaw;
	} else
	{
		pm->ps->movementDir = 0;
	}
#else
	if ( pm->cmd.forwardmove || pm->cmd.rightmove ) {
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 0;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 2;
		} else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 3;
		} else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 4;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 ) {
			pm->ps->movementDir = 5;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 ) {
			pm->ps->movementDir = 6;
		} else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 ) {
			pm->ps->movementDir = 7;
		}
	} else {
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 ) {
			pm->ps->movementDir = 1;
		} else if ( pm->ps->movementDir == 6 ) {
			pm->ps->movementDir = 7;
		}
	}
#endif
}


/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump( void ) {
	// JPW NERVE -- jumping in multiplayer uses and requires sprint juice (to prevent turbo skating, sprint + jumps)
	// don't allow jump accel
//	if (pm->cmd.serverTime - pm->ps->jumpTime < 850)
	if ( pm->cmd.serverTime - pm->ps->jumpTime < 500 ) {  // (SA) trying shorter time.  I find this effect annoying ;)
		return qfalse;
	}

	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return qfalse;      // don't allow jump until all buttons are up
	}

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD ) {
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane = qfalse;       // jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pm->ps->velocity[2] = JUMP_VELOCITY;
	PM_AddEvent( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 ) {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMP, qfalse, qtrue );
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	} else {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMPBK, qfalse, qtrue );
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean PM_CheckWaterJump( void ) {
	vec3_t spot;
	int cont;
	vec3_t flatforward;

	if ( pm->ps->pm_time ) {
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 ) {
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize( flatforward );

	VectorMA( pm->ps->origin, 30, flatforward, spot );
	spot[2] += 4;
	cont = pm->pointcontents( spot, pm->ps->clientNum );
	if ( !( cont & CONTENTS_SOLID ) ) {
		return qfalse;
	}

	spot[2] += 16;
	cont = pm->pointcontents( spot, pm->ps->clientNum );
	if ( cont ) {
		return qfalse;
	}

	// jump out of water
	VectorScale( pml.forward, 200, pm->ps->velocity );
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================


/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void ) {
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue );

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if ( pm->ps->velocity[2] < 0 ) {
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove( void ) {
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;
	float vel;

	if ( PM_CheckWaterJump() ) {
		PM_WaterJumpMove();
		return;
	}
#if 0
	// jump = head for surface
	if ( pm->cmd.upmove >= 10 ) {
		if ( pm->ps->velocity[2] > -300 ) {
			if ( pm->watertype == CONTENTS_WATER ) {
				pm->ps->velocity[2] = 100;
			} else if ( pm->watertype == CONTENTS_SLIME ) {
				pm->ps->velocity[2] = 80;
			} else {
				pm->ps->velocity[2] = 50;
			}
		}
	}
#endif
	PM_Friction();

	scale = PM_CmdScale( &pm->cmd );
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;       // sink towards bottom
	} else {
		for ( i = 0 ; i < 3 ; i++ )
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	if ( pm->watertype == CONTENTS_SLIME ) {    //----(SA)	slag
		if ( wishspeed > pm->ps->speed * pm_slagSwimScale ) {
			wishspeed = pm->ps->speed * pm_slagSwimScale;
		}

		PM_Accelerate( wishdir, wishspeed, pm_slagaccelerate );
	} else {
		if ( wishspeed > pm->ps->speed * pm_waterSwimScale ) {
			wishspeed = pm->ps->speed * pm_waterSwimScale;
		}

		PM_Accelerate( wishdir, wishspeed, pm_wateraccelerate );
	}


	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 ) {
		vel = VectorLength( pm->ps->velocity );
		// slide along the ground plane
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
						 pm->ps->velocity, OVERCLIP );

		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
	}

	PM_SlideMove( qfalse );
}


/*
===================
PM_InvulnerabilityMove

Only with the invulnerability powerup
===================
*/
// TTimo: unused
/*
static void PM_InvulnerabilityMove( void ) {
	pm->cmd.forwardmove = 0;
	pm->cmd.rightmove = 0;
	pm->cmd.upmove = 0;
	VectorClear(pm->ps->velocity);
}
*/

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove( void ) {
	int i;
	vec3_t wishvel;
	float wishspeed;
	vec3_t wishdir;
	float scale;

	// normal slowdown
	PM_Friction();

	if ( pm->ps->aiChar == AICHAR_NONE || pml.ladder ) {
		scale = PM_CmdScale( &pm->cmd );
	} else {
		// AI is allowed to fly freely
		scale = 1.0;
	}
	//
	// user intentions
	//
	if ( !scale ) {
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	} else {
		for ( i = 0 ; i < 3 ; i++ ) {
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_flyaccelerate );

	PM_StepSlideMove( qfalse );
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( void ) {
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	usercmd_t cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

// Ridah, moved this down, so we use the actual movement direction
	// set the movementDir so clients can rotate the legs for strafing
//	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for ( i = 0 ; i < 2 ; i++ ) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] = 0;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	PM_Accelerate( wishdir, wishspeed, pm_airaccelerate );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane ) {
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
						 pm->ps->velocity, OVERCLIP );
	}

#if 0
	//ZOID:  If we are on the grapple, try stair-stepping
	//this allows a player to use the grapple to pull himself
	//over a ledge
	if ( pm->ps->pm_flags & PMF_GRAPPLE_PULL ) {
		PM_StepSlideMove( qtrue );
	} else {
		PM_SlideMove( qtrue );
	}
#endif

	PM_StepSlideMove( qtrue );

// Ridah, moved this down, so we use the actual movement direction
	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();
}

/*
===================
PM_GrappleMove

===================
*/
// TTimo: unused
/*
static void PM_GrappleMove( void ) {
// Ridah, removed this code since we don't have a grapple, and the grapplePoint was consuming valuable msg space
#if 0
	vec3_t vel, v;
	float vlen;

	VectorScale(pml.forward, -16, v);
	VectorAdd(pm->ps->grapplePoint, v, v);
	VectorSubtract(v, pm->ps->origin, vel);
	vlen = VectorLength(vel);
	VectorNormalize( vel );

	if (vlen <= 100)
		VectorScale(vel, 10 * vlen, vel);
	else
		VectorScale(vel, 800, vel);

	VectorCopy(vel, pm->ps->velocity);

	pml.groundPlane = qfalse;
#endif
}
*/

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void ) {
	int i, stamtake;            //----(SA)
	vec3_t wishvel, oldvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;
	usercmd_t cmd;
	float accelerate;
	float vel;

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 ) {
		// begin swimming
		PM_WaterMove();
		return;
	}


	if ( PM_CheckJump() ) {
		// jumped away
		if ( pm->waterlevel > 1 ) {
			PM_WaterMove();
		} else {
			PM_AirMove();
// JPW NERVE
#if defined ( CGAMEDLL )
			if ( cg_gameType.integer != GT_SINGLE_PLAYER )
#elif defined ( GAMEDLL )
			if ( g_gametype.integer != GT_SINGLE_PLAYER )
#endif
			{
				pm->ps->sprintTime -= 2500;
				if ( pm->ps->sprintTime < 0 ) {
					pm->ps->sprintTime = 0;
				}
			} else {
				pm->ps->jumpTime = pm->cmd.serverTime;

				stamtake = 2000;    // amount to take for jump

				// take time from powerup before taking it from sprintTime
				if ( pm->ps->powerups[PW_NOFATIGUE] ) {
					if ( pm->ps->powerups[PW_NOFATIGUE] > stamtake ) {
						pm->ps->powerups[PW_NOFATIGUE] -= stamtake;
						if ( pm->ps->powerups[PW_NOFATIGUE] < 0 ) {
							pm->ps->powerups[PW_NOFATIGUE] = 0;
						}
						stamtake = 0;
					} else {
						// don't have that much bonus.  clear what you've got and take the remainder from regular stamina
						stamtake -= pm->ps->powerups[PW_NOFATIGUE];
						pm->ps->powerups[PW_NOFATIGUE] = 0;
					}
				}
				if ( stamtake ) {
					pm->ps->sprintTime -= stamtake;
					if ( pm->ps->sprintTime < 0 ) {
						pm->ps->sprintTime = 0;
					}
				}
			}
// jpw
		}
		return;
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd );

// Ridah, moved this down, so we use the actual movement direction
	// set the movementDir so clients can rotate the legs for strafing
//	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity( pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity( pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for ( i = 0 ; i < 3 ; i++ ) {
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	// when going up or down slopes the wish velocity should Not be zero
//	wishvel[2] = 0;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		/*
		if ( wishspeed > pm->ps->speed * pm_duckScale ) {
			wishspeed = pm->ps->speed * pm_duckScale;
		}
		*/
		if ( wishspeed > pm->ps->speed * pm->ps->crouchSpeedScale ) {
			wishspeed = pm->ps->speed * pm->ps->crouchSpeedScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel ) {
		float waterScale;

		waterScale = pm->waterlevel / 3.0;
		if ( pm->watertype == CONTENTS_SLIME ) { //----(SA)	slag
			waterScale = 1.0 - ( 1.0 - pm_slagSwimScale ) * waterScale;
		} else {
			waterScale = 1.0 - ( 1.0 - pm_waterSwimScale ) * waterScale;
		}

		if ( wishspeed > pm->ps->speed * waterScale ) {
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		accelerate = pm_airaccelerate;
	} else if ( ( pm->ps->stats[STAT_HEALTH] <= 0 ) && pm->ps->aiChar && ( pml.groundTrace.surfaceFlags & SURF_MONSTERSLICK ) )    {
		accelerate = pm_airaccelerate;
	} else {
		accelerate = pm_accelerate;
	}

	PM_Accelerate( wishdir, wishspeed, accelerate );

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	} else if ( ( pm->ps->stats[STAT_HEALTH] <= 0 ) && pm->ps->aiChar && ( pml.groundTrace.surfaceFlags & SURF_MONSTERSLICK ) )   {
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	} else {
		// don't reset the z velocity for slopes
//		pm->ps->velocity[2] = 0;
	}

	// show breath when standing on 'snow' surfaces
	if ( pml.groundTrace.surfaceFlags & SURF_SNOW ) {
		pm->ps->eFlags |= EF_BREATH;
	} else {
		pm->ps->eFlags &= ~EF_BREATH;
	}

	if ( pm->ps->eFlags & EF_CIG ) {
		pm->ps->eFlags |= EF_BREATH;
	}


	vel = VectorLength( pm->ps->velocity );
	VectorCopy( pm->ps->velocity, oldvel );

	// slide along the ground plane
	PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal,
					 pm->ps->velocity, OVERCLIP );

	// RF, only maintain speed if the direction is similar
	if ( DotProduct( pm->ps->velocity, oldvel ) > 0 ) {
		// don't decrease velocity when going up or down a slope
		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
	}

	// don't do anything if standing still
	if ( !pm->ps->velocity[0] && !pm->ps->velocity[1] ) {
		return;
	}

	PM_StepSlideMove( qfalse );

// Ridah, moved this down, so we use the actual movement direction
	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();
}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void ) {
	float forward;

	if ( !pml.walking ) {
		return;
	}

	// extra friction

	forward = VectorLength( pm->ps->velocity );
	forward -= 20;
	if ( forward <= 0 ) {
		VectorClear( pm->ps->velocity );
	} else {
		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, forward, pm->ps->velocity );
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void ) {
	float speed, drop, friction, control, newspeed;
	int i;
	vec3_t wishvel;
	float fmove, smove;
	vec3_t wishdir;
	float wishspeed;
	float scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength( pm->ps->velocity );
	if ( speed < 1 ) {
		VectorCopy( vec3_origin, pm->ps->velocity );
	} else
	{
		drop = 0;

		friction = pm_friction * 1.5; // extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if ( newspeed < 0 ) {
			newspeed = 0;
		}
		newspeed /= speed;

		VectorScale( pm->ps->velocity, newspeed, pm->ps->velocity );
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for ( i = 0 ; i < 3 ; i++ )
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA( pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin );
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void ) {

	if ( pm->ps->aiChar == AICHAR_HEINRICH ) {
		return EV_FOOTSTEP;
	}

	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS ) {
		return 0;
	}
	// JOSEPH 9-16-99
	if ( pml.groundTrace.surfaceFlags & SURF_METAL ) {
		return EV_FOOTSTEP_METAL;
	}

	if ( pml.groundTrace.surfaceFlags & SURF_WOOD ) {
		return EV_FOOTSTEP_WOOD;
	}

	if ( pml.groundTrace.surfaceFlags & SURF_GRASS ) {
		return EV_FOOTSTEP_GRASS;
	}

	if ( pml.groundTrace.surfaceFlags & SURF_GRAVEL ) {
		return EV_FOOTSTEP_GRAVEL;
	}
	// END JOSEPH

	if ( pml.groundTrace.surfaceFlags & SURF_ROOF ) {
		return EV_FOOTSTEP_ROOF;
	}

	if ( pml.groundTrace.surfaceFlags & SURF_SNOW ) {
		return EV_FOOTSTEP_SNOW;
	}

//----(SA)	added
	if ( pml.groundTrace.surfaceFlags & SURF_CARPET ) {
		return EV_FOOTSTEP_CARPET;
	}
//----(SA)	end
	return EV_FOOTSTEP;
}



/*
==============
PM_AddFallEvent
==============
*/
void PM_AddFallEvent( int landing, int surfaceparms ) {
//	PM_AddEvent( landing );		// old way
	BG_AddPredictableEventToPlayerstate( landing, surfaceparms, pm->ps );
}

/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void ) {
	float delta;
	float dist;
	float vel, acc;
	float t;
	float a, b, c, den;

	// Ridah, only play this if coming down hard
	if ( !pm->ps->legsTimer ) {
		if ( pml.previous_velocity[2] < -220 ) {
			BG_AnimScriptEvent( pm->ps, ANIM_ET_LAND, qfalse, qtrue );
		}
	}

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel = pml.previous_velocity[2];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den =  b * b - 4 * a * c;
	if ( den < 0 ) {
		return;
	}
	t = ( -b - sqrt( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta * delta * 0.0001;

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 ) {
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 ) {
		delta *= 0.25;
	}
	if ( pm->waterlevel == 1 ) {
		delta *= 0.5;
	}

	if ( delta < 1 ) {
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !( pml.groundTrace.surfaceFlags & SURF_NODAMAGE ) ) {
		if ( pm->debugLevel ) {
			Com_Printf( "delta: %5.2f\n", delta );
		}

//----(SA)	removed per DM
		// Rafael gameskill
//		if (bg_pmove_gameskill_integer == 1)
//		{
//			if (delta > 7)
//				delta = 8;
//		}
		// done
//----(SA)	end

		if ( delta > 77 ) {
			PM_AddFallEvent( EV_FALL_NDIE, pml.groundTrace.surfaceFlags );
		} else if ( delta > 67 ) {
			PM_AddFallEvent( EV_FALL_DMG_50, pml.groundTrace.surfaceFlags );
		} else if ( delta > 58 ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 ) {
				PM_AddFallEvent( EV_FALL_DMG_25, pml.groundTrace.surfaceFlags );
			}
		} else if ( delta > 48 ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 ) {
				PM_AddFallEvent( EV_FALL_DMG_15, pml.groundTrace.surfaceFlags );
			}
		} else if ( delta > 38.75 ) {
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[STAT_HEALTH] > 0 ) {
				PM_AddFallEvent( EV_FALL_DMG_10, pml.groundTrace.surfaceFlags );
			}
		} else if ( delta > 7 ) {
			PM_AddFallEvent( EV_FALL_SHORT, pml.groundTrace.surfaceFlags );
		} else {
			if ( !( pm->ps->pm_flags & PMF_DUCKED ) && !( pm->cmd.buttons & BUTTON_WALKING ) ) {  // quiet if crouching or walking
				PM_AddFallEvent( PM_FootstepForSurface(), pml.groundTrace.surfaceFlags );
			}
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
	pm->ps->footstepCount = 0;
}



/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace ) {
	int i, j, k;
	vec3_t point;

	if ( pm->debugLevel ) {
		Com_Printf( "%i:allsolid\n", c_pmove );
	}

	// jitter around
	for ( i = -1; i <= 1; i++ ) {
		for ( j = -1; j <= 1; j++ ) {
			for ( k = -1; k <= 1; k++ ) {
				VectorCopy( pm->ps->origin, point );
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				pm->trace( trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
				if ( !trace->allsolid ) {
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					pm->trace( trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void ) {
	trace_t trace;
	vec3_t point;

#define AI_STEPTEST_FALLDIST_PER_SEC    ( STEPSIZE + pm->ps->gravity * 0.35 )   // always allow them to fall down the minimum stepsize

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) {
		// we just transitioned into freefall
		if ( pm->debugLevel ) {
			Com_Printf( "%i:lift\n", c_pmove );
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[2] -= 64;

		pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
		//
		// RF, try and keep AI's on the ground if walking down steps
		if ( pm->ps->aiChar ) {
			if ( trace.fraction < 1.0 ) {
				float falldist, xyspeed;
				vec3_t vel;
				falldist = Distance( pm->ps->origin, trace.endpos );
				VectorCopy( pm->ps->velocity, vel );
				vel[2] = 0;
				xyspeed = VectorLength( vel );
				if ( xyspeed > 120 && falldist * pml.frametime < ( xyspeed * pml.frametime + STEPSIZE ) ) {
					// put them on the ground
					VectorCopy( trace.endpos, pm->ps->origin );
					return;
				}
			}
		}
		//
		if ( trace.fraction == 1.0 && !( pm->ps->pm_flags & PMF_LADDER ) ) {
			if ( pm->cmd.forwardmove >= 0 ) {
				BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMP, qfalse, qtrue );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMPBK, qfalse, qtrue );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace( void ) {
	vec3_t point;
	trace_t trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] - 0.25;

	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid ) {
		if ( !PM_CorrectAllSolid( &trace ) ) {
			return;
		}
	}

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0 ) {
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[2] > 0 && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10 ) {
		if ( pm->debugLevel ) {
			Com_Printf( "%i:kickoff\n", c_pmove );
		}
		if ( !( pm->ps->pm_flags & PMF_LADDER ) ) {
			// go into jump animation
			if ( pm->cmd.forwardmove >= 0 ) {
				BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMP, qfalse, qfalse );
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			} else {
				BG_AnimScriptEvent( pm->ps, ANIM_ET_JUMPBK, qfalse, qfalse );
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[2] < MIN_WALK_NORMAL ) {
		if ( pm->debugLevel ) {
			Com_Printf( "%i:steep\n", c_pmove );
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP ) {
		pm->ps->pm_flags &= ~( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		// just hit the ground
		if ( pm->debugLevel ) {
			Com_Printf( "%i:Land\n", c_pmove );
		}

		PM_CrashLand();

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[2] < -200 ) {
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//	pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel( void ) {
	vec3_t point;
	int cont;
	int sample1;
	int sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	// Ridah, modified this
	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + pm->ps->mins[2] + 1;
	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & MASK_WATER ) {
		sample2 = pm->ps->viewheight - pm->ps->mins[2];
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[2] = pm->ps->origin[2] + pm->ps->mins[2] + sample1;
		cont = pm->pointcontents( point, pm->ps->clientNum );
		if ( cont & MASK_WATER ) {
			pm->waterlevel = 2;
			point[2] = pm->ps->origin[2] + pm->ps->mins[2] + sample2;
			cont = pm->pointcontents( point, pm->ps->clientNum );
			if ( cont & MASK_WATER ) {
				pm->waterlevel = 3;
			}
		}
	}
	// done.

	// UNDERWATER
	BG_UpdateConditionValue( pm->ps->clientNum, ANIM_COND_UNDERWATER, ( pm->waterlevel > 1 ), qtrue );

}



/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck( void ) {
	trace_t trace;

	// Ridah, modified this for configurable bounding boxes
	pm->mins[0] = pm->ps->mins[0];
	pm->mins[1] = pm->ps->mins[1];

	pm->maxs[0] = pm->ps->maxs[0];
	pm->maxs[1] = pm->ps->maxs[1];

	pm->mins[2] = pm->ps->mins[2];

	if ( pm->ps->pm_type == PM_DEAD ) {
		pm->maxs[2] = pm->ps->maxs[2];          // NOTE: must set death bounding box in game code
		pm->ps->viewheight = pm->ps->deadViewHeight;
		return;
	}

	// RF, disable crouching while using MG42
	if ( pm->ps->eFlags & EF_MG42_ACTIVE ) {
		pm->maxs[2] = pm->ps->maxs[2];
		pm->ps->viewheight = pm->ps->standViewHeight;
		return;
	}

	if ( pm->cmd.upmove < 0 ) { // duck
		pm->ps->pm_flags |= PMF_DUCKED;
	} else
	{   // stand up if possible
		if ( pm->ps->pm_flags & PMF_DUCKED ) {
			// try to stand up
			pm->maxs[2] = pm->ps->maxs[2];
			pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );
			if ( !trace.allsolid ) {
				pm->ps->pm_flags &= ~PMF_DUCKED;
			}
		}
	}

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		pm->maxs[2] = pm->ps->crouchMaxZ;
		pm->ps->viewheight = pm->ps->crouchViewHeight;
	} else
	{
		pm->maxs[2] = pm->ps->maxs[2];
		pm->ps->viewheight = pm->ps->standViewHeight;
	}
	// done.
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void ) {
	float bobmove, animGap;
	int old;
	qboolean footstep;
	qboolean iswalking;
	int animResult = -1;

	if ( pm->ps->eFlags & EF_DEAD ) {
		return;
	}

	iswalking = qfalse;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt( pm->ps->velocity[0] * pm->ps->velocity[0]
						+  pm->ps->velocity[1] * pm->ps->velocity[1] );

	// mg42, always idle
	if ( pm->ps->persistant[PERS_HWEAPON_USE] ) {
		animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_IDLE, qtrue );
		//
		return;
	}

	// swimming
	if ( pm->waterlevel > 1 ) {

		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_SWIMBK, qtrue );
		} else {
			animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_SWIM, qtrue );
		}

		return;
	}

	// in the air
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE ) {
		if ( pm->ps->pm_flags & PMF_LADDER ) {             // on ladder
			if ( pm->ps->velocity[2] >= 0 ) {
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_CLIMBUP, qtrue );
				//BG_PlayAnimName( pm->ps, "BOTH_CLIMB", ANIM_BP_BOTH, qfalse, qtrue, qfalse );
			} else if ( pm->ps->velocity[2] < 0 )     {
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_CLIMBDOWN, qtrue );
				//BG_PlayAnimName( pm->ps, "BOTH_CLIMB_DOWN", ANIM_BP_BOTH, qfalse, qtrue, qfalse );
			}
		}

		return;
	}

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) {
		if (  pm->xyspeed < 5 ) {
			pm->ps->bobCycle = 0;   // start at beginning of cycle again
			pm->ps->footstepCount = 0;
		}
		if ( pm->xyspeed > 120 ) {
			return; // continue what they were doing last frame, until we stop
		}
		if ( pm->ps->pm_flags & PMF_DUCKED ) {
			animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_IDLECR, qtrue );
		}
		if ( animResult < 0 ) {
			animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_IDLE, qtrue );
		}
		//
		return;
	}


	footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED ) {
		bobmove = 0.5;  // ducked characters bob much faster
		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
			animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_WALKCRBK, qtrue );
		} else {
			animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_WALKCR, qtrue );
		}
		// ducked characters never play footsteps
	} else if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN ) {
		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4;  // faster speeds bob faster
			footstep = qtrue;
			// check for strafing
			if ( pm->cmd.rightmove && !pm->cmd.forwardmove ) {
				if ( pm->cmd.rightmove > 0 ) {
					animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_STRAFERIGHT, qtrue );
				} else {
					animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_STRAFELEFT, qtrue );
				}
			}
			if ( animResult < 0 ) {   // if we havent found an anim yet, play the run
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_RUNBK, qtrue );
			}
			if ( animResult < 0 ) {   // if we havent found an anim yet, play the run
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_WALKBK, qtrue );
			}
		} else {
			bobmove = 0.3;
			// check for strafing
			if ( pm->cmd.rightmove && !pm->cmd.forwardmove ) {
				if ( pm->cmd.rightmove > 0 ) {
					animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_STRAFERIGHT, qtrue );
				} else {
					animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_STRAFELEFT, qtrue );
				}
			}
			if ( animResult < 0 ) {   // if we havent found an anim yet, play the run
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_WALKBK, qtrue );
			}
			if ( animResult < 0 ) {   // if we havent found an anim yet, play the run
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_RUNBK, qtrue );
			}
		}

	} else {

		if ( !( pm->cmd.buttons & BUTTON_WALKING ) ) {
			bobmove = 0.4;  // faster speeds bob faster
			footstep = qtrue;
			// check for strafing
			if ( pm->cmd.rightmove && !pm->cmd.forwardmove ) {
				if ( pm->cmd.rightmove > 0 ) {
					animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_STRAFERIGHT, qtrue );
				} else {
					animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_STRAFELEFT, qtrue );
				}
			}
			if ( animResult < 0 ) {   // if we havent found an anim yet, play the run
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_RUN, qtrue );
			}
			if ( animResult < 0 ) {   // if we havent found an anim yet, play the run
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_WALK, qtrue );
			}
		} else {
			bobmove = 0.3;  // walking bobs slow
			if ( pm->ps->aiChar != AICHAR_NONE ) {
				footstep = qtrue;
				iswalking = qtrue;
			} else {
				footstep = qfalse;  // walking is quiet for the player
			}
			if ( pm->cmd.rightmove && !pm->cmd.forwardmove ) {
				if ( pm->cmd.rightmove > 0 ) {
					animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_STRAFERIGHT, qtrue );
				} else {
					animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_STRAFELEFT, qtrue );
				}
			}
			if ( animResult < 0 ) {   // if we havent found an anim yet, play the run
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_WALK, qtrue );
			}
			if ( animResult < 0 ) {   // if we havent found an anim yet, play the run
				animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_RUN, qtrue );
			}
		}
	}

	// if no anim found yet, then just use the idle as default
	if ( animResult < 0 ) {
		animResult = BG_AnimScriptAnimation( pm->ps, pm->ps->aiState, ANIM_MT_IDLE, qtrue );
	}

	// check for footstep / splash sounds
	animGap = BG_AnimGetFootstepGap( pm->ps, pm->xyspeed );

	// new method
	if ( animGap > 0 ) {

		// do the bobCycle for weapon bobbing
		pm->ps->bobCycle = (int)( pm->ps->bobCycle + bobmove * pml.msec ) & 255;

		// now footsteps
		pm->ps->footstepCount += pm->xyspeed * pml.frametime;

		if ( pm->ps->footstepCount > animGap ) {

			pm->ps->footstepCount = pm->ps->footstepCount - animGap;

			if ( !iswalking && pm->ps->sprintExertTime && pm->waterlevel <= 2 ) {
				PM_ExertSound();
			}

			if ( pm->waterlevel == 0 ) {
				if ( footstep && !pm->noFootsteps ) {
					if ( pm->ps->aiChar == AICHAR_HEINRICH ) {
						PM_AddEvent( EV_FOOTSTEP );
					} else {
						PM_AddEvent( PM_FootstepForSurface() );
					}
				}
			} else if ( pm->waterlevel == 1 ) {
				// splashing
				PM_AddEvent( EV_FOOTSPLASH );
			} else if ( pm->waterlevel == 2 ) {
				// wading / swimming at surface
				PM_AddEvent( EV_SWIM );
			} else if ( pm->waterlevel == 3 ) {
				// no sound when completely underwater
			}

		}

	} else {    // default back to old method

		old = pm->ps->bobCycle;

		if ( pm->ps->aiChar == AICHAR_SUPERSOLDIER || pm->ps->aiChar == AICHAR_PROTOSOLDIER ) {
			//iswalking = qfalse;
			bobmove = 0.4 * 0.75f;  // slow down footsteps for big guys
		}

		if ( pm->ps->aiChar == AICHAR_HEINRICH ) {
			iswalking = qfalse;
			bobmove = 0.4 * 1.3f;
		}

		if ( pm->ps->aiChar == AICHAR_HELGA ) {
			iswalking = qfalse;
			bobmove = 0.4 * 1.5f;
		}

		pm->ps->bobCycle = (int)( old + bobmove * pml.msec ) & 255;

		// if we just crossed a cycle boundary, play an apropriate footstep event
		if ( iswalking /*|| (pm->ps->aiChar == AICHAR_HEINRICH)*/ ) {
			// sounds much more natural this way
			if ( old > pm->ps->bobCycle ) {

				if ( pm->waterlevel == 0 ) {
					if ( footstep && !pm->noFootsteps ) {
						if ( pm->ps->aiChar == AICHAR_HEINRICH ) {
							PM_AddEvent( EV_FOOTSTEP );
						} else {
							PM_AddEvent( PM_FootstepForSurface() );
						}
					}
				} else if ( pm->waterlevel == 1 ) {
					// splashing
					PM_AddEvent( EV_FOOTSPLASH );
				} else if ( pm->waterlevel == 2 ) {
					// wading / swimming at surface
					PM_AddEvent( EV_SWIM );
				} else if ( pm->waterlevel == 3 ) {
					// no sound when completely underwater
				}

			}
		} else if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 )   {

			if ( pm->ps->sprintExertTime && pm->waterlevel <= 2 ) {
				PM_ExertSound();
			}

			if ( pm->waterlevel == 0 ) {
				// on ground will only play sounds if running
				if ( footstep && !pm->noFootsteps ) {
					//				if (pm->ps->aiChar == AICHAR_HEINRICH) {	// <-- this stuff now handled in PM_footstepforsurf
					//					PM_AddEvent( EV_FOOTSTEP );
					//				} else {
					PM_AddEvent( PM_FootstepForSurface() );
					//				}
				}
			} else if ( pm->waterlevel == 1 ) {
				// splashing
				PM_AddEvent( EV_FOOTSPLASH );
			} else if ( pm->waterlevel == 2 ) {
				// wading / swimming at surface
				PM_AddEvent( EV_SWIM );
			} else if ( pm->waterlevel == 3 ) {
				// no sound when completely underwater

			}
		}

	}

}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void ) {        // FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if ( !pml.previous_waterlevel && pm->waterlevel ) {
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if ( pml.previous_waterlevel && !pm->waterlevel ) {
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if ( pml.previous_waterlevel != 3 && pm->waterlevel == 3 ) {
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if ( pml.previous_waterlevel == 3 && pm->waterlevel != 3 ) {
		PM_AddEvent( EV_WATER_CLEAR );
	}
}


/*
==============
PM_BeginWeaponReload
==============
*/
static void PM_BeginWeaponReload( int weapon ) {
	// only allow reload if the weapon isn't already occupied (firing is okay)
	if ( pm->ps->weaponstate != WEAPON_READY && pm->ps->weaponstate != WEAPON_FIRING ) {
		return;
	}

	if ( weapon < WP_BEGINGERMAN || weapon > WP_DYNAMITE ) {
		return;
	}

	// no reload when you've got a chair in your hands
	if ( pm->ps->eFlags & EF_MELEE_ACTIVE ) {
		return;
	}

	switch ( weapon ) {
	case WP_DYNAMITE:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
		break;

		// no reloading
	case WP_KNIFE:
	case WP_TESLA:
		return;

	default:
		// DHM - Nerve :: override current animation (so reloading after firing will work)
		BG_AnimScriptEvent( pm->ps, ANIM_ET_RELOAD, qfalse, qtrue );
		break;
	}


	PM_ContinueWeaponAnim( WEAP_RELOAD1 );


	// okay to reload while overheating without tacking the reload time onto the end of the
	// current weaponTime (the reload time is partially absorbed into the overheat time)
	if ( pm->ps->weaponstate == WEAPON_READY ) {                  // set wait to the reload duration
		pm->ps->weaponTime += ammoTable[weapon].reloadTime;
	} else if ( pm->ps->weaponTime < ammoTable[weapon].reloadTime ) {
		pm->ps->weaponTime += ( ammoTable[weapon].reloadTime - pm->ps->weaponTime );
	}

	pm->ps->weaponstate = WEAPON_RELOADING;
	PM_AddEvent( EV_FILL_CLIP );    // play reload sound
}




/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int oldweapon, int newweapon, qboolean reload ) {  //----(SA)	modified to play 1st person alt-mode transition animations.
	int switchtime;
	qboolean altswitch, showdrop;

	if ( newweapon < WP_NONE || newweapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( !pm->ps->aiChar && !( pm->ps->eFlags & EF_DEAD ) && ( newweapon == WP_NONE ) ) {   // RF, dont allow changing to null weapon
		return;
	}

	if ( newweapon != WP_NONE && !( COM_BitCheck( pm->ps->weapons, newweapon ) ) ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING || pm->ps->weaponstate == WEAPON_DROPPING_TORELOAD ) {   //----(SA)	added
		return;
	}

	if ( pm->ps->grenadeTimeLeft > 0 ) {   // don't allow switch if you're holding a hot potato or dynamite
		return;
	}

	if ( !pm->ps->aiChar && !oldweapon ) {    // go straight to the new weapon
		pm->ps->weaponDelay = 0;
		pm->ps->weaponTime = 0;
		pm->ps->weaponstate = WEAPON_RAISING;
		pm->ps->weapon = newweapon;
		return;
	}

	altswitch = (qboolean)( newweapon == weapAlts[oldweapon] );

	showdrop = qtrue;

	if ( oldweapon == WP_GRENADE_LAUNCHER ||
		 oldweapon == WP_GRENADE_PINEAPPLE ||
		 oldweapon == WP_DYNAMITE ||
		 oldweapon == WP_PANZERFAUST ) {
		if ( !pm->ps->ammoclip[oldweapon] ) {  // you're empty, don't show grenade '0'
			showdrop = qfalse;
		}
	}


	switch ( newweapon ) {

	case WP_GAUNTLET:
	case WP_MONSTER_ATTACK1:
	case WP_MONSTER_ATTACK2:
	case WP_MONSTER_ATTACK3:
		break;

	case WP_DYNAMITE:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
		pm->ps->grenadeTimeLeft = 0;        // initialize the timer on the potato you're switching to

	default:
		//----(SA)	only play the weapon switch sound for the player
		if ( !( pm->ps->aiChar ) ) {
			PM_AddEvent( EV_CHANGE_WEAPON );
		}

		if ( altswitch ) {  // it's an alt mode, play different anim
			PM_StartWeaponAnim( WEAP_ALTSWITCHFROM );
		} else {
			if ( showdrop ) {
				PM_StartWeaponAnim( WEAP_DROP );    // PM_ContinueWeaponAnim(WEAP_DROP);
			}
		}
	}

	BG_AnimScriptEvent( pm->ps, ANIM_ET_DROPWEAPON, qfalse, qfalse );

	if ( reload ) {
		pm->ps->weaponstate = WEAPON_DROPPING_TORELOAD;
	} else {
		pm->ps->weaponstate = WEAPON_DROPPING;
	}

	switchtime = 250;   // dropping/raising usually takes 1/4 sec.
	// sometimes different switch times for alt weapons
	switch ( oldweapon ) {
	case WP_LUGER:
		if ( altswitch ) {
			switchtime = 50;
		}
		break;
	case WP_SILENCER:
		if ( altswitch ) {
			switchtime = 1200;
		}
		break;
	case WP_FG42:
	case WP_FG42SCOPE:
		if ( altswitch ) {
			switchtime = 50;        // fast
		}
		break;

//		case WP_MAUSER:
//		case WP_SNIPERRIFLE:
//		case WP_GARAND:
//		case WP_SNOOPERSCOPE:
//			if(altswitch)
//				switchtime  = 100;
//			break;
	}

	pm->ps->weaponTime += switchtime;
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	int oldweapon, newweapon, switchtime;

	newweapon = pm->cmd.weapon;
	if ( newweapon < WP_NONE || newweapon >= WP_NUM_WEAPONS ) {
		newweapon = WP_NONE;
	}

	if ( !( COM_BitCheck( pm->ps->weapons, newweapon ) ) ) {
		newweapon = WP_NONE;
	}

	oldweapon = pm->ps->weapon;

	pm->ps->weapon = newweapon;

	if ( pm->ps->weaponstate == WEAPON_DROPPING_TORELOAD ) {
		pm->ps->weaponstate = WEAPON_RAISING_TORELOAD;  //----(SA)	added
	} else {
		pm->ps->weaponstate = WEAPON_RAISING;
	}

	switch ( newweapon )
	{
		// don't really care about anim since these weapons don't show in view.
		// However, need to set the animspreadscale so they are initally at worst accuracy
	case WP_SNOOPERSCOPE:
	case WP_SNIPERRIFLE:
	case WP_FG42SCOPE:
		pm->ps->aimSpreadScale = 255;               // initially at lowest accuracy
		pm->ps->aimSpreadScaleFloat = 255.0f;       // initially at lowest accuracy

	default:
		break;
	}

	// doesn't happen too often (player switched weapons away then back very quickly)
	if ( oldweapon == newweapon ) {
		return;
	}

	// dropping/raising usually takes 1/4 sec.
	switchtime = 250;

	// sometimes different switch times for alt weapons
	switch ( newweapon ) {
	case WP_LUGER:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 50;
		}
		break;
	case WP_SILENCER:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 1190;
		}
		break;
	case WP_FG42:
	case WP_FG42SCOPE:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 50;        // fast
		}
		break;
	}

	pm->ps->weaponTime += switchtime;

	// make scripting aware of new weapon
	BG_UpdateConditionValue( pm->ps->clientNum, ANIM_COND_WEAPON, newweapon, qtrue );

	// play an animation
	BG_AnimScriptEvent( pm->ps, ANIM_ET_RAISEWEAPON, qfalse, qfalse );

	// alt weapon switch was played when switching away, just go into idle
	if ( weapAlts[oldweapon] == newweapon ) {
		PM_StartWeaponAnim( WEAP_ALTSWITCHTO );
	} else {
		PM_StartWeaponAnim( WEAP_RAISE );
	}

}


/*
==============
PM_ReloadClip
==============
*/
static void PM_ReloadClip( int weapon ) {
	int ammoreserve, ammoclip, ammomove;

	ammoreserve = pm->ps->ammo[ BG_FindAmmoForWeapon( weapon )];
	ammoclip    = pm->ps->ammoclip[BG_FindClipForWeapon( weapon )];

	ammomove = ammoTable[weapon].maxclip - ammoclip;

	if ( ammoreserve < ammomove ) {
		ammomove = ammoreserve;
	}

	if ( ammomove ) {
		pm->ps->ammo[ BG_FindAmmoForWeapon( weapon )] -= ammomove;
		pm->ps->ammoclip[BG_FindClipForWeapon( weapon )] += ammomove;
	}

	if ( weapon == WP_AKIMBO ) { // reload colt too
		PM_ReloadClip( WP_COLT );
	}
}

/*
==============
PM_FinishWeaponReload
==============
*/

static void PM_FinishWeaponReload( void ) {
	PM_ReloadClip( pm->ps->weapon );          // move ammo into clip
	pm->ps->weaponstate = WEAPON_READY;     // ready to fire
}


/*
==============
PM_CheckforReload
==============
*/
void PM_CheckForReload( int weapon ) {
	qboolean reloadRequested;
	qboolean doReload = qfalse;
	int clipWeap, ammoWeap;

	if ( pm->noWeapClips ) { // no need to reload
		return;
	}

	// user is forcing a reload (manual reload)
	reloadRequested = (qboolean)( pm->cmd.wbuttons & WBUTTON_RELOAD );

	switch ( pm->ps->weaponstate ) {
	case WEAPON_RAISING:
	case WEAPON_RAISING_TORELOAD:       //----(SA)	added
	case WEAPON_DROPPING:
	case WEAPON_DROPPING_TORELOAD:      //----(SA)	added
	case WEAPON_READYING:
	case WEAPON_RELAXING:
	case WEAPON_RELOADING:
		return;
		break;
	default:
		break;
	}

	clipWeap = BG_FindClipForWeapon( weapon );
	ammoWeap = BG_FindAmmoForWeapon( weapon );

	// don't allow reloading for these weapons.  when the player hits 'reload' or 'fire'
	// when the weapon is empty, it will switch away to the primary.  (so player can see results of
	// last shot through scope without it auto-switching away for the reload)
	if ( !pm->ps->aiChar ) {  // RF, added this check since this confuses the AI
		switch ( weapon ) {
		case WP_SNOOPERSCOPE:
		case WP_SNIPERRIFLE:
		case WP_FG42SCOPE:
			if ( reloadRequested ) {
				doReload = qtrue;
				if ( !( pm->ps->ammo[ammoWeap] ) ) { // no ammo left. when you switch out, don't try to reload
					doReload = qfalse;
				}
				PM_BeginWeaponChange( weapon, weapAlts[weapon], doReload );
			}
			return;
		default:
			break;
		}
	}


//	if ( pm->ps->weaponTime <= 0) {

	if ( reloadRequested ) {
		// don't allow a force reload if it won't have any effect (no more ammo reserves or full clip)
		if ( pm->ps->ammo[ammoWeap] ) {
			if ( pm->ps->ammoclip[clipWeap] < ammoTable[weapon].maxclip ) {
				doReload = qtrue;
			}
			if ( weapon == WP_AKIMBO ) {
				// akimbo should also check Colt status
				if ( pm->ps->ammoclip[BG_FindClipForWeapon( WP_COLT )] < ammoTable[BG_FindClipForWeapon( WP_COLT )].maxclip ) {
					doReload = qtrue;
				}
			}
		}
	}
	// clip is empty, but you have reserves.  (auto reload)
	else if ( !( pm->ps->ammoclip[clipWeap] ) ) {   // clip is empty...
		if ( pm->ps->ammo[ammoWeap] ) {         // and you have reserves
			if ( weapon == WP_AKIMBO ) {    // if colt's got ammo, don't force reload yet (you know you've got it 'out' since you've got the akimbo selected
				if ( !( pm->ps->ammoclip[WP_COLT] ) ) {
					doReload = qtrue;
				}
				// likewise.  however, you need to check if you've got the akimbo selected, since you could have the colt alone
			} else if ( weapon == WP_COLT ) {   // weapon checking for reload is colt...
				if ( pm->ps->weapon == WP_AKIMBO ) {    // you've got the akimbo selected...
					if ( !( pm->ps->ammoclip[WP_AKIMBO] ) ) {   // and it's got no ammo either
						doReload = qtrue;       // so reload
					}
				} else {     // single colt selected
					doReload = qtrue;       // so reload
				}
			} else {
				doReload = qtrue;
			}
		}
	}

//	}

	if ( doReload ) {
		PM_BeginWeaponReload( weapon );
	}


}

/*
==============
PM_SwitchIfEmpty
==============
*/
static void PM_SwitchIfEmpty( void ) {
	// TODO: cvar for emptyswitch
//	if(!cg_emptyswitch.integer)
//		return;

	// weapon from here down will be a thrown explosive
	switch ( pm->ps->weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_PANZERFAUST:
		break;
	default:
		return;
	}


	if ( pm->ps->ammoclip[ BG_FindClipForWeapon( pm->ps->weapon )] ) { // still got ammo in clip
		return;
	}

	if ( pm->ps->ammo[ BG_FindAmmoForWeapon( pm->ps->weapon )] ) { // still got ammo in reserve
		return;
	}

	// If this was the last one, remove the weapon and switch away before the player tries to fire next

	// NOTE: giving grenade ammo to a player will re-give him the weapon (if you do it through add_ammo())
	switch ( pm->ps->weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
		// take the 'weapon' away from the player
		COM_BitClear( pm->ps->weapons, pm->ps->weapon );
		break;
	default:
		break;
	}

	PM_AddEvent( EV_NOAMMO );
}


/*
==============
PM_WeaponUseAmmo
	accounts for clips being used/not used
==============
*/
void PM_WeaponUseAmmo( int wp, int amount ) {
	int takeweapon;

	if ( pm->noWeapClips ) {
		pm->ps->ammo[ BG_FindAmmoForWeapon( wp )] -= amount;
	} else {
		takeweapon = BG_FindClipForWeapon( wp );
		if ( wp == WP_AKIMBO ) {
			if ( !BG_AkimboFireSequence( wp, pm->ps->ammoclip[WP_AKIMBO], pm->ps->ammoclip[WP_COLT] ) ) {
				takeweapon = WP_COLT;
			}
		}

		pm->ps->ammoclip[takeweapon] -= amount;
	}
}


/*
==============
PM_WeaponAmmoAvailable
	accounts for clips being used/not used
==============
*/
int PM_WeaponAmmoAvailable( int wp ) {
	int takeweapon;

	if ( pm->noWeapClips ) {
		return pm->ps->ammo[ BG_FindAmmoForWeapon( wp )];
	} else {
//		return pm->ps->ammoclip[BG_FindClipForWeapon( wp )];
		takeweapon = BG_FindClipForWeapon( wp );
		if ( wp == WP_AKIMBO ) {
			if ( !BG_AkimboFireSequence( pm->ps->weapon, pm->ps->ammoclip[WP_AKIMBO], pm->ps->ammoclip[WP_COLT] ) ) {
				takeweapon = WP_COLT;
			}
		}

		return pm->ps->ammoclip[takeweapon];
	}
}

/*
==============
PM_WeaponClipEmpty
	accounts for clips being used/not used
==============
*/
int PM_WeaponClipEmpty( int wp ) {
	if ( pm->noWeapClips ) {
		if ( !( pm->ps->ammo[ BG_FindAmmoForWeapon( wp )] ) ) {
			return 1;
		}
	} else {
		if ( !( pm->ps->ammoclip[BG_FindClipForWeapon( wp )] ) ) {
			return 1;
		}
	}

	return 0;
}


/*
==============
PM_CoolWeapons
==============
*/
void PM_CoolWeapons( void ) {
	int wp;

	for ( wp = 0; wp < WP_NUM_WEAPONS; wp++ ) {

		// if you have the weapon
		if ( COM_BitCheck( pm->ps->weapons, wp ) ) {
			// and it's hot
			if ( pm->ps->weapHeat[wp] ) {
				pm->ps->weapHeat[wp] -= ( (float)ammoTable[wp].coolRate * pml.frametime );

				if ( pm->ps->weapHeat[wp] < 0 ) {
					pm->ps->weapHeat[wp] = 0;
				}

			}
		}
	}

	// a weapon is currently selected, convert current heat value to 0-255 range for client transmission
	if ( pm->ps->weapon ) {
		pm->ps->curWeapHeat = ( ( (float)pm->ps->weapHeat[pm->ps->weapon] / (float)ammoTable[pm->ps->weapon].maxHeat ) ) * 255.0f;

//		if(pm->ps->weapHeat[pm->ps->weapon])
//			Com_Printf("pm heat: %d, %d\n", pm->ps->weapHeat[pm->ps->weapon], pm->ps->curWeapHeat);
	}

}

/*
==============
PM_AdjustAimSpreadScale
==============
*/
void PM_AdjustAimSpreadScale( void ) {
//	int		increase, decrease, i;
	int i;
	float increase, decrease;       // (SA) was losing lots of precision on slower weapons (scoped)
	float viewchange, cmdTime, wpnScale;
//#define	AIMSPREAD_DECREASE_RATE		300.0f
#define AIMSPREAD_DECREASE_RATE     200.0f      // (SA) when I made the increase/decrease floats (so slower weapon recover could happen for scoped weaps) the average rate increased significantly
#define AIMSPREAD_INCREASE_RATE     800.0f
#define AIMSPREAD_VIEWRATE_MIN      30.0f       // degrees per second
#define AIMSPREAD_VIEWRATE_RANGE    120.0f      // degrees per second

	// all weapons are very inaccurate in zoomed mode

	if ( pm->ps->eFlags & EF_ZOOMING ) {

		pm->ps->aimSpreadScale = 255;
		pm->ps->aimSpreadScaleFloat = 255;
		return;
	}

	cmdTime = (float)( pm->cmd.serverTime - pm->oldcmd.serverTime ) / 1000.0;

	wpnScale = 0.0f;
	switch ( pm->ps->weapon ) {
	case WP_LUGER:
	case WP_SILENCER:
		wpnScale = 0.5f;
		break;
	case WP_AKIMBO: //----(SA)	added
		wpnScale = 0.5;
		break;
	case WP_COLT:
		wpnScale = 0.4f;        // doesn't fire as fast, but easier to handle than luger
		break;
	case WP_VENOM:
		wpnScale = 0.9f;        // very heavy
		break;
	case WP_SNIPERRIFLE:    // (SA) looong time to recover
		wpnScale = 10.0f;
		break;
	case WP_SNOOPERSCOPE:   // (SA) looong time to recover
		wpnScale = 8.0f;
		break;
	case WP_MAUSER:
		wpnScale = 0.5f;
		break;
	case WP_GARAND:
		wpnScale = 0.5f;
		break;
	case WP_MP40:
		wpnScale = 0.5f;        // 2 handed, but not as long as mauser, so harder to keep aim
		break;
	case WP_FG42:
		wpnScale = 0.9f;
		break;
	case WP_FG42SCOPE:
		wpnScale = 0.7f;
		break;
	case WP_THOMPSON:
		wpnScale = 0.4f;
		break;
	case WP_STEN:
		wpnScale = 0.6f;
		break;
	case WP_PANZERFAUST:
//		wpnScale = 1.3f;
		wpnScale = 0.6f;
		break;
	}

	if ( wpnScale ) {

// JPW NERVE crouched players recover faster (mostly useful for snipers)
		if ( ( pm->ps->eFlags & EF_CROUCHING ) && ( pm->ps->groundEntityNum != ENTITYNUM_NONE ) ) {  //----(SA)	modified so you can't do this in the air.  cool?
			wpnScale *= 0.5;
		}
// jpw

		decrease = ( cmdTime * AIMSPREAD_DECREASE_RATE ) / wpnScale;

		viewchange = 0;
		// take player view rotation into account
		for ( i = 0; i < 2; i++ )
			viewchange += fabs( SHORT2ANGLE( pm->cmd.angles[i] ) - SHORT2ANGLE( pm->oldcmd.angles[i] ) );

		// take player movement into account (even if only for the scoped weapons)
		// TODO: also check for jump/crouch and adjust accordingly
		switch ( pm->ps->weapon ) {
		case WP_SNIPERRIFLE:
		case WP_SNOOPERSCOPE:
		case WP_FG42SCOPE:
			for ( i = 0; i < 2; i++ )
				viewchange += fabs( pm->ps->velocity[i] );
			break;
		case WP_PANZERFAUST:        // don't take movement into account as much
			for ( i = 0; i < 2; i++ )
				viewchange += ( 0.01f * fabs( pm->ps->velocity[i] ) );
			break;
		default:
			break;
		}



		viewchange = (float)viewchange / cmdTime;   // convert into this movement for a second
		viewchange -= AIMSPREAD_VIEWRATE_MIN / wpnScale;
		if ( viewchange <= 0 ) {
			viewchange = 0;
		} else if ( viewchange > ( AIMSPREAD_VIEWRATE_RANGE / wpnScale ) ) {
			viewchange = AIMSPREAD_VIEWRATE_RANGE / wpnScale;
		}

		// now give us a scale from 0.0 to 1.0 to apply the spread increase
		viewchange = viewchange / (float)( AIMSPREAD_VIEWRATE_RANGE / wpnScale );

		increase = (int)( cmdTime * viewchange * AIMSPREAD_INCREASE_RATE );
	} else {
		increase = 0;
		decrease = AIMSPREAD_DECREASE_RATE;
	}

	// update the aimSpreadScale
	pm->ps->aimSpreadScaleFloat += ( increase - decrease );
	if ( pm->ps->aimSpreadScaleFloat < 0 ) {
		pm->ps->aimSpreadScaleFloat = 0;
	}
	if ( pm->ps->aimSpreadScaleFloat > 255 ) {
		pm->ps->aimSpreadScaleFloat = 255;
	}

	pm->ps->aimSpreadScale = (int)pm->ps->aimSpreadScaleFloat;  // update the int for the client
}

#define weaponstateFiring ( pm->ps->weaponstate == WEAPON_FIRING || pm->ps->weaponstate == WEAPON_FIRINGALT )

#define GRENADE_DELAY   250

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/

#define VENOM_LOW_IDLE  WEAP_IDLE1
#define VENOM_HI_IDLE   WEAP_IDLE2
#define VENOM_RAISE     WEAP_ATTACK1
#define VENOM_ATTACK    WEAP_ATTACK2
#define VENOM_LOWER     WEAP_ATTACK_LASTSHOT

// JPW NERVE
#ifdef CGAMEDLL
extern vmCvar_t cg_soldierChargeTime;
extern vmCvar_t cg_engineerChargeTime;
#endif
#ifdef GAMEDLL
extern vmCvar_t g_soldierChargeTime;
extern vmCvar_t g_engineerChargeTime;
#endif
// jpw


#ifdef CGAMEDLL
extern vmCvar_t cg_reloading;
#endif
#ifdef GAMEDLL
extern vmCvar_t g_reloading;
#endif


/*
==============
PM_Weapon
==============
*/
static void PM_Weapon( void ) {
	int addTime;
	int ammoNeeded;
	qboolean delayedFire;       //----(SA)  true if the delay time has just expired and this is the frame to send the fire event
	int aimSpreadScaleAdd;
	int weapattackanim;
	qboolean akimboFire;
	qboolean gameReloading;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// game is reloading (mission fail/success)
#ifdef CGAMEDLL
	if ( cg_reloading.integer )
#endif
#ifdef GAMEDLL
	if ( g_reloading.integer )
#endif
	gameReloading = qtrue;
	else {
		gameReloading = qfalse;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->weapon = WP_NONE;
		return;
	}

	// don't allow any weapon stuff if using the mg42
	if ( pm->ps->persistant[PERS_HWEAPON_USE] ) {
		return;
	}

	// RF, remoed this, was preventing lava from hurting player
	//pm->watertype = 0;

	akimboFire = BG_AkimboFireSequence( pm->ps->weapon, pm->ps->ammoclip[WP_AKIMBO], pm->ps->ammoclip[WP_COLT] );

	if ( 0 ) {
		switch ( pm->ps->weaponstate ) {
		case WEAPON_READY:
			Com_Printf( " -- WEAPON_READY\n" );
			break;
		case WEAPON_RAISING:
			Com_Printf( " -- WEAPON_RAISING\n" );
			break;
		case WEAPON_RAISING_TORELOAD:       //----(SA)	added
			Com_Printf( " -- WEAPON_RAISING_TORELOAD\n" );
			break;
		case WEAPON_DROPPING:
			Com_Printf( " -- WEAPON_DROPPING\n" );
			break;
		case WEAPON_DROPPING_TORELOAD:      //----(SA)	added
			Com_Printf( " -- WEAPON_DROPPING_TORELOAD\n" );
			break;
		case WEAPON_READYING:
			Com_Printf( " -- WEAPON_READYING\n" );
			break;
		case WEAPON_RELAXING:
			Com_Printf( " -- WEAPON_RELAXING\n" );
			break;
		case WEAPON_VENOM_REST:
			Com_Printf( " -- WEAPON_VENOM_REST\n" );
			break;
		case WEAPON_FIRING:
			Com_Printf( " -- WEAPON_FIRING\n" );
			break;
		case WEAPON_FIRINGALT:
			Com_Printf( " -- WEAPON_FIRINGALT\n" );
			break;
		case WEAPON_RELOADING:
			Com_Printf( " -- WEAPON_RELOADING\n" );
			break;
		}
	}

	// dec venom timer
	if ( pm->ps->venomTime > 0 ) {
		pm->ps->venomTime -= pml.msec;
	}


//----(SA)	removed 'quickgrenade'

	// weapon cool down
	PM_CoolWeapons();

	// check for item using
	if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) {
		if ( !( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) ) {
			gitem_t *item;

			pm->ps->pm_flags |= PMF_USE_ITEM_HELD;

			if ( pm->cmd.holdable ) {
				item = BG_FindItemForHoldable( pm->cmd.holdable );

				if ( item && ( pm->ps->holdable[pm->cmd.holdable] >= item->quantity ) ) { // ->quantity being how much 'ammo' is taken per use
					PM_AddEvent( EV_USE_ITEM0 + pm->cmd.holdable );
					// don't take books away when used
					if ( pm->cmd.holdable < HI_BOOK1 || pm->cmd.holdable > HI_BOOK3 ) {
						pm->ps->holdable[ pm->cmd.holdable ] -= item->quantity;
					}

					if ( pm->ps->holdable[pm->cmd.holdable] <= 0 ) {   // empty
						PM_AddEvent( EV_NOITEM );
					}
				}
			} else {
				PM_AddEvent( EV_USE_ITEM0 );     // send "using nothing"
			}
			return;
		}
	} else
	{
		pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
	}


	delayedFire = qfalse;

	if ( pm->ps->weapon == WP_GRENADE_LAUNCHER || pm->ps->weapon == WP_GRENADE_PINEAPPLE || pm->ps->weapon == WP_DYNAMITE ) {
		// (SA) AI's don't set grenadeTimeLeft on +attack, so I don't check for (pm->ps->aiChar) here
		if ( pm->ps->grenadeTimeLeft > 0 ) {
			if ( pm->ps->weapon == WP_DYNAMITE ) {
				pm->ps->grenadeTimeLeft += pml.msec;
// JPW NERVE -- in multiplayer, dynamite becomes strategic, so start timer @ 30 seconds
#ifdef CGAMEDLL
				if ( cg_gameType.integer != GT_SINGLE_PLAYER ) {
#endif
#ifdef GAMEDLL
				if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
#endif
				if ( pm->ps->grenadeTimeLeft < 5000 ) {
					pm->ps->grenadeTimeLeft = 5000;
				}
			}
// jpw

			if ( pm->ps->grenadeTimeLeft > 8000 ) {
				PM_AddEvent( EV_FIRE_WEAPON );
				pm->ps->weaponTime = 1600;
				PM_WeaponUseAmmo( pm->ps->weapon, 1 );      //----(SA)	take ammo
				return;
			}

//				Com_Printf("Dynamite Timer: %d\n", pm->ps->grenadeTimeLeft);
		} else {
			pm->ps->grenadeTimeLeft -= pml.msec;
//				Com_Printf("Grenade Timer: %d\n", pm->ps->grenadeTimeLeft);

			if ( pm->ps->grenadeTimeLeft <= 0 ) {   // give two frames advance notice so there's time to launch and detonate
//					pm->ps->grenadeTimeLeft = 100;
//					PM_AddEvent( EV_FIRE_WEAPON );
				PM_WeaponUseAmmo( pm->ps->weapon, 1 );      //----(SA)	take ammo
//					pm->ps->weaponTime = 1600;

				PM_AddEvent( EV_GRENADE_SUICIDE );      //----(SA)	die, dumbass

				return;
			}
		}

		if ( !( pm->cmd.buttons & BUTTON_ATTACK ) ) { //----(SA)	modified
			if ( pm->ps->weaponDelay == ammoTable[pm->ps->weapon].fireDelayTime ) {
				// released fire button.  Fire!!!
				BG_AnimScriptEvent( pm->ps, ANIM_ET_FIREWEAPON, qfalse, qtrue );
			}
		} else {
			return;
		}
	}
}

if ( pm->ps->weaponDelay > 0 ) {
	pm->ps->weaponDelay -= pml.msec;
	if ( pm->ps->weaponDelay <= 0 ) {
		pm->ps->weaponDelay = 0;
		delayedFire = qtrue;            // weapon delay has expired.  Fire this frame

		// double check the player is still holding the fire button down for these weapons
		// so you don't get a delayed "non-fire" (fire hit and released, then shot fires)
		switch ( pm->ps->weapon ) {
		case WP_VENOM:
			if ( pm->ps->weaponstate == WEAPON_FIRING ) {
				delayedFire = qfalse;
			}
			break;
		default:
			break;
		}
	}
}


if ( pm->ps->weaponstate == WEAPON_RELAXING ) {
	pm->ps->weaponstate = WEAPON_READY;
	return;
}

// make weapon function
if ( pm->ps->weaponTime > 0 ) {
	pm->ps->weaponTime -= pml.msec;
	if ( pm->ps->weaponTime < 0 ) {
		pm->ps->weaponTime = 0;
	}

//----(SA)	removed for DM and id
/*
		// RF, testing special case Pistol, which fires faster if you tap the fire button
		if ( pm->ps->weapon == WP_LUGER ) {
			if ( pm->ps->releasedFire ) {
				if (pm->ps->weaponTime <= 250 && (pm->cmd.buttons & BUTTON_ATTACK)) {
					pm->ps->weaponTime = 0;
				}
			} else if (!(pm->cmd.buttons & BUTTON_ATTACK)) {
				pm->ps->releasedFire = qtrue;
			}
		} else if ( pm->ps->weapon == WP_COLT ) {
			if ( pm->ps->releasedFire ) {
				if (pm->ps->weaponTime <= 150 && (pm->cmd.buttons & BUTTON_ATTACK)) {
					pm->ps->weaponTime = 0;
				}
			} else if (!(pm->cmd.buttons & BUTTON_ATTACK)) {
				pm->ps->releasedFire = qtrue;
			}
		} else if ( pm->ps->weapon == WP_SILENCER ) {
			if ( pm->ps->releasedFire ) {
				if (pm->ps->weaponTime <= 250 && (pm->cmd.buttons & BUTTON_ATTACK)) {
					pm->ps->weaponTime = 0;
				}
			} else if (!(pm->cmd.buttons & BUTTON_ATTACK)) {
				pm->ps->releasedFire = qtrue;
			}
		}
*/
//----(SA)	end

// JPW NERVE -- added back for multiplayer pistol balancing
#ifdef CGAMEDLL
	if ( cg_gameType.integer != GT_SINGLE_PLAYER ) {
#endif
#ifdef GAMEDLL
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
#endif
	if ( pm->ps->weapon == WP_LUGER ) {
		if ( pm->ps->releasedFire ) {
			if ( pm->ps->weaponTime <= 150 && ( pm->cmd.buttons & BUTTON_ATTACK ) ) {
				pm->ps->weaponTime = 0;
			}
		} else if ( !( pm->cmd.buttons & BUTTON_ATTACK ) && ( pm->ps->weaponTime >= 50 ) ) {
			pm->ps->releasedFire = qtrue;
		}
	} else if ( pm->ps->weapon == WP_COLT ) {
		if ( pm->ps->releasedFire ) {
			if ( pm->ps->weaponTime <= 150 && ( pm->cmd.buttons & BUTTON_ATTACK ) ) {
				pm->ps->weaponTime = 0;
			}
		} else if ( !( pm->cmd.buttons & BUTTON_ATTACK ) && ( pm->ps->weaponTime >= 100 ) ) {
			pm->ps->releasedFire = qtrue;
		}
	}
}
// jpw

}


// check for weapon change
// can't change if weapon is firing, but can change
// again if lowering or raising

// TTimo gcc: suggest parentheses around && within ||
if ( pm->ps->weaponTime <= 0 || ( !weaponstateFiring && pm->ps->weaponDelay <= 0 ) ) {
	if ( pm->ps->weapon != pm->cmd.weapon ) {
		PM_BeginWeaponChange( pm->ps->weapon, pm->cmd.weapon, qfalse );     //----(SA)	modified
	}
}

// check for clip change
PM_CheckForReload( pm->ps->weapon );

if ( pm->ps->weaponTime > 0 || pm->ps->weaponDelay > 0 ) {
	return;
}

if ( pm->ps->weaponstate == WEAPON_RELOADING ) {
	PM_FinishWeaponReload();
}

// change weapon if time
if ( pm->ps->weaponstate == WEAPON_DROPPING || pm->ps->weaponstate == WEAPON_DROPPING_TORELOAD ) {
	PM_FinishWeaponChange();
	return;
}

if ( pm->ps->weaponstate == WEAPON_RAISING ) {
	pm->ps->weaponstate = WEAPON_READY;
	PM_StartWeaponAnim( WEAP_IDLE1 );
	return;
} else if ( pm->ps->weaponstate == WEAPON_RAISING_TORELOAD ) {
	pm->ps->weaponstate = WEAPON_READY;     // need to switch to READY so the reload will work
	PM_BeginWeaponReload( pm->ps->weapon );
	return;
}


if ( pm->ps->weapon == WP_NONE ) {  // this is possible since the player starts with nothing
	return;
}


// JPW NERVE -- in multiplayer, don't allow panzerfaust or dynamite to fire if charge bar isn't full
#ifdef GAMEDLL
if ( g_gametype.integer == GT_WOLF ) {
	if ( pm->ps->weapon == WP_PANZERFAUST ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < g_soldierChargeTime.integer ) {
			return;
		}
	}
	if ( pm->ps->weapon == WP_DYNAMITE ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < g_engineerChargeTime.integer ) {        // had to use multiplier because chargetime is used elsewhere as bomb diffuse time FIXME not really but NOTE changing bomb diffuse time will require changing this time as well (intended function: new charge ready when old one explodes, ie every 30 seconds or so)

			return;
		}
	}
}
#endif
#ifdef CGAMEDLL
if ( cg_gameType.integer == GT_WOLF ) {
	if ( pm->ps->weapon == WP_PANZERFAUST ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < cg_soldierChargeTime.integer ) {
			return;
		}
	}
	if ( pm->ps->weapon == WP_DYNAMITE ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < cg_engineerChargeTime.integer ) {
			return;
		}
	}
}
#endif
// jpw

// check for fire
if ( !( pm->cmd.buttons & ( BUTTON_ATTACK | WBUTTON_ATTACK2 ) ) && !delayedFire ) {     // if not on fire button and there's not a delayed shot this frame...
	pm->ps->weaponTime  = 0;
	pm->ps->weaponDelay = 0;

	if ( weaponstateFiring ) {  // you were just firing, time to relax
		PM_ContinueWeaponAnim( WEAP_IDLE1 );
	}

	pm->ps->weaponstate = WEAPON_READY;
	return;
}


if ( gameReloading ) {
	return;
}

// player is zooming - no fire
if ( pm->ps->eFlags & EF_ZOOMING ) {
	return;
}

// player is leaning - no fire
if ( pm->ps->leanf != 0 && pm->ps->weapon != WP_GRENADE_LAUNCHER && pm->ps->weapon != WP_GRENADE_PINEAPPLE && pm->ps->weapon != WP_DYNAMITE ) {
	return;
}

// player is underwater - no fire
if ( pm->waterlevel == 3 ) {
	if ( pm->ps->weapon != WP_KNIFE &&
		 pm->ps->weapon != WP_GRENADE_LAUNCHER &&
		 pm->ps->weapon != WP_GRENADE_PINEAPPLE ) {
		PM_AddEvent( EV_NOFIRE_UNDERWATER );        // event for underwater 'click' for nofire
		pm->ps->weaponTime  = 500;
		return;
	}
}

// start the animation even if out of ammo
switch ( pm->ps->weapon )
{
default:
	if ( !weaponstateFiring ) {
		pm->ps->weaponDelay = ammoTable[pm->ps->weapon].fireDelayTime;          // delay so the weapon can get up into position before firing (and showing the flash)
	} else {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_FIREWEAPON, qfalse, qtrue );
	}
	break;
	// machineguns should continue the anim, rather than start each fire
case WP_MP40:
case WP_THOMPSON:
case WP_STEN:
case WP_VENOM:
case WP_FG42:
case WP_FG42SCOPE:
	if ( !weaponstateFiring ) {
		if ( pm->ps->aiChar && pm->ps->weapon == WP_VENOM ) {
			// AI get fast spin-up
			pm->ps->weaponDelay = 150;
		} else {
			pm->ps->weaponDelay = ammoTable[pm->ps->weapon].fireDelayTime;          // delay so the weapon can get up into position before firing (and showing the flash)
		}
	} else {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_FIREWEAPON, qtrue, qtrue );
	}
	break;
case WP_PANZERFAUST:
case WP_SILENCER:
case WP_LUGER:
case WP_COLT:
case WP_AKIMBO:         //----(SA)	added
case WP_SNIPERRIFLE:
case WP_SNOOPERSCOPE:
case WP_MAUSER:
case WP_GARAND:
	if ( !weaponstateFiring ) {
		// NERVE's panzerfaust spinup
//				if (pm->ps->weapon == WP_PANZERFAUST)
//					PM_AddEvent( EV_SPINUP );
		pm->ps->weaponDelay = ammoTable[pm->ps->weapon].fireDelayTime;
	} else {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_FIREWEAPON, qfalse, qtrue );
	}
	break;
// melee
case WP_KNIFE:
	if ( !delayedFire ) {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_FIREWEAPON, qfalse, qfalse );
	}
	break;
case WP_GAUNTLET:
	if ( !delayedFire ) {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_FIREWEAPON, qfalse, qfalse );
	}
	break;
// throw
case WP_DYNAMITE:
case WP_GRENADE_LAUNCHER:
case WP_GRENADE_PINEAPPLE:
	if ( !delayedFire ) {
		if ( pm->ps->aiChar ) {         // ai characters go into their regular animation setup
			BG_AnimScriptEvent( pm->ps, ANIM_ET_FIREWEAPON, qtrue, qtrue );
		} else {                    // the player pulls the fuse and holds the hot potato
			if ( PM_WeaponAmmoAvailable( pm->ps->weapon ) ) {
				if ( pm->ps->weapon == WP_DYNAMITE ) {
					pm->ps->grenadeTimeLeft = 50;
				} else {
					pm->ps->grenadeTimeLeft = 4000;         // start at four seconds and count down

				}
				PM_StartWeaponAnim( WEAP_ATTACK1 );
			}
		}

		pm->ps->weaponDelay = ammoTable[pm->ps->weapon].fireDelayTime;

	}
	break;
}

pm->ps->weaponstate = WEAPON_FIRING;



// check for out of ammo

ammoNeeded = ammoTable[pm->ps->weapon].uses;

if ( pm->ps->weapon ) {
	int ammoAvailable;
	qboolean reloadingW, playswitchsound = qtrue;

	ammoAvailable = PM_WeaponAmmoAvailable( pm->ps->weapon );

	if ( ammoNeeded > ammoAvailable ) {

		reloadingW = (qboolean)( ammoNeeded <= pm->ps->ammo[ BG_FindAmmoForWeapon( pm->ps->weapon )] ); // you have ammo for this, just not in the clip

		if ( pm->ps->eFlags & EF_MELEE_ACTIVE ) {   // not going to be allowed to reload if holding a chair
			reloadingW = qfalse;
		}

		if ( pm->ps->weapon == WP_SNOOPERSCOPE ) {
			reloadingW = qfalse;
		}

		switch ( pm->ps->weapon ) {
			// Ridah, only play if using a triggered weapon
		case WP_GAUNTLET:
		case WP_MONSTER_ATTACK1:
		case WP_DYNAMITE:
		case WP_GRENADE_LAUNCHER:
		case WP_GRENADE_PINEAPPLE:
			playswitchsound = qfalse;
			break;

			// some weapons not allowed to reload.  must switch back to primary first
		case WP_SNOOPERSCOPE:
		case WP_SNIPERRIFLE:
		case WP_FG42SCOPE:
			reloadingW = qfalse;
			break;
		}

		if ( playswitchsound ) {
			if ( reloadingW ) {
				PM_AddEvent( EV_EMPTYCLIP );
			} else {
				PM_AddEvent( EV_NOAMMO );
			}
		}

		if ( reloadingW ) {
			PM_ContinueWeaponAnim( WEAP_RELOAD1 );      //----(SA)
		} else {
			PM_ContinueWeaponAnim( WEAP_IDLE1 );
			pm->ps->weaponTime += 500;
		}

		return;
	}
}

if ( pm->ps->weaponDelay > 0 ) {
	// if it hits here, the 'fire' has just been hit and the weapon dictated a delay.
	// animations have been started, weaponstate has been set, but no weapon events yet. (except possibly EV_NOAMMO)
	// checks for delayed weapons that have already been fired are return'ed above.
	return;
}


// take an ammo away if not infinite
if ( PM_WeaponAmmoAvailable( pm->ps->weapon ) != -1 ) {
	// Rafael - check for being mounted on mg42
	if ( !( pm->ps->persistant[PERS_HWEAPON_USE] ) ) {
		PM_WeaponUseAmmo( pm->ps->weapon, ammoNeeded );
	}
}


// fire weapon

// add weapon heat
if ( ammoTable[pm->ps->weapon].maxHeat ) {
	pm->ps->weapHeat[pm->ps->weapon] += ammoTable[pm->ps->weapon].nextShotTime;
}

// first person weapon animations

// if this was the last round in the clip, play the 'lastshot' animation
// this animation has the weapon in a "ready to reload" state
if ( pm->ps->weapon == WP_AKIMBO ) {
	if ( akimboFire ) {
		weapattackanim = WEAP_ATTACK1;      // attack1 is right hand
	} else {
		weapattackanim = WEAP_ATTACK2;      // attack2 is left hand
	}
} else {
	if ( PM_WeaponClipEmpty( pm->ps->weapon ) ) {
		weapattackanim = WEAP_ATTACK_LASTSHOT;
	} else {
		weapattackanim = WEAP_ATTACK1;
	}
}

switch ( pm->ps->weapon ) {
case WP_MAUSER:
case WP_GRENADE_LAUNCHER:
case WP_GRENADE_PINEAPPLE:
case WP_DYNAMITE:
	PM_StartWeaponAnim( weapattackanim );
	break;

case WP_VENOM:
case WP_MP40:
case WP_THOMPSON:
case WP_STEN:
	PM_ContinueWeaponAnim( weapattackanim );
	break;

default:
	// RF, testing
//			PM_ContinueWeaponAnim(weapattackanim);
	PM_StartWeaponAnim( weapattackanim );
	break;
}



if ( pm->ps->weapon == WP_AKIMBO ) {
	if ( pm->ps->weapon == WP_AKIMBO && !akimboFire ) {
		PM_AddEvent( EV_FIRE_WEAPONB );     // really firing colt
	} else {
		PM_AddEvent( EV_FIRE_WEAPON );
	}
} else {
	if ( PM_WeaponClipEmpty( pm->ps->weapon ) ) {
		PM_AddEvent( EV_FIRE_WEAPON_LASTSHOT );
	} else {
		PM_AddEvent( EV_FIRE_WEAPON );
	}
}


// RF
pm->ps->releasedFire = qfalse;
pm->ps->lastFireTime = pm->cmd.serverTime;


aimSpreadScaleAdd = 0;

switch ( pm->ps->weapon ) {
case WP_KNIFE:
case WP_DYNAMITE:
case WP_GRENADE_LAUNCHER:
case WP_GRENADE_PINEAPPLE:
case WP_FLAMETHROWER:
	addTime = ammoTable[pm->ps->weapon].nextShotTime;
	break;

case WP_PANZERFAUST:
	addTime = ammoTable[pm->ps->weapon].nextShotTime;
	aimSpreadScaleAdd = 30;
	break;

case WP_LUGER:
	addTime = ammoTable[pm->ps->weapon].nextShotTime;
	aimSpreadScaleAdd = 35;
	break;

case WP_COLT:
	addTime = ammoTable[pm->ps->weapon].nextShotTime;
	aimSpreadScaleAdd = 20;
	break;

//----(SA)	added
case WP_AKIMBO:
	// if you're firing an akimbo colt, and your other gun is dry,
	// nextshot needs to take 2x time

	addTime = ammoTable[pm->ps->weapon].nextShotTime;

	// (SA) (added check for last shot in both guns so there's no delay for the last shot)
	if ( !pm->ps->ammoclip[WP_AKIMBO] || !pm->ps->ammoclip[WP_COLT] ) {
		if ( ( !pm->ps->ammoclip[WP_AKIMBO] && !akimboFire ) || ( !pm->ps->ammoclip[WP_COLT] && akimboFire ) ) {
			addTime = 2 * ammoTable[pm->ps->weapon].nextShotTime;
		}
	}

	aimSpreadScaleAdd = 20;
	break;
//----(SA)	end

case WP_MAUSER:
case WP_GARAND:
	addTime = ammoTable[pm->ps->weapon].nextShotTime;
	aimSpreadScaleAdd = 50;
	break;

case WP_SNIPERRIFLE:        // (SA) not so much added per shot.  these weapons mostly uses player movement to get out of whack
	addTime = ammoTable[pm->ps->weapon].nextShotTime;
// JPW NERVE crippling the rifle a bit in multiplayer; it's way too strong so make it go completely out every time you fire
#ifdef CGAMEDLL
	if ( cg_gameType.integer != GT_SINGLE_PLAYER ) {
#endif
#ifdef GAMEDLL
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
#endif
//			addTime *= 2; // pulled this and reduced rifle damage
	aimSpreadScaleAdd = 100;
} else {
// jpw
	aimSpreadScaleAdd = 20;
}
break;
case WP_SNOOPERSCOPE:
// JPW NERVE crippling the rifle a bit in multiplayer; it's way too strong so make it go completely out every time you fire
// snooper doesn't do one-shot body kills, so give it a little less bounce
addTime = ammoTable[pm->ps->weapon].nextShotTime;
#ifdef CGAMEDLL
if ( cg_gameType.integer != GT_SINGLE_PLAYER ) {
#endif
#ifdef GAMEDLL
if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
#endif
aimSpreadScaleAdd = 50;
//			addTime *= 2;
} else {
// jpw
	aimSpreadScaleAdd = 10;
}
break;

case WP_FG42SCOPE:
addTime = ammoTable[pm->ps->weapon].nextShotTime;
aimSpreadScaleAdd = 10;
break;

case WP_FG42:
case WP_MP40:
case WP_THOMPSON:
addTime = ammoTable[pm->ps->weapon].nextShotTime;
aimSpreadScaleAdd = 15 + rand() % 10;       // (SA) new values for DM
break;

case WP_STEN:
addTime = ammoTable[pm->ps->weapon].nextShotTime;
aimSpreadScaleAdd = 15 + rand() % 10;       // (SA) new values for DM
break;

case WP_SILENCER:
addTime = ammoTable[pm->ps->weapon].nextShotTime;
aimSpreadScaleAdd = 35;
break;

case WP_VENOM:
addTime = ammoTable[pm->ps->weapon].nextShotTime;
aimSpreadScaleAdd = 10;
break;

// JPW NERVE
case WP_CLASS_SPECIAL:
// can't reliably get cvars for restart time in here, so set it small and check it in fireweapon routine
addTime = 50;
break;
// jpw
case WP_MONSTER_ATTACK1:
switch ( pm->ps->aiChar ) {
case AICHAR_ZOMBIE:
	// Zombie spitting blood
	addTime = 1000;
	break;
default:
	break;
}

default:
case WP_GAUNTLET:
switch ( pm->ps->aiChar )
{
case AICHAR_LOPER:              // delay 'til next attack
	addTime = 1000;
	break;
default:
	addTime = 250;
	break;
}
break;
}


// check for overheat

// the weapon can overheat, and it's hot
if ( ( pm->ps->aiChar != AICHAR_PROTOSOLDIER ) &&
	 ( pm->ps->aiChar != AICHAR_SUPERSOLDIER ) &&
	 ( ammoTable[pm->ps->weapon].maxHeat && pm->ps->weapHeat[pm->ps->weapon] ) ) {
	// it is overheating
	if ( pm->ps->weapHeat[pm->ps->weapon] >= ammoTable[pm->ps->weapon].maxHeat ) {
		pm->ps->weapHeat[pm->ps->weapon] = ammoTable[pm->ps->weapon].maxHeat;       // cap heat to max
		PM_AddEvent( EV_WEAP_OVERHEAT );
//			PM_StartWeaponAnim(WEAP_IDLE1);	// removed.  client handles anim in overheat event
		addTime = 2000;         // force "heat recovery minimum" to 2 sec right now
	}
}

if ( pm->ps->powerups[PW_HASTE] ) {
	addTime /= 1.3;
}

// add the recoil amount to the aimSpreadScale
//	pm->ps->aimSpreadScale += 3.0*aimSpreadScaleAdd;
//	if (pm->ps->aimSpreadScale > 255) pm->ps->aimSpreadScale = 255;
pm->ps->aimSpreadScaleFloat += 3.0 * aimSpreadScaleAdd;
if ( pm->ps->aimSpreadScaleFloat > 255 ) {
	pm->ps->aimSpreadScaleFloat = 255;
}
pm->ps->aimSpreadScale = (int)( pm->ps->aimSpreadScaleFloat );

pm->ps->weaponTime += addTime;

PM_SwitchIfEmpty();
}


/*
================
PM_Animate
================
*/
#define MYTIMER_SALUTE   1133   // 17 frames, 15 fps
#define MYTIMER_DISMOUNT 667    // 10 frames, 15 fps

static void PM_Animate( void ) {
/*
	if ( pm->cmd.buttons & BUTTON_GESTURE ) {
		if ( pm->ps->torsoTimer == 0) {
			PM_StartTorsoAnim( BOTH_SALUTE );
			PM_StartLegsAnim( BOTH_SALUTE );

			pm->ps->torsoTimer = MYTIMER_SALUTE;
			pm->ps->legsTimer = MYTIMER_SALUTE;

			if (!pm->ps->aiChar)	// Ridah, we'll play a custom sound upon calling the Taunt
				PM_AddEvent( EV_TAUNT );	// for playing the sound
		}
	}
*/
}


/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void ) {
	// drop misc timing counter
	if ( pm->ps->pm_time ) {
		if ( pml.msec >= pm->ps->pm_time ) {
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		} else {
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if ( pm->ps->legsTimer > 0 ) {
		pm->ps->legsTimer -= pml.msec;
		if ( pm->ps->legsTimer < 0 ) {
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 ) {
		pm->ps->torsoTimer -= pml.msec;
		if ( pm->ps->torsoTimer < 0 ) {
			pm->ps->torsoTimer = 0;
		}
	}

	// first person weapon counter
	if ( pm->ps->weapAnimTimer > 0 ) {
		pm->ps->weapAnimTimer -= pml.msec;
		if ( pm->ps->weapAnimTimer < 0 ) {
			pm->ps->weapAnimTimer = 0;
		}
	}
}



#define LEAN_MAX    28.0f
#define LEAN_TIME_TO    280.0f  // time to get to full lean
#define LEAN_TIME_FR    350.0f  // time to get from full lean

/*
==============
PM_CalcLean

==============
*/
void PM_UpdateLean( playerState_t *ps, usercmd_t *cmd, pmove_t *tpm ) {
	vec3_t start, end, tmins, tmaxs, right;
	int leaning = 0;            // -1 left, 1 right
	float leanofs = 0;
	vec3_t viewangles;
	trace_t trace;

	if ( ps->aiChar ) {
		return;
	}

	if ( ( cmd->wbuttons & ( WBUTTON_LEANLEFT | WBUTTON_LEANRIGHT ) )  && !cmd->forwardmove && cmd->upmove <= 0 ) {
		// if both are pressed, result is no lean
		if ( cmd->wbuttons & WBUTTON_LEANLEFT ) {
			leaning -= 1;
		}
		if ( cmd->wbuttons & WBUTTON_LEANRIGHT ) {
			leaning += 1;
		}
	}

	// not allowed when...
	if ( ( ps->eFlags & ( EF_MELEE_ACTIVE |   // ...holding a chair
						  EF_MG42_ACTIVE  | // ...on mg42
						  EF_FIRING ) ) ) { // ...firing
		leaning = 0;
	}

	leanofs = ps->leanf;

	if ( !leaning ) {  // go back to center position
		if ( leanofs > 0 ) {        // right
			//FIXME: play lean anim backwards?
			leanofs -= ( ( (float)pml.msec / LEAN_TIME_FR ) * LEAN_MAX );
			if ( leanofs < 0 ) {
				leanofs = 0;
			}
		} else if ( leanofs < 0 )   { // left
			//FIXME: play lean anim backwards?
			leanofs += ( ( (float)pml.msec / LEAN_TIME_FR ) * LEAN_MAX );
			if ( leanofs > 0 ) {
				leanofs = 0;
			}
		}
	}

	if ( leaning ) {
		if ( leaning > 0 ) {   // right
			if ( leanofs < LEAN_MAX ) {
				leanofs += ( ( (float)pml.msec / LEAN_TIME_TO ) * LEAN_MAX );
			}

			if ( leanofs > LEAN_MAX ) {
				leanofs = LEAN_MAX;
			}

		} else {              // left
			if ( leanofs > -LEAN_MAX ) {
				leanofs -= ( ( (float)pml.msec / LEAN_TIME_TO ) * LEAN_MAX );
			}

			if ( leanofs < -LEAN_MAX ) {
				leanofs = -LEAN_MAX;
			}

		}
	}

	ps->leanf = leanofs;

	if ( leaning ) {
		VectorCopy( ps->origin, start );
		start[2] += ps->viewheight;
		VectorCopy( ps->viewangles, viewangles );
		viewangles[ROLL] = 0;
		AngleVectors( ps->viewangles, NULL, right, NULL );
		VectorNormalize( right );
		right[2] = ( leanofs < 0 ) ? 0.25 : -0.25;
		VectorMA( start, leanofs, right, end );
//		VectorSet( tmins, -8, -8, -4 );
//		VectorSet( tmaxs, 8, 8, 4 );
		VectorSet( tmins, -12, -12, -6 );
		VectorSet( tmaxs, 12, 12, 10 );

//	(SA) there's a problem with this tpm.trace (crashes)
//		tpm.trace = (void *)&trace;
//		tpm.trace (&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);
		if ( pm ) {
			pm->trace( &trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID );
		} else {
			tpm->trace( &trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID );
		}

		ps->leanf *= trace.fraction;
	}


	if ( ps->leanf ) {
		cmd->rightmove = 0;     // also disallowed in cl_input ~391

	}
}



/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, usercmd_t *cmd, void( trace ) ( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask ) ) {   //----(SA)	modified
	short temp;
	int i;
	pmove_t tpm;

	if ( pm->ps->pm_type == PM_FREEZE ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;     // no view changes at all
	}

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0 ) {
		return;     // no view changes at all
	}

	// circularly clamp the angles with deltas
	for ( i = 0 ; i < 3 ; i++ ) {
		temp = cmd->angles[i] + ps->delta_angles[i];
		if ( i == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) {
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp = 16000;
			} else if ( temp < -16000 ) {
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE( temp );
	}


	tpm.trace = (void *)&trace;
//	tpm.trace (&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);

	PM_UpdateLean( ps, cmd, &tpm );
}

/*
================
PM_CheckLadderMove

  Checks to see if we are on a ladder
================
*/
qboolean ladderforward;
vec3_t laddervec;

void PM_CheckLadderMove( void ) {
	vec3_t spot;
	vec3_t flatforward;
	trace_t trace;
	float tracedist;
	#define TRACE_LADDER_DIST   48.0
	qboolean wasOnLadder;

	if ( pm->ps->pm_time ) {
		return;
	}

	//if (pm->ps->pm_flags & PM_DEAD)
	//	return;

	if ( pml.walking ) {
		tracedist = 1.0;
	} else {
		tracedist = TRACE_LADDER_DIST;
	}

	wasOnLadder = ( ( pm->ps->pm_flags & PMF_LADDER ) != 0 );

	pml.ladder = qfalse;
	pm->ps->pm_flags &= ~PMF_LADDER;    // clear ladder bit
	ladderforward = qfalse;

	/*
	if (pm->ps->eFlags & EF_DEAD) {	// dead bodies should fall down ladders
		return;
	}

	if (pm->ps->pm_flags & PM_DEAD && pm->ps->stats[STAT_HEALTH] <= 0)
	{
		return;
	}
	*/
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize( flatforward );

	VectorMA( pm->ps->origin, tracedist, flatforward, spot );
	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, spot, pm->ps->clientNum, pm->tracemask );
	if ( ( trace.fraction < 1 ) && ( trace.surfaceFlags & SURF_LADDER ) ) {
		pml.ladder = qtrue;
	}
/*
	if (!pml.ladder && DotProduct(pm->ps->velocity, pml.forward) < 0) {
		// trace along the negative velocity, so we grab onto a ladder if we are trying to reverse onto it from above the ladder
		flatforward[0] = -pm->ps->velocity[0];
		flatforward[1] = -pm->ps->velocity[1];
		flatforward[2] = 0;
		VectorNormalize (flatforward);

		VectorMA (pm->ps->origin, tracedist, flatforward, spot);
		pm->trace (&trace, pm->ps->origin, pm->mins, pm->maxs, spot, pm->ps->clientNum, pm->tracemask);
		if ((trace.fraction < 1) && (trace.surfaceFlags & SURF_LADDER))
		{
			pml.ladder = qtrue;
		}
	}
*/
	if ( pml.ladder ) {
		VectorCopy( trace.plane.normal, laddervec );
	}

	if ( pml.ladder && !pml.walking && ( trace.fraction * tracedist > 1.0 ) ) {
		vec3_t mins;
		// if we are only just on the ladder, don't do this yet, or it may throw us back off the ladder
		pml.ladder = qfalse;
		VectorCopy( pm->mins, mins );
		mins[2] = -1;
		VectorMA( pm->ps->origin, -tracedist, laddervec, spot );
		pm->trace( &trace, pm->ps->origin, mins, pm->maxs, spot, pm->ps->clientNum, pm->tracemask );
		if ( ( trace.fraction < 1 ) && ( trace.surfaceFlags & SURF_LADDER ) ) {
			// if AI, then be more stringent on their viewangles
			if ( pm->ps->aiChar && ( DotProduct( trace.plane.normal, pml.forward ) > -0.9 ) ) {
				pml.ladder = qfalse;
			} else {
				ladderforward = qtrue;
				pml.ladder = qtrue;
				pm->ps->pm_flags |= PMF_LADDER; // set ladder bit
			}
		} else {
			pml.ladder = qfalse;
		}
	} else if ( pml.ladder ) {
		pm->ps->pm_flags |= PMF_LADDER; // set ladder bit
	}

	// create some up/down velocity if touching ladder
	if ( pml.ladder ) {
		if ( pml.walking ) {
			// we are currently on the ground, only go up and prevent X/Y if we are pushing forwards
			if ( pm->cmd.forwardmove <= 0 ) {
				pml.ladder = qfalse;
			}
		}
	}

	// if we have just dismounted the ladder at the top, play dismount
	if ( !pml.ladder && wasOnLadder && pm->ps->velocity[2] > 0 ) {
		BG_AnimScriptEvent( pm->ps, ANIM_ET_CLIMB_DISMOUNT, qfalse, qfalse );
	}
	// if we have just mounted the ladder
	if ( pml.ladder && !wasOnLadder && pm->ps->velocity[2] < 0 ) {    // only play anim if going down ladder
		BG_AnimScriptEvent( pm->ps, ANIM_ET_CLIMB_MOUNT, qfalse, qfalse );
	}
}

/*
============
PM_LadderMove
============
*/
void PM_LadderMove( void ) {
	float wishspeed, scale;
	vec3_t wishdir, wishvel;
	float upscale;

	if ( ladderforward ) {
		// move towards the ladder
		VectorScale( laddervec, -200.0, wishvel );
		pm->ps->velocity[0] = wishvel[0];
		pm->ps->velocity[1] = wishvel[1];
	}

	upscale = ( pml.forward[2] + 0.5 ) * 2.5;
	if ( upscale > 1.0 ) {
		upscale = 1.0;
	} else if ( upscale < -1.0 ) {
		upscale = -1.0;
	}

	// forward/right should be horizontal only
	pml.forward[2] = 0;
	pml.right[2] = 0;
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	// move depending on the view, if view is straight forward, then go up
	// if view is down more then X degrees, start going down
	// if they are back pedalling, then go in reverse of above
	scale = PM_CmdScale( &pm->cmd );
	VectorClear( wishvel );

	if ( pm->cmd.forwardmove ) {
		if ( pm->ps->aiChar ) {
			wishvel[2] = 0.5 * upscale * scale * (float)pm->cmd.forwardmove;
		} else { // player speed
			wishvel[2] = 0.9 * upscale * scale * (float)pm->cmd.forwardmove;
		}
	}
//Com_Printf("wishvel[2] = %i, fwdmove = %i\n", (int)wishvel[2], (int)pm->cmd.forwardmove );

	if ( pm->cmd.rightmove ) {
		// strafe, so we can jump off ladder
		vec3_t ladder_right, ang;
		vectoangles( laddervec, ang );
		AngleVectors( ang, NULL, ladder_right, NULL );

		// if we are looking away from the ladder, reverse the right vector
		if ( DotProduct( laddervec, pml.forward ) > 0 ) {
			VectorInverse( ladder_right );
		}

		VectorMA( wishvel, 0.5 * scale * (float)pm->cmd.rightmove, pml.right, wishvel );
	}

	// do strafe friction
	PM_Friction();

	wishspeed = VectorNormalize2( wishvel, wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );
	if ( !wishvel[2] ) {
		if ( pm->ps->velocity[2] > 0 ) {
			pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
			if ( pm->ps->velocity[2] < 0 ) {
				pm->ps->velocity[2]  = 0;
			}
		} else
		{
			pm->ps->velocity[2] += pm->ps->gravity * pml.frametime;
			if ( pm->ps->velocity[2] > 0 ) {
				pm->ps->velocity[2]  = 0;
			}
		}
	}

//Com_Printf("vel[2] = %i\n", (int)pm->ps->velocity[2] );

	PM_StepSlideMove( qfalse );  // no gravity while going up ladder

	// always point legs forward
	pm->ps->movementDir = 0;
}


/*
==============
PM_Sprint
==============
*/
//----(SA)	cleaned up for SP (10/22/01)
void PM_Sprint( void ) {
	if (    ( pm->cmd.buttons & BUTTON_SPRINT ) &&
			( pm->cmd.forwardmove || pm->cmd.rightmove ) &&
			!( pm->ps->pm_flags & PMF_DUCKED ) &&
			( !pm->waterlevel )
			) {

		if ( pm->ps->powerups[PW_NOFATIGUE] ) {    // take time from powerup before taking it from sprintTime
			pm->ps->powerups[PW_NOFATIGUE] -= 50;

			pm->ps->sprintTime += 10;           // (SA) go ahead and continue to recharge stamina at double rate with stamina powerup even when exerting
			if ( pm->ps->sprintTime > 20000 ) {
				pm->ps->sprintTime = 20000;
			}

			if ( pm->ps->powerups[PW_NOFATIGUE] < 0 ) {
				pm->ps->powerups[PW_NOFATIGUE] = 0;
			}
		} else {
			// RF, dont drain sprintTime if not moving
			if ( VectorLength( pm->ps->velocity ) > 128 ) { // (SA) check for a bit more movement
				pm->ps->sprintTime -= 50;
			}
		}

		if ( pm->ps->sprintTime < 0 ) {
			pm->ps->sprintTime = 0;
		}

		if ( !pm->ps->sprintExertTime ) {
			pm->ps->sprintExertTime = 1;
		}
	} else
	{
		// JPW NERVE adjusted for framerate independence

		// regular recharge
		pm->ps->sprintTime += 500 * pml.frametime;

		// additional (2x) recharge if in top 75% of sprint bar, or with stamina powerup
		if ( pm->ps->sprintTime > 5000 || pm->ps->powerups[PW_NOFATIGUE] ) {
			pm->ps->sprintTime += 500 * pml.frametime;
		}

		// additional recharge if standing still
		if ( !( pm->cmd.forwardmove || pm->cmd.rightmove ) ) {
			pm->ps->sprintTime += 500 * pml.frametime;
		}

		if ( pm->ps->sprintTime > 20000 ) {
			pm->ps->sprintTime = 20000;
		}

		pm->ps->sprintExertTime = 0;
	}
}

/*
================
PmoveSingle

================
*/
void trap_SnapVector( float *v );

void PmoveSingle( pmove_t *pmove ) {
	// Ridah
	qboolean isDummy;

	isDummy = ( ( pmove->ps->eFlags & EF_DUMMY_PMOVE ) != 0 );
	// done.

	if ( !isDummy ) {
		// RF, update conditional values for anim system
		BG_AnimUpdatePlayerStateConditions( pmove );
	}

	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		pm->tracemask &= ~CONTENTS_BODY;    // corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( !pm->ps->aiChar ) {
		if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 ) {
			pm->cmd.buttons &= ~BUTTON_WALKING;
		}
	}

	// set the talk balloon flag
	if ( !isDummy ) {
		if ( !pm->ps->aiChar && pm->cmd.buttons & BUTTON_TALK ) {
			pm->ps->eFlags |= EF_TALK;
		} else {
			pm->ps->eFlags &= ~EF_TALK;
		}
	}

	// set the firing flag for continuous beam weapons

	pm->ps->eFlags &= ~( EF_FIRING | EF_ZOOMING );

	if ( pm->cmd.wbuttons & WBUTTON_ZOOM ) {
		if ( pm->ps->stats[STAT_KEYS] & ( 1 << INV_BINOCS ) ) {        // (SA) binoculars are an inventory item (inventory==keys)
			if ( pm->ps->weapon != WP_SNIPERRIFLE && pm->ps->weapon != WP_SNOOPERSCOPE && pm->ps->weapon != WP_FG42SCOPE ) {   // don't allow binocs if using scope
				if ( !( pm->ps->eFlags & EF_MG42_ACTIVE ) ) {    // or if mounted on a weapon
					pm->ps->eFlags |= EF_ZOOMING;
				}
			}

			// don't allow binocs if in the middle of throwing grenade
			if ( ( pm->ps->weapon == WP_GRENADE_LAUNCHER || pm->ps->weapon == WP_GRENADE_PINEAPPLE || pm->ps->weapon == WP_DYNAMITE ) && pm->ps->grenadeTimeLeft > 0 ) {
				pm->ps->eFlags &= ~EF_ZOOMING;
			}
		}
	}


	if (    !( pm->ps->pm_flags & PMF_RESPAWNED ) &&
			( pm->ps->pm_type != PM_INTERMISSION ) ) {
		// check if zooming
		if ( !( pm->cmd.wbuttons & WBUTTON_ZOOM ) ) {
			if ( pm->cmd.buttons & BUTTON_ATTACK ) {
				// check for ammo
				if ( PM_WeaponAmmoAvailable( pm->ps->weapon ) ) {
					// all clear, fire!
					pm->ps->eFlags |= EF_FIRING;
				}
			}
		}
	}


	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[STAT_HEALTH] > 0 &&
		 !( pm->cmd.buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) ) {
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK ) {
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.wbuttons = 0;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset( &pml, 0, sizeof( pml ) );

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if ( !isDummy ) {
		if ( pml.msec < 1 ) {
			pml.msec = 1;
		} else if ( pml.msec > 200 ) {
			pml.msec = 200;
		}
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy( pm->ps->origin, pml.previous_origin );

	// save old velocity for crashlanding
	VectorCopy( pm->ps->velocity, pml.previous_velocity );

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	// Ridah
	if ( !isDummy ) {
		// done.
		if ( !( pm->ps->pm_flags & PMF_LIMBO ) ) { // JPW NERVE
			PM_UpdateViewAngles( pm->ps, &pm->cmd, pm->trace ); //----(SA)	modified

		}
	}
	AngleVectors( pm->ps->viewangles, pml.forward, pml.right, pml.up );

	if ( pm->cmd.upmove < 10 ) {
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 ) {
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	} else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) ) {
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( pm->ps->pm_type >= PM_DEAD || pm->ps->pm_flags & PMF_LIMBO ) {         // DHM - Nerve
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( ( pm->ps->pm_type == PM_SPECTATOR ) || ( pm->ps->pm_flags & PMF_LIMBO ) ) { // JPW NERVE for limbo mode chase
		PM_CheckDuck();
		PM_FlyMove();
		PM_DropTimers();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP ) {
		PM_NoclipMove();
		PM_DropTimers();
		return;
	}

	if ( pm->ps->pm_type == PM_FREEZE ) {
		return;     // no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION ) {
		return;     // no movement at all
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck();

	// set groundentity
	PM_GroundTrace();

	if ( pm->ps->pm_type == PM_DEAD ) {
		PM_DeadMove();
	}

	// Ridah, ladders
	PM_CheckLadderMove();

	if ( !isDummy ) {
		PM_DropTimers();
	}

	if ( pm->ps->powerups[PW_FLIGHT] ) {
		// flight powerup doesn't allow jump and has different friction
		PM_FlyMove();
// RF, removed grapple flag since it's not used
#if 0
	} else if ( pm->ps->pm_flags & PMF_GRAPPLE_PULL ) {
		PM_GrappleMove();
		// We can wiggle a bit
		PM_AirMove();
#endif
		// Ridah, ladders
	} else if ( pml.ladder ) {
		PM_LadderMove();
		// done.
	} else if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP ) {
		PM_WaterJumpMove();
	} else if ( pm->waterlevel > 1 ) {
		// swimming
		PM_WaterMove();
	} else if ( pml.walking ) {
		// walking on ground
		PM_WalkMove();
	} else {
		// airborne
		PM_AirMove();
	}


	PM_Sprint();


	// Ridah
	if ( !isDummy ) {
		// done.
		PM_Animate();
	}

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// Ridah
	if ( !isDummy ) {
		// done.

		// weapons
		PM_Weapon();

		// footstep events / legs animations
		PM_Footsteps();

		// entering / leaving water splashes
		PM_WaterEvents();

		// snap some parts of playerstate to save network bandwidth
		trap_SnapVector( pm->ps->velocity );
//		SnapVector( pm->ps->velocity );

		// Ridah
	}
	// done.
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
int Pmove( pmove_t *pmove ) {
	int finalTime;

	// Ridah
	if ( pmove->ps->eFlags & EF_DUMMY_PMOVE ) {
		PmoveSingle( pmove );
		return ( 0 );
	} else if ( pmove->ps->pm_flags & PMF_IGNORE_INPUT ) {
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
		pmove->cmd.buttons = 0;
		pmove->cmd.wbuttons = 0;
		pmove->cmd.wolfkick = 0;
	}
	// done.

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime ) {
		return ( 0 ); // should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 ) {
		pmove->ps->commandTime = finalTime - 1000;
	}

	// RF, after a loadgame, prevent huge pmove's
	if ( pmove->ps->pm_flags & PMF_TIME_LOAD ) {
		if ( !pmove->ps->aiChar ) {
			if ( finalTime - pmove->ps->commandTime > 50 ) {
				pmove->ps->commandTime = finalTime - 50;
			}
		} else {
			if ( finalTime - pmove->ps->commandTime > 50 ) {
				pmove->ps->commandTime = finalTime - 50;
			}
		}
	}

	pmove->ps->pmove_framecount = ( pmove->ps->pmove_framecount + 1 ) & ( ( 1 << PS_PMOVEFRAMECOUNTBITS ) - 1 );

	// RF
	pm = pmove;
	PM_AdjustAimSpreadScale();

//	startedTorsoAnim = -1;
//	startedLegAnim = -1;

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime ) {
		int msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed ) {
			if ( msec > pmove->pmove_msec ) {
				msec = pmove->pmove_msec;
			}
		} else {
			if ( msec > 66 ) {
				msec = 66;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD ) {
			pmove->cmd.upmove = 20;
		}
	}

	//PM_CheckStuck();

	if ( ( pm->ps->stats[STAT_HEALTH] <= 0 || pm->ps->pm_type == PM_DEAD ) && pml.groundTrace.surfaceFlags & SURF_MONSTERSLICK ) {
		return ( pml.groundTrace.surfaceFlags );
	} else {
		return ( 0 );
	}

}
