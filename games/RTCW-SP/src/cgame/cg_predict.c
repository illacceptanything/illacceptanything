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



// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static pmove_t cg_pmove;

static int cg_numSolidEntities;
static centity_t   *cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static int cg_numTriggerEntities;
static centity_t   *cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList( void ) {
	int i;
	centity_t   *cent;
	snapshot_t  *snap;
	entityState_t   *ent;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		snap = cg.nextSnap;
	} else {
		snap = cg.snap;
	}

	for ( i = 0 ; i < snap->numEntities ; i++ ) {
		cent = &cg_entities[ snap->entities[ i ].number ];
		ent = &cent->currentState;

		// RF, dont clip again non-solid bmodels
		if ( cent->nextState.solid == SOLID_BMODEL && ( cent->nextState.eFlags & EF_NONSOLID_BMODEL ) ) {
			continue;
		}

		if ( ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER ) {
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if ( cent->nextState.solid ) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}
}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void CG_ClipMoveToEntities( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
								   int skipNumber, int mask, int capsule, trace_t *tr ) {
	int i, x, zd, zu;
	trace_t trace;
	entityState_t   *ent;
	clipHandle_t cmodel;
	vec3_t bmins, bmaxs;
	vec3_t origin, angles;
	centity_t   *cent;

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if ( ent->number == skipNumber ) {
			continue;
		}

		// RF, special case, ignore chairs if we are carrying them
		if ( ent->eType == ET_PROP && ent->otherEntityNum == skipNumber + 1 ) {
			continue;
		}

		if ( ent->solid == SOLID_BMODEL ) {
			// special value for bmodel
			cmodel = trap_CM_InlineModel( ent->modelindex );
//			VectorCopy( cent->lerpAngles, angles );
			BG_EvaluateTrajectory( &cent->currentState.apos, cg.physicsTime, angles );
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.physicsTime, origin );
		} else {
			// encoded bbox
			x = ( ent->solid & 255 );
			zd = ( ( ent->solid >> 8 ) & 255 );
			zu = ( ( ent->solid >> 16 ) & 255 ) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			// MrE: use bbox or capsule
			if ( ent->eFlags & EF_CAPSULE ) {
				cmodel = trap_CM_TempCapsuleModel( bmins, bmaxs );
			} else {
				cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
			}
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}
		// MrE: use bbox of capsule
		if ( capsule ) {
			trap_CM_TransformedCapsuleTrace( &trace, start, end,
											 mins, maxs, cmodel,  mask, origin, angles );
		} else {
			trap_CM_TransformedBoxTrace( &trace, start, end,
										 mins, maxs, cmodel,  mask, origin, angles );
		}

		if ( trace.allsolid || trace.fraction < tr->fraction ) {
			trace.entityNum = ent->number;
			*tr = trace;
		} else if ( trace.startsolid ) {
			tr->startsolid = qtrue;
		}
		if ( tr->allsolid ) {
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void    CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
				  int skipNumber, int mask ) {
	trace_t t;

	trap_CM_BoxTrace( &t, start, end, mins, maxs, 0, mask );
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities( start, mins, maxs, end, skipNumber, mask, qfalse, &t );

	*result = t;
}

/*
================
CG_TraceCapsule
================
*/
void    CG_TraceCapsule( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
						 int skipNumber, int mask ) {
	trace_t t;

	trap_CM_CapsuleTrace( &t, start, end, mins, maxs, 0, mask );
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities( start, mins, maxs, end, skipNumber, mask, qtrue, &t );

	*result = t;
}

/*
================
CG_PointContents
================
*/
int     CG_PointContents( const vec3_t point, int passEntityNum ) {
	int i;
	entityState_t   *ent;
	centity_t   *cent;
	clipHandle_t cmodel;
	int contents;

	contents = trap_CM_PointContents( point, 0 );

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum ) {
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) { // special value for bmodel
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		contents |= trap_CM_TransformedPointContents( point, cmodel, ent->origin, ent->angles );
	}

	return contents;
}


/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState( qboolean grabAngles ) {
	float f;
	int i;
	playerState_t   *out;
	snapshot_t      *prev, *next;

	out = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t cmd;
		int cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );

		PM_UpdateViewAngles( out, &cmd, CG_Trace );
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.nextFrameTeleport ) {
		return;
	}

	if ( !next || next->serverTime <= prev->serverTime ) {
		return;
	}

	f = (float)( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );

	i = next->ps.bobCycle;
	if ( i < prev->ps.bobCycle ) {
		i += 256;       // handle wraparound
	}
	out->bobCycle = prev->ps.bobCycle + f * ( i - prev->ps.bobCycle );

	for ( i = 0 ; i < 3 ; i++ ) {
		out->origin[i] = prev->ps.origin[i] + f * ( next->ps.origin[i] - prev->ps.origin[i] );
		if ( !grabAngles ) {
			out->viewangles[i] = LerpAngle(
				prev->ps.viewangles[i], next->ps.viewangles[i], f );
		}
		out->velocity[i] = prev->ps.velocity[i] +
						   f * ( next->ps.velocity[i] - prev->ps.velocity[i] );
	}

}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem( centity_t *cent ) {
	gitem_t     *item;

	if ( !cg_predictItems.integer ) {
		return;
	}

//----(SA) wolf -- not allowing this for single player games
//	if( cgs.gametype == GT_SINGLE_PLAYER) {
//		return;
//	}

//----(SA) autoactivate
	if ( !cg_autoactivate.integer ) {
		return;
	}
//----(SA) end


	if ( !BG_PlayerTouchesItem( &cg.predictedPlayerState, &cent->currentState, cg.time ) ) {
		return;
	}

	// never pick an item up twice in a prediction
	if ( cent->miscTime == cg.time ) {
		return;
	}

	if ( !BG_CanItemBeGrabbed( &cent->currentState, &cg.predictedPlayerState ) ) {
		return;     // can't hold it
	}

	item = &bg_itemlist[ cent->currentState.modelindex ];

	// (SA) no prediction of books/clipboards
	if ( item->giType == IT_HOLDABLE ) {
		if ( item->giTag >= HI_BOOK1 && item->giTag <= HI_BOOK3 ) {
			return;
		}
	}
	if ( item->giType == IT_CLIPBOARD ) {
		return;
	}

	// Special case for flags.
	// We don't predict touching our own flag
	if ( cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_RED &&
		 item->giTag == PW_REDFLAG ) {
		return;
	}
	if ( cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_BLUE &&
		 item->giTag == PW_BLUEFLAG ) {
		return;
	}


	// grab it
	BG_AddPredictableEventToPlayerstate( EV_ITEM_PICKUP, cent->currentState.modelindex, &cg.predictedPlayerState );

	// remove it from the frame so it won't be drawn
	cent->currentState.eFlags |= EF_NODRAW;

	// don't touch it again this prediction
	cent->miscTime = cg.time;

	// if its a weapon, give them some predicted ammo so the autoswitch will work
	if ( item->giType == IT_WEAPON ) {
		int weapon;
//----(SA)	added
		weapon = item->giTag;

		if ( weapon == WP_COLT ) {
			if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_COLT ) ) {
				// you got the colt, you gettin' another
				weapon = WP_AKIMBO;
			}
		}
//----(SA)	end

		COM_BitSet( cg.predictedPlayerState.weapons, weapon );

//----(SA)	added
		if ( weapon == WP_SNOOPERSCOPE ) {
			COM_BitSet( cg.predictedPlayerState.weapons, WP_GARAND );
		} else if ( weapon == WP_GARAND ) {
			COM_BitSet( cg.predictedPlayerState.weapons, WP_SNOOPERSCOPE );
		} else if ( weapon == WP_FG42 ) {
			COM_BitSet( cg.predictedPlayerState.weapons, WP_FG42SCOPE );
		} else if ( weapon == WP_SNIPERRIFLE ) {
			COM_BitSet( cg.predictedPlayerState.weapons, WP_MAUSER );
		}
//----(SA)	end

		if ( !cg.predictedPlayerState.ammo[ BG_FindAmmoForWeapon( weapon )] ) {
			cg.predictedPlayerState.ammo[ BG_FindAmmoForWeapon( weapon )] = 1;
		}
	}

//----(SA)
	if ( item->giType == IT_HOLDABLE ) {
		cg.predictedPlayerState.stats[ STAT_HOLDABLE_ITEM ] |= 1 << item->giTag;
	}
//----(SA)	end
}


/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction( void ) {
	int i;
	trace_t trace;
	entityState_t   *ent;
	clipHandle_t cmodel;
	centity_t   *cent;
	qboolean spectator;

	// dead clients don't activate triggers
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	spectator = ( ( cg.predictedPlayerState.pm_type == PM_SPECTATOR ) || ( cg.predictedPlayerState.pm_flags & PMF_LIMBO ) ); // JPW NERVE

	if ( cg.predictedPlayerState.pm_type != PM_NORMAL && !spectator ) {
		return;
	}

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM && !spectator ) {
			CG_TouchItem( cent );
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		trap_CM_BoxTrace( &trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin,
						  cg_pmove.mins, cg_pmove.maxs, cmodel, -1 );

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			cg.hyperspace = qtrue;
		} else {
			float s;
			vec3_t dir;

			// we hit this push trigger
			if ( spectator ) {
				continue;
			}

			// flying characters don't hit bounce pads
			if ( cg.predictedPlayerState.powerups[PW_FLIGHT] ) {
				continue;
			}

			// if we are already flying along the bounce direction, don't play sound again
			VectorNormalize2( ent->origin2, dir );
			s = DotProduct( cg.predictedPlayerState.velocity, dir );
			if ( s < 500 ) {
				// don't play the event sound again if we are in a fat trigger
				BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, 0, &cg.predictedPlayerState );
			}
			VectorCopy( ent->origin2, cg.predictedPlayerState.velocity );
		}
	}
}



/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction. (this is "dead reckoning" and would definately
be nice to have in there (SA))

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/
void CG_PredictPlayerState( void ) {
	int cmdNum, current;
	playerState_t oldPlayerState;
	qboolean moved;
	usercmd_t oldestCmd;
	usercmd_t latestCmd;
	vec3_t deltaAngles;

	cg.hyperspace = qfalse; // will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if ( !cg.validPPS ) {
		cg.validPPS = qtrue;
		cg.predictedPlayerState = cg.snap->ps;
	}

	// demo playback just copies the moves
	if ( cg.demoPlayback || ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ) {
		CG_InterpolatePlayerState( qfalse );
		return;
	}

	// non-predicting local movement will grab the latest angles
	if ( cg_nopredict.integer || cg_synchronousClients.integer
		 || ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) ) { // RF, somewhat of a hack, but just disable prediction if on MG42, since it's just not very prediction friendly
		CG_InterpolatePlayerState( qtrue );
		return;
	}

	// prepare for pmove
	cg_pmove.ps = &cg.predictedPlayerState;
	cg_pmove.trace = CG_TraceCapsule;
	cg_pmove.pointcontents = CG_PointContents;
	if ( cg_pmove.ps->pm_type == PM_DEAD ) {
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
		// DHM-Nerve added:: EF_DEAD is checked for in Pmove functions, but wasn't being set
		//              until after Pmove
		cg_pmove.ps->eFlags |= EF_DEAD;
		// dhm-Nerve end
	} else {
		cg_pmove.tracemask = MASK_PLAYERSOLID;
	}
	if ( ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) || ( cg.snap->ps.pm_flags & PMF_LIMBO ) ) { // JPW NERVE limbo
		cg_pmove.tracemask &= ~CONTENTS_BODY;   // spectators can fly through bodies
	}
	cg_pmove.noFootsteps = ( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;

	//----(SA)	added
	cg_pmove.noWeapClips = ( cgs.dmflags & DF_NO_WEAPRELOAD ) > 0;
	if ( cg.predictedPlayerState.aiChar ) {
		cg_pmove.noWeapClips = qtrue;   // ensure AI characters don't use clips
	}
//----(SA)	end


	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.predictedPlayerState;

	current = trap_GetCurrentCmdNumber();

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &oldestCmd );
	if ( oldestCmd.serverTime > cg.snap->ps.commandTime
		 && oldestCmd.serverTime < cg.time ) {  // special check for map_restart
		if ( cg_showmiss.integer ) {
			CG_Printf( "exceeded PACKET_BACKUP on commands\n" );
		}
//		return;
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd( current, &latestCmd );

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to
	// be ahead of everything else anyway
	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		cg.predictedPlayerState = cg.nextSnap->ps;
		cg.physicsTime = cg.nextSnap->serverTime;
	} else {
		cg.predictedPlayerState = cg.snap->ps;
		cg.physicsTime = cg.snap->serverTime;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set( "pmove_msec", "8" );
	} else if ( pmove_msec.integer > 33 )     {
		trap_Cvar_Set( "pmove_msec", "33" );
	}

	cg_pmove.pmove_fixed = pmove_fixed.integer; // | cg_pmove_fixed.integer;
	cg_pmove.pmove_msec = pmove_msec.integer;

//----(SA)	added
	// restore persistant client-side playerstate variables before doing the pmove
	// this could be done as suggested in qshared.h ~line 991, but right now I copy each variable individually
	cg.predictedPlayerState.weapAnim                = oldPlayerState.weapAnim;
	cg.predictedPlayerState.weapAnimTimer           = oldPlayerState.weapAnimTimer;
	cg.predictedPlayerState.venomTime               = oldPlayerState.venomTime;
	// show_bug.cgi?id=416
	// FIXME TTimo this causing a double weapon reload sound if you hit reload at the same time you run out of ammo
	// cg.predictedPlayerState.weaponstate				= oldPlayerState.weaponstate;	// RF, added this, since they can become unsynched on loadgame, leaving incorrect anims

//----(SA)	end

	// RF, anim system
	if ( cg_animState.integer ) {
		cg.predictedPlayerState.aiState = cg_animState.integer - 1;
	}

	// run cmds
	moved = qfalse;
	for ( cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++ ) {
		// get the command
		trap_GetUserCmd( cmdNum, &cg_pmove.cmd );
		// get the previous command
		trap_GetUserCmd( cmdNum - 1, &cg_pmove.oldcmd );

		if ( cg_pmove.pmove_fixed ) {
			PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd, CG_Trace );
		}

		// don't do anything if the time is before the snapshot player time
		if ( cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime ) {
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if ( cg_pmove.cmd.serverTime > latestCmd.serverTime ) {
			continue;
		}

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if ( cg.predictedPlayerState.commandTime == oldPlayerState.commandTime ) {
			vec3_t delta;
			float len;

			if ( cg.thisFrameTeleport ) {
				// a teleport will not cause an error decay
				VectorClear( cg.predictedError );
				if ( cg_showmiss.integer ) {
					CG_Printf( "PredictionTeleport\n" );
				}
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t adjusted;
				CG_AdjustPositionForMover( cg.predictedPlayerState.origin,
										   cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted, deltaAngles );
				// RF, add the deltaAngles (fixes jittery view while riding trains)
				cg.predictedPlayerState.delta_angles[YAW] += ANGLE2SHORT( deltaAngles[YAW] );

				if ( cg_showmiss.integer ) {
					if ( !VectorCompare( oldPlayerState.origin, adjusted ) ) {
						CG_Printf( "prediction error\n" );
					}
				}
				VectorSubtract( oldPlayerState.origin, adjusted, delta );
				len = VectorLength( delta );
				if ( len > 0.1 ) {
					if ( cg_showmiss.integer ) {
						CG_Printf( "Prediction miss: %f\n", len );
					}
					if ( cg_errorDecay.integer ) {
						int t;
						float f;

						t = cg.time - cg.predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
						if ( f < 0 ) {
							f = 0;
						}
						if ( f > 0 && cg_showmiss.integer ) {
							CG_Printf( "Double prediction decay: %f\n", f );
						}
						VectorScale( cg.predictedError, f, cg.predictedError );
					} else {
						VectorClear( cg.predictedError );
					}
					VectorAdd( delta, cg.predictedError, cg.predictedError );
					cg.predictedErrorTime = cg.oldTime;
				}
			}
		}

		// don't predict gauntlet firing, which is only supposed to happen
		// when it actually inflicts damage
		cg_pmove.gauntletHit = qfalse;

		if ( cg_pmove.pmove_fixed ) {
			cg_pmove.cmd.serverTime = ( ( cg_pmove.cmd.serverTime + pmove_msec.integer - 1 ) / pmove_msec.integer ) * pmove_msec.integer;
		}

		// RF, if waiting for mission stats to go, ignore all input
		if ( ( cgs.scrFadeAlphaCurrent ) || cg_norender.integer ) {
			cg_pmove.cmd.buttons = 0;
			cg_pmove.cmd.forwardmove = 0;
			cg_pmove.cmd.rightmove = 0;
			cg_pmove.cmd.upmove = 0;
			cg_pmove.cmd.wbuttons = 0;
			cg_pmove.cmd.wolfkick = 0;
			cg_pmove.cmd.angles[0] = cg_pmove.oldcmd.angles[0];
			cg_pmove.cmd.angles[1] = cg_pmove.oldcmd.angles[1];
			cg_pmove.cmd.angles[2] = cg_pmove.oldcmd.angles[2];
			if ( cg_pmove.cmd.serverTime - cg.predictedPlayerState.commandTime > 1 ) {
				cg_pmove.cmd.serverTime = cg.predictedPlayerState.commandTime + 1;
			}
		}

		Pmove( &cg_pmove );

		moved = qtrue;

		// add push trigger movement effects
		CG_TouchTriggerPrediction();
	}

	if ( cg_showmiss.integer > 1 ) {
		CG_Printf( "[%i : %i] ", cg_pmove.cmd.serverTime, cg.time );
	}

	if ( !moved ) {
		if ( cg_showmiss.integer ) {
			CG_Printf( "not moved\n" );
		}
		return;
	}

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover( cg.predictedPlayerState.origin,
							   cg.predictedPlayerState.groundEntityNum,
							   cg.physicsTime, cg.time, cg.predictedPlayerState.origin, deltaAngles );

	// fire events and other transition triggered things
	CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );
}

