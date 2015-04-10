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
 * name:		be_aas_main.h
 *
 * desc:		AAS
 *
 *
 *****************************************************************************/

#ifdef AASINTERN

extern aas_t( *aasworld );

//AAS error message
void QDECL AAS_Error( char *fmt, ... );
//set AAS initialized
void AAS_SetInitialized( void );
//setup AAS with the given number of entities and clients
int AAS_Setup( void );
//shutdown AAS
void AAS_Shutdown( void );
//start a new map
int AAS_LoadMap( const char *mapname );
//start a new time frame
int AAS_StartFrame( float time );
#endif //AASINTERN

//returns true if AAS is initialized
int AAS_Initialized( void );
//returns true if the AAS file is loaded
int AAS_Loaded( void );
//returns the model name from the given index
char *AAS_ModelFromIndex( int index );
//returns the index from the given model name
int AAS_IndexFromModel( char *modelname );
//returns the current time
float AAS_Time( void );

// Ridah
void AAS_SetCurrentWorld( int index );
// done.
