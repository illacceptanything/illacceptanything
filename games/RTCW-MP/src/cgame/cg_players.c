/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		cg_players.c
 *
 * desc:		handle the media and animation for player entities
 *
*/

static char text[100000];

#include "cg_local.h"

#define SWING_RIGHT 1
#define SWING_LEFT  2

char    *cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav",
	"*exert1.wav",
	"*exert2.wav",
	"*exert3.wav",
};


/*
================
CG_EntOnFire
================
*/
qboolean CG_EntOnFire( centity_t *cent ) {
	if ( cent->currentState.number == cg.snap->ps.clientNum ) {
		return  (   ( cg.snap->ps.onFireStart < cg.time ) &&
					( ( cg.snap->ps.onFireStart + 2000 ) > cg.time ) );
	} else {
		return  (   ( cent->currentState.onFireStart < cg.time ) &&
					( cent->currentState.onFireEnd > cg.time ) );
	}
}

/*
================
CG_IsCrouchingAnim
================
*/
qboolean CG_IsCrouchingAnim( clientInfo_t *ci, int animNum ) {
	animation_t *anim;

	// FIXME: make compatible with new scripting
	animNum &= ~ANIM_TOGGLEBIT;
	//
	anim = BG_GetAnimationForIndex( ci->clientNum, animNum );
	//
	if ( anim->movetype & ( ( 1 << ANIM_MT_IDLECR ) | ( 1 << ANIM_MT_WALKCR ) | ( 1 << ANIM_MT_WALKCRBK ) ) ) {
		return qtrue;
	}
	//
	return qfalse;
}

/*
================
CG_CustomSound

================
*/
sfxHandle_t CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int i;

	if ( soundName[0] != '*' ) {
		return trap_S_RegisterSound( soundName );
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i] ; i++ ) {
		if ( !strcmp( soundName, cg_customSoundNames[i] ) ) {
			return ci->sounds[i];
		}
	}

	CG_Error( "Unknown custom sound: %s", soundName );
	return 0;
}



/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
======================
CG_ParseGibModels

Read a configuration file containing gib models for use with this character
======================
*/
static qboolean CG_ParseGibModels( const char *filename, clientInfo_t *ci ) {
	char        *text_p;
	int len;
	int i;
	char        *token;
	fileHandle_t f;

	memset( ci->gibModels, 0, sizeof( ci->gibModels ) );

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	for ( i = 0; i < MAX_GIB_MODELS; i++ ) {
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}
		// cache this model
		ci->gibModels[i] = trap_R_RegisterModel( token );
	}

	return qtrue;
}


/*
==================
CG_CalcMoveSpeeds
==================
*/
void CG_CalcMoveSpeeds( clientInfo_t *ci ) {
	char *tags[2] = {"tag_footleft", "tag_footright"};
	vec3_t oldPos[2];
	refEntity_t refent;
	animation_t *anim;
	int i, j, k;
	float totalSpeed;
	int numSpeed;
	int lastLow, low;
	orientation_t o[2];

	refent.hModel = ci->legsModel;

	for ( i = 0, anim = ci->modelInfo->animations; i < ci->modelInfo->numAnimations; i++, anim++ ) {

		if ( anim->moveSpeed >= 0 ) {
			continue;
		}

		totalSpeed = 0;
		lastLow = -1;
		numSpeed = 0;

		// for each frame
		for ( j = 0; j < anim->numFrames; j++ ) {

			refent.frame = anim->firstFrame + j;
			refent.oldframe = refent.frame;

			// for each foot
			for ( k = 0; k < 2; k++ ) {
				if ( trap_R_LerpTag( &o[k], &refent, tags[k], 0 ) < 0 ) {
					CG_Error( "CG_CalcMoveSpeeds: unable to find tag %s, cannot calculate movespeed", tags[k] );
				}
			}

			// find the contact foot
			if ( anim->flags & ANIMFL_LADDERANIM ) {
				if ( o[0].origin[0] > o[1].origin[0] ) {
					low = 0;
				} else {
					low = 1;
				}
				totalSpeed += fabs( oldPos[low][2] - o[low].origin[2] );
			} else {
				if ( o[0].origin[2] < o[1].origin[2] ) {
					low = 0;
				} else {
					low = 1;
				}
				totalSpeed += fabs( oldPos[low][0] - o[low].origin[0] );
			}

			numSpeed++;

			// save the positions
			for ( k = 0; k < 2; k++ ) {
				VectorCopy( o[k].origin, oldPos[k] );
			}
			lastLow = low;
		}

		// record the speed
		anim->moveSpeed = (int)( ( totalSpeed / numSpeed ) * 1000.0 / anim->frameLerp );
	}

	if ( cgs.localServer ) {
		CG_SendMoveSpeed( ci->modelInfo->animations, ci->modelInfo->numAnimations, ci->modelInfo->modelname );
	}
}

/*
======================
CG_ParseAnimationFiles

  Read in all the configuration and script files for this model.
======================
*/
static qboolean CG_ParseAnimationFiles( const char *modelname, clientInfo_t *ci, int client ) {
	char filename[MAX_QPATH];
	fileHandle_t f;
	int len;

	// set the name of the model in the modelinfo structure
	Q_strncpyz( ci->modelInfo->modelname, modelname, sizeof( ci->modelInfo->modelname ) );

	// load the cfg file
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/wolfanim.cfg", modelname );
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	BG_AnimParseAnimConfig( ci->modelInfo, filename, text );

	if ( ci->isSkeletal != ci->modelInfo->isSkeletal ) {
		CG_Error( "Mis-match in %s, loaded skeletal model, but file does not specify SKELETAL\n", filename );
	}

	// calc movespeed values if required
	CG_CalcMoveSpeeds( ci );

	// load the script file
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/wolfanim.script", modelname );
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		if ( ci->modelInfo->version > 1 ) {
			return qfalse;
		}
		// try loading the default script for old legacy models
		Com_sprintf( filename, sizeof( filename ), "models/players/default.script", modelname );
		len = trap_FS_FOpenFile( filename, &f, FS_READ );
		if ( len <= 0 ) {
			return qfalse;
		}
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	BG_AnimParseAnimScript( ci->modelInfo, &cgs.animScriptData, ci->clientNum, filename, text );
	return qtrue;
}

/*
==========================
CG_RegisterClientSkin
==========================
*/

//----(SA) modified this for head separation

qboolean CG_RegisterClientSkin( clientInfo_t *ci, const char *modelName, const char *skinName ) {
	char filename[MAX_QPATH];

	// RF, try and register the new "body_*.skin" file for skeletal animation
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/body_%s.skin", modelName, skinName );
	ci->legsSkin = trap_R_RegisterSkin( filename );
	if ( ci->legsSkin ) { // skeletal model
		ci->torsoSkin = ci->legsSkin;
		return qtrue;
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower_%s.skin", modelName, skinName );
	ci->legsSkin = trap_R_RegisterSkin( filename );

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper_%s.skin", modelName, skinName );
	ci->torsoSkin = trap_R_RegisterSkin( filename );

	if ( !ci->legsSkin || !ci->torsoSkin ) {
		return qfalse;
	}

	return qtrue;
}

/*
==============
CG_RegisterClientHeadSkin
==============
*/
static qboolean CG_RegisterClientHeadSkin( clientInfo_t *ci, const char *modelName, const char *hSkinName ) {
	char filename[MAX_QPATH];

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/head_%s.skin", modelName, hSkinName );
	ci->headSkin = trap_R_RegisterSkin( filename );

	if ( !ci->headSkin ) {
		return qfalse;
	}

	return qtrue;

}

//----(SA) end



//----(SA)	added
/*
==============
CG_RegisterAcc
==============
*/
static qboolean CG_RegisterAcc( clientInfo_t *ci, const char *modelName, const char *skinName, int *model, int *skin ) {
	char namefromskin[MAX_QPATH];
	char filename[MAX_QPATH];

	if ( !model || !skin ) {
		return qfalse;
	}

	// FIXME: have the check the last 4 chars rather than strstr()
	if ( !strstr( skinName, ".md3" ) ) {          // try to find a skin in the acc folder that matches
		*skin = trap_R_RegisterSkin( va( "%s/%s.skin", modelName, skinName ) );

		if ( *skin ) {
			if ( trap_R_GetSkinModel( *skin, "md3_part", &namefromskin[0] ) ) {
				Com_sprintf( filename, sizeof( filename ), "%s/acc/%s", modelName, namefromskin );
				// NOTE: FIXME: this will currently only work with accessories in the <modelName>/acc directory.
				//				It will have to strip the directory off the end and then use the remaining
				//				path in order to work for arbitrary sub-directories.
//				Com_sprintf( filename, sizeof( filename ), "%s/%s/%s.md3", modelName, <accpath>, namefromskin );

			} else {
				Com_sprintf( filename, sizeof( filename ), "%s/%s.md3", modelName, skinName );
			}
		} else {
			Com_sprintf( filename, sizeof( filename ), "%s/%s.md3", modelName, skinName );
		}
	} else {                                    // the skin wants a straight model
		Com_sprintf( filename, sizeof( filename ), "%s/%s", modelName, skinName );
	}


	*model = trap_R_RegisterModel( filename );

	if ( *model ) {
		return qtrue;
	}

	return qfalse;
}

//----(SA)	end


/*
==================
CG_CheckForExistingModelInfo

  If this player model has already been parsed, then use the existing information.
  Otherwise, set the modelInfo pointer to the first free slot.

  returns qtrue if existing model found, qfalse otherwise
==================
*/
qboolean CG_CheckForExistingModelInfo( clientInfo_t *ci, char *modelName, animModelInfo_t **modelInfo ) {
	int i;
	animModelInfo_t *trav, *firstFree = NULL;
	clientInfo_t *ci_trav;
	char modelsUsed[MAX_ANIMSCRIPT_MODELS];

	for ( i = 0, trav = cgs.animScriptData.modelInfo; i < MAX_ANIMSCRIPT_MODELS; i++, trav++ ) {
		if ( trav->modelname[0] ) {
			if ( !Q_stricmp( trav->modelname, modelName ) ) {
				// found a match, use this modelinfo
				*modelInfo = trav;
				cgs.animScriptData.clientModels[ci->clientNum] = i + 1;
				return qtrue;
			}
		} else if ( !firstFree ) {
			firstFree = trav;
			cgs.animScriptData.clientModels[ci->clientNum] = i + 1;
		}
	}

	// set the modelInfo to the first free slot
	if ( !firstFree ) {
		// attempt to free a model that is no longer being used
		memset( modelsUsed, 0, sizeof( modelsUsed ) );
		for ( i = 0, ci_trav = cgs.clientinfo; i < MAX_CLIENTS; i++, ci_trav++ ) {
			if ( ci_trav->infoValid && ci_trav != ci ) {
				modelsUsed[ (int)( ci_trav->modelInfo - cgs.animScriptData.modelInfo ) ] = 1;
			}
		}
		// now use the first slot that isn't being utilized
		for ( i = 0, trav = cgs.animScriptData.modelInfo; i < MAX_ANIMSCRIPT_MODELS; i++, trav++ ) {
			if ( !modelsUsed[i] ) {
				firstFree = trav;
				cgs.animScriptData.clientModels[ci->clientNum] = i + 1;
				break;
			}
		}
	}

	if ( !firstFree ) {
		CG_Error( "unable to find a free modelinfo slot, cannot continue\n" );
	} else {
		*modelInfo = firstFree;
		// clear the structure out ready for use
		memset( *modelInfo, 0, sizeof( *modelInfo ) );
	}
	// qfalse signifies that we need to parse the information from the script files
	return qfalse;
}


/*
==========================
CG_RegisterClientModelname
==========================
*/

//----(SA) modified this for head separation

qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName ) {
	char namefromskin[MAX_QPATH];
	char filename[MAX_QPATH];

	// if any skins failed to load, return failure
//----(SA) modified this for head separation
	if ( !CG_RegisterClientSkin( ci, modelName, skinName ) ) {
		Com_Printf( "Failed to load skin file: %s/%s\n", modelName, skinName );
		return qfalse;
	}

	// load cmodels before models so filecache works

	if ( trap_R_GetSkinModel( ci->legsSkin, "md3_part", &namefromskin[0] ) ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s", modelName, namefromskin );
		ci->legsModel = trap_R_RegisterModel( filename );
	} else {    // try skeletal model
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/body.mds", modelName );
		ci->legsModel = trap_R_RegisterModel( filename );

		if ( !ci->legsModel ) {   // revert to mesh animation
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
			ci->legsModel = trap_R_RegisterModel( filename );
		} else {                // found skeletal model
			ci->isSkeletal = qtrue;
			ci->torsoModel = ci->legsModel;
		}
	}

	if ( !ci->isSkeletal ) {

		if ( !ci->legsModel ) {
			Com_Printf( "Failed to load legs model file %s\n", filename );
			return qfalse;
		}

		if ( trap_R_GetSkinModel( ci->torsoSkin, "md3_part", &namefromskin[0] ) ) {
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s", modelName, namefromskin );
		} else {
			Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
		}

		ci->torsoModel = trap_R_RegisterModel( filename );

		if ( !ci->torsoModel ) {
			Com_Printf( "Failed to load torso model file %s\n", filename );
			return qfalse;
		}

	}

//----(SA)	testing
	{
		char scaleString[MAX_QPATH];
		char    *string_p;
		char    *scaleToken;
		qboolean badscale = qfalse;

		string_p = scaleString;

		if ( trap_R_GetSkinModel( ci->legsSkin, "playerscale", &scaleString[0] ) ) {
			scaleToken = COM_Parse( &string_p );
			if ( !scaleToken ) {
				badscale = qtrue;   // and drop to "if(badscale)" below
			} else
			{
				ci->playermodelScale[0] = atof( scaleToken );

				scaleToken = COM_Parse( &string_p );
				if ( !scaleToken ) {
					badscale = qtrue;   // and drop to "if(badscale)" below
				} else
				{
					ci->playermodelScale[1] = atof( scaleToken );

					scaleToken = COM_Parse( &string_p );
					if ( !scaleToken ) {
						badscale = qtrue;   // and drop to "if(badscale)" below
					} else
					{
						ci->playermodelScale[2] = atof( scaleToken );
					}
				}
			}
		}

		if ( badscale ) {
			ci->playermodelScale[0] =
				ci->playermodelScale[1] =
					ci->playermodelScale[2] = 0.0f;
		}
	}
//----(SA)	end


	// try all the accessories
	if ( trap_R_GetSkinModel( ci->legsSkin, "md3_beltr", &namefromskin[0] ) ) {
		CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_BELT_LEFT], &ci->accSkins[ACC_BELT_LEFT] );
	}
	if ( trap_R_GetSkinModel( ci->legsSkin, "md3_beltl", &namefromskin[0] ) ) {
		CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_BELT_RIGHT], &ci->accSkins[ACC_BELT_RIGHT] );
	}
	if ( trap_R_GetSkinModel( ci->torsoSkin, "md3_belt", &namefromskin[0] ) ) {
		CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_BELT], &ci->accSkins[ACC_BELT] );
	}
	if ( trap_R_GetSkinModel( ci->torsoSkin, "md3_back", &namefromskin[0] ) ) {
		CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_BACK], &ci->accSkins[ACC_BACK] );
	}
//----(SA)	added
	if ( trap_R_GetSkinModel( ci->torsoSkin, "md3_weapon", &namefromskin[0] ) ) {
		CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_WEAPON], &ci->accSkins[ACC_WEAPON] );
	}
	if ( trap_R_GetSkinModel( ci->torsoSkin, "md3_weapon2", &namefromskin[0] ) ) {
		CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_WEAPON2], &ci->accSkins[ACC_WEAPON2] );
	}
//----(SA)	end

	// look for this model in the list of models already opened
	if ( !CG_CheckForExistingModelInfo( ci, (char *)modelName, &ci->modelInfo ) ) {

		if ( !CG_ParseAnimationFiles( modelName, ci, ci->clientNum ) ) {
			Com_Printf( "Failed to load animation file %s\n", filename );
			return qfalse;
		}
	}

	return qtrue;
}

/*
==============
CG_RegisterClientHeadname
==============
*/
static qboolean CG_RegisterClientHeadname( clientInfo_t *ci, const char *modelName, const char *hSkinName ) {
	char namefromskin[MAX_QPATH];
	char filename[MAX_QPATH];
	int i;

	if ( !CG_RegisterClientHeadSkin( ci, modelName, hSkinName ) ) {
		Com_Printf( "Failed to load head skin file: %s/head_%s.skin\n", modelName, hSkinName );   //----(SA)
		return qfalse;
	}

	if ( trap_R_GetSkinModel( ci->headSkin, "md3_part", &namefromskin[0] ) ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/%s", modelName, namefromskin );
	} else {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", modelName );
	}

	ci->headModel = trap_R_RegisterModel( filename );
	if ( !ci->headModel ) {
		Com_Printf( "Failed to load head model file %s\n", filename );    //----(SA)
		return qfalse;
	}

	if ( trap_R_GetSkinModel( ci->headSkin, "md3_hat", &namefromskin[0] ) ) {
		CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_HAT], &ci->accSkins[ACC_HAT] );
	}

	for ( i = 0; i < ACC_NUM_MOUTH - 1; i++ ) {
		if ( trap_R_GetSkinModel( ci->headSkin, va( "md3_hat%d", 2 + i ), &namefromskin[0] ) ) {
			CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_MOUTH2 + i], &ci->accSkins[ACC_MOUTH2 + i] );
		}
	}

	return qtrue;
}
/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char *v, vec3_t color ) {
	int val;

	VectorClear( color );

	val = atoi( v );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo( clientInfo_t *ci ) {
	const char  *dir, *fallback;
	int i;
	const char  *s;
	int clientNum;
	int headfail = 0;
	char filename[MAX_QPATH];

//----(SA) modified this for head separation

	// load the head first (since if the head loads but there is a problem with something in the lower
	// body, you will want to default the model back to a default and want the head to match)
	//

	if ( !CG_RegisterClientHeadname( ci, ci->modelName, ci->hSkinName ) ) {
		if ( cg_buildScript.integer ) {
			CG_Error( "CG_RegisterClientHeadname( %s, %s ) failed.  setting default", ci->modelName, ci->hSkinName );
		}

		// fall back to default head
		if ( !CG_RegisterClientHeadname( ci, ci->modelName, "default" ) ) {
			headfail = 1;
			if ( cg_buildScript.integer ) {
				CG_Error( "head model/skin (%s/default) failed to register", ci->modelName );    //----(SA)
			}
		}
	}

	if ( headfail || !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName ) ) {
		if ( cg_buildScript.integer ) {
			CG_Error( "CG_RegisterClientModelname( %s, %s ) failed", ci->modelName, ci->skinName );
		}

		// fall back
		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name but set default model
			if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, ci->skinName ) ) {
				CG_Error( "DEFAULT_MODEL / skin (%s/%s) failed to register", DEFAULT_MODEL, ci->skinName );
			}
		} else if ( cgs.gametype == GT_SINGLE_PLAYER && !headfail ) {
			// try to keep the model but default the skin (so you can tell bad guys from good)
			if ( !CG_RegisterClientModelname( ci, ci->modelName, "default" ) ) {
				CG_Error( "DEFAULT_MODEL (%s/default) failed to register", ci->modelName );
			}
		} else {
			// go totally default
			if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, "default" ) ) {
				CG_Error( "DEFAULT_MODEL (%s/default) failed to register", DEFAULT_MODEL );
			}

			// fall back to default head
			if ( !CG_RegisterClientHeadname( ci, DEFAULT_MODEL, "default" ) ) {
				CG_Error( "model/ DEFAULT_HEAD / skin (%s/default) failed to register", DEFAULT_HEAD );
			}

		}

	}

//----(SA) end

	// sounds
	dir = ci->modelName;
	fallback = DEFAULT_MODEL;

	// DHM - Nerve :: Not using in multiplayer, was causing hitches on player respawns/joins
	if ( cgs.gametype < GT_WOLF ) {
		for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ ) {
			s = cg_customSoundNames[i];
			if ( !s ) {
				break;
			}
			ci->sounds[i] = trap_S_RegisterSound( va( "sound/player/%s/%s", dir, s + 1 ) );
			if ( !ci->sounds[i] ) {
				ci->sounds[i] = trap_S_RegisterSound( va( "sound/player/%s/%s", fallback, s + 1 ) );
			}
		}
	}
	// dhm

	// load the gibs
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/gibs.cfg", dir );
	if ( !CG_ParseGibModels( filename, ci ) ) {
		// n/mind.. gib code will automatically fall back to old gibs
	}

	ci->deferred = qfalse;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	clientNum = ci - cgs.clientinfo;
	for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
		if ( cg_entities[i].currentState.clientNum == clientNum
			 && cg_entities[i].currentState.eType == ET_PLAYER ) {
			CG_ResetPlayerEntity( &cg_entities[i] );
		}
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to ) {
	VectorCopy( from->playermodelScale, to->playermodelScale );

	to->legsModel   = from->legsModel;
	to->legsSkin    = from->legsSkin;
	to->torsoModel  = from->torsoModel;
	to->torsoSkin   = from->torsoSkin;
	to->headModel   = from->headModel;
	to->headSkin    = from->headSkin;
	to->isSkeletal  = from->isSkeletal;
	to->modelIcon   = from->modelIcon;

//----(SA)
	memcpy( to->accModels,  from->accModels,    sizeof( to->accModels   ) );
	memcpy( to->accSkins,   from->accSkins,     sizeof( to->accSkins    ) );
	memcpy( to->partModels, from->partModels,   sizeof( to->partModels  ) );
//	memcpy( to->partSkins,	from->partSkins,	sizeof( to->partSkins	) );
//----(SA)	end

	memcpy( to->sounds,     from->sounds,       sizeof( to->sounds ) );

	// Ridah
	memcpy( to->gibModels, from->gibModels, sizeof( to->gibModels ) );
	// done.

	// copy the modelInfo
	to->modelInfo = from->modelInfo;
	cgs.animScriptData.clientModels[ to->clientNum ] = cgs.animScriptData.clientModels[ from->clientNum ];

}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci ) {
	int i;
	clientInfo_t    *match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
//----(SA) added checks for same head.  FIXME: soon this will be more efficient than just looking for a complete character (body/head) match.
		if (
			!Q_stricmp( ci->modelName,  match->modelName )
			&&  !Q_stricmp( ci->skinName,   match->skinName )
			&&  !Q_stricmp( ci->hSkinName,  match->hSkinName )
			) {

//----(SA) done

			// this clientinfo is identical, so use it's handles

			ci->deferred = qfalse;

			CG_CopyClientInfoModel( match, ci );

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo( clientInfo_t *ci ) {
	int i;
	clientInfo_t    *match;

	// if we are in teamplay, only grab a model if the skin is correct
	if ( cgs.gametype >= GT_TEAM ) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid ) {
				continue;
			}
			if ( Q_stricmp( ci->skinName, match->skinName ) ) {
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}

		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( ci );
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	CG_Printf( "CG_SetDeferredClientInfo: no valid clients!\n" );

	CG_LoadClientInfo( ci );
}


/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo( int clientNum ) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char  *configstring;
	const char  *v;
	char        *slash;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) {
		memset( ci, 0, sizeof( *ci ) );
		return;     // player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );

	newInfo.clientNum = clientNum;

	// isolate the player's name
	v = Info_ValueForKey( configstring, "n" );
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );

	// colors
	v = Info_ValueForKey( configstring, "c1" );
	CG_ColorFromString( v, newInfo.color );

	// bot skill
	v = Info_ValueForKey( configstring, "skill" );
	newInfo.botSkill = atoi( v );

	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	newInfo.handicap = atoi( v );

	// wins
	v = Info_ValueForKey( configstring, "w" );
	newInfo.wins = atoi( v );

	// losses
	v = Info_ValueForKey( configstring, "l" );
	newInfo.losses = atoi( v );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );

//----(SA) modified this for head separation

	// head
	v = Info_ValueForKey( configstring, "head" );
	if ( cg_forceModel.integer ) {
		char modelStr[MAX_QPATH];

		// forcemodel makes everyone use a single model
		// to prevent load hitches

		trap_Cvar_VariableStringBuffer( "head", modelStr, sizeof( modelStr ) );
		Q_strncpyz( newInfo.hSkinName, modelStr, sizeof( newInfo.hSkinName ) );
	} else {
		Q_strncpyz( newInfo.hSkinName, v, sizeof( newInfo.hSkinName ) );
	}

//----(SA) modified this for head separation

	// model
	v = Info_ValueForKey( configstring, "model" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
		if ( ( skin = strchr( modelStr, '/' ) ) == NULL ) {
			skin = "default";
		} else {
			*skin++ = 0;
		}

		Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
		Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );

		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			}
		}
	} else {
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

		slash = strchr( newInfo.modelName, '/' );
		if ( !slash ) {
			// modelName did not include a skin name
			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
		} else {
			Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			// truncate modelName
			*slash = 0;
		}
	}

	//----(SA) modify \/ to differentiate for head models/skins as well


	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo ) ) {
		qboolean forceDefer;

		forceDefer = trap_MemoryRemaining() < 4000000;

		if ( cgs.gametype != GT_SINGLE_PLAYER && ( forceDefer || ( cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading ) ) ) {
			// keep whatever they had if it won't violate team skins
			if ( ci->infoValid &&
				 ( cgs.gametype < GT_TEAM || !Q_stricmp( newInfo.skinName, ci->skinName ) ) ) {
				CG_CopyClientInfoModel( ci, &newInfo );
				newInfo.deferred = qtrue;
			} else {
				// use whatever is available
				CG_SetDeferredClientInfo( &newInfo );
			}
			// if we are low on memory, leave them with this model
			if ( forceDefer ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				newInfo.deferred = qfalse;
			}
		} else {
			CG_LoadClientInfo( &newInfo );
		}
	}

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;
}



/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void ) {
	int i;
	clientInfo_t    *ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			// if we are low on memory, leave it deferred
			if ( trap_MemoryRemaining() < 4000000 ) {
				CG_Printf( "Memory is low.  Using deferred model.\n" );
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo( ci );
		}
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/


/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t *anim;

	if ( !ci->modelInfo ) {
		return;
	}

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= ci->modelInfo->numAnimations ) {
		CG_Error( "Bad animation number (CG_SLFA): %i", newAnimation );
	}

	anim = &ci->modelInfo->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer == 1 ) {              // DHM - Nerve :: extra debug info
		CG_Printf( "Anim: %i, %s\n", newAnimation, ci->modelInfo->animations[newAnimation].name );
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
void CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale ) {
	int f;
	animation_t *anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( ci && ( newAnimation != lf->animationNumber || !lf->animation ) ) {  //----(SA)	modified
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;     // shouldn't happen
		}
		if ( cg.time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;      // initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale;        // adjust for haste, etc
		if ( f >= anim->numFrames ) {
			f -= anim->numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame = anim->firstFrame + f;
		if ( cg.time > lf->frameTime ) {
			lf->frameTime = cg.time;
			if ( cg_debugAnim.integer ) {
				CG_Printf( "Clamp lf->frameTime\n" );
			}
		}
	}

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}


/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber );
	if ( lf->animation ) {
		lf->oldFrame = lf->frame = lf->animation->firstFrame;
	}
}

//------------------------------------------------------------------------------
// Ridah, variable speed animations
/*
===============
CG_SetLerpFrameAnimationRate

may include ANIM_TOGGLEBIT
===============
*/
void CG_SetLerpFrameAnimationRate( centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t *anim, *oldanim;
	int transitionMin = -1, oldAnimTime, oldAnimNum;
	qboolean firstAnim = qfalse;

	if ( !ci->modelInfo ) {
		return;
	}

	oldAnimTime = lf->animationTime;
	oldanim = lf->animation;
	oldAnimNum = lf->animationNumber;

	if ( !lf->animation ) {
		firstAnim = qtrue;
	}

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= ci->modelInfo->numAnimations ) {
		CG_Error( "Bad animation number (CG_SLFAR): %i", newAnimation );
	}

	anim = &ci->modelInfo->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if ( !( anim->flags & ANIMFL_FIRINGANIM ) || ( lf != &cent->pe.torso ) ) {
		if ( ( lf == &cent->pe.legs ) && ( CG_IsCrouchingAnim( ci, newAnimation ) != CG_IsCrouchingAnim( ci, oldAnimNum ) ) ) {
			if ( anim->moveSpeed || ( anim->movetype & ( ( 1 << ANIM_MT_TURNLEFT ) | ( 1 << ANIM_MT_TURNRIGHT ) ) ) ) { // if unknown movetype, go there faster
				transitionMin = lf->frameTime + 200;    // slowly raise/drop
			} else {
				transitionMin = lf->frameTime + 350;    // slowly raise/drop
			}
		} else if ( anim->moveSpeed ) {
			transitionMin = lf->frameTime + 120;    // always do some lerping (?)
		} else { // not moving, so take your time
			transitionMin = lf->frameTime + 170;    // always do some lerping (?)

		}
		if ( oldanim && oldanim->animBlend ) { //transitionMin < lf->frameTime + oldanim->animBlend) {
			transitionMin = lf->frameTime + oldanim->animBlend;
			lf->animationTime = transitionMin;
		} else {
			// slow down transitions according to speed
			if ( anim->moveSpeed && lf->animSpeedScale < 1.0 ) {
				lf->animationTime += anim->initialLerp;
			}

			if ( lf->animationTime < transitionMin ) {
				lf->animationTime = transitionMin;
			}
		}
	}

	// if first anim, go immediately
	if ( firstAnim ) {
		lf->frameTime = cg.time - 1;
		lf->animationTime = cg.time - 1;
		lf->frame = anim->firstFrame;
	}

	if ( cg_debugAnim.integer == 1 ) {           // DHM - Nerve :: extra debug info
		CG_Printf( "Anim: %i, %s\n", newAnimation, ci->modelInfo->animations[newAnimation].name );
	}
}

/*
===============
CG_RunLerpFrameRate

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
void CG_RunLerpFrameRate( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, centity_t *cent, int recursion ) {
	int f;
	animation_t *anim, *oldAnim;
	animation_t *otherAnim = NULL;
	qboolean isLadderAnim;

#define ANIM_SCALEMAX_LOW   1.1
#define ANIM_SCALEMAX_HIGH  1.6

#define ANIM_SPEEDMAX_LOW   100
#define ANIM_SPEEDMAX_HIGH  20

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	isLadderAnim = lf->animation && ( lf->animation->flags & ANIMFL_LADDERANIM );

	oldAnim = lf->animation;

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		CG_SetLerpFrameAnimationRate( cent, ci, lf, newAnimation );
	}

	// Ridah, make sure the animation speed is updated when possible
	anim = lf->animation;
	if ( anim->moveSpeed && lf->oldFrameSnapshotTime ) {
		float moveSpeed;

		// calculate the speed at which we moved over the last frame
		if ( cg.latestSnapshotTime != lf->oldFrameSnapshotTime && cg.nextSnap ) {
			if ( cent->currentState.number == cg.snap->ps.clientNum ) {
				if ( isLadderAnim ) { // only use Z axis for speed
					if ( cent->currentState.aiChar != AICHAR_FEMZOMBIE ) {    // femzombie has sideways climbing
						lf->oldFramePos[0] = cent->lerpOrigin[0];
						lf->oldFramePos[1] = cent->lerpOrigin[1];
					}
				} else {    // only use x/y axis
					lf->oldFramePos[2] = cent->lerpOrigin[2];
				}
				moveSpeed = Distance( cent->lerpOrigin, lf->oldFramePos ) / ( (float)( cg.time - lf->oldFrameTime ) / 1000.0 );
			} else {
				if ( isLadderAnim ) { // only use Z axis for speed
					lf->oldFramePos[0] = cent->currentState.pos.trBase[0];
					lf->oldFramePos[1] = cent->currentState.pos.trBase[1];
				}

				if ( cgs.gametype == GT_SINGLE_PLAYER ) {
					moveSpeed = Distance( cent->currentState.pos.trBase, cent->nextState.pos.trBase ) / ( (float)( cg.nextSnap->serverTime - cg.snap->serverTime ) / 1000.0 );
				} else {
					moveSpeed = Distance( cent->lerpOrigin, lf->oldFramePos ) / ( (float)( cg.time - lf->oldFrameTime ) / 1000.0 );
				}
			}
			//
			// convert it to a factor of this animation's movespeed
			lf->animSpeedScale = moveSpeed / (float)anim->moveSpeed;
			lf->oldFrameSnapshotTime = cg.latestSnapshotTime;
		}
	} else {
		// move at normal speed
		lf->animSpeedScale = 1.0;
		lf->oldFrameSnapshotTime = cg.latestSnapshotTime;
	}
	// adjust with manual setting (pain anims)
	lf->animSpeedScale *= cent->pe.animSpeed;

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {

		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;
		VectorCopy( cent->lerpOrigin, lf->oldFramePos );

		// restrict the speed range
		if ( lf->animSpeedScale < 0.25 ) {    // if it's too slow, then a really slow spped, combined with a sudden take-off, can leave them playing a really slow frame while they a moving really fast
			if ( lf->animSpeedScale < 0.01 && isLadderAnim ) {
				lf->animSpeedScale = 0.0;
			} else {
				lf->animSpeedScale = 0.25;
			}
		} else if ( lf->animSpeedScale > ANIM_SCALEMAX_LOW ) {

			if ( !( anim->flags & ANIMFL_LADDERANIM ) ) {
				// allow slower anims to speed up more than faster anims
				if ( anim->moveSpeed > ANIM_SPEEDMAX_LOW ) {
					lf->animSpeedScale = ANIM_SCALEMAX_LOW;
				} else if ( anim->moveSpeed < ANIM_SPEEDMAX_HIGH ) {
					if ( lf->animSpeedScale > ANIM_SCALEMAX_HIGH ) {
						lf->animSpeedScale = ANIM_SCALEMAX_HIGH;
					}
				} else {
					lf->animSpeedScale = ANIM_SCALEMAX_HIGH - ( ANIM_SCALEMAX_HIGH - ANIM_SCALEMAX_LOW ) * (float)( anim->moveSpeed - ANIM_SPEEDMAX_HIGH ) / (float)( ANIM_SPEEDMAX_LOW - ANIM_SPEEDMAX_HIGH );
				}
			} else if ( lf->animSpeedScale > 4.0 ) {
				lf->animSpeedScale = 4.0;
			}

		}

		if ( lf == &cent->pe.legs ) {
			otherAnim = cent->pe.torso.animation;
		} else if ( lf == &cent->pe.torso ) {
			otherAnim = cent->pe.legs.animation;
		}

		// get the next frame based on the animation
		if ( !lf->animSpeedScale ) {
			// stopped on the ladder, so stay on the same frame
			f = lf->frame - anim->firstFrame;
			lf->frameTime += anim->frameLerp;       // don't wait too long before starting to move again
		} else if ( lf->oldAnimationNumber != lf->animationNumber &&
					( !anim->moveSpeed || lf->oldFrame < anim->firstFrame || lf->oldFrame >= anim->firstFrame + anim->numFrames ) ) { // Ridah, added this so walking frames don't always get reset to 0, which can happen in the middle of a walking anim, which looks wierd
			lf->frameTime = lf->animationTime;      // initial lerp
			if ( oldAnim && anim->moveSpeed ) {   // keep locomotions going continuously
				f = ( lf->frame - oldAnim->firstFrame ) + 1;
				while ( f < 0 ) {
					f += anim->numFrames;
				}
			} else {
				f = 0;
			}
		} else if ( ( lf == &cent->pe.legs ) && otherAnim && !( anim->flags & ANIMFL_FIRINGANIM ) && ( ( lf->animationNumber & ~ANIM_TOGGLEBIT ) == ( cent->pe.torso.animationNumber & ~ANIM_TOGGLEBIT ) ) && ( !anim->moveSpeed ) ) {
			// legs should synch with torso
			f = cent->pe.torso.frame - otherAnim->firstFrame;
			if ( f >= anim->numFrames || f < 0 ) {
				f = 0;  // wait at the start for the legs to catch up (assuming they are still in an old anim)
			}
			lf->frameTime = cent->pe.torso.frameTime;
		} else if ( ( lf == &cent->pe.torso ) && otherAnim && !( anim->flags & ANIMFL_FIRINGANIM ) && ( ( lf->animationNumber & ~ANIM_TOGGLEBIT ) == ( cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT ) ) && ( otherAnim->moveSpeed ) ) {
			// torso needs to sync with legs
			f = cent->pe.legs.frame - otherAnim->firstFrame;
			if ( f >= anim->numFrames || f < 0 ) {
				f = 0;  // wait at the start for the legs to catch up (assuming they are still in an old anim)
			}
			lf->frameTime = cent->pe.legs.frameTime;
		} else {
			lf->frameTime = lf->oldFrameTime + (int)( (float)anim->frameLerp * ( 1.0 / lf->animSpeedScale ) );
			if ( lf->frameTime < cg.time ) {
				lf->frameTime = cg.time;
			}

			// check for skipping frames (eg. death anims play in slo-mo if low framerate)
			if ( cg.time > lf->frameTime && !anim->moveSpeed ) {
				f = ( lf->frame - anim->firstFrame ) + 1 + ( cg.time - lf->frameTime ) / anim->frameLerp;
			} else {
				f = ( lf->frame - anim->firstFrame ) + 1;
			}

			if ( f < 0 ) {
				f = 0;
			}
		}
		//f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		if ( f >= anim->numFrames ) {
			f -= anim->numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame = anim->firstFrame + f;
		if ( cg.time > lf->frameTime ) {

			// Ridah, run the frame again until we move ahead of the current time, fixes walking speeds for zombie
			if ( /*!anim->moveSpeed ||*/ recursion > 4 ) {
				lf->frameTime = cg.time;
			} else {
				CG_RunLerpFrameRate( ci, lf, newAnimation, cent, recursion + 1 );
			}

			if ( cg_debugAnim.integer > 3 ) {
				CG_Printf( "Clamp lf->frameTime\n" );
			}
		}
		lf->oldAnimationNumber = lf->animationNumber;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}

/*
===============
CG_ClearLerpFrameRate
===============
*/
void CG_ClearLerpFrameRate( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber, centity_t *cent ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimationRate( cent, ci, lf, animationNumber );
	if ( lf->animation ) {
		lf->oldFrame = lf->frame = lf->animation->firstFrame;
	}
}

// done.
//------------------------------------------------------------------------------



/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
								int *torsoOld, int *torso, float *torsoBackLerp ) {
	clientInfo_t    *ci;
	int clientNum;
	int animIndex, tempIndex;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// default to whatever the legs are currently doing
	animIndex = cent->currentState.legsAnim;

	// do the shuffle turn frames locally
	if ( !( cent->currentState.eFlags & EF_DEAD ) && cent->pe.legs.yawing ) {
//CG_Printf("turn: %i\n", cg.time );
		tempIndex = BG_GetAnimScriptAnimation( clientNum, cent->currentState.aiState & ~( 1 << 2 ), ( cent->pe.legs.yawing == SWING_RIGHT ? ANIM_MT_TURNRIGHT : ANIM_MT_TURNLEFT ) );
		if ( tempIndex > -1 ) {
			animIndex = tempIndex;
		}
	}
	// run the animation
	CG_RunLerpFrameRate( ci, &cent->pe.legs, animIndex, cent, 0 );

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrameRate( ci, &cent->pe.torso, cent->currentState.torsoAnim, cent, 0 );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
							float speed, float *angle, qboolean *swinging ) {
	float swing;
	float move;
	float scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );
	scale *= 0.05;
	if ( scale < 0.5 ) {
		scale = 0.5;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = cg.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		} else {
			*swinging = SWING_LEFT;     // left
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = cg.frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		} else {
			*swinging = SWING_RIGHT;    // right
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - ( clampTolerance - 1 ) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + ( clampTolerance - 1 ) );
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch( centity_t *cent, vec3_t torsoAngles ) {
	int t;
	float f;
	int duration;
	float direction;

	if ( !cent->pe.animSpeed ) {
		// we need to inititialize this stuff
		cent->pe.painAnimLegs = -1;
		cent->pe.painAnimTorso = -1;
		cent->pe.animSpeed = 1.0;
	}

	if ( cent->currentState.eFlags & EF_DEAD ) {
		cent->pe.painAnimLegs = -1;
		cent->pe.painAnimTorso = -1;
		cent->pe.animSpeed = 1.0;
		return;
	}

	// special pain anims for AI
	if ( cent->currentState.aiChar ) {
		if ( cent->pe.painAnimTorso >= 0 ) {
			animation_t *anim;
			clientInfo_t *ci;

			ci = &cgs.clientinfo[ cent->currentState.number ];
			anim = &ci->modelInfo->animations[ cent->pe.painAnimTorso ];

			// play the current animation
			if ( cent->pe.torso.frame != cent->pe.torso.oldFrame || cent->pe.torso.frame != anim->firstFrame + anim->numFrames - 1 ) {
				if ( cent->pe.painAnimLegs >= 0 ) {
					cent->currentState.legsAnim = cent->pe.painAnimLegs;
				}
				cent->currentState.torsoAnim = cent->pe.painAnimTorso;
			} else {    // end it
				cent->pe.painAnimLegs = -1;
				cent->pe.painAnimTorso = -1;
				cent->pe.animSpeed = 1.0;
			}
		}

		return;
	}

	if ( cent->pe.painDuration ) {
		duration = cent->pe.painDuration;
	} else {
		duration = PAIN_TWITCH_TIME;
	}
	direction = (float)duration * 0.085;
	if ( direction > 30 ) {
		direction = 30;
	}
	if ( direction < 10 ) {
		direction = 10;
	}
	direction *= (float)( cent->pe.painDirection * 2 ) - 1;

	t = cg.time - cent->pe.painTime;
	if ( t >= duration ) {
		return;
	}

	if ( cent->currentState.clientNum && cgs.gametype == GT_SINGLE_PLAYER ) {
		#define FADEIN_RATIO    0.25
		#define FADEOUT_RATIO   ( 1.0 - FADEIN_RATIO )
		f = (float)t / duration;
		if ( f < FADEIN_RATIO ) {
			torsoAngles[ROLL] += ( 0.5 * direction * ( f * ( 1.0 / FADEIN_RATIO ) ) );
			torsoAngles[PITCH] -= ( fabs( direction ) * ( f * ( 1.0 / FADEIN_RATIO ) ) );
			torsoAngles[YAW] += ( direction * ( f * ( 1.0 / FADEIN_RATIO ) ) );
		} else {
			torsoAngles[ROLL] += ( 0.5 * direction * ( 1.0 - ( f - FADEIN_RATIO ) ) * ( 1.0 / FADEOUT_RATIO ) );
			torsoAngles[PITCH] -= ( fabs( direction ) * ( 1.0 - ( f - FADEIN_RATIO ) ) * ( 1.0 / FADEOUT_RATIO ) );
			torsoAngles[YAW] += ( direction * ( 1.0 - ( f - FADEIN_RATIO ) ) * ( 1.0 / FADEOUT_RATIO ) );
		}
	} else {    // fast, Q3 style
		f = 1.0 - (float)t / duration;
		if ( cent->pe.painDirection ) {
			torsoAngles[ROLL] += 20 * f;
		} else {
			torsoAngles[ROLL] -= 20 * f;
		}
	}
}

/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] ) {
	vec3_t legsAngles, torsoAngles, headAngles;
	float dest;
//	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 }; // TTimo: unused
	vec3_t velocity;
	float speed;
	float clampTolerance;
	int legsSet, torsoSet;
	clientInfo_t *ci;
	ci = &cgs.clientinfo[ cent->currentState.number ];

	// special case (female zombie while climbing wall)
	if ( cent->currentState.eFlags & EF_FORCED_ANGLES ) {
		// torso & legs parts should turn to face the given angles
		VectorCopy( cent->lerpAngles, legsAngles );
		AnglesToAxis( legsAngles, legs );
		AnglesToAxis( vec3_origin, torso );
		AnglesToAxis( vec3_origin, head );
		return;
	}

	legsSet = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	torsoSet = cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT;

	VectorCopy( cent->lerpAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit, unless these conditions don't allow them
	if (    !( BG_GetConditionValue( cent->currentState.number, ANIM_COND_MOVETYPE, qfalse ) & ( ( 1 << ANIM_MT_IDLE ) | ( 1 << ANIM_MT_IDLECR ) ) )/*
		||	 (BG_GetConditionValue( cent->currentState.number, ANIM_COND_MOVETYPE, qfalse ) & ((1<<ANIM_MT_STRAFELEFT) | (1<<ANIM_MT_STRAFERIGHT)) )*/) {

		// always point all in the same direction
		cent->pe.torso.yawing = qtrue;  // always center
		cent->pe.torso.pitching = qtrue;    // always center
		cent->pe.legs.yawing = qtrue;   // always center

		// if firing, make sure torso and head are always aligned
	} else if ( BG_GetConditionValue( cent->currentState.number, ANIM_COND_FIRING, qtrue ) ) {

		cent->pe.torso.yawing = qtrue;  // always center
		cent->pe.torso.pitching = qtrue;    // always center

	}

	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		legsAngles[YAW] = headAngles[YAW];
		torsoAngles[YAW] = headAngles[YAW];
	} else {
		legsAngles[YAW] = headAngles[YAW] + cent->currentState.angles2[YAW];

		if ( cent->currentState.eFlags & EF_NOSWINGANGLES ) {
			legsAngles[YAW] = torsoAngles[YAW] = headAngles[YAW];   // always face firing direction
			clampTolerance = 60;
		} else if ( !( cent->currentState.eFlags & EF_FIRING ) ) {
			torsoAngles[YAW] = headAngles[YAW] + 0.35 * cent->currentState.angles2[YAW];
			clampTolerance = 90;
		} else {    // must be firing
			torsoAngles[YAW] = headAngles[YAW]; // always face firing direction
			//if (fabs(cent->currentState.angles2[YAW]) > 30)
			//	legsAngles[YAW] = headAngles[YAW];
			clampTolerance = 60;
		}

		// torso
		CG_SwingAngles( torsoAngles[YAW], 25, clampTolerance, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );

		// if the legs are yawing (facing heading direction), allow them to rotate a bit, so we don't keep calling
		// the legs_turn animation while an AI is firing, and therefore his angles will be randomizing according to their accuracy

		clampTolerance = 150;

		if  ( BG_GetConditionValue( ci->clientNum, ANIM_COND_MOVETYPE, qfalse ) & ( 1 << ANIM_MT_IDLE ) ) {
			cent->pe.legs.yawing = qfalse; // set it if they really need to swing
			CG_SwingAngles( legsAngles[YAW], 20, clampTolerance, 0.5 * cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
		} else
		//if	( BG_GetConditionValue( ci->clientNum, ANIM_COND_MOVETYPE, qfalse ) & ((1<<ANIM_MT_STRAFERIGHT)|(1<<ANIM_MT_STRAFELEFT)) )
		if  ( strstr( BG_GetAnimString( ci->clientNum, legsSet ), "strafe" ) ) {
			cent->pe.legs.yawing = qfalse; // set it if they really need to swing
			legsAngles[YAW] = headAngles[YAW];
			CG_SwingAngles( legsAngles[YAW], 0, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
		} else
		if ( cent->pe.legs.yawing ) {
			CG_SwingAngles( legsAngles[YAW], 0, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
		} else
		{
			CG_SwingAngles( legsAngles[YAW], 40, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
		}

		torsoAngles[YAW] = cent->pe.torso.yawAngle;
		legsAngles[YAW] = cent->pe.legs.yawAngle;
	}

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 ) {
		dest = ( -360 + headAngles[PITCH] ) * 0.75;
	} else {
		dest = headAngles[PITCH] * 0.75;
	}
	CG_SwingAngles( dest, 15, 30, 0.1, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy( cent->currentState.pos.trDelta, velocity );
	speed = VectorNormalize( velocity );
	if ( speed ) {
		vec3_t axis[3];
		float side;

		speed *= 0.05;

		AnglesToAxis( legsAngles, axis );
		side = speed * DotProduct( velocity, axis[1] );
		legsAngles[ROLL] -= side;

		side = speed * DotProduct( velocity, axis[0] );
		legsAngles[PITCH] += side;
	}

	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}


//==========================================================================

/*
===============
CG_HasteTrail
===============
*/
static void CG_HasteTrail( centity_t *cent ) {
	localEntity_t   *smoke;
	vec3_t origin;
	int anim;

	if ( cent->trailTime > cg.time ) {
		return;
	}
	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
// RF, this is all broken by scripting system
//	if ( anim != LEGS_RUN && anim != LEGS_BACK ) {
//		return;
//	}

	cent->trailTime += 100;
	if ( cent->trailTime < cg.time ) {
		cent->trailTime = cg.time;
	}

	VectorCopy( cent->lerpOrigin, origin );
	origin[2] -= 16;

	smoke = CG_SmokePuff( origin, vec3_origin,
						  8,
						  1, 1, 1, 1,
						  500,
						  cg.time,
						  0,
						  0,
						  cgs.media.hastePuffShader );

	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}


//----(SA)	added and modified from missionpack
/*
===============
CG_BreathPuffs
===============
*/
static void CG_BreathPuffs( centity_t *cent, refEntity_t *head ) {
	clientInfo_t *ci;
	vec3_t up, forward;
	int contents;

	vec3_t mang, morg, maxis[3]; //, mang;

	ci = &cgs.clientinfo[ cent->currentState.number ];

	if ( !cg_enableBreath.integer ) {
		return;
	}

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		return;
	}
	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}

	// allow cg_enableBreath to force everyone to have breath
	if ( cg_enableBreath.integer == 1 ) {
		if ( !( cent->currentState.eFlags & EF_BREATH ) ) {
			return;
		}
	}

	contents = trap_CM_PointContents( head->origin, 0 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}
	if ( ci->breathPuffTime > cg.time ) {
		return;
	}

	CG_GetOriginForTag( cent, head, "tag_mouth", 0, morg, maxis );
	AxisToAngles( maxis, mang );
	AngleVectors( mang, forward, NULL, up );

	//push the origin out a tad so it's not right in the guys face (tad==4)
	VectorMA( morg, 4, forward, morg );

	forward[0] = up[0] * 8 + forward[0] * 5;
	forward[1] = up[1] * 8 + forward[1] * 5;
	forward[2] = up[2] * 8 + forward[2] * 5;

	CG_SmokePuff( morg, forward, 4, 1, 1, 1, 0.5f, 2000, cg.time, cg.time + 400, 0, cgs.media.shotgunSmokePuffShader );


	ci->breathPuffTime = cg.time + 3000 + random() * 1000;

}

//----(SA)	end

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t ent;
	vec3_t angles;
	qboolean ducking;

	// DHM - Nerve :: Don't draw icon above your own head
	if ( cent->currentState.number == cg.snap->ps.clientNum ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, ent.axis );

	// DHM - Nerve :: adjusted values
	VectorCopy( cent->lerpOrigin, ent.origin );

	// Account for ducking
	if ( cent->currentState.clientNum == cg.snap->ps.clientNum ) {
		ducking = ( cg.snap->ps.pm_flags & PMF_DUCKED );
	} else {
		ducking = (qboolean)cent->currentState.animMovetype;
	}

	if ( ducking ) {
		ent.origin[2] += 38;
	} else {
		ent.origin[2] += 56;
	}

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups( centity_t *cent ) {
	int powerups;

	if ( cent->pe.teslaDamagedTime > cg.time - 400 ) {
		trap_R_AddLightToScene( cent->lerpOrigin, 128 + 128 * sin( cg.time * cg.time ), 0.2, 0.6, 1, 0 );
	}

	// RF, AI don't use these effects, they are generally added manually by the game
	if ( cent->currentState.aiChar ) {
		return;
	}

	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}

	// quad gives a dlight
	if ( powerups & ( 1 << PW_QUAD ) ) {
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + ( rand() & 31 ), 0.2, 0.2, 1, 0 );
	}

	// redflag
	if ( powerups & ( 1 << PW_REDFLAG ) ) {
		CG_TrailItem( cent, cgs.media.redFlagModel );
	}

	// blueflag
	if ( powerups & ( 1 << PW_BLUEFLAG ) ) {
		CG_TrailItem( cent, cgs.media.blueFlagModel );
	}

	// haste leaves smoke trails
	if ( powerups & ( 1 << PW_HASTE ) ) {
		CG_HasteTrail( cent );
	}
}


/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
DHM - Nerve :: added height parameter
===============
*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader, int height ) {
	int rf;
	refEntity_t ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;       // only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += height;            // DHM - Nerve :: was '48'
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 6.66;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}



/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
	int team;

	if ( cent->currentState.eFlags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader, 48 );
		return;
	}

	if ( cent->currentState.powerups & ( 1 << PW_INVULNERABLE )
		 && ( ( cent->currentState.effect3Time + 3000 ) > cg.time ) ) {
		CG_PlayerFloatSprite( cent, cgs.media.spawnInvincibleShader, 56 );
		return;
	}

	team = cgs.clientinfo[ cent->currentState.clientNum ].team;

	// DHM - Nerve :: If this client is a medic, draw a 'revive' icon over
	//					dead players that are not in limbo yet.
	if ( cgs.gametype >= GT_WOLF && ( cent->currentState.eFlags & EF_DEAD )
		 && cent->currentState.number == cent->currentState.clientNum
		 && cg.snap->ps.stats[ STAT_PLAYER_CLASS ] == PC_MEDIC
		 && cg.snap->ps.stats[ STAT_HEALTH ] > 0
		 && cg.snap->ps.persistant[PERS_TEAM] == team ) {

		CG_PlayerFloatSprite( cent, cgs.media.medicReviveShader, 8 );
		return;
	}

	// DHM - Nerve :: show voice chat signal so players know who's talking
	if ( cgs.gametype >= GT_WOLF && cent->voiceChatSpriteTime > cg.time
		 && cg.snap->ps.persistant[PERS_TEAM] == team ) {
		CG_PlayerFloatSprite( cent, cent->voiceChatSprite, 56 );
		return;
	}

	// DHM - Nerve :: only show talk icon to team-mates
	if ( cent->currentState.eFlags & EF_TALK && cg.snap->ps.persistant[PERS_TEAM] == team ) {
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader, 48 );
		return;
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define SHADOW_DISTANCE     64
#define ZOFS    6.0
#define SHADOW_MIN_DIST 250.0
#define SHADOW_MAX_DIST 512.0

typedef struct {
	char *tagname;
	float size;
	float maxdist;
	float maxalpha;
	qhandle_t shader;
} shadowPart_t;

static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vec3_t end;
	trace_t trace;
	float alpha, dist, distfade;
	int tagIndex, subIndex;
	vec3_t origin, angles, axis[3];
	shadowPart_t shadowParts[] = {
		{"tag_footleft", 10, 4,  1.0,    0},
		{"tag_footright",    10, 4,  1.0,    0},
		{"tag_torso",        18, 96, 0.8,    0},
		{NULL, 0}
	};

	shadowParts[0].shader = cgs.media.shadowFootShader;     //DAJ pulled out of initliization
	shadowParts[1].shader = cgs.media.shadowFootShader;
	shadowParts[2].shader = cgs.media.shadowTorsoShader;

	*shadowPlane = 0;

	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}

	// no shadows when invisible
	if ( cent->currentState.powerups & ( 1 << PW_INVIS ) ) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, NULL, NULL, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.fraction == 0.0f ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if ( cg_shadows.integer != 1 ) {    // no mark for stencil or projection shadows
		return qtrue;
	}

	// no shadows when dead
	if ( cent->currentState.eFlags & EF_DEAD ) {
		return qfalse;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	dist = VectorDistance( cent->lerpOrigin, cg.snap->ps.origin );
	if ( !( cent->currentState.eFlags & EF_ZOOMING ) && ( dist > SHADOW_MIN_DIST ) ) {
		if ( dist < SHADOW_MAX_DIST ) {
			distfade = ( dist - SHADOW_MIN_DIST ) / ( SHADOW_MAX_DIST - SHADOW_MIN_DIST );
		} else {
			if ( dist > SHADOW_MAX_DIST * 2 ) {
				return qfalse;
			} else { // fade out
				distfade = 1.0 - ( ( dist - SHADOW_MAX_DIST ) / SHADOW_MAX_DIST );
			}
		}
		alpha *= distfade;
		CG_ImpactMark( cgs.media.shadowTorsoShader, trace.endpos, trace.plane.normal,
					   0, alpha,alpha,alpha,1, qfalse, 16, qtrue, -1 );
	} else {
		distfade = 0.0;
	}

	if ( dist < SHADOW_MAX_DIST ) {   // show more detail
		// now add shadows for the various body parts
		for ( tagIndex = 0; shadowParts[tagIndex].tagname; tagIndex++ ) {
			// grab each tag with this name
			for ( subIndex = 0; ( subIndex = CG_GetOriginForTag( cent, &cent->pe.legsRefEnt, shadowParts[tagIndex].tagname, subIndex, origin, axis ) ) >= 0; subIndex++ ) {
				// project it onto the shadow plane
				if ( origin[2] < *shadowPlane ) {
					origin[2] = *shadowPlane;
				}
				alpha = 1.0 - ( ( origin[2] - ( *shadowPlane + ZOFS ) ) / shadowParts[tagIndex].maxdist );
				if ( alpha < 0 ) {
					continue;
				}
				if ( alpha > shadowParts[tagIndex].maxalpha ) {
					alpha = shadowParts[tagIndex].maxalpha;
				}
				alpha *= ( 1.0 - distfade );

				origin[2] = *shadowPlane;

				AxisToAngles( axis, angles );

				CG_ImpactMark( shadowParts[tagIndex].shader, origin, trace.plane.normal,
							   angles[YAW] /*cent->pe.legs.yawAngle*/, alpha,alpha,alpha,1, qfalse, shadowParts[tagIndex].size, qtrue, -1 );
			}
		}
	}

	return qtrue;
}



/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( centity_t *cent ) {
	vec3_t start, end;
	trace_t trace;
	int contents;
	polyVert_t verts[4];

	if ( !cg_shadows.integer ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, end );
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = trap_CM_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0 ) {
		return;
	}

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );
}


//==========================================================================

/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, int powerups, int team, entityState_t *es, const vec3_t fireRiseDir ) {
	centity_t *cent;
	refEntity_t backupRefEnt; //, parentEnt;
	qboolean onFire = qfalse;
	float alpha = 0.0;
	float fireStart, fireEnd;

	cent = &cg_entities[es->number];

	ent->entityNum = es->number;

	if ( cent->pe.forceLOD ) {
		ent->reFlags |= REFLAG_FORCE_LOD;
	}

	backupRefEnt = *ent;

	if ( CG_EntOnFire( &cg_entities[es->number] ) ) {
		ent->reFlags |= REFLAG_FORCE_LOD;
	}

	trap_R_AddRefEntityToScene( ent );

	if ( !onFire && CG_EntOnFire( &cg_entities[es->number] ) ) {
		onFire = qtrue;
		// set the alpha
		if ( ent->entityNum == cg.snap->ps.clientNum ) {
			fireStart = cg.snap->ps.onFireStart;
			fireEnd = cg.snap->ps.onFireStart + 1500;
		} else {
			fireStart = es->onFireStart;
			fireEnd = es->onFireEnd;
		}

		alpha = ( cg.time - fireStart ) / 1500.0;
		if ( alpha > 1.0 ) {
			alpha = ( fireEnd - cg.time ) / 1500.0;
			if ( alpha > 1.0 ) {
				alpha = 1.0;
			}
		}
	}

	if ( onFire ) {
		if ( alpha < 0.0 ) {
			alpha = 0.0;
		}
		ent->shaderRGBA[3] = ( unsigned char )( 255.0 * alpha );
		VectorCopy( fireRiseDir, ent->fireRiseDir );
		if ( VectorCompare( ent->fireRiseDir, vec3_origin ) ) {
			VectorSet( ent->fireRiseDir, 0, 0, 1 );
		}
		ent->customShader = cgs.media.onFireShader;
		trap_R_AddRefEntityToScene( ent );

		ent->customShader = cgs.media.onFireShader2;
		trap_R_AddRefEntityToScene( ent );

		if ( ent->hModel == cent->pe.legsRefEnt.hModel ) {
			trap_S_AddLoopingSound( es->number, ent->origin, vec3_origin, cgs.media.flameCrackSound, (int)( 255.0 * alpha ) );
		}
	}

	// tesla effect
	if ( cg_entities[es->number].pe.teslaDamagedTime > cg.time - 400 ) {
		float alpha;

		alpha = ( 400.0 - (float)( cg.time - cg_entities[es->number].pe.teslaDamagedTime ) ) / 400.0;

		ent->shaderRGBA[0] = ( unsigned char )( 50.0 * alpha );
		ent->shaderRGBA[1] = ( unsigned char )( 130.0 * alpha );
		ent->shaderRGBA[2] = ( unsigned char )( 255.0 * alpha );

		if ( ( cg.time / 50 ) % ( 2 + ( cg.time % 2 ) ) == 0 ) {
			ent->customShader = cgs.media.teslaAltDamageEffectShader;
		} else {
			ent->customShader = cgs.media.teslaDamageEffectShader;
		}
		trap_R_AddRefEntityToScene( ent );
	}

	*ent = backupRefEnt;
}

char    *vtosf( const vec3_t v ) {
	static int index;
	static char str[8][64];
	char    *s;

	// use an array so that multiple vtos won't collide
	s = str[index];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 64, "(%f %f %f)", v[0], v[1], v[2] );

	return s;
}


/*
===============
CG_AnimPlayerConditions

	predict, or calculate condition for this entity, if it is not the local client
===============
*/
void CG_AnimPlayerConditions( centity_t *cent ) {
	entityState_t *es;
	clientInfo_t *ci;
	int legsAnim;

	if ( cg.snap && cg.snap->ps.clientNum == cent->currentState.number && !cg.renderingThirdPerson ) {
		return;
	}

	es = &cent->currentState;
	ci = &cgs.clientinfo[es->clientNum];

	// WEAPON
	if ( es->eFlags & EF_ZOOMING ) {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_WEAPON, WP_BINOCULARS, qtrue );
	} else {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_WEAPON, es->weapon, qtrue );
	}

	// MOUNTED
	if ( es->eFlags & EF_MG42_ACTIVE ) {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_MOUNTED, MOUNTED_MG42, qtrue );
	} else {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_MOUNTED, MOUNTED_UNUSED, qtrue );
	}

	// UNDERHAND
	BG_UpdateConditionValue( es->clientNum, ANIM_COND_UNDERHAND, cent->lerpAngles[0] > 0, qtrue );

	if ( es->eFlags & EF_CROUCHING ) {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_CROUCHING, qtrue, qtrue );
	} else {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_CROUCHING, qfalse, qtrue );
	}

	if ( es->eFlags & EF_FIRING ) {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_FIRING, qtrue, qtrue );
	} else {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_FIRING, qfalse, qtrue );
	}

	// reverse engineer the legs anim -> movetype (if possible)
	legsAnim = es->legsAnim & ~ANIM_TOGGLEBIT;
	if ( ci->modelInfo->animations[legsAnim].movetype ) {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_MOVETYPE, ci->modelInfo->animations[legsAnim].movetype, qfalse );
	}

}

/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent ) {
	clientInfo_t    *ci;
	refEntity_t legs;
	refEntity_t torso;
	refEntity_t head;
	refEntity_t acc;

	vec3_t playerOrigin;
	vec3_t lightorigin;

	int clientNum,i;
	int renderfx;
	qboolean shadow;
	float shadowPlane;

	qboolean usingBinocs = qfalse;              // NERVE - SMF

	centity_t   *cgsnap;

	cgsnap = &cg_entities[cg.snap->ps.clientNum];

	shadow = qfalse;                                                // gjd added to make sure it was initialized
	shadowPlane = 0.0;                                              // ditto
	VectorCopy( vec3_origin, playerOrigin );

	// if set to invisible, skip
	if ( cent->currentState.eFlags & EF_NODRAW ) {
		return;
	}

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity" );
	}
	ci = &cgs.clientinfo[ clientNum ];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid ) {
		return;
	}

	// Arnout: see if we're attached to a gun
	if ( cent->currentState.eFlags & EF_MG42_ACTIVE ) {
		centity_t *mg42;
		int num;

		// find the mg42 we're attached to
		for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
			mg42 = &cg_entities[ cg.snap->entities[ num ].number ];
			if ( mg42->currentState.eType == ET_MG42_BARREL &&
				 mg42->currentState.otherEntityNum == cent->currentState.number ) {
				// found it, clamp behind gun
				vec3_t forward, right, up;

				//AngleVectors (mg42->s.apos.trBase, forward, right, up);
				AngleVectors( cent->lerpAngles, forward, right, up );
				VectorMA( mg42->currentState.pos.trBase, -36, forward, playerOrigin );
				playerOrigin[2] = cent->lerpOrigin[2];
				break;
			}

			if ( num == cg.snap->numEntities ) {
				VectorCopy( cent->lerpOrigin, playerOrigin );
			}
		}
	} else {
		VectorCopy( cent->lerpOrigin, playerOrigin );
	}

	memset( &legs, 0, sizeof( legs ) );
	memset( &torso, 0, sizeof( torso ) );
	memset( &head, 0, sizeof( head ) );
	memset( &acc, 0, sizeof( acc ) );

	// get the rotation information
	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );

//----(SA)	added
	// setting the legs axis should pass the scale down through the other parts/tags/etc.
	// and will need to be 'undone' for the weapon
	if ( ci->playermodelScale[0] ) {
		VectorScale( legs.axis[0], ci->playermodelScale[0], legs.axis[0] );
		VectorScale( legs.axis[1], ci->playermodelScale[1], legs.axis[1] );
		VectorScale( legs.axis[2], ci->playermodelScale[2], legs.axis[2] );
		legs.nonNormalizedAxes  = qtrue;
		torso.nonNormalizedAxes = qtrue;
		head.nonNormalizedAxes  = qtrue;
	}
//----(SA)	end

	// copy the torso rotation to the accessories
	AxisCopy( torso.axis, acc.axis );

	// calculate client-side conditions
	CG_AnimPlayerConditions( cent );

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
						&torso.oldframe, &torso.frame, &torso.backlerp );

	// NERVE - SMF - forcibly set binoc animation
	if ( cent->currentState.eFlags & EF_ZOOMING ) {
		usingBinocs = qtrue;
	}
	// -NERVE - SMF

	// add powerups floating behind the player
	CG_PlayerPowerups( cent );

	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );

	// get the player model information
	renderfx = 0;
	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		renderfx = RF_THIRD_PERSON;         // only draw in mirrors
	}

	// draw the player in cameras
	if ( cg.cameraMode ) {
		renderfx &= ~RF_THIRD_PERSON;
	}

	if ( cg_shadows.integer == 3 && shadow ) {
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;         // use the same origin for all

	// set renderfx for accessories
	acc.renderfx    = renderfx;

	//VectorCopy(cent->lerpOrigin, lightorigin);
	VectorCopy( playerOrigin, lightorigin );
	lightorigin[2] += 31 + (float)cg_drawFPGun.integer;

	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	//VectorCopy( cent->lerpOrigin, legs.origin );
	VectorCopy( playerOrigin, legs.origin );

	if ( ci->playermodelScale[0] != 0 ) {  // player scaled, adjust for the (-24) offset of player legs origin to ground
		legs.origin[2] -= 24 * ( 1.0 - ci->playermodelScale[2] );
	}

	VectorCopy( lightorigin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy( legs.origin, legs.oldorigin );   // don't positionally lerp at all

	if ( !ci->isSkeletal ) {
		CG_AddRefEntityWithPowerups( &legs, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );
	}

	cent->pe.legsRefEnt = legs;

	// if the model failed, allow the default nullmodel to be displayed
	if ( !legs.hModel ) {
		return;
	}

	// (SA) only need to set this once...
	VectorCopy( lightorigin, acc.lightingOrigin );


	//
	// add the torso
	//
	torso.hModel    = ci->torsoModel;

	if ( !torso.hModel ) {
		return;
	}

	torso.customSkin = ci->torsoSkin;

	VectorCopy( lightorigin, torso.lightingOrigin );

	//----(SA) check for ladder and if you're on it, don't allow torso model rotation (so the body climbs aligned with the ladder)
	//----(SA)	also taking care of the Loper's interesting heirarchy (his upper body is effectively the same as a weapon_hand.md3.  it keeps things connected, but has no geometry)

	if ( !ci->isSkeletal ) {
		if ( cgsnap == cent && ( cg.snap->ps.pm_flags & PMF_LADDER ) ) {
			CG_PositionEntityOnTag( &torso, &legs, "tag_torso", 0, NULL );
		} else {
			CG_PositionRotatedEntityOnTag( &torso,  &legs, "tag_torso" );
		}
	} else {    // just clear out the angles
		if ( cgsnap == cent && ( cg.snap->ps.pm_flags & PMF_LADDER ) ) {
			memcpy( torso.axis, legs.axis, sizeof( torso.axis ) );
		}
	}

	torso.shadowPlane   = shadowPlane;
	torso.renderfx      = renderfx;

	if ( !ci->isSkeletal ) {

		CG_AddRefEntityWithPowerups( &torso,    cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );

	} else {    // SKELETAL ANIMATION

		// skeletal models combine the legs and torso, so we must build a compiled refEntity_t

		legs.torsoFrame = torso.frame;
		legs.oldTorsoFrame = torso.oldframe;

		memcpy( legs.torsoAxis, torso.axis, sizeof( torso.axis ) );
		legs.torsoBacklerp = torso.backlerp;

		CG_AddRefEntityWithPowerups( &legs, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );

		cent->pe.legsRefEnt = legs;
		torso = legs;       // so tag calls use the correct values
	}

	cent->pe.torsoRefEnt = torso;

	//
	// add the head
	//
	head.hModel = ci->headModel;
	if ( !head.hModel ) {
		return;
	}
	head.customSkin = ci->headSkin;

	VectorCopy( lightorigin, head.lightingOrigin );

	CG_PositionRotatedEntityOnTag( &head, &torso, "tag_head" );

	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	head.frame = 0;
	head.oldframe = 0;
	head.backlerp = 0.0;

// JPW NERVE -- test
#ifndef PRE_RELEASE_DEMO
	if ( cg_fxflags & 2 ) {
		int i;
		for ( i = 0; i < 3; i++ )
			VectorScale( head.axis[i], 2.5, head.axis[i] );
		head.nonNormalizedAxes = qtrue;
	}
#endif
// jpw

	CG_AddRefEntityWithPowerups( &head, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );

	cent->pe.headRefEnt = head;

	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane );

	// set the shadowplane for accessories
	acc.shadowPlane = shadowPlane;

	CG_BreathPuffs( cent, &head );

	//
	// add the gun / barrel / flash
	//
	if ( !( cent->currentState.eFlags & EF_DEAD ) && !usingBinocs ) {         // NERVE - SMF
		CG_AddPlayerWeapon( &torso, NULL, cent );
	}

	cent->lastWeaponClientFrame = cg.clientFrame;

	//
	// add binoculars (if it's not the player)
	//
	if ( usingBinocs ) {            // NERVE - SMF
		acc.hModel = cgs.media.thirdPersonBinocModel;
		CG_PositionEntityOnTag( &acc, &torso, "tag_weapon", 0, NULL );
		CG_AddRefEntityWithPowerups( &acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );
	}

	//
	// add accessories
	//
	for ( i = ACC_BELT_LEFT; i < ACC_MAX; i++ ) {
		if ( !( ci->accModels[i] ) ) {
			continue;
		}

		acc.hModel = ci->accModels[i];  // set the model

		if ( ci->accSkins[i] ) {
			acc.customSkin = ci->accSkins[i];   // and the skin if there is one


		}
		switch ( i ) {
		case ACC_BELT_LEFT:
			CG_PositionEntityOnTag( &acc,   &legs,  "tag_bright", 0, NULL );
			break;
		case ACC_BELT_RIGHT:
			CG_PositionEntityOnTag( &acc,   &legs,  "tag_bleft", 0, NULL );
			break;

		case ACC_BELT:
			CG_PositionEntityOnTag( &acc,   &torso, "tag_ubelt", 0, NULL );
			break;
		case ACC_BACK:
			CG_PositionEntityOnTag( &acc,   &torso, "tag_back", 0, NULL );
			break;

		case ACC_HAT:           //hat
			if ( cent->currentState.eFlags & EF_HEADSHOT ) {
				continue;
			}
		case ACC_MOUTH2:            // hat2
		case ACC_MOUTH3:            // hat3
			CG_PositionEntityOnTag( &acc,   &head,  "tag_mouth", 0, NULL );
			break;

			// weapon and weapon2
			// these are used by characters who have permanent weapons attached to their character in the skin
		case ACC_WEAPON:        // weap
			CG_PositionEntityOnTag( &acc,   &torso, "tag_weapon", 0, NULL );
			break;
		case ACC_WEAPON2:       // weap2
			CG_PositionEntityOnTag( &acc,   &torso, "tag_weapon2", 0, NULL );
			break;

		default:
			continue;
		}

		CG_AddRefEntityWithPowerups( &acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );
	}

}


//=====================================================================

extern void CG_ClearWeapLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber );

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent ) {
	cent->errorTime = -99999;       // guarantee no error decay added
	cent->extrapolated = qfalse;

	if ( !( cent->currentState.eFlags & EF_DEAD ) ) {
		CG_ClearLerpFrameRate( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim, cent );
		CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim );

		memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
		cent->pe.legs.yawAngle = cent->rawAngles[YAW];
		cent->pe.legs.yawing = qfalse;
		cent->pe.legs.pitchAngle = 0;
		cent->pe.legs.pitching = qfalse;

		memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
		cent->pe.torso.yawAngle = cent->rawAngles[YAW];
		cent->pe.torso.yawing = qfalse;
		cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
		cent->pe.torso.pitching = qfalse;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );

	if ( cg_debugPosition.integer ) {
		CG_Printf( "%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}

	cent->pe.painAnimLegs = -1;
	cent->pe.painAnimTorso = -1;
	cent->pe.animSpeed = 1.0;

}

void CG_GetBleedOrigin( vec3_t head_origin, vec3_t torso_origin, vec3_t legs_origin, int fleshEntityNum ) {
	clientInfo_t    *ci;
	refEntity_t legs;
	refEntity_t torso;
	refEntity_t head;
	centity_t       *cent, backupCent;

	ci = &cgs.clientinfo[ fleshEntityNum ];

	cent = &cg_entities [ fleshEntityNum ];
	backupCent = *cent;

	if ( !ci->infoValid ) {
		return;
	}

	memset( &legs, 0, sizeof( legs ) );
	memset( &torso, 0, sizeof( torso ) );
	memset( &head, 0, sizeof( head ) );

	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
						&torso.oldframe, &torso.frame, &torso.backlerp );

	legs.hModel = ci->legsModel;
	VectorCopy( cent->lerpOrigin, legs.origin );
	VectorCopy( legs.origin, legs.oldorigin );

	// Ridah, restore the cent so we don't interfere with animation timings
	*cent = backupCent;

	if ( !legs.hModel ) {
		return;
	}

	torso.hModel = ci->torsoModel;
	if ( !torso.hModel ) {
		return;
	}

	head.hModel = ci->headModel;
	if ( !head.hModel ) {
		return;
	}

	CG_PositionRotatedEntityOnTag( &torso, &legs, "tag_torso" );
	CG_PositionRotatedEntityOnTag( &head, &torso, "tag_head" );

	VectorCopy( head.origin, head_origin );
	VectorCopy( torso.origin, torso_origin );
	VectorCopy( legs.origin, legs_origin );
}

/*
===============
CG_GetTag
===============
*/
qboolean CG_GetTag( int clientNum, char *tagname, orientation_t *or ) {
	clientInfo_t    *ci;
	centity_t       *cent;
	refEntity_t     *refent;
	vec3_t tempAxis[3];
	vec3_t org;
	int i;

	ci = &cgs.clientinfo[ clientNum ];

	if ( !ci->isSkeletal ) {
		return qfalse;      // only skeletal models supported

	}
	if ( cg.snap && clientNum == cg.snap->ps.clientNum && cg.renderingThirdPerson ) {
		cent = &cg.predictedPlayerEntity;
	} else {
		cent = &cg_entities[ci->clientNum];
		if ( !cent->currentValid ) {
			return qfalse;      // not currently in PVS
		}
	}

	refent = &cent->pe.legsRefEnt;

	if ( trap_R_LerpTag( or, refent, tagname, 0 ) < 0 ) {
		return qfalse;
	}

	VectorCopy( refent->origin, org );

	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( org, or->origin[i], refent->axis[i], org );
	}

	VectorCopy( org, or->origin );

	// rotate with entity
	MatrixMultiply( refent->axis, or->axis, tempAxis );
	memcpy( or->axis, tempAxis, sizeof( vec3_t ) * 3 );

	return qtrue;
}

/*
===============
CG_GetWeaponTag
===============
*/
qboolean CG_GetWeaponTag( int clientNum, char *tagname, orientation_t *or ) {
	clientInfo_t    *ci;
	centity_t       *cent;
	refEntity_t     *refent;
	vec3_t tempAxis[3];
	vec3_t org;
	int i;

	ci = &cgs.clientinfo[ clientNum ];

	if ( !ci->isSkeletal ) {
		return qfalse;      // only skeletal models supported

	}
	if ( cg.snap && clientNum == cg.snap->ps.clientNum && cg.renderingThirdPerson ) {
		cent = &cg.predictedPlayerEntity;
	} else {
		cent = &cg_entities[ci->clientNum];
		if ( !cent->currentValid ) {
			return qfalse;      // not currently in PVS
		}
	}

	if ( cent->pe.gunRefEntFrame < cg.clientFrame - 1 ) {
		return qfalse;
	}

	refent = &cent->pe.gunRefEnt;

	if ( trap_R_LerpTag( or, refent, tagname, 0 ) < 0 ) {
		return qfalse;
	}

	VectorCopy( refent->origin, org );

	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( org, or->origin[i], refent->axis[i], org );
	}

	VectorCopy( org, or->origin );

	// rotate with entity
	MatrixMultiply( refent->axis, or->axis, tempAxis );
	memcpy( or->axis, tempAxis, sizeof( vec3_t ) * 3 );

	return qtrue;
}
