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

// sv_bot.c

#include "server.h"
#include "../game/botlib.h"
#include "../botai/botai.h"

#define MAX_DEBUGPOLYS      128

typedef struct bot_debugpoly_s
{
	int inuse;
	int color;
	int numPoints;
	vec3_t points[128];
} bot_debugpoly_t;

static bot_debugpoly_t debugpolygons[MAX_DEBUGPOLYS];

extern botlib_export_t  *botlib_export;
int bot_enable;

/*
==================
SV_BotAllocateClient
==================
*/
int SV_BotAllocateClient( void ) {
	int i;
	client_t    *cl;

	// find a client slot
	for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ ) {
		// Wolfenstein, never use the first slot, otherwise if a bot connects before the first client on a listen server, game won't start
		if ( i < 1 ) {
			continue;
		}
		// done.
		if ( cl->state == CS_FREE ) {
			break;
		}
	}

	if ( i == sv_maxclients->integer ) {
		return -1;
	}

	cl->gentity = SV_GentityNum( i );
	cl->gentity->s.number = i;
	cl->state = CS_ACTIVE;
	cl->lastPacketTime = svs.time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	return i;
}

/*
==================
SV_BotFreeClient
==================
*/
void SV_BotFreeClient( int clientNum ) {
	client_t    *cl;

	if ( clientNum < 0 || clientNum >= sv_maxclients->integer ) {
		Com_Error( ERR_DROP, "SV_BotFreeClient: bad clientNum: %i", clientNum );
	}
	cl = &svs.clients[clientNum];
	cl->state = CS_FREE;
	cl->name[0] = 0;
	if ( cl->gentity ) {
		cl->gentity->r.svFlags &= ~SVF_BOT;
	}
}

/*
==================
BotDrawDebugPolygons
==================
*/
void BotDrawDebugPolygons( void ( *drawPoly )( int color, int numPoints, float *points ), int value ) {
	static cvar_t *bot_debug, *bot_groundonly, *bot_reachability, *bot_highlightarea;
	static cvar_t *bot_testhidepos, *bot_testroutevispos;
	bot_debugpoly_t *poly;
	int i, parm0;

	if ( !bot_enable ) {
		return;
	}
	//bot debugging
	if ( !bot_debug ) {
		bot_debug = Cvar_Get( "bot_debug", "0", 0 );
	}
	//show reachabilities
	if ( !bot_reachability ) {
		bot_reachability = Cvar_Get( "bot_reachability", "0", 0 );
	}
	//show ground faces only
	if ( !bot_groundonly ) {
		bot_groundonly = Cvar_Get( "bot_groundonly", "1", 0 );
	}
	//get the hightlight area
	if ( !bot_highlightarea ) {
		bot_highlightarea = Cvar_Get( "bot_highlightarea", "0", 0 );
	}
	//
	if ( !bot_testhidepos ) {
		bot_testhidepos = Cvar_Get( "bot_testhidepos", "0", 0 );
	}
	//
	if ( !bot_testroutevispos ) {
		bot_testroutevispos = Cvar_Get( "bot_testroutevispos", "0", 0 );
	}
	//
	if ( bot_debug->integer ) {
		parm0 = 0;
		if ( svs.clients[0].lastUsercmd.buttons & BUTTON_ATTACK ) {
			parm0 |= 1;
		}
		if ( bot_reachability->integer ) {
			parm0 |= 2;
		}
		if ( bot_groundonly->integer ) {
			parm0 |= 4;
		}
		botlib_export->BotLibVarSet( "bot_highlightarea", bot_highlightarea->string );
		botlib_export->BotLibVarSet( "bot_testhidepos", bot_testhidepos->string );
		botlib_export->BotLibVarSet( "bot_testroutevispos", bot_testroutevispos->string );
		botlib_export->Test( parm0, NULL, svs.clients[0].gentity->r.currentOrigin,
							 svs.clients[0].gentity->r.currentAngles );
	} //end if
	for ( i = 0; i < MAX_DEBUGPOLYS; i++ ) {
		poly = &debugpolygons[i];
		if ( !poly->inuse ) {
			continue;
		}
		drawPoly( poly->color, poly->numPoints, (float *) poly->points );
		//Com_Printf("poly %i, numpoints = %d\n", i, poly->numPoints);
	}
}

/*
==================
BotImport_Print
==================
*/
void QDECL BotImport_Print( int type, char *fmt, ... ) {
	char str[2048];
	va_list ap;

	va_start( ap, fmt );
	vsprintf( str, fmt, ap );
	va_end( ap );

	switch ( type ) {
	case PRT_MESSAGE: {
		Com_Printf( "%s", str );
		break;
	}
	case PRT_WARNING: {
		Com_Printf( S_COLOR_YELLOW "Warning: %s", str );
		break;
	}
	case PRT_ERROR: {
		Com_Printf( S_COLOR_RED "Error: %s", str );
		break;
	}
	case PRT_FATAL: {
		Com_Printf( S_COLOR_RED "Fatal: %s", str );
		break;
	}
	case PRT_EXIT: {
		Com_Error( ERR_DROP, S_COLOR_RED "Exit: %s", str );
		break;
	}
	default: {
		Com_Printf( "unknown print type\n" );
		break;
	}
	}
}

/*
==================
BotImport_Trace
==================
*/
void BotImport_Trace( bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask ) {
	trace_t trace;

	// always use bounding box for bot stuff ?
	SV_Trace( &trace, start, mins, maxs, end, passent, contentmask, qfalse );
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy( trace.endpos, bsptrace->endpos );
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy( trace.plane.normal, bsptrace->plane.normal );
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}

/*
==================
BotImport_EntityTrace
==================
*/
void BotImport_EntityTrace( bsp_trace_t *bsptrace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask ) {
	trace_t trace;

	// always use bounding box for bot stuff ?
	SV_ClipToEntity( &trace, start, mins, maxs, end, entnum, contentmask, qfalse );
	//copy the trace information
	bsptrace->allsolid = trace.allsolid;
	bsptrace->startsolid = trace.startsolid;
	bsptrace->fraction = trace.fraction;
	VectorCopy( trace.endpos, bsptrace->endpos );
	bsptrace->plane.dist = trace.plane.dist;
	VectorCopy( trace.plane.normal, bsptrace->plane.normal );
	bsptrace->plane.signbits = trace.plane.signbits;
	bsptrace->plane.type = trace.plane.type;
	bsptrace->surface.value = trace.surfaceFlags;
	bsptrace->ent = trace.entityNum;
	bsptrace->exp_dist = 0;
	bsptrace->sidenum = 0;
	bsptrace->contents = 0;
}


/*
==================
BotImport_PointContents
==================
*/
int BotImport_PointContents( vec3_t point ) {
	return SV_PointContents( point, -1 );
}

/*
==================
BotImport_inPVS
==================
*/
int BotImport_inPVS( vec3_t p1, vec3_t p2 ) {
	return SV_inPVS( p1, p2 );
}

/*
==================
BotImport_BSPEntityData
==================
*/
char *BotImport_BSPEntityData( void ) {
	return CM_EntityString();
}

/*
==================
BotImport_BSPModelMinsMaxsOrigin
==================
*/
void BotImport_BSPModelMinsMaxsOrigin( int modelnum, vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin ) {
	clipHandle_t h;
	vec3_t mins, maxs;
	float max;
	int i;

	h = CM_InlineModel( modelnum );
	CM_ModelBounds( h, mins, maxs );
	//if the model is rotated
	if ( ( angles[0] || angles[1] || angles[2] ) ) {
		// expand for rotation

		max = RadiusFromBounds( mins, maxs );
		for ( i = 0; i < 3; i++ ) {
			mins[i] = -max;
			maxs[i] = max;
		}
	}
	if ( outmins ) {
		VectorCopy( mins, outmins );
	}
	if ( outmaxs ) {
		VectorCopy( maxs, outmaxs );
	}
	if ( origin ) {
		VectorClear( origin );
	}
}

/*
==================
BotImport_GetMemory
==================
*/
void *BotImport_GetMemory( int size ) {
	return malloc( size );
}

/*
==================
BotImport_FreeMemory
==================
*/
void BotImport_FreeMemory( void *ptr ) {
	free( ptr );
}

/*
=================
BotImport_HunkAlloc
=================
*/
void *BotImport_HunkAlloc( int size ) {
	if ( Hunk_CheckMark() ) {
		Com_Error( ERR_DROP, "SV_Bot_HunkAlloc: Alloc with marks already set\n" );
	}
	return Hunk_Alloc( size, h_high );
}

/*
==================
BotImport_DebugPolygonCreate
==================
*/
int BotImport_DebugPolygonCreate( int color, int numPoints, vec3_t *points ) {
	bot_debugpoly_t *poly;
	int i;

	for ( i = 1; i < MAX_DEBUGPOLYS; i++ )    {
		if ( !debugpolygons[i].inuse ) {
			break;
		}
	}
	if ( i >= MAX_DEBUGPOLYS ) {
		return 0;
	}
	poly = &debugpolygons[i];
	poly->inuse = qtrue;
	poly->color = color;
	poly->numPoints = numPoints;
	memcpy( poly->points, points, numPoints * sizeof( vec3_t ) );
	//
	return i;
}

/*
==================
BotImport_DebugPolygonShow
==================
*/
void BotImport_DebugPolygonShow( int id, int color, int numPoints, vec3_t *points ) {
	bot_debugpoly_t *poly;

	poly = &debugpolygons[id];
	poly->inuse = qtrue;
	poly->color = color;
	poly->numPoints = numPoints;
	memcpy( poly->points, points, numPoints * sizeof( vec3_t ) );
}

/*
==================
BotImport_DebugPolygonDelete
==================
*/
void BotImport_DebugPolygonDelete( int id ) {
	debugpolygons[id].inuse = qfalse;
}

/*
==================
BotImport_DebugLineCreate
==================
*/
int BotImport_DebugLineCreate( void ) {
	vec3_t points[1];
	return BotImport_DebugPolygonCreate( 0, 0, points );
}

/*
==================
BotImport_DebugLineDelete
==================
*/
void BotImport_DebugLineDelete( int line ) {
	BotImport_DebugPolygonDelete( line );
}

/*
==================
BotImport_DebugLineShow
==================
*/
void BotImport_DebugLineShow( int line, vec3_t start, vec3_t end, int color ) {
	vec3_t points[4], dir, cross, up = {0, 0, 1};
	float dot;

	VectorCopy( start, points[0] );
	VectorCopy( start, points[1] );
	//points[1][2] -= 2;
	VectorCopy( end, points[2] );
	//points[2][2] -= 2;
	VectorCopy( end, points[3] );


	VectorSubtract( end, start, dir );
	VectorNormalize( dir );
	dot = DotProduct( dir, up );
	if ( dot > 0.99 || dot < -0.99 ) {
		VectorSet( cross, 1, 0, 0 );
	} else { CrossProduct( dir, up, cross );}

	VectorNormalize( cross );

	VectorMA( points[0], 2, cross, points[0] );
	VectorMA( points[1], -2, cross, points[1] );
	VectorMA( points[2], -2, cross, points[2] );
	VectorMA( points[3], 2, cross, points[3] );

	BotImport_DebugPolygonShow( line, color, 4, points );
}

/*
==================
SV_BotClientCommand
==================
*/
void BotClientCommand( int client, char *command ) {
	SV_ExecuteClientCommand( &svs.clients[client], command, qtrue );
}

/*
==================
SV_BotFrame
==================
*/
void SV_BotFrame( int time ) {
	if ( !bot_enable ) {
		return;
	}
	//NOTE: maybe the game is already shutdown
	if ( !gvm ) {
		return;
	}
	VM_Call( gvm, BOTAI_START_FRAME, time );
}

/*
===============
SV_BotLibSetup
===============
*/
int SV_BotLibSetup( void ) {
	if ( !bot_enable ) {
		return 0;
	}

	if ( !botlib_export ) {
		Com_Printf( S_COLOR_RED "Error: SV_BotLibSetup without SV_BotInitBotLib\n" );
		return -1;
	}

	return botlib_export->BotLibSetup();
}

/*
===============
SV_ShutdownBotLib

Called when either the entire server is being killed, or
it is changing to a different game directory.
===============
*/
int SV_BotLibShutdown( void ) {

	if ( !botlib_export ) {
		return -1;
	}

	return botlib_export->BotLibShutdown();
}

/*
==================
SV_BotInitCvars
==================
*/
void SV_BotInitCvars( void ) {

	Cvar_Get( "bot_enable", "1", 0 );               //enable the bot
	Cvar_Get( "bot_developer", "0", 0 );            //bot developer mode
	Cvar_Get( "bot_debug", "0", 0 );                //enable bot debugging
	Cvar_Get( "bot_groundonly", "1", 0 );           //only show ground faces of areas
	Cvar_Get( "bot_reachability", "0", 0 );     //show all reachabilities to other areas
	Cvar_Get( "bot_thinktime", "100", 0 );      //msec the bots thinks
	Cvar_Get( "bot_reloadcharacters", "0", 0 ); //reload the bot characters each time
	Cvar_Get( "bot_testichat", "0", 0 );            //test ichats
	Cvar_Get( "bot_testrchat", "0", 0 );            //test rchats
	Cvar_Get( "bot_fastchat", "0", 0 );         //fast chatting bots
	Cvar_Get( "bot_nochat", "0", 0 );               //disable chats
	Cvar_Get( "bot_grapple", "0", 0 );          //enable grapple
	Cvar_Get( "bot_rocketjump", "1", 0 );           //enable rocket jumping
	Cvar_Get( "bot_miniplayers", "0", 0 );      //minimum players in a team or the game
}

// Ridah, Cast AI
/*
===============
BotImport_AICast_VisibleFromPos
===============
*/
qboolean BotImport_AICast_VisibleFromPos(   vec3_t srcpos, int srcnum,
											vec3_t destpos, int destnum, qboolean updateVisPos ) {
	return VM_Call( gvm, AICAST_VISIBLEFROMPOS, (int)srcpos, srcnum, (int)destpos, destnum, updateVisPos );
}

/*
===============
BotImport_AICast_CheckAttackAtPos
===============
*/
qboolean BotImport_AICast_CheckAttackAtPos( int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld ) {
	return VM_Call( gvm, AICAST_CHECKATTACKATPOS, entnum, enemy, (int)pos, ducking, allowHitWorld );
}
// done.

/*
==================
SV_BotInitBotLib
==================
*/
void SV_BotInitBotLib( void ) {
	botlib_import_t botlib_import;

#if COPY_PROTECT
	if ( !Cvar_VariableValue( "fs_restrict" ) && !Sys_CheckCD() ) {
		Com_Error( ERR_NEED_CD, "Game CD not in drive" );
	}
#else
	Com_Printf( "Bypassing CD checks\n" );
#endif

	/*
	if ( botlib_export ) {
		SV_BotLibShutdown();
	}*/

	botlib_import.Print = BotImport_Print;
	botlib_import.Trace = BotImport_Trace;
	botlib_import.EntityTrace = BotImport_EntityTrace;
	botlib_import.PointContents = BotImport_PointContents;
	botlib_import.inPVS = BotImport_inPVS;
	botlib_import.BSPEntityData = BotImport_BSPEntityData;
	botlib_import.BSPModelMinsMaxsOrigin = BotImport_BSPModelMinsMaxsOrigin;
	botlib_import.BotClientCommand = BotClientCommand;

	//memory management
	botlib_import.GetMemory = BotImport_GetMemory;
	botlib_import.FreeMemory = BotImport_FreeMemory;
	botlib_import.HunkAlloc = BotImport_HunkAlloc;

	// file system acess
	botlib_import.FS_FOpenFile = FS_FOpenFileByMode;
	botlib_import.FS_Read = FS_Read;
	botlib_import.FS_Write = FS_Write;
	botlib_import.FS_FCloseFile = FS_FCloseFile;
	botlib_import.FS_Seek = FS_Seek;

	//debug lines
	botlib_import.DebugLineCreate = BotImport_DebugLineCreate;
	botlib_import.DebugLineDelete = BotImport_DebugLineDelete;
	botlib_import.DebugLineShow = BotImport_DebugLineShow;

	//debug polygons
	botlib_import.DebugPolygonCreate = BotImport_DebugPolygonCreate;
	botlib_import.DebugPolygonDelete = BotImport_DebugPolygonDelete;

	// Ridah, Cast AI
	botlib_import.AICast_VisibleFromPos = BotImport_AICast_VisibleFromPos;
	botlib_import.AICast_CheckAttackAtPos = BotImport_AICast_CheckAttackAtPos;
	// done.

	botlib_export = (botlib_export_t *)GetBotLibAPI( BOTLIB_API_VERSION, &botlib_import );
}


//
//  * * * BOT AI CODE IS BELOW THIS POINT * * *
//

/*
==================
SV_BotGetConsoleMessage
==================
*/
int SV_BotGetConsoleMessage( int client, char *buf, int size ) {
	client_t    *cl;
	int index;
	char        *msg;

	cl = &svs.clients[client];
	cl->lastPacketTime = svs.time;

	if ( cl->reliableAcknowledge == cl->reliableSequence ) {
		return qfalse;
	}

	cl->reliableAcknowledge++;
	index = cl->reliableAcknowledge & ( MAX_RELIABLE_COMMANDS - 1 );

	//if ( !cl->reliableCommands[index][0] ) {
	if ( !( msg = SV_GetReliableCommand( cl, index ) ) || !msg[0] ) {
		return qfalse;
	}

	//Q_strncpyz( buf, cl->reliableCommands[index], size );
	Q_strncpyz( buf, msg, size );
	return qtrue;
}

#if 0
/*
==================
EntityInPVS
==================
*/
int EntityInPVS( int client, int entityNum ) {
	client_t            *cl;
	clientSnapshot_t    *frame;
	int i;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	for ( i = 0; i < frame->num_entities; i++ ) {
		if ( svs.snapshotEntities[( frame->first_entity + i ) % svs.numSnapshotEntities].number == entityNum ) {
			return qtrue;
		}
	}
	return qfalse;
}
#endif

/*
==================
SV_BotGetSnapshotEntity
==================
*/
int SV_BotGetSnapshotEntity( int client, int sequence ) {
	client_t            *cl;
	clientSnapshot_t    *frame;

	cl = &svs.clients[client];
	frame = &cl->frames[cl->netchan.outgoingSequence & PACKET_MASK];
	if ( sequence < 0 || sequence >= frame->num_entities ) {
		return -1;
	}
	return svs.snapshotEntities[( frame->first_entity + sequence ) % svs.numSnapshotEntities].number;
}

