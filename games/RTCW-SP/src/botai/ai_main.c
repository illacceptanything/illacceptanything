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
 * name:		ai_main.c
 *
 * desc:		Quake3 bot AI
 *
 *
 *****************************************************************************/

#include "../game/g_local.h"
#include "../game/q_shared.h"
#include "../game/botlib.h"      //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"
#include "../botai/botai.h"          //bot ai interface

#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
//
#include "chars.h"
#include "inv.h"
#include "syn.h"

#define MAX_PATH        144

//bot states
bot_state_t *botstates[MAX_CLIENTS];
//number of bots
int numbots;
//time to do a regular update
float regularupdate_time;
//
vmCvar_t bot_thinktime;
vmCvar_t memorydump;


/*
==================
BotAI_Print
==================
*/
void QDECL BotAI_Print( int type, char *fmt, ... ) {
	char str[2048];
	va_list ap;

	va_start( ap, fmt );
	vsprintf( str, fmt, ap );
	va_end( ap );

	switch ( type ) {
	case PRT_MESSAGE: {
		G_Printf( "%s", str );
		break;
	}
	case PRT_WARNING: {
		G_Printf( S_COLOR_YELLOW "Warning: %s", str );
		break;
	}
	case PRT_ERROR: {
		G_Printf( S_COLOR_RED "Error: %s", str );
		break;
	}
	case PRT_FATAL: {
		G_Printf( S_COLOR_RED "Fatal: %s", str );
		break;
	}
	case PRT_EXIT: {
		G_Error( S_COLOR_RED "Exit: %s", str );
		break;
	}
	default: {
		G_Printf( "unknown print type\n" );
		break;
	}
	}
}

/*
==================
BotAI_Trace
==================
*/
void BotAI_Trace( bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask ) {
	trace_t trace;

	trap_Trace( &trace, start, mins, maxs, end, passent, contentmask );
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy( trace.endpos, bsptrace->endpos );
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy( trace.plane.normal, bsptrace->plane.normal );
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotAI_GetClientState
==================
*/
int BotAI_GetClientState( int clientNum, playerState_t *state ) {
	gentity_t   *ent;

	ent = &g_entities[clientNum];
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->client ) {
		return qfalse;
	}

	memcpy( state, &ent->client->ps, sizeof( playerState_t ) );
	return qtrue;
}

/*
==================
BotAI_GetEntityState
==================
*/
int BotAI_GetEntityState( int entityNum, entityState_t *state ) {
	gentity_t   *ent;

	ent = &g_entities[entityNum];
	memset( state, 0, sizeof( entityState_t ) );
	if ( !ent->inuse ) {
		return qfalse;
	}
	if ( !ent->r.linked ) {
		return qfalse;
	}
	if ( ent->r.svFlags & SVF_NOCLIENT ) {
		return qfalse;
	}
	memcpy( state, &ent->s, sizeof( entityState_t ) );
	return qtrue;
}

/*
==================
BotAI_GetSnapshotEntity
==================
*/
int BotAI_GetSnapshotEntity( int clientNum, int sequence, entityState_t *state ) {
	int entNum;

	entNum = trap_BotGetSnapshotEntity( clientNum, sequence );
	if ( entNum == -1 ) {
		memset( state, 0, sizeof( entityState_t ) );
		return -1;
	}

	BotAI_GetEntityState( entNum, state );

	return sequence + 1;
}

/*
==================
BotAI_BotInitialChat
==================
*/
void QDECL BotAI_BotInitialChat( bot_state_t *bs, char *type, ... ) {
	int i, mcontext;
	va_list ap;
	char    *p;
	char    *vars[MAX_MATCHVARIABLES];

	memset( vars, 0, sizeof( vars ) );
	va_start( ap, type );
	p = va_arg( ap, char * );
	for ( i = 0; i < MAX_MATCHVARIABLES; i++ ) {
		if ( !p ) {
			break;
		}
		vars[i] = p;
		p = va_arg( ap, char * );
	}
	va_end( ap );

	mcontext = CONTEXT_NORMAL | CONTEXT_NEARBYITEM | CONTEXT_NAMES;
	if ( BotCTFTeam( bs ) == CTF_TEAM_RED ) {
		mcontext |= CONTEXT_CTFREDTEAM;
	} else { mcontext |= CONTEXT_CTFBLUETEAM;}

	trap_BotInitialChat( bs->cs, type, mcontext, vars[0], vars[1], vars[2], vars[3], vars[4], vars[5], vars[6], vars[7] );
}

/*
==============
BotInterbreeding
==============
*/
void BotInterbreeding( void ) {
	float ranks[MAX_CLIENTS];
	int parent1, parent2, child;
	int i;

	// get rankings for all the bots
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( botstates[i] && botstates[i]->inuse ) {
			ranks[i] = botstates[i]->num_kills * 2 - botstates[i]->num_deaths;
		} else {
			ranks[i] = -1;
		}
	}

	if ( trap_GeneticParentsAndChildSelection( MAX_CLIENTS, ranks, &parent1, &parent2, &child ) ) {
		trap_BotInterbreedGoalFuzzyLogic( botstates[parent1]->gs, botstates[parent2]->gs, botstates[child]->gs );
		trap_BotMutateGoalFuzzyLogic( botstates[child]->gs, 1 );
	}
	// reset the kills and deaths
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( botstates[i] && botstates[i]->inuse ) {
			botstates[i]->num_kills = 0;
			botstates[i]->num_deaths = 0;
		}
	}
}

/*
==============
BotEntityInfo
==============
*/
void BotEntityInfo( int entnum, aas_entityinfo_t *info ) {
	trap_AAS_EntityInfo( entnum, info );
}

/*
==============
NumBots
==============
*/
int NumBots( void ) {
	return numbots;
}

/*
==============
AngleDifference
==============
*/
float AngleDifference( float ang1, float ang2 ) {
	float diff;

	diff = ang1 - ang2;
	if ( ang1 > ang2 ) {
		if ( diff > 180.0 ) {
			diff -= 360.0;
		}
	} else {
		if ( diff < -180.0 ) {
			diff += 360.0;
		}
	}
	return diff;
}

/*
==============
BotChangeViewAngle
==============
*/
float BotChangeViewAngle( float angle, float ideal_angle, float speed ) {
	float move;

	angle = AngleMod( angle );
	ideal_angle = AngleMod( ideal_angle );
	if ( angle == ideal_angle ) {
		return angle;
	}
	move = ideal_angle - angle;
	if ( ideal_angle > angle ) {
		if ( move > 180.0 ) {
			move -= 360.0;
		}
	} else {
		if ( move < -180.0 ) {
			move += 360.0;
		}
	}
	if ( move > 0 ) {
		if ( move > speed ) {
			move = speed;
		}
	} else {
		if ( move < -speed ) {
			move = -speed;
		}
	}
	return AngleMod( angle + move );
}

/*
==============
BotChangeViewAngles
==============
*/
void BotChangeViewAngles( bot_state_t *bs, float thinktime ) {
	float diff, factor, maxchange, anglespeed;
	int i;

	if ( bs->ideal_viewangles[PITCH] > 180 ) {
		bs->ideal_viewangles[PITCH] -= 360;
	}
	//
	if ( bs->enemy >= 0 ) {
		factor = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_VIEW_FACTOR, 0.01, 1 );
		maxchange = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_VIEW_MAXCHANGE, 1, 1800 );
	} else {
		factor = 0.25;
		maxchange = 300;
	}
	maxchange *= thinktime;
	for ( i = 0; i < 2; i++ ) {
		diff = abs( AngleDifference( bs->viewangles[i], bs->ideal_viewangles[i] ) );
		anglespeed = diff * factor;
		if ( anglespeed > maxchange ) {
			anglespeed = maxchange;
		}
		bs->viewangles[i] = BotChangeViewAngle( bs->viewangles[i],
												bs->ideal_viewangles[i], anglespeed );
		//BotAI_Print(PRT_MESSAGE, "ideal_angles %f %f\n", bs->ideal_viewangles[0], bs->ideal_viewangles[1], bs->ideal_viewangles[2]);`
		//bs->viewangles[i] = bs->ideal_viewangles[i];
	}
	if ( bs->viewangles[PITCH] > 180 ) {
		bs->viewangles[PITCH] -= 360;
	}
	//elementary action: view
	trap_EA_View( bs->client, bs->viewangles );
}

/*
==============
BotInputToUserCommand
==============
*/
void BotInputToUserCommand( bot_input_t *bi, usercmd_t *ucmd, int delta_angles[3], int time ) {
	vec3_t angles, forward, right;
	short temp;
	int j;

	//clear the whole structure
	memset( ucmd, 0, sizeof( usercmd_t ) );
	//
	//Com_Printf("dir = %f %f %f speed = %f\n", bi->dir[0], bi->dir[1], bi->dir[2], bi->speed);
	//the duration for the user command in milli seconds
	ucmd->serverTime = time;
	//
	if ( bi->actionflags & ACTION_DELAYEDJUMP ) {
		bi->actionflags |= ACTION_JUMP;
		bi->actionflags &= ~ACTION_DELAYEDJUMP;
	}
	//set the buttons
	if ( bi->actionflags & ACTION_RESPAWN ) {
		ucmd->buttons = BUTTON_ATTACK;
	}
	if ( bi->actionflags & ACTION_ATTACK ) {
		ucmd->buttons |= BUTTON_ATTACK;
	}
	if ( bi->actionflags & ACTION_TALK ) {
		ucmd->buttons |= BUTTON_TALK;
	}
	if ( bi->actionflags & ACTION_GESTURE ) {
		ucmd->buttons |= BUTTON_GESTURE;
	}
	if ( bi->actionflags & ACTION_USE ) {
		ucmd->buttons |= BUTTON_USE_HOLDABLE;
	}
	if ( bi->actionflags & ACTION_WALK ) {
		ucmd->buttons |= BUTTON_WALKING;
	}
	ucmd->weapon = bi->weapon;
	//set the view angles
	//NOTE: the ucmd->angles are the angles WITHOUT the delta angles
	ucmd->angles[PITCH] = ANGLE2SHORT( bi->viewangles[PITCH] );
	ucmd->angles[YAW] = ANGLE2SHORT( bi->viewangles[YAW] );
	ucmd->angles[ROLL] = ANGLE2SHORT( bi->viewangles[ROLL] );
	//subtract the delta angles
	for ( j = 0; j < 3; j++ ) {
		temp = ucmd->angles[j] - delta_angles[j];
		/*NOTE: disabled because temp should be mod first
		if ( j == PITCH ) {
			// don't let the player look up or down more than 90 degrees
			if ( temp > 16000 ) temp = 16000;
			else if ( temp < -16000 ) temp = -16000;
		}
		*/
		ucmd->angles[j] = temp;
	}
	//NOTE: movement is relative to the REAL view angles
	//get the horizontal forward and right vector
	//get the pitch in the range [-180, 180]
	if ( bi->dir[2] ) {
		angles[PITCH] = bi->viewangles[PITCH];
	} else { angles[PITCH] = 0;}
	angles[YAW] = bi->viewangles[YAW];
	angles[ROLL] = 0;
	AngleVectors( angles, forward, right, NULL );
	//bot input speed is in the range [0, 400]
	bi->speed = bi->speed * 127 / 400;
	//set the view independent movement
	ucmd->forwardmove = DotProduct( forward, bi->dir ) * bi->speed;
	ucmd->rightmove = DotProduct( right, bi->dir ) * bi->speed;
	ucmd->upmove = abs( forward[2] ) * bi->dir[2] * bi->speed;
	//normal keyboard movement
	if ( bi->actionflags & ACTION_MOVEFORWARD ) {
		ucmd->forwardmove += 127;
	}
	if ( bi->actionflags & ACTION_MOVEBACK ) {
		ucmd->forwardmove -= 127;
	}
	if ( bi->actionflags & ACTION_MOVELEFT ) {
		ucmd->rightmove -= 127;
	}
	if ( bi->actionflags & ACTION_MOVERIGHT ) {
		ucmd->rightmove += 127;
	}
	//jump/moveup
	if ( bi->actionflags & ACTION_JUMP ) {
		ucmd->upmove += 127;
	}
	//crouch/movedown
	if ( bi->actionflags & ACTION_CROUCH ) {
		ucmd->upmove -= 127;
	}
	//
	//Com_Printf("forward = %d right = %d up = %d\n", ucmd.forwardmove, ucmd.rightmove, ucmd.upmove);
	//Com_Printf("ucmd->serverTime = %d\n", ucmd->serverTime);
}

/*
==============
BotUpdateInput
==============
*/
void BotUpdateInput( bot_state_t *bs, int time ) {
	bot_input_t bi;
	int j;

	//add the delta angles to the bot's current view angles
	for ( j = 0; j < 3; j++ ) {
		bs->viewangles[j] = AngleMod( bs->viewangles[j] + SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}
	//
	BotChangeViewAngles( bs, (float) time / 1000 );
	trap_EA_GetInput( bs->client, (float) time / 1000, &bi );
	//respawn hack
	if ( bi.actionflags & ACTION_RESPAWN ) {
		if ( bs->lastucmd.buttons & BUTTON_ATTACK ) {
			bi.actionflags &= ~( ACTION_RESPAWN | ACTION_ATTACK );
		}
	}
	//
	BotInputToUserCommand( &bi, &bs->lastucmd, bs->cur_ps.delta_angles, time );
	bs->lastucmd.serverTime = time;
	//subtract the delta angles
	for ( j = 0; j < 3; j++ ) {
		bs->viewangles[j] = AngleMod( bs->viewangles[j] - SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}
}

/*
==============
BotAIRegularUpdate
==============
*/
void BotAIRegularUpdate( void ) {
	if ( regularupdate_time < trap_AAS_Time() ) {
		trap_BotUpdateEntityItems();
		regularupdate_time = trap_AAS_Time() + 1;
	}
}

/*
==============
BotAI
==============
*/
int BotAI( int client, float thinktime ) {
	bot_state_t *bs;
	char buf[1024], *args;
	int j;

	trap_EA_ResetInput( client, NULL );
	//
	bs = botstates[client];
	if ( !bs || !bs->inuse ) {
		BotAI_Print( PRT_FATAL, "client %d hasn't been setup\n", client );
		return BLERR_AICLIENTNOTSETUP;
	}

	//retrieve the current client state
	BotAI_GetClientState( client, &bs->cur_ps );

	//retrieve any waiting console messages
	while ( trap_BotGetServerCommand( client, buf, sizeof( buf ) ) ) {
		//have buf point to the command and args to the command arguments
		args = strchr( buf, ' ' );
		if ( !args ) {
			continue;
		}
		*args++ = '\0';

		//remove color espace sequences from the arguments
		Q_CleanStr( args );

		//botai_import.Print(PRT_MESSAGE, "ConsoleMessage: \"%s\"\n", buf);
		if ( !Q_stricmp( buf, "cp " ) ) { /*CenterPrintf*/
		} else if ( !Q_stricmp( buf, "cs" ) )                                                      { /*ConfigStringModified*/
		} else if ( !Q_stricmp( buf, "print" ) )                                                                                                                       {
			trap_BotQueueConsoleMessage( bs->cs, CMS_NORMAL, args );
		} else if ( !Q_stricmp( buf, "chat" ) ) {
			trap_BotQueueConsoleMessage( bs->cs, CMS_CHAT, args );
		} else if ( !Q_stricmp( buf, "tchat" ) ) {
			trap_BotQueueConsoleMessage( bs->cs, CMS_CHAT, args );
		} else if ( !Q_stricmp( buf, "scores" ) ) { /*FIXME: parse scores?*/
		} else if ( !Q_stricmp( buf, "clientLevelShot" ) )                                                                    { /*ignore*/
		}
	}
	//add the delta angles to the bot's current view angles
	for ( j = 0; j < 3; j++ ) {
		bs->viewangles[j] = AngleMod( bs->viewangles[j] + SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}
	//increase the local time of the bot
	bs->ltime += thinktime;
	//
	bs->thinktime = thinktime;
	//origin of the bot
	VectorCopy( bs->cur_ps.origin, bs->origin );
	//eye coordinates of the bot
	VectorCopy( bs->cur_ps.origin, bs->eye );
	bs->eye[2] += bs->cur_ps.viewheight;
	//get the area the bot is in
	bs->areanum = BotPointAreaNum( bs->origin );
	//the real AI
	BotDeathmatchAI( bs, thinktime );
	//set the weapon selection every AI frame
	trap_EA_SelectWeapon( bs->client, bs->weaponnum );
	//subtract the delta angles
	for ( j = 0; j < 3; j++ ) {
		bs->viewangles[j] = AngleMod( bs->viewangles[j] - SHORT2ANGLE( bs->cur_ps.delta_angles[j] ) );
	}
	//everything was ok
	return BLERR_NOERROR;
}

/*
==================
BotScheduleBotThink
==================
*/
void BotScheduleBotThink( void ) {
	int i, botnum;

	botnum = 0;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		//initialize the bot think residual time
		botstates[i]->botthink_residual = bot_thinktime.integer * botnum / numbots;
		botnum++;
	}
}

/*
==============
BotAISetupClient
==============
*/
int BotAISetupClient( int client, struct bot_settings_s *settings ) {
	char filename[MAX_PATH], name[MAX_PATH], gender[MAX_PATH];
	bot_state_t *bs;
	int errnum;

	if ( !botstates[client] ) {
		botstates[client] = G_Alloc( sizeof( bot_state_t ) );
	}
	bs = botstates[client];

	if ( bs && bs->inuse ) {
		BotAI_Print( PRT_FATAL, "client %d already setup\n", client );
		return qfalse;
	}

	if ( !trap_AAS_Initialized() ) {
		BotAI_Print( PRT_FATAL, "AAS not initialized\n" );
		return qfalse;
	}

	//load the bot character
	bs->character = trap_BotLoadCharacter( settings->characterfile, settings->skill );
	if ( !bs->character ) {
		BotAI_Print( PRT_FATAL, "couldn't load skill %d from %s\n", settings->skill, settings->characterfile );
		return qfalse;
	}
	//copy the settings
	memcpy( &bs->settings, settings, sizeof( bot_settings_t ) );
	//allocate a goal state
	bs->gs = trap_BotAllocGoalState( client );
	//load the item weights
	trap_Characteristic_String( bs->character, CHARACTERISTIC_ITEMWEIGHTS, filename, MAX_PATH );
	errnum = trap_BotLoadItemWeights( bs->gs, filename );
	if ( errnum != BLERR_NOERROR ) {
		trap_BotFreeGoalState( bs->gs );
		return qfalse;
	}
	//allocate a weapon state
	bs->ws = trap_BotAllocWeaponState();
	//load the weapon weights
	trap_Characteristic_String( bs->character, CHARACTERISTIC_WEAPONWEIGHTS, filename, MAX_PATH );
	errnum = trap_BotLoadWeaponWeights( bs->ws, filename );
	if ( errnum != BLERR_NOERROR ) {
		trap_BotFreeGoalState( bs->gs );
		trap_BotFreeWeaponState( bs->ws );
		return qfalse;
	}
	//allocate a chat state
	bs->cs = trap_BotAllocChatState();
	//load the chat file
	trap_Characteristic_String( bs->character, CHARACTERISTIC_CHAT_FILE, filename, MAX_PATH );
	trap_Characteristic_String( bs->character, CHARACTERISTIC_CHAT_NAME, name, MAX_PATH );
	errnum = trap_BotLoadChatFile( bs->cs, filename, name );
	if ( errnum != BLERR_NOERROR ) {
		trap_BotFreeChatState( bs->cs );
		trap_BotFreeGoalState( bs->gs );
		trap_BotFreeWeaponState( bs->ws );
		return qfalse;
	}
	//get the gender characteristic
	trap_Characteristic_String( bs->character, CHARACTERISTIC_GENDER, gender, MAX_PATH );
	//set the chat gender
	if ( *gender == 'f' || *gender == 'F' ) {
		trap_BotSetChatGender( bs->cs, CHAT_GENDERFEMALE );
	} else if ( *gender == 'm' || *gender == 'M' )  {
		trap_BotSetChatGender( bs->cs, CHAT_GENDERMALE );
	} else { trap_BotSetChatGender( bs->cs, CHAT_GENDERLESS );}

	bs->inuse = qtrue;
	bs->client = client;
	bs->entitynum = client;
	bs->setupcount = 4;
	bs->entergame_time = trap_AAS_Time();
	bs->ms = trap_BotAllocMoveState();
	bs->walker = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_WALKER, 0, 1 );
	numbots++;

	if ( trap_Cvar_VariableIntegerValue( "bot_testichat" ) ) {
		trap_BotLibVarSet( "bot_testichat", "1" );
		BotChatTest( bs );
	}
	//NOTE: reschedule the bot thinking
	BotScheduleBotThink();
	//
	return qtrue;
}

/*
==============
BotAIShutdownClient
==============
*/
int BotAIShutdownClient( int client ) {
	bot_state_t *bs;

	// Wolfenstein
	if ( g_entities[client].r.svFlags & SVF_CASTAI ) {
		AICast_ShutdownClient( client );
		return BLERR_NOERROR;
	}
	// done.

	bs = botstates[client];
	if ( !bs || !bs->inuse ) {
		// BotAI_Print(PRT_ERROR, "client %d already shutdown\n", client);
		return BLERR_AICLIENTALREADYSHUTDOWN;
	}

	if ( BotChat_ExitGame( bs ) ) {
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}

	trap_BotFreeMoveState( bs->ms );
	//free the goal state
	trap_BotFreeGoalState( bs->gs );
	//free the chat file
	trap_BotFreeChatState( bs->cs );
	//free the weapon weights
	trap_BotFreeWeaponState( bs->ws );
	//free the bot character
	trap_BotFreeCharacter( bs->character );
	//
	BotFreeWaypoints( bs->checkpoints );
	BotFreeWaypoints( bs->patrolpoints );
	//clear the bot state
	memset( bs, 0, sizeof( bot_state_t ) );
	//set the inuse flag to qfalse
	bs->inuse = qfalse;
	//there's one bot less
	numbots--;
	//everything went ok
	return BLERR_NOERROR;
}

/*
==============
BotResetState

called when a bot enters the intermission or observer mode and
when the level is changed
==============
*/
void BotResetState( bot_state_t *bs ) {
	int client, entitynum, inuse;
	int movestate, goalstate, chatstate, weaponstate;
	bot_settings_t settings;
	int character;
	playerState_t ps;                           //current player state
	float entergame_time;

	//save some things that should not be reset here
	memcpy( &settings, &bs->settings, sizeof( bot_settings_t ) );
	memcpy( &ps, &bs->cur_ps, sizeof( playerState_t ) );
	inuse = bs->inuse;
	client = bs->client;
	entitynum = bs->entitynum;
	character = bs->character;
	movestate = bs->ms;
	goalstate = bs->gs;
	chatstate = bs->cs;
	weaponstate = bs->ws;
	entergame_time = bs->entergame_time;
	//free checkpoints and patrol points
	BotFreeWaypoints( bs->checkpoints );
	BotFreeWaypoints( bs->patrolpoints );
	//reset the whole state
	memset( bs, 0, sizeof( bot_state_t ) );
	//copy back some state stuff that should not be reset
	bs->ms = movestate;
	bs->gs = goalstate;
	bs->cs = chatstate;
	bs->ws = weaponstate;
	memcpy( &bs->cur_ps, &ps, sizeof( playerState_t ) );
	memcpy( &bs->settings, &settings, sizeof( bot_settings_t ) );
	bs->inuse = inuse;
	bs->client = client;
	bs->entitynum = entitynum;
	bs->character = character;
	bs->entergame_time = entergame_time;
	//reset several states
	if ( bs->ms ) {
		trap_BotResetMoveState( bs->ms );
	}
	if ( bs->gs ) {
		trap_BotResetGoalState( bs->gs );
	}
	if ( bs->ws ) {
		trap_BotResetWeaponState( bs->ws );
	}
	if ( bs->gs ) {
		trap_BotResetAvoidGoals( bs->gs );
	}
	if ( bs->ms ) {
		trap_BotResetAvoidReach( bs->ms );
	}
}

/*
==============
BotAILoadMap
==============
*/
int BotAILoadMap( int restart ) {
	int i;
	vmCvar_t mapname;

	if ( !restart ) {
		trap_Cvar_Register( &mapname, "mapname", "", CVAR_SERVERINFO | CVAR_ROM );
		trap_BotLibLoadMap( mapname.string );
	}

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( botstates[i] && botstates[i]->inuse ) {
			BotResetState( botstates[i] );
			botstates[i]->setupcount = 4;
		}
	}

	BotSetupDeathmatchAI();

	return BLERR_NOERROR;
}

/*
==================
BotAIStartFrame
==================
*/
int BotAIStartFrame( int time ) {
	int i;
	gentity_t   *ent;
	bot_entitystate_t state;
	//entityState_t entitystate;
	//vec3_t mins = {-15, -15, -24}, maxs = {15, 15, 32};
	int elapsed_time, thinktime;
	static int local_time;
	static int botlib_residual;
	static int lastbotthink_time;

	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		G_CheckBotSpawn();
	}

	trap_Cvar_Update( &bot_rocketjump );
	trap_Cvar_Update( &bot_grapple );
	trap_Cvar_Update( &bot_fastchat );
	trap_Cvar_Update( &bot_nochat );
	trap_Cvar_Update( &bot_testrchat );
	trap_Cvar_Update( &bot_thinktime );
	// Ridah, set the default AAS world
	trap_AAS_SetCurrentWorld( 0 );
	trap_Cvar_Update( &memorydump );

	if ( memorydump.integer ) {
		trap_BotLibVarSet( "memorydump", "1" );
		trap_Cvar_Set( "memorydump", "0" );
	}

	//if the bot think time changed we should reschedule the bots
	if ( bot_thinktime.integer != lastbotthink_time ) {
		lastbotthink_time = bot_thinktime.integer;
		BotScheduleBotThink();
	}

	elapsed_time = time - local_time;
	local_time = time;

	botlib_residual += elapsed_time;

	if ( elapsed_time > bot_thinktime.integer ) {
		thinktime = elapsed_time;
	} else { thinktime = bot_thinktime.integer;}

	// update the bot library
	if ( botlib_residual >= thinktime ) {
		botlib_residual -= thinktime;

		trap_BotLibStartFrame( (float) time / 1000 );

		// Ridah, only check the default world
		trap_AAS_SetCurrentWorld( 0 );

		if ( !trap_AAS_Initialized() ) {
			return BLERR_NOERROR;
		}

		//update entities in the botlib
		for ( i = 0; i < MAX_GENTITIES; i++ ) {

			// Ridah, in single player, we only need client entity information
			if ( g_gametype.integer == GT_SINGLE_PLAYER && i > level.maxclients ) {
				break;
			}

			ent = &g_entities[i];
			if ( !ent->inuse ) {
				continue;
			}
			if ( !ent->r.linked ) {
				continue;
			}
			if ( ent->r.svFlags & SVF_NOCLIENT ) {
				continue;
			}
			if ( ent->aiInactive ) {
				continue;
			}
			//
			memset( &state, 0, sizeof( bot_entitystate_t ) );
			//
			VectorCopy( ent->r.currentOrigin, state.origin );
			VectorCopy( ent->r.currentAngles, state.angles );
			VectorCopy( ent->s.origin2, state.old_origin );
			VectorCopy( ent->r.mins, state.mins );
			VectorCopy( ent->r.maxs, state.maxs );
			state.type = ent->s.eType;
			state.flags = ent->s.eFlags;
			if ( ent->r.bmodel ) {
				state.solid = SOLID_BSP;
			} else { state.solid = SOLID_BBOX;}
			state.groundent = ent->s.groundEntityNum;
			state.modelindex = ent->s.modelindex;
			state.modelindex2 = ent->s.modelindex2;
			state.frame = ent->s.frame;
			//state.event = ent->s.event;
			//state.eventParm = ent->s.eventParm;
			state.powerups = ent->s.powerups;
			state.legsAnim = ent->s.legsAnim;
			state.torsoAnim = ent->s.torsoAnim;
//			state.weapAnim = ent->s.weapAnim;	//----(SA)
//----(SA)	didn't want to comment in as I wasn't sure of any implications of changing the aas_entityinfo_t and bot_entitystate_t structures.
			state.weapon = ent->s.weapon;
			/*
			if (!BotAI_GetEntityState(i, &entitystate)) continue;
			//
			memset(&state, 0, sizeof(bot_entitystate_t));
			//
			VectorCopy(entitystate.pos.trBase, state.origin);
			VectorCopy(entitystate.angles, state.angles);
			VectorCopy(ent->s.origin2, state.old_origin);
			//VectorCopy(ent->r.mins, state.mins);
			//VectorCopy(ent->r.maxs, state.maxs);
			state.type = entitystate.eType;
			state.flags = entitystate.eFlags;
			if (ent->r.bmodel) state.solid = SOLID_BSP;
			else state.solid = SOLID_BBOX;
			state.modelindex = entitystate.modelindex;
			state.modelindex2 = entitystate.modelindex2;
			state.frame = entitystate.frame;
			state.event = entitystate.event;
			state.eventParm = entitystate.eventParm;
			state.powerups = entitystate.powerups;
			state.legsAnim = entitystate.legsAnim;
			state.torsoAnim = entitystate.torsoAnim;
			state.weapon = entitystate.weapon;
			*/
			//
			trap_BotLibUpdateEntity( i, &state );
		}

		BotAIRegularUpdate();

	}

	// Ridah, in single player, don't need bot's thinking
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return BLERR_NOERROR;
	}

	// execute scheduled bot AI
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		// Ridah
		if ( g_entities[i].r.svFlags & SVF_CASTAI ) {
			continue;
		}
		// done.
		//
		botstates[i]->botthink_residual += elapsed_time;
		//
		if ( botstates[i]->botthink_residual >= thinktime ) {
			botstates[i]->botthink_residual -= thinktime;

			if ( !trap_AAS_Initialized() ) {
				return BLERR_NOERROR;
			}

			if ( g_entities[i].client->pers.connected == CON_CONNECTED ) {
				BotAI( i, (float) thinktime / 1000 );
			}
		}
	}


	// execute bot user commands every frame
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !botstates[i] || !botstates[i]->inuse ) {
			continue;
		}
		// Ridah
		if ( g_entities[i].r.svFlags & SVF_CASTAI ) {
			continue;
		}
		// done.
		if ( g_entities[i].client->pers.connected != CON_CONNECTED ) {
			continue;
		}

		BotUpdateInput( botstates[i], time );
		trap_BotUserCommand( botstates[i]->client, &botstates[i]->lastucmd );
	}

	return BLERR_NOERROR;
}

/*
==============
BotInitLibrary
==============
*/
int BotInitLibrary( void ) {
	char buf[144];

	//set the maxclients and maxentities library variables before calling BotSetupLibrary
	trap_Cvar_VariableStringBuffer( "sv_maxclients", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "8" );
	}
	trap_BotLibVarSet( "maxclients", buf );
	Com_sprintf( buf, sizeof( buf ), "%d", MAX_GENTITIES );
	trap_BotLibVarSet( "maxentities", buf );
	//bsp checksum
	trap_Cvar_VariableStringBuffer( "sv_mapChecksum", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "sv_mapChecksum", buf );
	}
	//maximum number of aas links
	trap_Cvar_VariableStringBuffer( "max_aaslinks", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "max_aaslinks", buf );
	}
	//maximum number of items in a level
	trap_Cvar_VariableStringBuffer( "max_levelitems", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "max_levelitems", buf );
	}
	//automatically launch WinBSPC if AAS file not available
	trap_Cvar_VariableStringBuffer( "autolaunchbspc", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "autolaunchbspc", "1" );
	}
	//
	trap_Cvar_VariableStringBuffer( "g_gametype", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "0" );
	}
	trap_BotLibVarSet( "g_gametype", buf );
	//
	// Rafael gameskill
	trap_Cvar_VariableStringBuffer( "g_gameskill", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "0" );
	}
	trap_BotLibVarSet( "g_gamekill", buf );
	// done
	//
	trap_Cvar_VariableStringBuffer( "bot_developer", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "0" );
	}
	trap_BotLibVarSet( "bot_developer", buf );
	//log file
	trap_Cvar_VariableStringBuffer( "bot_developer", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "0" );
	}
	trap_BotLibVarSet( "log", buf );
	//no chatting
	trap_Cvar_VariableStringBuffer( "bot_nochat", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "nochat", "0" );
	}
	//forced clustering calculations
	trap_Cvar_VariableStringBuffer( "forceclustering", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "forceclustering", buf );
	}
	//forced reachability calculations
	trap_Cvar_VariableStringBuffer( "forcereachability", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "forcereachability", buf );
	}
	//force writing of AAS to file
	trap_Cvar_VariableStringBuffer( "forcewrite", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "forcewrite", buf );
	}
	//no AAS optimization
	trap_Cvar_VariableStringBuffer( "nooptimize", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "nooptimize", buf );
	}
	//number of reachabilities to calculate each frame
	trap_Cvar_VariableStringBuffer( "framereachability", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "20" );
	}
	trap_BotLibVarSet( "framereachability", buf );
	//
	trap_Cvar_VariableStringBuffer( "bot_reloadcharacters", buf, sizeof( buf ) );
	if ( !strlen( buf ) ) {
		strcpy( buf, "0" );
	}
	trap_BotLibVarSet( "bot_reloadcharacters", buf );
	//base directory
	trap_Cvar_VariableStringBuffer( "fs_basepath", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "basedir", buf );
	}
	//game directory
	trap_Cvar_VariableStringBuffer( "fs_game", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "gamedir", buf );
	}
	//cd directory
	trap_Cvar_VariableStringBuffer( "fs_cdpath", buf, sizeof( buf ) );
	if ( strlen( buf ) ) {
		trap_BotLibVarSet( "cddir", buf );
	}
	//setup the bot library
	return trap_BotLibSetup();
}

/*
==============
BotAISetup
==============
*/
int BotAISetup( int restart ) {
	int errnum;

#ifdef RANDOMIZE
	srand( (unsigned)time( NULL ) );
#endif //RANDOMIZE

	trap_Cvar_Register( &bot_thinktime, "bot_thinktime", "100", 0 );
	trap_Cvar_Register( &memorydump, "memorydump", "0", 0 );

	//if the game is restarted for a tournament
	if ( restart ) {
		return BLERR_NOERROR;
	}

	//initialize the bot states
	memset( botstates, 0, sizeof( botstates ) );

	trap_Cvar_Register( &bot_thinktime, "bot_thinktime", "100", 0 );

	errnum = BotInitLibrary();
	if ( errnum != BLERR_NOERROR ) {
		return qfalse;
	}
	return BLERR_NOERROR;
}

/*
==============
BotAIShutdown
==============
*/
int BotAIShutdown( int restart ) {

	int i;

	//if the game is restarted for a tournament
	if ( restart ) {
		//shutdown all the bots in the botlib
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( botstates[i] && botstates[i]->inuse ) {
				BotAIShutdownClient( botstates[i]->client );
			}
		}
		//don't shutdown the bot library
	} else {
		trap_BotLibShutdown();
	}
	return qtrue;
}

