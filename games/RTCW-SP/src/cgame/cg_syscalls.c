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

// cg_syscalls.c -- this file is only included when building a dll
// cg_syscalls.asm is included instead when building a qvm
#include "cg_local.h"

static int ( QDECL * syscall )( int arg, ... ) = ( int ( QDECL * )( int, ... ) ) - 1;

// TTimo: guarding
#if defined( __MACOS__ )
#pragma export on
#endif
void dllEntry( int ( QDECL  *syscallptr )( int arg,... ) ) {
#if defined( __MACOS__ )
#pragma export off
#endif
	syscall = syscallptr;
}


int PASSFLOAT( float x ) {
	float floatTemp;
	floatTemp = x;
	return *(int *)&floatTemp;
}

void    trap_Print( const char *fmt ) {
	syscall( CG_PRINT, fmt );
}

void    trap_Error( const char *fmt ) {
	syscall( CG_ERROR, fmt );
}

int     trap_Milliseconds( void ) {
	return syscall( CG_MILLISECONDS );
}

void    trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags ) {
	syscall( CG_CVAR_REGISTER, vmCvar, varName, defaultValue, flags );
}

void    trap_Cvar_Update( vmCvar_t *vmCvar ) {
	syscall( CG_CVAR_UPDATE, vmCvar );
}

void    trap_Cvar_Set( const char *var_name, const char *value ) {
	syscall( CG_CVAR_SET, var_name, value );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	syscall( CG_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

int     trap_Argc( void ) {
	return syscall( CG_ARGC );
}

void    trap_Argv( int n, char *buffer, int bufferLength ) {
	syscall( CG_ARGV, n, buffer, bufferLength );
}

void    trap_Args( char *buffer, int bufferLength ) {
	syscall( CG_ARGS, buffer, bufferLength );
}

int     trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( CG_FS_FOPENFILE, qpath, f, mode );
}

void    trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	syscall( CG_FS_READ, buffer, len, f );
}

void    trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	syscall( CG_FS_WRITE, buffer, len, f );
}

void    trap_FS_FCloseFile( fileHandle_t f ) {
	syscall( CG_FS_FCLOSEFILE, f );
}

void    trap_SendConsoleCommand( const char *text ) {
	syscall( CG_SENDCONSOLECOMMAND, text );
}

void    trap_AddCommand( const char *cmdName ) {
	syscall( CG_ADDCOMMAND, cmdName );
}

void    trap_SendClientCommand( const char *s ) {
	syscall( CG_SENDCLIENTCOMMAND, s );
}

void    trap_UpdateScreen( void ) {
	syscall( CG_UPDATESCREEN );
}

void    trap_CM_LoadMap( const char *mapname ) {
	syscall( CG_CM_LOADMAP, mapname );
}

int     trap_CM_NumInlineModels( void ) {
	return syscall( CG_CM_NUMINLINEMODELS );
}

clipHandle_t trap_CM_InlineModel( int index ) {
	return syscall( CG_CM_INLINEMODEL, index );
}

clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs ) {
	return syscall( CG_CM_TEMPBOXMODEL, mins, maxs );
}

clipHandle_t trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs ) {
	return syscall( CG_CM_TEMPCAPSULEMODEL, mins, maxs );
}

int     trap_CM_PointContents( const vec3_t p, clipHandle_t model ) {
	return syscall( CG_CM_POINTCONTENTS, p, model );
}

int     trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
	return syscall( CG_CM_TRANSFORMEDPOINTCONTENTS, p, model, origin, angles );
}

void    trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
						  const vec3_t mins, const vec3_t maxs,
						  clipHandle_t model, int brushmask ) {
	syscall( CG_CM_BOXTRACE, results, start, end, mins, maxs, model, brushmask );
}

void    trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
									 const vec3_t mins, const vec3_t maxs,
									 clipHandle_t model, int brushmask,
									 const vec3_t origin, const vec3_t angles ) {
	syscall( CG_CM_TRANSFORMEDBOXTRACE, results, start, end, mins, maxs, model, brushmask, origin, angles );
}

void    trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
							  const vec3_t mins, const vec3_t maxs,
							  clipHandle_t model, int brushmask ) {
	syscall( CG_CM_CAPSULETRACE, results, start, end, mins, maxs, model, brushmask );
}

void    trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
										 const vec3_t mins, const vec3_t maxs,
										 clipHandle_t model, int brushmask,
										 const vec3_t origin, const vec3_t angles ) {
	syscall( CG_CM_TRANSFORMEDCAPSULETRACE, results, start, end, mins, maxs, model, brushmask, origin, angles );
}

int     trap_CM_MarkFragments( int numPoints, const vec3_t *points,
							   const vec3_t projection,
							   int maxPoints, vec3_t pointBuffer,
							   int maxFragments, markFragment_t *fragmentBuffer ) {
	return syscall( CG_CM_MARKFRAGMENTS, numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
}

void    trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx ) {
	syscall( CG_S_STARTSOUND, origin, entityNum, entchannel, sfx );
}

//----(SA)	added
void    trap_S_StartSoundEx( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags ) {
	syscall( CG_S_STARTSOUNDEX, origin, entityNum, entchannel, sfx, flags );
}
//----(SA)	end

void    trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	syscall( CG_S_STARTLOCALSOUND, sfx, channelNum );
}

void    trap_S_ClearLoopingSounds( qboolean killall ) {
	syscall( CG_S_CLEARLOOPINGSOUNDS, killall );
}

void    trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int volume ) {
	syscall( CG_S_ADDLOOPINGSOUND, entityNum, origin, velocity, 1250, sfx, volume );     // volume was previously removed from CG_S_ADDLOOPINGSOUND.  I added 'range'
}

//----(SA)	added
void    trap_S_AddRangedLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range ) {
	syscall( CG_S_ADDLOOPINGSOUND, entityNum, origin, velocity, range, sfx, 255 );   // RF, assume full volume, since thats how it worked before
}
//----(SA)	end

void    trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx ) {
// not in use
//	syscall( CG_S_ADDREALLOOPINGSOUND, entityNum, origin, velocity, 1250, sfx, 255 );	//----(SA)	modified
}

void    trap_S_StopLoopingSound( int entityNum ) {
	syscall( CG_S_STOPLOOPINGSOUND, entityNum );
}

//----(SA)	added
void    trap_S_StopStreamingSound( int entityNum ) {
	syscall( CG_S_STOPSTREAMINGSOUND, entityNum );
}
//----(SA)	end

void    trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	syscall( CG_S_UPDATEENTITYPOSITION, entityNum, origin );
}

// Ridah, talking animations
int     trap_S_GetVoiceAmplitude( int entityNum ) {
	return syscall( CG_S_GETVOICEAMPLITUDE, entityNum );
}
// done.

void    trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater ) {
	syscall( CG_S_RESPATIALIZE, entityNum, origin, axis, inwater );
}

sfxHandle_t trap_S_RegisterSound( const char *sample ) {
	CG_DrawInformation();
	return syscall( CG_S_REGISTERSOUND, sample );
}

void    trap_S_StartBackgroundTrack( const char *intro, const char *loop, int fadeupTime ) {
	syscall( CG_S_STARTBACKGROUNDTRACK, intro, loop, fadeupTime );
}

void    trap_S_FadeBackgroundTrack( float targetvol, int time, int num ) {   // yes, i know.  fadebackground coming in, fadestreaming going out.  will have to see where functionality leads...
	syscall( CG_S_FADESTREAMINGSOUND, PASSFLOAT( targetvol ), time, num ); // 'num' is '0' if it's music, '1' if it's "all streaming sounds"
}

void    trap_S_FadeAllSound( float targetvol, int time ) {
	syscall( CG_S_FADEALLSOUNDS, PASSFLOAT( targetvol ), time );
}

//----(SA)	end

void    trap_S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation ) {
	syscall( CG_S_STARTSTREAMINGSOUND, intro, loop, entnum, channel, attenuation );
}

void    trap_R_LoadWorldMap( const char *mapname ) {
	syscall( CG_R_LOADWORLDMAP, mapname );
}

qhandle_t trap_R_RegisterModel( const char *name ) {
	CG_DrawInformation();
	return syscall( CG_R_REGISTERMODEL, name );
}

//----(SA)	added
qboolean trap_R_GetSkinModel( qhandle_t skinid, const char *type, char *name ) {
	return syscall( CG_R_GETSKINMODEL, skinid, type, name );
}

qhandle_t trap_R_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap ) {
	return syscall( CG_R_GETMODELSHADER, modelid, surfnum, withlightmap );
}
//----(SA)	end

qhandle_t trap_R_RegisterSkin( const char *name ) {
	CG_DrawInformation();
	return syscall( CG_R_REGISTERSKIN, name );
}

qhandle_t trap_R_RegisterShader( const char *name ) {
	CG_DrawInformation();
	return syscall( CG_R_REGISTERSHADER, name );
}

qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	CG_DrawInformation();
	return syscall( CG_R_REGISTERSHADERNOMIP, name );
}

void trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font ) {
	syscall( CG_R_REGISTERFONT, fontName, pointSize, font );
}

void    trap_R_ClearScene( void ) {
	syscall( CG_R_CLEARSCENE );
}

void    trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	syscall( CG_R_ADDREFENTITYTOSCENE, re );
}

void    trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	syscall( CG_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

// Ridah
void    trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys ) {
	syscall( CG_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, numPolys );
}

void    trap_RB_ZombieFXAddNewHit( int entityNum, const vec3_t hitPos, const vec3_t hitDir ) {
	syscall( CG_RB_ZOMBIEFXADDNEWHIT, entityNum, hitPos, hitDir );
}
// done.

void    trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw ) {
	syscall( CG_R_ADDLIGHTTOSCENE, org, PASSFLOAT( intensity ), PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), overdraw );
}

//----(SA)
void    trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int flags ) {
	syscall( CG_R_ADDCORONATOSCENE, org, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( scale ), id, flags );
}
//----(SA)

//----(SA)
void    trap_R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density ) {
	syscall( CG_R_SETFOG, fogvar, var1, var2, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( density ) );
}
//----(SA)
void    trap_R_RenderScene( const refdef_t *fd ) {
	syscall( CG_R_RENDERSCENE, fd );
}

void    trap_R_SetColor( const float *rgba ) {
	syscall( CG_R_SETCOLOR, rgba );
}

void    trap_R_DrawStretchPic( float x, float y, float w, float h,
							   float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	syscall( CG_R_DRAWSTRETCHPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader );
}

void    trap_R_DrawStretchPicGradient(  float x, float y, float w, float h,
										float s1, float t1, float s2, float t2, qhandle_t hShader,
										const float *gradientColor, int gradientType ) {
	syscall( CG_R_DRAWSTRETCHPIC_GRADIENT, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader, gradientColor, gradientType  );
}

void    trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	syscall( CG_R_MODELBOUNDS, model, mins, maxs );
}

int     trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex ) {
	return syscall( CG_R_LERPTAG, tag, refent, tagName, startIndex );
}

void    trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset ) {
	syscall( CG_R_REMAP_SHADER, oldShader, newShader, timeOffset );
}

void        trap_GetGlconfig( glconfig_t *glconfig ) {
	syscall( CG_GETGLCONFIG, glconfig );
}

void        trap_GetGameState( gameState_t *gamestate ) {
	syscall( CG_GETGAMESTATE, gamestate );
}

void        trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime ) {
	syscall( CG_GETCURRENTSNAPSHOTNUMBER, snapshotNumber, serverTime );
}

qboolean    trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot ) {
	return syscall( CG_GETSNAPSHOT, snapshotNumber, snapshot );
}

qboolean    trap_GetServerCommand( int serverCommandNumber ) {
	return syscall( CG_GETSERVERCOMMAND, serverCommandNumber );
}

int         trap_GetCurrentCmdNumber( void ) {
	return syscall( CG_GETCURRENTCMDNUMBER );
}

qboolean    trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd ) {
	return syscall( CG_GETUSERCMD, cmdNumber, ucmd );
}

void        trap_SetUserCmdValue( int stateValue, int holdableValue, float sensitivityScale, int cld ) {    //----(SA)	// NERVE - SMF - added cld
	syscall( CG_SETUSERCMDVALUE, stateValue, holdableValue, PASSFLOAT( sensitivityScale ), cld );
}

void        testPrintInt( char *string, int i ) {
	syscall( CG_TESTPRINTINT, string, i );
}

void        testPrintFloat( char *string, float f ) {
	syscall( CG_TESTPRINTFLOAT, string, PASSFLOAT( f ) );
}

int trap_MemoryRemaining( void ) {
	return syscall( CG_MEMORY_REMAINING );
}

qboolean trap_loadCamera( int camNum, const char *name ) {
	return syscall( CG_LOADCAMERA, camNum, name );
}

void trap_startCamera( int camNum, int time ) {
	syscall( CG_STARTCAMERA, camNum, time );
}

//----(SA)	added
void trap_stopCamera( int camNum ) {
	syscall( CG_STOPCAMERA, camNum );
}
//----(SA)	end

qboolean trap_getCameraInfo( int camNum, int time, vec3_t *origin, vec3_t *angles, float *fov ) {
	return syscall( CG_GETCAMERAINFO, camNum, time, origin, angles, fov );
}


qboolean trap_Key_IsDown( int keynum ) {
	return syscall( CG_KEY_ISDOWN, keynum );
}

int trap_Key_GetCatcher( void ) {
	return syscall( CG_KEY_GETCATCHER );
}

void trap_Key_SetCatcher( int catcher ) {
	syscall( CG_KEY_SETCATCHER, catcher );
}

int trap_Key_GetKey( const char *binding ) {
	return syscall( CG_KEY_GETKEY, binding );
}


int trap_PC_AddGlobalDefine( char *define ) {
	return syscall( CG_PC_ADD_GLOBAL_DEFINE, define );
}

int trap_PC_LoadSource( const char *filename ) {
	return syscall( CG_PC_LOAD_SOURCE, filename );
}

int trap_PC_FreeSource( int handle ) {
	return syscall( CG_PC_FREE_SOURCE, handle );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return syscall( CG_PC_READ_TOKEN, handle, pc_token );
}


int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return syscall( CG_PC_SOURCE_FILE_AND_LINE, handle, filename, line );
}

void    trap_S_StopBackgroundTrack( void ) {
	syscall( CG_S_STOPBACKGROUNDTRACK );
}

int trap_RealTime( qtime_t *qtime ) {
	return syscall( CG_REAL_TIME, qtime );
}

void trap_SnapVector( float *v ) {
	syscall( CG_SNAPVECTOR, v );
}

void trap_SendMoveSpeedsToGame( int entnum, char *movespeeds ) {
	syscall( CG_SENDMOVESPEEDSTOGAME, entnum, movespeeds );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits ) {
	return syscall( CG_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits );
}

// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic( int handle ) {
	return syscall( CG_CIN_STOPCINEMATIC, handle );
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic( int handle ) {
	return syscall( CG_CIN_RUNCINEMATIC, handle );
}


// draws the current frame
void trap_CIN_DrawCinematic( int handle ) {
	syscall( CG_CIN_DRAWCINEMATIC, handle );
}


// allows you to resize the animation dynamically
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h ) {
	syscall( CG_CIN_SETEXTENTS, handle, x, y, w, h );
}

qboolean trap_GetEntityToken( char *buffer, int bufferSize ) {
	return syscall( CG_GET_ENTITY_TOKEN, buffer, bufferSize );
}

//----(SA)	added
// bring up a popup menu
extern void Menus_OpenByName( const char *p );

void trap_UI_Popup( const char *arg0 ) {
	syscall( CG_INGAME_POPUP, arg0 );
}

// NERVE - SMF
void trap_UI_LimboChat( const char *arg0 ) {
	syscall( CG_LIMBOCHAT, arg0 );
}

void trap_UI_ClosePopup( const char *arg0 ) {
	syscall( CG_INGAME_CLOSEPOPUP, arg0 );
}
// -NERVE - SMF

qboolean trap_GetModelInfo( int clientNum, char *modelName, animModelInfo_t **modelInfo ) {
	return syscall( CG_GETMODELINFO, clientNum, modelName, modelInfo );
}