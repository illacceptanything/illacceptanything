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
// Name:         botai.h
// Function:     bot AI
// Programmer:   Mr Elusive (MrElusive@idsoftware.com)
// Last update:  1999-08-18
// Tab Size:     3
//===========================================================================

//debug line colors
#define LINECOLOR_NONE          -1
#define LINECOLOR_RED           1
#define LINECOLOR_GREEN         2
#define LINECOLOR_BLUE          3
#define LINECOLOR_YELLOW        4
#define LINECOLOR_ORANGE        5

//Print types
#define PRT_MESSAGE             1
#define PRT_WARNING             2
#define PRT_ERROR               3
#define PRT_FATAL               4
#define PRT_EXIT                5

//console message types
#define CMS_NORMAL              0
#define CMS_CHAT                1

//some maxs
#define MAX_NETNAME             36
#define MAX_CLIENTSKINNAME      128
#define MAX_FILEPATH            144
#define MAX_CHARACTERNAME       144

#ifndef BSPTRACE

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

//remove the bsp_trace_s structure definition l8r on
//a trace is returned when a box is swept through the world
typedef struct bsp_trace_s
{
	qboolean allsolid;          // if true, plane is not valid
	qboolean startsolid;        // if true, the initial point was in a solid area
	float fraction;             // time completed, 1.0 = didn't hit anything
	vec3_t endpos;          // final position
	cplane_t plane;             // surface normal at impact
	float exp_dist;             // expanded plane distance
	int sidenum;                // number of the brush side hit
	bsp_surface_t surface;  // the hit point surface
	int contents;           // contents on other side of surface hit
	int ent;                    // number of entity hit
} bsp_trace_t;

#define BSPTRACE
#endif  // BSPTRACE

//
// imported functions used for the BotAI
//


// from the server
/*
void	trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void	trap_Cvar_Update( vmCvar_t *cvar );
void	trap_Cvar_Set( const char *var_name, const char *value );
int		trap_Cvar_VariableIntegerValue( const char *var_name );
void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void	trap_GetConfigstring( int num, char *buffer, int bufferSize );
void	trap_GetServerinfo( char *buffer, int bufferSize );
int		trap_PointContents( const vec3_t point, int passEntityNum );
qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 );
int		trap_BotAllocateClient( void );
void	trap_BotFreeClient( int clientNum );
*/
