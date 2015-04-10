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
 * name:		g_target.c
 *
 * desc:
 *
*/

#include "g_local.h"

//==========================================================

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_Give( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t   *t;
	trace_t trace;

	if ( !activator->client ) {
		return;
	}

	if ( !ent->target ) {
		return;
	}

	memset( &trace, 0, sizeof( trace ) );
	t = NULL;
	while ( ( t = G_Find( t, FOFS( targetname ), ent->target ) ) != NULL ) {
		if ( !t->item ) {
			continue;
		}
		Touch_Item( t, activator, &trace );

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap_UnlinkEntity( t );
	}
}

void SP_target_give( gentity_t *ent ) {
	ent->use = Use_Target_Give;
}


//==========================================================

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
takes away all the activators powerups.
Used to drop flight powerups into death puts.
*/
void Use_target_remove_powerups( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( !activator->client ) {
		return;
	}

	if ( activator->client->ps.powerups[PW_REDFLAG] ) {
		Team_ReturnFlag( TEAM_RED );
	} else if ( activator->client->ps.powerups[PW_BLUEFLAG] ) {
		Team_ReturnFlag( TEAM_BLUE );
	}

	memset( activator->client->ps.powerups, 0, sizeof( activator->client->ps.powerups ) );
}

void SP_target_remove_powerups( gentity_t *ent ) {
	ent->use = Use_target_remove_powerups;
}


//==========================================================

/*QUAKED target_delay (1 1 0) (-8 -8 -8) (8 8 8)
"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
void Think_Target_Delay( gentity_t *ent ) {
	G_UseTargets( ent, ent->activator );
}

void Use_Target_Delay( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	ent->think = Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay( gentity_t *ent ) {
	// check delay for backwards compatability
	if ( !G_SpawnFloat( "delay", "0", &ent->wait ) ) {
		G_SpawnFloat( "wait", "1", &ent->wait );
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->use = Use_Target_Delay;
}


//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	AddScore( activator, ent->count );
}

void SP_target_score( gentity_t *ent ) {
	if ( !ent->count ) {
		ent->count = 1;
	}
	ent->use = Use_Target_Score;
}



//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
"message"	text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void Use_Target_Print( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( activator->client && ( ent->spawnflags & 4 ) ) {
		trap_SendServerCommand( activator - g_entities, va( "cp \"%s\"", ent->message ) );
		return;
	}

	if ( ent->spawnflags & 3 ) {
		if ( ent->spawnflags & 1 ) {
			G_TeamCommand( TEAM_RED, va( "cp \"%s\"", ent->message ) );
		}
		if ( ent->spawnflags & 2 ) {
			G_TeamCommand( TEAM_BLUE, va( "cp \"%s\"", ent->message ) );
		}
		return;
	}

	trap_SendServerCommand( -1, va( "cp \"%s\"", ent->message ) );
}

void SP_target_print( gentity_t *ent ) {
	ent->use = Use_Target_Print;
}


//==========================================================


/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) LOOPED_ON LOOPED_OFF GLOBAL ACTIVATOR VIS_MULTIPLE NO_PVS
"noise"		wav file to play

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
NO_PVS - this sound will not turn off when not in the player's PVS
"wait" : Seconds between auto triggerings, 0 = don't auto trigger
"random" : wait variance, default is 0
*/
void Use_Target_Speaker( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->spawnflags & 3 ) {  // looping sound toggles
		if ( ent->s.loopSound ) {
			ent->s.loopSound = 0;   // turn it off
		} else {
			ent->s.loopSound = ent->noise_index;    // start it
		}
	} else { // normal sound
		if ( ent->spawnflags & 8 ) {
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->noise_index );
		} else if ( ent->spawnflags & 4 ) {
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->noise_index );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );
		}
	}
}

void target_speaker_multiple( gentity_t *ent ) {
	gentity_t *vis_dummy = NULL;

	if ( !( ent->target ) ) {
		G_Error( "target_speaker missing target at pos %s", vtos( ent->s.origin ) );
	}

	vis_dummy = G_Find( NULL, FOFS( targetname ), ent->target );

	if ( vis_dummy ) {
		ent->s.otherEntityNum = vis_dummy->s.number;
	} else {
		G_Error( "target_speaker cant find vis_dummy_multiple %s", vtos( ent->s.origin ) );
	}

}

void SP_target_speaker( gentity_t *ent ) {
	char buffer[MAX_QPATH];
	char    *s;

	G_SpawnFloat( "wait", "0", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if ( !G_SpawnString( "noise", "NOSOUND", &s ) ) {
		G_Error( "target_speaker without a noise key at %s", vtos( ent->s.origin ) );
	}

	// force all client reletive sounds to be "activator" speakers that
	// play on the entity that activates it
	if ( s[0] == '*' ) {
		ent->spawnflags |= 8;
	}

	// Ridah, had to disable this so we can use sound scripts
	// don't worry, if the script isn't found, it'll default back to
	// .wav on the client-side
	//if (!strstr( s, ".wav" )) {
	//	Com_sprintf (buffer, sizeof(buffer), "%s.wav", s );
	//} else {
	Q_strncpyz( buffer, s, sizeof( buffer ) );
	//}
	ent->noise_index = G_SoundIndex( buffer );

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;


	// check for prestarted looping sound
	if ( ent->spawnflags & 1 ) {
		ent->s.loopSound = ent->noise_index;
	}

	ent->use = Use_Target_Speaker;

	// GLOBAL
	if ( ent->spawnflags & ( 4 | 32 ) ) {
		ent->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	if ( ent->spawnflags & 16 ) {
		ent->think = target_speaker_multiple;
		ent->nextthink = level.time + 50;
	}

	// NO_PVS
	if ( ent->spawnflags & 32 ) {
		ent->s.density = 1;
	} else {
		ent->s.density = 0;
	}

	if ( ent->radius ) {
		ent->s.dmgFlags = ent->radius;  // store radius in dmgflags
	} else {
		ent->s.dmgFlags = 0;
	}


	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( ent );
}



//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
void target_laser_think( gentity_t *self ) {
	vec3_t end;
	trace_t tr;
	vec3_t point;

	// if pointed at another entity, set movedir to point at it
	if ( self->enemy ) {
		VectorMA( self->enemy->s.origin, 0.5, self->enemy->r.mins, point );
		VectorMA( point, 0.5, self->enemy->r.maxs, point );
		VectorSubtract( point, self->s.origin, self->movedir );
		VectorNormalize( self->movedir );
	}

	// fire forward and see what we hit
	VectorMA( self->s.origin, 2048, self->movedir, end );

	trap_Trace( &tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE );

	if ( tr.entityNum ) {
		// hurt it if we can
		G_Damage( &g_entities[tr.entityNum], self, self->activator, self->movedir,
				  tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER );
	}

	VectorCopy( tr.endpos, self->s.origin2 );

	trap_LinkEntity( self );
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on( gentity_t *self ) {
	if ( !self->activator ) {
		self->activator = self;
	}
	target_laser_think( self );
}

void target_laser_off( gentity_t *self ) {
	trap_UnlinkEntity( self );
	self->nextthink = 0;
}

void target_laser_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	self->activator = activator;
	if ( self->nextthink > 0 ) {
		target_laser_off( self );
	} else {
		target_laser_on( self );
	}
}

void target_laser_start( gentity_t *self ) {
	gentity_t *ent;

	self->s.eType = ET_BEAM;

	if ( self->target ) {
		ent = G_Find( NULL, FOFS( targetname ), self->target );
		if ( !ent ) {
			G_Printf( "%s at %s: %s is a bad target\n", self->classname, vtos( self->s.origin ), self->target );
		}
		self->enemy = ent;
	} else {
		G_SetMovedir( self->s.angles, self->movedir );
	}

	self->use = target_laser_use;
	self->think = target_laser_think;

	if ( !self->damage ) {
		self->damage = 1;
	}

	if ( self->spawnflags & 1 ) {
		target_laser_on( self );
	} else {
		target_laser_off( self );
	}
}

void SP_target_laser( gentity_t *self ) {
	// let everything else get spawned before we start firing
	self->think = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}


//==========================================================

void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t   *dest;

	if ( !activator->client ) {
		return;
	}
	dest =  G_PickTarget( self->target );
	if ( !dest ) {
		G_Printf( "Couldn't find teleporter destination\n" );
		return;
	}

	TeleportPlayer( activator, dest->s.origin, dest->s.angles );
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter( gentity_t *self ) {
	if ( !self->targetname ) {
		G_Printf( "untargeted %s at %s\n", self->classname, vtos( self->s.origin ) );
	}

	self->use = target_teleporter_use;
}

//==========================================================


/*QUAKED target_relay (1 1 0) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM NOKEY_ONLY TAKE_KEY NO_LOCKED_NOISE
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them
"key" specifies an item you can be carrying that affects the operation of this relay
this key is currently an int (1-16) which matches the id of a key entity (key_key1 = 1, etc)
NOKEY_ONLY means "fire only if I do /not/ have the specified key"
TAKE_KEY removes the key from the players inventory
"lockednoise" specifies a .wav file to play if the relay is used and the player doesn't have the necessary key.
By default this sound is "sound/movers/doors/default_door_locked.wav"
NO_LOCKED_NOISE specifies that it will be silent if activated without proper key
*/
void target_relay_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if ( ( self->spawnflags & 1 ) && activator && activator->client
		 && activator->client->sess.sessionTeam != TEAM_RED ) {
		return;
	}
	if ( ( self->spawnflags & 2 ) && activator && activator->client
		 && activator->client->sess.sessionTeam != TEAM_BLUE ) {
		return;
	}

	if ( self->spawnflags & 4 ) {
		gentity_t   *ent;

		ent = G_PickTarget( self->target );
		if ( ent && ent->use ) {
			ent->use( ent, self, activator );
		}
		return;
	}

	if ( activator ) { // activator can be NULL if called from script
		if ( self->key ) {
			gitem_t *item;

//			if(self->key == -1)	// relay permanently locked
			if ( self->key >= KEY_LOCKED_ENT ) { // relay permanently locked
				if ( self->soundPos1 ) {
					G_Sound( self, self->soundPos1 );    //----(SA)	added
				}
				return;
			}

			item = BG_FindItemForKey( self->key, 0 );

			if ( item ) {
				if ( activator->client->ps.stats[STAT_KEYS] & ( 1 << item->giTag ) ) { // user has key
					if ( self->spawnflags & 8 ) {    // relay is NOKEY_ONLY and player has key
						if ( self->soundPos1 ) {
							G_Sound( self, self->soundPos1 );    //----(SA)	added
						}
						return;
					}
				} else                            // user does not have key
				{
					if ( !( self->spawnflags & 8 ) ) {
						if ( self->soundPos1 ) {
							G_Sound( self, self->soundPos1 );    //----(SA)	added
						}
						return;
					}
				}
			}

			if ( self->spawnflags & 16 ) { // (SA) take key
				activator->client->ps.stats[STAT_KEYS] &= ~( 1 << item->giTag );
				// (SA) TODO: "took inventory item" sound
			}
		}
	}

	G_UseTargets( self, activator );
}


void relay_AIScript_AlertEntity( gentity_t *self ) {
	self->use( self, NULL, NULL );
}


/*
==============
SP_target_relay
==============
*/
void SP_target_relay( gentity_t *self ) {
	char    *sound;
	int key;

	self->use = target_relay_use;
	self->AIScript_AlertEntity = relay_AIScript_AlertEntity;

	if ( G_SpawnInt( "key", "", &key ) ) {    // if door has a key entered, set it
		self->key = key;

		if ( key == -1 ) {
			self->key = KEY_LOCKED_ENT; // locked
		} else if ( self->key > KEY_NUM_KEYS || self->key < KEY_NONE ) {          // if the key is invalid, set the key in the finishSpawning routine
			G_Error( "invalid key (%d) set for func_door_rotating\n", self->key );
			self->key = KEY_NONE;   // un-locked
		}
	} else {
		self->key = KEY_NONE;   // un-locked
	}

	if ( !( self->spawnflags & 32 ) ) {  // !NO_LOCKED_NOISE
		if ( G_SpawnString( "lockednoise", "0", &sound ) ) {
			self->soundPos1 = G_SoundIndex( sound );
		} else {
			self->soundPos1 = G_SoundIndex( "sound/movers/doors/default_door_locked.wav" );
		}
	}

}


//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8) kill_user_too
Kills the activator. (default)
If targets, they will be killed when this is fired
"kill_user_too" will still kill the activator when this ent has targets (default is only kill targets, not activator)
*/
void target_kill_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t *targ = NULL;

	if ( self->spawnflags & 1 ) {  // kill usertoo
		G_Damage( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
	}

	while ( ( targ = G_Find( targ, FOFS( targetname ), self->target ) ) != NULL ) {
		if ( targ->aiCharacter ) {       // (SA) if it's an ai character, free it nicely
			targ->aiInactive = qtrue;
		} else
		{
			// make sure it isn't going to respawn or show any events
			targ->nextthink = 0;
			if ( targ == activator ) {
				continue;
			}

			// RF, script_movers should die!
			if ( !Q_stricmp( targ->classname, "script_mover" ) && targ->die ) {
				targ->die( targ, self, self, targ->health, 0 );
				continue;
			}

			trap_UnlinkEntity( targ );
			targ->use = 0;
			targ->touch = 0;
			targ->nextthink = level.time + FRAMETIME;
			targ->think = G_FreeEntity;
		}
	}
}

void SP_target_kill( gentity_t *self ) {
	self->use = target_kill_use;
}

/*DEFUNCT target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
void SP_target_position( gentity_t *self ) {
	G_SetOrigin( self, self->s.origin );
}

// Ridah, note to everyone: static functions can cause problems with the savegame code, so avoid
// using them unless necessary. if that function is then assigned to an entity, client or cast field,
// the game will not save.
//static void target_location_linkup(gentity_t *ent)
void target_location_linkup( gentity_t *ent ) {
	int i;
	int n;

	if ( level.locationLinked ) {
		return;
	}

	level.locationLinked = qtrue;

	level.locationHead = NULL;

	trap_SetConfigstring( CS_LOCATIONS, "unknown" );

	for ( i = 0, ent = g_entities, n = 1;
		  i < level.num_entities;
		  i++, ent++ ) {
		if ( ent->classname && !Q_stricmp( ent->classname, "target_location" ) ) {
			// lets overload some variables!
			ent->health = n; // use for location marking
			trap_SetConfigstring( CS_LOCATIONS + n, ent->message );
			n++;
			ent->nextTrain = level.locationHead;
			level.locationHead = ent;
		}
	}

	// All linked together now
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
void SP_target_location( gentity_t *self ) {
	self->think = target_location_linkup;
	self->nextthink = level.time + 200;  // Let them all spawn first

	G_SetOrigin( self, self->s.origin );
}







//---- (SA) Wolf targets

/*
==============
Use_Target_Autosave
	save game for emergency backup or convienience
==============
*/
void Use_Target_Autosave( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	G_SaveGame( "autosave.svg" );
}



/*
==============
Use_Target_Counter
==============
*/
void Use_Target_Counter( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->count < 0 ) { // if the count has already been hit, ignore this
		return;
	}

	ent->count -= 1;    // dec count

//	G_Printf("count at: %d\n", ent->count);

	if ( !ent->count ) {   // specified count is now hit
//		G_Printf("firing!!\n");
		G_UseTargets( ent, other );
	}
}

//==========================================================

/*
==============
Use_target_fog
==============
*/
void Use_target_fog( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
//	CS_FOGVARS reads:
//		near
//		far
//		density
//		r,g,b
//		time to complete
	trap_SetConfigstring( CS_FOGVARS, va( "%f %f %f %f %f %f %i", ent->accuracy, ent->random, 1.0f, (float)ent->dl_color[0], (float)ent->dl_color[1], (float)ent->dl_color[2], ent->s.time ) );
}

/*QUAKED target_fog (1 1 0) (-8 -8 -8) (8 8 8)
color picker chooses color of fog
"distance" sets fog distance.  Use value '0' to give control back to the game (and use the fog values specified in the sky shader if present)
distance value sets the type of fog.  values > 1 are distance fog (ex. 2048), values < 1 are density fog (ex. .0002)
"near" is fog start distance when using distance fog
"time" time it takes to change fog to new value.  default time is 1 sec
*/
void SP_target_fog( gentity_t *ent ) {
	int dist;
	float startdist;
	float ftime;

	ent->use = Use_target_fog;

	// ent->random will carry the 'far' or density value
	G_SpawnInt( "distance", "0", &dist );
	if ( dist >= 0 ) {
		ent->random = dist;
	}

	G_SpawnFloat( "near", "1.0", &startdist );
	ent->accuracy = startdist;


	// ent->s.time will carry the 'time' value
	G_SpawnFloat( "time", "0.5", &ftime );
	if ( ftime >= 0 ) {
		ent->s.time = ftime * 1000; // sec to ms
	}
}

//==========================================================

/*QUAKED target_counter (1 1 0) (-8 -8 -8) (8 8 8)
Increments the counter pointed to.
"count" is the key for the count value
*/
void SP_target_counter( gentity_t *ent ) {
//	G_Printf("target counter created with val of: %d\n", ent->count);
	ent->use = Use_Target_Counter;
}



/*QUAKED target_autosave (1 1 0) (-8 -8 -8) (8 8 8)
saves game to 'autosave.svg' when triggered then dies.
*/
void SP_target_autosave( gentity_t *ent ) {
	ent->use = Use_Target_Autosave;
}

//==========================================================

/*QUAKED target_lock (1 1 0) (-8 -8 -8) (8 8 8)
Sets the door to a state requiring key n
"key" is the required key
so:
key:0  unlocks the door
key:-1 locks the door until a target_lock with key:0
key:n  means the door now requires key n
*/

void Use_Target_Lock( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t   *t = 0;

	while ( ( t = G_Find( t, FOFS( targetname ), ent->target ) ) != NULL ) {
		t->key = ent->key;
		G_SetAASBlockingEntity( t, t->key != 0 );
	}

}

void SP_target_lock( gentity_t *ent ) {
	ent->use = Use_Target_Lock;
	if ( ent->key == -1 ) { // force locked
		ent->key = KEY_LOCKED_TRIGGERED;
	}
}



void Use_Target_Alarm( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	G_UseTargets( ent, other );
}

/*QUAKED target_alarm (1 1 0) (-4 -4 -4) (4 4 4)
does nothing yet (effectively a relay right now)
*/
void SP_target_alarm( gentity_t *ent ) {
	ent->use = Use_Target_Alarm;
}

//---- end

/*QUAKED target_smoke (1 0 0) (-4 -4 -4) (4 4 4) Black White SmokeON Gravity STEAM
1 second = 1000
1 FRAME = 100
delay = 100 = one millisecond default this is the maximum smoke that will show up
time = 5000 default before the smoke disipates
duration = 2000 before the smoke starts to alpha
start_size = 24 default
end_size = 96 default
wait	= default is 50 the rate at which it will travel up
*/

void smoke_think( gentity_t *ent ) {
	gentity_t   *tent;

	ent->nextthink = level.time + ent->delay;

	if ( !( ent->spawnflags & 4 ) ) {
		return;
	}

	if ( ent->health ) {
		ent->health--;
		if ( !ent->health ) {
			ent->think = G_FreeEntity;
			ent->nextthink = level.time + FRAMETIME;
		}
	}

	tent = G_TempEntity( ent->r.currentOrigin, EV_SMOKE );
	VectorCopy( ent->r.currentOrigin, tent->s.origin );
	tent->s.time = ent->speed;
	tent->s.time2 = ent->duration;
	tent->s.density = ent->s.density;

	// this is used to set the size of the smoke particle
	tent->s.angles2[0] = ent->start_size;
	tent->s.angles2[1] = ent->end_size;
	tent->s.angles2[2] = ent->wait;

	VectorCopy( ent->pos3, tent->s.origin2 );

	if ( ent->s.frame ) { // denotes reverse gravity effect
		tent->s.frame = 1;
	}

}

void smoke_toggle( gentity_t *ent, gentity_t *self, gentity_t *activator ) {
	if ( ent->spawnflags & 4 ) { // smoke is on turn it off
		ent->spawnflags &= ~4;
	} else
	{
		ent->spawnflags |= 4;
	}
}

void smoke_init( gentity_t *ent ) {
	gentity_t *target;
	vec3_t vec;

	ent->think = smoke_think;
	ent->nextthink = level.time + FRAMETIME;

	if ( ent->target ) {
		target = G_Find( NULL, FOFS( targetname ), ent->target );
		if ( target ) {
			VectorSubtract( target->s.origin, ent->s.origin, vec );
			VectorCopy( vec, ent->pos3 );
		} else {
			VectorSet( ent->pos3, 0, 0, 1 );
		}
	} else
	{
		VectorSet( ent->pos3, 0, 0, 1 );
	}

	trap_LinkEntity( ent );
}

void SP_target_smoke( gentity_t *ent ) {

	// (SA) don't use in multiplayer right now since it makes decyphering net messages almost impossible
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		ent->think = G_FreeEntity;
		return;
	}

	if ( !ent->delay ) {
		ent->delay = 100;
	}

	ent->use = smoke_toggle;

	ent->think = smoke_init;
	ent->nextthink = level.time + FRAMETIME;

	G_SetOrigin( ent, ent->s.origin );
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_GENERAL;

	if ( ent->spawnflags & 2 ) {
		ent->s.density = 4;
	} else if ( ent->spawnflags & 16 ) {
		ent->s.density = 7; // steam
	} else {
		ent->s.density = 0;
	}

	// using "time"
	if ( !ent->speed ) {
		ent->speed = 5000; // 5 seconds

	}
	if ( !ent->duration ) {
		ent->duration = 2000;
	}

	if ( !ent->start_size ) {
		ent->start_size = 24;
	}

	if ( !ent->end_size ) {
		ent->end_size = 96;
	}

	if ( !ent->wait ) {
		ent->wait = 50;
	}

	// idiot check
	if ( ent->speed < ent->duration ) {
		ent->speed = ent->duration + 100;
	}

	if ( ent->spawnflags & 8 ) {
		ent->s.frame = 1;
	}

	trap_LinkEntity( ent );

}


/*QUAKED target_script_trigger (1 .7 .2) (-8 -8 -8) (8 8 8)
must have an aiName
must have a target

when used it will fire its targets
*/
void target_script_trigger_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t   *player;

	if ( ent->aiName ) {
		player = AICast_FindEntityForName( "player" );
		if ( player ) {
			AICast_ScriptEvent( AICast_GetCastState( player->s.number ), "trigger", ent->target );
		}
	}

	// DHM - Nerve :: In multiplayer, we use the brush scripting only
	if ( g_gametype.integer == GT_WOLF && ent->scriptName ) {
		G_Script_ScriptEvent( ent, "trigger", ent->target );
	}

	G_UseTargets( ent, other );

}

void SP_target_script_trigger( gentity_t *ent ) {
	G_SetOrigin( ent, ent->s.origin );
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_GENERAL;
	ent->use = target_script_trigger_use;
}


/*QUAKED target_rumble (0 0.75 0.8) (-8 -8 -8) (8 8 8) STARTOFF
wait = default is 2 seconds = time the entity will enable rumble effect
"pitch" value from 1 to 10 default is 5
"yaw"   value from 1 to 10 default is 5

"rampup" how much time it will take to reach maximum pitch and yaw in seconds
"rampdown" how long till effect ends after rampup is reached in seconds

"startnoise" startingsound
"noise"  the looping sound entity is to make
"endnoise" endsound

"duration" the amount of time the effect is to last ei 1.0 sec 3.6 sec
*/
int rumble_snd;

void target_rumble_think( gentity_t * ent ) {
	//gentity_t	*tent;
	float ratio;
	float time, time2;
	float dapitch, dayaw;
	qboolean validrumble = qtrue;

	if ( !( ent->count ) ) {
		ent->timestamp = level.time;
		ent->count++;
		// start sound here
		if ( ent->soundPos1 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
		}
	} else
	{
		// looping sound
		ent->s.loopSound = ent->soundLoop;
	}

	dapitch = ent->delay;
	dayaw = ent->random;
	ratio = 1.0f;

	if ( ent->start_size ) {
		if ( level.time < ( ent->timestamp + ent->start_size ) ) {
			time = level.time - ent->timestamp;
			time2 = ( ent->timestamp + ent->start_size ) - ent->timestamp;
			ratio = time / time2;
		} else if ( level.time < ( ent->timestamp + ent->end_size + ent->start_size ) )       {
			time = level.time - ent->timestamp;
			time2 = ( ent->timestamp + ent->start_size + ent->end_size ) - ent->timestamp;
			ratio = time2 / time;
		} else {
			validrumble = qfalse;
		}
	}

	VectorClear( ent->s.angles );

	if ( validrumble ) {
		//ent->s.angles[0] = dapitch * ratio;
		//ent->s.angles[1] = dayaw * ratio;
		ent->s.angles[2] = ratio * dapitch / 24.0f; // CGAME uses this for shake effects, this is the scale
	}

	// end sound
	if ( level.time > ent->duration + ent->timestamp ) {
		if ( ent->soundPos2 ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
			ent->s.loopSound = 0;
		}

		ent->nextthink = 0;
	} else {
		ent->nextthink = level.time + 50;
	}

}

void target_rumble_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->spawnflags & 1 ) {
		// RF, broadcast this entity
		ent->r.svFlags |= SVF_BROADCAST;
		ent->spawnflags &= ~1;
		ent->think = target_rumble_think;
		ent->count = 0;
		ent->nextthink = level.time + 50;
	} else
	{
		// RF, don't broadcast this entity
		ent->r.svFlags &= ~SVF_BROADCAST;
		ent->spawnflags |= 1;
		ent->think = NULL;
		ent->count = 0;
	}
}

void SP_target_rumble( gentity_t *self ) {
	char        *pitch;
	char        *yaw;
	char        *rampup;
	char        *rampdown;
	float dapitch;
	float dayaw;
	char        *sound;
	char        *startsound;
	char        *endsound;

	if ( G_SpawnString( "noise", "100", &sound ) ) {
		self->soundLoop = G_SoundIndex( sound );
	}

	if ( G_SpawnString( "startnoise", "100", &startsound ) ) {
		self->soundPos1 = G_SoundIndex( startsound );
	}

	if ( G_SpawnString( "endnoise", "100", &endsound ) ) {
		self->soundPos2 = G_SoundIndex( endsound );
	}

	self->s.eType = ET_RUMBLE;

	self->use = target_rumble_use;

	G_SpawnString( "pitch", "0", &pitch );
	dapitch = atof( pitch );
	self->delay = dapitch;
	if ( !( self->delay ) ) {
		self->delay = 5;
	}

	G_SpawnString( "yaw", "0", &yaw );
	dayaw = atof( yaw );
	self->random = dayaw;
	if ( !( self->random ) ) {
		self->random = 5;
	}

	G_SpawnString( "rampup", "0", &rampup );
	self->start_size = atoi( rampup ) * 1000;
	if ( !( self->start_size ) ) {
		self->start_size = 1000;
	}

	G_SpawnString( "rampdown", "0", &rampdown );
	self->end_size = atoi( rampdown ) * 1000;
	if ( !( self->end_size ) ) {
		self->end_size = 1000;
	}

	if ( !( self->duration ) ) {
		self->duration = 1000;
	} else {
		self->duration *= 1000;
	}

	trap_LinkEntity( self );
}
