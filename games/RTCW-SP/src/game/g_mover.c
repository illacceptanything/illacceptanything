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
 * name:		g_mover.c
 *
 * desc:
 *
*/

#include "g_local.h"

char *hintStrings[] = {
	"",                  // HINT_NONE
	"HINT_NONE",     // actually HINT_FORCENONE, but since this is being specified in the ent, the designer actually means HINT_FORCENONE
	"HINT_PLAYER",
	"HINT_ACTIVATE",
	"HINT_NOACTIVATE",
	"HINT_DOOR",
	"HINT_DOOR_ROTATING",
	"HINT_DOOR_LOCKED",
	"HINT_DOOR_ROTATING_LOCKED",
	"HINT_MG42",
	"HINT_BREAKABLE",
	"HINT_BREAKABLE_BIG",
	"HINT_CHAIR",
	"HINT_ALARM",
	"HINT_HEALTH",
	"HINT_TREASURE",
	"HINT_KNIFE",
	"HINT_LADDER",
	"HINT_BUTTON",
	"HINT_WATER",
	"HINT_CAUTION",
	"HINT_DANGER",
	"HINT_SECRET",
	"HINT_QUESTION",
	"HINT_EXCLAMATION",
	"HINT_CLIPBOARD",
	"HINT_WEAPON",
	"HINT_AMMO",
	"HINT_ARMOR",
	"HINT_POWERUP",
	"HINT_HOLDABLE",
	"HINT_INVENTORY",
	"HINT_SCENARIC",
	"HINT_EXIT",
	"HINT_NOEXIT",
	"HINT_EXIT_FAR",
	"HINT_NOEXIT_FAR",
	"HINT_PLYR_FRIEND",
	"HINT_PLYR_NEUTRAL",
	"HINT_PLYR_ENEMY",
	"HINT_PLYR_UNKNOWN",
	"HINT_BUILD",

	"",                  // HINT_BAD_USER
};

/*
===============================================================================

PUSHMOVE

===============================================================================
*/

void MatchTeam( gentity_t *teamLeader, int moverState, int time );
void Reached_Train( gentity_t *ent );
void Think_BeginMoving( gentity_t *ent );
void Use_Func_Rotate( gentity_t * ent, gentity_t * other, gentity_t * activator );
void Blocked_Door( gentity_t *ent, gentity_t *other );
void Blocked_DoorRotate( gentity_t *ent, gentity_t *other );

typedef struct {
	gentity_t   *ent;
	vec3_t origin;
	vec3_t angles;
	float deltayaw;
} pushed_t;
pushed_t pushed[MAX_GENTITIES], *pushed_p;


/*
============
G_TestEntityPosition

============
*/
gentity_t   *G_TestEntityPosition( gentity_t *ent ) {
	trace_t tr;
	int mask;

	if ( ent->clipmask ) {
		if ( ent->r.contents == CONTENTS_CORPSE ) {
			// corpse aren't important
			//G_Damage( ent, NULL, NULL, NULL, NULL, 99999, 0, MOD_CRUSH );
			return NULL;
		} else {
			mask = ent->clipmask;
		}
	} else {
		mask = MASK_SOLID;
	}
	if ( ent->client ) {
		trap_TraceCapsule( &tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs, ent->client->ps.origin, ent->s.number, mask );
	} else if ( ent->s.eType == ET_MISSILE ) {
		trap_Trace( &tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, ent->s.pos.trBase, ent->r.ownerNum, mask );
	} else {
		trap_Trace( &tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, ent->s.pos.trBase, ent->s.number, mask );
	}

	if ( tr.startsolid ) {
		return &g_entities[ tr.entityNum ];
	}

	return NULL;
}

/*
============
G_TestEntityDropToFloor

============
*/
void G_TestEntityDropToFloor( gentity_t *ent, float maxdrop ) {
	trace_t tr;
	int mask;
	vec3_t endpos;

	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_SOLID;
	}
	if ( ent->client ) {
		VectorCopy( ent->client->ps.origin, endpos );
	} else {
		VectorCopy( ent->s.pos.trBase, endpos );
	}

	endpos[2] -= maxdrop;
	if ( ent->client ) {
		trap_TraceCapsule( &tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs, endpos, ent->s.number, mask );
	} else {
		trap_Trace( &tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, endpos, ent->s.number, mask );
	}

	VectorCopy( tr.endpos, ent->s.pos.trBase );
	if ( ent->client ) {
		VectorCopy( tr.endpos, ent->client->ps.origin );
	}
}

/*
============
G_TestEntityMoveTowardsPos

============
*/
void G_TestEntityMoveTowardsPos( gentity_t *ent, vec3_t pos ) {
	trace_t tr;
	int mask;

	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_SOLID;
	}
	if ( ent->client ) {
		trap_TraceCapsule( &tr, ent->client->ps.origin, ent->r.mins, ent->r.maxs, pos, ent->s.number, mask );
	} else {
		trap_Trace( &tr, ent->s.pos.trBase, ent->r.mins, ent->r.maxs, pos, ent->s.number, mask );
	}

	VectorCopy( tr.endpos, ent->s.pos.trBase );
	if ( ent->client ) {
		VectorCopy( tr.endpos, ent->client->ps.origin );
	}
}

/*
================
G_CreateRotationMatrix
================
*/
void G_CreateRotationMatrix( const vec3_t angles, vec3_t matrix[3] ) {
	AngleVectors( angles, matrix[0], matrix[1], matrix[2] );
	VectorInverse( matrix[1] );
}

/*
================
G_TransposeMatrix
================
*/
// TTimo: const vec_t ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c in Wolf MP source
void G_TransposeMatrix( /*const*/ vec3_t matrix[3], vec3_t transpose[3] ) {
	int i, j;
	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			transpose[i][j] = matrix[j][i];
		}
	}
}

/*
================
G_RotatePoint
================
*/
// TTimo: const vec_t ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c in Wolf MP source
void G_RotatePoint( vec3_t point, /*const*/ vec3_t matrix[3] ) {
	vec3_t tvec;

	VectorCopy( point, tvec );
	point[0] = DotProduct( matrix[0], tvec );
	point[1] = DotProduct( matrix[1], tvec );
	point[2] = DotProduct( matrix[2], tvec );
}


/*
==================
G_TryPushingEntity

Returns qfalse if the move is blocked
==================
*/
qboolean    G_TryPushingEntity( gentity_t *check, gentity_t *pusher, vec3_t move, vec3_t amove ) {
	vec3_t org, org2, move2;
	gentity_t   *block;
	vec3_t matrix[3], transpose[3];
	float x, fx, y, fy, z, fz;
#define JITTER_INC  4
#define JITTER_MAX  ( check->r.maxs[0] / 2.0 )

	// EF_MOVER_STOP will just stop when contacting another entity
	// instead of pushing it, but entities can still ride on top of it
	if ( ( pusher->s.eFlags & EF_MOVER_STOP ) &&
		 check->s.groundEntityNum != pusher->s.number ) {
		//pusher->s.eFlags |= EF_MOVER_BLOCKED;
		return qfalse;
	}

	// save off the old position
	if ( pushed_p > &pushed[MAX_GENTITIES] ) {
		G_Error( "pushed_p > &pushed[MAX_GENTITIES]" );
	}
	pushed_p->ent = check;
	VectorCopy( check->s.pos.trBase, pushed_p->origin );
	VectorCopy( check->s.apos.trBase, pushed_p->angles );
	if ( check->client ) {
		pushed_p->deltayaw = check->client->ps.delta_angles[YAW];
		VectorCopy( check->client->ps.origin, pushed_p->origin );
	}
	pushed_p++;

	// try moving the contacted entity
	VectorAdd( check->s.pos.trBase, move, check->s.pos.trBase );
	if ( check->client ) {
		// make sure the client's view rotates when on a rotating mover
		// RF, this is done client-side now
		check->client->ps.delta_angles[YAW] += ANGLE2SHORT( amove[YAW] );
		//
		// RF, AI's need their ideal angle adjusted instead
		if ( check->aiCharacter ) {
			AICast_AdjustIdealYawForMover( check->s.number, ANGLE2SHORT( amove[YAW] ) );
		}
	}

	// figure movement due to the pusher's amove
	G_CreateRotationMatrix( amove, transpose );
	G_TransposeMatrix( transpose, matrix );
	VectorSubtract( check->s.pos.trBase, pusher->r.currentOrigin, org );
	if ( check->client ) {
		VectorSubtract( check->client->ps.origin, pusher->r.currentOrigin, org );
	}
	VectorCopy( org, org2 );
	G_RotatePoint( org2, matrix );
	VectorSubtract( org2, org, move2 );
	VectorAdd( check->s.pos.trBase, move2, check->s.pos.trBase );
	if ( check->client ) {
		VectorAdd( check->client->ps.origin, move, check->client->ps.origin );
		VectorAdd( check->client->ps.origin, move2, check->client->ps.origin );
	}

	// may have pushed them off an edge
	if ( check->s.groundEntityNum != pusher->s.number ) {
		check->s.groundEntityNum = -1;
	}

	block = G_TestEntityPosition( check );
	if ( !block ) {
		// pushed ok
		if ( check->client ) {
			VectorCopy( check->client->ps.origin, check->r.currentOrigin );
		} else {
			VectorCopy( check->s.pos.trBase, check->r.currentOrigin );
		}
		return qtrue;
	}

	// RF, if still not valid, move them around to see if we can find a good spot
	if ( JITTER_MAX > JITTER_INC ) {
		VectorCopy( check->s.pos.trBase, org );
		if ( check->client ) {
			VectorCopy( check->client->ps.origin, org );
		}
		for ( z = 0; z < JITTER_MAX; z += JITTER_INC )
			for ( fz = -z; fz <= z; fz += 2 * z ) {
				for ( x = JITTER_INC; x < JITTER_MAX; x += JITTER_INC )
					for ( fx = -x; fx <= x; fx += 2 * x ) {
						for ( y = JITTER_INC; y < JITTER_MAX; y += JITTER_INC )
							for ( fy = -y; fy <= y; fy += 2 * y ) {
								VectorSet( move2, fx, fy, fz );
								VectorAdd( org, move2, org2 );
								VectorCopy( org2, check->s.pos.trBase );
								if ( check->client ) {
									VectorCopy( org2, check->client->ps.origin );
								}
								//
								// do the test
								block = G_TestEntityPosition( check );
								if ( !block ) {
									// pushed ok
									if ( check->client ) {
										VectorCopy( check->client->ps.origin, check->r.currentOrigin );
									} else {
										VectorCopy( check->s.pos.trBase, check->r.currentOrigin );
									}
									return qtrue;
								}
							}
					}
				if ( !fz ) {
					break;
				}
			}
		// didnt work, so set the position back
		VectorCopy( org, check->s.pos.trBase );
		if ( check->client ) {
			VectorCopy( org, check->client->ps.origin );
		}
	}

	// if it is ok to leave in the old position, do it
	// this is only relevent for riding entities, not pushed
	// Sliding trapdoors can cause this.
	VectorCopy( ( pushed_p - 1 )->origin, check->s.pos.trBase );
	if ( check->client ) {
		VectorCopy( ( pushed_p - 1 )->origin, check->client->ps.origin );
	}
	VectorCopy( ( pushed_p - 1 )->angles, check->s.apos.trBase );
	block = G_TestEntityPosition( check );
	if ( !block ) {
		check->s.groundEntityNum = -1;
		pushed_p--;
		return qtrue;
	}

	// blocked
	return qfalse;
}


/*
============
G_MoverPush

Objects need to be moved back on a failed push,
otherwise riders would continue to slide.
If qfalse is returned, *obstacle will be the blocking entity
============
*/
qboolean G_MoverPush( gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle ) {
	int i, e;
	gentity_t   *check;
	vec3_t mins, maxs;
	pushed_t    *p;
	int entityList[MAX_GENTITIES];
	int moveList[MAX_GENTITIES];
	int listedEntities, moveEntities;
	vec3_t totalMins, totalMaxs;

	*obstacle = NULL;


	// mins/maxs are the bounds at the destination
	// totalMins / totalMaxs are the bounds for the entire move
	if ( pusher->r.currentAngles[0] || pusher->r.currentAngles[1] || pusher->r.currentAngles[2]
		 || amove[0] || amove[1] || amove[2] ) {
		float radius;

		radius = RadiusFromBounds( pusher->r.mins, pusher->r.maxs );
		for ( i = 0; i < 3; i++ ) {
			mins[i] = pusher->r.currentOrigin[i] - radius + move[i];
			maxs[i] = pusher->r.currentOrigin[i] + radius + move[i];
			totalMins[i] = pusher->r.currentOrigin[i] - radius;
			totalMaxs[i] = pusher->r.currentOrigin[i] + radius;
		}
	} else {
		for ( i = 0; i < 3; i++ ) {
			mins[i] = pusher->r.absmin[i] + move[i];
			maxs[i] = pusher->r.absmax[i] + move[i];
		}
		VectorCopy( pusher->r.absmin, totalMins );
		VectorCopy( pusher->r.absmax, totalMaxs );
	}
	for ( i = 0; i < 3; i++ ) {
		if ( move[i] > 0 ) {
			totalMaxs[i] += move[i];
		} else {
			totalMins[i] += move[i];
		}
	}

	// unlink the pusher so we don't get it in the entityList
	trap_UnlinkEntity( pusher );

	listedEntities = trap_EntitiesInBox( totalMins, totalMaxs, entityList, MAX_GENTITIES );

	// move the pusher to it's final position
	VectorAdd( pusher->r.currentOrigin, move, pusher->r.currentOrigin );
	VectorAdd( pusher->r.currentAngles, amove, pusher->r.currentAngles );
	trap_LinkEntity( pusher );

	moveEntities = 0;
	// see if any solid entities are inside the final position
	for ( e = 0 ; e < listedEntities ; e++ ) {
		check = &g_entities[ entityList[ e ] ];

		if ( check->s.eType == ET_ALARMBOX ) {
			continue;
		}

		if ( check->isProp && check->s.eType == ET_PROP ) {
			continue;
		}

		// only push items and players
		if ( check->s.eType != ET_MISSILE && check->s.eType != ET_ITEM && check->s.eType != ET_PLAYER && !check->physicsObject ) {
			continue;
		}

		if ( check->s.eType == ET_ITEM && check->item->giType == IT_CLIPBOARD && ( check->spawnflags & 1 ) ) {
			continue;
		}

		//if ( check->s.eType == ET_MISSILE && VectorLength( check->s.pos.trDelta ) ) {
		//	continue;	// it's moving
		//}

		// if the entity is standing on the pusher, it will definitely be moved
		if ( check->s.groundEntityNum != pusher->s.number ) {
			// see if the ent needs to be tested
			if ( check->r.absmin[0] >= maxs[0]
				 || check->r.absmin[1] >= maxs[1]
				 || check->r.absmin[2] >= maxs[2]
				 || check->r.absmax[0] <= mins[0]
				 || check->r.absmax[1] <= mins[1]
				 || check->r.absmax[2] <= mins[2] ) {
				continue;
			}
			// see if the ent's bbox is inside the pusher's final position
			// this does allow a fast moving object to pass through a thin entity...
			if ( G_TestEntityPosition( check ) != pusher ) {
				continue;
			}
		}

		moveList[moveEntities++] = entityList[e];
	}

	// unlink all to be moved entities so they cannot get stuck in each other
	for ( e = 0; e < moveEntities; e++ ) {
		check = &g_entities[ moveList[e] ];

		trap_UnlinkEntity( check );
	}

	for ( e = 0; e < moveEntities; e++ ) {
		check = &g_entities[ moveList[e] ];

		// the entity needs to be pushed
		if ( G_TryPushingEntity( check, pusher, move, amove ) ) {
			// link it in now so nothing else tries to clip into us
			trap_LinkEntity( check );
			continue;
		}

		// the move was blocked an entity

		// bobbing entities are instant-kill and never get blocked
		if ( pusher->s.pos.trType == TR_SINE || pusher->s.apos.trType == TR_SINE ) {
			G_Damage( check, pusher, pusher, NULL, NULL, 99999, 0, MOD_CRUSH );
			continue;
		}


		// save off the obstacle so we can call the block function (crush, etc)
		*obstacle = check;

		// move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for ( p = pushed_p - 1 ; p >= pushed ; p-- ) {
			VectorCopy( p->origin, p->ent->s.pos.trBase );
			VectorCopy( p->angles, p->ent->s.apos.trBase );
			if ( p->ent->client ) {
				p->ent->client->ps.delta_angles[YAW] = p->deltayaw;
				VectorCopy( p->origin, p->ent->client->ps.origin );
			}
		}
		// link all entities at their original position
		for ( e = 0; e < moveEntities; e++ ) {
			check = &g_entities[ moveList[e] ];

			trap_LinkEntity( check );
		}
		// movement failed
		return qfalse;
	}
	// link all entities at their final position
	for ( e = 0; e < moveEntities; e++ ) {
		check = &g_entities[ moveList[e] ];

		trap_LinkEntity( check );
	}
	// movement was successfull
	return qtrue;
}


/*
=================
G_MoverTeam
=================
*/
void G_MoverTeam( gentity_t *ent ) {
	vec3_t move, amove;
	gentity_t   *part, *obstacle;
	vec3_t origin, angles;

	obstacle = NULL;

	// make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// if the move is blocked, all moved objects will be backed out
	pushed_p = pushed;
	for ( part = ent ; part ; part = part->teamchain ) {
		// get current position
		BG_EvaluateTrajectory( &part->s.pos, level.time, origin );
		BG_EvaluateTrajectory( &part->s.apos, level.time, angles );
		VectorSubtract( origin, part->r.currentOrigin, move );
		VectorSubtract( angles, part->r.currentAngles, amove );

		//if (part->s.eFlags == EF_MOVER_STOP)
		//	part->s.eFlags &= ~EF_MOVER_BLOCKED;

		if ( part->s.eType == ET_BAT && part->model && !G_MoverPush( part, move, amove, &obstacle ) ) {
			break;
		} else if ( !G_MoverPush( part, move, amove, &obstacle ) )    {
			break;  // move was blocked
		}
	}

	if ( part ) {
		// go back to the previous position
		for ( part = ent ; part ; part = part->teamchain ) {
			part->s.pos.trTime += level.time - level.previousTime;
			part->s.apos.trTime += level.time - level.previousTime;
			BG_EvaluateTrajectory( &part->s.pos, level.time, part->r.currentOrigin );
			BG_EvaluateTrajectory( &part->s.apos, level.time, part->r.currentAngles );
			trap_LinkEntity( part );
		}

		// if the pusher has a "blocked" function, call it
		if ( ent->blocked ) {
			ent->blocked( ent, obstacle );
		}
		return;
	}

	// the move succeeded
	for ( part = ent ; part ; part = part->teamchain ) {
		// call the reached function if time is at or past end point

		// opening/closing sliding door (or bats)
		if ( part->s.pos.trType == TR_LINEAR_STOP ) {
			if ( level.time >= part->s.pos.trTime + part->s.pos.trDuration ) {
				if ( part->reached ) {
					part->reached( part );
				}
			}
		}
//----(SA)	removed
		// opening or closing rotating door
		else if ( part->s.apos.trType == TR_LINEAR_STOP ) {
			if ( level.time >= part->s.apos.trTime + part->s.apos.trDuration ) {
				if ( part->reached ) {
					part->reached( part );
				}
			}
		}
	}
}

/*
================
G_RunMover

================
*/
void G_RunMover( gentity_t *ent ) {
	// if not a team captain, don't do anything, because
	// the captain will handle everything
	if ( ent->flags & FL_TEAMSLAVE ) {
		// FIXME
		// hack to fix problem of tram car slaves being linked
		// after being unlinked in G_FindTeams
		if ( ent->r.linked && !Q_stricmp( ent->classname, "func_tramcar" ) ) {
			trap_UnlinkEntity( ent );
		}
		// Sigh... need to figure out why re links in
		else if ( ent->r.linked && !Q_stricmp( ent->classname, "func_rotating" ) ) {
			trap_UnlinkEntity( ent );
		}
		return;
	}

	// if stationary at one of the positions, don't move anything
	if ( ent->s.pos.trType != TR_STATIONARY || ent->s.apos.trType != TR_STATIONARY ) {
		G_MoverTeam( ent );
	}

	// check think function
	G_RunThink( ent );
}

/*
============================================================================

GENERAL MOVERS

Doors, plats, and buttons are all binary (two position) movers
Pos1 is "at rest", pos2 is "activated"
============================================================================
*/

/*
===============
SetMoverState
===============
*/
void SetMoverState( gentity_t *ent, moverState_t moverState, int time ) {
	vec3_t delta;
	float f;
	qboolean kicked = qfalse, soft = qfalse;

	kicked = (qboolean)( ent->flags & FL_KICKACTIVATE );
	soft = (qboolean)( ent->flags & FL_SOFTACTIVATE );    //----(SA)	added

	if ( ent->flags & FL_DOORNOISE ) { // this door is always 'regular' open
		kicked = qfalse;
		soft = qfalse;
	}

	ent->moverState     = moverState;
	ent->s.pos.trTime   = time;
	ent->s.apos.trTime  = time;
	switch ( moverState ) {
	case MOVER_POS1:
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		ent->active = qfalse;
		break;
	case MOVER_POS2:
		VectorCopy( ent->pos2, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;

		// JOSEPH 1-26-00
	case MOVER_POS3:
		VectorCopy( ent->pos3, ent->s.pos.trBase );
		ent->s.pos.trType = TR_STATIONARY;
		break;

	case MOVER_2TO3:
		VectorCopy( ent->pos2, ent->s.pos.trBase );
		VectorSubtract( ent->pos3, ent->pos2, delta );
		f = 1000.0 / ent->s.pos.trDuration;
		VectorScale( delta, f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;
	case MOVER_3TO2:
		VectorCopy( ent->pos3, ent->s.pos.trBase );
		VectorSubtract( ent->pos2, ent->pos3, delta );
		f = 1000.0 / ent->s.pos.trDuration;
		VectorScale( delta, f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;
		// END JOSEPH

	case MOVER_1TO2:        // opening
		VectorCopy( ent->pos1, ent->s.pos.trBase );
		VectorSubtract( ent->pos2, ent->pos1, delta );
//----(SA)	numerous changes start here
		ent->s.pos.trDuration = ent->gDuration;
		f = 1000.0 / ent->s.pos.trDuration;
		VectorScale( delta, f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;
	case MOVER_2TO1:        // closing
		VectorCopy( ent->pos2, ent->s.pos.trBase );
		VectorSubtract( ent->pos1, ent->pos2, delta );
		if ( ent->closespeed ) {                        //----(SA)	handle doors with different close speeds
			ent->s.pos.trDuration = ent->gDurationBack;
			f = 1000.0 / ent->gDurationBack;
		} else {
			ent->s.pos.trDuration = ent->gDuration;
			f = 1000.0 / ent->s.pos.trDuration;
		}
		VectorScale( delta, f, ent->s.pos.trDelta );
		ent->s.pos.trType = TR_LINEAR_STOP;
		break;


	case MOVER_POS1ROTATE:      // at close
		VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
		ent->s.apos.trType = TR_STATIONARY;
		break;
	case MOVER_POS2ROTATE:      // at open
		VectorCopy( ent->r.currentAngles, ent->s.apos.trBase );
		ent->s.apos.trType = TR_STATIONARY;
		break;
	case MOVER_1TO2ROTATE:      // opening
		VectorClear( ent->s.apos.trBase );              // set base to start position {0,0,0}

		if ( kicked ) {
			f = 2000.0 / ent->gDuration;        // double speed when kicked open
			ent->s.apos.trDuration = ent->gDuration / 2.0;
		} else if ( soft ) {
			f = 500.0 / ent->gDuration;         // 1/2 speed when soft opened
			ent->s.apos.trDuration = ent->gDuration * 2;
		} else {
			f = 1000.0 / ent->gDuration;
//				ent->s.apos.trDuration = ent->gDurationBack;	// (SA) durationback?
			ent->s.apos.trDuration = ent->gDuration;
		}
		VectorScale( ent->rotate, f * ent->angle, ent->s.apos.trDelta );
		ent->s.apos.trType = TR_LINEAR_STOP;
		break;
	case MOVER_2TO1ROTATE:      // closing
		VectorScale( ent->rotate, ent->angle, ent->s.apos.trBase );     // set base to end position
		// (kicked closes same as normally opened)
		// (soft closes at 1/2 speed)
		f = 1000.0 / ent->gDuration;
		ent->s.apos.trDuration = ent->gDuration;
		if ( soft ) {
			ent->s.apos.trDuration *= 2;
			f *= 0.5f;
		}
		VectorScale( ent->s.apos.trBase, -f, ent->s.apos.trDelta );
		ent->s.apos.trType = TR_LINEAR_STOP;
		ent->active = qfalse;
		break;


	}
	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin );
	if ( !( ent->r.svFlags & SVF_NOCLIENT ) || ( ent->r.contents ) ) {    // RF, added this for bats, but this is safe for all movers, since if they aren't solid, and aren't visible to the client, they don't need to be linked
		trap_LinkEntity( ent );
		// if this entity is blocking AAS, then update it
		if ( ent->AASblocking && ent->s.pos.trType == TR_STATIONARY ) {
			// reset old blocking areas
			G_SetAASBlockingEntity( ent, qfalse );
			// set new areas
			G_SetAASBlockingEntity( ent, qtrue );
		}
	}
}

/*
================
MatchTeam

All entities in a mover team will move from pos1 to pos2
in the same amount of time
================
*/
void MatchTeam( gentity_t *teamLeader, int moverState, int time ) {
	gentity_t       *slave;

	for ( slave = teamLeader ; slave ; slave = slave->teamchain ) {

		// pass along flags for how door was activated
		if ( teamLeader->flags & FL_KICKACTIVATE ) {
			slave->flags |= FL_KICKACTIVATE;
		}
		if ( teamLeader->flags & FL_SOFTACTIVATE ) {
			slave->flags |= FL_SOFTACTIVATE;
		}

		SetMoverState( slave, moverState, time );
	}
}

/*
MatchTeamReverseAngleOnSlaves

the activator was blocking the door so reverse its direction
*/
void MatchTeamReverseAngleOnSlaves( gentity_t *teamLeader, int moverState, int time ) {
	gentity_t       *slave;

	for ( slave = teamLeader ; slave ; slave = slave->teamchain ) {
		// reverse open dir for teamLeader and all slaves
		slave->angle *= -1;

		// pass along flags for how door was activated
		if ( teamLeader->flags & FL_KICKACTIVATE ) {
			slave->flags |= FL_KICKACTIVATE;
		}
		if ( teamLeader->flags & FL_SOFTACTIVATE ) {
			slave->flags |= FL_SOFTACTIVATE;
		}

		SetMoverState( slave, moverState, time );
	}
}

/*
================
ReturnToPos1
================
*/
void ReturnToPos1( gentity_t *ent ) {
	MatchTeam( ent, MOVER_2TO1, level.time );

	// play starting sound
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );

	ent->s.loopSound = 0;
	// set looping sound
	ent->s.loopSound = ent->sound3to2;

}

// JOSEPH 1-26-00
/*
================
ReturnToPos2
================
*/
void ReturnToPos2( gentity_t *ent ) {
	MatchTeam( ent, MOVER_3TO2, level.time );

	ent->s.loopSound = 0;
	// looping sound
	ent->s.loopSound = ent->soundLoop;

	// starting sound
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound3to2 );
}

/*
================
GotoPos3
================
*/
void GotoPos3( gentity_t *ent ) {
	MatchTeam( ent, MOVER_2TO3, level.time );

	ent->s.loopSound = 0;
	// looping sound
	ent->s.loopSound = ent->soundLoop;

	// starting sound
	G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to3 );
}
// END JOSEPH

/*
================
ReturnToPos1Rotate
	closing
================
*/
void ReturnToPos1Rotate( gentity_t *ent ) {
	qboolean inPVS = qfalse;
	gentity_t   *player;

	MatchTeam( ent, MOVER_2TO1ROTATE, level.time );

	player = AICast_FindEntityForName( "player" );

	if ( player ) {
		inPVS = trap_InPVS( player->r.currentOrigin, ent->r.currentOrigin );
	}

	// play starting sound
	if ( inPVS ) {
//		if( (ent->flags & FL_SOFTACTIVATE) &! (ent->flags & FL_DOORNOISE) )
		if ( ( ent->flags & FL_SOFTACTIVATE ) && !( ent->flags & FL_DOORNOISE ) ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftclose );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
	}

	ent->s.loopSound = ent->sound3to2;
}

/*
================
Reached_BinaryMover
================
*/
void Reached_BinaryMover( gentity_t *ent ) {
	qboolean kicked = qfalse, soft = qfalse;
	// stop the looping sound
	ent->s.loopSound = 0;
//	ent->s.loopSound = ent->soundLoop;

	if ( ent->flags & FL_SOFTACTIVATE ) {
		soft = qtrue;
	}
	if ( ent->flags & FL_KICKACTIVATE ) {
		kicked = qtrue;
	}
	if ( ent->flags & FL_DOORNOISE ) {
		kicked = soft = qfalse;
	}

	if ( ent->moverState == MOVER_1TO2 ) {
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// play sound
		if ( soft ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftendo );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
		}

		// fire targets
		if ( !ent->activator ) {
			ent->activator = ent;
		}

		G_UseTargets( ent, ent->activator );

		if ( ent->flags & FL_TOGGLE ) {
			ent->active = qfalse;   // enable door activation again
			ent->think = ReturnToPos1;
			ent->nextthink = 0;
			return;
		}

		// JOSEPH 1-27-00
		// return to pos1 after a delay
		if ( ent->wait != -1000 ) {
			ent->think = ReturnToPos1;
			ent->nextthink = level.time + ent->wait;
		}
		// END JOSEPH
	} else if ( ent->moverState == MOVER_2TO1 ) {
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		// play sound
		if ( soft ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftendc );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
		}

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qfalse );
		}
	} else if ( ent->moverState == MOVER_1TO2ROTATE )   {
		// reached pos2
		SetMoverState( ent, MOVER_POS2ROTATE, level.time );

		// play sound
		if ( kicked ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundKickedEnd );
		} else if ( soft ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftendo );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
		}

		// fire targets
		if ( !ent->activator ) {
			ent->activator = ent;
		}
		G_UseTargets( ent, ent->activator );

		if ( ent->flags & FL_TOGGLE ) {
			ent->active = qfalse;   // enable door activation again
			ent->think = ReturnToPos1Rotate;
			ent->nextthink = 0;
			return;
		}

		if ( ent->wait != -1000 ) {
			// return to pos1 after a delay (if not wait -1)
			ent->think = ReturnToPos1Rotate;
			ent->nextthink = level.time + ent->wait;
		}

	} else if ( ent->moverState == MOVER_2TO1ROTATE )   {
		// reached pos1
		SetMoverState( ent, MOVER_POS1ROTATE, level.time );

		// to stop sound from being requested if not in pvs anoying bug
		{
			qboolean inPVS = qfalse;
			gentity_t *player;

			player = AICast_FindEntityForName( "player" );

			if ( player ) {
				inPVS = trap_InPVS( player->r.currentOrigin, ent->r.currentOrigin );
			}

			// play sound
			if ( inPVS ) {
				if ( soft ) {
					G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftendc );
				} else {
					G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );
				}
			}
		}

		// clear the 'soft' flag
		ent->flags &= ~FL_SOFTACTIVATE; //----(SA)	added

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qfalse );
		}
	} else {
		G_Error( "Reached_BinaryMover: bad moverState" );
	}

//	ent->flags &= ~(FL_KICKACTIVATE|FL_SOFTACTIVATE);	// (SA) it was not opened normally.  Clear this so it thinks it's closed normally
	ent->flags &= ~FL_KICKACTIVATE; // (SA) it was not opened normally.  Clear this so it thinks it's closed normally

}

/*
================
IsBinaryMoverBlocked
================
*/
qboolean IsBinaryMoverBlocked( gentity_t *ent, gentity_t *other, gentity_t *activator ) {

	vec3_t dir, angles;
	vec3_t pos;
	vec3_t vec;
	float dot;
	vec3_t forward;
	qboolean is_relay = qfalse;

	if ( Q_stricmp( ent->classname, "func_door_rotating" ) == 0 ) {
		if ( ent->spawnflags & 32 ) {
			return qfalse;
		}

		//----(SA)	only check for blockage by players
		if ( !activator ) {
			if ( Q_stricmp( other->classname, "target_relay" ) == 0 ) {
				is_relay = qtrue;
			} else if ( !activator->client )      {
				return qfalse;
			}
		}
		//----(SA)	end

		VectorAdd( ent->r.absmin, ent->r.absmax, pos );
		VectorScale( pos, 0.5, pos );

		VectorSubtract( pos, ent->s.origin, dir );
		vectoangles( dir, angles );

		if ( ent->rotate[YAW] ) {
			angles[YAW] += ent->angle;
		} else if ( ent->rotate[PITCH] ) {
			angles[PITCH] += ent->angle;
		} else if ( ent->rotate[ROLL] ) {
			angles[ROLL] += ent->angle;
		}

		AngleVectors( angles, forward, NULL, NULL );
		// VectorSubtract (other->r.currentOrigin, pos, vec);

		if ( is_relay ) {
			VectorSubtract( other->r.currentOrigin, pos, vec );
		} else {
			VectorSubtract( activator->r.currentOrigin, pos, vec );
		}

		VectorNormalize( vec );
		dot = DotProduct( vec, forward );

		if ( dot >= 0 ) {
			return qtrue;
		} else {
			return qfalse;
		}

	}

	return qfalse;

}

// JOSEPH 1-26-00
/*
================
Reached_TrinaryMover
================
*/
void Reached_TrinaryMover( gentity_t *ent ) {

	// stop the looping sound
	ent->s.loopSound = ent->soundLoop;

	if ( ent->moverState == MOVER_1TO2 ) {
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// goto pos 3
		ent->think = GotoPos3;
		ent->nextthink = level.time + 1000; //FRAMETIME;

		// play sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos2 );
	} else if ( ent->moverState == MOVER_2TO1 ) {
		// reached pos1
		SetMoverState( ent, MOVER_POS1, level.time );

		// play sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos1 );

		// close areaportals
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qfalse );
		}
	} else if ( ent->moverState == MOVER_2TO3 )   {
		// reached pos3
		SetMoverState( ent, MOVER_POS3, level.time );

		// play sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3 );

		// return to pos2 after a delay
		if ( ent->wait != -1000 ) {
			ent->think = ReturnToPos2;
			ent->nextthink = level.time + ent->wait;
		}

		// fire targets
		if ( !ent->activator ) {
			ent->activator = ent;
		}
		G_UseTargets( ent, ent->activator );
	} else if ( ent->moverState == MOVER_3TO2 )   {
		// reached pos2
		SetMoverState( ent, MOVER_POS2, level.time );

		// return to pos1
		ent->think = ReturnToPos1;
		ent->nextthink = level.time + 1000; //FRAMETIME;

		// play sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3 );
	} else {
		G_Error( "Reached_BinaryMover: bad moverState" );
	}
}
// END JOSEPH

// JOSEPH 1-26-00
/*
================
Use_TrinaryMover
================
*/
void Use_TrinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	int total;
	int partial;
	qboolean isblocked = qfalse;

	isblocked = IsBinaryMoverBlocked( ent, other, activator );

	if ( isblocked ) {
		MatchTeamReverseAngleOnSlaves( ent, MOVER_1TO2ROTATE, level.time + 50 );

		// starting sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );

		// looping sound
		ent->s.loopSound = ent->soundLoop;

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qtrue );
		}
		return;
	}

	// only the master should be used
	if ( ent->flags & FL_TEAMSLAVE ) {
		Use_TrinaryMover( ent->teammaster, other, activator );
		return;
	}

	ent->activator = activator;

	if ( ent->moverState == MOVER_POS1 ) {

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2, level.time + 50 );

		// starting sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );

		// looping sound
		ent->s.loopSound = ent->soundLoop;

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qtrue );
		}
		return;
	}

	if ( ent->moverState == MOVER_POS2 ) {

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_2TO3, level.time + 50 );

		// starting sound
		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to3 );

		// looping sound
		ent->s.loopSound = ent->soundLoop;

		return;
	}

	// if all the way up, just delay before coming down
	if ( ent->moverState == MOVER_POS3 ) {
		if ( ent->wait != -1000 ) {
			ent->nextthink = level.time + ent->wait;
		}
		return;
	}

	// only partway down before reversing
	if ( ent->moverState == MOVER_2TO1 ) {
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.time;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_1TO2, level.time - ( total - partial ) );

		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		return;
	}

	if ( ent->moverState == MOVER_3TO2 ) {
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.time;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_2TO3, level.time - ( total - partial ) );

		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to3 );
		return;
	}

	// only partway up before reversing
	if ( ent->moverState == MOVER_1TO2 ) {
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.time;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_2TO1, level.time - ( total - partial ) );

		if ( ent->flags & FL_SOFTACTIVATE ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftclose );
		} else {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
		return;
	}

	if ( ent->moverState == MOVER_2TO3 ) {
		total = ent->s.pos.trDuration;
		partial = level.time - ent->s.time;
		if ( partial > total ) {
			partial = total;
		}

		MatchTeam( ent, MOVER_3TO2, level.time - ( total - partial ) );

		G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound3to2 );
		return;
	}
}
// END JOSEPH

/*
================
Use_BinaryMover
================
*/
void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
//	int		total;
//	int		partial;
	qboolean kicked = qfalse, soft = qfalse;
	qboolean isblocked = qfalse;
	qboolean nosound = qfalse;

	if ( ent->flags & FL_SOFTACTIVATE ) {
		soft = qtrue;
	}
	if ( ent->flags & FL_KICKACTIVATE ) {
		kicked = qtrue;
	}
	if ( ent->flags & FL_DOORNOISE ) {
		kicked = soft = qfalse;
	}

	if ( level.time <= 4000 ) { // hack.  don't play door sounds if in the first /four/ seconds of game (FIXME: TODO: THIS IS STILL A HACK)
		nosound = qtrue;
	}

	// only the master should be used
	if ( ent->flags & FL_TEAMSLAVE ) {

		// pass along flags for how door was activated
		if ( kicked ) {
			ent->teammaster->flags |= FL_KICKACTIVATE;
		}
		if ( soft ) {
			ent->teammaster->flags |= FL_SOFTACTIVATE;
		}

		Use_BinaryMover( ent->teammaster, other, activator );
		return;
	}

	// only check for blocking when opening, otherwise the door has no choice
	if ( ent->moverState == MOVER_POS1 || ent->moverState == MOVER_POS1ROTATE ) {
		isblocked = IsBinaryMoverBlocked( ent, other, activator );
	}


	if ( isblocked ) {
		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		// ent->angle *= -1;
		// MatchTeam( ent, MOVER_1TO2ROTATE, level.time + 50 );
		MatchTeamReverseAngleOnSlaves( ent, MOVER_1TO2ROTATE, level.time + 50 );

		// starting sound
		if ( !nosound ) {
			if ( kicked ) {    // kicked
				if ( activator ) {
					AICast_AudibleEvent( activator->s.number, ent->s.origin, HEAR_RANGE_DOOR_KICKOPEN );
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundKicked );
			} else if ( soft ) {
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftopen );
			} else {
				if ( activator ) {
					AICast_AudibleEvent( activator->s.number, ent->s.origin, HEAR_RANGE_DOOR_OPEN );
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
			}
		}

		ent->s.loopSound = 0;

		// looping sound
		if ( !nosound ) {
			ent->s.loopSound = ent->sound2to3;
		} else if ( !nosound ) {
			ent->s.loopSound = ent->soundLoop;
		}

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qtrue );
		}
		return;
	}

	ent->activator = activator;

	// Rafael
	if ( ent->nextTrain && ent->nextTrain->wait == -1 && ent->nextTrain->count == 1 ) {
		ent->nextTrain->count = 0;
		return;
	}

	if ( ent->moverState == MOVER_POS1 ) {

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2, level.time + 50 );

		// play starting sound
		if ( !nosound ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}

		ent->s.loopSound = 0;

		// set looping sound
		if ( !nosound ) {
			ent->s.loopSound = ent->sound2to3;
		}

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qtrue );
		}
		return;
	}

	if ( ent->moverState == MOVER_POS1ROTATE ) {

		// start moving 50 msec later, becase if this was player
		// triggered, level.time hasn't been advanced yet
		MatchTeam( ent, MOVER_1TO2ROTATE, level.time + 50 );

		// play starting sound
		if ( !nosound ) {
			if ( kicked ) {    // kicked
				if ( activator ) {
					AICast_AudibleEvent( activator->s.number, ent->s.origin, HEAR_RANGE_DOOR_KICKOPEN );
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundKicked );
			} else if ( soft ) {
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftopen );
			} else {
				if ( activator ) {
					AICast_AudibleEvent( activator->s.number, ent->s.origin, HEAR_RANGE_DOOR_OPEN );
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
			}
		}

		ent->s.loopSound = 0;
		// set looping sound
		if ( !nosound ) {
			ent->s.loopSound = ent->sound2to3;
		}

		// open areaportal
		if ( ent->teammaster == ent || !ent->teammaster ) {
			trap_AdjustAreaPortalState( ent, qtrue );
		}
		return;
	}

	// if all the way up, just delay before coming down
	// JOSEPH 1-27-00
	if ( ent->moverState == MOVER_POS2 ) {
		if ( ent->flags & FL_TOGGLE ) {
			ent->nextthink = level.time + 50;
			return;
		}

		if ( ent->wait != -1000 ) {
			ent->nextthink = level.time + ent->wait;
		}
		return;
	}
	// END JOSEPH

	// if all the way up, just delay before coming down
	if ( ent->moverState == MOVER_POS2ROTATE ) {
		if ( ent->flags & FL_TOGGLE ) {
			ent->nextthink = level.time + 50;   // do it *now* for toggles
		} else {
			ent->nextthink = level.time + ent->wait;
		}
		return;
	}

	// only partway down before reversing
	if ( ent->moverState == MOVER_2TO1 ) {
		Blocked_Door( ent, NULL );

		if ( !nosound ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}
		return;
	}

	// only partway up before reversing
	if ( ent->moverState == MOVER_1TO2 ) {
		Blocked_Door( ent, NULL );

		if ( !nosound ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
		}
		return;
	}

	// only partway closed before reversing
	if ( ent->moverState == MOVER_2TO1ROTATE ) {
		Blocked_DoorRotate( ent, NULL );

		if ( !nosound ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound1to2 );
		}
		return;
	}

	// only partway open before reversing
	if ( ent->moverState == MOVER_1TO2ROTATE ) {
		Blocked_DoorRotate( ent, NULL );

		if ( !nosound ) {
			if ( soft ) {
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundSoftclose );
			} else {
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->sound2to1 );
			}
		}
		return;
	}
}



/*
================
InitMover

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMover( gentity_t *ent ) {
	vec3_t move;
	float distance;
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;
	char        *sound;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
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

	// JOSEPH 1-26-00
	if ( !Q_stricmp( ent->classname,"func_secret" ) ) {
		ent->use = Use_TrinaryMover;
		ent->reached = Reached_TrinaryMover;
	} else if ( !Q_stricmp( ent->classname, "func_rotating" ) )       {
		ent->use = Use_Func_Rotate;
		ent->reached = NULL; // rotating can never reach
	} else
	{
		ent->use = Use_BinaryMover;
		ent->reached = Reached_BinaryMover;
	}
	// END JOSEPH

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

//----(SA)	changes
	// open time based on speed
//	VectorScale( move, ent->speed, ent->s.pos.trDelta );
	VectorScale( move, ent->speed, ent->gDelta );
	ent->s.pos.trDuration = distance * 1000 / ent->speed;
	if ( ent->s.pos.trDuration <= 0 ) {
		ent->s.pos.trDuration = 1;
	}
	ent->gDurationBack = ent->gDuration = ent->s.pos.trDuration;

	// close time based on speed
	if ( ent->closespeed ) {
		VectorScale( move, ent->closespeed, ent->gDelta );
		ent->gDurationBack = distance * 1000 / ent->closespeed;
		if ( ent->gDurationBack <= 0 ) {
			ent->gDurationBack = 1;
//----(SA) end
		}
	}
}

/*
================
InitMoverRotate

"pos1", "pos2", and "speed" should be set before calling,
so the movement delta can be calculated
================
*/
void InitMoverRotate( gentity_t *ent ) {
	vec3_t move;
	float distance;
	float light;
	vec3_t color;
	qboolean lightSet, colorSet;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
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

	if ( !( ent->spawnflags & 64 ) ) { // STAYOPEN
		ent->reached = Reached_BinaryMover;
	}

	ent->moverState = MOVER_POS1ROTATE;
	ent->r.svFlags = SVF_USE_CURRENT_ORIGIN;
	ent->s.eType = ET_MOVER;
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
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

	ent->s.apos.trDuration = ent->speed;
	if ( ent->s.apos.trDuration <= 0 ) {
		ent->s.apos.trDuration = 1;
	}

	ent->gDuration = ent->gDurationBack = ent->s.apos.trDuration;   // (SA) store 'real' durations so doors can be opened/closed at different speeds
}


/*
===============================================================================

DOOR

A use can be triggered either by a touch function, by being shot, or by being
targeted by another entity.

===============================================================================
*/


/*
================
Blocked_Door
================
*/
void Blocked_Door( gentity_t *ent, gentity_t *other ) {
	gentity_t *slave;
	int time;

	// remove anything other than a client
	if ( other ) {
		if ( !other->client ) {
			// except CTF flags!!!!
			if ( other->s.eType == ET_ITEM && other->item->giType == IT_TEAM ) {
				Team_DroppedFlagThink( other );
				return;
			}
//			G_TempEntity( other->s.origin, EV_ITEM_POP );
			if ( other->s.eType == ET_MOVER ) {
				if ( strstr( other->classname, "chair" ) ) {
					// break crushed chairs
					G_Damage( other, ent, ent, NULL, NULL, 99999, 0, MOD_CRUSH );   // Die!
					return;
				}
			}
			G_FreeEntity( other );
			return;
		}

		if ( ent->damage ) {
			G_Damage( other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH );
		}
	}

	if ( ent->spawnflags & 4 ) {
		return;     // crushers don't reverse
	}

	// reverse direction
//	Use_BinaryMover( ent, ent, other );
	for ( slave = ent ; slave ; slave = slave->teamchain )
	{
//		time = level.time - slave->s.pos.trTime;
		time = level.time - ( slave->s.pos.trDuration - ( level.time - slave->s.pos.trTime ) );

		if ( slave->moverState == MOVER_1TO2 ) {
			SetMoverState( slave, MOVER_2TO1, time );
		} else {
			SetMoverState( slave, MOVER_1TO2, time );
		}
		trap_LinkEntity( slave );
	}

}

/*
================
Touch_DoorTriggerSpectator
================
*/
static void Touch_DoorTriggerSpectator( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	int i, axis;
	vec3_t origin, dir, angles;

	axis = ent->count;
	VectorClear( dir );
	if ( fabs( other->s.origin[axis] - ent->r.absmax[axis] ) <
		 fabs( other->s.origin[axis] - ent->r.absmin[axis] ) ) {
		origin[axis] = ent->r.absmin[axis] - 10;
		dir[axis] = -1;
	} else {
		origin[axis] = ent->r.absmax[axis] + 10;
		dir[axis] = 1;
	}
	for ( i = 0; i < 3; i++ ) {
		if ( i == axis ) {
			continue;
		}
		origin[i] = ( ent->r.absmin[i] + ent->r.absmax[i] ) * 0.5;
	}
	vectoangles( dir, angles );
	TeleportPlayer( other, origin, angles );
}

/*
================
Blocked_DoorRotate
================
*/

#define DOORPUSHBACK    16

void Blocked_DoorRotate( gentity_t *ent, gentity_t *other ) {

	gentity_t       *slave;
	int time;

	// remove anything other than a client
	if ( other ) {
		if ( !other->client ) {
			G_TempEntity( other->s.origin, EV_ITEM_POP );
			G_FreeEntity( other );
			return;
		}

		if ( other->health <= 0 ) {
			G_Damage( other, ent, ent, NULL, NULL, 99999, 0, MOD_CRUSH );
		}

		if ( ent->damage ) {
			G_Damage( other, ent, ent, NULL, NULL, ent->damage, 0, MOD_CRUSH );
		}
	}

	// RF, set this timer, so AI know not to try and open the door immediately
	ent->grenadeFired = level.time + 2000;

	for ( slave = ent ; slave ; slave = slave->teamchain )
	{
		// RF, set this timer, so AI know not to try and open the door immediately
		slave->grenadeFired = level.time + 2000;

		// RF, trying to fix "stuck in door" bug
		time = level.time - ( slave->s.apos.trDuration - ( level.time - slave->s.apos.trTime ) );
		//time = level.time - slave->s.apos.trTime;

		if ( slave->moverState == MOVER_1TO2ROTATE ) {
			SetMoverState( slave, MOVER_2TO1ROTATE, time );
		} else
		{
			SetMoverState( slave, MOVER_1TO2ROTATE, time );
		}
		trap_LinkEntity( slave );
	}


}


/*
================
Touch_DoorTrigger
================
*/
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( other->client && other->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		// if the door is not open and not opening
		if ( ent->parent->moverState != MOVER_1TO2 &&
			 ent->parent->moverState != MOVER_POS2 ) {
			Touch_DoorTriggerSpectator( ent, other, trace );
		}
	} else if ( ent->parent->moverState != MOVER_1TO2 )   {
		Use_BinaryMover( ent->parent, ent, other );
	}
}

/*
======================
Think_SpawnNewDoorTrigger

All of the parts of a door have been spawned, so create
a trigger that encloses all of them
======================
*/
void Think_SpawnNewDoorTrigger( gentity_t *ent ) {
	gentity_t       *other;
	vec3_t mins, maxs;
	int i, best;

	// set all of the slaves as shootable
	for ( other = ent ; other ; other = other->teamchain ) {
		other->takedamage = qtrue;
	}

	// find the bounds of everything on the team
	VectorCopy( ent->r.absmin, mins );
	VectorCopy( ent->r.absmax, maxs );

	for ( other = ent->teamchain ; other ; other = other->teamchain ) {
		AddPointToBounds( other->r.absmin, mins, maxs );
		AddPointToBounds( other->r.absmax, mins, maxs );
	}

	// find the thinnest axis, which will be the one we expand
	best = 0;
	for ( i = 1 ; i < 3 ; i++ ) {
		if ( maxs[i] - mins[i] < maxs[best] - mins[best] ) {
			best = i;
		}
	}
	maxs[best] += 120;
	mins[best] -= 120;

	// create a trigger with this size
	other = G_Spawn();
	VectorCopy( mins, other->r.mins );
	VectorCopy( maxs, other->r.maxs );
	other->parent = ent;
	other->r.contents = CONTENTS_TRIGGER;
	other->touch = Touch_DoorTrigger;
	trap_LinkEntity( other );

	MatchTeam( ent, ent->moverState, level.time );
}

void Think_MatchTeam( gentity_t *ent ) {
	MatchTeam( ent, ent->moverState, level.time );
}


//----(SA) added

/*
==============
findNonAIBrushTargeter
	determine if there is an entity pointing at ent that is not a "trigger_aidoor"
	(used now for checking which key to set for a door)
==============
*/
qboolean findNonAIBrushTargeter( gentity_t *ent ) {
	gentity_t *targeter = NULL;

	if ( !( ent->targetname ) ) {
		return qfalse;
	}

	while ( ( targeter = G_Find( targeter, FOFS( target ), ent->targetname ) ) != NULL )
	{
		if ( strcmp( targeter->classname,"trigger_aidoor" ) &&
			 Q_stricmp( targeter->classname, "func_invisible_user" ) ) {
			return qtrue;
		}
	}

	return qfalse;
}


/*
==============
finishSpawningKeyedMover
==============
*/
void finishSpawningKeyedMover( gentity_t *ent ) {
	gentity_t       *slave;

	// all ents should be spawned, so it's okay to check for special door triggers now

//----(SA)	modified
	if ( ent->key == KEY_UNLOCKED_ENT ) {  // the key was not set in the spawn
		if ( ent->targetname && findNonAIBrushTargeter( ent ) ) {
			ent->key = KEY_LOCKED_TARGET;   // something is targeting this (other than a trigger_aidoor) so leave locked
		} else {
			ent->key = KEY_NONE;
		}
	}
//----(SA)	end

	if ( ent->key ) {
		G_SetAASBlockingEntity( ent, qtrue );
	}

	ent->nextthink = level.time + FRAMETIME;

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		if ( ent->targetname || ent->takedamage ) {  // non touch/shoot doors
			ent->think = Think_MatchTeam;
		}
// (SA) is this safe?  is ent->spawnflags & 8 consistant among all keyed ents?
		else if ( ( ent->spawnflags & 8 ) && ( strcmp( ent->classname, "func_door_rotating" ) ) ) {
			ent->think = Think_SpawnNewDoorTrigger;
		} else {
			ent->think = Think_MatchTeam;
		}

		// (SA) slaves have been marked as FL_TEAMSLAVE now, so they won't
		// finish their think on their own.  So set keys for teamed doors
		for ( slave = ent ; slave ; slave = slave->teamchain )
		{
			if ( slave == ent ) {
				continue;
			}

			slave->key = ent->key;

			if ( slave->key ) {
				G_SetAASBlockingEntity( slave, qtrue );
			}
		}
	}
}

//----(SA) end



/*
==============
Door_reverse_sounds
	The door has been marked as "START_OPEN" which means the open/closed
	positions have been swapped.
	This swaps the sounds around as well
==============
*/
void Door_reverse_sounds( gentity_t *ent ) {
	int stemp;

	stemp = ent->sound1to2;
	ent->sound1to2 = ent->sound2to1;
	ent->sound2to1 = stemp;

	stemp = ent->soundPos1;
	ent->soundPos1 = ent->soundPos2;
	ent->soundPos2 = stemp;

	stemp = ent->sound2to3;
	ent->sound2to3 = ent->sound3to2;
	ent->sound3to2 = stemp;


	stemp = ent->soundSoftopen;
	ent->soundSoftopen = ent->soundSoftclose;
	ent->soundSoftclose = stemp;

	stemp = ent->soundSoftendo;
	ent->soundSoftendo = ent->soundSoftendc;
	ent->soundSoftendc = stemp;

}


/*
==============
DoorSetSounds
	get sound indexes for the various door sounds
	(used by SP_func_door() and SP_func_door_rotating() )
==============
*/
void DoorSetSounds( gentity_t *ent, int doortype, qboolean isRotating ) {
	ent->sound1to2 = G_SoundIndex( va( "door%i_open", doortype ) );      // opening
	ent->soundPos2 = G_SoundIndex( va( "door%i_endo", doortype ) );      // open
	ent->sound2to1 = G_SoundIndex( va( "door%i_close", doortype ) ); // closing
	ent->soundPos1 = G_SoundIndex( va( "door%i_endc", doortype ) );      // closed
	ent->sound2to3 = G_SoundIndex( va( "door%i_loopo", doortype ) ); // loopopen
	ent->sound3to2 = G_SoundIndex( va( "door%i_loopc", doortype ) ); // loopclosed
	ent->soundPos3 = G_SoundIndex( va( "door%i_locked", doortype ) );    // locked


	ent->soundSoftopen  = G_SoundIndex( va( "door%i_openq", doortype ) );    // opening quietly
	ent->soundSoftendo  = G_SoundIndex( va( "door%i_endoq", doortype ) );    // open quietly
	ent->soundSoftclose = G_SoundIndex( va( "door%i_closeq", doortype ) );   // closing quietly
	ent->soundSoftendc  = G_SoundIndex( va( "door%i_endcq", doortype ) );    // closed quietly

	if ( isRotating ) {
		ent->soundKicked    = G_SoundIndex( va( "door%i_kicked", doortype ) );
		ent->soundKickedEnd = G_SoundIndex( va( "door%i_kickedend", doortype ) );
	}

}

//----(SA)	added
/*
==============
G_TryDoor
	seemed better to have this isolated.  this way i can get func_invisible_user's using the
	regular rules of doors.
==============
*/
void G_TryDoor( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	int soundrange = 0;
	qboolean walking = qfalse, locked = qfalse;

	walking = (qboolean)( ent->flags & FL_SOFTACTIVATE );


	if ( ( ent->s.apos.trType == TR_STATIONARY && ent->s.pos.trType == TR_STATIONARY ) ) {
		if ( ent->active == qfalse ) {
			if ( ent->key >= KEY_LOCKED_ENT ) {    // door force locked
				locked = qtrue;
			} else if ( ent->key == KEY_LOCKED_TARGET ) {
				// door locked because it was targeted, check if the 'other' ent is targeting this door
				if ( Q_stricmp( other->target, ent->targetname ) ) {
					locked = qtrue;
				}
			}

			if ( locked ) {
				if ( !walking && activator ) { // only send audible event if not trying to open slowly
					AICast_AudibleEvent( activator->s.clientNum, ent->s.origin, HEAR_RANGE_DOOR_LOCKED );   // "someone tried locked door near me!"
				}
				G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3 );
				return;
			}

			if ( activator ) {
				if ( ent->key > KEY_NONE && ent->key < KEY_NUM_KEYS ) { // door requires key
					gitem_t *item = BG_FindItemForKey( ent->key, 0 );
					if ( !( activator->client->ps.stats[STAT_KEYS] & ( 1 << item->giTag ) ) ) {
						if ( !walking ) {  // only send audible event if not trying to open slowly
							AICast_AudibleEvent( activator->s.clientNum, ent->s.origin, HEAR_RANGE_DOOR_LOCKED );   // "someone tried locked door near me!"
						}
						// player does not have key
						G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundPos3 );
						return;
					}
				}
			}


			if ( ent->teammaster && ent->team && ent != ent->teammaster ) {
				ent->teammaster->active = qtrue;
				if ( walking ) {
					ent->teammaster->flags |= FL_SOFTACTIVATE;      // no noise generated
				} else {
					if ( activator ) {
						soundrange = HEAR_RANGE_DOOR_OPEN;
					}
				}

				Use_BinaryMover( ent->teammaster, activator, activator );
				G_UseTargets( ent->teammaster, activator );
			} else
			{
				ent->active = qtrue;
				if ( walking ) {
					ent->flags |= FL_SOFTACTIVATE;      // no noise
				} else {
					if ( activator ) {
						soundrange = HEAR_RANGE_DOOR_OPEN;
					}
				}

				Use_BinaryMover( ent, activator, activator );
				G_UseTargets( ent, activator );
			}

			if ( ent->flags & FL_DOORNOISE ) { // this door always plays the 'regular' open sound event
				soundrange = HEAR_RANGE_DOOR_OPEN;
			}

//			if(soundrange)
//				AICast_AudibleEvent( activator->s.clientNum, ent->s.origin, soundrange );
		}
	}
}
//----(SA)	end


/*QUAKED func_door (0 .5 .8) ? START_OPEN TOGGLE CRUSHER TOUCH SHOOT-THRU
TOGGLE      wait in both the start and end states for a trigger event.
START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door
SHOOT-THRU	Bullets don't stop when they hit the door.  Set "shoot_thru_scale" with bullet damage scale (see below)

"key"       -1 for locked, key number for which key opens, 0 for open.  default '0' unless door is targeted.  (trigger_aidoor entities targeting this door do /not/ affect the key status)
"model2"    .md3 model to also draw
"angle"	    determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"	    movement speed (100 default)
"closespeed" optional different movement speed for door closing
"wait"      wait before returning (3 default, -1 = never return)
"lip"       lip remaining at end of move (8 default)
"dmg"       damage to inflict when blocked (2 default)
"color"     constantLight color
"light"     constantLight radius
"health"    if set, the door must be shot open
"team"		team name.  other doors with same team name will open/close in syncronicity
"noisescale"multiplier for how far the noise from the door will travel to alert AI
"type"		use sounds based on construction of door:
	 0 - nosound (default)
	 1 - metal
	 2 - stone
	 3 - lab
	 4 - wood
	 5 - iron/jail
	 6 - portcullis
	 7 - wood (quiet)

SOUND NAMING INFO -
inside "sound/movers/doors/door<number>...
	_open.wav		// opening
	_endo.wav		// open
	_close.wav		// closing
	_endc.wav		// closed
	_loopo.wav		// opening loop
	_loopc.wav		// closing loop
	_locked.wav		// locked

	_openq.wav		// opening quietly
	_endoq.wav		// open quietly
	_closeq.wav		// closing quietly
	_endcq.wav		// closed quietly

and for rotating doors:
	_kicked.wav
	_kickedend.wav

*/
void SP_func_door( gentity_t *ent ) {
	vec3_t abs_movedir;
	float distance;
	vec3_t size;
	float lip;
	int key, doortype;

	G_SpawnInt( "type", "0", &doortype );

	if ( doortype ) { // /*why on earthy did this check for <=8?*/ && doortype <= 8)	// no doortype = silent
		DoorSetSounds( ent, doortype, qfalse );
	}

	ent->blocked = Blocked_Door;

	// default speed of 400
	if ( !ent->speed ) {
		ent->speed = 400;
	}

	// default wait of 2 seconds
	if ( !ent->wait ) {
		ent->wait = 2;
	}
	ent->wait *= 1000;


	if ( G_SpawnInt( "key", "", &key ) ) {    // if door has a key entered, set it
		ent->key = key;

		if ( key == -1 ) {
			ent->key = KEY_LOCKED_ENT;
		} else if ( ent->key > KEY_NUM_KEYS || ent->key < KEY_NONE ) {            // if the key is invalid, set the key in the finishSpawning routine
			G_Error( "invalid key (%d) set for func_door_rotating\n", ent->key );
			ent->key = KEY_UNLOCKED_ENT;
		}
	} else {
		ent->key = KEY_UNLOCKED_ENT;    // otherwise, set the key when this ent finishes spawning
	}


	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );

	// first position at start
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	trap_SetBrushModel( ent, ent->model );
	G_SetMovedir( ent->s.angles, ent->movedir );
	abs_movedir[0] = fabs( ent->movedir[0] );
	abs_movedir[1] = fabs( ent->movedir[1] );
	abs_movedir[2] = fabs( ent->movedir[2] );
	VectorSubtract( ent->r.maxs, ent->r.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	if ( ent->spawnflags & 1 ) {    // START_OPEN - reverse position 1 and 2
		vec3_t temp;
		int tempi;

		VectorCopy( ent->pos2, temp );
		VectorCopy( ent->s.origin, ent->pos2 );
		VectorCopy( temp, ent->pos1 );

		// swap speeds if door has 'closespeed'
		if ( ent->closespeed ) {
			tempi = ent->speed;
			ent->speed = ent->closespeed;
			ent->closespeed = tempi;
		}

		// swap sounds
		Door_reverse_sounds( ent );
	}

	// TOGGLE
	if ( ent->spawnflags & 2 ) {
		ent->flags |= FL_TOGGLE;
	}

	InitMover( ent );

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = qtrue;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
	ent->think = finishSpawningKeyedMover;
}

// JOSEPH 1-26-00
/*QUAKED func_secret (0 .5 .8) ? REVERSE x CRUSHER TOUCH
TOGGLE      wait in both the start and end states for a trigger event.
START_OPEN  the door to moves to its destination when spawned, and operate in reverse.  It is used to temporarily or permanently close off an area when triggered (not useful for touch or takedamage doors).
NOMONSTER   monsters will not trigger this door

"key"       -1 for locked, key number for which key opens, 0 for open.  default '0' unless door is targeted.  (trigger_aidoor entities targeting this door do /not/ affect the key status)
"model2"    .md3 model to also draw
"angle"	    determines the opening direction
"targetname" if set, no touch field will be spawned and a remote button or trigger field activates the door.
"speed"	    movement speed (100 default)
"wait"      wait before returning (2 default, -1 = never return)
"lip"       lip remaining at end of move (8 default)
"dmg"       damage to inflict when blocked (2 default)
"color"     constantLight color
"light"     constantLight radius
"health"    if set, the door must be shot open
*/
void SP_func_secret( gentity_t *ent ) {
	vec3_t abs_movedir;
	vec3_t angles2;
	float distance;
	vec3_t size;
	float lip;
	int key;

	ent->sound1to2 = ent->sound2to1 = ent->sound2to3 = G_SoundIndex( "sound/movers/doors/dr1_strt.wav" );
	ent->soundPos1 = ent->soundPos3 = G_SoundIndex( "sound/movers/doors/dr1_end.wav" );

	ent->blocked = Blocked_Door;

	// default speed of 100
	if ( !ent->speed ) {
		ent->speed = 100;
	}

	// default wait of 2 seconds
	if ( !ent->wait ) {
		ent->wait = 2;
	}
	ent->wait *= 1000;


	if ( G_SpawnInt( "key", "", &key ) ) {    // if door has a key entered, set it
		ent->key = key;

		if ( key == -1 ) {
			ent->key = KEY_LOCKED_ENT;
		} else if ( ent->key > KEY_NUM_KEYS || ent->key < KEY_NONE ) {            // if the key is invalid, set the key in the finishSpawning routine
			G_Error( "invalid key (%d) set for func_door_rotating\n", ent->key );
			ent->key = KEY_UNLOCKED_ENT;
		}
	} else {
		ent->key = KEY_UNLOCKED_ENT;    // otherwise, set the key when this ent finishes spawning
	}

	// default lip of 8 units
	G_SpawnFloat( "lip", "8", &lip );

	// default damage of 2 points
	G_SpawnInt( "dmg", "2", &ent->damage );

	// first position at start
	VectorCopy( ent->s.origin, ent->pos1 );

	VectorCopy( ent->s.angles, angles2 );

	if ( ent->spawnflags & 1 ) {
		angles2[1] -= 90;
	} else {
		angles2[1] += 90;
	}

	// calculate second position
	trap_SetBrushModel( ent, ent->model );
	G_SetMovedir( ent->s.angles, ent->movedir );
	abs_movedir[0] = fabs( ent->movedir[0] );
	abs_movedir[1] = fabs( ent->movedir[1] );
	abs_movedir[2] = fabs( ent->movedir[2] );
	VectorSubtract( ent->r.maxs, ent->r.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	// calculate third position
	G_SetMovedir( angles2, ent->movedir );
	abs_movedir[0] = fabs( ent->movedir[0] );
	abs_movedir[1] = fabs( ent->movedir[1] );
	abs_movedir[2] = fabs( ent->movedir[2] );
	VectorSubtract( ent->r.maxs, ent->r.mins, size );
	distance = DotProduct( abs_movedir, size ) - lip;
	VectorMA( ent->pos2, distance, ent->movedir, ent->pos3 );

	// if "start_open", reverse position 1 and 3
	/*if ( ent->spawnflags & 1 ) {
		vec3_t	temp;

		VectorCopy( ent->pos3, temp );
		VectorCopy( ent->s.origin, ent->pos3 );
		VectorCopy( temp, ent->pos1 );
	}*/

	InitMover( ent );

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = qtrue;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
	ent->think = finishSpawningKeyedMover;
}
// END JOSEPH

/*
===============================================================================

PLAT

===============================================================================
*/

/*
==============
Touch_Plat

Don't allow decent if a living player is on it
===============
*/
void Touch_Plat( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client || other->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	// delay return-to-pos1 by one second
	if ( ent->moverState == MOVER_POS2 ) {
		ent->nextthink = level.time + 1000;
	}
}

/*
==============
Touch_PlatCenterTrigger

If the plat is at the bottom position, start it going up
===============
*/
void Touch_PlatCenterTrigger( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->parent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent->parent, ent, other );
	}
}


/*
================
SpawnPlatTrigger

Spawn a trigger in the middle of the plat's low position
Elevator cars require that the trigger extend through the entire low position,
not just sit on top of it.
================
*/
void SpawnPlatTrigger( gentity_t *ent ) {
	gentity_t   *trigger;
	vec3_t tmin, tmax;

	// the middle trigger will be a thin trigger just
	// above the starting position
	trigger = G_Spawn();
	trigger->touch = Touch_PlatCenterTrigger;
	trigger->r.contents = CONTENTS_TRIGGER;
	trigger->parent = ent;

	tmin[0] = ent->pos1[0] + ent->r.mins[0] + 33;
	tmin[1] = ent->pos1[1] + ent->r.mins[1] + 33;
	tmin[2] = ent->pos1[2] + ent->r.mins[2];

	tmax[0] = ent->pos1[0] + ent->r.maxs[0] - 33;
	tmax[1] = ent->pos1[1] + ent->r.maxs[1] - 33;
	tmax[2] = ent->pos1[2] + ent->r.maxs[2] + 8;

	if ( tmax[0] <= tmin[0] ) {
		tmin[0] = ent->pos1[0] + ( ent->r.mins[0] + ent->r.maxs[0] ) * 0.5;
		tmax[0] = tmin[0] + 1;
	}
	if ( tmax[1] <= tmin[1] ) {
		tmin[1] = ent->pos1[1] + ( ent->r.mins[1] + ent->r.maxs[1] ) * 0.5;
		tmax[1] = tmin[1] + 1;
	}

	VectorCopy( tmin, trigger->r.mins );
	VectorCopy( tmax, trigger->r.maxs );

	trap_LinkEntity( trigger );
}


/*QUAKED func_plat (0 .5 .8) ?
Plats are always drawn in the extended position so they will light correctly.

"lip"		default 8, protrusion above rest position
"height"	total height of movement, defaults to model height
"speed"		overrides default 200.
"dmg"		overrides default 2
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_plat( gentity_t *ent ) {
	float lip, height;

	ent->sound1to2 = ent->sound2to1 = G_SoundIndex( "sound/movers/plats/pt1_strt.wav" );
	ent->soundPos1 = ent->soundPos2 = G_SoundIndex( "sound/movers/plats/pt1_end.wav" );

	VectorClear( ent->s.angles );

	G_SpawnFloat( "speed", "200", &ent->speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "wait", "1", &ent->wait );
	G_SpawnFloat( "lip", "8", &lip );

	ent->wait = 1000;

	// create second position
	trap_SetBrushModel( ent, ent->model );

	if ( !G_SpawnFloat( "height", "0", &height ) ) {
		height = ( ent->r.maxs[2] - ent->r.mins[2] ) - lip;
	}

	// pos1 is the rest (bottom) position, pos2 is the top
	VectorCopy( ent->s.origin, ent->pos2 );
	VectorCopy( ent->pos2, ent->pos1 );
	ent->pos1[2] -= height;

	InitMover( ent );

	// touch function keeps the plat from returning while
	// a live player is standing on it
	ent->touch = Touch_Plat;

	ent->blocked = Blocked_Door;

	ent->parent = ent;  // so it can be treated as a door

	// spawn the trigger if one hasn't been custom made
	if ( !ent->targetname ) {
		SpawnPlatTrigger( ent );
	}
}


/*
===============================================================================

BUTTON

===============================================================================
*/

/*
==============
Touch_Button

===============
*/
void Touch_Button( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( !other->client ) {
		return;
	}

	if ( ent->moverState == MOVER_POS1 ) {
		Use_BinaryMover( ent, other, other );
	}
}


/*QUAKED func_button (0 .5 .8) ? x x x TOUCH x x STAYOPEN
When a button is touched, it moves some distance in the direction of it's angle, triggers all of it's targets, waits some time, then returns to it's original position where it can be triggered again.

"model2"	.md3 model to also draw
"angle"		determines the opening direction
"target"	all entities with a matching targetname will be used
"speed"		override the default 40 speed
"wait"		override the default 1 second wait (-1 = never return)
"lip"		override the default 4 pixel lip remaining at end of move
"health"	if set, the button must be killed instead of touched
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_button( gentity_t *ent ) {
	vec3_t abs_movedir;
	float distance;
	vec3_t size;
	float lip;

	ent->sound1to2 = G_SoundIndex( "sound/movers/switches/butn2.wav" );

	if ( !ent->speed ) {
		ent->speed = 40;
	}

	if ( !ent->wait ) {
		ent->wait = 1;
	}
	ent->wait *= 1000;

	// first position
	VectorCopy( ent->s.origin, ent->pos1 );

	// calculate second position
	trap_SetBrushModel( ent, ent->model );

	G_SpawnFloat( "lip", "4", &lip );

	G_SetMovedir( ent->s.angles, ent->movedir );
	abs_movedir[0] = fabs( ent->movedir[0] );
	abs_movedir[1] = fabs( ent->movedir[1] );
	abs_movedir[2] = fabs( ent->movedir[2] );
	VectorSubtract( ent->r.maxs, ent->r.mins, size );
	distance = abs_movedir[0] * size[0] + abs_movedir[1] * size[1] + abs_movedir[2] * size[2] - lip;
	VectorMA( ent->pos1, distance, ent->movedir, ent->pos2 );

	if ( ent->health ) {
		// shootable button
		ent->takedamage = qtrue;
	} else if ( ent->spawnflags & 8 ) {
		// touchable button
		ent->touch = Touch_Button;
	}

	InitMover( ent );
}



/*
===============================================================================

TRAIN

===============================================================================
*/


#define TRAIN_START_ON      1
#define TRAIN_TOGGLE        2
#define TRAIN_BLOCK_STOPS   4

/*
===============
Think_BeginMoving

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving( gentity_t *ent ) {
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
}

/*
===============
Reached_Train
===============
*/
void Reached_Train( gentity_t *ent ) {
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
		return;
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

	if ( !strcmp( ent->classname, "func_bats" ) && next->radius ) {
		ent->radius = next->radius;
	}

	// calculate duration
	VectorSubtract( ent->pos2, ent->pos1, move );
	length = VectorLength( move );

	ent->s.pos.trDuration = length * 1000 / speed;
	ent->gDuration = ent->s.pos.trDuration;

	// looping sound
	ent->s.loopSound = next->soundLoop;

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	// if there is a "wait" value on the target, don't start moving yet
	if ( next->wait ) {
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving;
		ent->s.pos.trType = TR_STATIONARY;
	}
}


/*
===============
Think_SetupTrainTargets

Link all the corners together
===============
*/
void Think_SetupTrainTargets( gentity_t *ent ) {
	gentity_t       *path, *next, *start;

	ent->nextTrain = G_Find( NULL, FOFS( targetname ), ent->target );
	if ( !ent->nextTrain ) {
		G_Printf( "func_train at %s with an unfound target\n",
				  vtos( ent->r.absmin ) );
		return;
	}

	start = NULL;

	if ( ent->s.eType == ET_BAT ) { // NOTE: this is odd.  it will never get hit.  remove?  what did it do?
		for ( path = ent->nextTrain ; path != start ; path = next ) {

			if ( !start ) {
				start = path;
			}

			if ( !path->target ) {
				G_Printf( "Train corner at %s without a target\n",
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
					G_Printf( "Train corner at %s without a target path_corner\n",
							  vtos( path->s.origin ) );
					return;
				}
			} while ( strcmp( next->classname, "path_corner" ) );

			path->nextTrain = next;
		}
	} else
	{
		for ( path = ent->nextTrain ; !path->nextTrain ; path = next ) {

			if ( !start ) {
				start = path;
			}

			if ( !path->target ) {
				G_Printf( "Train corner at %s without a target\n",
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
					G_Printf( "Train corner at %s without a target path_corner\n",
							  vtos( path->s.origin ) );
					return;
				}
			} while ( strcmp( next->classname, "path_corner" ) );

			path->nextTrain = next;
		}
	}

	if ( !Q_stricmp( ent->classname, "func_train" ) && ent->spawnflags & 2 ) { // TOGGLE
		VectorCopy( ent->nextTrain->s.origin, ent->s.pos.trBase );
		VectorCopy( ent->nextTrain->s.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	} else if ( !Q_stricmp( ent->classname, "func_train_particles" ) && ent->spawnflags & 2 )       { // TOGGLE
		VectorCopy( ent->nextTrain->s.origin, ent->s.pos.trBase );
		VectorCopy( ent->nextTrain->s.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	} else if ( !Q_stricmp( ent->classname, "func_tramcar" ) && ent->spawnflags & 2 )       { // TOGGLE
		VectorCopy( ent->nextTrain->s.origin, ent->s.pos.trBase );
		VectorCopy( ent->nextTrain->s.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	} else if ( !Q_stricmp( ent->classname, "func_bat" ) )       {
		//VectorCopy (ent->nextTrain->s.origin, ent->s.pos.trBase);
		//VectorCopy (ent->nextTrain->s.origin, ent->r.currentOrigin);
		//trap_LinkEntity (ent);
		if ( ent->spawnflags & 1 ) {  // start on
			ent->use( ent, ent, ent );
		}
	} else if ( !Q_stricmp( ent->classname, "truck_cam" ) && ent->spawnflags & 2 )     { // TOGGLE
		VectorCopy( ent->nextTrain->s.origin, ent->s.pos.trBase );
		VectorCopy( ent->nextTrain->s.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	} else
	{
		if ( !Q_stricmp( ent->classname, "func_tramcar" ) ) {
			Reached_Tramcar( ent );
		} else if ( !Q_stricmp( ent->classname, "truck_cam" ) ) {
			Reached_Tramcar( ent );
		} else if ( !Q_stricmp( ent->classname, "camera_cam" ) ) {
			Reached_Tramcar( ent );
		} else {
			Reached_Train( ent );
		}
	}
}



/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) STOP END REVERSE
Train path corners.
Target: next path corner and other targets to fire
"speed" speed to move to the next corner
"wait" seconds to wait before behining move to next corner

"count2" used only in conjunction with the truck_cam to control playing of gear changes
*/
void SP_path_corner( gentity_t *self ) {
	if ( !self->targetname ) {
		G_Printf( "path_corner with no targetname at %s\n", vtos( self->s.origin ) );
		G_FreeEntity( self );
		return;
	}
	// path corners don't need to be linked in

	if ( self->wait == -1 ) {
		self->count = 1;
	}
}



/*QUAKED func_train (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.
"model2"	.md3 model to also draw
"speed"		default 100
"dmg"		default	2
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_train( gentity_t *self ) {
	VectorClear( self->s.angles );

	if ( self->spawnflags & TRAIN_BLOCK_STOPS ) {
		self->damage = 0;
		self->s.eFlags |= EF_MOVER_STOP;
	} else {
		if ( !self->damage ) {
			self->damage = 2;
		}
	}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		G_Printf( "func_train without a target at %s\n", vtos( self->r.absmin ) );
		G_FreeEntity( self );
		return;
	}

	trap_SetBrushModel( self, self->model );
	InitMover( self );

	self->reached = Reached_Train;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;

	self->blocked = Blocked_Door;

}

// Rafael - bats
/*QUAKED func_train_particles ( 0.3 0.1 0.8) ? START_ON TOGGLE
health = default 16 bats
*/
void Func_train_particles_reached( gentity_t *self ) {
	gentity_t *tent;
	vec3_t vec, ang;
	vec3_t forward;

	Reached_Train( self );

	if ( self->nextTrain->wait == -1 && self->nextTrain->count ) {
		return;
	}

	if ( !self->count ) {
		tent = G_TempEntity( self->r.currentOrigin, EV_BATS );
		tent->s.time = self->speed;
		tent->s.density = self->health;
		VectorCopy( self->r.currentOrigin, tent->s.origin );
		VectorSubtract( self->nextTrain->s.origin, self->r.currentOrigin, vec );
		vectoangles( vec, ang );
		AngleVectors( ang, forward, NULL, NULL );
		VectorCopy( forward, tent->s.angles );
		self->count = 1;
	} else
	{
		tent = G_TempEntity( self->r.currentOrigin, EV_BATS_UPDATEPOSITION );
		tent->s.time = self->speed;
		VectorCopy( self->r.currentOrigin, tent->s.origin );
		VectorSubtract( self->nextTrain->s.origin, self->r.currentOrigin, vec );
		vectoangles( vec, ang );
		AngleVectors( ang, forward, NULL, NULL );
		VectorCopy( forward, tent->s.angles );
	}

	tent->s.frame = self->s.number;
	trap_LinkEntity( self );

}

void SP_func_train_particles( gentity_t *self ) {
	SP_func_train( self );
	self->reached = Func_train_particles_reached;
	self->blocked = NULL;

	self->damage = 0;

	if ( !self->health ) {
		self->health = 16;
	}

	if ( !self->speed ) {
		self->speed = 50;
	}
}

// RF, bats v2.0
/*QUAKED func_bats ( 0.3 0.1 0.8) (-32 -32 -32) (32 32 32) START_ON TOGGLE END_OUTER
count = default 10 bats
radius = maximum distance from center of entity to place each bat (default=32)
speed = speed to travel to next waypoint (default=300)
wait = (used for end map) wait seconds in between spawning
target = (used for end map) distance check from this entity to enable spawning if player is more than
	"radius" distance from the target
delay = (end map) wait in seconds this long after player steps outside, before spawning spirits
*/
void FuncBatsReached( gentity_t *self ) {
	if ( self->active == 2 ) {
		self->nextthink = -1;
		self->think = NULL;
		return;
	}

	Reached_Train( self );

	if ( !self->nextTrain || !self->nextTrain->target ) {
		self->active = 2;   // remove the bats at next point
		return;
	}

//	if(self->nextTrain) {
//		if(Q_stricmp(self->nextTrain->classname, "path_corner")) {
//			self->active = 2;
//			return;
//		}
//	}
}

// each bat calls this every server frame, so it moves towards it's ideal position
void BatMoveThink( gentity_t *bat ) {
	gentity_t *owner;
	vec3_t goalpos, vec;
	float speed, dist;
	int i;
//	trace_t		tr;

	owner = &g_entities[bat->r.ownerNum];
	if ( owner->active == qtrue && owner->inuse ) { // move towards the owner
		BG_EvaluateTrajectory( &owner->s.pos, level.time, goalpos );

		// randomize ther movedir as we go
		for ( i = 0; i < 3; i++ )
			bat->movedir[i] += crandom() * (float)owner->radius * 0.1;
		if ( VectorLength( bat->movedir ) > (float)owner->radius ) {
			VectorNormalize( bat->movedir );
			VectorScale( bat->movedir, (float)owner->radius, bat->movedir );
		}
		VectorAdd( goalpos, bat->movedir, goalpos );

		VectorSubtract( goalpos, bat->s.pos.trBase, vec );
		dist = VectorLength( vec );
		speed = dist / 64;
		VectorMA( bat->s.pos.trBase, 0.05 * speed, vec, bat->s.pos.trBase );
		bat->s.pos.trTime = level.time;
		VectorCopy( bat->s.pos.trBase, bat->r.currentOrigin );
		if ( dist * speed > 20 ) {
			vectoangles( vec, bat->s.angles );
		}
		trap_LinkEntity( bat );
/*
		// check for hurting someone
		if (bat->damage < level.time) {
			trap_Trace( &tr, bat->r.currentOrigin, NULL, NULL, bat->r.currentOrigin, bat->s.number, CONTENTS_BODY );
			if (tr.startsolid && tr.entityNum < MAX_CLIENTS && !g_entities[tr.entityNum].aiCharacter) {
				G_Damage( &g_entities[tr.entityNum], bat, bat, vec3_origin, bat->r.currentOrigin, 1+rand()%3, DAMAGE_NO_KNOCKBACK, MOD_BAT );

				// !! TODO: bat biting sound and view feedback

				// don't keep hurting them each time we think
				bat->damage = level.time + 1000;
			}
		}
*/
	} else if ( owner->active == 2 || !owner->inuse ) {
		// owner has finished
		G_FreeEntity( bat );
		return;
	}
	bat->nextthink = level.time + 50;
}

void BatDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	G_AddEvent( self, EV_BATS_DEATH, 0 );
	self->think = G_FreeEntity;
	self->nextthink = level.time + 100;
}

void FuncBatsActivate( gentity_t *self, gentity_t * other, gentity_t * activator ) {
	int i;
	gentity_t *bat;
	vec3_t vec;

	if ( !self->active ) {
		self->active = qtrue;

		// spawn "count" bats
		for ( i = 0; i < self->count; i++ ) {
			bat = G_Spawn();
			bat->classname = "func_bat";
			bat->s.eType = ET_BAT;

			VectorSet( vec, crandom(), crandom(), crandom() );
			VectorNormalize( vec );
			VectorScale( vec, random() * (float)self->radius, bat->movedir );
			VectorAdd( self->s.pos.trBase, bat->movedir, bat->s.pos.trBase );
			bat->s.pos.trTime = level.time;
			VectorClear( bat->s.pos.trDelta );
			VectorCopy( bat->s.pos.trBase, bat->r.currentOrigin );

			bat->r.ownerNum = self->s.number;
			bat->r.contents = 0; //CONTENTS_CORPSE;
			bat->takedamage = qfalse;
			bat->health = 1;
			bat->pain = NULL;
			bat->die = NULL; //BatDie;
			//VectorSet( bat->r.mins, -18, -18, -18 );
			//VectorSet( bat->r.maxs,  18,  18,  18 );

			bat->speed = self->speed;
			bat->radius = self->radius;

			bat->think = BatMoveThink;
			bat->nextthink = level.time + 50;

			trap_LinkEntity( bat );
		}

		InitMover( self );  // start moving
		FuncBatsReached( self );
		self->reached = FuncBatsReached;
		self->blocked = NULL;

		// disable this to debug path
		self->r.svFlags |= SVF_NOCLIENT;
		self->r.contents = 0;

		self->use = FuncBatsActivate;   // make sure this stays the same

	} else {    // second use kills bats
		self->active = 2;
	}
}

void FuncEndSpiritsThink( gentity_t *self ) {
	vec3_t enemyPos;
	gentity_t *cEnt, *heinrich;
	//
	self->nextthink = level.time + (int)( ( 1.5 + 2.0 * random() ) * ( self->wait * 1000 ) );
	//
	if ( !self->active ) {
		return; // we are not allowed to release spirits yet
	}
	//
	// if heinrich isn't active yet, dont spawn
	// TTimo: gcc: suggest parentheses around assignment used as truth value
	if ( ( heinrich = AICast_FindEntityForName( "heinrich" ) ) ) {
		if ( heinrich->aiInactive ) {
			return;
		}
		if ( heinrich->health <= 0 ) {
			return;
		}
		if ( heinrich->s.aiState < AISTATE_COMBAT ) {
			return;
		}
		if ( !g_entities[0].client || g_entities[0].client->cameraPortal ) {
			return;
		}
	} else {    // no heinrich?
		return;
	}
	//
	// if the player is close enough, and outside arena, spawn a spirit
	VectorCopy( g_entities[0].s.pos.trBase, enemyPos );
	cEnt = G_Find( NULL, FOFS( targetname ), self->target );
	if ( !cEnt ) {
		G_Error( "couldnt find center marker for spirit spawner" );
	}
	if ( VectorDistance( enemyPos, cEnt->s.origin ) > self->radius ) {
		// also make sure the player is between us and the center entity
		if ( VectorDistance( self->s.origin, enemyPos ) < VectorDistance( self->s.origin, cEnt->s.origin ) ) {
			// if we are not "spawning" then set the delay
			if ( !self->botDelayBegin ) {
				self->botDelayBegin = qtrue;
				// set the delay before we start spawning them
				self->nextthink = level.time + (int)( self->delay * 1000.0 );
			} else {
				G_AddEvent( self, EV_SPAWN_SPIRIT, 0 );
			}
		} else {
			self->botDelayBegin = qfalse;
		}
	} else {
		self->botDelayBegin = qfalse;
	}
}

void SP_func_bats( gentity_t *self ) {
	if ( !self->count ) {
		self->count = 10;
	}

	if ( !self->radius ) {
		self->radius = 32;
	}

	if ( !self->speed ) {
		self->speed = 300;
	}

	// setup train waypoints
	self->active = qfalse;
	self->use = FuncBatsActivate;

	self->damage = 0;

	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets;

	// disable this to debug path
	self->r.svFlags |= SVF_NOCLIENT;
	self->r.contents = 0;

	if ( self->spawnflags & 4 ) { // END spirit spawners
		self->r.svFlags &= ~SVF_NOCLIENT;
		self->r.svFlags |= SVF_BROADCAST;
		self->s.eFlags |= EF_NODRAW;
		self->s.eType = ET_SPIRIT_SPAWNER;
		self->s.otherEntityNum2 = 0;    // HACK: point to the player
		self->s.time = (int)( self->delay * 1000 );
		self->use = NULL;
		self->botDelayBegin = qfalse;
		//
		self->think = FuncEndSpiritsThink;
		self->nextthink = level.time + ( self->wait * 1000 );
		//
		self->r.contents = 0;
		trap_LinkEntity( self );
	}
}

// JOSEPH 9-27-99
/*
===============
Think_BeginMoving_rotating

The wait time at a corner has completed, so start moving again
===============
*/
void Think_BeginMoving_rotating( gentity_t *ent ) {
	ent->s.pos.trTime = level.time;
	ent->s.pos.trType = TR_LINEAR_STOP;
}

/*
===============
Reached_Train_rotating
===============
*/
void Reached_Train_rotating( gentity_t *ent ) {
	gentity_t       *next;
	float speed;
	vec3_t move;
	float length;
	float frames;

	// copy the apropriate values
	next = ent->nextTrain;
	if ( !next || !next->nextTrain ) {
		return;     // just stop
	}

	// fire all other targets
	G_UseTargets( next, NULL );

	// set the new trajectory
	ent->nextTrain = next->nextTrain;
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

	ent->rotate[0] = next->rotate[2];
	ent->rotate[1] = next->rotate[0];
	ent->rotate[2] = next->rotate[1];

	// calculate duration
	VectorSubtract( ent->pos2, ent->pos1, move );
	length = VectorLength( move );

	if ( next->duration ) {
		ent->s.pos.trDuration = ( next->duration * 1000 );
	} else {
		ent->s.pos.trDuration = length * 1000 / speed;
	}

	// Rotate the train
	frames = floor( ent->s.pos.trDuration / 100 );

	if ( !frames ) {
		frames = 0.001;
	}

	ent->s.apos.trType = TR_LINEAR;

	if ( ent->TargetFlag ) {
		VectorCopy( ent->TargetAngles, ent->r.currentAngles );
		VectorCopy( ent->r.currentAngles, ent->s.angles );
		VectorCopy( ent->s.angles, ent->s.apos.trBase );
		ent->TargetFlag = 0;
	}

	//G_Printf( "Train angles %s\n",
	//			vtos(ent->s.angles) );

	//G_Printf( "Add  X  Y  X %s\n",
	//			vtos(ent->rotate) );

	// X
	if ( ent->rotate[2] ) {
		ent->s.apos.trDelta[2] = ( ent->rotate[2] / frames ) * 10;
	} else {
		ent->s.apos.trDelta[2] = 0;
	}
	// Y
	if ( ent->rotate[0] ) {
		ent->s.apos.trDelta[0] = ( ent->rotate[0] / frames ) * 10;
	} else {
		ent->s.apos.trDelta[0] = 0;
	}
	// Z
	if ( ent->rotate[1] ) {
		ent->s.apos.trDelta[1] = ( ent->rotate[1] / frames ) * 10;
	} else {
		ent->s.apos.trDelta[1] = 0;
	}

	// looping sound
	ent->s.loopSound = next->soundLoop;

	ent->TargetFlag = 1;
	ent->TargetAngles[0] = ent->r.currentAngles[0] + ent->rotate[0];
	//ent->TargetAngles[0] = AngleNormalize360 (ent->TargetAngles[0]);
	ent->TargetAngles[1] = ent->r.currentAngles[1] + ent->rotate[1];
	//ent->TargetAngles[1] = AngleNormalize360 (ent->TargetAngles[1]);
	ent->TargetAngles[2] = ent->r.currentAngles[2] + ent->rotate[2];
	//ent->TargetAngles[2] = AngleNormalize360 (ent->TargetAngles[2]);

	// start it going
	SetMoverState( ent, MOVER_1TO2, level.time );

	// if there is a "wait" value on the target, don't start moving yet
	if ( next->wait ) {
		ent->nextthink = level.time + next->wait * 1000;
		ent->think = Think_BeginMoving_rotating;
		ent->s.pos.trType = TR_STATIONARY;
	}
}

/*
===============
Think_SetupTrainTargets_rotating

Link all the corners together
===============
*/
void Think_SetupTrainTargets_rotating( gentity_t *ent ) {
	gentity_t       *path, *next, *start;


	ent->nextTrain = G_Find( NULL, FOFS( targetname ), ent->target );
	if ( !ent->nextTrain ) {
		G_Printf( "func_train at %s with an unfound target\n",
				  vtos( ent->r.absmin ) );
		return;
	}

	VectorCopy( ent->s.angles, ent->s.apos.trBase );
	VectorCopy( ent->s.angles, ent->TargetAngles );
	ent->TargetFlag = 1;

	start = NULL;
	for ( path = ent->nextTrain ; path != start ; path = next ) {
		if ( !start ) {
			start = path;
		}

		if ( !path->target ) {
			G_Printf( "Train corner at %s without a target\n",
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
				G_Printf( "Train corner at %s without a target path_corner\n",
						  vtos( path->s.origin ) );
				return;
			}
		} while ( strcmp( next->classname, "path_corner" ) );

		path->nextTrain = next;
	}

	// start the train moving from the first corner
	Reached_Train_rotating( ent );
}

/*QUAKED func_train_rotating (0 .5 .8) ? START_ON TOGGLE BLOCK_STOPS
A train is a mover that moves between path_corner target points.
This train can also rotate along the X Y Z
Trains MUST HAVE AN ORIGIN BRUSH.
The train spawns at the first target it is pointing at.

"model2"	.md3 model to also draw
"dmg"		default	2
"speed"		default 100
"noise"		looping sound to play when the train is in motion
"target"	next path corner
"color"		constantLight color
"light"		constantLight radius

On the path corner:
speed    departure speed from that corner
rotate   angle change for X Y Z to next corner
duration duration for angle change (overrides speed)
*/

void SP_func_train_rotating( gentity_t *self ) {
	VectorClear( self->s.angles );

	if ( self->spawnflags & TRAIN_BLOCK_STOPS ) {
		self->damage = 0;
	} else {
		if ( !self->damage ) {
			self->damage = 2;
		}
	}

	if ( !self->speed ) {
		self->speed = 100;
	}

	if ( !self->target ) {
		G_Printf( "func_train without a target at %s\n", vtos( self->r.absmin ) );
		G_FreeEntity( self );
		return;
	}

	trap_SetBrushModel( self, self->model );
	InitMover( self );

	self->reached = Reached_Train_rotating;

	// start trains on the second frame, to make sure their targets have had
	// a chance to spawn
	self->nextthink = level.time + FRAMETIME;
	self->think = Think_SetupTrainTargets_rotating;
}
// END JOSEPH

/*
===============================================================================

STATIC

===============================================================================
*/

/*
==============
Use_Static
	toggle hide or show (including collisions) this entity
==============
*/
void Use_Static( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
		// DISABLED since func_static will carve up AAS anyway, so blocking makes no sense
		// RF, AAS areas are now free
		//if (ent->model)
		//	G_SetAASBlockingEntity( ent, qfalse );
	} else {
		trap_LinkEntity( ent );
		// DISABLED since func_static will carve up AAS anyway, so blocking makes no sense
		// RF, AAS areas are now occupied
		//if (ent->model)
		//	G_SetAASBlockingEntity( ent, qtrue );
	}
}

void Static_Pain( gentity_t *ent, gentity_t *attacker, int damage, vec3_t point ) {
	vec3_t temp;

	if ( ent->spawnflags & 4 ) {
		if ( level.time > ent->wait + ent->delay + rand() % 1000 + 500 ) {
			ent->wait = level.time;
		} else {
			return;
		}

		// TBD only venom mg42 rocket and grenade can inflict damage
		if ( attacker && attacker->client
			 && ( attacker->s.weapon == WP_VENOM
				  || attacker->s.weapon == WP_GRENADE_LAUNCHER
				  || attacker->client->ps.persistant[PERS_HWEAPON_USE] ) ) {

			VectorCopy( ent->r.currentOrigin, temp );
			VectorCopy( ent->pos3, ent->r.currentOrigin );
			Spawn_Shard( ent, attacker, 3, ent->count );
			VectorCopy( temp, ent->r.currentOrigin );
		}
		return;
	}

	if ( level.time > ent->wait + ent->delay + rand() % 1000 + 500 ) {
		G_UseTargets( ent, NULL );
		ent->wait = level.time;
	}

}

void G_BlockThink( gentity_t *ent ) {
	if ( ent->r.linked ) {
		G_SetAASBlockingEntity( ent, qtrue );
	} else {
		G_SetAASBlockingEntity( ent, qfalse );
	}
}


/*QUAKED func_leaky (0 .5 .8) ?
"leaktype" - leaks particles of this type ("type" is equiv in this ent)
"leakpressure" - force the particles come out with.  '0' would cause them to fall straight down
"leaktime" - how long it leaks before it quits
"leakcount" - how many holes in the entity before it no longer leaks.

1:oil
2:water
3:steam
4:wine
5:smoke
6:electrical


*/

void SP_func_leaky( gentity_t *ent ) {

	if ( ent->model2 ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}
	trap_SetBrushModel( ent, ent->model );
	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// (SA) this is not ideal, but gets us finished.
	G_SpawnInt( "type", "0", &ent->emitID );
	if ( !ent->emitID ) {
		G_SpawnInt( "leaktype", "0", &ent->emitID );
	}

	G_SpawnInt( "leakpressure", "30", &ent->emitPressure );

// hacks
	if ( ent->emitID == 2 ) {  // no water
		ent->emitID = 3;    // make it steam

	}
	if ( ent->emitID == 3 ) {  // steam
		ent->emitPressure = 100;
	}

// end hacks

	G_SpawnInt( "leaktime", "10", &ent->emitTime );
	ent->emitTime *= 1000;  // make ms
	G_SpawnInt( "leakcount", "10", &ent->emitNum );
	ent->s.eType = ET_LEAKY;
	trap_LinkEntity( ent );
}


/*QUAKED func_static (0 .5 .8) ? start_invis pain painEFX
A bmodel that just sits there, doing nothing.  Can be used for conditional walls and models.
"model2"	.md3 model to also draw
"color"		constantLight color
"light"		constantLight radius
"start_invis" will start the entity as non-existant
If targeted, it will toggle existance when triggered

pain will use its target

When using pain you will need to specify the delay time
value of 1 = 1 sec 2 = 2 sec so on...
default is 1 sec you can use decimals
example :
delay
1.27

painEFX will spawn a shards
example:
shard
4
will spawn rubble

shard default is 4

shard =
shard_glass = 0,
shard_wood = 1,
shard_metal = 2,
shard_ceramic = 3,
shard_pebbles = 4
*/
void SP_func_static( gentity_t *ent ) {
	if ( ent->model2 ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model2 );
	}
	trap_SetBrushModel( ent, ent->model );
	InitMover( ent );
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->use = Use_Static;

	if ( ent->spawnflags & 1 ) {
		trap_UnlinkEntity( ent );
	}

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = qtrue;
		}
	}

	if ( ent->spawnflags & 2 || ent->spawnflags & 4 ) {
		ent->pain = Static_Pain;

		if ( !ent->delay ) {
			ent->delay = 1000;
		} else {
			ent->delay *= 1000;
		}

		ent->takedamage = qtrue;

		ent->isProp = qtrue;

		ent->health = 9999;

		if ( !( ent->count ) ) {
			ent->count = 4;
		}
	}

	// DISABLED since func_static will carve up AAS anyway, so blocking makes no sense
	/*
	// RF, check for blocking AAS
	if ( ent->spawnflags & 1 ) {
		// RF, AAS areas are now occupied
		if (ent->model) {
			ent->think = G_BlockThink;
			ent->nextthink = level.time + FRAMETIME;
		}
	}
	*/
}


/*
===============================================================================

ROTATING

===============================================================================
*/


/*QUAKED func_rotating (0 .5 .8) ? START_ON STARTINVIS X_AXIS Y_AXIS
You need to have an origin brush as part of this entity.
The center of that brush will be the point around which it is rotated. It will rotate around the Z axis by default.  You can check either the X_AXIS or Y_AXIS box to change that.

"model2"	.md3 model to also draw
"speed"		determines how fast it moves; default value is 100.
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/

void Use_Func_Rotate( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	if ( ent->spawnflags & 4 ) {
		ent->s.apos.trDelta[2] = ent->speed;
	} else if ( ent->spawnflags & 8 )   {
		ent->s.apos.trDelta[0] = ent->speed;
	} else {
		ent->s.apos.trDelta[1] = ent->speed;
	}

	if ( ent->spawnflags & 2 ) {
		ent->flags &= ~FL_TEAMSLAVE;
	}

	trap_LinkEntity( ent );
}

void SP_func_rotating( gentity_t *ent ) {
	if ( !ent->speed ) {
		ent->speed = 100;
	}

	// set the axis of rotation
	ent->s.apos.trType = TR_LINEAR;

	if ( ent->spawnflags & 1 ) {
		if ( ent->spawnflags & 4 ) {
			ent->s.apos.trDelta[2] = ent->speed;
		} else if ( ent->spawnflags & 8 ) {
			ent->s.apos.trDelta[0] = ent->speed;
		} else {
			ent->s.apos.trDelta[1] = ent->speed;
		}
	}

	if ( !ent->damage ) {
		ent->damage = 2;
	}

	trap_SetBrushModel( ent, ent->model );
	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );
	VectorCopy( ent->s.apos.trBase, ent->r.currentAngles );

	if ( ent->spawnflags & 2 ) {
		ent->flags |= FL_TEAMSLAVE;
		trap_UnlinkEntity( ent );
	} else {
		trap_LinkEntity( ent );
	}

}


/*
===============================================================================

BOBBING

===============================================================================
*/


/*QUAKED func_bobbing (0 .5 .8) ? X_AXIS Y_AXIS
Normally bobs on the Z axis
"model2"	.md3 model to also draw
"height"	amplitude of bob (32 default)
"speed"		seconds to complete a bob cycle (4 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_bobbing( gentity_t *ent ) {
	float height;
	float phase;

	G_SpawnFloat( "speed", "4", &ent->speed );
	G_SpawnFloat( "height", "32", &height );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	trap_SetBrushModel( ent, ent->model );
	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	ent->s.pos.trDuration = ent->speed * 1000;
	ent->s.pos.trTime = ent->s.pos.trDuration * phase;
	ent->s.pos.trType = TR_SINE;

	// set the axis of bobbing
	if ( ent->spawnflags & 1 ) {
		ent->s.pos.trDelta[0] = height;
	} else if ( ent->spawnflags & 2 ) {
		ent->s.pos.trDelta[1] = height;
	} else {
		ent->s.pos.trDelta[2] = height;
	}
}

/*
===============================================================================

PENDULUM

===============================================================================
*/


/*QUAKED func_pendulum (0 .5 .8) ?
You need to have an origin brush as part of this entity.
Pendulums always swing north / south on unrotated models.  Add an angles field to the model to allow rotation in other directions.
Pendulum frequency is a physical constant based on the length of the beam and gravity.
"model2"	.md3 model to also draw
"speed"		the number of degrees each way the pendulum swings, (30 default)
"phase"		the 0.0 to 1.0 offset in the cycle to start at
"dmg"		damage to inflict when blocked (2 default)
"color"		constantLight color
"light"		constantLight radius
*/
void SP_func_pendulum( gentity_t *ent ) {
	float freq;
	float length;
	float phase;
	float speed;

	G_SpawnFloat( "speed", "30", &speed );
	G_SpawnInt( "dmg", "2", &ent->damage );
	G_SpawnFloat( "phase", "0", &phase );

	trap_SetBrushModel( ent, ent->model );

	// find pendulum length
	length = fabs( ent->r.mins[2] );
	if ( length < 8 ) {
		length = 8;
	}

	freq = 1 / ( M_PI * 2 ) * sqrt( g_gravity.value / ( 3 * length ) );

	ent->s.pos.trDuration = ( 1000 / freq );

	InitMover( ent );

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	VectorCopy( ent->s.angles, ent->s.apos.trBase );

	ent->s.apos.trDuration = 1000 / freq;
	ent->s.apos.trTime = ent->s.apos.trDuration * phase;
	ent->s.apos.trType = TR_SINE;
	ent->s.apos.trDelta[2] = speed;
}

/*QUAKED func_door_rotating (0 .5 .8) ? - TOGGLE X_AXIS Y_AXIS REVERSE FORCE STAYOPEN
You need to have an origin brush as part of this entity.
The center of that brush will be the point around which it is rotated. It will rotate around the Z axis by default.  You can check either the X_AXIS or Y_AXIS box to change that (only one axis allowed. If both X and Y are checked, the default of Z will be used).
FORCE		door opens even if blocked

"key"       -1 for locked, key number for which key opens, 0 for open.  default '0' unless door is targeted.  (trigger_aidoor entities targeting this door do /not/ affect the key status)
"model2"    .md3 model to also draw
"degrees"   determines how many degrees it will turn (90 default)
"speed"	    movement speed (100 default)
"closespeed" optional different movement speed for door closing
"time"      how many milliseconds it will take to open 1 sec = 1000
"dmg"       damage to inflict when blocked (2 default)
"color"     constantLight color
"light"     constantLight radius
"type"		use sounds based on construction of door:
	 0 - nosound (default)
	 1 - metal
	 2 - stone
	 3 - lab
	 4 - wood
	 5 - iron/jail
	 6 - portcullis
	 7 - wood (quiet)
"team"		team name.  other doors with same team name will open/close in syncronicity
*/




//
//
void SP_func_door_rotating( gentity_t *ent ) {
	int key, doortype;

	G_SpawnInt( "type", "0", &doortype );

	if ( doortype ) {  // /*why on earthy did this check for <=8?*/ && doortype <= 8)	// no doortype = silent
		DoorSetSounds( ent, doortype, qtrue );
		if ( doortype == 5 ) { // iron/jail always makes same noise
			ent->flags |= FL_DOORNOISE;
		}
	}


	// set the duration
	if ( !ent->speed ) {
		ent->speed = 1000;
	}

	// degrees door will open
	if ( !ent->angle ) {
		ent->angle = 90;
	}

	// reverse direction
	if ( ent->spawnflags & 16 ) {
		ent->angle *= -1;
	}

	// TOGGLE
	if ( ent->spawnflags & 2 ) {
		ent->flags |= FL_TOGGLE;
	}

	if ( G_SpawnInt( "key", "", &key ) ) {    // if door has a key entered, set it
		ent->key = key;

		if ( key == -1 ) {
			ent->key = KEY_LOCKED_ENT;
		} else if ( ent->key > KEY_NUM_KEYS || ent->key < KEY_NONE ) {            // if the key is invalid, set the key in the finishSpawning routine
			G_Error( "invalid key (%d) set for func_door_rotating\n", ent->key );
			ent->key = KEY_UNLOCKED_ENT;
		}
	} else {
		ent->key = KEY_UNLOCKED_ENT;    // otherwise, set the key when this ent finishes spawning
	}


	// set the rotation axis
	VectorClear( ent->rotate );
	if      ( ent->spawnflags & 4 ) {
		ent->rotate[2] = 1;
	} else if ( ent->spawnflags & 8 ) {
		ent->rotate[0] = 1;
	} else { ent->rotate[1] = 1;}

	if ( VectorLength( ent->rotate ) > 1 ) { // check that rotation is only set for one axis
		G_Error( "Too many axis marked in func_door_rotating entity.  Only choose one axis of rotation. (defaulting to standard door rotation)" );
		VectorClear( ent->rotate );
		ent->rotate[1] = 1;
	}

	if ( !ent->wait ) {
		ent->wait = 2;
	}
	ent->wait *= 1000;

	//if (!ent->damage) {
	//	ent->damage = 2;
	//}

	trap_SetBrushModel( ent, ent->model );

	InitMoverRotate( ent );

	if ( !( ent->flags & FL_TEAMSLAVE ) ) {
		int health;

		G_SpawnInt( "health", "0", &health );
		if ( health ) {
			ent->takedamage = qtrue;
		}
	}

	ent->nextthink = level.time + FRAMETIME;
	ent->think = finishSpawningKeyedMover;

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );
	VectorCopy( ent->s.apos.trBase, ent->r.currentAngles );

	ent->blocked = Blocked_DoorRotate;

	trap_LinkEntity( ent );
}




/*
===============================================================================

EFFECTS

  I'm keeping all this stuff in here just to avoid collisions with Raf right now in g_misc or g_props
  Will move.
===============================================================================
*/

/*
==============
use_target_effect
==============
*/
void use_target_effect( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	gentity_t   *tent;

	tent = G_TempEntity( self->r.currentOrigin, EV_EFFECT );
	VectorCopy( self->r.currentOrigin, tent->s.origin );
	VectorCopy( self->r.currentOrigin, tent->s.origin2 );
	if ( self->spawnflags & 32 ) {
		tent->s.dl_intensity = 1;   // low grav
	} else {
		tent->s.dl_intensity = 0;
	}

	trap_SetConfigstring( CS_TARGETEFFECT, self->dl_shader );   //----(SA)	allow shader to be set from entity

	// (SA) this should match the values from func_explosive
	tent->s.frame = self->key;      // pass the type to the client ("glass", "wood", "metal", "gibs", "brick", "stone", "fabric", 0, 1, 2, 3, 4, 5, 6)

	tent->s.eventParm       = self->spawnflags;
	tent->s.density         = self->health;
	// RF, added this 9/30/01, seems to be needed by CG_Explodef()
	tent->s.effect3Time     = self->key;            // pass the type to the client ("glass", "wood", "metal", "gibs", "brick", "stone", "fabric", 0, 1, 2, 3, 4, 5, 6)
	// RF, specify player damage
	if ( self->spawnflags & 128 ) {
		tent->s.teamNum = 1;
	} else {
		tent->s.teamNum = 0;
	}

	if ( self->damage ) {
		G_RadiusDamage( self->s.pos.trBase, self, self->damage, self->damage, self, MOD_EXPLOSIVE );
	}

	G_UseTargets( self, other );
}


/*QUAKED target_effect (0 .5 .8) (-6 -6 -6) (6 6 6) TNT explode smoke rubble gore lowgrav debris player_damage
"mass" defaults to 15.  This determines how much debris is emitted when it explodes.  (number of pieces)
"dmg" defaults to 0.  damage radius blast when triggered
"type" - if 'rubble' is specified, this is the model type ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
*/
void SP_target_effect( gentity_t *ent ) {
	int mass;
	char    *type;

	ent->use = use_target_effect;

	if ( G_SpawnInt( "mass", "15", &mass ) ) {
		ent->health = mass;
	} else {
		ent->health = 15;
	}

	// (SA) this should match the values from func_explosive
	if ( G_SpawnString( "type", "wood", &type ) ) {
		if ( !Q_stricmp( type,"wood" ) ) {
			ent->key = 0;
		} else if ( !Q_stricmp( type,"glass" ) ) {
			ent->key = 1;
		} else if ( !Q_stricmp( type,"metal" ) )                                                       {
			ent->key = 2;
		} else if ( !Q_stricmp( type,"gibs" ) )                                                                                                               {
			ent->key = 3;
		} else if ( !Q_stricmp( type,"brick" ) )                                                                                                                                                                      {
			ent->key = 4;
		} else if ( !Q_stricmp( type,"rock" ) )                                                                                                                                                                                                                              {
			ent->key = 5;
		} else if ( !Q_stricmp( type,"fabric" ) )                                                                                                                                                                                                                                                                                     {
			ent->key = 6;
		}
	} else {
		ent->key = 5;   // default to 'rock'
	}

}



/*
===============================================================================

EXPLOSIVE
  I'm keeping all this stuff in here just to avoid collisions with Raf right now in g_misc or g_props
  Will move.
===============================================================================
*/


/*
==============
ThrowDebris
==============
*/
void ThrowDebris( gentity_t *self, char *modelname, float speed, vec3_t origin ) {
	// probably use le->leType = LE_FRAGMENT like brass and gibs
}

/*
==============
ClearExplosive
	nuke the original entity
==============
*/
void ClearExplosive( gentity_t *self ) {
	// RF, AAS areas are now free
	if ( !( self->spawnflags & 16 ) ) {
		G_SetAASBlockingEntity( self, qfalse );
	}

	self->die   = NULL;
	self->pain  = NULL;
	self->touch = NULL;
	self->use   = NULL;
	self->nextthink = level.time + FRAMETIME;
	self->think = G_FreeEntity;


	G_FreeEntity( self );
}



/*
==============
func_explosive_explode
	NOTE: the 'damage' passed in is ignored completely
==============
*/
void func_explosive_explode( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	vec3_t origin;
	vec3_t size;
	vec3_t dir = {0, 0, 1};
	gentity_t   *tent = 0;
	int timeToDeath;            //----(SA)	added

	self->takedamage = qfalse;          // don't allow anything try to hurt me now that i'm exploding

	self->touch = NULL; //----(SA)	added

	if ( self->wait >= 0 ) {

		self->think = ClearExplosive;

		timeToDeath = (int)( self->wait * 1000.0f ) + FRAMETIME;

		self->nextthink = level.time + timeToDeath; // delay removal until the animation has played out and leave as long as the user requested
		self->s.time = timeToDeath < 3000 ? timeToDeath : self->nextthink - 3000;   // 3 sec fade at end, unless the life time is less than 2 seconds, then fade from now to death
		self->s.time2 = self->nextthink;
	}

	VectorSubtract( self->r.absmax, self->r.absmin, size );
	VectorScale( size, 0.5, size );
	VectorAdd( self->r.absmin, size, origin );

	self->s.frame = 1;  // play death animation

//	VectorCopy(origin, self->s.pos.trBase);
	VectorCopy( origin, self->s.origin2 );    // (SA) changed so i can have the model remain for a bit, but still have the explosion at the 'center'

	G_UseTargets( self, attacker );

	self->s.density = self->count;      // pass the "mass" to the client
	self->s.weapon = self->duration;    // pass the "force lowgrav" to client
	self->s.effect3Time = self->key;            // pass the type to the client ("glass", "wood", "metal", "gibs", "brick", "stone", "fabric", 0, 1, 2, 3, 4, 5, 6)

	if ( self->damage ) {
		G_RadiusDamage( origin, self, self->damage, self->damage + 40, self, MOD_EXPLOSIVE );
	}

	// find target, aim at that
	if ( self->target ) {

		// since the explosive might need to fire the target rather than
		// aim at it, only aim at 'info_notnull' ents
		while ( 1 )
		{
			tent = G_Find( tent, FOFS( targetname ), self->target );
			if ( !tent ) {
				break;
			}

			if ( !Q_stricmp( tent->classname, "info_notnull" ) ) {
				break;  // found an info_notnull
			}
		}

		if ( tent ) {
			VectorSubtract( tent->s.pos.trBase, origin, dir );
			VectorNormalize( dir );
		}
	}

	// if a valid target entity was not found, check for a specified 'angle' for the explosion direction
	if ( !tent && !self->model2 ) {    // if there's a 'model2', the dir is for that, not the explosion
		if ( self->s.angles[1] ) {
			// up
			if ( self->s.angles[1] == -1 ) {
				// it's 'up' by default
			}
			// down
			else if ( self->s.angles[1] == -2 ) {
				dir[2] = -1;
			}
			// yawed
			else
			{
				RotatePointAroundVector( dir, dir, tv( 1, 0, 0 ), self->s.angles[1] );
			}
		}
	}

	G_AddEvent( self, EV_EXPLODE, DirToByte( dir ) );

}

/*
==============
func_explosive_touch
==============
*/
void func_explosive_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
//	func_explosive_explode(self, self, other, self->health, 0);
	func_explosive_explode( self, self, other, self->damage, 0 );
}


/*
==============
func_explosive_use
==============
*/
void func_explosive_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
//	func_explosive_explode (self, self, other, self->health, 0);
	func_explosive_explode( self, self, other, self->damage, 0 );
}

/*
==============
func_explosive_alert
==============
*/
void func_explosive_alert( gentity_t *self ) {
	func_explosive_explode( self, self, self, self->damage, 0 );
}

/*
==============
func_explosive_spawn
==============
*/
void func_explosive_spawn( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	trap_LinkEntity( self );
	self->use = func_explosive_use;
	// turn the brush to visible

	// RF, AAS areas are now occupied
	if ( !( self->spawnflags & 16 ) ) {
		G_SetAASBlockingEntity( self, qtrue );
	}
}





/*
==============
InitExplosive
==============
*/
void InitExplosive( gentity_t *ent ) {
	char        *damage;

	// if the "model2" key is set, use a seperate model
	// for drawing, but clip against the brushes
	if ( ent->model2 ) {
		int numLiving, numDead;

		ent->s.modelindex2 = G_ModelIndex( ent->model2 );

		// (assume start frame of 'living' is frame '0')
		G_SpawnInt( "numLivingFrames", "0", &numLiving );
		G_SpawnInt( "numDeadFrames", "0", &numDead );

		// these are used later for the explosion,
		// so we can use them now for the animation info
		ent->s.effect3Time = numLiving;
		ent->s.density = numDead;

//		VectorCopy( ent->s.origin, ent->s.pos.trBase );
//		VectorCopy( ent->s.origin, ent->r.currentOrigin );
	}
	ent->s.frame = 0;

	// pick it up if the level designer uses "damage" instead of "dmg"
	if ( G_SpawnString( "damage", "0", &damage ) ) {
		ent->damage = atoi( damage );
	}

	ent->s.eType = ET_EXPLOSIVE;
	trap_LinkEntity( ent );

	if ( !( ent->spawnflags & 16 ) ) {
		ent->think = G_BlockThink;
		ent->nextthink = level.time + FRAMETIME;
	}
}


/*QUAKED func_explosive (0 .5 .8) ? START_INVIS TOUCHABLE USESHADER LOWGRAV NOBLOCKAAS EXPLO DYNOMITE
EXPLO only explosives can damage it rockets grendades etc
DYNOMITE only can be damaged by DY-NO-MITE!
Any brush that you want to explode or break apart.  If you want an explosion, set dmg and it will do a radius explosion of that amount at the center of the bursh.
TOUCHABLE means automatic use on player contact.
USESHADER will apply the shader used on the brush model to the debris.
LOWGRAV specifies that the debris will /always/ fall slowly
"model2" optional md3 to draw over the solid clip brush
"numLivingFrames" - how many frames to loop while the ent is 'alive' (model should have two animations: living->dead. living always starts at frame '0')
"numDeadFrames" - how many frames to play when the ent 'dies' ('nnnFrames' parameters must go together)
"wait" - how long (in seconds) to leave the model after it's 'dead'.  '-1' leaves forever.
NOTE: if you use model2, you must have an origin brush in the explosive with the center of the origin at the origin of the model
"item" - when it explodes, pop this item out with the debirs (use QUAKED name. ex: "item_health_small")
"dmg" - how much radius damage should be done, defaults to 0
"health" - defaults to 100.  If health is set to '0' the brush will not be shootable.
"targetname" - if set, no touch field will be spawned and a remote button or trigger field triggers the explosion.
"type" - type of debris ("glass", "wood", "metal", "gibs", "brick", "rock", "fabric") default is "wood"
"mass" - defaults to 75.  This determines how much debris is emitted when it explodes.  You get one large chunk per 100 of mass (up to 8) and one small chunk per 25 of mass (up to 16).  So 800 gives the most.
"noise" - sound to play when triggered.  The explosive will default to a sound that matches it's 'type'.  Use the sound name "nosound" (case in-sensitive) if you want it silent.
"leaktype" - leaks particles of this type
"leakpressure" - force the particles come out with.  '0' would cause them to fall straight down
"leaktime" - how long it leaks before it quits
"leakcount" - how many holes in the entity before it no longer leaks.
the default sounds are:
  "wood"	- "sound/world/boardbreak.wav"
  "glass"	- "sound/world/glassbreak.wav"
  "metal"	- "sound/world/metalbreak.wav"
  "gibs"	- "sound/player/gibsplit1.wav"
  "brick"	- "sound/world/brickfall.wav"
  "stone"	- "sound/world/stonefall.wav"
  "fabric"	- "sound/world/metalbreak.wav"	// (SA) temp
'leaktypes':
1:oil
2:water
3:steam
4:wine
5:smoke
6:electrical
*/
/*
"fxdensity" size of explosion 1 - 100 (default is 10)
*/
void SP_func_explosive( gentity_t *ent ) {
	int health, mass, dam, i;
	char buffer[MAX_QPATH];
	char    *s;
	char    *type;
	char    *cursorhint;

	trap_SetBrushModel( ent, ent->model );
	InitExplosive( ent );

	if ( ent->spawnflags & 1 ) {  // start invis
		ent->use = func_explosive_spawn;
		trap_UnlinkEntity( ent );
	} else if ( ent->targetname )     {
		ent->use = func_explosive_use;
		ent->AIScript_AlertEntity = func_explosive_alert;
	}


	if ( ent->spawnflags & 2 ) {  // touchable
		ent->touch = func_explosive_touch;
	} else {
		ent->touch = NULL;
	}

	if ( ( ent->spawnflags & 4 ) && ent->model && strlen( ent->model ) ) {   // use shader
		ent->s.eFlags |= EF_INHERITSHADER;
	}

	if ( ent->spawnflags & 8 ) {  // force lowgravity
		ent->duration = 1;
	}

	G_SpawnInt( "health", "100", &health );
	ent->health = health;

	G_SpawnInt( "dmg", "0", &dam );
	ent->damage = dam;

	if ( ent->health ) {
		ent->takedamage = qtrue;
	}

	if ( G_SpawnInt( "mass", "75", &mass ) ) {
		ent->count = mass;
	} else {
		ent->count = 75;
	}

	if ( G_SpawnString( "type", "wood", &type ) ) {
		if ( !Q_stricmp( type,"wood" ) ) {
			ent->key = 0;
		} else if ( !Q_stricmp( type,"glass" ) ) {
			ent->key = 1;
		} else if ( !Q_stricmp( type,"metal" ) )                                                       {
			ent->key = 2;
		} else if ( !Q_stricmp( type,"gibs" ) )                                                                                                               {
			ent->key = 3;
		} else if ( !Q_stricmp( type,"brick" ) )                                                                                                                                                                      {
			ent->key = 4;
		} else if ( !Q_stricmp( type,"rock" ) )                                                                                                                                                                                                                              {
			ent->key = 5;
		} else if ( !Q_stricmp( type,"fabric" ) )                                                                                                                                                                                                                                                                                     {
			ent->key = 6;
		}
	} else {
		ent->key = 0;
	}

	if ( G_SpawnString( "noise", "NOSOUND", &s ) ) {
		if ( Q_stricmp( s, "nosound" ) ) {
			Q_strncpyz( buffer, s, sizeof( buffer ) );
			ent->s.dl_intensity = G_SoundIndex( buffer );
		}
	} else {
		switch ( ent->key ) {
		case 0:     // "wood"
			ent->s.dl_intensity = G_SoundIndex( "sound/world/boardbreak.wav" );
			break;
		case 1:     // "glass"
			ent->s.dl_intensity = G_SoundIndex( "sound/world/glassbreak.wav" );
			break;
		case 2:     // "metal"
			ent->s.dl_intensity = G_SoundIndex( "sound/world/metalbreak.wav" );
			break;
		case 3:     // "gibs"
			ent->s.dl_intensity = G_SoundIndex( "sound/player/gibsplit1.wav" );
			break;
		case 4:     // "brick"
			ent->s.dl_intensity = G_SoundIndex( "sound/world/brickfall.wav" );
			break;
		case 5:     // "stone"
			ent->s.dl_intensity = G_SoundIndex( "sound/world/stonefall.wav" );
			break;

		default:
			break;
		}
	}

//----(SA)	added

	ent->s.dmgFlags = 0;

	if ( G_SpawnString( "cursorhint", "0", &cursorhint ) ) {

		for ( i = 1; i < HINT_NUM_HINTS; i++ ) {  // skip "HINT_NONE"
			if ( !Q_strcasecmp( cursorhint, hintStrings[i] ) ) {
				ent->s.dmgFlags = i;
				break;
			}
		}
	}
//----(SA)	end



	ent->die = func_explosive_explode;
}

/*QUAKED func_invisible_user (.3 .5 .8) ? STARTOFF HAS_USER NO_OFF_NOISE NOT_KICKABLE
when activated will use its target
"delay" - time (in seconds) before it can be used again
"offnoise" - specifies an alternate sound
"cursorhint" - overrides the auto-location of targeted entity (list below)
Normally when a player 'activates' this entity, if the entity has been turned 'off' (by a scripted command) you will hear a sound to indicate that you cannot activate the user.
The sound defaults to "sound/movers/invis_user_off.wav"

NO_OFF_NOISE - no sound will play if the invis_user is used when 'off'
NOT_KICKABLE - kicking doesn't fire, only player activating

"cursorhint" cursor types: (probably more, ask sherman if you think the list is out of date)
they /don't/ need to be all uppercase
	HINT_NONE
	HINT_PLAYER
	HINT_ACTIVATE
	HINT_DOOR
	HINT_DOOR_ROTATING
	HINT_DOOR_LOCKED
	HINT_DOOR_ROTATING_LOCKED
	HINT_MG42
	HINT_BREAKABLE
	HINT_BREAKABLE_BIG
	HINT_CHAIR
	HINT_ALARM
	HINT_HEALTH
	HINT_TREASURE
	HINT_KNIFE
	HINT_LADDER
	HINT_BUTTON
	HINT_WATER
	HINT_CAUTION
	HINT_DANGER
	HINT_SECRET
	HINT_QUESTION
	HINT_EXCLAMATION
	HINT_CLIPBOARD
	HINT_WEAPON
	HINT_AMMO
	HINT_ARMOR
	HINT_POWERUP
	HINT_HOLDABLE
	HINT_INVENTORY
	HINT_SCENARIC
	HINT_EXIT
	HINT_NOEXIT
	HINT_EXIT_FAR
	HINT_NOEXIT_FAR
	HINT_PLYR_FRIEND
	HINT_PLYR_NEUTRAL
	HINT_PLYR_ENEMY
	HINT_PLYR_UNKNOWN
*/

void use_invisible_user( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	gentity_t *player;

	if ( ent->wait < level.time ) {
		ent->wait = level.time + ent->delay;
	} else {
		return;
	}

	if ( !( other->client ) ) {
		if ( ent->spawnflags & 1 ) {
			ent->spawnflags &= ~1;
		} else
		{
			ent->spawnflags |= 1;
		}

		if ( ent->spawnflags & 2 && !( ent->spawnflags & 1 ) ) {
			if ( ent->aiName ) {
				player = AICast_FindEntityForName( "player" );
				if ( player ) {
					AICast_ScriptEvent( AICast_GetCastState( player->s.number ), "trigger", ent->target );
				}
			}

			G_UseTargets( ent, other );

			// G_Printf ("ent%s used by %s\n", ent->classname, other->classname);
		}

		return;
	}

	if ( other->client && ent->spawnflags & 1 ) {
		//----(SA)	play 'off' sound
		//----(SA)	I think this is where this goes.  Raf, let me know if it's wrong.  I need someone to tell me what a test map is for this (I'll ask Dan tomorrow)
		// not usable by player.  turned off.
		G_Sound( ent, ent->soundPos1 );
		return;
	}

	if ( ent->aiName ) {
		player = AICast_FindEntityForName( "player" );
		if ( player ) {
			AICast_ScriptEvent( AICast_GetCastState( player->s.number ), "trigger", ent->target );
		}
	}

	G_UseTargets( ent, other ); //----(SA)	how about this so the triggered targets have an 'activator' as well as an 'other'?
								//----(SA)	Please let me know if you forsee any problems with this.
}


void func_invisible_user( gentity_t *ent ) {
	int i;
	char    *sound;
	char    *cursorhint;

	VectorCopy( ent->s.origin, ent->pos1 );
	trap_SetBrushModel( ent, ent->model );

	// InitMover (ent);
	VectorCopy( ent->pos1, ent->r.currentOrigin );
	trap_LinkEntity( ent );

	ent->s.pos.trType = TR_STATIONARY;
	VectorCopy( ent->pos1, ent->s.pos.trBase );

	ent->r.contents = CONTENTS_TRIGGER;

	ent->r.svFlags = SVF_NOCLIENT;

	ent->delay *= 1000; // convert to ms

	ent->use = use_invisible_user;


//----(SA)	added
	if ( G_SpawnString( "cursorhint", "0", &cursorhint ) ) {

		for ( i = 1; i < HINT_NUM_HINTS; i++ ) {
			if ( !Q_strcasecmp( cursorhint, hintStrings[i] ) ) {
				ent->s.dmgFlags = i;
				break;
			}
		}
	}
//----(SA)	end


	if ( !( ent->spawnflags & 4 ) ) {    // !NO_OFF_NOISE
		if ( G_SpawnString( "offnoise", "0", &sound ) ) {
			ent->soundPos1 = G_SoundIndex( sound );
		} else {
			ent->soundPos1 = G_SoundIndex( "sound/movers/invis_user_off.wav" );
		}
	}


}

/*
==========
G_Activate

  Generic activation routine for doors
==========
*/
void G_Activate( gentity_t *ent, gentity_t *activator ) {
	if ( ( ent->s.apos.trType == TR_STATIONARY && ent->s.pos.trType == TR_STATIONARY )
		 && ent->active == qfalse ) {
		// trigger the ent if possible, if not, then we'll just wait at the marker until it opens, which could be never(!?)
		if ( ent->key >= KEY_LOCKED_TARGET ) { // ent force locked
			return;
		}

		if ( ent->key > KEY_NONE && ent->key < KEY_NUM_KEYS ) { // ent requires key
			gitem_t *item = BG_FindItemForKey( ent->key, 0 );
			if ( !( activator->client->ps.stats[STAT_KEYS] & ( 1 << item->giTag ) ) ) {
				return;
			}
		}

		if ( !Q_stricmp( ent->classname, "script_mover" ) ) { // RF, dont activate script_mover's
			if ( activator->aiName ) {
				G_Script_ScriptEvent( ent, "activate", activator->aiName );
			}
			return;
		}

		// hack fix for bigdoor1 on tram1_21

		if ( !( ent->teammaster ) ) {
			ent->active = qtrue;
			Use_BinaryMover( ent, activator, activator );
			G_UseTargets( ent->teammaster, activator );
			return;
		}

		if ( ent->team && ent != ent->teammaster ) {
			ent->teammaster->active = qtrue;
			Use_BinaryMover( ent->teammaster, activator, activator );
			G_UseTargets( ent->teammaster, activator );
		} else
		{
			ent->active = qtrue;
			Use_BinaryMover( ent, activator, activator );
			G_UseTargets( ent->teammaster, activator );
		}
	}
}
