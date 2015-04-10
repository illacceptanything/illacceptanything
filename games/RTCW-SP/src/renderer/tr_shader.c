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

#include "tr_local.h"

// tr_shader.c -- this file deals with the parsing and definition of shaders

static char *s_shaderText;

// the shader is parsed into these global variables, then copied into
// dynamically allocated memory if it is valid.
static shaderStage_t stages[MAX_SHADER_STAGES];
static shader_t shader;
static texModInfo_t texMods[MAX_SHADER_STAGES][TR_MAX_TEXMODS];
static qboolean deferLoad;

#define FILE_HASH_SIZE      4096

static shader_t*       hashTable[FILE_HASH_SIZE];

// Ridah
// Table containing string indexes for each shader found in the scripts, referenced by their checksum
// values.
typedef struct shaderStringPointer_s
{
	char *pStr;
	struct shaderStringPointer_s *next;
} shaderStringPointer_t;
//
shaderStringPointer_t shaderChecksumLookup[FILE_HASH_SIZE];
// done.

/*
================
return a hash value for the filename
================
*/
static long generateHashValue( const char *fname ) {
	int i;
	long hash;
	char letter;

	hash = 0;
	i = 0;
	while ( fname[i] != '\0' ) {
		letter = tolower( fname[i] );
		if ( letter == '.' ) {
			break;                          // don't include extension
		}
		if ( letter == '\\' ) {
			letter = '/';                   // damn path names
		}
		if ( letter == PATH_SEP ) {
			letter = '/';                           // damn path names
		}
		hash += (long)( letter ) * ( i + 119 );
		i++;
	}
	hash &= ( FILE_HASH_SIZE - 1 );
	return hash;
}

void R_RemapShader( const char *shaderName, const char *newShaderName, const char *timeOffset ) {
	char strippedName[MAX_QPATH];
	int hash;
	shader_t    *sh, *sh2;
	qhandle_t h;

	sh = R_FindShaderByName( shaderName );
	if ( sh == NULL || sh == tr.defaultShader ) {
		h = RE_RegisterShaderLightMap( shaderName, 0 );
		sh = R_GetShaderByHandle( h );
	}
	if ( sh == NULL || sh == tr.defaultShader ) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: shader %s not found\n", shaderName );
		return;
	}

	sh2 = R_FindShaderByName( newShaderName );
	if ( sh2 == NULL || sh2 == tr.defaultShader ) {
		h = RE_RegisterShaderLightMap( newShaderName, 0 );
		sh2 = R_GetShaderByHandle( h );
	}

	if ( sh2 == NULL || sh2 == tr.defaultShader ) {
		ri.Printf( PRINT_WARNING, "WARNING: R_RemapShader: new shader %s not found\n", newShaderName );
		return;
	}

	// remap all the shaders with the given name
	// even tho they might have different lightmaps
	COM_StripExtension( shaderName, strippedName );
	hash = generateHashValue( strippedName );
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
		if ( Q_stricmp( sh->name, strippedName ) == 0 ) {
			if ( sh != sh2 ) {
				sh->remappedShader = sh2;
			} else {
				sh->remappedShader = NULL;
			}
		}
	}
	if ( timeOffset ) {
		sh2->timeOffset = atof( timeOffset );
	}
}

/*
===============
ParseVector
===============
*/
static qboolean ParseVector( char **text, int count, float *v ) {
	char    *token;
	int i;

	// FIXME: spaces are currently required after parens, should change parseext...
	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, "(" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	for ( i = 0 ; i < count ; i++ ) {
		token = COM_ParseExt( text, qfalse );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing vector element in shader '%s'\n", shader.name );
			return qfalse;
		}
		v[i] = atof( token );
	}

	token = COM_ParseExt( text, qfalse );
	if ( strcmp( token, ")" ) ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing parenthesis in shader '%s'\n", shader.name );
		return qfalse;
	}

	return qtrue;
}


/*
===============
NameToAFunc
===============
*/
static unsigned NameToAFunc( const char *funcname ) {
	if ( !Q_stricmp( funcname, "GT0" ) ) {
		return GLS_ATEST_GT_0;
	} else if ( !Q_stricmp( funcname, "LT128" ) )    {
		return GLS_ATEST_LT_80;
	} else if ( !Q_stricmp( funcname, "GE128" ) )    {
		return GLS_ATEST_GE_80;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid alphaFunc name '%s' in shader '%s'\n", funcname, shader.name );
	return 0;
}


/*
===============
NameToSrcBlendMode
===============
*/
static int NameToSrcBlendMode( const char *name ) {
	if ( !Q_stricmp( name, "GL_ONE" ) ) {
		return GLS_SRCBLEND_ONE;
	} else if ( !Q_stricmp( name, "GL_ZERO" ) )    {
		return GLS_SRCBLEND_ZERO;
	} else if ( !Q_stricmp( name, "GL_DST_COLOR" ) )    {
		return GLS_SRCBLEND_DST_COLOR;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_COLOR" ) )    {
		return GLS_SRCBLEND_ONE_MINUS_DST_COLOR;
	} else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )    {
		return GLS_SRCBLEND_SRC_ALPHA;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )    {
		return GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )    {
		return GLS_SRCBLEND_DST_ALPHA;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )    {
		return GLS_SRCBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !Q_stricmp( name, "GL_SRC_ALPHA_SATURATE" ) )    {
		return GLS_SRCBLEND_ALPHA_SATURATE;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_SRCBLEND_ONE;
}

/*
===============
NameToDstBlendMode
===============
*/
static int NameToDstBlendMode( const char *name ) {
	if ( !Q_stricmp( name, "GL_ONE" ) ) {
		return GLS_DSTBLEND_ONE;
	} else if ( !Q_stricmp( name, "GL_ZERO" ) )    {
		return GLS_DSTBLEND_ZERO;
	} else if ( !Q_stricmp( name, "GL_SRC_ALPHA" ) )    {
		return GLS_DSTBLEND_SRC_ALPHA;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_ALPHA" ) )    {
		return GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( !Q_stricmp( name, "GL_DST_ALPHA" ) )    {
		return GLS_DSTBLEND_DST_ALPHA;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_DST_ALPHA" ) )    {
		return GLS_DSTBLEND_ONE_MINUS_DST_ALPHA;
	} else if ( !Q_stricmp( name, "GL_SRC_COLOR" ) )    {
		return GLS_DSTBLEND_SRC_COLOR;
	} else if ( !Q_stricmp( name, "GL_ONE_MINUS_SRC_COLOR" ) )    {
		return GLS_DSTBLEND_ONE_MINUS_SRC_COLOR;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown blend mode '%s' in shader '%s', substituting GL_ONE\n", name, shader.name );
	return GLS_DSTBLEND_ONE;
}

/*
===============
NameToGenFunc
===============
*/
static genFunc_t NameToGenFunc( const char *funcname ) {
	if ( !Q_stricmp( funcname, "sin" ) ) {
		return GF_SIN;
	} else if ( !Q_stricmp( funcname, "square" ) )    {
		return GF_SQUARE;
	} else if ( !Q_stricmp( funcname, "triangle" ) )    {
		return GF_TRIANGLE;
	} else if ( !Q_stricmp( funcname, "sawtooth" ) )    {
		return GF_SAWTOOTH;
	} else if ( !Q_stricmp( funcname, "inversesawtooth" ) )    {
		return GF_INVERSE_SAWTOOTH;
	} else if ( !Q_stricmp( funcname, "noise" ) )    {
		return GF_NOISE;
	}

	ri.Printf( PRINT_WARNING, "WARNING: invalid genfunc name '%s' in shader '%s'\n", funcname, shader.name );
	return GF_SIN;
}


/*
===================
ParseWaveForm
===================
*/
static void ParseWaveForm( char **text, waveForm_t *wave ) {
	char *token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->func = NameToGenFunc( token );

	// BASE, AMP, PHASE, FREQ
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->base = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->amplitude = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->phase = atof( token );

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing waveform parm in shader '%s'\n", shader.name );
		return;
	}
	wave->frequency = atof( token );
}


/*
===================
ParseTexMod
===================
*/
static void ParseTexMod( char *_text, shaderStage_t *stage ) {
	const char *token;
	char **text = &_text;
	texModInfo_t *tmi;

	if ( stage->bundle[0].numTexMods == TR_MAX_TEXMODS ) {
		ri.Error( ERR_DROP, "ERROR: too many tcMod stages in shader '%s'\n", shader.name );
		return;
	}

	tmi = &stage->bundle[0].texMods[stage->bundle[0].numTexMods];
	stage->bundle[0].numTexMods++;

	token = COM_ParseExt( text, qfalse );

	//
	// swap
	//
	if ( !Q_stricmp( token, "swap" ) ) { // swap S/T coords (rotate 90d)
		tmi->type = TMOD_SWAP;
	}
	//
	// turb
	//
	// (SA) added 'else' so it wouldn't claim 'swap' was unknown.
	else if ( !Q_stricmp( token, "turb" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod turb in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_TURBULENT;
	}
	//
	// scale
	//
	else if ( !Q_stricmp( token, "scale" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scale[1] = atof( token );
		tmi->type = TMOD_SCALE;
	}
	//
	// scroll
	//
	else if ( !Q_stricmp( token, "scroll" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[0] = atof( token );
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing scale scroll parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->scroll[1] = atof( token );
		tmi->type = TMOD_SCROLL;
	}
	//
	// stretch
	//
	else if ( !Q_stricmp( token, "stretch" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.func = NameToGenFunc( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.base = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.phase = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing stretch parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->wave.frequency = atof( token );

		tmi->type = TMOD_STRETCH;
	}
	//
	// transform
	//
	else if ( !Q_stricmp( token, "transform" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[0][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->matrix[1][1] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[0] = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing transform parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->translate[1] = atof( token );

		tmi->type = TMOD_TRANSFORM;
	}
	//
	// rotate
	//
	else if ( !Q_stricmp( token, "rotate" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing tcMod rotate parms in shader '%s'\n", shader.name );
			return;
		}
		tmi->rotateSpeed = atof( token );
		tmi->type = TMOD_ROTATE;
	}
	//
	// entityTranslate
	//
	else if ( !Q_stricmp( token, "entityTranslate" ) ) {
		tmi->type = TMOD_ENTITY_TRANSLATE;
	} else
	{
		ri.Printf( PRINT_WARNING, "WARNING: unknown tcMod '%s' in shader '%s'\n", token, shader.name );
	}
}


/*
===================
ParseStage
===================
*/
static qboolean ParseStage( shaderStage_t *stage, char **text ) {
	char *token;
	int depthMaskBits = GLS_DEPTHMASK_TRUE, blendSrcBits = 0, blendDstBits = 0, atestBits = 0, depthFuncBits = 0;
	qboolean depthMaskExplicit = qfalse;

	stage->active = qtrue;

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: no matching '}' found\n" );
			return qfalse;
		}

		if ( token[0] == '}' ) {
			break;
		}
		//
		// check special case for map16/map32/mapcomp/mapnocomp (compression enabled)
		if ( !Q_stricmp( token, "map16" ) ) {    // only use this texture if 16 bit color depth
			if ( glConfig.colorBits <= 16 ) {
				token = "map";   // use this map
			} else {
				COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "map32" ) )    { // only use this texture if 16 bit color depth
			if ( glConfig.colorBits > 16 ) {
				token = "map";   // use this map
			} else {
				COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "mapcomp" ) )    { // only use this texture if compression is enabled
			if ( glConfig.textureCompression && r_ext_compressed_textures->integer ) {
				token = "map";   // use this map
			} else {
				COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "mapnocomp" ) )    { // only use this texture if compression is not available or disabled
			if ( !glConfig.textureCompression ) {
				token = "map";   // use this map
			} else {
				COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "animmapcomp" ) )    { // only use this texture if compression is enabled
			if ( glConfig.textureCompression && r_ext_compressed_textures->integer ) {
				token = "animmap";   // use this map
			} else {
				while ( token[0] )
					COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		} else if ( !Q_stricmp( token, "animmapnocomp" ) )    { // only use this texture if compression is not available or disabled
			if ( !glConfig.textureCompression ) {
				token = "animmap";   // use this map
			} else {
				while ( token[0] )
					COM_ParseExt( text, qfalse );   // ignore the map
				continue;
			}
		}
		//
		// map <name>
		//
		if ( !Q_stricmp( token, "map" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'map' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

//----(SA)	fixes startup error and allows polygon shadows to work again
			if ( !Q_stricmp( token, "$whiteimage" ) || !Q_stricmp( token, "*white" ) ) {
//----(SA)	end
				stage->bundle[0].image[0] = tr.whiteImage;
				continue;
			}
//----(SA) added
			else if ( !Q_stricmp( token, "$dlight" ) ) {
				stage->bundle[0].image[0] = tr.dlightImage;
				continue;
			}
//----(SA) end
			else if ( !Q_stricmp( token, "$lightmap" ) ) {
				stage->bundle[0].isLightmap = qtrue;
				if ( shader.lightmapIndex < 0 ) {
					stage->bundle[0].image[0] = tr.whiteImage;
				} else {
					stage->bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
				}
				continue;
			} else
			{
//----(SA)	modified
//				stage->bundle[0].image[0] = R_FindImageFile( token, !shader.noMipMaps, !shader.noPicMip, GL_REPEAT );
				stage->bundle[0].image[0] = R_FindImageFileExt( token, !shader.noMipMaps, !shader.noPicMip, shader.characterMip, GL_REPEAT );
//----(SA)	end
				if ( !stage->bundle[0].image[0] ) {
					ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
					return qfalse;
				}
			}
		}
		//
		// clampmap <name>
		//
		else if ( !Q_stricmp( token, "clampmap" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'clampmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			stage->bundle[0].image[0] = R_FindImageFileExt( token, !shader.noMipMaps, !shader.noPicMip, shader.characterMip, GL_CLAMP );
			if ( !stage->bundle[0].image[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
				return qfalse;
			}
		}
		//
		// animMap <frequency> <image1> .... <imageN>
		//
		else if ( !Q_stricmp( token, "animMap" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'animMmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].imageAnimationSpeed = atof( token );

			// parse up to MAX_IMAGE_ANIMATIONS animations
			while ( 1 ) {
				int num;

				token = COM_ParseExt( text, qfalse );
				if ( !token[0] ) {
					break;
				}
				num = stage->bundle[0].numImageAnimations;
				if ( num < MAX_IMAGE_ANIMATIONS ) {
					stage->bundle[0].image[num] = R_FindImageFileExt( token, !shader.noMipMaps, !shader.noPicMip, shader.characterMip, GL_REPEAT );
					if ( !stage->bundle[0].image[num] ) {
						ri.Printf( PRINT_WARNING, "WARNING: R_FindImageFile could not find '%s' in shader '%s'\n", token, shader.name );
						return qfalse;
					}
					stage->bundle[0].numImageAnimations++;
				}
			}
		} else if ( !Q_stricmp( token, "videoMap" ) )    {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'videoMmap' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}
			stage->bundle[0].videoMapHandle = ri.CIN_PlayCinematic( token, 0, 0, 256, 256, ( CIN_loop | CIN_silent | CIN_shader ) );
			if ( stage->bundle[0].videoMapHandle != -1 ) {
				stage->bundle[0].isVideoMap = qtrue;
				stage->bundle[0].image[0] = tr.scratchImage[stage->bundle[0].videoMapHandle];
			}
		}
		//
		// alphafunc <func>
		//
		else if ( !Q_stricmp( token, "alphaFunc" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'alphaFunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			atestBits = NameToAFunc( token );
		}
		//
		// depthFunc <func>
		//
		else if ( !Q_stricmp( token, "depthfunc" ) ) {
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameter for 'depthfunc' keyword in shader '%s'\n", shader.name );
				return qfalse;
			}

			if ( !Q_stricmp( token, "lequal" ) ) {
				depthFuncBits = 0;
			} else if ( !Q_stricmp( token, "equal" ) )    {
				depthFuncBits = GLS_DEPTHFUNC_EQUAL;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown depthfunc '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// detail
		//
		else if ( !Q_stricmp( token, "detail" ) ) {
			stage->isDetail = qtrue;
		}
		//
		// fog
		//
		else if ( !Q_stricmp( token, "fog" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for fog in shader '%s'\n", shader.name );
				continue;
			}
			if ( !Q_stricmp( token, "on" ) ) {
				stage->isFogged = qtrue;
			} else {
				stage->isFogged = qfalse;
			}
		}
		//
		// blendfunc <srcFactor> <dstFactor>
		// or blendfunc <add|filter|blend>
		//
		else if ( !Q_stricmp( token, "blendfunc" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
				continue;
			}
			// check for "simple" blends first
			if ( !Q_stricmp( token, "add" ) ) {
				blendSrcBits = GLS_SRCBLEND_ONE;
				blendDstBits = GLS_DSTBLEND_ONE;
			} else if ( !Q_stricmp( token, "filter" ) ) {
				blendSrcBits = GLS_SRCBLEND_DST_COLOR;
				blendDstBits = GLS_DSTBLEND_ZERO;
			} else if ( !Q_stricmp( token, "blend" ) ) {
				blendSrcBits = GLS_SRCBLEND_SRC_ALPHA;
				blendDstBits = GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
			} else {
				// complex double blends
				blendSrcBits = NameToSrcBlendMode( token );

				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 ) {
					ri.Printf( PRINT_WARNING, "WARNING: missing parm for blendFunc in shader '%s'\n", shader.name );
					continue;
				}
				blendDstBits = NameToDstBlendMode( token );
			}

			// clear depth mask for blended surfaces
			if ( !depthMaskExplicit ) {
				depthMaskBits = 0;
			}
		}
		//
		// rgbGen
		//
		else if ( !Q_stricmp( token, "rgbGen" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for rgbGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) ) {
				ParseWaveForm( text, &stage->rgbWave );
				stage->rgbGen = CGEN_WAVEFORM;
			} else if ( !Q_stricmp( token, "const" ) )    {
				vec3_t color;

				ParseVector( text, 3, color );
				stage->constantColor[0] = 255 * color[0];
				stage->constantColor[1] = 255 * color[1];
				stage->constantColor[2] = 255 * color[2];

				stage->rgbGen = CGEN_CONST;
			} else if ( !Q_stricmp( token, "identity" ) )    {
				stage->rgbGen = CGEN_IDENTITY;
			} else if ( !Q_stricmp( token, "identityLighting" ) )    {
				stage->rgbGen = CGEN_IDENTITY_LIGHTING;
			} else if ( !Q_stricmp( token, "entity" ) )    {
				stage->rgbGen = CGEN_ENTITY;
			} else if ( !Q_stricmp( token, "oneMinusEntity" ) )    {
				stage->rgbGen = CGEN_ONE_MINUS_ENTITY;
			} else if ( !Q_stricmp( token, "vertex" ) )    {
				stage->rgbGen = CGEN_VERTEX;
				if ( stage->alphaGen == 0 ) {
					stage->alphaGen = AGEN_VERTEX;
				}
			} else if ( !Q_stricmp( token, "exactVertex" ) )    {
				stage->rgbGen = CGEN_EXACT_VERTEX;
			} else if ( !Q_stricmp( token, "lightingDiffuse" ) )    {
				stage->rgbGen = CGEN_LIGHTING_DIFFUSE;
			} else if ( !Q_stricmp( token, "oneMinusVertex" ) )    {
				stage->rgbGen = CGEN_ONE_MINUS_VERTEX;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown rgbGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// alphaGen
		//
		else if ( !Q_stricmp( token, "alphaGen" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parameters for alphaGen in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "wave" ) ) {
				ParseWaveForm( text, &stage->alphaWave );
				stage->alphaGen = AGEN_WAVEFORM;
			} else if ( !Q_stricmp( token, "const" ) )    {
				token = COM_ParseExt( text, qfalse );
				stage->constantColor[3] = 255 * atof( token );
				stage->alphaGen = AGEN_CONST;
			} else if ( !Q_stricmp( token, "identity" ) )    {
				stage->alphaGen = AGEN_IDENTITY;
			} else if ( !Q_stricmp( token, "entity" ) )    {
				stage->alphaGen = AGEN_ENTITY;
			} else if ( !Q_stricmp( token, "oneMinusEntity" ) )    {
				stage->alphaGen = AGEN_ONE_MINUS_ENTITY;
			}
			// Ridah
			else if ( !Q_stricmp( token, "normalzfade" ) ) {
				stage->alphaGen = AGEN_NORMALZFADE;
				token = COM_ParseExt( text, qfalse );
				if ( token[0] ) {
					stage->constantColor[3] = 255 * atof( token );
				} else {
					stage->constantColor[3] = 255;
				}

				token = COM_ParseExt( text, qfalse );
				if ( token[0] ) {
					stage->zFadeBounds[0] = atof( token );    // lower range
					token = COM_ParseExt( text, qfalse );
					stage->zFadeBounds[1] = atof( token );    // upper range
				} else {
					stage->zFadeBounds[0] = -1.0;   // lower range
					stage->zFadeBounds[1] =  1.0;   // upper range
				}

			}
			// done.
			else if ( !Q_stricmp( token, "vertex" ) ) {
				stage->alphaGen = AGEN_VERTEX;
			} else if ( !Q_stricmp( token, "lightingSpecular" ) )    {
				stage->alphaGen = AGEN_LIGHTING_SPECULAR;
			} else if ( !Q_stricmp( token, "oneMinusVertex" ) )    {
				stage->alphaGen = AGEN_ONE_MINUS_VERTEX;
			} else if ( !Q_stricmp( token, "portal" ) )    {
				stage->alphaGen = AGEN_PORTAL;
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 ) {
					shader.portalRange = 256;
					ri.Printf( PRINT_WARNING, "WARNING: missing range parameter for alphaGen portal in shader '%s', defaulting to 256\n", shader.name );
				} else
				{
					shader.portalRange = atof( token );
				}
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown alphaGen parameter '%s' in shader '%s'\n", token, shader.name );
				continue;
			}
		}
		//
		// tcGen <function>
		//
		else if ( !Q_stricmp( token, "texgen" ) || !Q_stricmp( token, "tcGen" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing texgen parm in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "environment" ) ) {
				stage->bundle[0].tcGen = TCGEN_ENVIRONMENT_MAPPED;
			} else if ( !Q_stricmp( token, "firerisenv" ) )    {
				stage->bundle[0].tcGen = TCGEN_FIRERISEENV_MAPPED;
			} else if ( !Q_stricmp( token, "lightmap" ) )    {
				stage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			} else if ( !Q_stricmp( token, "texture" ) || !Q_stricmp( token, "base" ) )     {
				stage->bundle[0].tcGen = TCGEN_TEXTURE;
			} else if ( !Q_stricmp( token, "vector" ) )    {
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[0] );
				ParseVector( text, 3, stage->bundle[0].tcGenVectors[1] );

				stage->bundle[0].tcGen = TCGEN_VECTOR;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: unknown texgen parm in shader '%s'\n", shader.name );
			}
		}
		//
		// tcMod <type> <...>
		//
		else if ( !Q_stricmp( token, "tcMod" ) ) {
			char buffer[1024] = "";

			while ( 1 )
			{
				token = COM_ParseExt( text, qfalse );
				if ( token[0] == 0 ) {
					break;
				}
				strcat( buffer, token );
				strcat( buffer, " " );
			}

			ParseTexMod( buffer, stage );

			continue;
		}
		//
		// depthmask
		//
		else if ( !Q_stricmp( token, "depthwrite" ) ) {
			depthMaskBits = GLS_DEPTHMASK_TRUE;
			depthMaskExplicit = qtrue;

			continue;
		} else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown parameter '%s' in shader '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// if cgen isn't explicitly specified, use either identity or identitylighting
	//
	if ( stage->rgbGen == CGEN_BAD ) {
		if ( blendSrcBits == 0 ||
			 blendSrcBits == GLS_SRCBLEND_ONE ||
			 blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) {
			stage->rgbGen = CGEN_IDENTITY_LIGHTING;
		} else {
			stage->rgbGen = CGEN_IDENTITY;
		}
	}


	//
	// implicitly assume that a GL_ONE GL_ZERO blend mask disables blending
	//
	if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) &&
		 ( blendDstBits == GLS_DSTBLEND_ZERO ) ) {
		blendDstBits = blendSrcBits = 0;
		depthMaskBits = GLS_DEPTHMASK_TRUE;
	}

	// decide which agens we can skip
	if ( stage->alphaGen == CGEN_IDENTITY ) {
		if ( stage->rgbGen == CGEN_IDENTITY
			 || stage->rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			stage->alphaGen = AGEN_SKIP;
		}
	}

	//
	// compute state bits
	//
	stage->stateBits = depthMaskBits |
					   blendSrcBits | blendDstBits |
					   atestBits |
					   depthFuncBits;

	return qtrue;
}

/*
===============
ParseDeform

deformVertexes wave <spread> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes normal <frequency> <amplitude>
deformVertexes move <vector> <waveform> <base> <amplitude> <phase> <frequency>
deformVertexes bulge <bulgeWidth> <bulgeHeight> <bulgeSpeed>
deformVertexes projectionShadow
deformVertexes autoSprite
deformVertexes autoSprite2
deformVertexes text[0-7]
===============
*/
static void ParseDeform( char **text ) {
	char    *token;
	deformStage_t   *ds;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing deform parm in shader '%s'\n", shader.name );
		return;
	}

	if ( shader.numDeforms == MAX_SHADER_DEFORMS ) {
		ri.Printf( PRINT_WARNING, "WARNING: MAX_SHADER_DEFORMS in '%s'\n", shader.name );
		return;
	}

	ds = &shader.deforms[ shader.numDeforms ];
	shader.numDeforms++;

	if ( !Q_stricmp( token, "projectionShadow" ) ) {
		ds->deformation = DEFORM_PROJECTION_SHADOW;
		return;
	}

	if ( !Q_stricmp( token, "autosprite" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE;
		return;
	}

	if ( !Q_stricmp( token, "autosprite2" ) ) {
		ds->deformation = DEFORM_AUTOSPRITE2;
		return;
	}

	if ( !Q_stricmpn( token, "text", 4 ) ) {
		int n;

		n = token[4] - '0';
		if ( n < 0 || n > 7 ) {
			n = 0;
		}
		ds->deformation = DEFORM_TEXT0 + n;
		return;
	}

	if ( !Q_stricmp( token, "bulge" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeWidth = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeHeight = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes bulge parm in shader '%s'\n", shader.name );
			return;
		}
		ds->bulgeSpeed = atof( token );

		ds->deformation = DEFORM_BULGE;
		return;
	}

	if ( !Q_stricmp( token, "wave" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}

		if ( atof( token ) != 0 ) {
			ds->deformationSpread = 1.0f / atof( token );
		} else
		{
			ds->deformationSpread = 100.0f;
			ri.Printf( PRINT_WARNING, "WARNING: illegal div value of 0 in deformVertexes command for shader '%s'\n", shader.name );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_WAVE;
		return;
	}

	if ( !Q_stricmp( token, "normal" ) ) {
		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.amplitude = atof( token );

		token = COM_ParseExt( text, qfalse );
		if ( token[0] == 0 ) {
			ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
			return;
		}
		ds->deformationWave.frequency = atof( token );

		ds->deformation = DEFORM_NORMALS;
		return;
	}

	if ( !Q_stricmp( token, "move" ) ) {
		int i;

		for ( i = 0 ; i < 3 ; i++ ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing deformVertexes parm in shader '%s'\n", shader.name );
				return;
			}
			ds->moveVector[i] = atof( token );
		}

		ParseWaveForm( text, &ds->deformationWave );
		ds->deformation = DEFORM_MOVE;
		return;
	}

	ri.Printf( PRINT_WARNING, "WARNING: unknown deformVertexes subtype '%s' found in shader '%s'\n", token, shader.name );
}


/*
===============
ParseSkyParms

skyParms <outerbox> <cloudheight> <innerbox>
===============
*/
static void ParseSkyParms( char **text ) {
	char        *token;
	static char *suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
	char pathname[MAX_QPATH];
	int i;

	// outerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for ( i = 0 ; i < 6 ; i++ ) {
			Com_sprintf( pathname, sizeof( pathname ), "%s_%s.tga"
						 , token, suf[i] );
			shader.sky.outerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, GL_CLAMP );
			if ( !shader.sky.outerbox[i] ) {
				shader.sky.outerbox[i] = tr.defaultImage;
			}
		}
	}

	// cloudheight
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	shader.sky.cloudHeight = atof( token );
	if ( !shader.sky.cloudHeight ) {
		shader.sky.cloudHeight = 512;
	}
	R_InitSkyTexCoords( shader.sky.cloudHeight );


	// innerbox
	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: 'skyParms' missing parameter in shader '%s'\n", shader.name );
		return;
	}
	if ( strcmp( token, "-" ) ) {
		for ( i = 0 ; i < 6 ; i++ ) {
			Com_sprintf( pathname, sizeof( pathname ), "%s_%s.tga"
						 , token, suf[i] );
			shader.sky.innerbox[i] = R_FindImageFile( ( char * ) pathname, qtrue, qtrue, GL_CLAMP );
			if ( !shader.sky.innerbox[i] ) {
				shader.sky.innerbox[i] = tr.defaultImage;
			}
		}
	}

	shader.isSky = qtrue;
}


/*
=================
ParseSort
=================
*/
void ParseSort( char **text ) {
	char    *token;

	token = COM_ParseExt( text, qfalse );
	if ( token[0] == 0 ) {
		ri.Printf( PRINT_WARNING, "WARNING: missing sort parameter in shader '%s'\n", shader.name );
		return;
	}

	if ( !Q_stricmp( token, "portal" ) ) {
		shader.sort = SS_PORTAL;
	} else if ( !Q_stricmp( token, "sky" ) ) {
		shader.sort = SS_ENVIRONMENT;
	} else if ( !Q_stricmp( token, "opaque" ) ) {
		shader.sort = SS_OPAQUE;
	} else if ( !Q_stricmp( token, "decal" ) )   {
		shader.sort = SS_DECAL;
	} else if ( !Q_stricmp( token, "seeThrough" ) ) {
		shader.sort = SS_SEE_THROUGH;
	} else if ( !Q_stricmp( token, "banner" ) ) {
		shader.sort = SS_BANNER;
	} else if ( !Q_stricmp( token, "additive" ) ) {
		shader.sort = SS_BLEND1;
	} else if ( !Q_stricmp( token, "nearest" ) ) {
		shader.sort = SS_NEAREST;
	} else if ( !Q_stricmp( token, "underwater" ) ) {
		shader.sort = SS_UNDERWATER;
	} else {
		shader.sort = atof( token );
	}
}



// this table is also present in q3map

typedef struct {
	char    *name;
	int clearSolid, surfaceFlags, contents;
} infoParm_t;

infoParm_t infoParms[] = {
	// server relevant contents

//----(SA)	modified
	{"clipmissile",  1,  0, CONTENTS_MISSILECLIP},       // impact only specific weapons (rl, gl)
//----(SA)	end

// RF, AI sight
	{"ai_nosight",   1,  0,  CONTENTS_AI_NOSIGHT},
	{"clipshot", 1,  0,  CONTENTS_CLIPSHOT},         // stops bullets
// RF, end

	{"water",        1,  0,  CONTENTS_WATER },
	{"slag",     1,  0,  CONTENTS_SLIME },       // uses the CONTENTS_SLIME flag, but the shader reference is changed to 'slag'
	// to idendify that this doesn't work the same as 'slime' did.
	// (slime hurts instantly, slag doesn't)
//	{"slime",		1,	0,	CONTENTS_SLIME },		// mildly damaging
	{"lava",     1,  0,  CONTENTS_LAVA },        // very damaging
	{"playerclip",   1,  0,  CONTENTS_PLAYERCLIP },
	{"monsterclip",  1,  0,  CONTENTS_MONSTERCLIP },
	{"nodrop",       1,  0,  CONTENTS_NODROP },      // don't drop items or leave bodies (death fog, lava, etc)
	{"nonsolid", 1,  SURF_NONSOLID,  0},                     // clears the solid flag

	// utility relevant attributes
	{"origin",       1,  0,  CONTENTS_ORIGIN },      // center of rotating brushes
	{"trans",        0,  0,  CONTENTS_TRANSLUCENT }, // don't eat contained surfaces
	{"detail",       0,  0,  CONTENTS_DETAIL },      // don't include in structural bsp
	{"structural",   0,  0,  CONTENTS_STRUCTURAL },  // force into structural bsp even if trnas
	{"areaportal",   1,  0,  CONTENTS_AREAPORTAL },  // divides areas
	{"clusterportal", 1,0,  CONTENTS_CLUSTERPORTAL },    // for bots
	{"donotenter",  1,  0,  CONTENTS_DONOTENTER },       // for bots

	// Rafael - nopass
	{"donotenterlarge", 1, 0,    CONTENTS_DONOTENTER_LARGE }, // for larger bots

	{"fog",          1,  0,  CONTENTS_FOG},          // carves surfaces entering
	{"sky",          0,  SURF_SKY,       0 },        // emit light from an environment map
	{"lightfilter",  0,  SURF_LIGHTFILTER, 0 },      // filter light going through it
	{"alphashadow",  0,  SURF_ALPHASHADOW, 0 },      // test light on a per-pixel basis
	{"hint",     0,  SURF_HINT,      0 },        // use as a primary splitter

	// server attributes
	{"slick",            0,  SURF_SLICK,     0 },
	{"noimpact",     0,  SURF_NOIMPACT,  0 },        // don't make impact explosions or marks
	{"nomarks",          0,  SURF_NOMARKS,   0 },        // don't make impact marks, but still explode
	{"ladder",           0,  SURF_LADDER,    0 },
	{"nodamage",     0,  SURF_NODAMAGE,  0 },

	{"monsterslick", 0,  SURF_MONSTERSLICK,  0},     // surf only slick for monsters

//	{"flesh",		0,	SURF_FLESH,		0 },
	{"glass",        0,  SURF_GLASS,     0 },    //----(SA)	added
	{"ceramic",      0,  SURF_CERAMIC,   0 },    //----(SA)	added

	// steps
	{"metal",        0,  SURF_METAL,     0 },
	{"metalsteps",   0,  SURF_METAL,     0 },    // retain bw compatibility with Q3A metal shaders... (SA)
	{"nosteps",      0,  SURF_NOSTEPS,   0 },
	{"woodsteps",    0,  SURF_WOOD,      0 },
	{"grasssteps",   0,  SURF_GRASS,     0 },
	{"gravelsteps",  0,  SURF_GRAVEL,    0 },
	{"carpetsteps",  0,  SURF_CARPET,    0 },
	{"snowsteps",    0,  SURF_SNOW,      0 },
	{"roofsteps",    0,  SURF_ROOF,      0 },    // tile roof

	{"rubble", 0, SURF_RUBBLE, 0 },

	// drawsurf attributes
	{"nodraw",       0,  SURF_NODRAW,    0 },    // don't generate a drawsurface (or a lightmap)
	{"pointlight",   0,  SURF_POINTLIGHT, 0 },   // sample lighting at vertexes
	{"nolightmap",   0,  SURF_NOLIGHTMAP,0 },        // don't generate a lightmap
	{"nodlight", 0,  SURF_NODLIGHT, 0 },     // don't ever add dynamic lights

	{"monsterslicknorth",    0, SURF_MONSLICK_N,0},
	{"monsterslickeast", 0, SURF_MONSLICK_E,0},
	{"monsterslicksouth",    0, SURF_MONSLICK_S,0},
	{"monsterslickwest", 0, SURF_MONSLICK_W,0}

};


/*
===============
ParseSurfaceParm

surfaceparm <name>
===============
*/
static void ParseSurfaceParm( char **text ) {
	char    *token;
	int numInfoParms = sizeof( infoParms ) / sizeof( infoParms[0] );
	int i;

	token = COM_ParseExt( text, qfalse );
	for ( i = 0 ; i < numInfoParms ; i++ ) {
		if ( !Q_stricmp( token, infoParms[i].name ) ) {
			shader.surfaceFlags |= infoParms[i].surfaceFlags;
			shader.contentFlags |= infoParms[i].contents;
#if 0
			if ( infoParms[i].clearSolid ) {
				si->contents &= ~CONTENTS_SOLID;
			}
#endif
			break;
		}
	}
}

/*
=================
ParseShader

The current text pointer is at the explicit text definition of the
shader.  Parse it into the global shader variable.  Later functions
will optimize it.
=================
*/
static qboolean ParseShader( char **text ) {
	char *token;
	int s;

	s = 0;

	token = COM_ParseExt( text, qtrue );
	if ( token[0] != '{' ) {
		ri.Printf( PRINT_WARNING, "WARNING: expecting '{', found '%s' instead in shader '%s'\n", token, shader.name );
		return qfalse;
	}

	while ( 1 )
	{
		token = COM_ParseExt( text, qtrue );
		if ( !token[0] ) {
			ri.Printf( PRINT_WARNING, "WARNING: no concluding '}' in shader %s\n", shader.name );
			return qfalse;
		}

		// end of shader definition
		if ( token[0] == '}' ) {
			break;
		}
		// stage definition
		else if ( token[0] == '{' ) {
			if ( !ParseStage( &stages[s], text ) ) {
				return qfalse;
			}
			stages[s].active = qtrue;
			s++;
			continue;
		}
		// skip stuff that only the QuakeEdRadient needs
		else if ( !Q_stricmpn( token, "qer", 3 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// sun parms
		else if ( !Q_stricmp( token, "q3map_sun" ) ) {
			float a, b;

			token = COM_ParseExt( text, qfalse );
			tr.sunLight[0] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[1] = atof( token );
			token = COM_ParseExt( text, qfalse );
			tr.sunLight[2] = atof( token );

			VectorNormalize( tr.sunLight );

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			VectorScale( tr.sunLight, a, tr.sunLight );

			token = COM_ParseExt( text, qfalse );
			a = atof( token );
			a = a / 180 * M_PI;

			token = COM_ParseExt( text, qfalse );
			b = atof( token );
			b = b / 180 * M_PI;

			tr.sunDirection[0] = cos( a ) * cos( b );
			tr.sunDirection[1] = sin( a ) * cos( b );
			tr.sunDirection[2] = sin( b );
		} else if ( !Q_stricmp( token, "deformVertexes" ) )    {
			ParseDeform( text );
			continue;
		} else if ( !Q_stricmp( token, "tesssize" ) )    {
			SkipRestOfLine( text );
			continue;
		} else if ( !Q_stricmp( token, "clampTime" ) )    {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] ) {
				shader.clampTime = atof( token );
			}
		}
		// skip stuff that only the q3map needs
		else if ( !Q_stricmpn( token, "q3map", 5 ) ) {
			SkipRestOfLine( text );
			continue;
		}
		// skip stuff that only q3map or the server needs
		else if ( !Q_stricmp( token, "surfaceParm" ) ) {
			ParseSurfaceParm( text );
			continue;
		}
		// no mip maps
		else if ( !Q_stricmp( token, "nomipmaps" ) ) {
			shader.noMipMaps = qtrue;
			shader.noPicMip = qtrue;
			continue;
		}
		// no picmip adjustment
		else if ( !Q_stricmp( token, "nopicmip" ) ) {
			shader.noPicMip = qtrue;
			continue;
		}
		// character picmip adjustment
		else if ( !Q_stricmp( token, "picmip2" ) ) {
			shader.characterMip = qtrue;
			continue;
		}
		// polygonOffset
		else if ( !Q_stricmp( token, "polygonOffset" ) ) {
			shader.polygonOffset = qtrue;
			continue;
		}
		// entityMergable, allowing sprite surfaces from multiple entities
		// to be merged into one batch.  This is a savings for smoke
		// puffs and blood, but can't be used for anything where the
		// shader calcs (not the surface function) reference the entity color or scroll
		else if ( !Q_stricmp( token, "entityMergable" ) ) {
			shader.entityMergable = qtrue;
			continue;
		}
		// fogParms
		else if ( !Q_stricmp( token, "fogParms" ) ) {
			if ( !ParseVector( text, 3, shader.fogParms.color ) ) {
				return qfalse;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing parm for 'fogParms' keyword in shader '%s'\n", shader.name );
				continue;
			}
			shader.fogParms.depthForOpaque = atof( token );

			// skip any old gradient directions
			SkipRestOfLine( text );
			continue;
		}
		// portal
		else if ( !Q_stricmp( token, "portal" ) ) {
			shader.sort = SS_PORTAL;
			continue;
		}
		// skyparms <cloudheight> <outerbox> <innerbox>
		else if ( !Q_stricmp( token, "skyparms" ) ) {
			ParseSkyParms( text );
			continue;
		}
		// This is fixed fog for the skybox/clouds determined solely by the shader
		// it will not change in a level and will not be necessary
		// to force clients to use a sky fog the server says to.
		// skyfogvars <(r,g,b)> <dist>
		else if ( !Q_stricmp( token, "skyfogvars" ) ) {
			vec3_t fogColor;

			if ( !ParseVector( text, 3, fogColor ) ) {
				return qfalse;
			}
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing density value for sky fog\n" );
				continue;
			}

			if ( atof( token ) > 1 ) {
				ri.Printf( PRINT_WARNING, "WARNING: last value for skyfogvars is 'density' which needs to be 0.0-1.0\n" );
				continue;
			}

			R_SetFog( FOG_SKY, 0, 5, fogColor[0], fogColor[1], fogColor[2], atof( token ) );
			continue;
		} else if ( !Q_stricmp( token, "sunshader" ) )        {
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing shader name for 'sunshader'\n" );
				continue;
			}
			tr.sunShaderName = CopyString( token );
		}
//----(SA)	added
		else if ( !Q_stricmp( token, "lightgridmulamb" ) ) { // ambient multiplier for lightgrid
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing value for 'lightgrid ambient multiplier'\n" );
				continue;
			}
			if ( atof( token ) > 0 ) {
				tr.lightGridMulAmbient = atof( token );
			}
		} else if ( !Q_stricmp( token, "lightgridmuldir" ) )        { // directional multiplier for lightgrid
			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing value for 'lightgrid directional multiplier'\n" );
				continue;
			}
			if ( atof( token ) > 0 ) {
				tr.lightGridMulDirected = atof( token );
			}
		}
//----(SA)	end
		else if ( !Q_stricmp( token, "waterfogvars" ) ) {
			vec3_t watercolor;
			float fogvar;
			char fogString[64];

			if ( !ParseVector( text, 3, watercolor ) ) {
				return qfalse;
			}
			token = COM_ParseExt( text, qfalse );

			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing density/distance value for water fog\n" );
				continue;
			}

			fogvar = atof( token );

			//----(SA)	right now allow one water color per map.  I'm sure this will need
			//			to change at some point, but I'm not sure how to track fog parameters
			//			on a "per-water volume" basis yet.

			if ( fogvar == 0 ) {       // '0' specifies "use the map values for everything except the fog color
				// TODO
			} else if ( fogvar > 1 )      { // distance "linear" fog
				Com_sprintf( fogString, sizeof( fogString ), "0 %d 1.1 %f %f %f 200", (int)fogvar, watercolor[0], watercolor[1], watercolor[2] );
//				R_SetFog(FOG_WATER, 0, fogvar, watercolor[0], watercolor[1], watercolor[2], 1.1);
			} else {                      // density "exp" fog
				Com_sprintf( fogString, sizeof( fogString ), "0 5 %f %f %f %f 200", fogvar, watercolor[0], watercolor[1], watercolor[2] );
//				R_SetFog(FOG_WATER, 0, 5, watercolor[0], watercolor[1], watercolor[2], fogvar);
			}

//		near
//		far
//		density
//		r,g,b
//		time to complete
			ri.Cvar_Set( "r_waterFogColor", fogString );

			continue;
		}
		// fogvars
		else if ( !Q_stricmp( token, "fogvars" ) ) {
			vec3_t fogColor;
			float fogDensity;
			int fogFar;

			if ( !ParseVector( text, 3, fogColor ) ) {
				return qfalse;
			}

			token = COM_ParseExt( text, qfalse );
			if ( !token[0] ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing density value for the fog\n" );
				continue;
			}


			//----(SA)	NOTE:	fogFar > 1 means the shader is setting the farclip, < 1 means setting
			//					density (so old maps or maps that just need softening fog don't have to care about farclip)

			fogDensity = atof( token );
			if ( fogDensity >= 1 ) { // linear
				fogFar      = fogDensity;
			} else {
				fogFar      = 5;
			}

//			R_SetFog(FOG_MAP, 0, fogFar, fogColor[0], fogColor[1], fogColor[2], fogDensity);
			ri.Cvar_Set( "r_mapFogColor", va( "0 %d %f %f %f %f 0", fogFar, fogDensity, fogColor[0], fogColor[1], fogColor[2] ) );
//			R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 50, 0, 0, 0, 0);

			continue;
		}
		// done.
		// Ridah, allow disable fog for some shaders
		else if ( !Q_stricmp( token, "nofog" ) ) {
			shader.noFog = qtrue;
			continue;
		}
		// done.
		// RF, allow each shader to permit compression if available
		else if ( !Q_stricmp( token, "allowcompress" ) ) {
			tr.allowCompress = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "nocompress" ) )   {
			tr.allowCompress = -1;
			continue;
		}
		// done.
		// light <value> determines flaring in q3map, not needed here
		else if ( !Q_stricmp( token, "light" ) ) {
			token = COM_ParseExt( text, qfalse );
			continue;
		}
		// cull <face>
		else if ( !Q_stricmp( token, "cull" ) ) {
			token = COM_ParseExt( text, qfalse );
			if ( token[0] == 0 ) {
				ri.Printf( PRINT_WARNING, "WARNING: missing cull parms in shader '%s'\n", shader.name );
				continue;
			}

			if ( !Q_stricmp( token, "none" ) || !Q_stricmp( token, "twosided" ) || !Q_stricmp( token, "disable" ) ) {
				shader.cullType = CT_TWO_SIDED;
			} else if ( !Q_stricmp( token, "back" ) || !Q_stricmp( token, "backside" ) || !Q_stricmp( token, "backsided" ) )      {
				shader.cullType = CT_BACK_SIDED;
			} else
			{
				ri.Printf( PRINT_WARNING, "WARNING: invalid cull parm '%s' in shader '%s'\n", token, shader.name );
			}
			continue;
		}
		// sort
		else if ( !Q_stricmp( token, "sort" ) ) {
			ParseSort( text );
			continue;
		} else
		{
			ri.Printf( PRINT_WARNING, "WARNING: unknown general shader parameter '%s' in '%s'\n", token, shader.name );
			return qfalse;
		}
	}

	//
	// ignore shaders that don't have any stages, unless it is a sky or fog
	//
	if ( s == 0 && !shader.isSky && !( shader.contentFlags & CONTENTS_FOG ) ) {
		return qfalse;
	}

	shader.explicitlyDefined = qtrue;

	return qtrue;
}

/*
========================================================================================

SHADER OPTIMIZATION AND FOGGING

========================================================================================
*/

/*
===================
ComputeStageIteratorFunc

See if we can use on of the simple fastpath stage functions,
otherwise set to the generic stage function
===================
*/
static void ComputeStageIteratorFunc( void ) {
	shader.optimalStageIteratorFunc = RB_StageIteratorGeneric;

	//
	// see if this should go into the sky path
	//
	if ( shader.isSky ) {
		shader.optimalStageIteratorFunc = RB_StageIteratorSky;
		goto done;
	}

	if ( r_ignoreFastPath->integer ) {
		return;
	}

	//
	// see if this can go into the vertex lit fast path
	//
	if ( shader.numUnfoggedPasses == 1 ) {
		if ( stages[0].rgbGen == CGEN_LIGHTING_DIFFUSE ) {
			if ( stages[0].alphaGen == AGEN_IDENTITY ) {
				if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE ) {
					if ( !shader.polygonOffset ) {
						if ( !shader.multitextureEnv ) {
							if ( !shader.numDeforms ) {
								shader.optimalStageIteratorFunc = RB_StageIteratorVertexLitTexture;
								goto done;
							}
						}
					}
				}
			}
		}
	}

	//
	// see if this can go into an optimized LM, multitextured path
	//
	if ( shader.numUnfoggedPasses == 1 ) {
		if ( ( stages[0].rgbGen == CGEN_IDENTITY ) && ( stages[0].alphaGen == AGEN_IDENTITY ) ) {
			if ( stages[0].bundle[0].tcGen == TCGEN_TEXTURE &&
				 stages[0].bundle[1].tcGen == TCGEN_LIGHTMAP ) {
				if ( !shader.polygonOffset ) {
					if ( !shader.numDeforms ) {
						if ( shader.multitextureEnv ) {
							shader.optimalStageIteratorFunc = RB_StageIteratorLightmappedMultitexture;
							goto done;
						}
					}
				}
			}
		}
	}

done:
	return;
}

typedef struct {
	int blendA;
	int blendB;

	int multitextureEnv;
	int multitextureBlend;
} collapse_t;

static collapse_t collapse[] = {
	{ 0, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, 0 },

	{ 0, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, 0 },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO, GLS_DSTBLEND_SRC_COLOR | GLS_SRCBLEND_ZERO,
	  GL_MODULATE, GLS_DSTBLEND_ZERO | GLS_SRCBLEND_DST_COLOR },

	{ 0, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, 0 },

	{ GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE,
	  GL_ADD, GLS_DSTBLEND_ONE | GLS_SRCBLEND_ONE },
#if 0
	{ 0, GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA | GLS_SRCBLEND_SRC_ALPHA,
	  GL_DECAL, 0 },
#endif
	{ -1 }
};

/*
================
CollapseMultitexture

Attempt to combine two stages into a single multitexture stage
FIXME: I think modulated add + modulated add collapses incorrectly
=================
*/
static qboolean CollapseMultitexture( void ) {
	int abits, bbits;
	int i;
	textureBundle_t tmpBundle;

	if ( !qglActiveTextureARB ) {
		return qfalse;
	}

	// make sure both stages are active
	if ( !stages[0].active || !stages[1].active ) {
		return qfalse;
	}

	// on voodoo2, don't combine different tmus
	if ( glConfig.driverType == GLDRV_VOODOO ) {
		if ( stages[0].bundle[0].image[0]->TMU ==
			 stages[1].bundle[0].image[0]->TMU ) {
			return qfalse;
		}
	}

	abits = stages[0].stateBits;
	bbits = stages[1].stateBits;

	// make sure that both stages have identical state other than blend modes
	if ( ( abits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) !=
		 ( bbits & ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS | GLS_DEPTHMASK_TRUE ) ) ) {
		return qfalse;
	}

	abits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	bbits &= ( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );

	// search for a valid multitexture blend function
	for ( i = 0; collapse[i].blendA != -1 ; i++ ) {
		if ( abits == collapse[i].blendA
			 && bbits == collapse[i].blendB ) {
			break;
		}
	}

	// nothing found
	if ( collapse[i].blendA == -1 ) {
		return qfalse;
	}

	// GL_ADD is a separate extension
	if ( collapse[i].multitextureEnv == GL_ADD && !glConfig.textureEnvAddAvailable ) {
		return qfalse;
	}

	// make sure waveforms have identical parameters
	if ( ( stages[0].rgbGen != stages[1].rgbGen ) ||
		 ( stages[0].alphaGen != stages[1].alphaGen ) ) {
		return qfalse;
	}

	// an add collapse can only have identity colors
	if ( collapse[i].multitextureEnv == GL_ADD && stages[0].rgbGen != CGEN_IDENTITY ) {
		return qfalse;
	}

	if ( stages[0].rgbGen == CGEN_WAVEFORM ) {
		if ( memcmp( &stages[0].rgbWave,
					 &stages[1].rgbWave,
					 sizeof( stages[0].rgbWave ) ) ) {
			return qfalse;
		}
	}
	if ( stages[0].alphaGen == CGEN_WAVEFORM ) {
		if ( memcmp( &stages[0].alphaWave,
					 &stages[1].alphaWave,
					 sizeof( stages[0].alphaWave ) ) ) {
			return qfalse;
		}
	}


	// make sure that lightmaps are in bundle 1 for 3dfx
	if ( stages[0].bundle[0].isLightmap ) {
		tmpBundle = stages[0].bundle[0];
		stages[0].bundle[0] = stages[1].bundle[0];
		stages[0].bundle[1] = tmpBundle;
	} else
	{
		stages[0].bundle[1] = stages[1].bundle[0];
	}

	// set the new blend state bits
	shader.multitextureEnv = collapse[i].multitextureEnv;
	stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
	stages[0].stateBits |= collapse[i].multitextureBlend;

	//
	// move down subsequent shaders
	//
	memmove( &stages[1], &stages[2], sizeof( stages[0] ) * ( MAX_SHADER_STAGES - 2 ) );
	memset( &stages[MAX_SHADER_STAGES - 1], 0, sizeof( stages[0] ) );

	return qtrue;
}


/*
==============
SortNewShader

Positions the most recently created shader in the tr.sortedShaders[]
array so that the shader->sort key is sorted reletive to the other
shaders.

Sets shader->sortedIndex
==============
*/
static void SortNewShader( void ) {
	int i;
	float sort;
	shader_t    *newShader;

	newShader = tr.shaders[ tr.numShaders - 1 ];
	sort = newShader->sort;

	for ( i = tr.numShaders - 2 ; i >= 0 ; i-- ) {
		if ( tr.sortedShaders[ i ]->sort <= sort ) {
			break;
		}
		tr.sortedShaders[i + 1] = tr.sortedShaders[i];
		tr.sortedShaders[i + 1]->sortedIndex++;
	}

	newShader->sortedIndex = i + 1;
	tr.sortedShaders[i + 1] = newShader;
}


/*
====================
GeneratePermanentShader
====================
*/
static shader_t *GeneratePermanentShader( void ) {
	shader_t    *newShader;
	int i, b;
	int size, hash;

	if ( tr.numShaders == MAX_SHADERS ) {
		ri.Printf( PRINT_WARNING, "WARNING: GeneratePermanentShader - MAX_SHADERS hit\n" );
		return tr.defaultShader;
	}

	// Ridah, caching system
	newShader = R_CacheShaderAlloc( sizeof( shader_t ) );

	*newShader = shader;

	if ( shader.sort <= SS_OPAQUE ) {
		newShader->fogPass = FP_EQUAL;
	} else if ( shader.contentFlags & CONTENTS_FOG ) {
		newShader->fogPass = FP_LE;
	}

	tr.shaders[ tr.numShaders ] = newShader;
	newShader->index = tr.numShaders;

	tr.sortedShaders[ tr.numShaders ] = newShader;
	newShader->sortedIndex = tr.numShaders;

	tr.numShaders++;

	for ( i = 0 ; i < newShader->numUnfoggedPasses ; i++ ) {
		if ( !stages[i].active ) {
			newShader->stages[i] = NULL;    // Ridah, make sure it's null
			break;
		}
		// Ridah, caching system
		newShader->stages[i] = R_CacheShaderAlloc( sizeof( stages[i] ) );

		*newShader->stages[i] = stages[i];

		for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
			if ( !newShader->stages[i]->bundle[b].numTexMods ) {
				// make sure unalloc'd texMods aren't pointing to some random point in memory
				newShader->stages[i]->bundle[b].texMods = NULL;
				continue;
			}
			size = newShader->stages[i]->bundle[b].numTexMods * sizeof( texModInfo_t );
			// Ridah, caching system
			newShader->stages[i]->bundle[b].texMods = R_CacheShaderAlloc( size );

			memcpy( newShader->stages[i]->bundle[b].texMods, stages[i].bundle[b].texMods, size );
		}
	}

	SortNewShader();

	hash = generateHashValue( newShader->name );
	newShader->next = hashTable[hash];
	hashTable[hash] = newShader;

	return newShader;
}

/*
=================
VertexLightingCollapse

If vertex lighting is enabled, only render a single
pass, trying to guess which is the correct one to best aproximate
what it is supposed to look like.
=================
*/
static void VertexLightingCollapse( void ) {
	int stage;
	shaderStage_t   *bestStage;
	int bestImageRank;
	int rank;

	// if we aren't opaque, just use the first pass
	if ( shader.sort == SS_OPAQUE ) {

		// pick the best texture for the single pass
		bestStage = &stages[0];
		bestImageRank = -999999;

		for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
			shaderStage_t *pStage = &stages[stage];

			if ( !pStage->active ) {
				break;
			}
			rank = 0;

			if ( pStage->bundle[0].isLightmap ) {
				rank -= 100;
			}
			if ( pStage->bundle[0].tcGen != TCGEN_TEXTURE ) {
				rank -= 5;
			}
			if ( pStage->bundle[0].numTexMods ) {
				rank -= 5;
			}
			if ( pStage->rgbGen != CGEN_IDENTITY && pStage->rgbGen != CGEN_IDENTITY_LIGHTING ) {
				rank -= 3;
			}

			if ( rank > bestImageRank  ) {
				bestImageRank = rank;
				bestStage = pStage;
			}
		}

		stages[0].bundle[0] = bestStage->bundle[0];
		stages[0].stateBits &= ~( GLS_DSTBLEND_BITS | GLS_SRCBLEND_BITS );
		stages[0].stateBits |= GLS_DEPTHMASK_TRUE;
		if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
			stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		} else {
			stages[0].rgbGen = CGEN_EXACT_VERTEX;
		}
		stages[0].alphaGen = AGEN_SKIP;
	} else {
		// don't use a lightmap (tesla coils)
		if ( stages[0].bundle[0].isLightmap ) {
			stages[0] = stages[1];
		}

		// if we were in a cross-fade cgen, hack it to normal
		if ( stages[0].rgbGen == CGEN_ONE_MINUS_ENTITY || stages[1].rgbGen == CGEN_ONE_MINUS_ENTITY ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_SAWTOOTH )
			 && ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_INVERSE_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
		if ( ( stages[0].rgbGen == CGEN_WAVEFORM && stages[0].rgbWave.func == GF_INVERSE_SAWTOOTH )
			 && ( stages[1].rgbGen == CGEN_WAVEFORM && stages[1].rgbWave.func == GF_SAWTOOTH ) ) {
			stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		}
	}

	for ( stage = 1; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		memset( pStage, 0, sizeof( *pStage ) );
	}
}

/*
=========================
FinishShader

Returns a freshly allocated shader with all the needed info
from the current global working shader
=========================
*/
static shader_t *FinishShader( void ) {
	int stage, i;
	qboolean hasLightmapStage;
	qboolean vertexLightmap;

	hasLightmapStage = qfalse;
	vertexLightmap = qfalse;

	//
	// set sky stuff appropriate
	//
	if ( shader.isSky ) {
		shader.sort = SS_ENVIRONMENT;
	}

	//
	// set polygon offset
	//
	if ( shader.polygonOffset && !shader.sort ) {
		shader.sort = SS_DECAL;
	}

	//
	// set appropriate stage information
	//
	for ( stage = 0; stage < MAX_SHADER_STAGES; stage++ ) {
		shaderStage_t *pStage = &stages[stage];

		if ( !pStage->active ) {
			break;
		}

		// check for a missing texture
		if ( !pStage->bundle[0].image[0] ) {
			ri.Printf( PRINT_WARNING, "Shader %s has a stage with no image\n", shader.name );
			pStage->active = qfalse;
			continue;
		}

		//
		// ditch this stage if it's detail and detail textures are disabled
		//
		if ( pStage->isDetail && !r_detailTextures->integer ) {
			if ( stage < ( MAX_SHADER_STAGES - 1 ) ) {
				memmove( pStage, pStage + 1, sizeof( *pStage ) * ( MAX_SHADER_STAGES - stage - 1 ) );
				// kill the last stage, since it's now a duplicate
				for ( i = MAX_SHADER_STAGES - 1; i > stage; i-- ) {
					if ( stages[i].active ) {
						memset(  &stages[i], 0, sizeof( *pStage ) );
						break;
					}
				}
				stage--;    // the next stage is now the current stage, so check it again
			} else {
				memset( pStage, 0, sizeof( *pStage ) );
			}
			continue;
		}

		//
		// default texture coordinate generation
		//
		if ( pStage->bundle[0].isLightmap ) {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_LIGHTMAP;
			}
			hasLightmapStage = qtrue;
		} else {
			if ( pStage->bundle[0].tcGen == TCGEN_BAD ) {
				pStage->bundle[0].tcGen = TCGEN_TEXTURE;
			}
		}


		// not a true lightmap but we want to leave existing
		// behaviour in place and not print out a warning
		//if (pStage->rgbGen == CGEN_VERTEX) {
		//  vertexLightmap = qtrue;
		//}



		//
		// determine sort order and fog color adjustment
		//
		if ( ( pStage->stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) &&
			 ( stages[0].stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) ) {
			int blendSrcBits = pStage->stateBits & GLS_SRCBLEND_BITS;
			int blendDstBits = pStage->stateBits & GLS_DSTBLEND_BITS;

			// fog color adjustment only works for blend modes that have a contribution
			// that aproaches 0 as the modulate values aproach 0 --
			// GL_ONE, GL_ONE
			// GL_ZERO, GL_ONE_MINUS_SRC_COLOR
			// GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA

			// modulate, additive
			if ( ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE ) ) ||
				 ( ( blendSrcBits == GLS_SRCBLEND_ZERO ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_COLOR ) ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGB;
			}
			// strict blend
			else if ( ( blendSrcBits == GLS_SRCBLEND_SRC_ALPHA ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_ALPHA;
			}
			// premultiplied alpha
			else if ( ( blendSrcBits == GLS_SRCBLEND_ONE ) && ( blendDstBits == GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA ) ) {
				pStage->adjustColorsForFog = ACFF_MODULATE_RGBA;
			} else {
				// we can't adjust this one correctly, so it won't be exactly correct in fog
			}

			// don't screw with sort order if this is a portal or environment
			if ( !shader.sort ) {
				// see through item, like a grill or grate
				if ( pStage->stateBits & GLS_DEPTHMASK_TRUE ) {
					shader.sort = SS_SEE_THROUGH;
				} else {
					shader.sort = SS_BLEND0;
				}
			}
		}
	}

	// there are times when you will need to manually apply a sort to
	// opaque alpha tested shaders that have later blend passes
	if ( !shader.sort ) {
		shader.sort = SS_OPAQUE;
	}

	//
	// if we are in r_vertexLight mode, never use a lightmap texture
	//
	if ( stage > 1 && ( ( r_vertexLight->integer && !r_uiFullScreen->integer ) || glConfig.hardwareType == GLHW_PERMEDIA2 ) ) {
		VertexLightingCollapse();
		stage = 1;
		hasLightmapStage = qfalse;
	}

	//
	// look for multitexture potential
	//
	if ( stage > 1 && CollapseMultitexture() ) {
		stage--;
	}

	if ( shader.lightmapIndex >= 0 && !hasLightmapStage ) {
		if ( vertexLightmap ) {
			ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has VERTEX forced lightmap!\n", shader.name );
		} else {
			ri.Printf( PRINT_DEVELOPER, "WARNING: shader '%s' has lightmap but no lightmap stage!\n", shader.name );
			shader.lightmapIndex = LIGHTMAP_NONE;
		}
	}


	//
	// compute number of passes
	//
	shader.numUnfoggedPasses = stage;

	// fogonly shaders don't have any normal passes
	if ( stage == 0 ) {
		shader.sort = SS_FOG;
	}

	// determine which stage iterator function is appropriate
	ComputeStageIteratorFunc();

	// RF default back to no compression for next shader
	if ( r_ext_compressed_textures->integer == 2 ) {
		tr.allowCompress = qfalse;
	}

	return GeneratePermanentShader();
}

//========================================================================================

/*
====================
FindShaderInShaderText

Scans the combined text description of all the shader files for
the given shader name.

return NULL if not found

If found, it will return a valid shader
=====================
*/
static char *FindShaderInShaderText( const char *shadername ) {
	char *p = s_shaderText;
	char *token;

	if ( !p ) {
		return NULL;
	}

	// Ridah, optimized shader loading
	{
		shaderStringPointer_t *pShaderString;
		unsigned short int checksum;

		checksum = generateHashValue( shadername );

		// if it's known, skip straight to it's position
		pShaderString = &shaderChecksumLookup[checksum];
		while ( pShaderString && pShaderString->pStr ) {
			p = pShaderString->pStr;

			token = COM_ParseExt( &p, qtrue );

			if ( ( token[0] != 0 ) && !Q_stricmp( token, shadername ) ) {
				return p;
			}

			pShaderString = pShaderString->next;
		}
	}

	// done.

	/*
	// look for label
	// note that this could get confused if a shader name is used inside
	// another shader definition
	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( token[0] == '{' ) {
			// skip the definition
			SkipBracedSection( &p );
		} else if ( !Q_stricmp( token, shadername ) ) {
			return p;
		} else {
			// skip to end of line
			SkipRestOfLine( &p );
		}
	}
	*/

	return NULL;
}

/*
==================
R_FindShaderByName

Will always return a valid shader, but it might be the
default shader if the real one can't be found.
==================
*/
shader_t *R_FindShaderByName( const char *name ) {
	char strippedName[MAX_QPATH];
	int hash;
	shader_t    *sh;

	if ( ( name == NULL ) || ( name[0] == 0 ) ) {  // bk001205
		return tr.defaultShader;
	}

	COM_StripExtension( name, strippedName );

	hash = generateHashValue( strippedName );

	//
	// see if the shader is already loaded
	//
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( Q_stricmp( sh->name, strippedName ) == 0 ) {
			// match found
			return sh;
		}
	}

	return tr.defaultShader;
}


/*
===============
R_FindShader

Will always return a valid shader, but it might be the
default shader if the real one can't be found.

In the interest of not requiring an explicit shader text entry to
be defined for every single image used in the game, three default
shader behaviors can be auto-created for any image:

If lightmapIndex == LIGHTMAP_NONE, then the image will have
dynamic diffuse lighting applied to it, as apropriate for most
entity skin surfaces.

If lightmapIndex == LIGHTMAP_2D, then the image will be used
for 2D rendering unless an explicit shader is found

If lightmapIndex == LIGHTMAP_BY_VERTEX, then the image will use
the vertex rgba modulate values, as apropriate for misc_model
pre-lit surfaces.

Other lightmapIndex values will have a lightmap stage created
and src*dest blending applied with the texture, as apropriate for
most world construction surfaces.

===============
*/
shader_t *R_FindShader( const char *name, int lightmapIndex, qboolean mipRawImage ) {
	char strippedName[MAX_QPATH];
	char fileName[MAX_QPATH];
	int i, hash;
	char        *shaderText;
	image_t     *image;
	shader_t    *sh;

	if ( name[0] == 0 ) {
		return tr.defaultShader;
	}

	// use (fullbright) vertex lighting if the bsp file doesn't have
	// lightmaps
	if ( lightmapIndex >= 0 && lightmapIndex >= tr.numLightmaps ) {
		lightmapIndex = LIGHTMAP_BY_VERTEX;
	}

	COM_StripExtension( name, strippedName );

	hash = generateHashValue( strippedName );

	//
	// see if the shader is already loaded
	//
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
		// index by name

		// Ridah, modified this so we don't keep trying to load an invalid lightmap shader
/*
		if ( sh->lightmapIndex == lightmapIndex &&
			!Q_stricmp(sh->name, strippedName)) {
			// match found
			return sh;
		}
*/
		if ( ( ( sh->lightmapIndex == lightmapIndex ) || ( sh->lightmapIndex < 0 && lightmapIndex >= 0 ) ) &&
			 !Q_stricmp( sh->name, strippedName ) ) {
			// match found
			return sh;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if ( r_smp->integer ) {
		R_SyncRenderThread();
	}

	// Ridah, check the cache
	sh = R_FindCachedShader( strippedName, lightmapIndex, hash );
	if ( sh ) {
		return sh;
	}
	// done.

	// clear the global shader
	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz( shader.name, strippedName, sizeof( shader.name ) );
	shader.lightmapIndex = lightmapIndex;
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1 = qtrue;
	shader.needsST2 = qtrue;
	shader.needsColor = qtrue;

	//
	// attempt to define shader from an explicit parameter file
	//
	shaderText = FindShaderInShaderText( strippedName );
	if ( shaderText ) {
		// enable this when building a pak file to get a global list
		// of all explicit shaders
		if ( r_printShaders->integer ) {
			ri.Printf( PRINT_ALL, "*SHADER* %s\n", name );
		}

		if ( !ParseShader( &shaderText ) ) {
			// had errors, so use default shader
			shader.defaultShader = qtrue;
		}
		sh = FinishShader();
		return sh;
	}


	//
	// if not defined in the in-memory shader descriptions,
	// look for a single TGA, BMP, or PCX
	//
	Q_strncpyz( fileName, name, sizeof( fileName ) );
	COM_DefaultExtension( fileName, sizeof( fileName ), ".tga" );
	image = R_FindImageFile( fileName, mipRawImage, mipRawImage, mipRawImage ? GL_REPEAT : GL_CLAMP );
	if ( !image ) {
		ri.Printf( PRINT_DEVELOPER, "Couldn't find image for shader %s\n", name );
		shader.defaultShader = qtrue;
		return FinishShader();
	}

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
							  GLS_SRCBLEND_SRC_ALPHA |
							  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;   // lightmaps are scaled on creation
		// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	return FinishShader();
}


qhandle_t RE_RegisterShaderFromImage( const char *name, int lightmapIndex, image_t *image, qboolean mipRawImage ) {
	int i, hash;
	shader_t    *sh;

	hash = generateHashValue( name );

	//
	// see if the shader is already loaded
	//
	for ( sh = hashTable[hash]; sh; sh = sh->next ) {
		// NOTE: if there was no shader or image available with the name strippedName
		// then a default shader is created with lightmapIndex == LIGHTMAP_NONE, so we
		// have to check all default shaders otherwise for every call to R_FindShader
		// with that same strippedName a new default shader is created.
		if ( ( sh->lightmapIndex == lightmapIndex || sh->defaultShader ) &&
			 // index by name
			 !Q_stricmp( sh->name, name ) ) {
			// match found
			return sh->index;
		}
	}

	// make sure the render thread is stopped, because we are probably
	// going to have to upload an image
	if ( r_smp->integer ) {
		R_SyncRenderThread();
	}

	// clear the global shader
	Com_Memset( &shader, 0, sizeof( shader ) );
	Com_Memset( &stages, 0, sizeof( stages ) );
	Q_strncpyz( shader.name, name, sizeof( shader.name ) );
	shader.lightmapIndex = lightmapIndex;
	for ( i = 0 ; i < MAX_SHADER_STAGES ; i++ ) {
		stages[i].bundle[0].texMods = texMods[i];
	}

	// FIXME: set these "need" values apropriately
	shader.needsNormal = qtrue;
	shader.needsST1 = qtrue;
	shader.needsST2 = qtrue;
	shader.needsColor = qtrue;

	//
	// create the default shading commands
	//
	if ( shader.lightmapIndex == LIGHTMAP_NONE ) {
		// dynamic colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_LIGHTING_DIFFUSE;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_BY_VERTEX ) {
		// explicit colors at vertexes
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_EXACT_VERTEX;
		stages[0].alphaGen = AGEN_SKIP;
		stages[0].stateBits = GLS_DEFAULT;
	} else if ( shader.lightmapIndex == LIGHTMAP_2D ) {
		// GUI elements
		stages[0].bundle[0].image[0] = image;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_VERTEX;
		stages[0].alphaGen = AGEN_VERTEX;
		stages[0].stateBits = GLS_DEPTHTEST_DISABLE |
							  GLS_SRCBLEND_SRC_ALPHA |
							  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA;
	} else if ( shader.lightmapIndex == LIGHTMAP_WHITEIMAGE ) {
		// fullbright level
		stages[0].bundle[0].image[0] = tr.whiteImage;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY_LIGHTING;
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	} else {
		// two pass lightmap
		stages[0].bundle[0].image[0] = tr.lightmaps[shader.lightmapIndex];
		stages[0].bundle[0].isLightmap = qtrue;
		stages[0].active = qtrue;
		stages[0].rgbGen = CGEN_IDENTITY;   // lightmaps are scaled on creation
		// for identitylight
		stages[0].stateBits = GLS_DEFAULT;

		stages[1].bundle[0].image[0] = image;
		stages[1].active = qtrue;
		stages[1].rgbGen = CGEN_IDENTITY;
		stages[1].stateBits |= GLS_SRCBLEND_DST_COLOR | GLS_DSTBLEND_ZERO;
	}

	sh = FinishShader();
	return sh->index;
}


/*
====================
RE_RegisterShaderLightMap

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShaderLightMap( const char *name, int lightmapIndex ) {
	shader_t    *sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, lightmapIndex, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShader

This is the exported shader entry point for the rest of the system
It will always return an index that will be valid.

This should really only be used for explicit shaders, because there is no
way to ask for different implicit lighting modes (vertex, lightmap, etc)
====================
*/
qhandle_t RE_RegisterShader( const char *name ) {
	shader_t    *sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, LIGHTMAP_2D, qtrue );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
RE_RegisterShaderNoMip

For menu graphics that should never be picmiped
====================
*/
qhandle_t RE_RegisterShaderNoMip( const char *name ) {
	shader_t    *sh;

	if ( strlen( name ) >= MAX_QPATH ) {
		Com_Printf( "Shader name exceeds MAX_QPATH\n" );
		return 0;
	}

	sh = R_FindShader( name, LIGHTMAP_2D, qfalse );

	// we want to return 0 if the shader failed to
	// load for some reason, but R_FindShader should
	// still keep a name allocated for it, so if
	// something calls RE_RegisterShader again with
	// the same name, we don't try looking for it again
	if ( sh->defaultShader ) {
		return 0;
	}

	return sh->index;
}


/*
====================
R_GetShaderByHandle

When a handle is passed in by another module, this range checks
it and returns a valid (possibly default) shader_t to be used internally.
====================
*/
shader_t *R_GetShaderByHandle( qhandle_t hShader ) {
	if ( hShader < 0 ) {
		ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader ); // bk: FIXME name
		return tr.defaultShader;
	}
	if ( hShader >= tr.numShaders ) {
		ri.Printf( PRINT_WARNING, "R_GetShaderByHandle: out of range hShader '%d'\n", hShader );
		return tr.defaultShader;
	}
	return tr.shaders[hShader];
}

/*
===============
R_ShaderList_f

Dump information on all valid shaders to the console
A second parameter will cause it to print in sorted order
===============
*/
void    R_ShaderList_f( void ) {
	int i;
	int count;
	shader_t    *shader;

	ri.Printf( PRINT_ALL, "-----------------------\n" );

	count = 0;
	for ( i = 0 ; i < tr.numShaders ; i++ ) {
		if ( ri.Cmd_Argc() > 1 ) {
			shader = tr.sortedShaders[i];
		} else {
			shader = tr.shaders[i];
		}

		ri.Printf( PRINT_ALL, "%i ", shader->numUnfoggedPasses );

		if ( shader->lightmapIndex >= 0 ) {
			ri.Printf( PRINT_ALL, "L " );
		} else {
			ri.Printf( PRINT_ALL, "  " );
		}
		if ( shader->multitextureEnv == GL_ADD ) {
			ri.Printf( PRINT_ALL, "MT(a) " );
		} else if ( shader->multitextureEnv == GL_MODULATE ) {
			ri.Printf( PRINT_ALL, "MT(m) " );
		} else if ( shader->multitextureEnv == GL_DECAL ) {
			ri.Printf( PRINT_ALL, "MT(d) " );
		} else {
			ri.Printf( PRINT_ALL, "      " );
		}
		if ( shader->explicitlyDefined ) {
			ri.Printf( PRINT_ALL, "E " );
		} else {
			ri.Printf( PRINT_ALL, "  " );
		}

		if ( shader->optimalStageIteratorFunc == RB_StageIteratorGeneric ) {
			ri.Printf( PRINT_ALL, "gen " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorSky ) {
			ri.Printf( PRINT_ALL, "sky " );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorLightmappedMultitexture ) {
			ri.Printf( PRINT_ALL, "lmmt" );
		} else if ( shader->optimalStageIteratorFunc == RB_StageIteratorVertexLitTexture ) {
			ri.Printf( PRINT_ALL, "vlt " );
		} else {
			ri.Printf( PRINT_ALL, "    " );
		}

		if ( shader->defaultShader ) {
			ri.Printf( PRINT_ALL,  ": %s (DEFAULTED)\n", shader->name );
		} else {
			ri.Printf( PRINT_ALL,  ": %s\n", shader->name );
		}
		count++;
	}
	ri.Printf( PRINT_ALL, "%i total shaders\n", count );
	ri.Printf( PRINT_ALL, "------------------\n" );
}

// Ridah, optimized shader loading

#define MAX_SHADER_STRING_POINTERS  100000
shaderStringPointer_t shaderStringPointerList[MAX_SHADER_STRING_POINTERS];

/*
====================
BuildShaderChecksumLookup
====================
*/
static void BuildShaderChecksumLookup( void ) {
	char *p = s_shaderText, *pOld;
	char *token;
	unsigned short int checksum;
	int numShaderStringPointers = 0;

	// initialize the checksums
	memset( shaderChecksumLookup, 0, sizeof( shaderChecksumLookup ) );

	if ( !p ) {
		return;
	}

	// loop for all labels
	while ( 1 ) {

		pOld = p;

		token = COM_ParseExt( &p, qtrue );
		if ( token[0] == 0 ) {
			break;
		}

		if ( !Q_stricmp( token, "{" ) ) {
			// skip braced section
			SkipBracedSection( &p );
			continue;
		}

		// get it's checksum
		checksum = generateHashValue( token );

		// if it's not currently used
		if ( !shaderChecksumLookup[checksum].pStr ) {
			shaderChecksumLookup[checksum].pStr = pOld;
		} else {
			// create a new list item
			shaderStringPointer_t *newStrPtr;

			if ( numShaderStringPointers >= MAX_SHADER_STRING_POINTERS ) {
				ri.Error( ERR_DROP, "MAX_SHADER_STRING_POINTERS exceeded, too many shaders" );
			}

			newStrPtr = &shaderStringPointerList[numShaderStringPointers++]; //ri.Hunk_Alloc( sizeof( shaderStringPointer_t ), h_low );
			newStrPtr->pStr = pOld;
			newStrPtr->next = shaderChecksumLookup[checksum].next;
			shaderChecksumLookup[checksum].next = newStrPtr;
		}
	}
}
// done.


/*
====================
ScanAndLoadShaderFiles

Finds and loads all .shader files, combining them into
a single large text block that can be scanned for shader names
=====================
*/
#define MAX_SHADER_FILES    4096
static void ScanAndLoadShaderFiles( void ) {
	char **shaderFiles;
	char *buffers[MAX_SHADER_FILES];
	char *p;
	int numShaders;
	int i;

	long sum = 0;
	// scan for shader files
	shaderFiles = ri.FS_ListFiles( "scripts", ".shader", &numShaders );

	if ( !shaderFiles || !numShaders ) {
		ri.Printf( PRINT_WARNING, "WARNING: no shader files found\n" );
		return;
	}

	if ( numShaders > MAX_SHADER_FILES ) {
		numShaders = MAX_SHADER_FILES;
	}

	// load and parse shader files
	for ( i = 0; i < numShaders; i++ )
	{
		char filename[MAX_QPATH];

		Com_sprintf( filename, sizeof( filename ), "scripts/%s", shaderFiles[i] );
		ri.Printf( PRINT_ALL, "...loading '%s'\n", filename );
		sum += ri.FS_ReadFile( filename, (void **)&buffers[i] );
		if ( !buffers[i] ) {
			ri.Error( ERR_DROP, "Couldn't load %s", filename );
		}
	}

	// build single large buffer
	s_shaderText = ri.Hunk_Alloc( sum + numShaders * 2, h_low );

	// free in reverse order, so the temp files are all dumped
	for ( i = numShaders - 1; i >= 0 ; i-- ) {
		strcat( s_shaderText, "\n" );
		p = &s_shaderText[strlen( s_shaderText )];
		strcat( s_shaderText, buffers[i] );
		ri.FS_FreeFile( buffers[i] );
		buffers[i] = p;
//		COM_Compress(p);
	}

	// free up memory
	ri.FS_FreeFileList( shaderFiles );

	// Ridah, optimized shader loading (18ms on a P3-500 for sfm1.bsp)
	BuildShaderChecksumLookup();
	// done.
}


/*
====================
CreateInternalShaders
====================
*/
static void CreateInternalShaders( void ) {
	tr.numShaders = 0;

	// init the default shader
	memset( &shader, 0, sizeof( shader ) );
	memset( &stages, 0, sizeof( stages ) );

	Q_strncpyz( shader.name, "<default>", sizeof( shader.name ) );

	shader.lightmapIndex = LIGHTMAP_NONE;
	stages[0].bundle[0].image[0] = tr.defaultImage;
	stages[0].active = qtrue;
	stages[0].stateBits = GLS_DEFAULT;
	tr.defaultShader = FinishShader();

	// shadow shader is just a marker
	Q_strncpyz( shader.name, "<stencil shadow>", sizeof( shader.name ) );
	shader.sort = SS_STENCIL_SHADOW;
	tr.shadowShader = FinishShader();
}

static void CreateExternalShaders( void ) {
//	tr.projectionShadowShader = R_FindShader( "projectionShadow", LIGHTMAP_NONE, qtrue );
	tr.flareShader = R_FindShader( "flareShader", LIGHTMAP_NONE, qtrue );
	tr.spotFlareShader = R_FindShader( "spotLight", LIGHTMAP_NONE, qtrue );
//	tr.sunShader = R_FindShader( "sun", LIGHTMAP_NONE, qtrue );	//----(SA)	let sky shader set this
	tr.sunflareShader[0] = R_FindShader( "sunflare1", LIGHTMAP_NONE, qtrue );
	tr.dlightShader = R_FindShader( "dlightshader", LIGHTMAP_NONE, qtrue );
}

//=============================================================================
// Ridah, shader caching
static int numBackupShaders = 0;
static shader_t *backupShaders[MAX_SHADERS];
static shader_t *backupHashTable[FILE_HASH_SIZE];

/*
===============
R_CacheShaderAlloc
===============
*/
void *R_CacheShaderAlloc( int size ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		//return malloc( size );
		return malloc( size );
	} else {
		return ri.Hunk_Alloc( size, h_low );
	}
}

/*
===============
R_CacheShaderFree
===============
*/
void R_CacheShaderFree( void *ptr ) {
	if ( r_cache->integer && r_cacheShaders->integer ) {
		//free( ptr );
		free( ptr );
	}
}

/*
===============
R_PurgeShaders
===============
*/
void R_PurgeShaders( int count ) {
	int i, j, c, b;
	shader_t **sh;
	static int lastPurged = 0;

	if ( !numBackupShaders ) {
		lastPurged = 0;
		return;
	}

	// find the first shader still in memory
	c = 0;
	sh = (shader_t **)&backupShaders;
	for ( i = lastPurged; i < numBackupShaders; i++, sh++ ) {
		if ( *sh ) {
			// free all memory associated with this shader
			for ( j = 0 ; j < ( *sh )->numUnfoggedPasses ; j++ ) {
				if ( !( *sh )->stages[j] ) {
					break;
				}
				for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
					if ( ( *sh )->stages[j]->bundle[b].texMods ) {
						R_CacheShaderFree( ( *sh )->stages[j]->bundle[b].texMods );
					}
				}
				R_CacheShaderFree( ( *sh )->stages[j] );
			}
			R_CacheShaderFree( *sh );
			*sh = NULL;

			if ( ++c >= count ) {
				lastPurged = i;
				return;
			}
		}
	}
	lastPurged = 0;
	numBackupShaders = 0;
}

/*
===============
R_BackupShaders
===============
*/
void R_BackupShaders( void ) {

	if ( !r_cache->integer ) {
		return;
	}
	if ( !r_cacheShaders->integer ) {
		return;
	}

	// copy each model in memory across to the backupModels
	memcpy( backupShaders, tr.shaders, sizeof( backupShaders ) );
	// now backup the hashTable
	memcpy( backupHashTable, hashTable, sizeof( hashTable ) );

	numBackupShaders = tr.numShaders;
}

/*
=================
R_RegisterShaderImages

  Make sure all images that belong to this shader remain valid
=================
*/
static qboolean R_RegisterShaderImages( shader_t *sh ) {
	int i,j,b;

	if ( sh->isSky ) {
		return qfalse;
	}

	for ( i = 0; i < sh->numUnfoggedPasses; i++ ) {
		if ( sh->stages[i] && sh->stages[i]->active ) {
			for ( b = 0 ; b < NUM_TEXTURE_BUNDLES ; b++ ) {
				for ( j = 0; sh->stages[i]->bundle[b].image[j] && j < MAX_IMAGE_ANIMATIONS; j++ ) {
					if ( !R_TouchImage( sh->stages[i]->bundle[b].image[j] ) ) {
						return qfalse;
					}
				}
			}
		}
	}
	return qtrue;
}

/*
===============
R_FindCachedShader

  look for the given shader in the list of backupShaders
===============
*/
shader_t *R_FindCachedShader( const char *name, int lightmapIndex, int hash ) {
	shader_t *sh, *shPrev;

	if ( !r_cacheShaders->integer ) {
		return NULL;
	}

	if ( !numBackupShaders ) {
		return NULL;
	}

	if ( !name ) {
		return NULL;
	}

	sh = backupHashTable[hash];
	shPrev = NULL;
	while ( sh ) {
		if ( sh->lightmapIndex == lightmapIndex && !Q_stricmp( sh->name, name ) ) {

			// make sure the images stay valid
			if ( !R_RegisterShaderImages( sh ) ) {
				return NULL;
			}

			// this is the one, so move this shader into the current list

			if ( !shPrev ) {
				backupHashTable[hash] = sh->next;
			} else {
				shPrev->next = sh->next;
			}

			sh->next = hashTable[hash];
			hashTable[hash] = sh;

			backupShaders[sh->index] = NULL;    // make sure we don't try and free it

			// set the index up, and add it to the current list
			tr.shaders[ tr.numShaders ] = sh;
			sh->index = tr.numShaders;

			tr.sortedShaders[ tr.numShaders ] = sh;
			sh->sortedIndex = tr.numShaders;

			tr.numShaders++;

			SortNewShader();    // make sure it renders in the right order

			return sh;
		}

		shPrev = sh;
		sh = sh->next;
	}

	return NULL;
}

/*
===============
R_LoadCacheShaders
===============
*/
void R_LoadCacheShaders( void ) {
	int len;
	byte *buf;
	char    *token, *pString;
	char name[MAX_QPATH];

	if ( !r_cacheShaders->integer ) {
		return;
	}

	// don't load the cache list in between level loads, only on startup, or after a vid_restart
	if ( numBackupShaders > 0 ) {
		return;
	}

	len = ri.FS_ReadFile( "shader.cache", NULL );

	if ( len <= 0 ) {
		return;
	}

	buf = (byte *)ri.Hunk_AllocateTempMemory( len );
	ri.FS_ReadFile( "shader.cache", (void **)&buf );
	pString = (char*)buf;   //DAJ added (char*)

	while ( ( token = COM_ParseExt( &pString, qtrue ) ) != NULL && token[0] ) {
		Q_strncpyz( name, token, sizeof( name ) );
		RE_RegisterModel( name );
	}

	ri.Hunk_FreeTempMemory( buf );
}
// done.
//=============================================================================

/*
==================
R_InitShaders
==================
*/
void R_InitShaders( void ) {

	glfogNum = FOG_NONE;
	ri.Cvar_Set( "r_waterFogColor", "0" );  // clear fog
	ri.Cvar_Set( "r_mapFogColor", "0" );        //
	ri.Cvar_Set( "r_savegameFogColor", "0" );

	ri.Printf( PRINT_ALL, "Initializing Shaders\n" );

	memset( hashTable, 0, sizeof( hashTable ) );
	deferLoad = qfalse;

	CreateInternalShaders();

	ScanAndLoadShaderFiles();

	CreateExternalShaders();

	// Ridah
	R_LoadCacheShaders();
}
