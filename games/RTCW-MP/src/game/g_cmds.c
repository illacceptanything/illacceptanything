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

#include "g_local.h"
/*
==================
DeathmatchScoreboardMessage

==================
*/
void DeathmatchScoreboardMessage( gentity_t *ent ) {
	char entry[1024];
	char string[1400];
	int stringlength;
	int i, j;
	gclient_t   *cl;
	int numSorted;
	int scoreFlags;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;
	scoreFlags = 0;

	// don't send more than 32 scores (FIXME?)
	numSorted = level.numConnectedClients;
	if ( numSorted > 32 ) {
		numSorted = 32;
	}

	for ( i = 0 ; i < numSorted ; i++ ) {
		int ping;
		int playerClass;
		int respawnsLeft;

		cl = &level.clients[level.sortedClients[i]];

		// NERVE - SMF - if on same team, send across player class
		if ( cl->ps.persistant[PERS_TEAM] == ent->client->ps.persistant[PERS_TEAM] ) {
			playerClass = cl->ps.stats[STAT_PLAYER_CLASS];
		} else {
			playerClass = 0;
		}

		// NERVE - SMF - number of respawns left
		respawnsLeft = cl->ps.persistant[PERS_RESPAWNS_LEFT];
		if ( respawnsLeft == 0 && ( ( cl->ps.pm_flags & PMF_LIMBO ) || ( level.intermissiontime && g_entities[level.sortedClients[i]].health <= 0 ) ) ) {
			respawnsLeft = -2;
		}

		if ( cl->pers.connected == CON_CONNECTING ) {
			ping = -1;
		} else {
			ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
		}
		Com_sprintf( entry, sizeof( entry ),
					 " %i %i %i %i %i %i %i %i", level.sortedClients[i],
					 cl->ps.persistant[PERS_SCORE], ping, ( level.time - cl->pers.enterTime ) / 60000,
					 scoreFlags, g_entities[level.sortedClients[i]].s.powerups, playerClass, respawnsLeft );
		j = strlen( entry );
		if ( stringlength + j > 1024 ) {
			break;
		}
		strcpy( string + stringlength, entry );
		stringlength += j;
	}

	trap_SendServerCommand( ent - g_entities, va( "scores %i %i %i%s", i,
												  level.teamScores[TEAM_RED], level.teamScores[TEAM_BLUE],
												  string ) );
}


/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	DeathmatchScoreboardMessage( ent );
}



/*
==================
CheatsOk
==================
*/
qboolean    CheatsOk( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent - g_entities, va( "print \"Cheats are not enabled on this server.\n\"" ) );
		return qfalse;
	}
	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent - g_entities, va( "print \"You must be alive to use this command.\n\"" ) );
		return qfalse;
	}
	return qtrue;
}


/*
==================
ConcatArgs
==================
*/
char    *ConcatArgs( int start ) {
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out ) {
	while ( *in ) {
		if ( *in == 27 ) {
			in += 2;        // skip color code
			continue;
		}
		if ( *in < 32 ) {
			in++;
			continue;
		}
		*out++ = tolower( *in++ );
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t   *cl;
	int idnum;
	char s2[MAX_STRING_CHARS];
	char n2[MAX_STRING_CHARS];

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			trap_SendServerCommand( to - g_entities, va( "print \"Bad client slot: [lof]%i\n\"", idnum ) );
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			trap_SendServerCommand( to - g_entities, va( "print \"Client[lof] %i [lon]is not active\n\"", idnum ) );
			return -1;
		}
		return idnum;
	}

	// check for a name match
	SanitizeString( s, s2 );
	for ( idnum = 0,cl = level.clients ; idnum < level.maxclients ; idnum++,cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
		SanitizeString( cl->pers.netname, n2 );
		if ( !strcmp( n2, s2 ) ) {
			return idnum;
		}
	}

	trap_SendServerCommand( to - g_entities, va( "print \"User [lof]%s [lon]is not on the server\n\"", s ) );
	return -1;
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f( gentity_t *ent ) {
	char        *name, *amt;
	gitem_t     *it;
	int i;
	qboolean give_all;
	gentity_t       *it_ent;
	trace_t trace;
	int amount;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	//----(SA)	check for an amount (like "give health 30")
	amt = ConcatArgs( 2 );
	amount = atoi( amt );
	//----(SA)	end

	name = ConcatArgs( 1 );

	if ( Q_stricmp( name, "all" ) == 0 ) {
		give_all = qtrue;
	} else {
		give_all = qfalse;
	}


	if ( give_all || Q_stricmpn( name, "health", 6 ) == 0 ) {
		//----(SA)	modified
		if ( amount ) {
			ent->health += amount;
		} else {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmp( name, "weapons" ) == 0 ) {
		for ( i = 0; i < WP_NUM_WEAPONS; i++ ) {
			if ( BG_WeaponInWolfMP( i ) ) {
				COM_BitSet( ent->client->ps.weapons, i );
			}
		}

		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmp( name, "holdable" ) == 0 ) {
		ent->client->ps.stats[STAT_HOLDABLE_ITEM] = ( 1 << ( HI_BOOK3 - 1 ) ) - 1 - ( 1 << HI_NONE );
		for ( i = 1 ; i <= HI_BOOK3 ; i++ ) {
			ent->client->ps.holdable[i] = 10;
		}

		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmpn( name, "ammo", 4 ) == 0 ) {
		if ( amount ) {
			if ( ent->client->ps.weapon ) {
				Add_Ammo( ent, ent->client->ps.weapon, amount, qtrue );
			}
		} else {
			for ( i = 1 ; i < WP_MONSTER_ATTACK1 ; i++ )
				Add_Ammo( ent, i, 9999, qtrue );
		}

		if ( !give_all ) {
			return;
		}
	}

	//	"give allammo <n>" allows you to give a specific amount of ammo to /all/ weapons while
	//	allowing "give ammo <n>" to only give to the selected weap.
	if ( Q_stricmpn( name, "allammo", 7 ) == 0 && amount ) {
		for ( i = 1 ; i < WP_MONSTER_ATTACK1 ; i++ )
			Add_Ammo( ent, i, amount, qtrue );

		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || Q_stricmpn( name, "armor", 5 ) == 0 ) {
		if ( g_gametype.integer == GT_SINGLE_PLAYER ) { // JPW NERVE -- no armor in multiplayer
			//----(SA)	modified
			if ( amount ) {
				ent->client->ps.stats[STAT_ARMOR] += amount;
			} else {
				ent->client->ps.stats[STAT_ARMOR] = 200;
			}
		} // jpw
		if ( !give_all ) {
			return;
		}
	}

	//---- (SA) Wolf keys
	if ( give_all || Q_stricmp( name, "keys" ) == 0 ) {
		ent->client->ps.stats[STAT_KEYS] = ( 1 << KEY_NUM_KEYS ) - 2;
		if ( !give_all ) {
			return;
		}
	}
	//---- (SA) end

	// spawn a specific item right on the player
	if ( !give_all ) {
		it = BG_FindItem( name );
		if ( !it ) {
			return;
		}

		it_ent = G_Spawn();
		VectorCopy( ent->r.currentOrigin, it_ent->s.origin );
		it_ent->classname = it->classname;
		G_SpawnItem( it_ent, it );
		FinishSpawningItem( it_ent );
		memset( &trace, 0, sizeof( trace ) );
		it_ent->active = qtrue;
		Touch_Item( it_ent, ent, &trace );
		it_ent->active = qfalse;
		if ( it_ent->inuse ) {
			G_FreeEntity( it_ent );
		}
	}
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_GODMODE;
	if ( !( ent->flags & FL_GODMODE ) ) {
		msg = "godmode OFF\n";
	} else {
		msg = "godmode ON\n";
	}

	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}

/*
==================
Cmd_Nofatigue_f

Sets client to nofatigue

argv(0) nofatigue
==================
*/

void Cmd_Nofatigue_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOFATIGUE;
	if ( !( ent->flags & FL_NOFATIGUE ) ) {
		msg = "nofatigue OFF\n";
	} else {
		msg = "nofatigue ON\n";
	}

	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	ent->flags ^= FL_NOTARGET;
	if ( !( ent->flags & FL_NOTARGET ) ) {
		msg = "notarget OFF\n";
	} else {
		msg = "notarget ON\n";
	}

	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char    *msg;

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}
	ent->client->noclip = !ent->client->noclip;

	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent ) {
	if ( !CheatsOk( ent ) ) {
		return;
	}

	// doesn't work in single player
	if ( g_gametype.integer != 0 ) {
		trap_SendServerCommand( ent - g_entities,
								"print \"Must be in g_gametype 0 for levelshot\n\"" );
		return;
	}

	BeginIntermission();
	trap_SendServerCommand( ent - g_entities, "clientLevelShot" );
}


/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	if ( g_gamestate.integer != GS_PLAYING ) {
		return;
	}
	if ( g_gametype.integer >= GT_WOLF && ent->client->ps.pm_flags & PMF_LIMBO ) {
		return;
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
	ent->client->ps.persistant[PERS_HWEAPON_USE] = 0; // TTimo - if using /kill while at MG42
	player_die( ent, ent, ent, 100000, MOD_SUICIDE );
}


/*
=================
SetTeam
=================
*/
void SetTeam( gentity_t *ent, char *s ) {
	int team, oldTeam;
	gclient_t           *client;
	int clientNum;
	spectatorState_t specState;
	int specClient;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;

	specState = SPECTATOR_NOT;
	if ( !Q_stricmp( s, "scoreboard" ) || !Q_stricmp( s, "score" )  ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_SCOREBOARD;
	} else if ( !Q_stricmp( s, "follow1" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -1;
	} else if ( !Q_stricmp( s, "follow2" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FOLLOW;
		specClient = -2;
	} else if ( !Q_stricmp( s, "spectator" ) || !Q_stricmp( s, "s" ) ) {
		team = TEAM_SPECTATOR;
		specState = SPECTATOR_FREE;
	} else if ( g_gametype.integer >= GT_TEAM ) {
		// if running a team game, assign player to one of the teams
		specState = SPECTATOR_NOT;
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}

		// NERVE - SMF
		if ( g_noTeamSwitching.integer && team != ent->client->sess.sessionTeam && g_gamestate.integer == GS_PLAYING ) {
			trap_SendServerCommand( clientNum, "cp \"You cannot switch during a match, please wait until the round ends.\n\"" );
			return; // ignore the request
		}

		// NERVE - SMF - merge from team arena
		if ( g_teamForceBalance.integer  ) {
			int counts[TEAM_NUM_TEAMS];

			counts[TEAM_BLUE] = TeamCount( ent - g_entities, TEAM_BLUE );
			counts[TEAM_RED] = TeamCount( ent - g_entities, TEAM_RED );

			// We allow a spread of one
			if ( team == TEAM_RED && counts[TEAM_RED] - counts[TEAM_BLUE] >= 1 ) {
				trap_SendServerCommand( clientNum,
										"cp \"The Axis has too many players.\n\"" );
				return; // ignore the request
			}
			if ( team == TEAM_BLUE && counts[TEAM_BLUE] - counts[TEAM_RED] >= 1 ) {
				trap_SendServerCommand( clientNum,
										"cp \"The Allies have too many players.\n\"" );
				return; // ignore the request
			}

			// It's ok, the team we are switching to has less or same number of players
		}
		// -NERVE - SMF
	} else {
		// force them to spectators if there aren't any spots free
		team = TEAM_FREE;
	}

	// override decision if limiting the players
	if ( g_gametype.integer == GT_TOURNAMENT
		 && level.numNonSpectatorClients >= 2 ) {
		team = TEAM_SPECTATOR;
	} else if ( g_maxGameClients.integer > 0 &&
				level.numNonSpectatorClients >= g_maxGameClients.integer ) {
		team = TEAM_SPECTATOR;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return;
	}

	// NERVE - SMF - prevent players from switching to regain deployments
	if ( g_maxlives.integer > 0 && ent->client->ps.persistant[PERS_RESPAWNS_LEFT] == 0 &&
		 oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( clientNum,
								"cp \"You can't switch teams because you are out of lives.\n\" 3" );
		return; // ignore the request
	}

	// DHM - Nerve :: Force players to wait 30 seconds before they can join a new team.
	if ( g_gametype.integer >= GT_WOLF && team != oldTeam && level.warmupTime == 0 && !client->pers.initialSpawn
		 && ( ( level.time - client->pers.connectTime ) > 10000 ) && ( ( level.time - client->pers.enterTime ) < 30000 ) ) {
		trap_SendServerCommand( ent - g_entities,
								va( "cp \"^3You must wait %i seconds before joining ^3a new team.\n\" 3", (int)( 30 - ( ( level.time - client->pers.enterTime ) / 1000 ) ) ) );
		return;
	}
	// dhm

	//
	// execute the team change
	//

	// DHM - Nerve
	if ( client->pers.initialSpawn && team != TEAM_SPECTATOR ) {
		client->pers.initialSpawn = qfalse;
	}

	// he starts at 'base'
	client->pers.teamState.state = TEAM_BEGIN;
	if ( oldTeam != TEAM_SPECTATOR ) {
		if ( !( ent->client->ps.pm_flags & PMF_LIMBO ) ) {
			// Kill him (makes sure he loses flags, etc)
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die( ent, ent, ent, 100000, MOD_SUICIDE );
		}
	}
	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		client->sess.spectatorTime = level.time;
	}

	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;

	if ( team == TEAM_RED ) {
		trap_SendServerCommand( -1, va( "cp \"[lof]%s" S_COLOR_WHITE " [lon]joined the Axis team.\n\"",
										client->pers.netname ) );
	} else if ( team == TEAM_BLUE ) {
		trap_SendServerCommand( -1, va( "cp \"[lof]%s" S_COLOR_WHITE " [lon]joined the Allied team.\n\"",
										client->pers.netname ) );
	} else if ( team == TEAM_SPECTATOR && oldTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va( "cp \"[lof]%s" S_COLOR_WHITE " [lon]joined the spectators.\n\"",
										client->pers.netname ) );
	} else if ( team == TEAM_FREE ) {
		trap_SendServerCommand( -1, va( "cp \"[lof]%s" S_COLOR_WHITE " [lon]joined the battle.\n\"",
										client->pers.netname ) );
	}

	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );

	ClientBegin( clientNum );
}

// DHM - Nerve
/*
=================
SetWolfData
=================
*/
void SetWolfData( gentity_t *ent, char *ptype, char *weap, char *grenade, char *skinnum ) { // DHM - Nerve
	gclient_t   *client;

	client = ent->client;

	client->sess.latchPlayerType = atoi( ptype );
	client->sess.latchPlayerWeapon = atoi( weap );
	client->sess.latchPlayerItem = atoi( grenade );
	client->sess.latchPlayerSkin = atoi( skinnum );
}
// dhm - end

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode

=================
*/
void StopFollowing( gentity_t *ent ) {
	if ( g_gametype.integer < GT_WOLF ) {       // NERVE - SMF - don't forcibly set this for multiplayer
		ent->client->sess.sessionTeam = TEAM_SPECTATOR;
		ent->client->ps.persistant[ PERS_TEAM ] = TEAM_SPECTATOR;
	}

	// ATVI Wolfenstein Misc #474
	// divert behaviour if TEAM_SPECTATOR, moved the code from SpectatorThink to put back into free fly correctly
	// (I am not sure this can be called in non-TEAM_SPECTATOR situation, better be safe)
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		// drop to free floating, somewhere above the current position (that's the client you were following)
		vec3_t pos, angle;
		int enterTime;
		gclient_t   *client = ent->client;
		VectorCopy( client->ps.origin, pos ); pos[2] += 16;
		VectorCopy( client->ps.viewangles, angle );
		// ATVI Wolfenstein Misc #414, backup enterTime
		enterTime = client->pers.enterTime;
		SetTeam( ent, "spectator" );
		client->pers.enterTime = enterTime;
		VectorCopy( pos, client->ps.origin );
		SetClientViewAngle( ent, angle );
	} else
	{
		// legacy code, FIXME: useless?
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->r.svFlags &= ~SVF_BOT;
		ent->client->ps.clientNum = ent - g_entities;
	}
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent ) {
	int oldTeam;
	char s[MAX_TOKEN_CHARS];
	char ptype[4], weap[4], pistol[4], grenade[4], skinnum[4];

	if ( trap_Argc() < 2 ) {
		oldTeam = ent->client->sess.sessionTeam;
		switch ( oldTeam ) {
		case TEAM_BLUE:
			trap_SendServerCommand( ent - g_entities, "print \"Blue team\n\"" );
			break;
		case TEAM_RED:
			trap_SendServerCommand( ent - g_entities, "print \"Red team\n\"" );
			break;
		case TEAM_FREE:
			trap_SendServerCommand( ent - g_entities, "print \"Free team\n\"" );
			break;
		case TEAM_SPECTATOR:
			trap_SendServerCommand( ent - g_entities, "print \"Spectator team\n\"" );
			break;
		}
		return;
	}

	// if they are playing a tournement game, count as a loss
	if ( g_gametype.integer == GT_TOURNAMENT && ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}

	// DHM - Nerve
	if ( g_gametype.integer >= GT_WOLF ) {
		trap_Argv( 2, ptype, sizeof( ptype ) );
		trap_Argv( 3, weap, sizeof( weap ) );
		trap_Argv( 4, pistol, sizeof( pistol ) );
		trap_Argv( 5, grenade, sizeof( grenade ) );
		trap_Argv( 6, skinnum, sizeof( skinnum ) );

		SetWolfData( ent, ptype, weap, grenade, skinnum );
	}
	// dhm - end

	trap_Argv( 1, s, sizeof( s ) );

	SetTeam( ent, s );
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent ) {
	int i;
	char arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	if ( g_gametype.integer >= GT_WOLF ) {
		if ( level.clients[ i ].ps.pm_flags & PMF_LIMBO ) {
			return;
		}
	}

	// if they are playing a tournement game, count as a loss
	if ( g_gametype.integer == GT_TOURNAMENT && ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator" );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int clientnum;
	int original;

	// if they are playing a tournement game, count as a loss
	if ( g_gametype.integer == GT_TOURNAMENT && ent->client->sess.sessionTeam == TEAM_FREE ) {
		ent->client->sess.losses++;
	}
	// first set them to spectator
	if ( ( ent->client->sess.spectatorState == SPECTATOR_NOT ) && ( !( ent->client->ps.pm_flags & PMF_LIMBO ) ) ) { // JPW NERVE for limbo state
		SetTeam( ent, "spectator" );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

// JPW NERVE -- couple extra checks for limbo mode
		if ( ent->client->ps.pm_flags & PMF_LIMBO ) {
			if ( level.clients[clientnum].ps.pm_flags & PMF_LIMBO ) {
				continue;
			}
			if ( level.clients[clientnum].sess.sessionTeam != ent->client->sess.sessionTeam ) {
				continue;
			}
		}
// jpw

		if ( g_gametype.integer >= GT_WOLF ) {
			if ( level.clients[clientnum].ps.pm_flags & PMF_LIMBO ) {
				continue;
			}
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*
==================
G_Say
==================
*/
#define MAX_SAY_TEXT    150

#define SAY_ALL     0
#define SAY_TEAM    1
#define SAY_TELL    2
#define SAY_LIMBO   3           // NERVE - SMF

void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, qboolean localize ) { // removed static so it would link
	if ( !other ) {
		return;
	}
	if ( !other->inuse ) {
		return;
	}
	if ( !other->client ) {
		return;
	}
	if ( mode == SAY_TEAM  && !OnSameTeam( ent, other ) ) {
		return;
	}
	// no chatting to players in tournements
	if ( g_gametype.integer == GT_TOURNAMENT
		 && other->client->sess.sessionTeam == TEAM_FREE
		 && ent->client->sess.sessionTeam != TEAM_FREE ) {
		return;
	}

	// NERVE - SMF - if spectator, no chatting to players in WolfMP
	if ( g_gametype.integer >= GT_WOLF
		 && ( ( ent->client->sess.sessionTeam == TEAM_FREE && other->client->sess.sessionTeam != TEAM_FREE ) ||
			  ( ent->client->sess.sessionTeam == TEAM_SPECTATOR && other->client->sess.sessionTeam != TEAM_SPECTATOR ) ) ) {
		return;
	}

	// NERVE - SMF
	if ( mode == SAY_LIMBO ) {
		trap_SendServerCommand( other - g_entities, va( "%s \"%s%c%c%s\"",
														"lchat", name, Q_COLOR_ESCAPE, color, message ) );
	}
	// -NERVE - SMF
	else {
		trap_SendServerCommand( other - g_entities, va( "%s \"%s%c%c%s\" %i",
														mode == SAY_TEAM ? "tchat" : "chat",
														name, Q_COLOR_ESCAPE, color, message, localize ) );
	}
}

void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText ) {
	int j;
	gentity_t   *other;
	int color;
	char name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];
	char location[64];
	qboolean localize = qfalse;

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf( name, sizeof( name ), "%s%c%c: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_TEAM:
		localize = qtrue;
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		if ( Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( name, sizeof( name ), "[lof](%s%c%c) (%s): ",
						 ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		} else {
			Com_sprintf( name, sizeof( name ), "(%s%c%c): ",
						 ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_CYAN;
		break;
	case SAY_TELL:
		if ( target && g_gametype.integer >= GT_TEAM &&
			 target->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
			 Team_GetLocationMsg( ent, location, sizeof( location ) ) ) {
			Com_sprintf( name, sizeof( name ), "[%s%c%c] (%s): ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, location );
		} else {
			Com_sprintf( name, sizeof( name ), "[%s%c%c]: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		}
		color = COLOR_MAGENTA;
		break;
		// NERVE - SMF
	case SAY_LIMBO:
		G_LogPrintf( "say_limbo: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf( name, sizeof( name ), "%s%c%c: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
		// -NERVE - SMF
	}

	Q_strncpyz( text, chatText, sizeof( text ) );

	if ( target ) {
		G_SayTo( ent, target, mode, color, name, text, localize );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text );
	}

	// send it to all the apropriate clients
	for ( j = 0; j < level.maxclients; j++ ) {
		other = &g_entities[j];
		G_SayTo( ent, other, mode, color, name, text, localize );
	}
}


/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( gentity_t *ent, int mode, qboolean arg0 ) {
	char        *p;

	if ( trap_Argc() < 2 && !arg0 ) {
		return;
	}

	if ( arg0 ) {
		p = ConcatArgs( 0 );
	} else
	{
		p = ConcatArgs( 1 );
	}

	G_Say( ent, NULL, mode, p );
}

/*
==================
Cmd_Tell_f
==================
*/
static void Cmd_Tell_f( gentity_t *ent ) {
	int targetNum;
	gentity_t   *target;
	char        *p;
	char arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	p = ConcatArgs( 2 );

	G_LogPrintf( "tell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, p );
	G_Say( ent, target, SAY_TELL, p );
	G_Say( ent, ent, SAY_TELL, p );
}

// NERVE - SMF
static void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	char *cmd;

	if ( !other ) {
		return;
	}
	if ( !other->inuse ) {
		return;
	}
	if ( !other->client ) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam( ent, other ) ) {
		return;
	}
	// no chatting to players in tournements
	if ( ( g_gametype.integer == GT_TOURNAMENT ) ) {
		return;
	}

	if ( mode == SAY_TEAM ) {
		color = COLOR_CYAN;
		cmd = "vtchat";
	} else if ( mode == SAY_TELL )     {
		color = COLOR_MAGENTA;
		cmd = "vtell";
	} else {
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	trap_SendServerCommand( other - g_entities, va( "%s %d %d %d %s %i %i %i", cmd, voiceonly, ent->s.number, color, id,
													(int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2] ) );
}

void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	int j;
	gentity_t   *other;

	if ( g_gametype.integer < GT_TEAM && mode == SAY_TEAM ) {
		mode = SAY_ALL;
	}

	// DHM - Nerve :: Don't allow excessive spamming of voice chats
	ent->voiceChatSquelch -= ( level.time - ent->voiceChatPreviousTime );
	ent->voiceChatPreviousTime = level.time;

	if ( ent->voiceChatSquelch < 0 ) {
		ent->voiceChatSquelch = 0;
	}

	if ( ent->voiceChatSquelch >= 30000 ) {
		trap_SendServerCommand( ent - g_entities, "print \"^1Spam Protection^7: VoiceChat ignored\n\"" );
		return;
	}

	if ( g_voiceChatsAllowed.integer ) {
		ent->voiceChatSquelch += ( 34000 / g_voiceChatsAllowed.integer );
	} else {
		return;
	}
	// dhm

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->pers.netname, id );
	}

	// send it to all the apropriate clients
	for ( j = 0; j < level.maxclients; j++ ) {
		other = &g_entities[j];
		G_VoiceTo( ent, other, mode, id, voiceonly );
	}
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	char        *p;

	if ( trap_Argc() < 2 && !arg0 ) {
		return;
	}

	if ( arg0 ) {
		p = ConcatArgs( 0 );
	} else
	{
		p = ConcatArgs( 1 );
	}

	G_Voice( ent, NULL, mode, p, voiceonly );
}

// TTimo gcc: defined but not used
#if 0
/*
==================
Cmd_VoiceTell_f
==================
*/
static void Cmd_VoiceTell_f( gentity_t *ent, qboolean voiceonly ) {
	int targetNum;
	gentity_t   *target;
	char        *id;
	char arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	targetNum = atoi( arg );
	if ( targetNum < 0 || targetNum >= level.maxclients ) {
		return;
	}

	target = &g_entities[targetNum];
	if ( !target || !target->inuse || !target->client ) {
		return;
	}

	id = ConcatArgs( 2 );

	G_LogPrintf( "vtell: %s to %s: %s\n", ent->client->pers.netname, target->client->pers.netname, id );
	G_Voice( ent, target, SAY_TELL, id, voiceonly );
	// don't tell to the player self if it was already directed to this player
	// also don't send the chat back to a bot
	if ( ent != target && !( ent->r.svFlags & SVF_BOT ) ) {
		G_Voice( ent, ent, SAY_TELL, id, voiceonly );
	}
}
#endif

// TTimo gcc: defined but not used
#if 0
/*
==================
Cmd_VoiceTaunt_f
==================
*/
static void Cmd_VoiceTaunt_f( gentity_t *ent ) {
	gentity_t *who;
	int i;

	if ( !ent->client ) {
		return;
	}

	// insult someone who just killed you
	if ( ent->enemy && ent->enemy->client && ent->enemy->client->lastkilled_client == ent->s.number ) {
		// i am a dead corpse
		if ( !( ent->enemy->r.svFlags & SVF_BOT ) ) {
//			G_Voice( ent, ent->enemy, SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		if ( !( ent->r.svFlags & SVF_BOT ) ) {
//			G_Voice( ent, ent,        SAY_TELL, VOICECHAT_DEATHINSULT, qfalse );
		}
		ent->enemy = NULL;
		return;
	}
	// insult someone you just killed
	if ( ent->client->lastkilled_client >= 0 && ent->client->lastkilled_client != ent->s.number ) {
		who = g_entities + ent->client->lastkilled_client;
		if ( who->client ) {
			// who is the person I just killed
			if ( who->client->lasthurt_mod == MOD_GAUNTLET ) {
				if ( !( who->r.svFlags & SVF_BOT ) ) {
//					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );	// and I killed them with a gauntlet
				}
				if ( !( ent->r.svFlags & SVF_BOT ) ) {
//					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLGAUNTLET, qfalse );
				}
			} else {
				if ( !( who->r.svFlags & SVF_BOT ) ) {
//					G_Voice( ent, who, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );	// and I killed them with something else
				}
				if ( !( ent->r.svFlags & SVF_BOT ) ) {
//					G_Voice( ent, ent, SAY_TELL, VOICECHAT_KILLINSULT, qfalse );
				}
			}
			ent->client->lastkilled_client = -1;
			return;
		}
	}

	if ( g_gametype.integer >= GT_TEAM ) {
		// praise a team mate who just got a reward
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			who = g_entities + i;
			if ( who->client && who != ent && who->client->sess.sessionTeam == ent->client->sess.sessionTeam ) {
				if ( who->client->rewardTime > level.time ) {
					if ( !( who->r.svFlags & SVF_BOT ) ) {
//						G_Voice( ent, who, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					if ( !( ent->r.svFlags & SVF_BOT ) ) {
//						G_Voice( ent, ent, SAY_TELL, VOICECHAT_PRAISE, qfalse );
					}
					return;
				}
			}
		}
	}

	// just say something
//	G_Voice( ent, NULL, SAY_ALL, VOICECHAT_TAUNT, qfalse );
}
// -NERVE - SMF
#endif

static char *gc_orders[] = {
	"hold your position",
	"hold this position",
	"come here",
	"cover me",
	"guard location",
	"search and destroy",
	"report"
};

void Cmd_GameCommand_f( gentity_t *ent ) {
	int player;
	int order;
	char str[MAX_TOKEN_CHARS];

	trap_Argv( 1, str, sizeof( str ) );
	player = atoi( str );
	trap_Argv( 2, str, sizeof( str ) );
	order = atoi( str );

	if ( player < 0 || player >= MAX_CLIENTS ) {
		return;
	}
	if ( order < 0 || order > sizeof( gc_orders ) / sizeof( char * ) ) {
		return;
	}
	G_Say( ent, &g_entities[player], SAY_TELL, gc_orders[order] );
	G_Say( ent, ent, SAY_TELL, gc_orders[order] );
}

/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent ) {
	trap_SendServerCommand( ent - g_entities, va( "print \"%s\n\"", vtos( ent->s.origin ) ) );
}


static const char *gameNames[] = {
	"Free For All",
	"Tournament",
	"Single Player",
	"Team Deathmatch",
	"Capture the Flag",
	"Wolf Multiplayer",
	"Wolf Stopwatch",
	"Wolf Checkpoint"
};


/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent ) {
	int i;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];
	char cleanName[64];    // JPW NERVE
	int mask = 0;

	if ( !g_voteFlags.integer ) {
		trap_SendServerCommand( ent - g_entities, "print \"Voting not enabled on this server.\n\"" );
		return;
	}

	if ( level.voteTime ) {
		trap_SendServerCommand( ent - g_entities, "print \"A vote is already in progress.\n\"" );
		return;
	}
	if ( ent->client->pers.voteCount >= MAX_VOTE_COUNT ) {
		trap_SendServerCommand( ent - g_entities, "print \"You have called the maximum number of votes.\n\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent - g_entities, "print \"Not allowed to call a vote as spectator.\n\"" );
		return;
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );

	if ( strchr( arg1, ';' ) || strchr( arg2, ';' ) ) {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid vote string.\n\"" );
		return;
	}

	if ( !Q_stricmp( arg1, "map_restart" ) ) {
		mask = VOTEFLAGS_RESTART;
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
		mask = VOTEFLAGS_NEXTMAP;
	} else if ( !Q_stricmp( arg1, "map" ) ) {
		mask = VOTEFLAGS_MAP;
	} else if ( !Q_stricmp( arg1, "g_gametype" ) ) {
		mask = VOTEFLAGS_TYPE;
	} else if ( !Q_stricmp( arg1, "kick" ) ) {
		mask = VOTEFLAGS_KICK;
	} else if ( !Q_stricmp( arg1, "clientkick" ) ) {
		mask = VOTEFLAGS_KICK;
	} else if ( !Q_stricmp( arg1, "start_match" ) ) {        // NERVE - SMF
		mask = VOTEFLAGS_STARTMATCH;
	} else if ( !Q_stricmp( arg1, "reset_match" ) ) {        // NERVE - SMF
		mask = VOTEFLAGS_RESETMATCH;
	} else if ( !Q_stricmp( arg1, "swap_teams" ) ) {     // NERVE - SMF
		mask = VOTEFLAGS_SWAP;
// JPW NERVE
#ifndef PRE_RELEASE_DEMO
	} else if ( !Q_stricmp( arg1, testid1 ) ) {
	} else if ( !Q_stricmp( arg1, testid2 ) ) {
	} else if ( !Q_stricmp( arg1, testid3 ) ) {
#endif
// jpw
	} else {
		trap_SendServerCommand( ent - g_entities, "print \"Invalid vote string.\n\"" );
		trap_SendServerCommand( ent - g_entities, "print \"Vote commands are: map_restart, nextmap, start_match, swap_teams, reset_match, map <mapname>, g_gametype <n>, kick <player>, clientkick <clientnum>\n\"" );
		return;
	}

	if ( !( g_voteFlags.integer & mask ) ) {
		trap_SendServerCommand( ent - g_entities, va( "print \"Voting for %s disabled on this server\n\"", arg1 ) );
		return;
	}

	// if there is still a vote to be executed
	if ( level.voteExecuteTime ) {
		level.voteExecuteTime = 0;
		trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
	}

	// special case for g_gametype, check for bad values
	if ( !Q_stricmp( arg1, "g_gametype" ) ) {
		i = atoi( arg2 );
		if ( i < GT_WOLF || i >= GT_MAX_GAME_TYPE ) {
			trap_SendServerCommand( ent - g_entities, "print \"Invalid gametype.\n\"" );
			return;
		}

		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %d", arg1, i );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s %s", arg1, gameNames[i] );
	} else if ( !Q_stricmp( arg1, "map_restart" ) ) {
		// NERVE - SMF - do a warmup when we restart maps
		if ( strlen( arg2 ) ) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s", arg1, arg2 );
		}

		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	} else if ( !Q_stricmp( arg1, "map" ) ) {
		// special case for map changes, we want to reset the nextmap setting
		// this allows a player to change maps, but not upset the map rotation
		char s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof( s ) );
		if ( *s ) {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s; set nextmap \"%s\"", arg1, arg2, s );
		} else {
			Com_sprintf( level.voteString, sizeof( level.voteString ), "%s %s", arg1, arg2 );
		}
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	} else if ( !Q_stricmp( arg1, "nextmap" ) ) {
		char s[MAX_STRING_CHARS];

		trap_Cvar_VariableStringBuffer( "nextmap", s, sizeof( s ) );
		if ( !*s ) {
			trap_SendServerCommand( ent - g_entities, "print \"nextmap not set.\n\"" );
			return;
		}
		Com_sprintf( level.voteString, sizeof( level.voteString ), "vstr nextmap" );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
// JPW NERVE
	} else if ( !Q_stricmp( arg1,"kick" ) ) {
		int i,kicknum = MAX_CLIENTS;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( level.clients[i].pers.connected != CON_CONNECTED ) {
				continue;
			}
// strip the color crap out
			Q_strncpyz( cleanName, level.clients[i].pers.netname, sizeof( cleanName ) );
			Q_CleanStr( cleanName );
			if ( !Q_stricmp( cleanName, arg2 ) ) {
				kicknum = i;
			}
		}
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "kick %s", level.clients[kicknum].pers.netname );
		if ( kicknum != MAX_CLIENTS ) { // found a client # to kick, so override votestring with better one
			Com_sprintf( level.voteString, sizeof( level.voteString ),"clientkick \"%d\"",kicknum );
		} else { // if it can't do a name match, don't allow kick (to prevent votekick text spam wars)
			trap_SendServerCommand( ent - g_entities, "print \"Client not on server.\n\"" );
			return;
		}
// jpw
	} else {
		Com_sprintf( level.voteString, sizeof( level.voteString ), "%s \"%s\"", arg1, arg2 );
		Com_sprintf( level.voteDisplayString, sizeof( level.voteDisplayString ), "%s", level.voteString );
	}

	trap_SendServerCommand( -1, va( "print \"[lof]%s [lon]called a vote.\n\"", ent->client->pers.netname ) );

	// start the voting, the caller autoamtically votes yes
	level.voteTime = level.time;
	level.voteYes = 1;
	level.voteNo = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		level.clients[i].ps.eFlags &= ~EF_VOTED;
	}
	ent->client->ps.eFlags |= EF_VOTED;

	trap_SetConfigstring( CS_VOTE_TIME, va( "%i", level.voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING, level.voteDisplayString );
	trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char msg[64];
	int num;

	// DHM - Nerve :: Complaints supercede voting (and share command)
	if ( ent->client->pers.complaintEndTime > level.time ) {

		gclient_t *cl = g_entities[ ent->client->pers.complaintClient ].client;
		if ( !cl ) {
			return;
		}
		if ( cl->pers.connected != CON_CONNECTED ) {
			return;
		}
		if ( cl->pers.localClient ) {
			trap_SendServerCommand( ent - g_entities, "complaint -3" );
			return;
		}

		// Reset this ent's complainEndTime so they can't send multiple complaints
		ent->client->pers.complaintEndTime = -1;
		ent->client->pers.complaintClient = -1;

		trap_Argv( 1, msg, sizeof( msg ) );

		if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
			// Increase their complaint counter
			cl->pers.complaints++;

			num = g_complaintlimit.integer - cl->pers.complaints;

			if ( num <= 0 && !cl->pers.localClient ) {
				trap_DropClient( cl - level.clients, "kicked after too many complaints." );
				trap_SendServerCommand( ent - g_entities, "complaint -1" );
				return;
			}

			trap_SendServerCommand( cl->ps.clientNum, va( "print \"^1Warning^7: Complaint filed against you. [lof](%d [lon]until kicked[lof])\n\"", num ) );
			trap_SendServerCommand( ent - g_entities, "complaint -1" );
		} else {
			trap_SendServerCommand( ent - g_entities, "complaint -2" );
		}

		return;
	}
	// dhm

	// Reset this ent's complainEndTime so they can't send multiple complaints
	ent->client->pers.complaintEndTime = -1;
	ent->client->pers.complaintClient = -1;

	if ( !level.voteTime ) {
		trap_SendServerCommand( ent - g_entities, "print \"No vote in progress.\n\"" );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent - g_entities, "print \"Vote already cast.\n\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent - g_entities, "print \"Not allowed to vote as spectator.\n\"" );
		return;
	}

	trap_SendServerCommand( ent - g_entities, "print \"Vote cast.\n\"" );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteYes ) );
	} else {
		level.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteNo ) );
	}

	// a majority will be determined in G_CheckVote, which will also account
	// for players entering or leaving
}


qboolean G_canPickupMelee( gentity_t *ent ) {

// JPW NERVE -- no "melee" weapons in net play
	if ( g_gametype.integer >= GT_WOLF ) {
		return qfalse;
	}
// jpw

	if ( !( ent->client ) ) {
		return qfalse;  // hmm, shouldn't be too likely...

	}
	if ( !( ent->s.weapon ) ) {  // no weap, go ahead
		return qtrue;
	}

	if ( ent->client->ps.weaponstate == WEAPON_RELOADING ) {
		return qfalse;
	}

	if ( WEAPS_ONE_HANDED & ( 1 << ( ent->s.weapon ) ) ) {
		return qtrue;
	}

	return qfalse;
}




/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent ) {
	vec3_t origin, angles;
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent - g_entities, va( "print \"Cheats are not enabled on this server.\n\"" ) );
		return;
	}
	if ( trap_Argc() != 5 ) {
		trap_SendServerCommand( ent - g_entities, va( "print \"usage: setviewpos x y z yaw\n\"" ) );
		return;
	}

	VectorClear( angles );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	trap_Argv( 4, buffer, sizeof( buffer ) );
	angles[YAW] = atof( buffer );

	TeleportPlayer( ent, origin, angles );
}

/*
=================
Cmd_StartCamera_f
=================
*/
void Cmd_StartCamera_f( gentity_t *ent ) {

	if ( !CheatsOk( ent ) ) {
		return;
	}

	g_camEnt->r.svFlags |= SVF_PORTAL;
	g_camEnt->r.svFlags &= ~SVF_NOCLIENT;
	ent->client->cameraPortal = g_camEnt;
	ent->client->ps.eFlags |= EF_VIEWING_CAMERA;
	ent->s.eFlags |= EF_VIEWING_CAMERA;
}

/*
=================
Cmd_StopCamera_f
=================
*/
void Cmd_StopCamera_f( gentity_t *ent ) {

	if ( !CheatsOk( ent ) ) {
		return;
	}

	if ( ent->client->cameraPortal ) {
		// send a script event
		G_Script_ScriptEvent( ent->client->cameraPortal, "stopcam", "" );
		// go back into noclient mode
		ent->client->cameraPortal->r.svFlags |= SVF_NOCLIENT;
		ent->client->cameraPortal = NULL;
		ent->s.eFlags &= ~EF_VIEWING_CAMERA;
		ent->client->ps.eFlags &= ~EF_VIEWING_CAMERA;
	}
}

/*
=================
Cmd_SetCameraOrigin_f
=================
*/
void Cmd_SetCameraOrigin_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int i;

	if ( trap_Argc() != 4 ) {
		return;
	}

	VectorClear( ent->client->cameraOrigin );
	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		ent->client->cameraOrigin[i] = atof( buffer );
	}
}


// Rafael
/*
==================
Cmd_Activate_f
==================
*/
void Cmd_Activate_f( gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	gentity_t   *traceEnt;
	vec3_t forward, right, up, offset;
	static int oldactivatetime = 0;
	int activatetime = level.time;
	qboolean walking = qfalse;

	if ( ent->active ) {
		if ( ent->client->ps.persistant[PERS_HWEAPON_USE] ) {
			// DHM - Nerve :: Restore original position if current position is bad
			trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, MASK_PLAYERSOLID );
			if ( tr.startsolid ) {
				VectorCopy( ent->TargetAngles, ent->client->ps.origin );
				VectorCopy( ent->TargetAngles, ent->r.currentOrigin );
				ent->r.contents = CONTENTS_CORPSE;      // DHM - this will correct itself in ClientEndFrame
			}
			ent->client->ps.eFlags &= ~EF_MG42_ACTIVE;          // DHM - Nerve :: unset flag
			ent->client->ps.persistant[PERS_HWEAPON_USE] = 0;
			ent->active = qfalse;
			return;
		} else
		{
			ent->active = qfalse;
		}
	}

	if ( ent->client->pers.cmd.buttons & BUTTON_WALKING ) {
		walking = qtrue;
	}

	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	CalcMuzzlePointForActivate( ent, forward, right, up, offset );
	VectorMA( offset, 96, forward, end );

	trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER ) );

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	if ( tr.entityNum == ENTITYNUM_WORLD ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( traceEnt->classname ) {
		traceEnt->flags &= ~FL_SOFTACTIVATE;    // FL_SOFTACTIVATE will be set if the user is holding 'walk' key

		if ( traceEnt->s.eType == ET_ALARMBOX ) {
			trace_t trace;

			if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
				return;
			}

			memset( &trace, 0, sizeof( trace ) );

			if ( traceEnt->use ) {
				traceEnt->use( traceEnt, ent, 0 );
			}
		} else if ( traceEnt->s.eType == ET_ITEM )     {
			trace_t trace;

			if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
				return;
			}

			memset( &trace, 0, sizeof( trace ) );

			if ( traceEnt->touch ) {
				if ( ent->client->pers.autoActivate == PICKUP_ACTIVATE ) {
					ent->client->pers.autoActivate = PICKUP_FORCE;      //----(SA) force pickup
				}
				traceEnt->active = qtrue;
				traceEnt->touch( traceEnt, ent, &trace );
			}

		} else if ( ( Q_stricmp( traceEnt->classname, "misc_mg42" ) == 0 ) && traceEnt->active == qfalse )         {
			if (
				( traceEnt->r.currentOrigin[2] - ent->r.currentOrigin[2] < 40 ) &&
				( traceEnt->r.currentOrigin[2] - ent->r.currentOrigin[2] > 0 ) &&
				!infront( traceEnt, ent ) ) {
				//----(SA)	make sure the client isn't holding a hot potato
				gclient_t   *cl;
				cl = &level.clients[ ent->s.clientNum ];

				if ( !( cl->ps.grenadeTimeLeft ) && !( cl->ps.pm_flags & PMF_DUCKED )
					 && !( traceEnt->s.eFlags & EF_SMOKING ) && ( cl->ps.weapon != WP_SNIPERRIFLE ) ) { // JPW NERVE no mg42 while scoped in
					// DHM - Remember initial gun position to restore later
					vec3_t point;

					AngleVectors( traceEnt->s.apos.trBase, forward, NULL, NULL );
					VectorMA( traceEnt->r.currentOrigin, -36, forward, point );
					point[2] = ent->r.currentOrigin[2];

					// Save initial position
					VectorCopy( point, ent->TargetAngles );

					// Zero out velocity
					VectorCopy( vec3_origin, ent->client->ps.velocity );
					VectorCopy( vec3_origin, ent->s.pos.trDelta );

					traceEnt->active = qtrue;
					ent->active = qtrue;
					traceEnt->r.ownerNum = ent->s.number;
					VectorCopy( traceEnt->s.angles, traceEnt->TargetAngles );
					traceEnt->s.otherEntityNum = ent->s.number;

					cl->pmext.harc = traceEnt->harc;
					cl->pmext.varc = traceEnt->varc;
					VectorCopy( traceEnt->s.angles, cl->pmext.centerangles );
					cl->pmext.centerangles[PITCH] = AngleNormalize180( cl->pmext.centerangles[PITCH] );
					cl->pmext.centerangles[YAW] = AngleNormalize180( cl->pmext.centerangles[YAW] );
					cl->pmext.centerangles[ROLL] = AngleNormalize180( cl->pmext.centerangles[ROLL] );

					if ( !( ent->r.svFlags & SVF_CASTAI ) ) {
						G_UseTargets( traceEnt, ent );   //----(SA)	added for Mike so mounting an MG42 can be a trigger event (let me know if there's any issues with this)
					}
				}
			}
		} else if ( ( ( Q_stricmp( traceEnt->classname, "func_door" ) == 0 ) || ( Q_stricmp( traceEnt->classname, "func_door_rotating" ) == 0 ) ) )           {
			if ( walking ) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			G_TryDoor( traceEnt, ent, ent );      // (door,other,activator)
		} else if ( ( Q_stricmp( traceEnt->classname, "team_WOLF_checkpoint" ) == 0 ) )       {
			if ( traceEnt->count != ent->client->sess.sessionTeam ) {
				traceEnt->health++;
			}
		} else if ( ( Q_stricmp( traceEnt->classname, "func_button" ) == 0 )
					&& ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY )
					&& traceEnt->active == qfalse ) {
			Use_BinaryMover( traceEnt, ent, ent );
			traceEnt->active = qtrue;
		} else if ( !Q_stricmp( traceEnt->classname, "func_invisible_user" ) )     {
			if ( walking ) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			traceEnt->use( traceEnt, ent, ent );
		} else if ( !Q_stricmp( traceEnt->classname, "script_mover" ) )     {
			G_Script_ScriptEvent( traceEnt, "activate", ent->aiName );
		} else if ( !Q_stricmp( traceEnt->classname, "props_footlocker" ) )     {
			traceEnt->use( traceEnt, ent, ent );
		}
	}

	if ( activatetime > oldactivatetime + 1000 ) {
		oldactivatetime = activatetime;
	}
}

// Rafael WolfKick
//===================
//	Cmd_WolfKick
//===================

#define WOLFKICKDISTANCE    96
int Cmd_WolfKick_f( gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	gentity_t   *traceEnt;
	vec3_t forward, right, up, offset;
	gentity_t   *tent;
	static int oldkicktime = 0;
	int kicktime = level.time;
	qboolean solidKick = qfalse;    // don't play "hit" sound on a trigger unless it's an func_invisible_user

	int damage = 15;

	// DHM - Nerve :: No kick in wolf multiplayer
	if ( g_gametype.integer >= GT_WOLF ) {
		return 0;
	}

	if ( ent->client->ps.leanf ) {
		return 0;   // no kick when leaning

	}
	if ( oldkicktime > kicktime ) {
		return ( 0 );
	} else {
		oldkicktime = kicktime + 1000;
	}

	// play the anim
	BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_KICK, qfalse, qtrue );

	ent->client->ps.persistant[PERS_WOLFKICK] = 1;

	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	CalcMuzzlePointForActivate( ent, forward, right, up, offset );

	// note to self: we need to determine the usable distance for wolf
	VectorMA( offset, WOLFKICKDISTANCE, forward, end );

	trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER ) );

	if ( tr.surfaceFlags & SURF_NOIMPACT || tr.fraction == 1.0 ) {
		tent = G_TempEntity( tr.endpos, EV_WOLFKICK_MISS );
		tent->s.eventParm = ent->s.number;
		return ( 1 );
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( !ent->melee ) { // because we dont want you to open a door with a prop
		if ( ( Q_stricmp( traceEnt->classname, "func_door_rotating" ) == 0 )
			 && ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY )
			 && traceEnt->active == qfalse ) {
			if ( traceEnt->key < 0 ) { // door force locked
				//----(SA)	play kick "hit" sound
				tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_WALL );
				tent->s.otherEntityNum = ent->s.number;	\
				//----(SA)	end

				AICast_AudibleEvent( ent->s.clientNum, tr.endpos, HEAR_RANGE_DOOR_KICKLOCKED ); // "someone kicked a locked door near me!"

				if ( traceEnt->soundPos3 ) {
					G_AddEvent( traceEnt, EV_GENERAL_SOUND, traceEnt->soundPos3 );
				} else {
					G_AddEvent( traceEnt, EV_GENERAL_SOUND, G_SoundIndex( "sound/movers/doors/default_door_locked.wav" ) );
				}
				return 1;   //----(SA)	changed.  shows boot for locked doors
			}

			if ( traceEnt->key > 0 ) { // door requires key
				gitem_t *item = BG_FindItemForKey( traceEnt->key, 0 );
				if ( !( ent->client->ps.stats[STAT_KEYS] & ( 1 << item->giTag ) ) ) {
					//----(SA)	play kick "hit" sound
					tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_WALL );
					tent->s.otherEntityNum = ent->s.number;	\
					//----(SA)	end

					AICast_AudibleEvent( ent->s.clientNum, tr.endpos, HEAR_RANGE_DOOR_KICKLOCKED ); // "someone kicked a locked door near me!"

					// player does not have key
					if ( traceEnt->soundPos3 ) {
						G_AddEvent( traceEnt, EV_GENERAL_SOUND, traceEnt->soundPos3 );
					} else {
						G_AddEvent( traceEnt, EV_GENERAL_SOUND, G_SoundIndex( "sound/movers/doors/default_door_locked.wav" ) );
					}
					return 1;   //----(SA)	changed.  shows boot animation for locked doors
				}
			}

			if ( traceEnt->teammaster && traceEnt->team && traceEnt != traceEnt->teammaster ) {
				traceEnt->teammaster->active = qtrue;
				traceEnt->teammaster->flags |= FL_KICKACTIVATE;
				Use_BinaryMover( traceEnt->teammaster, ent, ent );
				G_UseTargets( traceEnt->teammaster, ent );
			} else
			{
				traceEnt->active = qtrue;
				traceEnt->flags |= FL_KICKACTIVATE;
				Use_BinaryMover( traceEnt, ent, ent );
				G_UseTargets( traceEnt, ent );
			}
		} else if ( ( Q_stricmp( traceEnt->classname, "func_button" ) == 0 )
					&& ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY )
					&& traceEnt->active == qfalse ) {
			Use_BinaryMover( traceEnt, ent, ent );
			traceEnt->active = qtrue;

		} else if ( !Q_stricmp( traceEnt->classname, "func_invisible_user" ) )     {
			traceEnt->flags |= FL_KICKACTIVATE;     // so cell doors know they were kicked
													// It doesn't hurt to pass this along since only ent use() funcs who care about it will check.
													// However, it may become handy to put a "KICKABLE" or "NOTKICKABLE" flag on the invisible_user
			traceEnt->use( traceEnt, ent, ent );
			traceEnt->flags &= ~FL_KICKACTIVATE;    // reset

			solidKick = qtrue;  //----(SA)
		} else if ( !Q_stricmp( traceEnt->classname, "props_flippy_table" ) && traceEnt->use )       {
			traceEnt->use( traceEnt, ent, ent );
		}
	}

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards( tr.endpos, offset );

	// send bullet impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
		if ( LogAccuracyHit( traceEnt, ent ) ) {
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	} else {
		// Ridah, bullet impact should reflect off surface
		vec3_t reflect;
		float dot;

		if ( traceEnt->r.contents >= 0 && ( traceEnt->r.contents & CONTENTS_TRIGGER ) && !solidKick ) {
			tent = G_TempEntity( tr.endpos, EV_WOLFKICK_MISS ); // (SA) don't play the "hit" sound if you kick most triggers
		} else {
			tent = G_TempEntity( tr.endpos, EV_WOLFKICK_HIT_WALL );
		}


		dot = DotProduct( forward, tr.plane.normal );
		VectorMA( forward, -2 * dot, tr.plane.normal, reflect );
		VectorNormalize( reflect );

		tent->s.eventParm = DirToByte( reflect );
		// done.

		if ( ent->melee ) {
			ent->active = qfalse;
			ent->melee->health = 0;
		}
	}

	tent->s.otherEntityNum = ent->s.number;

	// try to swing chair
	if ( traceEnt->takedamage ) {

		if ( ent->melee ) {
			ent->active = qfalse;
			ent->melee->health = 0;
			ent->client->ps.eFlags &= ~EF_MELEE_ACTIVE;

		}

		G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_KICKED );   //----(SA)	modified
	}

	return ( 1 );
}
// done

/*
============================
Cmd_ClientMonsterSlickAngle
============================
*/
/*
void Cmd_ClientMonsterSlickAngle (gentity_t *clent) {

	char s[MAX_STRING_CHARS];
	int	entnum;
	int angle;
	gentity_t *ent;
	vec3_t	dir, kvel;
	vec3_t	forward;

	if (trap_Argc() != 3) {
		G_Printf( "ClientDamage command issued with incorrect number of args\n" );
	}

	trap_Argv( 1, s, sizeof( s ) );
	entnum = atoi(s);
	ent = &g_entities[entnum];

	trap_Argv( 2, s, sizeof( s ) );
	angle = atoi(s);

	// sanity check (also protect from cheaters)
	if (g_gametype.integer != GT_SINGLE_PLAYER && entnum != clent->s.number) {
		trap_DropClient( clent->s.number, "Dropped due to illegal ClientMonsterSlick command\n" );
		return;
	}

	VectorClear (dir);
	dir[YAW] = angle;
	AngleVectors (dir, forward, NULL, NULL);

	VectorScale (forward, 32, kvel);
	VectorAdd (ent->client->ps.velocity, kvel, ent->client->ps.velocity);
}
*/

// NERVE - SMF
/*
============
ClientDamage
============
*/
void ClientDamage( gentity_t *clent, int entnum, int enemynum, int id ) {
	gentity_t *enemy, *ent;
	vec3_t vec;

	ent = &g_entities[entnum];

	enemy = &g_entities[enemynum];

	// NERVE - SMF - took this out, this is causing more problems than its helping
	//  Either a new way has to be found, or this check needs to change.
	// sanity check (also protect from cheaters)
//	if (g_gametype.integer != GT_SINGLE_PLAYER && entnum != clent->s.number) {
//		trap_DropClient( clent->s.number, "Dropped due to illegal ClientDamage command\n" );
//		return;
//	}
	// -NERVE - SMF

	// if the attacker can't see the target, then don't allow damage
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		// TTimo it can happen that enemy->client == NULL
		// see Changelog 09/22/2001
		if ( ( enemy->client ) && ( !CanDamage( ent, enemy->client->ps.origin ) ) ) {
			return; // don't allow damage
		}
	}

	switch ( id ) {
	case CLDMG_SPIRIT:
		if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
			G_Damage( ent, enemy, enemy, vec3_origin, vec3_origin, 3, DAMAGE_NO_KNOCKBACK, MOD_ZOMBIESPIRIT );
		}
		break;
	case CLDMG_BOSS1LIGHTNING:
		if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
			break;
		}
		if ( ent->takedamage ) {
			VectorSubtract( ent->r.currentOrigin, enemy->r.currentOrigin, vec );
			VectorNormalize( vec );
			G_Damage( ent, enemy, enemy, vec, ent->r.currentOrigin, 6 + rand() % 3, 0, MOD_LIGHTNING );
		}
		break;
	case CLDMG_TESLA:
		// do some cheat protection
		if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
			if ( enemy->s.weapon != WP_TESLA ) {
				break;
			}
			if ( !( enemy->client->buttons & BUTTON_ATTACK ) ) {
				break;
			}
			//if ( AICast_GetCastState( enemy->s.number )->lastWeaponFiredWeaponNum != WP_TESLA )
			//	break;
			//if ( AICast_GetCastState( enemy->s.number )->lastWeaponFired < level.time - 400 )
			//	break;
		}

		if (    ( ent->aiCharacter == AICHAR_PROTOSOLDIER ) ||
				( ent->aiCharacter == AICHAR_SUPERSOLDIER ) ||
				( ent->aiCharacter == AICHAR_LOPER ) ||
				( ent->aiCharacter >= AICHAR_STIMSOLDIER1 && ent->aiCharacter <= AICHAR_STIMSOLDIER3 ) ) {
			break;
		}

		if ( ent->takedamage /*&& !AICast_NoFlameDamage(ent->s.number)*/ ) {
			VectorSubtract( ent->r.currentOrigin, enemy->r.currentOrigin, vec );
			VectorNormalize( vec );
			G_Damage( ent, enemy, enemy, vec, ent->r.currentOrigin, 3, 0, MOD_LIGHTNING );
		}
		break;
	case CLDMG_FLAMETHROWER:
		// do some cheat protection
/*  JPW NERVE pulled flamethrower client damage completely
		if (g_gametype.integer != GT_SINGLE_PLAYER) {
			if ( enemy->s.weapon != WP_FLAMETHROWER )
				break;
//			if ( !(enemy->client->buttons & BUTTON_ATTACK) ) // JPW NERVE flames should be able to damage while puffs are active
//				break;
		} else {
			// this is required for Zombie flame attack
			//if ((enemy->aiCharacter == AICHAR_ZOMBIE) && !AICast_VisibleFromPos( enemy->r.currentOrigin, enemy->s.number, ent->r.currentOrigin, ent->s.number, qfalse ))
			//	break;
		}

		if ( ent->takedamage && !AICast_NoFlameDamage(ent->s.number) ) {
			#define	FLAME_THRESHOLD	50
			int damage = 5;

			// RF, only do damage once they start burning
			//if (ent->health > 0)	// don't explode from flamethrower
			//	G_Damage( traceEnt, ent, ent, forward, tr.endpos, 1, 0, MOD_LIGHTNING);

			// now check the damageQuota to see if we should play a pain animation
			// first reduce the current damageQuota with time
			if (ent->flameQuotaTime && ent->flameQuota > 0) {
				ent->flameQuota -= (int)(((float)(level.time - ent->flameQuotaTime)/1000) * (float)damage/2.0);
				if (ent->flameQuota < 0)
					ent->flameQuota = 0;
			}

			// add the new damage
			ent->flameQuota += damage;
			ent->flameQuotaTime = level.time;

			// Ridah, make em burn
			if (ent->client && ( !(ent->r.svFlags & SVF_CASTAI) || ent->health <= 0 || ent->flameQuota > FLAME_THRESHOLD)) {				if (ent->s.onFireEnd < level.time)
					ent->s.onFireStart = level.time;
				if (ent->health <= 0 || !(ent->r.svFlags & SVF_CASTAI) || (g_gametype.integer != GT_SINGLE_PLAYER)) {
					if (ent->r.svFlags & SVF_CASTAI) {
						ent->s.onFireEnd = level.time + 6000;
					} else {
						ent->s.onFireEnd = level.time + FIRE_FLASH_TIME;
					}
				} else {
					ent->s.onFireEnd = level.time + 99999;	// make sure it goes for longer than they need to die
				}
				ent->flameBurnEnt = enemy->s.number;
				// add to playerState for client-side effect
				ent->client->ps.onFireStart = level.time;
			}
		}
*/
		break;
	}
}
// -NERVE - SMF

/*
============
Cmd_ClientDamage_f
============
*/
void Cmd_ClientDamage_f( gentity_t *clent ) {
	char s[MAX_STRING_CHARS];
	int entnum, id, enemynum;

	if ( trap_Argc() != 4 ) {
		G_Printf( "ClientDamage command issued with incorrect number of args\n" );
	}

	trap_Argv( 1, s, sizeof( s ) );
	entnum = atoi( s );

	trap_Argv( 2, s, sizeof( s ) );
	enemynum = atoi( s );

	trap_Argv( 3, s, sizeof( s ) );
	id = atoi( s );

	ClientDamage( clent, entnum, enemynum, id );
}

/*
==============
Cmd_EntityCount_f
==============
*/
#define AITEAM_NAZI     0
#define AITEAM_ALLIES   1
#define AITEAM_MONSTER  2
void Cmd_EntityCount_f( gentity_t *ent ) {
	if ( !g_cheats.integer ) {
		return;
	}

	G_Printf( "entity count = %i\n", level.num_entities );

	{
		int kills[2];
		int nazis[2];
		int monsters[2];
		int i;
		gentity_t *ent;

		// count kills
		kills[0] = kills[1] = 0;
		nazis[0] = nazis[1] = 0;
		monsters[0] = monsters[1] = 0;
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			ent = &g_entities[i];

			if ( !ent->inuse ) {
				continue;
			}

			if ( !( ent->r.svFlags & SVF_CASTAI ) ) {
				continue;
			}

			if ( ent->aiTeam == AITEAM_ALLIES ) {
				continue;
			}

			kills[1]++;

			if ( ent->health <= 0 ) {
				kills[0]++;
			}

			if ( ent->aiTeam == AITEAM_NAZI ) {
				nazis[1]++;
				if ( ent->health <= 0 ) {
					nazis[0]++;
				}
			} else {
				monsters[1]++;
				if ( ent->health <= 0 ) {
					monsters[0]++;
				}
			}
		}
		G_Printf( "kills %i/%i nazis %i/%i monsters %i/%i \n",kills[0], kills[1], nazis[0], nazis[1], monsters[0], monsters[1] );

	}
}

// NERVE - SMF
/*
============
Cmd_SetSpawnPoint_f
============
*/
void Cmd_SetSpawnPoint_f( gentity_t *clent ) {
	char arg[MAX_TOKEN_CHARS];
//	int		spawnIndex;

	if ( trap_Argc() != 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	if ( clent->client ) { // JPW NERVE
		clent->client->sess.spawnObjectiveIndex = atoi( arg ); // JPW NERVE
	}
}
// -NERVE - SMF

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char cmd[MAX_TOKEN_CHARS];

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;     // not fully in game yet
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if ( Q_stricmp( cmd, "say" ) == 0 ) {
		Cmd_Say_f( ent, SAY_ALL, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "say_team" ) == 0 ) {
		Cmd_Say_f( ent, SAY_TEAM, qfalse );
		return;
	}
	// NERVE - SMF
	if ( Q_stricmp( cmd, "say_limbo" ) == 0 ) {
		Cmd_Say_f( ent, SAY_LIMBO, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "vsay" ) == 0 ) {
		Cmd_Voice_f( ent, SAY_ALL, qfalse, qfalse );
		return;
	}
	if ( Q_stricmp( cmd, "vsay_team" ) == 0 ) {
		Cmd_Voice_f( ent, SAY_TEAM, qfalse, qfalse );
		return;
	}
	// -NERVE - SMF

	if ( Q_stricmp( cmd, "tell" ) == 0 ) {
		Cmd_Tell_f( ent );
		return;
	}
	if ( Q_stricmp( cmd, "score" ) == 0 ) {
		Cmd_Score_f( ent );
		return;
	}

	// NERVE - SMF - moved this here so current/new players can set team during scoreboard win
	if ( Q_stricmp( cmd, "team" ) == 0 ) {
		Cmd_Team_f( ent );
		return;
	}

	// ignore all other commands when at intermission
	if ( level.intermissiontime ) {
//		Cmd_Say_f (ent, qfalse, qtrue);			// NERVE - SMF - we don't want to spam the clients with this.
		return;
	}

	if ( Q_stricmp( cmd, "give" ) == 0 ) {
		Cmd_Give_f( ent );
	} else if ( Q_stricmp( cmd, "god" ) == 0 )  {
		Cmd_God_f( ent );
	} else if ( Q_stricmp( cmd, "nofatigue" ) == 0 )  {
		Cmd_Nofatigue_f( ent );
	} else if ( Q_stricmp( cmd, "notarget" ) == 0 )  {
		Cmd_Notarget_f( ent );
	} else if ( Q_stricmp( cmd, "noclip" ) == 0 )  {
		Cmd_Noclip_f( ent );
	} else if ( Q_stricmp( cmd, "kill" ) == 0 )  {
		Cmd_Kill_f( ent );
	} else if ( Q_stricmp( cmd, "levelshot" ) == 0 )  {
		Cmd_LevelShot_f( ent );
	} else if ( Q_stricmp( cmd, "follow" ) == 0 )  {
		Cmd_Follow_f( ent );
	} else if ( Q_stricmp( cmd, "follownext" ) == 0 )  {
		Cmd_FollowCycle_f( ent, 1 );
	} else if ( Q_stricmp( cmd, "followprev" ) == 0 )  {
		Cmd_FollowCycle_f( ent, -1 );
	}
//	else if (Q_stricmp (cmd, "team") == 0)		// NERVE - SMF - moved above intermission check
//		Cmd_Team_f (ent);
	else if ( Q_stricmp( cmd, "where" ) == 0 ) {
		Cmd_Where_f( ent );
	} else if ( Q_stricmp( cmd, "callvote" ) == 0 )  {
		Cmd_CallVote_f( ent );
	} else if ( Q_stricmp( cmd, "vote" ) == 0 )  {
		Cmd_Vote_f( ent );
	} else if ( Q_stricmp( cmd, "gc" ) == 0 )  {
		Cmd_GameCommand_f( ent );
	}
//	else if (Q_stricmp (cmd, "startCamera") == 0)
//		Cmd_StartCamera_f( ent );
//	else if (Q_stricmp (cmd, "stopCamera") == 0)
//		Cmd_StopCamera_f( ent );
//	else if (Q_stricmp (cmd, "setCameraOrigin") == 0)
//		Cmd_SetCameraOrigin_f( ent );
	else if ( Q_stricmp( cmd, "setviewpos" ) == 0 ) {
		Cmd_SetViewpos_f( ent );
	} else if ( Q_stricmp( cmd, "entitycount" ) == 0 )  {
		Cmd_EntityCount_f( ent );
	} else if ( Q_stricmp( cmd, "setspawnpt" ) == 0 )  {
		Cmd_SetSpawnPoint_f( ent );
	} else {
		trap_SendServerCommand( clientNum, va( "print \"unknown cmd[lof] %s\n\"", cmd ) );
	}
}
