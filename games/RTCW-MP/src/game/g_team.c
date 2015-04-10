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

	// NOTE: if buffer overflow, it's more likely to corrupt stack and crash than do a proper G_Error?
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

// JPW NERVE moved these up
#define WCP_ANIM_NOFLAG             0
#define WCP_ANIM_RAISE_AXIS         1
#define WCP_ANIM_RAISE_AMERICAN     2
#define WCP_ANIM_AXIS_RAISED        3
#define WCP_ANIM_AMERICAN_RAISED    4
#define WCP_ANIM_AXIS_TO_AMERICAN   5
#define WCP_ANIM_AMERICAN_TO_AXIS   6
#define WCP_ANIM_AXIS_FALLING       7
#define WCP_ANIM_AMERICAN_FALLING   8
// jpw

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
// JPW NERVE -- no bonuses for fragging friendlies, penalties scored elsewhere
	if ( team == attacker->client->sess.sessionTeam ) {
		return;
	}
// jpw

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
		if ( g_gametype.integer >= GT_WOLF ) {
			AddScore( attacker, WOLF_FRAG_CARRIER_BONUS );
		} else {
			AddScore( attacker, CTF_FRAG_CARRIER_BONUS );
			PrintMsg( NULL, "%s" S_COLOR_WHITE " fragged %s's flag carrier!\n",
					  attacker->client->pers.netname, TeamName( team ) );
		}
		attacker->client->pers.teamState.fragcarrier++;

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
	if ( g_gametype.integer < GT_WOLF ) { // JPW NERVE no danger protect in wolf
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

	if ( flag ) { // JPW NERVE -- added some more stuff after this fn
//		return; // can't find attacker's flag

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
		VectorSubtract( targ->client->ps.origin, flag->s.origin, v1 );
		VectorSubtract( attacker->client->ps.origin, flag->s.origin, v2 );

		if ( ( VectorLength( v1 ) < CTF_TARGET_PROTECT_RADIUS ||
			   VectorLength( v2 ) < CTF_TARGET_PROTECT_RADIUS ||
			   CanDamage( flag, targ->client->ps.origin ) || CanDamage( flag, attacker->client->ps.origin ) ) &&
			 attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam ) {
			// we defended the base flag
			if ( g_gametype.integer >= GT_WOLF ) { // JPW NERVE FIXME -- don't report flag defense messages, change to gooder message
				AddScore( attacker, WOLF_FLAG_DEFENSE_BONUS );
			} else {
				AddScore( attacker, CTF_FLAG_DEFENSE_BONUS );
				if ( !flag->r.contents ) {
					PrintMsg( NULL, "%s" S_COLOR_WHITE " defends the %s base.\n",
							  attacker->client->pers.netname,
							  TeamName( attacker->client->sess.sessionTeam ) );
				} else {
					PrintMsg( NULL, "%s" S_COLOR_WHITE " defends the %s flag.\n",
							  attacker->client->pers.netname,
							  TeamName( attacker->client->sess.sessionTeam ) );
				}
			}
			attacker->client->pers.teamState.basedefense++;
			return;
		}

		if ( g_gametype.integer < GT_WOLF ) { // JPW NERVE no attacker protect in wolf MP
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

	} // JPW NERVE

// JPW NERVE -- look for nearby checkpoints and spawnpoints
	flag = NULL;
	while ( ( flag = G_Find( flag, FOFS( classname ), "team_WOLF_checkpoint" ) ) != NULL ) {
		VectorSubtract( targ->client->ps.origin, flag->s.origin, v1 );
		if ( ( flag->s.frame != WCP_ANIM_NOFLAG ) && ( flag->count == attacker->client->sess.sessionTeam ) ) {
			if ( VectorLength( v1 ) < WOLF_CP_PROTECT_RADIUS ) {
				if ( flag->spawnflags & 1 ) {                     // protected spawnpoint
					AddScore( attacker, WOLF_SP_PROTECT_BONUS );
				} else {
					AddScore( attacker, WOLF_CP_PROTECT_BONUS );  // protected checkpoint
				}
			}
		}
	}
// jpw

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
									"sound/multiplayer/axis/g-objective_secure.wav" :
									"sound/multiplayer/allies/a-objective_secure.wav" );
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
	// TTimo might be used uninitialized
	gentity_t *gm = NULL;

	if ( g_gametype.integer >= GT_WOLF ) {
		gm = G_Find( NULL, FOFS( scriptName ), "game_manager" );
	}

	if ( ent->item->giTag == PW_REDFLAG ) {
		Team_ReturnFlagSound( Team_ResetFlag( TEAM_RED ), TEAM_RED );
		if ( gm ) {
			trap_SendServerCommand( -1, "cp \"Axis have returned the objective!\" 2" );
			G_Script_ScriptEvent( gm, "trigger", "axis_object_returned" );
		}
	} else if ( ent->item->giTag == PW_BLUEFLAG )     {
		Team_ReturnFlagSound( Team_ResetFlag( TEAM_BLUE ), TEAM_BLUE );
		if ( gm ) {
			trap_SendServerCommand( -1, "cp \"Allies have returned the objective!\" 2" );
			G_Script_ScriptEvent( gm, "trigger", "allied_object_returned" );
		}
	}
	// Reset Flag will delete this entity
}

int Team_TouchOurFlag( gentity_t *ent, gentity_t *other, int team ) {
	int i;
	gentity_t *player;
	gclient_t *cl = other->client;
	int our_flag, enemy_flag;
	gentity_t   *te, *gm;

	if ( cl->sess.sessionTeam == TEAM_RED ) {
		our_flag = PW_REDFLAG;
		enemy_flag = PW_BLUEFLAG;
	} else {
		our_flag = PW_BLUEFLAG;
		enemy_flag = PW_REDFLAG;
	}

	if ( ent->flags & FL_DROPPED_ITEM ) {
		// hey, its not home.  return it by teleporting it back
// JPW NERVE
		if ( g_gametype.integer >= GT_WOLF ) {
			AddScore( other, WOLF_SECURE_OBJ_BONUS );
			te = G_TempEntity( other->s.pos.trBase, EV_GLOBAL_SOUND );
			te->r.svFlags |= SVF_BROADCAST;
			te->s.teamNum = cl->sess.sessionTeam;

			// DHM - Nerve :: Call trigger function in the 'game_manager' entity script
			gm = G_Find( NULL, FOFS( scriptName ), "game_manager" );

			if ( cl->sess.sessionTeam == TEAM_RED ) {
				te->s.eventParm = G_SoundIndex( "sound/multiplayer/axis/g-objective_secure.wav" );
				trap_SendServerCommand( -1, va( "cp \"Axis have returned %s!\n\" 2", ent->message ) );
				if ( gm ) {
					G_Script_ScriptEvent( gm, "trigger", "axis_object_returned" );
				}
			} else {
				te->s.eventParm = G_SoundIndex( "sound/multiplayer/allies/a-objective_secure.wav" );
				trap_SendServerCommand( -1, va( "cp \"Allies have returned %s!\n\" 2", ent->message ) );
				if ( gm ) {
					G_Script_ScriptEvent( gm, "trigger", "allied_object_returned" );
				}
			}
			// dhm
		}
// jpw 800 672 2420
		else {
			PrintMsg( NULL, "%s" S_COLOR_WHITE " returned the %s flag!\n",
					  cl->pers.netname, TeamName( team ) );
			AddScore( other, CTF_RECOVERY_BONUS );
		}
		other->client->pers.teamState.flagrecovery++;
		other->client->pers.teamState.lastreturnedflag = level.time;
		//ResetFlag will remove this entity!  We must return zero
		Team_ReturnFlagSound( Team_ResetFlag( team ), team );
		return 0;
	}

	// DHM - Nerve :: GT_WOLF doesn't support capturing the flag
	if ( g_gametype.integer >= GT_WOLF ) {
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
	if ( g_gametype.integer >= GT_WOLF ) {
		AddScore( other, WOLF_CAPTURE_BONUS );
		PrintMsg( NULL,"%s" S_COLOR_WHITE " captured enemy objective!\n",cl->pers.netname );
	} else {
		AddScore( other, CTF_CAPTURE_BONUS );
	}

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
			// TTimo gcc: suggest explicit braces to avoid ambiguous `else`
			if ( player != other ) {
// JPW NERVE
				if ( g_gametype.integer >= GT_WOLF ) {
					AddScore( player, WOLF_CAPTURE_BONUS );
				} else {
// jpw
					AddScore( player, CTF_CAPTURE_BONUS );
				}
			}
			// award extra points for capture assists
// JPW NERVE in non-wolf-mp only
			if ( g_gametype.integer < GT_WOLF ) {
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
	}
	Team_ResetFlags();

	CalculateRanks();

	return 0; // Do not respawn this automatically
}

int Team_TouchEnemyFlag( gentity_t *ent, gentity_t *other, int team ) {
	gclient_t *cl = other->client;
	gentity_t *te, *gm;

	// hey, its not our flag, pick it up
	if ( g_gametype.integer >= GT_WOLF ) {
// JPW NERVE
		AddScore( other, WOLF_STEAL_OBJ_BONUS );
		te = G_TempEntity( other->s.pos.trBase, EV_GLOBAL_SOUND );
		te->r.svFlags |= SVF_BROADCAST;
		te->s.teamNum = cl->sess.sessionTeam;

		// DHM - Nerve :: Call trigger function in the 'game_manager' entity script
		gm = G_Find( NULL, FOFS( scriptName ), "game_manager" );

		if ( cl->sess.sessionTeam == TEAM_RED ) {
			te->s.eventParm = G_SoundIndex( "sound/multiplayer/axis/g-objective_taken.wav" );
			trap_SendServerCommand( -1, va( "cp \"Axis have stolen %s!\n\" 2", ent->message ) );
			if ( gm ) {
				G_Script_ScriptEvent( gm, "trigger", "allied_object_stolen" );
			}
		} else {
			te->s.eventParm = G_SoundIndex( "sound/multiplayer/allies/a-objective_taken.wav" );
			trap_SendServerCommand( -1, va( "cp \"Allies have stolen %s!\n\" 2", ent->message ) );
			if ( gm ) {
				G_Script_ScriptEvent( gm, "trigger", "axis_object_stolen" );
			}
		}
		// dhm
// jpw
	} else {
		PrintMsg( NULL, "%s" S_COLOR_WHITE " got the %s flag!\n",
				  other->client->pers.netname, TeamName( team ) );
		AddScore( other, CTF_FLAG_BONUS );
	}

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
	if ( g_gametype.integer >= GT_WOLF ) {
		other->message = ent->message;
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
		Com_sprintf( loc, loclen, "%c%c[lon]%s[lof]" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message );
	} else {
		Com_sprintf( loc, loclen, "[lon]%s[lof]", best->message );
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

// NERVE - SMF
/*
=======================
FindClosestObjectiveIndex

NERVE - SMF - pick MP objective closest to the passed in vector, return table index
=======================
*/
int FindClosestObjectiveIndex( vec3_t source ) {
	int i,j = 0;
	float dist = 10E20,tdist;
	vec3_t tmp;

	for ( i = 0; i < level.numspawntargets; i++ ) {
		VectorSubtract( level.spawntargets[i],source,tmp );
		tdist = VectorLength( tmp );
		if ( tdist < dist ) {
			dist = tdist;
			j = i;
		}
	}

	return j;
}
// -NERVE - SMF

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_TEAM_SPAWN_POINTS   32
gentity_t *SelectRandomTeamSpawnPoint( int teamstate, team_t team, int spawnObjective ) {
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
	char cs[MAX_STRING_CHARS];          // NERVE - SMF
	char        *def;
	int defendingTeam;
	qboolean defender = qfalse;

	// NERVE - SMF - get defender
	trap_GetConfigstring( CS_MULTI_INFO, cs, sizeof( cs ) );
	def = Info_ValueForKey( cs, "defender" );

	if ( strlen( def ) > 0 ) {
		defendingTeam = atoi( def );
	} else {
		defendingTeam = -1;
	}

	if ( defendingTeam && team == TEAM_BLUE ) {         // allies
		defender = qtrue;
	} else if ( !defendingTeam && team == TEAM_RED )   {  // axis
		defender = qtrue;
	}

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
		if ( g_gametype.integer >= GT_WOLF ) {
			// Arnout - modified to allow intial spawnpoints to be disabled at gamestart
			//if (!(spot->spawnflags & 2)  && !initialSpawn )
			if ( ( initialSpawn && ( spot->spawnflags & 4 ) ) ||
				 ( !initialSpawn && !( spot->spawnflags & 2 ) ) ) {
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
	if ( ( g_gametype.integer < GT_WOLF ) || ( !level.numspawntargets ) || initialSpawn ) { // no spawn targets or not wolf MP, do it the old way
		selection = rand() % count;
		return spots[ selection ];
	} else {
		// If no spawnObjective, select target as farthest point from first team spawnpoint
		// else replace this with the target coords pulled from the UI target selection
		if ( spawnObjective ) {
			i = spawnObjective - 1;
		} else {
			j = 0;
			for ( j = 0; j < count; j++ ) {
				if ( spots[j]->spawnflags & 1 ) { // only use spawnpoint if it's a permanent one
					// NERVE - SMF - make defenders spawn all the way back by default
					if ( defendingTeam < 0 ) {
						i = FindFarthestObjectiveIndex( spots[j]->s.origin );
					} else if ( defender ) {
						i = FindClosestObjectiveIndex( spots[j]->s.origin );
					} else {
						i = FindFarthestObjectiveIndex( spots[j]->s.origin );
					}

					j = count;
				}
			}
		}
		VectorCopy( level.spawntargets[i],farthest );
//		G_Printf("using spawntarget %d (%f %f %f)\n",i,farthest[0],farthest[1],farthest[2]);

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
gentity_t *SelectCTFSpawnPoint( team_t team, int teamstate, vec3_t origin, vec3_t angles, int spawnObjective ) {
	gentity_t   *spot;

	spot = SelectRandomTeamSpawnPoint( teamstate, team, spawnObjective );

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
	int identClientNum, identHealth;                // NERVE - SMF
	char entry[1024];
	char string[1400];
	int stringlength;
	int i, j;
	gentity_t   *player;
	int cnt;
	int h;

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;

	for ( i = 0, cnt = 0; i < level.numConnectedClients && cnt < TEAM_MAXOVERLAY; i++ ) {
		player = g_entities + level.sortedClients[i];
		if ( player->inuse && player->client->sess.sessionTeam ==
			 ent->client->sess.sessionTeam ) {

			// DHM - Nerve :: If in LIMBO, don't show followee's health
			if ( player->client->ps.pm_flags & PMF_LIMBO ) {
				h = 0;
			} else {
				h = player->client->ps.stats[STAT_HEALTH];
			}

			if ( h < 0 ) {
				h = 0;
			}

			Com_sprintf( entry, sizeof( entry ),
						 " %i %i %i %i %i",
						 level.sortedClients[i], player->client->pers.teamState.location, h, player->s.powerups, player->client->ps.stats[STAT_PLAYER_CLASS] );
			j = strlen( entry );
			if ( stringlength + j > sizeof( string ) ) {
				break;
			}
			strcpy( string + stringlength, entry );
			stringlength += j;
			cnt++;
		}
	}

	// NERVE - SMF
	identClientNum = ent->client->ps.identifyClient;

	if ( g_entities[identClientNum].team == ent->team && g_entities[identClientNum].client ) {
		identHealth =  g_entities[identClientNum].health;
	} else {
		identClientNum = -1;
		identHealth = 0;
	}
	// -NERVE - SMF

	trap_SendServerCommand( ent - g_entities, va( "tinfo %i %i %i%s", identClientNum, identHealth, cnt, string ) );
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

void Use_Team_InitialSpawnpoint( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->spawnflags & 4 ) {
		ent->spawnflags &= ~4;
	} else {
		ent->spawnflags |= 4;
	}
}

void Use_Team_Spawnpoint( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->spawnflags & 2 ) {
		ent->spawnflags &= ~2;
	} else {
		ent->spawnflags |= 2;
	}
}

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32) invulnerable unused startdisabled
Only in CTF games.  Red players spawn here at game start.
*/
void SP_team_CTF_redplayer( gentity_t *ent ) {
	ent->use = Use_Team_InitialSpawnpoint;
}


/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32) invulnerable unused startdisabled
Only in CTF games.  Blue players spawn here at game start.
*/
void SP_team_CTF_blueplayer( gentity_t *ent ) {
	ent->use = Use_Team_InitialSpawnpoint;
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

	ent->use = Use_Team_Spawnpoint;
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

	ent->use = Use_Team_Spawnpoint;
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
static int numobjectives = 0; // TTimo

void objective_Register( gentity_t *self ) {

	char numspawntargets[128];
	int cs_obj = CS_MULTI_SPAWNTARGETS;
	char cs[MAX_STRING_CHARS];

	if ( numobjectives == MAX_MULTI_SPAWNTARGETS ) {
		G_Error( "SP_team_WOLF_objective: exceeded MAX_MULTI_SPAWNTARGETS (%d)\n",MAX_MULTI_SPAWNTARGETS );
	} else { // Set config strings
		cs_obj += numobjectives;
		trap_GetConfigstring( cs_obj, cs, sizeof( cs ) );
		Info_SetValueForKey( cs, "spawn_targ", self->message );
		trap_SetConfigstring( cs_obj, cs );
		VectorCopy( self->s.origin, level.spawntargets[numobjectives] );
	}

	numobjectives++;

	// set current # spawntargets
	level.numspawntargets = numobjectives;
	trap_GetConfigstring( CS_MULTI_INFO, cs, sizeof( cs ) );
	sprintf( numspawntargets,"%d",numobjectives );
	Info_SetValueForKey( cs, "numspawntargets", numspawntargets );
	trap_SetConfigstring( CS_MULTI_INFO, cs );
}

void SP_team_WOLF_objective( gentity_t *ent ) {
	char *desc;

	G_SpawnString( "description", "WARNING: No objective description set", &desc );
	ent->message = G_Alloc( strlen( desc ) + 1 );
	Q_strncpyz( ent->message, desc, strlen( desc ) + 1 );

	// DHM - Nerve :: Give the script time to remove this ent if necessary
	ent->nextthink = level.time + 150;
	ent->think = objective_Register;
}
// jpw


// DHM - Nerve :: Capture and Hold Checkpoint flag
#define SPAWNPOINT  1
#define CP_HOLD     2
#define AXIS_ONLY   4
#define ALLIED_ONLY 8

void checkpoint_touch( gentity_t *self, gentity_t *other, trace_t *trace );

void checkpoint_use_think( gentity_t *self ) {

	self->count2 = -1;

	if ( self->count == TEAM_RED ) {
		self->health = 0;
	} else {
		self->health = 10;
	}
}

void checkpoint_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {

	int holderteam;
	int time;

	if ( !activator->client ) {
		return;
	}

	if ( ent->count < 0 ) {
		checkpoint_touch( ent, activator, NULL );
	}

	holderteam = activator->client->sess.sessionTeam;

	if ( ent->count == holderteam ) {
		return;
	}

	if ( ent->count2 == level.time ) {
		if ( holderteam == TEAM_RED ) {
			time = ent->health / 2;
			time++;
			trap_SendServerCommand( activator - g_entities, va( "cp \"Flag will be held in %i seconds!\n\"", time ) );
		} else {
			time = ( 10 - ent->health ) / 2;
			time++;
			trap_SendServerCommand( activator - g_entities, va( "cp \"Flag will be held in %i seconds!\n\"", time ) );
		}
		return;
	}

	if ( holderteam == TEAM_RED ) {
		ent->health--;
		if ( ent->health < 0 ) {
			checkpoint_touch( ent, activator, NULL );
			return;
		}

		time = ent->health / 2;
		time++;
		trap_SendServerCommand( activator - g_entities, va( "cp \"Flag will be held in %i seconds!\n\"", time ) );
	} else {
		ent->health++;
		if ( ent->health > 10 ) {
			checkpoint_touch( ent, activator, NULL );
			return;
		}

		time = ( 10 - ent->health ) / 2;
		time++;
		trap_SendServerCommand( activator - g_entities, va( "cp \"Flag will be held in %i seconds!\n\"", time ) );
	}

	ent->count2 = level.time;
	ent->think = checkpoint_use_think;
	ent->nextthink = level.time + 2000;
}

void checkpoint_spawntouch( gentity_t *self, gentity_t *other, trace_t *trace ); // JPW NERVE

// JPW NERVE
void checkpoint_hold_think( gentity_t *self ) {
	switch ( self->s.frame ) {
	case WCP_ANIM_RAISE_AXIS:
	case WCP_ANIM_AXIS_RAISED:
		level.capturetimes[TEAM_RED]++;
		break;
	case WCP_ANIM_RAISE_AMERICAN:
	case WCP_ANIM_AMERICAN_RAISED:
		level.capturetimes[TEAM_BLUE]++;
		break;
	default:
		break;
	}
	self->nextthink = level.time + 5000;
}
// jpw

void checkpoint_think( gentity_t *self ) {

	switch ( self->s.frame ) {

	case WCP_ANIM_NOFLAG:
		break;
	case WCP_ANIM_RAISE_AXIS:
		self->s.frame = WCP_ANIM_AXIS_RAISED;
		break;
	case WCP_ANIM_RAISE_AMERICAN:
		self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		break;
	case WCP_ANIM_AXIS_RAISED:
		break;
	case WCP_ANIM_AMERICAN_RAISED:
		break;
	case WCP_ANIM_AXIS_TO_AMERICAN:
		self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		break;
	case WCP_ANIM_AMERICAN_TO_AXIS:
		self->s.frame = WCP_ANIM_AXIS_RAISED;
		break;
	case WCP_ANIM_AXIS_FALLING:
		self->s.frame = WCP_ANIM_NOFLAG;
		break;
	case WCP_ANIM_AMERICAN_FALLING:
		self->s.frame = WCP_ANIM_NOFLAG;
		break;
	default:
		break;

	}

// JPW NERVE
	if ( self->spawnflags & SPAWNPOINT ) {
		self->touch = checkpoint_spawntouch;
	} else if ( !( self->spawnflags & CP_HOLD ) ) {
		self->touch = checkpoint_touch;
	}
	if ( ( g_gametype.integer == GT_WOLF_CPH ) && ( !( self->spawnflags & SPAWNPOINT ) ) ) {
		self->think = checkpoint_hold_think;
		self->nextthink = level.time + 5000;
	} else {
		self->nextthink = 0;
	}
// jpw
}

void checkpoint_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {

	if ( self->count == other->client->sess.sessionTeam ) {
		return;
	}

// JPW NERVE
	if ( self->s.frame == WCP_ANIM_NOFLAG ) {
		AddScore( other, WOLF_CP_CAPTURE );
	} else {
		AddScore( other, WOLF_CP_RECOVER );
	}
// jpw

	// Set controlling team
	self->count = other->client->sess.sessionTeam;

	// Set animation
	if ( self->count == TEAM_RED ) {
		if ( self->s.frame == WCP_ANIM_NOFLAG ) {
			self->s.frame = WCP_ANIM_RAISE_AXIS;
		} else if ( self->s.frame == WCP_ANIM_AMERICAN_RAISED ) {
			self->s.frame = WCP_ANIM_AMERICAN_TO_AXIS;
		} else {
			self->s.frame = WCP_ANIM_AXIS_RAISED;
		}
	} else {
		if ( self->s.frame == WCP_ANIM_NOFLAG ) {
			self->s.frame = WCP_ANIM_RAISE_AMERICAN;
		} else if ( self->s.frame == WCP_ANIM_AXIS_RAISED ) {
			self->s.frame = WCP_ANIM_AXIS_TO_AMERICAN;
		} else {
			self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		}
	}

	// Run script trigger
	if ( self->count == TEAM_RED ) {
		self->health = 0;
		G_Script_ScriptEvent( self, "trigger", "axis_capture" );
	} else {
		self->health = 10;
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
	gentity_t   *ent = NULL;
	qboolean playsound = qtrue;
	qboolean firsttime = qfalse;

	if ( self->count == other->client->sess.sessionTeam ) {
		return;
	}

// JPW NERVE
	if ( self->s.frame == WCP_ANIM_NOFLAG ) {
		AddScore( other, WOLF_SP_CAPTURE );
	} else {
		AddScore( other, WOLF_SP_RECOVER );
	}
// jpw

	if ( self->count < 0 ) {
		firsttime = qtrue;
	}

	// Set controlling team
	self->count = other->client->sess.sessionTeam;

	// Set animation
	if ( self->count == TEAM_RED ) {
		if ( self->s.frame == WCP_ANIM_NOFLAG && !( self->spawnflags & ALLIED_ONLY ) ) {
			self->s.frame = WCP_ANIM_RAISE_AXIS;
		} else if ( self->s.frame == WCP_ANIM_NOFLAG ) {
			self->s.frame = WCP_ANIM_NOFLAG;
			playsound = qfalse;
		} else if ( self->s.frame == WCP_ANIM_AMERICAN_RAISED && !( self->spawnflags & ALLIED_ONLY ) )     {
			self->s.frame = WCP_ANIM_AMERICAN_TO_AXIS;
		} else if ( self->s.frame == WCP_ANIM_AMERICAN_RAISED ) {
			self->s.frame = WCP_ANIM_AMERICAN_FALLING;
		} else {
			self->s.frame = WCP_ANIM_AXIS_RAISED;
		}
	} else {
		if ( self->s.frame == WCP_ANIM_NOFLAG && !( self->spawnflags & AXIS_ONLY ) ) {
			self->s.frame = WCP_ANIM_RAISE_AMERICAN;
		} else if ( self->s.frame == WCP_ANIM_NOFLAG ) {
			self->s.frame = WCP_ANIM_NOFLAG;
			playsound = qfalse;
		} else if ( self->s.frame == WCP_ANIM_AXIS_RAISED && !( self->spawnflags & AXIS_ONLY ) )     {
			self->s.frame = WCP_ANIM_AXIS_TO_AMERICAN;
		} else if ( self->s.frame == WCP_ANIM_AXIS_RAISED ) {
			self->s.frame = WCP_ANIM_AXIS_FALLING;
		} else {
			self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		}
	}

	// If this is the first time it's being touched, and it was the opposing team
	// touching a single-team reinforcement flag... don't do anything.
	if ( firsttime && !playsound ) {
		return;
	}

	// Play a sound
	if ( playsound ) {
		G_AddEvent( self, EV_GENERAL_SOUND, self->soundPos1 );
	}

	// Run script trigger
	if ( self->count == TEAM_RED ) {
		G_Script_ScriptEvent( self, "trigger", "axis_capture" );
	} else {
		G_Script_ScriptEvent( self, "trigger", "allied_capture" );
	}

	// Don't allow touch again until animation is finished
	self->touch = NULL;

	self->think = checkpoint_think;
	self->nextthink = level.time + 1000;

	// activate all targets
	// Arnout - updated this to allow toggling of initial spawnpoints as well, plus now it only
	// toggles spawnflags 2 for spawnpoint entities
	if ( self->target ) {
		while ( 1 ) {
			ent = G_Find( ent, FOFS( targetname ), self->target );
			if ( !ent ) {
				break;
			}
			if ( other->client->sess.sessionTeam == TEAM_RED ) {
				if ( !strcmp( ent->classname,"team_CTF_redspawn" ) ) {
					ent->spawnflags |= 2;
				} else if ( !strcmp( ent->classname,"team_CTF_bluespawn" ) ) {
					ent->spawnflags &= ~2;
				} else if ( !strcmp( ent->classname,"team_CTF_redplayer" ) ) {
					ent->spawnflags &= ~4;
				} else if ( !strcmp( ent->classname,"team_CTF_blueplayer" ) ) {
					ent->spawnflags |= 4;
				}
			} else {
				if ( !strcmp( ent->classname,"team_CTF_bluespawn" ) ) {
					ent->spawnflags |= 2;
				} else if ( !strcmp( ent->classname,"team_CTF_redspawn" ) ) {
					ent->spawnflags &= ~2;
				} else if ( !strcmp( ent->classname,"team_CTF_blueplayer" ) ) {
					ent->spawnflags &= ~4;
				} else if ( !strcmp( ent->classname,"team_CTF_redplayer" ) ) {
					ent->spawnflags |= 4;
				}
			}
		}
	}

}
// jpw
/*QUAKED team_WOLF_checkpoint (.9 .3 .9) (-16 -16 0) (16 16 128) SPAWNPOINT CP_HOLD AXIS_ONLY ALLIED_ONLY
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

	// Used to time how long it must be "held" to switch
	ent->health = -1;
	ent->count2 = -1;

	// 'count' signifies which team holds the checkpoint
	ent->count = -1;

// JPW NERVE
	if ( ent->spawnflags & SPAWNPOINT ) {
		ent->touch      = checkpoint_spawntouch;
	} else {
		if ( ent->spawnflags & CP_HOLD ) {
			ent->use        = checkpoint_use;
		} else {
			ent->touch      = checkpoint_touch;
		}
	}
// jpw

	trap_LinkEntity( ent );
}
