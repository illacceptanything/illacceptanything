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


// cg_localents.c -- every frame, generate renderer commands for locally
// processed entities, like smoke puffs, gibs, shells, etc.

#include "cg_local.h"

// Ridah, increased this
//#define	MAX_LOCAL_ENTITIES	512
#define MAX_LOCAL_ENTITIES  768     // renderer can only handle 1024 entities max, so we should avoid
									// overwriting game entities
// done.

localEntity_t cg_localEntities[MAX_LOCAL_ENTITIES];
localEntity_t cg_activeLocalEntities;       // double linked list
localEntity_t   *cg_freeLocalEntities;      // single linked list

// Ridah, debugging
int localEntCount = 0;

/*
===================
CG_InitLocalEntities

This is called at startup and for tournement restarts
===================
*/
void    CG_InitLocalEntities( void ) {
	int i;

	memset( cg_localEntities, 0, sizeof( cg_localEntities ) );
	cg_activeLocalEntities.next = &cg_activeLocalEntities;
	cg_activeLocalEntities.prev = &cg_activeLocalEntities;
	cg_freeLocalEntities = cg_localEntities;
	for ( i = 0 ; i < MAX_LOCAL_ENTITIES - 1 ; i++ ) {
		cg_localEntities[i].next = &cg_localEntities[i + 1];
	}

	// Ridah, debugging
	localEntCount = 0;
}


/*
==================
CG_FreeLocalEntity
==================
*/
void CG_FreeLocalEntity( localEntity_t *le ) {
	if ( !le->prev ) {
		CG_Error( "CG_FreeLocalEntity: not active" );
	}

	// Ridah, debugging
	localEntCount--;
//	trap_Print( va("FreeLocalEntity: locelEntCount = %d\n", localEntCount) );
	// done.

	// remove from the doubly linked active list
	le->prev->next = le->next;
	le->next->prev = le->prev;

	// the free list is only singly linked
	le->next = cg_freeLocalEntities;
	cg_freeLocalEntities = le;
}

/*
===================
CG_AllocLocalEntity

Will allways succeed, even if it requires freeing an old active entity
===================
*/
localEntity_t   *CG_AllocLocalEntity( void ) {
	localEntity_t   *le;

	if ( !cg_freeLocalEntities ) {
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		CG_FreeLocalEntity( cg_activeLocalEntities.prev );
	}

	// Ridah, debugging
	localEntCount++;
//	trap_Print( va("AllocLocalEntity: locelEntCount = %d\n", localEntCount) );
	// done.

	le = cg_freeLocalEntities;
	cg_freeLocalEntities = cg_freeLocalEntities->next;

	memset( le, 0, sizeof( *le ) );

	// link into the active list
	le->next = cg_activeLocalEntities.next;
	le->prev = &cg_activeLocalEntities;
	cg_activeLocalEntities.next->prev = le;
	cg_activeLocalEntities.next = le;
	return le;
}


/*
====================================================================================

FRAGMENT PROCESSING

A fragment localentity interacts with the environment in some way (hitting walls),
or generates more localentities along a trail.

====================================================================================
*/

/*
================
CG_BloodTrail

Leave expanding blood puffs behind gibs
================
*/
// use this to change between particle and trail code
//#define BLOOD_PARTICLE_TRAIL
void CG_BloodTrail( localEntity_t *le ) {
	int t;
	int t2;
	int step;
	vec3_t newOrigin;

#ifndef BLOOD_PARTICLE_TRAIL
	static vec3_t col = {1,1,1};
#endif

	centity_t   *cent;
	cent = &cg_entities[le->ownerNum];

	if ( !cg_blood.integer ) {
		return;
	}

	// step = 150;
#ifdef BLOOD_PARTICLE_TRAIL
	step = 10;
#else
	// time it takes to move 3 units
	step = ( 1000 * 3 ) / VectorLength( le->pos.trDelta );
#endif

	if ( cent && cent->currentState.aiChar == AICHAR_ZOMBIE ) {
		step = 30;
	}

	t = step * ( ( cg.time - cg.frametime + step ) / step );
	t2 = step * ( cg.time / step );

	for ( ; t <= t2; t += step ) {
		BG_EvaluateTrajectory( &le->pos, t, newOrigin );

#ifdef BLOOD_PARTICLE_TRAIL
		CG_Particle_Bleed( cgs.media.smokePuffShader, newOrigin, vec3_origin, 0, 500 + rand() % 200 );
#else


		if ( cent && cent->currentState.aiChar == AICHAR_ZOMBIE ) {
			CG_Particle_Bleed( cgs.media.smokePuffShader, newOrigin, vec3_origin, 1, 500 + rand() % 200 );
		} else {
			// Ridah, blood trail using trail code (should be faster since we don't have to spawn as many)
			le->headJuncIndex = CG_AddTrailJunc( le->headJuncIndex,
												 cgs.media.bloodTrailShader,
												 t,
												 STYPE_STRETCH,
												 newOrigin,
												 180,
												 1.0, // start alpha
												 0.0, // end alpha
												 12.0,
												 12.0,
												 TJFL_NOCULL,
												 col, col,
												 0, 0 );
		}
#endif

	}
}


/*
================
CG_FragmentBounceMark
================
*/
void CG_FragmentBounceMark( localEntity_t *le, trace_t *trace ) {
	int radius;

	if ( le->leMarkType == LEMT_BLOOD ) {
		static int lastBloodMark;

		// don't drop too many blood marks
		if ( !( lastBloodMark > cg.time || lastBloodMark > cg.time - 100 ) ) {
			radius = 16 + ( rand() & 31 );
			CG_ImpactMark( cgs.media.bloodDotShaders[rand() % 5], trace->endpos, trace->plane.normal, random() * 360,
						   1,1,1,1, qtrue, radius, qfalse, cg_bloodTime.integer * 1000 );

			lastBloodMark = cg.time;
		}
	}

	// don't allow a fragment to make multiple marks, or they
	// pile up while settling
	le->leMarkType = LEMT_NONE;
}

/*
================
CG_FragmentBounceSound
================
*/
void CG_FragmentBounceSound( localEntity_t *le, trace_t *trace ) {
	if ( le->leBounceSoundType == LEBS_BLOOD ) {
		// half the gibs will make splat sounds
		if ( rand() & 1 ) {
			int r = rand() & 3;
			sfxHandle_t s;

			if ( r < 2 ) {
				s = cgs.media.gibBounce1Sound;
			} else if ( r == 2 ) {
				s = cgs.media.gibBounce2Sound;
			} else {
				s = cgs.media.gibBounce3Sound;
			}
			trap_S_StartSound( trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s );
		}
	} else if ( le->leBounceSoundType == LEBS_BRASS ) {

//----(SA) added
	} else if ( le->leBounceSoundType == LEBS_ROCK ) {
		// half the hits will make thunk sounds	(this is just to start since we don't even have the sound yet... (SA))
		if ( rand() & 1 ) {
			int r = rand() & 3;
			sfxHandle_t s;

			if ( r < 2 ) {
				s = cgs.media.debBounce1Sound;
			} else if ( r == 2 ) {
				s = cgs.media.debBounce2Sound;
			} else {
				s = cgs.media.debBounce3Sound;
			}
			trap_S_StartSound( trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, s );
		}
//----(SA) end

	} else if ( le->leBounceSoundType == LEBS_BONE ) {

		trap_S_StartSound( trace->endpos, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.boneBounceSound );

	}

	// don't allow a fragment to make multiple bounce sounds,
	// or it gets too noisy as they settle
	le->leBounceSoundType = LEBS_NONE;
}


/*
================
CG_ReflectVelocity
================
*/
void CG_ReflectVelocity( localEntity_t *le, trace_t *trace ) {
	vec3_t velocity;
	float dot;
	int hitTime;

	// reflect the velocity on the trace plane
	hitTime = cg.time - cg.frametime + cg.frametime * trace->fraction;
	BG_EvaluateTrajectoryDelta( &le->pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2 * dot, trace->plane.normal, le->pos.trDelta );

	VectorScale( le->pos.trDelta, le->bounceFactor, le->pos.trDelta );

	VectorCopy( trace->endpos, le->pos.trBase );
	le->pos.trTime = cg.time;


	// check for stop, making sure that even on low FPS systems it doesn't bobble

	if ( le->leMarkType == LEMT_BLOOD && trace->startsolid ) {
		//centity_t *cent;
		//cent = &cg_entities[trace->entityNum];
		//if (cent && cent->currentState.apos.trType != TR_STATIONARY)
		//	le->pos.trType = TR_STATIONARY;
	} else if ( trace->allsolid ||
				( trace->plane.normal[2] > 0 &&
				  ( le->pos.trDelta[2] < 40 || le->pos.trDelta[2] < -cg.frametime * le->pos.trDelta[2] ) ) ) {

//----(SA)	if it's a fragment and it's not resting on the world...
//			if(le->leType == LE_DEBRIS && trace->entityNum < (MAX_ENTITIES - 1))
		if ( le->leType == LE_FRAGMENT && trace->entityNum < ( MAX_ENTITIES - 1 ) ) {
			le->pos.trType = TR_GRAVITY_PAUSED;
		} else
		{
			le->pos.trType = TR_STATIONARY;
		}
	} else {

	}
}


//----(SA)	added

/*
==============
CG_AddEmitter
==============
*/
void CG_AddEmitter( localEntity_t *le ) {
	vec3_t dir;
	int nextTime = 50;

	if ( le->breakCount > cg.time ) {  // using 'breakCount' for 'wait'
		return;
	}

	if ( cg_paused.integer ) { // don't add while paused
		return;
	}

	// TODO: look up particle script and use proper effect rather than this check
	//if(water){}
	//else if(oil) {}
	//else if(steam) {}
	//else if(wine) {}

	switch ( le->headJuncIndex ) {
//		CG_ParticleImpactSmokePuff (cgs.media.oilParticle, le->pos.trBase);
//		CG_Particle_Bleed(cgs.media.smokePuffShader, le->pos.trBase, dir, 0, 200);

	case 1:     // oil
		VectorScale( le->angles.trBase, le->radius, dir );
		CG_Particle_OilParticle( cgs.media.oilParticle, le->pos.trBase, dir,  10000, le->ownerNum );
		break;
	case 2:     // water
		VectorScale( le->angles.trBase, le->radius, dir );
		CG_Particle_OilParticle( cgs.media.oilParticle, le->pos.trBase, dir,  10000, le->ownerNum );
		break;
	case 3:     // steam
		nextTime = 100;
//			CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, le->pos.trBase, 8, 1000, 8, 20, 20, 0.25f);
		CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, le->pos.trBase, le->angles.trBase, 8, 1000, 8, le->radius, 20, 0.25f );
		break;
	case 4:     // wine
		VectorScale( le->angles.trBase, le->radius, dir );
		CG_Particle_OilParticle( cgs.media.oilParticle, le->pos.trBase, dir,  10000, le->ownerNum );
		break;
	case 5:     // smoke
		nextTime = 100;
		CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, le->pos.trBase, dir, 8, 1000, 8, 20, 20, 0.25f );
		break;
	case 6:     // electrical
		nextTime = 100;
		CG_AddBulletParticles( le->pos.trBase, dir, 2, 800, 4, 16.0f );
//			CG_AddBulletParticles( le->pos.trBase, dir, 1, 800, 4, 0 );
		break;

	case 0:
	default:
		nextTime = 100;
		CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, le->pos.trBase, dir, 8, 1000, 8, 20, 20, 0.25f );
		break;
	}

	le->breakCount = cg.time + nextTime;
}

//----(SA)	end


/*
================
CG_AddFragment
================
*/
void CG_AddFragment( localEntity_t *le ) {
	vec3_t newOrigin;
	trace_t trace;
	refEntity_t     *re;
	float flameAlpha = 0.0f;   // TTimo: init
	vec3_t flameDir;
	qboolean hasFlame = qfalse;
	int i;
	int contents;

	// Ridah
	re = &le->refEntity;
	if ( !re->fadeStartTime || re->fadeEndTime < le->endTime ) {
		if ( le->endTime - cg.time > 5000 ) {
			re->fadeStartTime = le->endTime - 5000;
		} else {
			re->fadeStartTime = le->endTime - 1000;
		}
		re->fadeEndTime = le->endTime;
	}

	// Ridah, flaming gibs
	if ( le->onFireStart && ( le->onFireStart < cg.time && le->onFireEnd > cg.time ) ) {
		hasFlame = qtrue;
		// calc the alpha
		flameAlpha = 1.0 - ( (float)( cg.time - le->onFireStart ) / (float)( le->onFireEnd - le->onFireStart ) );
		if ( flameAlpha < 0.0 ) {
			flameAlpha = 0.0;
		}
		if ( flameAlpha > 1.0 ) {
			flameAlpha = 1.0;
		}
		trap_S_AddLoopingSound( -1, le->refEntity.origin, vec3_origin, cgs.media.flameCrackSound, (int)( 5.0 * flameAlpha ) );
	}

//----(SA)	added
	if ( le->leFlags & LEF_SMOKING ) {
		float alpha;
		refEntity_t flash;

		// create a little less smoke

		//	TODO: FIXME: this is not quite right, because it'll become fps dependant - in a bad way.
		//		the slower the fps, the /more/ smoke there'll be, probably driving the fps lower.
		if ( !cg_paused.integer ) {    // don't add while paused
			if ( !( rand() % 5 ) ) {
				alpha = 1.0 - ( (float)( cg.time - le->startTime ) / (float)( le->endTime - le->startTime ) );
				alpha *= 0.25f;
				memset( &flash, 0, sizeof( flash ) );
				CG_PositionEntityOnTag( &flash, &le->refEntity, "tag_flash", 0, NULL );
				CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, flash.origin, tv( 0,0,1 ), 8, 1000, 8, 20, 20, alpha );
			}
		}
	}
//----(SA)	end

	if ( le->pos.trType == TR_STATIONARY ) {
		int t;

		// Ridah, add the flame
		if ( hasFlame ) {
			refEntity_t backupEnt;

			backupEnt = le->refEntity;

			VectorClear( flameDir );
			flameDir[2] = 1;

			le->refEntity.shaderRGBA[3] = ( unsigned char )( 255.0 * flameAlpha );
			VectorCopy( flameDir, le->refEntity.fireRiseDir );
			le->refEntity.customShader = cgs.media.onFireShader;
			trap_R_AddRefEntityToScene( &le->refEntity );
			le->refEntity.customShader = cgs.media.onFireShader2;
			trap_R_AddRefEntityToScene( &le->refEntity );

			le->refEntity = backupEnt;
		}

		t = le->endTime - cg.time;
		trap_R_AddRefEntityToScene( &le->refEntity );

		return;

	} else if ( le->pos.trType == TR_GRAVITY_PAUSED ) {
		int t;

		// Ridah, add the flame
		if ( hasFlame ) {
			refEntity_t backupEnt;

			backupEnt = le->refEntity;

			VectorClear( flameDir );
			flameDir[2] = 1;

			le->refEntity.shaderRGBA[3] = ( unsigned char )( 255.0 * flameAlpha );
			VectorCopy( flameDir, le->refEntity.fireRiseDir );
			le->refEntity.customShader = cgs.media.onFireShader;
			trap_R_AddRefEntityToScene( &le->refEntity );
			le->refEntity.customShader = cgs.media.onFireShader2;
			trap_R_AddRefEntityToScene( &le->refEntity );

			le->refEntity = backupEnt;
		}

		t = le->endTime - cg.time;
		trap_R_AddRefEntityToScene( &le->refEntity );


		// trace a line from previous position down, to see if I should start falling again

		VectorCopy( le->refEntity.origin, newOrigin );
		newOrigin [2] -= 5;
		CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_MISSILECLIP );

		if ( trace.fraction == 1.0 ) { // it's clear, start moving again
			VectorClear( le->pos.trDelta );
			VectorClear( le->angles.trDelta );
			le->pos.trType = TR_GRAVITY;    // nothing below me, start falling again
		} else {
			return;
		}
	}

	// calculate new position
	BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );

	if ( hasFlame ) {
		// calc the flame dir
		VectorSubtract( le->refEntity.origin, newOrigin, flameDir );
		if ( VectorLength( flameDir ) == 0 ) {
			flameDir[2] = 1;
			// play a burning sound when not moving
			trap_S_AddLoopingSound( 0, newOrigin, vec3_origin, cgs.media.flameSound, (int)( 0.3 * 255.0 * flameAlpha ) );
		} else {
			VectorNormalize( flameDir );
			// play a flame blow sound when moving
			trap_S_AddLoopingSound( 0, newOrigin, vec3_origin, cgs.media.flameBlowSound, (int)( 0.3 * 255.0 * flameAlpha ) );
		}
	}

	contents = CONTENTS_SOLID;

	// trace a line from previous position to new position
	if ( ( le->leFlags & LEF_NOTOUCHPARENT ) && le->ownerNum ) {
		CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, le->ownerNum, contents );
	} else {
		CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, contents );
	}

	// did we hit someone?
	if ( le->leFlags & LEF_PLAYER_DAMAGE ) {
		// only do damage if travelling at a fast enough velocity
		if ( newOrigin[2] < le->refEntity.origin[2] ) {
			vec3_t pmin, pmax, dmin = {-12,-12,-12}, dmax = {12,12,12};
			// debris bounds
			VectorAdd( dmin, newOrigin, dmin );
			VectorAdd( dmax, newOrigin, dmax );
			dmax[2] += le->refEntity.origin[2] - newOrigin[2];  // add falling distance to box
			// are we touching the player?
			VectorAdd( cg.snap->ps.mins, cg.snap->ps.origin, pmin );
			VectorAdd( cg.snap->ps.maxs, cg.snap->ps.origin, pmax );
			pmin[2] = pmax[2] - 32; // only hit on the head
			for ( i = 0; i < 3; i++ ) {
				if ( ( dmax[i] < pmin[i] ) || ( dmin[i] > pmax[i] ) ) {
					break;
				}
			}
			if ( i == 3 ) {
				trap_S_StartSound( cg.snap->ps.origin, cg.snap->ps.clientNum, CHAN_VOICE, cgs.media.debrisHitSound );
				CG_ClientDamage( cg.snap->ps.clientNum, ENTITYNUM_WORLD, CLDMG_DEBRIS );
				// disable damage now for this debris
				le->leFlags &= ~LEF_PLAYER_DAMAGE;
			}
		}
	}

	if ( trace.fraction == 1.0 ) {

		// still in free fall
		VectorCopy( newOrigin, le->refEntity.origin );

		if ( le->leFlags & LEF_TUMBLE || le->angles.trType == TR_LINEAR ) {
			vec3_t angles;

			BG_EvaluateTrajectory( &le->angles, cg.time, angles );
			AnglesToAxis( angles, le->refEntity.axis );
			if ( le->sizeScale && le->sizeScale != 1.0 ) {
				for ( i = 0; i < 3; i++ ) VectorScale( le->refEntity.axis[i], le->sizeScale, le->refEntity.axis[i] );
			}
		}

		// Ridah, add the flame
		if ( hasFlame ) {
			refEntity_t backupEnt;

			backupEnt = le->refEntity;

			le->refEntity.shaderRGBA[3] = ( unsigned char )( 255.0 * flameAlpha );
			VectorCopy( flameDir, le->refEntity.fireRiseDir );
			le->refEntity.customShader = cgs.media.onFireShader;
			trap_R_AddRefEntityToScene( &le->refEntity );
			le->refEntity.customShader = cgs.media.onFireShader2;
			trap_R_AddRefEntityToScene( &le->refEntity );

			le->refEntity = backupEnt;
		}

		trap_R_AddRefEntityToScene( &le->refEntity );

		// add a blood trail
		if ( le->leBounceSoundType == LEBS_BLOOD ) {
			CG_BloodTrail( le );
		}

		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if ( trap_CM_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) {
		CG_FreeLocalEntity( le );
		return;
	}

	// do a bouncy sound
	CG_FragmentBounceSound( le, &trace );

	// reflect the velocity on the trace plane
	CG_ReflectVelocity( le, &trace );


	// (SA) disabled since it caused debris to get stuck in walls (flags mostly)
	// RF, if it has come to a halt, pause velocity
//	if (VectorLength( le->pos.trDelta ) < 1) {
//		le->pos.trType = TR_STATIONARY;
//		VectorCopy( trace.endpos, le->pos.trBase );
//		VectorClear( le->pos.trDelta );
//	}

	// break on contact?
	if ( le->breakCount ) {
		clientInfo_t    *ci;
		int clientNum;
		localEntity_t   *nle;
		vec3_t dir;

		clientNum = le->ownerNum;
		if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
			CG_Error( "Bad clientNum on player entity" );
		}
		ci = &cgs.clientinfo[ clientNum ];

		// spawn some new fragments
		for ( i = 0; i <= le->breakCount; i++ ) {
			nle = CG_AllocLocalEntity();
			memcpy( &( nle->leType ), &( le->leType ), sizeof( localEntity_t ) - 2 * sizeof( localEntity_t * ) );
			if ( nle->breakCount-- < 2 ) {
				nle->refEntity.hModel = ci->gibModels[rand() % 2];
			} else {
				nle->refEntity.hModel = ci->gibModels[rand() % 4];
			}
			// make it smaller
			nle->endTime = le->endTime + ( cg.time - le->startTime );
			nle->sizeScale *= 0.8;
			if ( nle->sizeScale < 0.7 ) {
				nle->sizeScale = 0.7;
				nle->leBounceSoundType = 0;
			}
			// move us a bit
			VectorNormalize2( nle->pos.trDelta, dir );
			VectorMA( trace.endpos, 4.0 * le->sizeScale * i, dir, nle->pos.trBase );
			// randomize vel a bit
			VectorMA( nle->pos.trDelta, VectorLength( nle->pos.trDelta ) * 0.3, bytedirs[rand() % NUMVERTEXNORMALS], nle->pos.trDelta );
		}
		// we're done
		CG_FreeLocalEntity( le );
		return;
	}

	if ( le->pos.trType == TR_STATIONARY && le->leMarkType == LEMT_BLOOD ) {
		// RF, disabled for performance reasons in boss1
		//if (le->leBounceSoundType)
		//	CG_BloodPool (le, cgs.media.bloodPool, &trace);

		// leave a mark
		if ( le->leMarkType ) {
			CG_FragmentBounceMark( le, &trace );
		}
	}

	// Ridah, add the flame
	if ( hasFlame ) {
		refEntity_t backupEnt;

		backupEnt = le->refEntity;

		le->refEntity.shaderRGBA[3] = ( unsigned char )( 255.0 * flameAlpha );
		VectorCopy( flameDir, le->refEntity.fireRiseDir );
		le->refEntity.customShader = cgs.media.onFireShader;
		trap_R_AddRefEntityToScene( &le->refEntity );
		le->refEntity.customShader = cgs.media.onFireShader2;
		trap_R_AddRefEntityToScene( &le->refEntity );

		le->refEntity = backupEnt;
	}

	trap_R_AddRefEntityToScene( &le->refEntity );
}

// Ridah
/*
================
CG_AddMovingTracer
================
*/
void CG_AddMovingTracer( localEntity_t *le ) {
	vec3_t start, end, dir;

	BG_EvaluateTrajectory( &le->pos, cg.time, start );
	VectorNormalize2( le->pos.trDelta, dir );
	VectorMA( start, cg_tracerLength.value, dir, end );

	CG_DrawTracer( start, end );
}

/*
================
CG_AddSparkElements
================
*/
void CG_AddSparkElements( localEntity_t *le ) {
	vec3_t newOrigin;
	trace_t trace;
	float time;
	float lifeFrac;

	time = (float)( cg.time - cg.frametime );

	while ( 1 ) {
		// calculate new position
		BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );

//		if ((le->endTime - le->startTime) > 500) {

		// trace a line from previous position to new position
		CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, MASK_SHOT );

		// if stuck, kill it
		if ( trace.startsolid ) {
			// HACK, some walls screw up, so just pass through if starting in a solid
			VectorCopy( newOrigin, trace.endpos );
			trace.fraction = 1.0;
		}

		// moved some distance
		VectorCopy( trace.endpos, le->refEntity.origin );
/*
		} else
		{	// just move it there

			VectorCopy( newOrigin, le->refEntity.origin );
			trace.fraction = 1.0;

		}
*/
		time += cg.frametime * trace.fraction;

		lifeFrac = (float)( cg.time - le->startTime ) / (float)( le->endTime - le->startTime );

		// add a trail
		le->headJuncIndex = CG_AddSparkJunc( le->headJuncIndex,
											 le->refEntity.customShader,
											 le->refEntity.origin,
											 200,
											 1.0 - lifeFrac, // start alpha
											 0.0, //1.0 - lifeFrac,	// end alpha
											 lifeFrac * 2.0 * ( ( ( le->endTime - le->startTime ) > 400 ) + 1 ) * 1.5,
											 lifeFrac * 2.0 * ( ( ( le->endTime - le->startTime ) > 400 ) + 1 ) * 1.5 );

		// if it is in a nodrop zone, remove it
		// this keeps gibs from waiting at the bottom of pits of death
		// and floating levels
// for some reason SFM1.BSP is one big NODROP zone
//		if ( trap_CM_PointContents( le->refEntity.origin, 0 ) & CONTENTS_NODROP ) {
//			CG_FreeLocalEntity( le );
//			return;
//		}

		if ( trace.fraction < 1.0 ) {
			// just kill it
			CG_FreeLocalEntity( le );
			return;
/*
			// reflect the velocity on the trace plane
			CG_ReflectVelocity( le, &trace );
			// the intersection is a fraction of the frametime
			le->pos.trTime = (int)time;
*/
		}

		if ( trace.fraction == 1.0 || time >= (float)cg.time ) {
			return;
		}
	}

}

/*
================
CG_AddFuseSparkElements
================
*/
void CG_AddFuseSparkElements( localEntity_t *le ) {

	float FUSE_SPARK_WIDTH      = 1.0;

	int step = 10;
	float time;
	float lifeFrac;
	static vec3_t whiteColor = {1,1,1};

	time = (float)( le->lastTrailTime );

	while ( time < cg.time ) {

		// calculate new position
		BG_EvaluateTrajectory( &le->pos, time, le->refEntity.origin );

		lifeFrac = (float)( time - le->startTime ) / (float)( le->endTime - le->startTime );

		//if (lifeFrac > 0.2) {
		// add a trail
		le->headJuncIndex = CG_AddTrailJunc( le->headJuncIndex, cgs.media.sparkParticleShader, time, STYPE_STRETCH, le->refEntity.origin, (int)( lifeFrac * (float)( le->endTime - le->startTime ) / 2.0 ),
											 1.0 /*(1.0 - lifeFrac)*/, 0.0, FUSE_SPARK_WIDTH * ( 1.0 - lifeFrac ), FUSE_SPARK_WIDTH * ( 1.0 - lifeFrac ), TJFL_SPARKHEADFLARE, whiteColor, whiteColor, 0, 0 );
		//}

		time += step;

		le->lastTrailTime = time;
	}

}

/*
================
CG_AddBloodElements
================
*/
void CG_AddBloodElements( localEntity_t *le ) {
	vec3_t newOrigin;
	trace_t trace;
	float time;
	float lifeFrac;

	time = (float)( cg.time - cg.frametime );

	while ( 1 ) {
		// calculate new position
		BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );

		// trace a line from previous position to new position
		CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, MASK_SHOT );

		// if stuck, kill it
		if ( trace.startsolid ) {
			// HACK, some walls screw up, so just pass through if starting in a solid
			VectorCopy( newOrigin, trace.endpos );
			trace.fraction = 1.0;
		}

		// moved some distance
		VectorCopy( trace.endpos, le->refEntity.origin );
		time += cg.frametime * trace.fraction;

		lifeFrac = (float)( cg.time - le->startTime ) / (float)( le->endTime - le->startTime );

		// add a trail
		le->headJuncIndex = CG_AddSparkJunc( le->headJuncIndex,
											 cgs.media.bloodTrailShader,
											 le->refEntity.origin,
											 200,
											 1.0 - lifeFrac, // start alpha
											 1.0 - lifeFrac, // end alpha
											 3.0,
											 5.0 );

		if ( trace.fraction < 1.0 ) {
			// reflect the velocity on the trace plane
			CG_ReflectVelocity( le, &trace );

			// TODO: spawn a blood decal here?

			// the intersection is a fraction of the frametime
			le->pos.trTime = (int)time;
		}

		if ( trace.fraction == 1.0 || time >= (float)cg.time ) {
			return;
		}
	}

}

/*
===================
CG_AddSpiritViewflash
===================
*/
void CG_AddSpiritViewflash( localEntity_t *le ) {
	float alpha;
#define SPIRIT_FLASH_FADEIN     50
#define SPIRIT_FLASH_DURATION   400
#define SPIRIT_FLASH_FADEOUT    2000

	if ( cg.viewFade > 1.0 ) {
		return;
	}

	if ( cg.time < le->startTime + SPIRIT_FLASH_FADEIN ) {
		alpha = (float)( cg.time - le->startTime ) / (float)SPIRIT_FLASH_FADEIN;
	} else if ( cg.time < le->startTime + SPIRIT_FLASH_FADEIN + SPIRIT_FLASH_DURATION )     {
		alpha = 1.0;
	} else if ( cg.time < le->startTime + SPIRIT_FLASH_FADEIN + SPIRIT_FLASH_DURATION + SPIRIT_FLASH_FADEOUT )     {
		alpha = 1.0 - (float)( cg.time - ( le->startTime + SPIRIT_FLASH_FADEIN + SPIRIT_FLASH_DURATION ) ) / (float)SPIRIT_FLASH_FADEOUT;
	} else {
		return;
	}

	if ( alpha < 0 ) {
		return;
	}

	// only ever use the highest fade
	if ( cg.viewFade < alpha ) {
		cg.viewFade = alpha;
	}
}

/*
================
CG_AddClientCritter
================
*/
void CG_AddClientCritter( localEntity_t *le ) {
	vec3_t newOrigin;
	trace_t trace;
	int time, step = 25, i;
	vec3_t v, ang, v2, oDelta;
	localEntity_t backup;
	float oldSpeed, enemyDist, of;
	vec3_t enemyPos;
	float alpha = 0.0f, fadeRatio; // TTimo: init

	if ( cg_entities[le->ownerNum].currentState.otherEntityNum2 == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, enemyPos );
		enemyPos[2] += cg.snap->ps.viewheight;
	} else {
		VectorCopy( cg_entities[le->ownerNum].currentState.origin2, enemyPos );
	}

	VectorCopy( le->pos.trDelta, oDelta );

	// vary the enemyPos to create a psuedo-randomness
	of = (float)cg.time + le->startTime;
	enemyPos[0] += 12 * ( sin( of / 100 ) * cos( of / 78 ) );
	enemyPos[1] += 12 * ( sin( of / 70 ) * cos( of / 82 ) );
	enemyPos[2] += 12 * ( sin( of / 67 ) * cos( of / 98 ) );

	time = le->lastTrailTime + step;

	fadeRatio = (float)( cg.time - le->startTime ) / 2000.0;
	if ( fadeRatio < 0.0 ) {
		fadeRatio = 0.0;
	}
	if ( fadeRatio > 1.0 ) {
		fadeRatio = 1.0;
	}

	while ( time <= cg.time ) {
		if ( time > le->refEntity.fadeStartTime ) {
			alpha = (float)( time - le->refEntity.fadeStartTime ) / (float)( le->refEntity.fadeEndTime - le->refEntity.fadeStartTime );
			if ( alpha < 0 ) {
				alpha = 0;
			} else if ( alpha > 1 ) {
				alpha = 1;
			}
		} else {
			alpha = fadeRatio;
		}

		// calculate new position
		BG_EvaluateTrajectory( &le->pos, time, newOrigin );

		VectorSubtract( enemyPos, le->refEntity.origin, v );
		enemyDist = VectorNormalize( v );

		// trace a line from previous position to new position
		CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, le->ownerNum, MASK_SHOT );

		// if stuck, kill it
		if ( trace.startsolid || ( trace.fraction < 1.0 ) ) {
			// let heinrich spirits pass through geometry
			if ( !( le->leType == LE_HELGA_SPIRIT && ( le->refEntity.hModel == cgs.media.ssSpiritSkullModel ) ) ) {
				// kill it
				CG_FreeLocalEntity( le );
				return;
			} else {
				VectorCopy( newOrigin, trace.endpos );
			}
		}

		// moved some distance
		VectorCopy( trace.endpos, le->refEntity.origin );

		// record this pos
		le->validOldPos[le->oldPosHead] = 1;
		VectorCopy( le->refEntity.origin, le->oldPos[le->oldPosHead] );
		if ( ++le->oldPosHead >= MAX_OLD_POS ) {
			le->oldPosHead = 0;
		}

		if ( le->leType == LE_ZOMBIE_SPIRIT ) {
			le->headJuncIndex = CG_AddTrailJunc( le->headJuncIndex,
												 cgs.media.zombieSpiritTrailShader,
												 time,
												 STYPE_STRETCH,
												 le->refEntity.origin,
												 (int)le->effectWidth,  // trail life
												 0.3 * alpha,
												 0.0,
												 le->radius,
												 0,
												 0, //TJFL_FIXDISTORT,
												 colorWhite,
												 colorWhite,
												 1.0, 1 );
		}

		if ( le->leType == LE_HELGA_SPIRIT && le->refEntity.hModel != cgs.media.ssSpiritSkullModel ) {
			le->headJuncIndex = CG_AddTrailJunc( le->headJuncIndex,
												 cgs.media.helgaSpiritTrailShader,
												 time,
												 STYPE_STRETCH,
												 le->refEntity.origin,
												 (int)le->effectWidth,  // trail life
												 0.3 * alpha,
												 0.0,
												 le->radius,
												 0,
												 0, //TJFL_FIXDISTORT,
												 colorWhite,
												 colorWhite,
												 1.0, 1 );
		}

		// tracking factor
		if ( le->leType == LE_ZOMBIE_BAT ) {
			le->bounceFactor = 3.0 * (float)step / 1000.0;
		} else {
			le->bounceFactor = 5.0 * (float)step / 1000.0;
		}
		oldSpeed = VectorLength( le->pos.trDelta );

		// track the enemy
		backup = *le;
		VectorSubtract( enemyPos, le->refEntity.origin, v );
		enemyDist = VectorNormalize( v );

		if ( alpha > 0.5 && ( le->lastSpiritDmgTime < time - 100 ) && enemyDist < 24 ) {
			localEntity_t *fb;

			// if dead, ignore
			if ( !( le->ownerNum != cg.snap->ps.clientNum ? cg_entities[le->ownerNum].currentState.eFlags & EF_DEAD : cg.snap->ps.pm_type == PM_DEAD ) ) {
				// inflict the damage!
				CG_ClientDamage( cg_entities[le->ownerNum].currentState.otherEntityNum2, le->ownerNum, CLDMG_SPIRIT );
				le->lastSpiritDmgTime = time;

				if ( le->leType == LE_HELGA_SPIRIT ) {
					// spawn a "flashbang" thinker
					fb = CG_AllocLocalEntity();
					fb->leType = LE_SPIRIT_VIEWFLASH;
					fb->startTime = cg.time + 50;
					fb->endTime = fb->startTime + SPIRIT_FLASH_FADEIN + SPIRIT_FLASH_DURATION + SPIRIT_FLASH_FADEOUT;
					// gasp!
					CG_SoundPlayIndexedScript( cgs.media.helgaGaspSound, NULL, cg_entities[le->ownerNum].currentState.otherEntityNum2 );
				}
			}
		}

		VectorMA( le->pos.trDelta, le->bounceFactor * oldSpeed, v, le->pos.trDelta );
		//VectorCopy( v, le->pos.trDelta );
		if ( VectorLength( le->pos.trDelta ) < 1 ) {
			CG_FreeLocalEntity( le );
			return;
		}

		le->bounceFactor = 5.0 * (float)step / 1000.0;  // avoidance factor

		// the intersection is a fraction of the frametime
		le->pos.trTime = time;
		VectorCopy( le->refEntity.origin, le->pos.trBase );
		VectorNormalize( le->pos.trDelta );
		VectorScale( le->pos.trDelta, oldSpeed, le->pos.trDelta );

		// now trace ahead of time, if we're going to hit something, then avoid it
		// only avoid dangers if we don't have direct sight to the enemy
		trap_CM_BoxTrace( &trace, le->refEntity.origin, enemyPos, NULL, NULL, 0, MASK_SOLID );
		if ( trace.fraction < 1.0 ) {
			BG_EvaluateTrajectory( &le->pos, time + 1000, newOrigin );

			// if we would go passed the enemy, don't bother
			if ( VectorDistance( le->refEntity.origin, enemyPos ) > VectorDistance( le->refEntity.origin, newOrigin ) ) {

				trap_CM_BoxTrace( &trace, le->refEntity.origin, newOrigin, NULL, NULL, 0, MASK_SOLID );

				if ( trace.fraction < 1.0 ) {
					// make sure we are not heading away from the enemy too much
					VectorNormalize2( le->pos.trDelta, v2 );
					if ( DotProduct( v, v2 ) > 0.7 ) {
						// avoid world geometry
						backup = *le;
						le->bounceFactor = ( 1.0 - trace.fraction ) * 10.0 * (float)step / 1000.0;  // tracking and avoidance factor
						// reflect the velocity on the trace plane
						VectorMA( le->pos.trDelta, le->bounceFactor * oldSpeed, trace.plane.normal, le->pos.trDelta );
						if ( VectorLength( le->pos.trDelta ) < 1 ) {
							CG_FreeLocalEntity( le );
							return;
						}
						// the intersection is a fraction of the frametime
						le->pos.trTime = time;
						VectorCopy( le->refEntity.origin, le->pos.trBase );
						VectorNormalize( le->pos.trDelta );
						VectorScale( le->pos.trDelta, oldSpeed, le->pos.trDelta );
						//
						// double check end velocity
						VectorNormalize2( le->pos.trDelta, v2 );
						if ( DotProduct( v, v2 ) <= 0.2 ) {
							// restore
							*le = backup;
						}
					}
				}
			}
		}

		// set the angles
		VectorNormalize2( oDelta, v );
		// HACK!!! skull model is back-to-front, need to fix
		if ( le->leType == LE_ZOMBIE_SPIRIT /*|| le->leType == LE_HELGA_SPIRIT*/ ) {
			VectorInverse( v );
		}
		vectoangles( v, ang );
		AnglesToAxis( ang, le->refEntity.axis );
		// lean when turning
		if ( le->leType == LE_ZOMBIE_BAT || le->leType == LE_HELGA_SPIRIT ) {
			VectorSubtract( le->pos.trDelta, oDelta, v2 );
			ang[ROLL] = -0.5 * DotProduct( le->refEntity.axis[1], v2 );
			if ( fabs( ang[ROLL] ) < 20 ) {
				ang[ROLL] = 0;
			} else {
				if ( ang[ROLL] < 0 ) {
					ang[ROLL] += 20;
				} else {
					ang[ROLL] -= 20;
				}
			}
			if ( fabs( ang[ROLL] ) > 80 ) {
				if ( ang[ROLL] > 80 ) {
					ang[ROLL] = 80;
				} else { ang[ROLL] = -80;}
			}
			AnglesToAxis( ang, le->refEntity.axis );
		}

		// HACK: the skull is slightly higher than the origin
		if ( le->leType == LE_ZOMBIE_SPIRIT ) {
			// set the size scale
			for ( i = 0; i < 3; i++ )
				VectorScale( le->refEntity.axis[i], 0.35, le->refEntity.axis[i] );
			VectorMA( le->refEntity.origin, -10, le->refEntity.axis[2], le->refEntity.origin );
		}

		le->lastTrailTime = time;
		time += step;
	}

	// Bats, set the frame
	if ( le->leType == LE_ZOMBIE_BAT ) {
		#define BAT_ANIM_FRAMETIME  30
		le->refEntity.frame = ( cg.time / BAT_ANIM_FRAMETIME + 1 ) % 19;
		le->refEntity.oldframe = ( cg.time / BAT_ANIM_FRAMETIME ) % 19;
		le->refEntity.backlerp = 1.0 - ( (float)( cg.time % BAT_ANIM_FRAMETIME ) / (float)BAT_ANIM_FRAMETIME );
	}

	// add the sound
	if ( le->loopingSound ) {
		if ( cg.time > le->refEntity.fadeStartTime ) {
			trap_S_AddLoopingSound( 0, le->refEntity.origin, vec3_origin, le->loopingSound, 255 - (int)( 255.0 * (float)( cg.time - le->refEntity.fadeStartTime ) / (float)( le->refEntity.fadeEndTime - le->refEntity.fadeStartTime ) ) );
		} else {
			trap_S_AddLoopingSound( 0, le->refEntity.origin, vec3_origin, le->loopingSound, 255 - (int)( 255.0 * ( 1.0 - alpha ) ) );
		}
	}
/*
	if (le->leType == LE_HELGA_SPIRIT) {
		int cnt=1;
		float alpha;
		refEntity_t re;

		// add the "motion blur" ghosts
		re = le->refEntity;
		i = le->oldPosHead - 1;
		if (i < 0) i = MAX_OLD_POS-1;
		while (i != le->oldPosHead && le->validOldPos[i]) {
			alpha = 1.0 - ((float)cnt / (float)MAX_OLD_POS);
			if (alpha > 1.0) alpha = 1.0;
			if (alpha < 0.0) alpha = 0.0;

			re.shaderTime = le->refEntity.shaderTime - cnt*100;
			VectorCopy( le->oldPos[i], re.origin );
			re.shaderRGBA[3] = (unsigned char)(255.0 * alpha);
			trap_R_AddRefEntityToScene( &re );

			if (--i<0) i=MAX_OLD_POS-1;
			cnt++;
		}
	} else {
*/  trap_R_AddRefEntityToScene( &le->refEntity );
//	}

	// Bats, add the flame
	if ( le->leType == LE_ZOMBIE_BAT ) {
		//
		le->refEntity.shaderRGBA[3] = 255;
		VectorNormalize2( le->pos.trDelta, v );
		VectorInverse( v );
		v[2] += 1;
		VectorNormalize2( v, le->refEntity.fireRiseDir );

		le->refEntity.customShader = cgs.media.onFireShader2;
		trap_R_AddRefEntityToScene( &le->refEntity );
		le->refEntity.shaderTime = 1434;
		trap_R_AddRefEntityToScene( &le->refEntity );

		le->refEntity.customShader = 0;
		le->refEntity.shaderTime = 0;
	}
}

/*
================
CG_AddDebrisElements
================
*/
void CG_AddDebrisElements( localEntity_t *le ) {
	vec3_t newOrigin;
	trace_t trace;
	float lifeFrac;
	int t, step = 50;

	for ( t = le->lastTrailTime + step; t < cg.time; t += step ) {
		// calculate new position
		BG_EvaluateTrajectory( &le->pos, t, newOrigin );

		// trace a line from previous position to new position
		CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, MASK_SHOT );

		// if stuck, kill it
		if ( trace.startsolid ) {
			// HACK, some walls screw up, so just pass through if starting in a solid
			VectorCopy( newOrigin, trace.endpos );
			trace.fraction = 1.0;
		}

		// moved some distance
		VectorCopy( trace.endpos, le->refEntity.origin );

		// add a trail
		lifeFrac = (float)( t - le->startTime ) / (float)( le->endTime - le->startTime );

#if 0
		// fire
#if 1   // flame
		if ( le->effectWidth > 0 ) {
			le->headJuncIndex = CG_AddSparkJunc( le->headJuncIndex,
												 cgs.media.fireTrailShader,
												 le->refEntity.origin,
												 (int)( 500.0 * ( 0.5 + 0.5 * ( 1.0 - lifeFrac ) ) ), // trail life
												 1.0, // alpha
												 0.5, // end alpha
												 3, // start width
												 le->effectWidth ); // end width
#else   // spark line
		if ( le->effectWidth > 0 ) {
			le->headJuncIndex = CG_AddSparkJunc( le->headJuncIndex,
												 cgs.media.sparkParticleShader,
												 le->refEntity.origin,
												 (int)( 600.0 * ( 0.5 + 0.5 * ( 0.5 - lifeFrac ) ) ), // trail life
												 1.0 - lifeFrac * 2, // alpha
												 0.5 * ( 1.0 - lifeFrac ), // end alpha
												 5.0 * ( 1.0 - lifeFrac ), // start width
												 5.0 * ( 1.0 - lifeFrac ) ); // end width
#endif
		}
#endif

		// smoke
		if ( le->effectFlags & 1 ) {
			le->headJuncIndex2 = CG_AddSmokeJunc( le->headJuncIndex2,
												  cgs.media.smokeTrailShader,
												  le->refEntity.origin,
												  (int)( 2000.0 * ( 0.5 + 0.5 * ( 1.0 - lifeFrac ) ) ), // trail life
												  1.0 * ( trace.fraction == 1.0 ) * ( 0.5 + 0.5 * ( 1.0 - lifeFrac ) ), // alpha
												  1, // start width
												  (int)( 60.0 * ( 0.5 + 0.5 * ( 1.0 - lifeFrac ) ) ) ); // end width
		}

		// if it is in a nodrop zone, remove it
		// this keeps gibs from waiting at the bottom of pits of death
		// and floating levels
//		if ( trap_CM_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) {
//			CG_FreeLocalEntity( le );
//			return;
//		}

		if ( trace.fraction < 1.0 ) {
			// reflect the velocity on the trace plane
			CG_ReflectVelocity( le, &trace );
			if ( VectorLength( le->pos.trDelta ) < 1 ) {
				CG_FreeLocalEntity( le );
				return;
			}
			// the intersection is a fraction of the frametime
			le->pos.trTime = t;
		}

		le->lastTrailTime = t;
	}

}

// Rafael Shrapnel
/*
===============
CG_AddShrapnel
===============
*/
void CG_AddShrapnel( localEntity_t *le ) {
	vec3_t newOrigin;
	trace_t trace;

	if ( le->pos.trType == TR_STATIONARY ) {
		// sink into the ground if near the removal time
		int t;
		float oldZ;

		t = le->endTime - cg.time;
		if ( t < SINK_TIME ) {
			// we must use an explicit lighting origin, otherwise the
			// lighting would be lost as soon as the origin went
			// into the ground
			VectorCopy( le->refEntity.origin, le->refEntity.lightingOrigin );
			le->refEntity.renderfx |= RF_LIGHTING_ORIGIN;
			oldZ = le->refEntity.origin[2];
			le->refEntity.origin[2] -= 16 * ( 1.0 - (float)t / SINK_TIME );
			trap_R_AddRefEntityToScene( &le->refEntity );
			le->refEntity.origin[2] = oldZ;
		} else {
			trap_R_AddRefEntityToScene( &le->refEntity );
			CG_AddParticleShrapnel( le );
		}

		return;
	}

	// calculate new position
	BG_EvaluateTrajectory( &le->pos, cg.time, newOrigin );

	// trace a line from previous position to new position
	CG_Trace( &trace, le->refEntity.origin, NULL, NULL, newOrigin, -1, CONTENTS_SOLID );
	if ( trace.fraction == 1.0 ) {
		// still in free fall
		VectorCopy( newOrigin, le->refEntity.origin );

		if ( le->leFlags & LEF_TUMBLE ) {
			vec3_t angles;

			BG_EvaluateTrajectory( &le->angles, cg.time, angles );
			AnglesToAxis( angles, le->refEntity.axis );
		}

		trap_R_AddRefEntityToScene( &le->refEntity );
		CG_AddParticleShrapnel( le );
		return;
	}

	// if it is in a nodrop zone, remove it
	// this keeps gibs from waiting at the bottom of pits of death
	// and floating levels
	if ( trap_CM_PointContents( trace.endpos, 0 ) & CONTENTS_NODROP ) {
		CG_FreeLocalEntity( le );
		return;
	}

	// leave a mark
	CG_FragmentBounceMark( le, &trace );

	// do a bouncy sound
	CG_FragmentBounceSound( le, &trace );

	// reflect the velocity on the trace plane
	CG_ReflectVelocity( le, &trace );

	trap_R_AddRefEntityToScene( &le->refEntity );
	CG_AddParticleShrapnel( le );
}
// done.

/*
=====================================================================

TRIVIAL LOCAL ENTITIES

These only do simple scaling or modulation before passing to the renderer
=====================================================================
*/

/*
====================
CG_AddFadeRGB
====================
*/
void CG_AddFadeRGB( localEntity_t *le ) {
	refEntity_t *re;
	float c;

	re = &le->refEntity;

	c = ( le->endTime - cg.time ) * le->lifeRate;
	c *= 0xff;

	re->shaderRGBA[0] = le->color[0] * c;
	re->shaderRGBA[1] = le->color[1] * c;
	re->shaderRGBA[2] = le->color[2] * c;
	re->shaderRGBA[3] = le->color[3] * c;

	trap_R_AddRefEntityToScene( re );
}

/*
==================
CG_AddMoveScaleFade
==================
*/
static void CG_AddMoveScaleFade( localEntity_t *le ) {
	refEntity_t *re;
	float c;
	vec3_t delta;
	float len;

	re = &le->refEntity;

	// fade / grow time
//	c = ( le->endTime - cg.time ) * le->lifeRate;
	if ( le->fadeInTime > le->startTime && cg.time < le->fadeInTime ) {
		// fade / grow time
		c = 1.0 - (float) ( le->fadeInTime - cg.time ) / ( le->fadeInTime - le->startTime );
	} else {
		// fade / grow time
		c = ( le->endTime - cg.time ) * le->lifeRate;
	}

	// Ridah, spark
	if ( !( le->leFlags & LEF_NOFADEALPHA ) ) {
		// done.
		re->shaderRGBA[3] = 0xff * c * le->color[3];
	}

	if ( !( le->leFlags & LEF_PUFF_DONT_SCALE ) ) {
		c = ( le->endTime - cg.time ) * le->lifeRate;
		re->radius = le->radius * ( 1.0 - c ) + 8;
	}

	BG_EvaluateTrajectory( &le->pos, cg.time, re->origin );

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	trap_R_AddRefEntityToScene( re );
}


/*
===================
CG_AddScaleFade

For rocket smokes that hang in place, fade out, and are
removed if the view passes through them.
There are often many of these, so it needs to be simple.
===================
*/
static void CG_AddScaleFade( localEntity_t *le ) {
	refEntity_t *re;
	float c;
	vec3_t delta;
	float len;

	re = &le->refEntity;

	// fade / grow time
	c = ( le->endTime - cg.time ) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];
	if ( !( le->leFlags & LEF_PUFF_DONT_SCALE ) ) {
		re->radius = le->radius * ( 1.0 - c ) + 8;
	}

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	trap_R_AddRefEntityToScene( re );
}


/*
=================
CG_AddFallScaleFade

This is just an optimized CG_AddMoveScaleFade
For blood mists that drift down, fade out, and are
removed if the view passes through them.
There are often 100+ of these, so it needs to be simple.
=================
*/
static void CG_AddFallScaleFade( localEntity_t *le ) {
	refEntity_t *re;
	float c;
	vec3_t delta;
	float len;

	re = &le->refEntity;

	// fade time
	c = ( le->endTime - cg.time ) * le->lifeRate;

	re->shaderRGBA[3] = 0xff * c * le->color[3];

	re->origin[2] = le->pos.trBase[2] - ( 1.0 - c ) * le->pos.trDelta[2];

	re->radius = le->radius * ( 1.0 - c ) + 16;

	// if the view would be "inside" the sprite, kill the sprite
	// so it doesn't add too much overdraw
	VectorSubtract( re->origin, cg.refdef.vieworg, delta );
	len = VectorLength( delta );
	if ( len < le->radius ) {
		CG_FreeLocalEntity( le );
		return;
	}

	trap_R_AddRefEntityToScene( re );
}



/*
================
CG_AddExplosion
================
*/
static void CG_AddExplosion( localEntity_t *ex ) {
	refEntity_t *ent;

	ent = &ex->refEntity;

	// add the entity
	// RF, don't add if shader is invalid
	if ( ent->customShader >= 0 ) {
		trap_R_AddRefEntityToScene( ent );
	}

	// add the dlight
	if ( ex->light ) {
		float light;

		light = (float)( cg.time - ex->startTime ) / ( ex->endTime - ex->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = ex->light * light;
		trap_R_AddLightToScene( ent->origin, light, ex->lightColor[0], ex->lightColor[1], ex->lightColor[2], 0 );
	}
}

/*
================
CG_AddSpriteExplosion
================
*/
static void CG_AddSpriteExplosion( localEntity_t *le ) {
	refEntity_t re;
	float c;

	re = le->refEntity;

	c = ( le->endTime - cg.time ) / ( float ) ( le->endTime - le->startTime );
	if ( c > 1 ) {
		c = 1.0;    // can happen during connection problems
	}

	re.shaderRGBA[0] = 0xff;
	re.shaderRGBA[1] = 0xff;
	re.shaderRGBA[2] = 0xff;
	re.shaderRGBA[3] = 0xff * c * 0.33;

	re.reType = RT_SPRITE;
	re.radius = 42 * ( 1.0 - c ) + 30;

	// Ridah, move away from surface
	VectorMA( le->pos.trBase, ( 1.0 - c ), le->pos.trDelta, re.origin );
	// done.

	// RF, don't add if shader is invalid
	if ( re.customShader >= 0 ) {
		trap_R_AddRefEntityToScene( &re );
	}

	// add the dlight
	if ( le->light ) {
		float light;

		// Ridah, modified this so the light fades out rather than shrinking
		/*
		light = (float)( cg.time - le->startTime ) / ( le->endTime - le->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		light = le->light * light;
		trap_R_AddLightToScene(re.origin, light, le->lightColor[0], le->lightColor[1], le->lightColor[2], 0 );
		*/
		light = (float)( cg.time - le->startTime ) / ( le->endTime - le->startTime );
		if ( light < 0.5 ) {
			light = 1.0;
		} else {
			light = 1.0 - ( light - 0.5 ) * 2;
		}
		trap_R_AddLightToScene( re.origin, le->light, light * le->lightColor[0], light * le->lightColor[1], light * le->lightColor[2], 0 );
		// done.
	}
}

//==============================================================================

/*
===================
CG_AddLocalEntities

===================
*/
void CG_AddLocalEntities( void ) {
	localEntity_t   *le, *next;

	cg.viewFade = 0.0;

	// walk the list backwards, so any new local entities generated
	// (trails, marks, etc) will be present this frame
	le = cg_activeLocalEntities.prev;
	for ( ; le != &cg_activeLocalEntities ; le = next ) {
		// grab next now, so if the local entity is freed we
		// still have it
		next = le->prev;

		if ( cg.time >= le->endTime ) {
			CG_FreeLocalEntity( le );
			continue;
		}
		switch ( le->leType ) {
		default:
			CG_Error( "Bad leType: %i", le->leType );
			break;

			// Ridah
		case LE_MOVING_TRACER:
			CG_AddMovingTracer( le );
			break;
		case LE_SPARK:
			CG_AddSparkElements( le );
			break;
		case LE_FUSE_SPARK:
			CG_AddFuseSparkElements( le );
			break;
		case LE_DEBRIS:
			CG_AddDebrisElements( le );
			break;
		case LE_BLOOD:
			CG_AddBloodElements( le );
			break;
		case LE_HELGA_SPIRIT:
		case LE_ZOMBIE_SPIRIT:
		case LE_ZOMBIE_BAT:
			CG_AddClientCritter( le );
			break;
		case LE_SPIRIT_VIEWFLASH:
			CG_AddSpiritViewflash( le );
			// done.

		case LE_MARK:
			break;

		case LE_SPRITE_EXPLOSION:
			CG_AddSpriteExplosion( le );
			break;

		case LE_EXPLOSION:
			CG_AddExplosion( le );
			break;

		case LE_FRAGMENT:           // gibs and brass
			CG_AddFragment( le );
			break;

		case LE_MOVE_SCALE_FADE:        // water bubbles
			CG_AddMoveScaleFade( le );
			break;

		case LE_FADE_RGB:               // teleporters, railtrails
			CG_AddFadeRGB( le );
			break;

		case LE_FALL_SCALE_FADE: // gib blood trails
			CG_AddFallScaleFade( le );
			break;

		case LE_SCALE_FADE:     // rocket trails
			CG_AddScaleFade( le );
			break;

		case LE_EMITTER:
			CG_AddEmitter( le );
			break;

		}
	}
}

