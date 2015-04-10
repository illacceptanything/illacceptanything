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
// Name:			g_script_actions.c
// Function:		Wolfenstein Entity Scripting
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "../game/g_local.h"
#include "../game/q_shared.h"

/*
Contains the code to handle the various commands available with an event script.

These functions will return true if the action has been performed, and the script
should proceed to the next item on the list.
*/

void script_linkentity( gentity_t *ent );

/*
===============
G_ScriptAction_GotoMarker

  syntax: gotomarker <targetname> <speed> [accel/deccel] [turntotarget] [wait]

  NOTE: speed may be modified to round the duration to the next 50ms for smooth
  transitions
===============
*/
qboolean G_ScriptAction_GotoMarker( gentity_t *ent, char *params ) {
	char    *pString, *token;
	gentity_t *target;
	vec3_t vec;
	float speed, dist;
	qboolean wait = qfalse, turntotarget = qfalse;
	int trType;
	int duration, i;
	vec3_t diff;
	vec3_t angles;

	if ( params && ( ent->scriptStatus.scriptFlags & SCFL_GOING_TO_MARKER ) ) {
		// we can't process a new movement until the last one has finished
		return qfalse;
	}

	if ( !params || ent->scriptStatus.scriptStackChangeTime < level.time ) {          // we are waiting for it to reach destination
		if ( ent->s.pos.trTime + ent->s.pos.trDuration <= level.time ) {  // we made it
			ent->scriptStatus.scriptFlags &= ~SCFL_GOING_TO_MARKER;

			// set the angles at the destination
			BG_EvaluateTrajectory( &ent->s.apos, ent->s.apos.trTime + ent->s.apos.trDuration, ent->s.angles );
			VectorCopy( ent->s.angles, ent->s.apos.trBase );
			VectorCopy( ent->s.angles, ent->r.currentAngles );
			ent->s.apos.trTime = level.time;
			ent->s.apos.trDuration = 0;
			ent->s.apos.trType = TR_STATIONARY;
			VectorClear( ent->s.apos.trDelta );

			// stop moving
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin );
			VectorCopy( ent->s.origin, ent->s.pos.trBase );
			VectorCopy( ent->s.origin, ent->r.currentOrigin );
			ent->s.pos.trTime = level.time;
			ent->s.pos.trDuration = 0;
			ent->s.pos.trType = TR_STATIONARY;
			VectorClear( ent->s.pos.trDelta );

			script_linkentity( ent );

			return qtrue;
		}
	} else {    // we have just started this command

		pString = params;
		token = COM_ParseExt( &pString, qfalse );
		if ( !token[0] ) {
			G_Error( "G_Scripting: gotomarker must have an targetname\n" );
		}

		// find the entity with the given "targetname"
		target = G_Find( NULL, FOFS( targetname ), token );

		if ( !target ) {
			G_Error( "G_Scripting: can't find entity with \"targetname\" = \"%s\"\n", token );
		}

		VectorSubtract( target->r.currentOrigin, ent->r.currentOrigin, vec );

		token = COM_ParseExt( &pString, qfalse );
		if ( !token[0] ) {
			G_Error( "G_Scripting: gotomarker must have a speed\n" );
		}

		speed = atof( token );
		trType = TR_LINEAR_STOP;

		while ( token[0] ) {
			token = COM_ParseExt( &pString, qfalse );
			if ( token[0] ) {
				if ( !Q_stricmp( token, "accel" ) ) {
					trType = TR_ACCELERATE;
				} else if ( !Q_stricmp( token, "deccel" ) )      {
					trType = TR_DECCELERATE;
				} else if ( !Q_stricmp( token, "wait" ) )      {
					wait = qtrue;
				} else if ( !Q_stricmp( token, "turntotarget" ) )      {
					turntotarget = qtrue;
				}
			}
		}

		// start the movement
		if ( ent->s.eType == ET_MOVER ) {

			VectorCopy( vec, ent->movedir );
			VectorCopy( ent->r.currentOrigin, ent->pos1 );
			VectorCopy( target->r.currentOrigin, ent->pos2 );
			ent->speed = speed;
			dist = VectorDistance( ent->pos1, ent->pos2 );
			// setup the movement with the new parameters
			InitMover( ent );

			// start the movement

			SetMoverState( ent, MOVER_1TO2, level.time );
			if ( trType != TR_LINEAR_STOP ) { // allow for acceleration/decceleration
				ent->s.pos.trDuration = 1000.0 * dist / ( speed / 2.0 );
				ent->s.pos.trType = trType;
			}
			ent->reached = NULL;

			if ( turntotarget ) {
				duration = ent->s.pos.trDuration;
				VectorCopy( target->s.angles, angles );

				for ( i = 0; i < 3; i++ ) {
					diff[i] = AngleDifference( angles[i], ent->s.angles[i] );
					while ( diff[i] > 180 )
						diff[i] -= 360;
					while ( diff[i] < -180 )
						diff[i] += 360;
				}
				VectorCopy( ent->s.angles, ent->s.apos.trBase );
				if ( duration ) {
					VectorScale( diff, 1000.0 / (float)duration, ent->s.apos.trDelta );
				} else {
					VectorClear( ent->s.apos.trDelta );
				}
				ent->s.apos.trDuration = duration;
				ent->s.apos.trTime = level.time;
				ent->s.apos.trType = TR_LINEAR_STOP;
				if ( trType != TR_LINEAR_STOP ) { // allow for acceleration/decceleration
					ent->s.pos.trDuration = 1000.0 * dist / ( speed / 2.0 );
					ent->s.pos.trType = trType;
				}
			}

		} else {
			// calculate the trajectory
			ent->s.pos.trType = TR_LINEAR_STOP;
			ent->s.pos.trTime = level.time;
			VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
			dist = VectorNormalize( vec );
			VectorScale( vec, speed, ent->s.pos.trDelta );
			ent->s.pos.trDuration = 1000 * ( dist / speed );

			if ( turntotarget ) {
				duration = ent->s.pos.trDuration;
				VectorCopy( target->s.angles, angles );

				for ( i = 0; i < 3; i++ ) {
					diff[i] = AngleDifference( angles[i], ent->s.angles[i] );
					while ( diff[i] > 180 )
						diff[i] -= 360;
					while ( diff[i] < -180 )
						diff[i] += 360;
				}
				VectorCopy( ent->s.angles, ent->s.apos.trBase );
				if ( duration ) {
					VectorScale( diff, 1000.0 / (float)duration, ent->s.apos.trDelta );
				} else {
					VectorClear( ent->s.apos.trDelta );
				}
				ent->s.apos.trDuration = duration;
				ent->s.apos.trTime = level.time;
				ent->s.apos.trType = TR_LINEAR_STOP;
			}

		}

		if ( !wait ) {
			// round the duration to the next 50ms
			if ( ent->s.pos.trDuration % 50 ) {
				float frac;

				frac = (float)( ( ( ent->s.pos.trDuration / 50 ) * 50 + 50 ) - ent->s.pos.trDuration ) / (float)( ent->s.pos.trDuration );
				if ( frac < 1 ) {
					VectorScale( ent->s.pos.trDelta, 1.0 / ( 1.0 + frac ), ent->s.pos.trDelta );
					ent->s.pos.trDuration = ( ent->s.pos.trDuration / 50 ) * 50 + 50;
				}
			}

			// set the goto flag, so we can keep processing the move until we reach the destination
			ent->scriptStatus.scriptFlags |= SCFL_GOING_TO_MARKER;
			return qtrue;   // continue to next command
		}

	}

	BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->r.currentOrigin );
	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles );
	script_linkentity( ent );

	return qfalse;
}

/*
=================
G_ScriptAction_Wait

  syntax: wait <duration>
=================
*/
qboolean G_ScriptAction_Wait( gentity_t *ent, char *params ) {
	char    *pString, *token;
	int duration;

	// get the duration
	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "G_Scripting: wait must have a duration\n" );
	}
	duration = atoi( token );

	return ( ent->scriptStatus.scriptStackChangeTime + duration < level.time );
}

/*
=================
G_ScriptAction_Trigger

  syntax: trigger <aiName/scriptName> <trigger>

  Calls the specified trigger for the given ai character or script entity
=================
*/
qboolean G_ScriptAction_Trigger( gentity_t *ent, char *params ) {
	gentity_t *trent;
	char *pString, name[MAX_QPATH], trigger[MAX_QPATH], *token;
	int oldId;

	// get the cast name
	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	Q_strncpyz( name, token, sizeof( name ) );
	if ( !name[0] ) {
		G_Error( "G_Scripting: trigger must have a name and an identifier\n" );
	}

	token = COM_ParseExt( &pString, qfalse );
	Q_strncpyz( trigger, token, sizeof( trigger ) );
	if ( !trigger[0] ) {
		G_Error( "G_Scripting: trigger must have a name and an identifier\n" );
	}

	trent = AICast_FindEntityForName( name );
	if ( trent ) { // we are triggering an AI
				  //oldId = trent->scriptStatus.scriptId;
		AICast_ScriptEvent( AICast_GetCastState( trent->s.number ), "trigger", trigger );
		return qtrue;
	}

	// look for an entity
	trent = G_Find( &g_entities[MAX_CLIENTS], FOFS( scriptName ), name );
	if ( trent ) {
		oldId = trent->scriptStatus.scriptId;
		G_Script_ScriptEvent( trent, "trigger", trigger );
		// if the script changed, return false so we don't muck with it's variables
		return ( ( trent != ent ) || ( oldId == trent->scriptStatus.scriptId ) );
	}

	G_Error( "G_Scripting: trigger has unknown name: %s\n", name );
	return qfalse;  // shutup the compiler
}

/*
================
G_ScriptAction_PlaySound

  syntax: playsound <soundname OR scriptname> [LOOPING]

  Currently only allows playing on the VOICE channel, unless you use a sound script.

  Use the optional LOOPING paramater to attach the sound to the entities looping channel.
================
*/
qboolean G_ScriptAction_PlaySound( gentity_t *ent, char *params ) {
	char *pString, *token;
	char sound[MAX_QPATH];

	if ( !params ) {
		G_Error( "G_Scripting: syntax error\n\nplaysound <soundname OR scriptname>\n" );
	}

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	Q_strncpyz( sound, token, sizeof( sound ) );

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] || Q_strcasecmp( token, "looping" ) ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( sound ) );
	} else {    // looping channel
		ent->s.loopSound = G_SoundIndex( sound );
	}

	return qtrue;
}

//----(SA)	added
/*
==================
AICast_ScriptAction_MusicStart

==================
*/
qboolean G_ScriptAction_MusicStart( gentity_t *ent, char *params ) {
	char    *pString, *token;
	char cvarName[MAX_QPATH];
	int fadeupTime = 0;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "G_Scripting: syntax: mu_start <musicfile> <fadeuptime>" );
	}
	Q_strncpyz( cvarName, token, sizeof( cvarName ) );

	token = COM_ParseExt( &pString, qfalse );
	if ( token[0] ) {
		fadeupTime = atoi( token );
	}

	trap_SendServerCommand( -1, va( "mu_start %s %d", cvarName, fadeupTime ) );

	return qtrue;
}

/*
==================
AICast_ScriptAction_MusicPlay

==================
*/
qboolean G_ScriptAction_MusicPlay( gentity_t *ent, char *params ) {
	char    *pString, *token;
	char cvarName[MAX_QPATH];
	int fadeupTime = 0;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "G_Scripting: syntax: mu_play <musicfile> [fadeup time]" );
	}
	Q_strncpyz( cvarName, token, sizeof( cvarName ) );

	trap_SendServerCommand( -1, va( "mu_play %s %d", cvarName, fadeupTime ) );

	return qtrue;
}


/*
==================
AICast_ScriptAction_MusicStop
==================
*/
qboolean G_ScriptAction_MusicStop( gentity_t *ent, char *params ) {
	char    *pString, *token;
	int fadeoutTime = 0;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( token[0] ) {
		fadeoutTime = atoi( token );
	}

	trap_SendServerCommand( -1, va( "mu_stop %i\n", fadeoutTime ) );

	return qtrue;
}


/*
==================
AICast_ScriptAction_MusicFade
==================
*/
qboolean G_ScriptAction_MusicFade( gentity_t *ent, char *params ) {
	char    *pString, *token;
	float targetvol;
	int fadetime;

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "G_Scripting: syntax: mu_fade <targetvol> <fadetime>" );
	}
	targetvol = atof( token );

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "G_Scripting: syntax: mu_fade <targetvol> <fadetime>" );
	}
	fadetime = atoi( token );

	trap_SendServerCommand( -1, va( "mu_fade %f %i\n", targetvol, fadetime ) );

	return qtrue;
}


/*
==================
AICast_ScriptAction_MusicQueue
==================
*/
qboolean G_ScriptAction_MusicQueue( gentity_t *ent, char *params ) {
	char    *pString, *token;
	char cvarName[MAX_QPATH];

	pString = params;
	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "G_Scripting: syntax: mu_queue <musicfile>" );
	}
	Q_strncpyz( cvarName, token, sizeof( cvarName ) );

	trap_SetConfigstring( CS_MUSIC_QUEUE, cvarName );

	return qtrue;
}

//----(SA)	end

/*
=================
G_ScriptAction_PlayAnim

  syntax: playanim <startframe> <endframe> [looping <FOREVER/duration>] [rate <FPS>]

  NOTE: all source animations must be at 20fps
=================
*/
qboolean G_ScriptAction_PlayAnim( gentity_t *ent, char *params ) {
	char *pString, *token, tokens[2][MAX_QPATH];
	int i, endtime = 0; // TTimo: init
	qboolean looping = qfalse, forever = qfalse;
	int startframe, endframe, idealframe;
	int rate = 20;

	if ( ( ent->scriptStatus.scriptFlags & SCFL_ANIMATING ) && ( ent->scriptStatus.scriptStackChangeTime == level.time ) ) {
		// this is a new call, so cancel the previous animation
		ent->scriptStatus.scriptFlags &= ~SCFL_ANIMATING;
	}

	pString = params;

	for ( i = 0; i < 2; i++ ) {
		token = COM_ParseExt( &pString, qfalse );
		if ( !token || !token[0] ) {
			G_Printf( "G_Scripting: syntax error\n\nplayanim <startframe> <endframe> [LOOPING <duration>]\n" );
			return qtrue;
		} else {
			Q_strncpyz( tokens[i], token, sizeof( tokens[i] ) );
		}
	}

	startframe = atoi( tokens[0] );
	endframe = atoi( tokens[1] );

	// check for optional parameters
	token = COM_ParseExt( &pString, qfalse );
	if ( token[0] ) {
		if ( !Q_strcasecmp( token, "looping" ) ) {
			looping = qtrue;

			token = COM_ParseExt( &pString, qfalse );
			if ( !token || !token[0] ) {
				G_Printf( "G_Scripting: syntax error\n\nplayanim <startframe> <endframe> [LOOPING <duration>]\n" );
				return qtrue;
			}
			if ( !Q_strcasecmp( token, "untilreachmarker" ) ) {
				if ( level.time < ent->s.pos.trTime + ent->s.pos.trDuration ) {
					endtime = level.time + 100;
				} else {
					endtime = 0;
				}
			} else if ( !Q_strcasecmp( token, "forever" ) ) {
				ent->scriptStatus.animatingParams = params;
				ent->scriptStatus.scriptFlags |= SCFL_ANIMATING;
				endtime = level.time + 100;     // we don't care when it ends, since we are going forever!
				forever = qtrue;
			} else {
				endtime = ent->scriptStatus.scriptStackChangeTime + atoi( token );
			}

			token = COM_ParseExt( &pString, qfalse );
		}

		if ( token[0] && !Q_strcasecmp( token, "rate" ) ) {
			token = COM_ParseExt( &pString, qfalse );
			if ( !token[0] ) {
				G_Error( "G_Scripting: playanim has RATE parameter without an actual rate specified" );
			}
			rate = atoi( token );
		}

		if ( !looping ) {
			endtime = ent->scriptStatus.scriptStackChangeTime + ( ( endframe - startframe ) * ( 1000 / 20 ) );
		}
	}

	idealframe = startframe + (int)floor( (float)( level.time - ent->scriptStatus.scriptStackChangeTime ) / ( 1000.0 / (float)rate ) );
	if ( looping ) {
		ent->s.frame = startframe + ( idealframe - startframe ) % ( endframe - startframe );
		ent->s.eFlags |= EF_MOVER_ANIMATE;
	} else {
		if ( idealframe > endframe ) {
			ent->s.frame = endframe;
			ent->s.eFlags &= ~EF_MOVER_ANIMATE; // stop interpolation, since we have gone passed the endframe
		} else {
			ent->s.frame = idealframe;
			ent->s.eFlags |= EF_MOVER_ANIMATE;
		}
	}

	if ( forever ) {
		ent->s.eFlags |= EF_MOVER_ANIMATE;
		return qtrue;   // continue to the next command
	}

	if ( endtime <= level.time ) {
		ent->s.eFlags &= ~EF_MOVER_ANIMATE; // stop animating
		return qtrue;
	} else {
		return qfalse;
	}
};

/*
=================
G_ScriptAction_AlertEntity

  syntax: alertentity <targetname>
=================
*/
qboolean G_ScriptAction_AlertEntity( gentity_t *ent, char *params ) {
	gentity_t   *alertent;

	if ( !params || !params[0] ) {
		G_Error( "G_Scripting: alertentity without targetname\n" );
	}

	// find this targetname
	alertent = G_Find( NULL, FOFS( targetname ), params );
	if ( !alertent ) {
		G_Error( "G_Scripting: alertentity cannot find targetname \"%s\"\n", params );
	}

	if ( alertent->client ) {
		// call this entity's AlertEntity function
		if ( !alertent->AIScript_AlertEntity ) {
			G_Error( "G_Scripting: alertentity \"%s\" (classname = %s) doesn't have an \"AIScript_AlertEntity\" function\n", params, alertent->classname );
		}
		alertent->AIScript_AlertEntity( alertent );
	} else {
		if ( !alertent->use ) {
			G_Error( "G_Scripting: alertentity \"%s\" (classname = %s) doesn't have a \"use\" function\n", params, alertent->classname );
		}
		alertent->use( alertent, NULL, NULL );
	}

	return qtrue;
}

/*
=================
G_ScriptAction_Accum

  syntax: accum <buffer_index> <command> <paramater>

  Commands:

	accum <n> inc <m>
	accum <n> abort_if_less_than <m>
	accum <n> abort_if_greater_than <m>
	accum <n> abort_if_not_equal <m>
	accum <n> abort_if_equal <m>
	accum <n> set <m>
	accum <n> random <m>
	accum <n> bitset <m>
	accum <n> bitreset <m>
	accum <n> abort_if_bitset <m>
	accum <n> abort_if_not_bitset <m>
=================
*/
qboolean G_ScriptAction_Accum( gentity_t *ent, char *params ) {
	char *pString, *token, lastToken[MAX_QPATH];
	int bufferIndex;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "G_Scripting: accum without a buffer index\n" );
	}

	bufferIndex = atoi( token );
	if ( bufferIndex >= G_MAX_SCRIPT_ACCUM_BUFFERS ) {
		G_Error( "G_Scripting: accum buffer is outside range (0 - %i)\n", G_MAX_SCRIPT_ACCUM_BUFFERS );
	}

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "G_Scripting: accum without a command\n" );
	}

	Q_strncpyz( lastToken, token, sizeof( lastToken ) );
	token = COM_ParseExt( &pString, qfalse );

	if ( !Q_stricmp( lastToken, "inc" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] += atoi( token );
	} else if ( !Q_stricmp( lastToken, "abort_if_less_than" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( ent->scriptAccumBuffer[bufferIndex] < atoi( token ) ) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_greater_than" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( ent->scriptAccumBuffer[bufferIndex] > atoi( token ) ) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_equal" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( ent->scriptAccumBuffer[bufferIndex] != atoi( token ) ) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_equal" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( ent->scriptAccumBuffer[bufferIndex] == atoi( token ) ) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "bitset" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] |= ( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "bitreset" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] &= ~( 1 << atoi( token ) );
	} else if ( !Q_stricmp( lastToken, "abort_if_bitset" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( ent->scriptAccumBuffer[bufferIndex] & ( 1 << atoi( token ) ) ) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "abort_if_not_bitset" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		if ( !( ent->scriptAccumBuffer[bufferIndex] & ( 1 << atoi( token ) ) ) ) {
			// abort the current script
			ent->scriptStatus.scriptStackHead = ent->scriptEvents[ent->scriptStatus.scriptEventIndex].stack.numItems;
		}
	} else if ( !Q_stricmp( lastToken, "set" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] = atoi( token );
	} else if ( !Q_stricmp( lastToken, "random" ) ) {
		if ( !token[0] ) {
			G_Error( "Scripting: accum %s requires a parameter\n", lastToken );
		}
		ent->scriptAccumBuffer[bufferIndex] = rand() % atoi( token );
	} else {
		G_Error( "Scripting: accum: \"%s\": unknown command\n", params );
	}

	return qtrue;
}

/*
=================
G_ScriptAction_MissionFailed

  syntax: missionfailed
=================
*/
qboolean G_ScriptAction_MissionFailed( gentity_t *ent, char *params ) {
	char    *pString, *token;
	int time = 6, mof = 0;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );   // time
	if ( token && token[0] ) {
		time = atoi( token );
	}

	token = COM_ParseExt( &pString, qfalse );   // mof (means of failure)
	if ( token && token[0] ) {
		mof = atoi( token );
	}

	// play mission fail music
	trap_SendServerCommand( -1, "mu_play sound/music/l_failed_1.wav 0\n" );
	trap_SetConfigstring( CS_MUSIC_QUEUE, "" );  // clear queue so it'll be quiet after hit

	trap_SendServerCommand( -1, va( "snd_fade 0 %d", time * 1000 ) );   //----(SA)	added

	if ( mof < 0 ) {
		mof = 0;
	}
	trap_SendServerCommand( -1, va( "cp missionfail%d", mof ) );

	// reload the current savegame, after a delay
	trap_SetConfigstring( CS_SCREENFADE, va( "1 %i %i", level.time + 250, time * 1000 ) );
//	reloading = RELOAD_FAILED;
	trap_Cvar_Set( "g_reloading", va( "%d", RELOAD_FAILED ) );

	level.reloadDelayTime = level.time + 1000 + time * 1000;

	return qtrue;
}

/*
=================
G_ScriptAction_MissionSuccess

  syntax: missionsuccess <mission_level>
=================
*/
qboolean G_ScriptAction_MissionSuccess( gentity_t *ent, char *params ) {
	gentity_t   *player;
	vmCvar_t cvar;
	int lvl;
	char *pString, *token;

	pString = params;

	token = COM_ParseExt( &pString, qfalse );
	if ( !token[0] ) {
		G_Error( "AI Scripting: missionsuccess requires a mission_level identifier\n" );
	}

	player = AICast_FindEntityForName( "player" );
	// double check that they are still alive
	if ( player->health <= 0 ) {
		return qfalse;  // hold the script here

	}
	lvl = atoi( token );

// if you've already got it, just return.  don't need to set 'yougotmail'
	if ( player->missionObjectives & ( 1 << ( lvl - 1 ) ) ) {
		return qtrue;
	}

	player->missionObjectives |= ( 1 << ( lvl - 1 ) );  // make this bitwise

	//set g_objective<n> cvar
	trap_Cvar_Register( &cvar, va( "g_objective%i", lvl ), "1", CVAR_ROM );
	// set it to make sure
	trap_Cvar_Set( va( "g_objective%i", lvl ), "1" );

	token = COM_ParseExt( &pString, qfalse );
	if ( token[0] ) {
		if ( Q_strcasecmp( token,"nodisplay" ) ) {   // unknown command
			G_Error( "AI Scripting: missionsuccess with unknown parameter: %s\n", token );
		}
	} else {    // show on-screen information
		trap_Cvar_Set( "cg_youGotMail", "2" ); // set flag to draw icon
	}

	return qtrue;
}


/*
=================
G_ScriptAction_Print

  syntax: print <text>

  Mostly for debugging purposes
=================
*/
qboolean G_ScriptAction_Print( gentity_t *ent, char *params ) {
	if ( !params || !params[0] ) {
		G_Error( "G_Scripting: print requires some text\n" );
	}

	G_Printf( "(G_Script) %s-> %s\n", ent->scriptName, params );
	return qtrue;
}

/*
=================
G_ScriptAction_FaceAngles

  syntax: faceangles <pitch> <yaw> <roll> <duration/GOTOTIME> [ACCEL/DECCEL]

  The entity will face the given angles, taking <duration> to get there. If the
  GOTOTIME is given instead of a timed duration, the duration calculated from the
  last gotomarker command will be used instead.
=================
*/
qboolean G_ScriptAction_FaceAngles( gentity_t *ent, char *params ) {
	char *pString, *token;
	int duration, i;
	vec3_t diff;
	vec3_t angles;
	int trType = TR_LINEAR_STOP;

	if ( !params || !params[0] ) {
		G_Error( "G_Scripting: syntax: faceangles <pitch> <yaw> <roll> <duration/GOTOTIME>\n" );
	}

	if ( ent->scriptStatus.scriptStackChangeTime == level.time ) {
		pString = params;
		for ( i = 0; i < 3; i++ ) {
			token = COM_Parse( &pString );
			if ( !token || !token[0] ) {
				G_Error( "G_Scripting: syntax: faceangles <pitch> <yaw> <roll> <duration/GOTOTIME>\n" );
			}
			angles[i] = atoi( token );
		}

		token = COM_Parse( &pString );
		if ( !token || !token[0] ) {
			G_Error( "G_Scripting: faceangles requires a <pitch> <yaw> <roll> <duration/GOTOTIME>\n" );
		}
		if ( !Q_strcasecmp( token, "gototime" ) ) {
			duration = ent->s.pos.trDuration;
		} else {
			duration = atoi( token );
		}

		token = COM_Parse( &pString );
		if ( token && token[0] ) {
			if ( !Q_strcasecmp( token, "accel" ) ) {
				trType = TR_ACCELERATE;
			}
			if ( !Q_strcasecmp( token, "deccel" ) ) {
				trType = TR_DECCELERATE;
			}
		}

		for ( i = 0; i < 3; i++ ) {
			diff[i] = AngleDifference( angles[i], ent->s.angles[i] );
			while ( diff[i] > 180 )
				diff[i] -= 360;
			while ( diff[i] < -180 )
				diff[i] += 360;
		}

		VectorCopy( ent->s.angles, ent->s.apos.trBase );
		if ( duration ) {
			VectorScale( diff, 1000.0 / (float)duration, ent->s.apos.trDelta );
		} else {
			VectorClear( ent->s.apos.trDelta );
		}
		ent->s.apos.trDuration = duration;
		ent->s.apos.trTime = level.time;
		ent->s.apos.trType = TR_LINEAR_STOP;

		if ( trType != TR_LINEAR_STOP ) { // accel / deccel logic
			// calc the speed from duration and start/end delta
			for ( i = 0; i < 3; i++ ) {
				ent->s.apos.trDelta[i] = 2.0 * 1000.0 * diff[i] / (float)duration;
			}
			ent->s.apos.trType = trType;
		}

	} else if ( ent->s.apos.trTime + ent->s.apos.trDuration <= level.time ) {
		// finished turning
		BG_EvaluateTrajectory( &ent->s.apos, ent->s.apos.trTime + ent->s.apos.trDuration, ent->s.angles );
		VectorCopy( ent->s.angles, ent->s.apos.trBase );
		VectorCopy( ent->s.angles, ent->r.currentAngles );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trDuration = 0;
		ent->s.apos.trType = TR_STATIONARY;
		VectorClear( ent->s.apos.trDelta );

		script_linkentity( ent );

		return qtrue;
	}

	BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->r.currentAngles );
	script_linkentity( ent );

	return qfalse;
}

/*
===================
G_ScriptAction_ResetScript

	causes any currently running scripts to abort, in favour of the current script
===================
*/
qboolean G_ScriptAction_ResetScript( gentity_t *ent, char *params ) {
	if ( level.time == ent->scriptStatus.scriptStackChangeTime ) {
		return qfalse;
	}

	return qtrue;
}

/*
===================
G_ScriptAction_TagConnect

	syntax: attachtotag <targetname/scriptname> <tagname>

	connect this entity onto the tag of another entity
===================
*/
qboolean G_ScriptAction_TagConnect( gentity_t *ent, char *params ) {
	char *pString, *token;
	gentity_t *parent;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_TagConnect: syntax: attachtotag <targetname> <tagname>\n" );
	}

	parent = G_Find( NULL, FOFS( targetname ), token );
	if ( !parent ) {
		parent = G_Find( NULL, FOFS( scriptName ), token );
		if ( !parent ) {
			G_Error( "G_ScriptAction_TagConnect: unable to find entity with targetname \"%s\"", token );
		}
	}

	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_TagConnect: syntax: attachtotag <targetname> <tagname>\n" );
	}

	ent->tagParent = parent;
	ent->tagName = G_Alloc( strlen( token ) + 1 );
	Q_strncpyz( ent->tagName, token, strlen( token ) + 1 );

	G_ProcessTagConnect( ent, qtrue );

	return qtrue;
}

/*
====================
G_ScriptAction_Halt

  syntax: halt

  Stop moving.
====================
*/
qboolean G_ScriptAction_Halt( gentity_t *ent, char *params ) {
	if ( level.time == ent->scriptStatus.scriptStackChangeTime ) {
		ent->scriptStatus.scriptFlags &= ~SCFL_GOING_TO_MARKER;

		// stop the angles
		BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->s.angles );
		VectorCopy( ent->s.angles, ent->s.apos.trBase );
		VectorCopy( ent->s.angles, ent->r.currentAngles );
		ent->s.apos.trTime = level.time;
		ent->s.apos.trDuration = 0;
		ent->s.apos.trType = TR_STATIONARY;
		VectorClear( ent->s.apos.trDelta );

		// stop moving
		BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin );
		VectorCopy( ent->s.origin, ent->s.pos.trBase );
		VectorCopy( ent->s.origin, ent->r.currentOrigin );
		ent->s.pos.trTime = level.time;
		ent->s.pos.trDuration = 0;
		ent->s.pos.trType = TR_STATIONARY;
		VectorClear( ent->s.pos.trDelta );

		script_linkentity( ent );

		return qfalse;  // kill any currently running script
	} else {
		return qtrue;
	}
}

/*
===================
G_ScriptAction_StopSound

  syntax: stopsound

  Stops any looping sounds for this entity.
===================
*/
qboolean G_ScriptAction_StopSound( gentity_t *ent, char *params ) {
	ent->s.loopSound = 0;
	return qtrue;
}

/*
===================
G_ScriptAction_StartCam

  syntax: startcam <camera filename>
===================
*/
qboolean G_ScriptAction_StartCam( gentity_t *ent, char *params ) {
	char *pString, *token;
	gentity_t *player;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_Cam: filename parameter required\n" );
	}

	// turn off noclient flag
	ent->r.svFlags &= ~SVF_NOCLIENT;

	// issue a start camera command to the client
	player = AICast_FindEntityForName( "player" );
	if ( !player ) {
		G_Error( "player not found, perhaps you should give them more time to spawn in" );
	}
	trap_SendServerCommand( player->s.number, va( "startCam %s", token ) );

	return qtrue;
}

/*
=================
G_ScriptAction_EntityScriptName
=================
*/
qboolean G_ScriptAction_EntityScriptName( gentity_t *ent, char *params ) {
	trap_Cvar_Set( "g_scriptName", params );
	return qtrue;
}


/*
=================
G_ScriptAction_AIScriptName
=================
*/
qboolean G_ScriptAction_AIScriptName( gentity_t *ent, char *params ) {
	trap_Cvar_Set( "ai_scriptName", params );
	return qtrue;
}

// -----------------------------------------------------------------------

// DHM - Nerve :: Multiplayer scripting commands
/*
===================
G_ScriptAction_MapDescription

  syntax: wm_mapdescription <"long description of map in quotes">
===================
*/
qboolean G_ScriptAction_MapDescription( gentity_t *ent, char *params ) {
	char *pString, *token;

	pString = params;
	token = COM_Parse( &pString );

	trap_SetConfigstring( CS_MULTI_MAPDESC, token );

	return qtrue;
}

/*
===================
G_ScriptAction_AxisRespawntime

  syntax: wm_axis_respawntime <seconds>
===================
*/
qboolean G_ScriptAction_AxisRespawntime( gentity_t *ent, char *params ) {
	char *pString, *token;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_AxisRespawntime: time parameter required\n" );
	}

	trap_Cvar_Set( "g_redlimbotime", va( "%s000", token ) );

	return qtrue;
}

/*
===================
G_ScriptAction_AlliedRespawntime

  syntax: wm_allied_respawntime <seconds>
===================
*/
qboolean G_ScriptAction_AlliedRespawntime( gentity_t *ent, char *params ) {
	char *pString, *token;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_AlliedRespawntime: time parameter required\n" );
	}

	trap_Cvar_Set( "g_bluelimbotime", va( "%s000", token ) );

	return qtrue;
}

/*
===================
G_ScriptAction_NumberofObjectives

  syntax: wm_number_of_objectives <number>
===================
*/
qboolean G_ScriptAction_NumberofObjectives( gentity_t *ent, char *params ) {
	char *pString, *token;
	char cs[MAX_STRING_CHARS];

	int num;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_NumberofObjectives: number parameter required\n" );
	}

	num = atoi( token );
	if ( num < 1 || num > MAX_OBJECTIVES ) {
		G_Error( "G_ScriptAction_NumberofObjectives: Invalid number of objectives\n" );
	}

	trap_GetConfigstring( CS_MULTI_INFO, cs, sizeof( cs ) );

	Info_SetValueForKey( cs, "numobjectives", token );

	trap_SetConfigstring( CS_MULTI_INFO, cs );

	return qtrue;
}

/*
===================
G_ScriptAction_ObjectiveAxisDesc

  syntax: wm_objective_axis_desc <objective_number "Description in quotes">
===================
*/
qboolean G_ScriptAction_ObjectiveAxisDesc( gentity_t *ent, char *params ) {
	char *pString, *token;
	char cs[MAX_STRING_CHARS];

	int num, cs_obj = CS_MULTI_OBJECTIVE1;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_ObjectiveAxisDesc: number parameter required\n" );
	}

	num = atoi( token );
	if ( num < 1 || num > MAX_OBJECTIVES ) {
		G_Error( "G_ScriptAction_ObjectiveAxisDesc: Invalid objective number\n" );
	}

	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_ObjectiveAxisDesc: description parameter required\n" );
	}

	// Move to correct objective config string
	cs_obj += ( num - 1 );

	trap_GetConfigstring( cs_obj, cs, sizeof( cs ) );

	Info_SetValueForKey( cs, "axis_desc", token );

	trap_SetConfigstring( cs_obj, cs );

	return qtrue;
}

/*
===================
G_ScriptAction_ObjectiveAlliedDesc

  syntax: wm_objective_allied_desc <objective_number "Description in quotes">
===================
*/
qboolean G_ScriptAction_ObjectiveAlliedDesc( gentity_t *ent, char *params ) {
	char *pString, *token;
	char cs[MAX_STRING_CHARS];

	int num, cs_obj = CS_MULTI_OBJECTIVE1;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_ObjectiveAlliedDesc: number parameter required\n" );
	}

	num = atoi( token );
	if ( num < 1 || num > MAX_OBJECTIVES ) {
		G_Error( "G_ScriptAction_ObjectiveAlliedDesc: Invalid objective number\n" );
	}

	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_ObjectiveAlliedDesc: description parameter required\n" );
	}

	// Move to correct objective config string
	cs_obj += ( num - 1 );

	trap_GetConfigstring( cs_obj, cs, sizeof( cs ) );

	Info_SetValueForKey( cs, "allied_desc", token );

	trap_SetConfigstring( cs_obj, cs );

	return qtrue;
}

/*
===================
G_ScriptAction_SetWinner

  syntax: wm_setwinner <team>

  team: 0==AXIS, 1==ALLIED
===================
*/
qboolean G_ScriptAction_SetWinner( gentity_t *ent, char *params ) {
	char *pString, *token;
	char cs[MAX_STRING_CHARS];

	int num;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_SetWinner: number parameter required\n" );
	}

	num = atoi( token );
	if ( num < -1 || num > 1 ) {
		G_Error( "G_ScriptAction_SetWinner: Invalid team number\n" );
	}

	trap_GetConfigstring( CS_MULTI_INFO, cs, sizeof( cs ) );

	Info_SetValueForKey( cs, "winner", token );

	trap_SetConfigstring( CS_MULTI_INFO, cs );

	return qtrue;
}

/*
===================
G_ScriptAction_SetObjectiveStatus

  syntax: wm_set_objective_status <status>

  status: -1==neutral, 0==held by axis, 1==held by allies
===================
*/
qboolean G_ScriptAction_SetObjectiveStatus( gentity_t *ent, char *params ) {
	char *pString, *token;
	char cs[MAX_STRING_CHARS];

	int num, status, cs_obj = CS_MULTI_OBJECTIVE1;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_SetObjectiveStatus: number parameter required\n" );
	}

	num = atoi( token );
	if ( num < 1 || num > MAX_OBJECTIVES ) {
		G_Error( "G_ScriptAction_SetObjectiveStatus: Invalid objective number\n" );
	}

	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_SetObjectiveStatus: status parameter required\n" );
	}

	status = atoi( token );
	if ( status < -1 || status > 1 ) {
		G_Error( "G_ScriptAction_SetObjectiveStatus: Invalid status number\n" );
	}

	// Move to correct objective config string
	cs_obj += ( num - 1 );

	trap_GetConfigstring( cs_obj, cs, sizeof( cs ) );

	Info_SetValueForKey( cs, "status", token );

	trap_SetConfigstring( cs_obj, cs );

	return qtrue;
}

/*
===================
G_ScriptAction_Announce

  syntax: wm_announce <"text to send to all clients">
===================
*/
qboolean G_ScriptAction_Announce( gentity_t *ent, char *params ) {
	char *pString, *token;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_Announce: statement parameter required\n" );
	}

	trap_SendServerCommand( -1, va( "cp \"%s\"", token ) );

	return qtrue;
}

/*
===================
G_ScriptAction_EndRound

  syntax: wm_endround <>
===================
*/

extern void LogExit( const char *string );

qboolean G_ScriptAction_EndRound( gentity_t *ent, char *params ) {
	LogExit( "Wolf EndRound." );

	return qtrue;
}

/*
===================
G_ScriptAction_SetRoundTimelimit

  syntax: wm_set_round_timelimit <number>
===================
*/
qboolean G_ScriptAction_SetRoundTimelimit( gentity_t *ent, char *params ) {
	char *pString, *token;

	pString = params;
	token = COM_Parse( &pString );
	if ( !token[0] ) {
		G_Error( "G_ScriptAction_SetRoundTimelimit: number parameter required\n" );
	}

	trap_Cvar_Set( "timelimit", token );

	return qtrue;
}


/*
=================
G_ScriptAction_BackupScript

  backs up the current state of the scripting, so we can restore it later and resume
  were we left off (useful if player gets in our way)
=================
*/
qboolean G_ScriptAction_BackupScript( gentity_t *ent, char *params ) {

	// if we're not at the top of an event, then something is _probably_ wrong with the script
//	if (ent->scriptStatus.scriptStackHead > 0) {
//		G_Printf( "ENTITY SCRIPT: WARNING: backupscript not at start of event, possibly harmful.\n");
//	}

	if ( !( ent->scriptStatus.scriptFlags & SCFL_WAITING_RESTORE ) ) {

		// if we are moving, stop here
		if ( ent->scriptStatus.scriptFlags & SCFL_GOING_TO_MARKER ) {
			ent->scriptStatus.scriptFlags &= ~SCFL_GOING_TO_MARKER;

			// set the angles at the destination
			BG_EvaluateTrajectory( &ent->s.apos, level.time, ent->s.angles );
			VectorCopy( ent->s.angles, ent->s.apos.trBase );
			VectorCopy( ent->s.angles, ent->r.currentAngles );
			ent->s.apos.trTime = level.time;
			ent->s.apos.trDuration = 0;
			ent->s.apos.trType = TR_STATIONARY;
			VectorClear( ent->s.apos.trDelta );

			// stop moving
			BG_EvaluateTrajectory( &ent->s.pos, level.time, ent->s.origin );
			VectorCopy( ent->s.origin, ent->s.pos.trBase );
			VectorCopy( ent->s.origin, ent->r.currentOrigin );
			ent->s.pos.trTime = level.time;
			ent->s.pos.trDuration = 0;
			ent->s.pos.trType = TR_STATIONARY;
			VectorClear( ent->s.pos.trDelta );

			script_linkentity( ent );
		}

		ent->scriptStatusBackup = ent->scriptStatusCurrent;
		ent->scriptStatus.scriptFlags |= SCFL_WAITING_RESTORE;
	}

	return qtrue;
}

/*
=================
G_ScriptAction_RestoreScript

  restores the state of the scripting to the previous backup
=================
*/
qboolean G_ScriptAction_RestoreScript( gentity_t *ent, char *params ) {

	ent->scriptStatus = ent->scriptStatusBackup;
	ent->scriptStatus.scriptStackChangeTime = level.time;       // start moves again
	return qfalse;  // dont continue scripting until next frame

}

/*
==================
G_ScriptAction_SetHealth
==================
*/
qboolean G_ScriptAction_SetHealth( gentity_t *ent, char *params ) {
	if ( !params || !params[0] ) {
		G_Error( "G_ScriptAction_SetHealth: sethealth requires a health value\n" );
	}
	ent->health = atoi( params );
	return qtrue;
}