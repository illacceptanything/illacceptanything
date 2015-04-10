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




// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"


/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/





typedef struct ipFilter_s
{
	unsigned mask;
	unsigned compare;
} ipFilter_t;

#define MAX_IPFILTERS   1024

static ipFilter_t ipFilters[MAX_IPFILTERS];
static int numIPFilters;

/*
=================
StringToFilter
=================
*/
static qboolean StringToFilter( char *s, ipFilter_t *f ) {
	char num[128];
	int i, j;
	byte b[4];
	byte m[4];

	for ( i = 0 ; i < 4 ; i++ )
	{
		b[i] = 0;
		m[i] = 0;
	}

	for ( i = 0 ; i < 4 ; i++ )
	{
		if ( *s < '0' || *s > '9' ) {
			G_Printf( "Bad filter address: %s\n", s );
			return qfalse;
		}

		j = 0;
		while ( *s >= '0' && *s <= '9' )
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi( num );
		if ( b[i] != 0 ) {
			m[i] = 255;
		}

		if ( !*s ) {
			break;
		}
		s++;
	}

	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;

	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans( void ) {
	byte b[4];
	int i;
	char iplist[MAX_INFO_STRING];

	*iplist = 0;
	for ( i = 0 ; i < numIPFilters ; i++ )
	{
		if ( ipFilters[i].compare == 0xffffffff ) {
			continue;
		}

		*(unsigned *)b = ipFilters[i].compare;
		Com_sprintf( iplist + strlen( iplist ), sizeof( iplist ) - strlen( iplist ),
					 "%i.%i.%i.%i ", b[0], b[1], b[2], b[3] );
	}

	trap_Cvar_Set( "g_banIPs", iplist );
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket( char *from ) {
	int i;
	unsigned in;
	byte m[4];
	char *p;

	i = 0;
	p = from;
	while ( *p && i < 4 ) {
		m[i] = 0;
		while ( *p >= '0' && *p <= '9' ) {
			m[i] = m[i] * 10 + ( *p - '0' );
			p++;
		}
		if ( !*p || *p == ':' ) {
			break;
		}
		i++, p++;
	}

	in = *(unsigned *)m;

	for ( i = 0 ; i < numIPFilters ; i++ )
		if ( ( in & ipFilters[i].mask ) == ipFilters[i].compare ) {
			return g_filterBan.integer != 0;
		}

	return g_filterBan.integer == 0;
}

/*
=================
AddIP
=================
*/
static void AddIP( char *str ) {
	int i;

	for ( i = 0 ; i < numIPFilters ; i++ )
		if ( ipFilters[i].compare == 0xffffffff ) {
			break;
		}               // free spot
	if ( i == numIPFilters ) {
		if ( numIPFilters == MAX_IPFILTERS ) {
			G_Printf( "IP filter list is full\n" );
			return;
		}
		numIPFilters++;
	}

	if ( !StringToFilter( str, &ipFilters[i] ) ) {
		ipFilters[i].compare = 0xffffffffu;
	}

	UpdateIPBans();
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans( void ) {
	char *s, *t;
	char str[MAX_TOKEN_CHARS];

	Q_strncpyz( str, g_banIPs.string, sizeof( str ) );

	for ( t = s = g_banIPs.string; *t; /* */ ) {
		s = strchr( s, ' ' );
		if ( !s ) {
			break;
		}
		while ( *s == ' ' )
			*s++ = 0;
		if ( *t ) {
			AddIP( t );
		}
		t = s;
	}
}


/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f( void ) {
	char str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf( "Usage:  addip <ip-mask>\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	AddIP( str );

}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f( void ) {
	ipFilter_t f;
	int i;
	char str[MAX_TOKEN_CHARS];

	if ( trap_Argc() < 2 ) {
		G_Printf( "Usage:  sv removeip <ip-mask>\n" );
		return;
	}

	trap_Argv( 1, str, sizeof( str ) );

	if ( !StringToFilter( str, &f ) ) {
		return;
	}

	for ( i = 0 ; i < numIPFilters ; i++ ) {
		if ( ipFilters[i].mask == f.mask &&
			 ipFilters[i].compare == f.compare ) {
			ipFilters[i].compare = 0xffffffffu;
			G_Printf( "Removed.\n" );

			UpdateIPBans();
			return;
		}
	}

	G_Printf( "Didn't find %s.\n", str );
}

/*
===================
Svcmd_EntityList_f
===================
*/
void    Svcmd_EntityList_f( void ) {
	int e;
	gentity_t       *check;

	check = g_entities + 1;
	for ( e = 1; e < level.num_entities ; e++, check++ ) {
		if ( !check->inuse ) {
			continue;
		}
		G_Printf( "%3i:", e );
		switch ( check->s.eType ) {
		case ET_GENERAL:
			G_Printf( "ET_GENERAL          " );
			break;
		case ET_PLAYER:
			G_Printf( "ET_PLAYER           " );
			break;
		case ET_ITEM:
			G_Printf( "ET_ITEM             " );
			break;
		case ET_MISSILE:
			G_Printf( "ET_MISSILE          " );
			break;
		case ET_MOVER:
			G_Printf( "ET_MOVER            " );
			break;
		case ET_BEAM:
			G_Printf( "ET_BEAM             " );
			break;
		case ET_PORTAL:
			G_Printf( "ET_PORTAL           " );
			break;
		case ET_SPEAKER:
			G_Printf( "ET_SPEAKER          " );
			break;
		case ET_PUSH_TRIGGER:
			G_Printf( "ET_PUSH_TRIGGER     " );
			break;
		case ET_TELEPORT_TRIGGER:
			G_Printf( "ET_TELEPORT_TRIGGER " );
			break;
		case ET_INVISIBLE:
			G_Printf( "ET_INVISIBLE        " );
			break;
		case ET_GRAPPLE:
			G_Printf( "ET_GRAPPLE          " );
			break;
		case ET_EXPLOSIVE:
			G_Printf( "ET_EXPLOSIVE        " );
			break;
		case ET_TESLA_EF:
			G_Printf( "ET_TESLA_EF         " );
			break;
		case ET_SPOTLIGHT_EF:
			G_Printf( "ET_SPOTLIGHT_EF     " );
			break;
		case ET_EFFECT3:
			G_Printf( "ET_EFFECT3          " );
			break;
		case ET_ALARMBOX:
			G_Printf( "ET_ALARMBOX          " );
			break;
		default:
			G_Printf( "%3i                 ", check->s.eType );
			break;
		}

		if ( check->classname ) {
			G_Printf( "%s", check->classname );
		}
		G_Printf( "\n" );
	}
}

gclient_t   *ClientForString( const char *s ) {
	gclient_t   *cl;
	int i;
	int idnum;

	// numeric values are just slot numbers
	if ( s[0] >= '0' && s[0] <= '9' ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			Com_Printf( "Bad client slot: %i\n", idnum );
			return NULL;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			G_Printf( "Client %i is not connected\n", idnum );
			return NULL;
		}
		return cl;
	}

	// check for a name match
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		cl = &level.clients[i];
		if ( cl->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( !Q_stricmp( cl->pers.netname, s ) ) {
			return cl;
		}
	}

	G_Printf( "User %s is not on the server\n", s );

	return NULL;
}

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/
void    Svcmd_ForceTeam_f( void ) {
	gclient_t   *cl;
	char str[MAX_TOKEN_CHARS];

	// find the player
	trap_Argv( 1, str, sizeof( str ) );
	cl = ClientForString( str );
	if ( !cl ) {
		return;
	}

	// set the team
	trap_Argv( 2, str, sizeof( str ) );
	SetTeam( &g_entities[cl - level.clients], str );
}

char    *ConcatArgs( int start );

/*
=================
ConsoleCommand

=================
*/
qboolean    ConsoleCommand( void ) {
	char cmd[MAX_TOKEN_CHARS];

	trap_Argv( 0, cmd, sizeof( cmd ) );

	// Ridah, savegame
	if ( Q_stricmp( cmd, "savegame" ) == 0 ) {

		// don't allow a manual savegame command while we are waiting for the game to start/exit
		if ( g_reloading.integer ) {
			return qtrue;
		}
		if ( saveGamePending ) {
			return qtrue;
		}

		trap_Argv( 1, cmd, sizeof( cmd ) );
		if ( strlen( cmd ) > 0 ) {
			// strip the extension if provided
			if ( strrchr( cmd, '.' ) ) {
				cmd[strrchr( cmd,'.' ) - cmd] = '\0';
			}
			if ( !Q_stricmp( cmd, "current" ) ) {     // beginning of map
				Com_Printf( "sorry, '%s' is a reserved savegame name.  please use another name.\n", cmd );
				return qtrue;
			}

			if ( G_SaveGame( cmd ) ) {
				trap_SendServerCommand( -1, "cp gamesaved" );  // deletedgame
			} else {
				G_Printf( "Unable to save game.\n" );
			}

		} else {    // need a name
			G_Printf( "syntax: savegame <name>\n" );
		}

		return qtrue;
	}
	// done.

	if ( Q_stricmp( cmd, "entitylist" ) == 0 ) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "forceteam" ) == 0 ) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "game_memory" ) == 0 ) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "addbot" ) == 0 ) {
		Svcmd_AddBot_f();
		return qtrue;
	}

	// TTimo: took out games/g_arenas.c
	/*
	  if (Q_stricmp (cmd, "abort_podium") == 0) {
		  Svcmd_AbortPodium_f();
		  return qtrue;
	  }
	*/

	if ( Q_stricmp( cmd, "addip" ) == 0 ) {
		Svcmd_AddIP_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "removeip" ) == 0 ) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if ( Q_stricmp( cmd, "listip" ) == 0 ) {
		trap_SendConsoleCommand( EXEC_INSERT, "g_banIPs\n" );
		return qtrue;
	}

	if ( g_dedicated.integer ) {
		if ( Q_stricmp( cmd, "say" ) == 0 ) {
			trap_SendServerCommand( -1, va( "print \"server: %s\"", ConcatArgs( 1 ) ) );
			return qtrue;
		}
		// everything else will also be printed as a say command
		trap_SendServerCommand( -1, va( "print \"server: %s\"", ConcatArgs( 0 ) ) );
		return qtrue;
	}

	return qfalse;
}

