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
 * name:		l_struct.h
 *
 * desc:		structure reading/writing
 *
 *
 *****************************************************************************/


#define MAX_STRINGFIELD             80
//field types
#define FT_CHAR                     1           // char
#define FT_INT                          2           // int
#define FT_FLOAT                        3           // float
#define FT_STRING                       4           // char [MAX_STRINGFIELD]
#define FT_STRUCT                       6           // struct (sub structure)
//type only mask
#define FT_TYPE                     0x00FF  // only type, clear subtype
//sub types
#define FT_ARRAY                        0x0100  // array of type
#define FT_BOUNDED                  0x0200  // bounded value
#define FT_UNSIGNED                 0x0400

//structure field definition
typedef struct fielddef_s
{
	char *name;                                     //name of the field
	int offset;                                     //offset in the structure
	int type;                                       //type of the field
	//type specific fields
	int maxarray;                                   //maximum array size
	float floatmin, floatmax;                   //float min and max
	struct structdef_s *substruct;          //sub structure
} fielddef_t;

//structure definition
typedef struct structdef_s
{
	int size;
	fielddef_t *fields;
} structdef_t;

//read a structure from a script
int ReadStructure( source_t *source, structdef_t *def, char *structure );
//write a structure to a file
int WriteStructure( FILE *fp, structdef_t *def, char *structure );
//writes indents
int WriteIndent( FILE *fp, int indent );
//writes a float without traling zeros
int WriteFloat( FILE *fp, float value );


