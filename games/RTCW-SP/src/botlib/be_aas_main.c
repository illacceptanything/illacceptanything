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
 * name:		be_aas_main.c
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_libvar.h"
#include "l_utils.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_log.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

aas_t aasworlds[MAX_AAS_WORLDS];

aas_t *aasworld;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void QDECL AAS_Error( char *fmt, ... ) {
	char str[1024];
	va_list arglist;

	va_start( arglist, fmt );
	vsprintf( str, fmt, arglist );
	va_end( arglist );
	botimport.Print( PRT_FATAL, str );
} //end of the function AAS_Error

// Ridah, multiple AAS worlds
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SetCurrentWorld( int index ) {
	if ( index >= MAX_AAS_WORLDS || index < 0 ) {
		AAS_Error( "AAS_SetCurrentWorld: index out of range\n" );
		return;
	}

	// set the current world pointer
	aasworld = &aasworlds[index];
}
// done.

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
char *AAS_StringFromIndex( char *indexname, char *stringindex[], int numindexes, int index ) {
	if ( !( *aasworld ).indexessetup ) {
		botimport.Print( PRT_ERROR, "%s: index %d not setup\n", indexname, index );
		return "";
	} //end if
	if ( index < 0 || index >= numindexes ) {
		botimport.Print( PRT_ERROR, "%s: index %d out of range\n", indexname, index );
		return "";
	} //end if
	if ( !stringindex[index] ) {
		if ( index ) {
			botimport.Print( PRT_ERROR, "%s: reference to unused index %d\n", indexname, index );
		} //end if
		return "";
	} //end if
	return stringindex[index];
} //end of the function AAS_StringFromIndex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_IndexFromString( char *indexname, char *stringindex[], int numindexes, char *string ) {
	int i;
	if ( !( *aasworld ).indexessetup ) {
		botimport.Print( PRT_ERROR, "%s: index not setup \"%s\"\n", indexname, string );
		return 0;
	} //end if
	for ( i = 0; i < numindexes; i++ )
	{
		if ( !stringindex[i] ) {
			continue;
		}
		if ( !Q_stricmp( stringindex[i], string ) ) {
			return i;
		}
	} //end for
	return 0;
} //end of the function AAS_IndexFromString
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
char *AAS_ModelFromIndex( int index ) {
//	return AAS_StringFromIndex("ModelFromIndex", &(*aasworld).configstrings[CS_MODELS], MAX_MODELS, index);
	return 0;   // removed so the CS_ defines could be removed from be_aas_def.h
} //end of the function AAS_ModelFromIndex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_IndexFromModel( char *modelname ) {
//	return AAS_IndexFromString("IndexFromModel", &(*aasworld).configstrings[CS_MODELS], MAX_MODELS, modelname);
	return 0;   // removed so the CS_ defines could be removed from be_aas_def.h
} //end of the function AAS_IndexFromModel
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_UpdateStringIndexes( int numconfigstrings, char *configstrings[] ) {
	int i;
	//set string pointers and copy the strings
	for ( i = 0; i < numconfigstrings; i++ )
	{
		if ( configstrings[i] ) {
			//if ((*aasworld).configstrings[i]) FreeMemory((*aasworld).configstrings[i]);
			( *aasworld ).configstrings[i] = (char *) GetMemory( strlen( configstrings[i] ) + 1 );
			strcpy( ( *aasworld ).configstrings[i], configstrings[i] );
		} //end if
	} //end for
	( *aasworld ).indexessetup = qtrue;
} //end of the function AAS_UpdateStringIndexes
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Loaded( void ) {
	return ( *aasworld ).loaded;
} //end of the function AAS_Loaded
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Initialized( void ) {
	return ( *aasworld ).initialized;
} //end of the function AAS_Initialized
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_SetInitialized( void ) {
	( *aasworld ).initialized = qtrue;
	botimport.Print( PRT_MESSAGE, "AAS initialized.\n" );
#ifdef DEBUG
	//create all the routing cache
	//AAS_CreateAllRoutingCache();
	//
	//AAS_RoutingInfo();
#endif

	// Ridah, build/load the route-table
	AAS_RT_BuildRouteTable();
	// done.

} //end of the function AAS_SetInitialized
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ContinueInit( float time ) {
	//if no AAS file loaded
	if ( !( *aasworld ).loaded ) {
		return;
	}
	//if AAS is already initialized
	if ( ( *aasworld ).initialized ) {
		return;
	}
	//calculate reachability, if not finished return
	if ( AAS_ContinueInitReachability( time ) ) {
		return;
	}
	//initialize clustering for the new map
	AAS_InitClustering();
	//if reachability has been calculated and an AAS file should be written
	//or there is a forced data optimization
	if ( ( *aasworld ).savefile || ( (int)LibVarGetValue( "forcewrite" ) ) ) {
		//optimize the AAS data
		if ( !( (int)LibVarValue( "nooptimize", "1" ) ) ) {
			AAS_Optimize();
		}
		//save the AAS file
		if ( AAS_WriteAASFile( ( *aasworld ).filename ) ) {
			botimport.Print( PRT_MESSAGE, "%s written succesfully\n", ( *aasworld ).filename );
		} //end if
		else
		{
			botimport.Print( PRT_ERROR, "couldn't write %s\n", ( *aasworld ).filename );
		} //end else
	} //end if
	  //initialize the routing
	AAS_InitRouting();
	//at this point AAS is initialized
	AAS_SetInitialized();
} //end of the function AAS_ContinueInit
//===========================================================================
// called at the start of every frame
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_StartFrame( float time ) {
	// Ridah, do each of the aasworlds
	int i;

	for ( i = 0; i < MAX_AAS_WORLDS; i++ )
	{
		AAS_SetCurrentWorld( i );

		( *aasworld ).time = time;
		//invalidate the entities
		AAS_InvalidateEntities();
		//initialize AAS
		AAS_ContinueInit( time );
		//
		( *aasworld ).frameroutingupdates = 0;
		//
		/* Ridah, disabled for speed
		if (LibVarGetValue("showcacheupdates"))
		{
			AAS_RoutingInfo();
			LibVarSet("showcacheupdates", "0");
		} //end if
		if (LibVarGetValue("showmemoryusage"))
		{
			PrintUsedMemorySize();
			LibVarSet("showmemoryusage", "0");
		} //end if
		if (LibVarGetValue("memorydump"))
		{
			PrintMemoryLabels();
			LibVarSet("memorydump", "0");
		} //end if
		*/
	} //end if
	( *aasworld ).numframes++;
	return BLERR_NOERROR;
} //end of the function AAS_StartFrame
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
float AAS_Time( void ) {
	return ( *aasworld ).time;
} //end of the function AAS_Time
//===========================================================================
// basedir	= Quake2 console basedir
// gamedir	= Quake2 console gamedir
//	mapname	= name of the map without extension (.bsp)
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_LoadFiles( const char *mapname ) {
	int errnum;
	char aasfile[MAX_PATH];
//	char bspfile[MAX_PATH];

	strcpy( ( *aasworld ).mapname, mapname );
	//NOTE: first reset the entity links into the AAS areas and BSP leaves
	// the AAS link heap and BSP link heap are reset after respectively the
	// AAS file and BSP file are loaded
	AAS_ResetEntityLinks();
	//

	// load bsp info
	AAS_LoadBSPFile();

	//load the aas file
	Com_sprintf( aasfile, MAX_PATH, "maps/%s.aas", mapname );
	errnum = AAS_LoadAASFile( aasfile );
	if ( errnum != BLERR_NOERROR ) {
		return errnum;
	}

	botimport.Print( PRT_MESSAGE, "loaded %s\n", aasfile );
	strncpy( ( *aasworld ).filename, aasfile, MAX_PATH );
	return BLERR_NOERROR;
} //end of the function AAS_LoadFiles
//===========================================================================
// called everytime a map changes
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================

// Ridah, modified this for multiple AAS files

int AAS_LoadMap( const char *mapname ) {
	int errnum;
	int i;
	char this_mapname[256], intstr[4];
	qboolean loaded = qfalse;
	int missingErrNum = 0;     // TTimo: init

	for ( i = 0; i < MAX_AAS_WORLDS; i++ )
	{
		AAS_SetCurrentWorld( i );

		strncpy( this_mapname, mapname, 256 );
		strncat( this_mapname, "_b", 256 );
		sprintf( intstr, "%i", i );
		strncat( this_mapname, intstr, 256 );

		//if no mapname is provided then the string indexes are updated
		if ( !mapname ) {
			return 0;
		} //end if
		  //
		( *aasworld ).initialized = qfalse;
		//NOTE: free the routing caches before loading a new map because
		// to free the caches the old number of areas, number of clusters
		// and number of areas in a clusters must be available
		AAS_FreeRoutingCaches();
		//load the map
		errnum = AAS_LoadFiles( this_mapname );
		if ( errnum != BLERR_NOERROR ) {
			( *aasworld ).loaded = qfalse;
			// RF, we are allowed to skip one of the files, but not both
			//return errnum;
			missingErrNum = errnum;
			continue;
		} //end if
		  //
		loaded = qtrue;
		//
		AAS_InitSettings();
		//initialize the AAS link heap for the new map
		AAS_InitAASLinkHeap();
		//initialize the AAS linked entities for the new map
		AAS_InitAASLinkedEntities();
		//initialize reachability for the new map
		AAS_InitReachability();
		//initialize the alternative routing
		AAS_InitAlternativeRouting();
	}

	if ( !loaded ) {
		return missingErrNum;
	}

	//everything went ok
	return 0;
} //end of the function AAS_LoadMap

// done.

//===========================================================================
// called when the library is first loaded
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_Setup( void ) {
	// Ridah, just use the default world for entities
	AAS_SetCurrentWorld( 0 );

	( *aasworlds ).maxclients = (int) LibVarValue( "maxclients", "128" );
	( *aasworlds ).maxentities = (int) LibVarValue( "maxentities", "1024" );
	//allocate memory for the entities
	if ( ( *aasworld ).entities ) {
		FreeMemory( ( *aasworld ).entities );
	}
	( *aasworld ).entities = (aas_entity_t *) GetClearedHunkMemory( ( *aasworld ).maxentities * sizeof( aas_entity_t ) );
	//invalidate all the entities
	AAS_InvalidateEntities();

	//force some recalculations
	//LibVarSet("forceclustering", "1");			//force clustering calculation
	//LibVarSet("forcereachability", "1");		//force reachability calculation
	( *aasworld ).numframes = 0;
	return BLERR_NOERROR;
} //end of the function AAS_Setup
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_Shutdown( void ) {
	// Ridah, do each of the worlds
	int i;

	for ( i = 0; i < MAX_AAS_WORLDS; i++ )
	{
		AAS_SetCurrentWorld( i );

		// Ridah, kill the route-table data
		AAS_RT_ShutdownRouteTable();

		AAS_ShutdownAlternativeRouting();
		AAS_DumpBSPData();
		//free routing caches
		AAS_FreeRoutingCaches();
		//free aas link heap
		AAS_FreeAASLinkHeap();
		//free aas linked entities
		AAS_FreeAASLinkedEntities();
		//free the aas data
		AAS_DumpAASData();

		if ( i == 0 ) {
			//free the entities
			if ( ( *aasworld ).entities ) {
				FreeMemory( ( *aasworld ).entities );
			}
		}

		//clear the (*aasworld) structure
		memset( &( *aasworld ), 0, sizeof( aas_t ) );
		//aas has not been initialized
		( *aasworld ).initialized = qfalse;
	}

	//NOTE: as soon as a new .bsp file is loaded the .bsp file memory is
	// freed an reallocated, so there's no need to free that memory here
	//print shutdown
	botimport.Print( PRT_MESSAGE, "AAS shutdown.\n" );
} //end of the function AAS_Shutdown
