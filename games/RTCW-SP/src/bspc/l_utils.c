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
// Name:         l_utils.c
// Function:     several utils
// Programmer:   Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:  1997-12-31
// Tab Size:     3
//===========================================================================

#include <unistd.h>
//#ifndef BOTLIB
//#define BOTLIB
//#endif //BOTLIB

#ifdef BOTLIB
#include "q_shared.h"
#include "qfiles.h"
#include "botlib.h"
#include "l_log.h"
#include "l_libvar.h"
#include "l_memory.h"
//#include "l_utils.h"
#include "be_interface.h"
#else //BOTLIB
#include "qbsp.h"
#include "l_mem.h"
#endif //BOTLIB

#ifdef BOTLIB
//========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//========================================================================
void Vector2Angles( vec3_t value1, vec3_t angles ) {
	float forward;
	float yaw, pitch;

	if ( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;
		if ( value1[2] > 0 ) {
			pitch = 90;
		} else { pitch = 270;}
	} //end if
	else
	{
		yaw = (int) ( atan2( value1[1], value1[0] ) * 180 / M_PI );
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt( value1[0] * value1[0] + value1[1] * value1[1] );
		pitch = (int) ( atan2( value1[2], forward ) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	} //end else

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
} //end of the function Vector2Angles
#endif //BOTLIB
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void ConvertPath( char *path ) {
	while ( *path )
	{
		if ( *path == '/' || *path == '\\' ) {
			*path = PATHSEPERATOR_CHAR;
		}
		path++;
	} //end while
} //end of the function ConvertPath
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AppendPathSeperator( char *path, int length ) {
	int pathlen = strlen( path );

	if ( strlen( path ) && length - pathlen > 1 && path[pathlen - 1] != '/' && path[pathlen - 1] != '\\' ) {
		path[pathlen] = PATHSEPERATOR_CHAR;
		path[pathlen + 1] = '\0';
	} //end if
} //end of the function AppenPathSeperator
//===========================================================================
// returns pointer to file handle
// sets offset to and length of 'filename' in the pak file
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean FindFileInPak( char *pakfile, char *filename, foundfile_t *file ) {
	FILE *fp;
	dpackheader_t packheader;
	dpackfile_t *packfiles;
	int numdirs, i;
	char path[MAX_PATH];

	//open the pak file
	fp = fopen( pakfile, "rb" );
	if ( !fp ) {
		return false;
	} //end if
	  //read pak header, check for valid pak id and seek to the dir entries
	if ( ( fread( &packheader, 1, sizeof( dpackheader_t ), fp ) != sizeof( dpackheader_t ) )
		 || ( packheader.ident != IDPAKHEADER )
		 ||  ( fseek( fp, LittleLong( packheader.dirofs ), SEEK_SET ) )
		 ) {
		fclose( fp );
		return false;
	} //end if
	  //number of dir entries in the pak file
	numdirs = LittleLong( packheader.dirlen ) / sizeof( dpackfile_t );
	packfiles = (dpackfile_t *) GetMemory( numdirs * sizeof( dpackfile_t ) );
	//read the dir entry
	if ( fread( packfiles, sizeof( dpackfile_t ), numdirs, fp ) != numdirs ) {
		fclose( fp );
		FreeMemory( packfiles );
		return false;
	} //end if
	fclose( fp );
	//
	strcpy( path, filename );
	ConvertPath( path );
	//find the dir entry in the pak file
	for ( i = 0; i < numdirs; i++ )
	{
		//convert the dir entry name
		ConvertPath( packfiles[i].name );
		//compare the dir entry name with the filename
		if ( Q_strcasecmp( packfiles[i].name, path ) == 0 ) {
			strcpy( file->filename, pakfile );
			file->offset = LittleLong( packfiles[i].filepos );
			file->length = LittleLong( packfiles[i].filelen );
			FreeMemory( packfiles );
			return true;
		} //end if
	} //end for
	FreeMemory( packfiles );
	return false;
} //end of the function FindFileInPak
//===========================================================================
// find a Quake2 file
// returns full path in 'filename'
// sets offset and length of the file
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
qboolean FindQuakeFile2( char *basedir, char *gamedir, char *filename, foundfile_t *file ) {
	int dir, i;
	//NOTE: 3 is necessary (LCC bug???)
	char gamedirs[3][MAX_PATH] = {"","",""};
	char filedir[MAX_PATH] = "";

	//
	if ( gamedir ) {
		strncpy( gamedirs[0], gamedir, MAX_PATH );
	}
	strncpy( gamedirs[1], "baseq2", MAX_PATH );
	//
	//find the file in the two game directories
	for ( dir = 0; dir < 2; dir++ )
	{
		//check if the file is in a directory
		filedir[0] = 0;
		if ( basedir && strlen( basedir ) ) {
			strncpy( filedir, basedir, MAX_PATH );
			AppendPathSeperator( filedir, MAX_PATH );
		} //end if
		if ( strlen( gamedirs[dir] ) ) {
			strncat( filedir, gamedirs[dir], MAX_PATH - strlen( filedir ) );
			AppendPathSeperator( filedir, MAX_PATH );
		} //end if
		strncat( filedir, filename, MAX_PATH - strlen( filedir ) );
		ConvertPath( filedir );
		Log_Write( "accessing %s", filedir );
		if ( !access( filedir, 0x04 ) ) {
			strcpy( file->filename, filedir );
			file->length = 0;
			file->offset = 0;
			return true;
		} //end if
		  //check if the file is in a pak?.pak
		for ( i = 0; i < 10; i++ )
		{
			filedir[0] = 0;
			if ( basedir && strlen( basedir ) ) {
				strncpy( filedir, basedir, MAX_PATH );
				AppendPathSeperator( filedir, MAX_PATH );
			} //end if
			if ( strlen( gamedirs[dir] ) ) {
				strncat( filedir, gamedirs[dir], MAX_PATH - strlen( filedir ) );
				AppendPathSeperator( filedir, MAX_PATH );
			} //end if
			// TTimo: huuuuh .. I suppose this means there needs to be two \0\0 at the end?
			//sprintf(&filedir[strlen(filedir)], "pak%d.pak\0", i);
			// doing it more 'clean'
			sprintf( &filedir[strlen( filedir )], "pak%d.pak ", i );
			filedir[strlen( filedir ) - 1] = '\0';
			if ( !access( filedir, 0x04 ) ) {
				Log_Write( "searching %s in %s", filename, filedir );
				if ( FindFileInPak( filedir, filename, file ) ) {
					return true;
				}
			} //end if
		} //end for
	} //end for
	file->offset = 0;
	file->length = 0;
	return false;
} //end of the function FindQuakeFile2
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
#ifdef BOTLIB
qboolean FindQuakeFile( char *filename, foundfile_t *file ) {
	return FindQuakeFile2( LibVarGetString( "basedir" ),
						   LibVarGetString( "gamedir" ), filename, file );
} //end of the function FindQuakeFile
#else //BOTLIB
qboolean FindQuakeFile( char *basedir, char *gamedir, char *filename, foundfile_t *file ) {
	return FindQuakeFile2( basedir, gamedir, filename, file );
} //end of the function FindQuakeFile
#endif //BOTLIB
