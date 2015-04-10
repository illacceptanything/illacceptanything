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

/*
 * name:	g_utils.c
 *
 * desc:	misc utility functions for game module
 *
*/

#include "g_local.h"

typedef struct {
	char oldShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	float timeOffset;
} shaderRemap_t;

#define MAX_SHADER_REMAPS 128

int remapCount = 0;
shaderRemap_t remappedShaders[MAX_SHADER_REMAPS];

void AddRemap( const char *oldShader, const char *newShader, float timeOffset ) {
	int i;

	for ( i = 0; i < remapCount; i++ ) {
		if ( Q_stricmp( oldShader, remappedShaders[i].oldShader ) == 0 ) {
			// found it, just update this one
			strcpy( remappedShaders[i].newShader,newShader );
			remappedShaders[i].timeOffset = timeOffset;
			return;
		}
	}
	if ( remapCount < MAX_SHADER_REMAPS ) {
		strcpy( remappedShaders[remapCount].newShader,newShader );
		strcpy( remappedShaders[remapCount].oldShader,oldShader );
		remappedShaders[remapCount].timeOffset = timeOffset;
		remapCount++;
	}
}

const char *BuildShaderStateConfig() {
	static char buff[MAX_STRING_CHARS * 4];
	char out[( MAX_QPATH * 2 ) + 5];
	int i;

	memset( buff, 0, MAX_STRING_CHARS );
	for ( i = 0; i < remapCount; i++ ) {
		Com_sprintf( out, ( MAX_QPATH * 2 ) + 5, "%s=%s:%5.2f@", remappedShaders[i].oldShader, remappedShaders[i].newShader, remappedShaders[i].timeOffset );
		Q_strcat( buff, sizeof( buff ), out );
	}
	return buff;
}

/*
=========================================================================

model / sound configstring indexes

=========================================================================
*/

/*
================
G_FindConfigstringIndex

================
*/
int G_FindConfigstringIndex( const char *name, int start, int max, qboolean create ) {
	int i;
	char s[MAX_STRING_CHARS];

	if ( !name || !name[0] ) {
		return 0;
	}

	for ( i = 1 ; i < max ; i++ ) {
		trap_GetConfigstring( start + i, s, sizeof( s ) );
		if ( !s[0] ) {
			break;
		}
		if ( !strcmp( s, name ) ) {
			return i;
		}
	}

	if ( !create ) {
		return 0;
	}

	if ( i == max ) {
		G_Error( "G_FindConfigstringIndex: overflow" );
	}

	trap_SetConfigstring( start + i, name );

	return i;
}


int G_ModelIndex( char *name ) {
	return G_FindConfigstringIndex( name, CS_MODELS, MAX_MODELS, qtrue );
}

int G_SoundIndex( const char *name ) {
	return G_FindConfigstringIndex( name, CS_SOUNDS, MAX_SOUNDS, qtrue );
}

//=====================================================================


/*
================
G_TeamCommand

Broadcasts a command to only a specific team
================
*/
void G_TeamCommand( team_t team, char *cmd ) {
	int i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			if ( level.clients[i].sess.sessionTeam == team ) {
				trap_SendServerCommand( i, va( "%s", cmd ) );
			}
		}
	}
}


/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
gentity_t *G_Find( gentity_t *from, int fieldofs, const char *match ) {
	char    *s;

	if ( !from ) {
		from = g_entities;
	} else {
		from++;
	}

	for ( ; from < &g_entities[level.num_entities] ; from++ )
	{
		if ( !from->inuse ) {
			continue;
		}
		s = *( char ** )( (byte *)from + fieldofs );
		if ( !s ) {
			continue;
		}
		if ( !Q_stricmp( s, match ) ) {
			return from;
		}
	}

	return NULL;
}


/*
=============
G_PickTarget

Selects a random entity from among the targets
=============
*/
#define MAXCHOICES  32

gentity_t *G_PickTarget( char *targetname ) {
	gentity_t   *ent = NULL;
	int num_choices = 0;
	gentity_t   *choice[MAXCHOICES];

	if ( !targetname ) {
		//G_Printf("G_PickTarget called with NULL targetname\n");
		return NULL;
	}

	while ( 1 )
	{
		ent = G_Find( ent, FOFS( targetname ), targetname );
		if ( !ent ) {
			break;
		}
		choice[num_choices++] = ent;
		if ( num_choices == MAXCHOICES ) {
			break;
		}
	}

	if ( !num_choices ) {
		G_Printf( "G_PickTarget: target %s not found\n", targetname );
		return NULL;
	}

	return choice[rand() % num_choices];
}


/*
==============================
G_UseTargets

"activator" should be set to the entity that initiated the firing.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets( gentity_t *ent, gentity_t *activator ) {
	gentity_t       *t;

	if ( !ent ) {
		return;
	}

	if ( ent->targetShaderName && ent->targetShaderNewName ) {
		float f = level.time * 0.001;
		AddRemap( ent->targetShaderName, ent->targetShaderNewName, f );
		trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );
	}

	if ( !ent->target ) {
		return;
	}

	t = NULL;
	while ( ( t = G_Find( t, FOFS( targetname ), ent->target ) ) != NULL ) {
		if ( t == ent ) {
			G_Printf( "WARNING: Entity used itself.\n" );
		} else {
			if ( t->use ) {
				//G_Printf ("ent->classname %s ent->targetname %s t->targetname %s t->s.number %d\n", ent->classname, ent->targetname, t->targetname, t->s.number);

				t->flags |= ( ent->flags & FL_KICKACTIVATE ); // (SA) If 'ent' was kicked to activate, pass this along to it's targets.
															  //		It may become handy to put a "KICKABLE" flag in ents so that it knows whether to pass this along or not
															  //		Right now, the only situation where it would be weird would be an invisible_user that is a 'button' near
															  //		a rotating door that it triggers.  Kick the switch and the door next to it flies open.

				t->flags |= ( ent->flags & FL_SOFTACTIVATE ); // (SA) likewise for soft activation

				if (    activator &&
						(   ( Q_stricmp( t->classname, "func_door" ) == 0 ) ||
							( Q_stricmp( t->classname, "func_door_rotating" ) == 0 )
						)
						) {
					// check door usage rules before allowing any entity to trigger a door open
					G_TryDoor( t, ent, activator );       // (door,other,activator)
				} else {
					t->use( t, ent, activator );
				}
			}
		}
		if ( !ent->inuse ) {
			G_Printf( "entity was removed while using targets\n" );
			return;
		}
	}
}


/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
/*
float	*tv( float x, float y, float z ) {
	static	int		index;
	static	vec3_t	vecs[8];
	float	*v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1)&7;

	v[0] = x;
	v[1] = y;
	v[2] = z;

	return v;
}
*/

/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char    *vtos( const vec3_t v ) {
	static int index;
	static char str[8][32];
	char    *s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 32, "(%i %i %i)", (int)v[0], (int)v[1], (int)v[2] );

	return s;
}
char    *vtosf( const vec3_t v ) {
	static int index;
	static char str[8][64];
	char    *s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 64, "(%f %f %f)", v[0], v[1], v[2] );

	return s;
}


/*
===============
G_SetMovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void G_SetMovedir( vec3_t angles, vec3_t movedir ) {
	static vec3_t VEC_UP        = {0, -1, 0};
	static vec3_t MOVEDIR_UP    = {0, 0, 1};
	static vec3_t VEC_DOWN      = {0, -2, 0};
	static vec3_t MOVEDIR_DOWN  = {0, 0, -1};

	if ( VectorCompare( angles, VEC_UP ) ) {
		VectorCopy( MOVEDIR_UP, movedir );
	} else if ( VectorCompare( angles, VEC_DOWN ) ) {
		VectorCopy( MOVEDIR_DOWN, movedir );
	} else {
		AngleVectors( angles, movedir, NULL, NULL );
	}
	VectorClear( angles );
}



void G_InitGentity( gentity_t *e ) {
	e->inuse = qtrue;
	e->classname = "noclass";
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
	e->headshotDamageScale = 1.0;   // RF, default value
	e->eventTime = 0;
	e->freeAfterEvent = qfalse;
	e->neverFree = qfalse;

	// RF, init scripting
	e->scriptStatus.scriptEventIndex = -1;
}

/*
=================
G_Spawn

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *G_Spawn( void ) {
	int i, force;
	gentity_t   *e;

	e = NULL;   // shut up warning
	i = 0;      // shut up warning
	for ( force = 0 ; force < 2 ; force++ ) {
		// if we go through all entities and can't find one to free,
		// override the normal minimum times before use
		e = &g_entities[MAX_CLIENTS];
		for ( i = MAX_CLIENTS ; i < level.num_entities ; i++, e++ ) {
			if ( e->inuse ) {
				continue;
			}

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if ( !force && e->freetime > level.startTime + 2000 && level.time - e->freetime < 1000 ) {
				continue;
			}

			// reuse this slot
			G_InitGentity( e );
			return e;
		}
		if ( i != MAX_GENTITIES ) {
			break;
		}
	}
	if ( i == ENTITYNUM_MAX_NORMAL ) {
		for ( i = 0; i < MAX_GENTITIES; i++ ) {
			G_Printf( "%4i: %s\n", i, g_entities[i].classname );
		}
		G_Error( "G_Spawn: no free entities" );
	}

	// open up a new slot
	level.num_entities++;

	// let the server system know that there are more entities
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
						 &level.clients[0].ps, sizeof( level.clients[0] ) );

	G_InitGentity( e );
	return e;
}

/*
=================
G_EntitiesFree
=================
*/
qboolean G_EntitiesFree( void ) {
	int i;
	gentity_t   *e;

	e = &g_entities[MAX_CLIENTS];
	for ( i = MAX_CLIENTS; i < level.num_entities; i++, e++ ) {
		if ( e->inuse ) {
			continue;
		}
		// slot available
		return qtrue;
	}
	return qfalse;
}


/*
=================
G_FreeEntity

Marks the entity as free
=================
*/
void G_FreeEntity( gentity_t *ed ) {
	trap_UnlinkEntity( ed );     // unlink from world

	if ( ed->neverFree ) {
		return;
	}

	memset( ed, 0, sizeof( *ed ) );
	ed->classname = "freed";
	ed->freetime = level.time;
	ed->inuse = qfalse;
}

/*
=================
G_TempEntity

Spawns an event entity that will be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *G_TempEntity( vec3_t origin, int event ) {
	gentity_t       *e;
	vec3_t snapped;

	e = G_Spawn();
	e->s.eType = ET_EVENTS + event;

	e->classname = "tempEntity";
	e->eventTime = level.time;
	e->r.eventTime = level.time;
	e->freeAfterEvent = qtrue;

	VectorCopy( origin, snapped );
	SnapVector( snapped );      // save network bandwidth
	G_SetOrigin( e, snapped );

	// find cluster for PVS
	trap_LinkEntity( e );

	return e;
}



/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
G_KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
void G_KillBox( gentity_t *ent ) {
	int i, num;
	int touch[MAX_GENTITIES];
	gentity_t   *hit;
	vec3_t mins, maxs;

	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for ( i = 0 ; i < num ; i++ ) {
		hit = &g_entities[touch[i]];
		if ( !hit->client ) {
			continue;
		}
		if ( !hit->r.linked ) { // RF, inactive AI shouldn't be gibbed
			continue;
		}

		// nail it
		G_Damage( hit, ent, ent, NULL, NULL,
				  100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
	}

}

//==============================================================================

/*
===============
G_AddPredictableEvent

Use for non-pmove events that would also be predicted on the
client side: jumppads and item pickups
Adds an event+parm and twiddles the event counter
===============
*/
void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm ) {
	if ( !ent->client ) {
		return;
	}
	BG_AddPredictableEventToPlayerstate( event, eventParm, &ent->client->ps );
}


/*
===============
G_AddEvent

Adds an event+parm and twiddles the event counter
===============
*/
void G_AddEvent( gentity_t *ent, int event, int eventParm ) {
//	int		bits;

	if ( !event ) {
		G_Printf( "G_AddEvent: zero event added for entity %i\n", ent->s.number );
		return;
	}

	// Ridah, use the sequential event list
	if ( ent->client ) {
		// NERVE - SMF - commented in - externalEvents not being handled properly in Wolf right now
		ent->client->ps.events[ent->client->ps.eventSequence & ( MAX_EVENTS - 1 )] = event;
		ent->client->ps.eventParms[ent->client->ps.eventSequence & ( MAX_EVENTS - 1 )] = eventParm;
		ent->client->ps.eventSequence++;
		// -NERVE - SMF

		// NERVE - SMF - commented out
//		bits = ent->client->ps.externalEvent & EV_EVENT_BITS;
//		bits = ( bits + EV_EVENT_BIT1 ) & EV_EVENT_BITS;
//		ent->client->ps.externalEvent = event | bits;
//		ent->client->ps.externalEventParm = eventParm;
//		ent->client->ps.externalEventTime = level.time;
		// -NERVE - SMF
	} else {
		// NERVE - SMF - commented in - externalEvents not being handled properly in Wolf right now
		ent->s.events[ent->s.eventSequence & ( MAX_EVENTS - 1 )] = event;
		ent->s.eventParms[ent->s.eventSequence & ( MAX_EVENTS - 1 )] = eventParm;
		ent->s.eventSequence++;
		// -NERVE - SMF

		// NERVE - SMF - commented out
//		bits = ent->s.event & EV_EVENT_BITS;
//		bits = ( bits + EV_EVENT_BIT1 ) & EV_EVENT_BITS;
//		ent->s.event = event | bits;
//		ent->s.eventParm = eventParm;
		// -NERVE - SMF
	}
	ent->eventTime = level.time;
	ent->r.eventTime = level.time;
}


/*
=============
G_Sound

  Ridah, removed channel parm, since it wasn't used, and could cause confusion
=============
*/
void G_Sound( gentity_t *ent, int soundIndex ) {
	gentity_t   *te;

	te = G_TempEntity( ent->r.currentOrigin, EV_GENERAL_SOUND );
	te->s.eventParm = soundIndex;
}

/*
=============
G_AnimScriptSound
=============
*/
void G_AnimScriptSound( int soundIndex, vec3_t org, int client ) {
	gentity_t *e;
	e = &g_entities[client];
	G_AddEvent( e, EV_GENERAL_SOUND, soundIndex );
	AICast_RecordScriptSound( client );
}

//==============================================================================


/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
*/
void G_SetOrigin( gentity_t *ent, vec3_t origin ) {
	VectorCopy( origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	VectorClear( ent->s.pos.trDelta );

	VectorCopy( origin, ent->r.currentOrigin );
}


/*
==============
G_SetOrigin
==============
*/
void G_SetAngle( gentity_t *ent, vec3_t angle ) {

	VectorCopy( angle, ent->s.apos.trBase );
	ent->s.apos.trType = TR_STATIONARY;
	ent->s.apos.trTime = 0;
	ent->s.apos.trDuration = 0;
	VectorClear( ent->s.apos.trDelta );

	VectorCopy( angle, ent->r.currentAngles );

//	VectorCopy (ent->s.angles, ent->s.apos.trDelta );

}

/*
====================
infront
====================
*/

qboolean infront( gentity_t *self, gentity_t *other ) {
	vec3_t vec;
	float dot;
	vec3_t forward, otherOrigin;

	if ( self->client ) {
		AngleVectors( self->client->ps.viewangles, forward, NULL, NULL );
	} else {
		AngleVectors( self->s.angles, forward, NULL, NULL );
	}


	if ( self->activateArc ) {
		// move the origin of the 'other' up/down so that it matches the 'self' so the check is along a horizontal plane
		VectorCopy( other->r.currentOrigin, otherOrigin );
		otherOrigin[2] = self->r.currentOrigin[2];
		VectorSubtract( otherOrigin, self->r.currentOrigin, vec );
	} else {
		VectorSubtract( other->r.currentOrigin, self->r.currentOrigin, vec );
	}

	VectorNormalize( vec );
	dot = DotProduct( vec, forward );
	// G_Printf( "other %5.2f\n",	dot);

	if ( !other->aiCharacter && self->activateArc ) {  //----(SA)	make sure ai's aren't constrained to the grabarc of an mg42
		float angle;
		angle = RAD2DEG( M_PI - acos( dot ) );
		if ( angle < ( self->activateArc * 2.0 ) ) { // arc is 'half arc' since that's the way the other angles in the mg42 were done
			return qfalse;
		} else {
			return qtrue;
		}
	}

	if ( dot > 0.0 ) {
		return qtrue;
	}
	return qfalse;
}

//RF, tag connections
/*
==================
G_ProcessTagConnect
==================
*/
void G_ProcessTagConnect( gentity_t *ent, qboolean clearAngles ) {
	if ( !ent->tagName ) {
		G_Error( "G_ProcessTagConnect: NULL ent->tagName\n" );
	}
	if ( !ent->tagParent ) {
		G_Error( "G_ProcessTagConnect: NULL ent->tagParent\n" );
	}
	G_FindConfigstringIndex( va( "%i %i %s", ent->s.number, ent->tagParent->s.number, ent->tagName ), CS_TAGCONNECTS, MAX_TAGCONNECTS, qtrue );
	ent->s.eFlags |= EF_TAGCONNECT;

	if ( clearAngles ) {
		// clear out the angles so it always starts out facing the tag direction
		VectorClear( ent->s.angles );
		VectorCopy( ent->s.angles, ent->s.apos.trBase );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trDuration = 0;
		ent->s.apos.trType = TR_STATIONARY;
		VectorClear( ent->s.apos.trDelta );
		VectorClear( ent->r.currentAngles );
	}
}

/*
================
DebugLine

  debug polygons only work when running a local game
  with r_debugSurface set to 2
================
*/
int DebugLine( vec3_t start, vec3_t end, int color ) {
	vec3_t points[4], dir, cross, up = {0, 0, 1};
	float dot;

	VectorCopy( start, points[0] );
	VectorCopy( start, points[1] );
	//points[1][2] -= 2;
	VectorCopy( end, points[2] );
	//points[2][2] -= 2;
	VectorCopy( end, points[3] );


	VectorSubtract( end, start, dir );
	VectorNormalize( dir );
	dot = DotProduct( dir, up );
	if ( dot > 0.99 || dot < -0.99 ) {
		VectorSet( cross, 1, 0, 0 );
	} else { CrossProduct( dir, up, cross );}

	VectorNormalize( cross );

	VectorMA( points[0], 2, cross, points[0] );
	VectorMA( points[1], -2, cross, points[1] );
	VectorMA( points[2], -2, cross, points[2] );
	VectorMA( points[3], 2, cross, points[3] );

	return trap_DebugPolygonCreate( color, 4, points );
}
