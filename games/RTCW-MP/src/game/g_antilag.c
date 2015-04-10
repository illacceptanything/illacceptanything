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

#define IS_ACTIVE( x ) ( \
		x->r.linked == qtrue &&	\
		x->client->ps.stats[STAT_HEALTH] > 0 &&	\
		x->client->sess.sessionTeam != TEAM_SPECTATOR && \
		( x->client->ps.pm_flags & PMF_LIMBO ) == 0	\
		)

// OSP -
//	Aside from the inline edits, I also changed the client loops to poll only
//  the active client slots on the server, rather than looping through every
//  potential (and usually unused) slot.
//

void G_StoreClientPosition( gentity_t* ent ) {
	int top, currentTime;

	if ( !IS_ACTIVE( ent ) ) {
		return;
	}

	top = ent->client->topMarker;

	// new frame, mark the old marker's time as the end of the last frame
	if ( ent->client->clientMarkers[top].time < level.time ) {
		ent->client->clientMarkers[top].time = level.previousTime;
		top = ent->client->topMarker = ent->client->topMarker == MAX_CLIENT_MARKERS - 1 ? 0 : ent->client->topMarker + 1;
	}

	currentTime = level.previousTime + trap_Milliseconds() - level.frameTime;

	if ( currentTime > level.time ) {
		// owwie, we just went into the next frame... let's push them back
		currentTime = level.time;
	}

	VectorCopy( ent->r.mins,                        ent->client->clientMarkers[top].mins );
	VectorCopy( ent->r.maxs,                        ent->client->clientMarkers[top].maxs );
	VectorCopy( ent->r.currentOrigin,               ent->client->clientMarkers[top].origin );

	// OSP - these timers appear to be questionable
	ent->client->clientMarkers[top].servertime =    level.time;
	ent->client->clientMarkers[top].time =          currentTime;
}

static void G_AdjustSingleClientPosition( gentity_t* ent, int time ) {
	int i, j;

	if ( time > level.time ) {
		time = level.time;
	} // no lerping forward....

	i = j = ent->client->topMarker;
	do {
		if ( ent->client->clientMarkers[i].time <= time ) {
			break;
		}

		j = i;
		i = ( i > 0 ) ? i - 1 : MAX_CLIENT_MARKERS;
	} while ( i != ent->client->topMarker );

	if ( i == j ) { // oops, no valid stored markers
		return;
	}

	// OSP - I don't trust this caching, as the "warped" player's position updates potentially
	//       wont be counted after his think until the next server frame, which will result
	//       in a bad backupMarker
//	if( ent->client->backupMarker.time != level.time ) {
//		ent->client->backupMarker.time = level.time;
	VectorCopy( ent->r.currentOrigin,    ent->client->backupMarker.origin );
	VectorCopy( ent->r.mins,             ent->client->backupMarker.mins );
	VectorCopy( ent->r.maxs,             ent->client->backupMarker.maxs );
//	}

	if ( i != ent->client->topMarker ) {
		float frac = ( (float)( ent->client->clientMarkers[j].time - time ) ) / ( ent->client->clientMarkers[j].time - ent->client->clientMarkers[i].time );

		LerpPosition( ent->client->clientMarkers[j].origin,      ent->client->clientMarkers[i].origin,   frac,   ent->r.currentOrigin );
		LerpPosition( ent->client->clientMarkers[j].mins,        ent->client->clientMarkers[i].mins,     frac,   ent->r.mins );
		LerpPosition( ent->client->clientMarkers[j].maxs,        ent->client->clientMarkers[i].maxs,     frac,   ent->r.maxs );
	} else {
		VectorCopy( ent->client->clientMarkers[j].origin,       ent->r.currentOrigin );
		VectorCopy( ent->client->clientMarkers[j].mins,         ent->r.mins );
		VectorCopy( ent->client->clientMarkers[j].maxs,         ent->r.maxs );
	}

	trap_LinkEntity( ent );
}

static void G_ReAdjustSingleClientPosition( gentity_t* ent ) {

	// OSP - I don't trust this caching, as the "warped" player's position updates potentially
	//       wont be counted after his think until the next server frame
//	if( ent->client->backupMarker.time == level.time) {
	VectorCopy( ent->client->backupMarker.origin,       ent->r.currentOrigin );
	VectorCopy( ent->client->backupMarker.mins,         ent->r.mins );
	VectorCopy( ent->client->backupMarker.maxs,         ent->r.maxs );
	ent->client->backupMarker.servertime =  0;

	trap_LinkEntity( ent );
//	}
}

void G_AdjustClientPositions( gentity_t* ent, int time, qboolean forward ) {
	int i;
	gentity_t   *list;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		list = g_entities + level.sortedClients[i];
		if ( list != ent && IS_ACTIVE( list ) ) {
			if ( forward ) {
				G_AdjustSingleClientPosition( list, time );
			} else { G_ReAdjustSingleClientPosition( list );}
		}
	}
}

void G_ResetMarkers( gentity_t* ent ) {
	int i, time;
	char buffer[256];
	float period;

	trap_Cvar_VariableStringBuffer( "sv_fps", buffer, sizeof( buffer ) - 1 );

	period = atoi( buffer );
	period = ( period == 0 ) ? 50.0f : 1000.f / period;

	ent->client->topMarker = MAX_CLIENT_MARKERS - 1;
	for ( i = MAX_CLIENT_MARKERS, time = level.time; i >= 0; i--, time -= period ) {
		ent->client->clientMarkers[i].servertime =  time;
		ent->client->clientMarkers[i].time =        time;

		VectorCopy( ent->r.mins,            ent->client->clientMarkers[i].mins );
		VectorCopy( ent->r.maxs,            ent->client->clientMarkers[i].maxs );
		VectorCopy( ent->r.currentOrigin,   ent->client->clientMarkers[i].origin );
	}
}

void G_AttachBodyParts( gentity_t* ent ) {
	int i;
	gentity_t   *list;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		list = g_entities + level.sortedClients[i];
		list->client->tempHead = ( list != ent && IS_ACTIVE( list ) ) ? G_BuildHead( list ) : NULL;
	}
}

void G_DettachBodyParts() {
	int i;
	gentity_t   *list;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		list = g_entities + level.sortedClients[i];
		if ( list->client->tempHead != NULL ) {
			G_FreeEntity( list->client->tempHead );
		}
	}
}

int G_SwitchBodyPartEntity( gentity_t* ent ) {
	if ( ent->s.eType == ET_TEMPHEAD ) {
		return ent->parent - g_entities;
	}
	return ent - g_entities;
}

#define POSITION_READJUST											\
	if ( res != results->entityNum ) {							   \
		VectorSubtract( end, start, dir );						  \
		VectorNormalizeFast( dir );								  \
																	\
		VectorMA( results->endpos, -1, dir, results->endpos );	  \
		results->entityNum = res;								\
	}

// Run a trace with players in historical positions.
void G_HistoricalTrace( gentity_t* ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
	trace_t tr;
	gentity_t *other;
	int res;
	vec3_t dir;

	if ( !g_antilag.integer || !ent->client ) {
		G_AttachBodyParts( ent ) ;

		trap_Trace( results, start, mins, maxs, end, passEntityNum, contentmask );

		res = G_SwitchBodyPartEntity( &g_entities[results->entityNum] );
		POSITION_READJUST

		G_DettachBodyParts();
		return;
	}

	G_AdjustClientPositions( ent, ent->client->pers.cmd.serverTime, qtrue );

	G_AttachBodyParts( ent ) ;

	trap_Trace( results, start, mins, maxs, end, passEntityNum, contentmask );

	res = G_SwitchBodyPartEntity( &g_entities[results->entityNum] );
	POSITION_READJUST

	G_DettachBodyParts();

	G_AdjustClientPositions( ent, 0, qfalse );

	if ( results->entityNum >= 0 && results->entityNum < MAX_CLIENTS && ( other = &g_entities[results->entityNum] )->inuse ) {
		G_AttachBodyParts( ent ) ;

		trap_Trace( &tr, start, mins, maxs, other->client->ps.origin, passEntityNum, contentmask );
		res = G_SwitchBodyPartEntity( &g_entities[results->entityNum] );
		POSITION_READJUST

		if ( tr.entityNum != results->entityNum ) {
			trap_Trace( results, start, mins, maxs, end, passEntityNum, contentmask );
			res = G_SwitchBodyPartEntity( &g_entities[results->entityNum] );
			POSITION_READJUST
		}

		G_DettachBodyParts();
	}
}
