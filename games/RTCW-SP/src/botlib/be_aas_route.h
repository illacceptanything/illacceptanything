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
 * name:		be_aas_route.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
//initialize the AAS routing
void AAS_InitRouting( void );
//free the AAS routing caches
void AAS_FreeRoutingCaches( void );
//returns the travel time from start to end in the given area
unsigned short int AAS_AreaTravelTime( int areanum, vec3_t start, vec3_t end );
//
void AAS_CreateAllRoutingCache( void );
//
void AAS_RoutingInfo( void );
#endif //AASINTERN

//returns the travel flag for the given travel type
int AAS_TravelFlagForType( int traveltype );
//
int AAS_AreaContentsTravelFlag( int areanum );
//returns the index of the next reachability for the given area
int AAS_NextAreaReachability( int areanum, int reachnum );
//returns the reachability with the given index
void AAS_ReachabilityFromNum( int num, struct aas_reachability_s *reach );
//returns a random goal area and goal origin
int AAS_RandomGoalArea( int areanum, int travelflags, int *goalareanum, vec3_t goalorigin );
//returns the travel time within the given area from start to end
unsigned short int AAS_AreaTravelTime( int areanum, vec3_t start, vec3_t end );
//returns the travel time from the area to the goal area using the given travel flags
int AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags );
//returns the travel time from the area to the goal area using the given travel flags
int AAS_AreaTravelTimeToGoalAreaCheckLoop( int areanum, vec3_t origin, int goalareanum, int travelflags, int loopareanum );

extern int BotFuzzyPointReachabilityArea( vec3_t origin );
