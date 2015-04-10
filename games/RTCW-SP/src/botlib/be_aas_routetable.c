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
// Name:			be_aas_routetable.c
// Function:		Area Awareness System, Route-table defines
// Programmer:		Ridah
// Tab Size:		3
//===========================================================================

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_libvar.h"
#include "l_utils.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_interface.h"
#include "be_aas_def.h"

// ugly hack to turn off route-tables, can't find a way to check cvar's
int disable_routetable = 0;

// this must be enabled for the route-tables to work, but it's not fully operational yet
#define CHECK_TRAVEL_TIMES
//#define DEBUG_ROUTETABLE
#define FILTERAREAS

// enable this to use the built-in route-cache system to find the routes
#define USE_ROUTECACHE

// enable this to disable Rocket/BFG Jumping, Grapple Hook
#define FILTER_TRAVEL

// hmm, is there a cleaner way of finding out memory usage?
extern int totalmemorysize;
static int memorycount, cachememory;

// globals to reduce function parameters
static unsigned short int *filtered_areas, childcount, num_parents;
static unsigned short int *rev_filtered_areas;

// misc defines
unsigned short CRC_ProcessString( unsigned char *data, int length );


//===========================================================================
// Memory debugging/optimization

void *AAS_RT_GetClearedMemory( unsigned long size ) {
	void *ptr;

	memorycount += size;

	// ptr = GetClearedMemory(size);
	//ptr = GetClearedHunkMemory(size);
	// Ryan - 01102k, need to use this, since the routetable calculations use up a lot of memory
	// this will be a non-issue once we transfer the remnants of the routetable over to the aasworld
	ptr = malloc( size );
	memset( ptr, 0, size );

	return ptr;
}

void AAS_RT_FreeMemory( void *ptr ) {
	int before;

	before = totalmemorysize;

	// FreeMemory( ptr );
	// Ryan - 01102k
	free( ptr );

	memorycount -= before - totalmemorysize;
}

void AAS_RT_PrintMemoryUsage() {
#ifdef  AAS_RT_MEMORY_USAGE

	botimport.Print( PRT_MESSAGE, "\n" );

	// TODO: print the usage from each of the aas_rt_t lumps

#endif
}
//===========================================================================


//===========================================================================
// return the number of unassigned areas that are in the given area's visible list
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_RT_GetValidVisibleAreasCount( aas_area_buildlocalinfo_t *localinfo, aas_area_childlocaldata_t **childlocaldata ) {
	int i, cnt;

	cnt = 1;        // assume it can reach itself

	for ( i = 0; i < localinfo->numvisible; i++ )
	{
		if ( childlocaldata[localinfo->visible[i]] ) {
			continue;
		}

		cnt++;
	}

	return cnt;
}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
static aas_rt_route_t   **routetable;

int AAS_AreaRouteToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags, int *traveltime, int *reachnum );

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================

void AAS_RT_CalcTravelTimesToGoalArea( int goalarea ) {
	int i;
	// TTimo: unused
//	static int tfl = TFL_DEFAULT & ~(TFL_JUMPPAD|TFL_ROCKETJUMP|TFL_BFGJUMP|TFL_GRAPPLEHOOK|TFL_DOUBLEJUMP|TFL_RAMPJUMP|TFL_STRAFEJUMP|TFL_LAVA);	//----(SA)	modified since slime is no longer deadly
//	static int tfl = TFL_DEFAULT & ~(TFL_JUMPPAD|TFL_ROCKETJUMP|TFL_BFGJUMP|TFL_GRAPPLEHOOK|TFL_DOUBLEJUMP|TFL_RAMPJUMP|TFL_STRAFEJUMP|TFL_SLIME|TFL_LAVA);
	aas_rt_route_t  *rt;
	int reach, travel;

	for ( i = 0; i < childcount; i++ ) {
		rt = &routetable[i][-1 + rev_filtered_areas[goalarea]];
		if ( AAS_AreaRouteToGoalArea( filtered_areas[i], ( *aasworld ).areas[filtered_areas[i]].center, goalarea, ~RTB_BADTRAVELFLAGS, &travel, &reach ) ) {
			rt->reachable_index = reach;
			rt->travel_time = travel;
		} else {
			rt->reachable_index = -1;
			rt->travel_time = 0;
		}
	}
}
//===========================================================================
// calculate the initial route-table for each filtered area to all other areas
//
// FIXME: this isn't fully operational yet, for some reason not all routes are found
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RT_CalculateRouteTable( aas_rt_route_t **parmroutetable ) {
	int i;

	routetable = parmroutetable;

	for ( i = 0; i < childcount; i++ )
	{
		AAS_RT_CalcTravelTimesToGoalArea( filtered_areas[i] );
	}
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RT_AddParentLink( aas_area_childlocaldata_t *child, int parentindex, int childindex ) {
	aas_parent_link_t       *oldparentlink;

	oldparentlink = child->parentlink;

	child->parentlink = (aas_parent_link_t *) AAS_RT_GetClearedMemory( sizeof( aas_parent_link_t ) );

	child->parentlink->childindex = (unsigned short int)childindex;
	child->parentlink->parent = (unsigned short int)parentindex;
	child->parentlink->next = oldparentlink;
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RT_WriteShort( unsigned short int si, fileHandle_t fp ) {
	unsigned short int lsi;

	lsi = LittleShort( si );
	botimport.FS_Write( &lsi, sizeof( lsi ), fp );
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RT_WriteByte( int si, fileHandle_t fp ) {
	unsigned char uc;

	uc = si;
	botimport.FS_Write( &uc, sizeof( uc ), fp );
}

//===========================================================================
//	writes the current route-table data to a .rtb file in tne maps folder
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RT_WriteRouteTable() {
	int ident, version;
	unsigned short crc_aas;
	fileHandle_t fp;
	char filename[MAX_QPATH];

	// open the file for writing
	Com_sprintf( filename, MAX_QPATH, "maps/%s.rtb", ( *aasworld ).mapname );
	botimport.Print( PRT_MESSAGE, "\nsaving route-table to %s\n", filename );
	botimport.FS_FOpenFile( filename, &fp, FS_WRITE );
	if ( !fp ) {
		AAS_Error( "Unable to open file: %s\n", filename );
		return;
	}

	// ident
	ident = LittleLong( RTBID );
	botimport.FS_Write( &ident, sizeof( ident ), fp );

	// version
	version = LittleLong( RTBVERSION );
	botimport.FS_Write( &version, sizeof( version ), fp );

	// crc
	crc_aas = CRC_ProcessString( (unsigned char *)( *aasworld ).areas, sizeof( aas_area_t ) * ( *aasworld ).numareas );
	botimport.FS_Write( &crc_aas, sizeof( crc_aas ), fp );

	// save the table data

	// children
	botimport.FS_Write( &( *aasworld ).routetable->numChildren, sizeof( int ), fp );
	botimport.FS_Write( ( *aasworld ).routetable->children, ( *aasworld ).routetable->numChildren * sizeof( aas_rt_child_t ), fp );

	// parents
	botimport.FS_Write( &( *aasworld ).routetable->numParents, sizeof( int ), fp );
	botimport.FS_Write( ( *aasworld ).routetable->parents, ( *aasworld ).routetable->numParents * sizeof( aas_rt_parent_t ), fp );

	// parentChildren
	botimport.FS_Write( &( *aasworld ).routetable->numParentChildren, sizeof( int ), fp );
	botimport.FS_Write( ( *aasworld ).routetable->parentChildren, ( *aasworld ).routetable->numParentChildren * sizeof( unsigned short int ), fp );

	// visibleParents
	botimport.FS_Write( &( *aasworld ).routetable->numVisibleParents, sizeof( int ), fp );
	botimport.FS_Write( ( *aasworld ).routetable->visibleParents, ( *aasworld ).routetable->numVisibleParents * sizeof( unsigned short int ), fp );

	// parentLinks
	botimport.FS_Write( &( *aasworld ).routetable->numParentLinks, sizeof( int ), fp );
	botimport.FS_Write( ( *aasworld ).routetable->parentLinks, ( *aasworld ).routetable->numParentLinks * sizeof( aas_rt_parent_link_t ), fp );

	botimport.FS_FCloseFile( fp );
	return;
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RT_DBG_Read( void *buf, int size, int fp ) {
	botimport.FS_Read( buf, size, fp );
}

//===========================================================================
//	reads the given file, and creates the structures required for the route-table system
//
// Parameter:				-
// Returns:					qtrue if succesful, qfalse if not
// Changes Globals:		-
//===========================================================================
#define DEBUG_READING_TIME
qboolean AAS_RT_ReadRouteTable( fileHandle_t fp ) {
	int ident, version, i;
	unsigned short int crc, crc_aas;
	aas_rt_t    *routetable;
	aas_rt_child_t  *child;
	aas_rt_parent_t *parent;
	aas_rt_parent_link_t *plink;
	unsigned short int  *psi;

	qboolean doswap;

#ifdef DEBUG_READING_TIME
	int pretime;

	pretime = Sys_MilliSeconds();
#endif

	routetable = ( *aasworld ).routetable;

	doswap = ( LittleLong( 1 ) != 1 );

	// check ident
	AAS_RT_DBG_Read( &ident, sizeof( ident ), fp );
	ident = LittleLong( ident );

	if ( ident != RTBID ) {
		AAS_Error( "File is not an RTB file\n" );
		botimport.FS_FCloseFile( fp );
		return qfalse;
	}

	// check version
	AAS_RT_DBG_Read( &version, sizeof( version ), fp );
	version = LittleLong( version );

	if ( version != RTBVERSION ) {
		AAS_Error( "File is version %i not %i\n", version, RTBVERSION );
		botimport.FS_FCloseFile( fp );
		return qfalse;
	}

	// read the CRC check on the AAS data
	AAS_RT_DBG_Read( &crc, sizeof( crc ), fp );
	crc = LittleShort( crc );

	// calculate a CRC on the AAS areas
	crc_aas = CRC_ProcessString( (unsigned char *)( *aasworld ).areas, sizeof( aas_area_t ) * ( *aasworld ).numareas );

	if ( crc != crc_aas ) {
		AAS_Error( "Route-table is from different AAS file, ignoring.\n" );
		botimport.FS_FCloseFile( fp );
		return qfalse;
	}

	// read the route-table

	// children
	botimport.FS_Read( &routetable->numChildren, sizeof( int ), fp );
	routetable->numChildren = LittleLong( routetable->numChildren );
	routetable->children = (aas_rt_child_t *) AAS_RT_GetClearedMemory( routetable->numChildren * sizeof( aas_rt_child_t ) );
	botimport.FS_Read( routetable->children, routetable->numChildren * sizeof( aas_rt_child_t ), fp );
	child = &routetable->children[0];
	if ( doswap ) {
		for ( i = 0; i < routetable->numChildren; i++, child++ ) {
			child->areanum = LittleShort( child->areanum );
			child->numParentLinks = LittleLong( child->numParentLinks );
			child->startParentLinks = LittleLong( child->startParentLinks );
		}
	}

	// parents
	botimport.FS_Read( &routetable->numParents, sizeof( int ), fp );
	routetable->numParents = LittleLong( routetable->numParents );
	routetable->parents = (aas_rt_parent_t *) AAS_RT_GetClearedMemory( routetable->numParents * sizeof( aas_rt_parent_t ) );
	botimport.FS_Read( routetable->parents, routetable->numParents * sizeof( aas_rt_parent_t ), fp );
	parent = &routetable->parents[0];
	if ( doswap ) {
		for ( i = 0; i < routetable->numParents; i++, parent++ ) {
			parent->areanum = LittleShort( parent->areanum );
			parent->numParentChildren = LittleLong( parent->numParentChildren );
			parent->startParentChildren = LittleLong( parent->startParentChildren );
			parent->numVisibleParents = LittleLong( parent->numVisibleParents );
			parent->startVisibleParents = LittleLong( parent->startVisibleParents );
		}
	}

	// parentChildren
	botimport.FS_Read( &routetable->numParentChildren, sizeof( int ), fp );
	routetable->numParentChildren = LittleLong( routetable->numParentChildren );
	routetable->parentChildren = (unsigned short int *) AAS_RT_GetClearedMemory( routetable->numParentChildren * sizeof( unsigned short int ) );
	botimport.FS_Read( routetable->parentChildren, routetable->numParentChildren * sizeof( unsigned short int ), fp );
	psi = &routetable->parentChildren[0];
	if ( doswap ) {
		for ( i = 0; i < routetable->numParentChildren; i++, psi++ ) {
			*psi = LittleShort( *psi );
		}
	}

	// visibleParents
	botimport.FS_Read( &routetable->numVisibleParents, sizeof( int ), fp );
	routetable->numVisibleParents = LittleLong( routetable->numVisibleParents );
	routetable->visibleParents = (unsigned short int *) AAS_RT_GetClearedMemory( routetable->numVisibleParents * sizeof( unsigned short int ) );
	botimport.FS_Read( routetable->visibleParents, routetable->numVisibleParents * sizeof( unsigned short int ), fp );
	psi = &routetable->visibleParents[0];
	if ( doswap ) {
		for ( i = 0; i < routetable->numVisibleParents; i++, psi++ ) {
			*psi = LittleShort( *psi );
		}
	}

	// parentLinks
	botimport.FS_Read( &routetable->numParentLinks, sizeof( int ), fp );
	routetable->numParentLinks = LittleLong( routetable->numParentLinks );
	routetable->parentLinks = (aas_rt_parent_link_t *) AAS_RT_GetClearedMemory( routetable->numParentLinks * sizeof( aas_rt_parent_link_t ) );
	botimport.FS_Read( routetable->parentLinks, routetable->numParentLinks * sizeof( aas_parent_link_t ), fp );
	plink = &routetable->parentLinks[0];
	if ( doswap ) {
		for ( i = 0; i < routetable->numParentLinks; i++, plink++ ) {
			plink->childIndex = LittleShort( plink->childIndex );
			plink->parent = LittleShort( plink->parent );
		}
	}

	// build the areaChildIndexes
	routetable->areaChildIndexes = (unsigned short int *) AAS_RT_GetClearedMemory( ( *aasworld ).numareas * sizeof( unsigned short int ) );
	child = routetable->children;
	for ( i = 0; i < routetable->numChildren; i++, child++ ) {
		routetable->areaChildIndexes[child->areanum] = i + 1;
	}

	botimport.Print( PRT_MESSAGE, "Total Parents: %d\n", routetable->numParents );
	botimport.Print( PRT_MESSAGE, "Total Children: %d\n", routetable->numChildren );
	botimport.Print( PRT_MESSAGE, "Total Memory Used: %d\n", memorycount );

#ifdef DEBUG_READING_TIME
	botimport.Print( PRT_MESSAGE, "Route-Table read time: %i\n", Sys_MilliSeconds() - pretime );
#endif

	botimport.FS_FCloseFile( fp );
	return qtrue;
}

int AAS_RT_NumParentLinks( aas_area_childlocaldata_t *child ) {
	aas_parent_link_t *plink;
	int i;

	i = 0;
	plink = child->parentlink;
	while ( plink )
	{
		i++;
		plink = plink->next;
	}

	return i;
}

//===========================================================================
//	main routine to build the route-table
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_CreateAllRoutingCache( void );

void AAS_RT_BuildRouteTable( void ) {
	int i,j,k;
	aas_area_t *srcarea;
	aas_areasettings_t  *srcsettings;
//	vec3_t	vec;
	unsigned int totalcount;
	unsigned int noroutecount;

	aas_area_buildlocalinfo_t   **area_localinfos;
	aas_area_buildlocalinfo_t   *localinfo;

	aas_area_childlocaldata_t   **area_childlocaldata;
	aas_area_childlocaldata_t   *child;

	aas_area_parent_t               *area_parents[MAX_PARENTS];
	aas_area_parent_t               *thisparent;

	int bestchild, bestcount, bestparent, cnt;

	int memoryend;

	unsigned short int      *visibleParents;

#ifdef CHECK_TRAVEL_TIMES
	aas_rt_route_t      **filteredroutetable;
	unsigned short int traveltime;
#endif

	fileHandle_t fp;
	char filename[MAX_QPATH];

// not used anymore
	return;

	// create the routetable in this aasworld
	aasworld->routetable = (aas_rt_t *) AAS_RT_GetClearedMemory( sizeof( aas_rt_t ) );

	// Try to load in a prepared route-table
	Com_sprintf( filename, MAX_QPATH, "maps/%s.rtb", ( *aasworld ).mapname );
	botimport.Print( PRT_MESSAGE, "\n---------------------------------\n" );
	botimport.Print( PRT_MESSAGE, "\ntrying to load %s\n", filename );
	botimport.FS_FOpenFile( filename, &fp, FS_READ );
	if ( fp ) {
		// read in the table..
		if ( AAS_RT_ReadRouteTable( fp ) ) {
			AAS_RT_PrintMemoryUsage();

			botimport.Print( PRT_MESSAGE, "\nAAS Route-Table loaded.\n" );
			botimport.Print( PRT_MESSAGE, "---------------------------------\n\n" );
			return;
		} else
		{
			botimport.Print( PRT_MESSAGE, "\nUnable to load %s, building route-table..\n", filename );
		}
	} else
	{
		botimport.Print( PRT_MESSAGE, "file not found, building route-table\n\n" );
	}


	botimport.Print( PRT_MESSAGE, "\n-------------------------------------\nRoute-table memory usage figures..\n\n" );

	totalcount = 0;
	childcount = 0;
	noroutecount = 0;
	childcount = 0;
	num_parents = 0;

	memorycount = 0;
	cachememory = 0;

	filtered_areas = (unsigned short int *) AAS_RT_GetClearedMemory( ( *aasworld ).numareas * sizeof( unsigned short int ) );
	rev_filtered_areas = (unsigned short int *) AAS_RT_GetClearedMemory( ( *aasworld ).numareas * sizeof( unsigned short int ) );

	// to speed things up, build a list of FILTERED areas first
	// do this so we can check for filtered areas
	AAS_CreateAllRoutingCache();
	for ( i = 0; i < ( *aasworld ).numareas; i++ )
	{
		srcarea = &( *aasworld ).areas[i];
		srcsettings = &( *aasworld ).areasettings[i];

#ifdef FILTERAREAS
		if ( !( srcsettings->areaflags & ( AREA_USEFORROUTING ) ) ) {
			continue;
		}
		if ( !( srcsettings->areaflags & ( AREA_GROUNDED | AREA_LIQUID | AREA_LADDER ) ) ) {
			continue;
		}
#endif

		rev_filtered_areas[i] = childcount + 1;
		filtered_areas[childcount++] = (unsigned short int)i;
	}

#ifdef CHECK_TRAVEL_TIMES
	// allocate and calculate the travel times
	filteredroutetable = (aas_rt_route_t **) AAS_RT_GetClearedMemory( childcount * sizeof( aas_rt_route_t * ) );
	for ( i = 0; i < childcount; i++ )
		filteredroutetable[i] = (aas_rt_route_t *) AAS_RT_GetClearedMemory( childcount * sizeof( aas_rt_route_t ) );

	AAS_RT_CalculateRouteTable( filteredroutetable );

#endif  // CHECK_TRAVEL_TIMES

	// allocate for the temporary build local data
	area_localinfos = (aas_area_buildlocalinfo_t **) AAS_RT_GetClearedMemory( childcount * sizeof( aas_area_buildlocalinfo_t * ) );

	for ( i = 0; i < childcount; i++ )
	{
		srcarea = &( *aasworld ).areas[filtered_areas[i]];
		srcsettings = &( *aasworld ).areasettings[filtered_areas[i]];

		// allocate memory for this area
		area_localinfos[i] = (aas_area_buildlocalinfo_t *) AAS_RT_GetClearedMemory( sizeof( aas_area_buildlocalinfo_t ) );
		localinfo = area_localinfos[i];

		for ( j = 0; j < childcount; j++ )
		{
			if ( i == j ) {
				continue;
			}

#ifdef CHECK_TRAVEL_TIMES

			// make sure travel time is reasonable
			// Get the travel time from i to j
			traveltime = (int)filteredroutetable[i][j].travel_time;

			if ( !traveltime ) {
				noroutecount++;
				continue;
			}
			if ( traveltime > MAX_LOCALTRAVELTIME ) {
				continue;
			}

#endif  // CHECK_TRAVEL_TIMES

			// Add it to the list
			localinfo->visible[localinfo->numvisible++] = j;
			totalcount++;

			if ( localinfo->numvisible >= MAX_VISIBLE_AREAS ) {
				botimport.Print( PRT_MESSAGE, "MAX_VISIBLE_AREAS exceeded, lower MAX_VISIBLE_RANGE\n" );
				break;
			}
		}
	}

	// now calculate the best list of locale's

	// allocate for the long-term child data
	area_childlocaldata = (aas_area_childlocaldata_t **) AAS_RT_GetClearedMemory( childcount * sizeof( aas_area_childlocaldata_t * ) );

	for ( i = 0; i < childcount; i++ )
	{
		area_childlocaldata[i] = (aas_area_childlocaldata_t *) AAS_RT_GetClearedMemory( sizeof( aas_area_childlocaldata_t ) );
		area_childlocaldata[i]->areanum = filtered_areas[i];
	}

	while ( 1 )
	{
		bestchild = -1;
		bestcount = 99999;

		// find the area with the least number of visible areas
		for ( i = 0; i < childcount; i++ )
		{
			if ( area_childlocaldata[i]->parentlink ) {
				continue;   // already has been allocated to a parent

			}
			cnt = AAS_RT_GetValidVisibleAreasCount( area_localinfos[i], area_childlocaldata );

			if ( cnt < bestcount ) {
				bestcount = area_localinfos[i]->numvisible;
				bestchild = i;
			}
		}

		if ( bestchild < 0 ) {
			break;      // our job is done


		}
		localinfo = area_localinfos[bestchild];


		// look through this area's list of visible areas, and pick the one with the most VALID visible areas
		bestparent = bestchild;

		for ( i = 0; i < localinfo->numvisible; i++ )
		{
			if ( area_childlocaldata[localinfo->visible[i]]->parentlink ) {
				continue;   // already has been allocated to a parent

			}
			// calculate how many of children are valid
			cnt = AAS_RT_GetValidVisibleAreasCount( area_localinfos[localinfo->visible[i]], area_childlocaldata );

			if ( cnt > bestcount ) {
				bestcount = cnt;
				bestparent = localinfo->visible[i];
			}
		}

		// now setup this parent, and assign all it's children
		localinfo = area_localinfos[bestparent];

		// we use all children now, not just valid ones
		bestcount = localinfo->numvisible;

		area_parents[num_parents] = (aas_area_parent_t *) AAS_RT_GetClearedMemory( sizeof( aas_area_parent_t ) );
		thisparent = area_parents[num_parents];

		thisparent->areanum = filtered_areas[bestparent];
		thisparent->children = (unsigned short int *) AAS_RT_GetClearedMemory( ( localinfo->numvisible + 1 ) * sizeof( unsigned short int ) );

		// first, add itself to the list (yes, a parent is a child of itself)
		child = area_childlocaldata[bestparent];
		AAS_RT_AddParentLink( child, num_parents, thisparent->numchildren );
		thisparent->children[thisparent->numchildren++] = filtered_areas[bestparent];

		// loop around all the parent's visible list, and make them children if they're aren't already assigned to a parent
		for ( i = 0; i < localinfo->numvisible; i++ )
		{
			// create the childlocaldata
			child = area_childlocaldata[localinfo->visible[i]];

			// Ridah, only one parent per child in the new system
			if ( child->parentlink ) {
				continue;   // already has been allocated to a parent

			}
			if ( child->areanum != thisparent->areanum ) {
				AAS_RT_AddParentLink( child, num_parents, thisparent->numchildren );
				thisparent->children[thisparent->numchildren++] = filtered_areas[localinfo->visible[i]];
			}
		}

		// now setup the list of children and the route-tables
		for ( i = 0; i < thisparent->numchildren; i++ )
		{
			child = area_childlocaldata[-1 + rev_filtered_areas[thisparent->children[i]]];
			localinfo = area_localinfos[-1 + rev_filtered_areas[thisparent->children[i]]];

			child->parentlink->routeindexes = (unsigned short int *) AAS_RT_GetClearedMemory( thisparent->numchildren * sizeof( unsigned short int ) );

			// now setup the indexes
			for ( j = 0; j < thisparent->numchildren; j++ )
			{
				// find this child in our list of visibles
				if ( j == child->parentlink->childindex ) {
					continue;
				}

				for ( k = 0; k < localinfo->numvisible; k++ )
				{
					if ( thisparent->children[j] == filtered_areas[localinfo->visible[k]] ) { // found a match
						child->parentlink->routeindexes[j] = (unsigned short int)k;
						break;
					}
				}

				if ( k == localinfo->numvisible ) { // didn't find it, so add it to our list
					if ( localinfo->numvisible >= MAX_VISIBLE_AREAS ) {
						botimport.Print( PRT_MESSAGE, "MAX_VISIBLE_AREAS exceeded, lower MAX_VISIBLE_RANGE\n" );
					} else
					{
						localinfo->visible[localinfo->numvisible] = -1 + rev_filtered_areas[thisparent->children[j]];
						child->parentlink->routeindexes[j] = (unsigned short int)localinfo->numvisible;
						localinfo->numvisible++;
					}
				}
			}
		}

		num_parents++;
	}

	// place all the visible areas from each child, into their childlocaldata route-table
	for ( i = 0; i < childcount; i++ )
	{
		localinfo = area_localinfos[i];
		child = area_childlocaldata[i];

		child->numlocal = localinfo->numvisible;
		child->localroutes = (aas_rt_route_t *) AAS_RT_GetClearedMemory( localinfo->numvisible * sizeof( aas_rt_route_t ) );

		for ( j = 0; j < localinfo->numvisible; j++ )
		{
			child->localroutes[j] = filteredroutetable[i][localinfo->visible[j]];
		}

		child->parentroutes = (aas_rt_route_t *) AAS_RT_GetClearedMemory( num_parents * sizeof( aas_rt_route_t ) );

		for ( j = 0; j < num_parents; j++ )
		{
			child->parentroutes[j] = filteredroutetable[i][-1 + rev_filtered_areas[area_parents[j]->areanum]];
		}
	}

	// build the visibleParents lists
	visibleParents = (unsigned short int *) AAS_RT_GetClearedMemory( num_parents * sizeof( unsigned short int ) );
	for ( i = 0; i < num_parents; i++ )
	{
		area_parents[i]->numVisibleParents = 0;

		for ( j = 0; j < num_parents; j++ )
		{
			if ( i == j ) {
				continue;
			}

			if ( !AAS_inPVS( ( *aasworld ).areas[area_parents[i]->areanum].center, ( *aasworld ).areas[area_parents[j]->areanum].center ) ) {
				continue;
			}

			visibleParents[area_parents[i]->numVisibleParents] = j;
			area_parents[i]->numVisibleParents++;
		}

		// now copy the list over to the current src area
		area_parents[i]->visibleParents = (unsigned short int *) AAS_RT_GetClearedMemory( area_parents[i]->numVisibleParents * sizeof( unsigned short int ) );
		memcpy( area_parents[i]->visibleParents, visibleParents, area_parents[i]->numVisibleParents * sizeof( unsigned short int ) );

	}
	AAS_RT_FreeMemory( visibleParents );

	// before we free the main childlocaldata, go through and assign the aas_area's to their appropriate childlocaldata
	//	this would require modification of the aas_area_t structure, so for now, we'll just place them in a global array, for external reference

//	aasworld->routetable->area_childlocaldata_list = (aas_area_childlocaldata_t **) AAS_RT_GetClearedMemory( (*aasworld).numareas * sizeof(aas_area_childlocaldata_t *) );
//	for (i=0; i<childcount; i++)
//	{
//		aasworld->routetable->area_childlocaldata_list[filtered_areas[i]] = area_childlocaldata[i];
//	}

	// copy the list of parents to a global structure for now (should eventually go into the (*aasworld) structure
//	aasworld->routetable->area_parents_global = (aas_area_parent_t **) AAS_RT_GetClearedMemory( num_parents * sizeof(aas_area_parent_t *) );
//	memcpy( aasworld->routetable->area_parents_global, area_parents, num_parents * sizeof(aas_area_parent_t *) );

	// ................................................
	// Convert the data into the correct format
	{
		aas_rt_t    *rt;
		aas_rt_child_t  *child;
		aas_rt_parent_t *parent;
		aas_rt_parent_link_t *plink;
		unsigned short int  *psi;

		aas_area_childlocaldata_t   *chloc;
		aas_area_parent_t           *apar;
		aas_parent_link_t           *oplink;

		int localRoutesCount, parentRoutesCount, parentChildrenCount, visibleParentsCount, parentLinkCount, routeIndexesCount;

		rt = ( *aasworld ).routetable;
		localRoutesCount = 0;
		parentRoutesCount = 0;
		parentChildrenCount = 0;
		visibleParentsCount = 0;
		parentLinkCount = 0;
		routeIndexesCount = 0;

		// areaChildIndexes
		rt->areaChildIndexes = (unsigned short int *) AAS_RT_GetClearedMemory( ( *aasworld ).numareas * sizeof( unsigned short int ) );
		for ( i = 0; i < childcount; i++ )
		{
			rt->areaChildIndexes[filtered_areas[i]] = i + 1;
		}

		// children
		rt->numChildren = childcount;
		rt->children = (aas_rt_child_t *) AAS_RT_GetClearedMemory( rt->numChildren * sizeof( aas_rt_child_t ) );
		child = rt->children;
		for ( i = 0; i < childcount; i++, child++ )
		{
			chloc = area_childlocaldata[i];

			child->areanum = chloc->areanum;
			child->numParentLinks = AAS_RT_NumParentLinks( chloc );

			child->startParentLinks = parentLinkCount;

			parentLinkCount += child->numParentLinks;
		}

		// parents
		rt->numParents = num_parents;
		rt->parents = (aas_rt_parent_t *) AAS_RT_GetClearedMemory( rt->numParents * sizeof( aas_rt_parent_t ) );
		parent = rt->parents;
		for ( i = 0; i < num_parents; i++, parent++ )
		{
			apar = area_parents[i];

			parent->areanum = apar->areanum;
			parent->numParentChildren = apar->numchildren;
			parent->numVisibleParents = apar->numVisibleParents;

			parent->startParentChildren = parentChildrenCount;
			parent->startVisibleParents = visibleParentsCount;

			parentChildrenCount += parent->numParentChildren;
			visibleParentsCount += parent->numVisibleParents;
		}

		// parentChildren
		rt->numParentChildren = parentChildrenCount;
		rt->parentChildren = (unsigned short int *) AAS_RT_GetClearedMemory( parentChildrenCount * sizeof( unsigned short int ) );
		psi = rt->parentChildren;
		for ( i = 0; i < num_parents; i++ )
		{
			apar = area_parents[i];
			for ( j = 0; j < apar->numchildren; j++, psi++ )
			{
				*psi = apar->children[j];
			}
		}

		// visibleParents
		rt->numVisibleParents = visibleParentsCount;
		rt->visibleParents = (unsigned short int *) AAS_RT_GetClearedMemory( rt->numVisibleParents * sizeof( unsigned short int ) );
		psi = rt->visibleParents;
		for ( i = 0; i < num_parents; i++ )
		{
			apar = area_parents[i];
			for ( j = 0; j < apar->numVisibleParents; j++, psi++ )
			{
				*psi = apar->visibleParents[j];
			}
		}

		// parentLinks
		rt->numParentLinks = parentLinkCount;
		rt->parentLinks = (aas_rt_parent_link_t *) AAS_RT_GetClearedMemory( parentLinkCount * sizeof( aas_rt_parent_link_t ) );
		plink = rt->parentLinks;
		for ( i = 0; i < childcount; i++ )
		{
			chloc = area_childlocaldata[i];
			for ( oplink = chloc->parentlink; oplink; plink++, oplink = oplink->next )
			{
				plink->childIndex = oplink->childindex;
				plink->parent = oplink->parent;
			}
		}

	}
	// ................................................

	// write the newly created table
	AAS_RT_WriteRouteTable();


	botimport.Print( PRT_MESSAGE, "Child Areas: %i\nTotal Parents: %i\nAverage VisAreas: %i\n", (int)childcount, num_parents, (int)( childcount / num_parents ) );
	botimport.Print( PRT_MESSAGE, "NoRoute Ratio: %i%%\n", (int)( ( 100.0 * noroutecount ) / ( 1.0 * childcount * childcount ) ) );

	memoryend = memorycount;

	// clear allocated memory

// causes crashes in route-caching
//#ifdef USE_ROUTECACHE
//	AAS_FreeRoutingCaches();
//#endif

	for ( i = 0; i < childcount; i++ )
	{
		AAS_RT_FreeMemory( area_localinfos[i] );
#ifdef CHECK_TRAVEL_TIMES
		AAS_RT_FreeMemory( filteredroutetable[i] );
#endif
	}

	{
		aas_parent_link_t *next, *trav;

		// kill the client areas
		for ( i = 0; i < childcount; i++ )
		{
			// kill the parent links
			next = area_childlocaldata[i]->parentlink;
			// TTimo: suggest () around assignment used as truth value
			while ( ( trav = next ) )
			{
				next = next->next;

				AAS_RT_FreeMemory( trav->routeindexes );
				AAS_RT_FreeMemory( trav );
			}

			AAS_RT_FreeMemory( area_childlocaldata[i]->localroutes );
			AAS_RT_FreeMemory( area_childlocaldata[i]->parentroutes );
			AAS_RT_FreeMemory( area_childlocaldata[i] );
		}

		// kill the parents
		for ( i = 0; i < num_parents; i++ )
		{
			AAS_RT_FreeMemory( area_parents[i]->children );
			AAS_RT_FreeMemory( area_parents[i]->visibleParents );
			AAS_RT_FreeMemory( area_parents[i] );
		}
	}

	AAS_RT_FreeMemory( area_localinfos );
	AAS_RT_FreeMemory( area_childlocaldata );
	AAS_RT_FreeMemory( filtered_areas );
	AAS_RT_FreeMemory( rev_filtered_areas );
#ifdef CHECK_TRAVEL_TIMES
	AAS_RT_FreeMemory( filteredroutetable );
#endif

	// check how much memory we've used, and intend to keep
	AAS_RT_PrintMemoryUsage();

	botimport.Print( PRT_MESSAGE, "Route-Table Permanent Memory Usage: %i\n", memorycount );
	botimport.Print( PRT_MESSAGE, "Route-Table Calculation Usage: %i\n", memoryend + cachememory );
	botimport.Print( PRT_MESSAGE, "---------------------------------\n" );
}

//===========================================================================
//	free permanent memory used by route-table system
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_RT_ShutdownRouteTable( void ) {
	if ( !aasworld->routetable ) {
		return;
	}

	// free the dynamic lists
	AAS_RT_FreeMemory( aasworld->routetable->areaChildIndexes );
	AAS_RT_FreeMemory( aasworld->routetable->children );
	AAS_RT_FreeMemory( aasworld->routetable->parents );
	AAS_RT_FreeMemory( aasworld->routetable->parentChildren );
	AAS_RT_FreeMemory( aasworld->routetable->visibleParents );
//	AAS_RT_FreeMemory( aasworld->routetable->localRoutes );
//	AAS_RT_FreeMemory( aasworld->routetable->parentRoutes );
	AAS_RT_FreeMemory( aasworld->routetable->parentLinks );
//	AAS_RT_FreeMemory( aasworld->routetable->routeIndexes );
//	AAS_RT_FreeMemory( aasworld->routetable->parentTravelTimes );

	// kill the table
	AAS_RT_FreeMemory( aasworld->routetable );
	aasworld->routetable = NULL;
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_rt_parent_link_t *AAS_RT_GetFirstParentLink( aas_rt_child_t *child ) {
	return &aasworld->routetable->parentLinks[child->startParentLinks];
}

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
aas_rt_child_t *AAS_RT_GetChild( int areanum ) {
	int i;

	i = (int)aasworld->routetable->areaChildIndexes[areanum] - 1;

	if ( i >= 0 ) {
		return &aasworld->routetable->children[i];
	} else {
		return NULL;
	}
}

//===========================================================================
//	returns a route between the areas
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_AreaRouteToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags, int *traveltime, int *reachnum );
aas_rt_route_t *AAS_RT_GetRoute( int srcnum, vec3_t origin, int destnum ) {
	#define GETROUTE_NUMROUTES  64
	static aas_rt_route_t routes[GETROUTE_NUMROUTES];   // cycle through these, so we don't overlap
	static int routeIndex = 0;
	aas_rt_route_t  *thisroute;
	int reach, traveltime;
	aas_rt_t *rt;
	static int tfl = TFL_DEFAULT & ~( TFL_JUMPPAD | TFL_ROCKETJUMP | TFL_BFGJUMP | TFL_GRAPPLEHOOK | TFL_DOUBLEJUMP | TFL_RAMPJUMP | TFL_STRAFEJUMP | TFL_LAVA );   //----(SA)	modified since slime is no longer deadly
//	static int tfl = TFL_DEFAULT & ~(TFL_JUMPPAD|TFL_ROCKETJUMP|TFL_BFGJUMP|TFL_GRAPPLEHOOK|TFL_DOUBLEJUMP|TFL_RAMPJUMP|TFL_STRAFEJUMP|TFL_SLIME|TFL_LAVA);

	if ( !( rt = aasworld->routetable ) ) { // no route table present
		return NULL;
	}

	if ( disable_routetable ) {
		return NULL;
	}

	if ( ++routeIndex >= GETROUTE_NUMROUTES ) {
		routeIndex = 0;
	}

	thisroute = &routes[routeIndex];

	if ( AAS_AreaRouteToGoalArea( srcnum, origin, destnum, tfl, &traveltime, &reach ) ) {
		thisroute->reachable_index = reach;
		thisroute->travel_time = traveltime;
		return thisroute;
	} else {
		return NULL;
	}
}

//===========================================================================
//	draws the route-table from src to dest
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#include "../game/be_ai_goal.h"
int BotGetReachabilityToGoal( vec3_t origin, int areanum, int entnum,
							  int lastgoalareanum, int lastareanum,
							  int *avoidreach, float *avoidreachtimes, int *avoidreachtries,
							  bot_goal_t *goal, int travelflags, int movetravelflags );

void AAS_RT_ShowRoute( vec3_t srcpos, int srcnum, int destnum ) {
#ifdef DEBUG
#define MAX_RT_AVOID_REACH 1
	AAS_ClearShownPolygons();
	AAS_ClearShownDebugLines();
	AAS_ShowAreaPolygons( srcnum, 1, qtrue );
	AAS_ShowAreaPolygons( destnum, 4, qtrue );
	{
		static int lastgoalareanum, lastareanum;
		static int avoidreach[MAX_RT_AVOID_REACH];
		static float avoidreachtimes[MAX_RT_AVOID_REACH];
		static int avoidreachtries[MAX_RT_AVOID_REACH];
		int reachnum;
		bot_goal_t goal;
		aas_reachability_t reach;

		goal.areanum = destnum;
		VectorCopy( botlibglobals.goalorigin, goal.origin );
		reachnum = BotGetReachabilityToGoal( srcpos, srcnum, -1,
											 lastgoalareanum, lastareanum,
											 avoidreach, avoidreachtimes, avoidreachtries,
											 &goal, TFL_DEFAULT | TFL_FUNCBOB, TFL_DEFAULT | TFL_FUNCBOB );
		AAS_ReachabilityFromNum( reachnum, &reach );
		AAS_ShowReachability( &reach );
	}
#endif
}

/*
=================
AAS_RT_GetHidePos

  "src" is hiding ent, "dest" is the enemy
=================
*/
int AAS_NearestHideArea( int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags );
qboolean AAS_RT_GetHidePos( vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos ) {
	static int tfl = TFL_DEFAULT & ~( TFL_JUMPPAD | TFL_ROCKETJUMP | TFL_BFGJUMP | TFL_GRAPPLEHOOK | TFL_DOUBLEJUMP | TFL_RAMPJUMP | TFL_STRAFEJUMP | TFL_LAVA );   //----(SA)	modified since slime is no longer deadly
//	static int tfl = TFL_DEFAULT & ~(TFL_JUMPPAD|TFL_ROCKETJUMP|TFL_BFGJUMP|TFL_GRAPPLEHOOK|TFL_DOUBLEJUMP|TFL_RAMPJUMP|TFL_STRAFEJUMP|TFL_SLIME|TFL_LAVA);

#if 1
	// use MrE's breadth first method
	int hideareanum;
//	int pretime;

	// disabled this so grenade hiding works
	//if (!srcarea || !destarea)
	//	return qfalse;

//	pretime = -Sys_MilliSeconds();

	hideareanum = AAS_NearestHideArea( srcnum, srcpos, srcarea, destnum, destpos, destarea, tfl );
	if ( !hideareanum ) {
//		botimport.Print(PRT_MESSAGE, "Breadth First HidePos FAILED: %i ms\n", pretime + Sys_MilliSeconds());
		return qfalse;
	}
	// we found a valid hiding area
	VectorCopy( ( *aasworld ).areawaypoints[hideareanum], returnPos );

//	botimport.Print(PRT_MESSAGE, "Breadth First HidePos: %i ms\n", pretime + Sys_MilliSeconds());

	return qtrue;

#else
	// look around at random parent areas, if any of them have a center point
	// that isn't visible from "destpos", then return it's position in returnPos

	int i, j, pathArea, dir;
	unsigned short int destTravelTime;
	aas_rt_parent_t *srcParent, *travParent, *destParent;
	aas_rt_child_t  *srcChild, *destChild, *travChild;
	aas_rt_route_t  *route;
	vec3_t destVec;
	float destTravelDist;
	static float lastTime;
	static int frameCount, maxPerFrame = 2;
	int firstreach;
	aas_reachability_t  *reachability, *reach;
	qboolean startVisible;
	unsigned short int bestTravelTime, thisTravelTime, elapsedTravelTime;
	#define MAX_HIDE_TRAVELTIME     1000    // 10 seconds is a very long way away
	unsigned char destVisLookup[MAX_PARENTS];
	unsigned short int *destVisTrav;

	aas_rt_t    *rt;

	const int MAX_CHECK_VISPARENTS    = 100;
	int visparents_count, total_parents_checked;
	int thisParentIndex;
	int pretime;

	if ( !( rt = aasworld->routetable ) ) { // no route table present
		return qfalse;
	}
/*
	if (lastTime > (AAS_Time() - 0.1)) {
		if (frameCount++ > maxPerFrame) {
			return qfalse;
		}
	} else {
		frameCount = 0;
		lastTime = AAS_Time();
	}
*/
	pretime = -Sys_MilliSeconds();

	// is the src area grounded?
	if ( !( srcChild = AAS_RT_GetChild( srcarea ) ) ) {
		return qfalse;
	}
	// does it have a parent?
// all valid areas have a parent
//	if (!srcChild->numParentLinks) {
//		return qfalse;
//	}
	// get the dest (enemy) area
	if ( !( destChild = AAS_RT_GetChild( destarea ) ) ) {
		return qfalse;
	}
	destParent = &rt->parents[ rt->parentLinks[destChild->startParentLinks].parent ];
	//
	// populate the destVisAreas
	memset( destVisLookup, 0, sizeof( destVisLookup ) );
	destVisTrav = rt->visibleParents + destParent->startVisibleParents;
	for ( i = 0; i < destParent->numVisibleParents; i++, destVisTrav++ ) {
		destVisLookup[*destVisTrav] = 1;
	}
	//
	// use the first parent to source the vis areas from
	srcParent = &rt->parents[ rt->parentLinks[srcChild->startParentLinks].parent ];
	//
	// set the destTravelTime
	if ( route = AAS_RT_GetRoute( srcarea, srcpos, destarea ) ) {
		destTravelTime = route->travel_time;
	} else {
		destTravelTime = 0;
	}
	bestTravelTime = MAX_HIDE_TRAVELTIME;   // ignore any routes longer than 10 seconds away
	// set the destVec
	VectorSubtract( destpos, srcpos, destVec );
	destTravelDist = VectorNormalize( destVec );
	//
	// randomize the direction we traverse the list, so the hiding spot isn't always the same
	if ( rand() % 2 ) {
		dir = 1;    // forward
	} else {
		dir = -1;   // reverse
	}
	// randomize the starting area
	if ( srcParent->numVisibleParents ) {
		i = rand() % srcParent->numVisibleParents;
	} else {    // prevent divide by zero
		i = 0;
	}
	//
	// setup misc stuff
	reachability = ( *aasworld ).reachability;
	startVisible = botimport.AICast_VisibleFromPos( destpos, destnum, srcpos, srcnum, qfalse );
	//
	// set the firstreach to prevent having to do an array and pointer lookup for each destination
	firstreach = ( *aasworld ).areasettings[srcarea].firstreachablearea;
	//
	// just check random parent areas, traversing the route until we find an area that can't be seen from the dest area
	for ( visparents_count = 0, total_parents_checked = 0; visparents_count < MAX_CHECK_VISPARENTS && total_parents_checked < rt->numParents; total_parents_checked++ ) {
		thisParentIndex = rand() % rt->numParents;
		travParent = &rt->parents[ thisParentIndex ];
		//
		// never go to the enemy's areas
		if ( travParent->areanum == destarea ) {
			continue;
		}
		//
		// if it's visible from dest, ignore it
		if ( destVisLookup[thisParentIndex] ) {
			continue;
		}
		//
		visparents_count++;
		// they might be visible, check to see if the path to the area, takes us towards the
		// enemy we are trying to hide from
		{
			qboolean invalidRoute;
			vec3_t curPos, lastVec;
			#define     GETHIDE_MAX_CHECK_PATHS     15
			//
			invalidRoute = qfalse;
			// initialize the pathArea
			pathArea = srcarea;
			VectorCopy( srcpos, curPos );
			// now evaluate the path
			for ( j = 0; j < GETHIDE_MAX_CHECK_PATHS; j++ ) {
				// get the reachability to the travParent
				if ( !( route = AAS_RT_GetRoute( pathArea, curPos, travParent->areanum ) ) ) {
					// we can't get to the travParent, so don't bother checking it
					invalidRoute = qtrue;
					break;
				}
				// set the pathArea
				reach = &reachability[route->reachable_index];
				// how far have we travelled so far?
				elapsedTravelTime = AAS_AreaTravelTimeToGoalArea( pathArea, curPos, reach->areanum, tfl );
				// add the travel to the center of the area
				elapsedTravelTime += AAS_AreaTravelTime( reach->areanum, reach->end, ( *aasworld ).areas[reach->areanum].center );
				// have we gone too far already?
				if ( elapsedTravelTime > bestTravelTime ) {
					invalidRoute = qtrue;
					break;
				} else {
					thisTravelTime = route->travel_time;
				}
				//
				// if this travel would have us do something wierd
				if ( ( reach->traveltype == TRAVEL_WALKOFFLEDGE ) && ( reach->traveltime > 500 ) ) {
					invalidRoute = qtrue;
					break;
				}
				//
				pathArea = reach->areanum;
				VectorCopy( reach->end, curPos );
				//
				// if this moves us into the enemies area, skip it
				if ( pathArea == destarea ) {
					invalidRoute = qtrue;
					break;
				}
				// if we are very close, don't get any closer under any circumstances
				{
					vec3_t vec;
					float dist;
					//
					VectorSubtract( destpos, reachability[firstreach + route->reachable_index].end, vec );
					dist = VectorNormalize( vec );
					//
					if ( destTravelTime < 400 ) {
						if ( dist < destTravelDist ) {
							invalidRoute = qtrue;
							break;
						}
						if ( DotProduct( destVec, vec ) < 0.2 ) {
							invalidRoute = qtrue;
							break;
						}
					} else {
						if ( dist < destTravelDist * 0.7 ) {
							invalidRoute = qtrue;
							break;
						}
					}
					//
					// check the directions to make sure we're not trying to run through them
					if ( j > 0 ) {
						if ( DotProduct( vec, lastVec ) < 0.2 ) {
							invalidRoute = qtrue;
							break;
						}
					} else if ( DotProduct( destVec, vec ) < 0.2 ) {
						invalidRoute = qtrue;
						break;
					}
					//
					VectorCopy( vec, lastVec );
				}
				//
				// if this area isn't in the visible list for the enemy's area, it's a good hiding spot
				if ( !( travChild = AAS_RT_GetChild( pathArea ) ) ) {
					invalidRoute = qtrue;
					break;
				}
				if ( !destVisLookup[rt->parentLinks[travChild->startParentLinks].parent] ) {
					// success ?
					if ( !botimport.AICast_VisibleFromPos( destpos, destnum, ( *aasworld ).areas[pathArea].center, srcnum, qfalse ) ) {
						// SUCESS !!
						travParent = &rt->parents[rt->parentLinks[travChild->startParentLinks].parent];
						break;
					}
				} else {
					// if we weren't visible when starting, make sure we don't move into their view
					if ( !startVisible ) { //botimport.AICast_VisibleFromPos( destpos, destnum, reachability[firstreach + route->reachable_index].end, srcnum, qfalse )) {
						invalidRoute = qtrue;
						break;
					}
				}
				//
				// if this is the travParent, then stop checking
				if ( pathArea == travParent->areanum ) {
					invalidRoute = qtrue;   // we didn't find a hiding spot
					break;
				}
			}   // end for areas in route
			//
			// if the route is invalid, skip this travParent
			if ( invalidRoute ) {
				continue;
			}
		}
		//
		// now last of all, check that this area is a safe hiding spot
//		if (botimport.AICast_VisibleFromPos( destpos, destnum, (*aasworld).areas[travParent->areanum].center, srcnum, qfalse )) {
//			continue;
//		}
		//
		// we've found a good hiding spot, so use it
		VectorCopy( ( *aasworld ).areas[travParent->areanum].center, returnPos );
		bestTravelTime = elapsedTravelTime;
		//
		if ( thisTravelTime < 300 ) {
			botimport.Print( PRT_MESSAGE, "Fuzzy RT HidePos: %i ms\n", pretime + Sys_MilliSeconds() );
			return qtrue;
		}
	}
	//
	// did we find something?
	if ( bestTravelTime < MAX_HIDE_TRAVELTIME ) {
		botimport.Print( PRT_MESSAGE, "Fuzzy RT HidePos: %i ms\n", pretime + Sys_MilliSeconds() );
		return qtrue;
	}
	//
	// couldn't find anything
	botimport.Print( PRT_MESSAGE, "Fuzzy RT HidePos FAILED: %i ms\n", pretime + Sys_MilliSeconds() );
	return qfalse;
#endif
}

/*
=================
AAS_RT_GetReachabilityIndex
=================
*/
int AAS_RT_GetReachabilityIndex( int areanum, int reachIndex ) {
//	return (*aasworld).areasettings[areanum].firstreachablearea + reachIndex;
	return reachIndex;
}
