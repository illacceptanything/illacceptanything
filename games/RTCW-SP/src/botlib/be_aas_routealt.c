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
 * name:		be_aas_routealt.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_utils.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

//#define ENABLE_ALTROUTING

typedef struct midrangearea_s
{
	int valid;
	unsigned short starttime;
	unsigned short goaltime;
} midrangearea_t;

midrangearea_t *midrangeareas;
int *clusterareas;
int numclusterareas;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_AltRoutingFloodCluster_r( int areanum ) {
	int i, otherareanum;
	aas_area_t *area;
	aas_face_t *face;

	//add the current area to the areas of the current cluster
	clusterareas[numclusterareas] = areanum;
	numclusterareas++;
	//remove the area from the mid range areas
	midrangeareas[areanum].valid = qfalse;
	//flood to other areas through the faces of this area
	area = &( *aasworld ).areas[areanum];
	for ( i = 0; i < area->numfaces; i++ )
	{
		face = &( *aasworld ).faces[abs( ( *aasworld ).faceindex[area->firstface + i] )];
		//get the area at the other side of the face
		if ( face->frontarea == areanum ) {
			otherareanum = face->backarea;
		} else { otherareanum = face->frontarea;}
		//if there is an area at the other side of this face
		if ( !otherareanum ) {
			continue;
		}
		//if the other area is not a midrange area
		if ( !midrangeareas[otherareanum].valid ) {
			continue;
		}
		//
		AAS_AltRoutingFloodCluster_r( otherareanum );
	} //end for
} //end of the function AAS_AltRoutingFloodCluster_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AlternativeRouteGoals( vec3_t start, vec3_t goal, int travelflags,
							   aas_altroutegoal_t *altroutegoals, int maxaltroutegoals,
							   int color ) {
#ifndef ENABLE_ALTROUTING
	return 0;
#else
	int i, j, startareanum, goalareanum, bestareanum;
	int numaltroutegoals, nummidrangeareas;
	int starttime, goaltime, goaltraveltime;
	float dist, bestdist;
	vec3_t mid, dir;
#ifdef DEBUG
	int startmillisecs;

	startmillisecs = Sys_MilliSeconds();
#endif

	startareanum = AAS_PointAreaNum( start );
	if ( !startareanum ) {
		return 0;
	}
	goalareanum = AAS_PointAreaNum( goal );
	if ( !goalareanum ) {
		return 0;
	}
	//travel time towards the goal area
	goaltraveltime = AAS_AreaTravelTimeToGoalArea( startareanum, start, goalareanum, travelflags );
	//clear the midrange areas
	memset( midrangeareas, 0, ( *aasworld ).numareas * sizeof( midrangearea_t ) );
	numaltroutegoals = 0;
	//
	nummidrangeareas = 0;
	//
	for ( i = 1; i < ( *aasworld ).numareas; i++ )
	{
		//
		if ( !( ( *aasworld ).areasettings[i].contents & AREACONTENTS_ROUTEPORTAL ) ) {
			continue;
		}
		//if the area has no reachabilities
		if ( !AAS_AreaReachability( i ) ) {
			continue;
		}
		//tavel time from the area to the start area
		starttime = AAS_AreaTravelTimeToGoalArea( startareanum, start, i, travelflags );
		if ( !starttime ) {
			continue;
		}
		//if the travel time from the start to the area is greater than the shortest goal travel time
		if ( starttime > 1.5 * goaltraveltime ) {
			continue;
		}
		//travel time from the area to the goal area
		goaltime = AAS_AreaTravelTimeToGoalArea( i, NULL, goalareanum, travelflags );
		if ( !goaltime ) {
			continue;
		}
		//if the travel time from the area to the goal is greater than the shortest goal travel time
		if ( goaltime > 1.5 * goaltraveltime ) {
			continue;
		}
		//this is a mid range area
		midrangeareas[i].valid = qtrue;
		midrangeareas[i].starttime = starttime;
		midrangeareas[i].goaltime = goaltime;
		Log_Write( "%d midrange area %d", nummidrangeareas, i );
		nummidrangeareas++;
	} //end for
	  //
	for ( i = 1; i < ( *aasworld ).numareas; i++ )
	{
		if ( !midrangeareas[i].valid ) {
			continue;
		}
		//get the areas in one cluster
		numclusterareas = 0;
		AAS_AltRoutingFloodCluster_r( i );
		//now we've got a cluster with areas through which an alternative route could go
		//get the 'center' of the cluster
		VectorClear( mid );
		for ( j = 0; j < numclusterareas; j++ )
		{
			VectorAdd( mid, ( *aasworld ).areas[clusterareas[j]].center, mid );
		} //end for
		VectorScale( mid, 1.0 / numclusterareas, mid );
		//get the area closest to the center of the cluster
		bestdist = 999999;
		bestareanum = 0;
		for ( j = 0; j < numclusterareas; j++ )
		{
			VectorSubtract( mid, ( *aasworld ).areas[clusterareas[j]].center, dir );
			dist = VectorLength( dir );
			if ( dist < bestdist ) {
				bestdist = dist;
				bestareanum = clusterareas[j];
			} //end if
		} //end for
		  //now we've got an area for an alternative route
		  //FIXME: add alternative goal origin
		VectorCopy( ( *aasworld ).areas[bestareanum].center, altroutegoals[numaltroutegoals].origin );
		altroutegoals[numaltroutegoals].areanum = bestareanum;
		altroutegoals[numaltroutegoals].starttraveltime = midrangeareas[bestareanum].starttime;
		altroutegoals[numaltroutegoals].goaltraveltime = midrangeareas[bestareanum].goaltime;
		altroutegoals[numaltroutegoals].extratraveltime =
			( midrangeareas[bestareanum].starttime + midrangeareas[bestareanum].goaltime ) -
			goaltraveltime;
		numaltroutegoals++;
		//
#ifdef DEBUG
		botimport.Print( PRT_MESSAGE, "alternative route goal area %d, numclusterareas = %d\n", bestareanum, numclusterareas );
		if ( color ) {
			AAS_DrawPermanentCross( ( *aasworld ).areas[bestareanum].center, 10, color );
		} //end if
		  //AAS_ShowArea(bestarea, qtrue);
#endif
		//don't return more than the maximum alternative route goals
		if ( numaltroutegoals >= maxaltroutegoals ) {
			break;
		}
	} //end for
	botimport.Print( PRT_MESSAGE, "%d alternative route goals\n", numaltroutegoals );
#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "alternative route goals in %d msec\n", Sys_MilliSeconds() - startmillisecs );
#endif
	return numaltroutegoals;
#endif
} //end of the function AAS_AlternativeRouteGoals
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitAlternativeRouting( void ) {
#ifdef ENABLE_ALTROUTING
	if ( midrangeareas ) {
		FreeMemory( midrangeareas );
	}
	midrangeareas = (midrangearea_t *) GetMemory( ( *aasworld ).numareas * sizeof( midrangearea_t ) );
	if ( clusterareas ) {
		FreeMemory( clusterareas );
	}
	clusterareas = (int *) GetMemory( aasworld.numareas * sizeof( int ) );
#endif
} //end of the function AAS_InitAlternativeRouting
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ShutdownAlternativeRouting( void ) {
#ifdef ENABLE_ALTROUTING
	if ( midrangeareas ) {
		FreeMemory( midrangeareas );
	}
	midrangeareas = NULL;
	if ( clusterareas ) {
		FreeMemory( clusterareas );
	}
	clusterareas = NULL;
	numclusterareas = 0;
#endif
}
