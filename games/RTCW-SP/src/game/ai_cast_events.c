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

//===========================================================================
//
// Name:			ai_cast_events.c
// Function:		Wolfenstein AI Character Events
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "../game/g_local.h"
#include "../game/q_shared.h"
#include "../game/botlib.h"      //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../botai/botai.h"          //bot ai interface

#include "ai_cast.h"

/*
Contains response functions for various events that require specific handling
for Cast AI's.
*/

/*
============
AICast_Sight
============
*/
void AICast_Sight( gentity_t *ent, gentity_t *other, int lastSight ) {
	cast_state_t    *cs, *ocs;

	cs = AICast_GetCastState( ent->s.number );
	ocs = AICast_GetCastState( other->s.number );

	//
	// call the sightfunc for this cast, so we can play associated sounds, or do any character-specific things
	//
	if ( cs->sightfunc ) {
		// factor in the reaction time
		if ( AICast_EntityVisible( cs, other->s.number, qfalse ) ) {
			cs->sightfunc( ent, other, lastSight );
		}
	}

	if ( other->aiName && other->health <= 0 ) {

		// they died since we last saw them
		if ( ocs->deathTime > lastSight ) {
			if ( !AICast_SameTeam( cs, other->s.number ) ) {
				AICast_ScriptEvent( cs, "enemysightcorpse", other->aiName );
			} else if ( !( cs->castScriptStatus.scriptFlags & SFL_FRIENDLYSIGHTCORPSE_TRIGGERED ) ) {
				cs->castScriptStatus.scriptFlags |= SFL_FRIENDLYSIGHTCORPSE_TRIGGERED;
				AICast_ScriptEvent( cs, "friendlysightcorpse", "" );
			}
		}

		// if this is the first time, call the sight script event
	} else if ( !lastSight && other->aiName ) {
		if ( !AICast_SameTeam( cs, other->s.number ) ) {
			// disabled.. triggered when entering combat mode
			//AICast_ScriptEvent( cs, "enemysight", other->aiName );
		} else {
			AICast_ScriptEvent( cs, "sight", other->aiName );
		}
	}
}

/*
============
AICast_Pain
============
*/
void AICast_Pain( gentity_t *targ, gentity_t *attacker, int damage, vec3_t point ) {
	cast_state_t    *cs;

	cs = AICast_GetCastState( targ->s.number );

	// print debugging message
	if ( aicast_debug.integer == 2 && attacker->s.number == 0 ) {
		G_Printf( "hit %s %i\n", targ->aiName, targ->health );
	}

	// if we are below alert mode, then go there immediately
	if ( cs->aiState < AISTATE_ALERT ) {
		AICast_StateChange( cs, AISTATE_ALERT );
	}

	if ( cs->aiFlags & AIFL_NOPAIN ) {
		return;
	}

	// process the event (turn to face the attacking direction? go into hide/retreat state?)
	// need to weigh up the situation, but foremost, an inactive AI cast should always react in some way to being hurt
	cs->lastPain = level.time;

	// record the sighting (FIXME: silent weapons shouldn't do this, but the AI should react in some way)
	if ( attacker->client ) {
		AICast_UpdateVisibility( targ, attacker, qtrue, qtrue );
	}

	// if either of us are neutral, then we are now enemies
	if ( targ->aiTeam == AITEAM_NEUTRAL || attacker->aiTeam == AITEAM_NEUTRAL ) {
		cs->vislist[attacker->s.number].flags |= AIVIS_ENEMY;
	}

	AICast_ScriptEvent( cs, "painenemy", attacker->aiName );

	AICast_ScriptEvent( cs, "pain", va( "%d %d", targ->health, targ->health + damage ) );

	if ( cs->aiFlags & AIFL_DENYACTION ) {
		// dont play any sounds
		return;
	}

	//
	// call the painfunc for this cast, so we can play associated sounds, or do any character-specific things
	//
	if ( cs->painfunc ) {
		cs->painfunc( targ, attacker, damage, point );
	}
}

/*
============
AICast_Die
============
*/
void AICast_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	int contents;
	int killer;
	cast_state_t    *cs;
	qboolean nogib = qtrue;
	char mapname[MAX_QPATH];

	// print debugging message
	if ( aicast_debug.integer == 2 && attacker->s.number == 0 ) {
		G_Printf( "killed %s\n", self->aiName );
	}

	cs = AICast_GetCastState( self->s.number );

	if ( attacker ) {
		killer = attacker->s.number;
	} else {
		killer = ENTITYNUM_WORLD;
	}

	// record the sighting (FIXME: silent weapons shouldn't do this, but the AI should react in some way)
	if ( attacker->client ) {
		AICast_UpdateVisibility( self, attacker, qtrue, qtrue );
	}

	if ( self->aiCharacter == AICHAR_HEINRICH || self->aiCharacter == AICHAR_HELGA || self->aiCharacter == AICHAR_SUPERSOLDIER || self->aiCharacter == AICHAR_PROTOSOLDIER ) {
		if ( self->health <= GIB_HEALTH ) {
			self->health = -1;
		}
	}

	// the zombie should show special effect instead of gibbing
	if ( self->aiCharacter == AICHAR_ZOMBIE && cs->secondDeadTime ) {
		if ( cs->secondDeadTime > 1 ) {
			// we are already totally dead
			self->health += damage; // don't drop below gib_health if we weren't already below it
			return;
		}
/*
		if (!cs->rebirthTime)
		{
			self->health = -999;
			damage = 999;
		} else if ( self->health >= GIB_HEALTH ) {
			// while waiting for rebirth, we only "die" if we drop below gib health
			return;
		}
*/
		// always gib
		self->health = -999;
		damage = 999;
	}

	// Zombies are very fragile against highly explosives
	if ( self->aiCharacter == AICHAR_ZOMBIE && damage > 20 && inflictor != attacker ) {
		self->health = -999;
		damage = 999;
	}

	// process the event
	if ( self->client->ps.pm_type == PM_DEAD ) {
		// already dead
		if ( self->health < GIB_HEALTH ) {
			if ( self->aiCharacter == AICHAR_ZOMBIE ) {
				// RF, changed this so Zombies always gib now
				GibEntity( self, killer );
				nogib = qfalse;
/*
				// Zombie has special exploding cloud effect
				if (attacker != inflictor || attacker->s.weapon == WP_VENOM)
				{
					GibEntity( self, killer );
					nogib = qfalse;
				} else {
					// Zombie will decompose upon dying
					self->client->ps.eFlags |= EF_MONSTER_EFFECT2;
					self->s.effect2Time = level.time+200;
					self->health = -1;
				}
*/
				self->takedamage = qfalse;
				self->r.contents = 0;
				cs->secondDeadTime = 2;
				cs->rebirthTime = 0;
				cs->revivingTime = 0;
			} else {
				body_die( self, inflictor, attacker, damage, meansOfDeath );
				return;
			}
		}

	} else {    // this is our first death, so set everything up

		if ( level.intermissiontime ) {
			return;
		}

		self->client->ps.pm_type = PM_DEAD;

		self->enemy = attacker;

		// drop a weapon?
		// if client is in a nodrop area, don't drop anything
		contents = trap_PointContents( self->r.currentOrigin, -1 );
		if ( !( contents & CONTENTS_NODROP ) ) {
			TossClientItems( self );
		}

		// make sure the client doesn't forget about this entity until it's set to "dead" frame
		// otherwise it might replay it's death animation if it goes out and into client view
		self->r.svFlags |= SVF_BROADCAST;

		self->takedamage = qtrue;   // can still be gibbed

		self->s.weapon = WP_NONE;
		if ( cs->bs ) {
			cs->weaponNum = WP_NONE;
		}
		self->client->ps.weapon = WP_NONE;

		self->s.powerups = 0;
		self->r.contents = CONTENTS_CORPSE;

		self->s.angles[0] = 0;
		self->s.angles[1] = self->client->ps.viewangles[1];
		self->s.angles[2] = 0;

		VectorCopy( self->s.angles, self->client->ps.viewangles );

		self->s.loopSound = 0;

		self->r.maxs[2] = -8;
		self->client->ps.maxs[2] = self->r.maxs[2];

		// remove powerups
		memset( self->client->ps.powerups, 0, sizeof( self->client->ps.powerups ) );

		//cs->rebirthTime = 0;

		// never gib in a nodrop
		if ( self->health <= GIB_HEALTH ) {
			if ( self->aiCharacter == AICHAR_ZOMBIE ) {
				// RF, changed this so Zombies always gib now
				GibEntity( self, killer );
				nogib = qfalse;
			} else if ( !( contents & CONTENTS_NODROP ) ) {
				body_die( self, inflictor, attacker, damage, meansOfDeath );
				//GibEntity( self, killer );
				nogib = qfalse;
			}
		}

		// if we are a zombie, and lying down during our first death, then we should just die
		if ( !( self->aiCharacter == AICHAR_ZOMBIE && cs->secondDeadTime && cs->rebirthTime ) ) {

			// set enemy weapon
			BG_UpdateConditionValue( self->s.number, ANIM_COND_ENEMY_WEAPON, 0, qfalse );
			if ( attacker->client ) {
				BG_UpdateConditionValue( self->s.number, ANIM_COND_ENEMY_WEAPON, inflictor->s.weapon, qtrue );
			} else {
				BG_UpdateConditionValue( self->s.number, ANIM_COND_ENEMY_WEAPON, 0, qfalse );
			}

			// set enemy location
			BG_UpdateConditionValue( self->s.number, ANIM_COND_ENEMY_POSITION, 0, qfalse );
			if ( infront( self, inflictor ) ) {
				BG_UpdateConditionValue( self->s.number, ANIM_COND_ENEMY_POSITION, POSITION_INFRONT, qtrue );
			} else {
				BG_UpdateConditionValue( self->s.number, ANIM_COND_ENEMY_POSITION, POSITION_BEHIND, qtrue );
			}

			if ( self->takedamage ) { // only play the anim if we haven't gibbed
				// play the animation
				BG_AnimScriptEvent( &self->client->ps, ANIM_ET_DEATH, qfalse, qtrue );
			}

			// set gib delay
			if ( cs->aiCharacter == AICHAR_HEINRICH || cs->aiCharacter == AICHAR_HELGA ) {
				cs->lastLoadTime = level.time + self->client->ps.torsoTimer - 200;
			}

			// set this flag so no other anims override us
			self->client->ps.eFlags |= EF_DEAD;
			self->s.eFlags |= EF_DEAD;

			// make sure we dont move around while on the ground
			//self->flags |= FL_NO_HEADCHECK;

		}

		// if end map, sink into ground
		cs->deadSinkStartTime = 0;
		if ( cs->aiCharacter == AICHAR_WARZOMBIE ) {
			trap_Cvar_VariableStringBuffer( "mapname", mapname, sizeof( mapname ) );
			if ( !Q_strncmp( mapname, "end", 3 ) ) {    // !! FIXME: post beta2, make this a spawnflag!
				cs->deadSinkStartTime = level.time + 4000;
			}
		}
	}

	if ( nogib ) {
		// set for rebirth
		if ( self->aiCharacter == AICHAR_ZOMBIE ) {
			if ( !cs->secondDeadTime ) {
				cs->rebirthTime = level.time + 5000 + rand() % 2000;
				// RF, only set for gib at next death, if NoRevive is not set
				if ( !( self->spawnflags & 2 ) ) {
					cs->secondDeadTime = qtrue;
				}
				cs->revivingTime = 0;
			} else if ( cs->secondDeadTime > 1 ) {
				cs->rebirthTime = 0;
				cs->revivingTime = 0;
				cs->deathTime = level.time;
			}
		} else {
			// the body can still be gibbed
			self->die = body_die;
		}
	}

	trap_LinkEntity( self );

	// kill, instanly, any streaming sound the character had going
	G_AddEvent( &g_entities[self->s.number], EV_STOPSTREAMINGSOUND, 0 );

	// mark the time of death
	cs->deathTime = level.time;

	// dying ai's can trigger a target
	if ( !cs->rebirthTime ) {
		G_UseTargets( self, self );
		// really dead now, so call the script
		AICast_ScriptEvent( cs, "death", attacker->aiName ? attacker->aiName : "" );
		// call the deathfunc for this cast, so we can play associated sounds, or do any character-specific things
		if ( !( cs->aiFlags & AIFL_DENYACTION ) && cs->deathfunc ) {
			cs->deathfunc( self, attacker, damage, meansOfDeath );   //----(SA)	added mod
		}
	} else {
		// really dead now, so call the script
		AICast_ScriptEvent( cs, "fakedeath", "" );
		// call the deathfunc for this cast, so we can play associated sounds, or do any character-specific things
		if ( !( cs->aiFlags & AIFL_DENYACTION ) && cs->deathfunc ) {
			cs->deathfunc( self, attacker, damage, meansOfDeath );   //----(SA)	added mod
		}
	}
}

/*
===============
AICast_EndChase
===============
*/
void AICast_EndChase( cast_state_t *cs ) {
	// anything?
}

/*
===============
AICast_AIDoor_Touch
===============
*/
void AICast_AIDoor_Touch( gentity_t *ent, gentity_t *aidoor_trigger, gentity_t *door ) {
	cast_state_t *cs, *ocs;
	gentity_t *trav;
	int i;
	trace_t tr;
	vec3_t mins, pos, dir;

	cs = AICast_GetCastState( ent->s.number );

	if ( !cs->bs ) {
		return;
	}

	// does the aidoor have ai_marker's?
	if ( !aidoor_trigger->targetname ) {
		G_Printf( "trigger_aidoor has no ai_marker's at %s\n", vtos( ent->r.currentOrigin ) );
		return;
	}

	// are we heading for an ai_marker?
	if ( cs->aifunc == AIFunc_DoorMarker ) {
		return;
	}

	// if they are moving away from the door, ignore them
	if ( VectorLength( cs->bs->velocity ) > 1 ) {
		VectorAdd( door->r.absmin, door->r.absmax, pos );
		VectorScale( pos, 0.5, pos );
		VectorSubtract( pos, cs->bs->origin, dir );
		if ( DotProduct( cs->bs->velocity, dir ) < 0 ) {
			return;
		}
	}

	// TTimo: gcc: suggest () around assignment used as truth value
	for ( trav = NULL; ( trav = G_Find( trav, FOFS( target ), aidoor_trigger->targetname ) ); ) {
		// make sure the marker is vacant
		trap_Trace( &tr, trav->r.currentOrigin, ent->r.mins, ent->r.maxs, trav->r.currentOrigin, ent->s.number, ent->clipmask );
		if ( tr.startsolid ) {
			continue;
		}
		// search all other AI's, to see if they are heading for this marker
		for ( i = 0, ocs = AICast_GetCastState( 0 ); i < aicast_maxclients; i++, ocs++ ) {
			if ( !ocs->bs ) {
				continue;
			}
			if ( ocs->aifunc != AIFunc_DoorMarker ) {
				continue;
			}
			if ( ocs->doorMarker != trav->s.number ) {
				continue;
			}
			// found a match
			break;
		}
		if ( i < aicast_maxclients ) {
			continue;
		}
		// make sure there is a clear path
		VectorCopy( ent->r.mins, mins );
		mins[2] += 16;  // step height
		trap_Trace( &tr, ent->r.currentOrigin, mins, ent->r.maxs, trav->r.currentOrigin, ent->s.number, ent->clipmask );
		if ( tr.fraction < 1.0 ) {
			continue;
		}
		// the marker is vacant and available
		cs->doorMarkerTime = level.time;
		cs->doorMarkerNum = trav->s.number;
		cs->doorMarkerDoor = door->s.number;
		break;
	}
}

/*
============
AICast_ProcessActivate
============
*/
void AICast_ProcessActivate( int entNum, int activatorNum ) {
	cast_state_t *cs;
	gentity_t *newent, *ent, *activator;
	gclient_t *client;

	cs = AICast_GetCastState( entNum );
	client = &level.clients[entNum];
	ent = &g_entities[entNum];
	activator = &g_entities[activatorNum];

	if ( cs->lastActivate > level.time - 1000 ) {
		return;
	}
	cs->lastActivate = level.time;

	if ( !AICast_SameTeam( cs, activatorNum ) ) {

		if ( ent->aiTeam == AITEAM_NEUTRAL ) {
			AICast_ScriptEvent( cs, "activate", g_entities[activatorNum].aiName );
		}

		return;
	}

	// try running the activate event, if it denies us the request, then abort
	cs->aiFlags &= ~AIFL_DENYACTION;
	AICast_ScriptEvent( cs, "activate", g_entities[activatorNum].aiName );
	if ( cs->aiFlags & AIFL_DENYACTION ) {
		return;
	}

	// if we are doing something else
	if ( cs->castScriptStatus.castScriptEventIndex >= 0 ) {
		if ( ent->eventTime != level.time ) {
			G_AddEvent( &g_entities[entNum], EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[cs->aiCharacter].soundScripts[ORDERSDENYSOUNDSCRIPT] ) );
		}
		return;
	}

	// if we are already following them, stop following
	if ( cs->leaderNum == activatorNum ) {
		if ( ent->eventTime != level.time ) {
			G_AddEvent( &g_entities[entNum], EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[cs->aiCharacter].soundScripts[STAYSOUNDSCRIPT] ) );
		}

		cs->leaderNum = -1;

		// create a goal at this position
		newent = G_Spawn();
		newent->classname = "AI_wait_goal";
		newent->r.ownerNum = entNum;
		G_SetOrigin( newent, cs->bs->origin );
		AIFunc_ChaseGoalStart( cs, newent->s.number, 128, qtrue );

		//AIFunc_IdleStart( cs );
	} else {    // start following
		int count, i;
		cast_state_t *tcs;

		// if they already have enough followers, deny
		for ( count = 0, i = 0, tcs = caststates; i < level.maxclients; i++, tcs++ ) {
			if ( tcs->bs && tcs != cs && tcs->entityNum != activatorNum && g_entities[tcs->entityNum].health > 0 && tcs->leaderNum == activatorNum ) {
				count++;
			}
		}
		if ( count >= 3 ) {
			if ( ent->eventTime != level.time ) {
				G_AddEvent( &g_entities[entNum], EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[cs->aiCharacter].soundScripts[ORDERSDENYSOUNDSCRIPT] ) );
			}
			return;
		}

		if ( ent->eventTime != level.time ) {
			G_AddEvent( &g_entities[entNum], EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[cs->aiCharacter].soundScripts[FOLLOWSOUNDSCRIPT] ) );
		}

		// if they have a wait goal, free it
		if ( cs->followEntity >= MAX_CLIENTS && g_entities[cs->followEntity].classname && !strcmp( g_entities[cs->followEntity].classname, "AI_wait_goal" ) ) {
			G_FreeEntity( &g_entities[cs->followEntity] );
		}

		cs->followEntity = -1;
		cs->leaderNum = activatorNum;
	}
}

/*
================
AICast_RecordScriptSound
================
*/
void AICast_RecordScriptSound( int client ) {
	cast_state_t *cs;

	cs = AICast_GetCastState( client );
	cs->lastScriptSound = level.time;
}
