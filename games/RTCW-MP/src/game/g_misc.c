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

/*
 * name:		g_misc.c
 *
 * desc:
 *
*/


#include "g_local.h"

extern void AimAtTarget( gentity_t * self );

int sniper_sound;
int snd_noammo;

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.
*/


/*QUAKED info_camp (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_camp( gentity_t *self ) {
	G_SetOrigin( self, self->s.origin );
}


/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_null( gentity_t *self ) {
	G_FreeEntity( self );
}


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void SP_info_notnull( gentity_t *self ) {
	G_SetOrigin( self, self->s.origin );
}


/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) nonlinear angle negative_spot negative_point q3map_non-dynamic
Non-displayed light.
"light" overrides the default 300 intensity.
Nonlinear checkbox gives inverse square falloff instead of linear
Angle adds light:surface angle calculations (only valid for "Linear" lights) (wolf)
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"fade" falloff/radius adjustment value. multiply the run of the slope by "fade" (1.0f default) (only valid for "Linear" lights) (wolf)
"q3map_non-dynamic" specifies that this light should not contribute to the world's 'light grid' and therefore will not light dynamic models in the game.(wolf)
*/
void SP_light( gentity_t *self ) {
	G_FreeEntity( self );
}

/*QUAKED lightJunior (0 0.7 0.3) (-8 -8 -8) (8 8 8) nonlinear angle negative_spot negative_point
Non-displayed light that only affects dynamic game models, but does not contribute to lightmaps
"light" overrides the default 300 intensity.
Nonlinear checkbox gives inverse square falloff instead of linear
Angle adds light:surface angle calculations (only valid for "Linear" lights) (wolf)
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
"fade" falloff/radius adjustment value. multiply the run of the slope by "fade" (1.0f default) (only valid for "Linear" lights) (wolf)
*/
void SP_lightJunior( gentity_t *self ) {
	G_FreeEntity( self );
}



/*
=================================================================================

TELEPORTERS

=================================================================================
*/
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles ) {
	gentity_t   *tent;

	// use temp events at source and destination to prevent the effect
	// from getting dropped by a second player event
	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		tent = G_TempEntity( player->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = player->s.clientNum;

		tent = G_TempEntity( origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = player->s.clientNum;
	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	trap_UnlinkEntity( player );

	VectorCopy( origin, player->client->ps.origin );
	player->client->ps.origin[2] += 1;

	// spit the player out
	AngleVectors( angles, player->client->ps.velocity, NULL, NULL );
	VectorScale( player->client->ps.velocity, 400, player->client->ps.velocity );
	player->client->ps.pm_time = 160;       // hold time
	player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	SetClientViewAngle( player, angles );

	// kill anything at the destination
	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		G_KillBox( player );
	}

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->r.currentOrigin );

	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		trap_LinkEntity( player );
	}
}


/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void SP_misc_teleporter_dest( gentity_t *ent ) {
}


/*
=================================================================================

	misc_grabber_trap

*/


static int attackDurations[] = {    ( 11 * 1000 ) / 15,
									( 16 * 1000 ) / 15,
									( 16 * 1000 ) / 15 };

static int attackHittimes[] = {     ( 7 * 1000 ) / 15,
									( 6 * 1000 ) / 15,
									( 7 * 1000 ) / 15 };

/*
==============
grabber_think_idle
	think func for the grabber ent to reset to idle if not attacking
==============
*/
void grabber_think_idle( gentity_t *ent ) {
	if ( ent->s.frame > 1 ) {  // non-idle status
		ent->s.frame = rand() % 2;
	}
}

/*
==============
grabber_think_hit
	think func for grabber ent following an attack command
==============
*/
void grabber_think_hit( gentity_t *ent ) {
	G_RadiusDamage( ent->s.pos.trBase, ent, ent->damage, ent->duration, ent, MOD_GRABBER );
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 ); // sound2to1 is the 'pain' sound

	ent->nextthink  = level.time + ( attackDurations[( ent->s.frame ) - 2] - attackHittimes[( ent->s.frame ) - 2] );
	ent->think      = grabber_think_idle;
}


/*
==============
grabber_die
==============
*/
extern void GibEntity( gentity_t * self, int killer ) ;

void grabber_die( gentity_t *ent, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {

	// FIXME FIXME
	// this is buggy.  the trigger brush entity (ent->enemy) does not free.
	// need to fix.

	GibEntity( ent, 0 );    // use temporarily to show 'death' of entity

//	trap_UnlinkEntity(ent->enemy);
	ent->enemy->think = G_FreeEntity;
	ent->enemy->nextthink = level.time + FRAMETIME;
//	G_FreeEntity(ent->enemy);

	G_UseTargets( ent, attacker );

//	trap_UnlinkEntity(ent);
	ent->think = G_FreeEntity;
	ent->nextthink = level.time + FRAMETIME;
//	G_FreeEntity(ent);
}




/*
==============
grabber_attack
	direct call to the grabber entity (not a trigger) to call the attack
==============
*/
void grabber_attack( gentity_t *ent ) {
	ent->s.frame    = ( rand() % 3 ) + 2;   // randomly choose an attack sequence

	ent->nextthink  = level.time + attackHittimes[( ent->s.frame ) - 2];
	ent->think      = grabber_think_hit;
}

/*
==============
grabber_close
	touch func for attack distance trigger entity
==============
*/
void grabber_close( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( ent->parent->nextthink > level.time ) {
		return;
	}

	grabber_attack( ent->parent );
}



/*
==============
grabber_pain
	pain func for the grabber entity (not triggers)
==============
*/
void grabber_pain( gentity_t *ent, gentity_t *attacker, int damage, vec3_t point ) {
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 ); // sound2to1 is the 'pain' sound
}


/*
==============
grabber_wake
	ent calling this is the bounding box for the grabber, not the grabber ent itself.
	the grabber ent is 'ent->parent'
==============
*/
void grabber_wake( gentity_t *ent ) {
	gentity_t *parent;

	parent = ent->parent;

	// change the 'a' trigger to the 'b' trigger for grabber attacking
	VectorCopy( parent->s.origin, ent->r.mins );
	VectorCopy( parent->s.origin, ent->r.maxs );

	if ( 1 ) {     // temp fast trigger
		VectorAdd( ent->r.mins, tv( -( ent->random ), -( ent->random ), -( ent->random ) ), ent->r.mins );
		VectorAdd( ent->r.maxs, tv( ent->random, ent->random, ent->random ), ent->r.maxs );
	}

	ent->touch = grabber_close;

	// parent entity: show model/play anim/take damage
	{
		parent->clipmask    = CONTENTS_SOLID;
		parent->r.contents  = CONTENTS_SOLID;
		parent->takedamage  = qtrue;
		parent->active      = qtrue;
		parent->die         = grabber_die;
		parent->pain        = grabber_pain;
		trap_LinkEntity( parent );

		ent->s.frame        = 5;    // starting position

		// go back to an idle if not attacking immediately
		parent->nextthink   = level.time + FRAMETIME;
		parent->think       = grabber_think_idle;
	}

	G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 ); // soundPos1 is the 'wake' sound
}


/*
==============
grabber_use
	use func for the grabber entity
	if not awake, allow waking by trigger
	if awake, allow attacking by trigger
==============
*/
void grabber_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	G_Printf( "grabber_use: %d\n", level.time );

	if ( !ent->active ) {
		grabber_wake( ent );
	} else {
		grabber_attack( ent );
	}
}

/*
==============
grabber_wake_touch
	touch func for the first 'wake' trigger entity
==============
*/
void grabber_wake_touch( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	grabber_wake( ent );
}


/*QUAKED misc_grabber_trap (1 0 0) (-8 -8 -8) (8 8 8)
fields:
"adist"  - radius of 'wakeup' box.  player passing closer than distance activates grabber (def: 64)
"bdist"  - radius of 'attack' box.  player passing into this gets a swipe.  (def: 32)
"health" - how much damage grabber can take after 'wakeup' (def: 100)
"range"  - when attacking, how far from the origin the grabber can strike (def: 64)
"dmg"    - max damage to give on a successful strike (def: 10)
"wait"   - how long to wait between strikes if the player stays within the 'attack' box (def: see below)

If you do not set a "wait" value, then it will default to the duration of the animations.  (so since the first attack animation is 11 frames long and plays at 15 fps, the default wait after using attack 1 would be 11/15, or 0.73 seconds)

grabber media:
model - "models/misc/grabber/grabber.md3"
wake sound - "models/misc/grabber/grabber_wake.wav"
attack sound - "models/misc/grabber/grabber_attack.wav"
pain sound - "models/misc/grabber/grabber_pain.wav"

The current frames are:
first frame
|   length
	|   looping frames
		|   fps
			|   damage at frame
				|
0   6   6   5   0  (main idle)
5   21  21  7   0  (random idle)
25  11  10  15  7  (attack big swipe)
35  16  0   15  6  (attack small swipe)
50  16  0   15  7  (attack grab)
66  1   1   15  0  (starting position)

*/
void SP_misc_grabber_trap( gentity_t *ent ) {
	int adist, bdist, range;
	gentity_t   *trig;

	// TODO: change from 'trap' to something else.  'trap' is a misnomer.  it's actually used for other stuff too
	ent->s.eType        = ET_TRAP;

	// TODO: make these user assignable?
	ent->s.modelindex   = G_ModelIndex( "models/misc/grabber/grabber.md3" );
	ent->soundPos1      = G_SoundIndex( "models/misc/grabber/grabber_wake.wav" );
	ent->sound1to2      = G_SoundIndex( "models/misc/grabber/grabber_attack.wav" );
	ent->sound2to1      = G_SoundIndex( "models/misc/grabber/grabber_pain.wav" );

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	ent->s.apos.trBase[YAW] -= 90;  // adjust for model rotation


	if ( !ent->health ) {
		ent->health = 100;  // default to 100

	}
	if ( !ent->damage ) {
		ent->damage = 10;   // default to 10

	}
	ent->s.frame    = 5;

	ent->use        = grabber_use;  // allow 'waking' from trigger

	VectorSet( ent->r.mins, -12, -12, 0 );   // target area for shooting it after it wakes
	VectorSet( ent->r.maxs, 12, 12, 48 );

	// create the 'a' trigger for waking up the grabber
	trig = ent->enemy = G_Spawn();

	VectorCopy( ent->s.origin, trig->r.mins );
	VectorCopy( ent->s.origin, trig->r.maxs );

	// store attack range in 'duration'
	G_SpawnInt( "range", "64", &range );
	ent->duration = range;

	// store adist/bdist in 'count/random' of the trigger brush ent
	G_SpawnInt( "adist", "64", &adist );
	trig->count = adist;
	G_SpawnInt( "bdist", "32", &bdist );
	trig->random = bdist;

	// just make an even trigger box around the ent (do properly sized/oriented trigger after it's working)
	if ( 1 ) {     // temp fast trigger
		VectorAdd( trig->r.mins, tv( -( trig->count ), -( trig->count ), -( trig->count ) ), trig->r.mins );
		VectorAdd( trig->r.maxs, tv( trig->count, trig->count, trig->count ), trig->r.maxs );
	}

	trig->parent        = ent;
	trig->r.contents    = CONTENTS_TRIGGER;
	trig->r.svFlags     = SVF_NOCLIENT;
	trig->touch         = grabber_wake_touch;
	trap_LinkEntity( trig );

}

void use_spotlight( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t   *tent;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	} else
	{
		tent = G_PickTarget( ent->target );
		VectorCopy( tent->s.origin, ent->s.origin2 );

		ent->active = 0;
		trap_LinkEntity( ent );
	}
}


void spotlight_finish_spawning( gentity_t *ent ) {
	if ( ent->spawnflags & 1 ) {   // START_ON
		ent->active = 0;
		trap_LinkEntity( ent );
	}

	ent->use        = use_spotlight;
	ent->think      = 0;
	ent->nextthink  = 0;
}


//----(SA)	added
/*QUAKED misc_spotlight (1 0 0) (-16 -16 -16) (16 16 16) START_ON BACK_AND_FORTH
"model" - 'base' model that moves with the light.  Default: "models/mapobjects/light/searchlight_pivot.md3"
"target" - .camera (spline) file for light to track.  do not specify file extension.

BACK_AND_FORTH - when end of target spline is hit, reverse direction rather than looping (looping is default)
( /\ not active yet /\ )
*/
void SP_misc_spotlight( gentity_t *ent ) {

	ent->s.eType        = ET_EF_SPOTLIGHT;

	ent->think = spotlight_finish_spawning;
	ent->nextthink = level.time + 100;

	if ( ent->model ) {
		ent->s.modelindex   = G_ModelIndex( ent->model );
	} else {
		ent->s.modelindex   = G_ModelIndex( "models/mapobjects/light/searchlight_pivot.md3" );
	}

	if ( ent->target ) {
		ent->s.density = G_FindConfigstringIndex( ent->target, CS_SPLINES, MAX_SPLINE_CONFIGSTRINGS, qtrue );
	}

}

//----(SA)	end

//===========================================================

/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"		arbitrary .md3 file to display
"modelscale"	scale multiplier (defaults to 1x)
"modelscale_vec"	scale multiplier (defaults to 1 1 1, scales each axis as requested)

"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both, the "modelscale" is ignored
*/
void SP_misc_model( gentity_t *ent ) {
	G_FreeEntity( ent );
}


//----(SA)
/*QUAKED misc_gamemodel (1 0 0) (-16 -16 -16) (16 16 16) ORIENT_LOD
md3 placed in the game at runtime (rather than in the bsp)
"model"			arbitrary .md3 file to display
"modelscale"	scale multiplier (defaults to 1x, and scales uniformly)
"modelscale_vec"	scale multiplier (defaults to 1 1 1, scales each axis as requested)
"trunk"			diameter of solid core (used for trace visibility and collision (not ai pathing))
"trunkheight"	height of trunk
ORIENT_LOD - if flagged, the entity will yaw towards the player when the LOD switches

"modelscale_vec" - Set scale per-axis.  Overrides "modelscale", so if you have both, the "modelscale" is ignored

*/
void SP_misc_gamemodel( gentity_t *ent ) {

	float scale[3] = {1,1,1};
	vec3_t scalevec;
	int trunksize, trunkheight;

	ent->s.eType        = ET_GAMEMODEL;
	ent->s.modelindex   = G_ModelIndex( ent->model );

	// look for general scaling
	if ( G_SpawnFloat( "modelscale", "1", &scale[0] ) ) {
		scale[2] = scale[1] = scale[0];
	}

	// look for axis specific scaling
	if ( G_SpawnVector( "modelscale_vec", "1 1 1", &scalevec[0] ) ) {
		VectorCopy( scalevec, scale );
	}

	G_SpawnInt( "trunk", "0", &trunksize );
	if ( !G_SpawnInt( "trunkhight", "0", &trunkheight ) ) {
		trunkheight = 256;
	}

	if ( trunksize ) {
		float rad;

		ent->clipmask       = CONTENTS_SOLID;
		ent->r.contents     = CONTENTS_SOLID;

		ent->r.svFlags |= SVF_CAPSULE;

		rad = (float)trunksize / 2.0f;
		VectorSet( ent->r.mins, -rad, -rad, 0 );
		VectorSet( ent->r.maxs, rad, rad, trunkheight );
	}

	// scale is stored in 'angles2'
	VectorCopy( scale, ent->s.angles2 );

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	if ( ent->spawnflags & 1 ) {
		ent->s.apos.trType = 1; // misc_gamemodels (since they have no movement) will use type = 0 for static models, type = 1 for auto-aligning models


	}
	trap_LinkEntity( ent );

}




//----(SA)

void locateMaster( gentity_t *ent ) {
	ent->target_ent = G_Find( NULL, FOFS( targetname ), ent->target );
	if ( ent->target_ent ) {
		ent->s.otherEntityNum = ent->target_ent->s.number;
	} else {
		G_Printf( "Couldn't find target(%s) for misc_vis_dummy at %s\n", ent->target, vtos( ent->r.currentOrigin ) );
		G_FreeEntity( ent );
	}
}

/*QUAKED misc_vis_dummy (1 .5 0) (-8 -8 -8) (8 8 8)
If this entity is "visible" (in player's PVS) then it's target is forced to be active whether it is in the player's PVS or not.
This entity itself is never visible or transmitted to clients.
For safety, you should have each dummy only point at one entity (however, it's okay to have many dummies pointing at one entity)
*/
void SP_misc_vis_dummy( gentity_t *ent ) {

	if ( !ent->target ) { //----(SA)	added safety check
		G_Printf( "No target specified for misc_vis_dummy at %s\n", vtos( ent->r.currentOrigin ) );
		G_FreeEntity( ent );
		return;
	}

	ent->r.svFlags |= SVF_VISDUMMY;
	G_SetOrigin( ent, ent->s.origin );
	trap_LinkEntity( ent );

	ent->think = locateMaster;
	ent->nextthink = level.time + 1000;

}

//----(SA) end

/*QUAKED misc_vis_dummy_multiple (1 .5 0) (-8 -8 -8) (8 8 8)
If this entity is "visible" (in player's PVS) then it's target is forced to be active whether it is in the player's PVS or not.
This entity itself is never visible or transmitted to clients.
This entity was created to have multiple speakers targeting it
*/
void SP_misc_vis_dummy_multiple( gentity_t *ent ) {
	if ( !ent->targetname ) {
		G_Printf( "misc_vis_dummy_multiple needs a targetname at %s\n", vtos( ent->r.currentOrigin ) );
		G_FreeEntity( ent );
		return;
	}

	ent->r.svFlags |= SVF_VISDUMMY_MULTIPLE;
	G_SetOrigin( ent, ent->s.origin );
	trap_LinkEntity( ent );

}


//===========================================================

//----(SA)
/*QUAKED misc_light_surface (1 .5 0) (-8 -8 -8) (8 8 8)
The surfaces nearest these entities will be the only surfaces lit by the targeting light
This must be within 64 world units of the surface to be lit!
*/
void SP_misc_light_surface( gentity_t *ent ) {
	G_FreeEntity( ent );
}

//----(SA) end

//===========================================================

void locateCamera( gentity_t *ent ) {
	vec3_t dir;
	gentity_t   *target;
	gentity_t   *owner;

	owner = G_PickTarget( ent->target );
	if ( !owner ) {
		G_Printf( "Couldn't find target for misc_partal_surface\n" );
		G_FreeEntity( ent );
		return;
	}
	ent->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if ( owner->spawnflags & 1 ) {
		ent->s.frame = 25;
	} else if ( owner->spawnflags & 2 ) {
		ent->s.frame = 75;
	}

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	VectorCopy( owner->s.origin, ent->s.origin2 );

	// see if the portal_camera has a target
	target = G_PickTarget( owner->target );
	if ( target ) {
		VectorSubtract( target->s.origin, owner->s.origin, dir );
		VectorNormalize( dir );
	} else {
		G_SetMovedir( owner->s.angles, dir );
	}

	ent->s.eventParm = DirToByte( dir );
}

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!
*/
void SP_misc_portal_surface( gentity_t *ent ) {
	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity( ent );

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;

	if ( !ent->target ) {
		VectorCopy( ent->s.origin, ent->s.origin2 );
	} else {
		ent->think = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate
The target for a misc_portal_director.  You can set either angles or target another entity to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void SP_misc_portal_camera( gentity_t *ent ) {
	float roll;

	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity( ent );

	G_SpawnFloat( "roll", "0", &roll );

	ent->s.clientNum = roll / 360.0 * 256;
}

/*
======================================================================

  SHOOTERS

======================================================================
*/

void Use_Shooter( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	vec3_t dir;
	float deg;
	vec3_t up, right;

	// see if we have a target
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->r.currentOrigin, ent->s.origin, dir );
		if ( ent->s.weapon != WP_SNIPER ) {
			VectorNormalize( dir );
		}
	} else {
		VectorCopy( ent->movedir, dir );
	}

	if ( ent->s.weapon == WP_MORTAR ) {
		AimAtTarget( ent );   // store in ent->s.origin2 the direction/force needed to pass through the target
		VectorCopy( ent->s.origin2, dir );
	}

	if ( ent->s.weapon != WP_SNIPER ) {
		// randomize a bit
		PerpendicularVector( up, dir );
		CrossProduct( up, dir, right );

		deg = crandom() * ent->random;
		VectorMA( dir, deg, up, dir );

		deg = crandom() * ent->random;
		VectorMA( dir, deg, right, dir );

		VectorNormalize( dir );
	}

	switch ( ent->s.weapon ) {
	case WP_GRENADE_LAUNCHER:
		VectorScale( dir, 700, dir );                 //----(SA)	had to add this as fire_grenade now expects a non-normalized direction vector
		fire_grenade( ent, ent->s.origin, dir, WP_GRENADE_LAUNCHER );
		break;
	case WP_PANZERFAUST:
	case WP_ROCKET_LAUNCHER:
		fire_rocket( ent, ent->s.origin, dir );
		break;

	case WP_SPEARGUN:
	case WP_SPEARGUN_CO2:
		fire_speargun( ent, ent->s.origin, dir );
		break;

		// Rafael sniper
	case WP_SNIPER:
		fire_lead( ent, ent->s.origin, dir, ent->damage );
		break;
		// done

	case WP_MORTAR:
		AimAtTarget( ent );   // store in ent->s.origin2 the direction/force needed to pass through the target
		VectorScale( dir, VectorLength( ent->s.origin2 ), dir );
		fire_mortar( ent, ent->s.origin, dir );
		break;

	}

	G_AddEvent( ent, EV_FIRE_WEAPON, 0 );
}

static void InitShooter_Finish( gentity_t *ent ) {
	ent->enemy = G_PickTarget( ent->target );
	ent->think = 0;
	ent->nextthink = 0;
}

void InitShooter( gentity_t *ent, int weapon ) {
	ent->use = Use_Shooter;
	ent->s.weapon = weapon;

	// Rafael sniper
	if ( weapon != WP_SNIPER ) {
		RegisterItem( BG_FindItemForWeapon( weapon ) );
	}
	// done

	G_SetMovedir( ent->s.angles, ent->movedir );

	if ( !ent->random ) {
		ent->random = 1.0;
	}

	if ( ent->s.weapon != WP_SNIPER ) {
		ent->random = sin( M_PI * ent->random / 180 );
	}

	// target might be a moving object, so we can't set movedir for it
	if ( ent->target ) {
		ent->think = InitShooter_Finish;
		ent->nextthink = level.time + 500;
	}
	trap_LinkEntity( ent );
}

/*QUAKED shooter_mortar (1 0 0) (-16 -16 -16) (16 16 16) SMOKE_FX FLASH_FX
Lobs a mortar so that it will pass through the info_notnull targeted by this entity
"random" the number of degrees of deviance from the taget. (1.0 default)
if LAUNCH_FX is checked a smoke effect will play at the origin of this entity.
if FLASH_FX is checked a muzzle flash effect will play at the origin of this entity.
*/
void SP_shooter_mortar( gentity_t *ent ) {
	// (SA) TODO: must have a self->target.  Do a check/print if this is not the case.
	InitShooter( ent, WP_MORTAR );

	if ( ent->spawnflags & 1 ) {   // smoke at source
	}
	if ( ent->spawnflags & 2 ) {   // muzzle flash at source
	}
}

/*QUAKED shooter_rocket (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_rocket( gentity_t *ent ) {
	InitShooter( ent, WP_ROCKET_LAUNCHER );
}

/*QUAKED shooter_zombiespit (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_zombiespit( gentity_t *ent ) {
	InitShooter( ent, WP_MONSTER_ATTACK1 );
}


/*
==============
use_shooter_tesla
==============
*/
void use_shooter_tesla( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t   *tent;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	} else
	{
		tent = G_PickTarget( ent->target );
		VectorCopy( tent->s.origin, ent->s.origin2 );

		ent->active = 0;
		trap_LinkEntity( ent );
	}
}

//----(SA)	added
/*QUAKED shooter_tesla (1 0 0) (-16 -16 -16) (16 16 16) START_ON DLIGHT
START_ON means it starts out active, the default is to start off and fire when triggered
DLIGHT will have a built-in dlight flashing too (use color picker to set color of dlight)

"sticktime" - how long each bolt should 'stick' to an impact point (def: .5)
"random" - how far away to drift from the target. (def: 0.0)
"width" - width of the bolts (def: 20)
"count" - number of bolts to fire per impact point.  (def: 2)
*/

void shooter_tesla_finish_spawning( gentity_t *ent ) {
	gentity_t   *tent;  // target ent

	ent->think = 0;
	ent->nextthink = 0;

	// locate the target and set the location
	tent = G_PickTarget( ent->target );
	if ( !tent ) { // if there's a problem with tent
		G_Printf( "shooter_tesla (%s) at %s has no target.\n", ent->target, vtos( ent->s.origin ) );
		return;
	}

	VectorCopy( tent->s.origin, ent->s.origin2 );

	if ( ent->spawnflags & 1 ) {   // START_ON
		ent->active = 0;
		trap_LinkEntity( ent );
	}
}

void SP_shooter_tesla( gentity_t *ent ) {

	float tempf;

	//	it's a shooter_ since it will act like any other shooter, except
	//	the tesla is a client-side effect that reports damage back to the
	//	game, so this will create an linked-entity on the client that acts as dictated
	//	and, if necessary, passes back damage info like the weapon.  this will
	//	keep the server->client messages to a minimum (start firing/stop firing)
	//	rather than sending an event for each bolt.
	ent->s.eType        = ET_EF_TESLA;
	ent->use            = use_shooter_tesla;

	// set number of bolts
	if ( ent->count ) {
		ent->s.density = ent->count;
	} else {
		ent->s.density = 2;
	}


	// width
	if ( G_SpawnFloat( "width", "", &tempf ) ) {
		ent->s.frame = (int)tempf;
	} else {
		ent->s.frame = 20;
	}


	// 'sticky' time (stored in .weapon)
	if ( G_SpawnFloat( "sticktime", "", &tempf ) ) {
		ent->s.time2 = (int)( tempf * 1000.0f );
	} else {
		ent->s.time2 = 500; // default to 1/2 sec

	}
	// randomness
	ent->s.angles2[0] = ent->random;


	// DLIGHT
	if ( ent->spawnflags & 2 ) {
		int dlightsize;
		if ( G_SpawnInt( "dlightsize", "", &dlightsize ) ) {
			ent->s.time = dlightsize;
		} else {
			ent->s.time = 500;
		}

		if ( ent->random ) {
			ent->s.time2 = ent->random;
		} else {
			ent->s.time2 = 4; // dlight randomness

		}
		if ( ent->dl_color[0] <= 0 &&    // if it's black or has no color assigned
			 ent->dl_color[1] <= 0 &&
			 ent->dl_color[2] <= 0 ) {
			// default is the same color as the tesla weapon
			ent->dl_color[0] = 0.2f;
			ent->dl_color[1] = 0.6f;
			ent->dl_color[2] = 1.0f;
		}

		ent->dl_color[0] = ent->dl_color[0] * 255;
		ent->dl_color[1] = ent->dl_color[1] * 255;
		ent->dl_color[2] = ent->dl_color[2] * 255;

		ent->s.dl_intensity = (int)ent->dl_color[0] | ( (int)ent->dl_color[1] << 8 ) | ( (int)ent->dl_color[2] << 16 );

	} else {
		ent->s.dl_intensity = 0;
	}


	// finish up after everything has spawned in so we know all potential targets are ready
	ent->think = shooter_tesla_finish_spawning;
	ent->nextthink = level.time + 100;
}
//----(SA)	end


/*QUAKED shooter_grenade (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
*/
void SP_shooter_grenade( gentity_t *ent ) {
	InitShooter( ent, WP_GRENADE_LAUNCHER );
}

// Rafael sniper
/*QUAKED shooter_sniper (1 0 0) (-16 -16 -16) (16 16 16)
Fires at either the target or the current direction.
"random" is the number of degrees of deviance from the taget. (1.0 default)
"damage" the amount of damage sniper will cause when he hits his target default is 10
"radius" is the dist the target would need to travel before sniper lost his beat default 256
"delay"	 is the rate of fire defaults to 1 sec
*/
void SP_shooter_sniper( gentity_t *ent ) {

	char        *damage;

	if ( G_SpawnString( "damage", "0", &damage ) ) {
		ent->damage = atoi( damage );
	}

	if ( !ent->damage ) {
		ent->damage = 10;
	}
	if ( !ent->radius ) { // radius
		ent->radius = 256;
	}
	if ( !ent->delay ) {
		ent->delay = 1.0; // one sec

	}
	InitShooter( ent, WP_SNIPER );

	ent->delay *= 1000;

	ent->wait = level.time + ent->delay;
}

void brush_activate_sniper( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	gentity_t *sniper;
	float dist;
	vec3_t vec;
	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( player && player != other ) {
		// G_Printf ("other: %s\n", other->aiName);
		return;
	}

	if ( other->client ) {
		ent->enemy = other;
	}

	sniper = G_Find( NULL, FOFS( targetname ), ent->target );

	if ( !sniper ) {
		G_Printf( "sniper not found: %s\n" );
	} else
	{
		if ( visible( sniper, other ) ) {
			if ( sniper->wait < level.time ) {
				if ( sniper->count == 0 ) {
					sniper->count = 1;
					sniper->wait = level.time + sniper->delay;
					// record enemypos pos
					VectorCopy( ent->enemy->r.currentOrigin, ent->pos1 );
				} else if ( sniper->count == 1 )     {
					VectorSubtract( ent->enemy->r.currentOrigin, ent->pos1, vec );
					dist = VectorLength( vec );
					if ( dist < sniper->radius ) {
						// ok the enemy is still inside the radius take a shot
						sniper->enemy = other;
						sniper->use( sniper, other, other );
						G_UseTargets( ent, other );

						// added sniper shot

						G_AddEvent( player, EV_GENERAL_SOUND, sniper_sound );

					}

					// reset the sniper delay
					sniper->count = 0;
					sniper->wait = level.time + sniper->delay;
				}
			}
		} else
		{
			//sniper->wait = level.time + sniper->delay;
			sniper->count = 0;
		}
	}

}

void sniper_brush_init( gentity_t *ent ) {
	vec3_t center;

	if ( !ent->target ) {
		VectorSubtract( ent->r.maxs, ent->r.mins, center );
		VectorScale( center, 0.5, center );

		G_Printf( "sniper_brush at %s without a target\n", vtos( center ) );
	}
}

extern void InitTrigger( gentity_t *self );

/*QUAKED sniper_brush (1 0 0) ?
this should be a volume that will encompase the area where the sniper target assigned to the
brush would fire at the player
*/
void SP_sniper_brush( gentity_t *ent ) {
	ent->nextthink = level.time + FRAMETIME;
	ent->think = sniper_brush_init;
	ent->touch = brush_activate_sniper;

	sniper_sound = G_SoundIndex( "sound/weapons/machinegun/machgf1b.wav" );

	InitTrigger( ent );
	trap_LinkEntity( ent );
}



/*QUAKED corona (0 1 0) (-4 -4 -4) (4 4 4) START_OFF
Use color picker to set color or key "color".  values are 0.0-1.0 for each color (rgb).
"scale" will designate a multiplier to the default size.  (so 2.0 is 2xdefault size, 0.5 is half)
*/

/*
==============
use_corona
	so level designers can toggle them on/off
==============
*/
void use_corona( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	} else
	{
		ent->active = 0;
		trap_LinkEntity( ent );
	}
}


/*
==============
SP_corona
==============
*/
void SP_corona( gentity_t *ent ) {
	float scale;

	ent->s.eType        = ET_CORONA;

	if ( ent->dl_color[0] <= 0 &&                // if it's black or has no color assigned
		 ent->dl_color[1] <= 0 &&
		 ent->dl_color[2] <= 0 ) {
		ent->dl_color[0] = ent->dl_color[1] = ent->dl_color[2] = 1; // set white

	}
	ent->dl_color[0] = ent->dl_color[0] * 255;
	ent->dl_color[1] = ent->dl_color[1] * 255;
	ent->dl_color[2] = ent->dl_color[2] * 255;

	ent->s.dl_intensity = (int)ent->dl_color[0] | ( (int)ent->dl_color[1] << 8 ) | ( (int)ent->dl_color[2] << 16 );

	G_SpawnFloat( "scale", "1", &scale );
	ent->s.density = (int)( scale * 255 );

	ent->use = use_corona;

	if ( !( ent->spawnflags & 1 ) ) {
		trap_LinkEntity( ent );
	}
}


// (SA) dlights and dlightstyles
// TTimo gcc: lots of braces around scalar initializer
// char* predef_lightstyles[] = {
//	{"mmnmmommommnonmmonqnmmo"},

char* predef_lightstyles[] = {
	"mmnmmommommnonmmonqnmmo",
	"abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba",
	"mmmmmaaaaammmmmaaaaaabcdefgabcdefg",
	"ma",
	"jklmnopqrstuvwxyzyxwvutsrqponmlkj",
	"nmonqnmomnmomomono",
	"mmmaaaabcdefgmmmmaaaammmaamm",
	"aaaaaaaazzzzzzzz",
	"mmamammmmammamamaaamammma",
	"abcdefghijklmnopqrrqponmlkjihgfedcba",
	"mmnommomhkmmomnonmmonqnmmo",
	"kmamaamakmmmaakmamakmakmmmma",
	"kmmmakakmmaaamammamkmamakmmmma",
	"mmnnoonnmmmmmmmmmnmmmmnonmmmmmmm",
	"mmmmnonmmmmnmmmmmnonmmmmmnmmmmmmm",
	"zzzzzzzzaaaaaaaa",
	"zzzzzzzzaaaaaaaaaaaaaaaa",
	"aaaaaaaazzzzzzzzaaaaaaaa",
	"aaaaaaaaaaaaaaaazzzzzzzz"
};


/*
==============
dlight_finish_spawning
	All the dlights should call this on the same frame, thereby
	being synched, starting	their sequences all at the same time.
==============
*/
void dlight_finish_spawning( gentity_t *ent ) {
	G_FindConfigstringIndex( va( "%i %s %i %i %i", ent->s.number, ent->dl_stylestring, ent->health, ent->soundLoop, ent->dl_atten ), CS_DLIGHTS, MAX_DLIGHT_CONFIGSTRINGS, qtrue );
}

static int dlightstarttime = 0;


/*QUAKED dlight (0 1 0) (-12 -12 -12) (12 12 12) FORCEACTIVE STARTOFF ONETIME
"style": value is an int from 1-19 that contains a pre-defined 'flicker' string.
"stylestring": set your own 'flicker' string.  (ex. "klmnmlk"). NOTE: this should be all lowercase
Stylestring characters run at 10 cps in the game. (meaning the alphabet, at 24 characters, would take 2.4 seconds to cycle)
"offset": change the initial index in a style string.  So val of 3 in the above example would start this light at 'N'.  (used to get dlights using the same style out of sync).
"atten": offset from the alpha values of the stylestring.  stylestring of "ddeeffzz" with an atten of -1 would result in "ccddeeyy"
Use color picker to set color or key "color".  values are 0.0-1.0 for each color (rgb).
FORCEACTIVE	- toggle makes sure this light stays alive in a map even if the user has r_dynamiclight set to 0.
STARTOFF	- means the dlight doesn't spawn in until ent is triggered
ONETIME		- when the dlight is triggered, it will play through it's cycle once, then shut down until triggered again
"shader" name of shader to apply
"sound" sound to loop every cycle (this actually just plays the sound at the beginning of each cycle)

styles:
1 - "mmnmmommommnonmmonqnmmo"
2 - "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba"
3 - "mmmmmaaaaammmmmaaaaaabcdefgabcdefg"
4 - "ma"
5 - "jklmnopqrstuvwxyzyxwvutsrqponmlkj"
6 - "nmonqnmomnmomomono"
7 - "mmmaaaabcdefgmmmmaaaammmaamm"
8 - "aaaaaaaazzzzzzzz"
9 - "mmamammmmammamamaaamammma"
10 - "abcdefghijklmnopqrrqponmlkjihgfedcba"
11 - "mmnommomhkmmomnonmmonqnmmo"
12 - "kmamaamakmmmaakmamakmakmmmma"
13 - "kmmmakakmmaaamammamkmamakmmmma"
14 - "mmnnoonnmmmmmmmmmnmmmmnonmmmmmmm"
15 - "mmmmnonmmmmnmmmmmnonmmmmmnmmmmmmm"
16 - "zzzzzzzzaaaaaaaa"
17 - "zzzzzzzzaaaaaaaaaaaaaaaa"
18 - "aaaaaaaazzzzzzzzaaaaaaaa"
19 - "aaaaaaaaaaaaaaaazzzzzzzz"
*/


/*
==============
shutoff_dlight
	the dlight knew when it was triggered to unlink after going through it's cycle once
==============
*/
void shutoff_dlight( gentity_t *ent ) {
	if ( !( ent->r.linked ) ) {
		return;
	}

	trap_UnlinkEntity( ent );
	ent->think = 0;
	ent->nextthink = 0;
}


/*
==============
use_dlight
==============
*/
void use_dlight( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	} else
	{
		ent->active = 0;
		trap_LinkEntity( ent );

		if ( ent->spawnflags & 4 ) {   // ONETIME
			ent->think = shutoff_dlight;
			ent->nextthink = level.time + (  strlen( ent->dl_stylestring )  * 100 ) - 100;
		}
	}
}



/*
==============
SP_dlight
	ent->dl_stylestring contains the lightstyle string
	ent->health tracks current index into style string
	ent->count tracks length of style string
==============
*/
void SP_dlight( gentity_t *ent ) {
	char    *snd, *shader;
	int i;
	int offset, style, atten;

	G_SpawnInt( "offset", "0", &offset );              // starting index into the stylestring
	G_SpawnInt( "style", "0", &style );                    // predefined stylestring
	G_SpawnString( "sound", "", &snd );                 //
	G_SpawnInt( "atten", "0", &atten );                    //
	G_SpawnString( "shader", "", &shader );             // name of shader to use for this dlight image

	if ( G_SpawnString( "sound", "0", &snd ) ) {
		ent->soundLoop = G_SoundIndex( snd );
	}

	if ( ent->dl_stylestring && strlen( ent->dl_stylestring ) ) {    // if they're specified in a string, use em
	} else if ( style )       {
		style = max( 1, style );                                  // clamp to predefined range
		style = min( 19, style );
		ent->dl_stylestring = predef_lightstyles[style - 1];    // these are input as 1-20
	} else {
		ent->dl_stylestring = "mmmaaa";                          // default to a strobe to call attention to this not being set
	}

	ent->count      = strlen( ent->dl_stylestring );

	ent->dl_atten = atten;

	// make the initial offset a valid index into the stylestring
	offset = offset % ( ent->count );

	ent->health     = offset;                   // set the offset into the string

	ent->think      = dlight_finish_spawning;
	if ( !dlightstarttime ) {                      // sync up all the dlights
		dlightstarttime = level.time + 100;
	}
	ent->nextthink  = dlightstarttime;

	if ( ent->dl_color[0] <= 0 &&                // if it's black or has no color assigned, make it white
		 ent->dl_color[1] <= 0 &&
		 ent->dl_color[2] <= 0 ) {
		ent->dl_color[0] = ent->dl_color[1] = ent->dl_color[2] = 1;
	}

	ent->dl_color[0] = ent->dl_color[0] * 255;  // range 0-255 now so the client doesn't have to on every update
	ent->dl_color[1] = ent->dl_color[1] * 255;
	ent->dl_color[2] = ent->dl_color[2] * 255;

	i = (int)( ent->dl_stylestring[offset] ) - (int)'a';
	i = i * ( 1000.0f / 24.0f );

	ent->s.constantLight =  (int)ent->dl_color[0] | ( (int)ent->dl_color[1] << 8 ) | ( (int)ent->dl_color[2] << 16 ) | ( i / 4 << 24 );

	ent->use = use_dlight;

	if ( !( ent->spawnflags & 2 ) ) {
		trap_LinkEntity( ent );
	}

}
// done (SA)



// Rafael particles
/*QUAKED misc_snow256 (1 0 0) (-256 -256 -16) (256 256 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_snow128 (1 0 0) (-128 -128 -16) (128 128 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_snow64 (1 0 0) (-64 -64 -16) (64 64 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_snow32 (1 0 0) (-32 -32 -16) (32 32 16) TURBULENT
health = density defaults to 32
*/

/*QUAKED misc_bubbles8 (1 0 0) (-8 -8 0) (8 8 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_bubbles16 (1 0 0) (-16 -16 0) (16 16 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_bubbles32 (1 0 0) (-32 -32 0) (32 32 16) TURBULENT
health = density defaults to 32
*/
/*QUAKED misc_bubbles64 (1 0 0) (-64 -64 0) (64 64 64) TURBULENT
health = density defaults to 32
*/


void snowInPVS( gentity_t *ent ) {
	gentity_t *tent;
	gentity_t *player;
	qboolean inPVS = qfalse;
	qboolean oldactive;

	oldactive = ent->active;

	ent->nextthink = level.time + FRAMETIME;

	player = AICast_FindEntityForName( "player" );

	if ( player ) {
		inPVS = trap_InPVS( player->r.currentOrigin, ent->r.currentOrigin );

		if ( inPVS ) {
			ent->active = qtrue;
		} else {
			ent->active = qfalse;
		}
	} else {
		return;
	}

	// there hasn't been a change so bail
	if ( oldactive == ent->active ) {
		return;
	}

	if ( ent->active ) {
		tent = G_TempEntity( player->r.currentOrigin, EV_SNOW_ON );
// G_Printf( "on\n");
	} else
	{
		tent = G_TempEntity( player->r.currentOrigin, EV_SNOW_OFF );
// G_Printf( "off\n");
	}

	tent->s.frame = ent->s.number;
	trap_LinkEntity( ent );
}

void snow_think( gentity_t *ent ) {
	trace_t tr;
	vec3_t dest;
	int turb;

	VectorCopy( ent->s.origin, dest );

	if ( ent->spawnflags & 2 ) { // bubble
		dest[2] += 8192;
	} else {
		dest[2] -= 8192;
	}

	trap_Trace( &tr, ent->s.origin, NULL, NULL, dest, ent->s.number, MASK_SHOT );

	if ( ent->spawnflags & 1 ) {
		turb = 1;
	} else {
		turb = 0;
	}

	if ( !Q_stricmp( ent->classname, "misc_snow256" ) ) {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_SNOW256, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, qtrue );
	} else if ( !Q_stricmp( ent->classname, "misc_snow128" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_SNOW128, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, qtrue );
	} else if ( !Q_stricmp( ent->classname, "misc_snow64" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_SNOW64, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, qtrue );
	} else if ( !Q_stricmp( ent->classname, "misc_snow32" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_SNOW32, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, qtrue );
	} else if ( !Q_stricmp( ent->classname, "misc_bubbles8" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_BUBBLE8, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, qtrue );
	} else if ( !Q_stricmp( ent->classname, "misc_bubbles16" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_BUBBLE16, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, qtrue );
	} else if ( !Q_stricmp( ent->classname, "misc_bubbles32" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_BUBBLE32, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, qtrue );
	} else if ( !Q_stricmp( ent->classname, "misc_bubbles64" ) )       {
		G_FindConfigstringIndex( va( "%i %.2f %.2f %.2f %.2f %.2f %.2f %i %i %i", PARTICLE_BUBBLE64, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2], ent->health, turb, ent->s.number ), CS_PARTICLES, MAX_PARTICLES_AREAS, qtrue );
	}

	ent->think = snowInPVS;
	ent->nextthink = level.time + FRAMETIME;

}

void SP_Snow( gentity_t *ent ) {
	ent->think = snow_think;
	ent->nextthink = level.time + FRAMETIME;

	G_SetOrigin( ent, ent->s.origin );

	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_GENERAL;

	trap_LinkEntity( ent );

	if ( !ent->health ) {
		ent->health = 32;
	}

	ent->active = qtrue;
}
// done.


void SP_Bubbles( gentity_t *ent ) {
	ent->think = snow_think;
	ent->nextthink = level.time + FRAMETIME;

	G_SetOrigin( ent, ent->s.origin );

	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_GENERAL;

	trap_LinkEntity( ent );

	if ( !ent->health ) {
		ent->health = 32;
	}

	ent->active = qtrue;

	ent->spawnflags |= 2;
}

static vec3_t forward, right, up;
static vec3_t muzzle;

void flakPuff( vec3_t origin, qboolean sky ) {
	gentity_t *tent;
	vec3_t point;

	VectorCopy( origin, point );
	if ( sky ) {
		VectorMA( point, -256, forward, point );
	}

	point[2] += 16; // raise puff off the ground some
	tent = G_TempEntity( point, EV_SMOKE );
	VectorCopy( point, tent->s.origin );
	tent->s.time = 2000;
	tent->s.time2 = 1000;
	tent->s.density = 0;
	tent->s.angles2[0] = 16 + 8;
	tent->s.angles2[1] = 48 + 24;
	tent->s.angles2[2] = 10;
}

/*
==============
Fire_Lead
==============
*/
//----(SA)	added 'activator' so the bits that used to expect 'ent' to be the gun still work
void Fire_Lead( gentity_t *ent, gentity_t *activator, float spread, int damage ) {
	trace_t tr;
	vec3_t end;
	float r;
	float u;
	gentity_t       *tent;
	gentity_t       *traceEnt;
	//qboolean	isflak = qfalse;
	int seed = rand() & 255;

	r = Q_crandom( &seed ) * spread;
	u = Q_crandom( &seed ) * spread;

	ent->s.eFlags |= EF_MG42_ACTIVE;
	activator->s.eFlags |= EF_MG42_ACTIVE;

	VectorMA( muzzle, 8192, forward, end );
	VectorMA( end, r, right, end );
	VectorMA( end, u, up, end );

	G_HistoricalTrace( ent, &tr, muzzle, NULL, NULL, end, ent->s.number, MASK_SHOT );

	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		AICast_ProcessBullet( activator, muzzle, tr.endpos );
	}

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
//		mg42_muzzleflash (ent);
//		G_AddEvent( ent, EV_FIRE_WEAPON_MG42, 0 );

// JPW NERVE added this event so tracers work if you're shooting mg42 into skybox, otherwise shouldn't ever see the event double-up
		tent = G_TempEntity( tr.endpos, EV_MG42BULLET_HIT_WALL );
		tent->s.otherEntityNum = ent->s.number;
		tent->s.otherEntityNum2 = activator->s.number;
		tent->s.effect1Time = seed;
// jpw

		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards( tr.endpos, muzzle );

	// send bullet impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_MG42BULLET_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
		tent->s.otherEntityNum = ent->s.number;
		tent->s.otherEntityNum2 = activator->s.number;
		tent->s.effect1Time = seed;

		if ( LogAccuracyHit( traceEnt, ent ) ) {
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}

	} else {
		// Ridah, bullet impact should reflect off surface
		vec3_t reflect;
		float dot;

		tent = G_TempEntity( tr.endpos, EV_MG42BULLET_HIT_WALL );

		dot = DotProduct( forward, tr.plane.normal );
		VectorMA( forward, -2 * dot, tr.plane.normal, reflect );
		VectorNormalize( reflect );

		tent->s.eventParm = DirToByte( reflect );
		tent->s.otherEntityNum = ent->s.number;
		tent->s.otherEntityNum2 = activator->s.number;  // (SA) store the user id, so the client can position the tracer
		tent->s.effect1Time = seed;
	}

	if ( traceEnt->takedamage ) {
		G_Damage( traceEnt, ent, activator, forward, tr.endpos,
				  damage, 0, MOD_MACHINEGUN );
	}

//	mg42_muzzleflash (ent);
//	G_AddEvent( ent, EV_FIRE_WEAPON_MG42, 0 );
}


float AngleDifference( float ang1, float ang2 );
// NOTE: use this value, and THEN the cl_input.c scales to tweak the feel
#define MG42_IDLEYAWSPEED   80.0    // degrees per second (while returning to base)

void clamp_hweapontofirearc( gentity_t *self, vec3_t dang ) {
	float diff, yawspeed;
	qboolean clamped;

	clamped = qfalse;

	// go back to start position
	VectorCopy( self->s.angles, dang );
	yawspeed = MG42_IDLEYAWSPEED;

	if ( dang[0] < 0 && dang[0] < -( self->varc ) ) {
		clamped = qtrue;
		dang[0] = -( self->varc );
	}

	if ( dang[0] > 0 && dang[0] > ( self->varc / 2 ) ) {
		clamped = qtrue;
		dang[0] = self->varc / 2;
	}

	// sanity check the angles again to make sure we don't go passed the harc
	diff = AngleDifference( self->s.angles[YAW], dang[YAW] );
	if ( fabs( diff ) > self->harc ) {
		clamped = qtrue;

		if ( diff > 0 ) {
			dang[YAW] = AngleMod( self->s.angles[YAW] - self->harc );
		} else {
			dang[YAW] = AngleMod( self->s.angles[YAW] + self->harc );
		}
	}

//	if (g_mg42arc.integer)
//		G_Printf ("varc = %5.2f\n", dang[0]);
}

// NOTE: this only effects the external view of the user, when using the mg42, the
// view position is set on the client-side to keep it firm behind the gun with
// interpolation
void clamp_playerbehindgun( gentity_t *self, gentity_t *other, vec3_t dang ) {
	vec3_t forward, right, up;
	vec3_t point;


	AngleVectors( self->s.apos.trBase, forward, right, up );
	VectorMA( self->r.currentOrigin, -36, forward, point );

	point[2] = other->r.currentOrigin[2];
	trap_UnlinkEntity( other );
	SnapVector( point );
	VectorCopy( point, other->client->ps.origin );

	// save results of pmove
	BG_PlayerStateToEntityState( &other->client->ps, &other->s, qfalse );

	// use the precise origin for linking
	VectorCopy( other->client->ps.origin, other->r.currentOrigin );

	// DHM - Nerve :: Zero out velocity
	other->client->ps.velocity[0] = other->client->ps.velocity[1] = 0.f;
	other->s.pos.trDelta[0] = other->s.pos.trDelta[1] = 0.f;

	trap_LinkEntity( other );
}

#define MG42_DAMAGE_MP      20          // DHM - Nerve :: testing with slowed firing rate // JPW NERVE was 30

void mg42_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	vec3_t dang;
	int i;

	if ( !self->active ) {
		return;
	}

	if ( other->active ) {
		for ( i = 0; i < 3; i++ )
			dang[i] = SHORT2ANGLE( other->client->pers.cmd.angles[i] );

		// now tell the client to lock the view in the direction of the gun
		other->client->ps.viewlocked = 3;
		other->client->ps.viewlocked_entNum = self->s.number;

		if ( self->s.frame ) {
			other->client->ps.gunfx = 1;
		} else {
			other->client->ps.gunfx = 0;
		}

		// clamp player behind the gun
		clamp_playerbehindgun( self, other, dang );
	}
}

void mg42_fire( gentity_t *other ) {
	gentity_t *self;

	self = &g_entities[other->client->ps.viewlocked_entNum];

	//AngleVectors (self->s.apos.trBase, forward, right, up);
	AngleVectors( other->client->ps.viewangles, forward, right, up );
	VectorCopy( self->s.pos.trBase, muzzle );

	// VectorMA (muzzle, 16, forward, muzzle); // JPW NERVE unnecessary and makes it so close-range enemies get missed
	VectorMA( muzzle, 16, up, muzzle );

	self->s.eFlags |= EF_MG42_ACTIVE;
	other->s.eFlags |= EF_MG42_ACTIVE;

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzle );

	Fire_Lead( self, other, MG42_SPREAD_MP, MG42_DAMAGE_MP );
}

void mg42_track( gentity_t *self, gentity_t *other ) {
	int i;

	if ( !self->active ) {
		return;
	}

	if ( other->active ) {
		// move to the position over the next frame
		VectorSubtract( other->client->ps.viewangles, self->s.apos.trBase, self->s.apos.trDelta );
		for ( i = 0; i < 3; i++ ) {
			self->s.apos.trDelta[i] = AngleNormalize180( self->s.apos.trDelta[i] );
		}
		VectorScale( self->s.apos.trDelta, 1000 / 50, self->s.apos.trDelta );
		self->s.apos.trTime = level.time;
		self->s.apos.trDuration = 50;

		SnapVector( self->s.apos.trDelta );
	}
}

#define USEMG42_DISTANCE 46
void mg42_think( gentity_t *self ) {
	vec3_t vec;
	gentity_t   *owner;
	int i;
	float len;
	float usedist;

	if ( level.intermissiontime ) {
		return;
	}

	VectorClear( vec );

	owner = &g_entities[self->r.ownerNum];

	// move to the current angles
	if ( self->timestamp > level.time ) {
		BG_EvaluateTrajectory( &self->s.apos, level.time, self->s.apos.trBase );
	}

	if ( owner->client ) {
		VectorSubtract( self->r.currentOrigin, owner->r.currentOrigin, vec );
		len = VectorLength( vec );

		usedist = 96;

		if ( len < usedist && owner->active && owner->health > 0 ) {
			// ATVI Wolfenstein Misc #433
			owner->client->ps.pm_flags &= ~PMF_DUCKED;

			self->active = qtrue;
			owner->client->ps.persistant[PERS_HWEAPON_USE] = 1;
			mg42_track( self, owner );
			self->nextthink = level.time + 50;
			self->timestamp = level.time + 1000;

			if ( !( owner->r.svFlags & SVF_CASTAI ) ) {
				clamp_playerbehindgun( self, owner, vec3_origin );
			}
			return;
		}
	}

	self->active = qfalse;

	if ( owner->client ) {
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		owner->client->ps.viewlocked = 0;   // let them look around
		owner->active = qfalse;
		owner->client->ps.gunfx = 0;
	}

	self->r.ownerNum = self->s.number;
	self->s.otherEntityNum = self->s.number;

	if ( self->timestamp > level.time ) {
		// slowly rotate back to position
		clamp_hweapontofirearc( self, vec );
		// move to the position over the next frame
		VectorSubtract( vec, self->s.apos.trBase, self->s.apos.trDelta );
		for ( i = 0; i < 3; i++ ) {
			self->s.apos.trDelta[i] = AngleNormalize180( self->s.apos.trDelta[i] );
		}
		VectorScale( self->s.apos.trDelta, 1000 / 50, self->s.apos.trDelta );
		self->s.apos.trTime = level.time;
		self->s.apos.trDuration = 50;
	}
	self->nextthink = level.time + 50;

	SnapVector( self->s.apos.trDelta );
}

void mg42_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	gentity_t   *gun;
	gentity_t   *owner;
	trace_t tr;

	// DHM - Nerve :: self->chain not set if no tripod
	if ( self->chain ) {
		gun = self->chain;
	} else {
		gun = self;
	}
	// dhm - end

	owner = &g_entities[gun->r.ownerNum];

	if ( gun && self->health <= 0 ) {
		gun->s.frame = 2;
		gun->takedamage = qfalse;

		// DHM - Nerve :: health is used in repairing later
		if ( g_gametype.integer >= GT_WOLF ) {
			gun->health = 0;
			gun->s.eFlags = EF_SMOKING;         // Make it smoke on client side
			self->health = 0;
		}
		// dhm - end
	}

	self->takedamage = qfalse;

	if ( owner && owner->client ) {
		// Restore original position if current position is bad
		trap_Trace( &tr, owner->r.currentOrigin, owner->r.mins, owner->r.maxs, owner->r.currentOrigin, owner->s.number, MASK_PLAYERSOLID );
		if ( tr.startsolid ) {
			VectorCopy( owner->TargetAngles, owner->client->ps.origin );
			VectorCopy( owner->TargetAngles, owner->r.currentOrigin );
			owner->r.contents = CONTENTS_CORPSE;        // this will correct itself in ClientEndFrame
		}
		owner->client->ps.eFlags &= ~EF_MG42_ACTIVE;            // DHM - Nerve :: unset flag
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		self->r.ownerNum = self->s.number;
		self->s.otherEntityNum = self->s.number;
		owner->client->ps.viewlocked = 0;   // let them look around
		owner->active = qfalse;
		owner->client->ps.gunfx = 0;

		self->active = qfalse;
		gun->active = qfalse;
	}


	trap_LinkEntity( self );
}

void mg42_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t *owner;

	owner = &g_entities[ent->r.ownerNum];

	if ( owner && owner->client ) {
		owner->client->ps.persistant[PERS_HWEAPON_USE] = 0;
		ent->r.ownerNum = ent->s.number;
		ent->s.otherEntityNum = ent->s.number;
		owner->client->ps.viewlocked = 0;   // let them look around
		owner->active = qfalse;
		owner->client->ps.gunfx = 0;
//		other-> // JPW NERVE FIXME insert other->someUnusedVariable = ent, so we can get proper kill/score info later
	}

	// G_Printf ("mg42 called use function\n");

	trap_LinkEntity( ent );
}

void mg42_spawn( gentity_t *ent ) {
	gentity_t *base, *gun;
	vec3_t offset;

	// Xian -- If in knifeonly mode, prevent MG42's from spawning
	if ( g_knifeonly.integer != 1 ) {
		// Need to spawn the base even when no tripod cause the gun itself isn't solid
		base = G_Spawn();

		if ( !( ent->spawnflags & 2 ) ) { // no tripod
			base->clipmask = CONTENTS_SOLID;
			base->r.contents = CONTENTS_SOLID;
			base->r.svFlags = SVF_USE_CURRENT_ORIGIN;
			base->s.eType = ET_GENERAL;
			base->takedamage = qtrue;
			base->die = mg42_die;

			base->s.modelindex = G_ModelIndex( "models/mapobjects/weapons/mg42b.md3" );
		} else {
			base->takedamage = qfalse;
		}

		VectorSet( base->r.mins, -8, -8, -8 );
		VectorSet( base->r.maxs, 8, 8, 48 );
		VectorCopy( ent->s.origin, offset );
		offset[2] -= 24;
		G_SetOrigin( base, offset );
		base->s.apos.trType = TR_STATIONARY;
		base->s.apos.trTime = 0;
		base->s.apos.trDuration = 0;
		base->s.dmgFlags = HINT_MG42;   // identify this for cursorhints
		VectorCopy( ent->s.angles, base->s.angles );
		VectorCopy( base->s.angles, base->s.apos.trBase );
		VectorCopy( base->s.angles, base->s.apos.trDelta );
		base->health = ent->health;
		base->target = ent->target; //----(SA)	added so mounting mg42 can trigger targets
		trap_LinkEntity( base );

		// Spawn the barrel
		gun = G_Spawn();
		gun->classname = "misc_mg42";
		gun->clipmask = CONTENTS_SOLID;
		gun->r.contents = CONTENTS_TRIGGER;
		gun->r.svFlags = SVF_USE_CURRENT_ORIGIN;
		gun->s.eType = ET_MG42_BARREL;
		gun->health = base->health; // JPW NERVE

		// DHM - Don't need to specify here, handled in G_CheckForCursorHints
		//gun->s.dmgFlags = HINT_MG42;	// identify this for cursorhints

		gun->touch = mg42_touch;

		gun->last_move_time = 0;
		// DHM - Nerve :: Use a different model in multiplayer
		gun->s.modelindex = G_ModelIndex( "models/multiplayer/mg42/mg42.md3" );
		VectorCopy( ent->s.origin, offset );
		offset[2] += 24;
		G_SetOrigin( gun, offset );
		VectorSet( gun->r.mins, -24, -24, -8 );
		VectorSet( gun->r.maxs, 24, 24, 48 );
		gun->s.apos.trTime = 0;
		gun->s.apos.trDuration = 0;
		VectorCopy( ent->s.angles, gun->s.angles );
		VectorCopy( gun->s.angles, gun->s.apos.trBase );
		VectorCopy( gun->s.angles, gun->s.apos.trDelta );

		VectorCopy( ent->s.angles, gun->s.angles2 );

		gun->think = mg42_think;
		gun->nextthink = level.time + FRAMETIME;
		gun->timestamp = level.time + 1000;
		gun->s.number = gun - g_entities;
		gun->harc = ent->harc;
		gun->varc = ent->varc;
		gun->s.origin2[0] = ent->harc;
		gun->s.origin2[1] = ent->varc;  // need these clientsided
		gun->s.apos.trType = TR_LINEAR_STOP;    // interpolate the angles
		gun->takedamage = qtrue;
		gun->targetname = ent->targetname;      // need this for scripting
		gun->damage = ent->damage;
		gun->accuracy = ent->accuracy;
		gun->target = ent->target;  //----(SA)	added so mounting mg42 can trigger targets
		gun->use = mg42_use;
		gun->die = mg42_die; // JPW NERVE we want it to be called for non-tripod machineguns too (for mp_beach etc)

		if ( !( ent->spawnflags & 2 ) ) { // no tripod
			gun->mg42BaseEnt = base->s.number;
		} else {
			gun->mg42BaseEnt = -1;
		}

		gun->spawnflags = ent->spawnflags;

		trap_LinkEntity( gun );

		if ( !( ent->spawnflags & 2 ) ) { // no tripod
			base->chain = gun;
		}

		G_FreeEntity( ent );
	}
}

/*QUAKED misc_mg42 (1 0 0) (-16 -16 -24) (16 16 24) HIGH NOTRIPOD
harc = horizonal fire arc Default is 57.5 (left and right)
varc = vertical fire arc Default is 45 (upwards and 22.5 down)
health = how much damage can it take default is 50
damage = determines how much the weapon will inflict if a non player uses it
"accuracy" all guns are 100% accurate an entry of 0.5 would make it 50%
*/
void SP_mg42( gentity_t *self ) {
	char        *damage;
	char        *accuracy;

	if ( !self->harc ) {
		self->harc = 57.5;
	} else
	{
		if ( self->harc < 45 ) {
			self->harc = 45;
		}
	}

	if ( !self->varc ) {
		self->varc = 45.0;
	}

	if ( !self->health ) {
		if ( g_gametype.integer == GT_SINGLE_PLAYER ) { // JPW NERVE
			self->health = 100;
		}
// JPW NERVE
		else {
			self->health = MG42_MULTIPLAYER_HEALTH;
		}
	}
// jpw

	self->think = mg42_spawn;
	self->nextthink = level.time + FRAMETIME;

	snd_noammo = G_SoundIndex( "sound/weapons/noammo.wav" );

	if ( G_SpawnString( "damage", "0", &damage ) ) {
		self->damage = atoi( damage );
	}

	G_SpawnString( "accuracy", "1.0", &accuracy );

	self->accuracy = atof( accuracy );

	if ( !self->accuracy ) {
		self->accuracy = 1;
	}
// JPW NERVE
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		if ( !self->damage ) {
			self->damage = 25;
		}
	}
// jpw
}

#define FLAK_SPREAD 100
#define FLAK_DAMAGE 36

#define GUN1_IDLE   0
#define GUN2_IDLE   4
#define GUN3_IDLE   8
#define GUN4_IDLE   12

#define GUN1_LASTFIRE   3
#define GUN2_LASTFIRE   7
#define GUN3_LASTFIRE   11
#define GUN4_LASTFIRE   15

void Flak_Animate( gentity_t *ent ) {
	//G_Printf ("frame %i\n", ent->s.frame);

	if ( ent->s.frame == GUN1_IDLE
		 || ent->s.frame == GUN2_IDLE
		 || ent->s.frame == GUN3_IDLE
		 || ent->s.frame == GUN4_IDLE ) {
		return;
	}

	if ( ent->count == 1 ) {
		if ( ent->s.frame == GUN1_LASTFIRE ) {
			ent->s.frame = GUN2_IDLE;
		} else if ( ent->s.frame > GUN1_IDLE ) {
			ent->s.frame++;
		}
	} else if ( ent->count == 2 )     {
		if ( ent->s.frame == GUN2_LASTFIRE ) {
			ent->s.frame = GUN3_IDLE;
		} else if ( ent->s.frame > GUN2_IDLE ) {
			ent->s.frame++;
		}
	} else if ( ent->count == 3 )     {
		if ( ent->s.frame == GUN3_LASTFIRE ) {
			ent->s.frame = GUN4_IDLE;
		} else if ( ent->s.frame > GUN3_IDLE ) {
			ent->s.frame++;
		}
	} else if ( ent->count == 4 )     {
		if ( ent->s.frame == GUN4_LASTFIRE ) {
			ent->s.frame = GUN1_IDLE;
		} else if ( ent->s.frame > GUN4_IDLE ) {
			ent->s.frame++;
		}
	}
}


void flak_spawn( gentity_t *ent ) {
	gentity_t *gun;
	vec3_t offset;

	gun = G_Spawn();
	gun->classname = "misc_flak";
	gun->clipmask = CONTENTS_SOLID;
	gun->r.contents = CONTENTS_TRIGGER;
	gun->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	gun->s.eType = ET_GENERAL;
	gun->touch = mg42_touch;
	gun->s.modelindex = G_ModelIndex( "models/mapobjects/weapons/flak_a.md3" );
	VectorCopy( ent->s.origin, offset );
	G_SetOrigin( gun, offset );
	VectorSet( gun->r.mins, -24, -24, -8 );
	VectorSet( gun->r.maxs, 24, 24, 48 );
	gun->s.apos.trTime = 0;
	gun->s.apos.trDuration = 0;
	VectorCopy( ent->s.angles, gun->s.angles );
	VectorCopy( gun->s.angles, gun->s.apos.trBase );
	VectorCopy( gun->s.angles, gun->s.apos.trDelta );
	gun->think = mg42_think;
	gun->nextthink = level.time + FRAMETIME;
	gun->s.number = gun - g_entities;
	gun->harc = ent->harc;
	gun->varc = ent->varc;
	gun->s.apos.trType = TR_LINEAR_STOP;    // interpolate the angles
	gun->takedamage = qtrue;
	gun->targetname = ent->targetname;      // need this for scripting
	gun->mg42BaseEnt = ent->s.number;

	trap_LinkEntity( gun );

}

/*QUAKED misc_flak (1 0 0) (-32 -32 0) (32 32 100)
*/
void SP_misc_flak( gentity_t *self ) {

	if ( !self->harc ) {
		self->harc = 180;
	} else
	{
		if ( self->harc < 90 ) {
			self->harc = 115;
		}
	}

	if ( !self->varc ) {
		self->varc = 90.0;
	}

	if ( !self->health ) {
		self->health = 100;
	}

	self->think = flak_spawn;
	self->nextthink = level.time + FRAMETIME;

	snd_noammo = G_SoundIndex( "sound/weapons/noammo.wav" );
}

/*QUAKED misc_spawner (.3 .7 .8) (-8 -8 -8) (8 8 8)
use the pickup name
  when this entity gets used it will spawn an item
that matches its spawnitem field
e.i.
spawnitem
9mm
*/

void misc_spawner_think( gentity_t *ent ) {

	gitem_t     *item;
	gentity_t   *drop = NULL;

	item = BG_FindItem( ent->spawnitem );

	drop = Drop_Item( ent, item, 0, qfalse );

	if ( !drop ) {
		G_Printf( "-----> WARNING <-------\n" );
		G_Printf( "misc_spawner used at %s failed to drop!\n", vtos( ent->r.currentOrigin ) );
	}

}

void misc_spawner_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {

	ent->think = misc_spawner_think;
	ent->nextthink = level.time + FRAMETIME;

//	VectorCopy (other->r.currentOrigin, ent->r.currentOrigin);
//	VectorCopy (ent->r.currentOrigin, ent->s.pos.trBase);

//	VectorCopy (other->r.currentAngles, ent->r.currentAngles);

	trap_LinkEntity( ent );
}

void SP_misc_spawner( gentity_t *ent ) {
	if ( !ent->spawnitem ) {
		G_Printf( "-----> WARNING <-------\n" );
		G_Printf( "misc_spawner at loc %s has no spawnitem!\n", vtos( ent->s.origin ) );
		return;
	}

	ent->use = misc_spawner_use;

	trap_LinkEntity( ent );

}

//===========================================================================
//
// Mounted Gunner, attached to a moving vehicle of some-sort
//

/*QUAKED misc_mounted_gunner (.4 .9 .7) (-16 -16 -24) (16 16 64) TriggerSpawn
Gunner controlling an MG42 to be mounted onto a vehicle.

Gun must be able to rotate full 360 degrees, with slight pitching.

The following entity script triggers will be called at appropriate times by the engine:

  trigger gunner_start_attack
  trigger gunner_end_attack
  trigger gunner_death

Fields:

*IMPORTANT* color = vector offest of gun mount tag on truck, from truck's origin
*IMPORTANT* delay = offset distance of gunnner mount tag on gun model, from gun's origin
target = targetname of vehicle we are attached to
aiTeam = 0 for Nazi, 1 for Allies, 2 for Monster
harc = visible arc, for initially sighting enemies Default  is 220
radius = visible distance, for sighting enemies Default is 4096

varc = vertical fire arc Default is 90 (45 above and below)
health = how much damage can it take default is 50
accuracy = all guns are 100% accurate an entry of 0.5 would make it 50%
damage = damage caused by each bullet
*/

#define MG42_SPREAD         200
#define MG42_DAMAGE_AI      9

void miscGunnerEnemyScan( gentity_t *ent, vec3_t angles ) {
	gentity_t *t;
	vec3_t v, tang;

	for ( t = g_entities; t < g_entities + level.maxclients; t++ ) {
		if ( !t->inuse ) {
			continue;
		}
		if ( t->health < 0 ) {
			continue;
		}
		if ( ent->aiTeam == t->aiTeam ) {
			continue;
		}
		if ( VectorDistance( ent->r.currentOrigin, t->r.currentOrigin ) > ent->radius ) {
			continue;
		}
		VectorSubtract( t->r.currentOrigin, ent->r.currentOrigin, v );
		vectoangles( v, tang );
		if ( !AICast_InFieldOfVision( angles, ent->harc, tang ) ) {
			continue;
		}
		if ( !AICast_VisibleFromPos( ent->r.currentOrigin, ent->s.number, t->r.currentOrigin, t->s.number, qfalse ) ) {
			continue;
		}
		// found an enemy
		ent->enemy = t;
		break;
	}
}


void miscGunnerThink( gentity_t *ent ) {
	gentity_t   *truck, *gun, *enemy;
	vec3_t vec, trang, gspot, gang;
	vec3_t fwd, r, u;
	qboolean fire = qfalse;
	float yawspeed, diff;
	vec3_t dang;
	qboolean clamped = qfalse;
	int i;

	// find the entities
	gun = &g_entities[ent->mg42BaseEnt];
	truck = &g_entities[gun->mg42BaseEnt];

	// calculate our position based on that of the truck and gun angles
	BG_EvaluateTrajectory( &truck->s.apos, level.time, trang );
	AngleVectors( trang, fwd, r, u );
	BG_EvaluateTrajectory( &truck->s.pos, level.time, gspot );
	VectorMA( gspot, ent->dl_color[0], fwd, gspot );
	VectorMA( gspot, -ent->dl_color[1], r, gspot );
	VectorMA( gspot, ent->dl_color[2], u, gspot );
	G_SetOrigin( ent, gspot );
	G_SetOrigin( gun, gspot );

	// calculate our facing angles
	//VectorAdd( trang, gun->r.currentAngles, gang );
	if ( !gun->s.density ) {
		VectorCopy( trang, gang );
	} else {
		BG_EvaluateTrajectory( &gun->s.apos, level.time, gang );
	}

	// look for an enemy
	if ( ent->enemy ) {
		if ( ent->enemy->health <= 0 ) {
			ent->enemy = NULL;
		} else if ( AICast_VisibleFromPos( ent->r.currentOrigin, ent->s.number, ent->enemy->r.currentOrigin, ent->enemy->s.number, qfalse ) ) {
			fire = qtrue;
		}
	}

	if ( !fire ) {
		miscGunnerEnemyScan( ent, trang );
		if ( ent->enemy ) {
			fire = qtrue;

			// if we have just found our first enemy, release us from the "fixed tag" angles
			// so we can track them
			if ( !gun->s.density ) {
				gun->s.density = 1;
				// start facing the same direct, so we don't snap to a different angle immediately
				BG_EvaluateTrajectory( &truck->s.apos, level.time, ent->s.angles );
				VectorCopy( gun->s.angles, gun->s.apos.trBase );
				gun->s.apos.trTime = level.time;
				gun->s.apos.trDuration = 0;
				gun->s.apos.trType = TR_STATIONARY;
				VectorClear( gun->s.apos.trDelta );
			}
		}
	}

	// third, attack that enemy if possible
	if ( fire ) {

		// get the enemy
		enemy = ent->enemy;

		// rotate to them
		VectorSubtract( enemy->r.currentOrigin, gun->r.currentOrigin, vec );
		vectoangles( vec, gun->TargetAngles );
		VectorCopy( gun->TargetAngles, dang );
		for ( i = 0; i < 3; i++ ) {
			dang[i] = AngleNormalize180( dang[i] );
		}

		// restrict vertical range
		if ( dang[0] < 0 && fabs( dang[0] ) > ( gun->varc / 2 ) ) {
			clamped = qtrue;
			if ( dang[0] < 0 ) {
				dang[0] = -( gun->varc / 2 );
			} else {
				dang[0] =  ( gun->varc / 2 );
			}
		}

		// dang is now the ideal angles, restrict movement by speed
		yawspeed = 60;
		for ( i = 0; i < 3; i++ ) {
			BG_EvaluateTrajectory( &gun->s.apos, level.time, gun->r.currentAngles );
			diff = AngleDifference( dang[i], gun->r.currentAngles[i] );
			if ( fabs( diff ) > ( yawspeed * ( (float)FRAMETIME / 1000.0 ) ) ) {
				clamped = qtrue;
				if ( diff > 0 ) {
					dang[i] = AngleMod( gun->r.currentAngles[i] + ( yawspeed * ( (float)FRAMETIME / 1000.0 ) ) );
				} else {
					dang[i] = AngleMod( gun->r.currentAngles[i] - ( yawspeed * ( (float)FRAMETIME / 1000.0 ) ) );
				}
			}
		}

		// move to the position over the next frame
		VectorSubtract( dang, gun->r.currentAngles, gun->s.apos.trDelta );
		for ( i = 0; i < 3; i++ ) {
			gun->s.apos.trDelta[i] = AngleNormalize180( gun->s.apos.trDelta[i] );
		}
		VectorCopy( gun->r.currentAngles, gun->s.apos.trBase );
		VectorScale( gun->s.apos.trDelta, 1000 / 50, gun->s.apos.trDelta );
		gun->s.apos.trTime = level.time;
		gun->s.apos.trType = TR_LINEAR_STOP;
		gun->s.apos.trDuration = 50;

		// if we are facing them, fire
		if ( fabs( AngleNormalize180( gun->r.currentAngles[YAW] - gun->TargetAngles[YAW] ) ) < 10 ) {
			AngleVectors( gun->r.currentAngles, forward, right, up );
			VectorCopy( gspot, muzzle );

			VectorMA( muzzle, 16, forward, muzzle );
			VectorMA( muzzle, 16, up, muzzle );

			// snap to integer coordinates for more efficient network bandwidth usage
			SnapVector( muzzle );

			if ( gun->damage ) {
				Fire_Lead( gun, gun, MG42_SPREAD / gun->accuracy, gun->damage );
			} else {
				Fire_Lead( gun, gun, MG42_SPREAD / gun->accuracy, MG42_DAMAGE_AI );
			}
		}
	}

	ent->think = miscGunnerThink;
	ent->nextthink = level.time + 50;
}

void miscGunnerSpawn( gentity_t *ent ) {
	gentity_t *sp, *veh;

	veh = G_Find( NULL, FOFS( targetname ), ent->target );
	if ( !veh ) {
		G_Error( "can't find vehicle with targetname \"%s\" for mounted gunner", ent->target );
	}

	// create the ring (base)
	sp = G_Spawn();
	sp->classname = "misc_gunner_ring";
	sp->r.contents = 0;
	sp->s.eType = ET_GENERAL;
	sp->s.modelindex = G_ModelIndex( "models/mapobjects/weapons/turret_c.md3" );
	sp->tagParent = veh;
	sp->tagName = "tag_ring";    // tag to connect to
	G_ProcessTagConnect( sp );
	trap_LinkEntity( sp );

	// create the gun
	sp = G_Spawn();
	sp->classname = "misc_gunner_gun";
	sp->r.contents = 0;
	sp->s.eType = ET_GENERAL;
	sp->s.modelindex = G_ModelIndex( "models/mapobjects/weapons/turret_a.md3" );
	sp->tagParent = veh;
	sp->tagName = "tag_rider";   // tag to connect to
	G_ProcessTagConnect( sp );
	trap_LinkEntity( sp );

	sp->mg42BaseEnt = veh->s.number;
	sp->varc = ent->varc;
	sp->health = ent->health;
	sp->accuracy = ent->accuracy;
	sp->damage = ent->damage;

	// setup the gunner ready for action
	sp->r.contents = 0; // fixme, make damagable so we can shoot them
	sp->s.eType = ET_GENERAL;
	ent->s.modelindex = G_ModelIndex( "models/mapobjects/weapons/turret_b.md3" );
	ent->tagParent = sp;
	ent->tagName = "tag_hand";   // tag to connect to
	G_ProcessTagConnect( ent );
	trap_LinkEntity( ent );

	ent->mg42BaseEnt = sp->s.number;

	// start us thinking
	ent->think = miscGunnerThink;
	ent->nextthink = level.time + 50;
}

void miscGunnerTriggerSpawn( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	miscGunnerSpawn( ent );
}

void SP_misc_mounted_gunner( gentity_t *self ) {
	char        *damage;
	char        *accuracy;

//	if (VectorCompare(vec3_origin, self->dl_color)) {
	if ( VectorCompare( vec3_origin, self->rotate ) ) {
		G_Error( "misc_mounted_gunner requires an offset position (color field)\n" );
	}
	if ( !self->delay ) {
		G_Error( "misc_mounted_gunner requires an offset distance from gun mount (delay field)\n" );
	}

	if ( !self->harc ) {
		self->harc = 115;
	} else
	{
		if ( self->harc < 45 ) {
			self->harc = 45;
		}
	}

	if ( !self->varc ) {
		self->varc = 90.0;
	}

	if ( !self->health ) {
		self->health = 100;
	}

	if ( !self->radius ) {
		self->radius = 4096;
	}

	snd_noammo = G_SoundIndex( "sound/weapons/noammo.wav" );

	if ( G_SpawnString( "damage", "0", &damage ) ) {
		self->damage = atoi( damage );
	}

	G_SpawnString( "accuracy", "1.0", &accuracy );

	self->accuracy = atof( accuracy );

	if ( !self->accuracy ) {
		self->accuracy = 1;
	}

	// we are idle at this point
	self->enemy = NULL;

	if ( self->spawnflags & 1 ) {     // TRIGGER_SPAWN
		self->use = miscGunnerTriggerSpawn;
	} else {
		// delay the actual spawning, so we are sure everything else exists
		self->think = miscGunnerSpawn;
		self->nextthink = level.time + FRAMETIME;
	}
}

void firetrail_die( gentity_t *ent ) {
	G_FreeEntity( ent );
}

void firetrail_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->s.eType == ET_RAMJET ) {
		ent->s.eType = ET_GENERAL;
	} else {
		ent->s.eType = ET_RAMJET;
	}

	trap_LinkEntity( ent );

}

/*QUAKED misc_firetrails (.4 .9 .7) (-16 -16 -16) (16 16 16)
This entity must target the plane its going to be attached to

  its use function will turn the fire stream effect on and off

  an alert entity call will kill it
*/

void misc_firetrails_think( gentity_t *ent ) {
	gentity_t *left, *right, *airplane;

	airplane = G_Find( NULL, FOFS( targetname ), ent->target );
	if ( !airplane ) {
		G_Error( "can't find airplane with targetname \"%s\" for firetrails", ent->target );
	}

	// left fire trail
	left = G_Spawn();
	left->classname = "left_firetrail";
	left->r.contents = 0;
	left->s.eType = ET_RAMJET;
	left->s.modelindex = G_ModelIndex( "models/ammo/rocket/rocket.md3" );
	left->tagParent = airplane;
	left->tagName = "tag_engine1";   // tag to connect to
	left->use = firetrail_use;
	left->AIScript_AlertEntity = firetrail_die;
	left->targetname = ent->targetname;
	G_ProcessTagConnect( left );
	trap_LinkEntity( left );

	// right fire trail
	right = G_Spawn();
	right->classname = "right_firetrail";
	right->r.contents = 0;
	right->s.eType = ET_RAMJET;
	right->s.modelindex = G_ModelIndex( "models/ammo/rocket/rocket.md3" );
	right->tagParent = airplane;
	right->tagName = "tag_engine2";  // tag to connect to
	right->use = firetrail_use;
	right->AIScript_AlertEntity = firetrail_die;
	right->targetname = ent->targetname;
	G_ProcessTagConnect( right );
	trap_LinkEntity( right );

}

void SP_misc_firetrails( gentity_t *ent ) {
	ent->think = misc_firetrails_think;
	ent->nextthink = level.time + 100;

}
