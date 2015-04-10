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
 * name:		be_aas_entity.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN
//invalidates all entity infos
void AAS_InvalidateEntities( void );
//resets the entity AAS and BSP links (sets areas and leaves pointers to NULL)
void AAS_ResetEntityLinks( void );
//updates an entity
int AAS_UpdateEntity( int ent, bot_entitystate_t *state );
//gives the entity data used for collision detection
void AAS_EntityBSPData( int entnum, bsp_entdata_t *entdata );
#endif //AASINTERN

//returns the size of the entity bounding box in mins and maxs
void AAS_EntitySize( int entnum, vec3_t mins, vec3_t maxs );
//returns the BSP model number of the entity
int AAS_EntityModelNum( int entnum );
//returns the origin of an entity with the given model number
int AAS_OriginOfEntityWithModelNum( int modelnum, vec3_t origin );
//returns the best reachable area the entity is situated in
int AAS_BestReachableEntityArea( int entnum );
//returns the info of the given entity
void AAS_EntityInfo( int entnum, aas_entityinfo_t *info );
//returns the next entity
int AAS_NextEntity( int entnum );
//returns the origin of the entity
void AAS_EntityOrigin( int entnum, vec3_t origin );
//returns the entity type
int AAS_EntityType( int entnum );
//returns the model index of the entity
int AAS_EntityModelindex( int entnum );
// Ridah
int AAS_IsEntityInArea( int entnumIgnore, int entnumIgnore2, int areanum );
