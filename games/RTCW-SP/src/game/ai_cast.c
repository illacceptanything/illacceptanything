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
// Name:			ai_cast.c
// Function:		Wolfenstein AI Character Routines
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

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
The Wolfenstein AI uses the bot movement functions, and goal handling.

The actual core thinking and decision making is handled by the Cast AI,
typically within the ai_cast*.c files.

Some modifications to the botlib and botai are to be expected, the extent of those
changes is currently unknown.

Currently, this seems to the the best approach, since if we're going to
use the AAS for navigation, we want to avoid having to re-write the movement
routines which are heavily associated with the AAS information.
*/

//cast states (allocated at run-time)
cast_state_t    *caststates;
//number of characters
int numcast;
//
qboolean saveGamePending;
//
// minimum time between thinks (maximum is double this)
int aicast_thinktime;
// maximum number of character thinks at once
int aicast_maxthink;
// maximum clients
int aicast_maxclients;
// skill scale (0.0 -> 1.0)
float aicast_skillscale;

// cvar to enable aicast debugging, set higher for more levels of debugging
vmCvar_t aicast_debug;
vmCvar_t aicast_debugname;
vmCvar_t aicast_scripts;

// string versions of the attributes used for per-level, per-character definitions
char *castAttributeStrings[] =
{
	"RUNNING_SPEED", // max = 300	(running speed)
	"WALKING_SPEED", // max = 300	(walking speed)
	"CROUCHING_SPEED",   // max = 300	(crouching speed)
	"FOV",               // max = 360	(field of view)
	"YAW_SPEED",     // max = 300	(yaw speed)
	"LEADER",            // max = 1.0	(ability to lead an AI squadron)
	"AIM_SKILL",     // max = 1.0	(skill while aiming)
	"AIM_ACCURACY",      // max = 1.0	(accuracy of firing)
	"ATTACK_SKILL",      // max = 1.0	(ability to attack and do other things, like retreat)
	"REACTION_TIME", // max = 1.0	(upon seeing enemy, wait this long before reaction)
	"ATTACK_CROUCH", // max = 1.0	(likely to crouch while firing)
	"IDLE_CROUCH",       // max = 1.0	(likely to crouch while idling)
	"AGGRESSION",        // max = 1.0	(willingness to fight till the death)
	"TACTICAL",          // max = 1.0	(ability to use strategy to their advantage, also behaviour whilst hunting enemy, more likely to creep around)
	"CAMPER",            // max = 1.0	(set this to make them stay in the spot they are spawned)
	"ALERTNESS",     // max = 1.0	(ability to notice enemies at long range)
	"STARTING_HEALTH",
	"HEARING_SCALE",
	"HEARING_SCALE_NOT_PVS",
	"INNER_DETECTION_RADIUS",
	"PAIN_THRESHOLD_SCALE",

	NULL
};

/*
============
AICast_Printf
============
*/
void AICast_Printf( int type, const char *fmt, ... ) {
	char str[2048];
	va_list ap;

	va_start( ap, fmt );
	vsprintf( str, fmt, ap );
	va_end( ap );

	switch ( type ) {
	case AICAST_PRT_ALWAYS: {
		G_Printf( "%s", str );
		break;
	}
	default: {
		if ( aicast_debug.integer >= type ) {
			G_Printf( "%s", str );
		}
		break;
	}
	}
}

/*
============
AICast_GetCastState
============
*/
cast_state_t *AICast_GetCastState( int entitynum ) {
	if ( entitynum < 0 || entitynum > level.maxclients ) {
		return NULL;
	}
	//
	return &( caststates[ entitynum ] );
}

/*
==============
AICast_SetupClient
==============
*/
int AICast_SetupClient( int client ) {
	cast_state_t    *cs;
	bot_state_t     *bs;

	if ( !botstates[client] ) {
		botstates[client] = G_Alloc( sizeof( bot_state_t ) );
		memset( botstates[client], 0, sizeof( bot_state_t ) );
	}
	bs = botstates[client];

	if ( bs->inuse ) {
		BotAI_Print( PRT_FATAL, "client %d already setup\n", client );
		return qfalse;
	}

	cs = AICast_GetCastState( client );
	cs->bs = bs;

	//allocate a goal state
	bs->gs = trap_BotAllocGoalState( client );

	bs->inuse = qtrue;
	bs->client = client;
	bs->entitynum = client;
	bs->setupcount = qtrue;
	bs->entergame_time = trap_AAS_Time();
	bs->ms = trap_BotAllocMoveState();

	return qtrue;
}

/*
==============
AICast_ShutdownClient
==============
*/
int AICast_ShutdownClient( int client ) {
	cast_state_t    *cs;
	bot_state_t *bs;

	if ( !( bs = botstates[client] ) ) {
		return BLERR_NOERROR;
	}
	if ( !bs->inuse ) {
		BotAI_Print( PRT_ERROR, "client %d already shutdown\n", client );
		return BLERR_AICLIENTALREADYSHUTDOWN;
	}

	cs = AICast_GetCastState( client );
	//
	memset( cs, 0, sizeof( cast_state_t ) );
	numcast--;

	// now do the other bot stuff

#ifdef DEBUG
//	botai_import.DebugLineDelete(bs->debugline);
#endif //DEBUG

	trap_BotFreeMoveState( bs->ms );
	//free the goal state
	trap_BotFreeGoalState( bs->gs );
	//
	//clear the bot state
	memset( bs, 0, sizeof( bot_state_t ) );
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//everything went ok
	return BLERR_NOERROR;
}

/*
============
AICast_AddCastToGame
============
*/
//----(SA) modified this for head separation
gentity_t *AICast_AddCastToGame( gentity_t *ent, char *castname, char *model, char *head, char *sex, char *color, char *handicap ) {
	int clientNum;
	gentity_t *bot;
	char userinfo[MAX_INFO_STRING];
	usercmd_t cmd;

	// create the bot's userinfo
	userinfo[0] = '\0';

	Info_SetValueForKey( userinfo, "name", castname );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	Info_SetValueForKey( userinfo, "handicap", handicap );
	Info_SetValueForKey( userinfo, "model", model );
	Info_SetValueForKey( userinfo, "head", head );
	Info_SetValueForKey( userinfo, "color", color );

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient();
	if ( clientNum == -1 ) {
		G_Printf( S_COLOR_RED "BotAllocateClient failed\n" );
		return NULL;
	}
	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;
	bot->r.svFlags |= SVF_CASTAI;       // flag it for special Cast AI behaviour

	// register the userinfo
	trap_SetUserinfo( bot->s.number, userinfo );

	// have it connect to the game as a normal client
//----(SA) ClientConnect requires a third 'isbot' parameter.  setting to qfalse and noting
	ClientConnect( bot->s.number, qtrue, qfalse );
//----(SA) end

	// copy the origin/angles across
	VectorCopy( ent->s.origin, bot->s.origin );
	VectorCopy( ent->s.angles, bot->s.angles );

	memset( &cmd, 0, sizeof( cmd ) );
	ClientBegin( bot->s.number );

	// set up the ai
	AICast_SetupClient( bot->s.number );

	return bot;
}

/*
============
AICast_CheckLevelAttributes
============
*/
void AICast_CheckLevelAttributes( cast_state_t *cs, gentity_t *ent, char **ppStr ) {
	char    *s;
	int i;

	if ( !*ppStr ) {
		return;
	}

	while ( 1 ) {
		s = COM_Parse( ppStr );
		if ( !s[0] || !Q_strncmp( s, "}", 2 ) ) {    // end of attributes
			break;
		}
		//
		for ( i = 0; i < AICAST_MAX_ATTRIBUTES; i++ ) {
			if ( !Q_strcasecmp( s, castAttributeStrings[i] ) ) {
				// found a match, read in the value
				s = COM_Parse( ppStr );
				if ( !s[0] ) {    // end of attributes
					break;
				}
				// set the attribute
				cs->attributes[i] = atof( s );
				break;
			}
		}
	}
}

/*
============
AICast_SetAASIndex
============
*/
void AICast_SetAASIndex( cast_state_t *cs ) {
	if ( aiDefaults[cs->aiCharacter].bboxType == BBOX_SMALL ) {
		cs->aasWorldIndex = AASWORLD_STANDARD;
		cs->travelflags = AICAST_TFL_DEFAULT;
	} else if ( aiDefaults[cs->aiCharacter].bboxType == BBOX_LARGE ) {
		cs->aasWorldIndex = AASWORLD_LARGE;
		cs->travelflags = AICAST_TFL_DEFAULT & ~TFL_DONOTENTER_LARGE;
	} else {
		Com_Error( ERR_DROP, "AICast_SetAASIndex: unsupported bounds size (%i)", aiDefaults[cs->aiCharacter].bboxType );
	}

	if ( !cs->attributes[ATTACK_CROUCH] ) {
		cs->travelflags &= ~TFL_CROUCH;
	}
}

/*
============
AICast_CreateCharacter

  returns 0 if unable to create the character
============
*/
gentity_t *AICast_CreateCharacter( gentity_t *ent, float *attributes, cast_weapon_info_t *weaponInfo, char *castname, char *model, char *head, char *sex, char *color, char *handicap ) {
	gentity_t       *newent;
	gclient_t       *client;
	cast_state_t    *cs;
	char            **ppStr;
	int j;

	if ( g_gametype.integer != GT_SINGLE_PLAYER ) { // no cast AI in multiplayer
		return NULL;
	}
	// are bots enabled?
	if ( !trap_Cvar_VariableIntegerValue( "bot_enable" ) ) {
		G_Printf( S_COLOR_RED "ERROR: Unable to spawn %s, 'bot_enable' is not set\n", ent->classname );
		return NULL;
	}
	//
	// make sure we have a free slot for them
	//
	if ( level.numPlayingClients + 1 > aicast_maxclients ) {
		G_Error( "Exceeded sv_maxclients (%d), unable to create %s\n", aicast_maxclients, ent->classname );
		return NULL;
	}
	//
	// add it to the list (only do this if everything else passed)
	//

	newent = AICast_AddCastToGame( ent, castname, model, head, sex, color, handicap );

	if ( !newent ) {
		return NULL;
	}
	client = newent->client;
	//
	// setup the character..
	//
	cs = AICast_GetCastState( newent->s.number );
	//
	cs->aiCharacter = ent->aiCharacter;
	client->ps.aiChar = ent->aiCharacter;
	// setup the attributes
	memcpy( cs->attributes, attributes, sizeof( cs->attributes ) );
	ppStr = &ent->aiAttributes;
	AICast_CheckLevelAttributes( cs, ent, ppStr );
	//
	AICast_SetAASIndex( cs );
	// make sure they face the right direction
	VectorCopy( ent->s.angles, cs->ideal_viewangles );
	// factor in the delta_angles
	for ( j = 0; j < 3; j++ ) {
		cs->viewangles[j] = AngleMod( newent->s.angles[j] - SHORT2ANGLE( newent->client->ps.delta_angles[j] ) );
	}
	VectorCopy( ent->s.angles, newent->s.angles );
	VectorCopy( ent->s.origin, cs->startOrigin );
	//
	cs->lastEnemy = -1;
	cs->enemyNum = -1;
	cs->leaderNum = -1;
	cs->castScriptStatus.scriptGotoEnt = -1;
	//
	newent->aiName = ent->aiName;
	newent->aiTeam = ent->aiTeam;
	newent->targetname = ent->targetname;
	//
	newent->AIScript_AlertEntity = ent->AIScript_AlertEntity;
	newent->aiInactive = ent->aiInactive;
	newent->aiCharacter = cs->aiCharacter;
	//
	// parse the AI script for this character (if applicable)
	cs->aiFlags |= AIFL_CORPSESIGHTING;     // this is on by default for all characters, disabled if they have a "friendlysightcorpse" script event
	AICast_ScriptParse( cs );
	//
	// setup bounding boxes
	//VectorCopy( mins, client->ps.mins );
	//VectorCopy( maxs, client->ps.maxs );
	AIChar_SetBBox( newent, cs, qfalse );
	client->ps.friction = cs->attributes[RUNNING_SPEED] / 300.0;
	//
	// clear weapons/ammo
	client->ps.weapon = 0;
	memcpy( client->ps.weapons, weaponInfo->startingWeapons, sizeof( weaponInfo->startingWeapons ) );
	memcpy( client->ps.ammo, weaponInfo->startingAmmo, sizeof( client->ps.ammo ) );
	//
	// starting health
	if ( ent->health ) {
		newent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = ent->health;
	} else {
		newent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH] = cs->attributes[STARTING_HEALTH];
	}
	//
	cs->weaponInfo = weaponInfo;
	//
	cs->lastThink = level.time;
	//
	newent->pain = AICast_Pain;
	newent->die = AICast_Die;
	//
	//update the attack inventory values
	AICast_UpdateBattleInventory( cs, cs->enemyNum );

//----(SA)	make sure all clips are loaded so we don't hear everyone loading up
//			(we don't want to do this inside AICast_UpdateBattleInventory(), only on spawn or giveweapon)
	for ( j = 0; j < WP_NUM_WEAPONS; j++ ) {
		Fill_Clip( &client->ps, j );
	}
//----(SA)	end

	// select a weapon
	AICast_ChooseWeapon( cs, qfalse );

	//
	// set the default function, overwrite if necessary
	cs->aiFlags |= AIFL_JUST_SPAWNED;
	AIFunc_DefaultStart( cs );
	//
	numcast++;
	//
	return newent;
}

/*
============
AICast_Init

  called at each level start, before the world and it's entities have been spawned
============
*/
static int numSpawningCast;

void AICast_Init( void ) {
	vmCvar_t cvar;
	int i;

	numcast = 0;
	numSpawningCast = 0;
	saveGamePending = qtrue;

	trap_Cvar_Register( &aicast_debug, "aicast_debug", "0", 0 );
	trap_Cvar_Register( &aicast_debugname, "aicast_debugname", "", 0 );
	trap_Cvar_Register( &aicast_scripts, "aicast_scripts", "1", 0 );

	// (aicast_thinktime / sv_fps) * aicast_maxthink = number of cast's to think between each aicast frame
	// so..
	// (100 / 20) * 6 = 30
	//
	// so if the level has more than 30 AI cast's, they could start to bunch up, resulting in slower thinks

	trap_Cvar_Register( &cvar, "aicast_thinktime", "50", 0 );
	aicast_thinktime = trap_Cvar_VariableIntegerValue( "aicast_thinktime" );

	trap_Cvar_Register( &cvar, "aicast_maxthink", "4", 0 );
	aicast_maxthink = trap_Cvar_VariableIntegerValue( "aicast_maxthink" );

	aicast_maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );

	aicast_skillscale = (float)trap_Cvar_VariableIntegerValue( "g_gameSkill" ) / (float)GSKILL_MAX;

	caststates = G_Alloc( aicast_maxclients * sizeof( cast_state_t ) );
	memset( caststates, 0, sizeof( caststates ) );
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		caststates[i].entityNum = i;
	}

/* RF, this is useless, since the AAS hasnt been loaded yet
	// try and load in the AAS now, so we can interact with it during spawning of entities
	i = 0;
	trap_AAS_SetCurrentWorld(0);
	while (!trap_AAS_Initialized() && (i++ < 10)) {
		trap_BotLibStartFrame((float) level.time / 1000);
	}
*/
}

/*
===============
AICast_FindEntityForName
===============
*/
gentity_t *AICast_FindEntityForName( char *name ) {
	gentity_t *trav;
	int i;

	for ( trav = g_entities, i = 0; i < aicast_maxclients; i++, trav++ ) {
		if ( !trav->inuse ) {
			continue;
		}
		if ( !trav->client ) {
			continue;
		}
		if ( !trav->aiName ) {
			continue;
		}
		if ( strcmp( trav->aiName, name ) ) {
			continue;
		}
		return trav;
	}
	return NULL;
}

/*
===============
AICast_TravEntityForName
===============
*/
gentity_t *AICast_TravEntityForName( gentity_t *startent, char *name ) {
	gentity_t *trav;

	if ( !startent ) {
		trav = g_entities;
	} else {
		trav = startent + 1;
	}

	for ( ; trav < g_entities + aicast_maxclients; trav++ ) {
		if ( !trav->inuse ) {
			continue;
		}
		if ( !trav->client ) {
			continue;
		}
		if ( !trav->aiName ) {
			continue;
		}
		if ( strcmp( trav->aiName, name ) ) {
			continue;
		}
		return trav;
	}
	return NULL;
}

/*
============
AIChar_AIScript_AlertEntity

  triggered spawning, called from AI scripting
============
*/
void AIChar_AIScript_AlertEntity( gentity_t *ent ) {
	vec3_t mins, maxs;
	int numTouch, touch[10], i;
	cast_state_t    *cs;

	if ( !ent->aiInactive ) {
		return;
	}

	cs = AICast_GetCastState( ent->s.number );

	// if the current bounding box is invalid, then wait
	VectorAdd( ent->r.currentOrigin, ent->r.mins, mins );
	VectorAdd( ent->r.currentOrigin, ent->r.maxs, maxs );
	trap_UnlinkEntity( ent );

	numTouch = trap_EntitiesInBox( mins, maxs, touch, 10 );

	// check that another client isn't inside us
	if ( numTouch ) {
		for ( i = 0; i < numTouch; i++ ) {
			// RF, note we should only check against clients since zombies need to spawn inside func_explosive (so they dont clip into view after it explodes)
			if ( g_entities[touch[i]].client && g_entities[touch[i]].r.contents == CONTENTS_BODY ) {
				//if (g_entities[touch[i]].r.contents & MASK_PLAYERSOLID)
				break;
			}
		}
		if ( i == numTouch ) {
			numTouch = 0;
		}
	}

	if ( numTouch ) {
		// invalid location
		cs->aiFlags |= AIFL_WAITINGTOSPAWN;
		return;
	}

	// RF, has to disable this so I could test some maps which have erroneously placed alertentity calls
	//ent->AIScript_AlertEntity = NULL;
	cs->aiFlags &= ~AIFL_WAITINGTOSPAWN;
	ent->aiInactive = qfalse;
	trap_LinkEntity( ent );

	// trigger a spawn script event
	AICast_ScriptEvent( AICast_GetCastState( ent->s.number ), "spawn", "" );
	// make it think so we update animations/angles
	AICast_Think( ent->s.number, (float)FRAMETIME / 1000 );
	cs->lastThink = level.time;
	AICast_UpdateInput( cs, FRAMETIME );
	trap_BotUserCommand( cs->bs->client, &( cs->lastucmd ) );
}


/*
================
AICast_DelayedSpawnCast
================
*/
void AICast_DelayedSpawnCast( gentity_t *ent, int castType ) {
	int i;

	// ............................
	// head separation
	if ( !ent->aiSkin ) {
		G_SpawnString( "skin", "", &ent->aiSkin );
	}
	if ( !ent->aihSkin ) {
		G_SpawnString( "head", "default", &ent->aihSkin );
	}
	G_SpawnInt( "aiteam", "-1", &ent->aiTeam );
	// ............................


//----(SA)	make sure client registers the default weapons for this char
	for ( i = 0; aiDefaults[ent->aiCharacter].weapons[i]; i++ ) {
		RegisterItem( BG_FindItemForWeapon( aiDefaults[ent->aiCharacter].weapons[i] ) );
	}
//----(SA)	end

	// we have to wait a bit before spawning it, otherwise the server will just delete it, since it's treated like a client
	ent->think = AIChar_spawn;
	ent->nextthink = level.time + FRAMETIME * 4;  // have to wait more than 3 frames, since the server runs 3 frames before it clears all clients

	// we don't really want to start this character right away, but if we don't spawn the client
	// now, if the game gets saved after the character spawns in, when it gets re-loaded, the client
	// won't get spawned properly.
	if ( ent->spawnflags & 1 ) { // TriggerSpawn
		ent->AIScript_AlertEntity = AIChar_AIScript_AlertEntity;
		ent->aiInactive = qtrue;
	}

	// RF, had to move this down since some dev maps don't properly spawn the guys in, so we
	// get a crash when transitioning between levels after they all spawn at once (overloading
	// the client/server command buffers)
	ent->nextthink += FRAMETIME * ( ( numSpawningCast + 1 ) / 3 );    // space them out a bit so we don't overflow the client

	ent->aiCharacter = castType;
	numSpawningCast++;
}

/*
==================
AICast_CastScriptThink
==================
*/
void AICast_CastScriptThink( void ) {
	int i;
	gentity_t *ent;
	cast_state_t *cs;

	for ( i = 0, ent = g_entities, cs = caststates; i < level.maxclients; i++, ent++, cs++ ) {
		if ( !ent->inuse ) {
			continue;
		}
		if ( !cs->bs ) {
			continue;
		}
		if ( ent->health <= 0 ) {
			continue;
		}
		AICast_ScriptRun( cs, qfalse );
	}
}

/*
==================
AICast_EnableRenderingThink
==================
*/
void AICast_EnableRenderingThink( gentity_t *ent ) {
	trap_Cvar_Set( "cg_norender", "0" );
//		trap_S_FadeAllSound(1.0f, 1000);	// fade sound up
	G_FreeEntity( ent );
}

/*
==================
AICast_CheckLoadGame

  at the start of a level, the game is either saved, or loaded

  we must wait for all AI to spawn themselves, and a real client to connect
==================
*/
void AICast_CheckLoadGame( void ) {
	char loading[4];
	gentity_t *ent = NULL; // TTimo: VC6 'may be used without having been init'
	qboolean ready;
	cast_state_t *pcs;

	// have we already done the save or load?
	if ( !saveGamePending ) {
		return;
	}

	// tell the cgame NOT to render the scene while we are waiting for things to settle
	trap_Cvar_Set( "cg_norender", "1" );

	trap_Cvar_VariableStringBuffer( "savegame_loading", loading, sizeof( loading ) );

//	reloading = qtrue;
	trap_Cvar_Set( "g_reloading", "1" );

	if ( strlen( loading ) > 0 && atoi( loading ) != 0 ) {
		// screen should be black if we are at this stage
		trap_SetConfigstring( CS_SCREENFADE, va( "1 %i 1", level.time - 10 ) );

//		if (!reloading && atoi(loading) == 2) {
		if ( !( g_reloading.integer ) && atoi( loading ) == 2 ) {
			// (SA) hmm, this seems redundant when it sets it above...
//			reloading = qtrue;	// this gets reset at the Map_Restart() since the server unloads the game dll
			trap_Cvar_Set( "g_reloading", "1" );
		}

		ready = qtrue;
		if ( numSpawningCast != numcast ) {
			ready = qfalse;
		} else if ( !( ent = AICast_FindEntityForName( "player" ) ) ) {
			ready = qfalse;
		} else if ( !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
			ready = qfalse;
		}

		if ( ready ) {
			trap_Cvar_Set( "savegame_loading", "0" ); // in-case it aborts
			saveGamePending = qfalse;
			G_LoadGame( NULL );     // always load the "current" savegame

			// RF, spawn a thinker that will enable rendering after the client has had time to process the entities and setup the display
			//trap_Cvar_Set( "cg_norender", "0" );
			ent = G_Spawn();
			ent->nextthink = level.time + 200;
			ent->think = AICast_EnableRenderingThink;

			// wait for the clients to return from faded screen
			//trap_SetConfigstring( CS_SCREENFADE, va("0 %i 1500", level.time + 500) );
			trap_SetConfigstring( CS_SCREENFADE, va( "0 %i 750", level.time + 500 ) );
			level.reloadPauseTime = level.time + 1100;

			// make sure sound fades up
			trap_SendServerCommand( -1, va( "snd_fade 1 %d", 2000 ) );  //----(SA)	added

			AICast_CastScriptThink();
		}
	} else {

		ready = qtrue;
		if ( numSpawningCast != numcast ) {
			ready = qfalse;
		} else if ( !( ent = AICast_FindEntityForName( "player" ) ) ) {
			ready = qfalse;
		} else if ( !ent->client || ent->client->pers.connected != CON_CONNECTED ) {
			ready = qfalse;
		}

		// not loading a game, we must be in a new level, so look for some persistant data to read in, then save the game
		if ( ready ) {
			G_LoadPersistant();     // make sure we save the game after we have brought across the items

			trap_Cvar_Set( "g_totalPlayTime", "0" );  // reset play time
			trap_Cvar_Set( "g_attempts", "0" );
			pcs = AICast_GetCastState( ent->s.number );
			pcs->totalPlayTime = 0;
			pcs->lastLoadTime = 0;
			pcs->attempts = 0;

			// RF, disabled, since the pregame menu turns this off after the button is pressed, this isn't
			// required here
			// RF, spawn a thinker that will enable rendering after the client has had time to process the entities and setup the display
			//trap_Cvar_Set( "cg_norender", "0" );
			//ent = G_Spawn();
			//ent->nextthink = level.time + 200;
			//ent->think = AICast_EnableRenderingThink;

			saveGamePending = qfalse;

			// wait for the clients to return from faded screen
//			trap_SetConfigstring( CS_SCREENFADE, va("0 %i 1500", level.time + 500) );
//			trap_SetConfigstring( CS_SCREENFADE, va("0 %i 750", level.time + 500) );
			// (SA) send a command that will be interpreted for both the screenfade and any other effects (music cues, pregame menu, etc)

// briefing menu will handle transition, just set a cvar for it to check for drawing the 'continue' button
			trap_SendServerCommand( -1, "rockandroll\n" );

			level.reloadPauseTime = level.time + 1100;

			AICast_CastScriptThink();
		}
	}
}

/*
===============
AICast_SolidsInBBox
===============
*/
qboolean AICast_SolidsInBBox( vec3_t pos, vec3_t mins, vec3_t maxs, int entnum, int mask ) {
	trace_t tr;

	if ( g_entities[entnum].health <= 0 ) {
		return qfalse;
	}

	trap_Trace( &tr, pos, mins, maxs, pos, entnum, mask );
	if ( tr.startsolid || tr.allsolid ) {
		return qtrue;
	} else {
		return qfalse;
	}
}

/*
===============
AICast_Activate
===============
*/
void AICast_Activate( int activatorNum, int entNum ) {
	cast_state_t *cs;

	cs = AICast_GetCastState( entNum );
	if ( cs->activate ) {
		cs->activate( entNum, activatorNum );
	}

	AICast_Printf( AICAST_PRT_DEBUG, "activated entity # %i\n", entNum );
}

/*
================
AICast_NoFlameDamage
================
*/
qboolean AICast_NoFlameDamage( int entNum ) {
	cast_state_t *cs;

	if ( entNum >= MAX_CLIENTS ) {
		return qfalse;
	}

	// DHM - Nerve :: Not in multiplayer
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		return qfalse;
	}

	cs = AICast_GetCastState( entNum );
	return ( ( cs->aiFlags & AIFL_NO_FLAME_DAMAGE ) != 0 );
}

/*
================
AICast_SetFlameDamage
================
*/
void AICast_SetFlameDamage( int entNum, qboolean status ) {
	cast_state_t *cs;

	if ( entNum >= MAX_CLIENTS ) {
		return;
	}

	// DHM - Nerve :: Not in multiplayer
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		return;
	}

	cs = AICast_GetCastState( entNum );

	if ( status ) {
		cs->aiFlags |= AIFL_NO_FLAME_DAMAGE;
	} else {
		cs->aiFlags &= ~AIFL_NO_FLAME_DAMAGE;
	}
}

/*
===============
G_SetAASBlockingEntity

  Adjusts routing so AI knows it can't move through this entity
===============
*/
void G_SetAASBlockingEntity( gentity_t *ent, qboolean blocking ) {
	ent->AASblocking = blocking;
	trap_AAS_SetAASBlockingEntity( ent->r.absmin, ent->r.absmax, blocking );
}

/*
===============
AICast_AdjustIdealYawForMover
===============
*/
void AICast_AdjustIdealYawForMover( int entnum, float yaw ) {
	cast_state_t *cs = AICast_GetCastState( entnum );
	//
	cs->ideal_viewangles[YAW] += yaw;
}

/*
===============
AICast_AgePlayTime
===============
*/
void AICast_AgePlayTime( int entnum ) {
	cast_state_t *cs = AICast_GetCastState( entnum );
	//
	if ( saveGamePending ) {
		return;
	}
//	if (reloading)
	if ( g_reloading.integer ) {
		return;
	}
	//
	if ( ( level.time - cs->lastLoadTime ) > 1000 ) {
		if ( /*(level.time - cs->lastLoadTime) < 2000 &&*/ ( level.time - cs->lastLoadTime ) > 0 ) {
			cs->totalPlayTime += level.time - cs->lastLoadTime;
			trap_Cvar_Set( "g_totalPlayTime", va( "%i", cs->totalPlayTime ) );
		}
		//
		cs->lastLoadTime = level.time;
	}
}

/*
===============
AICast_NoReload
===============
*/
int AICast_NoReload( int entnum ) {
	cast_state_t *cs = AICast_GetCastState( entnum );
	//
	return ( ( cs->aiFlags & AIFL_NO_RELOAD ) != 0 );
}


/*
==============
AICast_PlayTime
==============
*/
int AICast_PlayTime( int entnum ) {
	cast_state_t *cs = AICast_GetCastState( entnum );
	return ( cs->totalPlayTime );
}

/*
==============
AICast_NumAttempts
==============
*/
int AICast_NumAttempts( int entnum ) {
	cast_state_t *cs = AICast_GetCastState( entnum );
	return ( cs->attempts );
}

void AICast_RegisterPain( int entnum ) {
	cast_state_t *cs = AICast_GetCastState( entnum );
	if ( cs ) {
		cs->lastPain = level.time;
	}
}