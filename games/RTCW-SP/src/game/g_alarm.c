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

#include "g_local.h"

/*
==============
alarmExplosion
	copied from propExplosion
==============
*/
void alarmExplosion( gentity_t *ent ) {

	// death sound
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );

	G_AddEvent( ent, EV_ENTDEATH, ent->s.eType );

	G_RadiusDamage( ent->s.origin, ent, ent->damage, ent->damage, ent, MOD_EXPLOSIVE );
	// return

	// old way.  (using grenade)
/*
	gentity_t *bolt;

	extern void G_ExplodeMissile( gentity_t *ent );
	bolt = G_Spawn();
	bolt->classname = "props_explosion";
	bolt->nextthink = level.time + FRAMETIME;
	bolt->think = G_ExplodeMissile;
	bolt->s.eType = ET_MISSILE;
	bolt->r.svFlags = SVF_USE_CURRENT_ORIGIN;

	bolt->s.weapon = WP_NONE;

	bolt->s.eFlags = EF_BOUNCE_HALF;
	bolt->r.ownerNum = ent->s.number;
	bolt->parent = ent;
	bolt->damage = ent->health;
	bolt->splashDamage = ent->health;
	bolt->splashRadius = ent->health * 1.5;
	bolt->methodOfDeath = MOD_GRENADE;
	bolt->splashMethodOfDeath = MOD_GRENADE_SPLASH;
	bolt->clipmask = MASK_SHOT;

	VectorCopy (ent->r.currentOrigin, bolt->s.pos.trBase );
	VectorCopy (ent->r.currentOrigin, bolt->r.currentOrigin);
*/
}


/*
==============
alarmbox_updateparts
==============
*/
void alarmbox_updateparts( gentity_t *ent, qboolean matestoo ) {
	gentity_t   *t, *mate;
	qboolean alarming = ( ent->s.frame == 1 );

	// update teammates
	if ( matestoo ) {
		for ( mate = ent->teammaster; mate; mate = mate->teamchain )
		{
			if ( mate == ent ) {
				continue;
			}

			if ( !( mate->active ) ) { // don't update dead alarm boxes, they stay dead
				continue;
			}

			if ( !( ent->active ) ) { // destroyed, so just turn teammates off
				mate->s.frame = 0;
			} else {
				mate->s.frame = ent->s.frame;
			}

			alarmbox_updateparts( mate, qfalse );
		}
	}

	// update lights
	if ( !ent->target ) {
		return;
	}

	t = NULL;
	while ( ( t = G_Find( t, FOFS( targetname ), ent->target ) ) != NULL )
	{
		if ( t == ent ) {
			G_Printf( "WARNING: Entity used itself.\n" );
		} else
		{
			// give the dlight the sound
			if ( !Q_stricmp( t->classname, "dlight" ) ) {
				t->soundLoop = ent->soundLoop;
				t->r.svFlags |= SVF_BROADCAST;  // no pvs

				if ( alarming ) {
					if ( !( t->r.linked ) ) {
						t->use( t, ent, 0 );
					}
				} else
				{
					if ( t->r.linked ) {
						t->use( t, ent, 0 );
					}
				}
			}
			// alarmbox can tell script_trigger about activation
			// (but don't trigger if dying, only activation)
			else if ( !Q_stricmp( t->classname, "target_script_trigger" ) ) {
				if ( ent->active && matestoo ) { // not dead (and this is the box that was used)
					t->use( t, ent, 0 );
				}
			}
		}
	}
}

/*
==============
alarmbox_use
==============
*/
void alarmbox_use( gentity_t *ent, gentity_t *other, gentity_t *foo ) {
	if ( !( ent->active ) ) {
		return;
	}

	if ( ent->s.frame ) {
		ent->s.frame = 0;
	} else {
		ent->s.frame = 1;
	}

	alarmbox_updateparts( ent, qtrue );
	if ( other->client ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3 );
	}
//	G_Printf("touched alarmbox\n");

}


/*
==============
alarmbox_die
==============
*/
void alarmbox_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	gentity_t *t;

	alarmExplosion( ent );
	ent->s.frame    = 2;
	ent->active     = qfalse;
	ent->takedamage = qfalse;
	alarmbox_updateparts( ent, qtrue );

	// fire 'death' targets
	if ( ent->targetdeath ) {
		t = NULL;
		while ( ( t = G_Find( t, FOFS( targetname ), ent->targetdeath ) ) != NULL )
		{
			if ( t == ent ) {
				G_Printf( "WARNING: Entity used itself.\n" );
			} else {
				// fire target
				t->use( t, ent, attacker );
			}
		}
	}

}




/*
==============
alarmbox_finishspawning
==============
*/
void alarmbox_finishspawning( gentity_t *ent ) {
	gentity_t *mate;

	// make sure they all have the same master (picked arbitrarily.  last spawned)
	for ( mate = ent; mate; mate = mate->teamchain )
		mate->teammaster = ent->teammaster;

	// find lights and set their state
	alarmbox_updateparts( ent, qtrue );
}


/*QUAKED alarm_box (1 0 1) START_ON
You need to have an origin brush as part of this entity
current alarm box model is (8 x 16 x 28)
"health" - defaults to 10
"dmg" - damage and radius value when it dies
"noise" - the sound to play over the system (this would be the siren sound)

START_ON means the button is pushed in, any dlights are cycling, and alarms are sounding

"team" key/value is valid for teamed alarm boxes
teamed alarm_boxes work in tandem (switches/lights syncronize)
target a box to dlights to have them activate/deactivate with the system (use a stylestring that matches the cycletime for the alarmbox sound)
alarm sound locations are also placed in the dlights, so wherever you place an attached dlight, you will hear the alarm
model: the model used is "models/mapobjects/electronics/alarmbox.md3"
place the origin at the center of your trigger box
*/
void SP_alarm_box( gentity_t *ent ) {
	char *s;

	if ( !ent->model ) {
		G_Printf( S_COLOR_RED "alarm_box with NULL model\n" );
		return;
	}

	// model
	trap_SetBrushModel( ent, ent->model );
	ent->s.modelindex2 = G_ModelIndex( "models/mapobjects/electronics/alarmbox.md3" );

	// sound
	if ( G_SpawnString( "noise", "0", &s ) ) {
		ent->soundLoop = G_SoundIndex( s );
	}

	// activation sound
	ent->soundPos3 = G_SoundIndex( "sound/world/alarmswitch.wav" );

	// death sound
	ent->sound1to2 = G_SoundIndex( "sound/world/alarmdeath.wav" );


	G_SetOrigin( ent, ent->s.origin );
	G_SetAngle( ent, ent->s.angles );

	if ( !ent->health ) {
		ent->health = 10;
	}

	if ( ent->spawnflags & 1 ) {
		ent->s.frame = 1;
	} else {
		ent->s.frame = 0;
	}

	ent->active     = qtrue;
	ent->s.eType    = ET_ALARMBOX;
	ent->takedamage = qtrue;
	ent->die        = alarmbox_die;
	ent->use        = alarmbox_use;
	ent->think      = alarmbox_finishspawning;
	ent->nextthink  = level.time + FRAMETIME;

	trap_LinkEntity( ent );
}


