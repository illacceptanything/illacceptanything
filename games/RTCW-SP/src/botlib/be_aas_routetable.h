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
// Name:			be_aas_routetable.h
// Function:		Area Awareness System, Route-table defines
// Programmer:		Ridah
// Tab Size:		3
//===========================================================================

#ifndef RT_DEFINED

#define RT_DEFINED

#define RTBID                       ( ( 'B' << 24 ) + ( 'T' << 16 ) + ( 'R' << 8 ) + 'X' )
#define RTBVERSION                  17

#define RTB_BADTRAVELFLAGS          ( TFL_JUMPPAD | TFL_ROCKETJUMP | TFL_BFGJUMP | TFL_GRAPPLEHOOK | TFL_DOUBLEJUMP | TFL_RAMPJUMP | TFL_STRAFEJUMP | TFL_LAVA )    //----(SA)	modified since slime is no longer deadly
//#define RTB_BADTRAVELFLAGS			(TFL_JUMPPAD|TFL_ROCKETJUMP|TFL_BFGJUMP|TFL_GRAPPLEHOOK|TFL_DOUBLEJUMP|TFL_RAMPJUMP|TFL_STRAFEJUMP|TFL_SLIME|TFL_LAVA)

#define MAX_VISIBLE_AREAS   1024    // going over this limit will result in excessive memory usage, try and keep RANGE low enough so this limit won't be reached
#define MAX_LOCALTRAVELTIME 60      // use this to tweak memory usage (reduces parent count, increases local count (and cpu usage) - find a balance)
#define MAX_PARENTS         8192

extern int disable_routetable;

//....................................................................
// Permanent structures (in order of highest to lowest count)
typedef struct
{
	unsigned short int reachable_index;         // reachability index (from this area's first reachability link in the world) to head for to get to the destination
	unsigned short int travel_time;             // travel time (!)
} aas_rt_route_t;

typedef struct
{
	unsigned short int parent;              // parent we belong to
	unsigned short int childIndex;          // our index in the parent's list of children
//	unsigned short int	numRouteIndexes;
//	int					startRouteIndexes;
} aas_rt_parent_link_t;

typedef struct
{
	unsigned short int areanum;
//	int					numLocalRoutes;
//	int					startLocalRoutes;
//	int					numParentRoutes;
//	int					startParentRoutes;
	int numParentLinks;
	int startParentLinks;
} aas_rt_child_t;

typedef struct
{
	unsigned short int areanum;                     // out area number in the global list
	int numParentChildren;
	int startParentChildren;
	int numVisibleParents;
	int startVisibleParents;                        // list of other parents that we can see (used for fast hide/retreat checks)
//	int					startParentTravelTimes;
} aas_rt_parent_t;

// this is what each aasworld attaches itself to
typedef struct
{
	unsigned short int          *areaChildIndexes;  // each aas area that is part of the Route-Table has a pointer here to their position in the list of children

	int numChildren;
	aas_rt_child_t              *children;

	int numParents;
	aas_rt_parent_t             *parents;

	int numParentChildren;
	unsigned short int          *parentChildren;

	int numVisibleParents;
	unsigned short int          *visibleParents;

//	int							numLocalRoutes;
//	aas_rt_route_t				*localRoutes;		// the list of routes to all other local areas

//	int							numParentRoutes;
//	unsigned char				*parentRoutes;		// reachability to each other parent, as an offset from our first reachability

	int numParentLinks;
	aas_rt_parent_link_t        *parentLinks;       // links from each child to the parent's it belongs to

//	int							numParentTravelTimes;
//	unsigned short int			*parentTravelTimes;	// travel times between all parent areas

//	int							numRouteIndexes;
//	unsigned short int			*routeIndexes;		// each parentLink has a list within here, which
	// contains the local indexes of each child that
	// belongs to the parent, within the source child's
	// localroutes
} aas_rt_t;

//....................................................................
// Temp structures used only during route-table contruction
typedef struct
{
	unsigned short int numvisible;          // number of areas that are visible and within range
	unsigned short int
	visible[MAX_VISIBLE_AREAS];             // list of area indexes of visible and within range areas
} aas_area_buildlocalinfo_t;

typedef struct aas_parent_link_s
{
	unsigned short int parent;              // parent we belong to
	unsigned short int childindex;          // our index in the parent's list of children
	unsigned short int *routeindexes;       // for this parent link, list the children that fall under that parent, and their associated indexes in our localroutes table
	struct aas_parent_link_s *next;
} aas_parent_link_t;

typedef struct
{
	unsigned short int areanum;
	unsigned short int numlocal;
	aas_parent_link_t   *parentlink;        // linked list of parents that we belong to
	aas_rt_route_t      *localroutes;       // the list of routes to all other local areas
	aas_rt_route_t      *parentroutes;      // the list of routes to all other parent areas
} aas_area_childlocaldata_t;

typedef struct
{
	unsigned short int areanum;                                 // out area number in the global list
	unsigned short int numchildren;
	unsigned short int  *children;
	unsigned short int numVisibleParents;
	unsigned short int  *visibleParents;        // list of other parents that we can see (used for fast hide/retreat checks)
} aas_area_parent_t;

#endif  // RT_DEFINED

//....................................................................

void AAS_RT_BuildRouteTable( void );
void AAS_RT_ShowRoute( vec3_t srcpos, int srcnum, int destnum );
aas_rt_route_t *AAS_RT_GetRoute( int srcnum, vec3_t origin, int destnum );
void AAS_RT_ShutdownRouteTable( void );
qboolean AAS_RT_GetHidePos( vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos );
int AAS_RT_GetReachabilityIndex( int areanum, int reachIndex );

