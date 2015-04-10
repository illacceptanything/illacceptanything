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
#include "win_local.h"

WinVars_t g_wv;

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL ( WM_MOUSELAST + 1 )  // message that will be supported by the OS
#endif

static UINT MSH_MOUSEWHEEL;

// Console variables that we need to access from this module
cvar_t      *vid_xpos;          // X coordinate of window position
cvar_t      *vid_ypos;          // Y coordinate of window position
cvar_t      *r_fullscreen;

#define VID_NUM_MODES ( sizeof( vid_modes ) / sizeof( vid_modes[0] ) )

LONG WINAPI MainWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static qboolean s_alttab_disabled;

static void WIN_DisableAltTab( void ) {
	if ( s_alttab_disabled ) {
		return;
	}

	if ( !Q_stricmp( Cvar_VariableString( "arch" ), "winnt" ) ) {
		RegisterHotKey( 0, 0, MOD_ALT, VK_TAB );
	} else
	{
		BOOL old;

		SystemParametersInfo( SPI_SCREENSAVERRUNNING, 1, &old, 0 );
	}
	s_alttab_disabled = qtrue;
}

static void WIN_EnableAltTab( void ) {
	if ( s_alttab_disabled ) {
		if ( !Q_stricmp( Cvar_VariableString( "arch" ), "winnt" ) ) {
			UnregisterHotKey( 0, 0 );
		} else
		{
			BOOL old;

			SystemParametersInfo( SPI_SCREENSAVERRUNNING, 0, &old, 0 );
		}

		s_alttab_disabled = qfalse;
	}
}

/*
==================
VID_AppActivate
==================
*/
static void VID_AppActivate( BOOL fActive, BOOL minimize ) {
	g_wv.isMinimized = minimize;

	Com_DPrintf( "VID_AppActivate: %i\n", fActive );

	Key_ClearStates();  // FIXME!!!

	// we don't want to act like we're active if we're minimized
	if ( fActive && !g_wv.isMinimized ) {
		g_wv.activeApp = qtrue;
	} else
	{
		g_wv.activeApp = qfalse;
	}

	// minimize/restore mouse-capture on demand
	if ( !g_wv.activeApp ) {
		IN_Activate( qfalse );
	} else
	{
		IN_Activate( qtrue );
	}
}

//==========================================================================

static byte s_scantokey[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '-',    '=',    K_BACKSPACE, 9, // 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13,    K_CTRL,'a',  's',      // 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    ';',
	'\'',    '`',    K_SHIFT,'\\',  'z',    'x',    'c',    'v',      // 2
	'b',    'n',    'm',    ',',    '.',    '/',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,    // 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             0,              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,                    // 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,                      // 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0                       // 7
};

static byte s_scantokey_german[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '?',    '\'',    K_BACKSPACE, 9, // 0
	'q',    'w',    'e',    'r',    't',    'z',    'u',    'i',
	'o',    'p',    '=',    '+',    13,    K_CTRL, 'a',  's',      // 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    '[',
	']',    '`',    K_SHIFT,'#',  'y',    'x',    'c',    'v',      // 2
	'b',    'n',    'm',    ',',    '.',    '-',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,    // 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             '<',              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,                    // 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,                      // 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0                       // 7
};

static byte s_scantokey_french[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    ')',    '=',    K_BACKSPACE, 9, // 0
	'a',    'z',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '^',    '$',    13,    K_CTRL, 'q',  's',      // 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    'm',
	'%',    '`',    K_SHIFT,'*',  'w',    'x',    'c',    'v',      // 2
	'b',    'n',    ',',    ';',    ':',    '!',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,    // 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             '<',              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,                    // 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,                      // 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0                       // 7
};

static byte s_scantokey_spanish[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '\'',    '!',    K_BACKSPACE, 9, // 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13,    K_CTRL, 'a',  's',      // 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    '=',
	'{',    '`',    K_SHIFT,'}',  'z',    'x',    'c',    'v',      // 2
	'b',    'n',    'm',    ',',    '.',    '-',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,    // 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             '<',              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,                    // 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,                      // 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0                       // 7
};

static byte s_scantokey_italian[128] =
{
//  0           1       2       3       4       5       6       7
//  8           9       A       B       C       D       E       F
	0,    27,     '1',    '2',    '3',    '4',    '5',    '6',
	'7',    '8',    '9',    '0',    '\'',    '^',    K_BACKSPACE, 9, // 0
	'q',    'w',    'e',    'r',    't',    'y',    'u',    'i',
	'o',    'p',    '[',    ']',    13,    K_CTRL, 'a',  's',      // 1
	'd',    'f',    'g',    'h',    'j',    'k',    'l',    '@',
	'#',    '`',    K_SHIFT,'=',  'z',    'x',    'c',    'v',      // 2
	'b',    'n',    'm',    ',',    '.',    '-',    K_SHIFT,'*',
	K_ALT,' ',   K_CAPSLOCK,    K_F1, K_F2, K_F3, K_F4, K_F5,    // 3
	K_F6, K_F7, K_F8, K_F9, K_F10,  K_PAUSE,    0, K_HOME,
	K_UPARROW,K_PGUP,K_KP_MINUS,K_LEFTARROW,K_KP_5,K_RIGHTARROW, K_KP_PLUS,K_END, //4
	K_DOWNARROW,K_PGDN,K_INS,K_DEL,0,0,             '<',              K_F11,
	K_F12,0,    0,    0,    0,    0,    0,    0,                    // 5
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0,                      // 6
	0,    0,    0,    0,    0,    0,    0,    0,
	0,    0,    0,    0,    0,    0,    0,    0                       // 7
};
/*
=======
MapKey

Map from windows to quake keynums
=======
*/
static int MapKey( int key ) {
	int result;
	int modified;
	qboolean is_extended;

//	Com_Printf( "0x%x\n", key);

	modified = ( key >> 16 ) & 255;

	if ( modified > 127 ) {
		return 0;
	}

	if ( key & ( 1 << 24 ) ) {
		is_extended = qtrue;
	} else
	{
		is_extended = qfalse;
	}

	result = s_scantokey[modified];

	if ( cl_language->integer - 1 == LANGUAGE_FRENCH ) {
		result = s_scantokey_french[modified];
	} else if ( cl_language->integer - 1 == LANGUAGE_GERMAN ) {
		result = s_scantokey_german[modified];
	} else if ( cl_language->integer - 1 == LANGUAGE_ITALIAN ) {
		result = s_scantokey_italian[modified];
	} else if ( cl_language->integer - 1 == LANGUAGE_SPANISH ) {
		result = s_scantokey_spanish[modified];
	}

	if ( !is_extended ) {
		switch ( result )
		{
		case K_HOME:
			return K_KP_HOME;
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_PGUP:
			return K_KP_PGUP;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_END:
			return K_KP_END;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_PGDN:
			return K_KP_PGDN;
		case K_INS:
			return K_KP_INS;
		case K_DEL:
			return K_KP_DEL;
		default:
			return result;
		}
	} else
	{
		switch ( result )
		{
		case K_PAUSE:
			return K_KP_NUMLOCK;
		case 0x0D:
			return K_KP_ENTER;
		case 0x2F:
			return K_KP_SLASH;
		case 0xAF:
			return K_KP_PLUS;
		}
		return result;
	}
}


/*
====================
MainWndProc

main window procedure
====================
*/
LONG WINAPI MainWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam ) {

	if ( uMsg == MSH_MOUSEWHEEL ) {
		if ( ( ( int ) wParam ) > 0 ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
		} else
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
		}
		return DefWindowProc( hWnd, uMsg, wParam, lParam );
	}

	switch ( uMsg )
	{
	case WM_MOUSEWHEEL:
		//
		//
		// this chunk of code theoretically only works under NT4 and Win98
		// since this message doesn't exist under Win95
		//
		if ( ( short ) HIWORD( wParam ) > 0 ) {
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qtrue, 0, NULL );
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELUP, qfalse, 0, NULL );
		} else
		{
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qtrue, 0, NULL );
			Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, K_MWHEELDOWN, qfalse, 0, NULL );
		}
		break;

	case WM_CREATE:

		g_wv.hWnd = hWnd;

		vid_xpos = Cvar_Get( "vid_xpos", "3", CVAR_ARCHIVE );
		vid_ypos = Cvar_Get( "vid_ypos", "22", CVAR_ARCHIVE );
		r_fullscreen = Cvar_Get( "r_fullscreen", "1", CVAR_ARCHIVE | CVAR_LATCH );

		MSH_MOUSEWHEEL = RegisterWindowMessage( "MSWHEEL_ROLLMSG" );
		if ( r_fullscreen->integer ) {
			WIN_DisableAltTab();
		} else
		{
			WIN_EnableAltTab();
		}

		break;
#if 0
	case WM_DISPLAYCHANGE:
		Com_DPrintf( "WM_DISPLAYCHANGE\n" );
		// we need to force a vid_restart if the user has changed
		// their desktop resolution while the game is running,
		// but don't do anything if the message is a result of
		// our own calling of ChangeDisplaySettings
		if ( com_insideVidInit ) {
			break;      // we did this on purpose
		}
		// something else forced a mode change, so restart all our gl stuff
		Cbuf_AddText( "vid_restart\n" );
		break;
#endif
	case WM_DESTROY:
		// let sound and input know about this?
		g_wv.hWnd = NULL;
		if ( r_fullscreen->integer ) {
			WIN_EnableAltTab();
		}
		break;

	case WM_CLOSE:
		Cbuf_ExecuteText( EXEC_APPEND, "quit" );
		break;

	case WM_ACTIVATE:
	{
		int fActive, fMinimized;

		fActive = LOWORD( wParam );
		fMinimized = (BOOL) HIWORD( wParam );

		VID_AppActivate( fActive != WA_INACTIVE, fMinimized );
#ifndef DOOMSOUND   ///// (SA) DOOMSOUND
		SNDDMA_Activate();
#endif  ///// (SA) DOOMSOUND
	}
	break;

	case WM_MOVE:
	{
		int xPos, yPos;
		RECT r;
		int style;

		if ( !r_fullscreen->integer ) {
			xPos = (short) LOWORD( lParam );      // horizontal position
			yPos = (short) HIWORD( lParam );      // vertical position

			r.left   = 0;
			r.top    = 0;
			r.right  = 1;
			r.bottom = 1;

			style = GetWindowLong( hWnd, GWL_STYLE );
			AdjustWindowRect( &r, style, FALSE );

			Cvar_SetValue( "vid_xpos", xPos + r.left );
			Cvar_SetValue( "vid_ypos", yPos + r.top );
			vid_xpos->modified = qfalse;
			vid_ypos->modified = qfalse;
			if ( g_wv.activeApp ) {
				IN_Activate( qtrue );
			}
		}
	}
	break;

// this is complicated because Win32 seems to pack multiple mouse events into
// one update sometimes, so we always check all states and look for events
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MOUSEMOVE:
	{
		int temp;

		temp = 0;

		if ( wParam & MK_LBUTTON ) {
			temp |= 1;
		}

		if ( wParam & MK_RBUTTON ) {
			temp |= 2;
		}

		if ( wParam & MK_MBUTTON ) {
			temp |= 4;
		}

		IN_MouseEvent( temp );
	}
	break;

	case WM_SYSCOMMAND:
		if ( wParam == SC_SCREENSAVE ) {
			return 0;
		}
		break;

	case WM_SYSKEYDOWN:
		if ( wParam == 13 ) {
			if ( r_fullscreen ) {
				Cvar_SetValue( "r_fullscreen", !r_fullscreen->integer );
				Cbuf_AddText( "vid_restart\n" );
			}
			return 0;
		}
		// fall through
	case WM_KEYDOWN:
		Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, MapKey( lParam ), qtrue, 0, NULL );
		break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		Sys_QueEvent( g_wv.sysMsgTime, SE_KEY, MapKey( lParam ), qfalse, 0, NULL );
		break;

	case WM_CHAR:
		Sys_QueEvent( g_wv.sysMsgTime, SE_CHAR, wParam, 0, 0, NULL );
		break;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}
