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
 * name:		be_aas_route.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_utils.h"
#include "l_memory.h"
#include "l_log.h"
#include "l_crc.h"
#include "l_libvar.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

#define ROUTING_DEBUG

//travel time in hundreths of a second = distance * 100 / speed
#define DISTANCEFACTOR_CROUCH       1.3     //crouch speed = 100
#define DISTANCEFACTOR_SWIM         1       //should be 0.66, swim speed = 150
#define DISTANCEFACTOR_WALK         0.33    //walk speed = 300

// Ridah, scale traveltimes with ground steepness of area
#define GROUNDSTEEPNESS_TIMESCALE   20  // this is the maximum scale, 1 being the usual for a flat ground

//cache refresh time
#define CACHE_REFRESHTIME       15.0    //15 seconds refresh time

//maximum number of routing updates each frame
#define MAX_FRAMEROUTINGUPDATES     100

extern aas_t aasworlds[MAX_AAS_WORLDS];


/*

  area routing cache:
  stores the distances within one cluster to a specific goal area
  this goal area is in this same cluster and could be a cluster portal
  for every cluster there's a list with routing cache for every area
  in that cluster (including the portals of that cluster)
  area cache stores (*aasworld).clusters[?].numreachabilityareas travel times

  portal routing cache:
  stores the distances of all portals to a specific goal area
  this goal area could be in any cluster and could also be a cluster portal
  for every area ((*aasworld).numareas) the portal cache stores
  (*aasworld).numportals travel times

*/

#ifdef ROUTING_DEBUG
int numareacacheupdates;
int numportalcacheupdates;
#endif //ROUTING_DEBUG

int routingcachesize;
int max_routingcachesize;

// Ridah, routing memory calls go here, so we can change between Hunk/Zone easily
void *AAS_RoutingGetMemory( int size ) {
	return GetClearedMemory( size );
}

void AAS_RoutingFreeMemory( void *ptr ) {
	FreeMemory( ptr );
}
// done.

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
#ifdef ROUTING_DEBUG
void AAS_RoutingInfo( void ) {
	botimport.Print( PRT_MESSAGE, "%d area cache updates\n", numareacacheupdates );
	botimport.Print( PRT_MESSAGE, "%d portal cache updates\n", numportalcacheupdates );
	botimport.Print( PRT_MESSAGE, "%d bytes routing cache\n", routingcachesize );
} //end of the function AAS_RoutingInfo
#endif //ROUTING_DEBUG
//===========================================================================
// returns the number of the area in the cluster
// assumes the given area is in the given cluster or a portal of the cluster
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
__inline int AAS_ClusterAreaNum( int cluster, int areanum ) {
	int side, areacluster;

	areacluster = ( *aasworld ).areasettings[areanum].cluster;
	if ( areacluster > 0 ) {
		return ( *aasworld ).areasettings[areanum].clusterareanum;
	} else
	{
/*#ifdef ROUTING_DEBUG
		if ((*aasworld).portals[-areacluster].frontcluster != cluster &&
				(*aasworld).portals[-areacluster].backcluster != cluster)
		{
			botimport.Print(PRT_ERROR, "portal %d: does not belong to cluster %d\n"
											, -areacluster, cluster);
		} //end if
#endif //ROUTING_DEBUG*/
		side = ( *aasworld ).portals[-areacluster].frontcluster != cluster;
		return ( *aasworld ).portals[-areacluster].clusterareanum[side];
	} //end else
} //end of the function AAS_ClusterAreaNum
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitTravelFlagFromType( void ) {
	int i;

	for ( i = 0; i < MAX_TRAVELTYPES; i++ )
	{
		( *aasworld ).travelflagfortype[i] = TFL_INVALID;
	} //end for
	( *aasworld ).travelflagfortype[TRAVEL_INVALID] = TFL_INVALID;
	( *aasworld ).travelflagfortype[TRAVEL_WALK] = TFL_WALK;
	( *aasworld ).travelflagfortype[TRAVEL_CROUCH] = TFL_CROUCH;
	( *aasworld ).travelflagfortype[TRAVEL_BARRIERJUMP] = TFL_BARRIERJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_JUMP] = TFL_JUMP;
	( *aasworld ).travelflagfortype[TRAVEL_LADDER] = TFL_LADDER;
	( *aasworld ).travelflagfortype[TRAVEL_WALKOFFLEDGE] = TFL_WALKOFFLEDGE;
	( *aasworld ).travelflagfortype[TRAVEL_SWIM] = TFL_SWIM;
	( *aasworld ).travelflagfortype[TRAVEL_WATERJUMP] = TFL_WATERJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_TELEPORT] = TFL_TELEPORT;
	( *aasworld ).travelflagfortype[TRAVEL_ELEVATOR] = TFL_ELEVATOR;
	( *aasworld ).travelflagfortype[TRAVEL_ROCKETJUMP] = TFL_ROCKETJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_BFGJUMP] = TFL_BFGJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_GRAPPLEHOOK] = TFL_GRAPPLEHOOK;
	( *aasworld ).travelflagfortype[TRAVEL_DOUBLEJUMP] = TFL_DOUBLEJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_RAMPJUMP] = TFL_RAMPJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_STRAFEJUMP] = TFL_STRAFEJUMP;
	( *aasworld ).travelflagfortype[TRAVEL_JUMPPAD] = TFL_JUMPPAD;
	( *aasworld ).travelflagfortype[TRAVEL_FUNCBOB] = TFL_FUNCBOB;
} //end of the function AAS_InitTravelFlagFromType
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_TravelFlagForType( int traveltype ) {
	if ( traveltype < 0 || traveltype >= MAX_TRAVELTYPES ) {
		return TFL_INVALID;
	}
	return ( *aasworld ).travelflagfortype[traveltype];
} //end of the function AAS_TravelFlagForType
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
__inline float AAS_RoutingTime( void ) {
	return AAS_Time();
} //end of the function AAS_RoutingTime
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_FreeRoutingCache( aas_routingcache_t *cache ) {
	routingcachesize -= cache->size;
	AAS_RoutingFreeMemory( cache );
} //end of the function AAS_FreeRoutingCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_RemoveRoutingCacheInCluster( int clusternum ) {
	int i;
	aas_routingcache_t *cache, *nextcache;
	aas_cluster_t *cluster;

	if ( !( *aasworld ).clusterareacache ) {
		return;
	}
	cluster = &( *aasworld ).clusters[clusternum];
	for ( i = 0; i < cluster->numareas; i++ )
	{
		for ( cache = ( *aasworld ).clusterareacache[clusternum][i]; cache; cache = nextcache )
		{
			nextcache = cache->next;
			AAS_FreeRoutingCache( cache );
		} //end for
		( *aasworld ).clusterareacache[clusternum][i] = NULL;
	} //end for
} //end of the function AAS_RemoveRoutingCacheInCluster
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_RemoveRoutingCacheUsingArea( int areanum ) {
	int i, clusternum;
	aas_routingcache_t *cache, *nextcache;

	clusternum = ( *aasworld ).areasettings[areanum].cluster;
	if ( clusternum > 0 ) {
		//remove all the cache in the cluster the area is in
		AAS_RemoveRoutingCacheInCluster( clusternum );
	} //end if
	else
	{
		// if this is a portal remove all cache in both the front and back cluster
		AAS_RemoveRoutingCacheInCluster( ( *aasworld ).portals[-clusternum].frontcluster );
		AAS_RemoveRoutingCacheInCluster( ( *aasworld ).portals[-clusternum].backcluster );
	} //end else
	  // remove all portal cache
	if ( ( *aasworld ).portalcache ) {
		for ( i = 0; i < ( *aasworld ).numareas; i++ )
		{
			//refresh portal cache
			for ( cache = ( *aasworld ).portalcache[i]; cache; cache = nextcache )
			{
				nextcache = cache->next;
				AAS_FreeRoutingCache( cache );
			} //end for
			( *aasworld ).portalcache[i] = NULL;
		} //end for
	}
} //end of the function AAS_RemoveRoutingCacheUsingArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_EnableRoutingArea( int areanum, int enable ) {
	int flags;

	if ( areanum <= 0 || areanum >= ( *aasworld ).numareas ) {
		if ( bot_developer ) {
			botimport.Print( PRT_ERROR, "AAS_EnableRoutingArea: areanum %d out of range\n", areanum );
		} //end if
		return 0;
	} //end if
	flags = ( *aasworld ).areasettings[areanum].areaflags & AREA_DISABLED;
	if ( enable < 0 ) {
		return !flags;
	}

	if ( enable ) {
		( *aasworld ).areasettings[areanum].areaflags &= ~AREA_DISABLED;
	} else {
		( *aasworld ).areasettings[areanum].areaflags |= AREA_DISABLED;
	}
	// if the status of the area changed
	if ( ( flags & AREA_DISABLED ) != ( ( *aasworld ).areasettings[areanum].areaflags & AREA_DISABLED ) ) {
		//remove all routing cache involving this area
		AAS_RemoveRoutingCacheUsingArea( areanum );
	} //end if
	return !flags;
} //end of the function AAS_EnableRoutingArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CreateReversedReachability( void ) {
	int i, n;
	aas_reversedlink_t *revlink;
	aas_reachability_t *reach;
	aas_areasettings_t *settings;
	char *ptr;
#ifdef DEBUG
	int starttime;

	starttime = Sys_MilliSeconds();
#endif
	//free reversed links that have already been created
	if ( ( *aasworld ).reversedreachability ) {
		AAS_RoutingFreeMemory( ( *aasworld ).reversedreachability );
	}
	//allocate memory for the reversed reachability links
	ptr = (char *) AAS_RoutingGetMemory( ( *aasworld ).numareas * sizeof( aas_reversedreachability_t ) +
										 ( *aasworld ).reachabilitysize * sizeof( aas_reversedlink_t ) );
	//
	( *aasworld ).reversedreachability = (aas_reversedreachability_t *) ptr;
	//pointer to the memory for the reversed links
	ptr += ( *aasworld ).numareas * sizeof( aas_reversedreachability_t );
	//check all other areas for reachability links to the area
	for ( i = 1; i < ( *aasworld ).numareas; i++ )
	{
		//settings of the area
		settings = &( *aasworld ).areasettings[i];
		//check the reachability links
		for ( n = 0; n < settings->numreachableareas; n++ )
		{
			//reachability link
			reach = &( *aasworld ).reachability[settings->firstreachablearea + n];
			//
			revlink = (aas_reversedlink_t *) ptr;
			ptr += sizeof( aas_reversedlink_t );
			//
			revlink->areanum = i;
			revlink->linknum = settings->firstreachablearea + n;
			revlink->next = ( *aasworld ).reversedreachability[reach->areanum].first;
			( *aasworld ).reversedreachability[reach->areanum].first = revlink;
			( *aasworld ).reversedreachability[reach->areanum].numlinks++;
		} //end for
	} //end for
#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "reversed reachability %d msec\n", Sys_MilliSeconds() - starttime );
#endif //DEBUG
} //end of the function AAS_CreateReversedReachability
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_AreaGroundSteepnessScale( int areanum ) {
	return ( 1.0 + ( *aasworld ).areasettings[areanum].groundsteepness * (float)( GROUNDSTEEPNESS_TIMESCALE - 1 ) );
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
unsigned short int AAS_AreaTravelTime( int areanum, vec3_t start, vec3_t end ) {
	int intdist;
	float dist;
	vec3_t dir;

	VectorSubtract( start, end, dir );
	dist = VectorLength( dir );
	// Ridah, factor in the groundsteepness now
	dist *= AAS_AreaGroundSteepnessScale( areanum );

	//if crouch only area
	if ( AAS_AreaCrouch( areanum ) ) {
		dist *= DISTANCEFACTOR_CROUCH;
	}
	//if swim area
	else if ( AAS_AreaSwim( areanum ) ) {
		dist *= DISTANCEFACTOR_SWIM;
	}
	//normal walk area
	else {dist *= DISTANCEFACTOR_WALK;}
	//
	intdist = (int) ceil( dist );
	//make sure the distance isn't zero
	if ( intdist <= 0 ) {
		intdist = 1;
	}
	return intdist;
} //end of the function AAS_AreaTravelTime
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CalculateAreaTravelTimes( void ) {
	int i, l, n, size;
	char *ptr;
	vec3_t end;
	aas_reversedreachability_t *revreach;
	aas_reversedlink_t *revlink;
	aas_reachability_t *reach;
	aas_areasettings_t *settings;
	int starttime;

	starttime = Sys_MilliSeconds();
	//if there are still area travel times, free the memory
	if ( ( *aasworld ).areatraveltimes ) {
		AAS_RoutingFreeMemory( ( *aasworld ).areatraveltimes );
	}
	//get the total size of all the area travel times
	size = ( *aasworld ).numareas * sizeof( unsigned short ** );
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		revreach = &( *aasworld ).reversedreachability[i];
		//settings of the area
		settings = &( *aasworld ).areasettings[i];
		//
		size += settings->numreachableareas * sizeof( unsigned short * );
		//
		size += settings->numreachableareas * revreach->numlinks * sizeof( unsigned short );
	} //end for
	  //allocate memory for the area travel times
	ptr = (char *) AAS_RoutingGetMemory( size );
	( *aasworld ).areatraveltimes = (unsigned short ***) ptr;
	ptr += ( *aasworld ).numareas * sizeof( unsigned short ** );
	//calcluate the travel times for all the areas
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		//reversed reachabilities of this area
		revreach = &( *aasworld ).reversedreachability[i];
		//settings of the area
		settings = &( *aasworld ).areasettings[i];
		//
		( *aasworld ).areatraveltimes[i] = (unsigned short **) ptr;
		ptr += settings->numreachableareas * sizeof( unsigned short * );
		//
		reach = &( *aasworld ).reachability[settings->firstreachablearea];
		for ( l = 0; l < settings->numreachableareas; l++, reach++ )
		{
			( *aasworld ).areatraveltimes[i][l] = (unsigned short *) ptr;
			ptr += revreach->numlinks * sizeof( unsigned short );
			//reachability link
			//
			for ( n = 0, revlink = revreach->first; revlink; revlink = revlink->next, n++ )
			{
				VectorCopy( ( *aasworld ).reachability[revlink->linknum].end, end );
				//
				( *aasworld ).areatraveltimes[i][l][n] = AAS_AreaTravelTime( i, end, reach->start );
			} //end for
		} //end for
	} //end for
#ifdef DEBUG
	botimport.Print( PRT_MESSAGE, "area travel times %d msec\n", Sys_MilliSeconds() - starttime );
#endif //DEBUG
} //end of the function AAS_CalculateAreaTravelTimes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_PortalMaxTravelTime( int portalnum ) {
	int l, n, t, maxt;
	aas_portal_t *portal;
	aas_reversedreachability_t *revreach;
	aas_reversedlink_t *revlink;
	aas_areasettings_t *settings;

	portal = &( *aasworld ).portals[portalnum];
	//reversed reachabilities of this portal area
	revreach = &( *aasworld ).reversedreachability[portal->areanum];
	//settings of the portal area
	settings = &( *aasworld ).areasettings[portal->areanum];
	//
	maxt = 0;
	for ( l = 0; l < settings->numreachableareas; l++ )
	{
		for ( n = 0, revlink = revreach->first; revlink; revlink = revlink->next, n++ )
		{
			t = ( *aasworld ).areatraveltimes[portal->areanum][l][n];
			if ( t > maxt ) {
				maxt = t;
			} //end if
		} //end for
	} //end for
	return maxt;
} //end of the function AAS_PortalMaxTravelTime
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_InitPortalMaxTravelTimes( void ) {
	int i;

	if ( ( *aasworld ).portalmaxtraveltimes ) {
		AAS_RoutingFreeMemory( ( *aasworld ).portalmaxtraveltimes );
	}

	( *aasworld ).portalmaxtraveltimes = (int *) AAS_RoutingGetMemory( ( *aasworld ).numportals * sizeof( int ) );

	for ( i = 0; i < ( *aasworld ).numportals; i++ )
	{
		( *aasworld ).portalmaxtraveltimes[i] = AAS_PortalMaxTravelTime( i );
		//botimport.Print(PRT_MESSAGE, "portal %d max tt = %d\n", i, (*aasworld).portalmaxtraveltimes[i]);
	} //end for
} //end of the function AAS_InitPortalMaxTravelTimes
/*
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_UnlinkCache(aas_routingcache_t *cache)
{
	if (cache->time_next) cache->time_next->time_prev = cache->time_prev;
	else newestcache = cache->time_prev;
	if (cache->time_prev) cache->time_prev->time_next = cache->time_next;
	else oldestcache = cache->time_next;
	cache->time_next = NULL;
	cache->time_prev = NULL;
} //end of the function AAS_UnlinkCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_LinkCache(aas_routingcache_t *cache)
{
	if (newestcache)
	{
		newestcache->time_next = cache;
		cache->time_prev = cache;
	} //end if
	else
	{
		oldestcache = cache;
		cache->time_prev = NULL;
	} //end else
	cache->time_next = NULL;
	newestcache = cache;
} //end of the function AAS_LinkCache*/
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_FreeOldestCache( void ) {
	int i, j, bestcluster, bestarea, freed;
	float besttime;
	aas_routingcache_t *cache, *bestcache;

	freed = qfalse;
	besttime = 999999999;
	bestcache = NULL;
	bestcluster = 0;
	bestarea = 0;
	//refresh cluster cache
	for ( i = 0; i < ( *aasworld ).numclusters; i++ )
	{
		for ( j = 0; j < ( *aasworld ).clusters[i].numareas; j++ )
		{
			for ( cache = ( *aasworld ).clusterareacache[i][j]; cache; cache = cache->next )
			{
				//never remove cache leading towards a portal
				if ( ( *aasworld ).areasettings[cache->areanum].cluster < 0 ) {
					continue;
				}
				//if this cache is older than the cache we found so far
				if ( cache->time < besttime ) {
					bestcache = cache;
					bestcluster = i;
					bestarea = j;
					besttime = cache->time;
				} //end if
			} //end for
		} //end for
	} //end for
	if ( bestcache ) {
		cache = bestcache;
		if ( cache->prev ) {
			cache->prev->next = cache->next;
		} else { ( *aasworld ).clusterareacache[bestcluster][bestarea] = cache->next;}
		if ( cache->next ) {
			cache->next->prev = cache->prev;
		}
		AAS_FreeRoutingCache( cache );
		freed = qtrue;
	} //end if
	besttime = 999999999;
	bestcache = NULL;
	bestarea = 0;
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		//refresh portal cache
		for ( cache = ( *aasworld ).portalcache[i]; cache; cache = cache->next )
		{
			if ( cache->time < besttime ) {
				bestcache = cache;
				bestarea = i;
				besttime = cache->time;
			} //end if
		} //end for
	} //end for
	if ( bestcache ) {
		cache = bestcache;
		if ( cache->prev ) {
			cache->prev->next = cache->next;
		} else { ( *aasworld ).portalcache[bestarea] = cache->next;}
		if ( cache->next ) {
			cache->next->prev = cache->prev;
		}
		AAS_FreeRoutingCache( cache );
		freed = qtrue;
	} //end if
	return freed;
} //end of the function AAS_FreeOldestCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
aas_routingcache_t *AAS_AllocRoutingCache( int numtraveltimes ) {
	aas_routingcache_t *cache;
	int size;

	//
	size = sizeof( aas_routingcache_t )
		   + numtraveltimes * sizeof( unsigned short int )
		   + numtraveltimes * sizeof( unsigned char );
	//
	routingcachesize += size;
	//
	cache = (aas_routingcache_t *) AAS_RoutingGetMemory( size );
	cache->reachabilities = (unsigned char *) cache + sizeof( aas_routingcache_t )
							+ numtraveltimes * sizeof( unsigned short int );
	cache->size = size;
	return cache;
} //end of the function AAS_AllocRoutingCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_FreeAllClusterAreaCache( void ) {
	int i, j;
	aas_routingcache_t *cache, *nextcache;
	aas_cluster_t *cluster;

	//free all cluster cache if existing
	if ( !( *aasworld ).clusterareacache ) {
		return;
	}
	//free caches
	for ( i = 0; i < ( *aasworld ).numclusters; i++ )
	{
		cluster = &( *aasworld ).clusters[i];
		for ( j = 0; j < cluster->numareas; j++ )
		{
			for ( cache = ( *aasworld ).clusterareacache[i][j]; cache; cache = nextcache )
			{
				nextcache = cache->next;
				AAS_FreeRoutingCache( cache );
			} //end for
			( *aasworld ).clusterareacache[i][j] = NULL;
		} //end for
	} //end for
	  //free the cluster cache array
	AAS_RoutingFreeMemory( ( *aasworld ).clusterareacache );
	( *aasworld ).clusterareacache = NULL;
} //end of the function AAS_FreeAllClusterAreaCache
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitClusterAreaCache( void ) {
	int i, size;
	char *ptr;

	//
	for ( size = 0, i = 0; i < ( *aasworld ).numclusters; i++ )
	{
		size += ( *aasworld ).clusters[i].numareas;
	} //end for
	  //two dimensional array with pointers for every cluster to routing cache
	  //for every area in that cluster
	ptr = (char *) AAS_RoutingGetMemory(
		( *aasworld ).numclusters * sizeof( aas_routingcache_t * * ) +
		size * sizeof( aas_routingcache_t * ) );
	( *aasworld ).clusterareacache = (aas_routingcache_t ***) ptr;
	ptr += ( *aasworld ).numclusters * sizeof( aas_routingcache_t * * );
	for ( i = 0; i < ( *aasworld ).numclusters; i++ )
	{
		( *aasworld ).clusterareacache[i] = (aas_routingcache_t **) ptr;
		ptr += ( *aasworld ).clusters[i].numareas * sizeof( aas_routingcache_t * );
	} //end for
} //end of the function AAS_InitClusterAreaCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_FreeAllPortalCache( void ) {
	int i;
	aas_routingcache_t *cache, *nextcache;

	//free all portal cache if existing
	if ( !( *aasworld ).portalcache ) {
		return;
	}
	//free portal caches
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		for ( cache = ( *aasworld ).portalcache[i]; cache; cache = nextcache )
		{
			nextcache = cache->next;
			AAS_FreeRoutingCache( cache );
		} //end for
		( *aasworld ).portalcache[i] = NULL;
	} //end for
	AAS_RoutingFreeMemory( ( *aasworld ).portalcache );
	( *aasworld ).portalcache = NULL;
} //end of the function AAS_FreeAllPortalCache
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitPortalCache( void ) {
	//
	( *aasworld ).portalcache = (aas_routingcache_t **) AAS_RoutingGetMemory(
		( *aasworld ).numareas * sizeof( aas_routingcache_t * ) );
} //end of the function AAS_InitPortalCache
//
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_FreeAreaVisibility( void ) {
	int i;

	if ( ( *aasworld ).areavisibility ) {
		for ( i = 0; i < ( *aasworld ).numareas; i++ )
		{
			if ( ( *aasworld ).areavisibility[i] ) {
				FreeMemory( ( *aasworld ).areavisibility[i] );
			}
		}
	}
	if ( ( *aasworld ).areavisibility ) {
		FreeMemory( ( *aasworld ).areavisibility );
	}
	( *aasworld ).areavisibility = NULL;
	if ( ( *aasworld ).decompressedvis ) {
		FreeMemory( ( *aasworld ).decompressedvis );
	}
	( *aasworld ).decompressedvis = NULL;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InitRoutingUpdate( void ) {
//	int i, maxreachabilityareas;

	//free routing update fields if already existing
	if ( ( *aasworld ).areaupdate ) {
		AAS_RoutingFreeMemory( ( *aasworld ).areaupdate );
	}
	//
// Ridah, had to change it to numareas for hidepos checking
/*
	maxreachabilityareas = 0;
	for (i = 0; i < (*aasworld).numclusters; i++)
	{
		if ((*aasworld).clusters[i].numreachabilityareas > maxreachabilityareas)
		{
			maxreachabilityareas = (*aasworld).clusters[i].numreachabilityareas;
		} //end if
	} //end for
	//allocate memory for the routing update fields
	(*aasworld).areaupdate = (aas_routingupdate_t *) AAS_RoutingGetMemory(
									maxreachabilityareas * sizeof(aas_routingupdate_t));
*/
	( *aasworld ).areaupdate = (aas_routingupdate_t *) AAS_RoutingGetMemory(
		( *aasworld ).numareas * sizeof( aas_routingupdate_t ) );
	//
	if ( ( *aasworld ).portalupdate ) {
		AAS_RoutingFreeMemory( ( *aasworld ).portalupdate );
	}
	//allocate memory for the portal update fields
	( *aasworld ).portalupdate = (aas_routingupdate_t *) AAS_RoutingGetMemory(
		( ( *aasworld ).numportals + 1 ) * sizeof( aas_routingupdate_t ) );
} //end of the function AAS_InitRoutingUpdate
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================

void AAS_CreateAllRoutingCache( void ) {
	int i, j, k, t, tfl, numroutingareas;
	aas_areasettings_t *areasettings;
	aas_reachability_t *reach;

	numroutingareas = 0;
	tfl = TFL_DEFAULT & ~( TFL_JUMPPAD | TFL_ROCKETJUMP | TFL_BFGJUMP | TFL_GRAPPLEHOOK | TFL_DOUBLEJUMP | TFL_RAMPJUMP | TFL_STRAFEJUMP | TFL_LAVA );  //----(SA)	modified since slime is no longer deadly
//	tfl = TFL_DEFAULT & ~(TFL_JUMPPAD|TFL_ROCKETJUMP|TFL_BFGJUMP|TFL_GRAPPLEHOOK|TFL_DOUBLEJUMP|TFL_RAMPJUMP|TFL_STRAFEJUMP|TFL_SLIME|TFL_LAVA);
	botimport.Print( PRT_MESSAGE, "AAS_CreateAllRoutingCache\n" );
	//
	for ( i = 1; i < ( *aasworld ).numareas; i++ )
	{
		if ( !AAS_AreaReachability( i ) ) {
			continue;
		}
		areasettings = &( *aasworld ).areasettings[i];
		for ( k = 0; k < areasettings->numreachableareas; k++ )
		{
			reach = &( *aasworld ).reachability[areasettings->firstreachablearea + k];
			if ( ( *aasworld ).travelflagfortype[reach->traveltype] & tfl ) {
				break;
			}
		}
		if ( k >= areasettings->numreachableareas ) {
			continue;
		}
		( *aasworld ).areasettings[i].areaflags |= AREA_USEFORROUTING;
		numroutingareas++;
	}
	for ( i = 1; i < ( *aasworld ).numareas; i++ )
	{
		if ( !( ( *aasworld ).areasettings[i].areaflags & AREA_USEFORROUTING ) ) {
			continue;
		}
		for ( j = 1; j < ( *aasworld ).numareas; j++ )
		{
			if ( i == j ) {
				continue;
			}
			if ( !( ( *aasworld ).areasettings[j].areaflags & AREA_USEFORROUTING ) ) {
				continue;
			}
			t = AAS_AreaTravelTimeToGoalArea( j, ( *aasworld ).areawaypoints[j], i, tfl );
			( *aasworld ).frameroutingupdates = 0;
			//if (t) break;
			//Log_Write("traveltime from %d to %d is %d", i, j, t);
		} //end for
	} //end for
} //end of the function AAS_CreateAllRoutingCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
unsigned short CRC_ProcessString( unsigned char *data, int length );

//the route cache header
//this header is followed by numportalcache + numareacache aas_routingcache_t
//structures that store routing cache
typedef struct routecacheheader_s
{
	int ident;
	int version;
	int numareas;
	int numclusters;
	int areacrc;
	int clustercrc;
	int reachcrc;
	int numportalcache;
	int numareacache;
} routecacheheader_t;

#define RCID                        ( ( 'C' << 24 ) + ( 'R' << 16 ) + ( 'E' << 8 ) + 'M' )
#define RCVERSION                   15

void AAS_DecompressVis( byte *in, int numareas, byte *decompressed );
int AAS_CompressVis( byte *vis, int numareas, byte *dest );

void AAS_WriteRouteCache( void ) {
	int i, j, numportalcache, numareacache, size;
	aas_routingcache_t *cache;
	aas_cluster_t *cluster;
	fileHandle_t fp;
	char filename[MAX_QPATH];
	routecacheheader_t routecacheheader;
	byte *buf;

	buf = (byte *) GetClearedMemory( ( *aasworld ).numareas * 2 * sizeof( byte ) );   // in case it ends up bigger than the decompressedvis, which is rare but possible

	numportalcache = 0;
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		for ( cache = ( *aasworld ).portalcache[i]; cache; cache = cache->next )
		{
			numportalcache++;
		} //end for
	} //end for
	numareacache = 0;
	for ( i = 0; i < ( *aasworld ).numclusters; i++ )
	{
		cluster = &( *aasworld ).clusters[i];
		for ( j = 0; j < cluster->numareas; j++ )
		{
			for ( cache = ( *aasworld ).clusterareacache[i][j]; cache; cache = cache->next )
			{
				numareacache++;
			} //end for
		} //end for
	} //end for
	  // open the file for writing
	Com_sprintf( filename, MAX_QPATH, "maps/%s.rcd", ( *aasworld ).mapname );
	botimport.FS_FOpenFile( filename, &fp, FS_WRITE );
	if ( !fp ) {
		AAS_Error( "Unable to open file: %s\n", filename );
		return;
	} //end if
	  //create the header
	routecacheheader.ident = RCID;
	routecacheheader.version = RCVERSION;
	routecacheheader.numareas = ( *aasworld ).numareas;
	routecacheheader.numclusters = ( *aasworld ).numclusters;
	routecacheheader.areacrc = CRC_ProcessString( (unsigned char *)( *aasworld ).areas, sizeof( aas_area_t ) * ( *aasworld ).numareas );
	routecacheheader.clustercrc = CRC_ProcessString( (unsigned char *)( *aasworld ).clusters, sizeof( aas_cluster_t ) * ( *aasworld ).numclusters );
	routecacheheader.reachcrc = CRC_ProcessString( (unsigned char *)( *aasworld ).reachability, sizeof( aas_reachability_t ) * ( *aasworld ).reachabilitysize );
	routecacheheader.numportalcache = numportalcache;
	routecacheheader.numareacache = numareacache;
	//write the header
	botimport.FS_Write( &routecacheheader, sizeof( routecacheheader_t ), fp );
	//write all the cache
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		for ( cache = ( *aasworld ).portalcache[i]; cache; cache = cache->next )
		{
			botimport.FS_Write( cache, cache->size, fp );
		} //end for
	} //end for
	for ( i = 0; i < ( *aasworld ).numclusters; i++ )
	{
		cluster = &( *aasworld ).clusters[i];
		for ( j = 0; j < cluster->numareas; j++ )
		{
			for ( cache = ( *aasworld ).clusterareacache[i][j]; cache; cache = cache->next )
			{
				botimport.FS_Write( cache, cache->size, fp );
			} //end for
		} //end for
	} //end for
	  // write the visareas
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		if ( !( *aasworld ).areavisibility[i] ) {
			size = 0;
			botimport.FS_Write( &size, sizeof( int ), fp );
			continue;
		}
		AAS_DecompressVis( ( *aasworld ).areavisibility[i], ( *aasworld ).numareas, ( *aasworld ).decompressedvis );
		size = AAS_CompressVis( ( *aasworld ).decompressedvis, ( *aasworld ).numareas, buf );
		botimport.FS_Write( &size, sizeof( int ), fp );
		botimport.FS_Write( buf, size, fp );
	}
	// write the waypoints
	botimport.FS_Write( ( *aasworld ).areawaypoints, sizeof( vec3_t ) * ( *aasworld ).numareas, fp );
	//
	botimport.FS_FCloseFile( fp );
	botimport.Print( PRT_MESSAGE, "\nroute cache written to %s\n", filename );
} //end of the function AAS_WriteRouteCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
aas_routingcache_t *AAS_ReadCache( fileHandle_t fp ) {
	int i, size;
	aas_routingcache_t *cache;

	botimport.FS_Read( &size, sizeof( size ), fp );
	size = LittleLong( size );
	cache = (aas_routingcache_t *) AAS_RoutingGetMemory( size );
	cache->size = size;
	botimport.FS_Read( (unsigned char *)cache + sizeof( size ), size - sizeof( size ), fp );

	if ( 1 != LittleLong( 1 ) ) {
		cache->time = LittleFloat( cache->time );
		cache->cluster = LittleLong( cache->cluster );
		cache->areanum = LittleLong( cache->areanum );
		cache->origin[0] = LittleFloat( cache->origin[0] );
		cache->origin[1] = LittleFloat( cache->origin[1] );
		cache->origin[2] = LittleFloat( cache->origin[2] );
		cache->starttraveltime = LittleFloat( cache->starttraveltime );
		cache->travelflags = LittleLong( cache->travelflags );
	}

//	cache->reachabilities = (unsigned char *) cache + sizeof(aas_routingcache_t) - sizeof(unsigned short) +
//		(size - sizeof(aas_routingcache_t) + sizeof(unsigned short)) / 3 * 2;
	cache->reachabilities = (unsigned char *) cache + sizeof( aas_routingcache_t ) +
							( ( size - sizeof( aas_routingcache_t ) ) / 3 ) * 2;

	//DAJ BUGFIX for missing byteswaps for traveltimes
	size = ( size - sizeof( aas_routingcache_t ) ) / 3 + 1;
	for ( i = 0; i < size; i++ ) {
		cache->traveltimes[i] = LittleShort( cache->traveltimes[i] );
	}
	return cache;
} //end of the function AAS_ReadCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_ReadRouteCache( void ) {
	int i, clusterareanum, size;
	fileHandle_t fp = 0;
	char filename[MAX_QPATH];
	routecacheheader_t routecacheheader;
	aas_routingcache_t *cache;

	Com_sprintf( filename, MAX_QPATH, "maps/%s.rcd", ( *aasworld ).mapname );
	botimport.FS_FOpenFile( filename, &fp, FS_READ );
	if ( !fp ) {
		return qfalse;
	} //end if
	botimport.FS_Read( &routecacheheader, sizeof( routecacheheader_t ), fp );

	// GJD: route cache data MUST be written on a PC because I've not altered the writing code.

	routecacheheader.areacrc = LittleLong( routecacheheader.areacrc );
	routecacheheader.clustercrc = LittleLong( routecacheheader.clustercrc );
	routecacheheader.ident = LittleLong( routecacheheader.ident );
	routecacheheader.numareacache = LittleLong( routecacheheader.numareacache );
	routecacheheader.numareas = LittleLong( routecacheheader.numareas );
	routecacheheader.numclusters = LittleLong( routecacheheader.numclusters );
	routecacheheader.numportalcache = LittleLong( routecacheheader.numportalcache );
	routecacheheader.reachcrc = LittleLong( routecacheheader.reachcrc );
	routecacheheader.version = LittleLong( routecacheheader.version );

	if ( routecacheheader.ident != RCID ) {
		botimport.FS_FCloseFile( fp );
		Com_Printf( "%s is not a route cache dump\n", filename );       // not an aas_error because we want to continue
		return qfalse;                                              // and remake them by returning false here
	} //end if

	if ( routecacheheader.version != RCVERSION ) {
		botimport.FS_FCloseFile( fp );
		Com_Printf( "route cache dump has wrong version %d, should be %d", routecacheheader.version, RCVERSION );
		return qfalse;
	} //end if
	if ( routecacheheader.numareas != ( *aasworld ).numareas ) {
		botimport.FS_FCloseFile( fp );
		//AAS_Error("route cache dump has wrong number of areas\n");
		return qfalse;
	} //end if
	if ( routecacheheader.numclusters != ( *aasworld ).numclusters ) {
		botimport.FS_FCloseFile( fp );
		//AAS_Error("route cache dump has wrong number of clusters\n");
		return qfalse;
	} //end if
#ifdef _WIN32                           // crc code is only good on intel machines
	if ( routecacheheader.areacrc !=
		 CRC_ProcessString( (unsigned char *)( *aasworld ).areas, sizeof( aas_area_t ) * ( *aasworld ).numareas ) ) {
		botimport.FS_FCloseFile( fp );
		//AAS_Error("route cache dump area CRC incorrect\n");
		return qfalse;
	} //end if
	if ( routecacheheader.clustercrc !=
		 CRC_ProcessString( (unsigned char *)( *aasworld ).clusters, sizeof( aas_cluster_t ) * ( *aasworld ).numclusters ) ) {
		botimport.FS_FCloseFile( fp );
		//AAS_Error("route cache dump cluster CRC incorrect\n");
		return qfalse;
	} //end if
	if ( routecacheheader.reachcrc !=
		 CRC_ProcessString( (unsigned char *)( *aasworld ).reachability, sizeof( aas_reachability_t ) * ( *aasworld ).reachabilitysize ) ) {
		botimport.FS_FCloseFile( fp );
		//AAS_Error("route cache dump reachability CRC incorrect\n");
		return qfalse;
	} //end if
#endif
	//read all the portal cache
	for ( i = 0; i < routecacheheader.numportalcache; i++ )
	{
		cache = AAS_ReadCache( fp );
		cache->next = ( *aasworld ).portalcache[cache->areanum];
		cache->prev = NULL;
		if ( ( *aasworld ).portalcache[cache->areanum] ) {
			( *aasworld ).portalcache[cache->areanum]->prev = cache;
		}
		( *aasworld ).portalcache[cache->areanum] = cache;
	} //end for
	  //read all the cluster area cache
	for ( i = 0; i < routecacheheader.numareacache; i++ )
	{
		cache = AAS_ReadCache( fp );
		clusterareanum = AAS_ClusterAreaNum( cache->cluster, cache->areanum );
		cache->next = ( *aasworld ).clusterareacache[cache->cluster][clusterareanum];
		cache->prev = NULL;
		if ( ( *aasworld ).clusterareacache[cache->cluster][clusterareanum] ) {
			( *aasworld ).clusterareacache[cache->cluster][clusterareanum]->prev = cache;
		}
		( *aasworld ).clusterareacache[cache->cluster][clusterareanum] = cache;
	} //end for
	  // read the visareas
	( *aasworld ).areavisibility = (byte **) GetClearedMemory( ( *aasworld ).numareas * sizeof( byte * ) );
	( *aasworld ).decompressedvis = (byte *) GetClearedMemory( ( *aasworld ).numareas * sizeof( byte ) );
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		botimport.FS_Read( &size, sizeof( size ), fp );
		size = LittleLong( size );
		if ( size ) {
			( *aasworld ).areavisibility[i] = (byte *) GetMemory( size );
			botimport.FS_Read( ( *aasworld ).areavisibility[i], size, fp );
		}
	}
	// read the area waypoints
	( *aasworld ).areawaypoints = (vec3_t *) GetClearedMemory( ( *aasworld ).numareas * sizeof( vec3_t ) );
	botimport.FS_Read( ( *aasworld ).areawaypoints, ( *aasworld ).numareas * sizeof( vec3_t ), fp );
	if ( 1 != LittleLong( 1 ) ) {
		for ( i = 0; i < ( *aasworld ).numareas; i++ ) {
			( *aasworld ).areawaypoints[i][0] = LittleFloat( ( *aasworld ).areawaypoints[i][0] );
			( *aasworld ).areawaypoints[i][1] = LittleFloat( ( *aasworld ).areawaypoints[i][1] );
			( *aasworld ).areawaypoints[i][2] = LittleFloat( ( *aasworld ).areawaypoints[i][2] );
		}
	}
	//
	botimport.FS_FCloseFile( fp );
	return qtrue;
} //end of the function AAS_ReadRouteCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_CreateVisibility( void );
void AAS_InitRouting( void ) {
	AAS_InitTravelFlagFromType();
	//initialize the routing update fields
	AAS_InitRoutingUpdate();
	//create reversed reachability links used by the routing update algorithm
	AAS_CreateReversedReachability();
	//initialize the cluster cache
	AAS_InitClusterAreaCache();
	//initialize portal cache
	AAS_InitPortalCache();
	//initialize the area travel times
	AAS_CalculateAreaTravelTimes();
	//calculate the maximum travel times through portals
	AAS_InitPortalMaxTravelTimes();
	//
#ifdef ROUTING_DEBUG
	numareacacheupdates = 0;
	numportalcacheupdates = 0;
#endif //ROUTING_DEBUG
	   //
	routingcachesize = 0;
	max_routingcachesize = 1024 * (int) LibVarValue( "max_routingcache", "4096" );
	//
	// Ridah, load or create the routing cache
	if ( !AAS_ReadRouteCache() ) {
		( *aasworld ).initialized = qtrue;    // Hack, so routing can compute traveltimes
		AAS_CreateVisibility();
		AAS_CreateAllRoutingCache();
		( *aasworld ).initialized = qfalse;

		AAS_WriteRouteCache();  // save it so we don't have to create it again
	}
	// done.
} //end of the function AAS_InitRouting
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_FreeRoutingCaches( void ) {
	// free all the existing cluster area cache
	AAS_FreeAllClusterAreaCache();
	// free all the existing portal cache
	AAS_FreeAllPortalCache();
	// free all the existing area visibility data
	AAS_FreeAreaVisibility();
	// free cached travel times within areas
	if ( ( *aasworld ).areatraveltimes ) {
		AAS_RoutingFreeMemory( ( *aasworld ).areatraveltimes );
	}
	( *aasworld ).areatraveltimes = NULL;
	// free cached maximum travel time through cluster portals
	if ( ( *aasworld ).portalmaxtraveltimes ) {
		AAS_RoutingFreeMemory( ( *aasworld ).portalmaxtraveltimes );
	}
	( *aasworld ).portalmaxtraveltimes = NULL;
	// free reversed reachability links
	if ( ( *aasworld ).reversedreachability ) {
		AAS_RoutingFreeMemory( ( *aasworld ).reversedreachability );
	}
	( *aasworld ).reversedreachability = NULL;
	// free routing algorithm memory
	if ( ( *aasworld ).areaupdate ) {
		AAS_RoutingFreeMemory( ( *aasworld ).areaupdate );
	}
	( *aasworld ).areaupdate = NULL;
	if ( ( *aasworld ).portalupdate ) {
		AAS_RoutingFreeMemory( ( *aasworld ).portalupdate );
	}
	( *aasworld ).portalupdate = NULL;
	// free area waypoints
	if ( ( *aasworld ).areawaypoints ) {
		FreeMemory( ( *aasworld ).areawaypoints );
	}
	( *aasworld ).areawaypoints = NULL;
} //end of the function AAS_FreeRoutingCaches
//===========================================================================
// this function could be replaced by a bubble sort or for even faster
// routing by a B+ tree
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
__inline void AAS_AddUpdateToList( aas_routingupdate_t **updateliststart,
								   aas_routingupdate_t **updatelistend,
								   aas_routingupdate_t *update ) {
	if ( !update->inlist ) {
		if ( *updatelistend ) {
			( *updatelistend )->next = update;
		} else { *updateliststart = update;}
		update->prev = *updatelistend;
		update->next = NULL;
		*updatelistend = update;
		update->inlist = qtrue;
	} //end if
} //end of the function AAS_AddUpdateToList
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaContentsTravelFlag( int areanum ) {
	int contents, tfl;

	contents = ( *aasworld ).areasettings[areanum].contents;
	tfl = 0;
	if ( contents & AREACONTENTS_WATER ) {
		return tfl |= TFL_WATER;
	} else if ( contents & AREACONTENTS_SLIME ) {
		return tfl |= TFL_SLIME;
	} else if ( contents & AREACONTENTS_LAVA )                                                                      {
		return tfl |= TFL_LAVA;
	} else { tfl |= TFL_AIR;}
	if ( contents & AREACONTENTS_DONOTENTER_LARGE ) {
		tfl |= TFL_DONOTENTER_LARGE;
	}
	if ( contents & AREACONTENTS_DONOTENTER ) {
		return tfl |= TFL_DONOTENTER;
	}
	return tfl;
} //end of the function AAS_AreaContentsTravelFlag
//===========================================================================
// update the given routing cache
//
// Parameter:			areacache		: routing cache to update
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_UpdateAreaRoutingCache( aas_routingcache_t *areacache ) {
	int i, nextareanum, cluster, badtravelflags, clusterareanum, linknum;
	int numreachabilityareas;
	unsigned short int t, startareatraveltimes[128];
	aas_routingupdate_t *updateliststart, *updatelistend, *curupdate, *nextupdate;
	aas_reachability_t *reach;
	aas_reversedreachability_t *revreach;
	aas_reversedlink_t *revlink;

#ifdef ROUTING_DEBUG
	numareacacheupdates++;
#endif //ROUTING_DEBUG
	   //number of reachability areas within this cluster
	numreachabilityareas = ( *aasworld ).clusters[areacache->cluster].numreachabilityareas;
	//
	//clear the routing update fields
//	memset((*aasworld).areaupdate, 0, (*aasworld).numareas * sizeof(aas_routingupdate_t));
	//
	badtravelflags = ~areacache->travelflags;
	//
	clusterareanum = AAS_ClusterAreaNum( areacache->cluster, areacache->areanum );
	if ( clusterareanum >= numreachabilityareas ) {
		return;
	}
	//
	memset( startareatraveltimes, 0, sizeof( startareatraveltimes ) );
	//
	curupdate = &( *aasworld ).areaupdate[clusterareanum];
	curupdate->areanum = areacache->areanum;
	//VectorCopy(areacache->origin, curupdate->start);
	curupdate->areatraveltimes = ( *aasworld ).areatraveltimes[areacache->areanum][0];
	curupdate->tmptraveltime = areacache->starttraveltime;
	//
	areacache->traveltimes[clusterareanum] = areacache->starttraveltime;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend = curupdate;
	//while there are updates in the current list, flip the lists
	while ( updateliststart )
	{
		curupdate = updateliststart;
		//
		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else { updatelistend = NULL;}
		updateliststart = curupdate->next;
		//
		curupdate->inlist = qfalse;
		//check all reversed reachability links
		revreach = &( *aasworld ).reversedreachability[curupdate->areanum];
		//
		for ( i = 0, revlink = revreach->first; revlink; revlink = revlink->next, i++ )
		{
			linknum = revlink->linknum;
			reach = &( *aasworld ).reachability[linknum];
			//if there is used an undesired travel type
			if ( ( *aasworld ).travelflagfortype[reach->traveltype] & badtravelflags ) {
				continue;
			}
			//if not allowed to enter the next area
			if ( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_DISABLED ) {
				continue;
			}
			//if the next area has a not allowed travel flag
			if ( AAS_AreaContentsTravelFlag( reach->areanum ) & badtravelflags ) {
				continue;
			}
			//number of the area the reversed reachability leads to
			nextareanum = revlink->areanum;
			//get the cluster number of the area
			cluster = ( *aasworld ).areasettings[nextareanum].cluster;
			//don't leave the cluster
			if ( cluster > 0 && cluster != areacache->cluster ) {
				continue;
			}
			//get the number of the area in the cluster
			clusterareanum = AAS_ClusterAreaNum( areacache->cluster, nextareanum );
			if ( clusterareanum >= numreachabilityareas ) {
				continue;
			}
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			t = curupdate->tmptraveltime +
				//AAS_AreaTravelTime(curupdate->areanum, curupdate->start, reach->end) +
				curupdate->areatraveltimes[i] +
				reach->traveltime;
			//
			( *aasworld ).frameroutingupdates++;
			//
			if ( !areacache->traveltimes[clusterareanum] ||
				 areacache->traveltimes[clusterareanum] > t ) {
				areacache->traveltimes[clusterareanum] = t;
				areacache->reachabilities[clusterareanum] = linknum - ( *aasworld ).areasettings[nextareanum].firstreachablearea;
				nextupdate = &( *aasworld ).areaupdate[clusterareanum];
				nextupdate->areanum = nextareanum;
				nextupdate->tmptraveltime = t;
				//VectorCopy(reach->start, nextupdate->start);
				nextupdate->areatraveltimes = ( *aasworld ).areatraveltimes[nextareanum][linknum -
																						 ( *aasworld ).areasettings[nextareanum].firstreachablearea];
				if ( !nextupdate->inlist ) {
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;
					if ( updatelistend ) {
						updatelistend->next = nextupdate;
					} else { updateliststart = nextupdate;}
					updatelistend = nextupdate;
					nextupdate->inlist = qtrue;
				} //end if
			} //end if
		} //end for
	} //end while
} //end of the function AAS_UpdateAreaRoutingCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
aas_routingcache_t *AAS_GetAreaRoutingCache( int clusternum, int areanum, int travelflags, qboolean forceUpdate ) {
	int clusterareanum;
	aas_routingcache_t *cache, *clustercache;

	//number of the area in the cluster
	clusterareanum = AAS_ClusterAreaNum( clusternum, areanum );
	//pointer to the cache for the area in the cluster
	clustercache = ( *aasworld ).clusterareacache[clusternum][clusterareanum];
	//find the cache without undesired travel flags
	for ( cache = clustercache; cache; cache = cache->next )
	{
		//if there aren't used any undesired travel types for the cache
		if ( cache->travelflags == travelflags ) {
			break;
		}
	} //end for
	  //if there was no cache
	if ( !cache ) {
		//NOTE: the number of routing updates is limited per frame
		if ( !forceUpdate && ( ( *aasworld ).frameroutingupdates > MAX_FRAMEROUTINGUPDATES ) ) {
			return NULL;
		} //end if

		cache = AAS_AllocRoutingCache( ( *aasworld ).clusters[clusternum].numreachabilityareas );
		cache->cluster = clusternum;
		cache->areanum = areanum;
		VectorCopy( ( *aasworld ).areas[areanum].center, cache->origin );
		cache->starttraveltime = 1;
		cache->travelflags = travelflags;
		cache->prev = NULL;
		cache->next = clustercache;
		if ( clustercache ) {
			clustercache->prev = cache;
		}
		( *aasworld ).clusterareacache[clusternum][clusterareanum] = cache;
		AAS_UpdateAreaRoutingCache( cache );
	} //end if
	  //the cache has been accessed
	cache->time = AAS_RoutingTime();
	return cache;
} //end of the function AAS_GetAreaRoutingCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_UpdatePortalRoutingCache( aas_routingcache_t *portalcache ) {
	int i, portalnum, clusterareanum, clusternum;
	unsigned short int t;
	aas_portal_t *portal;
	aas_cluster_t *cluster;
	aas_routingcache_t *cache;
	aas_routingupdate_t *updateliststart, *updatelistend, *curupdate, *nextupdate;

#ifdef ROUTING_DEBUG
	numportalcacheupdates++;
#endif //ROUTING_DEBUG
	   //clear the routing update fields
//	memset((*aasworld).portalupdate, 0, ((*aasworld).numportals+1) * sizeof(aas_routingupdate_t));
	//
	curupdate = &( *aasworld ).portalupdate[( *aasworld ).numportals];
	curupdate->cluster = portalcache->cluster;
	curupdate->areanum = portalcache->areanum;
	curupdate->tmptraveltime = portalcache->starttraveltime;
	//if the start area is a cluster portal, store the travel time for that portal
	clusternum = ( *aasworld ).areasettings[portalcache->areanum].cluster;
	if ( clusternum < 0 ) {
		portalcache->traveltimes[-clusternum] = portalcache->starttraveltime;
	} //end if
	  //put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend = curupdate;
	//while there are updates in the current list, flip the lists
	while ( updateliststart )
	{
		curupdate = updateliststart;
		//remove the current update from the list
		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else { updatelistend = NULL;}
		updateliststart = curupdate->next;
		//current update is removed from the list
		curupdate->inlist = qfalse;
		//
		cluster = &( *aasworld ).clusters[curupdate->cluster];
		//
		cache = AAS_GetAreaRoutingCache( curupdate->cluster,
										 curupdate->areanum, portalcache->travelflags, qtrue );
		//take all portals of the cluster
		for ( i = 0; i < cluster->numportals; i++ )
		{
			portalnum = ( *aasworld ).portalindex[cluster->firstportal + i];
			portal = &( *aasworld ).portals[portalnum];
			//if this is the portal of the current update continue
			if ( portal->areanum == curupdate->areanum ) {
				continue;
			}
			//
			clusterareanum = AAS_ClusterAreaNum( curupdate->cluster, portal->areanum );
			if ( clusterareanum >= cluster->numreachabilityareas ) {
				continue;
			}
			//
			t = cache->traveltimes[clusterareanum];
			if ( !t ) {
				continue;
			}
			t += curupdate->tmptraveltime;
			//
			if ( !portalcache->traveltimes[portalnum] ||
				 portalcache->traveltimes[portalnum] > t ) {
				portalcache->traveltimes[portalnum] = t;
				portalcache->reachabilities[portalnum] = cache->reachabilities[clusterareanum];
				nextupdate = &( *aasworld ).portalupdate[portalnum];
				if ( portal->frontcluster == curupdate->cluster ) {
					nextupdate->cluster = portal->backcluster;
				} //end if
				else
				{
					nextupdate->cluster = portal->frontcluster;
				} //end else
				nextupdate->areanum = portal->areanum;
				//add travel time through actual portal area for the next update
				nextupdate->tmptraveltime = t + ( *aasworld ).portalmaxtraveltimes[portalnum];
				if ( !nextupdate->inlist ) {
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;
					if ( updatelistend ) {
						updatelistend->next = nextupdate;
					} else { updateliststart = nextupdate;}
					updatelistend = nextupdate;
					nextupdate->inlist = qtrue;
				} //end if
			} //end if
		} //end for
	} //end while
} //end of the function AAS_UpdatePortalRoutingCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
aas_routingcache_t *AAS_GetPortalRoutingCache( int clusternum, int areanum, int travelflags ) {
	aas_routingcache_t *cache;

	//find the cached portal routing if existing
	for ( cache = ( *aasworld ).portalcache[areanum]; cache; cache = cache->next )
	{
		if ( cache->travelflags == travelflags ) {
			break;
		}
	} //end for
	  //if the portal routing isn't cached
	if ( !cache ) {
		cache = AAS_AllocRoutingCache( ( *aasworld ).numportals );
		cache->cluster = clusternum;
		cache->areanum = areanum;
		VectorCopy( ( *aasworld ).areas[areanum].center, cache->origin );
		cache->starttraveltime = 1;
		cache->travelflags = travelflags;
		//add the cache to the cache list
		cache->prev = NULL;
		cache->next = ( *aasworld ).portalcache[areanum];
		if ( ( *aasworld ).portalcache[areanum] ) {
			( *aasworld ).portalcache[areanum]->prev = cache;
		}
		( *aasworld ).portalcache[areanum] = cache;
		//update the cache
		AAS_UpdatePortalRoutingCache( cache );
	} //end if
	  //the cache has been accessed
	cache->time = AAS_RoutingTime();
	return cache;
} //end of the function AAS_GetPortalRoutingCache
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaRouteToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags, int *traveltime, int *reachnum ) {
	int clusternum, goalclusternum, portalnum, i, clusterareanum, bestreachnum;
	unsigned short int t, besttime;
	aas_portal_t *portal;
	aas_cluster_t *cluster;
	aas_routingcache_t *areacache, *portalcache;
	aas_reachability_t *reach;
	aas_portalindex_t *pPortalnum;

	if ( !( *aasworld ).initialized ) {
		return qfalse;
	}

	if ( areanum == goalareanum ) {
		*traveltime = 1;
		*reachnum = 0;
		return qtrue;
	} //end if
	  //
	if ( areanum <= 0 || areanum >= ( *aasworld ).numareas ) {
		if ( bot_developer ) {
			botimport.Print( PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: areanum %d out of range\n", areanum );
		} //end if
		return qfalse;
	} //end if
	if ( goalareanum <= 0 || goalareanum >= ( *aasworld ).numareas ) {
		if ( bot_developer ) {
			botimport.Print( PRT_ERROR, "AAS_AreaTravelTimeToGoalArea: goalareanum %d out of range\n", goalareanum );
		} //end if
		return qfalse;
	} //end if

	//make sure the routing cache doesn't grow to large
	while ( routingcachesize > max_routingcachesize ) {
		if ( !AAS_FreeOldestCache() ) {
			break;
		}
	}
	//
	if ( AAS_AreaDoNotEnter( areanum ) || AAS_AreaDoNotEnter( goalareanum ) ) {
		travelflags |= TFL_DONOTENTER;
	} //end if
	if ( AAS_AreaDoNotEnterLarge( areanum ) || AAS_AreaDoNotEnterLarge( goalareanum ) ) {
		travelflags |= TFL_DONOTENTER_LARGE;
	} //end if
	  //NOTE: the number of routing updates is limited per frame
	  /*
	  if ((*aasworld).frameroutingupdates > MAX_FRAMEROUTINGUPDATES)
	  {
  #ifdef DEBUG
		  //Log_Write("WARNING: AAS_AreaTravelTimeToGoalArea: frame routing updates overflowed");
  #endif
		  return 0;
	  } //end if
	  */
	  //
	clusternum = ( *aasworld ).areasettings[areanum].cluster;
	goalclusternum = ( *aasworld ).areasettings[goalareanum].cluster;
	//check if the area is a portal of the goal area cluster
	if ( clusternum < 0 && goalclusternum > 0 ) {
		portal = &( *aasworld ).portals[-clusternum];
		if ( portal->frontcluster == goalclusternum ||
			 portal->backcluster == goalclusternum ) {
			clusternum = goalclusternum;
		} //end if
	} //end if
	  //check if the goalarea is a portal of the area cluster
	else if ( clusternum > 0 && goalclusternum < 0 ) {
		portal = &( *aasworld ).portals[-goalclusternum];
		if ( portal->frontcluster == clusternum ||
			 portal->backcluster == clusternum ) {
			goalclusternum = clusternum;
		} //end if
	} //end if
	  //if both areas are in the same cluster
	  //NOTE: there might be a shorter route via another cluster!!! but we don't care
	if ( clusternum > 0 && goalclusternum > 0 && clusternum == goalclusternum ) {
		//
		areacache = AAS_GetAreaRoutingCache( clusternum, goalareanum, travelflags, qfalse );
		// RF, note that the routing cache might be NULL now since we are restricting
		// the updates per frame, hopefully rejected cache's will be requested again
		// when things have settled down
		if ( !areacache ) {
			return qfalse;
		}
		//the number of the area in the cluster
		clusterareanum = AAS_ClusterAreaNum( clusternum, areanum );
		//the cluster the area is in
		cluster = &( *aasworld ).clusters[clusternum];
		//if the area is NOT a reachability area
		if ( clusterareanum >= cluster->numreachabilityareas ) {
			return qfalse;
		}
		//if it is possible to travel to the goal area through this cluster
		if ( areacache->traveltimes[clusterareanum] != 0 ) {
			*reachnum = ( *aasworld ).areasettings[areanum].firstreachablearea +
						areacache->reachabilities[clusterareanum];
			//
			if ( !origin ) {
				*traveltime = areacache->traveltimes[clusterareanum];
				return qtrue;
			}
			//
			reach = &( *aasworld ).reachability[*reachnum];
			*traveltime = areacache->traveltimes[clusterareanum] +
						  AAS_AreaTravelTime( areanum, origin, reach->start );
			return qtrue;
		} //end if
	} //end if
	  //
	clusternum = ( *aasworld ).areasettings[areanum].cluster;
	goalclusternum = ( *aasworld ).areasettings[goalareanum].cluster;
	//if the goal area is a portal
	if ( goalclusternum < 0 ) {
		//just assume the goal area is part of the front cluster
		portal = &( *aasworld ).portals[-goalclusternum];
		goalclusternum = portal->frontcluster;
	} //end if
	  //get the portal routing cache
	portalcache = AAS_GetPortalRoutingCache( goalclusternum, goalareanum, travelflags );
	//if the area is a cluster portal, read directly from the portal cache
	if ( clusternum < 0 ) {
		*traveltime = portalcache->traveltimes[-clusternum];
		*reachnum = ( *aasworld ).areasettings[areanum].firstreachablearea +
					portalcache->reachabilities[-clusternum];
		return qtrue;
	}
	//
	besttime = 0;
	bestreachnum = -1;
	//the cluster the area is in
	cluster = &( *aasworld ).clusters[clusternum];
	//current area inside the current cluster
	clusterareanum = AAS_ClusterAreaNum( clusternum, areanum );
	//if the area is NOT a reachability area
	if ( clusterareanum >= cluster->numreachabilityareas ) {
		return qfalse;
	}
	//
	pPortalnum = ( *aasworld ).portalindex + cluster->firstportal;
	//find the portal of the area cluster leading towards the goal area
	for ( i = 0; i < cluster->numportals; i++, pPortalnum++ )
	{
		portalnum = *pPortalnum;
		//if the goal area isn't reachable from the portal
		if ( !portalcache->traveltimes[portalnum] ) {
			continue;
		}
		//
		portal = ( *aasworld ).portals + portalnum;
		// if the area in disabled
		if ( ( *aasworld ).areasettings[portal->areanum].areaflags & AREA_DISABLED ) {
			continue;
		}
		//get the cache of the portal area
		areacache = AAS_GetAreaRoutingCache( clusternum, portal->areanum, travelflags, qfalse );
		// RF, this may be NULL if we were unable to calculate the cache this frame
		if ( !areacache ) {
			return qfalse;
		}
		//if the portal is NOT reachable from this area
		if ( !areacache->traveltimes[clusterareanum] ) {
			continue;
		}
		//total travel time is the travel time the portal area is from
		//the goal area plus the travel time towards the portal area
		t = portalcache->traveltimes[portalnum] + areacache->traveltimes[clusterareanum];
		//FIXME: add the exact travel time through the actual portal area
		//NOTE: for now we just add the largest travel time through the area portal
		//		because we can't directly calculate the exact travel time
		//		to be more specific we don't know which reachability is used to travel
		//		into the portal area when coming from the current area
		t += ( *aasworld ).portalmaxtraveltimes[portalnum];
		//
		// Ridah, needs to be up here
		*reachnum = ( *aasworld ).areasettings[areanum].firstreachablearea +
					areacache->reachabilities[clusterareanum];

//botimport.Print(PRT_MESSAGE, "portal reachability: %i\n", (int)areacache->reachabilities[clusterareanum] );

		if ( origin ) {
			reach = ( *aasworld ).reachability + *reachnum;
			t += AAS_AreaTravelTime( areanum, origin, reach->start );
		} //end if
		  //if the time is better than the one already found
		if ( !besttime || t < besttime ) {
			bestreachnum = *reachnum;
			besttime = t;
		} //end if
	} //end for
	  // Ridah, check a route was found
	if ( bestreachnum < 0 ) {
		return qfalse;
	}
	*reachnum = bestreachnum;
	*traveltime = besttime;
	return qtrue;
} //end of the function AAS_AreaRouteToGoalArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags ) {
	int traveltime, reachnum;

	if ( AAS_AreaRouteToGoalArea( areanum, origin, goalareanum, travelflags, &traveltime, &reachnum ) ) {
		return traveltime;
	}
	return 0;
} //end of the function AAS_AreaTravelTimeToGoalArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaTravelTimeToGoalAreaCheckLoop( int areanum, vec3_t origin, int goalareanum, int travelflags, int loopareanum ) {
	int traveltime, reachnum;
	aas_reachability_t *reach;

	if ( AAS_AreaRouteToGoalArea( areanum, origin, goalareanum, travelflags, &traveltime, &reachnum ) ) {
		reach = &( *aasworld ).reachability[reachnum];
		if ( loopareanum && reach->areanum == loopareanum ) {
			return 0;   // going here will cause a looped route
		}
		return traveltime;
	}
	return 0;
} //end of the function AAS_AreaTravelTimeToGoalArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaReachabilityToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags ) {
	int traveltime, reachnum;

	if ( AAS_AreaRouteToGoalArea( areanum, origin, goalareanum, travelflags, &traveltime, &reachnum ) ) {
		return reachnum;
	}
	return 0;
} //end of the function AAS_AreaReachabilityToGoalArea
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_ReachabilityFromNum( int num, struct aas_reachability_s *reach ) {
	if ( !( *aasworld ).initialized ) {
		memset( reach, 0, sizeof( aas_reachability_t ) );
		return;
	} //end if
	if ( num < 0 || num >= ( *aasworld ).reachabilitysize ) {
		memset( reach, 0, sizeof( aas_reachability_t ) );
		return;
	} //end if
	memcpy( reach, &( *aasworld ).reachability[num], sizeof( aas_reachability_t ) );;
} //end of the function AAS_ReachabilityFromNum
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_NextAreaReachability( int areanum, int reachnum ) {
	aas_areasettings_t *settings;

	if ( !( *aasworld ).initialized ) {
		return 0;
	}

	if ( areanum <= 0 || areanum >= ( *aasworld ).numareas ) {
		botimport.Print( PRT_ERROR, "AAS_NextAreaReachability: areanum %d out of range\n", areanum );
		return 0;
	} //end if

	settings = &( *aasworld ).areasettings[areanum];
	if ( !reachnum ) {
		return settings->firstreachablearea;
	} //end if
	if ( reachnum < settings->firstreachablearea ) {
		botimport.Print( PRT_FATAL, "AAS_NextAreaReachability: reachnum < settings->firstreachableara" );
		return 0;
	} //end if
	reachnum++;
	if ( reachnum >= settings->firstreachablearea + settings->numreachableareas ) {
		return 0;
	} //end if
	return reachnum;
} //end of the function AAS_NextAreaReachability
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_NextModelReachability( int num, int modelnum ) {
	int i;

	if ( num <= 0 ) {
		num = 1;
	} else if ( num >= ( *aasworld ).reachabilitysize )  {
		return 0;
	} else { num++;}
	//
	for ( i = num; i < ( *aasworld ).reachabilitysize; i++ )
	{
		if ( ( *aasworld ).reachability[i].traveltype == TRAVEL_ELEVATOR ) {
			if ( ( *aasworld ).reachability[i].facenum == modelnum ) {
				return i;
			}
		} //end if
		else if ( ( *aasworld ).reachability[i].traveltype == TRAVEL_FUNCBOB ) {
			if ( ( ( *aasworld ).reachability[i].facenum & 0x0000FFFF ) == modelnum ) {
				return i;
			}
		} //end if
	} //end for
	return 0;
} //end of the function AAS_NextModelReachability
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_RandomGoalArea( int areanum, int travelflags, int *goalareanum, vec3_t goalorigin ) {
	int i, n, t;
	vec3_t start, end;
	aas_trace_t trace;

	//if the area has no reachabilities
	if ( !AAS_AreaReachability( areanum ) ) {
		return qfalse;
	}
	//
	n = ( *aasworld ).numareas * random();
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		if ( n <= 0 ) {
			n = 1;
		}
		if ( n >= ( *aasworld ).numareas ) {
			n = 1;
		}
		if ( AAS_AreaReachability( n ) ) {
			t = AAS_AreaTravelTimeToGoalArea( areanum, ( *aasworld ).areas[areanum].center, n, travelflags );
			//if the goal is reachable
			if ( t > 0 ) {
				if ( AAS_AreaSwim( n ) ) {
					*goalareanum = n;
					VectorCopy( ( *aasworld ).areas[n].center, goalorigin );
					//botimport.Print(PRT_MESSAGE, "found random goal area %d\n", *goalareanum);
					return qtrue;
				} //end if
				VectorCopy( ( *aasworld ).areas[n].center, start );
				if ( !AAS_PointAreaNum( start ) ) {
					Log_Write( "area %d center %f %f %f in solid?", n,
							   start[0], start[1], start[2] );
				}
				VectorCopy( start, end );
				end[2] -= 300;
				trace = AAS_TraceClientBBox( start, end, PRESENCE_CROUCH, -1 );
				if ( !trace.startsolid && AAS_PointAreaNum( trace.endpos ) == n ) {
					if ( AAS_AreaGroundFaceArea( n ) > 300 ) {
						*goalareanum = n;
						VectorCopy( trace.endpos, goalorigin );
						//botimport.Print(PRT_MESSAGE, "found random goal area %d\n", *goalareanum);
						return qtrue;
					} //end if
				} //end if
			} //end if
		} //end if
		n++;
	} //end for
	return qfalse;
} //end of the function AAS_RandomGoalArea
//===========================================================================
// run-length compression on zeros
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_CompressVis( byte *vis, int numareas, byte *dest ) {
	int j;
	int rep;
	//int		visrow;
	byte    *dest_p;
	byte check;

	//
	dest_p = dest;
	//visrow = (numareas + 7)>>3;

	for ( j = 0 ; j < numareas /*visrow*/ ; j++ )
	{
		*dest_p++ = vis[j];
		check = vis[j];
		//if (vis[j])
		//	continue;

		rep = 1;
		for ( j++; j < numareas /*visrow*/ ; j++ )
			if ( vis[j] != check || rep == 255 ) {
				break;
			} else {
				rep++;
			}
		*dest_p++ = rep;
		j--;
	}
	return dest_p - dest;
} //end of the function AAS_CompressVis
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_DecompressVis( byte *in, int numareas, byte *decompressed ) {
	byte c;
	byte    *out;
	//int		row;
	byte    *end;

	// initialize the vis data, only set those that are visible
	memset( decompressed, 0, numareas );

	//row = (numareas+7)>>3;
	out = decompressed;
	end = ( byte * )( (int)decompressed + numareas );

	do
	{
		/*
		if (*in)
		{
			*out++ = *in++;
			continue;
		}
		*/

		c = in[1];
		if ( !c ) {
			AAS_Error( "DecompressVis: 0 repeat" );
		}
		if ( *in ) { // we need to set these bits
			memset( out, 1, c );
		}
		in += 2;
		/*
		while (c)
		{
			*out++ = 0;
			c--;
		}
		*/
		out += c;
	} while ( out < end );
} //end of the function AAS_DecompressVis
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_AreaVisible( int srcarea, int destarea ) {
	if ( srcarea != ( *aasworld ).decompressedvisarea ) {
		if ( !( *aasworld ).areavisibility[srcarea] ) {
			return qfalse;
		}
		AAS_DecompressVis( ( *aasworld ).areavisibility[srcarea],
						   ( *aasworld ).numareas, ( *aasworld ).decompressedvis );
		( *aasworld ).decompressedvisarea = srcarea;
	}
	return ( *aasworld ).decompressedvis[destarea];
} //end of the function AAS_AreaVisible
//===========================================================================
// just center to center visibility checking...
// FIXME: implement a correct full vis
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void AAS_CreateVisibility( void ) {
	int i, j, size, totalsize;
	vec3_t endpos, mins, maxs;
	bsp_trace_t trace;
	byte *buf;
	byte *validareas;
	int numvalid = 0;
	byte *areaTable = NULL;
	int numAreas, numAreaBits;

	numAreas = ( *aasworld ).numareas;
	numAreaBits = ( ( numAreas + 8 ) >> 3 );
	areaTable = (byte *) GetClearedMemory( numAreas * numAreaBits * sizeof( byte ) );

	buf = (byte *) GetClearedMemory( numAreas * 2 * sizeof( byte ) );   // in case it ends up bigger than the decompressedvis, which is rare but possible
	validareas = (byte *) GetClearedMemory( numAreas * sizeof( byte ) );

	( *aasworld ).areavisibility = (byte **) GetClearedMemory( numAreas * sizeof( byte * ) );
	( *aasworld ).decompressedvis = (byte *) GetClearedMemory( numAreas * sizeof( byte ) );
	( *aasworld ).areawaypoints = (vec3_t *) GetClearedMemory( numAreas * sizeof( vec3_t ) );
	totalsize = numAreas * sizeof( byte * );
	for ( i = 1; i < numAreas; i++ )
	{
		if ( !AAS_AreaReachability( i ) ) {
			continue;
		}

		// find the waypoint
		VectorCopy( ( *aasworld ).areas[i].center, endpos );
		endpos[2] -= 256;
		AAS_PresenceTypeBoundingBox( PRESENCE_NORMAL, mins, maxs );
//		maxs[2] = 0;
		trace = AAS_Trace( ( *aasworld ).areas[i].center, mins, maxs, endpos, -1, CONTENTS_SOLID | CONTENTS_PLAYERCLIP | CONTENTS_MONSTERCLIP );
		if ( !trace.startsolid && trace.fraction < 1 && AAS_PointAreaNum( trace.endpos ) == i ) {
			VectorCopy( trace.endpos, ( *aasworld ).areawaypoints[i] );
			validareas[i] = 1;
			numvalid++;
		} else {
			continue;
		}
	}

	for ( i = 1; i < numAreas; i++ )
	{
		if ( !validareas[i] ) {
			continue;
		}

		for ( j = 1; j < numAreas; j++ )
		{
			( *aasworld ).decompressedvis[j] = 0;
			if ( i == j ) {
				( *aasworld ).decompressedvis[j] = 1;
				if ( areaTable ) {
					areaTable[ ( i * numAreaBits ) + ( j >> 3 ) ] |= ( 1 << ( j & 7 ) );
				}
				continue;
			}
			if ( !validareas[j] ) {
				continue;
			} //end if
			  // if we have already checked this combination, copy the result
			if ( areaTable && ( i > j ) ) {
				// use the reverse result stored in the table
				if ( areaTable[ ( j * numAreaBits ) + ( i >> 3 ) ] & ( 1 << ( i & 7 ) ) ) {
					( *aasworld ).decompressedvis[j] = 1;
				}
				// done, move to the next area
				continue;
			}

			// RF, check PVS first, since it's much faster
			if ( !AAS_inPVS( ( *aasworld ).areawaypoints[i], ( *aasworld ).areawaypoints[j] ) ) {
				continue;
			}
			trace = AAS_Trace( ( *aasworld ).areawaypoints[i], NULL, NULL, ( *aasworld ).areawaypoints[j], -1, CONTENTS_SOLID );
			if ( trace.fraction >= 1 ) {
				( *aasworld ).decompressedvis[j] = 1;
			} //end if
		} //end for
		size = AAS_CompressVis( ( *aasworld ).decompressedvis, numAreas, buf );
		( *aasworld ).areavisibility[i] = (byte *) GetMemory( size );
		memcpy( ( *aasworld ).areavisibility[i], buf, size );
		totalsize += size;
	} //end for
	if ( areaTable ) {
		FreeMemory( areaTable );
	}
	botimport.Print( PRT_MESSAGE, "AAS_CreateVisibility: compressed vis size = %i\n", totalsize );
} //end of the function AAS_CreateVisibility
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
float VectorDistance( vec3_t v1, vec3_t v2 );
extern void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj ) ;
int AAS_NearestHideArea( int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags ) {
	int i, j, nextareanum, badtravelflags, numreach, bestarea;
	unsigned short int t, besttraveltime, enemytraveltime;
	aas_routingupdate_t *updateliststart, *updatelistend, *curupdate, *nextupdate;
	aas_reachability_t *reach;
	float dist1, dist2;
	float enemytraveldist;
	vec3_t enemyVec;
	qboolean startVisible;
	vec3_t v1, v2, p;
	#define MAX_HIDEAREA_LOOPS  3000
	static float lastTime;
	static int loopCount;
	//
	if ( srcnum < 0 ) {   // hack to force run this call
		srcnum = -srcnum - 1;
		lastTime = 0;
	}
	// don't run this more than once per frame
	if ( lastTime == AAS_Time() && loopCount >= MAX_HIDEAREA_LOOPS ) {
		return 0;
	}
	lastTime = AAS_Time();
	loopCount = 0;
	//
	if ( !( *aasworld ).hidetraveltimes ) {
		( *aasworld ).hidetraveltimes = (unsigned short int *) GetClearedMemory( ( *aasworld ).numareas * sizeof( unsigned short int ) );
	} else {
		memset( ( *aasworld ).hidetraveltimes, 0, ( *aasworld ).numareas * sizeof( unsigned short int ) );
	} //end else
	  //
	if ( !( *aasworld ).visCache ) {
		( *aasworld ).visCache = (byte *) GetClearedMemory( ( *aasworld ).numareas * sizeof( byte ) );
	} else {
		memset( ( *aasworld ).visCache, 0, ( *aasworld ).numareas * sizeof( byte ) );
	} //end else
	besttraveltime = 0;
	bestarea = 0;
	if ( enemyareanum ) {
		enemytraveltime = AAS_AreaTravelTimeToGoalArea( areanum, origin, enemyareanum, travelflags );
	}
	VectorSubtract( enemyorigin, origin, enemyVec );
	enemytraveldist = VectorNormalize( enemyVec );
	startVisible = botimport.AICast_VisibleFromPos( enemyorigin, enemynum, origin, srcnum, qfalse );
	//
	badtravelflags = ~travelflags;
	//
	curupdate = &( *aasworld ).areaupdate[areanum];
	curupdate->areanum = areanum;
	VectorCopy( origin, curupdate->start );
	curupdate->areatraveltimes = ( *aasworld ).areatraveltimes[areanum][0];
	curupdate->tmptraveltime = 0;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend = curupdate;
	//while there are updates in the current list, flip the lists
	while ( updateliststart )
	{
		curupdate = updateliststart;
		//
		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else { updatelistend = NULL;}
		updateliststart = curupdate->next;
		//
		curupdate->inlist = qfalse;
		//check all reversed reachability links
		numreach = ( *aasworld ).areasettings[curupdate->areanum].numreachableareas;
		reach = &( *aasworld ).reachability[( *aasworld ).areasettings[curupdate->areanum].firstreachablearea];
		//
		for ( i = 0; i < numreach; i++, reach++ )
		{
			//if an undesired travel type is used
			if ( ( *aasworld ).travelflagfortype[reach->traveltype] & badtravelflags ) {
				continue;
			}
			//
			if ( AAS_AreaContentsTravelFlag( reach->areanum ) & badtravelflags ) {
				continue;
			}
			// dont pass through ladder areas
			if ( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_LADDER ) {
				continue;
			}
			//
			if ( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_DISABLED ) {
				continue;
			}
			//number of the area the reachability leads to
			nextareanum = reach->areanum;
			// if this moves us into the enemies area, skip it
			if ( nextareanum == enemyareanum ) {
				continue;
			}
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			t = curupdate->tmptraveltime +
				AAS_AreaTravelTime( curupdate->areanum, curupdate->start, reach->start ) +
				reach->traveltime;
			// inc the loopCount, we are starting to use a bit of cpu time
			loopCount++;
			// if this isn't the fastest route to this area, ignore
			if ( ( *aasworld ).hidetraveltimes[nextareanum] && ( *aasworld ).hidetraveltimes[nextareanum] < t ) {
				continue;
			}
			( *aasworld ).hidetraveltimes[nextareanum] = t;
			// if the bestarea is this area, then it must be a longer route, so ignore it
			if ( bestarea == nextareanum ) {
				bestarea = 0;
				besttraveltime = 0;
			}
			// do this test now, so we can reject the route if it starts out too long
			if ( besttraveltime && t >= besttraveltime ) {
				continue;
			}
			//
			//avoid going near the enemy
			ProjectPointOntoVector( enemyorigin, curupdate->start, reach->end, p );
			for ( j = 0; j < 3; j++ ) {
				if ( ( p[j] > curupdate->start[j] + 0.1 && p[j] > reach->end[j] + 0.1 ) ||
					 ( p[j] < curupdate->start[j] - 0.1 && p[j] < reach->end[j] - 0.1 ) ) {
					break;
				}
			}
			if ( j < 3 ) {
				VectorSubtract( enemyorigin, reach->end, v2 );
			} //end if
			else
			{
				VectorSubtract( enemyorigin, p, v2 );
			} //end else
			dist2 = VectorLength( v2 );
			//never go through the enemy
			if ( enemytraveldist > 32 && dist2 < enemytraveldist && dist2 < 256 ) {
				continue;
			}
			//
			VectorSubtract( reach->end, origin, v2 );
			if ( enemytraveldist > 32 && DotProduct( v2, enemyVec ) > enemytraveldist / 2 ) {
				continue;
			}
			//
			VectorSubtract( enemyorigin, curupdate->start, v1 );
			dist1 = VectorLength( v1 );
			//
			if ( enemytraveldist > 32 && dist2 < dist1 ) {
				t += ( dist1 - dist2 ) * 10;
				// test it again after modifying it
				if ( besttraveltime && t >= besttraveltime ) {
					continue;
				}
			}
			// make sure the hide area doesn't have anyone else in it
			if ( AAS_IsEntityInArea( srcnum, -1, nextareanum ) ) {
				t += 1000;  // avoid this path/area
				//continue;
			}
			//
			// if we weren't visible when starting, make sure we don't move into their view
			if ( enemyareanum && !startVisible && AAS_AreaVisible( enemyareanum, nextareanum ) ) {
				continue;
				//t += 1000;
			}
			//
			if ( !besttraveltime || besttraveltime > t ) {
				//
				// if this area doesn't have a vis list, ignore it
				if ( ( *aasworld ).areavisibility[nextareanum] ) {
					//if the nextarea is not visible from the enemy area
					if ( !AAS_AreaVisible( enemyareanum, nextareanum ) ) { // now last of all, check that this area is a safe hiding spot
						if (    ( ( *aasworld ).visCache[nextareanum] == 2 ) ||
								( !( *aasworld ).visCache[nextareanum] && !botimport.AICast_VisibleFromPos( enemyorigin, enemynum, ( *aasworld ).areawaypoints[nextareanum], srcnum, qfalse ) ) ) {
							( *aasworld ).visCache[nextareanum] = 2;
							besttraveltime = t;
							bestarea = nextareanum;
						} else {
							( *aasworld ).visCache[nextareanum] = 1;
						}
					} //end if
				}
				//
				// getting down to here is bad for cpu usage
				if ( loopCount++ > MAX_HIDEAREA_LOOPS ) {
					//botimport.Print(PRT_MESSAGE, "AAS_NearestHideArea: exceeded max loops, aborting\n" );
					continue;
				}
				//
				// otherwise, add this to the list so we check is reachables
				// disabled, this should only store the raw traveltime, not the adjusted time
				//(*aasworld).hidetraveltimes[nextareanum] = t;
				nextupdate = &( *aasworld ).areaupdate[nextareanum];
				nextupdate->areanum = nextareanum;
				nextupdate->tmptraveltime = t;
				//remember where we entered this area
				VectorCopy( reach->end, nextupdate->start );
				//if this update is not in the list yet
				if ( !nextupdate->inlist ) {
					//add the new update to the end of the list
					nextupdate->next = NULL;
					nextupdate->prev = updatelistend;
					if ( updatelistend ) {
						updatelistend->next = nextupdate;
					} else { updateliststart = nextupdate;}
					updatelistend = nextupdate;
					nextupdate->inlist = qtrue;
				} //end if
			} //end if
		} //end for
	} //end while
	  //botimport.Print(PRT_MESSAGE, "AAS_NearestHideArea: hidearea: %i, %i loops\n", bestarea, count );
	return bestarea;
} //end of the function AAS_NearestHideArea

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int AAS_FindAttackSpotWithinRange( int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float *outpos ) {
	int i, nextareanum, badtravelflags, numreach, bestarea;
	unsigned short int t, besttraveltime, enemytraveltime;
	aas_routingupdate_t *updateliststart, *updatelistend, *curupdate, *nextupdate;
	aas_reachability_t *reach;
	vec3_t srcorg, rangeorg, enemyorg;
	int srcarea, rangearea, enemyarea;
	unsigned short int srctraveltime;
	int count = 0;
	#define MAX_ATTACKAREA_LOOPS    200
	static float lastTime;
	//
	// RF, currently doesn't work with multiple AAS worlds, so only enable for the default world
	//if (aasworld != aasworlds) return 0;
	//
	// don't run this more than once per frame
	if ( lastTime == AAS_Time() ) {
		return 0;
	}
	lastTime = AAS_Time();
	//
	if ( !( *aasworld ).hidetraveltimes ) {
		( *aasworld ).hidetraveltimes = (unsigned short int *) GetClearedMemory( ( *aasworld ).numareas * sizeof( unsigned short int ) );
	} else {
		memset( ( *aasworld ).hidetraveltimes, 0, ( *aasworld ).numareas * sizeof( unsigned short int ) );
	} //end else
	  //
	if ( !( *aasworld ).visCache ) {
		( *aasworld ).visCache = (byte *) GetClearedMemory( ( *aasworld ).numareas * sizeof( byte ) );
	} else {
		memset( ( *aasworld ).visCache, 0, ( *aasworld ).numareas * sizeof( byte ) );
	} //end else
	  //
	AAS_EntityOrigin( srcnum, srcorg );
	AAS_EntityOrigin( rangenum, rangeorg );
	AAS_EntityOrigin( enemynum, enemyorg );
	//
	srcarea = BotFuzzyPointReachabilityArea( srcorg );
	rangearea = BotFuzzyPointReachabilityArea( rangeorg );
	enemyarea = BotFuzzyPointReachabilityArea( enemyorg );
	//
	besttraveltime = 0;
	bestarea = 0;
	enemytraveltime = AAS_AreaTravelTimeToGoalArea( srcarea, srcorg, enemyarea, travelflags );
	//
	badtravelflags = ~travelflags;
	//
	curupdate = &( *aasworld ).areaupdate[rangearea];
	curupdate->areanum = rangearea;
	VectorCopy( rangeorg, curupdate->start );
	curupdate->areatraveltimes = ( *aasworld ).areatraveltimes[srcarea][0];
	curupdate->tmptraveltime = 0;
	//put the area to start with in the current read list
	curupdate->next = NULL;
	curupdate->prev = NULL;
	updateliststart = curupdate;
	updatelistend = curupdate;
	//while there are updates in the current list, flip the lists
	while ( updateliststart )
	{
		curupdate = updateliststart;
		//
		if ( curupdate->next ) {
			curupdate->next->prev = NULL;
		} else { updatelistend = NULL;}
		updateliststart = curupdate->next;
		//
		curupdate->inlist = qfalse;
		//check all reversed reachability links
		numreach = ( *aasworld ).areasettings[curupdate->areanum].numreachableareas;
		reach = &( *aasworld ).reachability[( *aasworld ).areasettings[curupdate->areanum].firstreachablearea];
		//
		for ( i = 0; i < numreach; i++, reach++ )
		{
			//if an undesired travel type is used
			if ( ( *aasworld ).travelflagfortype[reach->traveltype] & badtravelflags ) {
				continue;
			}
			//
			if ( AAS_AreaContentsTravelFlag( reach->areanum ) & badtravelflags ) {
				continue;
			}
			// dont pass through ladder areas
			if ( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_LADDER ) {
				continue;
			}
			//
			if ( ( *aasworld ).areasettings[reach->areanum].areaflags & AREA_DISABLED ) {
				continue;
			}
			//number of the area the reachability leads to
			nextareanum = reach->areanum;
			// if this moves us into the enemies area, skip it
			if ( nextareanum == enemyarea ) {
				continue;
			}
			// if we've already been to this area
			if ( ( *aasworld ).hidetraveltimes[nextareanum] ) {
				continue;
			}
			//time already travelled plus the traveltime through
			//the current area plus the travel time from the reachability
			if ( count++ > MAX_ATTACKAREA_LOOPS ) {
				//botimport.Print(PRT_MESSAGE, "AAS_FindAttackSpotWithinRange: exceeded max loops, aborting\n" );
				if ( bestarea ) {
					VectorCopy( ( *aasworld ).areawaypoints[bestarea], outpos );
				}
				return bestarea;
			}
			t = curupdate->tmptraveltime +
				AAS_AreaTravelTime( curupdate->areanum, curupdate->start, reach->start ) +
				reach->traveltime;
			//
			// if it's too far from rangenum, ignore
			if ( Distance( rangeorg, ( *aasworld ).areawaypoints[nextareanum] ) > rangedist ) {
				continue;
			}
			//
			// find the traveltime from srcnum
			srctraveltime = AAS_AreaTravelTimeToGoalArea( srcarea, srcorg, nextareanum, travelflags );
			// do this test now, so we can reject the route if it starts out too long
			if ( besttraveltime && srctraveltime >= besttraveltime ) {
				continue;
			}
			//
			// if this area doesn't have a vis list, ignore it
			if ( ( *aasworld ).areavisibility[nextareanum] ) {
				//if the nextarea can see the enemy area
				if ( AAS_AreaVisible( enemyarea, nextareanum ) ) { // now last of all, check that this area is a good attacking spot
					if (    ( ( *aasworld ).visCache[nextareanum] == 2 ) ||
							(   !( *aasworld ).visCache[nextareanum] &&
								( count += 10 ) &&    // we are about to use lots of CPU time
								botimport.AICast_CheckAttackAtPos( srcnum, enemynum, ( *aasworld ).areawaypoints[nextareanum], qfalse, qfalse ) ) ) {
						( *aasworld ).visCache[nextareanum] = 2;
						besttraveltime = srctraveltime;
						bestarea = nextareanum;
					} else {
						( *aasworld ).visCache[nextareanum] = 1;
					}
				} //end if
			}
			( *aasworld ).hidetraveltimes[nextareanum] = t;
			nextupdate = &( *aasworld ).areaupdate[nextareanum];
			nextupdate->areanum = nextareanum;
			nextupdate->tmptraveltime = t;
			//remember where we entered this area
			VectorCopy( reach->end, nextupdate->start );
			//if this update is not in the list yet
			if ( !nextupdate->inlist ) {
				//add the new update to the end of the list
				nextupdate->next = NULL;
				nextupdate->prev = updatelistend;
				if ( updatelistend ) {
					updatelistend->next = nextupdate;
				} else { updateliststart = nextupdate;}
				updatelistend = nextupdate;
				nextupdate->inlist = qtrue;
			} //end if
		} //end for
	} //end while
//botimport.Print(PRT_MESSAGE, "AAS_NearestHideArea: hidearea: %i, %i loops\n", bestarea, count );
	if ( bestarea ) {
		VectorCopy( ( *aasworld ).areawaypoints[bestarea], outpos );
	}
	return bestarea;
} //end of the function AAS_NearestHideArea

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
qboolean AAS_GetRouteFirstVisPos( vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos ) {
	int srcarea, destarea, travarea;
	vec3_t travpos;
	int ftraveltime, freachnum, lasttraveltime;
	aas_reachability_t reach;
	int loops = 0;
#define MAX_GETROUTE_VISPOS_LOOPS   200
	//
	// SRCPOS: enemy
	// DESTPOS: self
	// RETPOS: first area that is visible from destpos, in route from srcpos to destpos
	srcarea = BotFuzzyPointReachabilityArea( srcpos );
	if ( !srcarea ) {
		return qfalse;
	}
	destarea = BotFuzzyPointReachabilityArea( destpos );
	if ( !destarea ) {
		return qfalse;
	}
	if ( destarea == srcarea ) {
		VectorCopy( srcpos, retpos );
		return qtrue;
	}
	//
	//if the srcarea can see the destarea
	if ( AAS_AreaVisible( srcarea, destarea ) ) {
		VectorCopy( srcpos, retpos );
		return qtrue;
	}
	// if this area doesn't have a vis list, ignore it
	if ( !( *aasworld ).areavisibility[destarea] ) {
		return qfalse;
	}
	//
	travarea = srcarea;
	VectorCopy( srcpos, travpos );
	lasttraveltime = -1;
	while ( ( loops++ < MAX_GETROUTE_VISPOS_LOOPS ) && AAS_AreaRouteToGoalArea( travarea, travpos, destarea, travelflags, &ftraveltime, &freachnum ) ) {
		if ( lasttraveltime >= 0 && ftraveltime >= lasttraveltime ) {
			return qfalse;  // we may be in a loop
		}
		lasttraveltime = ftraveltime;
		//
		AAS_ReachabilityFromNum( freachnum, &reach );
		if ( reach.areanum == destarea ) {
			VectorCopy( travpos, retpos );
			return qtrue;
		}
		//if the reach area can see the destarea
		if ( AAS_AreaVisible( reach.areanum, destarea ) ) {
			VectorCopy( reach.end, retpos );
			return qtrue;
		}
		//
		travarea = reach.areanum;
		VectorCopy( reach.end, travpos );
	}
	//
	// unsuccessful
	return qfalse;
}