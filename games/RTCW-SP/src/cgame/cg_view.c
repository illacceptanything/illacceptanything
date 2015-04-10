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

// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"

//========================
extern int notebookModel;
//========================

/*
=============================================================================

  MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f( void ) {
	vec3_t angles;

	memset( &cg.testModelEntity, 0, sizeof( cg.testModelEntity ) );
	if ( trap_Argc() < 2 ) {
		return;
	}

	Q_strncpyz( cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );

	if ( trap_Argc() == 3 ) {
		cg.testModelEntity.backlerp = atof( CG_Argv( 2 ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if ( !cg.testModelEntity.hModel ) {
		CG_Printf( "Can't register model\n" );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin );

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis( angles, cg.testModelEntity.axis );
	cg.testGun = qfalse;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f( void ) {
	CG_TestModel_f();
	cg.testGun = qtrue;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f( void ) {
	cg.testModelEntity.frame++;
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f( void ) {
	cg.testModelEntity.frame--;
	if ( cg.testModelEntity.frame < 0 ) {
		cg.testModelEntity.frame = 0;
	}
	CG_Printf( "frame %i\n", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f( void ) {
	cg.testModelEntity.skinNum++;
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f( void ) {
	cg.testModelEntity.skinNum--;
	if ( cg.testModelEntity.skinNum < 0 ) {
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf( "skin %i\n", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel( void ) {
	int i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );
	if ( !cg.testModelEntity.hModel ) {
		CG_Printf( "Can't register model\n" );
		return;
	}

	// if testing a gun, set the origin reletive to the view origin
	if ( cg.testGun ) {
		VectorCopy( cg.refdef.vieworg, cg.testModelEntity.origin );
		VectorCopy( cg.refdef.viewaxis[0], cg.testModelEntity.axis[0] );
		VectorCopy( cg.refdef.viewaxis[1], cg.testModelEntity.axis[1] );
		VectorCopy( cg.refdef.viewaxis[2], cg.testModelEntity.axis[2] );

		// allow the position to be adjusted
		for ( i = 0 ; i < 3 ; i++ ) {
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	trap_R_AddRefEntityToScene( &cg.testModelEntity );
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
// TTimo: unused
//static float letterbox_frac = 1.0f;	// used for transitioning to letterbox for cutscenes // TODO: add to cg.

static void CG_CalcVrect( void ) {
	int xsize, ysize;
	float lbheight, lbdiff;

	// NERVE - SMF
	if ( cg.limboMenu ) {
		float x, y, w, h;
		x = LIMBO_3D_X;
		y = LIMBO_3D_Y;
		w = LIMBO_3D_W;
		h = LIMBO_3D_H;

		cg.refdef.width = 0;
		CG_AdjustFrom640( &x, &y, &w, &h );

		cg.refdef.x = x;
		cg.refdef.y = y;
		cg.refdef.width = w;
		cg.refdef.height = h;
		return;
	}
	// -NERVE - SMF

	// the intermission should allways be full screen
	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		xsize = ysize = 100;
	} else {
		// bound normal viewsize
		if ( cg_viewsize.integer < 30 ) {
			trap_Cvar_Set( "cg_viewsize","30" );
			xsize = ysize = 30;
		} else if ( cg_viewsize.integer > 100 ) {
			trap_Cvar_Set( "cg_viewsize","100" );
			xsize = ysize = 100;
		} else {
			xsize = ysize = cg_viewsize.integer;
		}
	}

//----(SA)	added transition to/from letterbox
// normal aspect is xx:xx
// letterbox is yy:yy  (85% of 'normal' height)

	lbheight = ysize * 0.85;
	lbdiff = ysize - lbheight;

	if ( cg_letterbox.integer ) {
		ysize = lbheight;
//		if(letterbox_frac != 0) {
//			letterbox_frac -= 0.01f;	// (SA) TODO: make non fps dependant
//			if(letterbox_frac < 0)
//				letterbox_frac = 0;
//			ysize += (lbdiff * letterbox_frac);
//		}
//	} else {
//		if(letterbox_frac != 1) {
//			letterbox_frac += 0.01f;	// (SA) TODO: make non fps dependant
//			if(letterbox_frac > 1)
//				letterbox_frac = 1;
//			ysize = lbheight + (lbdiff * letterbox_frac);
//		}
	}
//----(SA)	end


	cg.refdef.width = cgs.glconfig.vidWidth * xsize / 100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight * ysize / 100;
	cg.refdef.height &= ~1;

	cg.refdef.x = ( cgs.glconfig.vidWidth - cg.refdef.width ) / 2;
	cg.refdef.y = ( cgs.glconfig.vidHeight - cg.refdef.height ) / 2;
}

//==============================================================================


/*
===============
CG_OffsetThirdPersonView

===============
*/
#define FOCUS_DISTANCE  512
static void CG_OffsetThirdPersonView( void ) {
	vec3_t forward, right, up;
	vec3_t view;
	vec3_t focusAngles;
	trace_t trace;
	static vec3_t mins = { -4, -4, -4 };
	static vec3_t maxs = { 4, 4, 4 };
	vec3_t focusPoint;
	float focusDist;
	float forwardScale, sideScale;

	cg.refdef.vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy( cg.refdefViewAngles, focusAngles );

	// if dead, look at killer
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		focusAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
		cg.refdefViewAngles[YAW] = cg.predictedPlayerState.stats[STAT_DEAD_YAW];
	}

	if ( focusAngles[PITCH] > 45 ) {
		focusAngles[PITCH] = 45;        // don't go too far overhead
	}
	AngleVectors( focusAngles, forward, NULL, NULL );

	VectorMA( cg.refdef.vieworg, FOCUS_DISTANCE, forward, focusPoint );

	VectorCopy( cg.refdef.vieworg, view );

	view[2] += 8;

	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors( cg.refdefViewAngles, forward, right, up );

	forwardScale = cos( cg_thirdPersonAngle.value / 180 * M_PI );
	sideScale = sin( cg_thirdPersonAngle.value / 180 * M_PI );
	VectorMA( view, -cg_thirdPersonRange.value * forwardScale, forward, view );
	VectorMA( view, -cg_thirdPersonRange.value * sideScale, right, view );

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID );

	if ( trace.fraction != 1.0 ) {
		VectorCopy( trace.endpos, view );
		view[2] += ( 1.0 - trace.fraction ) * 32;
		// try another trace to this position, because a tunnel may have the ceiling
		// close enogh that this is poking out

		CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID );
		VectorCopy( trace.endpos, view );
	}


	VectorCopy( view, cg.refdef.vieworg );

	// select pitch to look at focus point from vieword
	VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
	focusDist = sqrt( focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1] );
	if ( focusDist < 1 ) {
		focusDist = 1;  // should never happen
	}
	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2( focusPoint[2], focusDist );
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;
}


// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset( void ) {
	int timeDelta;

	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	// Ridah
	if ( timeDelta < 0 ) {
		cg.stepTime = cg.time;
	}
	if ( timeDelta < STEP_TIME ) {
		cg.refdef.vieworg[2] -= cg.stepChange
								* ( STEP_TIME - timeDelta ) / STEP_TIME;
	}
}

/*
================
CG_KickAngles
================
*/
void CG_KickAngles( void ) {
	const vec3_t centerSpeed = {2400, 2400, 2400};
	const float recoilCenterSpeed = 200;
	const float recoilIgnoreCutoff = 15;
	const float recoilMaxSpeed = 50;
	const vec3_t maxKickAngles = {10,10,10};
	float idealCenterSpeed, kickChange;
	int i, frametime, t;
	float ft;
	#define STEP 20

	// this code is frametime-dependant, so split it up into small chunks
	//cg.kickAngles[PITCH] = 0;
	cg.recoilPitchAngle = 0;
	for ( t = cg.frametime; t > 0; t -= STEP ) {
		if ( t > STEP ) {
			frametime = STEP;
		} else {
			frametime = t;
		}

		ft = ( (float)frametime / 1000 );

		// kickAngles is spring-centered
		for ( i = 0; i < 3; i++ ) {
			if ( cg.kickAVel[i] || cg.kickAngles[i] ) {
				// apply centering forces to kickAvel
				if ( cg.kickAngles[i] && frametime ) {
					idealCenterSpeed = -( 2.0 * ( cg.kickAngles[i] > 0 ) - 1.0 ) * centerSpeed[i];
					if ( idealCenterSpeed ) {
						cg.kickAVel[i] += idealCenterSpeed * ft;
					}
				}
				// add the kickAVel to the kickAngles
				kickChange = cg.kickAVel[i] * ft;
				if ( cg.kickAngles[i] && ( cg.kickAngles[i] < 0 ) != ( kickChange < 0 ) ) { // slower when returning to center
					kickChange *= 0.06;
				}
				// check for crossing back over the center point
				if ( !cg.kickAngles[i] || ( ( cg.kickAngles[i] + kickChange ) < 0 ) == ( cg.kickAngles[i] < 0 ) ) {
					cg.kickAngles[i] += kickChange;
					if ( !cg.kickAngles[i] && frametime ) {
						cg.kickAVel[i] = 0;
					} else if ( fabs( cg.kickAngles[i] ) > maxKickAngles[i] ) {
						cg.kickAngles[i] = maxKickAngles[i] * ( ( 2 * ( cg.kickAngles[i] > 0 ) ) - 1 );
						cg.kickAVel[i] = 0; // force Avel to return us to center rather than keep going outside range
					}
				} else { // about to cross, so just zero it out
					cg.kickAngles[i] = 0;
					cg.kickAVel[i] = 0;
				}
			}
		}

		// recoil is added to input viewangles per frame
		if ( cg.recoilPitch ) {
			// apply max recoil
			if ( fabs( cg.recoilPitch ) > recoilMaxSpeed ) {
				if ( cg.recoilPitch > 0 ) {
					cg.recoilPitch = recoilMaxSpeed;
				} else {
					cg.recoilPitch = -recoilMaxSpeed;
				}
			}
			// apply centering forces to kickAvel
			if ( frametime ) {
				idealCenterSpeed = -( 2.0 * ( cg.recoilPitch > 0 ) - 1.0 ) * recoilCenterSpeed * ft;
				if ( idealCenterSpeed ) {
					if ( fabs( idealCenterSpeed ) < fabs( cg.recoilPitch ) ) {
						cg.recoilPitch += idealCenterSpeed;
					} else {    // back zero out
						cg.recoilPitch = 0;
					}
				}
			}
		}
		if ( fabs( cg.recoilPitch ) > recoilIgnoreCutoff ) {
			cg.recoilPitchAngle += cg.recoilPitch * ft;
		}
	}
	// encode the kick angles into a 24bit number, for sending to the client exe
//----(SA)	commented out since it doesn't appear to be used, and it spams the console when in "developer 1"
//	trap_Cvar_Set( "cg_recoilPitch", va("%f", cg.recoilPitchAngle) );
}


/*
CG_Concussive
*/
void CG_Concussive( centity_t *cent ) {
	float length;
//	vec3_t	dir, forward;
	vec3_t vec;
//	float	dot;

	//
	float pitchRecoilAdd, pitchAdd;
	float yawRandom;
	vec3_t recoil;
	//

	if ( !cg.renderingThirdPerson && cent->currentState.density == cg.snap->ps.clientNum ) {
		//
		pitchRecoilAdd = 0;
		pitchAdd = 0;
		yawRandom = 0;
		//

		VectorSubtract( cg.snap->ps.origin, cent->currentState.origin, vec );
		length = VectorLength( vec );

		// pitchAdd = 12+rand()%3;
		// yawRandom = 6;

		if ( length > 1024 ) {
			return;
		}

		pitchAdd = ( 32 / length ) * 64;
		yawRandom = ( 32 / length ) * 64;

		// recoil[YAW] = crandom()*yawRandom;
		if ( rand() % 100 > 50 ) {
			recoil[YAW] = -yawRandom;
		} else {
			recoil[YAW] = yawRandom;
		}

		recoil[ROLL] = -recoil[YAW];    // why not
		recoil[PITCH] = -pitchAdd;
		// scale it up a bit (easier to modify this while tweaking)
		VectorScale( recoil, 30, recoil );
		// set the recoil
		VectorCopy( recoil, cg.kickAVel );
		// set the recoil
		cg.recoilPitch -= pitchRecoilAdd;

	}
}


/*
==============
CG_ZoomSway
	sway for scoped weapons.
	this takes aimspread into account so the view settles after a bit
==============
*/
static void CG_ZoomSway( void ) {
	float spreadfrac;
	float phase;

	if ( !cg.zoomval ) { // not zoomed
		return;
	}

	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) { // don't draw when on mg_42
		return;
	}

	spreadfrac = (float)cg.snap->ps.aimSpreadScale / 255.0;

	phase = cg.time / 1000.0 * ZOOM_PITCH_FREQUENCY * M_PI * 2;
	cg.refdefViewAngles[PITCH] += ZOOM_PITCH_AMPLITUDE * sin( phase ) * ( spreadfrac + ZOOM_PITCH_MIN_AMPLITUDE );

	phase = cg.time / 1000.0 * ZOOM_YAW_FREQUENCY * M_PI * 2;
	cg.refdefViewAngles[YAW] += ZOOM_YAW_AMPLITUDE * sin( phase ) * ( spreadfrac + ZOOM_YAW_MIN_AMPLITUDE );

}



/*
===============
CG_OffsetFirstPersonView

===============
*/
static void CG_OffsetFirstPersonView( void ) {
	float           *origin;
	float           *angles;
	float bob;
	float ratio;
	float delta;
	float speed;
	float f;
	vec3_t predictedVelocity;
	int timeDelta;

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		return;
	}

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	// if dead, fix the angle and don't add any kick
	if ( cg.snap->ps.stats[STAT_HEALTH] <= 0 ) {
		angles[ROLL] = 40;
		angles[PITCH] = -15;
		angles[YAW] = cg.snap->ps.stats[STAT_DEAD_YAW];
		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}

	// add angles based on weapon kick
	VectorAdd( angles, cg.kick_angles, angles );

	// RF, add new weapon kick angles
	CG_KickAngles();
	VectorAdd( angles, cg.kickAngles, angles );
	// RF, pitch is already added
	//angles[0] -= cg.kickAngles[PITCH];

	// add angles based on damage kick
	if ( cg.damageTime ) {
		ratio = cg.time - cg.damageTime;
		if ( ratio < DAMAGE_DEFLECT_TIME ) {
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		} else {
			ratio = 1.0 - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;
			if ( ratio > 0 ) {
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL] += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime ) / FALL_TIME;
	if ( ratio < 0 ) {
		ratio = 0;
	}
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy( cg.predictedPlayerState.velocity, predictedVelocity );

	delta = DotProduct( predictedVelocity, cg.refdef.viewaxis[0] );
	angles[PITCH] += delta * cg_runpitch.value;

	delta = DotProduct( predictedVelocity, cg.refdef.viewaxis[1] );
	angles[ROLL] -= delta * cg_runroll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	delta = cg.bobfracsin * cg_bobpitch.value * speed;
	if ( cg.predictedPlayerState.pm_flags & PMF_DUCKED ) {
		delta *= 3;     // crouching
	}
	angles[PITCH] += delta;
	delta = cg.bobfracsin * cg_bobroll.value * speed;
	if ( cg.predictedPlayerState.pm_flags & PMF_DUCKED ) {
		delta *= 3;     // crouching accentuates roll
	}
	if ( cg.bobcycle & 1 ) {
		delta = -delta;
	}
	angles[ROLL] += delta;

//===================================

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if ( timeDelta < 0 ) { // Ridah
		cg.duckTime = cg.time - DUCK_TIME;
	}
	if ( timeDelta < DUCK_TIME ) {
		cg.refdef.vieworg[2] -= cg.duckChange
								* ( DUCK_TIME - timeDelta ) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if ( bob > 6 ) {
		bob = 6;
	}

	origin[2] += bob;


	// add fall height
	delta = cg.time - cg.landTime;
	if ( delta < 0 ) { // Ridah
		cg.landTime = cg.time - ( LAND_DEFLECT_TIME + LAND_RETURN_TIME );
	}
	if ( delta < LAND_DEFLECT_TIME ) {
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[2] += cg.landChange * f;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		cg.refdef.vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	CG_ZoomSway();

	// adjust for 'lean'
	if ( cg.predictedPlayerState.leanf != 0 ) {
		//add leaning offset
		vec3_t right;
		cg.refdefViewAngles[2] += cg.predictedPlayerState.leanf / 2.0f;
		AngleVectors( cg.refdefViewAngles, NULL, right, NULL );
		VectorMA( cg.refdef.vieworg, cg.predictedPlayerState.leanf, right, cg.refdef.vieworg );
	}

	// add kick offset

	VectorAdd( origin, cg.kick_origin, origin );

	// pivot the eye based on a neck length
#if 0
	{
#define NECK_LENGTH     8
		vec3_t forward, up;

		cg.refdef.vieworg[2] -= NECK_LENGTH;
		AngleVectors( cg.refdefViewAngles, forward, NULL, up );
		VectorMA( cg.refdef.vieworg, 3, forward, cg.refdef.vieworg );
		VectorMA( cg.refdef.vieworg, NECK_LENGTH, up, cg.refdef.vieworg );
	}
#endif
}

//======================================================================

//
// Zoom controls
//


// probably move to server variables
float zoomTable[ZOOM_MAX_ZOOMS][2] = {
// max {out,in}
	{0, 0},

	{36, 8},    //	binoc
	{20, 4},    //	sniper
	{60, 20},   //	snooper
	{55, 55},   //	fg42
	{55, 55}    //	mg42
};

void CG_AdjustZoomVal( float val, int type ) {
	cg.zoomval += val;
	if ( cg.zoomval > zoomTable[type][ZOOM_OUT] ) {
		cg.zoomval = zoomTable[type][ZOOM_OUT];
	}
	if ( cg.zoomval < zoomTable[type][ZOOM_IN] ) {
		cg.zoomval = zoomTable[type][ZOOM_IN];
	}
}

void CG_ZoomIn_f( void ) {
	if ( cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_SNIPERRIFLE ) {
		CG_AdjustZoomVal( -( cg_zoomStepSniper.value ), ZOOM_SNIPER );
	} else if ( cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_SNOOPERSCOPE )      {
		CG_AdjustZoomVal( -( cg_zoomStepSnooper.value ), ZOOM_SNOOPER );
	} else if ( cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_FG42SCOPE )      {
		CG_AdjustZoomVal( -( cg_zoomStepSnooper.value ), ZOOM_FG42SCOPE );
	} else if ( cg.zoomedBinoc )      {
		CG_AdjustZoomVal( -( cg_zoomStepBinoc.value ), ZOOM_BINOC );
	}
}

void CG_ZoomOut_f( void ) {
	if ( cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_SNIPERRIFLE ) {
		CG_AdjustZoomVal( cg_zoomStepSniper.value, ZOOM_SNIPER );
	} else if ( cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_SNOOPERSCOPE )      {
		CG_AdjustZoomVal( cg_zoomStepSnooper.value, ZOOM_SNOOPER );
	} else if ( cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_FG42SCOPE )      {
		CG_AdjustZoomVal( cg_zoomStepSnooper.value, ZOOM_FG42SCOPE );
	} else if ( cg.zoomedBinoc )      {
		CG_AdjustZoomVal( cg_zoomStepBinoc.value, ZOOM_BINOC );
	}
}


/*
==============
CG_Zoom
==============
*/
void CG_Zoom( void ) {
	if ( cg.predictedPlayerState.eFlags & EF_ZOOMING ) {
		if ( cg.zoomedBinoc ) {
			return;
		}
		cg.zoomedBinoc  = qtrue;
		cg.zoomTime = cg.time;
		cg.zoomval = cg_zoomDefaultBinoc.value;
	} else {
		if ( !cg.zoomedBinoc ) {
			return;
		}
		cg.zoomedBinoc  = qfalse;
		cg.zoomTime = cg.time;

		// check for scope wepon in use, and switch to if necessary
		if ( cg.predictedPlayerState.weapon == WP_SNOOPERSCOPE ) {
			cg.zoomval = cg_zoomDefaultSnooper.value;
		} else if ( cg.predictedPlayerState.weapon == WP_SNIPERRIFLE ) {
			cg.zoomval = cg_zoomDefaultSniper.value;
		} else if ( cg.predictedPlayerState.weapon == WP_FG42SCOPE ) {
			cg.zoomval = cg_zoomDefaultFG.value;
		} else {
			cg.zoomval = 0;
		}
	}
}


/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
#define WAVE_AMPLITUDE  1
#define WAVE_FREQUENCY  0.4

static int CG_CalcFov( void ) {
	static float lastfov = 90;      // for transitions back from zoomed in modes
	float x;
	float phase;
	float v;
	int contents;
	float fov_x, fov_y;
	float zoomFov;
	float f;
	int inwater;
	qboolean dead;

	CG_Zoom();

	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		dead = qtrue;
		cg.zoomedBinoc = qfalse;
		cg.zoomTime = 0;
		cg.zoomval = 0;
	} else {
		dead = qfalse;
	}

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		// if in intermission, use a fixed value
		fov_x = 90;
	} else {
		// user selectable
		if ( cgs.dmflags & DF_FIXED_FOV ) {
			// dmflag to prevent wide fov for all clients
			fov_x = 90;
		} else {
			fov_x = cg_fov.value;
			if ( fov_x < 1 ) {
				fov_x = 1;
			} else if ( fov_x > 160 ) {
				fov_x = 160;
			}
		}

		// account for zooms
		if ( cg.zoomval ) {
			zoomFov = cg.zoomval;   // (SA) use user scrolled amount

			if ( zoomFov < 1 ) {
				zoomFov = 1;
			} else if ( zoomFov > 160 ) {
				zoomFov = 160;
			}
		} else {
			zoomFov = lastfov;
		}

		// do smooth transitions for the binocs
		if ( cg.zoomedBinoc ) {        // binoc zooming in
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_TIME;
			if ( f > 1.0 ) {
				fov_x = zoomFov;
			} else {
				fov_x = fov_x + f * ( zoomFov - fov_x );
			}
			lastfov = fov_x;
		} else if ( cg.zoomval ) {    // zoomed by sniper/snooper
			fov_x = cg.zoomval;
			lastfov = fov_x;
		} else {                    // binoc zooming out
			f = ( cg.time - cg.zoomTime ) / (float)ZOOM_TIME;
			if ( f > 1.0 ) {
				fov_x = fov_x;
			} else {
				fov_x = zoomFov + f * ( fov_x - zoomFov );
			}
		}
	}

	// DHM - Nerve :: zoom in for Limbo or Spectator
	if ( cgs.gametype == GT_WOLF ) {
		if ( cg.snap->ps.pm_flags & PMF_FOLLOW && cg.snap->ps.weapon == WP_SNIPERRIFLE ) {
			fov_x = cg_zoomDefaultSniper.value;
		}
	}
	// dhm - end

	if ( !dead && ( cg.weaponSelect == WP_SNOOPERSCOPE ) ) {
		cg.refdef.rdflags |= RDF_SNOOPERVIEW;
	} else {
		cg.refdef.rdflags &= ~RDF_SNOOPERVIEW;
	}

	if ( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
		fov_x = 55;
	}

	x = cg.refdef.width / tan( fov_x / 360 * M_PI );
	fov_y = atan2( cg.refdef.height, x );
	fov_y = fov_y * 360 / M_PI;

	// warp if underwater
	contents = CG_PointContents( cg.refdef.vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin( phase );
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
		cg.refdef.rdflags |= RDF_UNDERWATER;
	} else {
		cg.refdef.rdflags &= ~RDF_UNDERWATER;
		inwater = qfalse;
	}

	contents = CG_PointContents( cg.refdef.vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		cg.refdef.rdflags |= RDF_UNDERWATER;
	} else {
		cg.refdef.rdflags &= ~RDF_UNDERWATER;
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if ( !cg.zoomedBinoc ) {
		// NERVE - SMF - fix for zoomed in/out movement bug
		if ( cg.zoomval ) {
			if ( cg.snap->ps.weapon == WP_SNOOPERSCOPE ) {
				cg.zoomSensitivity = 0.3f * ( cg.zoomval / 90.f );  // NERVE - SMF - changed to get less sensitive as you zoom in;
			}
//				cg.zoomSensitivity = 0.2;
			else {
				cg.zoomSensitivity = 0.6 * ( cg.zoomval / 90.f );   // NERVE - SMF - changed to get less sensitive as you zoom in
			}
//				cg.zoomSensitivity = 0.1;
		} else {
			cg.zoomSensitivity = 1;
		}
		// -NERVE - SMF
	} else {
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0;
	}

	return inwater;
}


/*
==============
CG_UnderwaterSounds
==============
*/
#define UNDERWATER_BIT 8
static void CG_UnderwaterSounds( void ) {
//	trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.underWaterSound, 255 );
	trap_S_AddLoopingSound( cg.snap->ps.clientNum, cg.snap->ps.origin, vec3_origin, cgs.media.underWaterSound, 255 & ( 1 << 8 ) );
}


/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob( void ) {
	int t,i;
	int maxTime;
	refEntity_t ent;
	qboolean pointDamage;
	viewDamage_t *vd;
	float redFlash;

	// ragePro systems can't fade blends, so don't obscure the screen
	if ( cgs.glconfig.hardwareType == GLHW_RAGEPRO ) {
		return;
	}

	redFlash = 0;

	for ( i = 0; i < MAX_VIEWDAMAGE; i++ ) {

		vd = &cg.viewDamage[i];

		if ( !vd->damageValue ) {
			continue;
		}

		maxTime = vd->damageDuration;
		t = cg.time - vd->damageTime;
		if ( t <= 0 || t >= maxTime ) {
			vd->damageValue = 0;
			continue;
		}

		pointDamage = !( !vd->damageX && !vd->damageY );

		// if not point Damage, only do flash blend
		if ( !pointDamage ) {
			redFlash += 10.0 * ( 1.0 - (float)t / maxTime );
			continue;
		}

		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		ent.renderfx = RF_FIRST_PERSON;

		VectorMA( cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin );
		VectorMA( ent.origin, vd->damageX * -8, cg.refdef.viewaxis[1], ent.origin );
		VectorMA( ent.origin, vd->damageY * 8, cg.refdef.viewaxis[2], ent.origin );

		ent.radius = vd->damageValue * 0.4 * ( 0.5 + 0.5 * (float)t / maxTime ) * ( 0.75 + 0.5 * fabs( sin( vd->damageTime ) ) );

		ent.customShader = cgs.media.viewBloodAni[(int)( floor( ( (float)t / maxTime ) * 4.9 ) )]; //cgs.media.viewBloodShader;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		trap_R_AddRefEntityToScene( &ent );

		redFlash += ent.radius;
	}

	/* moved over to cg_draw.c
	if (cg.v_dmg_time > cg.time) {
		redFlash = fabs(cg.v_dmg_pitch * ((cg.v_dmg_time - cg.time) / DAMAGE_TIME));

		// blend the entire screen red
		if (redFlash > 5)
			redFlash = 5;

		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		ent.renderfx = RF_FIRST_PERSON;

		VectorMA( cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin );
		ent.radius = 80;	// occupy entire screen
		ent.customShader = cgs.media.viewFlashBlood;
		ent.shaderRGBA[3] = (int)(180.0 * redFlash/5.0);

		trap_R_AddRefEntityToScene( &ent );
	}
	*/
}

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
static int CG_CalcViewValues( void ) {
	playerState_t   *ps;
	static vec3_t oldOrigin = {0,0,0};

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// strings for in game rendering
	// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef.text[0]) );
	// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef.text[1]) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;

	if ( cg.cameraMode ) {
		vec3_t origin, angles;
		float fov = 90;
		float x;

		if ( trap_getCameraInfo( CAM_PRIMARY, cg.time, &origin, &angles, &fov ) ) {
			VectorCopy( origin, cg.refdef.vieworg );
			angles[ROLL] = 0;
			angles[PITCH] = -angles[PITCH];     // (SA) compensate for reversed pitch (this makes the game match the editor, however I'm guessing the real fix is to be done there)
			VectorCopy( angles, cg.refdefViewAngles );
			AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

			x = cg.refdef.width / tan( fov / 360 * M_PI );
			cg.refdef.fov_y = atan2( cg.refdef.height, x );
			cg.refdef.fov_y = cg.refdef.fov_y * 360 / M_PI;
			cg.refdef.fov_x = fov;

			// RF, had to disable, sometimes a loadgame to a camera in the same position
			// can cause the game to not know where the camera is, therefore snapshots
			// dont show the correct entities
			//if(VectorCompare(origin, oldOrigin))
			//	return 0;

			VectorCopy( origin, oldOrigin );
			trap_SendClientCommand( va( "setCameraOrigin %f %f %f", origin[0], origin[1], origin[2] ) );
			return 0;

		} else {
			cg.cameraMode = qfalse;                 // camera off in cgame
			trap_Cvar_Set( "cg_letterbox", "0" );
			trap_SendClientCommand( "stopCamera" );    // camera off in game
			trap_stopCamera( CAM_PRIMARY );           // camera off in client

			CG_Fade( 0, 0, 0, 255, 0, 0 );                // go black
			CG_Fade( 0, 0, 0, 0, cg.time + 200, 1500 );   // then fadeup
		}
	}

	// intermission view
	if ( ps->pm_type == PM_INTERMISSION ) {
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );
		return CG_CalcFov();
	}

	cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
	cg.bobfracsin = fabs( sin( ( ps->bobCycle & 127 ) / 127.0 * M_PI ) );
	cg.xyspeed = sqrt( ps->velocity[0] * ps->velocity[0] +
					   ps->velocity[1] * ps->velocity[1] );


	VectorCopy( ps->origin, cg.refdef.vieworg );
	VectorCopy( ps->viewangles, cg.refdefViewAngles );

	// add error decay
	if ( cg_errorDecay.value > 0 ) {
		int t;
		float f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
		if ( f > 0 && f < 1 ) {
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		} else {
			cg.predictedErrorTime = 0;
		}
	}

	// Ridah, lock the viewangles if the game has told us to
	if ( ps->viewlocked ) {

		/*
		if (ps->viewlocked == 4)
		{
			centity_t *tent;
			tent = &cg_entities[ps->viewlocked_entNum];
			VectorCopy (tent->currentState.apos.trBase, cg.refdefViewAngles);
		}
		else
		*/
		BG_EvaluateTrajectory( &cg_entities[ps->viewlocked_entNum].currentState.apos, cg.time, cg.refdefViewAngles );

		if ( ps->viewlocked == 2 ) {
			cg.refdefViewAngles[0] += crandom();
			cg.refdefViewAngles[1] += crandom();
		}
	}
	// done.

	if ( cg.renderingThirdPerson ) {
		// back away from character
		CG_OffsetThirdPersonView();
	} else {
		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView();

		// Ridah, lock the viewangles if the game has told us to
		if ( ps->viewlocked == 4 ) {
			vec3_t fwd;
			AngleVectors( cg.refdefViewAngles, fwd, NULL, NULL );
			VectorMA( cg_entities[ps->viewlocked_entNum].currentState.pos.trBase, 16, fwd, cg.refdef.vieworg );
		} else if ( ps->viewlocked )     {
			vec3_t fwd;
			float oldZ;
			// set our position to be behind it
			oldZ = cg.refdef.vieworg[2];
			AngleVectors( cg.refdefViewAngles, fwd, NULL, NULL );
			VectorMA( cg_entities[ps->viewlocked_entNum].currentState.pos.trBase, -34, fwd, cg.refdef.vieworg );
			cg.refdef.vieworg[2] = oldZ;
		}
		// done.
	}

	// position eye reletive to origin
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

	if ( cg.hyperspace ) {
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


/*
=====================
CG_PowerupTimerSounds
=====================
*/
static void CG_PowerupTimerSounds( void ) {
	int i;
	int t;

	// powerup timers going away
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		t = cg.snap->ps.powerups[i];
		if ( t <= cg.time ) {
			continue;
		}
		if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
			continue;
		}
		if ( ( t - cg.time ) / POWERUP_BLINK_TIME != ( t - cg.oldTime ) / POWERUP_BLINK_TIME ) {
			trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_ITEM, cgs.media.wearOffSound );
		}
	}
}

//=========================================================================

/*
==============
CG_DrawSkyBoxPortal
==============
*/
void CG_DrawSkyBoxPortal( void ) {
	static float lastfov = 90;      // for transitions back from zoomed in modes
	refdef_t backuprefdef;
	float fov_x;
	float fov_y;
	float x;
	char *cstr;
	char *token;
	float zoomFov;
	float f;
	static qboolean foginited = qfalse; // only set the portal fog values once

	if ( !( cstr = (char *)CG_ConfigString( CS_SKYBOXORG ) ) || !strlen( cstr ) ) {
		// no skybox in this map
		return;
	}

	// if they are waiting at the mission stats screen, show the stats
	if ( cg_gameType.integer == GT_SINGLE_PLAYER ) {
		if ( strlen( cg_missionStats.string ) > 1 ) {
			return;
		}
	}

	backuprefdef = cg.refdef;

	if ( cg_skybox.integer ) {
		token = COM_ParseExt( &cstr, qfalse );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n" );
		}
		cg.refdef.vieworg[0] = atof( token );

		token = COM_ParseExt( &cstr, qfalse );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n" );
		}
		cg.refdef.vieworg[1] = atof( token );

		token = COM_ParseExt( &cstr, qfalse );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n" );
		}
		cg.refdef.vieworg[2] = atof( token );

		token = COM_ParseExt( &cstr, qfalse );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring\n" );
		}
		fov_x = atoi( token );

		if ( !fov_x ) {
			fov_x = 90;
		}


		// setup fog the first time, ignore this part of the configstring after that
		token = COM_ParseExt( &cstr, qfalse );
		if ( !token || !token[0] ) {
			CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog state\n" );
		} else {
			vec4_t fogColor;
			int fogStart, fogEnd;

			if ( atoi( token ) ) {   // this camera has fog
				//			if(!foginited) {
				if ( 1 ) {
					token = COM_ParseExt( &cstr, qfalse );
					if ( !token || !token[0] ) {
						CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[0]\n" );
					}
					fogColor[0] = atof( token );

					token = COM_ParseExt( &cstr, qfalse );
					if ( !token || !token[0] ) {
						CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[1]\n" );
					}
					fogColor[1] = atof( token );

					token = COM_ParseExt( &cstr, qfalse );
					if ( !token || !token[0] ) {
						CG_Error( "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[2]\n" );
					}
					fogColor[2] = atof( token );

					token = COM_ParseExt( &cstr, qfalse );
					if ( !token || !token[0] ) {
						fogStart = 0;
					} else {
						fogStart = atoi( token );
					}

					token = COM_ParseExt( &cstr, qfalse );
					if ( !token || !token[0] ) {
						fogEnd = 0;
					} else {
						fogEnd = atoi( token );
					}

					trap_R_SetFog( FOG_PORTALVIEW, fogStart, fogEnd, fogColor[0], fogColor[1], fogColor[2], 1.1 );
					foginited = qtrue;
				}
			} else {
				if ( !foginited ) {
					trap_R_SetFog( FOG_PORTALVIEW, 0,0,0,0,0,0 ); // init to null
					foginited = qtrue;
				}
			}
		}

		//----(SA)	end


		if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
			// if in intermission, use a fixed value
			fov_x = 90;
		} else {
			// user selectable
			if ( cgs.dmflags & DF_FIXED_FOV ) {
				// dmflag to prevent wide fov for all clients
				fov_x = 90;
			} else {
				fov_x = cg_fov.value;
				if ( fov_x < 1 ) {
					fov_x = 1;
				} else if ( fov_x > 160 ) {
					fov_x = 160;
				}
			}

			// account for zooms
			if ( cg.zoomval ) {
				zoomFov = cg.zoomval;   // (SA) use user scrolled amount

				if ( zoomFov < 1 ) {
					zoomFov = 1;
				} else if ( zoomFov > 160 ) {
					zoomFov = 160;
				}
			} else {
				zoomFov = lastfov;
			}

			// do smooth transitions for the binocs
			if ( cg.zoomedBinoc ) {        // binoc zooming in
				f = ( cg.time - cg.zoomTime ) / (float)ZOOM_TIME;
				if ( f > 1.0 ) {
					fov_x = zoomFov;
				} else {
					fov_x = fov_x + f * ( zoomFov - fov_x );
				}
				lastfov = fov_x;
			} else if ( cg.zoomval ) {    // zoomed by sniper/snooper
				fov_x = cg.zoomval;
				lastfov = fov_x;
			} else {                    // binoc zooming out
				f = ( cg.time - cg.zoomTime ) / (float)ZOOM_TIME;
				if ( f > 1.0 ) {
					fov_x = fov_x;
				} else {
					fov_x = zoomFov + f * ( fov_x - zoomFov );
				}
			}
		}

		if ( cg.weaponSelect == WP_SNOOPERSCOPE ) {
			cg.refdef.rdflags |= RDF_SNOOPERVIEW;
		} else {
			cg.refdef.rdflags &= ~RDF_SNOOPERVIEW;
		}

		if ( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
			fov_x = 55;
		}

		x = cg.refdef.width / tan( fov_x / 360 * M_PI );
		fov_y = atan2( cg.refdef.height, x );
		fov_y = fov_y * 360 / M_PI;

		cg.refdef.fov_x = fov_x;
		cg.refdef.fov_y = fov_y;

		cg.refdef.rdflags |= RDF_SKYBOXPORTAL;
		cg.refdef.rdflags |= RDF_DRAWSKYBOX;

	} else {    // end if(cg_skybox.integer)

		cg.refdef.rdflags |= RDF_SKYBOXPORTAL;
		cg.refdef.rdflags &= ~RDF_DRAWSKYBOX;
	}


	cg.refdef.time = cg.time;

	// draw the skybox
	trap_R_RenderScene( &cg.refdef );

	cg.refdef = backuprefdef;
}

/*
=========================
removed CG_DrawNotebook
=========================
*/


//=========================================================================

extern void CG_SetupDlightstyles( void );


//#define DEBUGTIME_ENABLED
#ifdef DEBUGTIME_ENABLED
#define DEBUGTIME CG_Printf( "t%i:%i ", dbgCnt++, elapsed = ( trap_Milliseconds() - dbgTime ) ); dbgTime += elapsed;
#else
#define DEBUGTIME
#endif

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback ) {
	int inwater;

	cg.cld = 0;         // NERVE - SMF - reset clientDamage

#ifdef DEBUGTIME_ENABLED
	int dbgTime = trap_Milliseconds(),elapsed;
	int dbgCnt = 0;
#endif

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	// update cvars
	CG_UpdateCvars();
/*
	// RF, if we should force a weapon, then do so
	if( !cg.weaponSelect ) {
		if (cg_loadWeaponSelect.integer > 0) {
			cg.weaponSelect = cg_loadWeaponSelect.integer;
			cg.weaponSelectTime = cg.time;
			trap_Cvar_Set( "cg_loadWeaponSelect", "0" );	// turn it off
		}
	}
*/
#ifdef DEBUGTIME_ENABLED
	CG_Printf( "\n" );
#endif
	DEBUGTIME

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.infoScreenText[0] != 0 ) {
		CG_DrawInformation();
		return;
	}

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds( qfalse );

	DEBUGTIME

	// clear all the render lists
	trap_R_ClearScene();

	DEBUGTIME

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	DEBUGTIME

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
		CG_DrawInformation();
		return;
	}

	if ( cg.weaponSelect == WP_FG42SCOPE || cg.weaponSelect == WP_SNOOPERSCOPE || cg.weaponSelect == WP_SNIPERRIFLE ) {
		float spd;
		spd = VectorLength( cg.snap->ps.velocity );
		if ( spd > 180.0f ) {
			switch ( cg.weaponSelect ) {
			case WP_FG42SCOPE:
				CG_FinishWeaponChange( cg.weaponSelect, WP_FG42 );
				break;
			case WP_SNOOPERSCOPE:
				CG_FinishWeaponChange( cg.weaponSelect, WP_GARAND );
				break;
			case WP_SNIPERRIFLE:
				CG_FinishWeaponChange( cg.weaponSelect, WP_MAUSER );
				break;
			}
		}
	}

	DEBUGTIME

	if ( !cg.lightstylesInited ) {
		CG_SetupDlightstyles();
	}

	DEBUGTIME

	// if we have been told not to render, don't
	if ( cg_norender.integer ) {
		return;
	}

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	DEBUGTIME

	// decide on third person view
	cg.renderingThirdPerson = cg_thirdPerson.integer /*|| (cg.snap->ps.stats[STAT_HEALTH] <= 0)*/;

	// build cg.refdef
	inwater = CG_CalcViewValues();

	CG_CalcShakeCamera();
	CG_ApplyShakeCamera();

	DEBUGTIME

	// RF, draw the skyboxportal
	CG_DrawSkyBoxPortal();

	DEBUGTIME

	if ( inwater ) {
		CG_UnderwaterSounds();
	}

	DEBUGTIME

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson ) {
		CG_DamageBlendBlob();
	}

	DEBUGTIME

	// build the render lists
	if ( !cg.hyperspace ) {
		CG_AddPacketEntities();         // adter calcViewValues, so predicted player state is correct
		CG_AddMarks();

		DEBUGTIME

		// Rafael particles
		CG_AddParticles();
		// done.

		DEBUGTIME

		CG_AddLocalEntities();

		DEBUGTIME
	}


	CG_AddViewWeapon( &cg.predictedPlayerState );


	DEBUGTIME

	// Ridah, trails
	if ( !cg.hyperspace ) {
		CG_AddFlameChunks();
		CG_AddTrails();         // this must come last, so the trails dropped this frame get drawn
	}
	// done.

	DEBUGTIME

	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel ) {
		CG_AddTestModel();
	}
	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	DEBUGTIME

	// warning sounds when powerup is wearing off
	CG_PowerupTimerSounds();

	// make sure the lagometerSample and frame timing isn't done twice when in stereo
	if ( stereoView != STEREO_RIGHT ) {
		cg.frametime = cg.time - cg.oldTime;
		if ( cg.frametime < 0 ) {
			cg.frametime = 0;
		}
		cg.oldTime = cg.time;
		CG_AddLagometerFrameInfo();
	}

	DEBUGTIME

	// let the client system know what our weapon, holdable item and zoom settings are
	trap_SetUserCmdValue( cg.weaponSelect, cg.holdableSelect, cg.zoomSensitivity, cg.cld );

	// actually issue the rendering calls
	CG_DrawActive( stereoView );

	DEBUGTIME

	// update audio positions
	trap_S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater );

	if ( cg_stats.integer ) {
		CG_Printf( "cg.clientFrame:%i\n", cg.clientFrame );
	}

	DEBUGTIME
}

