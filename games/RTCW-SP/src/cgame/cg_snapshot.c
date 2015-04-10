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

// cg_snapshot.c -- things that happen on snapshot transition,
// not necessarily every single rendered frame



#include "cg_local.h"



/*
==================
CG_ResetEntity
==================
*/
static void CG_ResetEntity( centity_t *cent ) {
	// if an event is set, assume it is new enough to use
	// if the event had timed out, it would have been cleared
	// RF, not needed for wolf
	// DHM - Nerve :: Wolf is now using this.
	cent->previousEvent = 0;

	cent->trailTime = cg.snap->serverTime;

	// Ridah
	cent->headJuncIndex = 0;
	cent->headJuncIndex2 = 0;
	// RF, disabled this, since we clear events out now in g_main.c, this is redundant, and actually
	// causes "double-door-sound" syndrome if a door is triggered and heard, and then comes into view before
	// it has timed out in g_main.c, therefore playing the sound again, since it thinks it hasn't processed this
	// event yet.
	//cent->previousEventSequence = 0;
	// done.

	VectorCopy( cent->currentState.origin, cent->lerpOrigin );
	VectorCopy( cent->currentState.angles, cent->lerpAngles );
	if ( cent->currentState.eType == ET_PLAYER ) {
		CG_ResetPlayerEntity( cent );
	}
}



/*
===============
CG_TransitionEntity

cent->nextState is moved to cent->currentState and events are fired
===============
*/
static void CG_TransitionEntity( centity_t *cent ) {

	// Ridah, update the fireDir if it's on fire
	if ( CG_EntOnFire( cent ) ) {
		vec3_t newDir, newPos, oldPos;
		float adjust;
		//
		BG_EvaluateTrajectory( &cent->nextState.pos, cg.snap->serverTime, newPos );
		BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, oldPos );
		// update the fireRiseDir
		VectorSubtract( oldPos, newPos, newDir );
		// fire should go upwards if travelling slow
		newDir[2] += 2;
		if ( VectorNormalize( newDir ) < 1 ) {
			VectorClear( newDir );
			newDir[2] = 1;
		}
		// now move towards the newDir
		adjust = 0.3;
		VectorMA( cent->fireRiseDir, adjust, newDir, cent->fireRiseDir );
		if ( VectorNormalize( cent->fireRiseDir ) <= 0.1 ) {
			VectorCopy( newDir, cent->fireRiseDir );
		}
	}

	//----(SA)	the ent lost or gained some part(s), do any necessary effects
	//TODO: check for ai first
	if ( cent->currentState.dmgFlags != cent->nextState.dmgFlags ) {
		CG_AttachedPartChange( cent );
	}


	cent->currentState = cent->nextState;
	cent->currentValid = qtrue;

	// reset if the entity wasn't in the last frame or was teleported
	if ( !cent->interpolate ) {
		CG_ResetEntity( cent );
	}

	// clear the next state.  if will be set by the next CG_SetNextSnap
	cent->interpolate = qfalse;

	// check for events
	CG_CheckEvents( cent );
}


/*
==================
CG_SetInitialSnapshot

This will only happen on the very first snapshot, or
on tourney restarts.  All other times will use
CG_TransitionSnapshot instead.

FIXME: Also called by map_restart?
==================
*/
void CG_SetInitialSnapshot( snapshot_t *snap ) {
	char buf[64];
	int i;
	centity_t       *cent;
	entityState_t   *state;

	cg.snap = snap;

	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );

	// sort out solid entities
	CG_BuildSolidList();

	CG_ExecuteNewServerCommands( snap->serverCommandSequence );

	trap_SendClientCommand( "fogswitch 0" );   // clear it out so the set below will take

	trap_Cvar_VariableStringBuffer( "r_savegameFogColor", buf, sizeof( buf ) );
	trap_Cvar_Set( "r_savegameFogColor", "0" );
	if ( strlen( buf ) > 1 ) {
		if ( !Q_stricmp( buf, "none" ) ) {
			trap_SendClientCommand( "fogswitch 0" );   // 'off'
		} else {
			trap_SendClientCommand( va( "fogswitch %s", buf ) );
		}
	} else {
		trap_Cvar_VariableStringBuffer( "r_mapFogColor", buf, sizeof( buf ) );
		trap_SendClientCommand( va( "fogswitch %s", buf ) );
	}

	// set our local weapon selection pointer to
	// what the server has indicated the current weapon is
	CG_Respawn();

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		state = &cg.snap->entities[ i ];
		cent = &cg_entities[ state->number ];

		memcpy( &cent->currentState, state, sizeof( entityState_t ) );
		//cent->currentState = *state;
		cent->interpolate = qfalse;
		cent->currentValid = qtrue;

		CG_ResetEntity( cent );

		// check for events
		CG_CheckEvents( cent );
	}

	// DHM - Nerve :: Set cg.clientNum so that it may be used elsewhere
	cg.clientNum = snap->ps.clientNum;

	// NERVE - SMF
	{
		static char prevmap[64] = { 0 };
		char curmap[64];

		trap_Cvar_VariableStringBuffer( "mapname", curmap, 64 );

		if ( cgs.gametype == GT_WOLF && Q_stricmp( curmap, prevmap ) ) {
			strcpy( prevmap, curmap );
			trap_SendConsoleCommand( "openLimboMenu\n" );
		}
	}
	// -NERVE - SMF
}


/*
===================
CG_TransitionSnapshot

The transition point from snap to nextSnap has passed
===================
*/
static void CG_TransitionSnapshot( void ) {
	centity_t           *cent;
	snapshot_t          *oldFrame;
	int i;

	if ( !cg.snap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.snap" );
	}
	if ( !cg.nextSnap ) {
		CG_Error( "CG_TransitionSnapshot: NULL cg.nextSnap" );
	}

	// execute any server string commands before transitioning entities
	CG_ExecuteNewServerCommands( cg.nextSnap->serverCommandSequence );

	// if we had a map_restart, set everthing with initial

	if ( !( cg.snap ) || !( cg.nextSnap ) ) {
		return;
	}

	// clear the currentValid flag for all entities in the existing snapshot
	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		cent->currentValid = qfalse;
	}

	// move nextSnap to snap and do the transitions
	oldFrame = cg.snap;
	cg.snap = cg.nextSnap;

	BG_PlayerStateToEntityState( &cg.snap->ps, &cg_entities[ cg.snap->ps.clientNum ].currentState, qfalse );
	cg_entities[ cg.snap->ps.clientNum ].interpolate = qfalse;

	for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		CG_TransitionEntity( cent );
	}

	cg.nextSnap = NULL;

	// check for playerstate transition events
	if ( oldFrame ) {
		playerState_t   *ops, *ps;

		ops = &oldFrame->ps;
		ps = &cg.snap->ps;
		// teleporting checks are irrespective of prediction
		if ( ( ps->eFlags ^ ops->eFlags ) & EF_TELEPORT_BIT ) {
			cg.thisFrameTeleport = qtrue;   // will be cleared by prediction code
		}

		// if we are not doing client side movement prediction for any
		// reason, then the client events and view changes will be issued now
		if ( cg.demoPlayback || ( cg.snap->ps.pm_flags & PMF_FOLLOW )
			 || cg_nopredict.integer || cg_synchronousClients.integer ) {
			CG_TransitionPlayerState( ps, ops );
		}
	}

}


/*
===================
CG_SetNextSnap

A new snapshot has just been read in from the client system.
===================
*/
static void CG_SetNextSnap( snapshot_t *snap ) {
	int num;
	entityState_t       *es;
	centity_t           *cent;

	cg.nextSnap = snap;

	BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].nextState, qfalse );
	cg_entities[ cg.snap->ps.clientNum ].interpolate = qtrue;

	// check for extrapolation errors
	for ( num = 0 ; num < snap->numEntities ; num++ ) {
		es = &snap->entities[num];
		cent = &cg_entities[ es->number ];

		memcpy( &cent->nextState, es, sizeof( entityState_t ) );
		//cent->nextState = *es;

		// if this frame is a teleport, or the entity wasn't in the
		// previous frame, don't interpolate
		if ( !cent->currentValid || ( ( cent->currentState.eFlags ^ es->eFlags ) & EF_TELEPORT_BIT )  ) {
			cent->interpolate = qfalse;
		} else {
			cent->interpolate = qtrue;
		}
	}

	// if the next frame is a teleport for the playerstate, we
	// can't interpolate during demos
	if ( cg.snap && ( ( snap->ps.eFlags ^ cg.snap->ps.eFlags ) & EF_TELEPORT_BIT ) ) {
		cg.nextFrameTeleport = qtrue;
	} else {
		cg.nextFrameTeleport = qfalse;
	}

	// if changing follow mode, don't interpolate
	if ( cg.nextSnap->ps.clientNum != cg.snap->ps.clientNum ) {
		cg.nextFrameTeleport = qtrue;
	}

	// if changing server restarts, don't interpolate
	if ( ( cg.nextSnap->snapFlags ^ cg.snap->snapFlags ) & SNAPFLAG_SERVERCOUNT ) {
		cg.nextFrameTeleport = qtrue;
	}

	// sort out solid entities
	CG_BuildSolidList();
}


/*
========================
CG_ReadNextSnapshot

This is the only place new snapshots are requested
This may increment cgs.processedSnapshotNum multiple
times if the client system fails to return a
valid snapshot.
========================
*/
static snapshot_t *CG_ReadNextSnapshot( void ) {
	qboolean r;
	snapshot_t  *dest;

	if ( cg.latestSnapshotNum > cgs.processedSnapshotNum + 1000 ) {
		CG_Printf( "WARNING: CG_ReadNextSnapshot: way out of range, %i > %i",
				   cg.latestSnapshotNum, cgs.processedSnapshotNum );
	}

	while ( cgs.processedSnapshotNum < cg.latestSnapshotNum ) {
		// decide which of the two slots to load it into
		if ( cg.snap == &cg.activeSnapshots[0] ) {
			dest = &cg.activeSnapshots[1];
		} else {
			dest = &cg.activeSnapshots[0];
		}

		// try to read the snapshot from the client system
		cgs.processedSnapshotNum++;
		r = trap_GetSnapshot( cgs.processedSnapshotNum, dest );

		// FIXME: why would trap_GetSnapshot return a snapshot with the same server time
		if ( cg.snap && r && dest->serverTime == cg.snap->serverTime ) {
			//continue;
		}

		// if it succeeded, return
		if ( r ) {
			CG_AddLagometerSnapshotInfo( dest );

			// RF, if we have no weapon selected, and this snapshots says we have a weapon, then switch to that
			if ( cg.snap && !cg.weaponSelect && cg.snap->ps.weapon ) {
				cg.weaponSelect = cg.snap->ps.weapon;
				cg.weaponSelectTime = cg.time;
			}

			// Ridah, savegame: we should use this as our new base snapshot
			// server has been restarted
			if ( cg.snap && ( dest->snapFlags ^ cg.snap->snapFlags ) & SNAPFLAG_SERVERCOUNT ) {
				int i;
				centity_t backupCent;
				CG_SetInitialSnapshot( dest );
				cg.nextFrameTeleport = qtrue;
				cg.damageTime = 0;
				cg.duckTime = -1;
				cg.landTime = -1;
				cg.stepTime = -1;
				// RF, loadgame hasn't occured yet, so this is likely wrong
				//cg.weaponSelect = cg.snap->ps.weapon;
				cg.weaponSelectTime = cg.time;
				memset( cg.viewDamage, 0, sizeof( cg.viewDamage ) );
				memset( cg.cameraShake, 0, sizeof( cg.cameraShake ) );
				// go through an reset the cent's
				for ( i = 0; i < MAX_GENTITIES; i++ ) {
					backupCent = cg_entities[i];
					memset( &cg_entities[i], 0, sizeof( centity_t ) );
					cg_entities[i].currentState = backupCent.currentState;
					cg_entities[i].nextState = backupCent.nextState;
					cg_entities[i].currentValid = backupCent.currentValid;
					cg_entities[i].interpolate = backupCent.interpolate;
				}
				// reset the predicted cent
				memset( &cg.predictedPlayerEntity, 0, sizeof( centity_t ) );
				cg.predictedPlayerEntity.currentState = backupCent.currentState;
				cg.predictedPlayerEntity.nextState = backupCent.nextState;
				cg.predictedPlayerEntity.currentValid = backupCent.currentValid;
				cg.predictedPlayerEntity.interpolate = backupCent.interpolate;

				return NULL;
			}

			return dest;
		}

		// a GetSnapshot will return failure if the snapshot
		// never arrived, or  is so old that its entities
		// have been shoved off the end of the circular
		// buffer in the client system.

		// record as a dropped packet
		CG_AddLagometerSnapshotInfo( NULL );

		// If there are additional snapshots, continue trying to
		// read them.
	}

	// nothing left to read
	return NULL;
}


/*
============
CG_ProcessSnapshots

We are trying to set up a renderable view, so determine
what the simulated time is, and try to get snapshots
both before and after that time if available.

If we don't have a valid cg.snap after exiting this function,
then a 3D game view cannot be rendered.  This should only happen
right after the initial connection.  After cg.snap has been valid
once, it will never turn invalid.

Even if cg.snap is valid, cg.nextSnap may not be, if the snapshot
hasn't arrived yet (it becomes an extrapolating situation instead
of an interpolating one)

============
*/
void CG_ProcessSnapshots( void ) {
	snapshot_t      *snap;
	int n;

	// see what the latest snapshot the client system has is
	trap_GetCurrentSnapshotNumber( &n, &cg.latestSnapshotTime );
	if ( n != cg.latestSnapshotNum ) {
		if ( n < cg.latestSnapshotNum ) {
			// this should never happen
			CG_Error( "CG_ProcessSnapshots: n < cg.latestSnapshotNum" );
		}
		cg.latestSnapshotNum = n;
	}

	// If we have yet to receive a snapshot, check for it.
	// Once we have gotten the first snapshot, cg.snap will
	// always have valid data for the rest of the game
	while ( !cg.snap ) {
		snap = CG_ReadNextSnapshot();
		if ( !snap ) {
			// we can't continue until we get a snapshot
			return;
		}

		// set our weapon selection to what
		// the playerstate is currently using
		if ( !( snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) ) {
			CG_SetInitialSnapshot( snap );
		}
	}

	// loop until we either have a valid nextSnap with a serverTime
	// greater than cg.time to interpolate towards, or we run
	// out of available snapshots
	do {
		// if we don't have a nextframe, try and read a new one in
		if ( !cg.nextSnap ) {
			snap = CG_ReadNextSnapshot();

			// if we still don't have a nextframe, we will just have to
			// extrapolate
			if ( !snap ) {
				break;
			}

			CG_SetNextSnap( snap );

			// if time went backwards, we have a level restart
			if ( cg.nextSnap->serverTime < cg.snap->serverTime ) {
				CG_Error( "CG_ProcessSnapshots: Server time went backwards" );
			}
		}

		// if our time is < nextFrame's, we have a nice interpolating state
		if ( cg.time >= cg.snap->serverTime && cg.time < cg.nextSnap->serverTime ) {
			break;
		}

		// we have passed the transition from nextFrame to frame
		CG_TransitionSnapshot();
	} while ( 1 );

	// assert our valid conditions upon exiting
	if ( cg.snap == NULL ) {
		CG_Error( "CG_ProcessSnapshots: cg.snap == NULL" );
	}
	if ( cg.time < cg.snap->serverTime ) {
		// this can happen right after a vid_restart
		cg.time = cg.snap->serverTime;
	}
	if ( cg.nextSnap != NULL && cg.nextSnap->serverTime <= cg.time ) {
		CG_Error( "CG_ProcessSnapshots: cg.nextSnap->serverTime <= cg.time" );
	}

}
