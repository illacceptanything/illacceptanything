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
 * name:		be_ai_char.h
 *
 * desc:		bot characters
 *
 *
 *****************************************************************************/

//loads a bot character from a file
int BotLoadCharacter( char *charfile, int skill );
//frees a bot character
void BotFreeCharacter( int character );
//float characteristic
float Characteristic_Float( int character, int index );
//bounded float characteristic
float Characteristic_BFloat( int character, int index, float min, float max );
//integer characteristic
int Characteristic_Integer( int character, int index );
//bounded integer characteristic
int Characteristic_BInteger( int character, int index, int min, int max );
//string characteristic
void Characteristic_String( int character, int index, char *buf, int size );
//free cached bot characters
void BotShutdownCharacters( void );
