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


// cg_flamethrower.c - special code for the flamethrower effects
//
//	the flameChunks behave similarly to the trailJunc's, except they are rendered differently, and
//	also interact with the environment
//
// NOTE: some AI's are treated different, mostly for aesthetical reasons.

#include "cg_local.h"

// a flameChunk is a ball or section of fuel which goes from fuel->blue ignition->flame ball
// optimization is necessary, since lots of these will be spawned, but as they grow, they can be
// merged so that less overdraw occurs
typedef struct flameChunk_s
{
	struct flameChunk_s *nextGlobal, *prevGlobal;   // next junction in the global list it is in (free or used)
	struct flameChunk_s *nextFlameChunk;            // next junction in the trail
	struct flameChunk_s *nextHead, *prevHead;       // next head junc in the world

	qboolean inuse;
	qboolean dead;          // set when a chunk is effectively inactive, but waiting to be freed
	int ownerCent;              // cent that spawned us

	int timeStart, timeEnd;
	float sizeMax;                  // start small, increase if we slow down
	float sizeRand;
	float sizeRate;                 // rate per ms, variable according to speed (larger if moving slower)
	vec3_t baseOrg;
	int baseOrgTime;
	vec3_t velDir;
	float velSpeed;                 // flame chunks should start with a fast velocity, then slow down if there is nothing behind them pushing them along
	float rollAngle;
	qboolean ignitionOnly;
	int blueLife;
	float gravity;
	vec3_t startVelDir;
	float speedScale;

	// current variables
	vec3_t org;
	float size;
	float lifeFrac;                 // 0.0 (baby) -> 1.0 (aged)

	int lastFriction, lastFrictionTake;
	vec3_t parentFwd;
} flameChunk_t;

// DHM - Nerve :: lowered this from 2048.  Still allows 6-9 people flaming.
#define MAX_FLAME_CHUNKS    1024
static flameChunk_t flameChunks[MAX_FLAME_CHUNKS];
static flameChunk_t *freeFlameChunks, *activeFlameChunks, *headFlameChunks;

static qboolean initFlameChunks = qfalse;

static int numFlameChunksInuse;

// this structure stores information relevant to each cent in the game, this way we keep
// the flamethrower data seperate to the rest of the code, which helps if we decide against
// using this weapon in the game
typedef struct centFlameInfo_s
{
	int lastClientFrame;            // client frame that we last fired the flamethrower
	vec3_t lastAngles;              // angles at last firing
	vec3_t lastOrigin;              // origin at last firing
	flameChunk_t
	*lastFlameChunk;                // flame chunk we last spawned
	int lastSoundUpdate;

	qboolean lastFiring;

	int lastDmgUpdate;              // time we last told server about this ent's flame damage
	int lastDmgCheck;               // only check once per 100ms
	int lastDmgEnemy;               // entity that inflicted the damage
} centFlameInfo_t;

static centFlameInfo_t centFlameInfo[MAX_GENTITIES];

typedef struct
{
//	float fireVolume;	// not needed, since we add individual loop sources for the flame, so it gets spacialized
	float blowVolume;
	float streamVolume;
} flameSoundStatus_t;

static flameSoundStatus_t centFlameStatus[MAX_GENTITIES];

// procedure defs
flameChunk_t *CG_SpawnFlameChunk( flameChunk_t *headFlameChunk );
void CG_FlameCalcOrg( flameChunk_t *f, int time, vec3_t outOrg );
void CG_FlameGetMuzzlePoint( vec3_t org, vec3_t fwd, vec3_t right, vec3_t up, vec3_t outPos );

// these must be globals, since they cannot expand or contract, since that might result in them getting
//	stuck in geometry. therefore when a chunk hits a surface, we should deflect it away from the surface
//	slightly, rather than running along it, so that as the chink grows, the sprites don't sink into the
//	wall too much.
static vec3_t flameChunkMins = {-4, -4, -4};
static vec3_t flameChunkMaxs = { 4,  4,  4};

// these define how the flame looks
#define FLAME_START_SIZE        1.0
#define FLAME_START_MAX_SIZE    140.0   // when the flame is spawned, it should endevour to reach this size
#define FLAME_START_MAX_SIZE_RAND   60.0
#define FLAME_MAX_SIZE          200.0   // flame sprites cannot be larger than this
#define FLAME_MIN_MAXSIZE       40.0    // don't ever let the sizeMax go less than this
#define FLAME_START_SPEED       1200.0 //1200.0	// speed of flame as it leaves the nozzle
#define FLAME_MIN_SPEED         60.0 //200.0
#define FLAME_CHUNK_DIST        4.0     // space in between chunks when fired

#define FLAME_BLUE_LENGTH       130.0
#define FLAME_BLUE_MAX_ALPHA    1.0

#define FLAME_FUEL_LENGTH       48.0
#define FLAME_FUEL_MAX_ALPHA    0.35
#define FLAME_FUEL_MIN_WIDTH    1.0

// these are calculated (don't change)
#define FLAME_LENGTH            ( FLAMETHROWER_RANGE + 50.0 ) // NOTE: only modify the range, since this should always reflect that range

#define FLAME_LIFETIME          (int)( ( FLAME_LENGTH / FLAME_START_SPEED ) * 1000 )    // life duration in milliseconds
#define FLAME_FRICTION_PER_SEC  ( 2.0 * FLAME_START_SPEED )
#define FLAME_BLUE_LIFE         (int)( ( FLAME_BLUE_LENGTH / FLAME_START_SPEED ) * 1000 )
#define FLAME_FUEL_LIFE         (int)( ( FLAME_FUEL_LENGTH / FLAME_START_SPEED ) * 1000 )
#define FLAME_FUEL_FADEIN_TIME  ( 0.2 * FLAME_FUEL_LIFE )

#define FLAME_BLUE_FADEIN_TIME( x )       ( 0.2 * x )
#define FLAME_BLUE_FADEOUT_TIME( x )      ( 0.05 * x )
#define GET_FLAME_BLUE_SIZE_SPEED( x )    ( ( (float)x / FLAME_LIFETIME ) / 1.0 ) // x is the current sizeMax
#define GET_FLAME_SIZE_SPEED( x )         ( ( (float)x / FLAME_LIFETIME ) / 0.3 ) // x is the current sizeMax

//#define	FLAME_MIN_DRAWSIZE		20

// enable this for the fuel stream
//#define FLAME_ENABLE_FUEL_STREAM

// enable this for dynamic lighting around flames
//#define FLAMETHROW_LIGHTS

// disable this to stop rotating flames (this is variable so we can change it at run-time)
int rotatingFlames = qtrue;

/*
===============
CG_FlameLerpVec
===============
*/
void CG_FlameLerpVec( const vec3_t oldV, const vec3_t newV, float backLerp, vec3_t outV ) {
	VectorScale( newV, ( 1.0 - backLerp ), outV );
	VectorMA( outV, backLerp, oldV, outV );
}

/*
===============
CG_FlameAdjustSpeed
===============
*/
void CG_FlameAdjustSpeed( flameChunk_t *f, float change ) {
	if ( !f->velSpeed && !change ) {
		return;
	}

	f->velSpeed += change;
	if ( f->velSpeed < FLAME_MIN_SPEED ) {
		f->velSpeed = FLAME_MIN_SPEED;
	}
}

/*
===============
CG_FireFlameChunks

  The given entity is firing a flamethrower
===============
*/
void CG_FireFlameChunks( centity_t *cent, vec3_t origin, vec3_t angles, float speedScale, qboolean firing ) {
	centFlameInfo_t *centInfo;
	flameChunk_t *f, *of;
	vec3_t lastFwd, thisFwd, fwd;
	vec3_t lastUp, thisUp, up;
	vec3_t lastRight, thisRight, right;
	vec3_t thisOrg, lastOrg, org;
	double timeInc, backLerp, fracInc;
	int t, numFrameChunks;
	double ft;
	trace_t trace;
	vec3_t parentFwd;

	centInfo = &centFlameInfo[cent->currentState.number];

	// for any other character or in 3rd person view, use entity angles for friction
	if ( cent->currentState.number != cg.snap->ps.clientNum || cg_thirdPerson.integer ) {
		AngleVectors( cent->currentState.angles, parentFwd, NULL, NULL );
	} else {
		AngleVectors( angles, parentFwd, NULL, NULL );
	}

	AngleVectors( angles, thisFwd, thisRight, thisUp );
	VectorCopy( origin, thisOrg );

	// if this entity was firing last frame, interpolate the angles as we spawn the chunks that
	// fired over the last frame
	if (    ( centInfo->lastClientFrame == cent->currentState.frame ) &&
			( centInfo->lastFlameChunk && centInfo->lastFiring == firing ) ) {
		AngleVectors( centInfo->lastAngles, lastFwd, lastRight, lastUp );
		VectorCopy( centInfo->lastOrigin, lastOrg );
		centInfo->lastFiring = firing;

		of = centInfo->lastFlameChunk;
		timeInc = 1000.0 * ( firing ? 1.0 : 0.5 ) * ( FLAME_CHUNK_DIST / ( FLAME_START_SPEED * speedScale ) );
		ft = ( (double)of->timeStart + timeInc );
		t = (int)ft;
		fracInc = timeInc / (double)( cg.time - of->timeStart );
		backLerp = 1.0 - fracInc;

		numFrameChunks = 0;         // CHANGE: id

		while ( t <= cg.time ) {
			// spawn a new chunk
			CG_FlameLerpVec( lastOrg, thisOrg, backLerp, org );

			CG_Trace( &trace, org, flameChunkMins, flameChunkMaxs, org, cent->currentState.number, MASK_SHOT | MASK_WATER ); // JPW NERVE water fixes

			if ( trace.startsolid ) {
				return;     // don't spawn inside a wall

			}
			f = CG_SpawnFlameChunk( of );

			if ( !f ) {
				//CG_Printf( "Out of flame chunks\n" );
				// CHANGE: id
				// to make sure we do not keep trying to add more and more chunks
				centInfo->lastFlameChunk->timeStart = cg.time;
				// end CHANGE: id
				return;
			}

			CG_FlameLerpVec( lastFwd, thisFwd, backLerp, fwd );
			VectorNormalize( fwd );
			CG_FlameLerpVec( lastRight, thisRight, backLerp, right );
			VectorNormalize( right );
			CG_FlameLerpVec( lastUp, thisUp, backLerp, up );
			VectorNormalize( up );

			f->timeStart = t;
			f->timeEnd = t + FLAME_LIFETIME * ( 1.0 / ( 0.5 + 0.5 * speedScale ) );
			f->size = FLAME_START_SIZE * speedScale;
			f->sizeMax = speedScale * ( FLAME_START_MAX_SIZE + f->sizeRand * ( firing ? 1.0 : 0.0 ) );
			f->sizeRand = 0;

			if ( f->sizeMax > FLAME_MAX_SIZE ) {
				f->sizeMax = FLAME_MAX_SIZE;
			}

			f->sizeRate = GET_FLAME_BLUE_SIZE_SPEED( f->sizeMax * speedScale * ( 1.0 + ( 0.5 * (float)!firing ) ) );
			VectorCopy( org, f->baseOrg );
			f->baseOrgTime = t;
			VectorCopy( fwd, f->velDir );
			VectorCopy( fwd, f->startVelDir );
			f->speedScale = speedScale;

			VectorNormalize( f->velDir );
			f->velSpeed = FLAME_START_SPEED * ( 0.5 + 0.5 * speedScale ) * ( firing ? 1.0 : 4.5 );
			f->ownerCent = cent->currentState.number;
			f->rollAngle = crandom() * 179;
			f->ignitionOnly = !firing;

			if ( !firing ) {
				f->gravity = -150;
				f->blueLife = FLAME_BLUE_LIFE * 0.1;
			} else {
				f->gravity = 0;
				f->blueLife = FLAME_BLUE_LIFE;
			}
			f->lastFriction = cg.time;
			f->lastFrictionTake = cg.time;
			VectorCopy( parentFwd, f->parentFwd );

			ft += timeInc;
			// always spawn a chunk right on the current time
			if ( (int)ft > cg.time && t < cg.time ) {
				ft = (double)cg.time;
				backLerp = fracInc; // so it'll get set to zero a few lines down
			}
			t = (int)ft;
			backLerp -= fracInc;
			centInfo->lastFlameChunk = of = f;
			// CHANGE: id
			// don't spawn too many chunks each frame
			if ( ++numFrameChunks > 50 ) {
				// to make sure we do not keep trying to add more and more chunks
				centInfo->lastFlameChunk->timeStart = cg.time;
				break;
			}
			// end CHANGE: id
		}
	} else {
		centInfo->lastFiring = firing;

		// just fire a single chunk to get us started
		f = CG_SpawnFlameChunk( NULL );

		if ( !f ) {
			//CG_Printf( "Out of flame chunks\n" );
			return;
		}

		VectorCopy( thisOrg, org );
		VectorCopy( thisFwd, fwd );
		VectorCopy( thisUp, up );
		VectorCopy( thisRight, right );

		f->timeStart = cg.time;
		f->timeEnd = cg.time + FLAME_LIFETIME * ( 1.0 / ( 0.5 + 0.5 * speedScale ) );
		f->size = FLAME_START_SIZE * speedScale;
		f->sizeMax = FLAME_START_MAX_SIZE * speedScale;
		if ( f->sizeMax > FLAME_MAX_SIZE ) {
			f->sizeMax = FLAME_MAX_SIZE;
		}

		f->sizeRand = 0;
		f->sizeRate = GET_FLAME_BLUE_SIZE_SPEED( f->sizeMax * speedScale );
		VectorCopy( org, f->baseOrg );
		f->baseOrgTime = cg.time;
		VectorCopy( fwd, f->velDir );
		VectorCopy( fwd, f->startVelDir );
		f->velSpeed = FLAME_START_SPEED * ( 0.5 + 0.5 * speedScale );
		f->ownerCent = cent->currentState.number;
		f->rollAngle = crandom() * 179;
		f->ignitionOnly = !firing;
		f->speedScale = speedScale;
		if ( !firing ) {
			f->gravity = -100;
			f->blueLife = (int)( 0.3 * ( 1.0 / speedScale ) * (float)FLAME_BLUE_LIFE );
		} else {
			f->gravity = 0;
			f->blueLife = FLAME_BLUE_LIFE;
		}
		f->lastFriction = cg.time;
		f->lastFrictionTake = cg.time;
		VectorCopy( parentFwd, f->parentFwd );

		centInfo->lastFlameChunk = f;
	}

	// push them along
	/*
	f = centInfo->lastFlameChunk;
	while (f) {

		if (f->lastFriction < cg.time - 50) {
			frametime = (float)(cg.time - f->lastFriction) / 1000.0;
			f->lastFriction = cg.time;
			dot = DotProduct(parentFwd, f->parentFwd);
			if (dot >= 0.99) {
				dot -= 0.99;
				dot *= (1.0/(1.0-0.99));
				CG_FlameAdjustSpeed( f, 0.5 * frametime * FLAME_FRICTION_PER_SEC * pow(dot,4) );
			}
		}

		f = f->nextFlameChunk;
	}
	*/

	VectorCopy( angles, centInfo->lastAngles );
	VectorCopy( origin, centInfo->lastOrigin );
	centInfo->lastClientFrame = cent->currentState.frame;
}

/*
===============
CG_ClearFlameChunks
===============
*/
void CG_ClearFlameChunks( void ) {
	int i;

	memset( flameChunks, 0, sizeof( flameChunks ) );
	memset( centFlameInfo, 0, sizeof( centFlameInfo ) );

	freeFlameChunks = flameChunks;
	activeFlameChunks = NULL;
	headFlameChunks = NULL;

	for ( i = 0 ; i < MAX_FLAME_CHUNKS ; i++ )
	{
		flameChunks[i].nextGlobal = &flameChunks[i + 1];

		if ( i > 0 ) {
			flameChunks[i].prevGlobal = &flameChunks[i - 1];
		} else {
			flameChunks[i].prevGlobal = NULL;
		}

		flameChunks[i].inuse = qfalse;
	}
	flameChunks[MAX_FLAME_CHUNKS - 1].nextGlobal = NULL;

	initFlameChunks = qtrue;
	numFlameChunksInuse = 0;
}

/*
===============
CG_SpawnFlameChunk
===============
*/
flameChunk_t *CG_SpawnFlameChunk( flameChunk_t *headFlameChunk ) {
	flameChunk_t    *f;

	if ( !freeFlameChunks ) {
		return NULL;
	}

	if ( headFlameChunks && headFlameChunks->dead ) {
		headFlameChunks = NULL;
	}

	// select the first free trail, and remove it from the list
	f = freeFlameChunks;
	freeFlameChunks = f->nextGlobal;
	if ( freeFlameChunks ) {
		freeFlameChunks->prevGlobal = NULL;
	}

	f->nextGlobal = activeFlameChunks;
	if ( activeFlameChunks ) {
		activeFlameChunks->prevGlobal = f;
	}
	activeFlameChunks = f;
	f->prevGlobal = NULL;
	f->inuse = qtrue;
	f->dead = qfalse;

	// if this owner has a headJunc, add us to the start
	if ( headFlameChunk ) {
		// remove the headJunc from the list of heads
		if ( headFlameChunk == headFlameChunks ) {
			headFlameChunks = headFlameChunks->nextHead;
			if ( headFlameChunks ) {
				headFlameChunks->prevHead = NULL;
			}
		} else {
			if ( headFlameChunk->nextHead ) {
				headFlameChunk->nextHead->prevHead = headFlameChunk->prevHead;
			}
			if ( headFlameChunk->prevHead ) {
				headFlameChunk->prevHead->nextHead = headFlameChunk->nextHead;
			}
		}
		headFlameChunk->prevHead = NULL;
		headFlameChunk->nextHead = NULL;
	}
	// make us the headTrail
	if ( headFlameChunks ) {
		headFlameChunks->prevHead = f;
	}
	f->nextHead = headFlameChunks;
	f->prevHead = NULL;
	headFlameChunks = f;

	f->nextFlameChunk = headFlameChunk; // if headJunc is NULL, then we'll just be the end of the list

	numFlameChunksInuse++;

	// debugging
//	if ( cg_drawRewards.integer > 1 )
//		CG_Printf( "NumFlameChunks: %i\n", numFlameChunksInuse );

	return f;
}

/*
===========
CG_FreeFlameChunk
===========
*/
void CG_FreeFlameChunk( flameChunk_t *f ) {
	// kill any juncs after us, so they aren't left hanging
	if ( f->nextFlameChunk ) {
		CG_FreeFlameChunk( f->nextFlameChunk );
		f->nextFlameChunk = NULL;
	}

	// make it non-active
	f->inuse = qfalse;
	f->dead = qfalse;
	if ( f->nextGlobal ) {
		f->nextGlobal->prevGlobal = f->prevGlobal;
	}
	if ( f->prevGlobal ) {
		f->prevGlobal->nextGlobal = f->nextGlobal;
	}
	if ( f == activeFlameChunks ) {
		activeFlameChunks = f->nextGlobal;
	}

	// if it's a head, remove it
	if ( f == headFlameChunks ) {
		headFlameChunks = f->nextHead;
	}
	if ( f->nextHead ) {
		f->nextHead->prevHead = f->prevHead;
	}
	if ( f->prevHead ) {
		f->prevHead->nextHead = f->nextHead;
	}
	f->nextHead = NULL;
	f->prevHead = NULL;

	// stick it in the free list
	f->prevGlobal = NULL;
	f->nextGlobal = freeFlameChunks;
	if ( freeFlameChunks ) {
		freeFlameChunks->prevGlobal = f;
	}
	freeFlameChunks = f;

	numFlameChunksInuse--;
}

/*
===============
CG_MergeFlameChunks

  Assumes f1 comes before f2
===============
*/
void CG_MergeFlameChunks( flameChunk_t *f1, flameChunk_t *f2 ) {
	if ( f1->nextFlameChunk != f2 ) {
		CG_Error( "CG_MergeFlameChunks: f2 doesn't follow f1, cannot merge\n" );
	}

	f1->nextFlameChunk = f2->nextFlameChunk;
	f2->nextFlameChunk = NULL;

	VectorCopy( f2->velDir, f1->velDir );

	VectorCopy( f2->baseOrg, f1->baseOrg );
	f1->baseOrgTime = f2->baseOrgTime;

	f1->velSpeed = f2->velSpeed;
	f1->sizeMax = f2->sizeMax;
	f1->size = f2->size;
	f1->timeStart = f2->timeStart;
	f1->timeEnd = f2->timeEnd;

	CG_FreeFlameChunk( f2 );
}

/*
===============
CG_FlameCalcOrg
===============
*/
void CG_FlameCalcOrg( flameChunk_t *f, int time, vec3_t outOrg ) {
	VectorMA( f->baseOrg, f->velSpeed * ( (float)( time - f->baseOrgTime ) / 1000 ), f->velDir, outOrg );
	//outOrg[2] -= f->gravity * ((float)(time - f->timeStart)/1000.0) * ((float)(time - f->timeStart)/1000.0);
}

/*
===============
CG_MoveFlameChunk
===============
*/
void CG_MoveFlameChunk( flameChunk_t *f ) {
	vec3_t newOrigin, sOrg;
	trace_t trace;
	int jiggleCount;
	float dot;
	// TTimo: unused
	//static vec3_t	umins = {-1,-1,-1};
	//static vec3_t	umaxs = { 1, 1, 1};

	// subtract friction from speed
	if ( f->velSpeed > 1 && f->lastFrictionTake < cg.time - 50 ) {
		CG_FlameAdjustSpeed( f, -( (float)( cg.time - f->lastFrictionTake ) / 1000.0 ) * FLAME_FRICTION_PER_SEC );
		f->lastFrictionTake = cg.time;
	}

	// adjust size
	if ( f->size < f->sizeMax ) {
		if ( ( cg.time - f->timeStart ) < f->blueLife ) {
			f->sizeRate = GET_FLAME_BLUE_SIZE_SPEED( FLAME_START_MAX_SIZE );  // use a constant so the blue flame doesn't distort
		} else {
			f->sizeRate = GET_FLAME_SIZE_SPEED( f->sizeMax );
		}

		f->size += f->sizeRate * (float)( cg.time - f->baseOrgTime );
		if ( f->size > f->sizeMax ) {
			f->size = f->sizeMax;
		}
	}

	jiggleCount = 0;
	VectorCopy( f->baseOrg, sOrg );
	while ( f->velSpeed > 1 ) {
		CG_FlameCalcOrg( f, cg.time, newOrigin );

		// trace a line from previous position to new position
		CG_Trace( &trace, sOrg, flameChunkMins, flameChunkMaxs, newOrigin, f->ownerCent, MASK_SHOT | MASK_WATER ); // JPW NERVE water fixes

		if ( trace.startsolid ) {
			f->velSpeed = 0;
			f->dead = 1; // JPW NERVE water fixes
			break;
		}

		if ( trace.surfaceFlags & SURF_NOIMPACT ) {
			break;
		}

		// moved some distance
		VectorCopy( trace.endpos, f->baseOrg );
		f->baseOrgTime += (int)( (float)( cg.time - f->baseOrgTime ) * trace.fraction );

		if ( trace.fraction == 1.0 ) {
			// check for hitting client
			if ( ( f->ownerCent != cg.snap->ps.clientNum ) && !( cg.snap->ps.eFlags & EF_DEAD ) && VectorDistance( newOrigin, cg.snap->ps.origin ) < 32 ) {
				VectorNegate( f->velDir, trace.plane.normal );
			} else {
				break;
			}
		}

		// reflect off surface
		dot = DotProduct( f->velDir, trace.plane.normal );
		VectorMA( f->velDir, -2 * dot, trace.plane.normal, f->velDir );
		VectorNormalize( f->velDir );
		// subtract some speed
		f->velSpeed *= 0.5 * ( 0.25 + 0.75 * ( ( dot + 1.0 ) * 0.5 ) );
		VectorCopy( f->velDir, f->parentFwd );

		VectorCopy( f->baseOrg, sOrg );
	}

	CG_FlameCalcOrg( f, cg.time, f->org );
	f->baseOrgTime = cg.time;   // incase we skipped the movement
}

/*
===============
CG_AddFlameSpriteToScene
===============
*/
static vec3_t vright, vup;
static vec3_t rright, rup;

#ifdef _DEBUG   // just in case we forget about it, but it should be disabled at all times (only enabled to generate updated shaders)
#ifdef ALLOW_GEN_SHADERS    // secondary security measure

//#define	GEN_FLAME_SHADER

#endif  // ALLOW_GEN_SHADERS
#endif  // _DEBUG

#define FLAME_BLEND_SRC     "GL_ONE"
#define FLAME_BLEND_DST     "GL_ONE_MINUS_SRC_COLOR"

#define NUM_FLAME_SPRITES       45
#define FLAME_SPRITE_DIR        "twiltb2"

#define NUM_NOZZLE_SPRITES  8

static qhandle_t flameShaders[NUM_FLAME_SPRITES];
static qhandle_t nozzleShaders[NUM_NOZZLE_SPRITES];
static qboolean initFlameShaders = qtrue;

#define MAX_CLIPPED_FLAMES  8       // dont draw more than this many per frame
static int numClippedFlames;


void CG_AddFlameSpriteToScene( flameChunk_t *f, float lifeFrac, float alpha ) {
	vec3_t point, p2, projVec, sProj;
	polyVert_t verts[4];
	float radius, sdist, x;
	int frameNum;
	vec3_t vec, rotate_ang;
	unsigned char alphaChar;
	vec2_t fovRadius;
	vec2_t rdist, rST;
	int rollAngleClamped;
	static vec3_t lastPos;

	// DHM - Nerve :: No client side damage
	//CG_FlameDamage( f->ownerCent, f->org, f->size );

	if ( alpha < 0 ) {
		return; // we dont want to see this

	}
	radius = ( f->size / 2.0 );
	if ( radius < 6 ) {
		radius = 6;
	}
	rST[0] = radius * 1.0;
	rST[1] = radius * 1.0 / 1.481;
	alphaChar = ( unsigned char )( 255.0 * alpha );

	verts[0].modulate[0] = alphaChar;
	verts[0].modulate[1] = alphaChar;
	verts[0].modulate[2] = alphaChar;
	verts[0].modulate[3] = alphaChar;
	verts[1] = verts[0];
	verts[2] = verts[0];
	verts[3] = verts[0];

	// find the projected distance from the eye to the projection of the flame origin
	// onto the view direction vector
	VectorMA( cg.refdef.vieworg, 1024, cg.refdef.viewaxis[0], p2 );
	ProjectPointOntoVector( f->org, cg.refdef.vieworg, p2, sProj );

	// make sure its infront of us
	VectorSubtract( sProj, cg.refdef.vieworg, vec );
	sdist = VectorNormalize( vec );
	if ( !sdist || DotProduct( vec, cg.refdef.viewaxis[0] ) < 0 ) {
		return;
	}

	// if we are "inside" this sprite, clip it to our view frustum
	if ( sdist < f->size * 0.6 ) {
		// clip the sprite to the viewport, avoiding rendering off-screen pixels which
		// is the main cause of slow-downs

		if ( ( sdist < f->size * 0.6 ) && ( numClippedFlames++ > MAX_CLIPPED_FLAMES ) ) {
			return;
		}

		// OPTIMIZATION: clamp to 90 degree rotations
		rollAngleClamped = 0;
		if ( ( rotatingFlames ) && ( !( cg_fxflags & 1 ) ) ) { // JPW NERVE no rotate for alt flame shaders
			vectoangles( cg.refdef.viewaxis[0], rotate_ang );
			rotate_ang[ROLL] += ( rollAngleClamped = ( (int)( f->rollAngle ) / 90 ) * 90 );
			AngleVectors( rotate_ang, NULL, rright, rup );
		} else {
			VectorCopy( vright, rright );
			VectorCopy( vup, rup );
		}

		VectorSubtract( sProj, f->org, projVec );

		// find the distances from the sprite origin to the projection along the refdef axis
		rdist[0] = -1.0 * DotProduct( rright, projVec );
		rdist[1] = -1.0 * DotProduct( rup, projVec );

		if ( fabs( rdist[0] ) > radius || fabs( rdist[1] ) > radius ) {
			return; // completely off-screen

		}
		// now set the bounds for clipping
		fovRadius[0] = tan( DEG2RAD( ( rollAngleClamped % 2 == 0 ? cg.refdef.fov_x : cg.refdef.fov_x ) * 0.52 ) ) * sdist;
		fovRadius[1] = tan( DEG2RAD( ( rollAngleClamped % 2 == 0 ? cg.refdef.fov_x : cg.refdef.fov_x ) * 0.52 ) ) * sdist;

		// BOTTOM LEFT
		x = ( -radius + rdist[0] );
		if ( x < -fovRadius[0] ) {    // clip
			VectorMA( f->org, -radius + ( -fovRadius[0] - x ), rright, point );
			verts[0].st[0] = 0.0 + ( -fovRadius[0] - x ) / ( radius * 2 );
		} else {
			VectorMA( f->org, -radius, rright, point );
			verts[0].st[0] = 0.0;
		}
		x = ( -radius + rdist[1] );
		if ( x < -fovRadius[1] ) {
			VectorMA( point, -radius + ( -fovRadius[1] - x ), rup, point );
			verts[0].st[1] = 0.0 + ( -fovRadius[1] - x ) / ( radius * 2 );
		} else {
			VectorMA( point, -radius, rup, point );
			verts[0].st[1] = 0.0;
		}
		VectorCopy( point, verts[0].xyz );

		// TOP LEFT
		x = ( -radius + rdist[0] );
		if ( x < -fovRadius[0] ) {    // clip
			VectorMA( f->org, -radius + ( -fovRadius[0] - x ), rright, point );
			verts[1].st[0] = 0.0 + ( -fovRadius[0] - x ) / ( radius * 2 );
		} else {
			VectorMA( f->org, -radius, rright, point );
			verts[1].st[0] = 0.0;
		}
		x = ( radius + rdist[1] );
		if ( x > fovRadius[1] ) {
			VectorMA( point, radius + ( fovRadius[1] - x ), rup, point );
			verts[1].st[1] = 1.0 + ( fovRadius[1] - x ) / ( radius * 2 );
		} else {
			VectorMA( point, radius, rup, point );
			verts[1].st[1] = 1.0;
		}
		VectorCopy( point, verts[1].xyz );

		// TOP RIGHT
		x = ( radius + rdist[0] );
		if ( x > fovRadius[0] ) {
			VectorMA( f->org, radius + ( fovRadius[0] - x ), rright, point );
			verts[2].st[0] = 1.0 + ( fovRadius[0] - x ) / ( radius * 2 );
		} else {
			VectorMA( f->org, radius, rright, point );
			verts[2].st[0] = 1.0;
		}
		x = ( radius + rdist[1] );
		if ( x > fovRadius[1] ) {
			VectorMA( point, radius + ( fovRadius[1] - x ), rup, point );
			verts[2].st[1] = 1.0 + ( fovRadius[1] - x ) / ( radius * 2 );
		} else {
			VectorMA( point, radius, rup, point );
			verts[2].st[1] = 1.0;
		}
		VectorCopy( point, verts[2].xyz );

		// BOTTOM RIGHT
		x = ( radius + rdist[0] );
		if ( x > fovRadius[0] ) {
			VectorMA( f->org, radius + ( fovRadius[0] - x ), rright, point );
			verts[3].st[0] = 1.0 + ( fovRadius[0] - x ) / ( radius * 2 );
		} else {
			VectorMA( f->org, radius, rright, point );
			verts[3].st[0] = 1.0;
		}
		x = ( -radius + rdist[1] );
		if ( x < -fovRadius[1] ) {
			VectorMA( point, -radius + ( -fovRadius[1] - x ), rup, point );
			verts[3].st[1] = 0.0 + ( -fovRadius[1] - x ) / ( radius * 2 );
		} else {
			VectorMA( point, -radius, rup, point );
			verts[3].st[1] = 0.0;
		}
		VectorCopy( point, verts[3].xyz );

	} else {    // set default values for no clipping

		if ( ( rotatingFlames ) && ( !( cg_fxflags & 1 ) ) ) { // JPW NERVE no rotate for alt flame shaders {
			vectoangles( cg.refdef.viewaxis[0], rotate_ang );
			rotate_ang[ROLL] += f->rollAngle;
			AngleVectors( rotate_ang, NULL, rright, rup );
		} else {
			VectorCopy( vright, rright );
			VectorCopy( vup, rup );
		}

		VectorMA( f->org, -rST[1], rup, point );
		VectorMA( point, -rST[0], rright, point );
		VectorCopy( point, verts[0].xyz );
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;

		VectorMA( point, rST[1] * 2, rup, point );
		VectorCopy( point, verts[1].xyz );
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;

		VectorMA( point, rST[0] * 2, rright, point );
		VectorCopy( point, verts[2].xyz );
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;

		VectorMA( point, -rST[1] * 2, rup, point );
		VectorCopy( point, verts[3].xyz );
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
	}

	frameNum = (int)floor( lifeFrac * NUM_FLAME_SPRITES );
	if ( frameNum < 0 ) {
		frameNum = 0;
	} else if ( frameNum > NUM_FLAME_SPRITES - 1 )  {
		frameNum = NUM_FLAME_SPRITES - 1;
	}


// JPW NERVE alternate FT shaders
	if ( cg_fxflags & 1 ) {
/*
		p->roll = 0;
		p->pshader = getTestShader();
		rotate_ang[ROLL]=90;
*/
		trap_R_AddPolyToScene( getTestShader(),4,verts );
	} else {
		trap_R_AddPolyToScene( flameShaders[frameNum], 4, verts );
	}
// jpw
	VectorCopy( f->org, lastPos );
}

static int nextFlameLight = 0;
static int lastFlameOwner = -1;
/*
===============
CG_AddFlameToScene
===============
*/
void CG_AddFlameToScene( flameChunk_t *fHead ) {
	flameChunk_t *f, *fNext;
	int blueTrailHead = 0, fuelTrailHead = 0;
	static vec3_t whiteColor = {1,1,1};
	vec3_t c;
	float alpha;
	float lived;
	int headTimeStart;
	// qboolean	firstFuel = qtrue; // TTimo: unused
	float vdist, bdot;
#define FLAME_SOUND_RANGE   1024.0
	//flameChunk_t *lastSoundFlameChunk=NULL; // TTimo: unused
	flameChunk_t *lastBlowChunk = NULL;
	qboolean isClientFlame, firing;
	int shader;
	flameChunk_t *lastBlueChunk = NULL;
	qboolean skip = qfalse, droppedTrail;
	vec3_t v;
	vec3_t lightOrg;                // origin to place light at
	float lightSize;
	float lightFlameCount;
	float lastFuelAlpha;

	isClientFlame = ( fHead == centFlameInfo[fHead->ownerCent].lastFlameChunk );

	if ( ( cg_entities[fHead->ownerCent].currentState.eFlags & EF_FIRING ) && ( centFlameInfo[fHead->ownerCent].lastFlameChunk == fHead ) ) {
		headTimeStart = fHead->timeStart;
		firing = qtrue;
	} else {
		headTimeStart = cg.time;
		firing = qfalse;
	}

	VectorClear( lightOrg );
	lightSize = 0;
	lightFlameCount = 0;

	lastFuelAlpha = 1.0;

	f = fHead;
	while ( f ) {

		if ( f->nextFlameChunk && f->nextFlameChunk->dead ) {
			// kill it
			CG_FreeFlameChunk( f->nextFlameChunk );
			f->nextFlameChunk = NULL;
		}

		// draw this chunk

		fNext = f->nextFlameChunk;
		lived = (float)( headTimeStart - f->timeStart );

		// update the "blow" sound volume (louder as we sway it)
		vdist = Distance( cg.refdef.vieworg, f->org );  // NOTE: this needs to be here or the flameSound code further below won't work
		if (    lastBlowChunk && ( centFlameStatus[f->ownerCent].blowVolume < 1.0 ) &&
				( ( bdot = DotProduct( lastBlowChunk->startVelDir, f->startVelDir ) ) < 1.0 ) ) {
			if ( vdist < FLAME_SOUND_RANGE ) {
				centFlameStatus[f->ownerCent].blowVolume += 500.0 * ( 1.0 - bdot ) * ( 1.0 - ( vdist / FLAME_SOUND_RANGE ) );
				if ( centFlameStatus[f->ownerCent].blowVolume > 1.0 ) {
					centFlameStatus[f->ownerCent].blowVolume = 1.0;
				}
			}
		}
		lastBlowChunk = f;

		VectorMA( lightOrg, f->size / 20.0, f->org, lightOrg );
		lightSize += f->size;
		lightFlameCount += f->size / 20.0;

		droppedTrail = qfalse;

		// is it a stream chunk? (no special handling)
		if ( !f->ignitionOnly && f->velSpeed < 1 ) {
			CG_AddFlameSpriteToScene( f, f->lifeFrac, 1.0 );

			// is it in the blue ignition section of the flame?
		} else if ( isClientFlame && f->blueLife > ( lived / 2.0 ) ) {

			skip = qfalse;

			// if this is backwards from the last chunk, then skip it
			if ( fNext && f != fHead && lastBlueChunk ) {
				VectorSubtract( f->org, lastBlueChunk->org, v );
				if ( VectorNormalize( v ) < f->size / 2 ) {
					skip = qtrue;
				} else if ( DotProduct( v, f->velDir ) < 0 ) {
					skip = qtrue;
				}
			}

			// stream sound
			if ( !f->ignitionOnly ) {
				centFlameStatus[f->ownerCent].streamVolume += 0.05;
				if ( centFlameStatus[f->ownerCent].streamVolume > 1.0 ) {
					centFlameStatus[f->ownerCent].streamVolume = 1.0;
				}
			}


			if ( !skip ) {

				// just call this for damage checking
				//if (!f->ignitionOnly)
				//CG_AddFlameSpriteToScene( f, f->lifeFrac, -1 );

				lastBlueChunk = f;

				alpha = 1.0;    // new nozzle sprite
				VectorScale( whiteColor, alpha, c );

				if ( f->blueLife > lived * ( f->ignitionOnly ? 3.0 : 3.0 ) ) {

					shader = nozzleShaders[( cg.time / 50 + ( cg.time / 50 >> 1 ) ) % NUM_NOZZLE_SPRITES];

					blueTrailHead = CG_AddTrailJunc(    blueTrailHead,
														shader,
														cg.time,
														STYPE_STRETCH,
														f->org,
														1,
														alpha, alpha,
														f->size * ( f->ignitionOnly /*&& (cg.snap->ps.clientNum != f->ownerCent || cg_thirdPerson.integer)*/ ? 2.0 : 1.0 ),
														FLAME_MAX_SIZE,
														TJFL_NOCULL | TJFL_FIXDISTORT,
														c, c, 1.0, 5.0 );
				}

				// fire stream
				if ( !f->ignitionOnly ) {
					float bscale;
					qboolean fskip = qfalse;

					bscale = 1.0;

					if ( !f->nextFlameChunk ) {
						alpha = 0;
					} else if ( lived / 1.3 < bscale * FLAME_BLUE_FADEIN_TIME( f->blueLife ) ) {
						alpha = FLAME_BLUE_MAX_ALPHA * ( ( lived / 1.3 ) / ( bscale * FLAME_BLUE_FADEIN_TIME( f->blueLife ) ) );
					} else if ( lived / 1.3 < ( f->blueLife - FLAME_BLUE_FADEOUT_TIME( f->blueLife ) ) ) {
						alpha = FLAME_BLUE_MAX_ALPHA;
					} else {
						alpha = FLAME_BLUE_MAX_ALPHA * ( 1.0 - ( ( lived / 1.3 - ( f->blueLife - FLAME_BLUE_FADEOUT_TIME( f->blueLife ) ) ) / ( FLAME_BLUE_FADEOUT_TIME( f->blueLife ) ) ) );
					}
					if ( alpha <= 0.0 ) {
						alpha = 0.0;
						if ( lastFuelAlpha <= 0.0 ) {
							fskip = qtrue;
						}
					}

					if ( !fskip ) {
						lastFuelAlpha = alpha;

						VectorScale( whiteColor, alpha, c );

						droppedTrail = qtrue;

						fuelTrailHead = CG_AddTrailJunc(    fuelTrailHead,
															cgs.media.flamethrowerFireStream,
															cg.time,
															( f->ignitionOnly ? STYPE_STRETCH : STYPE_REPEAT ),
															f->org,
															1,
															alpha, alpha,
															( f->size / 2 < f->sizeMax / 4 ? f->size / 2 : f->sizeMax / 4 ),
															FLAME_MAX_SIZE,
															TJFL_NOCULL | TJFL_FIXDISTORT | TJFL_CROSSOVER,
															c, c, 0.5, 1.5 );
					}
				}
			}
		}

#define FLAME_SPRITE_START_BLUE_SCALE   0.2

		if ( !f->ignitionOnly &&
			 ( (float)( FLAME_SPRITE_START_BLUE_SCALE * f->blueLife ) < (float)lived ) ) {

			float alpha, lifeFrac;
			qboolean skip = qfalse;

			// should we merge it with the next sprite?
			while ( fNext && !droppedTrail ) {
				if ( ( Distance( f->org, fNext->org ) < ( ( 0.1 + 0.9 * f->lifeFrac ) * f->size * 0.35 ) )
					 &&  ( fabs( f->size - fNext->size ) < ( 40.0 ) )
					 &&  ( fabs( f->timeStart - fNext->timeStart ) < 100 )
					 &&  ( DotProduct( f->velDir, fNext->velDir ) > 0.99 )
					 ) {
					if ( !droppedTrail ) {
						CG_MergeFlameChunks( f, fNext );
						fNext = f->nextFlameChunk;      // it may have changed
					} else {
						skip = qtrue;
						break;
					}
				} else {
					break;
				}
			}

			lifeFrac = ( lived - FLAME_SPRITE_START_BLUE_SCALE * f->blueLife ) / ( FLAME_LIFETIME - FLAME_SPRITE_START_BLUE_SCALE * f->blueLife );

			alpha = ( 1.0 - lifeFrac ) * 1.4;
			if ( alpha > 1.0 ) {
				alpha = 1.0;
			}

			if ( !skip ) {
				// draw the sprite
				CG_AddFlameSpriteToScene( f, lifeFrac, alpha );
			}
			// update the sizeRate
			f->sizeRate = GET_FLAME_SIZE_SPEED( f->sizeMax );
		}

		f = fNext;
	}

	if ( lastFlameOwner == fHead->ownerCent && nextFlameLight == cg.clientFrame ) {
		return;
	}

	if ( !fHead->ignitionOnly ) {
		nextFlameLight = cg.clientFrame;
		lastFlameOwner = fHead->ownerCent;
	}

	if ( lightSize < 80 ) {
		lightSize = 80;
	}

	if ( lightSize > 500 ) {
		lightSize = 500;
	}
	lightSize *= 1.0 + 0.2 * ( sin( 1.0 * cg.time / 50.0 ) * cos( 1.0 * cg.time / 43.0 ) );
	// set the alpha
	alpha = lightSize / 500.0;
	if ( alpha > 1.0 ) {
		alpha = 1.0;
	}
	VectorScale( lightOrg, 1.0 / lightFlameCount, lightOrg );
	// if it's only a nozzle, make it blue
	if ( fHead->ignitionOnly ) {
		if ( lightSize > 80 ) {
			lightSize = 80;
		}
		trap_R_AddLightToScene( lightOrg, 90 + lightSize, 0, 0, alpha, 0 + isClientFlame * ( fHead->ownerCent == cg.snap->ps.clientNum ) );
	} else if ( isClientFlame || ( fHead->ownerCent == cg.snap->ps.clientNum ) ) {
		trap_R_AddLightToScene( lightOrg, 90 + lightSize, 1.000000 * alpha, 0.603922 * alpha, 0.207843 * alpha, 0 );
	}
}

/*
=============
CG_GenerateShaders

  A util to create a bunch of shaders in a unique shader file, which represent an animation
=============
*/
void CG_GenerateShaders( char *filename, char *shaderName, char *dir, int numFrames, char *srcBlend, char *dstBlend, char *extras, qboolean compressedVersionAvailable, qboolean nomipmap ) {
	fileHandle_t f;
	int b, c, d, lastNumber;
	char str[512];
	int i;

	trap_FS_FOpenFile( filename, &f, FS_WRITE );
	for ( i = 0; i < numFrames; i++ ) {
		lastNumber = i;
		b = lastNumber / 100;
		lastNumber -= b * 100;
		c = lastNumber / 10;
		lastNumber -= c * 10;
		d = lastNumber;

		if ( compressedVersionAvailable ) {
			Com_sprintf( str, sizeof( str ), "%s%i\n{\n\tnofog%s\n\tallowCompress\n\tcull none\n\t{\n\t\tmapcomp sprites/%s_lg/spr%i%i%i.tga\n\t\tmapnocomp sprites/%s/spr%i%i%i.tga\n\t\tblendFunc %s %s\n%s\t}\n}\n", shaderName, i + 1, nomipmap ? "\n\tnomipmaps" : "", dir, b, c, d, dir, b, c, d, srcBlend, dstBlend, extras );
		} else {
			Com_sprintf( str, sizeof( str ), "%s%i\n{\n\tnofog%s\n\tallowCompress\n\tcull none\n\t{\n\t\tmap sprites/%s/spr%i%i%i.tga\n\t\tblendFunc %s %s\n%s\t}\n}\n", shaderName, i + 1, nomipmap ? "\n\tnomipmap" : "", dir, b, c, d, srcBlend, dstBlend, extras );
		}
		trap_FS_Write( str, strlen( str ), f );
	}
	trap_FS_FCloseFile( f );
}

/*
===============
CG_InitFlameChunks
===============
*/
void CG_InitFlameChunks( void ) {
	int i;
	char filename[MAX_QPATH];

	CG_ClearFlameChunks();

#ifdef GEN_FLAME_SHADER
	CG_GenerateShaders( "scripts/flamethrower.shader",
						"flamethrowerFire",
						FLAME_SPRITE_DIR,
						NUM_FLAME_SPRITES,
						FLAME_BLEND_SRC,
						FLAME_BLEND_DST,
						"",
						qtrue, qtrue );

	CG_GenerateShaders( "scripts/blacksmokeanim.shader",
						"blacksmokeanim",
						"explode1",
						23,
						"GL_ZERO",
						"GL_ONE_MINUS_SRC_ALPHA",
						"\t\talphaGen const 0.2\n",
						qfalse, qfalse );

	CG_GenerateShaders( "scripts/viewflames.shader",
						"viewFlashFire",
						"clnfire",
						16,
						"GL_ONE",
						"GL_ONE",
						"\t\talphaGen vertex\n\t\trgbGen vertex\n",
						qtrue, qtrue );

	CG_GenerateShaders( "scripts/twiltb.shader",
						"twiltb",
						"twiltb",
						42,
						"GL_SRC_ALPHA",
						"GL_ONE_MINUS_SRC_COLOR",
						"",
						qtrue, qfalse );

	CG_GenerateShaders( "scripts/twiltb2.shader",
						"twiltb2",
						"twiltb2",
						45,
						"GL_ONE",
						"GL_ONE_MINUS_SRC_COLOR",
						"",
						qtrue, qfalse );
/*
	CG_GenerateShaders( "scripts/expblue.shader",
						"expblue",
						"expblue",
						25,
						"GL_ONE",
						"GL_ONE_MINUS_SRC_COLOR",
						"",
						qfalse, qfalse );
*/
	CG_GenerateShaders( "scripts/firest.shader",
						"firest",
						"firest",
						36,
						"GL_ONE",
						"GL_ONE_MINUS_SRC_COLOR",
						"",
						qtrue, qfalse );

	CG_GenerateShaders( "scripts/explode1.shader",
						"explode1",
						"explode1",
						23,
						"GL_ONE",
						"GL_ONE_MINUS_SRC_COLOR",
						"",
						qtrue, qfalse );

	CG_GenerateShaders( "scripts/funnel.shader",
						"funnel",
						"funnel",
						21,
						"GL_ONE",
						"GL_ONE_MINUS_SRC_COLOR",
						"",
						qfalse, qfalse );
#endif

	for ( i = 0; i < NUM_FLAME_SPRITES; i++ ) {
		Com_sprintf( filename, MAX_QPATH, "flamethrowerFire%i", i + 1 );
		flameShaders[i] = trap_R_RegisterShader( filename );
	}
	for ( i = 0; i < NUM_NOZZLE_SPRITES; i++ ) {
		Com_sprintf( filename, MAX_QPATH, "nozzleFlame%i", i + 1 );
		nozzleShaders[i] = trap_R_RegisterShader( filename );
	}
	initFlameShaders = qfalse;
}

/*
===============
CG_AddFlameChunks
===============
*/
void CG_AddFlameChunks( void ) {
	flameChunk_t *f, *fNext;

	//AngleVectors( cg.refdef.viewangles, NULL, vright, vup );
	VectorCopy( cg.refdef.viewaxis[1], vright );
	VectorCopy( cg.refdef.viewaxis[2], vup );

	// clear out the volumes so we can rebuild them
	memset( centFlameStatus, 0, sizeof( centFlameStatus ) );

	numClippedFlames = 0;

	// age them
	f = activeFlameChunks;
	while ( f ) {
		if ( !f->dead ) {
			if ( cg.time > f->timeEnd ) {
				f->dead = qtrue;
			} else if ( f->ignitionOnly && ( f->blueLife < ( cg.time - f->timeStart ) ) ) {
				f->dead = qtrue;
			} else {
				CG_MoveFlameChunk( f );
				f->lifeFrac = (float)( cg.time - f->timeStart ) / (float)( f->timeEnd - f->timeStart );
			}
		}
		f = f->nextGlobal;
	}

	// draw each of the headFlameChunk's
	f = headFlameChunks;
	while ( f ) {
		fNext = f->nextHead;        // in case it gets removed
		if ( f->dead ) {
			if ( centFlameInfo[f->ownerCent].lastFlameChunk == f ) {
				centFlameInfo[f->ownerCent].lastFlameChunk = NULL;
				centFlameInfo[f->ownerCent].lastClientFrame = 0;
			}
			CG_FreeFlameChunk( f );
		} else if ( !f->ignitionOnly || ( centFlameInfo[f->ownerCent].lastFlameChunk == f ) ) { // don't draw the ignition flame after we start firing
			CG_AddFlameToScene( f );
		}
		f = fNext;
	}
}

/*
===============
CG_UpdateFlamethrowerSounds
===============
*/
void CG_UpdateFlamethrowerSounds( void ) {
	flameChunk_t *f, *trav; // , *lastSoundFlameChunk=NULL; // TTimo: unused
	#define MIN_BLOW_VOLUME     30

	// draw each of the headFlameChunk's
	f = headFlameChunks;
	while ( f ) {
		// update this entity?
		if ( centFlameInfo[f->ownerCent].lastSoundUpdate != cg.time ) {
			// blow/ignition sound
			if ( centFlameStatus[f->ownerCent].blowVolume * 255.0 > MIN_BLOW_VOLUME ) {
				trap_S_AddLoopingSound( f->ownerCent, f->org, vec3_origin, cgs.media.flameBlowSound, (int)( 255.0 * centFlameStatus[f->ownerCent].blowVolume ) ); // JPW NERVE
			} else {
				trap_S_AddLoopingSound( f->ownerCent, f->org, vec3_origin, cgs.media.flameBlowSound, MIN_BLOW_VOLUME ); // JPW NERVE
			}

			if ( centFlameStatus[f->ownerCent].streamVolume ) {
				if ( cg_entities[f->ownerCent].currentState.aiChar != AICHAR_ZOMBIE ) {
					trap_S_AddLoopingSound( f->ownerCent, f->org, vec3_origin, cgs.media.flameStreamSound, (int)( 255.0 * centFlameStatus[f->ownerCent].streamVolume ) ); // JPW NERVE
				} else {
					trap_S_AddLoopingSound( f->ownerCent, f->org, vec3_origin, cgs.media.flameCrackSound, (int)( 255.0 * centFlameStatus[f->ownerCent].streamVolume ) ); // JPW NERVE
				}
			}

			centFlameInfo[f->ownerCent].lastSoundUpdate = cg.time;
		}

		// traverse the chunks, spawning flame sound sources as we go
		for ( trav = f; trav; trav = trav->nextFlameChunk ) {
			// update the sound volume
			if ( trav->blueLife + 100 < ( cg.time - trav->timeStart ) ) {
				trap_S_AddLoopingSound( trav->ownerCent, trav->org, vec3_origin, cgs.media.flameSound, (int)( 255.0 * ( 0.2 * ( trav->size / FLAME_MAX_SIZE ) ) ) );
			}
		}

		f = f->nextHead;
	}

	// DHM - Nerve :: No more client side damage
	// send client damage updates if required
	/*
	for (cent=cg_entities, i=0; i<cgs.maxclients; cent++, i++) {
		if (centFlameInfo[i].lastDmgCheck > centFlameInfo[i].lastDmgUpdate &&
			centFlameInfo[i].lastDmgUpdate < cg.time - 50 ) // JPW NERVE (cgs.gametype == GT_SINGLE_PLAYER ? 50 : 50)) -- sean changed clientdamage so this isn't a saturation issue any longer
		{
			if ((cg.snap->ps.pm_flags & PMF_LIMBO) || ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR )) // JPW NERVE
				return; // JPW NERVE don't do flame damage to guys in limbo or spectator, they drop out of the game
			CG_ClientDamage( i, centFlameInfo[i].lastDmgEnemy, CLDMG_FLAMETHROWER );
			centFlameInfo[i].lastDmgUpdate = cg.time;
		}
	}
	*/
}
