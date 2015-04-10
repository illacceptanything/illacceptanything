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

#include "ai_cast_fight.h"   // need these for avoidance


extern void G_CheckForCursorHints( gentity_t *ent );



/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t   *client;
	float count;
	vec3_t angles;

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;     // didn't take any damage
	}

	if ( count > 127 ) {
		count = 127;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH] / 360.0 * 256;
		client->ps.damageYaw = angles[YAW] / 360.0 * 256;
	}

	// play an apropriate pain sound
	if ( ( level.time > player->pain_debounce_time ) && !( player->flags & FL_GODMODE ) && !( player->r.svFlags & SVF_CASTAI ) ) {
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, player->health );
	}

	client->ps.damageEvent++;   // Ridah, always increment this since we do multiple view damage anims


	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
	qboolean envirosuit;
	int waterlevel;

	if ( ent->client->noclip ) {
		ent->client->airOutTime = level.time + 12000;   // don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > level.time;

//	G_Printf("breathe: %d   invuln: %d   nofatigue: %d\n", ent->client->ps.powerups[PW_BREATHER], level.time - ent->client->ps.powerups[PW_INVULNERABLE], ent->client->ps.powerups[PW_NOFATIGUE]);

	//
	// check for drowning
	//
	if ( waterlevel == 3 ) {
		// envirosuit give air
		if ( envirosuit ) {
			ent->client->airOutTime = level.time + 10000;
		}

		//----(SA)	both these will end up being by virtue of having the 'breather' powerup
		if ( ent->client->ps.aiChar == AICHAR_FROGMAN ) {  // let frogmen breathe forever
			ent->client->airOutTime = level.time + 10000;
		}

		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time ) {

			if ( 0 ) {
//			if(ent->client->ps.powerups[PW_BREATHER]) {	// take air from the breather now that we need it
//				ent->client->ps.powerups[PW_BREATHER] -= (level.time - ent->client->airOutTime);
//				ent->client->airOutTime = level.time + (level.time - ent->client->airOutTime);
			} else {


				// drown!
				ent->client->airOutTime += 1000;
				if ( ent->health > 0 ) {
					// take more damage the longer underwater
					ent->damage += 2;
					if ( ent->damage > 15 ) {
						ent->damage = 15;
					}

					// play a gurp sound instead of a normal pain sound
					if ( ent->health <= ent->damage ) {
						G_Sound( ent, G_SoundIndex( "*drown.wav" ) );
					} else if ( rand() & 1 ) {
						G_Sound( ent, G_SoundIndex( "sound/player/gurp1.wav" ) );
					} else {
						G_Sound( ent, G_SoundIndex( "sound/player/gurp2.wav" ) );
					}

					// don't play a normal pain sound
					ent->pain_debounce_time = level.time + 200;

					G_Damage( ent, NULL, NULL, NULL, NULL,
							  ent->damage, DAMAGE_NO_ARMOR, MOD_WATER );
				}
			}
		}
	} else {
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if ( waterlevel &&
		 ( ent->watertype & CONTENTS_LAVA ) ) { //----(SA)	modified since slime is no longer deadly
//		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if ( ent->health > 0
			 && ent->pain_debounce_time <= level.time    ) {

			if ( envirosuit ) {
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			} else {
				if ( ent->watertype & CONTENTS_LAVA ) {
					G_Damage( ent, NULL, NULL, NULL, NULL,
							  30 * waterlevel, 0, MOD_LAVA );
					// play the lava sizzle
					G_Sound( ent, G_SoundIndex( "sound/world/hurt_me.wav" ) );   // sound gets cached by target_speaker... last minute hack.
				}

//	(SA)		if (ent->watertype & CONTENTS_SLIME) {
//					G_Damage (ent, NULL, NULL, NULL, NULL,
//						10*waterlevel, 0, MOD_SLIME);
//	end			}
			}
		}
	}

	//
	// check for burning from flamethrower
	//
	if ( ent->s.onFireEnd > level.time && ( AICast_AllowFlameDamage( ent->s.number ) ) ) {
		gentity_t *attacker;

		if ( ent->health > 0 ) {
			attacker = g_entities + ent->flameBurnEnt;
			if ( g_gametype.integer == GT_SINGLE_PLAYER ) { // JPW NERVE
				if ( ent->r.svFlags & SVF_CASTAI ) {
					G_Damage( ent, attacker, attacker, NULL, NULL, 2, DAMAGE_NO_KNOCKBACK, MOD_FLAMETHROWER );
				} else if ( ( ent->s.onFireEnd - level.time ) > FIRE_FLASH_TIME / 2 && rand() % 5000 < ( ent->s.onFireEnd - level.time ) ) { // as it fades out, also fade out damage rate
					G_Damage( ent, attacker, attacker, NULL, NULL, 1, DAMAGE_NO_KNOCKBACK, MOD_FLAMETHROWER );
				}
			} // JPW NERVE
			else { // JPW NERVE multiplayer flamethrower
				if ( level.time < ent->s.onFireEnd ) { // flamethrower does total 80 pts damage in multiplayer
					// (it's short range and hard to hit with)
					G_Damage( ent, attacker, attacker, NULL, NULL, 2, DAMAGE_NO_KNOCKBACK, MOD_FLAMETHROWER );
				}
			}
			// jpw
		} else if ( ent->s.onFireEnd > level.time + 4000 ) {  // dead, so sto pthe flames soon
			ent->s.onFireEnd = level.time + 4000;   // stop burning soon
		}
	}
}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
	if ( ent->aiCharacter ) {
		return;
	}

	if ( ent->waterlevel && ( ent->watertype & CONTENTS_LAVA ) ) { //----(SA)	modified since slime is no longer deadly
//	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) )
		ent->s.loopSound = level.snd_fry;
	} else {
		ent->s.loopSound = 0;
	}
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int i, j;
	trace_t trace;
	gentity_t   *other;

	memset( &trace, 0, sizeof( trace ) );
	for ( i = 0 ; i < pm->numtouch ; i++ ) {
		for ( j = 0 ; j < i ; j++ ) {
			if ( pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if ( j != i ) {
			continue;   // duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void    G_TouchTriggers( gentity_t *ent ) {
	int i, num;
	int touch[MAX_GENTITIES];
	gentity_t   *hit;
	trace_t trace;
	vec3_t mins, maxs;
	static vec3_t range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i = 0 ; i < num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER ) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else {
			// MrE: always use capsule for player
			if ( !trap_EntityContactCapsule( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof( trace ) );

		if ( hit->touch ) {
			hit->touch( hit, ent, &trace );
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t pm;
	gclient_t   *client;

	client = ent->client;

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = 400; // faster than normal
		if ( client->ps.sprintExertTime ) {
			client->ps.speed *= 3;  // (SA) allow sprint in free-cam mode


		}
		// set up for pmove
		memset( &pm, 0, sizeof( pm ) );
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;   // spectators can fly through bodies
		pm.trace = trap_Trace;
		pm.pointcontents = trap_PointContents;

		Pmove( &pm ); // JPW NERVE

		// Rafael - Activate
		// Ridah, made it a latched event (occurs on keydown only)
		if ( client->latched_buttons & BUTTON_ACTIVATE ) {
			Cmd_Activate_f( ent );
		}

		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	}

	if ( ent->flags & FL_NOFATIGUE ) {
		ent->client->ps.sprintTime = 20000;
	}


	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

//----(SA)	added
	client->oldwbuttons = client->wbuttons;
	client->wbuttons = ucmd->wbuttons;

	// attack button cycles through spectators
	if ( ( client->buttons & BUTTON_ATTACK ) && !( client->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent, 1 );
	}
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( !g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if ( client->pers.cmd.forwardmove ||
				client->pers.cmd.rightmove ||
				client->pers.cmd.upmove ||
				( client->pers.cmd.wbuttons & WBUTTON_ATTACK2 ) ||
				( client->pers.cmd.buttons & BUTTON_ATTACK ) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->pers.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap_DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			trap_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t *client;

	client = ent->client;
	client->timeResidual += msec;

	while ( client->timeResidual >= 1000 ) {
		client->timeResidual -= 1000;

		// regenerate
// JPW NERVE, split these completely
		if ( g_gametype.integer != GT_WOLF ) {
			if ( client->ps.powerups[PW_REGEN] ) {
				if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] ) {
					ent->health += 15;
					if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.1 ) {
						ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.1;
					}
					G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
				} else if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] * 2 ) {
					ent->health += 2;
					if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 2 ) {
						ent->health = client->ps.stats[STAT_MAX_HEALTH] * 2;
					}
					G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
				}
			} else {
				// count down health when over max
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
					ent->health--;
				}
			}
		}
// JPW NERVE
		else { // GT_WOLF
			if ( client->ps.powerups[PW_REGEN] ) {
				if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] ) {
					ent->health += 3;
					if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.1 ) {
						ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.1;
					}
				} else if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] * 1.12 ) {
					ent->health += 2;
					if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.12 ) {
						ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.12;
					}
				}
			} else {
				// count down health when over max
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
					ent->health--;
				}
			}
		}
// jpw
		// count down armor when over max
		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH] ) {
			client->ps.stats[STAT_ARMOR]--;
		}
	}
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;

//----(SA)	added
	client->oldwbuttons = client->wbuttons;
	client->wbuttons = client->pers.cmd.wbuttons;

	if ( ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) ||
		 ( client->wbuttons & WBUTTON_ATTACK2 & ( client->oldwbuttons ^ client->wbuttons ) ) ) {
		client->readyToExit ^= 1;
	}
}


/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int i;
	int event, eventParm;
	gclient_t   *client;
	int damage;
	vec3_t dir;
	int stunTime;           //----(SA)	added
	float fallSoundMul;
//	vec3_t		origin, angles;
//	qboolean	fired;
//	gitem_t		*item;
//	gentity_t	*drop;

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {

		event = client->ps.events[ i & ( MAX_EVENTS - 1 ) ];
		eventParm = client->ps.eventParms[ i & ( MAX_EVENTS - 1 ) ];

		switch ( event ) {
		case EV_FALL_NDIE:
		case EV_FALL_DMG_10:
		case EV_FALL_DMG_15:
		case EV_FALL_DMG_25:
		case EV_FALL_DMG_50:

			if ( ent->s.eType != ET_PLAYER ) {
				break;      // not in the player model
			}

			if ( g_dmflags.integer & DF_NO_FALLING ) {
				break;
			}

			stunTime = 0;   //----(SA)	added
			fallSoundMul = 1.0f;    // default to normal range

//----(SA)	FIXME: TODO:  hmm, going through here adding surfaceparms it seems that the value for ent->client->ps.pm_time was weird.  (1000 for all but dmg_25 which has 250?)
			if ( event == EV_FALL_NDIE ) {
				damage = 9999;
			} else if ( event == EV_FALL_DMG_50 ) {
				damage = 50;
				stunTime = 1000;
			} else if ( event == EV_FALL_DMG_25 ) {
				damage = 25;
				stunTime = 250;
			} else if ( event == EV_FALL_DMG_15 ) {
				damage = 15;
				stunTime = 1000;
			} else if ( event == EV_FALL_DMG_10 ) {
				damage = 10;
				stunTime = 1000;
			} else {
				damage = 5; // never used
			}

			fallSoundMul *= 2;  // double range for falls greater than FALL_SHORT

			if ( eventParm & SURF_SLICK ) { // don't stun when you drop onto slick
				stunTime = 0;
			}

			if ( stunTime ) {
				ent->client->ps.pm_time = stunTime;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
				VectorClear( ent->client->ps.velocity );
			}

			VectorSet( dir, 0, 0, 1 );
			ent->pain_debounce_time = level.time + 200; // no normal pain sound
			G_Damage( ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING );
			// falls through to FALL_SHORT

//----(SA)	added the audible events for jumping/falling

// (SA) right now just do a short range event on all surfaces until surface-specific stuff is working
			AICast_AudibleEvent( ent->s.number, ent->s.pos.trBase, g_footstepAudibleRange.value );

		case EV_FALL_SHORT:

//				 if(eventParm & (SURF_GRAVEL|SURF_GLASS))						fallSoundMul *= 0.5f;
//			else if(eventParm & SURF_ROOF)										fallSoundMul *= 0.75f;
//			else if(eventParm & SURF_NOSTEPS|SURF_GRASS|SURF_CARPET|SURF_SNOW)	fallSoundMul *= 0.0f;		// no sound event
//
//			if(fallSoundMul)
//				AICast_AudibleEvent( ent->s.number, ent->s.pos.trBase, fallSoundMul * g_footstepAudibleRange.value);
			break;
//----(SA)	end

		case EV_FIRE_WEAPON_MG42:
		case EV_FIRE_WEAPON:
		case EV_FIRE_WEAPONB:
		case EV_FIRE_WEAPON_LASTSHOT:
			FireWeapon( ent );
			break;

		case EV_GRENADE_SUICIDE:
			if ( !ent->client ) {
				break;
			}
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die( ent, ent, ent, 100000, MOD_SUICIDE );
			break;

//----(SA)	added (testing)
		case EV_FIRE_QUICKGREN:
			break;  // quickgren gone
			{
				// don't fire if on mounted weapon
				if ( ent->client->ps.persistant[PERS_HWEAPON_USE] && ent->active ) {
					break;
				}

				CalcMuzzlePoints( ent, WP_GRENADE_LAUNCHER );
				weapon_grenadelauncher_fire( ent, WP_GRENADE_LAUNCHER );
			}
			break;
//----(SA)	end

//----(SA)	modified
		case EV_USE_ITEM1:      // ( HI_MEDKIT )	medkit
		case EV_USE_ITEM2:      // ( HI_WINE )		wine
		case EV_USE_ITEM3:      // ( HI_SKULL )		skull of invulnerable
		case EV_USE_ITEM4:      // ( HI_WATER )		protection from drowning
		case EV_USE_ITEM5:      // ( HI_ELECTRIC )	protection from electric attacks
		case EV_USE_ITEM6:      // ( HI_FIRE )		protection from fire attacks
		case EV_USE_ITEM7:      // ( HI_STAMINA )	restores fatigue bar and sets "nofatigue" for a time period
		case EV_USE_ITEM8:      // ( HI_BOOK1 )
		case EV_USE_ITEM9:      // ( HI_BOOK2 )
		case EV_USE_ITEM10:     // ( HI_BOOK3 )
			UseHoldableItem( ent, event - EV_USE_ITEM0 );
			break;
//----(SA)	end

		default:
			if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
				// RF, handle footstep sounds
				if ( ent->client->ps.pm_flags & PMF_DUCKED ) { // no when crouching
					break;
				}

				if ( ent->client->pers.cmd.buttons & BUTTON_WALKING ) {
					break;
				}

				if ( event >= EV_FOOTSTEP && event <= EV_FOOTSTEP_CARPET ) {
					AICast_AudibleEvent( ent->s.number, ent->s.pos.trBase, g_footstepAudibleRange.value );
				}
			}

			break;
		}
	}

}

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps ) {
	/*
	gentity_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence ) {
		// create a temporary entity for this event which is sent to everyone
		// except the client generated the event
		seq = ps->entityEventSequence & (MAX_EVENTS-1);
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity( ps->origin, event );
		number = t->s.number;
		BG_PlayerStateToEntityState( ps, &t->s, qtrue );
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
	*/
}

void limbo( gentity_t *ent ); // JPW NERVE
void reinforce( gentity_t *ent ); // JPW NERVE

void ClientDamage( gentity_t *clent, int entnum, int enemynum, int id );        // NERVE - SMF

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent ) {
	gclient_t   *client;
	pmove_t pm;
//	vec3_t		oldOrigin;
	int oldEventSequence;
	int msec;
	usercmd_t   *ucmd;
	//int i;
	int monsterslick = 0;
	vec3_t muzzlebounce;      // JPW NERVE


	// Rafael wolfkick
	int validkick;
	static int wolfkicktimer = 0;

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if ( client->pers.connected != CON_CONNECTED ) {
		return;
	}

	if ( client->cameraPortal ) {
		G_SetOrigin( client->cameraPortal, client->ps.origin );
		trap_LinkEntity( client->cameraPortal );
		VectorCopy( client->cameraOrigin, client->cameraPortal->s.origin2 );
	}

	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	// NERVE - SMF - parse client damage command
	if ( ucmd->cld ) {
		int id, enemynum;

		// NOTE: MAX_CLIENTS currently only needs 7 bits, the rest is for id tag
		id = ( ucmd->cld >> 7 );
		enemynum = ucmd->cld & 0x7F;

		ClientDamage( ent, ent->client->ps.clientNum, enemynum, id );
	}
	// -NERVE - SMF

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	}
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		G_Printf("serverTime >>>>>\n" );
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		// Ridah, fixes savegame timing issue
		if ( msec < -100 ) {
			client->ps.commandTime = ucmd->serverTime - 100;
			msec = 100;
		} else {
			return;
		}
		// done.
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set( "pmove_msec", "8" );
	} else if ( pmove_msec.integer > 33 )     {
		trap_Cvar_Set( "pmove_msec", "33" );
	}

	if ( pmove_fixed.integer || client->pers.pmoveFixed ) {
		ucmd->serverTime = ( ( ucmd->serverTime + pmove_msec.integer - 1 ) / pmove_msec.integer ) * pmove_msec.integer;
		//if (ucmd->serverTime - client->ps.commandTime <= 0)
		//	return;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// spectators don't do much
	// DHM - Nerve :: In limbo use SpectatorThink
	if ( client->sess.sessionTeam == TEAM_SPECTATOR || client->ps.pm_flags & PMF_LIMBO ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}

	// NOTE: -------------- SP uses now too
// JPW NERVE do some time-based muzzle flip -- this never gets touched in single player (see g_weapon.c)
// #define RIFLE_SHAKE_TIME 150 // JPW NERVE this one goes with the commented out old damped "realistic" behavior below
#define RIFLE_SHAKE_TIME 300 // per Id request, longer recoil time
	if ( client->sniperRifleFiredTime ) {
		if ( level.time - client->sniperRifleFiredTime > RIFLE_SHAKE_TIME ) {
			client->sniperRifleFiredTime = 0;
		} else {
			VectorCopy( client->ps.viewangles,muzzlebounce );

// JPW old damped behavior -- feels more like a real rifle (modeled on Remington 700 7.62x51mm w/ 14x scope)
/*
			muzzlebounce[PITCH] -= 2*cos(1.0-(level.time - client->sniperRifleFiredTime)*3/RIFLE_SHAKE_TIME);
			muzzlebounce[YAW] += client->sniperRifleMuzzleYaw*cos(1.0-(level.time - client->sniperRifleFiredTime)*3/RIFLE_SHAKE_TIME);
			muzzlebounce[PITCH] -= random()*(1.0f-(level.time - client->sniperRifleFiredTime)/RIFLE_SHAKE_TIME);
			muzzlebounce[YAW] += crandom()*(1.0f-(level.time - client->sniperRifleFiredTime)/RIFLE_SHAKE_TIME);
*/

// JPW per Id request, longer recoil time

			// MP method \/
/*			muzzlebounce[PITCH] -= 2*cos(2.5*(level.time - client->sniperRifleFiredTime)/RIFLE_SHAKE_TIME);
			muzzlebounce[YAW] += 0.5*client->sniperRifleMuzzleYaw*cos(1.0-(level.time - client->sniperRifleFiredTime)*3/RIFLE_SHAKE_TIME);
			muzzlebounce[PITCH] -= 0.25*random()*(1.0f-(level.time - client->sniperRifleFiredTime)/RIFLE_SHAKE_TIME);
			muzzlebounce[YAW] += 0.5*crandom()*(1.0f-(level.time - client->sniperRifleFiredTime)/RIFLE_SHAKE_TIME);
*/

			// NOTE: ----------------- SP uses this method
			muzzlebounce[PITCH] -= 0.25*client->sniperRifleMuzzlePitch*cos( 2.5*( level.time - client->sniperRifleFiredTime ) / RIFLE_SHAKE_TIME );
			muzzlebounce[YAW] += 0.2*client->sniperRifleMuzzleYaw*cos( 1.0 - ( level.time - client->sniperRifleFiredTime )*3 / RIFLE_SHAKE_TIME );
			muzzlebounce[PITCH] -= 0.25*client->sniperRifleMuzzlePitch*random() * ( 1.0f - ( level.time - client->sniperRifleFiredTime ) / RIFLE_SHAKE_TIME );
			muzzlebounce[YAW] += 0.2 * crandom() * ( 1.0f - ( level.time - client->sniperRifleFiredTime ) / RIFLE_SHAKE_TIME );
			SetClientViewAngle( ent,muzzlebounce );
		}
	}
	if ( client->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC ) {
		if ( level.time > client->ps.powerups[PW_REGEN] + 5000 ) {
			client->ps.powerups[PW_REGEN] = level.time;
		}
	}
	// also update weapon recharge time
/*
	if (g_gametype.integer != GT_SINGLE_PLAYER) {
		if (client->ps.classWeaponTime < 1.0f) { // FIXME check based on character class
			client->ps.classWeaponPercent += 0.001;
		}
		G_Printf("server %f\n",client->ps.classWeaponPercent);
	}
*/
// jpw

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}
//----(SA) commented out
	// clear the rewards if time
//	if ( level.time > client->rewardTime ) {
//		client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
//	}

	if (    !saveGamePending &&
			( g_gametype.integer == GT_SINGLE_PLAYER ) &&
			!( ent->r.svFlags & SVF_CASTAI ) ) {

		trap_Cvar_Update( &g_missionStats );

		if ( strlen( g_missionStats.string ) > 1 ) {

			// ignore movements and buttons
			ucmd->buttons = 0;
			ucmd->forwardmove = 0;
			ucmd->rightmove = 0;
			ucmd->upmove = 0;
			ucmd->wbuttons = 0;
			ucmd->wolfkick = 0;

		} else {    // age their play time

			//AICast_AgePlayTime( ent->s.number );

		}
	}

	if ( ( ( saveGamePending || !ent->aiCharacter ) && g_reloading.integer && ( g_reloading.integer != RELOAD_FAILED ) ) || client->cameraPortal ) {

		ucmd->buttons = 0;
		ucmd->forwardmove = 0;
		ucmd->rightmove = 0;
		ucmd->upmove = 0;
		ucmd->wbuttons = 0;
		ucmd->wolfkick = 0;

		// freeze player (RELOAD_FAILED still allowed to move/look)
		if ( client->cameraPortal || ( g_reloading.integer & ( RELOAD_NEXTMAP_WAITING | RELOAD_ENDGAME ) ) ) {
			VectorClear( client->ps.velocity );
			client->ps.pm_type = PM_FREEZE;
		}

	} else if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else {
		client->ps.pm_type = PM_NORMAL;
	}

	// set parachute anim condition flag
	BG_UpdateConditionValue( ent->s.number, ANIM_COND_PARACHUTE, ( ent->flags & FL_PARACHUTE ) != 0, qfalse );

	// all playing clients are assumed to be in combat mode
	if ( !client->ps.aiChar ) {
		client->ps.aiState = AISTATE_COMBAT;
	}

	client->ps.gravity = g_gravity.value;

	// set speed
	client->ps.speed = g_speed.value;

	if ( client->ps.powerups[PW_HASTE] ) {
		client->ps.speed *= 1.3;
	}

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	client->currentAimSpreadScale = (float)client->ps.aimSpreadScale / 255.0;

	memset( &pm, 0, sizeof( pm ) );

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	pm.oldcmd = client->pers.oldcmd;
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
		// DHM-Nerve added:: EF_DEAD is checked for in Pmove functions, but wasn't being set
		//              until after Pmove
		pm.ps->eFlags |= EF_DEAD;
		// dhm-Nerve end
	} else {
		pm.tracemask = ent->clipmask;
	}
	// MrE: always use capsule for AI and player
	pm.trace = trap_TraceCapsule; //trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;

	pm.noWeapClips = ( g_dmflags.integer & DF_NO_WEAPRELOAD ) > 0;
	if ( ent->aiCharacter && AICast_NoReload( ent->s.number ) ) {
		pm.noWeapClips = qtrue; // ensure AI characters don't use clips if they're not supposed to.

	}
	// Ridah
//	if (ent->r.svFlags & SVF_NOFOOTSTEPS)
//		pm.noFootsteps = qtrue;

	VectorCopy( client->ps.origin, client->oldOrigin );

	// perform a pmove
#ifdef MISSIONPACK
	if ( level.intermissionQueued != 0 && g_singlePlayer.integer ) {
		if ( level.time - level.intermissionQueued >= 1000  ) {
			pm.cmd.buttons = 0;
			pm.cmd.forwardmove = 0;
			pm.cmd.rightmove = 0;
			pm.cmd.upmove = 0;
			if ( level.time - level.intermissionQueued >= 2000 && level.time - level.intermissionQueued <= 2500 ) {
				trap_SendConsoleCommand( EXEC_APPEND, "centerview\n" );
			}
			ent->client->ps.pm_type = PM_SPINTERMISSION;
		}
	}
#endif
	monsterslick = Pmove( &pm );

	if ( monsterslick && !( ent->flags & FL_NO_MONSTERSLICK ) ) {
		{
			vec3_t dir;
			vec3_t kvel;
			vec3_t forward;
			float angle = 0.0f;   // TTimo: init
			qboolean bogus = qfalse;

			// NE
			if ( ( monsterslick & SURF_MONSLICK_N ) && ( monsterslick & SURF_MONSLICK_E ) ) {
				angle = 45;
			}
			// NW
			else if ( ( monsterslick & SURF_MONSLICK_N ) && ( monsterslick & SURF_MONSLICK_W ) ) {
				angle = 135;
			}
			// N
			else if ( monsterslick & SURF_MONSLICK_N ) {
				angle = 90;
			}
			// SE
			else if ( ( monsterslick & SURF_MONSLICK_S ) && ( monsterslick & SURF_MONSLICK_E ) ) {
				angle = 315;
			}
			// SW
			else if ( ( monsterslick & SURF_MONSLICK_S ) && ( monsterslick & SURF_MONSLICK_W ) ) {
				angle = 225;
			}
			// S
			else if ( monsterslick & SURF_MONSLICK_S ) {
				angle = 270;
			}
			// E
			else if ( monsterslick & SURF_MONSLICK_E ) {
				angle = 0;
			}
			// W
			else if ( monsterslick & SURF_MONSLICK_W ) {
				angle = 180;
			} else
			{
				bogus = qtrue;
			}

			if ( !bogus ) {
				// when an actor dies test if tag_head is in a contents_solid and stop slide
				{
					orientation_t   or;
					trace_t tr;
					vec3_t start, end;
					qboolean slide = qtrue;

					if ( trap_GetTag( ent->s.number, "tag_head", &or ) ) {
						VectorCopy( or.origin, start );
						VectorCopy( start, end );
						end[2] += 1.0;

						trap_Trace( &tr, start, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER ) );

						if ( tr.contents & CONTENTS_SOLID ) {
							//VectorClear (pm.ps->velocity);
							slide = qfalse;
							// stop checking for slicks
							ent->flags |= FL_NO_MONSTERSLICK;
						}

						if ( slide ) {
							VectorClear( dir );
							dir[YAW] = angle;
							AngleVectors( dir, forward, NULL, NULL );

							VectorScale( forward, 32, kvel );
							VectorAdd( pm.ps->velocity, kvel, pm.ps->velocity );
						}
					}
				}

				/*
				VectorClear (dir);
				dir[YAW] = angle;
				AngleVectors (dir, forward, NULL, NULL);

				VectorScale (forward, 32, kvel);
				VectorAdd (pm.ps->velocity, kvel, pm.ps->velocity);
				*/
			}
		}

	} else if ( ( ent->health <= 0 ) && !( ent->flags & FL_NO_HEADCHECK ) ) { // move bodies away from walls

		orientation_t or;
		vec3_t src, vel;
		trace_t tr;

		if ( VectorLength( pm.ps->velocity ) < 100 && trap_InPVS( pm.ps->origin, g_entities[0].s.pos.trBase ) ) {
			// find the head position
			if ( trap_GetTag( ent->s.number, "tag_head", &or ) ) {
				// move up a tad
				or.origin[2] += 3;
				// move to tip of head
				VectorMA( or.origin, 12, or.axis[2], or.origin );

				// trace from the base of our bounding box, to the head
				VectorCopy( pm.ps->origin, src );
				src[2] += pm.ps->mins[2] + 3;
				if ( or.origin[2] < src[2] ) {
					or.origin[2] = src[2];  // dont let the head sink into the ground (even if it is visually)
				}
				trap_Trace( &tr, src, vec3_origin, vec3_origin, or.origin, ent->s.number, MASK_SOLID );

				// if we hit something, move away from it
				if ( !tr.startsolid && !tr.allsolid && tr.fraction < 1.0 ) {
					// move towards feet
					VectorSubtract( src, or.origin, vel );
					vel[2] = 0;
					VectorNormalize( vel );
					VectorScale( vel, 80 /** (1.0-tr.fraction)*/, vel );
					// dont move against current velocity
					if ( DotProduct( vel, pm.ps->velocity ) >= 0 ) {
						VectorAdd( pm.ps->velocity, vel, pm.ps->velocity );
						if ( VectorLength( pm.ps->velocity ) > 100 ) {
							VectorNormalize( pm.ps->velocity );
							VectorScale( pm.ps->velocity, 100, pm.ps->velocity );
						}
						// stop obeying monsterslick pushes
						ent->flags |= FL_NO_MONSTERSLICK;
					} // else {
					 // disable headcheck, otherwise we risk a constant jitter
					//	ent->flags |= FL_NO_HEADCHECK;
					//}
				}
			}
		}

	}

	// RF, prevent guys from standing ontop of each other
	if ( ent->client->ps.groundEntityNum >= 0 && ent->client->ps.groundEntityNum < MAX_CLIENTS
		 && ( VectorLength( ent->client->ps.velocity ) < 200 ) ) {
		// give them some random velocity
		ent->client->ps.velocity[0] += crandom() * 100;
		ent->client->ps.velocity[1] += crandom() * 100;
		ent->client->ps.velocity[2] += 200;
	}

	// RF, prevent player from standing on a chair which is on a script_mover
	if ( ent->client->ps.groundEntityNum >= MAX_CLIENTS && ent->client->ps.groundEntityNum < ENTITYNUM_WORLD ) {
		gentity_t *groundEnt = &g_entities[ent->client->ps.groundEntityNum];
		if ( groundEnt->isProp && groundEnt->takedamage && !groundEnt->nopickup ) {
			// it's a chair, is it on a mover?
			if ( groundEnt->s.groundEntityNum >= MAX_CLIENTS && groundEnt->s.groundEntityNum < ENTITYNUM_WORLD ) {
				if ( !Q_stricmp( g_entities[groundEnt->s.groundEntityNum].classname, "script_mover" ) ) {
					// break it
					G_Damage( groundEnt, ent, ent, NULL, NULL, 9999, 0, MOD_CRUSH );
				}
			}
		}
	}

	// RF, no events while waiting for a loadgame
	if ( saveGamePending ) {
		memset( client->ps.events, 0, sizeof( client->ps.events ) );
		memset( client->ps.eventParms, 0, sizeof( client->ps.eventParms ) );
		client->ps.eventSequence = 0;
	}

	// server cursor hints
	G_CheckForCursorHints( ent ); //----(SA)	added

	// get this for the clients
	ent->s.animMovetype = BG_GetConditionValue( ent->s.number, ANIM_COND_MOVETYPE, qtrue );

	// Rafael Kick
	if ( ucmd->wolfkick && ent->health > 0 ) {
		validkick = Cmd_WolfKick_f( ent );

		if ( validkick ) {
			wolfkicktimer = level.time + 1000;
		}
	}

	if ( ent->client->ps.persistant[PERS_WOLFKICK] && !ent->melee && !( ent->active ) ) {
		if ( wolfkicktimer < level.time ) {
			ent->client->ps.persistant[PERS_WOLFKICK] = 0;
		} else
		{
			ent->client->ps.persistant[PERS_WOLFKICK] = (int) ( 22 - ( ( wolfkicktimer - level.time ) / 50 ) );

			if ( ent->client->ps.persistant[PERS_WOLFKICK] > 8 ) {
				ent->client->ps.persistant[PERS_WOLFKICK] = 0;
			}
		}
	} else if ( ent->melee )     {
		if ( wolfkicktimer > level.time ) {
			ent->client->ps.persistant[PERS_WOLFKICK] = 0;
			if ( ent->health <= 0 ) {
				ent->melee = NULL;
				ent->client->ps.eFlags &= ~EF_MELEE_ACTIVE;
			} else {

				ent->melee->s.density = (int) ( 22 - ( ( wolfkicktimer - level.time ) / 50 ) );

				if ( ent->melee->s.density > 12 ) {
					ent->melee->s.density = 1;
				}
			}
		} else {
			ent->melee->s.density = 1;
		}
	}

	// done

	// Ridah, allow AI Cast's to evaluate results of their pmove's
	// DHM - Nerve :: Don't do this in multiplayer
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		extern void AICast_EvaluatePmove( int clientnum, pmove_t *pm );
		AICast_EvaluatePmove( ent->s.number, &pm );
	}
	// done.

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
		ent->r.eventTime = level.time;
	}

	// RF, AI needs to be interpolated, so use the old method (which sets TR_INTERPOLATE)
	if ( g_smoothClients.integer && !( ent->r.svFlags & SVF_CASTAI ) ) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, ( ( ent->r.svFlags & SVF_CASTAI ) == 0 ) );
	} else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, ( ( ent->r.svFlags & SVF_CASTAI ) == 0 ) );
	}

	if ( !( ent->client->ps.eFlags & EF_FIRING ) ) {
		client->fireHeld = qfalse;      // for grapple
	}

//
//	// use the precise origin for linking
//	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
//
//	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	VectorCopy( pm.mins, ent->r.mins );
	VectorCopy( pm.maxs, ent->r.maxs );

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents( ent, oldEventSequence );

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity( ent );
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons = client->buttons & ~client->oldbuttons;
//	client->latched_buttons |= client->buttons & ~client->oldbuttons;	// FIXME:? (SA) MP method (causes problems for us.  activate 'sticks')

	//----(SA)	added
	client->oldwbuttons = client->wbuttons;
	client->wbuttons = ucmd->wbuttons;
	client->latched_wbuttons = client->wbuttons & ~client->oldwbuttons;
//	client->latched_wbuttons |= client->wbuttons & ~client->oldwbuttons;	// FIXME:? (SA) MP method

	// Rafael - Activate
	// Ridah, made it a latched event (occurs on keydown only)
	if ( client->latched_buttons & BUTTON_ACTIVATE ) {
		Cmd_Activate_f( ent );
	}

	if ( ent->flags & FL_NOFATIGUE ) {
		ent->client->ps.sprintTime = 20000;
	}


	// Ridah, AI's don't respawn
	if ( !( ent->r.svFlags & SVF_CASTAI ) ) {

		// check for respawning
		if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
			// wait for the attack button to be pressed
			if ( level.time > client->respawnTime ) {
				// forcerespawn is to prevent users from waiting out powerups
				if ( ( g_gametype.integer != GT_SINGLE_PLAYER ) &&
					 ( g_forcerespawn.integer > 0 ) &&
					 ( ( level.time - client->respawnTime ) > g_forcerespawn.integer * 1000 )  &&
					 ( !( ent->client->ps.pm_flags & PMF_LIMBO ) ) ) { // JPW NERVE
					// JPW NERVE
					if ( g_gametype.integer >= GT_WOLF ) {
						limbo( ent );
					} else {
						respawn( ent );
					}
					// jpw
					return;
				}

				// DHM - Nerve :: Single player game respawns immediately as before,
				//				  but in multiplayer, require button press before respawn
				if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
					respawn( ent );
				}
				// pressing attack or use is the normal respawn method
				else if ( ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) &&
						  ( !( ent->client->ps.pm_flags & PMF_LIMBO ) ) ) { // JPW NERVE
					// JPW NERVE
					if ( g_gametype.integer >= GT_WOLF ) {
						limbo( ent );
					} else {
						respawn( ent );
					}
					// jpw
				}
				// dhm - Nerve :: end
			}
			return;
		}

	}
	// done.

	// perform once-a-second actions
	ClientTimerActions( ent, msec );
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t *ent;

	ent = g_entities + clientNum;
	ent->client->pers.oldcmd = ent->client->pers.cmd;
	trap_GetUsercmd( clientNum, &ent->client->pers.cmd );

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if ( !g_syncronousClients.integer ) {
		ClientThink_real( ent );
	}

	// Ridah, let the AI think now
	if ( g_gametype.integer == GT_SINGLE_PLAYER && !( ent->r.svFlags & SVF_CASTAI ) ) {
		AICast_StartFrame( level.time /*ent->client->pers.cmd.serverTime*/ );
		//AICast_StartServerFrame ( level.time );
	}
	// done.
}


void G_RunClient( gentity_t *ent ) {
	if ( !g_syncronousClients.integer ) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );
}

/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t   *cl;
	int do_respawn = 0; // JPW NERVE
	int savedScore;     // DHM
	static int lastRedReinforceTime = 0, lastBlueReinforceTime = 0;
	int testtime;

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) || ( ent->client->ps.pm_flags & PMF_LIMBO ) ) { // JPW NERVE for limbo
		int clientNum;

// JPW NERVE check if it's time to respawn limbo players back into game
//		G_Printf("rlt=%d blt=%d diff=%d\n",g_redlimbotime.integer,g_bluelimbotime.integer, level.time - level.redReinforceTime);
// new way, based on constant spawnrates

		if ( ent->client->sess.sessionTeam == TEAM_RED ) {
			testtime = level.time % g_redlimbotime.integer;
			if ( testtime < lastRedReinforceTime ) {
				do_respawn = 1;
			}
			lastRedReinforceTime = testtime;
		} else if ( ent->client->sess.sessionTeam == TEAM_BLUE )     {
			testtime = level.time % g_bluelimbotime.integer;
			if ( testtime < lastBlueReinforceTime ) {
				do_respawn = 1;
			}
			lastBlueReinforceTime = testtime;
		}

		if ( do_respawn ) {
			reinforce( ent );
			return;
		}

/* old way, based on first guy to die
		if (ent->client->ps.pm_flags & PMF_LIMBO) {
			if (ent->client->sess.sessionTeam == TEAM_RED) {
				ent->client->ps.pm_time = g_redlimbotime.integer - (level.time - level.redReinforceTime);
				if (ent->client->ps.pm_time < 0) {
					do_respawn = 1;
					level.redReinforceTime = level.time+6000; // JPW NERVE 5 sec min delay
				}
			}
			if (ent->client->sess.sessionTeam == TEAM_BLUE) {
				ent->client->ps.pm_time =  g_bluelimbotime.integer - (level.time - level.blueReinforceTime);
				if (ent->client->ps.pm_time < 0) {
					do_respawn = 1;
					level.blueReinforceTime = level.time+6000; // JPW NERVE 5 sec min delay
				}
			}
		}
		if (do_respawn) {
			reinforce(ent);
			return;
		}
*/
// jpw

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}
		if ( clientNum >= 0 ) {
			cl = &level.clients[ clientNum ];
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
//				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
// JPW NERVE -- limbo latch
				if ( ent->client->ps.pm_flags & PMF_LIMBO ) {
					// abuse do_respawn var
					savedScore = ent->client->ps.persistant[PERS_SCORE];
					do_respawn = ent->client->ps.pm_time;
					ent->client->ps = cl->ps;
					ent->client->ps.pm_flags |= PMF_FOLLOW;
					ent->client->ps.pm_flags |= PMF_LIMBO;
					ent->client->ps.pm_time = do_respawn; // put pm_time back
					ent->client->ps.persistant[PERS_SCORE] = savedScore;    // put score back
				} else {
					ent->client->ps = cl->ps;
					ent->client->ps.pm_flags |= PMF_FOLLOW;
				}
// jpw
//				ent->client->ps.eFlags = flags;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->client->sess.spectatorClient >= 0 ) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin( ent->client - level.clients );
				}
			}
		}
	}

	if ( ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	int i;
	clientPersistant_t  *pers;

	if ( ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) || ( ent->client->ps.pm_flags & PMF_LIMBO ) ) { // JPW NERVE
		SpectatorClientEndFrame( ent );
		return;
	}

	pers = &ent->client->pers;

	if ( !ent->aiCharacter ) {
		// turn off any expired powerups
		for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {

			if ( i == PW_NOFATIGUE ) {     // not dependant on level.time
				continue;
			}

			if ( ent->client->ps.powerups[ i ] < level.time ) {
				ent->client->ps.powerups[ i ] = 0;
			}
		}
	}

	// save network bandwidth
#if 0
	if ( !g_synchronousClients->integer && ent->client->ps.pm_type == PM_NORMAL ) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear( ent->client->ps.viewangles );
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		return;
	}

	// burn from lava, etc
	P_WorldEffects( ent );

	// apply all the damage taken this frame
	P_DamageFeedback( ent );

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		ent->s.eFlags |= EF_CONNECTION;
	} else {
		ent->s.eFlags &= ~EF_CONNECTION;
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;   // FIXME: get rid of ent->health...

	G_SetClientSound( ent );

	// set the latest infor

	// Ridah, fixes jittery zombie movement
	if ( g_smoothClients.integer ) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, ( ( ent->r.svFlags & SVF_CASTAI ) == 0 ) );
	} else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, ( ( ent->r.svFlags & SVF_CASTAI ) == 0 ) );
	}

	SendPendingPredictableEvents( &ent->client->ps );

	// DHM - Nerve :: Only in single player...
	// Ridah, if they are using a dangerous weapon, let AI do their avoidance
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		switch ( ent->client->ps.weapon ) {
		case WP_TESLA:          // fear the tesla
			AICast_CheckDangerousEntity( ent, DANGER_CLIENTAIM, TESLA_RANGE + 150, 0.5, 0.6, ( ent->client->buttons & BUTTON_ATTACK ? qtrue : qfalse ) );
			break;
		case WP_MONSTER_ATTACK1:
			if ( ent->aiCharacter == AICHAR_ZOMBIE ) {
				AICast_CheckDangerousEntity( ent, DANGER_CLIENTAIM | DANGER_FLAMES, FLAMETHROWER_RANGE + 150, 0.5, 0.8, qtrue );
			}
			break;
		case WP_MONSTER_ATTACK2:
			if ( ent->aiCharacter == AICHAR_ZOMBIE ) {
				if ( ent->client->ps.eFlags & EF_MONSTER_EFFECT ) {
					AICast_CheckDangerousEntity( ent, 0, 4000, 0.5, 0.8, qtrue );
				}
			}
			break;
		case WP_MONSTER_ATTACK3:
			if ( ent->aiCharacter == AICHAR_LOPER ) {
				AICast_CheckDangerousEntity( ent, 0, LOPER_GROUND_RANGE + 100, 0.5, 0.8, qtrue );
			}
			break;
		case WP_FLAMETHROWER:           // fear the flamethrower
			AICast_CheckDangerousEntity( ent, DANGER_CLIENTAIM | DANGER_FLAMES, FLAMETHROWER_RANGE + 150, 0.5, 0.8, ( ent->client->buttons & BUTTON_ATTACK ? qtrue : qfalse ) );
			break;
		case WP_VENOM:              // avoid the venom
			AICast_CheckDangerousEntity( ent, DANGER_CLIENTAIM, 4000, 0.5, 0.8, ( ent->client->buttons & BUTTON_ATTACK ? qtrue : qfalse ) );
			break;
		case WP_PANZERFAUST:    // just predict a rocket attack, rather than waiting for them to fire, then avoiding
			AICast_CheckDangerousEntity( ent, DANGER_CLIENTAIM, 1000, 0.5, 0.8, ( ent->client->buttons & BUTTON_ATTACK ? qtrue : qfalse ) );
			break;
		case WP_GRENADE_PINEAPPLE:
		case WP_GRENADE_LAUNCHER:   // if they are wearing down a grenade fuse, we should be very afraid
			if ( ent->client->ps.grenadeTimeLeft && ent->client->ps.grenadeTimeLeft < 3000 ) {
				AICast_CheckDangerousEntity( ent, DANGER_CLIENTAIM, 1000, 0.5, 0.9, qtrue );
			}
			break;
		}
	}
	// dhm
}
