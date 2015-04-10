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
 * name:		be_ea.h
 *
 * desc:		elementary actions
 *
 *
 *****************************************************************************/

//ClientCommand elementary actions
void EA_Say( int client, char *str );
void EA_SayTeam( int client, char *str );
void EA_UseItem( int client, char *it );
void EA_DropItem( int client, char *it );
void EA_UseInv( int client, char *inv );
void EA_DropInv( int client, char *inv );
void EA_Command( int client, char *command );
//regular elementary actions
void EA_SelectWeapon( int client, int weapon );
void EA_Attack( int client );
void EA_Reload( int client );
void EA_Respawn( int client );
void EA_Talk( int client );
void EA_Gesture( int client );
void EA_Use( int client );
void EA_Jump( int client );
void EA_DelayedJump( int client );
void EA_Crouch( int client );
void EA_Walk( int client );
void EA_MoveUp( int client );
void EA_MoveDown( int client );
void EA_MoveForward( int client );
void EA_MoveBack( int client );
void EA_MoveLeft( int client );
void EA_MoveRight( int client );
void EA_Move( int client, vec3_t dir, float speed );
void EA_View( int client, vec3_t viewangles );
//send regular input to the server
void EA_EndRegular( int client, float thinktime );
void EA_GetInput( int client, float thinktime, bot_input_t *input );
void EA_ResetInput( int client, bot_input_t *init );
//setup and shutdown routines
int EA_Setup( void );
void EA_Shutdown( void );
