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
	SP_Tramcar
*/
#include "g_local.h"

// defines
#define TRAMCAR_START_ON        1
#define TRAMCAR_TOGGLE          2
#define TRAMCAR_BLOCK_STOPS     4
#define TRAMCAR_LEADER          8

void props_me109_think( gentity_t *ent );
void ExplodePlaneSndFx( gentity_t *self );

void Think_SetupAirplaneWaypoints( gentity_t *ent );
void truck_cam_think( gentity_t *ent );

// extern calls
extern void Think_SetupTrainTargets( gentity_t *ent );
extern void Reached_BinaryMover( gentity_t *ent );
extern void MatchTeam( gentity_t *teamLeader, int moverState, int time );
extern void SetMoverState( gentity_t *ent, moverState_t moverState, int time );
extern void Blocked_Door( gentity_t *ent, gentity_t *other );
extern void Think_BeginMoving( gentity_t *ent );
extern void propExplosionLarge( gentity_t *ent );
////////////////////////
// truck states
////////////////////////
typedef enum
{
	truck_nosound = 0,
	truck_idle,
	truck_gear1,
	truck_gear2,
	truck_gear3,
	truck_reverse,
	truck_moving,
	truck_breaking,
	truck_bouncy1,
	truck_bouncy2,
	truck_bouncy3
} truck_states1;

/////////////////////////
// truck sounds
/////////////////////////

int truck_idle_snd;
int truck_gear1_snd;
int truck_gear2_snd;
int truck_gear3_snd;
int truck_reverse_snd;
int truck_moving_snd;
int truck_breaking_snd;
int truck_bouncy1_snd;
int truck_bouncy2_snd;
int truck_bouncy3_snd;
int truck_sound;


////////////////////////
// plane states
////////////////////////
typedef enum
{
	plane_nosound = 0,
	plane_idle,
	plane_flyby1,
	plane_flyby2,
	plane_loop,
	plane_choke,
	plane_startup
} truck_states2;

///////////////////////////
// aircraft sounds
///////////////////////////

int fploop_snd;
int fpchoke_snd;
int fpattack_snd;
int fpexpdebris_snd;

int fpflyby1_snd;
int fpflyby2_snd;
int fpidle_snd;
int fpstartup_snd;

///////////////////////////
// airplane parts
///////////////////////////
int fuse_part;
int wing_part;
int tail_part;
int nose_part;
int crash_part;

// functions to be added
void InitTramcar( gentity_t *ent ) {
	vec3_t move;
	float distance;
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;
	char        *sound;

	// This is here just for show
	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}

	if ( !Q_stricmp( ent->classname, "props_me109" ) ) {
		ent->s.modelindex2 = G_ModelIndex( "models/mapobjects/vehicles/m109s.md3" );
	}

	if ( !Q_stricmp( ent->classname, "truck_cam" ) ) {
		ent->s.modelindex2 = G_ModelIndex( "models/mapobjects/vehicles/truck_base.md3" );
	}

	// if the "loopsound" key is set, use a constant looping sound when moving
	if ( G_SpawnString( "noise", "100", &sound ) ) {
		ent->s.loopSound = G_SoundIndex( sound );
	}

	// if the "color" or "light" keys are set, setup constantLight
	lightSet = G_SpawnFloat( "light", "100", &light );
	colorSet = G_SpawnVector( "color", "1 1 1", color );
	if ( lightSet || colorSet ) {
		int r, g, b, i;

		r = color[0] * 255;
		if ( r > 255 ) {
			r = 255;
		}
		g = color[1] * 255;
		if ( g > 255 ) {
			g = 255;
		}
		b = color[2] * 255;
		if ( b > 255 ) {
			b = 255;
		}
		i = light / 4;
		if ( i > 255 ) {
			i = 255;
		}
		ent->s.constantLight = r | ( g << 8 ) | ( b << 16 ) | ( i << 24 );
	}

	ent->use = Use_BinaryMover;
//	ent->reached = Reached_BinaryMover;

	ent->moverState = MOVER_POS1;
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;

	VectorCopy( ent->pos1, ent->r.currentOrigin );

	trap_LinkEntity( ent );

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->s.pos.trBase );

	// calculate time to reach second position from speed
	VectorSubtract( ent->pos2, ent->pos1, move );
	distance = VectorLength( move );
	if ( !ent->speed ) {
		ent->speed = 100;
	}
	VectorScale( move, ent->speed, ent->s.pos.trDelta );
	ent->s.pos.trDuration = distance * 1000 / ent->speed;
	if ( ent->s.pos.trDuration <= 0 ) {
		ent->s.pos.trDuration = 1;
	}
}

void Calc_Roll( gentity_t *ent ) {
	gentity_t *target;
	vec3_t vec;
	vec3_t forward;
	vec3_t right;
	float dot;
	float dot2;
	vec3_t tang;

	target = ent->nextTrain;

	VectorCopy( ent->r.currentAngles, tang );
	tang[ROLL] = 0;

	AngleVectors( tang, forward, right, NULL );
	VectorSubtract( target->nextTrain->nextTrain->s.origin, ent->r.currentOrigin, vec );
	VectorNormalize( vec );

	dot = DotProduct( vec, forward );
	dot2 = DotProduct( vec, right );

	ent->angle = (int) ent->angle;

	if ( dot2 > 0 ) {
		if ( ent->s.apos.trBase[ROLL] < -( ent->angle * 2 ) ) {
			ent->s.apos.trBase[ROLL] += 2;
		} else if ( ent->s.apos.trBase[ROLL] > -( ent->angle * 2 ) ) {
			ent->s.apos.trBase[ROLL] -= 2;
		}

		if ( ent->s.apos.trBase[ROLL] > 90 ) {
			ent->s.apos.trBase[ROLL] = 90;
		}
	} else if ( dot2 < 0 )     {
		if ( ent->s.apos.trBase[ROLL] > -( ent->angle * 2 ) ) {
			ent->s.apos.trBase[ROLL] -= 2;
		} else if ( ent->s.apos.trBase[ROLL] < -( ent->angle * 2 ) ) {
			ent->s.apos.trBase[ROLL] += 2;
		}

		if ( ent->s.apos.trBase[ROLL] < -90 ) {
			ent->s.apos.trBase[ROLL] = -90;
		}
	} else {
		ent->s.apos.trBase[ROLL] = 0;
	}


// G_Printf ("dot: %5.2f dot2: %5.2f\n", dot, dot2);


//	VectorCopy (ent->r.currentAngles, ent->TargetAngles);

	trap_LinkEntity( ent );

	ent->nextthink = level.time + 50;
}


#define MAXCHOICES  8

void GetNextTrack( gentity_t *ent ) {
	gentity_t   *track = NULL;
	gentity_t   *next;
	gentity_t   *choice[MAXCHOICES];
	int num_choices = 0;
	int rval;

	next = ent->nextTrain;

	if ( !( next->track ) ) {
		G_Printf( "NULL track name for %s on %s\n", ent->classname, next->targetname );
		return;
	}

	while ( 1 )
	{
		track = G_Find( track, FOFS( targetname ), next->track );

		if ( !track ) {
			break;
		}

		choice[num_choices++] = track;

		if ( num_choices == MAXCHOICES ) {
			break;
		}
	}

	if ( !num_choices ) {
		G_Printf( "GetNextTrack didnt find a track\n" );
		return;
	}

	rval = rand() % num_choices;

	ent->nextTrain = NULL;
	ent->target = choice[rval]->targetname;

}

void Reached_Tramcar( gentity_t *ent ) {
	gentity_t       *next;
	float speed;
	vec3_t move;
	float length;

	// copy the apropriate values
	next = ent->nextTrain;
	if ( !next || !next->nextTrain ) {
		return;     // just stop
	}

	// Rafael
	if ( next->wait == -1 && next->count ) {
		// G_Printf ("stoped wait = -1 count %i\n",next->count);
		return;
	}

	if ( !Q_stricmp( ent->classname, "props_me109" ) ) {
		vec3_t vec, angles;
		float diff;

		if ( next->spawnflags & 8 ) { // laps
			next->count--;

			if ( !next->count ) {
				next->count = next->count2;

				GetNextTrack( ent );
				Think_SetupAirplaneWaypoints( ent );

				next = ent->nextTrain;

				G_Printf( "changed track to %s\n", next->targetname );
			} else {
				G_Printf( "%s lap %i\n", next->targetname, next->count );
			}
		} else if ( ( next->spawnflags & 1 ) && !( next->count ) && ent->health > 0 )         { // SCRIPT flag
			GetNextTrack( ent );
			Think_SetupAirplaneWaypoints( ent );
		} else if ( ( next->spawnflags & 2 ) && ( ent->spawnflags & 8 ) && ent->health <= 0 && ent->takedamage )         { // death path
			ent->takedamage = qfalse;

			GetNextTrack( ent );
			Think_SetupAirplaneWaypoints( ent );
		} else if ( ( next->spawnflags & 4 ) )       { // explode the plane
			ExplodePlaneSndFx( ent );

			ent->s.modelindex = crash_part;
			// spawn the wing at the player effect

			ent->nextTrain = NULL;
			G_UseTargets( next, NULL );

			return;
		}

		VectorSubtract( ent->nextTrain->nextTrain->s.origin, ent->r.currentOrigin, vec );
		vectoangles( vec, angles );


		diff = AngleSubtract( ent->r.currentAngles [YAW], angles[YAW] );
		// diff = AngleSubtract (ent->TargetAngles [YAW], angles[YAW]);

		ent->rotate[1] = 1;
		ent->angle = -diff;

		//if (angles[YAW] == 0)
		//	ent->s.apos.trDuration = ent->s.pos.trDuration;
		//else
		//	ent->s.apos.trDuration = 1000;

		{
			VectorCopy( next->s.origin, ent->pos1 );
			VectorCopy( next->nextTrain->s.origin, ent->pos2 );

			// if the path_corner has a speed, use that
			if ( next->speed ) {
				speed = next->speed;
			} else {
				// otherwise use the train's speed
				speed = ent->speed;
			}
			if ( speed < 1 ) {
				speed = 1;
			}

			// calculate duration
			VectorSubtract( ent->pos2, ent->pos1, move );
			length = VectorLength( move );

			ent->s.apos.trDuration = length * 1000 / speed;

//testing
// ent->gDuration = ent->s.apos.trDuration;
			ent->gDurationBack = ent->gDuration = ent->s.apos.trDuration;
// ent->gDeltaBack = ent->gDelta =

		}

		VectorClear( ent->s.apos.trDelta );

		SetMoverState( ent, MOVER_1TO2ROTATE, level.time );
		VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );

		trap_LinkEntity( ent );

		ent->think = props_me109_think;
		ent->nextthink = level.time + 50;
	} else if ( !Q_stricmp( ent->classname, "truck_cam" ) )       {
		G_Printf( "target: %s\n", next->targetname );

		if ( next->spawnflags & 2 ) { // END
			ent->s.loopSound = 0; // stop sound
			ent->nextTrain = NULL;
			return;
		} else
		{
			vec3_t vec, angles;
			float diff;

			if ( next->spawnflags & 4 ) { // reverse
				ent->props_frame_state = truck_reverse;
				VectorSubtract( ent->r.currentOrigin, ent->nextTrain->nextTrain->s.origin, vec );
			} else
			{
				ent->props_frame_state = truck_moving;
				VectorSubtract( ent->nextTrain->nextTrain->s.origin, ent->r.currentOrigin, vec );
			}

			vectoangles( vec, angles );

			diff = AngleSubtract( ent->r.currentAngles [YAW], angles[YAW] );

			ent->rotate[1] = 1;
			ent->angle = -diff;

			if ( angles[YAW] == 0 ) {
				ent->s.apos.trDuration = ent->s.pos.trDuration;
			} else {
				ent->s.apos.trDuration = 1000;
			}

//testing
			ent->gDuration = ent->s.pos.trDuration;

			VectorClear( ent->s.apos.trDelta );

			SetMoverState( ent, MOVER_1TO2ROTATE, level.time );
			VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );

			trap_LinkEntity( ent );
		}

		if ( next->wait == -1 ) {
			ent->props_frame_state = truck_idle;
		}

		if ( next->count2 == 1 ) {
			ent->props_frame_state = truck_gear1;
		} else if ( next->count2 == 2 ) {
			ent->props_frame_state = truck_gear2;
		} else if ( next->count2 == 3 ) {
			ent->props_frame_state = truck_gear3;
		}

		switch ( ent->props_frame_state )
		{
		case truck_idle: ent->s.loopSound = truck_idle_snd; break;
		case truck_gear1: ent->s.loopSound = truck_gear1_snd; break;
		case truck_gear2: ent->s.loopSound = truck_gear2_snd; break;
		case truck_gear3: ent->s.loopSound = truck_gear3_snd; break;
		case truck_reverse: ent->s.loopSound = truck_reverse_snd; break;
		case truck_moving: ent->s.loopSound = truck_moving_snd; break;
		case truck_breaking: ent->s.loopSound = truck_breaking_snd; break;
		case truck_bouncy1: ent->s.loopSound = truck_bouncy1_snd; break;
		case truck_bouncy2: ent->s.loopSound = truck_bouncy2_snd; break;
		case truck_bouncy3: ent->s.loopSound = truck_bouncy3_snd; break;
		}

//testing
		ent->s.loopSound = truck_sound;
		ent->think = truck_cam_think;
		ent->nextthink = level.time + ( FRAMETIME / 2 );

	} else if ( !Q_stricmp( ent->classname, "camera_cam" ) )       {

	}

	// fire all other targets
	G_UseTargets( next, NULL );

	// set the new trajectory
	ent->nextTrain = next->nextTrain;

	if ( next->wait == -1 ) {
		next->count = 1;
	}

	VectorCopy( next->s.origin, ent->pos1 );
	VectorCopy( next->nextTrain->s.origin, ent->pos2 );

	// if the path_corner has a speed, use that
	if ( next->speed ) {
		speed = next->speed;
	} else {
		// otherwise use the train's speed
		speed = ent->speed;
	}
	if ( speed < 1 ) {
		speed = 1;
	}

	// calculate duration
	VectorSubtract( ent->pos2, ent->pos1, move );
	length = VectorLength( move );

	ent->s.pos.trDuration = length * 1000 / speed;

//testing
// ent->gDuration = ent->s.pos.trDuration;
	ent->gDurationBack = ent->gDuration = ent->s.pos.trDuration;
// ent->gDeltaBack = ent->gDelta = ;

	// looping sound
	if ( next->soundLoop ) {
		ent->s.loopSound = next->soundLoop;
	}

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	// if there is a "wait" value on the target, don't start moving yet
	// if ( next->wait )
	if ( next->wait && next->wait != -1 ) {
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
}

extern void func_explosive_explode( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );

void Tramcar_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	gentity_t       *slave;

	func_explosive_explode( self, self, inflictor, 0, 0 );

	// link all teammembers
	for ( slave = self ; slave ; slave = slave->teamchain )
	{

		if ( slave == self ) {
			continue;
		}

		// slaves need to inherit position
		slave->nextTrain = self->nextTrain;

		slave->s.pos.trType = self->s.pos.trType;
		slave->s.pos.trTime = self->s.pos.trTime;
		slave->s.pos.trDuration = self->s.pos.trDuration;
		VectorCopy( self->s.pos.trBase, slave->s.pos.trBase );
		VectorCopy( self->s.pos.trDelta, slave->s.pos.trDelta );

		slave->s.apos.trType = self->s.apos.trType;
		slave->s.apos.trTime = self->s.apos.trTime;
		slave->s.apos.trDuration = self->s.apos.trDuration;
		VectorCopy( self->s.apos.trBase, slave->s.apos.trBase );
		VectorCopy( self->s.apos.trDelta, slave->s.apos.trDelta );

		slave->think = self->think;
		slave->nextthink = self->nextthink;

		VectorCopy( self->pos1, slave->pos1 );
		VectorCopy( self->pos2, slave->pos2 );

		slave->speed = self->speed;

		slave->flags &= ~FL_TEAMSLAVE;
		// make it visible

		if ( self->use ) {
			slave->use = self->use;
		}

		trap_LinkEntity( slave );
	}

	self->use = NULL;

	self->is_dead = qtrue;

	self->takedamage = qfalse;

	if ( self->nextTrain ) {
		self->nextTrain = 0;
	}

	self->s.loopSound = 0;

	VectorCopy( self->r.currentOrigin, self->s.pos.trBase );
	VectorCopy( self->r.currentAngles, self->s.apos.trBase );

	self->flags |= FL_TEAMSLAVE;
	trap_UnlinkEntity( self );

}

void TramCarUse( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t       *next;

	if ( level.time >= ent->s.pos.trTime + ent->s.pos.trDuration ) {

		next = ent->nextTrain;

		if ( next->wait == -1 && next->count ) {
			next->count = 0;
			//G_Printf ("Moving next->count %i\n", next->count);
		}

		Reached_Tramcar( ent );

	}
//	else
//		G_Printf ("no can do havent reached yet\n");

}


void Blocked_Tramcar( gentity_t *ent, gentity_t *other ) {
	// remove anything other than a client
	if ( !other->client ) {
		// except CTF flags!!!!
		if ( other->s.eType == ET_ITEM && other->item->giType == IT_TEAM ) {
			Team_DroppedFlagThink( other );
			return;
		}
		G_TempEntity( other->s.origin, EV_ITEM_POP );
		G_FreeEntity( other );
		return;
	}

	if ( other->flags & FL_GODMODE ) {
		other->flags &= ~FL_GODMODE;
		other->client->ps.stats[STAT_HEALTH] = other->health = 0;
	}

	G_Damage( other, ent, ent, NULL, NULL, 99999, 0, MOD_CRUSH );

}

/*QUAKED func_tramcar (0 .5 .8) ? START_ON TOGGLE - LEADER
health value of 999 will designate the piece as non damageable

The leader of the tramcar group must have the leader flag set or
you'll end up with co-planer poly heaven. When the health of the Leader
of the team hits zero it will unlink and the team will become visible

A tramcar is a mover that moves between path_corner target points.
all tramcar parts MUST HAVE AN ORIGIN BRUSH. (this is true for all parts)

The tramcar spawns at the first target it is pointing at.

If you are going to have enemies ride the tramcar it must be placed in its ending
position when you bsp the map you can the start it by targeting the desired path_corner

"model2"	.md3 model to also draw
"speed"		default 100
"dmg"		default	2
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius

"type" type of debris ("glass", "wood", "metal", "gibs") default is "wood"
"mass" defaults to 75.  This determines how much debris is emitted when it explodes.  You get one large chunk per 100 of mass (up to 8) and one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/
void SP_func_tramcar( gentity_t *self ) {

	int mass;
	char    *type;
	char    *s;
	char buffer[MAX_QPATH];

	VectorClear( self->s.angles );

	//if (self->spawnflags & TRAMCAR_BLOCK_STOPS) {
	//	self->damage = 0;
	//	self->s.eFlags |= EF_MOVER_STOP;
	//}
	//else {
	if ( !self->damage ) {
		self->damage = 100;
	}
	//}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		G_Printf( "func_tramcar without a target at %s\n", vtos( self->r.absmin ) );
		G_FreeEntity( self );
		return;
	}

	if ( self->spawnflags & TRAMCAR_LEADER ) {
		if ( !self->health ) {
			self->health = 50;
		}

		self->takedamage = qtrue;
		self->die = Tramcar_die;

		if ( self->health < 999 ) {
			self->isProp = qtrue;
		}
	}

	trap_SetBrushModel( self, self->model );

	if ( G_SpawnInt( "mass", "75", &mass ) ) {
		self->count = mass;
	} else {
		self->count = 75;
	}

	G_SpawnString( "type", "wood", &type );
	if ( !Q_stricmp( type,"wood" ) ) {
		self->key = 0;
	} else if ( !Q_stricmp( type,"glass" ) )   {
		self->key = 1;
	} else if ( !Q_stricmp( type,"metal" ) )                                                            {
		self->key = 2;
	} else if ( !Q_stricmp( type,"gibs" ) )                                                                                                                     {
		self->key = 3;
	}

	if ( G_SpawnString( "noise", "NOSOUND", &s ) ) {
		if ( Q_stricmp( s, "nosound" ) ) {
			Q_strncpyz( buffer, s, sizeof( buffer ) );
			self->s.dl_intensity = G_SoundIndex( buffer );
		}
	} else {
		switch ( self->key )
		{
		case 0:     // "wood"
			self->s.dl_intensity = G_SoundIndex( "sound/world/boardbreak.wav" );
			break;
		case 1:     // "glass"
			self->s.dl_intensity = G_SoundIndex( "sound/world/glassbreak.wav" );
			break;
		case 2:     // "metal"
			self->s.dl_intensity = G_SoundIndex( "sound/world/metalbreak.wav" );
			break;
		case 3:     // "gibs"
			self->s.dl_intensity = G_SoundIndex( "sound/player/gibsplit1.wav" );
			break;
		}
	}

	self->s.density = self->count;  // pass the "mass" to the client

	InitTramcar( self );

	self->reached = Reached_Tramcar;

	self->nextthink = level.time + ( FRAMETIME / 2 );

	self->think = Think_SetupTrainTargets;

	self->blocked = Blocked_Tramcar;

	if ( self->spawnflags & TRAMCAR_TOGGLE ) {
		self->use = TramCarUse;
	}

}


////////////////////////////
// me109
////////////////////////////

void plane_AIScript_AlertEntity( gentity_t *ent ) {

	// when count reaches 0, the marker is active
	ent->count--;

	if ( ent->count <= 0 ) {
		ent->count = 0;
	}
}

/*QUAKED plane_waypoint (.5 .3 0) (-8 -8 -8) (8 8 8) SCRIPTS DIE EXPLODE LAPS ATTACK
"count" number of times this waypoint needs to be triggered by an AIScript "alertentity" call before the aircraft changes tracks
"track"	tells it what track to branch off to there can be several track with the same track name
the program will pick one randomly there can be a maximum of eight tracks at any branch

the entity will fire its target when reached
*/
void SP_plane_waypoint( gentity_t *self ) {

	if ( !self->targetname ) {
		G_Printf( "plane_waypoint with no targetname at %s\n", vtos( self->s.origin ) );
		G_FreeEntity( self );
		return;
	}

	if ( self->spawnflags & 1 ) {
		self->AIScript_AlertEntity = plane_AIScript_AlertEntity;
	}

	if ( self->count ) {
		self->count2 = self->count;
	}

	if ( self->wait == -1 ) {
		self->count = 1;
	}
}


/*QUAKED props_me109 (.7 .3 .1) (-128 -128 0) (128 128 64) START_ON TOGGLE SPINNINGPROP FIXED_DIE
default health = 1000
*/

void ExplodePlaneSndFx( gentity_t *self ) {
	gentity_t   *temp;
	vec3_t dir;
	gentity_t   *part;
	int i;
	vec3_t start;

	temp = G_Spawn();

	if ( !temp ) {
		return;
	}

	G_SetOrigin( temp, self->melee->s.pos.trBase );
	G_AddEvent( temp, EV_GLOBAL_SOUND, fpexpdebris_snd );
	temp->think = G_FreeEntity;
	temp->nextthink = level.time + 10000;
	trap_LinkEntity( temp );

	// added this because plane may be parked on runway
	// we may want to add some exotic deaths to parked aircraft
	if ( self->nextTrain && self->nextTrain->spawnflags & 4 ) { // explode the plane
		// spawn the wing at the player
		gentity_t   *player;
		vec3_t vec, ang;

		player = AICast_FindEntityForName( "player" );

		if ( !player ) {
			return;
		}

		VectorSubtract( player->s.origin, self->r.currentOrigin, vec );
		vectoangles( vec, ang );
		AngleVectors( ang, dir, NULL, NULL );

		dir[2] = 1;

		VectorCopy( self->r.currentOrigin, start );

		part = fire_flamebarrel( temp, start, dir );

		if ( !part ) {
			G_Printf( "ExplodePlaneSndFx Failed to spawn part\n" );
			return;
		}

		part->s.eType = ET_FP_PARTS;

		part->s.modelindex = wing_part;

		return;
	}

	AngleVectors( self->r.currentAngles, dir, NULL, NULL );

	for ( i = 0; i < 4; i++ )
	{
		VectorCopy( self->r.currentOrigin, start );

		start[0] += crandom() * 64;
		start[1] += crandom() * 64;
		start[2] += crandom() * 32;

		part = fire_flamebarrel( temp, start, dir );

		if ( !part ) {
			continue;
		}

		part->s.eType = ET_FP_PARTS;

		if ( i == 0 ) {
			part->s.modelindex = fuse_part;
		} else if ( i == 1 ) {
			part->s.modelindex = wing_part;
		} else if ( i == 2 ) {
			part->s.modelindex = tail_part;
		} else {
			part->s.modelindex = nose_part;
		}
	}
}

void props_me109_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	G_Printf( "dead\n" );

	VectorClear( self->rotate );
	VectorSet( self->rotate, 0, 1, 0 ); //sigh

	if ( self->spawnflags & 8 ) { // FIXED_DIE
		return;
	}

	propExplosionLarge( self );
	self->melee->s.loopSound = self->melee->noise_index = 0;
	ExplodePlaneSndFx( self );
	G_FreeEntity( self );
}

void props_me109_pain( gentity_t *self, gentity_t *attacker, int damage, vec3_t point ) {
	vec3_t temp;

	G_Printf( "pain: health = %i\n", self->health );

	VectorCopy( self->r.currentOrigin, temp );
	VectorCopy( self->pos3, self->r.currentOrigin );
	Spawn_Shard( self, NULL, 6, 999 );
	VectorCopy( temp, self->r.currentOrigin );

	VectorClear( self->rotate );
	VectorSet( self->rotate, 0, 1, 0 ); //sigh
}

void Plane_Fire_Lead( gentity_t *self ) {
	vec3_t dir, right;
	vec3_t pos1, pos2;
	vec3_t position;

	AngleVectors( self->r.currentAngles, dir, right, NULL );
	VectorCopy( self->r.currentOrigin, position );
	VectorMA( position, 64, right, pos1 );
	VectorMA( position, -64, right, pos2 );

	fire_lead( self, pos1, dir, 12 );
	fire_lead( self, pos2, dir, 12 );
}

void Plane_Attack( gentity_t *self, qboolean in_PVS ) {
	if ( self->nextTrain->spawnflags & 16 ) {
		self->count++;

		if ( self->count == 3 ) {
			self->s.density = 8, self->count = 0;

			if ( in_PVS ) {
				G_AddEvent( self, EV_GLOBAL_SOUND, fpattack_snd );
			} else {
				G_AddEvent( self, EV_GENERAL_SOUND, fpattack_snd );
			}

			Plane_Fire_Lead( self );
		} else {
			self->s.density = 7;
		}
	} else if ( self->spawnflags & 4 )     { // spinning prop
		self->s.density = 7;
	} else {
		self->s.density = 0;
	}
}

void props_me109_think( gentity_t *self ) {

	qboolean in_PVS = qfalse;

	{
		gentity_t *player;

		player = AICast_FindEntityForName( "player" );

		if ( player ) {
			in_PVS = trap_InPVS( player->r.currentOrigin, self->s.pos.trBase );

			if ( in_PVS ) {
				self->melee->s.eType = ET_GENERAL;

				{
					float len;
					vec3_t vec;
					vec3_t forward;
					vec3_t dir;
					vec3_t point;

					VectorCopy( player->r.currentOrigin, point );
					VectorSubtract( player->r.currentOrigin, self->r.currentOrigin, vec );
					len = VectorLength( vec );
					vectoangles( vec, dir );
					AngleVectors( dir, forward, NULL, NULL );
					VectorMA( point, len * 0.1, forward, point );

					G_SetOrigin( self->melee, point );
				}
			} else
			{
				self->melee->s.eType = ET_GENERAL;
			}

			trap_LinkEntity( self->melee );
		}
	}

	Plane_Attack( self, in_PVS );

	Calc_Roll( self );

	if ( self->health < 250 ) {
		gentity_t *tent;
		vec3_t point;

		VectorCopy( self->r.currentOrigin, point );
		tent = G_TempEntity( point, EV_SMOKE );
		VectorCopy( point, tent->s.origin );
		tent->s.time = 2000;
		tent->s.time2 = 1000;
		tent->s.density = 4;
		tent->s.angles2[0] = 16;
		tent->s.angles2[1] = 48;
		tent->s.angles2[2] = 10;

		self->props_frame_state = plane_choke;
		self->health--;
	}

	if ( self->health > 0 ) {
		self->nextthink = level.time + 50;

		if ( self->props_frame_state == plane_choke ) {
			self->melee->s.loopSound = self->melee->noise_index = fpchoke_snd;
		} else if ( self->props_frame_state == plane_startup )     {
			self->melee->s.loopSound = self->melee->noise_index = fpstartup_snd;
		} else if ( self->props_frame_state == plane_idle )     {
			self->melee->s.loopSound = self->melee->noise_index = fpidle_snd;
		} else if ( self->props_frame_state == plane_flyby1 )     {
			self->melee->s.loopSound = self->melee->noise_index = fpflyby1_snd;
		} else if ( self->props_frame_state == plane_flyby2 )     {
			self->melee->s.loopSound = self->melee->noise_index = fpflyby2_snd;
		}
	} else
	{
		propExplosionLarge( self );
		self->melee->s.loopSound = self->melee->noise_index = 0;

		ExplodePlaneSndFx( self );
		G_FreeEntity( self->melee );
		G_FreeEntity( self );


	}

}

void Think_SetupAirplaneWaypoints( gentity_t *ent ) {
	gentity_t       *path, *next, *start;

	ent->nextTrain = G_Find( NULL, FOFS( targetname ), ent->target );
	if ( !ent->nextTrain ) {
		G_Printf( "plane at %s with an unfound target\n",
				  vtos( ent->r.absmin ) );
		return;
	}

	start = NULL;
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if ( !start ) {
			start = path;
		}

		if ( !path->target ) {
			G_Printf( "plane at %s without a target\n",
					  vtos( path->s.origin ) );
			return;
		}

		// find a path_corner among the targets
		// there may also be other targets that get fired when the corner
		// is reached
		next = NULL;
		do {
			next = G_Find( next, FOFS( targetname ), path->target );
			if ( !next ) {
				G_Printf( "plane at %s without a target path_corner\n",
						  vtos( path->s.origin ) );
				return;
			}
		} while ( strcmp( next->classname, "plane_waypoint" ) );

		path->nextTrain = next;
	}

	if ( ent->spawnflags & 2 ) { // Toggle
		VectorCopy( ent->nextTrain->s.origin, ent->s.pos.trBase );
		VectorCopy( ent->nextTrain->s.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	} else {
		Reached_Tramcar( ent );
	}
}


void PlaneUse( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t       *next;

	if ( level.time >= ent->s.pos.trTime + ent->s.pos.trDuration ) {

		next = ent->nextTrain;

		if ( next->wait == -1 && next->count ) {
			next->count = 0;
			//G_Printf ("Moving next->count %i\n", next->count);
		}

		Reached_Tramcar( ent );

	}
//	else
//		G_Printf ("no can do havent reached yet\n");

}


void InitPlaneSpeaker( gentity_t *ent ) {
	gentity_t   *snd;

	snd = G_Spawn();

	snd->noise_index = fploop_snd;

	snd->s.eType = ET_SPEAKER;
	snd->s.eventParm = snd->noise_index;
	snd->s.frame = 0;
	snd->s.clientNum = 0;

	snd->s.loopSound = snd->noise_index;

	snd->r.svFlags |= SVF_BROADCAST;

	VectorCopy( ent->s.origin, snd->s.pos.trBase );

	ent->melee = snd;

	trap_LinkEntity( snd );

}

void SP_props_me109( gentity_t *ent ) {

	VectorSet( ent->r.mins, -128, -128, -128 );
	VectorSet( ent->r.maxs, 128, 128, 128 );

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;
	ent->r.svFlags  = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;

	ent->isProp = qtrue;

	ent->s.modelindex = G_ModelIndex( "models/mapobjects/vehicles/m109.md3" );

	if ( !ent->health ) {
		ent->health = 500;
	}

	ent->takedamage = qtrue;

	ent->die = props_me109_die;
	ent->pain = props_me109_pain;

	ent->reached = Reached_Tramcar;

	ent->nextthink = level.time + ( FRAMETIME / 2 );

	ent->think = Think_SetupAirplaneWaypoints;

	ent->use = PlaneUse;

	if ( !( ent->speed ) ) {
		ent->speed = 1000;
	}

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngle( ent, ent->s.angles );

	if ( ent->spawnflags & 4 ) {
		ent->s.density = 7;
	}

	trap_LinkEntity( ent );

	fploop_snd = G_SoundIndex( "sound/fighterplane/fploop.wav" );
	fpchoke_snd = G_SoundIndex( "sound/fighterplane/fpchoke.wav" );
	fpattack_snd = G_SoundIndex( "sound/weapons/mg42/37mm.wav" );
	fpexpdebris_snd = G_SoundIndex( "sound/fighterplane/fpexpdebris.wav" );


	fpflyby1_snd = G_SoundIndex( "sound/fighterplane/fpflyby1.wav" );
	fpflyby2_snd = G_SoundIndex( "sound/fighterplane/fpflyby2.wav" );
	fpidle_snd = G_SoundIndex( "sound/fighterplane/fpidle.wav" );
	fpstartup_snd = G_SoundIndex( "sound/fighterplane/fpstartup.wav" );


	fuse_part = G_ModelIndex( "models/mapobjects/vehicles/m109debris_a.md3" );
	wing_part = G_ModelIndex( "models/mapobjects/vehicles/m109debris_b.md3" );
	tail_part = G_ModelIndex( "models/mapobjects/vehicles/m109debris_c.md3" );
	nose_part = G_ModelIndex( "models/mapobjects/vehicles/m109debris_d.md3" );

	crash_part = G_ModelIndex( "models/mapobjects/vehicles/m109crash.md3" );

	InitPlaneSpeaker( ent );

}

/////////////////////////
// TRUCK DRIVE
/////////////////////////

/*QUAKED truck_cam (.7 .3 .1) ? START_ON TOGGLE - -
*/
void truck_cam_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( player && player != other ) {
		// G_Printf ("other: %s\n", other->aiName);
		return;
	}

	if ( !self->nextTrain ) {
		self->touch = NULL;
		return;
	}

	// lock the player to the moving truck
	{
		vec3_t point;

		trap_UnlinkEntity( other );

		// VectorCopy ( self->r.currentOrigin, other->client->ps.origin );
		VectorCopy( self->r.currentOrigin, point );
		point[2] = other->client->ps.origin[2];
		VectorCopy( point, other->client->ps.origin );

		// save results of pmove
		BG_PlayerStateToEntityState( &other->client->ps, &other->s, qtrue );

		// use the precise origin for linking
		VectorCopy( other->client->ps.origin, other->r.currentOrigin );

		other->client->ps.persistant[PERS_HWEAPON_USE] = 1;

		trap_LinkEntity( other );
	}

}

void truck_cam_think( gentity_t *ent ) {
	ent->nextthink = level.time + ( FRAMETIME / 2 );
}

void SP_truck_cam( gentity_t *self ) {
	int mass;

	VectorClear( self->s.angles );

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		G_Printf( "truck_cam without a target at %s\n", vtos( self->r.absmin ) );
		G_FreeEntity( self );
		return;
	}

	trap_SetBrushModel( self, self->model );

	if ( G_SpawnInt( "mass", "20", &mass ) ) {
		self->count = mass;
	} else {
		self->count = 20;
	}

	InitTramcar( self );

	self->nextthink = level.time + ( FRAMETIME / 2 );

	self->think = Think_SetupTrainTargets;

	self->touch = truck_cam_touch;

	self->s.loopSound = 0;
	self->props_frame_state = 0;

	self->clipmask = CONTENTS_SOLID;

	// G_SetOrigin (self, self->s.origin);
	// G_SetAngle (self, self->s.angles);

	self->reached = Reached_Tramcar;

	self->s.density = 6;

	//start_drive_grind_gears
	truck_sound = G_SoundIndex( "sound/vehicles/start_drive_grind_gears_01_11k.wav" );
	// truck_sound = G_SoundIndex ( "sound/vehicles/tankmove1.wav" );

	truck_idle_snd = G_SoundIndex( "sound/vehicles/truckidle.wav" );
	truck_gear1_snd = G_SoundIndex( "sound/vehicles/truckgear1.wav" );
	truck_gear2_snd = G_SoundIndex( "sound/vehicles/truckgear2.wav" );
	truck_gear3_snd = G_SoundIndex( "sound/vehicles/truckgear3.wav" );
	truck_reverse_snd = G_SoundIndex( "sound/vehicles/truckreverse.wav" );
	truck_moving_snd = G_SoundIndex( "sound/vehicles/truckmoving.wav" );
	truck_breaking_snd = G_SoundIndex( "sound/vehicles/truckbreaking.wav" );
	truck_bouncy1_snd = G_SoundIndex( "sound/vehicles/truckbouncy1.wav" );
	truck_bouncy2_snd = G_SoundIndex( "sound/vehicles/truckbouncy2.wav" );
	truck_bouncy3_snd = G_SoundIndex( "sound/vehicles/truckbouncy3.wav" );
}

/////////////////////////
// camera cam
/////////////////////////

/*QUAKED camera_cam (.5 .7 .3) (-8 -8 -8) (8 8 8) ON TRACKING MOVING -
"track" is the targetname of the entity providing the starting direction use an info_notnull
*/

void delayOnthink( gentity_t *ent ) {
	if ( ent->melee ) {
		ent->melee->use( ent->melee, NULL, NULL );
	}
}

void Init_Camera( gentity_t *ent ) {
	vec3_t move;
	float distance;

	ent->moverState = MOVER_POS1;
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;

	VectorCopy( ent->pos1, ent->r.currentOrigin );

	trap_LinkEntity( ent );

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->s.pos.trBase );

	// calculate time to reach second position from speed
	VectorSubtract( ent->pos2, ent->pos1, move );
	distance = VectorLength( move );
	if ( !ent->speed ) {
		ent->speed = 100;
	}
	VectorScale( move, ent->speed, ent->s.pos.trDelta );
	ent->s.pos.trDuration = distance * 1000 / ent->speed;
	if ( ent->s.pos.trDuration <= 0 ) {
		ent->s.pos.trDuration = 1;
	}
}

void camera_cam_think( gentity_t *ent ) {
	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( !player ) {
		return;
	}

	if ( ent->spawnflags & 2 ) { // tracking
		vec3_t point;

		trap_UnlinkEntity( player );

		// VectorCopy ( self->r.currentOrigin, other->client->ps.origin );
		VectorCopy( ent->r.currentOrigin, point );
		point[2] = player->client->ps.origin[2];
		VectorCopy( point, player->client->ps.origin );

		// save results of pmove
		BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );

		// use the precise origin for linking
		VectorCopy( player->client->ps.origin, player->r.currentOrigin );

		// tracking
		{
			gentity_t   *target = NULL;
			vec3_t dang;
			vec3_t vec;

			if ( ent->track ) {
				target = G_Find( NULL, FOFS( targetname ), ent->track );
			}

			if ( target ) {
				VectorSubtract( target->r.currentOrigin, ent->r.currentOrigin, vec );
				vectoangles( vec, dang );
				SetClientViewAngle( player, dang );

				VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
				VectorCopy( dang, ent->s.apos.trBase );

				trap_LinkEntity( ent );
			}
		}

		trap_LinkEntity( player );
	}

	ent->nextthink = level.time + ( FRAMETIME / 2 );
}

void camera_cam_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( !player ) {
		return;
	}

	if ( !( ent->spawnflags & 1 ) ) {
		ent->think = camera_cam_think;
		ent->nextthink = level.time + ( FRAMETIME / 2 );
		ent->spawnflags |= 1;
		{
			player->client->ps.persistant[PERS_HWEAPON_USE] = 1;
			player->client->ps.viewlocked = 4;
			player->client->ps.viewlocked_entNum = ent->s.number;
		}
	} else
	{
		ent->spawnflags &= ~1;
		ent->think = NULL;
		{
			player->client->ps.persistant[PERS_HWEAPON_USE] = 0;
			player->client->ps.viewlocked = 0;
			player->client->ps.viewlocked_entNum = 0;
		}
	}

}

void camera_cam_firstthink( gentity_t *ent ) {
	gentity_t   *target = NULL;
	vec3_t dang;
	vec3_t vec;

	if ( ent->track ) {
		target = G_Find( NULL, FOFS( targetname ), ent->track );
	}

	if ( target ) {
		VectorSubtract( target->s.origin, ent->r.currentOrigin, vec );
		vectoangles( vec, dang );
		G_SetAngle( ent, dang );
	}

	if ( ent->target ) {
		ent->nextthink = level.time + ( FRAMETIME / 2 );
		ent->think = Think_SetupTrainTargets;
	}
}

void SP_camera_cam( gentity_t *ent ) {
	Init_Camera( ent );

	ent->r.svFlags  = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;

	G_SetOrigin( ent, ent->s.origin );
	G_SetAngle( ent, ent->s.angles );

	ent->reached = Reached_Tramcar;

	ent->nextthink = level.time + ( FRAMETIME / 2 );

	ent->think = camera_cam_firstthink;

	ent->use = camera_cam_use;

	if ( ent->spawnflags & 1 ) { // On
		gentity_t *delayOn;

		delayOn = G_Spawn();
		delayOn->think = delayOnthink;
		delayOn->nextthink = level.time + 1000;
		delayOn->melee = ent;
		trap_LinkEntity( delayOn );
	}

}


/*QUAKED screen_fade (.3 .7 .9) (-8 -8 -8) (8 8 8)
"wait" duration of fade out
"delay" duration of fade in

  1 = 1 sec
  .5 = .5 sec

defaults are .5 sec
*/
void screen_fade_use( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->spawnflags & 1 ) {
		// fade out
		trap_SetConfigstring( CS_SCREENFADE, va( "1 %i %i", level.time + 100, (int) ent->wait ) );
		ent->spawnflags &= ~1;
	} else
	{
		// fade in
		trap_SetConfigstring( CS_SCREENFADE, va( "0 %i %i", level.time + 100, (int) ent->delay ) );
		ent->spawnflags |= 1;
	}

}

void SP_screen_fade( gentity_t *ent ) {
	ent->use = screen_fade_use;

	if ( !ent->wait ) {
		ent->wait = 500;
	}
	if ( !ent->delay ) {
		ent->delay = 500;
	}

}

/*QUAKED camera_reset_player (.5 .7 .3) ?
touched will record the players position and fire off its targets and or cameras

used will reset the player back to his last position
*/

void mark_players_pos( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	gentity_t   *player;

	player = AICast_FindEntityForName( "player" );

	if ( player == other ) {
		VectorCopy( player->r.currentOrigin, ent->s.origin2 );
		VectorCopy( player->r.currentAngles, ent->s.angles2 );

		G_UseTargets( ent, NULL );
	}

}

void reset_players_pos( gentity_t *ent, gentity_t *other, gentity_t *activator ) {

	gentity_t *player;

	player = AICast_FindEntityForName( "player" );

	if ( !player ) {
		return;
	}

	trap_UnlinkEntity( player );

	VectorCopy( ent->s.origin2, player->client->ps.origin );

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->r.currentOrigin );

	SetClientViewAngle( player, ent->s.angles2 );

	player->client->ps.persistant[PERS_HWEAPON_USE] = 0;
	player->client->ps.viewlocked = 0;
	player->client->ps.viewlocked_entNum = 0;

	trap_LinkEntity( player );

}

extern void InitTrigger( gentity_t *self );

void SP_camera_reset_player( gentity_t *ent ) {
	InitTrigger( ent );

	ent->r.contents = CONTENTS_TRIGGER;

	ent->touch = mark_players_pos;
	ent->use = reset_players_pos;

	trap_LinkEntity( ent );
}
