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

/*
 * name:		cg_players.c
 *
 * desc:		handle the media and animation for player entities
 *
*/

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
	"*fall2.wav",
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
	return  (   ( cent->currentState.onFireStart < cg.time ) &&
				( cent->currentState.onFireEnd > cg.time ) );
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
		if ( !Q_stricmp( soundName, cg_customSoundNames[i] ) ) {
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
	char text[20000];
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
	int lastLow, low, numSteps, lastFirst, thisFirst;
	qboolean isStrafe;
	orientation_t o[2];

	refent.hModel = ci->legsModel;

	for ( i = 0, anim = ci->modelInfo->animations; i < ci->modelInfo->numAnimations; i++, anim++ ) {

		if ( anim->moveSpeed == 0 ) {
			continue;
		}

		totalSpeed = 0;
		lastLow = -1;
		numSpeed = 0;
		numSteps = 0;
		isStrafe = qfalse;
		if ( strstr( anim->name, "strafe" ) ) {
			isStrafe = qtrue;
		}

		// first, get the end frame positions, since thats where we loop from
		refent.frame = anim->firstFrame + anim->numFrames - 1;
		refent.oldframe = refent.frame;
		// for each foot
		for ( k = 0; k < 2; k++ ) {
			if ( trap_R_LerpTag( &o[k], &refent, tags[k], 0 ) < 0 ) {
				CG_Error( "CG_CalcMoveSpeeds: unable to find tag %s, cannot calculate movespeed", tags[k] );
			}
		}
		// save the positions
		for ( k = 0; k < 2; k++ ) {
			VectorCopy( o[k].origin, oldPos[k] );
		}
		//
		// set the numSteps
		if ( !isStrafe ) {
			if ( o[0].origin[0] > o[1].origin[0] ) {
				thisFirst = 0;
			} else { thisFirst = 1;}
		} else {
			if ( o[0].origin[1] > o[1].origin[1] ) {
				thisFirst = 0;
			} else { thisFirst = 1;}
		}
		lastFirst = thisFirst;

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
				if ( !isStrafe ) {
					totalSpeed += fabs( oldPos[low][0] - o[low].origin[0] );
				} else {
					totalSpeed += fabs( oldPos[low][1] - o[low].origin[1] );
				}
				//
				// set the numSteps
				if ( !isStrafe ) {
					if ( o[0].origin[0] > o[1].origin[0] ) {
						thisFirst = 0;
					} else { thisFirst = 1;}
				} else {
					if ( o[0].origin[1] > o[1].origin[1] ) {
						thisFirst = 0;
					} else { thisFirst = 1;}
				}
				// if they have changed, record the step
				if ( lastFirst != thisFirst ) {
					numSteps++;
					lastFirst = thisFirst;
				}
			}

			numSpeed++;

			// save the positions
			for ( k = 0; k < 2; k++ ) {
				VectorCopy( o[k].origin, oldPos[k] );
			}
			lastLow = low;
		}

		// record the speed
		if ( anim->moveSpeed < 0 ) {  // use the auto calculations
			anim->moveSpeed = (int)( ( totalSpeed / numSpeed ) * 1000.0 / anim->frameLerp );
		}
		// set the stepGap
		if ( !numSteps ) {
			numSteps = 2;
		}
		if ( numSteps % 2 ) {
			numSteps++;             // round it up if odd number of steps
		}
		anim->stepGap = ( 0.5 * ( (float)anim->moveSpeed * (float)anim->duration / 1000.0 ) );
		anim->stepGap /= ( numSteps / 2 );  // in case there are more than 2 steps in animation
		if ( isStrafe ) {
			anim->stepGap *= 1.3;   // HACK
		}
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

#if 0   // RF, this entire function not used anymore, since we now grab all this stuff from the server

static qboolean CG_ParseAnimationFiles( const char *modelname, clientInfo_t *ci, int client ) {
	char text[100000];
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
#endif

/*
==========================
CG_RegisterClientSkin
==========================
*/

//----(SA) modified this for head separation

static qboolean CG_RegisterClientSkin( clientInfo_t *ci, const char *modelName, const char *skinName ) {
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
extern animScriptData_t *globalScriptData;
qboolean CG_CheckForExistingModelInfo( clientInfo_t *ci, char *modelName, animModelInfo_t **modelInfo ) {
	int i;
	animModelInfo_t *trav; // *firstFree=NULL; // TTimo: unused

	globalScriptData = &cgs.animScriptData;

	for ( i = 0; i < MAX_ANIMSCRIPT_MODELS; i++ ) {
		trav = cgs.animScriptData.modelInfo[i];
		if ( trav && trav->modelname[0] ) {
			// this model is used, so check if it's a match
			if ( !Q_stricmp( trav->modelname, modelName ) ) {
				// found a match, use this modelinfo
				*modelInfo = trav;
				cgs.animScriptData.clientModels[ci->clientNum] = i + 1;
				return qtrue;
			}
		} else {
			// if we fell down to here, then we have found a free slot

			// request it from the server (game module)
			if ( trap_GetModelInfo( ci->clientNum, modelName, &cgs.animScriptData.modelInfo[i] ) ) {

				// success
				cgs.animScriptData.clientModels[ci->clientNum] = i + 1;
				*modelInfo = cgs.animScriptData.modelInfo[i];
				// calc movespeed/footstep values
				CG_CalcMoveSpeeds( ci );
				return qfalse;  // we need to cache all the assets for this character

			}

			// huh!?
			CG_Error( "CG_CheckForExistingModelInfo: unable to optain modelInfo from server" );
		}

	}

	CG_Error( "unable to find a free modelinfo slot, cannot continue\n" );
	// qfalse signifies that we need to parse the information from the script files
	return qfalse;
}


/*
==========================
CG_RegisterClientModelname
==========================
*/

//----(SA) modified this for head separation

static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName ) {
	char namefromskin[MAX_QPATH];
	char filename[MAX_QPATH];
	char name[MAX_QPATH];
	int i;

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
	if ( trap_R_GetSkinModel( ci->torsoSkin, "md3_weapon", &namefromskin[0] ) ) {
		CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_WEAPON], &ci->accSkins[ACC_WEAPON] );
	}
	if ( trap_R_GetSkinModel( ci->torsoSkin, "md3_weapon2", &namefromskin[0] ) ) {
		CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->accModels[ACC_WEAPON2], &ci->accSkins[ACC_WEAPON2] );
	}
//----(SA)	added
	// try anim script parts
	for ( i = 0; i < 8; i++ ) {
		if ( trap_R_GetSkinModel( ci->torsoSkin, va( "md3_animscript%d", i ), &namefromskin[0] ) ) {
			CG_RegisterAcc( ci, va( "models/players/%s", modelName ), namefromskin, &ci->partModels[ACC_WEAPON], &ci->partSkins[ACC_WEAPON] );
		}
	}
//----(SA)	end

	// look for this model in the list of models already opened
	if ( !CG_CheckForExistingModelInfo( ci, (char *)modelName, &ci->modelInfo ) ) {
/*
		if ( !CG_ParseAnimationFiles( modelName, ci, ci->clientNum ) ) {
			Com_Printf( "Failed to load animation file %s\n", filename );
			return qfalse;
		}
*/
		// special case, only cache certain shaders/models for certain characters
		if ( !Q_strcasecmp( (char *)modelName, "zombie" ) ) {
			cgs.media.zombieSpiritWallShader = trap_R_RegisterShader( "zombieDeathWindTrail" );
			cgs.media.zombieSpiritTrailShader = trap_R_RegisterShader( "zombieSpiritTrail" );
			cgs.media.zombieSpiritSkullShader = trap_R_RegisterShader( "zombieSpiritSkull" );
			//cgs.media.zombieDeathDustShader = trap_R_RegisterShader( "zombieDeathDust" );
			//cgs.media.zombieBodyFadeShader = trap_R_RegisterShader( "zombieBodyFade" );
			//cgs.media.zombieHeadFadeShader = trap_R_RegisterShader( "zombieHeadFade" );

			cgs.media.skeletonSkinShader = trap_R_RegisterShader( "skeletonSkin" );
			cgs.media.skeletonLegsModel = trap_R_RegisterModel( "models/players/skel/lower.md3" );
			cgs.media.skeletonLegsSkin = trap_R_RegisterSkin( "models/players/skel/lower_default.skin" );
			cgs.media.skeletonTorsoModel = trap_R_RegisterModel( "models/players/skel/upper.md3" );
			cgs.media.skeletonTorsoSkin = trap_R_RegisterSkin( "models/players/skel/upper_default.skin" );
			cgs.media.skeletonHeadModel = trap_R_RegisterModel( "models/players/skel/head.md3" );
			cgs.media.skeletonHeadSkin = trap_R_RegisterSkin( "models/players/skel/head_default.skin" );

			cgs.media.zombieSpiritSound = trap_S_RegisterSound( "sound/zombie/attack/spirit_start.wav" );
			cgs.media.zombieSpiritLoopSound = trap_S_RegisterSound( "sound/zombie/attack/spirit_loop.wav" );
			cgs.media.zombieDeathSound = trap_S_RegisterSound( "sound/world/ceramicbreak.wav" ); // Zombie Gib

			cgs.media.spiritSkullModel = trap_R_RegisterModel( "models/mapobjects/skull/skul2t.md3" );

			CG_RegisterWeapon( WP_GAUNTLET );
		} else if ( !Q_strcasecmp( (char *)modelName, "beast" ) )      {
			cgs.media.helgaSpiritSkullShader = trap_R_RegisterShader( "helgaSpiritGhost" );
			cgs.media.helgaSpiritTrailShader = trap_R_RegisterShader( "helgaSpiritTrail" );
			cgs.media.helgaGhostModel = trap_R_RegisterModel( "models/players/beast/ghost.md3" );
			cgs.media.helgaSpiritLoopSound = trap_S_RegisterSound( "sound/beast/tortured_souls_loop.wav" );
			cgs.media.helgaSpiritSound = CG_SoundScriptPrecache( "helgaSpiritStartSound" );
			cgs.media.helgaGaspSound = CG_SoundScriptPrecache( "helgaSpiritGasp" );
		} else if ( !Q_strcasecmp( (char *)modelName, "loper" ) )      {
			//cgs.media.loperGroundChargeShader = trap_R_RegisterShader( "loperGroundCharge" );
		} else if ( !Q_strcasecmp( (char *)modelName, "protosoldier" ) )        {
			cgs.media.protoArmorBreak = CG_SoundScriptPrecache( "Protosoldier_loseArmor" );

			cgs.media.protoArmor[0]     = trap_R_RegisterModel( "models/players/protosoldier/armor/nodam_chest.md3" );
			cgs.media.protoArmor[1]     = trap_R_RegisterModel( "models/players/protosoldier/armor/nodam_lftcalf.md3" );
			cgs.media.protoArmor[2]     = trap_R_RegisterModel( "models/players/protosoldier/armor/nodam_lftforarm.md3" );
			cgs.media.protoArmor[3]     = trap_R_RegisterModel( "models/players/protosoldier/armor/nodam_lftshoulder.md3" );
			cgs.media.protoArmor[4]     = trap_R_RegisterModel( "models/players/protosoldier/armor/nodam_lftthigh.md3" );
			cgs.media.protoArmor[5]     = trap_R_RegisterModel( "models/players/protosoldier/armor/nodam_rtcalf.md3" );
			cgs.media.protoArmor[6]     = trap_R_RegisterModel( "models/players/protosoldier/armor/nodam_rtforarm.md3" );
			cgs.media.protoArmor[7]     = trap_R_RegisterModel( "models/players/protosoldier/armor/nodam_rtshoulder.md3" );
			cgs.media.protoArmor[8]     = trap_R_RegisterModel( "models/players/protosoldier/armor/nodam_rtthigh.md3" );

			cgs.media.protoArmor[9]     = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_chest1.md3" );
			cgs.media.protoArmor[10]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_lftcalf1.md3" );
			cgs.media.protoArmor[11]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_lftforarm1.md3" );
			cgs.media.protoArmor[12]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_lftshoulder1.md3" );
			cgs.media.protoArmor[13]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_lftthigh1.md3" );
			cgs.media.protoArmor[14]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_rtcalf1.md3" );
			cgs.media.protoArmor[15]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_rtforarm1.md3" );
			cgs.media.protoArmor[16]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_rtshoulder1.md3" );
			cgs.media.protoArmor[17]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_rtthigh1.md3" );

			cgs.media.protoArmor[18]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_chest2.md3" );
			cgs.media.protoArmor[19]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_lftcalf2.md3" );
			cgs.media.protoArmor[20]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_lftforarm2.md3" );
			cgs.media.protoArmor[21]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_lftshoulder2.md3" );
			cgs.media.protoArmor[22]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_lftthigh2.md3" );
			cgs.media.protoArmor[23]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_rtcalf2.md3" );
			cgs.media.protoArmor[24]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_rtforarm2.md3" );
			cgs.media.protoArmor[25]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_rtshoulder2.md3" );
			cgs.media.protoArmor[26]    = trap_R_RegisterModel( "models/players/protosoldier/armor/dam_rtthigh2.md3" );
		} else if ( !Q_strcasecmp( (char *)modelName, "supersoldier" ) )        {

			cgs.media.superArmorBreak = CG_SoundScriptPrecache( "Supersoldier_loseArmor" );

			cgs.media.superArmor[0]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_chest.md3" );
			cgs.media.superArmor[1]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_lftcalf.md3" );
			cgs.media.superArmor[2]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_lftforarm.md3" );
			cgs.media.superArmor[3]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_lftshoulder.md3" );
			cgs.media.superArmor[4]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_lftthigh.md3" );
			cgs.media.superArmor[5]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_rtcalf.md3" );
			cgs.media.superArmor[6]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_rtforarm.md3" );
			cgs.media.superArmor[7]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_rtshoulder.md3" );
			cgs.media.superArmor[8]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_rtthigh.md3" );

			cgs.media.superArmor[9]     = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_lftfoot.md3" );
			cgs.media.superArmor[10]    = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_rtfoot.md3" );
			cgs.media.superArmor[11]    = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_lftuparm.md3" );
			cgs.media.superArmor[12]    = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_rtuparm.md3" );
			cgs.media.superArmor[13]    = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_waist.md3" );
			cgs.media.superArmor[14]    = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_lftknee.md3" );
			cgs.media.superArmor[15]    = trap_R_RegisterModel( "models/players/supersoldier/armor/nodam_rtknee.md3" );



			cgs.media.superArmor[16]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_chest1.md3" );
			cgs.media.superArmor[17]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftcalf1.md3" );
			cgs.media.superArmor[18]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftforarm1.md3" );
			cgs.media.superArmor[19]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftshoulder1.md3" );
			cgs.media.superArmor[20]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftthigh1.md3" );
			cgs.media.superArmor[21]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtcalf1.md3" );
			cgs.media.superArmor[22]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtforarm1.md3" );
			cgs.media.superArmor[23]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtshoulder1.md3" );
			cgs.media.superArmor[24]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtthigh1.md3" );

			cgs.media.superArmor[25]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftfoot1.md3" );
			cgs.media.superArmor[26]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtfoot1.md3" );
			cgs.media.superArmor[27]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftuparm1.md3" );
			cgs.media.superArmor[28]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtuparm1.md3" );
			cgs.media.superArmor[29]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_waist1.md3" );

			cgs.media.superArmor[30]    = 0;
			cgs.media.superArmor[31]    = 0;


			cgs.media.superArmor[32]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_chest2.md3" );
			cgs.media.superArmor[33]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftcalf2.md3" );
			cgs.media.superArmor[34]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftforarm2.md3" );
			cgs.media.superArmor[35]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftshoulder2.md3" );
			cgs.media.superArmor[36]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftthigh2.md3" );
			cgs.media.superArmor[37]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtcalf2.md3" );
			cgs.media.superArmor[38]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtforarm2.md3" );
			cgs.media.superArmor[39]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtshoulder2.md3" );
			cgs.media.superArmor[30]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtthigh2.md3" );

			cgs.media.superArmor[31]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftfoot2.md3" );
			cgs.media.superArmor[32]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtfoot2.md3" );
			cgs.media.superArmor[33]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_lftuparm2.md3" );
			cgs.media.superArmor[44]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_rtuparm2.md3" );
			cgs.media.superArmor[45]    = trap_R_RegisterModel( "models/players/supersoldier/armor/dam_waist2.md3" );

			cgs.media.superArmor[46]    = 0;
			cgs.media.superArmor[47]    = 0;
/*
super has that proto doesn't...

dam_chest3              attached to tag_chest

dam_lftfoot1
dam_lftfoot2
nodam_lftfoot            attached to tag_footleft

dam_rtfoot1
dam_rtfoot2
nodam_rtfoot            attached to tag_footright

dam_lftuparm1
dam_lftuparm2
nodam_lftuparm       attached to tag_sholeft

dam_rtuparm1
dam_rtuparm2
nodam_rtuparm       attached to tag_shoright

dam_waist1
dam_waist2
nodam_waist            attached to tag_torso

nodam_lftknee          attached to tag_calfleft

nodam_rtknee          attached to tag_calfright
*/
		} else if ( !Q_strcasecmp( (char *)modelName, "dark" ) )        {

			cgs.media.superArmorBreak = CG_SoundScriptPrecache( "Supersoldier_loseArmor" );

			cgs.media.superArmor[0]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_chest.md3" );
			cgs.media.superArmor[1]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_lftcalf.md3" );
			cgs.media.superArmor[2]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_lftforarm.md3" );
			cgs.media.superArmor[3]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_lftshoulder.md3" );
			cgs.media.superArmor[4]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_lftthigh.md3" );
			cgs.media.superArmor[5]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_rtcalf.md3" );
			cgs.media.superArmor[6]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_rtforarm.md3" );
			cgs.media.superArmor[7]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_rtshoulder.md3" );
			cgs.media.superArmor[8]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_rtthigh.md3" );

			cgs.media.superArmor[9]     = trap_R_RegisterModel( "models/players/dark/armor/nodam_lftfoot.md3" );
			cgs.media.superArmor[10]    = trap_R_RegisterModel( "models/players/dark/armor/nodam_rtfoot.md3" );
			cgs.media.superArmor[11]    = trap_R_RegisterModel( "models/players/dark/armor/nodam_lftuparm.md3" );
			cgs.media.superArmor[12]    = trap_R_RegisterModel( "models/players/dark/armor/nodam_rtuparm.md3" );
			cgs.media.superArmor[13]    = trap_R_RegisterModel( "models/players/dark/armor/nodam_waist.md3" );
			cgs.media.superArmor[14]    = trap_R_RegisterModel( "models/players/dark/armor/nodam_lftknee.md3" );
			cgs.media.superArmor[15]    = trap_R_RegisterModel( "models/players/dark/armor/nodam_rtknee.md3" );



			cgs.media.superArmor[16]    = trap_R_RegisterModel( "models/players/dark/armor/dam_chest1.md3" );
			cgs.media.superArmor[17]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftcalf1.md3" );
			cgs.media.superArmor[18]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftforarm1.md3" );
			cgs.media.superArmor[19]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftshoulder1.md3" );
			cgs.media.superArmor[20]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftthigh1.md3" );
			cgs.media.superArmor[21]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtcalf1.md3" );
			cgs.media.superArmor[22]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtforarm1.md3" );
			cgs.media.superArmor[23]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtshoulder1.md3" );
			cgs.media.superArmor[24]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtthigh1.md3" );

			cgs.media.superArmor[25]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftfoot1.md3" );
			cgs.media.superArmor[26]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtfoot1.md3" );
			cgs.media.superArmor[27]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftuparm1.md3" );
			cgs.media.superArmor[28]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtuparm1.md3" );
			cgs.media.superArmor[29]    = trap_R_RegisterModel( "models/players/dark/armor/dam_waist1.md3" );

			cgs.media.superArmor[30]    = 0;
			cgs.media.superArmor[31]    = 0;


			cgs.media.superArmor[32]    = trap_R_RegisterModel( "models/players/dark/armor/dam_chest2.md3" );
			cgs.media.superArmor[33]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftcalf2.md3" );
			cgs.media.superArmor[34]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftforarm2.md3" );
			cgs.media.superArmor[35]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftshoulder2.md3" );
			cgs.media.superArmor[36]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftthigh2.md3" );
			cgs.media.superArmor[37]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtcalf2.md3" );
			cgs.media.superArmor[38]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtforarm2.md3" );
			cgs.media.superArmor[39]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtshoulder2.md3" );
			cgs.media.superArmor[30]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtthigh2.md3" );

			cgs.media.superArmor[31]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftfoot2.md3" );
			cgs.media.superArmor[32]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtfoot2.md3" );
			cgs.media.superArmor[33]    = trap_R_RegisterModel( "models/players/dark/armor/dam_lftuparm2.md3" );
			cgs.media.superArmor[44]    = trap_R_RegisterModel( "models/players/dark/armor/dam_rtuparm2.md3" );
			cgs.media.superArmor[45]    = trap_R_RegisterModel( "models/players/dark/armor/dam_waist2.md3" );

			cgs.media.superArmor[46]    = 0;
			cgs.media.superArmor[47]    = 0;

		} else if ( !Q_strcasecmp( (char *)modelName, "heinrich" ) ) {

			cgs.media.heinrichArmorBreak = CG_SoundScriptPrecache( "Heinrich_loseArmor" );

			// RF, these are also used but supersoldier "spirits" in end map
			cgs.media.zombieSpiritLoopSound = trap_S_RegisterSound( "sound/zombie/attack/spirit_loop.wav" );
			cgs.media.ssSpiritSkullModel = trap_R_RegisterModel( "models/players/supersoldier/ssghost.md3" );

			cgs.media.zombieSpiritTrailShader = trap_R_RegisterShader( "zombieSpiritTrail" );
			cgs.media.zombieSpiritLoopSound = trap_S_RegisterSound( "sound/zombie/attack/spirit_loop.wav" );
			cgs.media.helgaGaspSound = CG_SoundScriptPrecache( "helgaSpiritGasp" );

			cgs.media.debrisHitSound = trap_S_RegisterSound( "sound/world/debris_hit.wav" );

			cgs.media.heinrichArmor[0]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_chest.md3" );
			cgs.media.heinrichArmor[1]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lftcalf.md3" );
			cgs.media.heinrichArmor[2]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lftforarm.md3" );
			cgs.media.heinrichArmor[3]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lftshoulder.md3" );
			cgs.media.heinrichArmor[4]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lftthigh.md3" );
			cgs.media.heinrichArmor[5]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rtcalf.md3" );
			cgs.media.heinrichArmor[6]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rtforarm.md3" );
			cgs.media.heinrichArmor[7]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rtshoulder.md3" );
			cgs.media.heinrichArmor[8]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rtthigh.md3" );

			cgs.media.heinrichArmor[9]  = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lftfoot.md3" );
			cgs.media.heinrichArmor[10] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rtfoot.md3" );
			cgs.media.heinrichArmor[11] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lftuparm.md3" );
			cgs.media.heinrichArmor[12] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rtuparm.md3" );
			cgs.media.heinrichArmor[13] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_waist.md3" );
			cgs.media.heinrichArmor[14] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lftknee.md3" );
			cgs.media.heinrichArmor[15] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rtknee.md3" );

			cgs.media.heinrichArmor[16] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lftelbow.md3" );
			cgs.media.heinrichArmor[17] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rtelbow.md3" );
			cgs.media.heinrichArmor[18] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lfthip.md3" );
			cgs.media.heinrichArmor[19] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rthip.md3" );
			cgs.media.heinrichArmor[20] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_lftshin.md3" );
			cgs.media.heinrichArmor[21] = trap_R_RegisterModel( "models/players/heinrich/armor/nodam_rtshin.md3" );


			cgs.media.heinrichArmor[22] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_chest1.md3" );
			cgs.media.heinrichArmor[23] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftcalf1.md3" );
			cgs.media.heinrichArmor[24] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftforarm1.md3" );
			cgs.media.heinrichArmor[25] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftshoulder1.md3" );
			cgs.media.heinrichArmor[26] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftthigh1.md3" );
			cgs.media.heinrichArmor[27] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtcalf1.md3" );
			cgs.media.heinrichArmor[28] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtforarm1.md3" );
			cgs.media.heinrichArmor[29] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtshoulder1.md3" );
			cgs.media.heinrichArmor[30] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtthigh1.md3" );

			cgs.media.heinrichArmor[31] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftfoot1.md3" );
			cgs.media.heinrichArmor[32] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtfoot1.md3" );
			cgs.media.heinrichArmor[33] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftuparm1.md3" );
			cgs.media.heinrichArmor[34] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtuparm1.md3" );
			cgs.media.heinrichArmor[35] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_waist1.md3" );
			cgs.media.heinrichArmor[36] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftknee1.md3" );
			cgs.media.heinrichArmor[37] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtknee1.md3" );

			cgs.media.heinrichArmor[38] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftelbow1.md3" );
			cgs.media.heinrichArmor[39] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtelbow1.md3" );
			cgs.media.heinrichArmor[40] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lfthip1.md3" );
			cgs.media.heinrichArmor[41] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rthip1.md3" );
			cgs.media.heinrichArmor[42] = 0;
			cgs.media.heinrichArmor[43] = 0;


			cgs.media.heinrichArmor[44] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_chest2.md3" );
			cgs.media.heinrichArmor[45] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftcalf2.md3" );
			cgs.media.heinrichArmor[46] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftforarm2.md3" );
			cgs.media.heinrichArmor[47] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftshoulder2.md3" );
			cgs.media.heinrichArmor[48] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftthigh2.md3" );
			cgs.media.heinrichArmor[49] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtcalf2.md3" );
			cgs.media.heinrichArmor[50] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtforarm2.md3" );
			cgs.media.heinrichArmor[51] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtshoulder2.md3" );
			cgs.media.heinrichArmor[52] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtthigh2.md3" );

			cgs.media.heinrichArmor[43] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftfoot2.md3" );
			cgs.media.heinrichArmor[54] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtfoot2.md3" );
			cgs.media.heinrichArmor[55] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftuparm2.md3" );
			cgs.media.heinrichArmor[56] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtuparm2.md3" );
			cgs.media.heinrichArmor[57] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_waist2.md3" );
			cgs.media.heinrichArmor[58] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftknee2.md3" );
			cgs.media.heinrichArmor[59] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtknee2.md3" );

			cgs.media.heinrichArmor[60] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lftelbow2.md3" );
			cgs.media.heinrichArmor[61] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rtelbow2.md3" );
			cgs.media.heinrichArmor[62] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_lfthip2.md3" );
			cgs.media.heinrichArmor[63] = trap_R_RegisterModel( "models/players/heinrich/armor/dam_rthip2.md3" );
			cgs.media.heinrichArmor[64] = 0;
			cgs.media.heinrichArmor[65] = 0;

/*
heinrich has that ss doesn't...

dam_lftelbow
dam_lftelbow1
dam_lftelbow2            attached to tag_sholeft

nodam_rtelbow
dam_rtelbow1
dam_rtelbow2            attached to tag_shoright

nodam_lfthip
dam_lfthip1
dam_lfthip2
dam_lfthip3                attached to tag_legleft

nodam_rthip
dam_rthip1
dam_rthip2
dam_rthip3                 attached to tag_legright

nodam_lftshin           attached to tag_calfleft

nodam_rtshin            attached to tag_calfright
*/
		}
//----(SA)	end

// end special AI model loading


		// -------- FOOTSTEP SOUNDS ---------
		// load model specific footsteps
		// FIXME: this should be moved over to per model scripts or animation scripting
		if ( !Q_strcasecmp( (char *)modelName, "eliteguard" ) ) {
			// ELITEGUARD

			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/eliteguard/step%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_ELITE_STEP][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/eliteguard/clank%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_ELITE_METAL][i] = trap_S_RegisterSound( name );

//				Com_sprintf (name, sizeof(name), "sound/player/footsteps/eliteguard/roof%i.wav", i+1);
//				cgs.media.footsteps[FOOTSTEP_ELITE_ROOF][i] = trap_S_RegisterSound (name);

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/eliteguard/wood%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_ELITE_WOOD][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/eliteguard/gravel%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_ELITE_GRAVEL][i] = trap_S_RegisterSound( name );
			}
		} else if (   !Q_strcasecmp( (char *)modelName, "protosoldier" ) )     {
			// ProtoSoldier
			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/protosoldier/step%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_PROTOSOLDIER_STEP][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/protosoldier/clank%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_PROTOSOLDIER_METAL][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/protosoldier/grass%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_PROTOSOLDIER_GRASS][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/protosoldier/gravel%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_PROTOSOLDIER_GRAVEL][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/protosoldier/wood%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_PROTOSOLDIER_WOOD][i] = trap_S_RegisterSound( name );
			}
		} else if ( !Q_strcasecmp( (char *)modelName, "supersoldier" ) )        {
			// SuperSoldier/HEINRICH
			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/supersoldier/step%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_SUPERSOLDIER_STEP][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/supersoldier/clank%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_SUPERSOLDIER_METAL][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/supersoldier/grass%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_SUPERSOLDIER_GRASS][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/supersoldier/gravel%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_SUPERSOLDIER_GRAVEL][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/supersoldier/wood%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_SUPERSOLDIER_WOOD][i] = trap_S_RegisterSound( name );
			}
		}
		// Heinrich special
		else if ( !Q_strcasecmp( (char *)modelName, "heinrich" ) ) {
			// SuperSoldier/HEINRICH
			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/heinrich/step%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_HEINRICH][i] = trap_S_RegisterSound( name );
			}
		} else if ( !Q_strcasecmp( (char *)modelName, "loper" ) )        {
			// Loper
			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/loper/clank%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_LOPER_METAL][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/loper/step%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_LOPER_STEP][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/loper/wood%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_LOPER_WOOD][i] = trap_S_RegisterSound( name );
			}
		} else if ( !Q_strcasecmp( (char *)modelName, "zombie" ) )        {
			// Zombie
			for ( i = 0; i < 4; i++ ) {
				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/zombie/gravel%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_ZOMBIE_GRAVEL][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/zombie/step%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_ZOMBIE_STEP][i] = trap_S_RegisterSound( name );

				Com_sprintf( name, sizeof( name ), "sound/player/footsteps/zombie/wood%i.wav", i + 1 );
				cgs.media.footsteps[FOOTSTEP_ZOMBIE_WOOD][i] = trap_S_RegisterSound( name );
			}
		} else if ( !Q_strcasecmp( (char *)modelName, "beast" ) )        {
			// Helga Boss
			cgs.media.footsteps[FOOTSTEP_BEAST][0] = CG_SoundScriptPrecache( "beastStep" );   // just precache the sound script
		}
	}

	// whoops!  this stuff would never get set if it found one existing already!!
	if ( !Q_strcasecmp( (char *)modelName, "loper" ) ) {
		ci->partModels[8] = trap_R_RegisterModel( va( "models/players/%s/spinner.md3", modelName ) );
	} else if ( !Q_strcasecmp( (char *)modelName, "sealoper" ) )      {
		ci->partModels[8] = trap_R_RegisterModel( va( "models/players/%s/spinner.md3", modelName ) );
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
void CG_LoadClientInfo( clientInfo_t *ci ) {
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

// (SA) note to Ryan: The problem I see with having the model set for cg_forceModel in the game (g_forcemodel)
//		is that it was initally there for a performance/fairness thing so you can connect to a
//		server and not use other players goofy models or whatever.  We should still have some simple
//		client-side thing for defaulting all models to one particular player model or something.  (did that make sense?)

	// head
	v = Info_ValueForKey( configstring, "head" );
/* RF, disabled this, not needed anymore
	if ( cg_forceModel.integer )
	{
		char modelStr[MAX_QPATH];

		// forcemodel makes everyone use a single model
		// to prevent load hitches

		trap_Cvar_VariableStringBuffer( "head", modelStr, sizeof( modelStr ) );
		Q_strncpyz( newInfo.hSkinName, modelStr, sizeof( newInfo.hSkinName ) );
	}
	else {
*/
	Q_strncpyz( newInfo.hSkinName, v, sizeof( newInfo.hSkinName ) );
//	}

//----(SA) modified this for head separation

	// model
	v = Info_ValueForKey( configstring, "model" );
/* RF, disabled this, not needed anymore
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
		if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
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
*/
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
//	}

	//----(SA) modify \/ to differentiate for head models/skins as well


	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo ) ) {
		qboolean forceDefer;

		// RF, disabled this, we can't have this happening in Wolf. If there is not enough memory,
		// then we have a leak or havent allocated enough hunk
		forceDefer = qfalse;

//		forceDefer = trap_MemoryRemaining() < 4000000;

		// if we are defering loads, just have it pick the first valid
//		if ( forceDefer || ( cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading ) ) {

		// very temporary!  do not defer any players just yet
		// we need to get ai's to be non-deferred players before we can
		// do this (SA)
		if ( forceDefer ) {
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

	// if death frame, move straight to last frame
	if ( cent->currentState.eFlags & EF_DEATH_FRAME ) {
		lf->frameTime = cg.time - 1;
		lf->animationTime = cg.time - 1;
		lf->frame = anim->firstFrame + anim->numFrames - 1;
		lf->oldFrame = lf->frame;
		lf->oldAnimationNumber = lf->animationNumber;
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
	animation_t *otherAnim = NULL; // TTimo: init
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
#if 0
#ifdef _DEBUG   // RF, debugging anims
//if ( cent->currentState.number == 3 )
		CG_Printf( "(%i) %s anim change on %s: %s -> %s\n", cg.time, ci->modelInfo->modelname, ( lf == &cent->pe.legs ? "LEGS" : "TORSO" ), lf->animation->name, ci->modelInfo->animations[newAnimation & ~( 1 << 9 )].name );
#endif
#endif
		CG_SetLerpFrameAnimationRate( cent, ci, lf, newAnimation );
	}

	anim = lf->animation;

	// check for forcing last frame
	if ( cent->currentState.eFlags & EF_FORCE_END_FRAME ) {
		lf->frame = anim->firstFrame + anim->numFrames - 1;
		lf->oldFrame = lf->frame;
		lf->backlerp = 0;
		return;
	}

	// Ridah, make sure the animation speed is updated when possible
	if ( anim->moveSpeed && lf->oldFrameSnapshotTime ) {
		float moveSpeed;

		// calculate the speed at which we moved over the last frame
		if ( cg.latestSnapshotTime != lf->oldFrameSnapshotTime && cg.nextSnap ) {
			if ( cent->currentState.number == cg.snap->ps.clientNum ) {
				if ( isLadderAnim ) { // only use Z axis for speed
					lf->oldFramePos[0] = cent->lerpOrigin[0];
					lf->oldFramePos[1] = cent->lerpOrigin[1];
				} else {    // only use x/y axis
					lf->oldFramePos[2] = cent->lerpOrigin[2];
				}
				moveSpeed = Distance( cent->lerpOrigin, lf->oldFramePos ) / ( (float)( cg.time - lf->oldFrameTime ) / 1000.0 );
			} else {
				if ( isLadderAnim ) { // only use Z axis for speed
					lf->oldFramePos[0] = cent->currentState.pos.trBase[0];
					lf->oldFramePos[1] = cent->currentState.pos.trBase[1];
				}
				// TTimo
				// show_bug.cgi?id=407
				if ( cg.snap != cg.nextSnap ) {
					moveSpeed = Distance( cent->currentState.pos.trBase, cent->nextState.pos.trBase ) / ( (float)( cg.nextSnap->serverTime - cg.snap->serverTime ) / 1000.0 );
				} else
				{
					moveSpeed = 0;
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
#if 0
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
#else
			if ( anim->moveSpeed || lf->frameTime + 1000 < cg.time ) {
				lf->frameTime = lf->oldFrameTime + (int)( (float)anim->frameLerp * ( 1.0 / lf->animSpeedScale ) );
				f = ( lf->frame - anim->firstFrame ) + 1;
/*				if (lf->frameTime < cg.time) {
					lf->frameTime = cg.time;
				}
*/
				while ( lf->frameTime < cg.time ) {
					lf->frameTime += (int)( (float)anim->frameLerp * ( 1.0 / lf->animSpeedScale ) );
					f++;
					while ( f >= anim->numFrames )
						f -= anim->numFrames;
				}
			} else {    // skip frames as required
				f = lf->frame - anim->firstFrame;
				if ( f < 0 ) {
					f = 0;
				}
				while ( lf->frameTime < cg.time ) {
					lf->frameTime += (int)anim->frameLerp;
					f++;    // f is allowed to go over anim->numFrame, since it gets adjusted later
				}
			}
#endif

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
			// NOTE TTimo
			// show_bug.cgi?id=424
			// is that a related problem?
#if 0
// disabled, causes bad jolting when oldFrame is updated incorrectly
			// Ridah, run the frame again until we move ahead of the current time, fixes walking speeds for zombie
			if ( recursion > MAX_LERPFRAME_RECURSION ) {
				lf->frameTime = cg.time;
			} else {
				CG_RunLerpFrameRate( ci, lf, newAnimation, cent, recursion + 1 );
			}
#endif
			lf->frameTime = cg.time;

#if 0
			if ( cg_debugAnim.integer ) {
				CG_Printf( "Clamp lf->frameTime\n" );
			}
#endif
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
	if ( !( cent->currentState.eFlags & ( EF_DEAD | EF_NO_TURN_ANIM ) ) && cent->pe.legs.yawing ) {
//CG_Printf("turn: %i\n", cg.time );
		tempIndex = BG_GetAnimScriptAnimation( clientNum, cent->currentState.aiState, ( cent->pe.legs.yawing == SWING_RIGHT ? ANIM_MT_TURNRIGHT : ANIM_MT_TURNLEFT ) );
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
================
CG_IdleHeadMovement
================
*/
static void CG_IdleHeadMovement( centity_t *cent, const vec3_t torsoAngles, vec3_t headAngles ) {
	const float angleSpeedMax = 60;
	const float angleSpeedAccel = 40;
	//const float diffThreshold = 50;	// don't yaw from torso beyond this in any axis
	const float diffRandMax = 30;
	const float stopTime = 3000;

	int i;

	if ( ( cent->currentState.eFlags & EF_DEAD ) || !( cent->currentState.eFlags & EF_HEADLOOK ) ) {
		// center
		VectorClear( cent->pe.headLookIdeal );
		cent->pe.headLookStopTime = 0;
		cent->pe.headLookSpeed = 100;       // hurry back to normal
	} else if ( !cent->pe.headLookSpeedMax && cent->pe.headLookStopTime < cg.time ) {
		// need new ideal angles
		VectorSet( cent->pe.headLookIdeal,
				   diffRandMax * ( -0.25 + crandom() ) * 0.25,  // pitch
				   diffRandMax * crandom(),     // yaw
				   0 );                         // roll
		cent->pe.headLookSpeedMax = angleSpeedMax; // * (0.5 + 0.5*random());
		cent->pe.headLookStopTime = cg.time - 1;
	}

	// move towards ideal position
	if ( cent->pe.headLookStopTime < cg.time && !VectorCompare( cent->pe.headLookIdeal, cent->pe.headLookOffset ) ) {
		// slow down as we get closer
		if ( fabs( cent->pe.headLookOffset[YAW] - cent->pe.headLookIdeal[YAW] ) < angleSpeedMax * 1.2 ) {
			cent->pe.headLookSpeedMax = angleSpeedMax * ( 0.1 + 0.9 * ( fabs( cent->pe.headLookOffset[YAW] - cent->pe.headLookIdeal[YAW] ) / ( angleSpeedMax * 1.2 ) ) );
		}
		// accelerate angle speed
		if ( cent->pe.headLookSpeed < cent->pe.headLookSpeedMax ) {
			cent->pe.headLookSpeed += angleSpeedAccel * ( 0.001 * cg.frametime );
			if ( cent->pe.headLookSpeed > cent->pe.headLookSpeedMax ) {
				cent->pe.headLookSpeed = cent->pe.headLookSpeedMax;
			}
		} else if ( cent->pe.headLookSpeed > cent->pe.headLookSpeedMax ) {
			cent->pe.headLookSpeed -= angleSpeedAccel * ( 0.001 * cg.frametime );
			if ( cent->pe.headLookSpeed < cent->pe.headLookSpeedMax ) {
				cent->pe.headLookSpeed = cent->pe.headLookSpeedMax;
			}
		}
		// move towards the ideal angles
		for ( i = 0; i < 3; i++ ) {
			if ( cent->pe.headLookOffset[i] < cent->pe.headLookIdeal[i] ) {
				cent->pe.headLookOffset[i] += cent->pe.headLookSpeed * 0.001 * cg.frametime;
				if ( cent->pe.headLookOffset[i] > cent->pe.headLookIdeal[i] ) {
					cent->pe.headLookOffset[i] = cent->pe.headLookIdeal[i];
				}
			} else if ( cent->pe.headLookOffset[i] > cent->pe.headLookIdeal[i] ) {
				cent->pe.headLookOffset[i] -= cent->pe.headLookSpeed * 0.001 * cg.frametime;
				if ( cent->pe.headLookOffset[i] < cent->pe.headLookIdeal[i] ) {
					cent->pe.headLookOffset[i] = cent->pe.headLookIdeal[i];
				}
			}
		}
		// if we made it, stop here for a bit
		if ( VectorCompare( cent->pe.headLookIdeal, cent->pe.headLookOffset ) ) {
			cent->pe.headLookStopTime = cg.time + (int)( stopTime * ( 0.5 + 0.5 * random() ) );
		}
		// randomly choose a new destination before reaching ideal angles
		if ( cent->pe.headLookStopTime % 2 && rand() % ( cg.time - cent->pe.headLookStopTime ) > 700 ) {
			cent->pe.headLookSpeedMax = 0;
		}
	} else {
		cent->pe.headLookSpeedMax = 0;
		cent->pe.headLookSpeed = 0;         // accelerate when resuming idle movement
	}
/*
	// make sure these angles don't place us too far from the torso angles
	for (i=0; i<3; i++) {
		if (fabs(cent->pe.headLookOffset[i] + headAngles[i] - torsoAngles[i]) > diffThreshold) {
			if (cent->pe.headLookOffset[i] + headAngles[i] - torsoAngles[i] > 0)
				cent->pe.headLookOffset[i] = diffThreshold - (headAngles[i] - torsoAngles[i]);
			else
				cent->pe.headLookOffset[i] = -diffThreshold - (headAngles[i] - torsoAngles[i]);
		}
	}
*/
	// finally, add the headLookOffset to the head angles
	VectorAdd( headAngles, cent->pe.headLookOffset, headAngles );
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
	//static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 }; // TTimo: unused
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
			clampTolerance = 40;
		} else if ( !( cent->currentState.eFlags & EF_FIRING ) && !( cent->currentState.eFlags & EF_RECENTLY_FIRING ) ) {
			torsoAngles[YAW] = headAngles[YAW] + 0.35 * cent->currentState.angles2[YAW];
			clampTolerance = 60;
		} else {    // must be firing
			torsoAngles[YAW] = headAngles[YAW]; // always face firing direction
			//if (fabs(cent->currentState.angles2[YAW]) > 30)
			//	legsAngles[YAW] = headAngles[YAW];
			clampTolerance = 40;
		}

		// torso
		if ( !cent->pe.torso.yawing ) {
			CG_SwingAngles( torsoAngles[YAW], 30, clampTolerance, 0.5 * cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
		} else if ( !( cent->currentState.eFlags & EF_FIRING ) && !( cent->currentState.eFlags & EF_RECENTLY_FIRING ) ) {
			// not firing
			CG_SwingAngles( torsoAngles[YAW], 0, clampTolerance, 0.5 * cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
		} else { // firing
			CG_SwingAngles( torsoAngles[YAW], 0, clampTolerance, 4.0 * cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
		}

		// if the legs are yawing (facing heading direction), allow them to rotate a bit, so we don't keep calling
		// the legs_turn animation while an AI is firing, and therefore his angles will be randomizing according to their accuracy

		clampTolerance = 90;

		if  ( BG_GetConditionValue( ci->clientNum, ANIM_COND_MOVETYPE, qfalse ) & ( 1 << ANIM_MT_IDLE ) ) {
			if ( cent->pe.legs.yawing ) {
				CG_SwingAngles( legsAngles[YAW], 0, clampTolerance, 0.75 * cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
			} else {
				cent->pe.legs.yawing = qfalse; // set it if they really need to swing
				CG_SwingAngles( legsAngles[YAW], 50, clampTolerance, 0.75 * cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
			}
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
			CG_SwingAngles( legsAngles[YAW], 50, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );
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

	// RF, add the head movement if flag is set for looking around
	CG_IdleHeadMovement( cent, torsoAngles, headAngles );

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
	vec3_t axis[3];

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	// DHM - Nerve :: adjusted values
	VectorMA( cent->lerpOrigin, -4, axis[0], ent.origin );
	ent.origin[2] += 36;
	VectorScale( cg.autoAxis[0], 0.75, ent.axis[0] );
	VectorScale( cg.autoAxis[1], 0.75, ent.axis[1] );
	VectorScale( cg.autoAxis[2], 0.75, ent.axis[2] );
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

//----(SA)	test stuff leave in, but comment out
//	if(cg_forceModel.integer) {
//		vec3_t orig, forward, li;
//		trace_t		trace;
//
//		AngleVectors(cg.refdefViewAngles, forward, NULL, NULL);
//		VectorNormalize(forward); // just in case
//		VectorCopy(cent->lerpOrigin, orig);
//		orig[2]+=cg.predictedPlayerState.viewheight;
//		VectorMA(orig, 1000, forward, li);
//		CG_Trace(&trace, orig, NULL, NULL, li, -1, MASK_SHOT);
//		VectorMA(trace.endpos, -5, forward, li);
//		trap_R_AddLightToScene( li, 100 + 100*trace.fraction, 1, 1, 1, 1 );
//	}
//----(SA)	end

	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}

	// quad gives a dlight
	if ( powerups & ( 1 << PW_QUAD ) ) {
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + ( rand() & 31 ), 0.2, 0.2, 1, 0 );
	}

	// flight plays a looped sound
//	if ( powerups & ( 1 << PW_FLIGHT ) ) {
//		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flightSound, 255 );
//	}

	// redflag
	if ( powerups & ( 1 << PW_REDFLAG ) ) {
		CG_TrailItem( cent, cgs.media.redFlagModel );
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + ( rand() & 31 ), 1, 0.2, 0.2, 0 );
	}

	// blueflag
	if ( powerups & ( 1 << PW_BLUEFLAG ) ) {
		CG_TrailItem( cent, cgs.media.blueFlagModel );
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + ( rand() & 31 ), 0.2, 0.2, 1, 0 );
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
	ent.radius = 10;
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

	if ( cg_showAIState.integer && cent->currentState.aiChar ) {
		CG_PlayerFloatSprite( cent, cgs.media.aiStateShaders[cent->currentState.aiState], 48 );
		return;
	}

	if ( cent->currentState.eFlags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader, 48 );
		return;
	}

	// DHM - Nerve :: If this client is a medic, draw a 'revive' icon over
	//					dead players that are not in limbo yet.
	team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	if ( cgs.gametype == GT_WOLF && ( cent->currentState.eFlags & EF_DEAD )
		 && cent->currentState.number == cent->currentState.clientNum
		 && cg.snap->ps.stats[ STAT_PLAYER_CLASS ] == PC_MEDIC
		 && cg.snap->ps.persistant[PERS_TEAM] == team ) {

//		CG_PlayerFloatSprite( cent, cgs.media.medicReviveShader, 8 );	//----(SA)	commented out from MP
		return;
	}

	// DHM - Nerve :: not using, gives away position if chatting to coordinate attack
//	if ( cent->currentState.eFlags & EF_TALK ) {
//		CG_PlayerFloatSprite( cent, cgs.media.balloonShader, 48 );
//		return;
//	}

//----(SA) commented out
//	if ( cent->currentState.eFlags & EF_AWARD_IMPRESSIVE ) {
//		CG_PlayerFloatSprite( cent, cgs.media.medalImpressive, 48 );
//		return;
//	}

//----(SA) commented out
//	if ( cent->currentState.eFlags & EF_AWARD_EXCELLENT ) {
//		CG_PlayerFloatSprite( cent, cgs.media.medalExcellent, 48 );
//		return;
//	}

//----(SA) commented out
//	if ( cent->currentState.eFlags & EF_AWARD_GAUNTLET ) {
//		CG_PlayerFloatSprite( cent, cgs.media.medalGauntlet, 48 );
//		return;
//	}
// DHM - Nerve :: Not using friendly sprites in GT_WOLF
	if ( cgs.gametype != GT_WOLF && cgs.gametype >= GT_TEAM &&
		 !( cent->currentState.eFlags & EF_DEAD ) &&
		 cg.snap->ps.persistant[PERS_TEAM] == team ) {

		CG_PlayerFloatSprite( cent, cgs.media.friendShader, 48 );
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
	float alpha, dist, distfade, scale, dot;
	int tagIndex, subIndex;
	vec3_t origin, angles, axis[3];
	#define ZOFS    6.0
	#define SHADOW_MIN_DIST 250.0
	#define SHADOW_MAX_DIST 512.0
	shadowPart_t shadowParts[] = {
		{"tag_footleft", 10, 4,  0.5, 0},
		{"tag_footright",    10, 4,  0.5, 0},
		{"tag_torso",        18, 96, 0.4, 0},
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
	if ( trace.fraction == 1.0 ) {
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

	if ( cg.snap && cent->currentState.number != cg.snap->ps.clientNum ) {
		// scale shadow according to bounding box size
		scale = ( cent->currentState.solid & 255 );
		scale /= 18.0;
	} else {
		scale = 1.0;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// if zooming, dont draw shadows if we're not looking in their direction
	if ( cg.predictedPlayerState.eFlags & EF_ZOOMING ) {
		VectorSubtract( cent->lerpOrigin, cg.refdef.vieworg, end );
		VectorNormalize( end );
		dot = DotProduct( cg.refdef.viewaxis[0], end );
		if ( dot < 0.96 ) {
			return qfalse;
		} else if ( dot < 0.98 ) {
			alpha *= ( dot - 0.94 ) / ( 0.97 - 0.94 );
		}
	}

	// if the torso is below the ground, dont draw shadows
	if ( CG_GetOriginForTag( cent, &cent->pe.legsRefEnt, "tag_torso", 0, origin, axis ) ) {
		if ( origin[2] - cent->lerpOrigin[2] < -26 ) {
			return qfalse;
		}
	}

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	dist = VectorDistance( cent->lerpOrigin, cg.refdef.vieworg );
	if ( dist > SHADOW_MIN_DIST ) {
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
					   0, alpha,alpha,alpha,1, qfalse, 18 * scale, qtrue, -1 );
	} else {
		distfade = 0.0;
	}

	if ( dist < SHADOW_MAX_DIST ) {   // show more detail
		// now add shadows for the various body parts
		for ( tagIndex = 0; shadowParts[tagIndex].tagname; tagIndex++ ) {
			// grab each tag with this name
			for ( subIndex = 0; ( subIndex = CG_GetOriginForTag( cent, &cent->pe.legsRefEnt, shadowParts[tagIndex].tagname, subIndex, origin, axis ) ) >= 0; subIndex++ ) {
				// move the shadow center forward
				VectorMA( origin, 4, axis[0], origin );
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
							   angles[YAW] /*cent->pe.legs.yawAngle*/, alpha,alpha,alpha,1, qfalse, shadowParts[tagIndex].size * scale, qtrue, -1 );
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
// Zombie Effects

/*
==============
CG_SpawnZombieSpirit
==============
*/
void CG_SpawnZombieSpirit( vec3_t origin, vec3_t vel, int trailHead, int ownerNum, refEntity_t *oldrefent, int trailLife, int idealWidth ) {
	localEntity_t   *le;
	refEntity_t     *re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	*re = *oldrefent;

	le->leType = LE_ZOMBIE_SPIRIT;
	le->startTime = cg.time - 5000; // set it back so we dont fade it in like the "end" spirits
	le->endTime = cg.time + 5000;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = cg.time;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( vel, le->pos.trDelta );

	le->effectWidth = trailLife;
	le->radius = idealWidth;
	le->lastTrailTime = cg.time;
	le->headJuncIndex = trailHead;
	le->loopingSound = cgs.media.zombieSpiritLoopSound;

	le->ownerNum = ownerNum;

	re->fadeStartTime = le->endTime - 2000;
	re->fadeEndTime = le->endTime;
}

/*
==============
CG_AddZombieSpiritEffect
==============
*/
void CG_AddZombieSpiritEffect( centity_t *cent ) {
	const int trailLife         = 600;
	const float idealWidth      = 14.0;
	const int fadeInTime        = 1200;
	const int fadeOutTime       = 2000;
	const int minRotationTime   = 3000;
	const int maxRotationTime   = 4000;
	const int minRadiusCycleTime    = 2400;
	const int maxRadiusCycleTime    = 2900;
	const int zCycleTime        = 3000;

	const float minDist         = 16;
	const float maxDist         = 40;
	const float fadeDist        = 4;

	const int sndIntervalMin    = 300;
	const int sndIntervalMax    = 1400;
	const int sndDelay          = 0;
	//const int sndDuration		= 99999;//1500; // TTimo: init

	const int step              = 50;

	int i, t;
	vec3_t v,p[MAX_ZOMBIE_SPIRITS], ang;
	float fadeRatio, alpha, radius;
	int rotationTime, radiusCycleTime;
	trace_t trace;
	refEntity_t refent;

	qboolean active = qfalse;

	static int lastSpiritRelease;

	if ( cent->currentState.aiChar != AICHAR_ZOMBIE ) {
		return;
	}

	// sanity check the server time to make sure we don't start the effect
	// to early, whilst reloading a savegame or something
	if ( cg.time < cent->currentState.effect1Time ) {
		return;
	}

	if ( cent->currentState.eFlags & EF_MONSTER_EFFECT ) {

		if ( !cent->pe.cueZombieSpirit ) {
			// starting a new effect
			cent->pe.cueZombieSpirit = qtrue;
			cent->pe.zombieSpiritStartTime = cent->currentState.effect1Time;
			cent->pe.lastZombieSpirit = cg.time;
			cent->pe.nextZombieSpiritSound = cg.time + sndDelay;
			for ( i = 0; i < MAX_ZOMBIE_SPIRITS; i++ ) {
				cent->pe.zombieSpiritTrailHead[i] = -1;
				cent->pe.zombieSpiritRotationTimes[i] = minRotationTime + ( random() * ( maxRotationTime - minRotationTime ) );
				cent->pe.zombieSpiritRadiusCycleTimes[i] = minRadiusCycleTime + ( random() * ( maxRadiusCycleTime - minRadiusCycleTime ) );
				cent->pe.zombieSpiritStartTimes[i] = cent->currentState.effect1Time;
			}
		}
		cent->pe.zombieSpiritEndTime = cg.time;

	} else {

		// if running another effect, dont mess with its variables
		if ( cent->currentState.eFlags & EF_MONSTER_EFFECT3 || cent->currentState.effect1Time < cent->currentState.effect3Time ) {
			return;
		}

		if ( !cent->pe.zombieSpiritEndTime ) {
			return;
		}
		// clear the flag, and let the effect fade itself out
		cent->pe.cueZombieSpirit = qfalse;
	}

	for ( t = cent->pe.lastZombieSpirit + step; t <= cg.time; t += step ) {

		// add the spirits
		for ( i = 0; i < MAX_ZOMBIE_SPIRITS; i++ ) {

			if ( cent->pe.zombieSpiritTrailHead[i] < -1 ) {
				// spirit has gone, create a new one
				cent->pe.zombieSpiritTrailHead[i] = -1;
				cent->pe.zombieSpiritRotationTimes[i] = minRotationTime + ( random() * ( maxRotationTime - minRotationTime ) );
				cent->pe.zombieSpiritRadiusCycleTimes[i] = minRadiusCycleTime + ( random() * ( maxRadiusCycleTime - minRadiusCycleTime ) );
				cent->pe.zombieSpiritStartTimes[i] = cg.time - step;
			}

			fadeRatio = (float)( cg.time - cent->pe.zombieSpiritStartTimes[i] ) / (float)fadeInTime;
			if ( fadeRatio < 0.0 ) {
				fadeRatio = 0.0;
			}
			if ( fadeRatio > 1.0 ) {
				fadeRatio = 1.0;
			}

			if ( cent->pe.cueZombieSpirit ) {
				alpha = fadeRatio;
			} else {
				alpha = 1.0 - ( (float)( cg.time - cent->pe.zombieSpiritEndTime ) / (float)fadeOutTime );
				fadeRatio = alpha;
				if ( alpha < 0.0 ) {
					cent->pe.zombieSpiritTrailHead[i] = -2; // kill it
					continue;
				}
			}

			active = qtrue; // we have an active spirit, so continue effect

			alpha *= 0.3;

			if ( cent->pe.cueZombieSpirit ) {
				rotationTime = cent->pe.zombieSpiritRotationTimes[i];
				radiusCycleTime = cent->pe.zombieSpiritRadiusCycleTimes[i];

				radius = ( minDist + sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( radiusCycleTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % radiusCycleTime ) / (float)radiusCycleTime ) ) * ( maxDist - minDist ) );

				// get the position
				v[0] = ( 0.5 + 0.5 * fadeRatio ) * sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( rotationTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % rotationTime ) / (float)rotationTime ) ) * radius;
				v[1] = ( 0.5 + 0.5 * fadeRatio ) * cos( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( rotationTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % rotationTime ) / (float)rotationTime ) ) * radius;
				v[2] = 12 + 36 * ( 0.5 + 0.5 * fadeRatio ) * sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( zCycleTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % zCycleTime ) / (float)zCycleTime ) );
				v[2] -= ( 1.0 - fadeRatio ) * 32;

				VectorAdd( cent->lerpOrigin, v, p[i] );
			} else {
				// expand the radius, if it enters world geometry, kill it
				rotationTime = cent->pe.zombieSpiritRotationTimes[i];
				radiusCycleTime = cent->pe.zombieSpiritRadiusCycleTimes[i];

				radius = pow( 1.0 - fadeRatio, 2 ) * fadeDist + ( 1.0 - pow( 1.0 - fadeRatio, 2 ) ) * ( minDist + sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( radiusCycleTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % radiusCycleTime ) / (float)radiusCycleTime ) ) * ( maxDist - minDist ) );

				// get the position
				v[0] = sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( rotationTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % rotationTime ) / (float)rotationTime ) ) * radius;
				v[1] = cos( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( rotationTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % rotationTime ) / (float)rotationTime ) ) * radius;
				v[2] = 12 + 36 * sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( zCycleTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % zCycleTime ) / (float)zCycleTime ) );
				v[2] -= ( 1.0 - fadeRatio ) * 32;

				VectorAdd( cent->lerpOrigin, v, p[i] );

				// check for sinking into geometry
				trap_CM_BoxTrace( &trace, p[i], p[i], NULL, NULL, 0, MASK_SOLID );
				// if we hit something, clip the velocity, but maintain speed
				if ( trace.startsolid ) {
					cent->pe.zombieSpiritTrailHead[i] = -2; // kill it
					continue;
				}
			}

			VectorSubtract( p[i], cent->pe.zombieSpiritPos[i], cent->pe.zombieSpiritDir[i] );
			cent->pe.zombieSpiritSpeed[i] = 1000.0 / step * VectorNormalize( cent->pe.zombieSpiritDir[i] );
			VectorCopy( p[i], cent->pe.zombieSpiritPos[i] );

			cent->pe.zombieSpiritTrailHead[i] = CG_AddTrailJunc( cent->pe.zombieSpiritTrailHead[i],
																 cgs.media.zombieSpiritTrailShader,
																 t,
																 STYPE_STRETCH,
																 p[i],
																 trailLife * 2,
																 alpha,
																 0.0,
																 ( 0.5 + 0.5 * fadeRatio ) * idealWidth,
																 0,
																 TJFL_NOCULL,
																 colorWhite,
																 colorWhite,
																 1.0, 1 );

		}

		cent->pe.lastZombieSpirit = t;

		if ( !active ) {  // effect has gone
			cent->pe.zombieSpiritEndTime = 0;
		}
	}

	// add the skull at the head of the spirits
	memset( &refent, 0, sizeof( refent ) );
	refent.hModel = cgs.media.spiritSkullModel;
	refent.backlerp = 0;
	refent.renderfx = RF_NOSHADOW | RF_MINLIGHT;    //----(SA)
	refent.customShader = cgs.media.zombieSpiritSkullShader;
	refent.reType = RT_MODEL;
	for ( i = 0; i < MAX_ZOMBIE_SPIRITS; i++ ) {
		if ( cent->pe.zombieSpiritTrailHead[i] < -1 ) {
			continue;   // spirit has gone
		}
		fadeRatio = (float)( cg.time - cent->pe.zombieSpiritStartTimes[i] ) / (float)fadeInTime;
		if ( fadeRatio < 0.0 ) {
			fadeRatio = 0.0;
		}
		if ( fadeRatio > 1.0 ) {
			fadeRatio = 1.0;
		}

		if ( cent->pe.cueZombieSpirit ) {
			alpha = fadeRatio;
		} else {
			alpha = 1.0 - ( (float)( cg.time - cent->pe.zombieSpiritEndTime ) / (float)fadeOutTime );
			fadeRatio = alpha;
			if ( alpha < 0.0 ) {
				cent->pe.zombieSpiritTrailHead[i] = -2; // kill it
				continue;
			}
		}

		refent.shaderRGBA[3] = (byte)( 0.5 * alpha * 255.0 );
		VectorCopy( cent->pe.zombieSpiritPos[i], refent.origin );

		// HACK!!! skull model is back-to-front, need to fix
		VectorInverse( cent->pe.zombieSpiritDir[i] );
		vectoangles( cent->pe.zombieSpiritDir[i], ang );
		VectorInverse( cent->pe.zombieSpiritDir[i] );
		AnglesToAxis( ang, refent.axis );
		// create the non-normalized axis so we can size it
		refent.nonNormalizedAxes = qtrue;
		for ( t = 0; t < 3; t++ ) {
			VectorNormalize( refent.axis[t] );
			VectorScale( refent.axis[t], 0.35, refent.axis[t] );
		}
		//
		// add the sound
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.zombieSpiritLoopSound, fadeRatio );
		//
		// if this spirit is in a good position to be released and head to the enemy, then release it
		if ( fadeRatio == 1.0 && ( lastSpiritRelease > cg.time || ( lastSpiritRelease < cg.time - 2000 ) ) ) {
			VectorSubtract( cent->currentState.origin2, refent.origin, v );
			VectorNormalize( v );
			if ( DotProduct( cent->pe.zombieSpiritDir[i], v ) > 0.6 || ( cent->currentState.eFlags & EF_DEAD ) ) {
				// check for sinking into geometry
				trap_CM_BoxTrace( &trace, refent.origin, refent.origin, NULL, NULL, 0, MASK_SOLID );
				// if we hit something, don't release it yet
				if ( !trace.startsolid ) {
					if ( cent->pe.zombieSpiritSpeed[i] < 300 ) {
						cent->pe.zombieSpiritSpeed[i] = 300;
					}
					VectorScale( cent->pe.zombieSpiritDir[i], cent->pe.zombieSpiritSpeed[i], v );
					CG_SpawnZombieSpirit( refent.origin, v, cent->pe.zombieSpiritTrailHead[i], cent->currentState.number, &refent, trailLife, idealWidth );
					lastSpiritRelease = cg.time;
					cent->pe.zombieSpiritTrailHead[i] = -2; // kill this version of it
					continue;
				}
			}
		}
		//
		// if we didn't kill it, draw it
		trap_R_AddRefEntityToScene( &refent );
	}

	if ( cg.time > cent->pe.nextZombieSpiritSound && cent->pe.cueZombieSpirit ) { //&& (cg.time < cent->pe.zombieSpiritStartTime + sndDuration)) {
		// spawn a new sound
		trap_S_StartSound( cent->lerpOrigin, -1, CHAN_AUTO, cgs.media.zombieSpiritSound );
		cent->pe.nextZombieSpiritSound = cg.time + sndIntervalMin + (int)( (float)( sndIntervalMax - sndIntervalMin ) * random() );
	}

	// add a negative light around us
	fadeRatio = (float)( cg.time - cent->pe.zombieSpiritStartTime ) / (float)fadeInTime;
	if ( fadeRatio < 0.0 ) {
		fadeRatio = 0.0;
	}
	if ( fadeRatio > 1.0 ) {
		fadeRatio = 1.0;
	}

	if ( cent->pe.cueZombieSpirit ) {
		alpha = fadeRatio;
	} else {
		alpha = 1.0 - ( (float)( cg.time - cent->pe.zombieSpiritEndTime ) / (float)fadeOutTime );
		fadeRatio = alpha;
		if ( alpha < 0.0 ) {
			cent->pe.zombieSpiritEndTime = 0;   // stop the effect
			return;
		}
	}
	fadeRatio *= 0.7;
	trap_R_AddLightToScene( cent->lerpOrigin, 300.0, 1.0 * fadeRatio, 1.0 * fadeRatio, 1.0 * fadeRatio, 10 );
}

/*
==============
CG_SpawnZombieBat
==============
*/
void CG_SpawnZombieBat( centity_t *cent, refEntity_t *oldrefent ) {
	localEntity_t   *le;
	refEntity_t     *re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	*re = *oldrefent;

	le->leType = LE_ZOMBIE_BAT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 6000; // TODO: some kind of effect when the bat "burns up"

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = cg.time;
	CG_PositionEntityOnTag( re, &cent->pe.headRefEnt, "tag_mouth", 0, NULL );
	VectorCopy( re->origin, le->pos.trBase );
	VectorScale( re->axis[0], 150 + 50 * random(), le->pos.trDelta );

	//le->effectWidth = trailLife;
	//le->radius = idealWidth;
	le->lastTrailTime = cg.time;
	//le->headJuncIndex = trailHead;
	le->loopingSound = cgs.media.batsFlyingLoopSound;

	le->ownerNum = cent->currentState.number;

	re->fadeStartTime = le->endTime - 2000;
	re->fadeEndTime = le->endTime;
}

/*
==============
CG_AddZombieFlameEffect
==============
*/
void CG_AddZombieFlameEffect( centity_t *cent ) {
	//const float	flameRadius		= ZOMBIE_FLAME_RADIUS*2; // TTimo: unused

	const int fadeInTime        = 500;
	const int fadeOutTime       = 0;

	// TTimo: unused
	/*
	  const int spawnIntervalMin	= 300;
	  const int spawnIntervalMax	= 1200;

	  const int sndIntervalMin	= 300;
	  const int sndIntervalMax	= 1400;
	*/
	const int sndDelay          = 0;
	// const int sndDuration		= 99999; // TTimo: unused

	// const int step				= 50; // TTimo: unused

	vec3_t morg, maxis[3], mang;

	float alpha, fadeRatio;

	// qboolean active=qfalse; // TTimo: unused

	if ( cent->currentState.aiChar != AICHAR_ZOMBIE ) {
		return;
	}

	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}

	if ( !IS_FLAMING_ZOMBIE( cent->currentState ) ) {
		return;
	}

	if ( cent->currentState.time > cg.time ) { // doing short burst
		return;
	}

	if ( ( cent->currentState.eFlags & EF_MONSTER_EFFECT3 ) && ( cg.time > cent->currentState.effect3Time ) ) {

		if ( !cent->pe.cueZombieSpirit ) {
			// starting a new effect
			cent->pe.cueZombieSpirit = qtrue;
			cent->pe.zombieSpiritStartTime = cent->currentState.effect3Time;
			cent->pe.lastZombieSpirit = cg.time;
			cent->pe.nextZombieSpiritSound = cg.time + sndDelay;
		}
		cent->pe.zombieSpiritEndTime = cg.time + fadeOutTime;
	} else {

		// if running another effect, dont mess with its variables
		if ( cent->currentState.eFlags & EF_MONSTER_EFFECT || cent->currentState.effect1Time > cent->currentState.effect3Time ) {
			CG_FireFlameChunks( cent, morg, mang, 0.05, qfalse, 0 );
			return;
		}

		if ( cent->pe.zombieSpiritEndTime < cg.time ) {
			CG_FireFlameChunks( cent, morg, mang, 0.05, qfalse, 0 );
			return;
		}

		// clear the flag, and let the effect fade itself out
		cent->pe.cueZombieSpirit = qfalse;
	}

	// expand the flame dlight
	fadeRatio = (float)( cg.time - cent->pe.zombieSpiritStartTime ) / (float)fadeInTime;
	if ( fadeRatio < 0.0 ) {
		fadeRatio = 0.0;
	}
	if ( fadeRatio > 1.0 ) {
		fadeRatio = 1.0;
	}

	if ( cent->pe.cueZombieSpirit ) {
		alpha = fadeRatio;
	} else {
		alpha = ( (float)( cent->pe.zombieSpiritEndTime - cg.time ) / (float)fadeOutTime );
		fadeRatio = alpha;
		if ( alpha < 0.0 ) {
			cent->pe.zombieSpiritEndTime = 0;   // stop the effect
			CG_FireFlameChunks( cent, morg, mang, 0.1, qfalse, 0 );
			return;
		}
	}

	if ( fadeRatio >= 1.0 ) {
		CG_GetOriginForTag( cent, &cent->pe.headRefEnt, "tag_mouth", 0, morg, maxis );
		AxisToAngles( maxis, mang );
		CG_FireFlameChunks( cent, morg, mang, ZOMBIE_FLAME_SCALE, qtrue, 0 );
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flameSound, 50 );
	}
}


/*
==============
CG_AddZombieFlameEffect
==============
*/
void CG_AddZombieFlameShort( centity_t *cent ) {
	vec3_t morg, maxis[3], mang;

	if ( cent->currentState.aiChar != AICHAR_ZOMBIE ) {
		return;
	}

	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}

	if ( !IS_FLAMING_ZOMBIE( cent->currentState ) ) {
		return;
	}

	if ( cent->currentState.time < cg.time ) {
		return;
	}

	CG_GetOriginForTag( cent, &cent->pe.headRefEnt, "tag_mouth", 0, morg, maxis );
	AxisToAngles( maxis, mang );

	// shoot this only in bursts
	if ( ( cg.time + cent->currentState.number * 100 ) % 1000 > 200 ) {
		CG_FireFlameChunks( cent, morg, cent->lerpAngles, 0.1, qfalse, 0 );
		return;
	}

	CG_FireFlameChunks( cent, morg, cent->lerpAngles, 0.4, 2, 0 );
	trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flameSound, 50 );
}

//==========================================================================

/*
===============
LoperLightningEffect

  If loper is alive, a constant lightning effect surrounds him
===============
*/
void CG_AddLoperLightningEffect( centity_t *cent ) {
#define LOPER_LIGHTNING_POINT_TIMEOUT   200
#define LOPER_LIGHTNING_MAX_DIST        360
#define LOPER_LIGHTNING_NORMAL_DIST     80
#define LOPER_MAX_POINT_TESTS           10
#define LOPER_MAX_POINT_TESTS_PERFRAME  20

	int i, j, pointTests = 0;
	vec3_t testPos, tagPos, c, v;
	trace_t tr;
	float maxDist;
	int numPoints;
	float colTake;

	if ( cent->currentState.aiChar != AICHAR_LOPER ) {
		return;
	}

	if ( !cent->currentValid ) {
		return;
	}

	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}

	if ( !cent->pe.legsRefEnt.hModel ) {
		return;
	}

	if ( cent->currentState.eFlags & EF_MONSTER_EFFECT3 ) {
		maxDist = LOPER_LIGHTNING_MAX_DIST;
		numPoints = MAX_LOPER_LIGHTNING_POINTS;
	} else {
		maxDist = LOPER_LIGHTNING_NORMAL_DIST;
		numPoints = MAX_LOPER_LIGHTNING_POINTS / 3;
	}

	CG_GetOriginForTag( cent, &cent->pe.legsRefEnt, "tag_spinner", 0, tagPos, NULL );

	// show a dlight
	// color
	colTake = 0.8 - fabs( sin( cg.time ) ) * 0.3;
	c[0] = 1.0 - colTake;
	c[1] = 1.0 - 0.7 * colTake;
	c[2] = 1.0; //c[1] + 0.2;
	if ( c[2] > 1.0 ) {
		c[2] = 1.0;
	}
	//VectorScale( c, alpha, c );
	// add the light
	trap_R_AddLightToScene( tagPos, LOPER_LIGHTNING_NORMAL_DIST * ( 2.5 + ( 1.0 + sin( cg.time ) ) / 4.0 ), c[0], c[1], c[2], 1 );

	for ( i = 0; i < numPoints; i++ ) {
		// if this point has timed out, find a new spot
		if (    ( cent->currentState.eFlags & EF_MONSTER_EFFECT ) &&
				(   ( !cent->pe.lightningTimes[i] ) ||
					( cent->pe.lightningTimes[i] > cg.time ) ||
					( cent->pe.lightningTimes[i] < cg.time - 50 ) ||
					( VectorDistance( cent->lerpOrigin, cent->pe.lightningPoints[i] ) > maxDist ) ) ) {
			// attacking the player
			VectorSet( testPos, 12 * crandom(),
					   12 * crandom(),
					   32 * crandom() );
			VectorAdd( testPos, cg.snap->ps.origin, testPos );
			cent->pe.lightningTimes[i] = cg.time - rand() % ( LOPER_LIGHTNING_POINT_TIMEOUT / 2 );
			VectorCopy( testPos, cent->pe.lightningPoints[i] );
			// play a zap sound
			if ( cent->pe.lightningSoundTime < cg.time - 100 ) {
				trap_S_StartSound( testPos, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.lightningZap /*cgs.media.lightningSounds[rand()%3]*/ );
				cent->pe.lightningSoundTime = cg.time;
			}
		} else if ( ( !cent->pe.lightningTimes[i] ) ||
					( cent->pe.lightningTimes[i] > cg.time ) ||
					( cent->pe.lightningTimes[i] < cg.time - LOPER_LIGHTNING_POINT_TIMEOUT ) ||
					( VectorDistance( cent->lerpOrigin, cent->pe.lightningPoints[i] ) > maxDist ) ) {

			if ( cent->currentState.groundEntityNum == ENTITYNUM_NONE ) {
				continue;   // must be on the ground

			}
			// find a new spot
			for ( j = 0; j < LOPER_MAX_POINT_TESTS; j++ ) {
				VectorSet( testPos, maxDist * crandom(),
						   maxDist * crandom(),
						   maxDist * ( crandom() - 0.5 ) );
				VectorAdd( testPos, cent->lerpOrigin, testPos );
				// try a trace to find a world collision
				CG_Trace( &tr, cent->lerpOrigin, NULL, NULL, testPos, cent->currentState.number, MASK_SOLID );
				if ( tr.fraction < 1 && tr.entityNum == ENTITYNUM_WORLD ) {
					// found a valid spot!
					cent->pe.lightningTimes[i] = cg.time - rand() % ( LOPER_LIGHTNING_POINT_TIMEOUT / 2 );
					VectorCopy( tr.endpos, cent->pe.lightningPoints[i] );
					// play a zap sound
					if ( cent->pe.lightningSoundTime < cg.time - 100 ) {
						// HACK, move ths sound away from the viewpos, to simulate lower volume
						VectorSubtract( testPos, cg.refdef.vieworg, v );
						VectorMA( cg.refdef.vieworg, 3.0, v, v );
						trap_S_StartSound( v, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.lightningSounds[rand() % 3] );
						cent->pe.lightningSoundTime = cg.time;
					}
					break;
				}
				if ( pointTests++ > LOPER_MAX_POINT_TESTS_PERFRAME ) {
					j = LOPER_MAX_POINT_TESTS;
					continue;
				}
			}
			if ( j == LOPER_MAX_POINT_TESTS ) {
				continue;   // just don't draw this point
			}
		}
		//
		// we have a valid lightning point, so draw it
		// sanity check though to make sure it's valid
		if ( VectorDistance( cent->lerpOrigin, cent->pe.lightningPoints[i] ) <= maxDist ) {
			CG_DynamicLightningBolt( cgs.media.lightningBoltShader, tagPos, cent->pe.lightningPoints[i], 1, 25 + 12.0 * random(), ( cent->currentState.eFlags & EF_MONSTER_EFFECT ) == 0, 1.0, 0, i * i * 2 );
		}
	}
}

/*
===============
LoperGroundEffect

  Electrical charge applied to ground through torso
===============
*/
void CG_AddLoperGroundEffect( centity_t *cent ) {
#define LOPER_GROUNDCHARGE_INTERVAL 30
#define LOPER_GROUNDCHARGE_DURATION 100
#define LOPER_GROUNDCHARGE_FADEOUT  400
#define LOPER_GROUNDCHARGE_RADIUS   150
	vec3_t org, c;
	//static vec3_t up = {0,0,1}; // TTimo: unused
	float colTake;
	int duration;
	float alpha, lightAlpha = 0.0f;   // TTimo: init

	if ( cent->currentState.aiChar != AICHAR_LOPER ) {
		return;
	}

	if ( !cent->currentValid ) {
		return;
	}

	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}

	if ( !cent->pe.legsRefEnt.hModel ) {
		return;
	}

	if ( !( cent->currentState.eFlags & EF_MONSTER_EFFECT3 ) ) {
		if ( !cent->pe.loperGroundValidTime ) {
			alpha = 0.0;
		} else {
			// alpha for lightning mark
			duration = LOPER_GROUNDCHARGE_FADEOUT - ( cg.time - cent->pe.loperGroundValidTime );
			if ( duration <= 0 ) {
				cent->pe.loperGroundValidTime = 0;
			}
			alpha = (float)duration / (float)LOPER_GROUNDCHARGE_FADEOUT;
			if ( alpha < 0 ) {
				alpha = 0;
			}
			// light lives on a bit after the last lightning mark is spawned
			lightAlpha = (float)( duration + LOPER_GROUNDCHARGE_DURATION ) / (float)( LOPER_GROUNDCHARGE_FADEOUT + LOPER_GROUNDCHARGE_DURATION );
			if ( lightAlpha < 0 ) {
				lightAlpha = 0;
			}
		}
	} else {
		cent->pe.loperGroundValidTime = cg.time;
		duration = LOPER_GROUNDCHARGE_DURATION;
		alpha = 1.0;
		lightAlpha = 1.0;
	}

	if ( !lightAlpha ) {
		cent->pe.loperGroundValidTime = 0;
		return;
	}

	// show a dlight
	// color
	colTake = 0.8 - fabs( sin( cg.time ) ) * 0.3;
	c[0] = 1.0 - colTake;
	c[1] = 1.0 - 0.8 * colTake;
	c[2] = 1.0; //c[1] + 0.2;
	if ( c[2] > 1.0 ) {
		c[2] = 1.0;
	}
	VectorScale( c, alpha, c );
	// add the light
	trap_R_AddLightToScene( cent->lerpOrigin, LOPER_GROUNDCHARGE_RADIUS * ( 3.0 + 2.0 * ( 1.0 + sin( 0.001 * ( ( cg.time ) % ( 1000 * ( 2 + cent->currentState.number ) ) ) ) ) / 2.0 ), c[0], c[1], c[2], 1 );
	//trap_R_AddLightToScene( cent->lerpOrigin, LOPER_GROUNDCHARGE_RADIUS*(2.0 + 1.0*(1.0+cos(0.001343*((cg.time)%(1000*(2+cent->currentState.number)))))/2.0), c[0], c[1], c[2], 0 );

	if ( !alpha ) {
		return;
	}

	// create a fading mark at intervals
	if ( cent->pe.loperLastGroundChargeTime < cg.time - LOPER_GROUNDCHARGE_INTERVAL ) {
		// org
		VectorCopy( cent->lerpOrigin, org );
		org[2] -= 20;
		// color
		if ( cent->pe.loperGroundChargeToggle ^= 1 ) {
			// random blue
			colTake = 0.5 + random() * 0.5;
			c[0] = 1.0 - colTake;
			c[1] = 1.0 - /*(0.5 + 0.5*random())**/ colTake;
			c[2] = c[1] + 0.2;
			if ( c[2] > 1.0 ) {
				c[2] = 1.0;
			}
		} else {
			VectorSet( c, 1,1,1 );
		}
		// draw the mark
		//CG_ImpactMark( cgs.media.loperGroundChargeShader, org, up, random()*360, c[0], c[1], c[2], alpha, qtrue, (0.5+0.5*random())*LOPER_GROUNDCHARGE_RADIUS, qfalse, LOPER_GROUNDCHARGE_DURATION );
		cent->pe.loperLastGroundChargeTime = cg.time;
		// make a new sound
		VectorSet( org, org[0] + crandom() * 256, org[1] + crandom() * 256, org[2] + crandom() * 256 );
		trap_S_StartSound( org, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.lightningZap );
	}

}

//==========================================================================

/*
==============
CG_SpawnHelgaSpirit
==============
*/
void CG_SpawnHelgaSpirit( vec3_t origin, vec3_t vel, int trailHead, int ownerNum, refEntity_t *oldrefent, int trailLife, int idealWidth ) {
	localEntity_t   *le;
	refEntity_t     *re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	*re = *oldrefent;

	le->leType = LE_HELGA_SPIRIT;
	le->startTime = cg.time - 5000; // set it back so we dont fade it in like the "end" spirits
	le->endTime = cg.time + 5000;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = cg.time;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( vel, le->pos.trDelta );

	le->effectWidth = trailLife;
	le->radius = idealWidth;
	le->lastTrailTime = cg.time;
	le->headJuncIndex = trailHead;
	le->loopingSound = cgs.media.helgaSpiritLoopSound;

	le->ownerNum = ownerNum;

	re->fadeStartTime = le->endTime - 2000;
	re->fadeEndTime = le->endTime;
	re->shaderTime = cg.time;
}

/*
==============
CG_AddHelgaSpiritEffect
==============
*/
void CG_AddHelgaSpiritEffect( centity_t *cent ) {
	const int trailLife         = 600;
	const float idealWidth      = 30.0;
	const int fadeInTime        = 2000;
	const int fadeOutTime       = 2000;
	const int minRotationTime   = 6000;
	const int maxRotationTime   = 10000;
	const int minRadiusCycleTime    = 2400;
	const int maxRadiusCycleTime    = 2900;
	const int zCycleTime        = 3000;

	const float minDist         = 50;
	const float maxDist         = 100;
	const float fadeDist        = 4;

	const int sndIntervalMin    = 300;
	const int sndIntervalMax    = 1400;
	const int sndDelay          = 0;
	//const int sndDuration		= 99999;//1500; // TTimo: unused

	const int step              = 25;

	int i, t;
	vec3_t v,p[MAX_ZOMBIE_SPIRITS], ang;
	float fadeRatio, alpha, radius;
	int rotationTime, radiusCycleTime;
	trace_t trace;
	refEntity_t refent;

	static int lastSpiritRelease;

	qboolean active = qfalse;

	if ( cent->currentState.aiChar != AICHAR_HELGA ) {
		return;
	}

	// sanity check the server time to make sure we don't start the effect
	// to early, whilst reloading a savegame or something
	if ( cg.time < cent->currentState.effect1Time ) {
		return;
	}

	if ( cent->currentState.eFlags & EF_MONSTER_EFFECT ) {

		if ( !cent->pe.cueZombieSpirit ) {
			// starting a new effect
			cent->pe.cueZombieSpirit = qtrue;
			cent->pe.zombieSpiritStartTime = cent->currentState.effect1Time;
			cent->pe.lastZombieSpirit = cg.time;
			cent->pe.nextZombieSpiritSound = cg.time + sndDelay;
			for ( i = 0; i < MAX_ZOMBIE_SPIRITS; i++ ) {
				cent->pe.zombieSpiritTrailHead[i] = -1;
				cent->pe.zombieSpiritRotationTimes[i] = minRotationTime + ( random() * ( maxRotationTime - minRotationTime ) );
				cent->pe.zombieSpiritRadiusCycleTimes[i] = minRadiusCycleTime + ( random() * ( maxRadiusCycleTime - minRadiusCycleTime ) );
				cent->pe.zombieSpiritStartTimes[i] = cent->currentState.effect1Time;
			}
		}
		cent->pe.zombieSpiritEndTime = cg.time;

	} else {

		// if running another effect, dont mess with its variables
		if ( cent->currentState.eFlags & EF_MONSTER_EFFECT3 || cent->currentState.effect1Time < cent->currentState.effect3Time ) {
			return;
		}

		if ( !cent->pe.zombieSpiritEndTime ) {
			return;
		}
		// clear the flag, and let the effect fade itself out
		cent->pe.cueZombieSpirit = qfalse;
	}

	for ( t = cent->pe.lastZombieSpirit + step; t <= cg.time; t += step ) {

		// add the spirits
		for ( i = 0; i < MAX_ZOMBIE_SPIRITS; i++ ) {

			if ( cent->pe.zombieSpiritTrailHead[i] < -1 ) {
				// spirit has gone, create a new one
				cent->pe.zombieSpiritTrailHead[i] = -1;
				cent->pe.zombieSpiritRotationTimes[i] = minRotationTime + ( random() * ( maxRotationTime - minRotationTime ) );
				cent->pe.zombieSpiritRadiusCycleTimes[i] = minRadiusCycleTime + ( random() * ( maxRadiusCycleTime - minRadiusCycleTime ) );
				cent->pe.zombieSpiritStartTimes[i] = cg.time - step;
			}

			fadeRatio = (float)( cg.time - cent->pe.zombieSpiritStartTimes[i] ) / (float)fadeInTime;
			if ( fadeRatio < 0.0 ) {
				fadeRatio = 0.0;
			}
			if ( fadeRatio > 1.0 ) {
				fadeRatio = 1.0;
			}

			if ( cent->pe.cueZombieSpirit ) {
				alpha = fadeRatio;
			} else {
				alpha = 1.0 - ( (float)( cg.time - cent->pe.zombieSpiritEndTime ) / (float)fadeOutTime );
				fadeRatio = alpha;
				if ( alpha < 0.0 ) {
					cent->pe.zombieSpiritTrailHead[i] = -2; // kill it
					continue;
				}
			}

			active = qtrue; // we have an active spirit, so continue effect

			alpha *= 0.3;

			if ( cent->pe.cueZombieSpirit ) {
				rotationTime = cent->pe.zombieSpiritRotationTimes[i];
				radiusCycleTime = cent->pe.zombieSpiritRadiusCycleTimes[i];

				radius = ( minDist + sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( radiusCycleTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % radiusCycleTime ) / (float)radiusCycleTime ) ) * ( maxDist - minDist ) );

				// get the position
				v[0] = ( 0.5 + 0.5 * fadeRatio ) * sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( rotationTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % rotationTime ) / (float)rotationTime ) ) * radius;
				v[1] = ( 0.5 + 0.5 * fadeRatio ) * cos( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( rotationTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % rotationTime ) / (float)rotationTime ) ) * radius;
				v[2] = 12 + 36 * ( 0.5 + 0.5 * fadeRatio ) * sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( zCycleTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % zCycleTime ) / (float)zCycleTime ) );
				v[2] -= ( 1.0 - fadeRatio ) * 32;

				VectorAdd( cent->lerpOrigin, v, p[i] );
			} else {
				// expand the radius, if it enters world geometry, kill it
				rotationTime = cent->pe.zombieSpiritRotationTimes[i];
				radiusCycleTime = cent->pe.zombieSpiritRadiusCycleTimes[i];

				radius = pow( 1.0 - fadeRatio, 2 ) * fadeDist + ( 1.0 - pow( 1.0 - fadeRatio, 2 ) ) * ( minDist + sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( radiusCycleTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % radiusCycleTime ) / (float)radiusCycleTime ) ) * ( maxDist - minDist ) );

				// get the position
				v[0] = sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( rotationTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % rotationTime ) / (float)rotationTime ) ) * radius;
				v[1] = cos( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( rotationTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % rotationTime ) / (float)rotationTime ) ) * radius;
				v[2] = 12 + 36 * sin( M_PI * 2 * (float)( (float)( (int)( t + ( (float)( zCycleTime * i ) / MAX_ZOMBIE_SPIRITS ) ) % zCycleTime ) / (float)zCycleTime ) );
				v[2] -= ( 1.0 - fadeRatio ) * 32;

				VectorAdd( cent->lerpOrigin, v, p[i] );

				// check for sinking into geometry
				trap_CM_BoxTrace( &trace, p[i], p[i], NULL, NULL, 0, MASK_SOLID );
				// if we hit something, clip the velocity, but maintain speed
				if ( trace.startsolid ) {
					cent->pe.zombieSpiritTrailHead[i] = -2; // kill it
					continue;
				}
			}

			VectorSubtract( p[i], cent->pe.zombieSpiritPos[i], cent->pe.zombieSpiritDir[i] );
			cent->pe.zombieSpiritSpeed[i] = 1000.0 / step * VectorNormalize( cent->pe.zombieSpiritDir[i] );
			VectorCopy( p[i], cent->pe.zombieSpiritPos[i] );

			cent->pe.zombieSpiritTrailHead[i] = CG_AddTrailJunc( cent->pe.zombieSpiritTrailHead[i],
																 cgs.media.zombieSpiritTrailShader,
																 t,
																 STYPE_STRETCH,
																 p[i],
																 trailLife * 2,
																 alpha,
																 0.0,
																 ( 0.5 + 0.5 * fadeRatio ) * idealWidth,
																 0,
																 TJFL_NOCULL,
																 colorWhite,
																 colorWhite,
																 1.0, 1 );

		}

		cent->pe.lastZombieSpirit = t;

		if ( !active ) {  // effect has gone
			cent->pe.zombieSpiritEndTime = 0;
		}
	}

	// add the skull at the head of the spirits
	memset( &refent, 0, sizeof( refent ) );
	refent.hModel = cgs.media.helgaGhostModel;
	refent.backlerp = 0;
	refent.renderfx = RF_NOSHADOW | RF_MINLIGHT;    //----(SA)
	refent.customShader = cgs.media.helgaSpiritSkullShader;
	refent.reType = RT_MODEL;
	for ( i = 0; i < MAX_ZOMBIE_SPIRITS; i++ ) {
		if ( cent->pe.zombieSpiritTrailHead[i] < -1 ) {
			continue;   // spirit has gone
		}
		fadeRatio = (float)( cg.time - cent->pe.zombieSpiritStartTimes[i] ) / (float)fadeInTime;
		if ( fadeRatio < 0.0 ) {
			fadeRatio = 0.0;
		}
		if ( fadeRatio > 1.0 ) {
			fadeRatio = 1.0;
		}

		if ( cent->pe.cueZombieSpirit ) {
			alpha = fadeRatio;
		} else {
			alpha = 1.0 - ( (float)( cg.time - cent->pe.zombieSpiritEndTime ) / (float)fadeOutTime );
			fadeRatio = alpha;
			if ( alpha < 0.0 ) {
				cent->pe.zombieSpiritTrailHead[i] = -2; // kill it
				continue;
			}
		}

		refent.shaderRGBA[3] = (byte)( 0.5 * alpha * 255.0 );
		VectorCopy( cent->pe.zombieSpiritPos[i], refent.origin );

		// HACK!!! skull model is back-to-front, need to fix
		//VectorInverse(cent->pe.zombieSpiritDir[i]);
		vectoangles( cent->pe.zombieSpiritDir[i], ang );
		//VectorInverse(cent->pe.zombieSpiritDir[i]);
		AnglesToAxis( ang, refent.axis );
/*		// create the non-normalized axis so we can size it
		refent.nonNormalizedAxes = qtrue;
		for (t=0; t<3; t++) {
			VectorNormalize( refent.axis[t] );
			VectorScale( refent.axis[t], 0.35, refent.axis[t] );
						}
*/                                                                                                                                                                                                                           //
		// add the sound
		trap_S_AddLoopingSound( -1, cent->lerpOrigin, vec3_origin, cgs.media.helgaSpiritLoopSound, fadeRatio );
		//
		// if this spirit is in a good position to be released and head to the enemy, then release it
		if ( fadeRatio == 1.0 && ( lastSpiritRelease > cg.time || ( lastSpiritRelease < cg.time - 1000 ) ) ) {
			VectorSubtract( cent->currentState.origin2, refent.origin, v );
			VectorNormalize( v );
			if ( DotProduct( cent->pe.zombieSpiritDir[i], v ) > 0.4 || ( cent->currentState.eFlags & EF_DEAD ) ) {
				// check for sinking into geometry
				trap_CM_BoxTrace( &trace, refent.origin, refent.origin, NULL, NULL, 0, MASK_SOLID );
				// if we hit something, don't release it yet
				if ( !trace.startsolid ) {
					if ( cent->pe.zombieSpiritSpeed[i] < 300 ) {
						cent->pe.zombieSpiritSpeed[i] = 300;
					}
					VectorScale( cent->pe.zombieSpiritDir[i], cent->pe.zombieSpiritSpeed[i], v );
					CG_SpawnHelgaSpirit( refent.origin, v, cent->pe.zombieSpiritTrailHead[i], cent->currentState.number, &refent, trailLife, idealWidth );
					lastSpiritRelease = cg.time;
					cent->pe.zombieSpiritTrailHead[i] = -2; // kill this version of it
					continue;
				}
			}
		}
		//
		// if we didn't kill it, draw it
		trap_R_AddRefEntityToScene( &refent );
	}

	if ( cg.time > cent->pe.nextZombieSpiritSound && cent->pe.cueZombieSpirit ) { //&& (cg.time < cent->pe.zombieSpiritStartTime + sndDuration)) {
		// spawn a new sound
		//trap_S_StartSound( cent->lerpOrigin, -1, CHAN_AUTO, cgs.media.helgaSpiritSound );
		CG_SoundPlayIndexedScript( cgs.media.helgaSpiritSound, NULL, cent->currentState.number );
		cent->pe.nextZombieSpiritSound = cg.time + sndIntervalMin + (int)( (float)( sndIntervalMax - sndIntervalMin ) * random() );
	}

	// add a negative light around us
	fadeRatio = (float)( cg.time - cent->pe.zombieSpiritStartTime ) / (float)fadeInTime;
	if ( fadeRatio < 0.0 ) {
		fadeRatio = 0.0;
	}
	if ( fadeRatio > 1.0 ) {
		fadeRatio = 1.0;
	}

	if ( cent->pe.cueZombieSpirit ) {
		alpha = fadeRatio;
	} else {
		alpha = 1.0 - ( (float)( cg.time - cent->pe.zombieSpiritEndTime ) / (float)fadeOutTime );
		fadeRatio = alpha;
		if ( alpha < 0.0 ) {
			cent->pe.zombieSpiritEndTime = 0;   // stop the effect
			return;
		}
	}
	fadeRatio *= 0.7;
	trap_R_AddLightToScene( cent->lerpOrigin, 500.0, 1.0 * fadeRatio, 1.0 * fadeRatio, 1.0 * fadeRatio, 10 );
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

	cent = &cg_entities[es->number];

	ent->entityNum = es->number;

	if ( cent->pe.forceLOD ) {
		ent->reFlags |= REFLAG_FORCE_LOD;
	}

	// RF, if in camera mode, force a full lod, since we are in a controlled environment
	if ( cg.cameraMode ) {
		ent->reFlags |= REFLAG_FULL_LOD;
	}

//----(SA)	testing
	// (SA) disabling
//	if(cent->currentState.eFlags & EF_DEAD) {
//		ent->reFlags |= REFLAG_DEAD_LOD;
//	}
//----(SA)	end

	backupRefEnt = *ent;

	if ( powerups & ( 1 << PW_INVIS ) ) {
		ent->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( ent );
#if 0
		// -------------------------------
		// Zombie effects
		//
	} else if ( es->aiChar == AICHAR_ZOMBIE ) {

		// Zombie needs special processing, to remove the bits of flesh that have been torn away

		if ( ent->hModel == cent->pe.torsoRefEnt.hModel ) {
			//ent->reFlags = REFLAG_ZOMBIEFX;
			ent->shaderTime = 0;
		} else if ( ent->hModel == cent->pe.legsRefEnt.hModel )     {
			//ent->reFlags = REFLAG_ZOMBIEFX2;	// ref needs to know this is the legs
			ent->shaderTime = 0;
		}

		// first, check for portal spawning
		if ( es->time2 ) {
			if ( es->time2 < cg.time ) {
				return; // not ready yet
			}
			// fade in the skeleton, skin should "compose" (reverse decomposition)
			alpha = (float)( cg.time - es->time2 ) / PORTAL_ZOMBIE_SPAWNTIME;
			if ( alpha > 1 ) {
				alpha = 1;
			}
			// skeleton fades in towards end of effect
			if ( alpha < 0.5 ) {
				ent->shaderRGBA[3] = 0;
			} else { ent->shaderRGBA[3] = ( unsigned char )( ( alpha - 0.5 ) * 2.0 * 255 );}
			//
			ent->shaderTime = 0.001 * ( 1.0 - alpha ) * ZOMBIEFX_FADEOUT_TIME;

			if ( ent->hModel == cent->pe.headRefEnt.hModel ) {
				//ent->reFlags = REFLAG_ZOMBIEFX;
				ent->customShader = cgs.media.zombieHeadFadeShader;
			}

			trap_R_AddRefEntityToScene( ent );

			// add flaming effect
			onFire = qtrue;
			alpha *= 0.5;

		} else {

			// if the Zombie is dead, the skin should decompose

			ent->shaderRGBA[3] = 255;

			if ( es->eFlags & EF_MONSTER_EFFECT2 &&
				 cent->currentState.effect2Time < cg.time ) { // Ridah, Zombie death effect

				if ( ent->hModel == cent->pe.headRefEnt.hModel ) {
					//ent->reFlags = REFLAG_ZOMBIEFX;
					ent->customShader = cgs.media.zombieHeadFadeShader;
				}

				ent->shaderTime = 0.001 * ( cg.time - cent->currentState.effect2Time );
				trap_R_AddRefEntityToScene( ent );
/*
				// skeleton: add legs and head parts
				if (ent->hModel == cent->pe.legsRefEnt.hModel) {
					ent->skinNum = 0;
					ent->reFlags = 0;
					ent->customShader = cgs.media.skeletonSkinShader;

					// legs
					ent->hModel = cgs.media.skeletonLegsModel;
					trap_R_AddRefEntityToScene( ent );

					// torso (just get this so we can place the head correctly)
					parentEnt = *ent;
					CG_PositionEntityOnTag( ent, &parentEnt, parentEnt.hModel, "tag_torso", NULL );
					ent->hModel = cgs.media.skeletonTorsoModel;

					// head
					parentEnt = *ent;
					CG_PositionEntityOnTag( ent, &parentEnt, parentEnt.hModel, "tag_head", NULL );
					ent->hModel = cgs.media.skeletonHeadModel;
					trap_R_AddRefEntityToScene( ent );
				}
*/
			} else {    // show it normally
				trap_R_AddRefEntityToScene( ent );
			}
		}

		// restore previous state
		*ent = backupRefEnt;
#endif
/*
	} else if (es->eFlags & EF_MONSTER_EFFECT2 && es->aiChar == AICHAR_ZOMBIE &&
				cent->currentState.effect2Time < cg.time) {	// Ridah, Zombie death effect
		const int fadeRiseTime = 4000;

		if (cent->pe.zombieDeathFadeStart < cg.time && ent->hModel == cent->pe.legsRefEnt.hModel) {
			// add the skeleton models starting with the legs
			ent->fadeEndTime = 0;
			ent->fadeStartTime = 0;
			ent->skinNum = 0;

			// legs
			ent->hModel = cgs.media.skeletonLegsModel;
			ent->customSkin = cgs.media.skeletonLegsSkin;
			trap_R_AddRefEntityToScene( ent );

			// torso
			parentEnt = *ent;
			CG_PositionEntityOnTag( ent, &parentEnt, parentEnt.hModel, "tag_torso", NULL );
			ent->hModel = cgs.media.skeletonTorsoModel;
			ent->customSkin = cgs.media.skeletonTorsoSkin;
			trap_R_AddRefEntityToScene( ent );

			// head
			parentEnt = *ent;
			CG_PositionEntityOnTag( ent, &parentEnt, parentEnt.hModel, "tag_head", NULL );
			ent->hModel = cgs.media.skeletonHeadModel;
			ent->customSkin = cgs.media.skeletonHeadSkin;
			trap_R_AddRefEntityToScene( ent );

			// restore previous state
			*ent = backupRefEnt;
		}

		if (cent->pe.zombieDeathFadeEnd + fadeRiseTime > cg.time) {
			// slowly fade the zombie "skin" out, revealing the skeleton underneath

			//VectorSubtract( ent->origin, cg.snap->ps.origin, ent->fireRiseDir );
			//VectorNegate( ent->axis[0], ent->fireRiseDir );
			VectorNormalize2( ent->axis[0], ent->fireRiseDir );

			if (cent->pe.zombieDeathFadeEnd > cg.time) {

				// the zombie has hard-edged alpha blending on it's body texture by default
				// so we need to override that for smooth fading
				if (ent->hModel == cent->pe.legsRefEnt.hModel ||
					ent->hModel == cent->pe.torsoRefEnt.hModel) {
					ent->customShader = cgs.media.zombieBodyFadeShader;
				} else if (ent->hModel == cent->pe.headRefEnt.hModel) {
					ent->customShader = cgs.media.zombieHeadFadeShader;
				}
				// fade the alpha from 0 -> 255 as the time goes, which will fade from front to back
				if (cent->pe.zombieDeathFadeStart > cg.time) {
					ent->shaderRGBA[3] = 128;
				} else {
					ent->shaderRGBA[3] = 128 - (unsigned char)(128.0*pow(((float)(cg.time - cent->pe.zombieDeathFadeStart) / (float)(cent->pe.zombieDeathFadeEnd - cent->pe.zombieDeathFadeStart)), 2));
				}
				ent->shaderTime = 1.0;

				trap_R_AddRefEntityToScene( ent );
			}
		}
*/
		// -------------------------------
	} else {

		if ( CG_EntOnFire( &cg_entities[es->number] ) ) {
			ent->reFlags |= REFLAG_FORCE_LOD;
		}

		trap_R_AddRefEntityToScene( ent );

		if ( powerups & ( 1 << PW_QUAD ) ) {
			if ( team == TEAM_RED ) {
				ent->customShader = cgs.media.redQuadShader;
			} else {
				ent->customShader = cgs.media.quadShader;
			}
			trap_R_AddRefEntityToScene( ent );
		}
		if ( powerups & ( 1 << PW_REGEN ) ) {
			if ( ( ( cg.time / 100 ) % 10 ) == 1 ) {
				ent->customShader = cgs.media.regenShader;
				trap_R_AddRefEntityToScene( ent );
			}
		}
		if ( powerups & ( 1 << PW_BATTLESUIT ) ) {
			ent->customShader = cgs.media.battleSuitShader;
			trap_R_AddRefEntityToScene( ent );
		}
	}

	if ( !onFire && CG_EntOnFire( &cg_entities[es->number] ) ) {
		onFire = qtrue;
		// set the alpha
		alpha = ( cg.time - es->onFireStart ) / 1500.0;
		if ( alpha > 1.0 ) {
			alpha = ( es->onFireEnd - cg.time ) / 1500.0;
			if ( alpha > 1.0 ) {
				alpha = 1.0;
			}
		}
	}
	// Flaming zombie always shows a little fire
	if ( !es->time2 && alpha < 1.0 && ( cent->currentState.aiChar == AICHAR_ZOMBIE ) && IS_FLAMING_ZOMBIE( cent->currentState ) /*&& !(cent->currentState.eFlags & EF_DEAD)*/ ) {
		onFire = qtrue;
		// set the alpha
		alpha = 1.0;
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
/*
		if (ent->fireRiseDir[2] > 0)
			ent->fireRiseDir[2] *= -1;

		ent->customShader = cgs.media.dripWetShader2;
		trap_R_AddRefEntityToScene( ent );

		ent->customShader = cgs.media.dripWetShader;
		trap_R_AddRefEntityToScene( ent );

		VectorCopy( fireRiseDir, ent->fireRiseDir );
*/
		ent->customShader = cgs.media.onFireShader;
		trap_R_AddRefEntityToScene( ent );

		ent->customShader = cgs.media.onFireShader2;
		trap_R_AddRefEntityToScene( ent );

		if ( ent->hModel == cent->pe.legsRefEnt.hModel ) {
			trap_S_AddLoopingSound( es->number, ent->origin, vec3_origin, cgs.media.flameCrackSound, (int)( 40.0 * alpha ) );
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


#define SPINNER_SPEED 0.3f
/*
==============
CG_SpinnerSpinAngle
==============
*/
static float CG_SpinnerSpinAngle( centity_t *cent ) {
	int delta;

	if ( cent->currentState.eFlags & EF_DEAD ) {  // don't spin for dead loper (TODO: spindown, or blow off parts rather than just stopping)
		return cent->pe.spinnerAngle;
	}

	delta = cg.time - cent->pe.spinnerTime;

	return -( cent->pe.spinnerAngle + delta * SPINNER_SPEED );
}

/*
==============
CG_AddFireLight
==============
*/
static void CG_AddFireLight( centity_t *cent ) {

	return;

/* OPTIMIZATION, TOO MANY DLIGHTS WHEN FIRE IS AROUND

	entityState_t *es;
	float alpha;

	if (CG_EntOnFire(&cg_entities[cent->currentState.number])) {

		es = &cent->currentState;

		// set the alpha
		alpha = (cg.time - es->onFireStart) / 1500.0;
		if (alpha > 1.0) {
			alpha = (es->onFireEnd - cg.time) / 1500.0;
			if (alpha > 1.0) {
				alpha = 1.0;
			}
		}
		if (alpha <= 0.0) return;

		trap_R_AddLightToScene( cent->lerpOrigin, 128 + 128*alpha, 1.000000*alpha, 0.603922*alpha, 0.207843*alpha, 0 );
	}
*/
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
//	int	legsAnim;

	if ( cg.snap && cg.snap->ps.clientNum == cent->currentState.number && !cg.renderingThirdPerson ) {
		return;
	}

	es = &cent->currentState;
	// DHM-Nerve
	//ci = &cgs.clientinfo[es->number];			// es->number is not always a valid client num
	ci = &cgs.clientinfo[es->clientNum];
	// dhm-Nerve

	// WEAPON
	BG_UpdateConditionValue( es->clientNum, ANIM_COND_WEAPON, es->weapon, qtrue );

	// MOUNTED
	if ( es->eFlags & EF_MG42_ACTIVE ) {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_MOUNTED, MOUNTED_MG42, qtrue );
	} else {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_MOUNTED, MOUNTED_UNUSED, qtrue );
	}

	// UNDERHAND
	BG_UpdateConditionValue( es->clientNum, ANIM_COND_UNDERHAND, cent->lerpAngles[0] > 0, qtrue );

	// LEANING
	/* TODO???
	if (es->lean > 0) {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_LEANING, LEANING_RIGHT, qtrue );
	} else if (es->lean < 0) {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_LEANING, LEANING_LEFT, qtrue );
	} else {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_LEANING, LEANING_UNUSED, qtrue );
	}
	*/

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
	//legsAnim = es->legsAnim & ~ANIM_TOGGLEBIT;
	//if (ci->modelInfo->animations[legsAnim].movetype) {
	//	BG_UpdateConditionValue( es->clientNum, ANIM_COND_MOVETYPE, ci->modelInfo->animations[legsAnim].movetype, qfalse );
	//}
	// RF, changed this since we dont need to be careful about bandwidth anymore, and this method
	// is much more accurate
	if ( cent->currentState.animMovetype ) {
		BG_UpdateConditionValue( es->clientNum, ANIM_COND_MOVETYPE, cent->currentState.animMovetype, qtrue );
	}
}

void CG_DeadSink( centity_t *cent ) {
	if ( cent->currentState.aiChar != AICHAR_WARZOMBIE ) {
		return;
	}
	if ( !( cent->currentState.eFlags & EF_DEAD ) ) {
		return;
	}
	if ( !cent->currentState.effect3Time ) {
		return;
	}
	if ( cent->currentState.effect3Time >= cg.time ) {
		return;
	}
	// sink
	cent->lerpOrigin[2] -= DEAD_SINK_DEPTH * ( (float)( cg.time - cent->currentState.effect3Time ) / DEAD_SINK_DURATION );
}

/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent ) {
	int i;
	clientInfo_t    *ci;
	refEntity_t legs;
	refEntity_t torso;
	refEntity_t head;
	refEntity_t acc;

	vec3_t lightorigin;

	int clientNum;
	int renderfx;
	qboolean shadow;       //, drawweap = qtrue; // TTimo: unused
	float shadowPlane;

	float gumsflappin = 0;              // talking amplitude

	centity_t   *cgsnap;

	cgsnap = &cg_entities[cg.snap->ps.clientNum];

	shadow = qfalse;                                                // gjd added to make sure it was initialized
	shadowPlane = 0.0;                                              // ditto

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

//----(SA)	we need to not see the player in the camera
//	if (cg.cameraMode && clientNum == cg.snap->ps.clientNum) {
//		return;
//	}
//----(SA)	end

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid ) {
		return;
	}

	// check time
	if ( cent->pe.lastTime > cg.time ) {
		CG_ResetPlayerEntity( cent );
	}
	cent->pe.lastTime = cg.time;

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
//	if(cg.cameraMode)
//		renderfx &= ~RF_THIRD_PERSON;

	if ( cg_shadows.integer == 3 && shadow ) {
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;         // use the same origin for all

	// set renderfx for accessories
	acc.renderfx    = renderfx;

//CG_Printf("%i cl_org: %s\n", clientNum, vtosf(cent->lerpOrigin) );

	VectorCopy( cent->lerpOrigin, lightorigin );
	lightorigin[2] += 31 + (float)cg_drawFPGun.integer;

	// dead sink?
	CG_DeadSink( cent );

	//
	// add special monster effects here
	//
	CG_AddZombieSpiritEffect( cent );
	CG_AddZombieFlameEffect( cent );
	CG_AddZombieFlameShort( cent );
	CG_AddLoperLightningEffect( cent );
	CG_AddLoperGroundEffect( cent );
	CG_AddHelgaSpiritEffect( cent );

	//
	// add dynamic lights, and other misc effects
	//
	CG_AddFireLight( cent );

	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	VectorCopy( cent->lerpOrigin, legs.origin );

	if ( ci->playermodelScale[0] != 0 ) {  // player scaled, adjust for the (-24) offset of player legs origin to ground
		legs.origin[2] -= 24.0f * ( 1.0f - ci->playermodelScale[2] );
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

		if (     ( cgsnap == cent && ( cg.snap->ps.pm_flags & PMF_LADDER ) )
				 ||  ( cent->currentState.aiChar == AICHAR_LOPER ) ) {
			CG_PositionEntityOnTag( &torso, &legs, "tag_torso", 0, NULL );
		} else {
			CG_PositionRotatedEntityOnTag( &torso,  &legs, "tag_torso" );
		}

	} else {    // just clear out the angles

		if (     ( cgsnap == cent && ( cg.snap->ps.pm_flags & PMF_LADDER ) )
				 ||  ( cent->currentState.aiChar == AICHAR_LOPER ) ) {
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


//	if ( cent->currentState.eFlags & EF_DEAD) {
//		if(cent->currentState.eFlags & EF_HEADSHOT)
//		// dead guy with no head
//			return;
//	}

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


	// Ridah, talking animations
	if ( !( cent->currentState.eFlags & EF_DEAD ) ) {
		#define HEAD_EMOTION_SUBTYPES   8       // closed, A, O, I, E
		int talk_frame; //, subtype;
		qboolean closed;

		#define NUM_EMOTIONS            2   // 0 neutral, 1 happy, 2 angry
		int emotion = 0;  // this should default to the entity's current emotion

		gumsflappin = (float)trap_S_GetVoiceAmplitude( clientNum );
		talk_frame = (int)floor( ( HEAD_EMOTION_SUBTYPES - 1 ) * ( gumsflappin / 256.0 ) );

		// add the current frame to the total, so when it comes to pick a new frame, we choose the average
		// of those we missed over the last frame
		cent->pe.head.animationTime += talk_frame;
		cent->pe.head.animationNumber++;

		// if we are starting a talk after idling, hurry up the animation
		if ( ( cent->pe.head.frameTime > cg.time + 30 ) && cent->pe.head.animationTime && !( ( cent->pe.head.frame - ci->modelInfo->headAnims[0].firstFrame ) % HEAD_EMOTION_SUBTYPES ) && !( ( cent->pe.head.oldFrame - ci->modelInfo->headAnims[0].firstFrame ) % HEAD_EMOTION_SUBTYPES ) ) {
			cent->pe.head.frameTime = cg.time + 30;
		}

		if ( cent->pe.head.frameTime < cg.time ) { // set the new frame
			cent->pe.head.oldFrame = cent->pe.head.frame;
			cent->pe.head.oldFrameTime = cent->pe.head.frameTime;

			if ( cent->pe.head.animationTime ) {

				talk_frame = (int)( (float)cent->pe.head.animationTime / (float)cent->pe.head.animationNumber );
				//subtype = rand()%HEAD_EMOTION_SUBTYPES;
				emotion = rand() % NUM_EMOTIONS;      // this helps animate the mouth more realistically while talking

				switch ( cent->currentState.aiChar ) {
				case AICHAR_ZOMBIE:
				case AICHAR_LOPER:
					talk_frame = (int)( (float)talk_frame * 1.2 );
					closed = qfalse;
					break;
				default:
					// randomly set it back to 0 amplitude to simulate certain synonyms pronounced with a closed mouth
					closed = ( ( rand() % 5 ) == 0 ) && ( talk_frame < ( HEAD_EMOTION_SUBTYPES - 1 ) );
				}

				if ( closed ) {
					talk_frame -= rand() % ( HEAD_EMOTION_SUBTYPES / 2 );
					if ( talk_frame < 0 ) {
						talk_frame = 0;
					}
				}

				if ( talk_frame >= HEAD_EMOTION_SUBTYPES ) {
					talk_frame = HEAD_EMOTION_SUBTYPES - 1;
				}

				cent->pe.head.frame = emotion * HEAD_EMOTION_SUBTYPES + talk_frame; //ci->modelInfo->headAnims[emotion*HEAD_EMOTION_SUBTYPES].firstFrame + talk_frame;
				cent->pe.head.frameTime = cg.time + 80 + rand() % 40; // interpolate for smoother animation vs latency

				//CG_Printf("%i head: frame %i, oldframe %i, nextframetime %i\n", cg.time, cent->pe.head.frame, cent->pe.head.oldFrame, cent->pe.head.frameTime );

				//if (closed)
				//	cent->pe.head.frameTime += 30;		// slow it down a bit
			} else {
#if 0
				// debugging, play all the frames and display the frame numbers
				if ( ++cent->pe.head.frame > ( ci->headAnims[( NUM_EMOTIONS - 1 ) * HEAD_EMOTION_SUBTYPES + HEAD_EMOTION_SUBTYPES - 1].firstFrame ) ) {
					cent->pe.head.frame = ci->headAnims[0].firstFrame;
				}
				cent->pe.head.frameTime = cg.time + 2000;
				CG_Printf( "%d - %d\n", cent->currentState.number, cent->pe.head.oldFrame );
#else
				while ( ( emotion = rand() % NUM_EMOTIONS ) == 1 ) ; // don't use happy emotion
				if ( ( cent->pe.head.frame - ci->modelInfo->headAnims[0].firstFrame ) % HEAD_EMOTION_SUBTYPES ) {
					cent->pe.head.frame = ci->modelInfo->headAnims[emotion * HEAD_EMOTION_SUBTYPES].firstFrame;
					cent->pe.head.frameTime = cg.time + 150;
				} else {    // mouth is currently closed
					cent->pe.head.frame = ci->modelInfo->headAnims[emotion * HEAD_EMOTION_SUBTYPES].firstFrame;
					cent->pe.head.frameTime = cent->pe.head.frameTime + 1000 + rand() % 2000;
					if ( cent->pe.head.frameTime < cg.time ) {
						cent->pe.head.frameTime = cg.time + 1000;
					}
				}
#endif
			}

			cent->pe.head.animationTime = 0;
			cent->pe.head.animationNumber = 0;
		}

		head.frame = cent->pe.head.frame;
		head.oldframe = cent->pe.head.oldFrame;
		head.backlerp = 1.0 - (float)( cg.time - cent->pe.head.oldFrameTime ) / ( cent->pe.head.frameTime - cent->pe.head.oldFrameTime );

	} else {    // dead
		head.frame = 0;
		head.oldframe = 0;
		head.backlerp = 0.0;
	}
	// done.



	// set blinking flag
	if ( cent->currentState.eFlags & EF_DEAD ) {
		// dead guy, eyes closed.
		head.renderfx |= RF_BLINK;

	} else if ( ci->blinkTime <= cg.time ) {

		head.renderfx |= RF_BLINK;

		if ( ci->blinkTime <= ( cg.time - cg_blinktime.integer ) ) {
			ci->blinkTime = cg.time + 500 + random() * 4000;

			// blink more often when talking
			if ( gumsflappin >= 0 ) {
				ci->blinkTime = max( cg.time, ci->blinkTime - 1000 );
			}
		}
	}


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
	CG_AddPlayerWeapon( &torso, NULL, cent );

	cent->lastWeaponClientFrame = cg.clientFrame;

	//
	// add binoculars (if it's not the player)
	//
	if ( ( cent->currentState.clientNum != cg.snap->ps.clientNum ) &&
		 cent->currentState.eFlags & EF_ZOOMING ) {

		acc.hModel = cgs.media.thirdPersonBinocModel;
		CG_PositionEntityOnTag( &acc, &torso, "tag_weapon", 0, NULL );
		CG_AddRefEntityWithPowerups( &acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );
	}

	if ( ( cent->currentState.clientNum != cg.snap->ps.clientNum ) &&
		 cent->currentState.eFlags & EF_CIG ) {

		acc.hModel = cgs.media.cigModel;
		CG_PositionEntityOnTag( &acc, &torso, "tag_weapon2", 0, NULL );
		CG_AddRefEntityWithPowerups( &acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );

		// smoke
		if ( !cg_paused.integer ) {    // don't add while paused
			if ( !( rand() % 3 ) ) {
				CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, acc.origin, tv( 0,0,1 ), 1, 1000, 6, 4, 10, 0.15f );
			}
		}
	}



	//
	// add player specific models
	//

	if ( cent->currentState.aiChar == AICHAR_LOPER ) {
		if ( ci->partModels[8] ) {
			vec3_t angles;

			acc.hModel = ci->partModels[8];
			VectorClear( angles );
			angles[YAW] = CG_SpinnerSpinAngle( cent );
			AnglesToAxis( angles, acc.axis );
			CG_PositionRotatedEntityOnTag( &acc, &legs, "tag_spinner" );
			CG_AddRefEntityWithPowerups( &acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );
		}
	}
//----(SA)	modified
	else if (   cent->currentState.aiChar == AICHAR_PROTOSOLDIER ||
				cent->currentState.aiChar == AICHAR_SUPERSOLDIER ||
				cent->currentState.aiChar == AICHAR_HEINRICH ) {

		char *protoTags[] = {   "tag_chest",
								"tag_calfleft",
								"tag_armleft",
								"tag_back",
								"tag_legleft",
								"tag_calfright",
								"tag_armright",
								"tag_back",
								"tag_legright"};

		char *ssTags[] = {      "tag_chest",
								"tag_calfleft",
								"tag_armleft",
								"tag_back",
								"tag_legleft",
								"tag_calfright",
								"tag_armright",
								"tag_back",
								"tag_legright",

								"tag_footleft",
								"tag_footright",
								"tag_sholeft",
								"tag_shoright",
								"tag_torso",
								"tag_calfleft",
								"tag_calfright"};

		char *heinrichTags[] = {"tag_chest",
								"tag_calfleft",
								"tag_armleft",
								"tag_back",
								"tag_legleft",
								"tag_calfright",
								"tag_armright",
								"tag_back",
								"tag_legright",

								"tag_footleft",
								"tag_footright",
								"tag_sholeft",
								"tag_shoright",
								"tag_torso",
								"tag_legleft",
								"tag_legright",

								"tag_sholeft",
								"tag_shoright",
								"tag_legleft",
								"tag_legright",
								"tag_legleft",
								"tag_legright"};

		// TTimo: init
		int totalparts = 0, dynamicparts = 0, protoParts = 9, superParts = 16, heinrichParts = 22;
		char        **tags = NULL;
		qhandle_t   *models = NULL;
		int dmgbits = 16;         // 32/2;

		if ( cent->currentState.aiChar == AICHAR_PROTOSOLDIER ) {
			tags = &protoTags[0];
			models = &cgs.media.protoArmor[0];
			dynamicparts = totalparts = protoParts;
		} else if ( cent->currentState.aiChar == AICHAR_SUPERSOLDIER ) {
			tags = &ssTags[0];
			models = &cgs.media.superArmor[0];
			dynamicparts = 14;  // the other two stay permanent
			totalparts = superParts;
		} else if ( cent->currentState.aiChar == AICHAR_HEINRICH ) {
			tags = &heinrichTags[0];
			models = &cgs.media.heinrichArmor[0];
			dynamicparts = 20;  // will get kicked down to 16
			totalparts = heinrichParts;
		}

		if ( dynamicparts > dmgbits ) {
			dynamicparts = dmgbits;
		}

		for ( i = 0; i < totalparts; i++ ) {
			if ( ( i >= dynamicparts ) || ( !( cent->currentState.dmgFlags & ( 1 << i ) ) ) ) {    // ones beyond 16 just draw the good part
				acc.hModel = models[i];
			} else {
				if ( cent->currentState.dmgFlags & ( 1 << ( i + totalparts ) ) ) {
					acc.hModel = models[i + totalparts];
				} else {
					acc.hModel = models[i + ( 2 * totalparts )];
				}
			}

			if ( !acc.hModel ) {
				continue;
			}

			CG_PositionEntityOnTag( &acc, &torso, tags[i], 0, NULL );

			if ( cent->currentState.aiChar == AICHAR_PROTOSOLDIER && !( cent->currentState.eFlags & EF_DEAD ) ) {
				if ( acc.hModel != models[i] ) {
					vec3_t dir;
					int mynum;

					VectorSubtract( acc.origin, cent->currentState.pos.trBase, dir );
					dir[2] += 20;
					VectorNormalize( dir );

					mynum = ( rand() % 100 );
					if ( mynum < 2 ) {
						CG_AddBulletParticles( acc.origin, dir, 30, 10 * mynum, 3, 300.0f );
					}
				}
			}

			CG_AddRefEntityWithPowerups( &acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );
		}
	} else if ( cent->currentState.aiChar == AICHAR_WARZOMBIE &&
				( !Q_strcasecmp( (char *)ci->modelInfo->modelname, "dark" ) ) ) {
		// TTimo: unused
		/*
			char *tags[] = {		"tag_armleft",
									"tag_armright",
									"tag_back",
									"tag_back",
									"tag_calfleft",
									"tag_calfleft",
									"tag_calfright",
									"tag_calfright",
									"tag_chest",
									"tag_chest",
									"tag_footleft",
									"tag_footright",
									"tag_legleft",
									"tag_sholeft",
									"tag_shoright",
									"tag_torso"
									};
					*/

// TTimo: unused
		/*
int parts[] = { 34,
				38,
				0,
				19,
				0,
				14,
				21,
				15,
				16,
				0,
				0,
				32,
				0,
				33,
				0,
				45
				};
	*/

//		char *parts[] = {		"dam_lftforarm2",//	34
//								"dam_rtforarm2",//38
//								"dam_rtshoulder",
//								"dam_lftshoulder1",//19
//								"dam_lftcalf",
//								"nodam_lftknee",//14
//								"dam_rtcalf1",//21
//								"nodam_rtknee",//15
//								"dam_chest1",//16
//								"dam_chest3",
//								"dam_lftfoot",
//								"dam_rtfoot2",//32
//								"dam_lftthigh",
//								"dam_lftuparm2",//33
//								"dam_rtuparm",
//								"dam_waist2"//45
//								};


// do not turn on unless asked for
/*
		for(i=0;i<16;i++) {
			acc.hModel = cgs.media.superArmor[parts[i]];
			CG_PositionEntityOnTag( &acc, &torso, tags[i], 0, NULL);
			CG_AddRefEntityWithPowerups( &acc, cent->currentState.powerups, ci->team, &cent->currentState, cent->fireRiseDir );
		}
*/
//
	}



//#if 1
#ifdef TEST_HEADLIGHT
	// used for testing spotlights.
	// this just puts one in the mouth of every other player so you can
	// get a good read of how well the various elements of spots are working
//	if(cent->currentState.number != cg.predictedPlayerState.clientNum)
	{
		vec3_t morg, viewDir;
//		vec4_t	color = {1,1,1,0.1f};
		vec4_t color = {0.7,0.7,0.7,0.1f};

		CG_GetOriginForTag( cent, &head, "tag_mouth", 0, morg, NULL );
		AngleVectors( cent->lerpAngles, viewDir, NULL, NULL );
		CG_Spotlight( cent, color, morg, viewDir, 12, 512, 2, 5, SL_TRACEWORLDONLY | SL_NOCORE | SL_LOCKUV ); // SL_NOTRACE
//		color[0] = 1;
//		color[1] = 1;
//		color[2] = 1;
		color[3] = 0.1f;
		CG_Spotlight( cent, color, morg, viewDir, 12, 512, 1.5, 2, SL_TRACEWORLDONLY | SL_NOCORE | SL_NODLIGHT | SL_LOCKUV );   // SL_NOTRACE
	}
#endif


	//
	// add accessories
	//

	for ( i = ACC_BELT_LEFT; i < ACC_MAX; i++ ) {
		if ( !( ci->accModels[i] ) ) {
			continue;
		}

		// first 8 can be hidden by animation scripts
		if ( i < 8 ) {
			if ( cg.snap->ps.accHideBits & ( 1 << i ) ) {
				continue;
			}
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

	for ( i = 0; i < 8; i++ ) {
		if ( !( ci->partModels[i] ) ) {
			continue;
		}

		// first 8 can be hidden by animation scripts
		if ( !( cg.snap->ps.accShowBits & ( 1 << i ) ) ) {
			continue;
		}

		acc.hModel = ci->partModels[i]; // set the model
		if ( ci->partSkins[i] ) {
			acc.customSkin = ci->partSkins[i];  // and the skin if there is one

		}
		CG_PositionEntityOnTag( &acc, &legs, va( "tag_animscript%s", i ), 0, NULL );
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
		cent->pe.legs.yawAngle = cent->nextState.apos.trBase[YAW]; //cent->rawAngles[YAW];
		cent->pe.legs.yawing = qfalse;
		cent->pe.legs.pitchAngle = cent->nextState.apos.trBase[PITCH];
		cent->pe.legs.pitching = qfalse;

		memset( &cent->pe.torso, 0, sizeof( cent->pe.legs ) );
		cent->pe.torso.yawAngle = cent->nextState.apos.trBase[YAW]; //cent->rawAngles[YAW];
		cent->pe.torso.yawing = qfalse;
		cent->pe.torso.pitchAngle = cent->nextState.apos.trBase[PITCH]; //cent->rawAngles[PITCH];
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
	//int				clientNum;
	centity_t       *cent, backupCent;

	// clientNum = cg.snap->entities[fleshEntityNum].clientNum;
	ci = &cgs.clientinfo[ fleshEntityNum ];

	// cent = &cg_entities[ cg.snap->entities[fleshEntityNum].number ];
	cent = &cg_entities [ fleshEntityNum ];
	backupCent = *cent;

	//	cent = &cg_entities [ cg.snap->entities [ fleshEntityNum - 1].clientNum ];

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
//		VectorCopy( cg.snap->entities[fleshEntityNum - 1].pos.trBase, legs.origin );
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

	// add the origin of the entity
	//VectorAdd( refent->origin, or->origin, or->origin );

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

	// add the origin of the entity
	//VectorAdd( refent->origin, or->origin, or->origin );

	// rotate with entity
	MatrixMultiply( refent->axis, or->axis, tempAxis );
	memcpy( or->axis, tempAxis, sizeof( vec3_t ) * 3 );

	return qtrue;
}
