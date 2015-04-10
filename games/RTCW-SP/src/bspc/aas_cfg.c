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
// Name:			cfg.c
// Function:
// Programmer:		Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:		1997-12-04
// Tab Size:		3
//===========================================================================

#include "qbsp.h"
#include "float.h"
#include "../botlib/aasfile.h"
#include "aas_store.h"
#include "aas_cfg.h"
#include "../botlib/l_precomp.h"
#include "../botlib/l_struct.h"
#include "../botlib/l_libvar.h"

// TTimo: this is a f*g mess
// I got rid of all occurences except for bspc it seems
// all code *should* be using Q_stricmp
#ifdef BSPC
#define stricmp strcasecmp
#endif

///////////////////////////////////
extern void LibVarSet( char *var_name, char *value );
///////////////////////////////////

//structure field offsets
#define BBOX_OFS( x ) (int)&( ( (aas_bbox_t *)0 )->x )
#define CFG_OFS( x ) (int)&( ( (cfg_t *)0 )->x )

//bounding box definition
fielddef_t bbox_fields[] =
{
	{"presencetype", BBOX_OFS( presencetype ), FT_INT},
	{"flags", BBOX_OFS( flags ), FT_INT},
	{"mins", BBOX_OFS( mins ), FT_FLOAT | FT_ARRAY, 3},
	{"maxs", BBOX_OFS( maxs ), FT_FLOAT | FT_ARRAY, 3},
	{NULL, 0, 0, 0}
};

fielddef_t cfg_fields[] =
{
	{"phys_gravitydirection", CFG_OFS( phys_gravitydirection ), FT_FLOAT | FT_ARRAY, 3},
	{"phys_friction", CFG_OFS( phys_friction ), FT_FLOAT},
	{"phys_stopspeed", CFG_OFS( phys_stopspeed ), FT_FLOAT},
	{"phys_gravity", CFG_OFS( phys_gravity ), FT_FLOAT},
	{"phys_waterfriction", CFG_OFS( phys_waterfriction ), FT_FLOAT},
	{"phys_watergravity", CFG_OFS( phys_watergravity ), FT_FLOAT},
	{"phys_maxvelocity", CFG_OFS( phys_maxvelocity ), FT_FLOAT},
	{"phys_maxwalkvelocity", CFG_OFS( phys_maxwalkvelocity ), FT_FLOAT},
	{"phys_maxcrouchvelocity", CFG_OFS( phys_maxcrouchvelocity ), FT_FLOAT},
	{"phys_maxswimvelocity", CFG_OFS( phys_maxswimvelocity ), FT_FLOAT},
	{"phys_walkaccelerate", CFG_OFS( phys_walkaccelerate ), FT_FLOAT},
	{"phys_airaccelerate", CFG_OFS( phys_airaccelerate ), FT_FLOAT},
	{"phys_swimaccelerate", CFG_OFS( phys_swimaccelerate ), FT_FLOAT},
	{"phys_maxstep", CFG_OFS( phys_maxstep ), FT_FLOAT},
	{"phys_maxsteepness", CFG_OFS( phys_maxsteepness ), FT_FLOAT},
	{"phys_maxwaterjump", CFG_OFS( phys_maxwaterjump ), FT_FLOAT},
	{"phys_maxbarrier", CFG_OFS( phys_maxbarrier ), FT_FLOAT},
	{"phys_jumpvel", CFG_OFS( phys_jumpvel ), FT_FLOAT},
	{"phys_falldelta5", CFG_OFS( phys_falldelta5 ), FT_FLOAT},
	{"phys_falldelta10", CFG_OFS( phys_falldelta10 ), FT_FLOAT},
	{"rs_waterjump", CFG_OFS( rs_waterjump ), FT_FLOAT},
	{"rs_teleport", CFG_OFS( rs_teleport ), FT_FLOAT},
	{"rs_barrierjump", CFG_OFS( rs_barrierjump ), FT_FLOAT},
	{"rs_startcrouch", CFG_OFS( rs_startcrouch ), FT_FLOAT},
	{"rs_startgrapple", CFG_OFS( rs_startgrapple ), FT_FLOAT},
	{"rs_startwalkoffledge", CFG_OFS( rs_startwalkoffledge ), FT_FLOAT},
	{"rs_startjump", CFG_OFS( rs_startjump ), FT_FLOAT},
	{"rs_rocketjump", CFG_OFS( rs_rocketjump ), FT_FLOAT},
	{"rs_bfgjump", CFG_OFS( rs_bfgjump ), FT_FLOAT},
	{"rs_jumppad", CFG_OFS( rs_jumppad ), FT_FLOAT},
	{"rs_aircontrolledjumppad", CFG_OFS( rs_aircontrolledjumppad ), FT_FLOAT},
	{"rs_funcbob", CFG_OFS( rs_funcbob ), FT_FLOAT},
	{"rs_startelevator", CFG_OFS( rs_startelevator ), FT_FLOAT},
	{"rs_falldamage5", CFG_OFS( rs_falldamage5 ), FT_FLOAT},
	{"rs_falldamage10", CFG_OFS( rs_falldamage10 ), FT_FLOAT},
	{"rs_maxjumpfallheight", CFG_OFS( rs_maxjumpfallheight ), FT_FLOAT},
	{"rs_allowladders", CFG_OFS( rs_allowladders ), FT_INT},
	{NULL, 0, 0, 0}
};

structdef_t bbox_struct =
{
	sizeof( aas_bbox_t ), bbox_fields
};
structdef_t cfg_struct =
{
	sizeof( cfg_t ), cfg_fields
};

//global cfg
cfg_t cfg;

#if 0
//===========================================================================
// the default Q3A configuration
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void DefaultCfg( void ) {
	int i;

	// default all float values to infinite
	for ( i = 0; cfg_fields[i].name; i++ )
	{
		if ( ( cfg_fields[i].type & FT_TYPE ) == FT_FLOAT ) {
			*( float * )( ( (char*)&cfg ) + cfg_fields[i].offset ) = FLT_MAX;
		}
	} //end for
	  //
	cfg.numbboxes = 2;
	//bbox 0
	cfg.bboxes[0].presencetype = PRESENCE_NORMAL;
	cfg.bboxes[0].flags = 0;
	cfg.bboxes[0].mins[0] = -18;
	cfg.bboxes[0].mins[1] = -18;
	cfg.bboxes[0].mins[2] = -24;
	cfg.bboxes[0].maxs[0] = 18;
	cfg.bboxes[0].maxs[1] = 18;
	cfg.bboxes[0].maxs[2] = 48;
	//bbox 1
	cfg.bboxes[1].presencetype = PRESENCE_CROUCH;
	cfg.bboxes[1].flags = 1;
	cfg.bboxes[1].mins[0] = -18;
	cfg.bboxes[1].mins[1] = -18;
	cfg.bboxes[1].mins[2] = -24;
	cfg.bboxes[1].maxs[0] = 18;
	cfg.bboxes[1].maxs[1] = 18;
	cfg.bboxes[1].maxs[2] = 24;
	//
	cfg.allpresencetypes = PRESENCE_NORMAL | PRESENCE_CROUCH;
	cfg.phys_gravitydirection[0]    = 0;
	cfg.phys_gravitydirection[1]    = 0;
	cfg.phys_gravitydirection[2]    = -1;
	cfg.phys_maxsteepness           = 0.7;

//	cfg.phys_maxbarrier			= -999;//32;	// RIDAH: this is calculated at run-time now, from the gravity and jump velocity settings
} //end of the function DefaultCfg
#else
//===========================================================================
// the default Q3A configuration
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void DefaultCfg( void ) {
	int i;

	// default all float values to infinite
	for ( i = 0; cfg_fields[i].name; i++ )
	{
		if ( ( cfg_fields[i].type & FT_TYPE ) == FT_FLOAT ) {
			*( float * )( ( (char*)&cfg ) + cfg_fields[i].offset ) = FLT_MAX;
		}
	} //end for
	  //
	cfg.numbboxes = 2;
	//bbox 0
	cfg.bboxes[0].presencetype = PRESENCE_NORMAL;
	cfg.bboxes[0].flags = 0;
	cfg.bboxes[0].mins[0] = -15;
	cfg.bboxes[0].mins[1] = -15;
	cfg.bboxes[0].mins[2] = -24;
	cfg.bboxes[0].maxs[0] = 15;
	cfg.bboxes[0].maxs[1] = 15;
	cfg.bboxes[0].maxs[2] = 32;
	//bbox 1
	cfg.bboxes[1].presencetype = PRESENCE_CROUCH;
	cfg.bboxes[1].flags = 1;
	cfg.bboxes[1].mins[0] = -15;
	cfg.bboxes[1].mins[1] = -15;
	cfg.bboxes[1].mins[2] = -24;
	cfg.bboxes[1].maxs[0] = 15;
	cfg.bboxes[1].maxs[1] = 15;
	cfg.bboxes[1].maxs[2] = 16;
	//
	cfg.allpresencetypes = PRESENCE_NORMAL | PRESENCE_CROUCH;
	cfg.phys_gravitydirection[0]    = 0;
	cfg.phys_gravitydirection[1]    = 0;
	cfg.phys_gravitydirection[2]    = -1;
	cfg.phys_maxsteepness           = 0.7;

//	cfg.phys_maxbarrier			= -999;//32;	// RIDAH: this is calculated at run-time now, from the gravity and jump velocity settings
} //end of the function DefaultCfg
#endif

//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
char    * QDECL va( char *format, ... ) {
	va_list argptr;
	static char string[2][32000];       // in case va is called by nested functions
	static int index = 0;
	char    *buf;

	buf = string[index & 1];
	index++;

	va_start( argptr, format );
	vsprintf( buf, format,argptr );
	va_end( argptr );

	return buf;
} //end of the function va
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
void SetCfgLibVars( void ) {
	int i;
	float value;

	for ( i = 0; cfg_fields[i].name; i++ )
	{
		if ( ( cfg_fields[i].type & FT_TYPE ) == FT_FLOAT ) {
			value = *( float * )( ( (char*)&cfg ) + cfg_fields[i].offset );
			if ( value != FLT_MAX ) {
				LibVarSet( cfg_fields[i].name, va( "%f", value ) );
			} //end if
		} //end if
	} //end for
} //end of the function SetCfgLibVars
//===========================================================================
//
// Parameter:			-
// Returns:				-
// Changes Globals:		-
//===========================================================================
int LoadCfgFile( char *filename ) {
	source_t *source;
	token_t token;
	int settingsdefined;

	source = LoadSourceFile( filename );
	if ( !source ) {
		Log_Print( "couldn't open cfg file %s\n", filename );
		return false;
	} //end if

	settingsdefined = false;
	memset( &cfg, 0, sizeof( cfg_t ) );

	while ( PC_ReadToken( source, &token ) )
	{
		if ( !stricmp( token.string, "bbox" ) ) {
			if ( cfg.numbboxes >= AAS_MAX_BBOXES ) {
				SourceError( source, "too many bounding box volumes defined" );
			} //end if
			if ( !ReadStructure( source, &bbox_struct, (char *) &cfg.bboxes[cfg.numbboxes] ) ) {
				FreeSource( source );
				return false;
			} //end if
			cfg.allpresencetypes |= cfg.bboxes[cfg.numbboxes].presencetype;
			cfg.numbboxes++;
		} //end if
		else if ( !stricmp( token.string, "settings" ) ) {
			if ( settingsdefined ) {
				SourceWarning( source, "settings already defined\n" );
			} //end if
			settingsdefined = true;
			if ( !ReadStructure( source, &cfg_struct, (char *) &cfg ) ) {
				FreeSource( source );
				return false;
			} //end if
		} //end else if
	} //end while
	if ( VectorLength( cfg.phys_gravitydirection ) < 0.9 || VectorLength( cfg.phys_gravitydirection ) > 1.1 ) {
		SourceError( source, "invalid gravity direction specified" );
	} //end if
	if ( cfg.numbboxes <= 0 ) {
		SourceError( source, "no bounding volumes specified" );
	} //end if
	FreeSource( source );
	SetCfgLibVars();
	Log_Print( "using cfg file %s\n", filename );
	return true;
} //end of the function LoadCfgFile
