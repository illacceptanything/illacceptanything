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

// tr_marks.c -- polygon projection on the world polygons

#include "tr_local.h"
//#include "assert.h"

#define MAX_VERTS_ON_POLY       64

#define MARKER_OFFSET           0   // 1

// Ridah, just make these global to prevent having to add more paramaters, which add overhead
static vec3_t bestnormal;
static float bestdist;

/*
=============
R_ChopPolyBehindPlane

Out must have space for two more vertexes than in
=============
*/
#define SIDE_FRONT  0
#define SIDE_BACK   1
#define SIDE_ON     2
static void R_ChopPolyBehindPlane( int numInPoints, vec3_t inPoints[MAX_VERTS_ON_POLY],
								   int *numOutPoints, vec3_t outPoints[MAX_VERTS_ON_POLY],
								   vec3_t normal, vec_t dist, vec_t epsilon ) {
	float dists[MAX_VERTS_ON_POLY + 4];
	int sides[MAX_VERTS_ON_POLY + 4];
	int counts[3];
	float dot;
	int i, j;
	float       *p1, *p2, *clip;
	float d;

	// don't clip if it might overflow
	if ( numInPoints >= MAX_VERTS_ON_POLY - 2 ) {
		*numOutPoints = 0;
		return;
	}

	counts[0] = counts[1] = counts[2] = 0;

	// determine sides for each point
	for ( i = 0 ; i < numInPoints ; i++ ) {
		dot = DotProduct( inPoints[i], normal );
		dot -= dist;
		dists[i] = dot;
		if ( dot > epsilon ) {
			sides[i] = SIDE_FRONT;
		} else if ( dot < -epsilon ) {
			sides[i] = SIDE_BACK;
		} else {
			sides[i] = SIDE_ON;
		}
		counts[sides[i]]++;
	}
	sides[i] = sides[0];
	dists[i] = dists[0];

	*numOutPoints = 0;

	if ( !counts[0] ) {
		return;
	}
	if ( !counts[1] ) {
		*numOutPoints = numInPoints;
		memcpy( outPoints, inPoints, numInPoints * sizeof( vec3_t ) );
		return;
	}

	for ( i = 0 ; i < numInPoints ; i++ ) {
		p1 = inPoints[i];
		clip = outPoints[ *numOutPoints ];

		if ( sides[i] == SIDE_ON ) {
			VectorCopy( p1, clip );
			( *numOutPoints )++;
			continue;
		}

		if ( sides[i] == SIDE_FRONT ) {
			VectorCopy( p1, clip );
			( *numOutPoints )++;
			clip = outPoints[ *numOutPoints ];
		}

		if ( sides[i + 1] == SIDE_ON || sides[i + 1] == sides[i] ) {
			continue;
		}

		// generate a split point
		p2 = inPoints[ ( i + 1 ) % numInPoints ];

		d = dists[i] - dists[i + 1];
		if ( d == 0 ) {
			dot = 0;
		} else {
			dot = dists[i] / d;
		}

		// clip xyz

		for ( j = 0 ; j < 3 ; j++ ) {
			clip[j] = p1[j] + dot * ( p2[j] - p1[j] );
		}

		( *numOutPoints )++;
	}
}

/*
=================
R_BoxSurfaces_r

=================
*/
void R_BoxSurfaces_r( mnode_t *node, vec3_t mins, vec3_t maxs, surfaceType_t **list, int listsize, int *listlength, vec3_t dir ) {

	int s, c;
	msurface_t  *surf, **mark;

	// RF, if this node hasn't been rendered recently, ignore it
	if ( node->visframe < tr.visCount - 2 ) { // allow us to be a few frames behind
		return;
	}

	// do the tail recursion in a loop
	while ( node->contents == -1 ) {
		s = BoxOnPlaneSide( mins, maxs, node->plane );
		if ( s == 1 ) {
			node = node->children[0];
		} else if ( s == 2 ) {
			node = node->children[1];
		} else {
			R_BoxSurfaces_r( node->children[0], mins, maxs, list, listsize, listlength, dir );
			node = node->children[1];
		}
	}

	// Ridah, don't mark alpha surfaces
	if ( node->contents & CONTENTS_TRANSLUCENT ) {
		return;
	}

	// add the individual surfaces
	mark = node->firstmarksurface;
	c = node->nummarksurfaces;
	while ( c-- ) {
		//
		if ( *listlength >= listsize ) {
			break;
		}
		//
		surf = *mark;
		// check if the surface has NOIMPACT or NOMARKS set
		if ( ( surf->shader->surfaceFlags & ( SURF_NOIMPACT | SURF_NOMARKS ) )
			 || ( surf->shader->contentFlags & CONTENTS_FOG ) ) {
			surf->viewCount = tr.viewCount;
		}
		// extra check for surfaces to avoid list overflows
		else if ( *( surf->data ) == SF_FACE ) {
			// the face plane should go through the box
			s = BoxOnPlaneSide( mins, maxs, &( ( srfSurfaceFace_t * ) surf->data )->plane );
			if ( s == 1 || s == 2 ) {
				surf->viewCount = tr.viewCount;
			} else if ( DotProduct( ( ( srfSurfaceFace_t * ) surf->data )->plane.normal, dir ) < -0.5 )         {
				// don't add faces that make sharp angles with the projection direction
				surf->viewCount = tr.viewCount;
			}
		} else if ( *( surfaceType_t * )( surf->data ) != SF_GRID )        {
			surf->viewCount = tr.viewCount;
		}
		// check the viewCount because the surface may have
		// already been added if it spans multiple leafs
		if ( surf->viewCount != tr.viewCount ) {
			surf->viewCount = tr.viewCount;
			list[*listlength] = (surfaceType_t *) surf->data;
			( *listlength )++;
		}
		mark++;
	}
}

/*
=================
R_AddMarkFragments

=================
*/
void R_AddMarkFragments( int numClipPoints, vec3_t clipPoints[2][MAX_VERTS_ON_POLY],
						 int numPlanes, vec3_t *normals, float *dists,
						 int maxPoints, vec3_t pointBuffer,
						 int maxFragments, markFragment_t *fragmentBuffer,
						 int *returnedPoints, int *returnedFragments,
						 vec3_t mins, vec3_t maxs ) {
	int pingPong, i;
	markFragment_t  *mf;

	// chop the surface by all the bounding planes of the to be projected polygon
	pingPong = 0;

	for ( i = 0 ; i < numPlanes ; i++ ) {

		R_ChopPolyBehindPlane( numClipPoints, clipPoints[pingPong],
							   &numClipPoints, clipPoints[!pingPong],
							   normals[i], dists[i], 0.5 );
		pingPong ^= 1;
		if ( numClipPoints == 0 ) {
			break;
		}
	}
	// completely clipped away?
	if ( numClipPoints == 0 ) {
		return;
	}

	// add this fragment to the returned list
	if ( numClipPoints + ( *returnedPoints ) > maxPoints ) {
		return; // not enough space for this polygon
	}
	/*
	// all the clip points should be within the bounding box
	for ( i = 0 ; i < numClipPoints ; i++ ) {
		int j;
		for ( j = 0 ; j < 3 ; j++ ) {
			if (clipPoints[pingPong][i][j] < mins[j] - 0.5) break;
			if (clipPoints[pingPong][i][j] > maxs[j] + 0.5) break;
		}
		if (j < 3) break;
	}
	if (i < numClipPoints) return;
	*/

	mf = fragmentBuffer + ( *returnedFragments );
	mf->firstPoint = ( *returnedPoints );
	mf->numPoints = numClipPoints;
	//memcpy( pointBuffer + (*returnedPoints) * 3, clipPoints[pingPong], numClipPoints * sizeof(vec3_t) );
	for ( i = 0; i < numClipPoints; i++ ) {
		VectorCopy( clipPoints[pingPong][i], (float *)pointBuffer + 5 * ( *returnedPoints + i ) );
	}

	( *returnedPoints ) += numClipPoints;
	( *returnedFragments )++;
}

/*
=================
R_OldMarkFragments

=================
*/
int R_OldMarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
						int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer ) {
	int numsurfaces, numPlanes;
	int i, j, k, m, n;
	surfaceType_t   *surfaces[64];
	vec3_t mins, maxs;
	int returnedFragments;
	int returnedPoints;
	vec3_t normals[MAX_VERTS_ON_POLY + 2];
	float dists[MAX_VERTS_ON_POLY + 2];
	vec3_t clipPoints[2][MAX_VERTS_ON_POLY];
	int numClipPoints;
	float           *v;
	srfSurfaceFace_t *surf;
	srfGridMesh_t   *cv;
	drawVert_t      *dv;
	vec3_t normal;
	vec3_t projectionDir;
	vec3_t v1, v2;
	int             *indexes;

	//increment view count for double check prevention
	tr.viewCount++;

	//
	VectorNormalize2( projection, projectionDir );
	// find all the brushes that are to be considered
	ClearBounds( mins, maxs );
	for ( i = 0 ; i < numPoints ; i++ ) {
		vec3_t temp;

		AddPointToBounds( points[i], mins, maxs );
		VectorAdd( points[i], projection, temp );
		AddPointToBounds( temp, mins, maxs );
		// make sure we get all the leafs (also the one(s) in front of the hit surface)
		VectorMA( points[i], -20, projectionDir, temp );
		AddPointToBounds( temp, mins, maxs );
	}

	if ( numPoints > MAX_VERTS_ON_POLY ) {
		numPoints = MAX_VERTS_ON_POLY;
	}
	// create the bounding planes for the to be projected polygon
	for ( i = 0 ; i < numPoints ; i++ ) {
		VectorSubtract( points[( i + 1 ) % numPoints], points[i], v1 );
		VectorAdd( points[i], projection, v2 );
		VectorSubtract( points[i], v2, v2 );
		CrossProduct( v1, v2, normals[i] );
		VectorNormalizeFast( normals[i] );
		dists[i] = DotProduct( normals[i], points[i] );
	}
	// add near and far clipping planes for projection
	VectorCopy( projectionDir, normals[numPoints] );
	dists[numPoints] = DotProduct( normals[numPoints], points[0] ) - 32;
	VectorCopy( projectionDir, normals[numPoints + 1] );
	VectorInverse( normals[numPoints + 1] );
	dists[numPoints + 1] = DotProduct( normals[numPoints + 1], points[0] ) - 20;
	numPlanes = numPoints + 2;

	numsurfaces = 0;
	R_BoxSurfaces_r( tr.world->nodes, mins, maxs, surfaces, 64, &numsurfaces, projectionDir );
	//assert(numsurfaces <= 64);
	//assert(numsurfaces != 64);

	returnedPoints = 0;
	returnedFragments = 0;

	for ( i = 0 ; i < numsurfaces ; i++ ) {

		if ( *surfaces[i] == SF_GRID ) {

			cv = (srfGridMesh_t *) surfaces[i];
			for ( m = 0 ; m < cv->height - 1 ; m++ ) {
				for ( n = 0 ; n < cv->width - 1 ; n++ ) {
					// We triangulate the grid and chop all triangles within
					// the bounding planes of the to be projected polygon.
					// LOD is not taken into account, not such a big deal though.
					//
					// It's probably much nicer to chop the grid itself and deal
					// with this grid as a normal SF_GRID surface so LOD will
					// be applied. However the LOD of that chopped grid must
					// be synced with the LOD of the original curve.
					// One way to do this; the chopped grid shares vertices with
					// the original curve. When LOD is applied to the original
					// curve the unused vertices are flagged. Now the chopped curve
					// should skip the flagged vertices. This still leaves the
					// problems with the vertices at the chopped grid edges.
					//
					// To avoid issues when LOD applied to "hollow curves" (like
					// the ones around many jump pads) we now just add a 2 unit
					// offset to the triangle vertices.
					// The offset is added in the vertex normal vector direction
					// so all triangles will still fit together.
					// The 2 unit offset should avoid pretty much all LOD problems.

					numClipPoints = 3;

					dv = cv->verts + m * cv->width + n;

					VectorCopy( dv[0].xyz, clipPoints[0][0] );
					VectorMA( clipPoints[0][0], MARKER_OFFSET, dv[0].normal, clipPoints[0][0] );
					VectorCopy( dv[cv->width].xyz, clipPoints[0][1] );
					VectorMA( clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal, clipPoints[0][1] );
					VectorCopy( dv[1].xyz, clipPoints[0][2] );
					VectorMA( clipPoints[0][2], MARKER_OFFSET, dv[1].normal, clipPoints[0][2] );
					// check the normal of this triangle
					VectorSubtract( clipPoints[0][0], clipPoints[0][1], v1 );
					VectorSubtract( clipPoints[0][2], clipPoints[0][1], v2 );
					CrossProduct( v1, v2, normal );
					VectorNormalizeFast( normal );
					if ( DotProduct( normal, projectionDir ) < -0.1 ) {
						// add the fragments of this triangle
						R_AddMarkFragments( numClipPoints, clipPoints,
											numPlanes, normals, dists,
											maxPoints, pointBuffer,
											maxFragments, fragmentBuffer,
											&returnedPoints, &returnedFragments, mins, maxs );

						if ( returnedFragments == maxFragments ) {
							return returnedFragments;   // not enough space for more fragments
						}
					}

					VectorCopy( dv[1].xyz, clipPoints[0][0] );
					VectorMA( clipPoints[0][0], MARKER_OFFSET, dv[1].normal, clipPoints[0][0] );
					VectorCopy( dv[cv->width].xyz, clipPoints[0][1] );
					VectorMA( clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal, clipPoints[0][1] );
					VectorCopy( dv[cv->width + 1].xyz, clipPoints[0][2] );
					VectorMA( clipPoints[0][2], MARKER_OFFSET, dv[cv->width + 1].normal, clipPoints[0][2] );
					// check the normal of this triangle
					VectorSubtract( clipPoints[0][0], clipPoints[0][1], v1 );
					VectorSubtract( clipPoints[0][2], clipPoints[0][1], v2 );
					CrossProduct( v1, v2, normal );
					VectorNormalizeFast( normal );
					if ( DotProduct( normal, projectionDir ) < -0.05 ) {
						// add the fragments of this triangle
						R_AddMarkFragments( numClipPoints, clipPoints,
											numPlanes, normals, dists,
											maxPoints, pointBuffer,
											maxFragments, fragmentBuffer,
											&returnedPoints, &returnedFragments, mins, maxs );

						if ( returnedFragments == maxFragments ) {
							return returnedFragments;   // not enough space for more fragments
						}
					}
				}
			}
		} else if ( *surfaces[i] == SF_FACE )     {

			surf = ( srfSurfaceFace_t * ) surfaces[i];
			// check the normal of this face
			if ( DotProduct( surf->plane.normal, projectionDir ) > -0.5 ) {
				continue;
			}

			/*
			VectorSubtract(clipPoints[0][0], clipPoints[0][1], v1);
			VectorSubtract(clipPoints[0][2], clipPoints[0][1], v2);
			CrossProduct(v1, v2, normal);
			VectorNormalize(normal);
			if (DotProduct(normal, projectionDir) > -0.5) continue;
			*/
			indexes = ( int * )( (byte *)surf + surf->ofsIndices );
			for ( k = 0 ; k < surf->numIndices ; k += 3 ) {
				for ( j = 0 ; j < 3 ; j++ ) {
					v = surf->points[0] + VERTEXSIZE * indexes[k + j];;
					VectorMA( v, MARKER_OFFSET, surf->plane.normal, clipPoints[0][j] );
				}
				// add the fragments of this face
				R_AddMarkFragments( 3, clipPoints,
									numPlanes, normals, dists,
									maxPoints, pointBuffer,
									maxFragments, fragmentBuffer,
									&returnedPoints, &returnedFragments, mins, maxs );
				if ( returnedFragments == maxFragments ) {
					return returnedFragments;   // not enough space for more fragments
				}
			}
			continue;
		} else {
			// ignore all other world surfaces
			// might be cool to also project polygons on a triangle soup
			// however this will probably create huge amounts of extra polys
			// even more than the projection onto curves
			continue;
		}
	}
	return returnedFragments;
}

/*
=================
R_MarkFragments

=================
*/
int R_MarkFragments( int orientation, const vec3_t *points, const vec3_t projection,
					 int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer ) {
	int numsurfaces, numPlanes;
	int i, j, k, m, n;
	surfaceType_t   *surfaces[4096];
	vec3_t mins, maxs;
	int returnedFragments;
	int returnedPoints;
	vec3_t normals[MAX_VERTS_ON_POLY + 2];
	float dists[MAX_VERTS_ON_POLY + 2];
	vec3_t clipPoints[2][MAX_VERTS_ON_POLY];
	int numClipPoints;
	float           *v;
	srfSurfaceFace_t *surf;
	srfGridMesh_t   *cv;
	drawVert_t      *dv;
	vec3_t normal;
	vec3_t projectionDir;
	vec3_t v1, v2;
	int             *indexes;
	float radius;
	vec3_t center;          // center of original mark
	//vec3_t			bestCenter;	// center point projected onto the closest surface
	float texCoordScale;
	//float			dot;
	int numPoints = 4;              // Ridah, we were only ever passing in 4, so I made this local and used the parameter for the orientation
	qboolean oldMapping = qfalse;

	//increment view count for double check prevention
	tr.viewCount++;

	// RF, negative maxFragments means we want original mapping
	if ( maxFragments < 0 ) {
		maxFragments = -maxFragments;
		//return R_OldMarkFragments( numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
		oldMapping = qtrue;
	}

	VectorClear( center );
	for ( i = 0 ; i < numPoints ; i++ ) {
		VectorAdd( points[i], center, center );
	}
	VectorScale( center, 1.0 / numPoints, center );
	//
	radius = VectorNormalize2( projection, projectionDir ) / 2.0;
	bestdist = 0;
	VectorNegate( projectionDir, bestnormal );
	// find all the brushes that are to be considered
	ClearBounds( mins, maxs );
	for ( i = 0 ; i < numPoints ; i++ ) {
		vec3_t temp;

		AddPointToBounds( points[i], mins, maxs );
		VectorMA( points[i], 1 * ( 1 + oldMapping * radius * 4 ), projection, temp );
		AddPointToBounds( temp, mins, maxs );
		// make sure we get all the leafs (also the one(s) in front of the hit surface)
		VectorMA( points[i], -20 * ( 1.0 + (float)oldMapping * ( radius / 20.0 ) * 4 ), projectionDir, temp );
		AddPointToBounds( temp, mins, maxs );
	}

	if ( numPoints > MAX_VERTS_ON_POLY ) {
		numPoints = MAX_VERTS_ON_POLY;
	}
	// create the bounding planes for the to be projected polygon
	for ( i = 0 ; i < numPoints ; i++ ) {
		VectorSubtract( points[( i + 1 ) % numPoints], points[i], v1 );
		VectorAdd( points[i], projection, v2 );
		VectorSubtract( points[i], v2, v2 );
		CrossProduct( v1, v2, normals[i] );
		VectorNormalize( normals[i] );
		dists[i] = DotProduct( normals[i], points[i] );
	}
	// add near and far clipping planes for projection
	VectorCopy( projectionDir, normals[numPoints] );
	dists[numPoints] = DotProduct( normals[numPoints], points[0] ) - radius * ( 1 + oldMapping * 10 );
	VectorCopy( projectionDir, normals[numPoints + 1] );
	VectorInverse( normals[numPoints + 1] );
	dists[numPoints + 1] = DotProduct( normals[numPoints + 1], points[0] ) - radius * ( 1 + oldMapping * 10 );
	numPlanes = numPoints + 2;

	numsurfaces = 0;
	R_BoxSurfaces_r( tr.world->nodes, mins, maxs, surfaces, 4096, &numsurfaces, projectionDir );
	//assert(numsurfaces <= 64);
	//assert(numsurfaces != 64);

	texCoordScale = 0.5 * 1.0 / radius;

	returnedPoints = 0;
	returnedFragments = 0;

	// find the closest surface to center the decal there, and wrap around other surfaces
	if ( !oldMapping ) {
/*
		for ( i = 0 ; i < numsurfaces ; i++ ) {
			if (*surfaces[i] == SF_FACE) {
				surf = ( srfSurfaceFace_t * ) surfaces[i];
				// Ridah, check if this is the closest surface
				dot = DotProduct( center, surf->plane.normal );
				dot -= surf->plane.dist;
				if (!bestdist) {
					if (dot < 0)
						bestdist = fabs(dot) + 1000;	// avoid this surface, since the point is behind it
					else
						bestdist = dot;
					VectorCopy( surf->plane.normal, bestnormal );
					VectorMA( center, -dot, surf->plane.normal, bestCenter );
				} else if (dot >= 0 && dot < bestdist) {
					bestdist = dot;
					VectorCopy( surf->plane.normal, bestnormal );
					VectorMA( center, -dot, surf->plane.normal, bestCenter );
				}
			}
		}
		// bestCenter is now the real center
		VectorCopy( bestCenter, center );
Com_Printf("bestnormal: %1.1f %1.1f %1.1f \n", bestnormal[0], bestnormal[1], bestnormal[2] );
*/
		VectorNegate( bestnormal, bestnormal );
	}

	for ( i = 0 ; i < numsurfaces ; i++ ) {

		if ( *surfaces[i] == SF_GRID ) {

			cv = (srfGridMesh_t *) surfaces[i];
			for ( m = 0 ; m < cv->height - 1 ; m++ ) {
				for ( n = 0 ; n < cv->width - 1 ; n++ ) {
					// We triangulate the grid and chop all triangles within
					// the bounding planes of the to be projected polygon.
					// LOD is not taken into account, not such a big deal though.
					//
					// It's probably much nicer to chop the grid itself and deal
					// with this grid as a normal SF_GRID surface so LOD will
					// be applied. However the LOD of that chopped grid must
					// be synced with the LOD of the original curve.
					// One way to do this; the chopped grid shares vertices with
					// the original curve. When LOD is applied to the original
					// curve the unused vertices are flagged. Now the chopped curve
					// should skip the flagged vertices. This still leaves the
					// problems with the vertices at the chopped grid edges.
					//
					// To avoid issues when LOD applied to "hollow curves" (like
					// the ones around many jump pads) we now just add a 2 unit
					// offset to the triangle vertices.
					// The offset is added in the vertex normal vector direction
					// so all triangles will still fit together.
					// The 2 unit offset should avoid pretty much all LOD problems.

					numClipPoints = 3;

					dv = cv->verts + m * cv->width + n;

					VectorCopy( dv[0].xyz, clipPoints[0][0] );
					VectorMA( clipPoints[0][0], MARKER_OFFSET, dv[0].normal, clipPoints[0][0] );
					VectorCopy( dv[cv->width].xyz, clipPoints[0][1] );
					VectorMA( clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal, clipPoints[0][1] );
					VectorCopy( dv[1].xyz, clipPoints[0][2] );
					VectorMA( clipPoints[0][2], MARKER_OFFSET, dv[1].normal, clipPoints[0][2] );
					// check the normal of this triangle
					VectorSubtract( clipPoints[0][0], clipPoints[0][1], v1 );
					VectorSubtract( clipPoints[0][2], clipPoints[0][1], v2 );
					CrossProduct( v1, v2, normal );
					VectorNormalize( normal );
					if ( DotProduct( normal, projectionDir ) < -0.1 ) {
						// add the fragments of this triangle
						R_AddMarkFragments( numClipPoints, clipPoints,
											numPlanes, normals, dists,
											maxPoints, pointBuffer,
											maxFragments, fragmentBuffer,
											&returnedPoints, &returnedFragments, mins, maxs );

						if ( returnedFragments == maxFragments ) {
							return returnedFragments;   // not enough space for more fragments
						}
					}

					VectorCopy( dv[1].xyz, clipPoints[0][0] );
					VectorMA( clipPoints[0][0], MARKER_OFFSET, dv[1].normal, clipPoints[0][0] );
					VectorCopy( dv[cv->width].xyz, clipPoints[0][1] );
					VectorMA( clipPoints[0][1], MARKER_OFFSET, dv[cv->width].normal, clipPoints[0][1] );
					VectorCopy( dv[cv->width + 1].xyz, clipPoints[0][2] );
					VectorMA( clipPoints[0][2], MARKER_OFFSET, dv[cv->width + 1].normal, clipPoints[0][2] );
					// check the normal of this triangle
					VectorSubtract( clipPoints[0][0], clipPoints[0][1], v1 );
					VectorSubtract( clipPoints[0][2], clipPoints[0][1], v2 );
					CrossProduct( v1, v2, normal );
					VectorNormalize( normal );
					if ( DotProduct( normal, projectionDir ) < -0.05 ) {
						// add the fragments of this triangle
						R_AddMarkFragments( numClipPoints, clipPoints,
											numPlanes, normals, dists,
											maxPoints, pointBuffer,
											maxFragments, fragmentBuffer,
											&returnedPoints, &returnedFragments, mins, maxs );

						if ( returnedFragments == maxFragments ) {
							return returnedFragments;   // not enough space for more fragments
						}
					}
				}
			}
		} else if ( *surfaces[i] == SF_FACE )     {
			extern float VectorDistance( vec3_t v1, vec3_t v2 );
			vec3_t axis[3];
			float texCoordScale, dot;
			vec3_t originalPoints[4];
			vec3_t newCenter, delta;
			int oldNumPoints;
			float epsilon = 0.5;
			// duplicated so we don't mess with the original clips for the curved surfaces
			vec3_t lnormals[MAX_VERTS_ON_POLY + 2];
			float ldists[MAX_VERTS_ON_POLY + 2];
			vec3_t lmins, lmaxs;

			surf = ( srfSurfaceFace_t * ) surfaces[i];

			if ( !oldMapping ) {

				// Ridah, create a new clip box such that this decal surface is mapped onto
				// the current surface without distortion. To find the center of the new clip box,
				// we project the center of the original impact center out along the projection vector,
				// onto the current surface

				// find the center of the new decal
				dot = DotProduct( center, surf->plane.normal );
				dot -= surf->plane.dist;
				// check the normal of this face
				if ( dot < -epsilon && DotProduct( surf->plane.normal, projectionDir ) >= 0.01 ) {
					continue;
				} else if ( fabs( dot ) > radius ) {
					continue;
				}
				// if the impact point is behind the surface, subtract the projection, otherwise add it
				VectorMA( center, -dot, bestnormal, newCenter );

				// recalc dot from the offset position
				dot = DotProduct( newCenter, surf->plane.normal );
				dot -= surf->plane.dist;
				VectorMA( newCenter, -dot, surf->plane.normal, newCenter );

				VectorMA( newCenter, MARKER_OFFSET, surf->plane.normal, newCenter );

				// create the texture axis
				VectorNormalize2( surf->plane.normal, axis[0] );
				PerpendicularVector( axis[1], axis[0] );
				RotatePointAroundVector( axis[2], axis[0], axis[1], (float)orientation );
				CrossProduct( axis[0], axis[2], axis[1] );

				texCoordScale = 0.5 * 1.0 / radius;

				// create the full polygon
				for ( j = 0 ; j < 3 ; j++ ) {
					originalPoints[0][j] = newCenter[j] - radius * axis[1][j] - radius * axis[2][j];
					originalPoints[1][j] = newCenter[j] + radius * axis[1][j] - radius * axis[2][j];
					originalPoints[2][j] = newCenter[j] + radius * axis[1][j] + radius * axis[2][j];
					originalPoints[3][j] = newCenter[j] - radius * axis[1][j] + radius * axis[2][j];
				}

				ClearBounds( lmins, lmaxs );

				// create the bounding planes for the to be projected polygon
				for ( j = 0 ; j < 4 ; j++ ) {
					AddPointToBounds( originalPoints[j], lmins, lmaxs );

					VectorSubtract( originalPoints[( j + 1 ) % numPoints], originalPoints[j], v1 );
					VectorSubtract( originalPoints[j], surf->plane.normal, v2 );
					VectorSubtract( originalPoints[j], v2, v2 );
					CrossProduct( v1, v2, lnormals[j] );
					VectorNormalize( lnormals[j] );
					ldists[j] = DotProduct( lnormals[j], originalPoints[j] );
				}
				numPlanes = numPoints;

				// done.

				indexes = ( int * )( (byte *)surf + surf->ofsIndices );
				for ( k = 0 ; k < surf->numIndices ; k += 3 ) {
					for ( j = 0 ; j < 3 ; j++ ) {
						v = surf->points[0] + VERTEXSIZE * indexes[k + j];
						VectorMA( v, MARKER_OFFSET, surf->plane.normal, clipPoints[0][j] );
					}

					oldNumPoints = returnedPoints;

					// add the fragments of this face
					R_AddMarkFragments( 3, clipPoints,
										numPlanes, lnormals, ldists,
										maxPoints, pointBuffer,
										maxFragments, fragmentBuffer,
										&returnedPoints, &returnedFragments, lmins, lmaxs );

					if ( oldNumPoints != returnedPoints ) {
						// flag this surface as already having computed ST's
						fragmentBuffer[returnedFragments - 1].numPoints *= -1;

						// Ridah, calculate ST's
						for ( j = 0 ; j < ( returnedPoints - oldNumPoints ) ; j++ ) {
							VectorSubtract( (float *)pointBuffer + 5 * ( oldNumPoints + j ), newCenter, delta );
							*( (float *)pointBuffer + 5 * ( oldNumPoints + j ) + 3 ) = 0.5 + DotProduct( delta, axis[1] ) * texCoordScale;
							*( (float *)pointBuffer + 5 * ( oldNumPoints + j ) + 4 ) = 0.5 + DotProduct( delta, axis[2] ) * texCoordScale;
						}
					}

					if ( returnedFragments == maxFragments ) {
						return returnedFragments;   // not enough space for more fragments
					}
				}

			} else {    // old mapping

				// check the normal of this face
				//if (DotProduct(surf->plane.normal, projectionDir) > 0.0) {
				//	continue;
				//}

				indexes = ( int * )( (byte *)surf + surf->ofsIndices );
				for ( k = 0 ; k < surf->numIndices ; k += 3 ) {
					for ( j = 0 ; j < 3 ; j++ ) {
						v = surf->points[0] + VERTEXSIZE * indexes[k + j];;
						VectorMA( v, MARKER_OFFSET, surf->plane.normal, clipPoints[0][j] );
					}
					// add the fragments of this face
					R_AddMarkFragments( 3, clipPoints,
										numPlanes, normals, dists,
										maxPoints, pointBuffer,
										maxFragments, fragmentBuffer,
										&returnedPoints, &returnedFragments, mins, maxs );
					if ( returnedFragments == maxFragments ) {
						return returnedFragments;   // not enough space for more fragments
					}
				}

			}

			continue;
		} else {
			// ignore all other world surfaces
			// might be cool to also project polygons on a triangle soup
			// however this will probably create huge amounts of extra polys
			// even more than the projection onto curves
			continue;
		}
	}
	return returnedFragments;
}

