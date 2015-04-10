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


//#define	CULL_BBOX

/*

This file does not reference any globals, and has these entry points:

void CM_ClearLevelPatches( void );
struct patchCollide_s	*CM_GeneratePatchCollide( int width, int height, const vec3_t *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
void CM_DrawDebugSurface( void (*drawPoly)(int color, int numPoints, flaot *points) );


Issues for collision against curved surfaces:

Surface edges need to be handled differently than surface planes

Plane expansion causes raw surfaces to expand past expanded bounding box

Position test of a volume against a surface is tricky.

Position test of a point against a surface is not well defined, because the surface has no volume.


Tracing leading edge points instead of volumes?
Position test by tracing corner to corner? (8*7 traces -- ouch)

coplanar edges
triangulated patches
degenerate patches

  endcaps
  degenerate

WARNING: this may misbehave with meshes that have rows or columns that only
degenerate a few triangles.  Completely degenerate rows and columns are handled
properly.
*/


#define MAX_FACETS          1024
#define MAX_PATCH_PLANES    2048

typedef struct {
	float plane[4];
	int signbits;           // signx + (signy<<1) + (signz<<2), used as lookup during collision
} patchPlane_t;

typedef struct {
	int surfacePlane;
	int numBorders;             // 3 or four + 6 axial bevels + 4 or 3 * 4 edge bevels
	int borderPlanes[4 + 6 + 16];
	int borderInward[4 + 6 + 16];
	qboolean borderNoAdjust[4 + 6 + 16];
} facet_t;

typedef struct patchCollide_s {
	vec3_t bounds[2];
	int numPlanes;              // surface planes plus edge planes
	patchPlane_t    *planes;
	int numFacets;
	facet_t *facets;
} patchCollide_t;


#define MAX_GRID_SIZE   129

typedef struct {
	int width;
	int height;
	qboolean wrapWidth;
	qboolean wrapHeight;
	vec3_t points[MAX_GRID_SIZE][MAX_GRID_SIZE];    // [width][height]
} cGrid_t;

#define SUBDIVIDE_DISTANCE  16  //4	// never more than this units away from curve
#define PLANE_TRI_EPSILON   0.1
#define WRAP_POINT_EPSILON  0.1


struct patchCollide_s   *CM_GeneratePatchCollide( int width, int height, vec3_t *points );
