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



// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"
#include "../ui/ui_shared.h" // bk001205 - for Q3_ui as well


/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores( void ) {
	int i, powerups;

	cg.numScores = atoi( CG_Argv( 1 ) );
	if ( cg.numScores > MAX_CLIENTS ) {
		cg.numScores = MAX_CLIENTS;
	}

	cg.teamScores[0] = atoi( CG_Argv( 2 ) );
	cg.teamScores[1] = atoi( CG_Argv( 3 ) );

	memset( cg.scores, 0, sizeof( cg.scores ) );
	for ( i = 0 ; i < cg.numScores ; i++ ) {
		//
		cg.scores[i].client = atoi( CG_Argv( i * 6 + 4 ) );
		cg.scores[i].score = atoi( CG_Argv( i * 6 + 5 ) );
		cg.scores[i].ping = atoi( CG_Argv( i * 6 + 6 ) );
		cg.scores[i].time = atoi( CG_Argv( i * 6 + 7 ) );
		cg.scores[i].scoreFlags = atoi( CG_Argv( i * 6 + 8 ) );
		powerups = atoi( CG_Argv( i * 6 + 9 ) );
		// DHM - Nerve :: the following parameters are not sent by server
		/*
		cg.scores[i].accuracy = atoi(CG_Argv(i * 14 + 10));
		cg.scores[i].impressiveCount = atoi(CG_Argv(i * 14 + 11));
		cg.scores[i].excellentCount = atoi(CG_Argv(i * 14 + 12));
		cg.scores[i].guantletCount = atoi(CG_Argv(i * 14 + 13));
		cg.scores[i].defendCount = atoi(CG_Argv(i * 14 + 14));
		cg.scores[i].assistCount = atoi(CG_Argv(i * 14 + 15));
		cg.scores[i].perfect = atoi(CG_Argv(i * 14 + 16));
		cg.scores[i].captures = atoi(CG_Argv(i * 14 + 17));
		*/

		if ( cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS ) {
			cg.scores[i].client = 0;
		}
		cgs.clientinfo[ cg.scores[i].client ].score = cg.scores[i].score;
		cgs.clientinfo[ cg.scores[i].client ].powerups = powerups;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;
	}
#ifdef MISSIONPACK
	CG_SetScoreSelection( NULL );
#endif

}

/*
=================
CG_ParseTeamInfo

=================
*/
static void CG_ParseTeamInfo( void ) {
	int i;
	int client;

	numSortedTeamPlayers = atoi( CG_Argv( 1 ) );

	for ( i = 0 ; i < numSortedTeamPlayers ; i++ ) {
		client = atoi( CG_Argv( i * 6 + 2 ) );

		sortedTeamPlayers[i] = client;

		cgs.clientinfo[ client ].location = atoi( CG_Argv( i * 6 + 3 ) );
		cgs.clientinfo[ client ].health = atoi( CG_Argv( i * 6 + 4 ) );
		cgs.clientinfo[ client ].armor = atoi( CG_Argv( i * 6 + 5 ) );
		cgs.clientinfo[ client ].curWeapon = atoi( CG_Argv( i * 6 + 6 ) );
		cgs.clientinfo[ client ].powerups = atoi( CG_Argv( i * 6 + 7 ) );
	}
}


/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo( void ) {
	const char  *info;
	char    *mapname;

	info = CG_ConfigString( CS_SERVERINFO );
	cgs.gametype = atoi( Info_ValueForKey( info, "g_gametype" ) );
	trap_Cvar_Set( "g_gametype", va( "%i", cgs.gametype ) );
	cgs.dmflags = atoi( Info_ValueForKey( info, "dmflags" ) );
	cgs.teamflags = atoi( Info_ValueForKey( info, "teamflags" ) );
	cgs.fraglimit = atoi( Info_ValueForKey( info, "fraglimit" ) );
	cgs.capturelimit = atoi( Info_ValueForKey( info, "capturelimit" ) );
	cgs.timelimit = atoi( Info_ValueForKey( info, "timelimit" ) );
	cgs.maxclients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	mapname = Info_ValueForKey( info, "mapname" );
	Com_sprintf( cgs.mapname, sizeof( cgs.mapname ), "maps/%s.bsp", mapname );

// JPW NERVE
// prolly should parse all CS_SERVERINFO keys automagically, but I don't want to break anything that might be improperly set for wolf SP, so I'm just parsing MP relevant stuff here
	trap_Cvar_Set( "g_medicChargeTime",Info_ValueForKey( info,"g_medicChargeTime" ) );
	trap_Cvar_Set( "g_engineerChargeTime",Info_ValueForKey( info,"g_engineerChargeTime" ) );
	trap_Cvar_Set( "g_soldierChargeTime",Info_ValueForKey( info,"g_soldierChargeTime" ) );
	trap_Cvar_Set( "g_LTChargeTime",Info_ValueForKey( info,"g_LTChargeTime" ) );
	trap_Cvar_Set( "g_redlimbotime",Info_ValueForKey( info,"g_redlimbotime" ) );
	trap_Cvar_Set( "g_bluelimbotime",Info_ValueForKey( info,"g_bluelimbotime" ) );
// jpw

	//	Q_strncpyz( cgs.redTeam, Info_ValueForKey( info, "g_redTeam" ), sizeof(cgs.redTeam) );
//	trap_Cvar_Set("g_redTeam", cgs.redTeam);
//	Q_strncpyz( cgs.blueTeam, Info_ValueForKey( info, "g_blueTeam" ), sizeof(cgs.blueTeam) );
//	trap_Cvar_Set("g_blueTeam", cgs.blueTeam);
}


//----(SA)	added
/*
==============
CG_ParseMissionStats
	time		h/m/s
	objectives	n/n
	secrets		n/n
	treasure	n/n
	artifacts	n/n
	attempts	num
==============
*/
static void CG_ParseMissionStats( void ) {
	const char  *info;
	char *token;

	info = CG_ConfigString( CS_MISSIONSTATS );

// time
	token = COM_Parse( (char **)&info );
	cg.playTimeH = atoi( token );
	token = COM_Parse( (char **)&info );
	cg.playTimeM = atoi( token );
	token = COM_Parse( (char **)&info );
	cg.playTimeS = atoi( token );

// objectives
	token = COM_Parse( (char **)&info );
	cg.numObjectivesFound = atoi( token );
	token = COM_Parse( (char **)&info );
	cg.numObjectives = atoi( token );

// secrets
	token = COM_Parse( (char **)&info );
	cg.numSecretsFound = atoi( token );
	token = COM_Parse( (char **)&info );
	cg.numSecrets = atoi( token );

// treasure
	token = COM_Parse( (char **)&info );
	cg.numTreasureFound = atoi( token );
	token = COM_Parse( (char **)&info );
	cg.numTreasure = atoi( token );

// artifacts
	token = COM_Parse( (char **)&info );
	cg.numArtifactsFound = atoi( token );
	token = COM_Parse( (char **)&info );
	cg.numArtifacts = atoi( token );

// attempts
	token = COM_Parse( (char **)&info );
	cg.attempts = atoi( token );

}
//----(SA)	end


/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup( void ) {
	const char  *info;
	int warmup;

	info = CG_ConfigString( CS_WARMUP );

	warmup = atoi( info );
	cg.warmupCount = -1;

	if ( warmup == 0 && cg.warmup ) {

	} else if ( warmup > 0 && cg.warmup <= 0 ) {
		trap_S_StartLocalSound( cgs.media.countPrepareSound, CHAN_ANNOUNCER );
	}

	cg.warmup = warmup;
}

/*
=====================
CG_ParseScreenFade
=====================
*/
static void CG_ParseScreenFade( void ) {
	const char  *info;
	char *token;
	float fadealpha;
	int fadestart, fadeduration;

	info = CG_ConfigString( CS_SCREENFADE );
	token = COM_Parse( (char **)&info );
	fadealpha = atof( token );
	token = COM_Parse( (char **)&info );
	fadestart = atoi( token );
	token = COM_Parse( (char **)&info );
	fadeduration = atoi( token );

	CG_Fade( 0, 0, 0, (int)( fadealpha * 255.0f ), fadestart, fadeduration );
}


/*
==============
CG_ParseFog
	float near dist
	float far dist
	float density
	float[3] r,g,b
	int		time
==============
*/
static void CG_ParseFog( void ) {
	const char  *info;
	char *token;
	float ne, fa, r, g, b, density;
	int time;

	info = CG_ConfigString( CS_FOGVARS );
	token = COM_Parse( (char **)&info );    ne = atof( token );
	token = COM_Parse( (char **)&info );

	if ( !token || !token[0] ) {
		// set to  'no fog'
		// 'FOG_MAP' is not registered, so it will always make fog go away
		trap_R_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP, (int)ne, 0, 0, 0, 0 );
		return;
	}

	fa = atof( token );

	token = COM_Parse( (char **)&info );    density = atof( token );
	token = COM_Parse( (char **)&info );    r = atof( token );
	token = COM_Parse( (char **)&info );    g = atof( token );
	token = COM_Parse( (char **)&info );    b = atof( token );
	token = COM_Parse( (char **)&info );    time = atoi( token );

	trap_R_SetFog( FOG_SERVER, (int)ne, (int)fa, r, g, b, density );
	trap_R_SetFog( FOG_CMD_SWITCHFOG, FOG_SERVER, time, 0, 0, 0, 0 );
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues( void ) {
#ifdef MISSIONPACK
	const char *s;
#endif

	cgs.scores1 = atoi( CG_ConfigString( CS_SCORES1 ) );
	cgs.scores2 = atoi( CG_ConfigString( CS_SCORES2 ) );
	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );
#ifdef MISSIONPACK
	if ( cgs.gametype == GT_CTF ) {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.redflag = s[0] - '0';
		cgs.blueflag = s[1] - '0';
	} else if ( cgs.gametype == GT_1FCTF )    {
		s = CG_ConfigString( CS_FLAGSTATUS );
		cgs.flagStatus = s[0] - '0';
	}
#endif
	cg.warmup = atoi( CG_ConfigString( CS_WARMUP ) );
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged( void ) {
	char originalShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	char timeOffset[16];
	const char *o;
	char *n,*t;

	o = CG_ConfigString( CS_SHADERSTATE );
	while ( o && *o ) {
		n = strstr( o, "=" );
		if ( n && *n ) {
			strncpy( originalShader, o, n - o );
			originalShader[n - o] = 0;
			n++;
			t = strstr( n, ":" );
			if ( t && *t ) {
				strncpy( newShader, n, t - n );
				newShader[t - n] = 0;
			} else {
				break;
			}
			t++;
			o = strstr( t, "@" );
			if ( o ) {
				strncpy( timeOffset, t, o - t );
				timeOffset[o - t] = 0;
				o++;
				trap_R_RemapShader( originalShader, newShader, timeOffset );
			}
		} else {
			break;
		}
	}
}

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified( void ) {
	const char  *str;
	int num;

	num = atoi( CG_Argv( 1 ) );

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState( &cgs.gameState );

	// look up the individual string that was modified
	str = CG_ConfigString( num );

	// do something with it if necessary
	if ( num == CS_MUSIC ) {
		CG_StartMusic();
	} else if ( num == CS_MUSIC_QUEUE ) {   //----(SA)	added
		CG_QueueMusic();
	} else if ( num == CS_MISSIONSTATS ) {  //----(SA)	added
		CG_ParseMissionStats();
	} else if ( num == CS_SERVERINFO ) {
		CG_ParseServerinfo();
	} else if ( num == CS_WARMUP ) {
		CG_ParseWarmup();
	} else if ( num == CS_SCORES1 ) {
		cgs.scores1 = atoi( str );
	} else if ( num == CS_SCORES2 ) {
		cgs.scores2 = atoi( str );
	} else if ( num == CS_LEVEL_START_TIME ) {
		cgs.levelStartTime = atoi( str );
	} else if ( num == CS_VOTE_TIME ) {
		cgs.voteTime = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_YES ) {
		cgs.voteYes = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_NO ) {
		cgs.voteNo = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_STRING ) {
		Q_strncpyz( cgs.voteString, str, sizeof( cgs.voteString ) );
#if 0
		trap_S_StartLocalSound( cgs.media.voteNow, CHAN_ANNOUNCER );
	} else if ( num >= CS_TEAMVOTE_TIME && num <= CS_TEAMVOTE_TIME + 1 ) {
		cgs.teamVoteTime[num - CS_TEAMVOTE_TIME] = atoi( str );
		cgs.teamVoteModified[num - CS_TEAMVOTE_TIME] = qtrue;
	} else if ( num >= CS_TEAMVOTE_YES && num <= CS_TEAMVOTE_YES + 1 ) {
		cgs.teamVoteYes[num - CS_TEAMVOTE_YES] = atoi( str );
		cgs.teamVoteModified[num - CS_TEAMVOTE_YES] = qtrue;
	} else if ( num >= CS_TEAMVOTE_NO && num <= CS_TEAMVOTE_NO + 1 ) {
		cgs.teamVoteNo[num - CS_TEAMVOTE_NO] = atoi( str );
		cgs.teamVoteModified[num - CS_TEAMVOTE_NO] = qtrue;
	} else if ( num >= CS_TEAMVOTE_STRING && num <= CS_TEAMVOTE_STRING + 1 ) {
		Q_strncpyz( cgs.teamVoteString[num - CS_TEAMVOTE_STRING], str, sizeof( cgs.teamVoteString ) );
		trap_S_StartLocalSound( cgs.media.voteNow, CHAN_ANNOUNCER );
#endif
	} else if ( num == CS_INTERMISSION ) {
		cg.intermissionStarted = atoi( str );
	} else if ( num == CS_SCREENFADE ) {
		CG_ParseScreenFade();
	} else if ( num == CS_FOGVARS ) {
		CG_ParseFog();
	} else if ( num >= CS_MODELS && num < CS_MODELS + MAX_MODELS ) {
		cgs.gameModels[ num - CS_MODELS ] = trap_R_RegisterModel( str );
	} else if ( num >= CS_SOUNDS && num < CS_SOUNDS + MAX_MODELS ) {
		if ( str[0] != '*' ) {   // player specific sounds don't register here

			// Ridah, register sound scripts seperately
			if ( !strstr( str, ".wav" ) ) {
				CG_SoundScriptPrecache( str );
			} else {
				cgs.gameSounds[ num - CS_SOUNDS] = trap_S_RegisterSound( str );
			}

		}
	} else if ( num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS ) {
		CG_NewClientInfo( num - CS_PLAYERS );
	}
	// Rafael particle configstring
	else if ( num >= CS_PARTICLES && num < CS_PARTICLES + MAX_PARTICLES_AREAS ) {
		CG_NewParticleArea( num );
	}
//----(SA)	have not reached this code yet so I don't know if I really need this here
	else if ( num >= CS_DLIGHTS && num < CS_DLIGHTS + MAX_DLIGHTS ) {
		CG_Printf( ">>>>>>>>>>>got configstring for dlight: %d\ntell Sherman!!!!!!!!!!", num - CS_DLIGHTS );
//----(SA)
	} else if ( num == CS_SHADERSTATE )   {
		CG_ShaderStateChanged();
	}

}


/*
=======================
CG_AddToTeamChat

=======================
*/
static void CG_AddToTeamChat( const char *str ) {
	int len;
	char *p, *ls;
	int lastcolor;
	int chatHeight;

	if ( cg_teamChatHeight.integer < TEAMCHAT_HEIGHT ) {
		chatHeight = cg_teamChatHeight.integer;
	} else {
		chatHeight = TEAMCHAT_HEIGHT;
	}

	if ( chatHeight <= 0 || cg_teamChatTime.integer <= 0 ) {
		// team chat disabled, dump into normal chat
		cgs.teamChatPos = cgs.teamLastChatPos = 0;
		return;
	}

	len = 0;

	p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while ( *str ) {
		if ( len > TEAMCHAT_WIDTH - 1 ) {
			if ( ls ) {
				str -= ( p - ls );
				str++;
				p -= ( p - ls );
			}
			*p = 0;

			cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;

			cgs.teamChatPos++;
			p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}

		if ( Q_IsColorString( str ) ) {
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if ( *str == ' ' ) {
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;

	cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
	cgs.teamChatPos++;

	if ( cgs.teamChatPos - cgs.teamLastChatPos > chatHeight ) {
		cgs.teamLastChatPos = cgs.teamChatPos - chatHeight;
	}
}

/*
===============
CG_SendMoveSpeed
===============
*/
void CG_SendMoveSpeed( animation_t *animList, int numAnims, char *modelName ) {
	animation_t *anim;
	int i;
	char text[10000];

	if ( !cgs.localServer ) {
		return;
	}

	text[0] = 0;
	Q_strcat( text, sizeof( text ), modelName );

	for ( i = 0, anim = animList; i < numAnims; i++, anim++ ) {
		if ( anim->moveSpeed <= 0 ) {
			continue;
		}

		// add this to the list
		Q_strcat( text, sizeof( text ), va( " %s %i %.1f", anim->name, anim->moveSpeed, anim->stepGap ) );
	}

	// send the movespeeds to the server
	trap_SendMoveSpeedsToGame( 0, text );
}

/*
===============
CG_SendMoveSpeeds

  send moveSpeeds for all unique models
===============
*/
#if 0
void CG_SendMoveSpeeds( void ) {
	int i,j;
	animModelInfo_t *modelInfo;
	clientInfo_t *ci;

	for ( i = 0; i < MAX_ANIMSCRIPT_MODELS; i++ ) {

		modelInfo = cgs.animScriptData.modelInfo[i];

		if ( modelInfo == NULL ) {
			continue;
		}

		if ( !modelInfo->modelname[0] ) {
			continue;
		}
/*
		// recalc them
		// find a client that uses this model
		for (ci = cgs.clientinfo, j=0; j<MAX_CLIENTS; j++, ci++) {
			if (ci->modelInfo && ci->modelInfo == modelInfo) {
				CG_CalcMoveSpeeds( ci );
				break;
			}
		}
*/
//		if (j==MAX_CLIENTS)
//			CG_Error( "CG_SendMoveSpeeds: cannot find client with modelName \"%s\" for moveSpeed calc", modelInfo->modelname );

		// send this model
		//CG_SendMoveSpeed( modelInfo->animations, modelInfo->numAnimations, modelInfo->modelname );
	}

}
#endif

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart( void ) {
//	char buff[64];
	int i;
	if ( cg_showmiss.integer ) {
		CG_Printf( "CG_MapRestart\n" );
	}

	memset( &cg.lastWeapSelInBank[0], 0, MAX_WEAP_BANKS * sizeof( int ) );  // clear weapon bank selections

	cg.centerPrintTime = 0; // reset centerprint counter so previous messages don't re-appear
	cg.itemPickupTime = 0;  // reset item pickup counter so previous messages don't re-appear
	cg.cursorHintFade = 0;  // reset cursor hint timer
	cg.yougotmailTime = 0;  // reset

	// (SA) clear zoom (so no warpies)
	cg.zoomedBinoc = qfalse;
	cg.zoomedBinoc = cg.zoomedScope = qfalse;
	cg.zoomTime = 0;
	cg.zoomval = 0;

	// reset fog to world fog (if present)
//	trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP,20,0,0,0,0);
//	trap_Cvar_VariableStringBuffer("r_mapFogColor", buff, sizeof(buff));
//	trap_SendClientCommand(va("fogswitch %s", buff) );

	CG_InitLocalEntities();
	CG_InitMarkPolys();

	//Rafael particles
	CG_ClearParticles();
	// done.

	for ( i = 1; i < MAX_PARTICLES_AREAS; i++ )
	{
		{
			int rval;

			rval = CG_NewParticleArea( CS_PARTICLES + i );
			if ( !rval ) {
				break;
			}
		}
	}


	// Ridah, trails
	CG_ClearTrails();
	// done.

	// Ridah
	CG_ClearFlameChunks();
	CG_SoundInit();
	// done.

	// RF, init ZombieFX
	trap_RB_ZombieFXAddNewHit( -1, NULL, NULL );

	// make sure the "3 frags left" warnings play again
	cg.fraglimitWarnings = 0;

	cg.timelimitWarnings = 0;

	cg.intermissionStarted = qfalse;

	cgs.voteTime = 0;

	cg.lightstylesInited    = qfalse;

	cg.mapRestart = qtrue;

	CG_StartMusic();

	trap_S_ClearLoopingSounds( qtrue );

	// we really should clear more parts of cg here and stop sounds
	cg.v_dmg_time = 0;
	cg.v_noFireTime = 0;
	cg.v_fireTime = 0;

	// RF, clear out animScriptData so we recalc everything and get new pointers from server
	memset( cgs.animScriptData.modelInfo, 0, sizeof( cgs.animScriptData.modelInfo ) );
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( cgs.clientinfo[i].infoValid ) {
			CG_LoadClientInfo( &cgs.clientinfo[i] );    // re-register the valid clients
		}
	}
	// always clear the weapon selection
	cg.weaponSelect = WP_NONE;
	// clear out the player weapon info
	memset( &cg_entities[0].pe.weap, 0, sizeof( cg_entities[0].pe.weap ) );
	// check for server set weapons we might not know about
	// (FIXME: this is a hack for the time being since a scripted "selectweapon" does
	// not hit the first snap, the server weapon set in cg_playerstate.c line 219 doesn't
	// do the trick)
	if ( !cg.weaponSelect ) {
		if ( cg_loadWeaponSelect.integer > 0 ) {
			cg.weaponSelect = cg_loadWeaponSelect.integer;
			cg.weaponSelectTime = cg.time;
			trap_Cvar_Set( "cg_loadWeaponSelect", "0" );  // turn it off
		}
	}
	// clear out rumble effects
	memset( cg.cameraShake, 0, sizeof( cg.cameraShake ) );
	memset( cg.cameraShakeAngles, 0, sizeof( cg.cameraShakeAngles ) );
	cg.rumbleScale = 0;

	// play the "fight" sound if this is a restart without warmup
//	if ( cg.warmup == 0 /* && cgs.gametype == GT_TOURNAMENT */) {
//		trap_S_StartLocalSound( cgs.media.countFightSound, CHAN_ANNOUNCER );
//		CG_CenterPrint( "FIGHT!", 120, GIANTCHAR_WIDTH*2 );
//	}
#ifdef MISSIONPACK
	if ( cg_singlePlayerActive.integer ) {
		trap_Cvar_Set( "ui_matchStartTime", va( "%i", cg.time ) );
		if ( cg_recordSPDemo.integer && cg_recordSPDemoName.string && *cg_recordSPDemoName.string ) {
			trap_SendConsoleCommand( va( "set g_synchronousclients 1 ; record %s \n", cg_recordSPDemoName.string ) );
		}
	}
#endif
	trap_Cvar_Set( "cg_thirdPerson", "0" );
}

/*
=================
CG_RequestMoveSpeed
=================
*/
void CG_RequestMoveSpeed( const char *modelname ) {
	animModelInfo_t *modelInfo;

	modelInfo = BG_ModelInfoForModelname( (char *)modelname );

	if ( !modelInfo ) {
		// ignore it
		return;
	}

	// send it
	CG_SendMoveSpeed( modelInfo->animations, modelInfo->numAnimations, (char *)modelname );
}

/*
=================
CG_RemoveChatEscapeChar
=================
*/
static void CG_RemoveChatEscapeChar( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if ( text[i] == '\x19' ) {
			continue;
		}
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
void CG_ObjectivePrint( const char *str, int charWidth, int team );     // NERVE - SMF

static void CG_ServerCommand( void ) {
	const char  *cmd;
	char text[MAX_SAY_TEXT];

	cmd = CG_Argv( 0 );

	if ( !cmd[0] ) {
		// server claimed the command
		return;
	}

	if ( !strcmp( cmd, "startCam" ) ) {
		CG_StartCamera( CG_Argv( 1 ), atoi( CG_Argv( 2 ) ) );
		return;
	}

	if ( !strcmp( cmd, "stopCam" ) ) {
		CG_StopCamera();
		return;
	}

	if ( !strcmp( cmd, "mvspd" ) ) {
		CG_RequestMoveSpeed( CG_Argv( 1 ) );
		return;
	}

	if ( !strcmp( cmd, "dp" ) ) {    // dynamite print (what a hack :(

		CG_CenterPrint( va( "%s %d %s", CG_translateString( "dynamitetimer" ), atoi( CG_Argv( 1 ) ), CG_translateString( "seconds" ) ),
						SCREEN_HEIGHT - ( SCREEN_HEIGHT * 0.25 ), SMALLCHAR_WIDTH );
		return;
	}

	if ( !strcmp( cmd, "cp" ) ) {
		CG_CenterPrint( CG_Argv( 1 ), SCREEN_HEIGHT - ( SCREEN_HEIGHT * 0.25 ), SMALLCHAR_WIDTH );
		return;
	}

	if ( !strcmp( cmd, "cs" ) ) {
		CG_ConfigStringModified();
		return;
	}

	if ( !strcmp( cmd, "print" ) ) {
		CG_Printf( "%s", CG_Argv( 1 ) );
#ifdef MISSIONPACK
		cmd = CG_Argv( 1 );           // yes, this is obviously a hack, but so is the way we hear about
									  // votes passing or failing
		if ( !Q_stricmpn( cmd, "vote failed", 11 ) || !Q_stricmpn( cmd, "team vote failed", 16 ) ) {
			trap_S_StartLocalSound( cgs.media.voteFailed, CHAN_ANNOUNCER );
		} else if ( !Q_stricmpn( cmd, "vote passed", 11 ) || !Q_stricmpn( cmd, "team vote passed", 16 ) ) {
			trap_S_StartLocalSound( cgs.media.votePassed, CHAN_ANNOUNCER );
		}
#endif
		return;
	}

	if ( !strcmp( cmd, "chat" ) ) {
		if ( !cg_teamChatsOnly.integer ) {
			trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
			Q_strncpyz( text, CG_Argv( 1 ), MAX_SAY_TEXT );
			CG_RemoveChatEscapeChar( text );
			CG_Printf( "%s\n", text );
		}
		return;
	}

	if ( !strcmp( cmd, "tchat" ) ) {
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
		Q_strncpyz( text, CG_Argv( 1 ), MAX_SAY_TEXT );
		CG_RemoveChatEscapeChar( text );
		CG_AddToTeamChat( text );
		CG_Printf( "%s\n", text );
		return;
	}

	// NERVE - SMF - limbo chat
	if ( !strcmp( cmd, "lchat" ) ) {
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
		Q_strncpyz( text, CG_Argv( 1 ), MAX_SAY_TEXT );
		CG_RemoveChatEscapeChar( text );
//		CG_AddToLimboChat( text );
		trap_UI_LimboChat( text );
		CG_Printf( "%s\n", text );
		return;
	}
	// -NERVE - SMF

	if ( !strcmp( cmd, "vchat" ) ) {
//		CG_VoiceChat( SAY_ALL );
		return;
	}

	if ( !strcmp( cmd, "vtchat" ) ) {
//		CG_VoiceChat( SAY_TEAM );
		return;
	}

	if ( !strcmp( cmd, "vtell" ) ) {
//		CG_VoiceChat( SAY_TELL );
		return;
	}

	if ( !strcmp( cmd, "scores" ) ) {
		CG_ParseScores();
		return;
	}

	if ( !strcmp( cmd, "tinfo" ) ) {
		CG_ParseTeamInfo();
		return;
	}

	if ( !strcmp( cmd, "map_restart" ) ) {
		CG_MapRestart();
		return;
	}

	if ( Q_stricmp( cmd, "remapShader" ) == 0 ) {
		if ( trap_Argc() == 4 ) {
			trap_R_RemapShader( CG_Argv( 1 ), CG_Argv( 2 ), CG_Argv( 3 ) );
		}
	}

	// loaddeferred can be both a servercmd and a consolecmd
	if ( !strcmp( cmd, "loaddeferred" ) ) {  // spelling fixed (SA)
		CG_LoadDeferredPlayers();
		return;
	}

	// clientLevelShot is sent before taking a special screenshot for
	// the menu system during development
	if ( !strcmp( cmd, "clientLevelShot" ) ) {
		cg.levelShot = qtrue;
		return;
	}

	// NERVE - SMF
	if ( !Q_stricmp( cmd, "oid" ) ) {
		CG_ObjectivePrint( CG_Argv( 2 ), SMALLCHAR_WIDTH, atoi( CG_Argv( 1 ) ) );
		return;
	}
	// -NERVE - SMF


	//
	// music
	//

	// loops \/
	if ( !strcmp( cmd, "mu_start" ) ) {  // has optional parameter for fade-up time
		int fadeTime = 0;   // default to instant start

		Q_strncpyz( text, CG_Argv( 2 ), MAX_SAY_TEXT );
		if ( text && strlen( text ) ) {
			fadeTime = atoi( text );
		}

		trap_S_StartBackgroundTrack( CG_Argv( 1 ), CG_Argv( 1 ), fadeTime );
		return;
	}
	// plays once then back to whatever the loop was \/
	if ( !strcmp( cmd, "mu_play" ) ) {   // has optional parameter for fade-up time
		int fadeTime = 0;   // default to instant start

		Q_strncpyz( text, CG_Argv( 2 ), MAX_SAY_TEXT );
		if ( text && strlen( text ) ) {
			fadeTime = atoi( text );
		}

		trap_S_StartBackgroundTrack( CG_Argv( 1 ), "onetimeonly", fadeTime );
		return;
	}

	if ( !strcmp( cmd, "mu_stop" ) ) {   // has optional parameter for fade-down time
		int fadeTime = 0;   // default to instant stop

		Q_strncpyz( text, CG_Argv( 1 ), MAX_SAY_TEXT );
		if ( text && strlen( text ) ) {
			fadeTime = atoi( text );
		}
		trap_S_FadeBackgroundTrack( 0.0f, fadeTime, 0 );
		trap_S_StartBackgroundTrack( "", "", -2 ); // '-2' for 'queue looping track' (QUEUED_PLAY_LOOPED)
		return;
	}

	if ( !strcmp( cmd, "mu_fade" ) ) {
		trap_S_FadeBackgroundTrack( atof( CG_Argv( 1 ) ), atoi( CG_Argv( 2 ) ), 0 );
		return;
	}

	if ( !strcmp( cmd, "snd_fade" ) ) {
		trap_S_FadeAllSound( atof( CG_Argv( 1 ) ), atoi( CG_Argv( 2 ) ) );
		return;
	}

	if ( !strcmp( cmd, "rockandroll" ) ) {   // map loaded, game is ready to begin.
		CG_Fade( 0, 0, 0, 255, cg.time, 0 );      // go black
		trap_UI_Popup( "pregame" );                // start pregame menu
		trap_Cvar_Set( "cg_norender", "1" );    // don't render the world until the player clicks in and the 'playerstart' func has been called (g_main in G_UpdateCvars() ~ilne 949)

		trap_S_FadeAllSound( 1.0f, 1000 );    // fade sound up

		return;
	}



	// ensure a file gets into a build (mainly for scripted music calls)
	if ( !strcmp( cmd, "addToBuild" ) ) {
		fileHandle_t f;

		if ( !cg_buildScript.integer ) {
			return;
		}

		// just open the file so it gets copied to the build dir
		//CG_FileTouchForBuild(CG_Argv(1));
		trap_FS_FOpenFile( CG_Argv( 1 ), &f, FS_READ );
		trap_FS_FCloseFile( f );
		return;
	}


	CG_Printf( "Unknown client game command: %s\n", cmd );
}


/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands( int latestSequence ) {
	while ( cgs.serverCommandSequence < latestSequence ) {
		if ( trap_GetServerCommand( ++cgs.serverCommandSequence ) ) {
			CG_ServerCommand();
		}
	}
}
