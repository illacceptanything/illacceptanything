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
 * name:		ai_chat.c
 *
 * desc:		Quake3 bot AI
 *
 *
 *****************************************************************************/

#include "../game/g_local.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"
#include "../botai/botai.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
//
#include "chars.h"               //characteristics
#include "inv.h"             //indexes into the inventory
#include "syn.h"             //synonyms
#include "match.h"               //string matching types and vars


/*
==================
BotNumActivePlayers
==================
*/
int BotNumActivePlayers( void ) {
	int i, num;
	char buf[MAX_INFO_STRING];
	static int maxclients;

	if ( !maxclients ) {
		maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	num = 0;
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		trap_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}
		//skip spectators
		if ( atoi( Info_ValueForKey( buf, "t" ) ) == TEAM_SPECTATOR ) {
			continue;
		}
		//
		num++;
	}
	return num;
}

/*
==================
BotIsFirstInRankings
==================
*/
int BotIsFirstInRankings( bot_state_t *bs ) {
	int i, score;
	char buf[MAX_INFO_STRING];
	static int maxclients;
	playerState_t ps;

	if ( !maxclients ) {
		maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	score = bs->cur_ps.persistant[PERS_SCORE];
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		trap_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}
		//skip spectators
		if ( atoi( Info_ValueForKey( buf, "t" ) ) == TEAM_SPECTATOR ) {
			continue;
		}
		//
		BotAI_GetClientState( i, &ps );
		if ( score < ps.persistant[PERS_SCORE] ) {
			return qfalse;
		}
	}
	return qtrue;
}

/*
==================
BotIsLastInRankings
==================
*/
int BotIsLastInRankings( bot_state_t *bs ) {
	int i, score;
	char buf[MAX_INFO_STRING];
	static int maxclients;
	playerState_t ps;

	if ( !maxclients ) {
		maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	score = bs->cur_ps.persistant[PERS_SCORE];
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		trap_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}
		//skip spectators
		if ( atoi( Info_ValueForKey( buf, "t" ) ) == TEAM_SPECTATOR ) {
			continue;
		}
		//
		BotAI_GetClientState( i, &ps );
		if ( score > ps.persistant[PERS_SCORE] ) {
			return qfalse;
		}
	}
	return qtrue;
}

/*
==================
BotFirstClientInRankings
==================
*/
char *BotFirstClientInRankings( void ) {
	int i, bestscore, bestclient;
	char buf[MAX_INFO_STRING];
	static char name[32];
	static int maxclients;
	playerState_t ps;

	if ( !maxclients ) {
		maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	bestscore = -999999;
	bestclient = 0;
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		trap_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}
		//skip spectators
		if ( atoi( Info_ValueForKey( buf, "t" ) ) == TEAM_SPECTATOR ) {
			continue;
		}
		//
		BotAI_GetClientState( i, &ps );
		if ( ps.persistant[PERS_SCORE] > bestscore ) {
			bestscore = ps.persistant[PERS_SCORE];
			bestclient = i;
		}
	}
	EasyClientName( bestclient, name, 32 );
	return name;
}

/*
==================
BotLastClientInRankings
==================
*/
char *BotLastClientInRankings( void ) {
	int i, worstscore, bestclient;
	char buf[MAX_INFO_STRING];
	static char name[32];
	static int maxclients;
	playerState_t ps;

	if ( !maxclients ) {
		maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	worstscore = 999999;
	bestclient = 0;
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		trap_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}
		//skip spectators
		if ( atoi( Info_ValueForKey( buf, "t" ) ) == TEAM_SPECTATOR ) {
			continue;
		}
		//
		BotAI_GetClientState( i, &ps );
		if ( ps.persistant[PERS_SCORE] < worstscore ) {
			worstscore = ps.persistant[PERS_SCORE];
			bestclient = i;
		}
	}
	EasyClientName( bestclient, name, 32 );
	return name;
}

/*
==================
BotRandomOpponentName
==================
*/
char *BotRandomOpponentName( bot_state_t *bs ) {
	int i, count;
	char buf[MAX_INFO_STRING];
	int opponents[MAX_CLIENTS], numopponents;
	static int maxclients;
	static char name[32];

	if ( !maxclients ) {
		maxclients = trap_Cvar_VariableIntegerValue( "sv_maxclients" );
	}

	numopponents = 0;
	opponents[0] = 0;
	for ( i = 0; i < maxclients && i < MAX_CLIENTS; i++ ) {
		if ( i == bs->client ) {
			continue;
		}
		//
		trap_GetConfigstring( CS_PLAYERS + i, buf, sizeof( buf ) );
		//if no config string or no name
		if ( !strlen( buf ) || !strlen( Info_ValueForKey( buf, "n" ) ) ) {
			continue;
		}
		//skip spectators
		if ( atoi( Info_ValueForKey( buf, "t" ) ) == TEAM_SPECTATOR ) {
			continue;
		}
		//skip team mates
		if ( BotSameTeam( bs, i ) ) {
			continue;
		}
		//
		opponents[numopponents] = i;
		numopponents++;
	}
	count = random() * numopponents;
	for ( i = 0; i < numopponents; i++ ) {
		count--;
		if ( count <= 0 ) {
			EasyClientName( opponents[i], name, sizeof( name ) );
			return name;
		}
	}
	EasyClientName( opponents[0], name, sizeof( name ) );
	return name;
}

/*
==================
BotMapTitle
==================
*/

char *BotMapTitle( void ) {
	char info[1024];
	static char mapname[128];

	trap_GetServerinfo( info, sizeof( info ) );

	strncpy( mapname, Info_ValueForKey( info, "mapname" ), sizeof( mapname ) - 1 );
	mapname[sizeof( mapname ) - 1] = '\0';

	return mapname;
}


/*
==================
BotWeaponNameForMeansOfDeath
==================
*/

char *BotWeaponNameForMeansOfDeath( int mod ) {
	switch ( mod ) {
	case MOD_SHOTGUN: return "Shotgun";
	case MOD_GAUNTLET: return "Gauntlet";
	case MOD_MACHINEGUN: return "Machinegun";
	case MOD_GRENADE:
	case MOD_GRENADE_SPLASH: return "Grenade Launcher";
	case MOD_ROCKET:
	case MOD_ROCKET_SPLASH: return "Rocket Launcher";
	case MOD_RAILGUN: return "Railgun";
	case MOD_LIGHTNING: return "Lightning Gun";
	case MOD_BFG:
	case MOD_BFG_SPLASH: return "BFG10K";
	case MOD_GRAPPLE: return "Grapple";
	default: return "[unknown weapon]";
	}
}

/*
==================
BotRandomWeaponName
==================
*/
char *BotRandomWeaponName( void ) {
	int rnd;

	rnd = random() * 8.9;
	switch ( rnd ) {
	case 0: return "Gauntlet";
	case 1: return "Shotgun";
	case 2: return "Machinegun";
	case 3: return "Grenade Launcher";
	case 4: return "Rocket Launcher";
	case 5: return "Plasmagun";
	case 6: return "Railgun";
	case 7: return "Lightning Gun";
	default: return "BFG10K";
	}
}

/*
==================
BotValidChatPosition
==================
*/
int BotValidChatPosition( bot_state_t *bs ) {
	vec3_t point, start, end, mins, maxs;
	bsp_trace_t trace;

	//if the bot is dead all positions are valid
	if ( BotIsDead( bs ) ) {
		return qtrue;
	}
	//must be on the ground
	//if (bs->cur_ps.groundEntityNum != ENTITYNUM_NONE) return qfalse;
	//do not chat if in lava or slime
	VectorCopy( bs->origin, point );
	point[2] -= 24;
	if ( trap_PointContents( point,bs->entitynum ) & ( CONTENTS_LAVA | CONTENTS_SLIME ) ) {
		return qfalse;
	}
	//do not chat if under water
	VectorCopy( bs->origin, point );
	point[2] += 32;
	if ( trap_PointContents( point,bs->entitynum ) & MASK_WATER ) {
		return qfalse;
	}
	//must be standing on the world entity
	VectorCopy( bs->origin, start );
	VectorCopy( bs->origin, end );
	start[2] += 1;
	end[2] -= 10;
	trap_AAS_PresenceTypeBoundingBox( PRESENCE_CROUCH, mins, maxs );
	BotAI_Trace( &trace, start, mins, maxs, end, bs->client, MASK_SOLID );
	if ( trace.ent != ENTITYNUM_WORLD ) {
		return qfalse;
	}
	//the bot is in a position where it can chat
	return qtrue;
}

/*
==================
BotChat_EnterGame
==================
*/
int BotChat_EnterGame( bot_state_t *bs ) {
	char name[32];
	float rnd;

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	//don't chat in teamplay
	if ( TeamPlayIsOn() ) {
		return qfalse;
	}
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_ENTEREXITGAME, 0, 1 );
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd ) {
			return qfalse;
		}
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	if ( !BotValidChatPosition( bs ) ) {
		return qfalse;
	}
	BotAI_BotInitialChat( bs, "game_enter",
						  EasyClientName( bs->client, name, 32 ), // 0
						  BotRandomOpponentName( bs ),  // 1
						  "[invalid var]",          // 2
						  "[invalid var]",          // 3
						  BotMapTitle(),                // 4
						  NULL );
	bs->lastchat_time = trap_AAS_Time();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

/*
==================
BotChat_ExitGame
==================
*/
int BotChat_ExitGame( bot_state_t *bs ) {
	char name[32];
	float rnd;

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	//don't chat in teamplay
	if ( TeamPlayIsOn() ) {
		return qfalse;
	}
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_ENTEREXITGAME, 0, 1 );
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd ) {
			return qfalse;
		}
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	//
	BotAI_BotInitialChat( bs, "game_exit",
						  EasyClientName( bs->client, name, 32 ), // 0
						  BotRandomOpponentName( bs ),  // 1
						  "[invalid var]",          // 2
						  "[invalid var]",          // 3
						  BotMapTitle(),                // 4
						  NULL );
	bs->lastchat_time = trap_AAS_Time();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

/*
==================
BotChat_StartLevel
==================
*/
int BotChat_StartLevel( bot_state_t *bs ) {
	char name[32];
	float rnd;

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( BotIsObserver( bs ) ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	//don't chat in teamplay
	if ( TeamPlayIsOn() ) {
		return qfalse;
	}
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_STARTENDLEVEL, 0, 1 );
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd ) {
			return qfalse;
		}
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	BotAI_BotInitialChat( bs, "level_start",
						  EasyClientName( bs->client, name, 32 ), // 0
						  NULL );
	bs->lastchat_time = trap_AAS_Time();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

/*
==================
BotChat_EndLevel
==================
*/
int BotChat_EndLevel( bot_state_t *bs ) {
	char name[32];
	float rnd;

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( BotIsObserver( bs ) ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	//don't chat in teamplay
	if ( TeamPlayIsOn() ) {
		return qfalse;
	}
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_STARTENDLEVEL, 0, 1 );
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd ) {
			return qfalse;
		}
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	//
	if ( BotIsFirstInRankings( bs ) ) {
		BotAI_BotInitialChat( bs, "level_end_victory",
							  EasyClientName( bs->client, name, 32 ), // 0
							  BotRandomOpponentName( bs ), // 1
							  "[invalid var]",      // 2
							  BotLastClientInRankings(), // 3
							  BotMapTitle(),            // 4
							  NULL );
	} else if ( BotIsLastInRankings( bs ) )       {
		BotAI_BotInitialChat( bs, "level_end_lose",
							  EasyClientName( bs->client, name, 32 ), // 0
							  BotRandomOpponentName( bs ), // 1
							  BotFirstClientInRankings(), // 2
							  "[invalid var]",      // 3
							  BotMapTitle(),            // 4
							  NULL );
	} else {
		BotAI_BotInitialChat( bs, "level_end",
							  EasyClientName( bs->client, name, 32 ), // 0
							  BotRandomOpponentName( bs ), // 1
							  BotFirstClientInRankings(), // 2
							  BotLastClientInRankings(), // 3
							  BotMapTitle(),            // 4
							  NULL );
	}
	bs->lastchat_time = trap_AAS_Time();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

/*
==================
BotChat_Death
==================
*/
int BotChat_Death( bot_state_t *bs ) {
	char name[32];
	float rnd;

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_DEATH, 0, 1 );
	//if fast chatting is off
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd ) {
			return qfalse;
		}
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	//
	if ( bs->lastkilledby >= 0 && bs->lastkilledby < MAX_CLIENTS ) {
		EasyClientName( bs->lastkilledby, name, 32 );
	} else {
		strcpy( name, "[world]" );
	}
	//
	if ( TeamPlayIsOn() && BotSameTeam( bs, bs->lastkilledby ) ) {
		if ( bs->lastkilledby == bs->client ) {
			return qfalse;
		}
		BotAI_BotInitialChat( bs, "death_teammate", name, NULL );
		bs->chatto = CHAT_TEAM;
	} else
	{
		//don't chat in teamplay
		if ( TeamPlayIsOn() ) {
			return qfalse;
		}
		//
		if ( bs->botdeathtype == MOD_WATER ) {
			BotAI_BotInitialChat( bs, "death_drown", BotRandomOpponentName( bs ), NULL );
		} else if ( bs->botdeathtype == MOD_SLIME ) {
			BotAI_BotInitialChat( bs, "death_slime", BotRandomOpponentName( bs ), NULL );
		} else if ( bs->botdeathtype == MOD_LAVA ) {
			BotAI_BotInitialChat( bs, "death_lava", BotRandomOpponentName( bs ), NULL );
		} else if ( bs->botdeathtype == MOD_FALLING ) {
			BotAI_BotInitialChat( bs, "death_cratered", BotRandomOpponentName( bs ), NULL );
		} else if ( bs->botsuicide || //all other suicides by own weapon
					bs->botdeathtype == MOD_CRUSH ||
					bs->botdeathtype == MOD_SUICIDE ||
					bs->botdeathtype == MOD_TARGET_LASER ||
					bs->botdeathtype == MOD_TRIGGER_HURT ||
					bs->botdeathtype == MOD_UNKNOWN ) {
			BotAI_BotInitialChat( bs, "death_suicide", BotRandomOpponentName( bs ), NULL );
		} else if ( bs->botdeathtype == MOD_TELEFRAG ) {
			BotAI_BotInitialChat( bs, "death_telefrag", name, NULL );
		} else {
			if ( ( bs->botdeathtype == MOD_GAUNTLET ||
				   bs->botdeathtype == MOD_RAILGUN ||
				   bs->botdeathtype == MOD_BFG ||
				   bs->botdeathtype == MOD_BFG_SPLASH ) && random() < 0.5 ) {

				if ( bs->botdeathtype == MOD_GAUNTLET ) {
					BotAI_BotInitialChat( bs, "death_gauntlet",
										  name,                                 // 0
										  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
										  NULL );
				} else if ( bs->botdeathtype == MOD_RAILGUN ) {
					BotAI_BotInitialChat( bs, "death_rail",
										  name,                                 // 0
										  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
										  NULL );
				} else {
					BotAI_BotInitialChat( bs, "death_bfg",
										  name,                                 // 0
										  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
										  NULL );
				}
			}
			//choose between insult and praise
			else if ( random() < trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_INSULT, 0, 1 ) ) {
				BotAI_BotInitialChat( bs, "death_insult",
									  name,                                     // 0
									  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
									  NULL );
			} else {
				BotAI_BotInitialChat( bs, "death_praise",
									  name,                                     // 0
									  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
									  NULL );
			}
		}
		bs->chatto = CHAT_ALL;
	}
	bs->lastchat_time = trap_AAS_Time();
	return qtrue;
}

/*
==================
BotChat_Kill
==================
*/
int BotChat_Kill( bot_state_t *bs ) {
	char name[32];
	float rnd;

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_KILL, 0, 1 );
	//if fast chat is off
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd ) {
			return qfalse;
		}
	}
	if ( bs->lastkilledplayer == bs->client ) {
		return qfalse;
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	if ( !BotValidChatPosition( bs ) ) {
		return qfalse;
	}
	//
	EasyClientName( bs->lastkilledplayer, name, 32 );
	//
	bs->chatto = CHAT_ALL;
	if ( TeamPlayIsOn() && BotSameTeam( bs, bs->lastkilledplayer ) ) {
		BotAI_BotInitialChat( bs, "kill_teammate", name, NULL );
		bs->chatto = CHAT_TEAM;
	} else
	{
		//don't chat in teamplay
		if ( TeamPlayIsOn() ) {
			return qfalse;
		}
		//
		if ( bs->enemydeathtype == MOD_GAUNTLET ) {
			BotAI_BotInitialChat( bs, "kill_gauntlet", name, NULL );
		} else if ( bs->enemydeathtype == MOD_RAILGUN )     {
			BotAI_BotInitialChat( bs, "kill_rail", name, NULL );
		} else if ( bs->enemydeathtype == MOD_TELEFRAG )     {
			BotAI_BotInitialChat( bs, "kill_telefrag", name, NULL );
		}
		//choose between insult and praise
		else if ( random() < trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_INSULT, 0, 1 ) ) {
			BotAI_BotInitialChat( bs, "kill_insult", name, NULL );
		} else {
			BotAI_BotInitialChat( bs, "kill_praise", name, NULL );
		}
	}
	bs->lastchat_time = trap_AAS_Time();
	return qtrue;
}

/*
==================
BotChat_EnemySuicide
==================
*/
int BotChat_EnemySuicide( bot_state_t *bs ) {
	char name[32];
	float rnd;

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	//
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_KILL, 0, 1 );
	//don't chat in teamplay
	if ( TeamPlayIsOn() ) {
		return qfalse;
	}
	//if fast chat is off
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd ) {
			return qfalse;
		}
	}
	if ( !BotValidChatPosition( bs ) ) {
		return qfalse;
	}
	//
	if ( bs->enemy >= 0 ) {
		EasyClientName( bs->enemy, name, 32 );
	} else { strcpy( name, "" );}
	BotAI_BotInitialChat( bs, "enemy_suicide", name, NULL );
	bs->lastchat_time = trap_AAS_Time();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

/*
==================
BotChat_HitTalking
==================
*/
int BotChat_HitTalking( bot_state_t *bs ) {
	char name[32], *weap;
	int lasthurt_client;
	float rnd;

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	lasthurt_client = g_entities[bs->client].client->lasthurt_client;
	if ( !lasthurt_client ) {
		return qfalse;
	}
	if ( lasthurt_client == bs->client ) {
		return qfalse;
	}
	//
	if ( lasthurt_client < 0 || lasthurt_client >= MAX_CLIENTS ) {
		return qfalse;
	}
	//
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_HITTALKING, 0, 1 );
	//don't chat in teamplay
	if ( TeamPlayIsOn() ) {
		return qfalse;
	}
	//if fast chat is off
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd * 0.5 ) {
			return qfalse;
		}
	}
	if ( !BotValidChatPosition( bs ) ) {
		return qfalse;
	}
	//
	ClientName( g_entities[bs->client].client->lasthurt_client, name, sizeof( name ) );
	weap = BotWeaponNameForMeansOfDeath( g_entities[bs->client].client->lasthurt_client );
	//
	BotAI_BotInitialChat( bs, "hit_talking", name, weap, NULL );
	bs->lastchat_time = trap_AAS_Time();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

/*
==================
BotChat_HitNoDeath
==================
*/
int BotChat_HitNoDeath( bot_state_t *bs ) {
	char name[32], *weap;
	float rnd;
	int lasthurt_client;
	aas_entityinfo_t entinfo;

	lasthurt_client = g_entities[bs->client].client->lasthurt_client;
	if ( !lasthurt_client ) {
		return qfalse;
	}
	if ( lasthurt_client == bs->client ) {
		return qfalse;
	}
	//
	if ( lasthurt_client < 0 || lasthurt_client >= MAX_CLIENTS ) {
		return qfalse;
	}
	//
	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_HITNODEATH, 0, 1 );
	//don't chat in teamplay
	if ( TeamPlayIsOn() ) {
		return qfalse;
	}
	//if fast chat is off
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd * 0.5 ) {
			return qfalse;
		}
	}
	if ( !BotValidChatPosition( bs ) ) {
		return qfalse;
	}
	//if the enemy is visible
	if ( BotEntityVisible( bs->client, bs->eye, bs->viewangles, 360, bs->enemy ) ) {
		return qfalse;
	}
	//
	BotEntityInfo( bs->enemy, &entinfo );
	if ( EntityIsShooting( &entinfo ) ) {
		return qfalse;
	}
	//
	ClientName( lasthurt_client, name, sizeof( name ) );
	weap = BotWeaponNameForMeansOfDeath( g_entities[bs->client].client->lasthurt_mod );
	//
	BotAI_BotInitialChat( bs, "hit_nodeath", name, weap, NULL );
	bs->lastchat_time = trap_AAS_Time();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

/*
==================
BotChat_HitNoKill
==================
*/
int BotChat_HitNoKill( bot_state_t *bs ) {
	char name[32], *weap;
	float rnd;
	aas_entityinfo_t entinfo;

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_HITNOKILL, 0, 1 );
	//don't chat in teamplay
	if ( TeamPlayIsOn() ) {
		return qfalse;
	}
	//if fast chat is off
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd * 0.5 ) {
			return qfalse;
		}
	}
	if ( !BotValidChatPosition( bs ) ) {
		return qfalse;
	}
	//if the enemy is visible
	if ( BotEntityVisible( bs->client, bs->eye, bs->viewangles, 360, bs->enemy ) ) {
		return qfalse;
	}
	//
	BotEntityInfo( bs->enemy, &entinfo );
	if ( EntityIsShooting( &entinfo ) ) {
		return qfalse;
	}
	//
	ClientName( bs->enemy, name, sizeof( name ) );
	weap = BotWeaponNameForMeansOfDeath( g_entities[bs->enemy].client->lasthurt_mod );
	//
	BotAI_BotInitialChat( bs, "hit_nokill", name, weap, NULL );
	bs->lastchat_time = trap_AAS_Time();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

/*
==================
BotChat_Random
==================
*/
int BotChat_Random( bot_state_t *bs ) {
	float rnd;
	char name[32];

	if ( bot_nochat.integer ) {
		return qfalse;
	}
	if ( BotIsObserver( bs ) ) {
		return qfalse;
	}
	if ( bs->lastchat_time > trap_AAS_Time() - 3 ) {
		return qfalse;
	}
	//don't chat in teamplay
	if ( TeamPlayIsOn() ) {
		return qfalse;
	}
	//don't chat when doing something important :)
	if ( bs->ltgtype == LTG_TEAMHELP ||
		 bs->ltgtype == LTG_TEAMACCOMPANY ||
		 bs->ltgtype == LTG_RUSHBASE ) {
		return qfalse;
	}
	//
	rnd = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_RANDOM, 0, 1 );
	if ( random() > bs->thinktime * 0.1 ) {
		return qfalse;
	}
	if ( !bot_fastchat.integer ) {
		if ( random() > rnd ) {
			return qfalse;
		}
		if ( random() > 0.25 ) {
			return qfalse;
		}
	}
	if ( BotNumActivePlayers() <= 1 ) {
		return qfalse;
	}
	if ( !BotValidChatPosition( bs ) ) {
		return qfalse;
	}
	//
	if ( bs->lastkilledplayer == bs->client ) {
		strcpy( name, BotRandomOpponentName( bs ) );
	} else {
		EasyClientName( bs->lastkilledplayer, name, sizeof( name ) );
	}
	//
	if ( random() < trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CHAT_MISC, 0, 1 ) ) {
		BotAI_BotInitialChat( bs, "random_misc",
							  BotRandomOpponentName( bs ), // 0
							  name,             // 1
							  "[invalid var]", // 2
							  "[invalid var]", // 3
							  BotMapTitle(),    // 4
							  BotRandomWeaponName(), // 5
							  NULL );
	} else {
		BotAI_BotInitialChat( bs, "random_insult",
							  BotRandomOpponentName( bs ), // 0
							  name,             // 1
							  "[invalid var]", // 2
							  "[invalid var]", // 3
							  BotMapTitle(),    // 4
							  BotRandomWeaponName(), // 5
							  NULL );
	}
	bs->lastchat_time = trap_AAS_Time();
	bs->chatto = CHAT_ALL;
	return qtrue;
}

/*
==================
BotChatTime
==================
*/
float BotChatTime( bot_state_t *bs ) {
	int cpm;

	cpm = trap_Characteristic_BInteger( bs->character, CHARACTERISTIC_CHAT_CPM, 1, 4000 );

	return 2.0; //(float) trap_BotChatLength(bs->cs) * 30 / cpm;
}

/*
==================
BotChatTest
==================
*/
void BotChatTest( bot_state_t *bs ) {

	char name[32];
	char *weap;
	int num, i;

	num = trap_BotNumInitialChats( bs->cs, "game_enter" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "game_enter",
							  EasyClientName( bs->client, name, 32 ), // 0
							  BotRandomOpponentName( bs ),  // 1
							  "[invalid var]",          // 2
							  "[invalid var]",          // 3
							  BotMapTitle(),                // 4
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "game_exit" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "game_exit",
							  EasyClientName( bs->client, name, 32 ), // 0
							  BotRandomOpponentName( bs ),  // 1
							  "[invalid var]",          // 2
							  "[invalid var]",          // 3
							  BotMapTitle(),                // 4
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "level_start" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "level_start",
							  EasyClientName( bs->client, name, 32 ), // 0
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "level_end_victory" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "level_end_victory",
							  EasyClientName( bs->client, name, 32 ), // 0
							  BotRandomOpponentName( bs ), // 1
							  BotFirstClientInRankings(), // 2
							  BotLastClientInRankings(), // 3
							  BotMapTitle(),            // 4
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "level_end_lose" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "level_end_lose",
							  EasyClientName( bs->client, name, 32 ), // 0
							  BotRandomOpponentName( bs ), // 1
							  BotFirstClientInRankings(), // 2
							  BotLastClientInRankings(), // 3
							  BotMapTitle(),            // 4
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "level_end" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "level_end",
							  EasyClientName( bs->client, name, 32 ), // 0
							  BotRandomOpponentName( bs ), // 1
							  BotFirstClientInRankings(), // 2
							  BotLastClientInRankings(), // 3
							  BotMapTitle(),            // 4
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	EasyClientName( bs->lastkilledby, name, sizeof( name ) );
	num = trap_BotNumInitialChats( bs->cs, "death_drown" );
	for ( i = 0; i < num; i++ )
	{
		//
		BotAI_BotInitialChat( bs, "death_drown", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_slime" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_slime", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_lava" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_lava", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_cratered" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_cratered", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_suicide" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_suicide", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_telefrag" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_telefrag", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_gauntlet" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_gauntlet",
							  name,                                 // 0
							  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_rail" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_rail",
							  name,                                 // 0
							  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_bfg" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_bfg",
							  name,                                 // 0
							  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_insult" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_insult",
							  name,                                     // 0
							  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "death_praise" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "death_praise",
							  name,                                     // 0
							  BotWeaponNameForMeansOfDeath( bs->botdeathtype ), // 1
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	//
	EasyClientName( bs->lastkilledplayer, name, 32 );
	//
	num = trap_BotNumInitialChats( bs->cs, "kill_gauntlet" );
	for ( i = 0; i < num; i++ )
	{
		//
		BotAI_BotInitialChat( bs, "kill_gauntlet", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "kill_rail" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "kill_rail", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "kill_telefrag" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "kill_telefrag", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "kill_insult" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "kill_insult", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "kill_praise" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "kill_praise", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "enemy_suicide" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "enemy_suicide", name, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	ClientName( g_entities[bs->client].client->lasthurt_client, name, sizeof( name ) );
	weap = BotWeaponNameForMeansOfDeath( g_entities[bs->client].client->lasthurt_client );
	num = trap_BotNumInitialChats( bs->cs, "hit_talking" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "hit_talking", name, weap, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "hit_nodeath" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "hit_nodeath", name, weap, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "hit_nokill" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "hit_nokill", name, weap, NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	//
	if ( bs->lastkilledplayer == bs->client ) {
		strcpy( name, BotRandomOpponentName( bs ) );
	} else {
		EasyClientName( bs->lastkilledplayer, name, sizeof( name ) );
	}
	//
	num = trap_BotNumInitialChats( bs->cs, "random_misc" );
	for ( i = 0; i < num; i++ )
	{
		//
		BotAI_BotInitialChat( bs, "random_misc",
							  BotRandomOpponentName( bs ), // 0
							  name,             // 1
							  "[invalid var]", // 2
							  "[invalid var]", // 3
							  BotMapTitle(),    // 4
							  BotRandomWeaponName(), // 5
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
	num = trap_BotNumInitialChats( bs->cs, "random_insult" );
	for ( i = 0; i < num; i++ )
	{
		BotAI_BotInitialChat( bs, "random_insult",
							  BotRandomOpponentName( bs ), // 0
							  name,             // 1
							  "[invalid var]", // 2
							  "[invalid var]", // 3
							  BotMapTitle(),    // 4
							  BotRandomWeaponName(), // 5
							  NULL );
		trap_BotEnterChat( bs->cs, bs->client, CHAT_ALL );
	}
}
