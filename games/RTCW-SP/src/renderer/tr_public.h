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

#ifndef __TR_PUBLIC_H
#define __TR_PUBLIC_H

#include "../cgame/tr_types.h"

#define REF_API_VERSION     8

//
// these are the functions exported by the refresh module
//
typedef struct {
	// called before the library is unloaded
	// if the system is just reconfiguring, pass destroyWindow = qfalse,
	// which will keep the screen from flashing to the desktop.
	void ( *Shutdown )( qboolean destroyWindow );

	// All data that will be used in a level should be
	// registered before rendering any frames to prevent disk hits,
	// but they can still be registered at a later time
	// if necessary.
	//
	// BeginRegistration makes any existing media pointers invalid
	// and returns the current gl configuration, including screen width
	// and height, which can be used by the client to intelligently
	// size display elements
	void ( *BeginRegistration )( glconfig_t *config );
	qhandle_t ( *RegisterModel )( const char *name );
	qhandle_t ( *RegisterSkin )( const char *name );
	qhandle_t ( *RegisterShader )( const char *name );
	qhandle_t ( *RegisterShaderNoMip )( const char *name );
	void ( *LoadWorld )( const char *name );
	qboolean ( *GetSkinModel )( qhandle_t skinid, const char *type, char *name );    //----(SA)	added
	qhandle_t ( *GetShaderFromModel )( qhandle_t modelid, int surfnum, int withlightmap );                //----(SA)	added

	// the vis data is a large enough block of data that we go to the trouble
	// of sharing it with the clipmodel subsystem
	void ( *SetWorldVisData )( const byte *vis );

	// EndRegistration will draw a tiny polygon with each texture, forcing
	// them to be loaded into card memory
	void ( *EndRegistration )( void );

	// a scene is built up by calls to R_ClearScene and the various R_Add functions.
	// Nothing is drawn until R_RenderScene is called.
	void ( *ClearScene )( void );
	void ( *AddRefEntityToScene )( const refEntity_t *re );
	int ( *LightForPoint )( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );
	void ( *AddPolyToScene )( qhandle_t hShader, int numVerts, const polyVert_t *verts );
	// Ridah
	void ( *AddPolysToScene )( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
	// done.
	void ( *AddLightToScene )( const vec3_t org, float intensity, float r, float g, float b, int overdraw );
//----(SA)
	void ( *AddCoronaToScene )( const vec3_t org, float r, float g, float b, float scale, int id, int flags );
	void ( *SetFog )( int fogvar, int var1, int var2, float r, float g, float b, float density );
//----(SA)
	void ( *RenderScene )( const refdef_t *fd );

	void ( *SetColor )( const float *rgba );    // NULL = 1,1,1,1
	void ( *DrawStretchPic )( float x, float y, float w, float h,
							  float s1, float t1, float s2, float t2, qhandle_t hShader ); // 0 = white
	void ( *DrawStretchPicGradient )( float x, float y, float w, float h,
									  float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType );

	// Draw images for cinematic rendering, pass as 32 bit rgba
	void ( *DrawStretchRaw )( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty );
	void ( *UploadCinematic )( int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty );

	void ( *BeginFrame )( stereoFrame_t stereoFrame );

	// if the pointers are not NULL, timing info will be returned
	void ( *EndFrame )( int *frontEndMsec, int *backEndMsec );


	int ( *MarkFragments )( int numPoints, const vec3_t *points, const vec3_t projection,
							int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );

	int ( *LerpTag )( orientation_t *tag,  const refEntity_t *refent, const char *tagName, int startIndex );
	void ( *ModelBounds )( qhandle_t model, vec3_t mins, vec3_t maxs );

#ifdef __USEA3D
	void ( *A3D_RenderGeometry )( void *pVoidA3D, void *pVoidGeom, void *pVoidMat, void *pVoidGeomStatus );
#endif

	void ( *RegisterFont )( const char *fontName, int pointSize, fontInfo_t *font );
	void ( *RemapShader )( const char *oldShader, const char *newShader, const char *offsetTime );
	// RF
	void ( *ZombieFXAddNewHit )( int entityNum, const vec3_t hitPos, const vec3_t hitDir );

	qboolean ( *GetEntityToken )( char *buffer, int size );
} refexport_t;

//
// these are the functions imported by the refresh module
//
typedef struct {
	// print message on the local console
	void ( QDECL * Printf )( int printLevel, const char *fmt, ... );

	// abort the game
	void ( QDECL * Error )( int errorLevel, const char *fmt, ... );

	// milliseconds should only be used for profiling, never
	// for anything game related.  Get time from the refdef
	int ( *Milliseconds )( void );

	// stack based memory allocation for per-level things that
	// won't be freed
	void ( *Hunk_Clear )( void );
#ifdef HUNK_DEBUG
	void    *( *Hunk_AllocDebug )( int size, ha_pref pref, char *label, char *file, int line );
#else
	void    *( *Hunk_Alloc )( int size, ha_pref pref );
#endif
	void    *( *Hunk_AllocateTempMemory )( int size );
	void ( *Hunk_FreeTempMemory )( void *block );

	cvar_t  *( *Cvar_Get )( const char *name, const char *value, int flags );
	void ( *Cvar_Set )( const char *name, const char *value );

	void ( *Cmd_AddCommand )( const char *name, void( *cmd ) ( void ) );
	void ( *Cmd_RemoveCommand )( const char *name );

	int ( *Cmd_Argc )( void );
	char    *( *Cmd_Argv )( int i );

	void ( *Cmd_ExecuteText )( int exec_when, const char *text );

	// visualization for debugging collision detection
	void ( *CM_DrawDebugSurface )( void( *drawPoly ) ( int color, int numPoints, float *points ) );

	// a -1 return means the file does not exist
	// NULL can be passed for buf to just determine existance
	int ( *FS_FileIsInPAK )( const char *name, int *pChecksum );
	int ( *FS_ReadFile )( const char *name, void **buf );
	void ( *FS_FreeFile )( void *buf );
	char ** ( *FS_ListFiles )( const char *name, const char *extension, int *numfilesfound );
	void ( *FS_FreeFileList )( char **filelist );
	void ( *FS_WriteFile )( const char *qpath, const void *buffer, int size );
	qboolean ( *FS_FileExists )( const char *file );

	// cinematic stuff
	void ( *CIN_UploadCinematic )( int handle );
	int ( *CIN_PlayCinematic )( const char *arg0, int xpos, int ypos, int width, int height, int bits );
	e_status ( *CIN_RunCinematic )( int handle );

} refimport_t;


// this is the only function actually exported at the linker level
// If the module can't init to a valid rendering state, NULL will be
// returned.
refexport_t*GetRefAPI( int apiVersion, refimport_t *rimp );

#endif  // __TR_PUBLIC_H
