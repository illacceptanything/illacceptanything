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
 * name:		be_aas_entity.c
 *
 * desc:		AAS entities
 *
 *
 *****************************************************************************/

#include "../game/q_shared.h"
#include "l_memory.h"
#include "l_script.h"
#include "l_precomp.h"
#include "l_struct.h"
#include "l_utils.h"
#include "l_log.h"
#include "aasfile.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "be_aas_funcs.h"
#include "be_interface.h"
#include "be_aas_def.h"

#define MASK_SOLID      CONTENTS_PLAYERCLIP

// Ridah, always use the default world for entities
extern aas_t aasworlds[MAX_AAS_WORLDS];

aas_t *defaultaasworld = aasworlds;

//FIXME: these might change
enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER
};

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_UpdateEntity( int entnum, bot_entitystate_t *state ) {
	int relink;
	aas_entity_t *ent;
	vec3_t absmins, absmaxs;

	if ( !( *defaultaasworld ).loaded ) {
		botimport.Print( PRT_MESSAGE, "AAS_UpdateEntity: not loaded\n" );
		return BLERR_NOAASFILE;
	} //end if

	ent = &( *defaultaasworld ).entities[entnum];

	ent->i.update_time = AAS_Time() - ent->i.ltime;
	ent->i.type = state->type;
	ent->i.flags = state->flags;
	ent->i.ltime = AAS_Time();
	VectorCopy( ent->i.origin, ent->i.lastvisorigin );
	VectorCopy( state->old_origin, ent->i.old_origin );
	ent->i.solid = state->solid;
	ent->i.groundent = state->groundent;
	ent->i.modelindex = state->modelindex;
	ent->i.modelindex2 = state->modelindex2;
	ent->i.frame = state->frame;
	//ent->i.event = state->event;
	ent->i.eventParm = state->eventParm;
	ent->i.powerups = state->powerups;
	ent->i.weapon = state->weapon;
	ent->i.legsAnim = state->legsAnim;
	ent->i.torsoAnim = state->torsoAnim;

//	ent->i.weapAnim = state->weapAnim;	//----(SA)
//----(SA)	didn't want to comment in as I wasn't sure of any implications of changing the aas_entityinfo_t and bot_entitystate_t structures.

	//number of the entity
	ent->i.number = entnum;
	//updated so set valid flag
	ent->i.valid = qtrue;
	//link everything the first frame

	if ( ( *defaultaasworld ).numframes == 1 ) {
		relink = qtrue;
	} else { relink = qfalse;}

	//
	if ( ent->i.solid == SOLID_BSP ) {
		//if the angles of the model changed
		if ( !VectorCompare( state->angles, ent->i.angles ) ) {
			VectorCopy( state->angles, ent->i.angles );
			relink = qtrue;
		} //end if
		  //get the mins and maxs of the model
		  //FIXME: rotate mins and maxs
		AAS_BSPModelMinsMaxsOrigin( ent->i.modelindex, ent->i.angles, ent->i.mins, ent->i.maxs, NULL );
	} //end if
	else if ( ent->i.solid == SOLID_BBOX ) {
		//if the bounding box size changed
		if ( !VectorCompare( state->mins, ent->i.mins ) ||
			 !VectorCompare( state->maxs, ent->i.maxs ) ) {
			VectorCopy( state->mins, ent->i.mins );
			VectorCopy( state->maxs, ent->i.maxs );
			relink = qtrue;
		} //end if
	} //end if
	  //if the origin changed
	if ( !VectorCompare( state->origin, ent->i.origin ) ) {
		VectorCopy( state->origin, ent->i.origin );
		relink = qtrue;
	} //end if
	  //if the entity should be relinked
	if ( relink ) {
		//don't link the world model
		if ( entnum != ENTITYNUM_WORLD ) {
			//absolute mins and maxs
			VectorAdd( ent->i.mins, ent->i.origin, absmins );
			VectorAdd( ent->i.maxs, ent->i.origin, absmaxs );

			//unlink the entity
			AAS_UnlinkFromAreas( ent->areas );
			//relink the entity to the AAS areas (use the larges bbox)
			ent->areas = AAS_LinkEntityClientBBox( absmins, absmaxs, entnum, PRESENCE_NORMAL );
			//unlink the entity from the BSP leaves
			AAS_UnlinkFromBSPLeaves( ent->leaves );
			//link the entity to the world BSP tree
			ent->leaves = AAS_BSPLinkEntity( absmins, absmaxs, entnum, 0 );
		} //end if
	} //end if
	return BLERR_NOERROR;
} //end of the function AAS_UpdateEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_EntityInfo( int entnum, aas_entityinfo_t *info ) {
	if ( !( *defaultaasworld ).initialized ) {
		botimport.Print( PRT_FATAL, "AAS_EntityInfo: (*defaultaasworld) not initialized\n" );
		memset( info, 0, sizeof( aas_entityinfo_t ) );
		return;
	} //end if

	if ( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityInfo: entnum %d out of range\n", entnum );
		memset( info, 0, sizeof( aas_entityinfo_t ) );
		return;
	} //end if

	memcpy( info, &( *defaultaasworld ).entities[entnum].i, sizeof( aas_entityinfo_t ) );
} //end of the function AAS_EntityInfo
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_EntityOrigin( int entnum, vec3_t origin ) {
	if ( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityOrigin: entnum %d out of range\n", entnum );
		VectorClear( origin );
		return;
	} //end if

	VectorCopy( ( *defaultaasworld ).entities[entnum].i.origin, origin );
} //end of the function AAS_EntityOrigin
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_EntityModelindex( int entnum ) {
	if ( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityModelindex: entnum %d out of range\n", entnum );
		return 0;
	} //end if
	return ( *defaultaasworld ).entities[entnum].i.modelindex;
} //end of the function AAS_EntityModelindex
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_EntityType( int entnum ) {
	if ( !( *defaultaasworld ).initialized ) {
		return 0;
	}

	if ( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityType: entnum %d out of range\n", entnum );
		return 0;
	} //end if
	return ( *defaultaasworld ).entities[entnum].i.type;
} //end of the AAS_EntityType
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_EntityModelNum( int entnum ) {
	if ( !( *defaultaasworld ).initialized ) {
		return 0;
	}

	if ( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntityModelNum: entnum %d out of range\n", entnum );
		return 0;
	} //end if
	return ( *defaultaasworld ).entities[entnum].i.modelindex;
} //end of the function AAS_EntityModelNum
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_OriginOfEntityWithModelNum( int modelnum, vec3_t origin ) {
	int i;
	aas_entity_t *ent;

	for ( i = 0; i < ( *defaultaasworld ).maxentities; i++ )
	{
		ent = &( *defaultaasworld ).entities[i];
		if ( ent->i.type == ET_MOVER ) {
			if ( ent->i.modelindex == modelnum ) {
				VectorCopy( ent->i.origin, origin );
				return qtrue;
			} //end if
		}
	} //end for
	return qfalse;
} //end of the function AAS_OriginOfEntityWithModelNum
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_EntitySize( int entnum, vec3_t mins, vec3_t maxs ) {
	aas_entity_t *ent;

	if ( !( *defaultaasworld ).initialized ) {
		return;
	}

	if ( entnum < 0 || entnum >= ( *defaultaasworld ).maxentities ) {
		botimport.Print( PRT_FATAL, "AAS_EntitySize: entnum %d out of range\n", entnum );
		return;
	} //end if

	ent = &( *defaultaasworld ).entities[entnum];
	VectorCopy( ent->i.mins, mins );
	VectorCopy( ent->i.maxs, maxs );
} //end of the function AAS_EntitySize
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_EntityBSPData( int entnum, bsp_entdata_t *entdata ) {
	aas_entity_t *ent;

	ent = &( *defaultaasworld ).entities[entnum];
	VectorCopy( ent->i.origin, entdata->origin );
	VectorCopy( ent->i.angles, entdata->angles );
	VectorAdd( ent->i.origin, ent->i.mins, entdata->absmins );
	VectorAdd( ent->i.origin, ent->i.maxs, entdata->absmaxs );
	entdata->solid = ent->i.solid;
	entdata->modelnum = ent->i.modelindex - 1;
} //end of the function AAS_EntityBSPData
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_ResetEntityLinks( void ) {
	int i;
	for ( i = 0; i < ( *defaultaasworld ).maxentities; i++ )
	{
		( *defaultaasworld ).entities[i].areas = NULL;
		( *defaultaasworld ).entities[i].leaves = NULL;
	} //end for
} //end of the function AAS_ResetEntityLinks
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_InvalidateEntities( void ) {
	int i;
	for ( i = 0; i < ( *defaultaasworld ).maxentities; i++ )
	{
		( *defaultaasworld ).entities[i].i.valid = qfalse;
		( *defaultaasworld ).entities[i].i.number = i;
	} //end for
} //end of the function AAS_InvalidateEntities
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_NearestEntity( vec3_t origin, int modelindex ) {
	int i, bestentnum;
	float dist, bestdist;
	aas_entity_t *ent;
	vec3_t dir;

	bestentnum = 0;
	bestdist = 99999;
	for ( i = 0; i < ( *defaultaasworld ).maxentities; i++ )
	{
		ent = &( *defaultaasworld ).entities[i];
		if ( ent->i.modelindex != modelindex ) {
			continue;
		}
		VectorSubtract( ent->i.origin, origin, dir );
		if ( abs( dir[0] ) < 40 ) {
			if ( abs( dir[1] ) < 40 ) {
				dist = VectorLength( dir );
				if ( dist < bestdist ) {
					bestdist = dist;
					bestentnum = i;
				} //end if
			} //end if
		} //end if
	} //end for
	return bestentnum;
} //end of the function AAS_NearestEntity
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_BestReachableEntityArea( int entnum ) {
	aas_entity_t *ent;

	ent = &( *defaultaasworld ).entities[entnum];
	return AAS_BestReachableLinkArea( ent->areas );
} //end of the function AAS_BestReachableEntityArea
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
int AAS_NextEntity( int entnum ) {
	if ( !( *defaultaasworld ).loaded ) {
		return 0;
	}

	if ( entnum < 0 ) {
		entnum = -1;
	}
	while ( ++entnum < ( *defaultaasworld ).maxentities )
	{
		if ( ( *defaultaasworld ).entities[entnum].i.valid ) {
			return entnum;
		}
	} //end while
	return 0;
} //end of the function AAS_NextEntity

// Ridah, used to find out if there is an entity touching the given area, if so, try and avoid it
/*
============
AAS_EntityInArea
============
*/
int AAS_IsEntityInArea( int entnumIgnore, int entnumIgnore2, int areanum ) {
	aas_link_t *link;
	aas_entity_t *ent;
//	int i;

	// RF, not functional (doesnt work with multiple areas)
	return qfalse;

	for ( link = ( *aasworld ).arealinkedentities[areanum]; link; link = link->next_ent )
	{
		//ignore the pass entity
		if ( link->entnum == entnumIgnore ) {
			continue;
		}
		if ( link->entnum == entnumIgnore2 ) {
			continue;
		}
		//
		ent = &( *defaultaasworld ).entities[link->entnum];
		if ( !ent->i.valid ) {
			continue;
		}
		if ( !ent->i.solid ) {
			continue;
		}
		return qtrue;
	}
/*
	ent = (*defaultaasworld).entities;
	for (i = 0; i < (*defaultaasworld).maxclients; i++, ent++)
	{
		if (!ent->i.valid)
			continue;
		if (!ent->i.solid)
			continue;
		if (i == entnumIgnore)
			continue;
		if (i == entnumIgnore2)
			continue;
		for (link = ent->areas; link; link = link->next_area)
		{
			if (link->areanum == areanum)
			{
				return qtrue;
			} //end if
		} //end for
	}
*/
	return qfalse;
}

/*
=============
AAS_SetAASBlockingEntity
=============
*/
int AAS_EnableRoutingArea( int areanum, int enable );
void AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, qboolean blocking ) {
	int areas[128];
	int numareas, i, w;
	//
	// check for resetting AAS blocking
	if ( VectorCompare( absmin, absmax ) && blocking < 0 ) {
		for ( w = 0; w < MAX_AAS_WORLDS; w++ ) {
			AAS_SetCurrentWorld( w );
			//
			if ( !( *aasworld ).loaded ) {
				continue;
			}
			// now clear blocking status
			for ( i = 1; i < ( *aasworld ).numareas; i++ ) {
				AAS_EnableRoutingArea( i, qtrue );
			}
		}
		//
		return;
	}
	//
	for ( w = 0; w < MAX_AAS_WORLDS; w++ ) {
		AAS_SetCurrentWorld( w );
		//
		if ( !( *aasworld ).loaded ) {
			continue;
		}
		// grab the list of areas
		numareas = AAS_BBoxAreas( absmin, absmax, areas, 128 );
		// now set their blocking status
		for ( i = 0; i < numareas; i++ ) {
			AAS_EnableRoutingArea( areas[i], !blocking );
		}
	}
}
