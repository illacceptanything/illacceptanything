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

// win_local.h: Win32-specific Quake3 header file

#if defined ( _MSC_VER ) && ( _MSC_VER >= 1200 )
#pragma warning(disable : 4201)
#pragma warning( push )
#endif
#include <windows.h>
#if defined ( _MSC_VER ) && ( _MSC_VER >= 1200 )
#pragma warning( pop )
#endif

#define DIRECTINPUT_VERSION 0x0800

#ifdef DOOMSOUND    ///// (SA) DOOMSOUND
#include "../mssdk/include/dinput.h"
#include "../mssdk/include/dsound.h"
#else
#include <dinput.h>
#include <dsound.h>
#endif  ///// (SA) DOOMSOUND

#include <winsock.h>
#include <wsipx.h>

#ifdef DOOMSOUND    ///// (SA)DOOMSOUND
#ifdef __cplusplus
extern "C" {
#endif
#endif  ///// (SA) DOOMSOUND

void    IN_MouseEvent( int mstate );

void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

void    Sys_CreateConsole( void );
void    Sys_DestroyConsole( void );

char    *Sys_ConsoleInput( void );

qboolean    Sys_GetPacket( netadr_t *net_from, msg_t *net_message );

// Input subsystem

void    IN_Init( void );
void    IN_Shutdown( void );
void    IN_JoystickCommands( void );

void    IN_Move( usercmd_t *cmd );
// add additional non keyboard / non mouse movement on top of the keyboard move cmd

void    IN_DeactivateWin32Mouse( void );

void    IN_Activate( qboolean active );
void    IN_Frame( void );

// window procedure
LONG WINAPI MainWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam );

void Conbuf_AppendText( const char *msg );

void SNDDMA_Activate( void );
int  SNDDMA_InitDS();

typedef struct
{

	HINSTANCE reflib_library;           // Handle to refresh DLL
	qboolean reflib_active;

	HWND hWnd;
	HINSTANCE hInstance;
	qboolean activeApp;
	qboolean isMinimized;
	OSVERSIONINFO osversion;

	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event
	unsigned sysMsgTime;
} WinVars_t;

extern WinVars_t g_wv;

#ifdef DOOMSOUND    ///// (SA) DOOMSOUND
#ifdef __cplusplus
}
#endif
#endif  ///// (SA) DOOMSOUND

