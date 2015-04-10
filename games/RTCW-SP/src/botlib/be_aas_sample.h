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


/*****************************************************************************
 * name:		be_aas_sample.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
void AAS_InitAASLinkHeap( void );
void AAS_InitAASLinkedEntities( void );
void AAS_FreeAASLinkHeap( void );
void AAS_FreeAASLinkedEntities( void );
aas_face_t *AAS_AreaGroundFace( int areanum, vec3_t point );
aas_face_t *AAS_TraceEndFace( aas_trace_t *trace );
aas_plane_t *AAS_PlaneFromNum( int planenum );
aas_link_t *AAS_AASLinkEntity( vec3_t absmins, vec3_t absmaxs, int entnum );
aas_link_t *AAS_LinkEntityClientBBox( vec3_t absmins, vec3_t absmaxs, int entnum, int presencetype );
qboolean AAS_PointInsideFace( int facenum, vec3_t point, float epsilon );
qboolean AAS_InsideFace( aas_face_t *face, vec3_t pnormal, vec3_t point, float epsilon );
void AAS_UnlinkFromAreas( aas_link_t *areas );
#endif //AASINTERN

//returns the mins and maxs of the bounding box for the given presence type
void AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs );
//returns the cluster the area is in (negative portal number if the area is a portal)
int AAS_AreaCluster( int areanum );
//returns the presence type(s) of the area
int AAS_AreaPresenceType( int areanum );
//returns the presence type(s) at the given point
int AAS_PointPresenceType( vec3_t point );
//returns the result of the trace of a client bbox
aas_trace_t AAS_TraceClientBBox( vec3_t start, vec3_t end, int presencetype, int passent );
//stores the areas the trace went through and returns the number of passed areas
int AAS_TraceAreas( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas );
//returns the area the point is in
int AAS_PointAreaNum( vec3_t point );
//returns the plane the given face is in
void AAS_FacePlane( int facenum, vec3_t normal, float *dist );

int AAS_BBoxAreas( vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas );
