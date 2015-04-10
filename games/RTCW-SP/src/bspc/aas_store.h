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
// Name:         aas_store.h
// Function:
// Programmer:   Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:  1997-12-04
// Tab Size:     3
//===========================================================================

#define AAS_MAX_BBOXES                      5
#define AAS_MAX_VERTEXES                    512000
#define AAS_MAX_PLANES                      65536
#define AAS_MAX_EDGES                       512000
#define AAS_MAX_EDGEINDEXSIZE               512000
#define AAS_MAX_FACES                       512000
#define AAS_MAX_FACEINDEXSIZE               512000
#define AAS_MAX_AREAS                       65536
#define AAS_MAX_AREASETTINGS                65536
#define AAS_MAX_REACHABILITYSIZE            65536
#define AAS_MAX_NODES                       256000
#define AAS_MAX_PORTALS                     65536
#define AAS_MAX_PORTALINDEXSIZE             65536
#define AAS_MAX_CLUSTERS                    65536

#define BSPCINCLUDE
#include "../botlib/be_aas.h"
#include "../botlib/be_aas_def.h"

/*
typedef struct bspc_aas_s
{
	int loaded;
	int initialized;								//true when AAS has been initialized
	int savefile;									//set true when file should be saved
	//bounding boxes
	int numbboxes;
	aas_bbox_t *bboxes;
	//vertexes
	int numvertexes;
	aas_vertex_t *vertexes;
	//planes
	int numplanes;
	aas_plane_t *planes;
	//edges
	int numedges;
	aas_edge_t *edges;
	//edge index
	int edgeindexsize;
	aas_edgeindex_t *edgeindex;
	//faces
	int numfaces;
	aas_face_t *faces;
	//face index
	int faceindexsize;
	aas_faceindex_t *faceindex;
	//convex areas
	int numareas;
	aas_area_t *areas;
	//convex area settings
	int numareasettings;
	aas_areasettings_t *areasettings;
	//reachablity list
	int reachabilitysize;
	aas_reachability_t *reachability;
	//nodes of the bsp tree
	int numnodes;
	aas_node_t *nodes;
	//cluster portals
	int numportals;
	aas_portal_t *portals;
	//cluster portal index
	int portalindexsize;
	aas_portalindex_t *portalindex;
	//clusters
	int numclusters;
	aas_cluster_t *clusters;
	//
	int numreachabilityareas;
	float reachabilitytime;
} bspc_aas_t;

extern bspc_aas_t aasworld;
//
*/

// Ridah
extern aas_t aasworlds[1];
extern aas_t *aasworld;
// done.

//stores the AAS file from the temporary AAS
void AAS_StoreFile( char *filename );
//returns a number of the given plane
qboolean AAS_FindPlane( vec3_t normal, float dist, int *planenum );
//allocates the maximum AAS memory for storage
void AAS_AllocMaxAAS( void );
//frees the maximum AAS memory for storage
void AAS_FreeMaxAAS( void );
