/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/


#include "../client/client.h"

cvar_t *cl_shownet;
// TTimo: win32 dedicated
cvar_t  *cl_language;

void CL_Shutdown( void ) {
}

void CL_Init( void ) {
	cl_shownet = Cvar_Get( "cl_shownet", "0", CVAR_TEMP );
	// TTimo: localisation, prolly not any use in dedicated / null client
	cl_language = Cvar_Get( "cl_language", "0", CVAR_ARCHIVE );
}

void CL_MouseEvent( int dx, int dy, int time ) {
}

void Key_WriteBindings( fileHandle_t f ) {
}

void CL_Frame( int msec ) {
}

void CL_PacketEvent( netadr_t from, msg_t *msg ) {
}

void CL_CharEvent( int key ) {
}

void CL_Disconnect( qboolean showMainMenu ) {
}

void CL_MapLoading( void ) {
}

qboolean CL_GameCommand( void ) {
	return qfalse; // bk001204 - non-void
}

void CL_KeyEvent( int key, qboolean down, unsigned time ) {
}

qboolean UI_GameCommand( void ) {
	return qfalse;
}

void CL_ForwardCommandToServer( const char *string ) {
}

void CL_ConsolePrint( char *txt ) {
}

void CL_JoystickEvent( int axis, int value, int time ) {
}

void CL_InitKeyCommands( void ) {
}

void CL_CDDialog( void ) {
}

void CL_FlushMemory( void ) {
}

void CL_StartHunkUsers( void ) {
}

// bk001119 - added new dummy for sv_init.c
void CL_ShutdownAll( void ) {};

// bk001208 - added new dummy (RC4)
qboolean CL_CDKeyValidate( const char *key, const char *checksum ) { return qtrue; }

// TTimo added for win32 dedicated
void Key_ClearStates( void ) {
}
