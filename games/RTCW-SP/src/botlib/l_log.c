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
 * name:		l_log.c
 *
 * desc:		log file
 *
 *
 *****************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../game/q_shared.h"
#include "../game/botlib.h"
#include "be_interface.h"            //for botimport.Print
#include "l_libvar.h"

#define MAX_LOGFILENAMESIZE     1024

typedef struct logfile_s
{
	char filename[MAX_LOGFILENAMESIZE];
	FILE *fp;
	int numwrites;
} logfile_t;

static logfile_t logfile;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Log_AlwaysOpen( char *filename ) {
	if ( !filename || !strlen( filename ) ) {
		botimport.Print( PRT_MESSAGE, "openlog <filename>\n" );
		return;
	} //end if
	if ( logfile.fp ) {
		botimport.Print( PRT_ERROR, "log file %s is already opened\n", logfile.filename );
		return;
	} //end if
	logfile.fp = fopen( filename, "wb" );
	if ( !logfile.fp ) {
		botimport.Print( PRT_ERROR, "can't open the log file %s\n", filename );
		return;
	} //end if
	strncpy( logfile.filename, filename, MAX_LOGFILENAMESIZE );
	botimport.Print( PRT_MESSAGE, "Opened log %s\n", logfile.filename );
} //end of the function Log_Create
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Log_Open( char *filename ) {
	if ( !LibVarValue( "log", "0" ) ) {
		return;
	}
	Log_AlwaysOpen( filename );

}
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Log_Close( void ) {
	if ( !logfile.fp ) {
		return;
	}
	if ( fclose( logfile.fp ) ) {
		botimport.Print( PRT_ERROR, "can't close log file %s\n", logfile.filename );
		return;
	} //end if
	logfile.fp = NULL;
	botimport.Print( PRT_MESSAGE, "Closed log %s\n", logfile.filename );
} //end of the function Log_Close
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Log_Shutdown( void ) {
	if ( logfile.fp ) {
		Log_Close();
	}
} //end of the function Log_Shutdown
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void QDECL Log_Write( char *fmt, ... ) {
	va_list ap;

	if ( !logfile.fp ) {
		return;
	}
	va_start( ap, fmt );
	vfprintf( logfile.fp, fmt, ap );
	va_end( ap );
	//fprintf(logfile.fp, "\r\n");
	fflush( logfile.fp );
} //end of the function Log_Write
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void QDECL Log_WriteTimeStamped( char *fmt, ... ) {
	va_list ap;

	if ( !logfile.fp ) {
		return;
	}
	fprintf( logfile.fp, "%d   %02d:%02d:%02d:%02d   ",
			 logfile.numwrites,
			 (int) ( botlibglobals.time / 60 / 60 ),
			 (int) ( botlibglobals.time / 60 ),
			 (int) ( botlibglobals.time ),
			 (int) ( (int) ( botlibglobals.time * 100 ) ) -
			 ( (int) botlibglobals.time ) * 100 );
	va_start( ap, fmt );
	vfprintf( logfile.fp, fmt, ap );
	va_end( ap );
	fprintf( logfile.fp, "\r\n" );
	logfile.numwrites++;
	fflush( logfile.fp );
} //end of the function Log_Write
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
FILE *Log_FilePointer( void ) {
	return logfile.fp;
} //end of the function Log_FilePointer
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void Log_Flush( void ) {
	if ( logfile.fp ) {
		fflush( logfile.fp );
	}
} //end of the function Log_Flush

