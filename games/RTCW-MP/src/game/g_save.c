/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		g_save.c
 *
 */

// Arnout: removed for multiplayer
#if 0

#include "../game/g_local.h"
#include "../game/q_shared.h"
#include "../game/botlib.h"      //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../botai/botai.h"          //bot ai interface

#include "ai_cast.h"

/*
Wolf savegame system.

Using the "checkpoint" system, we only need to save at specific locations, but various entities
may have changed behind us, so therefore we need to save as much as possible, but without going
overboard.

For now, everything is saved from the entity, client and cast_state structures, except the fields
defined in the ignoreField structures below. Any pointer fields need to be specified in the
saveField structures below.

!! NOTE: when working on Wolf patches, make sure you only add fields to the very end of the three
  main structures saved here (entity, client, cast_state). If any fields are inserted in the middle
  of these structures, savegames will become corrupted, and there is no way of checking for this,
  so it'll just crash.
*/

#define SAVE_VERSION    7

typedef enum {
	F_NONE,
	F_STRING,
	F_ENTITY,           // index on disk, pointer in memory
	F_ITEM,             // index on disk, pointer in memory
	F_CLIENT,           // index on disk, pointer in memory
	F_FUNCTION
} saveFieldtype_t;

typedef struct {
	int ofs;
	saveFieldtype_t type;
} saveField_t;

//.......................................................................................
// these are the fields that cannot be saved directly, so they need to be converted
static saveField_t gentityFields[] = {
	{FOFS( client ),      F_CLIENT},
	{FOFS( classname ),   F_STRING},
	{FOFS( model ),       F_STRING},
	{FOFS( model2 ),      F_STRING},
	{FOFS( parent ),      F_ENTITY},
	{FOFS( nextTrain ),   F_ENTITY},
	{FOFS( prevTrain ),   F_ENTITY},
	{FOFS( message ),     F_STRING},
	{FOFS( target ),      F_STRING},
	{FOFS( targetname ),  F_STRING},
	{FOFS( team ),        F_STRING},
	{FOFS( target_ent ),  F_ENTITY},
	{FOFS( think ),       F_FUNCTION},
	{FOFS( reached ),     F_FUNCTION},
	{FOFS( blocked ),     F_FUNCTION},
	{FOFS( touch ),       F_FUNCTION},
	{FOFS( use ),         F_FUNCTION},
	{FOFS( pain ),        F_FUNCTION},
	{FOFS( die ),         F_FUNCTION},
	{FOFS( chain ),       F_ENTITY},
	{FOFS( enemy ),       F_ENTITY},
	{FOFS( activator ),   F_ENTITY},
	{FOFS( teamchain ),   F_ENTITY},
	{FOFS( teammaster ),  F_ENTITY},
	{FOFS( item ),        F_ITEM},
	{FOFS( aiAttributes ),F_STRING},
	{FOFS( aiName ),      F_STRING},
	{FOFS( AIScript_AlertEntity ), F_FUNCTION},
	{FOFS( aiSkin ),      F_STRING},
	{FOFS( aihSkin ),     F_STRING},
	{FOFS( dl_stylestring ),  F_STRING},
	{FOFS( dl_shader ),   F_STRING},
	{FOFS( melee ),       F_ENTITY},
	{FOFS( spawnitem ),   F_STRING},
	{FOFS( track ),       F_STRING},
	{FOFS( scriptName ),  F_STRING},
	{FOFS( scriptStatus.animatingParams ),    F_STRING},
	{FOFS( tagName ),     F_STRING},
	{FOFS( tagParent ),   F_ENTITY},

	{0, 0}
};

static saveField_t gclientFields[] = {
	{CFOFS( hook ),       F_ENTITY},

	{0, 0}
};

static saveField_t castStateFields[] = {
	{CSFOFS( aifunc ),    F_FUNCTION},
	{CSFOFS( oldAifunc ), F_FUNCTION},
	{CSFOFS( painfunc ),  F_FUNCTION},
	{CSFOFS( deathfunc ), F_FUNCTION},
	{CSFOFS( sightfunc ), F_FUNCTION},
	{CSFOFS( sightEnemy ),    F_FUNCTION},
	{CSFOFS( sightFriend ),   F_FUNCTION},
	{CSFOFS( activate ),  F_FUNCTION},
	{CSFOFS( aifuncAttack1 ), F_FUNCTION},
	{CSFOFS( aifuncAttack2 ), F_FUNCTION},
	{CSFOFS( aifuncAttack3 ), F_FUNCTION},

	{0, 0}
};

//.......................................................................................
// this is where we define fields or sections of structures that we should totally ignore
typedef struct {
	int ofs;
	int len;
} ignoreField_t;

static ignoreField_t gentityIgnoreFields[] = {
	// don't process events that have already occured before the game was saved
	{FOFS( s.events[0] ),     sizeof( int ) * MAX_EVENTS},
	{FOFS( s.eventParms[0] ), sizeof( int ) * MAX_EVENTS},
	{FOFS( s.eventSequence ), sizeof( int )},

	{FOFS( numScriptEvents ), sizeof( int )},
	{FOFS( scriptEvents ),    sizeof( g_script_event_t * ) },   // gets created upon parsing the script file, this is static while playing

	{0, 0}
};

static ignoreField_t gclientIgnoreFields[] = {
	// don't process events that have already occured before the game was saved
	{CFOFS( ps.events[0] ),       sizeof( int ) * MAX_EVENTS},
	{CFOFS( ps.eventParms[0] ),   sizeof( int ) * MAX_EVENTS},
	{CFOFS( ps.eventSequence ),   sizeof( int )},
	{CFOFS( ps.oldEventSequence ),sizeof( int )},

	{0, 0}
};

static ignoreField_t castStateIgnoreFields[] = {
	{CSFOFS( bs ),    sizeof( bot_state_t * )},
	{CSFOFS( numCastScriptEvents ),   sizeof( int )},
	{CSFOFS( castScriptEvents ), sizeof( cast_script_event_t * ) }, // gets created upon parsing the script file, this is static while playing
	{CSFOFS( castScriptStatusCurrent ), sizeof( cast_script_status_t * ) },
	{CSFOFS( weaponInfo ),    sizeof( cast_weapon_info_t * )},
	{CSFOFS( totalPlayTime ), sizeof( int )},

	{0, 0}
};

//.......................................................................................
// persistant data is optionally carried across level changes
// !! WARNING: cannot save pointer or string variables
typedef struct {
	int ofs;
	int len;
} persField_t;

static persField_t gentityPersFields[] = {
	{FOFS( health ),              sizeof( int )},
	{0, 0}
};

static persField_t gclientPersFields[] = {
	{CFOFS( ps.weapon ),          sizeof( int )},
	{CFOFS( ps.ammo[0] ),         sizeof( int ) * MAX_WEAPONS},
	{CFOFS( ps.ammoclip[0] ),     sizeof( int ) * MAX_WEAPONS}, //----(SA)	added for ammo in clip
	{CFOFS( ps.persistant[0] ),   sizeof( int ) * MAX_PERSISTANT},
	{CFOFS( ps.stats[0] ),        sizeof( int ) * MAX_STATS},
	{CFOFS( ps.powerups[0] ),     sizeof( int ) * MAX_POWERUPS},
	{0, 0}
};

static persField_t castStatePersFields[] = {
	// TODO: will we be transporting AI's between levels?
	// FIXME: if so, we can't save strings in here, so how are we going to create the new AI
	// in the next level, with all his strings and pointers attached?
	{0, 0}
};

//.......................................................................................
// this stores all functions in the game code
typedef struct {
	char *funcStr;
	byte *funcPtr;
} funcList_t;

//-----------------
// MSVC likes to needlessly(?) warn about these defines, so disable certain warnings temporarily
#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4054 )
#endif
//-----------------

#include "g_func_decs.h" // declare all game functions

funcList_t funcList[] = {
	#include "g_funcs.h"
};

//-----------------
#ifdef _WIN32
#pragma warning( pop ) // return previous warning state
#endif
//-----------------


//=========================================================

/*
===============
G_SaveWriteError
===============
*/
void G_SaveWriteError( void ) {
// TTimo
#ifdef __linux__
	G_Error( "Unable to save game.\n\nPlease check that you have at least 5mb free of disk space in your home directory." );
#else
	G_Error( "Unable to save game.\n\nPlease check that game drive has at least 5mb free space." );
#endif
}

//=========================================================

funcList_t *G_FindFuncAtAddress( byte *adr ) {
	int i;

	for ( i = 0; funcList[i].funcStr; i++ ) {
		if ( funcList[i].funcPtr == adr ) {
			return &funcList[i];
		}
	}
	return NULL;
}

byte *G_FindFuncByName( char *name ) {
	int i;

	for ( i = 0; funcList[i].funcStr; i++ ) {
		if ( !strcmp( name, funcList[i].funcStr ) ) {
			return funcList[i].funcPtr;
		}
	}
	return NULL;
}

void WriteField1( saveField_t *field, byte *base ) {
	void        *p;
	int len;
	int index;
	funcList_t  *func;

	p = ( void * )( base + field->ofs );
	switch ( field->type )
	{
	case F_STRING:
		if ( *(char **)p ) {
			len = strlen( *(char **)p ) + 1;
		} else {
			len = 0;
		}
		*(int *)p = len;
		break;
	case F_ENTITY:
		if ( *(gentity_t **)p == NULL ) {
			index = -1;
		} else {
			index = *(gentity_t **)p - g_entities;
		}
		*(int *)p = index;
		break;
	case F_CLIENT:
		if ( *(gclient_t **)p == NULL ) {
			index = -1;
		} else {
			index = *(gclient_t **)p - level.clients;
		}
		*(int *)p = index;
		break;
	case F_ITEM:
		if ( *(gitem_t **)p == NULL ) {
			index = -1;
		} else {
			index = *(gitem_t **)p - bg_itemlist;
		}
		*(int *)p = index;
		break;

		//	match this with a function address in the function list, which is built using the
		//	"extractfuncs.bat" in the utils folder. We then save the string equivalent
		//	of the function. This effectively gives us cross-version save games.
	case F_FUNCTION:
		if ( *(byte **)p == NULL ) {
			len = 0;
		} else {
			func = G_FindFuncAtAddress( *(byte **)p );
			if ( !func ) {
				G_Error( "WriteField1: unknown function, cannot save game" );
			}
			len = strlen( func->funcStr ) + 1;
		}
		*(int *)p = len;
		break;

	default:
		G_Error( "WriteField1: unknown field type" );
	}
}


void WriteField2( fileHandle_t f, saveField_t *field, byte *base ) {
	int len;
	void        *p;
	funcList_t  *func;

	p = ( void * )( base + field->ofs );
	switch ( field->type )
	{
	case F_STRING:
		if ( *(char **)p ) {
			len = strlen( *(char **)p ) + 1;
			if ( !trap_FS_Write( *(char **)p, len, f ) ) {
				G_SaveWriteError();
			}
		}
		break;
	case F_FUNCTION:
		if ( *(byte **)p ) {
			func = G_FindFuncAtAddress( *(byte **)p );
			if ( !func ) {
				G_Error( "WriteField1: unknown function, cannot save game" );
			}
			len = strlen( func->funcStr ) + 1;
			if ( !trap_FS_Write( func->funcStr, len, f ) ) {
				G_SaveWriteError();
			}
		}
		break;
	default: // TTimo F_NOE F_ENTITY F_ITEM F_CLIENT not handled in switch
		break;
	}
}

void ReadField( fileHandle_t f, saveField_t *field, byte *base ) {
	void        *p;
	int len;
	int index;
	char        *funcStr;

	p = ( void * )( base + field->ofs );
	switch ( field->type )
	{
	case F_STRING:
		len = *(int *)p;
		if ( !len ) {
			*(char **)p = NULL;
		} else
		{
			*(char **)p = G_Alloc( len );
			trap_FS_Read( *(char **)p, len, f );
		}
		break;
	case F_ENTITY:
		index = *(int *)p;
		if ( index == -1 ) {
			*(gentity_t **)p = NULL;
		} else {
			*(gentity_t **)p = &g_entities[index];
		}
		break;
	case F_CLIENT:
		index = *(int *)p;
		if ( index == -1 ) {
			*(gclient_t **)p = NULL;
		} else {
			*(gclient_t **)p = &level.clients[index];
		}
		break;
	case F_ITEM:
		index = *(int *)p;
		if ( index == -1 ) {
			*(gitem_t **)p = NULL;
		} else {
			*(gitem_t **)p = &bg_itemlist[index];
		}
		break;

		//relative to code segment
	case F_FUNCTION:
		len = *(int *)p;
		if ( !len ) {
			*(byte **)p = NULL;
		} else
		{
			funcStr = G_Alloc( len );
			trap_FS_Read( funcStr, len, f );
			if ( !( *(byte **)p = G_FindFuncByName( funcStr ) ) ) {
				G_Error( "ReadField: unknown function '%s'\ncannot load game", funcStr );
			}
		}
		break;

	default:
		G_Error( "ReadField: unknown field type" );
	}
}

//=========================================================

/*
===============
WriteClient
===============
*/
void WriteClient( fileHandle_t f, gclient_t *cl ) {
	saveField_t *field;
	gclient_t temp;

	// copy the structure across, then process the fields
	temp = *cl;

	// first, kill all events (assume they have been processed)
	memset( temp.ps.events, 0, sizeof( temp.ps.events ) );
	memset( temp.ps.eventParms, 0, sizeof( temp.ps.eventParms ) );
	temp.ps.eventSequence = 0;

	// change the pointers to lengths or indexes
	for ( field = gclientFields ; field->type ; field++ )
	{
		WriteField1( field, (byte *)&temp );
	}

	// write the block
	if ( !trap_FS_Write( &temp, sizeof( temp ), f ) ) {
		G_SaveWriteError();
	}

	// now write any allocated data following the edict
	for ( field = gclientFields ; field->type ; field++ )
	{
		WriteField2( f, field, (byte *)cl );
	}

}

/*
===============
ReadClient
===============
*/
void ReadClient( fileHandle_t f, gclient_t *client, int size ) {
	saveField_t *field;
	ignoreField_t *ifield;
	gclient_t temp;
	gentity_t   *ent;

	trap_FS_Read( &temp, size, f );

	// convert any feilds back to the correct data
	for ( field = gclientFields ; field->type ; field++ )
	{
		ReadField( f, field, (byte *)&temp );
	}

	// backup any fields that we don't want to read in
	for ( ifield = gclientIgnoreFields ; ifield->len ; ifield++ )
	{
		memcpy( ( (byte *)&temp ) + ifield->ofs, ( (byte *)client ) + ifield->ofs, ifield->len );
	}

	// now copy the temp structure into the existing structure
	memcpy( client, &temp, size );

	// make sure they face the right way
	//client->ps.pm_flags |= PMF_RESPAWNED;
	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	ent = &g_entities[client->ps.clientNum];

	// make sure they face the right way
	// if it's the player, see if we need to put them at a mission marker
	if ( !( ent->r.svFlags & SVF_CASTAI ) && ent->missionLevel > 0 ) {
		gentity_t *trav;

		// TTimo gcc: suggest parentheses around assignment used as truth value
		for ( trav = NULL; ( trav = G_Find( trav, FOFS( classname ), "info_player_checkpoint" ) ); ) {
			if ( trav->missionLevel == ent->missionLevel && Distance( trav->s.origin, ent->r.currentOrigin ) < 800 ) {
				G_SetOrigin( ent, trav->s.origin );
				VectorCopy( trav->s.origin, ent->client->ps.origin );

				trap_GetUsercmd( ent->client - level.clients, &ent->client->pers.cmd );
				SetClientViewAngle( ent, trav->s.angles );
				break;
			}
		}

		if ( !trav ) {
			trap_GetUsercmd( ent->client - level.clients, &ent->client->pers.cmd );
			SetClientViewAngle( ent, ent->client->ps.viewangles );
		}
	} else {
		trap_GetUsercmd( ent->client - level.clients, &ent->client->pers.cmd );
		SetClientViewAngle( ent, ent->client->ps.viewangles );
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	trap_GetUsercmd( ent - g_entities, &ent->client->pers.cmd );
	ent->client->ps.commandTime = ent->client->pers.cmd.serverTime - 100;
	ClientThink( ent - g_entities );

	// tell the client to reset it's cgame stuff
	if ( !( ent->r.svFlags & SVF_CASTAI ) ) {
		trap_SendServerCommand( client->ps.clientNum, "map_restart\n" );
	}
}

//=========================================================

/*
===============
WriteEntity
===============
*/
void WriteEntity( fileHandle_t f, gentity_t *ent ) {
	saveField_t *field;
	gentity_t temp;

	// copy the structure across, then process the fields
	temp = *ent;

	// first, kill all events (assume they have been processed)
	memset( temp.s.events, 0, sizeof( temp.s.events ) );
	memset( temp.s.eventParms, 0, sizeof( temp.s.eventParms ) );
	temp.s.eventSequence = 0;

	// change the pointers to lengths or indexes
	for ( field = gentityFields ; field->type ; field++ )
	{
		WriteField1( field, (byte *)&temp );
	}

	// write the block
	if ( !trap_FS_Write( &temp, sizeof( temp ), f ) ) {
		G_SaveWriteError();
	}

	// now write any allocated data following the edict
	for ( field = gentityFields ; field->type ; field++ )
	{
		WriteField2( f, field, (byte *)ent );
	}

}

/*
===============
ReadEntity
===============
*/
void ReadEntity( fileHandle_t f, gentity_t *ent, int size ) {
	saveField_t *field;
	ignoreField_t *ifield;
	gentity_t temp, backup;

	backup = *ent;

	trap_FS_Read( &temp, size, f );

	// convert any feilds back to the correct data
	for ( field = gentityFields ; field->type ; field++ )
	{
		ReadField( f, field, (byte *)&temp );
	}

	// backup any fields that we don't want to read in
	for ( ifield = gentityIgnoreFields ; ifield->len ; ifield++ )
	{
		memcpy( ( (byte *)&temp ) + ifield->ofs, ( (byte *)ent ) + ifield->ofs, ifield->len );
	}

	// now copy the temp structure into the existing structure
	memcpy( ent, &temp, size );

	// notify server of changes in position/orientation
	if ( ent->r.linked ) {
		trap_LinkEntity( ent );
	} else {
		trap_UnlinkEntity( ent );
	}

	// if this is a mover, check areaportals
	if ( ent->s.eType == ET_MOVER && ent->moverState != backup.moverState ) {
		if ( ent->teammaster == ent || !ent->teammaster ) {
			if ( ent->moverState == MOVER_POS1ROTATE || ent->moverState == MOVER_POS1 ) {
				// closed areaportal
				trap_AdjustAreaPortalState( ent, qfalse );
			} else {    // must be open
				trap_AdjustAreaPortalState( ent, qtrue );
			}
		}
	}

	// check for blocking AAS at save time
	if ( ent->AASblocking ) {
		G_SetAASBlockingEntity( ent, qtrue );
	}

	// check for this being a tagconnect entity
	if ( ent->tagName ) {
		G_ProcessTagConnect( ent );
	}
}

//=========================================================

/*
===============
WriteCastState
===============
*/
void WriteCastState( fileHandle_t f, cast_state_t *cs ) {
	saveField_t *field;
	cast_state_t temp;

	// copy the structure across, then process the fields
	temp = *cs;

	// change the pointers to lengths or indexes
	for ( field = castStateFields ; field->type ; field++ )
	{
		WriteField1( field, (byte *)&temp );
	}

	// write the block
	if ( !trap_FS_Write( &temp, sizeof( temp ), f ) ) {
		G_SaveWriteError();
	}

	// now write any allocated data following the edict
	for ( field = castStateFields ; field->type ; field++ )
	{
		WriteField2( f, field, (byte *)cs );
	}

}

/*
===============
ReadCastState
===============
*/
void ReadCastState( fileHandle_t f, cast_state_t *cs, int size ) {
	saveField_t *field;
	ignoreField_t *ifield;
	cast_state_t temp;

	trap_FS_Read( &temp, size, f );

	// convert any feilds back to the correct data
	for ( field = castStateFields ; field->type ; field++ )
	{
		ReadField( f, field, (byte *)&temp );
	}

	// backup any fields that we don't want to read in
	for ( ifield = castStateIgnoreFields ; ifield->len ; ifield++ )
	{
		memcpy( ( (byte *)&temp ) + ifield->ofs, ( (byte *)cs ) + ifield->ofs, ifield->len );
	}

	// now copy the temp structure into the existing structure
	memcpy( cs, &temp, size );
}

//=========================================================

/*
===============
G_SaveGame

  returns qtrue if successful

  TODO: have trap_FS_Write return the number of byte's written, so if it doesn't
  succeed, we can abort the save, and not save the file. This means we should
  save to a temporary name, then copy it across to the real name after success,
  so full disks don't result in lost saved games.
===============
*/
qboolean G_SaveGame( char *username ) {
	char filename[MAX_QPATH];
	char mapstr[MAX_QPATH];
	vmCvar_t mapname;
	fileHandle_t f;
	int i;
	gentity_t   *ent;
	gclient_t   *cl;
	cast_state_t    *cs;

// JPW NERVE -- save/load not supported in MP and causes problems during level loads if "pounding random keys"
	return qfalse;
// jpw

	if ( !username ) {
		username = "current";
	}

	// open the file
	Com_sprintf( filename, MAX_QPATH, "save/temp.svg", username );
	if ( trap_FS_FOpenFile( filename, &f, FS_WRITE ) < 0 ) {
		G_Error( "G_SaveGame: cannot open file for saving\n" );
	}

	// write the version
	i = SAVE_VERSION;
	if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
		G_SaveWriteError();
	}

	// write the mapname
	trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	Com_sprintf( mapstr, MAX_QPATH, mapname.string );
	if ( !trap_FS_Write( mapstr, MAX_QPATH, f ) ) {
		G_SaveWriteError();
	}

	// write out the level time
	if ( !trap_FS_Write( &level.time, sizeof( level.time ), f ) ) {
		G_SaveWriteError();
	}

	// write the totalPlayTime
	i = caststates[0].totalPlayTime;
	if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
		G_SaveWriteError();
	}

	// write out the entity structures
	i = sizeof( gentity_t );
	if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
		G_SaveWriteError();
	}
	for ( i = 0 ; i < level.num_entities ; i++ )
	{
		ent = &g_entities[i];
		if ( !ent->inuse || ent->s.number == ENTITYNUM_WORLD ) {
			continue;
		}
		if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
			G_SaveWriteError();
		}
		WriteEntity( f, ent );
	}
	i = -1;
	if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
		G_SaveWriteError();
	}

	// write out the client structures
	i = sizeof( gclient_t );
	if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
		G_SaveWriteError();
	}
	for ( i = 0 ; i < MAX_CLIENTS ; i++ )
	{
		cl = &level.clients[i];
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
			G_SaveWriteError();
		}
		WriteClient( f, cl );
	}
	i = -1;
	if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
		G_SaveWriteError();
	}

	// write out the cast_state structures
	i = sizeof( cast_state_t );
	if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
		G_SaveWriteError();
	}
	for ( i = 0 ; i < level.numConnectedClients ; i++ )
	{
		cs = &caststates[i];
		if ( !cs->bs ) {
			continue;
		}
		if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
			G_SaveWriteError();
		}
		WriteCastState( f, cs );
	}
	i = -1;
	if ( !trap_FS_Write( &i, sizeof( i ), f ) ) {
		G_SaveWriteError();
	}

	trap_FS_FCloseFile( f );

	// now rename the file to the actual file
	Com_sprintf( mapstr, MAX_QPATH, "save/%s.svg", username );
	trap_FS_Rename( filename, mapstr );

	return qtrue;
}

/*
===============
G_UpdatePlayTime

  updates the total playing time in the current savegame
===============
*/
void G_UpdatePlayTime( void ) {
	fileHandle_t f, of;
	int i, len;
	char mapname[MAX_QPATH];

	// open the input file
	if ( ( len = trap_FS_FOpenFile( "save/current.svg", &f, FS_READ ) ) < 0 ) {
		G_Error( "G_SaveGame: cannot open file for saving\n" );
	}

	// open the output file
	if ( trap_FS_FOpenFile( "save/current.tmp", &of, FS_WRITE ) < 0 ) {
		G_Error( "G_SaveGame: cannot open file for saving\n" );
	}

	// start writing out the new file

	// read the version
	trap_FS_Read( &i, sizeof( i ), f );
	if ( i != SAVE_VERSION ) {
		G_Error( "G_LoadGame: savegame is wrong version (%i, should be %i)\n", i, SAVE_VERSION );
	}
	if ( !trap_FS_Write( &i, sizeof( i ), of ) ) {
		G_SaveWriteError();
	}
	len -= sizeof( i );

	// read the mapname
	trap_FS_Read( mapname, MAX_QPATH, f );
	if ( !trap_FS_Write( mapname, MAX_QPATH, of ) ) {
		G_SaveWriteError();
	}
	len -= MAX_QPATH;

	// read the level time
	trap_FS_Read( &i, sizeof( i ), f );
	if ( !trap_FS_Write( &i, sizeof( i ), of ) ) {
		G_SaveWriteError();
	}
	len -= sizeof( i );

	// read the totalPlayTime
	trap_FS_Read( &i, sizeof( i ), f );

	// write the updated totalPlayTime
	i = caststates[0].totalPlayTime;
	if ( !trap_FS_Write( &i, sizeof( i ), of ) ) {
		G_SaveWriteError();
	}
	len -= sizeof( i );

	// just copy the rest of the file
	while ( len ) {
		i = MAX_QPATH;
		len -= i;
		if ( len < 0 ) {
			i -= -len;
			len = 0;
		}
		trap_FS_Read( mapname, i, f );
		if ( !trap_FS_Write( mapname, i, of ) ) {
			G_SaveWriteError();
		}
	}

	trap_FS_FCloseFile( f );
	trap_FS_FCloseFile( of );

	// now rename the temp file back to current
	trap_FS_Rename( "save/current.tmp", "save/current.svg" );

/*
	// open the input file
	if ((len = trap_FS_FOpenFile( "save/current.tmp", &f, FS_READ )) < 0) {
		G_Error( "G_SaveGame: cannot open file for saving\n" );
	}

	// open the output file
	if (trap_FS_FOpenFile( "save/current.svg", &f, FS_WRITE ) < 0) {
		G_Error( "G_SaveGame: cannot open file for saving\n" );
	}

	// just copy the rest of the file
	while (len) {
		i = MAX_QPATH;
		len -= i;
		if (len < 0) {
			i -= -len;
			len = 0;
		}
		trap_FS_Read (mapname, i, f);
		trap_FS_Write (mapname, i, of);
	}

	trap_FS_FCloseFile(f);
	trap_FS_FCloseFile(of);

	// clear the temp file, since we can't delete it, just make it ZERO size
	if (trap_FS_FOpenFile( "save/current.tmp", &of, FS_WRITE ) < 0) {
		G_Error( "G_SaveGame: cannot open file for saving\n" );
	}
	trap_FS_FCloseFile(of);
*/
}


/*
===============
G_LoadGame

  Always loads in "current.svg". So if loading a specific savegame, first copy it to that.
===============
*/
void G_LoadGame( char *filename ) {
	char mapname[MAX_QPATH];
	fileHandle_t f;
	int i, size, last;
	gentity_t   *ent;
	gclient_t   *cl;
	cast_state_t    *cs;
	// TTimo might be used uninitialized
	int totalPlayTime = 0;
	int ver;

// JPW NERVE -- save/load not permitted in MP and causes problems if players are "pounding keys" in unsupported configs during level load
	return;
// jpw

	//if (!filename) {
	filename = "save/current.svg";
	//}

	// open the file
	if ( trap_FS_FOpenFile( filename, &f, FS_READ ) < 0 ) {
		G_Error( "G_LoadGame: savegame '%s' not found\n", filename );
	}

	// read the version
	trap_FS_Read( &i, sizeof( i ), f );
	if ( i != SAVE_VERSION && i != 6 ) {    // special case, so I can debug savegames from our testing session
		G_Error( "G_LoadGame: savegame '%s' is wrong version (%i, should be %i)\n", filename, i, SAVE_VERSION );
	}
	ver = i;

	// read the mapname (this is only used in the sever exe, so just discard it)
	trap_FS_Read( mapname, MAX_QPATH, f );

	// read the level time
	trap_FS_Read( &i, sizeof( i ), f );

	if ( ver > 6 ) {
		// read the totalPlayTime
		trap_FS_Read( &i, sizeof( i ), f );
		totalPlayTime = i;
	}

	// NOTE: do not change the above order without also changing the server code

	// reset all AAS blocking entities
	trap_AAS_SetAASBlockingEntity( vec3_origin, vec3_origin, -1 );

	// read the entity structures
	trap_FS_Read( &i, sizeof( i ), f );
	size = i;
	last = 0;
	while ( 1 )
	{
		trap_FS_Read( &i, sizeof( i ), f );
		if ( i < 0 ) {
			break;
		}
		if ( i >= MAX_GENTITIES ) {
			G_Error( "G_LoadGame: entitynum out of range (%i, MAX = %i)\n", i, MAX_GENTITIES );
		}
		ent = &g_entities[i];
		ReadEntity( f, ent, size );
		// free all entities that we skipped
		for ( ; last < i; last++ ) {
			if ( g_entities[last].inuse && i != ENTITYNUM_WORLD ) {
				if ( last < MAX_CLIENTS ) {
					trap_DropClient( last, "" );
				} else {
					G_FreeEntity( &g_entities[last] );
				}
			}
		}
		last = i + 1;
	}

	// read the client structures
	trap_FS_Read( &i, sizeof( i ), f );
	size = i;
	while ( 1 )
	{
		trap_FS_Read( &i, sizeof( i ), f );
		if ( i < 0 ) {
			break;
		}
		if ( i > MAX_CLIENTS ) {
			G_Error( "G_LoadGame: clientnum out of range\n" );
		}
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			trap_FS_FCloseFile( f );
			G_Error( "G_LoadGame: client mis-match in savegame" );
		}
		ReadClient( f, cl, size );
	}

	// read the cast_state structures
	trap_FS_Read( &i, sizeof( i ), f );
	size = i;
	while ( 1 )
	{
		trap_FS_Read( &i, sizeof( i ), f );
		if ( i < 0 ) {
			break;
		}
		if ( i > MAX_CLIENTS ) {
			G_Error( "G_LoadGame: clientnum out of range\n" );
		}
		cs = &caststates[i];
		ReadCastState( f, cs, size );
	}

	trap_FS_FCloseFile( f );

	// now increment the attempts field, and save
	++caststates[0].attempts;
	caststates[0].lastLoadTime = level.time;
	caststates[0].totalPlayTime = totalPlayTime;
	G_SaveGame( NULL );
}

//=========================================================

/*
===============
PersWriteClient
===============
*/
void PersWriteClient( fileHandle_t f, gclient_t *cl ) {
	persField_t *field;

	// save the fields
	for ( field = gclientPersFields ; field->len ; field++ )
	{   // write the block
		trap_FS_Write( ( void * )( (byte *)cl + field->ofs ), field->len, f );
	}
}

/*
===============
PersReadClient
===============
*/
void PersReadClient( fileHandle_t f, gclient_t *cl ) {
	persField_t *field;

	// read the fields
	for ( field = gclientPersFields ; field->len ; field++ )
	{   // read the block
		trap_FS_Read( ( void * )( (byte *)cl + field->ofs ), field->len, f );
	}
}

//=========================================================

/*
===============
PersWriteEntity
===============
*/
void PersWriteEntity( fileHandle_t f, gentity_t *ent ) {
	persField_t *field;

	// save the fields
	for ( field = gentityPersFields ; field->len ; field++ )
	{   // write the block
		trap_FS_Write( ( void * )( (byte *)ent + field->ofs ), field->len, f );
	}
}

/*
===============
PersReadEntity
===============
*/
void PersReadEntity( fileHandle_t f, gentity_t *cl ) {
	persField_t *field;

	// read the fields
	for ( field = gentityPersFields ; field->len ; field++ )
	{   // read the block
		trap_FS_Read( ( void * )( (byte *)cl + field->ofs ), field->len, f );
	}
}

//=========================================================

/*
===============
PersWriteCastState
===============
*/
void PersWriteCastState( fileHandle_t f, cast_state_t *cs ) {
	persField_t *field;

	// save the fields
	for ( field = castStatePersFields ; field->len ; field++ )
	{   // write the block
		trap_FS_Write( ( void * )( (byte *)cs + field->ofs ), field->len, f );
	}
}

/*
===============
PersReadCastState
===============
*/
void PersReadCastState( fileHandle_t f, cast_state_t *cs ) {
	persField_t *field;

	// read the fields
	for ( field = castStatePersFields ; field->len ; field++ )
	{   // read the block
		trap_FS_Read( ( void * )( (byte *)cs + field->ofs ), field->len, f );
	}
}

//=========================================================

/*
===============
G_SavePersistant

  returns qtrue if successful

  NOTE: only saves the local player's data, doesn't support AI characters

  TODO: have trap_FS_Write return the number of byte's written, so if it doesn't
  succeed, we can abort the save, and not save the file. This means we should
  save to a temporary name, then copy it across to the real name after success,
  so full disks don't result in lost saved games.
===============
*/
qboolean G_SavePersistant( char *nextmap ) {
	char filename[MAX_QPATH];
	fileHandle_t f;
	int persid;

	// open the file
	Com_sprintf( filename, MAX_QPATH, "save/current.psw" );
	if ( trap_FS_FOpenFile( filename, &f, FS_WRITE ) < 0 ) {
		G_Error( "G_SavePersistant: cannot open '%s' for saving\n", filename );
	}

	// write the mapname
	trap_FS_Write( nextmap, MAX_QPATH, f );

	// save out the pers id
	persid = trap_Milliseconds();
	trap_FS_Write( &persid, sizeof( persid ), f );
	trap_Cvar_Set( "persid", va( "%i", persid ) );

	// write out the entity structure
	PersWriteEntity( f, &g_entities[0] );

	// write out the client structure
	PersWriteClient( f, &level.clients[0] );

	// write out the cast_state structure
	PersWriteCastState( f, AICast_GetCastState( 0 ) );

	trap_FS_FCloseFile( f );

	return qtrue;
}

/*
===============
G_LoadPersistant
===============
*/
void G_LoadPersistant( void ) {
	fileHandle_t f;
	char *filename;
	char mapstr[MAX_QPATH];
	vmCvar_t cvar_mapname;
	int persid;

	filename = "save/current.psw";

	// open the file
	if ( trap_FS_FOpenFile( filename, &f, FS_READ ) < 0 ) {
		// not here, we shall assume they didn't want one
		return;
	}

	// read the mapname, if it's not the same, then ignore the file
	trap_FS_Read( mapstr, MAX_QPATH, f );
	trap_Cvar_Register( &cvar_mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
	if ( Q_strcasecmp( cvar_mapname.string, mapstr ) ) {
		trap_FS_FCloseFile( f );
		return;
	}

	// check the pers id
	trap_FS_Read( &persid, sizeof( persid ), f );
	if ( persid != trap_Cvar_VariableIntegerValue( "persid" ) ) {
		trap_FS_FCloseFile( f );
		return;
	}

	// read the entity structure
	PersReadEntity( f, &g_entities[0] );

	// read the client structure
	PersReadClient( f, &level.clients[0] );

	// read the cast_state structure
	PersReadCastState( f, AICast_GetCastState( 0 ) );

	trap_FS_FCloseFile( f );
}

#endif // 0
