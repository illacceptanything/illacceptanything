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



// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"

/*
==============
CG_CheckAmmo

If the ammo has gone low enough to generate the warning, play a sound
==============
*/
void CG_CheckAmmo( void ) {
	int i;
	int total;
	int weapons[MAX_WEAPONS / ( sizeof( int ) * 8 )];

	// see about how many seconds of ammo we have remaining
	memcpy( weapons, cg.snap->ps.weapons, sizeof( weapons ) );

	if ( !weapons[0] && !weapons[1] ) { // (SA) we start out with no weapons, so don't make a click on startup
		return;
	}

	total = 0;

	// first weap now WP_LUGER
	for ( i = WP_FIRST ; i < WP_NUM_WEAPONS ; i++ )
	{
		if ( !( weapons[0] & ( 1 << i ) ) ) {
			continue;
		}
		switch ( i )
		{
		case WP_PANZERFAUST:
		case WP_GRENADE_LAUNCHER:
		case WP_GRENADE_PINEAPPLE:
		case WP_LUGER:
		case WP_COLT:
		case WP_AKIMBO:
		case WP_SILENCER:
		case WP_FG42:
		case WP_FG42SCOPE:
		case WP_MP40:
		case WP_THOMPSON:
		case WP_STEN:
		case WP_VENOM:
		case WP_TESLA:
		case WP_MAUSER:
		case WP_GARAND:
		default:
			total += cg.snap->ps.ammo[BG_FindAmmoForWeapon( i )] * 1000;
//				break;
//			default:
//				total += cg.snap->ps.ammo[BG_FindAmmoForWeapon(i)] * 200;
//				break;
		}

		if ( total >= 5000 ) {
			cg.lowAmmoWarning = 0;
			return;
		}
	}

	if ( !cg.lowAmmoWarning ) {
		// play a sound on this transition
		trap_S_StartLocalSound( cgs.media.noAmmoSound, CHAN_LOCAL_SOUND );
	}

	if ( total == 0 ) {
		cg.lowAmmoWarning = 2;
	} else {
		cg.lowAmmoWarning = 1;
	}
}

/*
==============
CG_DamageFeedback
==============
*/
void CG_DamageFeedback( int yawByte, int pitchByte, int damage ) {
	float left, front, up;
	float kick;
	int health;
	float scale;
	vec3_t dir;
	vec3_t angles;
	float dist;
	float yaw, pitch;
	int slot;
	viewDamage_t *vd;

	// show the attacking player's head and name in corner
	cg.attackerTime = cg.time;

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if ( health < 40 ) {
		scale = 1;
	} else {
		scale = 40.0 / health;
	}
	kick = damage * scale;

	if ( kick < 5 ) {
		kick = 5;
	}
	if ( kick > 10 ) {
		kick = 10;
	}

	// find a free slot
	for ( slot = 0; slot < MAX_VIEWDAMAGE; slot++ ) {
		if ( cg.viewDamage[slot].damageTime + cg.viewDamage[slot].damageDuration < cg.time ) {
			break;
		}
	}

	if ( slot == MAX_VIEWDAMAGE ) {
		return;     // no free slots, never override or splats will suddenly disappear

	}
	vd = &cg.viewDamage[slot];

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if ( yawByte == 255 && pitchByte == 255 ) {
		vd->damageX = 0;
		vd->damageY = 0;
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	} else {
		// positional
		pitch = pitchByte / 255.0 * 360;
		yaw = yawByte / 255.0 * 360;

		angles[PITCH] = pitch;
		angles[YAW] = yaw;
		angles[ROLL] = 0;

		AngleVectors( angles, dir, NULL, NULL );
		VectorSubtract( vec3_origin, dir, dir );

		front = DotProduct( dir, cg.refdef.viewaxis[0] );
		left = DotProduct( dir, cg.refdef.viewaxis[1] );
		up = DotProduct( dir, cg.refdef.viewaxis[2] );

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		dist = VectorLength( dir );
		if ( dist < 0.1 ) {
			dist = 0.1;
		}

		cg.v_dmg_roll = kick * left;

		cg.v_dmg_pitch = -kick * front;

		if ( front <= 0.1 ) {
			front = 0.1;
		}
		vd->damageX = crandom() * 0.3 + - left / front;
		vd->damageY = crandom() * 0.3 + up / dist;
	}

	// clamp the position
	if ( vd->damageX > 1.0 ) {
		vd->damageX = 1.0;
	}
	if ( vd->damageX < -1.0 ) {
		vd->damageX = -1.0;
	}

	if ( vd->damageY > 1.0 ) {
		vd->damageY = 1.0;
	}
	if ( vd->damageY < -1.0 ) {
		vd->damageY = -1.0;
	}

	// don't let the screen flashes vary as much
	if ( kick > 10 ) {
		kick = 10;
	}
	vd->damageValue = kick;
	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	vd->damageTime = cg.snap->serverTime;
	vd->damageDuration = kick * 50 * ( 1 + 2 * ( !vd->damageX && !vd->damageY ) );
	cg.damageTime = cg.snap->serverTime;
	cg.damageIndex = slot;
}




/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn( void ) {
	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// need to reset client-side weapon animations
	cg.predictedPlayerState.weapAnim = WEAP_IDLE1;  // reset weapon animations
	cg.predictedPlayerState.weapAnimTimer = 0;      // allow other animations to happen right away
	cg.predictedPlayerState.weaponstate = WEAPON_RAISING;   // hmm, set this?  what to?

	// display weapons available
	cg.weaponSelectTime = cg.time;

	cg.holdableSelectTime = 0;  //----(SA) reset holdable timer
	cg.centerPrintTime = 0;     //----(SA)	reset centerprint counter so previous messages don't re-appear
	cg.cursorHintIcon = 0;
	cg.cursorHintTime = 0;
	cg.yougotmailTime = 0;

//	cg.cameraMode = 0;	//----(SA)	get out of camera for sure

	// select the weapon the server says we are using
	//cg.weaponSelect = cg.snap->ps.weapon;
	// DHM - Nerve :: Clear even more things on respawn
	cg.zoomedBinoc = qfalse;
	cg.zoomedBinoc = cg.zoomedScope = qfalse;
	cg.zoomTime = 0;
	cg.zoomval = 0;

	// reset fog to world fog (if present)
//	trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP,20,0,0,0,0);

	// dhm - end

	trap_Cvar_Set( "cg_notebookpages", "3" );           // (SA) TEMP: clear notebook pages on spawn (cept for page 1&2)  this is temporary
	trap_Cvar_Set( "ui_notebookCurrentPage", "0" ); // (SA) TEMP: clear notebook pages on spawn (cept for page 1)  this is temporary


//	trap_S_FadeAllSound(1.0f, 1000);	// make sure sound fades up

}

extern char *eventnames[];

/*
==============
CG_CheckPlayerstateEvents
==============
*/
void CG_CheckPlayerstateEvents_wolf( playerState_t *ps, playerState_t *ops ) {
	int i;
	int event;
	centity_t   *cent;
/*
	if ( ps->externalEvent && ps->externalEvent != ops->externalEvent ) {
		cent = &cg_entities[ ps->clientNum ];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent( cent, cent->lerpOrigin );
	}
*/
	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for ( i = ps->eventSequence - MAX_EVENTS ; i < ps->eventSequence ; i++ ) {
		if ( ps->events[i & ( MAX_EVENTS - 1 )] != ops->events[i & ( MAX_EVENTS - 1 )]
			 || i >= ops->eventSequence ) {
			event = ps->events[ i & ( MAX_EVENTS - 1 ) ];

			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[ i & ( MAX_EVENTS - 1 ) ];
			CG_EntityEvent( cent, cent->lerpOrigin );
		}
	}
}

void CG_CheckPlayerstateEvents( playerState_t *ps, playerState_t *ops ) {
	int i;
	int event;
	centity_t   *cent;

	if ( ps->externalEvent && ps->externalEvent != ops->externalEvent ) {
		cent = &cg_entities[ ps->clientNum ];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent( cent, cent->lerpOrigin );
	}

	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];
	// go through the predictable events buffer
	for ( i = ps->eventSequence - MAX_EVENTS ; i < ps->eventSequence ; i++ ) {
		// if we have a new predictable event
		if ( i >= ops->eventSequence
			 // or the server told us to play another event instead of a predicted event we already issued
			 // or something the server told us changed our prediction causing a different event
			 || ( i > ops->eventSequence - MAX_EVENTS && ps->events[i & ( MAX_EVENTS - 1 )] != ops->events[i & ( MAX_EVENTS - 1 )] ) ) {

			event = ps->events[ i & ( MAX_EVENTS - 1 ) ];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[ i & ( MAX_EVENTS - 1 ) ];
			CG_EntityEvent( cent, cent->lerpOrigin );

			cg.predictableEvents[ i & ( MAX_PREDICTED_EVENTS - 1 ) ] = event;

			cg.eventSequence++;
		}
	}
}

/*
==================
CG_CheckChangedPredictableEvents
==================
*/
void CG_CheckChangedPredictableEvents( playerState_t *ps ) {
	int i;
	int event;
	centity_t   *cent;

	cent = &cg.predictedPlayerEntity;
	for ( i = ps->eventSequence - MAX_EVENTS ; i < ps->eventSequence ; i++ ) {
		//
		if ( i >= cg.eventSequence ) {
			continue;
		}
		// if this event is not further back in than the maximum predictable events we remember
		if ( i > cg.eventSequence - MAX_PREDICTED_EVENTS ) {
			// if the new playerstate event is different from a previously predicted one
			if ( ps->events[i & ( MAX_EVENTS - 1 )] != cg.predictableEvents[i & ( MAX_PREDICTED_EVENTS - 1 ) ] ) {

				event = ps->events[ i & ( MAX_EVENTS - 1 ) ];
				cent->currentState.event = event;
				cent->currentState.eventParm = ps->eventParms[ i & ( MAX_EVENTS - 1 ) ];
				CG_EntityEvent( cent, cent->lerpOrigin );

				cg.predictableEvents[ i & ( MAX_PREDICTED_EVENTS - 1 ) ] = event;

				if ( cg_showmiss.integer ) {
					CG_Printf( "WARNING: changed predicted event\n" );
				}
			}
		}
	}
}


/*
==================
CG_CheckLocalSounds
==================
*/
void CG_CheckLocalSounds( playerState_t *ps, playerState_t *ops ) {
	const char  *s;
	int highScore;

	// hit changes
	if ( ps->persistant[PERS_HITS] > ops->persistant[PERS_HITS] ) {
		trap_S_StartLocalSound( cgs.media.hitSound, CHAN_LOCAL_SOUND );
	} else if ( ps->persistant[PERS_HITS] < ops->persistant[PERS_HITS] ) {
		trap_S_StartLocalSound( cgs.media.hitTeamSound, CHAN_LOCAL_SOUND );
	}

	// health changes of more than -1 should make pain sounds
	if ( ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 1 ) {
		if ( ps->stats[STAT_HEALTH] > 0 ) {
			CG_PainEvent( &cg.predictedPlayerEntity, ps->stats[STAT_HEALTH], qfalse );
		}
	}


	// if we are going into the intermission, don't start any voices
	if ( cg.intermissionStarted ) {
		return;
	}

	// reward sounds
	if ( ps->persistant[PERS_REWARD_COUNT] > ops->persistant[PERS_REWARD_COUNT] ) {
		switch ( ps->persistant[PERS_REWARD] ) {
		case REWARD_IMPRESSIVE:
			trap_S_StartLocalSound( cgs.media.impressiveSound, CHAN_ANNOUNCER );
			cg.rewardTime = cg.time;
			cg.rewardShader = cgs.media.medalImpressive;
			cg.rewardCount = ps->persistant[PERS_IMPRESSIVE_COUNT];
			break;
		case REWARD_EXCELLENT:
			trap_S_StartLocalSound( cgs.media.excellentSound, CHAN_ANNOUNCER );
			cg.rewardTime = cg.time;
			cg.rewardShader = cgs.media.medalExcellent;
			cg.rewardCount = ps->persistant[PERS_EXCELLENT_COUNT];
			break;
		case REWARD_DENIED:
			trap_S_StartLocalSound( cgs.media.deniedSound, CHAN_ANNOUNCER );
			break;
		case REWARD_GAUNTLET:
			trap_S_StartLocalSound( cgs.media.humiliationSound, CHAN_ANNOUNCER );
			// if we are the killer and not the killee, show the award
			if ( ps->stats[STAT_HEALTH] ) {
				cg.rewardTime = cg.time;
				cg.rewardShader = cgs.media.medalGauntlet;
				cg.rewardCount = ps->persistant[PERS_GAUNTLET_FRAG_COUNT];
			}
			break;
		default:
			CG_Error( "Bad reward_t" );
		}
	} else {
		// lead changes (only if no reward)
		s = CG_ConfigString( CS_WARMUP );
		if ( !s[0] ) {
			// never play lead changes during warmup
			if ( ps->persistant[PERS_RANK] != ops->persistant[PERS_RANK] ) {
				if ( cgs.gametype >= GT_TEAM ) {
					if ( ps->persistant[PERS_RANK] == 2 ) {
						trap_S_StartLocalSound( cgs.media.teamsTiedSound, CHAN_ANNOUNCER );
					} else if (  ps->persistant[PERS_RANK] == 0 ) {
						trap_S_StartLocalSound( cgs.media.redLeadsSound, CHAN_ANNOUNCER );
					} else if ( ps->persistant[PERS_RANK] == 1 ) {
						trap_S_StartLocalSound( cgs.media.blueLeadsSound, CHAN_ANNOUNCER );
					}
				} else {
					if (  ps->persistant[PERS_RANK] == 0 ) {
						trap_S_StartLocalSound( cgs.media.takenLeadSound, CHAN_ANNOUNCER );
					} else if ( ps->persistant[PERS_RANK] == RANK_TIED_FLAG ) {
						trap_S_StartLocalSound( cgs.media.tiedLeadSound, CHAN_ANNOUNCER );
					} else if ( ( ops->persistant[PERS_RANK] & ~RANK_TIED_FLAG ) == 0 ) {
						trap_S_StartLocalSound( cgs.media.lostLeadSound, CHAN_ANNOUNCER );
					}
				}
			}
		}
	}

	// timelimit warnings
	if ( cgs.timelimit > 0 ) {
		int msec;

		msec = cg.time - cgs.levelStartTime;

		if ( cgs.timelimit > 5 && !( cg.timelimitWarnings & 1 ) && msec > ( cgs.timelimit - 5 ) * 60 * 1000 ) {
			cg.timelimitWarnings |= 1;
			trap_S_StartLocalSound( cgs.media.fiveMinuteSound, CHAN_ANNOUNCER );
		}
		if ( !( cg.timelimitWarnings & 2 ) && msec > ( cgs.timelimit - 1 ) * 60 * 1000 ) {
			cg.timelimitWarnings |= 2;
			trap_S_StartLocalSound( cgs.media.oneMinuteSound, CHAN_ANNOUNCER );
		}
		if ( !( cg.timelimitWarnings & 4 ) && msec > ( cgs.timelimit * 60 + 2 ) * 1000 ) {
			cg.timelimitWarnings |= 4;
			trap_S_StartLocalSound( cgs.media.suddenDeathSound, CHAN_ANNOUNCER );
		}
	}

	// fraglimit warnings
	if ( cgs.fraglimit > 0 && cgs.gametype != GT_CTF ) {
		highScore = cgs.scores1;
		if ( cgs.fraglimit > 3 && !( cg.fraglimitWarnings & 1 ) && highScore == ( cgs.fraglimit - 3 ) ) {
			cg.fraglimitWarnings |= 1;
			trap_S_StartLocalSound( cgs.media.threeFragSound, CHAN_ANNOUNCER );
		}
		if ( cgs.fraglimit > 2 && !( cg.fraglimitWarnings & 2 ) && highScore == ( cgs.fraglimit - 2 ) ) {
			cg.fraglimitWarnings |= 2;
			trap_S_StartLocalSound( cgs.media.twoFragSound, CHAN_ANNOUNCER );
		}
		if ( !( cg.fraglimitWarnings & 4 ) && highScore == ( cgs.fraglimit - 1 ) ) {
			cg.fraglimitWarnings |= 4;
			trap_S_StartLocalSound( cgs.media.oneFragSound, CHAN_ANNOUNCER );
		}
	}
}

/*
===============
CG_TransitionPlayerState

===============
*/
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops ) {
	// check for changing follow mode
	if ( ps->clientNum != ops->clientNum ) {
		cg.thisFrameTeleport = qtrue;
		// make sure we don't get any unwanted transition effects
		*ops = *ps;

		// DHM - Nerve :: After Limbo, make sure and do a CG_Respawn
		if ( ps->clientNum == cg.clientNum ) {
			ops->persistant[PERS_SPAWN_COUNT]--;
		}
	}

	// damage events (player is getting wounded)
	if ( ps->damageEvent != ops->damageEvent && ps->damageCount ) {
		CG_DamageFeedback( ps->damageYaw, ps->damagePitch, ps->damageCount );
	}

	// respawning
	if ( ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT] ) {
		CG_Respawn();
	}

	if ( cg.mapRestart ) {
		CG_Respawn();
		cg.mapRestart = qfalse;
	}

	if ( cg.snap->ps.pm_type != PM_INTERMISSION
		 && ps->persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
		CG_CheckLocalSounds( ps, ops );
	}

	// check for going low on ammo
	CG_CheckAmmo();

	// run events
	CG_CheckPlayerstateEvents( ps, ops );

	// smooth the ducking viewheight change
	if ( ps->viewheight != ops->viewheight ) {
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime = cg.time;
	}
}

