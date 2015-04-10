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

// Ridah, cg_trails.c - draws a trail using multiple junction points

#include "cg_local.h"

typedef struct trailJunc_s
{
	struct trailJunc_s *nextGlobal, *prevGlobal;    // next junction in the global list it is in (free or used)
	struct trailJunc_s *nextJunc;                   // next junction in the trail
	struct trailJunc_s *nextHead, *prevHead;        // next head junc in the world

	qboolean inuse, freed;
	int ownerIndex;
	qhandle_t shader;

	int sType;
	int flags;
	float sTex;
	vec3_t pos;
	int spawnTime, endTime;
	float alphaStart, alphaEnd;
	vec3_t colorStart, colorEnd;
	float widthStart, widthEnd;

	// current settings
	float alpha;
	float width;
	vec3_t color;

} trailJunc_t;

#define MAX_TRAILJUNCS  4096

trailJunc_t trailJuncs[MAX_TRAILJUNCS];
trailJunc_t *freeTrails, *activeTrails;
trailJunc_t *headTrails;

qboolean initTrails = qfalse;

int numTrailsInuse;

/*
===============
CG_ClearTrails
===============
*/
void CG_ClearTrails( void ) {
	int i;

	memset( trailJuncs, 0, sizeof( trailJunc_t ) * MAX_TRAILJUNCS );

	freeTrails = trailJuncs;
	activeTrails = NULL;
	headTrails = NULL;

	for ( i = 0 ; i < MAX_TRAILJUNCS ; i++ )
	{
		trailJuncs[i].nextGlobal = &trailJuncs[i + 1];

		if ( i > 0 ) {
			trailJuncs[i].prevGlobal = &trailJuncs[i - 1];
		} else {
			trailJuncs[i].prevGlobal = NULL;
		}

		trailJuncs[i].inuse = qfalse;
	}
	trailJuncs[MAX_TRAILJUNCS - 1].nextGlobal = NULL;

	initTrails = qtrue;
	numTrailsInuse = 0;
}

/*
===============
CG_SpawnTrailJunc
===============
*/
trailJunc_t *CG_SpawnTrailJunc( trailJunc_t *headJunc ) {
	trailJunc_t *j;

	if ( !freeTrails ) {
		return NULL;
	}

	if ( cg_paused.integer ) {
		return NULL;
	}

	// select the first free trail, and remove it from the list
	j = freeTrails;
	freeTrails = j->nextGlobal;
	if ( freeTrails ) {
		freeTrails->prevGlobal = NULL;
	}

	j->nextGlobal = activeTrails;
	if ( activeTrails ) {
		activeTrails->prevGlobal = j;
	}
	activeTrails = j;
	j->prevGlobal = NULL;
	j->inuse = qtrue;
	j->freed = qfalse;

	// if this owner has a headJunc, add us to the start
	if ( headJunc ) {
		// remove the headJunc from the list of heads
		if ( headJunc == headTrails ) {
			headTrails = headJunc->nextHead;
			if ( headTrails ) {
				headTrails->prevHead = NULL;
			}
		} else {
			if ( headJunc->nextHead ) {
				headJunc->nextHead->prevHead = headJunc->prevHead;
			}
			if ( headJunc->prevHead ) {
				headJunc->prevHead->nextHead = headJunc->nextHead;
			}
		}
		headJunc->prevHead = NULL;
		headJunc->nextHead = NULL;
	}
	// make us the headTrail
	if ( headTrails ) {
		headTrails->prevHead = j;
	}
	j->nextHead = headTrails;
	j->prevHead = NULL;
	headTrails = j;

	j->nextJunc = headJunc; // if headJunc is NULL, then we'll just be the end of the list

	numTrailsInuse++;

	// debugging
//	CG_Printf( "NumTrails: %i\n", numTrailsInuse );

	return j;
}


/*
===============
CG_AddTrailJunc

  returns the index of the trail junction created

  Used for generic trails
===============
*/
int CG_AddTrailJunc( int headJuncIndex, qhandle_t shader, int spawnTime, int sType, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth, int flags, vec3_t colorStart, vec3_t colorEnd, float sRatio, float animSpeed ) {
	trailJunc_t *j, *headJunc;

	if ( headJuncIndex > 0 ) {
		headJunc = &trailJuncs[headJuncIndex - 1];

		if ( !headJunc->inuse ) {
			headJunc = NULL;
		}
	} else {
		headJunc = NULL;
	}

	j = CG_SpawnTrailJunc( headJunc );
	if ( !j ) {
//		CG_Printf("couldnt spawn trail junc\n");
		return 0;
	}

	if ( alphaStart > 1.0 ) {
		alphaStart = 1.0;
	}
	if ( alphaStart < 0.0 ) {
		alphaStart = 0.0;
	}
	if ( alphaEnd > 1.0 ) {
		alphaEnd = 1.0;
	}
	if ( alphaEnd < 0.0 ) {
		alphaEnd = 0.0;
	}

	// setup the trail junction
	j->shader = shader;
	j->sType = sType;
	VectorCopy( pos, j->pos );
	j->flags = flags;

	j->spawnTime = spawnTime;
	j->endTime = spawnTime + trailLife;

	VectorCopy( colorStart, j->colorStart );
	VectorCopy( colorEnd, j->colorEnd );

	j->alphaStart = alphaStart;
	j->alphaEnd = alphaEnd;

	j->widthStart = startWidth;
	j->widthEnd = endWidth;

	if ( sType == STYPE_REPEAT ) {
		if ( headJunc ) {
			j->sTex = headJunc->sTex + ( ( Distance( headJunc->pos, pos ) / sRatio ) / j->widthEnd );
		} else {
			// FIXME: need a way to specify offset timing
			j->sTex = ( animSpeed * ( 1.0 - ( (float)( cg.time % 1000 ) / 1000.0 ) ) ) / ( sRatio );
//			j->sTex = 0;
		}
	}

	return ( (int)( j - trailJuncs ) + 1 );
}

/*
===============
CG_AddSparkJunc

  returns the index of the trail junction created
===============
*/
int CG_AddSparkJunc( int headJuncIndex, qhandle_t shader, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth ) {
	trailJunc_t *j, *headJunc;

	if ( headJuncIndex > 0 ) {
		headJunc = &trailJuncs[headJuncIndex - 1];

		if ( !headJunc->inuse ) {
			headJunc = NULL;
		}
	} else {
		headJunc = NULL;
	}

	j = CG_SpawnTrailJunc( headJunc );
	if ( !j ) {
		return 0;
	}

	// setup the trail junction
	j->shader = shader;
	j->sType = STYPE_STRETCH;
	VectorCopy( pos, j->pos );
	j->flags = TJFL_NOCULL;     // don't worry about fading up close

	j->spawnTime = cg.time;
	j->endTime = cg.time + trailLife;

	VectorSet( j->colorStart, 1.0, 0.8 + 0.2 * alphaStart, 0.4 + 0.4 * alphaStart );
	VectorSet( j->colorEnd, 1.0, 0.8 + 0.2 * alphaEnd, 0.4 + 0.4 * alphaEnd );
//	VectorScale( j->colorStart, alphaStart, j->colorStart );
//	VectorScale( j->colorEnd, alphaEnd, j->colorEnd );

	j->alphaStart = alphaStart * 2;
	j->alphaEnd = alphaEnd * 2;
//	j->alphaStart = 1.0;
//	j->alphaEnd = 1.0;

	j->widthStart = startWidth;
	j->widthEnd = endWidth;

	return ( (int)( j - trailJuncs ) + 1 );
}

/*
===============
CG_AddSmokeJunc

  returns the index of the trail junction created
===============
*/
int CG_AddSmokeJunc( int headJuncIndex, qhandle_t shader, vec3_t pos, int trailLife, float alpha, float startWidth, float endWidth ) {
#define ST_RATIO    4.0     // sprite image: width / height
	trailJunc_t *j, *headJunc;

	if ( headJuncIndex > 0 ) {
		headJunc = &trailJuncs[headJuncIndex - 1];

		if ( !headJunc->inuse ) {
			headJunc = NULL;
		}
	} else {
		headJunc = NULL;
	}

	j = CG_SpawnTrailJunc( headJunc );
	if ( !j ) {
		return 0;
	}

	// setup the trail junction
	j->shader = shader;
	j->sType = STYPE_REPEAT;
	VectorCopy( pos, j->pos );
	j->flags = TJFL_FADEIN;

	j->spawnTime = cg.time;
	j->endTime = cg.time + trailLife;

	// VectorSet(j->colorStart, 0.2, 0.2, 0.2);
	VectorSet( j->colorStart, 0.0, 0.0, 0.0 );
	// VectorSet(j->colorEnd, 0.1, 0.1, 0.1);
	VectorSet( j->colorEnd, 0.0, 0.0, 0.0 );

	j->alphaStart = alpha;
	j->alphaEnd = 0.0;

	j->widthStart = startWidth;
	j->widthEnd = endWidth;

	if ( headJunc ) {
		j->sTex = headJunc->sTex + ( ( Distance( headJunc->pos, pos ) / ST_RATIO ) / j->widthEnd );
	} else {
		// first junction, so this will become the "tail" very soon, make it fade out
		j->sTex = 0;
		j->alphaStart = 0.0;
		j->alphaEnd = 0.0;
	}

	return ( (int)( j - trailJuncs ) + 1 );
}

void CG_KillTrail( trailJunc_t *t );

/*
===========
CG_FreeTrailJunc
===========
*/
void CG_FreeTrailJunc( trailJunc_t *junc ) {
	// kill any juncs after us, so they aren't left hanging
	if ( junc->nextJunc ) {
		CG_KillTrail( junc );
	}

	// make it non-active
	junc->inuse = qfalse;
	junc->freed = qtrue;
	if ( junc->nextGlobal ) {
		junc->nextGlobal->prevGlobal = junc->prevGlobal;
	}
	if ( junc->prevGlobal ) {
		junc->prevGlobal->nextGlobal = junc->nextGlobal;
	}
	if ( junc == activeTrails ) {
		activeTrails = junc->nextGlobal;
	}

	// if it's a head, remove it
	if ( junc == headTrails ) {
		headTrails = junc->nextHead;
	}
	if ( junc->nextHead ) {
		junc->nextHead->prevHead = junc->prevHead;
	}
	if ( junc->prevHead ) {
		junc->prevHead->nextHead = junc->nextHead;
	}
	junc->nextHead = NULL;
	junc->prevHead = NULL;

	// stick it in the free list
	junc->prevGlobal = NULL;
	junc->nextGlobal = freeTrails;
	if ( freeTrails ) {
		freeTrails->prevGlobal = junc;
	}
	freeTrails = junc;

	numTrailsInuse--;
}

/*
===========
CG_KillTrail
===========
*/
void CG_KillTrail( trailJunc_t *t ) {
	trailJunc_t *next;

	next = t->nextJunc;

	// kill the trail here
	t->nextJunc = NULL;

	if ( next ) {
		CG_FreeTrailJunc( next );
	}
}

/*
==============
CG_AddTrailToScene

  TODO: this can do with some major optimization
==============
*/
static vec3_t vforward, vright, vup;

void CG_AddTrailToScene( trailJunc_t *trail, int iteration, int numJuncs ) {
	#define MAX_TRAIL_VERTS     2048
	polyVert_t verts[MAX_TRAIL_VERTS];
	polyVert_t outVerts[MAX_TRAIL_VERTS * 3];
	int k, i, n, l, numOutVerts;
	polyVert_t mid;
	float mod[4];
	float sInc = 0.0f, s = 0.0f;   // TTimo: init
	trailJunc_t *j, *jNext;
	vec3_t fwd, up, p, v;
	// clipping vars
	#define TRAIL_FADE_CLOSE_DIST   64.0
	#define TRAIL_FADE_FAR_SCALE    4.0
	vec3_t viewProj;
	float viewDist, fadeAlpha;

	// add spark shader at head position
	if ( trail->flags & TJFL_SPARKHEADFLARE ) {
		j = trail;
		VectorCopy( j->pos, p );
		VectorMA( p, -j->width * 2, vup, p );
		VectorMA( p, -j->width * 2, vright, p );
		VectorCopy( p, verts[0].xyz );
		verts[0].st[0] = 0;
		verts[0].st[1] = 0;
		verts[0].modulate[0] = 255;
		verts[0].modulate[1] = 255;
		verts[0].modulate[2] = 255;
		verts[0].modulate[3] = ( unsigned char )( j->alpha * 255.0 );

		VectorCopy( j->pos, p );
		VectorMA( p, -j->width * 2, vup, p );
		VectorMA( p, j->width * 2, vright, p );
		VectorCopy( p, verts[1].xyz );
		verts[1].st[0] = 0;
		verts[1].st[1] = 1;
		verts[1].modulate[0] = 255;
		verts[1].modulate[1] = 255;
		verts[1].modulate[2] = 255;
		verts[1].modulate[3] = ( unsigned char )( j->alpha * 255.0 );

		VectorCopy( j->pos, p );
		VectorMA( p, j->width * 2, vup, p );
		VectorMA( p, j->width * 2, vright, p );
		VectorCopy( p, verts[2].xyz );
		verts[2].st[0] = 1;
		verts[2].st[1] = 1;
		verts[2].modulate[0] = 255;
		verts[2].modulate[1] = 255;
		verts[2].modulate[2] = 255;
		verts[2].modulate[3] = ( unsigned char )( j->alpha * 255.0 );

		VectorCopy( j->pos, p );
		VectorMA( p,  j->width * 2, vup, p );
		VectorMA( p, -j->width * 2, vright, p );
		VectorCopy( p, verts[3].xyz );
		verts[3].st[0] = 1;
		verts[3].st[1] = 0;
		verts[3].modulate[0] = 255;
		verts[3].modulate[1] = 255;
		verts[3].modulate[2] = 255;
		verts[3].modulate[3] = ( unsigned char )( j->alpha * 255.0 );

		trap_R_AddPolyToScene( cgs.media.sparkFlareShader, 4, verts );
	}

//	if (trail->flags & TJFL_CROSSOVER && iteration < 1) {
//		iteration = 1;
//	}

	if ( !numJuncs ) {
		// first count the number of juncs in the trail
		j = trail;
		numJuncs = 0;
		sInc = 0;
		while ( j ) {
			numJuncs++;

			// check for a dead next junc
			if ( !j->inuse && j->nextJunc && !j->nextJunc->inuse ) {
				CG_KillTrail( j );
			} else if ( j->nextJunc && j->nextJunc->freed ) {
				// not sure how this can happen, but it does, and causes infinite loops
				j->nextJunc = NULL;
			}

			if ( j->nextJunc ) {
				sInc += VectorDistance( j->nextJunc->pos, j->pos );
			}

			j = j->nextJunc;
		}
	}

	if ( numJuncs < 2 ) {
		return;
	}

	if ( trail->sType == STYPE_STRETCH ) {
		//sInc = ((1.0 - 0.1) / (float)(numJuncs));	// hack, the end of funnel shows a bit of the start (looping)
		s = 0.05;
		//s = 0.05;
	} else if ( trail->sType == STYPE_REPEAT ) {
		s = trail->sTex;
	}

	// now traverse the list
	j = trail;
	jNext = j->nextJunc;
	i = 0;
	while ( jNext ) {

		// first get the directional vectors to the next junc
		VectorSubtract( jNext->pos, j->pos, fwd );
		GetPerpendicularViewVector( cg.refdef.vieworg, j->pos, jNext->pos, up );

		// if it's a crossover, draw it twice
		if ( j->flags & TJFL_CROSSOVER ) {
			if ( iteration > 0 ) {
				ProjectPointOntoVector( cg.refdef.vieworg, j->pos, jNext->pos, viewProj );
				VectorSubtract( cg.refdef.vieworg, viewProj, v );
				VectorNormalize( v );

				if ( iteration == 1 ) {
					VectorMA( up, 0.3, v, up );
				} else {
					VectorMA( up, -0.3, v, up );
				}
				VectorNormalize( up );
			}
		}
		// do fading when moving towards the projection point onto the trail segment vector
		else if ( !( j->flags & TJFL_NOCULL ) && ( j->widthEnd > 4 || jNext->widthEnd > 4 ) ) {
			ProjectPointOntoVector( cg.refdef.vieworg, j->pos, jNext->pos, viewProj );
			viewDist = Distance( viewProj, cg.refdef.vieworg );
			if ( viewDist < ( TRAIL_FADE_CLOSE_DIST * TRAIL_FADE_FAR_SCALE ) ) {
				if ( viewDist < TRAIL_FADE_CLOSE_DIST ) {
					fadeAlpha = 0.0;
				} else {
					fadeAlpha = ( viewDist - TRAIL_FADE_CLOSE_DIST ) / ( TRAIL_FADE_CLOSE_DIST * TRAIL_FADE_FAR_SCALE );
				}
				if ( fadeAlpha < j->alpha ) {
					j->alpha = fadeAlpha;
				}
				if ( fadeAlpha < jNext->alpha ) {
					jNext->alpha = fadeAlpha;
				}
			}
		}

		// now output the QUAD for this segment

		// 1 ----
		VectorMA( j->pos, 0.5 * j->width, up, p );
		VectorCopy( p, verts[i].xyz );
		verts[i].st[0] = s;
		verts[i].st[1] = 1.0;
		for ( k = 0; k < 3; k++ )
			verts[i].modulate[k] = ( unsigned char )( j->color[k] * 255.0 );
		verts[i].modulate[3] = ( unsigned char )( j->alpha * 255.0 );

		// blend this with the previous junc
		if ( j != trail ) {
			VectorAdd( verts[i].xyz, verts[i - 1].xyz, verts[i].xyz );
			VectorScale( verts[i].xyz, 0.5, verts[i].xyz );
			VectorCopy( verts[i].xyz, verts[i - 1].xyz );
		} else if ( j->flags & TJFL_FADEIN ) {
			verts[i].modulate[3] = 0;   // fade in
		}

		i++;

		// 2 ----
		VectorMA( p, -1 * j->width, up, p );
		VectorCopy( p, verts[i].xyz );
		verts[i].st[0] = s;
		verts[i].st[1] = 0.0;
		for ( k = 0; k < 3; k++ )
			verts[i].modulate[k] = ( unsigned char )( j->color[k] * 255.0 );
		verts[i].modulate[3] = ( unsigned char )( j->alpha * 255.0 );

		// blend this with the previous junc
		if ( j != trail ) {
			VectorAdd( verts[i].xyz, verts[i - 3].xyz, verts[i].xyz );
			VectorScale( verts[i].xyz, 0.5, verts[i].xyz );
			VectorCopy( verts[i].xyz, verts[i - 3].xyz );
		} else if ( j->flags & TJFL_FADEIN ) {
			verts[i].modulate[3] = 0;   // fade in
		}

		i++;

		if ( trail->sType == STYPE_REPEAT ) {
			s = jNext->sTex;
		} else {
			//s += sInc;
			s += VectorDistance( j->pos, jNext->pos ) / sInc;
			if ( s > 1.0 ) {
				s = 1.0;
			}
		}

		// 3 ----
		VectorMA( jNext->pos, -0.5 * jNext->width, up, p );
		VectorCopy( p, verts[i].xyz );
		verts[i].st[0] = s;
		verts[i].st[1] = 0.0;
		for ( k = 0; k < 3; k++ )
			verts[i].modulate[k] = ( unsigned char )( jNext->color[k] * 255.0 );
		verts[i].modulate[3] = ( unsigned char )( jNext->alpha * 255.0 );
		i++;

		// 4 ----
		VectorMA( p, jNext->width, up, p );
		VectorCopy( p, verts[i].xyz );
		verts[i].st[0] = s;
		verts[i].st[1] = 1.0;
		for ( k = 0; k < 3; k++ )
			verts[i].modulate[k] = ( unsigned char )( jNext->color[k] * 255.0 );
		verts[i].modulate[3] = ( unsigned char )( jNext->alpha * 255.0 );
		i++;

		if ( i + 4 > MAX_TRAIL_VERTS ) {
			break;
		}

		j = jNext;
		jNext = j->nextJunc;
	}

	if ( trail->flags & TJFL_FIXDISTORT ) {
		// build the list of outVerts, by dividing up the QUAD's into 4 Tri's each, so as to allow
		//	any shaped (convex) Quad without bilinear distortion
		for ( k = 0, numOutVerts = 0; k < i; k += 4 ) {
			VectorCopy( verts[k].xyz, mid.xyz );
			mid.st[0] = verts[k].st[0];
			mid.st[1] = verts[k].st[1];
			for ( l = 0; l < 4; l++ ) {
				mod[l] = (float)verts[k].modulate[l];
			}
			for ( n = 1; n < 4; n++ ) {
				VectorAdd( verts[k + n].xyz, mid.xyz, mid.xyz );
				mid.st[0] += verts[k + n].st[0];
				mid.st[1] += verts[k + n].st[1];
				for ( l = 0; l < 4; l++ ) {
					mod[l] += (float)verts[k + n].modulate[l];
				}
			}
			VectorScale( mid.xyz, 0.25, mid.xyz );
			mid.st[0] *= 0.25;
			mid.st[1] *= 0.25;
			for ( l = 0; l < 4; l++ ) {
				mid.modulate[l] = ( unsigned char )( mod[l] / 4.0 );
			}

			// now output the tri's
			for ( n = 0; n < 4; n++ ) {
				outVerts[numOutVerts++] = verts[k + n];
				outVerts[numOutVerts++] = mid;
				if ( n < 3 ) {
					outVerts[numOutVerts++] = verts[k + n + 1];
				} else {
					outVerts[numOutVerts++] = verts[k];
				}
			}

		}

		if ( !( trail->flags & TJFL_NOPOLYMERGE ) ) {
			trap_R_AddPolysToScene( trail->shader, 3, &outVerts[0], numOutVerts / 3 );
		} else {
			int k;
			for ( k = 0; k < numOutVerts / 3; k++ ) {
				trap_R_AddPolyToScene( trail->shader, 3, &outVerts[k * 3] );
			}
		}
	} else
	{
		// send the polygons
		// FIXME: is it possible to send a GL_STRIP here? We are actually sending 2x the verts we really need to
		if ( !( trail->flags & TJFL_NOPOLYMERGE ) ) {
			trap_R_AddPolysToScene( trail->shader, 4, &verts[0], i / 4 );
		} else {
			int k;
			for ( k = 0; k < i / 4; k++ ) {
				trap_R_AddPolyToScene( trail->shader, 4, &verts[k * 4] );
			}
		}
	}

	// do we need to make another pass?
	if ( trail->flags & TJFL_CROSSOVER ) {
		if ( iteration < 2 ) {
			CG_AddTrailToScene( trail, iteration + 1, numJuncs );
		}
	}

}

/*
===============
CG_AddTrails
===============
*/
void CG_AddTrails( void ) {
	float lifeFrac;
	trailJunc_t *j, *jNext;

	if ( !initTrails ) {
		CG_ClearTrails();
	}

	//AngleVectors( cg.snap->ps.viewangles, vforward, vright, vup );
	VectorCopy( cg.refdef.viewaxis[0], vforward );
	VectorCopy( cg.refdef.viewaxis[1], vright );
	VectorCopy( cg.refdef.viewaxis[2], vup );

	// update the settings for each junc
	j = activeTrails;
	while ( j ) {
		lifeFrac = (float)( cg.time - j->spawnTime ) / (float)( j->endTime - j->spawnTime );
		if ( lifeFrac >= 1.0 ) {
			j->inuse = qfalse;          // flag it as dead
			j->width = j->widthEnd;
			j->alpha = j->alphaEnd;
			if ( j->alpha > 1.0 ) {
				j->alpha = 1.0;
			} else if ( j->alpha < 0.0 ) {
				j->alpha = 0.0;
			}
			VectorCopy( j->colorEnd, j->color );
		} else {
			j->width = j->widthStart + ( j->widthEnd - j->widthStart ) * lifeFrac;
			j->alpha = j->alphaStart + ( j->alphaEnd - j->alphaStart ) * lifeFrac;
			if ( j->alpha > 1.0 ) {
				j->alpha = 1.0;
			} else if ( j->alpha < 0.0 ) {
				j->alpha = 0.0;
			}
			VectorSubtract( j->colorEnd, j->colorStart, j->color );
			VectorMA( j->colorStart, lifeFrac, j->color, j->color );
		}

		j = j->nextGlobal;
	}

	// draw the trailHeads
	j = headTrails;
	while ( j ) {
		jNext = j->nextHead;        // in case it gets removed
		if ( !j->inuse ) {
			CG_FreeTrailJunc( j );
		} else {
			CG_AddTrailToScene( j, 0, 0 );
		}
		j = jNext;
	}
}
