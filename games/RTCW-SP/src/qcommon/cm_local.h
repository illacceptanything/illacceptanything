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


#include "../game/q_shared.h"
#include "qcommon.h"
#include "cm_polylib.h"


//	(SA) DM needs more than 256 since this includes func_static and func_explosives
//#define	MAX_SUBMODELS		256
//#define	BOX_MODEL_HANDLE	255

#define MAX_SUBMODELS           512
#define BOX_MODEL_HANDLE        511
#define CAPSULE_MODEL_HANDLE    510


typedef struct {
	cplane_t    *plane;
	int children[2];                // negative numbers are leafs
} cNode_t;

typedef struct {
	int cluster;
	int area;

	int firstLeafBrush;
	int numLeafBrushes;

	int firstLeafSurface;
	int numLeafSurfaces;
} cLeaf_t;

typedef struct cmodel_s {
	vec3_t mins, maxs;
	cLeaf_t leaf;               // submodels don't reference the main tree
} cmodel_t;

typedef struct {
	cplane_t    *plane;
	int surfaceFlags;
	int shaderNum;
} cbrushside_t;

typedef struct {
	int shaderNum;              // the shader that determined the contents
	int contents;
	vec3_t bounds[2];
	int numsides;
	cbrushside_t    *sides;
	int checkcount;             // to avoid repeated testings
} cbrush_t;


typedef struct {
	int checkcount;                     // to avoid repeated testings
	int surfaceFlags;
	int contents;
	struct patchCollide_s   *pc;
} cPatch_t;


typedef struct {
	int floodnum;
	int floodvalid;
} cArea_t;

typedef struct {
	char name[MAX_QPATH];

	int numShaders;
	dshader_t   *shaders;

	int numBrushSides;
	cbrushside_t *brushsides;

	int numPlanes;
	cplane_t    *planes;

	int numNodes;
	cNode_t     *nodes;

	int numLeafs;
	cLeaf_t     *leafs;

	int numLeafBrushes;
	int         *leafbrushes;

	int numLeafSurfaces;
	int         *leafsurfaces;

	int numSubModels;
	cmodel_t    *cmodels;

	int numBrushes;
	cbrush_t    *brushes;

	int numClusters;
	int clusterBytes;
	byte        *visibility;
	qboolean vised;             // if false, visibility is just a single cluster of ffs

	int numEntityChars;
	char        *entityString;

	int numAreas;
	cArea_t     *areas;
	int         *areaPortals;   // [ numAreas*numAreas ] reference counts

	int numSurfaces;
	cPatch_t    **surfaces;         // non-patches will be NULL

	int floodvalid;
	int checkcount;                         // incremented on each trace
} clipMap_t;


// keep 1/8 unit away to keep the position valid before network snapping
// and to avoid various numeric issues
#define SURFACE_CLIP_EPSILON    ( 0.125 )

extern clipMap_t cm;
extern int c_pointcontents;
extern int c_traces, c_brush_traces, c_patch_traces;
extern cvar_t      *cm_noAreas;
extern cvar_t      *cm_noCurves;
extern cvar_t      *cm_playerCurveClip;

// cm_test.c

// Used for oriented capsule collision detection
typedef struct
{
	qboolean use;
	float radius;
	float halfheight;
	vec3_t offset;
} sphere_t;

typedef struct {
	vec3_t start;
	vec3_t end;
	vec3_t size[2];         // size of the box being swept through the model
	vec3_t offsets[8];      // [signbits][x] = either size[0][x] or size[1][x]
	float maxOffset;        // longest corner length from origin
	vec3_t extents;         // greatest of abs(size[0]) and abs(size[1])
	vec3_t bounds[2];       // enclosing box of start and end surrounding by size
	vec3_t modelOrigin;     // origin of the model tracing through
	int contents;           // ored contents of the model tracing through
	qboolean isPoint;       // optimized case
	trace_t trace;          // returned from trace call
	sphere_t sphere;        // sphere for oriendted capsule collision
} traceWork_t;

typedef struct leafList_s {
	int count;
	int maxcount;
	qboolean overflowed;
	int     *list;
	vec3_t bounds[2];
	int lastLeaf;           // for overflows where each leaf can't be stored individually
	void ( *storeLeafs )( struct leafList_s *ll, int nodenum );
} leafList_t;


int CM_BoxBrushes( const vec3_t mins, const vec3_t maxs, cbrush_t **list, int listsize );

void CM_StoreLeafs( leafList_t *ll, int nodenum );
void CM_StoreBrushes( leafList_t *ll, int nodenum );

void CM_BoxLeafnums_r( leafList_t *ll, int nodenum );

cmodel_t    *CM_ClipHandleToModel( clipHandle_t handle );

// cm_patch.c

struct patchCollide_s   *CM_GeneratePatchCollide( int width, int height, vec3_t *points );
void CM_TraceThroughPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
qboolean CM_PositionTestInPatchCollide( traceWork_t *tw, const struct patchCollide_s *pc );
void CM_ClearLevelPatches( void );
