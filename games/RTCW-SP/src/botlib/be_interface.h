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


/*****************************************************************************
 * name:		be_interface.h
 *
 * desc:		botlib interface
 *
 *
 *****************************************************************************/

/*
"Do not go where the path leads, rather go where there's no track and leave a trail."

"AAS (Area Awareness System)"

"Part of the Gladiator is BoGuS (Bot Guidance System)"

"ANSI (Advanced Navigational System Interface)"

"to make things work the only thing you really have to do is think things work."

"a madman is just someone living in another reality which isn't shared among many people"
*/

//#define DEBUG			//debug code
#define RANDOMIZE       //randomize bot behaviour
#if defined( WIN32 ) || defined( _WIN32 )
#define AASZIP          //allow reading directly from aasX.zip files
#endif
#define QUAKE2          //bot for Quake2
//#define HALFLIFE		//bot for Half-Life

//==========================================================
//
// global variable structures
//
//==========================================================

//FIXME: get rid of this global structure
typedef struct botlib_globals_s
{
	int botlibsetup;                        //true when the bot library has been setup
	int maxentities;                        //maximum number of entities
	int maxclients;                         //maximum number of clients
	float time;                             //the global time
#ifdef DEBUG
	qboolean debug;                         //true if debug is on
	int goalareanum;
	vec3_t goalorigin;
	int runai;
#endif
} botlib_globals_t;

//==========================================================
//
// global variables
//
//==========================================================

extern botlib_globals_t botlibglobals;
extern botlib_import_t botimport;
extern int bot_developer;                   //true if developer is on

//
int Sys_MilliSeconds( void );

