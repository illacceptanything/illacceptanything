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


#ifndef MAX_MAP_ENTITIES
#define MAX_MAP_ENTITIES    2048
#endif

typedef struct epair_s
{
	struct epair_s  *next;
	char    *key;
	char    *value;
} epair_t;

typedef struct
{
	vec3_t origin;
	int firstbrush;
	int numbrushes;
	epair_t     *epairs;
	// only valid for func_areaportals
	int areaportalnum;
	int portalareas[2];
	int modelnum;           //for bsp 2 map conversion
	qboolean wasdetail;     //for SIN
} entity_t;

extern int num_entities;
extern entity_t entities[MAX_MAP_ENTITIES];

void StripTrailing( char *e );
void SetKeyValue( entity_t *ent, char *key, char *value );
char *ValueForKey( entity_t *ent, char *key ); // will return "" if not present
vec_t FloatForKey( entity_t *ent, char *key );
void GetVectorForKey( entity_t *ent, char *key, vec3_t vec );
qboolean ParseEntity( script_t *script );
epair_t *ParseEpair( script_t *script );
void PrintEntity( entity_t *ent );

