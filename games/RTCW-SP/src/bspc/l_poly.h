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
// Name:				l_poly.h
// Function:
// Programmer:		id Sofware
//						Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:	1997-12-04
// Tab Size:		3
//===========================================================================

//a winding gives the bounding points of a convex polygon
typedef struct
{
	int numpoints;
	vec3_t p[4];            //variable sized
} winding_t;

#define MAX_POINTS_ON_WINDING   96

//you can define on_epsilon in the makefile as tighter
#ifndef ON_EPSILON
#define ON_EPSILON  0.1
#endif
//winding errors
#define WE_NONE                     0
#define WE_NOTENOUGHPOINTS          1
#define WE_SMALLAREA                    2
#define WE_POINTBOGUSRANGE          3
#define WE_POINTOFFPLANE            4
#define WE_DEGENERATEEDGE           5
#define WE_NONCONVEX                    6

//allocates a winding
winding_t *AllocWinding( int points );
//returns the area of the winding
vec_t WindingArea( winding_t *w );
//gives the center of the winding
void WindingCenter( winding_t *w, vec3_t center );
//clips the given winding to the given plane and gives the front
//and back part of the clipped winding
void ClipWindingEpsilon( winding_t *in, vec3_t normal, vec_t dist,
						 vec_t epsilon, winding_t **front, winding_t **back );
//returns the fragment of the given winding that is on the front
//side of the cliping plane. The original is freed.
winding_t *ChopWinding( winding_t *in, vec3_t normal, vec_t dist );
//returns a copy of the given winding
winding_t *CopyWinding( winding_t *w );
//returns the reversed winding of the given one
winding_t *ReverseWinding( winding_t *w );
//returns a base winding for the given plane
winding_t *BaseWindingForPlane( vec3_t normal, vec_t dist );
//checks the winding for errors
void CheckWinding( winding_t *w );
//returns the plane normal and dist the winding is in
void WindingPlane( winding_t *w, vec3_t normal, vec_t *dist );
//removes colinear points from the winding
void RemoveColinearPoints( winding_t *w );
//returns on which side of the plane the winding is situated
int WindingOnPlaneSide( winding_t *w, vec3_t normal, vec_t dist );
//frees the winding
void FreeWinding( winding_t *w );
//gets the bounds of the winding
void WindingBounds( winding_t *w, vec3_t mins, vec3_t maxs );
//chops the winding with the given plane, the original winding is freed if clipped
void ChopWindingInPlace( winding_t **w, vec3_t normal, vec_t dist, vec_t epsilon );
//prints the winding points on STDOUT
void pw( winding_t *w );
//try to merge the two windings which are in the given plane
//the original windings are undisturbed
//the merged winding is returned when merging was possible
//NULL is returned otherwise
winding_t *TryMergeWinding( winding_t *f1, winding_t *f2, vec3_t planenormal );
//brute force winding merging... creates a convex winding out of
//the two whatsoever
winding_t *MergeWindings( winding_t *w1, winding_t *w2, vec3_t planenormal );

//#ifdef ME
void ResetWindings( void );
//returns the amount of winding memory
int WindingMemory( void );
int WindingPeakMemory( void );
int ActiveWindings( void );
//returns the winding error string
char *WindingErrorString( void );
//returns one of the WE_ flags when the winding has errors
int WindingError( winding_t *w );
//removes equal points from the winding
void RemoveEqualPoints( winding_t *w, float epsilon );
//returns a winding with a point added at the given spot to the
//given winding, original winding is NOT freed
winding_t *AddWindingPoint( winding_t *w, vec3_t point, int spot );
//returns true if the point is on one of the winding 'edges'
//when the point is on one of the edged the number of the first
//point of the edge is stored in 'spot'
int PointOnWinding( winding_t *w, vec3_t normal, float dist, vec3_t point, int *spot );
//find a plane seperating the two windings
//true is returned when the windings area adjacent
//the seperating plane normal and distance area stored in 'normal' and 'dist'
//this plane will contain both the piece of common edge of the two windings
//and the vector 'dir'
int FindPlaneSeperatingWindings( winding_t *w1, winding_t *w2, vec3_t dir,
								 vec3_t normal, float *dist );
//
int WindingsNonConvex( winding_t *w1, winding_t *w2,
					   vec3_t normal1, vec3_t normal2,
					   float dist1, float dist2 );
//#endif //ME

