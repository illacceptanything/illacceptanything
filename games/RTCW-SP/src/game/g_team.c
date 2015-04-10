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


#include <limits.h>

#include "g_local.h"

typedef struct teamgame_s
{
	float last_flag_capture;
	int last_capture_team;
} teamgame_t;

teamgame_t teamgame;

void Team_InitGame( void ) {
	memset( &teamgame, 0, sizeof teamgame );
}

int OtherTeam( int team ) {
	if ( team == TEAM_RED ) {
		return TEAM_BLUE;
	} else if ( team == TEAM_BLUE )  {
		return TEAM_RED;
	}
	return team;
}

const char *TeamName( int team ) {
	if ( team == TEAM_RED ) {
		return "RED";
	} else if ( team == TEAM_BLUE )  {
		return "BLUE";
	} else if ( team == TEAM_SPECTATOR )  {
		return "SPECTATOR";
	}
	return "FREE";
}

const char *OtherTeamName( int team ) {
	if ( team == TEAM_RED ) {
		return "BLUE";
	} else if ( team == TEAM_BLUE )  {
		return "RED";
	} else if ( team == TEAM_SPECTATOR )  {
		return "SPECTATOR";
	}
	return "FREE";
}

const char *TeamColorString( int team ) {
	if ( team == TEAM_RED ) {
		return S_COLOR_RED;
	} else if ( team == TEAM_BLUE )  {
		return S_COLOR_BLUE;
	} else if ( team == TEAM_SPECTATOR )  {
		return S_COLOR_YELLOW;
	}
	return S_COLOR_WHITE;
}

// NULL for everyone
void QDECL PrintMsg( gentity_t *ent, const char *fmt, ... ) {
	char msg[1024];
	va_list argptr;
	char        *p;

	va_start( argptr,fmt );
	if ( vsprintf( msg, fmt, argptr ) > sizeof( msg ) ) {
		G_Error( "PrintMsg overrun" );
	}
	va_end( argptr );

	// double quotes are bad
	while ( ( p = strchr( msg, '"' ) ) != NULL )
		*p = '\'';

	trap_SendServerCommand( ( ( ent == NULL ) ? -1 : ent - g_entities ), va( "print \"%s\"", msg ) );
}

/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 ) {
	if ( !ent1->client || !ent2->client ) {
		return qfalse;
	}

	if ( g_gametype.integer < GT_TEAM ) {
		return qfalse;
	}

	if ( ent1->client->sess.sessionTeam == ent2->client->sess.sessionTeam ) {
		return qtrue;
	}

	return qfalse;
}


/*
================
Team_FragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumlative.  You get one, they are in importance
order.
================
*/
void Team_FragBonuses( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker ) {
	int i;
	gentity_t *ent;
	int flag_pw, enemy_flag_pw;
	int otherteam;
	gentity_t *flag, *carrier = NULL;
	char *c;
	vec3_t v1, v2;
	int team;

	// no bonus for fragging yourself
	if ( !targ->client || !attacker->client || targ == attacker ) {
		return;
	}

	team = targ->client->sess.sessionTeam;
	otherteam = OtherTeam( targ->client->sess.sessionTeam );
	if ( otherteam < 0 ) {
		return; // whoever died isn't on a team

	}
	// same team, if the flag at base, check to he has the enemy flag
	if ( team == TEAM_RED ) {
		flag_pw = PW_REDFLAG;
		enemy_flag_pw = PW_BLUEFLAG;
	} else {
		flag_pw = PW_BLUEFLAG;
		enemy_flag_pw = PW_REDFLAG;
	}

	// did the attacker frag the flag carrier?
	if ( targ->client->ps.powerups[enemy_flag_pw] ) {
		attacker->client->pers.teamState.lastfraggedcarrier = level.time;
		AddScore( attacker, CTF_FRAG_CARRIER_BONUS );
		attacker->client->pers.teamState.fragcarrier++;
		PrintMsg( NULL, "%s" S_COLOR_WHITE " fragged %s's flag carrier!\n",
				  attacker->client->pers.netname, TeamName( team ) );

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for ( i = 0; i < g_maxclients.integer; i++ ) {
			ent = g_entities + i;
			if ( ent->inuse && ent->client->sess.sessionTeam == otherteam ) {
				ent->client->pers.teamState.lasthurtcarrier = 0;
			}
		}
		return;
	}

	if ( targ->client->pers.teamState.lasthurtcarrier &&
		 level.time - targ->client->pers.teamState.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		 !attacker->client->ps.powerups[flag_pw] ) {
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		AddScore( attacker, CTF_CARRIER_DANGER_PROTECT_BONUS );

		attacker->client->pers.teamState.carrierdefense++;
		team = attacker->client->sess.sessionTeam;
		PrintMsg( NULL, "%s" S_COLOR_WHITE " defends %s's flag carrier against an agressive enemy\n",
				  attacker->client->pers.netname, TeamName( team ) );
		return;
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

	// find the flag
	switch ( attacker->client->sess.sessionTeam ) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	default:
		return;
	}

	flag = NULL;
	while ( ( flag = G_Find( flag, FOFS( classname ), c ) ) != NULL ) {
		if ( !( flag->flags & FL_DROPPED_ITEM ) ) {
			break;
		}
	}

	if ( !flag ) {
		return; // can't find attacker's flag

	}
	// find attacker's team's flag carrier
	for ( i = 0; i < g_maxclients.integer; i++ ) {
		carrier = g_entities + i;
		if ( carrier->inuse && carrier->client->ps.powerups[flag_pw] ) {
			break;
		}
		carrier = NULL;
	}

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	VectorSubtract( targ->s.origin, flag->s.origin, v1 );
	VectorSubtract( attacker->s.origin, flag->s.origin, v2 );

	if ( ( VectorLength( v1 ) < CTF_TARGET_PROTECT_RADIUS ||
		   VectorLength( v2 ) < CTF_TARGET_PROTECT_RADIUS ||
		   CanDamage( flag, targ->s.origin ) || CanDamage( flag, attacker->s.origin ) ) &&
		 attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam ) {
		// we defended the base flag
		AddScore( attacker, CTF_FLAG_DEFENSE_BONUS );
		attacker->client->pers.teamState.basedefense++;
		if ( !flag->r.contents ) {
			PrintMsg( NULL, "%s" S_COLOR_WHITE " defends the %s base.\n",
					  attacker->client->pers.netname,
					  TeamName( attacker->client->sess.sessionTeam ) );
		} else {
			PrintMsg( NULL, "%s" S_COLOR_WHITE " defends the %s flag.\n",
					  attacker->client->pers.netname,
					  TeamName( attacker->client->sess.sessionTeam ) );
		}
		return;
	}

	if ( carrier && carrier != attacker ) {
		VectorSubtract( targ->s.origin, carrier->s.origin, v1 );
		VectorSubtract( attacker->s.origin, carrier->s.origin, v1 );

		if ( VectorLength( v1 ) < CTF_ATTACKER_PROTECT_RADIUS ||
			 VectorLength( v2 ) < CTF_ATTACKER_PROTECT_RADIUS ||
			 CanDamage( carrier, targ->s.origin ) || CanDamage( carrier, attacker->s.origin ) ) {
			AddScore( attacker, CTF_CARRIER_PROTECT_BONUS );
			attacker->client->pers.teamState.carrierdefense++;
			PrintMsg( NULL, "%s" S_COLOR_WHITE " defends the %s's flag carrier.\n",
					  attacker->client->pers.netname,
					  TeamName( attacker->client->sess.sessionTeam ) );
			return;
		}
	}
}

/*
================
Team_CheckHurtCarrier

Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses for assistance to flag
carrier defense.
================
*/
void Team_CheckHurtCarrier( gentity_t *targ, gentity_t *attacker ) {
	int flag_pw;

	if ( !targ->client || !attacker->client ) {
		return;
	}

	if ( targ->client->sess.sessionTeam == TEAM_RED ) {
		flag_pw = PW_BLUEFLAG;
	} else {
		flag_pw = PW_REDFLAG;
	}

	if ( targ->client->ps.powerups[flag_pw] &&
		 targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam ) {
		attacker->client->pers.teamState.lasthurtcarrier = level.time;
	}
}


gentity_t *Team_ResetFlag( int team ) {
	char *c;
	gentity_t *ent, *rent = NULL;

	switch ( team ) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	default:
		return NULL;
	}

	ent = NULL;
	while ( ( ent = G_Find( ent, FOFS( classname ), c ) ) != NULL ) {
		if ( ent->flags & FL_DROPPED_ITEM ) {
			G_FreeEntity( ent );
		} else {
			rent = ent;
			RespawnItem( ent );
		}
	}

	return rent;
}

void Team_ResetFlags( void ) {
	Team_ResetFlag( TEAM_RED );
	Team_ResetFlag( TEAM_BLUE );
}

void Team_ReturnFlagSound( gentity_t *ent, int team ) {
	// play powerup spawn sound to all clients
	gentity_t   *te;

	if ( ent == NULL ) {
		G_Printf( "Warning:  NULL passed to Team_ReturnFlagSound\n" );
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
	te->s.eventParm = G_SoundIndex( team == TEAM_RED ?
									"sound/teamplay/flagret_red.wav" :
									"sound/teamplay/flagret_blu.wav" );
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_ReturnFlag( int team ) {
	Team_ReturnFlagSound( Team_ResetFlag( team ), team );
	PrintMsg( NULL, "The %s flag has returned!\n", TeamName( team ) );
}

void Team_FreeEntity( gentity_t *ent ) {
	if ( ent->item->giTag == PW_REDFLAG ) {
		Team_ReturnFlag( TEAM_RED );
	} else if ( ent->item->giTag == PW_BLUEFLAG ) {
		Team_ReturnFlag( TEAM_BLUE );
	}
}

/*
==============
Team_DroppedFlagThink

Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned when they time out
==============
*/
void Team_DroppedFlagThink( gentity_t *ent ) {
	if ( ent->item->giTag == PW_REDFLAG ) {
		Team_ReturnFlagSound( Team_ResetFlag( TEAM_RED ), TEAM_RED );
	} else if ( ent->item->giTag == PW_BLUEFLAG ) {
		Team_ReturnFlagSound( Team_ResetFlag( TEAM_BLUE ), TEAM_BLUE );
	}
	// Reset Flag will delete this entity
}

int Team_TouchOurFlag( gentity_t *ent, gentity_t *other, int team ) {
	int i;
	gentity_t *player;
	gclient_t *cl = other->client;
	int our_flag, enemy_flag;
	gentity_t   *te;

	if ( cl->sess.sessionTeam == TEAM_RED ) {
		our_flag = PW_REDFLAG;
		enemy_flag = PW_BLUEFLAG;
	} else {
		our_flag = PW_BLUEFLAG;
		enemy_flag = PW_REDFLAG;
	}

	if ( ent->flags & FL_DROPPED_ITEM ) {
		// hey, its not home.  return it by teleporting it back
		PrintMsg( NULL, "%s" S_COLOR_WHITE " returned the %s flag!\n",
				  cl->pers.netname, TeamName( team ) );
		AddScore( other, CTF_RECOVERY_BONUS );
		other->client->pers.teamState.flagrecovery++;
		other->client->pers.teamState.lastreturnedflag = level.time;
		//ResetFlag will remove this entity!  We must return zero
		Team_ReturnFlagSound( Team_ResetFlag( team ), team );
		return 0;
	}

	// the flag is at home base.  if the player has the enemy
	// flag, he's just won!
	if ( !cl->ps.powerups[enemy_flag] ) {
		return 0; // We don't have the flag

	}
	PrintMsg( NULL, "%s" S_COLOR_WHITE " captured the %s flag!\n",
			  cl->pers.netname, TeamName( OtherTeam( team ) ) );

	cl->ps.powerups[enemy_flag] = 0;

	teamgame.last_flag_capture = level.time;
	teamgame.last_capture_team = team;

	// Increase the team's score
	level.teamScores[ other->client->sess.sessionTeam ]++;

	other->client->pers.teamState.captures++;

	// other gets another 10 frag bonus
	AddScore( other, CTF_CAPTURE_BONUS );

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
	te->s.eventParm = G_SoundIndex( our_flag == PW_REDFLAG ?
									"sound/teamplay/flagcap_red.wav" :
									"sound/teamplay/flagcap_blu.wav" );
	te->r.svFlags |= SVF_BROADCAST;

	// Ok, let's do the player loop, hand out the bonuses
	for ( i = 0; i < g_maxclients.integer; i++ ) {
		player = &g_entities[i];
		if ( !player->inuse ) {
			continue;
		}

		if ( player->client->sess.sessionTeam !=
			 cl->sess.sessionTeam ) {
			player->client->pers.teamState.lasthurtcarrier = -5;
		} else if ( player->client->sess.sessionTeam ==
					cl->sess.sessionTeam ) {
			if ( player != other ) {
				AddScore( player, CTF_CAPTURE_BONUS );
			}
			// award extra points for capture assists
			if ( player->client->pers.teamState.lastreturnedflag +
				 CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time ) {
				PrintMsg( NULL,
						  "%s" S_COLOR_WHITE " gets an assist for returning the %s flag!\n",
						  player->client->pers.netname,
						  TeamName( team ) );
				AddScore( player, CTF_RETURN_FLAG_ASSIST_BONUS );
				other->client->pers.teamState.assists++;
			}
			if ( player->client->pers.teamState.lastfraggedcarrier +
				 CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time ) {
				PrintMsg( NULL, "%s" S_COLOR_WHITE " gets an assist for fragging the %s flag carrier!\n",
						  player->client->pers.netname,
						  TeamName( OtherTeam( team ) ) );
				AddScore( player, CTF_FRAG_CARRIER_ASSIST_BONUS );
				other->client->pers.teamState.assists++;
			}
		}
	}
	Team_ResetFlags();

	CalculateRanks();

	return 0; // Do not respawn this automatically
}

int Team_TouchEnemyFlag( gentity_t *ent, gentity_t *other, int team ) {
	gclient_t *cl = other->client;

	// hey, its not our flag, pick it up
	PrintMsg( NULL, "%s" S_COLOR_WHITE " got the %s flag!\n",
			  other->client->pers.netname, TeamName( team ) );
	AddScore( other, CTF_FLAG_BONUS );

	if ( team == TEAM_RED ) {
		cl->ps.powerups[PW_REDFLAG] = INT_MAX; // flags never expire
	} else {
		cl->ps.powerups[PW_BLUEFLAG] = INT_MAX; // flags never expire

	}
	cl->pers.teamState.flagsince = level.time;

	return -1; // Do not respawn this automatically, but do delete it if it was FL_DROPPED
}

int Pickup_Team( gentity_t *ent, gentity_t *other ) {
	int team;
	gclient_t *cl = other->client;

	// figure out what team this flag is
	if ( strcmp( ent->classname, "team_CTF_redflag" ) == 0 ) {
		team = TEAM_RED;
	} else if ( strcmp( ent->classname, "team_CTF_blueflag" ) == 0 )   {
		team = TEAM_BLUE;
	} else {
		PrintMsg( other, "Don't know what team the flag is on.\n" );
		return 0;
	}

// JPW NERVE -- set flag model in carrying entity if multiplayer and flagmodel is set
	if ( g_gametype.integer == GT_WOLF ) {
		other->s.otherEntityNum2 = ent->s.modelindex2;
	}
// jpw

	return ( ( team == cl->sess.sessionTeam ) ?
			 Team_TouchOurFlag : Team_TouchEnemyFlag )
						( ent, other, team );
}

/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
gentity_t *Team_GetLocation( gentity_t *ent ) {
	gentity_t       *eloc, *best;
	float bestlen, len;
	vec3_t origin;

	best = NULL;
	bestlen = 3 * 8192.0 * 8192.0;

	VectorCopy( ent->r.currentOrigin, origin );

	for ( eloc = level.locationHead; eloc; eloc = eloc->nextTrain ) {
		len = ( origin[0] - eloc->r.currentOrigin[0] ) * ( origin[0] - eloc->r.currentOrigin[0] )
			  + ( origin[1] - eloc->r.currentOrigin[1] ) * ( origin[1] - eloc->r.currentOrigin[1] )
			  + ( origin[2] - eloc->r.currentOrigin[2] ) * ( origin[2] - eloc->r.currentOrigin[2] );

		if ( len > bestlen ) {
			continue;
		}

		if ( !trap_InPVS( origin, eloc->r.currentOrigin ) ) {
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}


/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
qboolean Team_GetLocationMsg( gentity_t *ent, char *loc, int loclen ) {
	gentity_t *best;

	best = Team_GetLocation( ent );

	if ( !best ) {
		return qfalse;
	}

	if ( best->count ) {
		if ( best->count < 0 ) {
			best->count = 0;
		}
		if ( best->count > 7 ) {
			best->count = 7;
		}
		Com_sprintf( loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message );
	} else {
		Com_sprintf( loc, loclen, "%s", best->message );
	}

	return qtrue;
}


/*---------------------------------------------------------------------------*/

// JPW NERVE
/*
=======================
FindFarthestObjectiveIndex

pick MP objective farthest from passed in vector, return table index
=======================
*/
int FindFarthestObjectiveIndex( vec3_t source ) {
	int i,j = 0;
	float dist = 0,tdist;
	vec3_t tmp;
//	int	cs_obj = CS_MULTI_SPAWNTARGETS;
//	char	cs[MAX_STRING_CHARS];
//	char *objectivename;

	for ( i = 0; i < level.numspawntargets; i++ ) {
		VectorSubtract( level.spawntargets[i],source,tmp );
		tdist = VectorLength( tmp );
		if ( tdist > dist ) {
			dist = tdist;
			j = i;
		}
	}

/*
	cs_obj += j;
	trap_GetConfigstring( cs_obj, cs, sizeof(cs) );
	objectivename = Info_ValueForKey( cs, "spawn_targ");

	G_Printf("got furthest dist (%f) at point %d (%s) of %d\n",dist,j,objectivename,i);
*/

	return j;
}
// jpw

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_TEAM_SPAWN_POINTS   16
gentity_t *SelectRandomTeamSpawnPoint( int teamstate, team_t team ) {
	gentity_t   *spot;
	int count;
	int selection;
	gentity_t   *spots[MAX_TEAM_SPAWN_POINTS];
	char        *classname;
	qboolean initialSpawn = qfalse;     // DHM - Nerve
	int i = 0,j;       // JPW NERVE
	int closest;         // JPW NERVE
	float shortest,tmp;       // JPW NERVE
	vec3_t target;      // JPW NERVE
	vec3_t farthest;      // JPW NERVE FIXME this is temp

	if ( teamstate == TEAM_BEGIN ) {

		// DHM - Nerve :: Don't check if spawn is active initially
		initialSpawn = qtrue;

		if ( team == TEAM_RED ) {
			classname = "team_CTF_redplayer";
		} else if ( team == TEAM_BLUE ) {
			classname = "team_CTF_blueplayer";
		} else {
			return NULL;
		}
	} else {
		if ( team == TEAM_RED ) {
			classname = "team_CTF_redspawn";
		} else if ( team == TEAM_BLUE ) {
			classname = "team_CTF_bluespawn";
		} else {
			return NULL;
		}
	}
	count = 0;

	spot = NULL;

	while ( ( spot = G_Find( spot, FOFS( classname ), classname ) ) != NULL ) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
// JPW NERVE
		if ( g_gametype.integer == GT_WOLF ) {
			if ( !( spot->spawnflags & 2 )  && !initialSpawn ) {
				continue;
			}
		}
// jpw
		spots[ count ] = spot;
		if ( ++count == MAX_TEAM_SPAWN_POINTS ) {
			break;
		}
	}

	if ( !count ) { // no spots that won't telefrag
		return G_Find( NULL, FOFS( classname ), classname );
	}

// JPW NERVE
	if ( ( g_gametype.integer != GT_WOLF ) || ( !level.numspawntargets ) || initialSpawn ) { // no spawn targets or not wolf MP, do it the old way
		selection = rand() % count;
		return spots[ selection ];
	} else {
		// FIXME: temporarily select target as farthest point from first team spawnpoint
		// we'll want to replace this with the target coords pulled from the UI target selection
		j = 0;
		for ( j = 0; j < count; j++ ) {
			if ( spots[j]->spawnflags & 1 ) { // only use spawnpoint if it's a permanent one
				i = FindFarthestObjectiveIndex( spots[j]->s.origin );
				j = count;
			}
		}
		VectorCopy( level.spawntargets[i],farthest );
		// end FIXME

		// now that we've got farthest vector, figure closest spawnpoint to it
		VectorSubtract( farthest,spots[0]->s.origin,target );
		shortest = VectorLength( target );
		closest = 0;
		for ( i = 0; i < count; i++ ) {
			VectorSubtract( farthest,spots[i]->s.origin,target );
			tmp = VectorLength( target );
			if ( ( spots[i]->spawnflags & 2 ) && ( tmp < shortest ) ) {
				shortest = tmp;
				closest = i;
			}
		}
		return spots[closest];
	}
// jpw
}


/*
===========
SelectCTFSpawnPoint

============
*/
gentity_t *SelectCTFSpawnPoint( team_t team, int teamstate, vec3_t origin, vec3_t angles ) {
	gentity_t   *spot;

	spot = SelectRandomTeamSpawnPoint( teamstate, team );

	if ( !spot ) {
		return SelectSpawnPoint( vec3_origin, origin, angles );
	}

	VectorCopy( spot->s.origin, origin );
	origin[2] += 9;
	VectorCopy( spot->s.angles, angles );

	return spot;
}

/*---------------------------------------------------------------------------*/

/*
==================
TeamplayLocationsMessage

Format:
	clientNum location health armor weapon powerups

==================
*/
void TeamplayInfoMessage( gentity_t *ent ) {
	char entry[1024];
	char string[1400];
	int stringlength;
	int i, j;
	gentity_t   *player;
	int cnt;
	int h, a;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;

	for ( i = 0, cnt = 0; i < level.numConnectedClients && cnt < TEAM_MAXOVERLAY; i++ ) {
		player = g_entities + level.sortedClients[i];
		if ( player->inuse && player->client->sess.sessionTeam ==
			 ent->client->sess.sessionTeam ) {

			h = player->client->ps.stats[STAT_HEALTH];
			a = player->client->ps.stats[STAT_ARMOR];
			if ( h < 0 ) {
				h = 0;
			}
			if ( a < 0 ) {
				a = 0;
			}

			Com_sprintf( entry, sizeof( entry ),
						 " %i %i %i %i %i %i",
						 level.sortedClients[i], player->client->pers.teamState.location, h, a,
						 player->client->ps.weapon, player->s.powerups );
			j = strlen( entry );
			if ( stringlength + j > sizeof( string ) ) {
				break;
			}
			strcpy( string + stringlength, entry );
			stringlength += j;
			cnt++;
		}
	}

	trap_SendServerCommand( ent - g_entities, va( "tinfo %i%s", cnt, string ) );
}

void CheckTeamStatus( void ) {
	int i;
	gentity_t *loc, *ent;

	if ( level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME ) {

		level.lastTeamLocationTime = level.time;

		for ( i = 0; i < g_maxclients.integer; i++ ) {
			ent = g_entities + i;
			if ( ent->inuse &&
				 ( ent->client->sess.sessionTeam == TEAM_RED ||
				   ent->client->sess.sessionTeam == TEAM_BLUE ) ) {
				loc = Team_GetLocation( ent );
				if ( loc ) {
					ent->client->pers.teamState.location = loc->health;
				} else {
					ent->client->pers.teamState.location = 0;
				}
			}
		}

		for ( i = 0; i < g_maxclients.integer; i++ ) {
			ent = g_entities + i;
			if ( ent->inuse &&
				 ( ent->client->sess.sessionTeam == TEAM_RED ||
				   ent->client->sess.sessionTeam == TEAM_BLUE ) ) {
				TeamplayInfoMessage( ent );
			}
		}
	}
}

/*-----------------------------------------------------------------*/

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32)
Only in CTF games.  Red players spawn here at game start.
*/
void SP_team_CTF_redplayer( gentity_t *ent ) {
}


/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in CTF games.  Blue players spawn here at game start.
*/
void SP_team_CTF_blueplayer( gentity_t *ent ) {
}

// JPW NERVE edited quaked def
/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32) invulnerable startactive
potential spawning position for axis team in wolfdm games.

TODO: SelectRandomTeamSpawnPoint() will choose team_CTF_redspawn point that:

1) has been activated (FL_SPAWNPOINT_ACTIVE)
2) isn't occupied and
3) is closest to team_WOLF_objective

This allows spawnpoints to advance across the battlefield as new ones are
placed and/or activated.

If target is set, point spawnpoint toward target activation
*/
void SP_team_CTF_redspawn( gentity_t *ent ) {
// JPW NERVE
	vec3_t dir;

	ent->enemy = G_PickTarget( ent->target );
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->s.origin, ent->s.origin, dir );
		vectoangles( dir, ent->s.angles );
	}
// jpw
}

// JPW NERVE edited quaked def
/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32) invulnerable startactive
potential spawning position for allied team in wolfdm games.

TODO: SelectRandomTeamSpawnPoint() will choose team_CTF_bluespawn point that:

1) has been activated (active)
2) isn't occupied and
3) is closest to selected team_WOLF_objective

This allows spawnpoints to advance across the battlefield as new ones are
placed and/or activated.

If target is set, point spawnpoint toward target activation
*/
void SP_team_CTF_bluespawn( gentity_t *ent ) {
// JPW NERVE
	vec3_t dir;

	ent->enemy = G_PickTarget( ent->target );
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->s.origin, ent->s.origin, dir );
		vectoangles( dir, ent->s.angles );
	}
// jpw
}


// JPW NERVE
/*QUAKED team_WOLF_objective (1 1 0.3) (-16 -16 -24) (16 16 32)
marker for objective

This marker will be used for computing effective radius for
dynamite damage, as well as generating a list of objectives
that players can elect to spawn near to in the limbo spawn
screen.

key "description" is short text key for objective name that
will appear in objective selection in limbo UI.
*/
void SP_team_WOLF_objective( gentity_t *ent ) {
	char *objectivename;
	char numspawntargets[128];
	static int numobjectives = 0;
	int cs_obj = CS_MULTI_SPAWNTARGETS;
	char cs[MAX_STRING_CHARS];
	vec3_t test;

	G_SpawnString( "description", "WARNING: No objective description set", &objectivename );

	if ( numobjectives == MAX_MULTI_SPAWNTARGETS ) {
		G_Error( "SP_team_WOLF_objective: exceeded MAX_MULTI_SPAWNTARGETS (%d)\n",MAX_MULTI_SPAWNTARGETS );
	} else { // Set config strings
		cs_obj += numobjectives;
		trap_GetConfigstring( cs_obj, cs, sizeof( cs ) );
		Info_SetValueForKey( cs, "spawn_targ", objectivename );
		trap_SetConfigstring( cs_obj, cs );
		VectorCopy( ent->s.origin, level.spawntargets[numobjectives] );
	}

	numobjectives++;

	// set current # spawntargets
	level.numspawntargets = numobjectives;
	trap_GetConfigstring( CS_MULTI_INFO, cs, sizeof( cs ) );
	sprintf( numspawntargets,"%d",numobjectives );
	Info_SetValueForKey( cs, "numspawntargets", numspawntargets );
	trap_SetConfigstring( CS_MULTI_INFO, cs );

	VectorCopy( level.spawntargets[numobjectives - 1],test );
	G_Printf( "OBJECTIVE %d: %s (total %s) x=%f %f %f\n",numobjectives,objectivename,numspawntargets,test[0],test[1],test[2] );
}
// jpw


// DHM - Nerve :: Capture and Hold Checkpoint flag

void checkpoint_touch( gentity_t *self, gentity_t *other, trace_t *trace );

#define WCP_ANIM_NOFLAG             0
#define WCP_ANIM_RAISE_NAZI         1
#define WCP_ANIM_RAISE_AMERICAN     2
#define WCP_ANIM_NAZI_RAISED        3
#define WCP_ANIM_AMERICAN_RAISED    4
#define WCP_ANIM_NAZI_TO_AMERICAN   5
#define WCP_ANIM_AMERICAN_TO_NAZI   6

void checkpoint_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	checkpoint_touch( ent, activator, NULL );
}

void checkpoint_think( gentity_t *self ) {

	switch ( self->s.frame ) {

	case WCP_ANIM_NOFLAG:
		break;
	case WCP_ANIM_RAISE_NAZI:
		self->s.frame = WCP_ANIM_NAZI_RAISED;
		break;
	case WCP_ANIM_RAISE_AMERICAN:
		self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		break;
	case WCP_ANIM_NAZI_RAISED:
		break;
	case WCP_ANIM_AMERICAN_RAISED:
		break;
	case WCP_ANIM_NAZI_TO_AMERICAN:
		self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		break;
	case WCP_ANIM_AMERICAN_TO_NAZI:
		self->s.frame = WCP_ANIM_NAZI_RAISED;
		break;
	default:
		break;

	}

	self->touch = checkpoint_touch;
	self->nextthink = 0;
}

void checkpoint_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {

	if ( self->count == other->client->sess.sessionTeam ) {
		return;
	}

	// Set controlling team
	self->count = other->client->sess.sessionTeam;

	// Set animation
	if ( self->count == TEAM_RED ) {
		if ( self->s.frame == WCP_ANIM_NOFLAG ) {
			self->s.frame = WCP_ANIM_RAISE_NAZI;
		} else if ( self->s.frame == WCP_ANIM_AMERICAN_RAISED ) {
			self->s.frame = WCP_ANIM_AMERICAN_TO_NAZI;
		} else {
			self->s.frame = WCP_ANIM_NAZI_RAISED;
		}
	} else {
		if ( self->s.frame == WCP_ANIM_NOFLAG ) {
			self->s.frame = WCP_ANIM_RAISE_AMERICAN;
		} else if ( self->s.frame == WCP_ANIM_NAZI_RAISED ) {
			self->s.frame = WCP_ANIM_NAZI_TO_AMERICAN;
		} else {
			self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		}
	}

	// Run script trigger
	if ( self->count == TEAM_RED ) {
		G_Script_ScriptEvent( self, "trigger", "axis_capture" );
	} else {
		G_Script_ScriptEvent( self, "trigger", "allied_capture" );
	}

	// Play a sound
	G_AddEvent( self, EV_GENERAL_SOUND, self->soundPos1 );

	// Don't allow touch again until animation is finished
	self->touch = NULL;

	self->think = checkpoint_think;
	self->nextthink = level.time + 1000;
}

// JPW NERVE -- if spawn flag is set, use this touch fn instead to turn on/off targeted spawnpoints
void checkpoint_spawntouch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	gentity_t *ent = NULL;

	if ( self->count == other->client->sess.sessionTeam ) {
		return;
	}

	// Set controlling team
	self->count = other->client->sess.sessionTeam;

	// Set animation
	if ( self->count == TEAM_RED ) {
		if ( self->s.frame == WCP_ANIM_NOFLAG ) {
			self->s.frame = WCP_ANIM_RAISE_NAZI;
		} else if ( self->s.frame == WCP_ANIM_AMERICAN_RAISED ) {
			self->s.frame = WCP_ANIM_AMERICAN_TO_NAZI;
		} else {
			self->s.frame = WCP_ANIM_NAZI_RAISED;
		}
	} else {
		if ( self->s.frame == WCP_ANIM_NOFLAG ) {
			self->s.frame = WCP_ANIM_RAISE_AMERICAN;
		} else if ( self->s.frame == WCP_ANIM_NAZI_RAISED ) {
			self->s.frame = WCP_ANIM_NAZI_TO_AMERICAN;
		} else {
			self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		}
	}

	// Play a sound
	G_AddEvent( self, EV_GENERAL_SOUND, self->soundPos1 );

	// Don't allow touch again until animation is finished
	self->touch = NULL;

	self->think = checkpoint_think;
	self->nextthink = level.time + 1000;

	// activate all targets
	if ( self->target ) {
//		G_Printf("targetname=%s\n",self->target);
		while ( 1 ) {
			ent = G_Find( ent, FOFS( targetname ), self->target );
			if ( !ent ) {
				break;
			}
			if ( self->count == TEAM_RED ) {
				if ( !strcmp( ent->classname,"team_CTF_redspawn" ) ) {
					ent->spawnflags |= 2;
				} else {
					ent->spawnflags &= ~2;
				}
			} else {
				if ( !strcmp( ent->classname,"team_CTF_bluespawn" ) ) {
					ent->spawnflags |= 2;
				} else {
					ent->spawnflags &= ~2;
				}
			}
		}
	}


}
// jpw

/*QUAKED team_WOLF_checkpoint (.6 .9 .6) (-16 -16 0) (16 16 128) spawnpoint
This is the flagpole players touch in Capture and Hold game scenarios.

It will call specific trigger funtions in the map script for this object.
When allies capture, it will call "allied_capture".
When axis capture, it will call "axis_capture".

// JPW NERVE if spawnpoint flag is set, think will turn on spawnpoints (specified as targets)
// for capture team and turn *off* targeted spawnpoints for opposing team
*/
void SP_team_WOLF_checkpoint( gentity_t *ent ) {
	char *capture_sound;

	if ( !ent->scriptName ) {
		G_Error( "team_WOLF_checkpoint must have a \"scriptname\"\n" );
	}

	// Make sure the ET_TRAP entity type stays valid
	ent->s.eType        = ET_TRAP;

	// Model is user assignable, but it will always try and use the animations for flagpole.md3
	if ( ent->model ) {
		ent->s.modelindex   = G_ModelIndex( ent->model );
	} else {
		ent->s.modelindex   = G_ModelIndex( "models/multiplayer/flagpole/flagpole.md3" );
	}

	G_SpawnString( "noise", "sound/movers/doors/door6_open.wav", &capture_sound );
	ent->soundPos1  = G_SoundIndex( capture_sound );

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;

	VectorSet( ent->r.mins, -8, -8, 0 );
	VectorSet( ent->r.maxs, 8, 8, 128 );

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngle( ent, ent->s.angles );

	// s.frame is the animation number
	ent->s.frame    = WCP_ANIM_NOFLAG;

	// s.teamNum is which set of animations to use ( only 1 right now )
	ent->s.teamNum  = 1;

	// Used later to set animations (and delay between captures)
	ent->nextthink = 0;

	// 'count' signifies which team holds the checkpoint
	ent->count = -1;

// JPW NERVE
	if ( ent->spawnflags & 1 ) {
		ent->touch      = checkpoint_spawntouch;
	} else {
		ent->touch      = checkpoint_touch;
	}
// jpw
	ent->use        = checkpoint_use;       // allow 'capture' from trigger

	trap_LinkEntity( ent );
}