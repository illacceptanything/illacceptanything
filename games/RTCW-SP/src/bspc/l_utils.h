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
// Name:         l_utils.h
// Function:     several utils
// Programmer:   Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:  1997-12-31
// Tab Size:     3
//===========================================================================

#ifndef MAX_PATH
	#define MAX_PATH            64
#endif

#ifndef PATH_SEPERATORSTR
	#if defined( WIN32 ) | defined( _WIN32 ) | defined( __NT__ ) | defined( __WINDOWS__ ) | defined( __WINDOWS_386__ )
		#define PATHSEPERATOR_STR       "\\"
	#else
		#define PATHSEPERATOR_STR       "/"
	#endif
#endif
#ifndef PATH_SEPERATORCHAR
	#if defined( WIN32 ) | defined( _WIN32 ) | defined( __NT__ ) | defined( __WINDOWS__ ) | defined( __WINDOWS_386__ )
		#define PATHSEPERATOR_CHAR      '\\'
	#else
		#define PATHSEPERATOR_CHAR      '/'
	#endif
#endif

//random in the range [0, 1]
#define random()            ( ( rand() & 0x7fff ) / ( (float)0x7fff ) )
//random in the range [-1, 1]
#define crandom()           ( 2.0 * ( random() - 0.5 ) )
//min and max
#define Maximum( x,y )        ( x > y ? x : y )
#define Minimum( x,y )        ( x < y ? x : y )
//absolute value
#define FloatAbs( x )     ( *(float *) &( ( *(int *) &( x ) ) & 0x7FFFFFFF ) )
#define IntAbs( x )           ( ~( x ) )
//coordinates
#define _X      0
#define _Y      1
#define _Z      2

typedef struct foundfile_s
{
	int offset;
	int length;
	char filename[MAX_PATH];        //screw LCC, array must be at end of struct
} foundfile_t;

void Vector2Angles( vec3_t value1, vec3_t angles );
//set the correct path seperators
void ConvertPath( char *path );
//append a path seperator to the given path not exceeding the length
void AppendPathSeperator( char *path, int length );
//find a file in a pak file
qboolean FindFileInPak( char *pakfile, char *filename, foundfile_t *file );
//find a quake file
#ifdef BOTLIB
qboolean FindQuakeFile( char *filename, foundfile_t *file );
#else //BOTLIB
qboolean FindQuakeFile( char *basedir, char *gamedir, char *filename, foundfile_t *file );
#endif //BOTLIB



