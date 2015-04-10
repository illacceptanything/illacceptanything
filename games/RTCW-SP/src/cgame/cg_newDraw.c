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



#include "cg_local.h"
#include "../ui/ui_shared.h"

extern displayContextDef_t cgDC;

// set in CG_ParseTeamInfo

//static int sortedTeamPlayers[TEAM_MAXOVERLAY];
//static int numSortedTeamPlayers;
int drawTeamOverlayModificationCount = -1;

//static char systemChat[256];
//static char teamChat1[256];
//static char teamChat2[256];

void CG_InitTeamChat() {
#ifdef MISSIONPACK
	memset( teamChat1, 0, sizeof( teamChat1 ) );
	memset( teamChat2, 0, sizeof( teamChat2 ) );
	memset( systemChat, 0, sizeof( systemChat ) );
#endif  // #ifdef MISSIONPACK
}

void CG_SetPrintString( int type, const char *p ) {
	if ( type == SYSTEM_PRINT ) {
		strcpy( systemChat, p );
	} else {
		strcpy( teamChat2, teamChat1 );
		strcpy( teamChat1, p );
	}
}

void CG_CheckOrderPending() {
#ifdef MISSIONPACK
	if ( cgs.gametype < GT_CTF ) {
		return;
	}
	if ( cgs.orderPending ) {
		//clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[cg_currentSelectedPlayer.integer];
		const char *p1, *p2, *b;
		p1 = p2 = b = NULL;
		switch ( cgs.currentOrder ) {
		case TEAMTASK_OFFENSE:
			p1 = VOICECHAT_ONOFFENSE;
			p2 = VOICECHAT_OFFENSE;
			b = "+button7; wait; -button7";
			break;
		case TEAMTASK_DEFENSE:
			p1 = VOICECHAT_ONDEFENSE;
			p2 = VOICECHAT_DEFEND;
			b = "+button8; wait; -button8";
			break;
		case TEAMTASK_PATROL:
			p1 = VOICECHAT_ONPATROL;
			p2 = VOICECHAT_PATROL;
			b = "+button9; wait; -button9";
			break;
		case TEAMTASK_FOLLOW:
			p1 = VOICECHAT_ONFOLLOW;
			p2 = VOICECHAT_FOLLOWME;
			b = "+button10; wait; -button10";
			break;
		case TEAMTASK_CAMP:
			p1 = VOICECHAT_ONCAMPING;
			p2 = VOICECHAT_CAMP;
			break;
		case TEAMTASK_RETRIEVE:
			p1 = VOICECHAT_ONGETFLAG;
			p2 = VOICECHAT_RETURNFLAG;
			break;
		case TEAMTASK_ESCORT:
			p1 = VOICECHAT_ONFOLLOWCARRIER;
			p2 = VOICECHAT_FOLLOWFLAGCARRIER;
			break;
		}

		if ( cg_currentSelectedPlayer.integer == numSortedTeamPlayers ) {
			// to everyone
			trap_SendConsoleCommand( va( "cmd vsay_team %s\n", p2 ) );
		} else {
			// for the player self
			if ( sortedTeamPlayers[cg_currentSelectedPlayer.integer] == cg.snap->ps.clientNum && p1 ) {
				trap_SendConsoleCommand( va( "teamtask %i\n", cgs.currentOrder ) );
				//trap_SendConsoleCommand(va("cmd say_team %s\n", p2));
				trap_SendConsoleCommand( va( "cmd vsay_team %s\n", p1 ) );
			} else if ( p2 ) {
				//trap_SendConsoleCommand(va("cmd say_team %s, %s\n", ci->name,p));
				trap_SendConsoleCommand( va( "cmd vtell %d %s\n", sortedTeamPlayers[cg_currentSelectedPlayer.integer], p2 ) );
			}
		}
		if ( b ) {
			trap_SendConsoleCommand( b );
		}
		cgs.orderPending = qfalse;
	}
#endif  // #ifdef MISSIONPACK
}

static void CG_SetSelectedPlayerName() {
	if ( cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers ) {
		clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[cg_currentSelectedPlayer.integer];
		if ( ci ) {
			trap_Cvar_Set( "cg_selectedPlayerName", ci->name );
			trap_Cvar_Set( "cg_selectedPlayer", va( "%d", sortedTeamPlayers[cg_currentSelectedPlayer.integer] ) );
//			cgs.currentOrder = ci->teamTask;
		}
	} else {
		trap_Cvar_Set( "cg_selectedPlayerName", "Everyone" );
	}
}
int CG_GetSelectedPlayer() {
	if ( cg_currentSelectedPlayer.integer < 0 || cg_currentSelectedPlayer.integer >= numSortedTeamPlayers ) {
		cg_currentSelectedPlayer.integer = 0;
	}
	return cg_currentSelectedPlayer.integer;
}

void CG_SelectNextPlayer() {
	CG_CheckOrderPending();
	if ( cg_currentSelectedPlayer.integer >= 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers ) {
		cg_currentSelectedPlayer.integer++;
	} else {
		cg_currentSelectedPlayer.integer = 0;
	}
	CG_SetSelectedPlayerName();
}

void CG_SelectPrevPlayer() {
	CG_CheckOrderPending();
	if ( cg_currentSelectedPlayer.integer > 0 && cg_currentSelectedPlayer.integer < numSortedTeamPlayers ) {
		cg_currentSelectedPlayer.integer--;
	} else {
		cg_currentSelectedPlayer.integer = numSortedTeamPlayers;
	}
	CG_SetSelectedPlayerName();
}


// (SA) not sure what you'd use this for anyway...

static void CG_DrawPlayerArmorIcon( rectDef_t *rect, qboolean draw2D ) {
#ifdef MISSIONPACK
	centity_t   *cent;
	playerState_t   *ps;
	vec3_t angles;
	vec3_t origin;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	if ( draw2D || !cg_draw3dIcons.integer && cg_drawIcons.integer ) {
		CG_DrawPic( rect->x, rect->y + rect->h / 2 + 1, rect->w, rect->h, cgs.media.armorIcon );
	} else if ( cg_draw3dIcons.integer ) {
		VectorClear( angles );
		origin[0] = 90;
		origin[1] = 0;
		origin[2] = -10;
		angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;

		CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, cgs.media.armorModel, 0, origin, angles );
	}
#endif
}

static void CG_DrawPlayerArmorValue( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	char num[16];
	int value;
	centity_t   *cent;
	playerState_t   *ps;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	value = ps->stats[STAT_ARMOR];


	if ( shader ) {
		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		trap_R_SetColor( NULL );
	} else {
		Com_sprintf( num, sizeof( num ), "%i", value );
		value = CG_Text_Width( num, font, scale, 0 );
		CG_Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
	}
}

// TTimo: unused
/*
static float healthColors[4][4] = {
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1, 0.69f, 0, 1.0f } ,		// normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },		// low health
		{0.5f, 0.5f, 0.5f, 1},			// weapon firing
		{ 1, 1, 1, 1 } };			// health > 100
*/

/*
==============
weapIconDrawSize
==============
*/
static int weapIconDrawSize( int weap ) {
	switch ( weap ) {

		// weapons to not draw
	case WP_KNIFE:
		return 0;

		// weapons with 'wide' icons
	case WP_THOMPSON:
	case WP_MP40:
	case WP_STEN:
	case WP_MAUSER:
	case WP_GARAND:
	case WP_VENOM:
	case WP_TESLA:
	case WP_PANZERFAUST:
	case WP_FLAMETHROWER:
	case WP_FG42:
	case WP_FG42SCOPE:
	case WP_SNOOPERSCOPE:
	case WP_SNIPERRIFLE:
		return 2;
	}

	return 1;
}


/*
==============
CG_DrawPlayerWeaponIcon
==============
*/
static void CG_DrawPlayerWeaponIcon( rectDef_t *rect, qboolean drawHighlighted, int align ) {
	int size;
	int realweap;                   // DHM - Nerve
	qhandle_t icon;
	float scale,halfScale;

	if ( !cg_drawIcons.integer ) {
		return;
	}

	// DHM - Nerve :: special case for WP_CLASS_SPECIAL

	realweap = cg.predictedPlayerState.weapon;

	if ( cgs.gametype == GT_WOLF && realweap == WP_CLASS_SPECIAL ) {
		switch ( cg.predictedPlayerState.stats[ STAT_PLAYER_CLASS ] ) {
		case PC_MEDIC:
			realweap = WP_MEDIC_HEAL;
			break;
		case PC_LT:
			realweap = WP_GRENADE_SMOKE;
			break;
		default:
			break;
		}
	}
	// dhm

	size = weapIconDrawSize( realweap );

	if ( !size ) {
		return;
	}

	if ( drawHighlighted ) {
		icon = cg_weapons[ realweap ].weaponIcon[1];
	} else {
		icon = cg_weapons[ realweap ].weaponIcon[0];
	}



	// pulsing grenade icon to help the player 'count' in their head
	if ( cg.predictedPlayerState.grenadeTimeLeft ) {   // grenades and dynamite set this

		// these time differently
		if ( realweap == WP_DYNAMITE ) {
			if ( ( ( cg.grenLastTime ) % 1000 ) > ( ( cg.predictedPlayerState.grenadeTimeLeft ) % 1000 ) ) {
				trap_S_StartLocalSound( cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND );
			}
		} else {
			if ( ( ( cg.grenLastTime ) % 1000 ) < ( ( cg.predictedPlayerState.grenadeTimeLeft ) % 1000 ) ) {
				switch ( cg.predictedPlayerState.grenadeTimeLeft / 1000 ) {
				case 3:
					trap_S_StartLocalSound( cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND );
					break;
				case 2:
					trap_S_StartLocalSound( cgs.media.grenadePulseSound3, CHAN_LOCAL_SOUND );
					break;
				case 1:
					trap_S_StartLocalSound( cgs.media.grenadePulseSound2, CHAN_LOCAL_SOUND );
					break;
				case 0:
					trap_S_StartLocalSound( cgs.media.grenadePulseSound1, CHAN_LOCAL_SOUND );
					break;
				}
			}
		}

		scale = (float)( ( cg.predictedPlayerState.grenadeTimeLeft ) % 1000 ) / 100.0f;
		halfScale = scale * 0.5f;

		cg.grenLastTime = cg.predictedPlayerState.grenadeTimeLeft;
	} else {
		scale = halfScale = 0;
	}


	if ( icon ) {
		float x, y, w, h;

		if ( size == 1 ) { // draw half width to match the icon asset

			// start at left
			x = rect->x - halfScale;
			y = rect->y - halfScale;
			w = rect->w / 2 + scale;
			h = rect->h + scale;

			switch ( align ) {
			case ITEM_ALIGN_CENTER:
				x += rect->w / 4;
				break;
			case ITEM_ALIGN_RIGHT:
				x += rect->w / 2;
				break;
			case ITEM_ALIGN_LEFT:
			default:
				break;
			}

		} else {
			x = rect->x - halfScale;
			y = rect->y - halfScale;
			w = rect->w + scale;
			h = rect->h + scale;
		}


		CG_DrawPic( x, y, w, h, icon );
	}
}


/*
==============
CG_DrawPlayerAmmoIcon
==============
*/
static void CG_DrawPlayerAmmoIcon( rectDef_t *rect, qboolean draw2D ) {
	centity_t   *cent;
	playerState_t   *ps;
	vec3_t angles;
	vec3_t origin;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	// TTimo: gcc: suggests () around && within ||
	if ( draw2D || ( !cg_draw3dIcons.integer && cg_drawIcons.integer ) ) {
		qhandle_t icon;
		icon = cg_weapons[ cg.predictedPlayerState.weapon ].ammoIcon;
		if ( icon ) {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, icon );
		}
	} else if ( cg_draw3dIcons.integer ) {
		if ( cent->currentState.weapon && cg_weapons[ cent->currentState.weapon ].ammoModel ) {
			VectorClear( angles );
			origin[0] = 70;
			origin[1] = 0;
			origin[2] = 0;
			angles[YAW] = 90 + 20 * sin( cg.time / 1000.0 );
			CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, cg_weapons[ cent->currentState.weapon ].ammoModel, 0, origin, angles );
		}
	}
}

extern void CG_CheckForCursorHints( void );
#define CURSORHINT_SCALE    10

/*
==============
CG_DrawCursorHints

  cg_cursorHints.integer ==
	0:	no hints
	1:	sin size pulse
	2:	one way size pulse
	3:	alpha pulse
	4+:	static image

==============
*/
extern void CG_DrawExitStats( void );

static void CG_DrawCursorhint( rectDef_t *rect ) {
	float       *color;
	qhandle_t icon, icon2 = 0;
	float scale, halfscale;
	qboolean redbar = qfalse;

	if ( !cg_cursorHints.integer ) {
		return;
	}

	CG_CheckForCursorHints();

	icon = cgs.media.hintShaders[cg.cursorHintIcon];

	switch ( cg.cursorHintIcon ) {
	case HINT_NONE:
	case HINT_FORCENONE:
		icon = 0;
		break;
	case HINT_BREAKABLE_DYNAMITE:
		icon = cgs.media.hintShaders[HINT_BREAKABLE];
		break;
	case HINT_CHAIR:
		// only show 'pickupable' if you're not armed, or are armed with a single handed weapon
		if ( cg.predictedPlayerState.weapon && !( WEAPS_ONE_HANDED & ( 1 << ( cg.predictedPlayerState.weapon ) ) ) ) { // (SA) this was backwards
			icon = cgs.media.hintShaders[HINT_NOACTIVATE];
		}
		break;

	case HINT_PLAYER:
		icon = cgs.media.hintShaders[HINT_NOACTIVATE];
		break;

	case HINT_PLYR_FRIEND:
		break;

	case HINT_NOEXIT_FAR:
		redbar = qtrue;     // draw the status bar in red to show that you can't exit yet
	case HINT_EXIT_FAR:
		break;

	case HINT_NOEXIT:
		redbar = qtrue;     // draw the status bar in red to show that you can't exit yet
	case HINT_EXIT:
		cg.exitStatsFade = 250;     // fade /up/ time
		if ( !cg.exitStatsTime ) {
			cg.exitStatsTime = cg.time;
		}
		CG_DrawExitStats();
		break;

	default:
		break;
	}

	if ( !icon ) {
		return;
	}


	// color
	color = CG_FadeColor( cg.cursorHintTime, cg.cursorHintFade );
	if ( !color ) {
		trap_R_SetColor( NULL );
		cg.exitStatsTime = 0;   // exit stats will fade up next time they're hit
		cg.cursorHintIcon = HINT_NONE;  // clear the hint
		return;
	}

	if ( cg_cursorHints.integer == 3 ) {
		color[3] *= 0.5 + 0.5 * sin( (float)cg.time / 150.0 );
	}


	// size
	if ( cg_cursorHints.integer >= 3 ) {   // no size pulsing
		scale = halfscale = 0;
	} else {
		if ( cg_cursorHints.integer == 2 ) {
			scale = (float)( ( cg.cursorHintTime ) % 1000 ) / 100.0f; // one way size pulse
		} else {
			scale = CURSORHINT_SCALE * ( 0.5 + 0.5 * sin( (float)cg.time / 150.0 ) ); // sin pulse

		}
		halfscale = scale * 0.5f;
	}

	// set color and draw the hint
	trap_R_SetColor( color );
	CG_DrawPic( rect->x - halfscale, rect->y - halfscale, rect->w + scale, rect->h + scale, icon );

	if ( icon2 ) {
		CG_DrawPic( rect->x - halfscale, rect->y - halfscale, rect->w + scale, rect->h + scale, icon2 );
	}

	trap_R_SetColor( NULL );

	// draw status bar under the cursor hint
	if ( cg.cursorHintValue && ( !( cg.time - cg.cursorHintTime ) ) ) {    // don't fade bar out w/ hint icon
		if ( redbar ) {
			Vector4Set( color, 1, 0, 0, 0.5f );
		} else {
			Vector4Set( color, 0, 0, 1, 0.5f );
		}
		CG_FilledBar( rect->x, rect->y + rect->h + 4, rect->w, 8, color, NULL, NULL, (float)cg.cursorHintValue / 255.0f, 0 );
	}

}



/*
==============
CG_DrawMessageIcon
==============
*/
//----(SA)	added

static void CG_DrawMessageIcon( rectDef_t *rect ) {
	int icon;

	if ( !cg_youGotMail.integer ) {
		return;
	}

	if ( cg_youGotMail.integer == 2 ) {
		icon = cgs.media.youGotObjectiveShader;
	} else {
		icon = cgs.media.youGotMailShader;
	}

	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, icon );
}



/*
==============
CG_DrawPlayerAmmoValue
	0 - ammo
	1 - clip
==============
*/
static void CG_DrawPlayerAmmoValue( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle, int type ) {
	char num[16];
	int value, value2 = 0;
	centity_t   *cent;
	playerState_t   *ps;
	int weap;
	qboolean special = qfalse;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	weap = cent->currentState.weapon;

	if ( !weap ) {
		return;
	}

	if ( ps->weaponstate == WEAPON_RELOADING && type != 0 ) {
		return;
	}

	switch ( weap ) {      // some weapons don't draw ammo count text
	case WP_KNIFE:
	case WP_CLASS_SPECIAL:              // DHM - Nerve
		return;

	case WP_AKIMBO:
		special = qtrue;
		break;

	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_TESLA:
	case WP_FLAMETHROWER:
		if ( type == 0 ) {  // don't draw reserve value, just clip (since these weapons have all their ammo in the clip)
			return;
		}
		break;

	default:
		break;
	}


	if ( type == 0 ) { // ammo
		value = cg.snap->ps.ammo[BG_FindAmmoForWeapon( weap )];
	} else {        // clip
		value = ps->ammoclip[BG_FindClipForWeapon( weap )];
		if ( special ) {
			value2 = value;
			if ( weapAlts[weap] ) {
				value = ps->ammoclip[weapAlts[weap]];
			}
//				value2 = ps->ammoclip[weapAlts[weap]];
		}
	}

	if ( value > -1 ) {
		if ( shader ) {
			trap_R_SetColor( color );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
			trap_R_SetColor( NULL );
		} else {
			Com_sprintf( num, sizeof( num ), "%i", value );
			value = CG_Text_Width( num, font, scale, 0 );
			CG_Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );

//			if(special) {	// draw '0' for akimbo guns
			if ( value2 || ( special && type == 1 ) ) {
				Com_sprintf( num, sizeof( num ), "%i /", value2 );
				value = CG_Text_Width( num, font, scale, 0 );
				CG_Text_Paint( -30 + rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
			}
		}
	}
}



static void CG_DrawPlayerHead( rectDef_t *rect, qboolean draw2D ) {
	vec3_t angles;
	float size, stretch;
	float frac;
	float x = rect->x;

	VectorClear( angles );

	if ( cg.damageTime && cg.time - cg.damageTime < DAMAGE_TIME ) {
		frac = (float)( cg.time - cg.damageTime ) / DAMAGE_TIME;
		size = rect->w * 1.25 * ( 1.5 - frac * 0.5 );

		stretch = size - rect->w * 1.25;
		// kick in the direction of damage
		x -= stretch * 0.5 + cg.damageX * stretch * 0.5;

		cg.headStartYaw = 180 + cg.damageX * 45;

		cg.headEndYaw = 180 + 20 * cos( crandom() * M_PI );
		cg.headEndPitch = 5 * cos( crandom() * M_PI );

		cg.headStartTime = cg.time;
		cg.headEndTime = cg.time + 100 + random() * 2000;
	} else {
		if ( cg.time >= cg.headEndTime ) {
			// select a new head angle
			cg.headStartYaw = cg.headEndYaw;
			cg.headStartPitch = cg.headEndPitch;
			cg.headStartTime = cg.headEndTime;
			cg.headEndTime = cg.time + 100 + random() * 2000;

			cg.headEndYaw = 180 + 20 * cos( crandom() * M_PI );
			cg.headEndPitch = 5 * cos( crandom() * M_PI );
		}

		size = rect->w * 1.25;
	}

	// if the server was frozen for a while we may have a bad head start time
	if ( cg.headStartTime > cg.time ) {
		cg.headStartTime = cg.time;
	}

	frac = ( cg.time - cg.headStartTime ) / (float)( cg.headEndTime - cg.headStartTime );
	frac = frac * frac * ( 3 - 2 * frac );
	angles[YAW] = cg.headStartYaw + ( cg.headEndYaw - cg.headStartYaw ) * frac;
	angles[PITCH] = cg.headStartPitch + ( cg.headEndPitch - cg.headStartPitch ) * frac;

	CG_DrawHead( x, rect->y, rect->w, rect->h, cg.snap->ps.clientNum, angles );
}

static void CG_DrawSelectedPlayerHealth( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	clientInfo_t *ci;
	int value;
	char num[16];

	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if ( ci ) {
		if ( shader ) {
			trap_R_SetColor( color );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
			trap_R_SetColor( NULL );
		} else {
			Com_sprintf( num, sizeof( num ), "%i", ci->health );
			value = CG_Text_Width( num, font, scale, 0 );
			CG_Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
		}
	}
}

static void CG_DrawSelectedPlayerArmor( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	clientInfo_t *ci;
	int value;
	char num[16];

	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if ( ci ) {
		if ( ci->armor > 0 ) {
			if ( shader ) {
				trap_R_SetColor( color );
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
				trap_R_SetColor( NULL );
			} else {
				Com_sprintf( num, sizeof( num ), "%i", ci->armor );
				value = CG_Text_Width( num, font, scale, 0 );
				CG_Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
			}
		}
	}
}

qhandle_t CG_StatusHandle( int task ) {
#ifdef MISSIONPACK
	qhandle_t h = cgs.media.assaultShader;
	switch ( task ) {
	case TEAMTASK_OFFENSE:
		h = cgs.media.assaultShader;
		break;
	case TEAMTASK_DEFENSE:
		h = cgs.media.defendShader;
		break;
	case TEAMTASK_PATROL:
		h = cgs.media.patrolShader;
		break;
	case TEAMTASK_FOLLOW:
		h = cgs.media.followShader;
		break;
	case TEAMTASK_CAMP:
		h = cgs.media.campShader;
		break;
	case TEAMTASK_RETRIEVE:
		h = cgs.media.retrieveShader;
		break;
	case TEAMTASK_ESCORT:
		h = cgs.media.escortShader;
		break;
	default:
		h = cgs.media.assaultShader;
		break;
	}
	return h;
#else
	return 0;
#endif  // #ifdef MISSIONPACK
}

static void CG_DrawSelectedPlayerStatus( rectDef_t *rect ) {
#ifdef MISSIONPACK
	clientInfo_t *ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if ( ci ) {
		qhandle_t h;
		if ( cgs.orderPending ) {
			// blink the icon
			if ( cg.time > cgs.orderTime - 2500 && ( cg.time >> 9 ) & 1 ) {
				return;
			}
			h = CG_StatusHandle( cgs.currentOrder );
		}   else {
			h = CG_StatusHandle( ci->teamTask );
		}
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, h );
	}
#endif  // #ifdef MISSIONPACK
}


static void CG_DrawPlayerStatus( rectDef_t *rect ) {
#ifdef MISSIONPACK
	clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	if ( ci ) {
		qhandle_t h = CG_StatusHandle( ci->teamTask );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, h );
	}
#endif  // #ifdef MISSIONPACK
}


static void CG_DrawSelectedPlayerName( rectDef_t *rect, int font, float scale, vec4_t color, qboolean voice, int textStyle ) {
	clientInfo_t *ci;

//	ci = cgs.clientinfo + ((voice) ? cgs.currentVoiceClient : sortedTeamPlayers[CG_GetSelectedPlayer()]);
	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if ( ci ) {
		CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color, ci->name, 0, 0, textStyle );
	}
}

static void CG_DrawSelectedPlayerLocation( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	clientInfo_t *ci;
	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if ( ci ) {
		const char *p = CG_ConfigString( CS_LOCATIONS + ci->location );

		if ( !p || !*p ) {
			p = "unknown";
		}
		CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color, p, 0, 0, textStyle );
	}
}

static void CG_DrawPlayerLocation( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle  ) {
	clientInfo_t *ci = &cgs.clientinfo[cg.snap->ps.clientNum];
	if ( ci ) {
		const char *p = CG_ConfigString( CS_LOCATIONS + ci->location );
		if ( !p || !*p ) {
			p = "unknown";
		}
		CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color, p, 0, 0, textStyle );
	}
}



static void CG_DrawSelectedPlayerWeapon( rectDef_t *rect ) {
	clientInfo_t *ci;

	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if ( ci ) {
		if ( cg_weapons[ci->curWeapon].weaponIcon[1] ) {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_weapons[ci->curWeapon].weaponIcon[1] );  // (SA) using the 'selected' version of the icon
		} else {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.deferShader );
		}
	}
}

static void CG_DrawPlayerScore( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	char num[16];
	int value = cg.snap->ps.persistant[PERS_SCORE];

	if ( shader ) {
		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		trap_R_SetColor( NULL );
	} else {
		Com_sprintf( num, sizeof( num ), "%i", value );
		value = CG_Text_Width( num, font, scale, 0 );
		CG_Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
	}
}


static void CG_DrawHoldableItem( rectDef_t *rect, int font, float scale, qboolean draw2D ) {
	int value;
	gitem_t *item;

	item    = BG_FindItemForHoldable( cg.holdableSelect );

	if ( !item ) {
		return;
	}

	value   = cg.predictedPlayerState.holdable[cg.holdableSelect];

	if ( value ) {
//		CG_RegisterItemVisuals( value );
		CG_RegisterItemVisuals( item - bg_itemlist );

		if ( cg.holdableSelect == HI_WINE ) {
			if ( value > 3 ) {
				value = 3;  // 3 stages to icon, just draw full if beyond 'full'
			}
//			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icons[2-(value-1)] );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[item - bg_itemlist].icons[2 - ( value - 1 )] );
		} else {
//			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icons[0] );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[item - bg_itemlist].icons[0] );
		}
	}
}

void flubfoo() {
	int value;
	gitem_t *item;

	if ( !cg.holdableSelect ) {
		return;
	}

	item    = BG_FindItemForHoldable( cg.holdableSelect );

	if ( !item ) {
		return;
	}

	value   = cg.predictedPlayerState.holdable[cg.holdableSelect];

	if ( value ) {

		trap_R_SetColor( NULL );

		CG_RegisterItemVisuals( item - bg_itemlist );

		if ( cg.holdableSelect == HI_WINE ) {
			if ( value > 3 ) {
				value = 3;  // 3 stages to icon, just draw full if beyond 'full'

			}
			//----(SA)	trying smaller text
			//----(SA)	and off to the right side of the HUD
//			CG_DrawPic( 100, (SCREEN_HEIGHT-ICON_SIZE)-8, ICON_SIZE/2, ICON_SIZE, cg_items[item - bg_itemlist].icons[2-(value-1)] );
			CG_DrawPic( 606, 366, 24, 48, cg_items[item - bg_itemlist].icons[2 - ( value - 1 )] );

		} else {
//			CG_DrawPic( 100, (SCREEN_HEIGHT-ICON_SIZE)-8, ICON_SIZE/2, ICON_SIZE, cg_items[item - bg_itemlist].icons[0] );
			CG_DrawPic( 606, 366, 24, 48, cg_items[item - bg_itemlist].icons[0] );

		}

		// draw the selection box so it's not just floating in space
		CG_DrawPic( 606 - 4, 366 - 4, 32, 56, cgs.media.selectShader );
	}
}



static void CG_DrawPlayerItem( rectDef_t *rect, int font, float scale, qboolean draw2D ) {
	int value;
	vec3_t origin, angles;

	value = cg.snap->ps.stats[STAT_HOLDABLE_ITEM];

	if ( value ) {
		CG_RegisterItemVisuals( value );

		if ( qtrue ) {
			CG_RegisterItemVisuals( value );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icons[0] );
		} else {
			VectorClear( angles );
			origin[0] = 90;
			origin[1] = 0;
			origin[2] = -10;
			angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
			CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].models[0], 0, origin, angles );
		}
	}
}


static void CG_DrawSelectedPlayerPowerup( rectDef_t *rect, qboolean draw2D ) {
	clientInfo_t *ci;
	int j;
	float x, y;

	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
	if ( ci ) {
		x = rect->x;
		y = rect->y;

		for ( j = 0; j < PW_NUM_POWERUPS; j++ ) {
			if ( ci->powerups & ( 1 << j ) ) {
				gitem_t *item;
				item = BG_FindItemForPowerup( j );
				if ( item ) {
					CG_DrawPic( x, y, rect->w, rect->h, trap_R_RegisterShader( item->icon ) );
					x += 3;
					y += 3;
					return;
				}
			}
		}
	}
}


static void CG_DrawSelectedPlayerHead( rectDef_t *rect, qboolean draw2D, qboolean voice ) {
	clipHandle_t cm;
	clientInfo_t    *ci;
	float len;
	vec3_t origin;
	vec3_t mins, maxs, angles;


//	ci = cgs.clientinfo + ((voice) ? cgs.currentVoiceClient : sortedTeamPlayers[CG_GetSelectedPlayer()]);
	ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];

	if ( ci ) {
		if ( cg_draw3dIcons.integer ) {
			cm = ci->headModel;
			if ( !cm ) {
				return;
			}

			// offset the origin y and z to center the head
			trap_R_ModelBounds( cm, mins, maxs );

			origin[2] = -0.5 * ( mins[2] + maxs[2] );
			origin[1] = 0.5 * ( mins[1] + maxs[1] );

			// calculate distance so the head nearly fills the box
			// assume heads are taller than wide
			len = 0.7 * ( maxs[2] - mins[2] );
			origin[0] = len / 0.268;    // len / tan( fov/2 )

			// allow per-model tweaking
			VectorAdd( origin, ci->modelInfo->headOffset, origin );

			angles[PITCH] = 0;
			angles[YAW] = 180;
			angles[ROLL] = 0;

			CG_Draw3DModel( rect->x, rect->y, rect->w, rect->h, ci->headModel, ci->headSkin, origin, angles );

		} else if ( cg_drawIcons.integer ) {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, ci->modelIcon );
		}

		// if they are deferred, draw a cross out
		if ( ci->deferred ) {
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.deferShader );
		}
	}
}


static void CG_DrawPlayerHealth( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	playerState_t   *ps;
	int value;
	char num[16];

	ps = &cg.snap->ps;

	value = ps->stats[STAT_HEALTH];

	if ( shader ) {
		trap_R_SetColor( color );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
		trap_R_SetColor( NULL );
	} else {
		Com_sprintf( num, sizeof( num ), "%i", value );
		value = CG_Text_Width( num, font, scale, 0 );
		CG_Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
	}
}


static void CG_DrawRedScore( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	int value;
	char num[16];
	if ( cgs.scores1 == SCORE_NOT_PRESENT ) {
		Com_sprintf( num, sizeof( num ), "-" );
	} else {
		Com_sprintf( num, sizeof( num ), "%i", cgs.scores1 );
	}
	value = CG_Text_Width( num, font, scale, 0 );
	CG_Text_Paint( rect->x + rect->w - value, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
}

static void CG_DrawBlueScore( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	int value;
	char num[16];

	if ( cgs.scores2 == SCORE_NOT_PRESENT ) {
		Com_sprintf( num, sizeof( num ), "-" );
	} else {
		Com_sprintf( num, sizeof( num ), "%i", cgs.scores2 );
	}
	value = CG_Text_Width( num, font, scale, 0 );
	CG_Text_Paint( rect->x + rect->w - value, rect->y + rect->h, font, scale, color, num, 0, 0, textStyle );
}

// FIXME: team name support
static void CG_DrawRedName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
#ifdef MISSIONPACK
	CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color, cg_redTeamName.string, 0, 0, textStyle );
#endif  // #ifdef MISSIONPACK
}

static void CG_DrawBlueName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
#ifdef MISSIONPACK
	CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color, cg_blueTeamName.string, 0, 0, textStyle );
#endif  // #ifdef MISSIONPACK
}

static void CG_DrawBlueFlagName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
#ifdef MISSIONPACK
	int i;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_RED  && cgs.clientinfo[i].powerups & ( 1 << PW_BLUEFLAG ) ) {
			CG_Text_Paint( rect->x, rect->y + rect->h, scale, color, cgs.clientinfo[i].name, 0, 0, textStyle );
			return;
		}
	}
#endif  // #ifdef MISSIONPACK
}

static void CG_DrawBlueFlagStatus( rectDef_t *rect, qhandle_t shader ) {
#ifdef MISSIONPACK
	if ( cgs.gametype != GT_CTF && cgs.gametype != GT_1FCTF ) {
		if ( cgs.gametype == GT_HARVESTER ) {
			vec4_t color = {0, 0, 1, 1};
			trap_R_SetColor( color );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.blueCubeIcon );
			trap_R_SetColor( NULL );
		}
		return;
	}
	if ( shader ) {
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	} else {
		gitem_t *item = BG_FindItemForPowerup( PW_BLUEFLAG );
		if ( item ) {
			vec4_t color = {0, 0, 1, 1};
			trap_R_SetColor( color );
			if ( cgs.blueflag >= 0 && cgs.blueflag <= 2 ) {
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[cgs.blueflag] );
			} else {
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[0] );
			}
			trap_R_SetColor( NULL );
		}
	}
#endif  // #ifdef MISSIONPACK
}

static void CG_DrawBlueFlagHead( rectDef_t *rect ) {
#ifdef MISSIONPACK
	int i;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_RED  && cgs.clientinfo[i].powerups & ( 1 << PW_BLUEFLAG ) ) {
			vec3_t angles;
			VectorClear( angles );
			angles[YAW] = 180 + 20 * sin( cg.time / 650.0 );;
			CG_DrawHead( rect->x, rect->y, rect->w, rect->h, 0,angles );
			return;
		}
	}
#endif  // #ifdef MISSIONPACK
}

static void CG_DrawRedFlagName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
#ifdef MISSIONPACK
	int i;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_BLUE  && cgs.clientinfo[i].powerups & ( 1 << PW_REDFLAG ) ) {
			CG_Text_Paint( rect->x, rect->y + rect->h, scale, color, cgs.clientinfo[i].name, 0, 0, textStyle );
			return;
		}
	}
#endif  // #ifdef MISSIONPACK
}

static void CG_DrawRedFlagStatus( rectDef_t *rect, qhandle_t shader ) {
#ifdef MISSIONPACK
	if ( cgs.gametype != GT_CTF && cgs.gametype != GT_1FCTF ) {
		if ( cgs.gametype == GT_HARVESTER ) {
			vec4_t color = {1, 0, 0, 1};
			trap_R_SetColor( color );
			CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.redCubeIcon );
			trap_R_SetColor( NULL );
		}
		return;
	}
	if ( shader ) {
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );
	} else {
		gitem_t *item = BG_FindItemForPowerup( PW_REDFLAG );
		if ( item ) {
			vec4_t color = {1, 0, 0, 1};
			trap_R_SetColor( color );
			if ( cgs.redflag >= 0 && cgs.redflag <= 2 ) {
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[cgs.redflag] );
			} else {
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[0] );
			}
			trap_R_SetColor( NULL );
		}
	}
#endif  // #ifdef MISSIONPACK
}

static void CG_DrawRedFlagHead( rectDef_t *rect ) {
#ifdef MISSIONPACK
	int i;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if ( cgs.clientinfo[i].infoValid && cgs.clientinfo[i].team == TEAM_BLUE  && cgs.clientinfo[i].powerups & ( 1 << PW_REDFLAG ) ) {
			vec3_t angles;
			VectorClear( angles );
			angles[YAW] = 180 + 20 * sin( cg.time / 650.0 );;
			CG_DrawHead( rect->x, rect->y, rect->w, rect->h, 0,angles );
			return;
		}
	}
#endif  // #ifdef MISSIONPACK
}

static void CG_HarvesterSkulls( rectDef_t *rect, int font, float scale, vec4_t color, qboolean force2D, int textStyle ) {
#ifdef MISSIONPACK
	char num[16];
	vec3_t origin, angles;
	qhandle_t handle;
	int value = cg.snap->ps.generic1;

	if ( cgs.gametype != GT_HARVESTER ) {
		return;
	}

	if ( value > 99 ) {
		value = 99;
	}

	Com_sprintf( num, sizeof( num ), "%i", value );
	value = CG_Text_Width( num, font, scale, 0 );
	CG_Text_Paint( rect->x + ( rect->w - value ), rect->y + rect->h, scale, color, num, 0, 0, textStyle );

	if ( cg_drawIcons.integer ) {
		if ( !force2D && cg_draw3dIcons.integer ) {
			VectorClear( angles );
			origin[0] = 90;
			origin[1] = 0;
			origin[2] = -10;
			angles[YAW] = ( cg.time & 2047 ) * 360 / 2048.0;
			if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
				handle = cgs.media.redCubeModel;
			} else {
				handle = cgs.media.blueCubeModel;
			}
			CG_Draw3DModel( rect->x, rect->y, 35, 35, handle, 0, origin, angles );
		} else {
			if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
				handle = cgs.media.redCubeIcon;
			} else {
				handle = cgs.media.blueCubeIcon;
			}
			CG_DrawPic( rect->x + 3, rect->y + 16, 20, 20, handle );
		}
	}
#endif  // #ifdef MISSIONPACK
}

static void CG_OneFlagStatus( rectDef_t *rect ) {
#ifdef MISSIONPACK
	if ( cgs.gametype != GT_1FCTF ) {
		return;
	} else {
		gitem_t *item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
		if ( item ) {
			if ( cgs.flagStatus >= 0 && cgs.flagStatus <= 4 ) {
				vec4_t color = {1, 1, 1, 1};
				int index = 0;
				if ( cgs.flagStatus == FLAG_TAKEN_RED ) {
					color[1] = color[2] = 0;
					index = 1;
				} else if ( cgs.flagStatus == FLAG_TAKEN_BLUE ) {
					color[0] = color[1] = 0;
					index = 1;
				} else if ( cgs.flagStatus == FLAG_DROPPED ) {
					index = 2;
				}
				trap_R_SetColor( color );
				CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cgs.media.flagShaders[index] );
			}
		}
	}
#endif  // #ifdef MISSIONPACK
}


static void CG_DrawCTFPowerUp( rectDef_t *rect ) {
#ifdef MISSIONPACK
	int value;
	value = cg.snap->ps.stats[STAT_PERSISTANT_POWERUP];
	if ( value ) {
		CG_RegisterItemVisuals( value );
		CG_DrawPic( rect->x, rect->y, rect->w, rect->h, cg_items[ value ].icon );
	}
#endif  // #ifdef MISSIONPACK
}



static void CG_DrawTeamColor( rectDef_t *rect, vec4_t color ) {
	CG_DrawTeamBackground( rect->x, rect->y, rect->w, rect->h, color[3], cg.snap->ps.persistant[PERS_TEAM] );
}


static void CG_DrawAreaHoldable( rectDef_t *rect, int align, float spacing, int font, float scale, vec4_t color ) {
}

static void CG_DrawAreaWeapons( rectDef_t *rect, int align, float spacing, int font, float scale, vec4_t color ) {
}


static void CG_DrawAreaPowerUp( rectDef_t *rect, int align, float spacing, int font, float scale, vec4_t color ) {
	char num[16];
	int sorted[MAX_POWERUPS];
	int sortedTime[MAX_POWERUPS];
	int i, j, k;
	int active;
	playerState_t   *ps;
	int t;
	gitem_t *item;
	float f;
	rectDef_t r2;
	float *inc;
	r2.x = rect->x;
	r2.y = rect->y;
	r2.w = rect->w;
	r2.h = rect->h;

	inc = ( align == HUD_VERTICAL ) ? &r2.y : &r2.x;

	ps = &cg.snap->ps;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	// sort the list by time remaining
	active = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( !ps->powerups[ i ] ) {
			continue;
		}
		t = ps->powerups[ i ] - cg.time;
		// ZOID--don't draw if the power up has unlimited time (999 seconds)
		// This is true of the CTF flags
		if ( t <= 0 || t >= 999000 ) {
			continue;
		}

		// insert into the list
		for ( j = 0 ; j < active ; j++ ) {
			if ( sortedTime[j] >= t ) {
				for ( k = active - 1 ; k >= j ; k-- ) {
					sorted[k + 1] = sorted[k];
					sortedTime[k + 1] = sortedTime[k];
				}
				break;
			}
		}
		sorted[j] = i;
		sortedTime[j] = t;
		active++;
	}

	// draw the icons and timers
	for ( i = 0 ; i < active ; i++ ) {
		item = BG_FindItemForPowerup( sorted[i] );

		if ( item ) {
			t = ps->powerups[ sorted[i] ];
			if ( t - cg.time >= POWERUP_BLINKS * POWERUP_BLINK_TIME ) {
				trap_R_SetColor( NULL );
			} else {
				vec4_t modulate;

				f = (float)( t - cg.time ) / POWERUP_BLINK_TIME;
				f -= (int)f;
				modulate[0] = modulate[1] = modulate[2] = modulate[3] = f;
				trap_R_SetColor( modulate );
			}

			CG_DrawPic( r2.x, r2.y, r2.w * .75, r2.h, trap_R_RegisterShader( item->icon ) );

			Com_sprintf( num, sizeof( num ), "%i", sortedTime[i] / 1000 );
			CG_Text_Paint( r2.x + ( r2.w * .75 ) + 3, r2.y + r2.h, font, scale, color, num, 0, 0, 0 );
			*inc += r2.w + spacing;
		}

	}
	trap_R_SetColor( NULL );

}

float CG_GetValue( int ownerDraw, int type ) {
	centity_t   *cent;
	clientInfo_t *ci;
	playerState_t   *ps;

	cent = &cg_entities[cg.snap->ps.clientNum];
	ps = &cg.snap->ps;

	switch ( ownerDraw ) {

	case CG_SELECTEDPLAYER_ARMOR:
		ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
		return ci->armor;
		break;
	case CG_SELECTEDPLAYER_HEALTH:
		ci = cgs.clientinfo + sortedTeamPlayers[CG_GetSelectedPlayer()];
		return ci->health;
		break;

	case CG_PLAYER_ARMOR_VALUE:
		return ps->stats[STAT_ARMOR];
		break;
	case CG_PLAYER_AMMO_VALUE:
		if ( cent->currentState.weapon ) {
			if ( type == RANGETYPE_RELATIVE ) {
				int weap = BG_FindAmmoForWeapon( cent->currentState.weapon );
				return (float)ps->ammo[weap] / (float)ammoTable[weap].maxammo;
			} else {
				return ps->ammo[BG_FindAmmoForWeapon( cent->currentState.weapon )];
			}
		}
		break;
	case CG_PLAYER_AMMOCLIP_VALUE:
		if ( cent->currentState.weapon ) {
			if ( type == RANGETYPE_RELATIVE ) {
				return (float)ps->ammoclip[BG_FindClipForWeapon( cent->currentState.weapon )] / (float)ammoTable[cent->currentState.weapon].maxclip;
			} else {
				return ps->ammoclip[BG_FindClipForWeapon( cent->currentState.weapon )];
			}
		}
		break;
	case CG_PLAYER_SCORE:
		return cg.snap->ps.persistant[PERS_SCORE];
		break;
	case CG_PLAYER_HEALTH:
		return ps->stats[STAT_HEALTH];
		break;
	case CG_RED_SCORE:
		return cgs.scores1;
		break;
	case CG_BLUE_SCORE:
		return cgs.scores2;
		break;
	case CG_PLAYER_WEAPON_STABILITY:    //----(SA)	added
		return ps->aimSpreadScale;
		break;

#define BONUSTIME 60000.0f
#define SPRINTTIME 20000.0f

	case CG_STAMINA:    //----(SA)	added
		if ( type == RANGETYPE_RELATIVE ) {
			return (float)cg.snap->ps.sprintTime / SPRINTTIME;
		} else {
			return cg.snap->ps.sprintTime;
		}
		break;
	default:
		break;
	}

	return -1;
}

qboolean CG_OtherTeamHasFlag() {
#ifdef MISSIONPACK
	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF ) {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		if ( cgs.gametype == GT_1FCTF ) {
			if ( team == TEAM_RED && cgs.flagStatus == FLAG_TAKEN_BLUE ) {
				return qtrue;
			} else if ( team == TEAM_BLUE && cgs.flagStatus == FLAG_TAKEN_RED ) {
				return qtrue;
			} else {
				return qfalse;
			}
		} else {
			if ( team == TEAM_RED && cgs.redflag == FLAG_TAKEN ) {
				return qtrue;
			} else if ( team == TEAM_BLUE && cgs.blueflag == FLAG_TAKEN ) {
				return qtrue;
			} else {
				return qfalse;
			}
		}
	}
#endif  // #ifdef MISSIONPACK
	return qfalse;
}

qboolean CG_YourTeamHasFlag() {
#ifdef MISSIONPACK
	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF ) {
		int team = cg.snap->ps.persistant[PERS_TEAM];
		if ( cgs.gametype == GT_1FCTF ) {
			if ( team == TEAM_RED && cgs.flagStatus == FLAG_TAKEN_RED ) {
				return qtrue;
			} else if ( team == TEAM_BLUE && cgs.flagStatus == FLAG_TAKEN_BLUE ) {
				return qtrue;
			} else {
				return qfalse;
			}
		} else {
			if ( team == TEAM_RED && cgs.blueflag == FLAG_TAKEN ) {
				return qtrue;
			} else if ( team == TEAM_BLUE && cgs.redflag == FLAG_TAKEN ) {
				return qtrue;
			} else {
				return qfalse;
			}
		}
	}
#endif  // #ifdef MISSIONPACK
	return qfalse;
}

// THINKABOUTME: should these be exclusive or inclusive..
//
qboolean CG_OwnerDrawVisible( int flags ) {

//----(SA)	added
	if ( flags & CG_SHOW_NOT_V_BINOC ) {      // if looking through binocs
		if ( cg.zoomedBinoc ) {
			return qfalse;
		}
	}

	if ( flags & CG_SHOW_NOT_V_SNIPER ) {     // if looking through sniper scope
		if ( cg.weaponSelect == WP_SNIPERRIFLE ) {
			return qfalse;
		}
	}

	if ( flags & CG_SHOW_NOT_V_SNOOPER ) {        // if looking through snooper scope
		if ( cg.weaponSelect == WP_SNOOPERSCOPE ) {
			return qfalse;
		}
	}

	if ( flags & CG_SHOW_NOT_V_FGSCOPE ) {        // if looking through fg42 scope
		if ( cg.weaponSelect == WP_FG42SCOPE ) {
			return qfalse;
		}
	}

//----(SA)	end

	if ( flags & CG_SHOW_TEAMINFO ) {
		return ( cg_currentSelectedPlayer.integer == numSortedTeamPlayers );
	}

	if ( flags & CG_SHOW_NOTEAMINFO ) {
		return !( cg_currentSelectedPlayer.integer == numSortedTeamPlayers );
	}

	if ( flags & CG_SHOW_OTHERTEAMHASFLAG ) {
		return CG_OtherTeamHasFlag();
	}

	if ( flags & CG_SHOW_YOURTEAMHASENEMYFLAG ) {
		return CG_YourTeamHasFlag();
	}

#ifdef MISSIONPACK
	if ( flags & ( CG_SHOW_BLUE_TEAM_HAS_REDFLAG | CG_SHOW_RED_TEAM_HAS_BLUEFLAG ) ) {
		if ( flags & CG_SHOW_BLUE_TEAM_HAS_REDFLAG && ( cgs.redflag == FLAG_TAKEN || cgs.flagStatus == FLAG_TAKEN_RED ) ) {
			return qtrue;
		} else if ( flags & CG_SHOW_RED_TEAM_HAS_BLUEFLAG && ( cgs.blueflag == FLAG_TAKEN || cgs.flagStatus == FLAG_TAKEN_BLUE ) ) {
			return qtrue;
		}
		return qfalse;
	}
#endif  // #ifdef MISSIONPACK

	if ( flags & CG_SHOW_ANYTEAMGAME ) {
		if ( cgs.gametype >= GT_TEAM ) {
			return qtrue;
		}
	}

	if ( flags & CG_SHOW_ANYNONTEAMGAME ) {
		if ( cgs.gametype < GT_TEAM ) {
			return qtrue;
		}
	}

#ifdef MISSIONPACK
	if ( flags & CG_SHOW_HARVESTER ) {
		if ( cgs.gametype == GT_HARVESTER ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

	if ( flags & CG_SHOW_ONEFLAG ) {
		if ( cgs.gametype == GT_1FCTF ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}

#endif  // #ifdef MISSIONPACK
	if ( flags & CG_SHOW_CTF ) {
		if ( cgs.gametype == GT_CTF ) {
			return qtrue;
		}
	}

#ifdef MISSIONPACK
	if ( flags & CG_SHOW_OBELISK ) {
		if ( cgs.gametype == GT_OBELISK ) {
			return qtrue;
		} else {
			return qfalse;
		}
	}
#endif  // #ifdef MISSIONPACK

	if ( flags & CG_SHOW_HEALTHCRITICAL ) {
		if ( cg.snap->ps.stats[STAT_HEALTH] < 25 ) {
			return qtrue;
		}
	}

	if ( flags & CG_SHOW_HEALTHOK ) {
		if ( cg.snap->ps.stats[STAT_HEALTH] > 25 ) {
			return qtrue;
		}
	}

	if ( flags & CG_SHOW_SINGLEPLAYER ) {
		if ( cgs.gametype == GT_SINGLE_PLAYER ) {
			return qtrue;
		}
	}

	if ( flags & CG_SHOW_TOURNAMENT ) {
		if ( cgs.gametype == GT_TOURNAMENT ) {
			return qtrue;
		}
	}

	if ( flags & CG_SHOW_DURINGINCOMINGVOICE ) {
	}

#ifdef MISSIONPACK
	if ( flags & CG_SHOW_IF_PLAYER_HAS_FLAG ) {
		if ( cg.snap->ps.powerups[PW_REDFLAG] || cg.snap->ps.powerups[PW_BLUEFLAG] || cg.snap->ps.powerups[PW_NEUTRALFLAG] ) {
			return qtrue;
		}
	}
#endif  // #ifdef MISSIONPACK

//----(SA)	added
	if ( flags & CG_SHOW_NOT_V_CLEAR ) {
		// if /not/ looking through binocs,snooper or sniper
		if ( !cg.zoomedBinoc && !( cg.weaponSelect == WP_SNIPERRIFLE ) && !( cg.weaponSelect == WP_SNOOPERSCOPE ) && !( cg.weaponSelect == WP_FG42SCOPE ) ) {
			return qfalse;
		}
	}

	if ( flags & ( CG_SHOW_NOT_V_BINOC | CG_SHOW_NOT_V_SNIPER | CG_SHOW_NOT_V_SNOOPER | CG_SHOW_NOT_V_FGSCOPE ) ) {
		// setting any of these does not necessarily disable drawing in regular view
		// CG_SHOW_NOT_V_CLEAR must also be set to hide for reg view
		if ( !( flags & CG_SHOW_NOT_V_CLEAR ) ) {
			return qtrue;
		}
	}

//----(SA)	end


	return qfalse;
}



static void CG_DrawPlayerHasFlag( rectDef_t *rect, qboolean force2D ) {
#ifdef MISSIONPACK
	int adj = ( force2D ) ? 0 : 2;
	if ( cg.predictedPlayerState.powerups[PW_REDFLAG] ) {
		CG_DrawFlagModel( rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_RED, force2D );
	} else if ( cg.predictedPlayerState.powerups[PW_BLUEFLAG] ) {
		CG_DrawFlagModel( rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_BLUE, force2D );
	} else if ( cg.predictedPlayerState.powerups[PW_NEUTRALFLAG] ) {
		CG_DrawFlagModel( rect->x + adj, rect->y + adj, rect->w - adj, rect->h - adj, TEAM_FREE, force2D );
	}
#endif  // #ifdef MISSIONPACK
}

static void CG_DrawAreaSystemChat( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader ) {
	CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color, systemChat, 0, 0, 0 );
}

static void CG_DrawAreaTeamChat( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader ) {
	CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color,teamChat1, 0, 0, 0 );
}

static void CG_DrawAreaChat( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader ) {
	CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color, teamChat2, 0, 0, 0 );
}

const char *CG_GetKillerText() {
	const unsigned char *s = "";
	if ( cg.killerName[0] ) {
		s = va( "Fragged by %s", cg.killerName );
	}
	return s;
}


static void CG_DrawKiller( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	// fragged by ... line
	if ( cg.killerName[0] ) {
		int x = rect->x + rect->w / 2;
		CG_Text_Paint( x - CG_Text_Width( CG_GetKillerText(),font, scale, 0 ) / 2, rect->y + rect->h, font, scale, color, CG_GetKillerText(), 0, 0, textStyle );
	}

}


static void CG_DrawCapFragLimit( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	int limit = ( cgs.gametype >= GT_CTF ) ? cgs.capturelimit : cgs.fraglimit;
	CG_Text_Paint( rect->x, rect->y, font, scale, color, va( "%2i", limit ),0, 0, textStyle );
}

static void CG_Draw1stPlace( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	if ( cgs.scores1 != SCORE_NOT_PRESENT ) {
		CG_Text_Paint( rect->x, rect->y, font, scale, color, va( "%2i", cgs.scores1 ),0, 0, textStyle );
	}
}

static void CG_Draw2ndPlace( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	if ( cgs.scores2 != SCORE_NOT_PRESENT ) {
		CG_Text_Paint( rect->x, rect->y, font, scale, color, va( "%2i", cgs.scores2 ),0, 0, textStyle );
	}
}

const char *CG_GetGameStatusText() {
	const unsigned char *s = "";
	if ( cgs.gametype < GT_TEAM ) {
		if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			s = va( "%s place with %i",CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),cg.snap->ps.persistant[PERS_SCORE] );
		}
	} else {
		if ( cg.teamScores[0] == cg.teamScores[1] ) {
			s = va( "Teams are tied at %i", cg.teamScores[0] );
		} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			s = va( "Red leads Blue, %i to %i", cg.teamScores[0], cg.teamScores[1] );
		} else {
			s = va( "Blue leads Red, %i to %i", cg.teamScores[1], cg.teamScores[0] );
		}
	}
	return s;
}

static void CG_DrawGameStatus( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color, CG_GetGameStatusText(), 0, 0, textStyle );
}

const char *CG_GameTypeString() {
	if ( cgs.gametype == GT_FFA ) {
		return "Free For All";
	} else if ( cgs.gametype == GT_TEAM ) {
		return "Team Deathmatch";
	} else if ( cgs.gametype == GT_CTF ) {
		return "Capture the Flag";
#ifdef MISSIONPACK
	} else if ( cgs.gametype == GT_1FCTF ) {
		return "One Flag CTF";
	} else if ( cgs.gametype == GT_OBELISK ) {
		return "Overload";
	} else if ( cgs.gametype == GT_HARVESTER ) {
		return "Harvester";
#endif  // #ifdef MISSIONPACK
	}
	return "";
}
static void CG_DrawGameType( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	CG_Text_Paint( rect->x, rect->y + rect->h, font, scale, color, CG_GameTypeString(), 0, 0, textStyle );
}

static void CG_Text_Paint_Limit( float *maxX, float x, float y, int font, float scale, vec4_t color, const char* text, float adjust, int limit ) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	if ( text ) {
		const unsigned char *s = text;
		float max = *maxX;
		float useScale;

		fontInfo_t *fnt = &cgDC.Assets.textFont;
		if ( font == UI_FONT_DEFAULT ) {
			if ( scale <= cg_smallFont.value ) {
				fnt = &cgDC.Assets.smallFont;
			} else if ( scale > cg_bigFont.value ) {
				fnt = &cgDC.Assets.bigFont;
			}
		} else if ( font == UI_FONT_BIG ) {
			fnt = &cgDC.Assets.bigFont;
		} else if ( font == UI_FONT_SMALL ) {
			fnt = &cgDC.Assets.smallFont;
		} else if ( font == UI_FONT_HANDWRITING ) {
			fnt = &cgDC.Assets.handwritingFont;
		}

		useScale = scale * fnt->glyphScale;
		trap_R_SetColor( color );
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			glyph = &fnt->glyphs[*s];
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if ( CG_Text_Width( s, font, useScale, 1 ) + x > max ) {
					*maxX = 0;
					break;
				}
				CG_Text_PaintChar( x, y - yadj,
								   glyph->imageWidth,
								   glyph->imageHeight,
								   useScale,
								   glyph->s,
								   glyph->t,
								   glyph->s2,
								   glyph->t2,
								   glyph->glyph );
				x += ( glyph->xSkip * useScale ) + adjust;
				*maxX = x;
				count++;
				s++;
			}
		}
		trap_R_SetColor( NULL );
	}

}



#define PIC_WIDTH 12

void CG_DrawNewTeamInfo( rectDef_t *rect, float text_x, float text_y, int font, float scale, vec4_t color, qhandle_t shader ) {
#ifdef MISSIONPACK
	int xx;
	float y;
	int i, j, len, count;
	const char *p;
	vec4_t hcolor;
	float pwidth, lwidth, maxx, leftOver;
	clientInfo_t *ci;
	gitem_t *item;
	qhandle_t h;

	// max player name width
	pwidth = 0;
	count = ( numSortedTeamPlayers > 8 ) ? 8 : numSortedTeamPlayers;
	for ( i = 0; i < count; i++ ) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM] ) {
			len = CG_Text_Width( ci->name, font, scale, 0 );
			if ( len > pwidth ) {
				pwidth = len;
			}
		}
	}

	// max location name width
	lwidth = 0;
	for ( i = 1; i < MAX_LOCATIONS; i++ ) {
		p = CG_ConfigString( CS_LOCATIONS + i );
		if ( p && *p ) {
			len = CG_Text_Width( p, font, scale, 0 );
			if ( len > lwidth ) {
				lwidth = len;
			}
		}
	}

	y = rect->y;

	for ( i = 0; i < count; i++ ) {
		ci = cgs.clientinfo + sortedTeamPlayers[i];
		if ( ci->infoValid && ci->team == cg.snap->ps.persistant[PERS_TEAM] ) {

			xx = rect->x + 1;
			for ( j = 0; j <= PW_NUM_POWERUPS; j++ ) {
				if ( ci->powerups & ( 1 << j ) ) {

					item = BG_FindItemForPowerup( j );

					if ( item ) {
						CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, trap_R_RegisterShader( item->icon ) );
						xx += PIC_WIDTH;
					}
				}
			}

			// FIXME: max of 3 powerups shown properly
			xx = rect->x + ( PIC_WIDTH * 3 ) + 2;

			CG_GetColorForHealth( ci->health, ci->armor, hcolor );
			trap_R_SetColor( hcolor );
			CG_DrawPic( xx, y + 1, PIC_WIDTH - 2, PIC_WIDTH - 2, cgs.media.heartShader );

			//Com_sprintf (st, sizeof(st), "%3i %3i", ci->health,	ci->armor);
			//CG_Text_Paint(xx, y + text_y, scale, hcolor, st, 0, 0);

			// draw weapon icon
			xx += PIC_WIDTH + 1;

// weapon used is not that useful, use the space for task
#if 0
			if ( cg_weapons[ci->curWeapon].weaponIcon ) {
				CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, cg_weapons[ci->curWeapon].weaponIcon );
			} else {
				CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, cgs.media.deferShader );
			}
#endif

			trap_R_SetColor( NULL );
			if ( cgs.orderPending ) {
				// blink the icon
				if ( cg.time > cgs.orderTime - 2500 && ( cg.time >> 9 ) & 1 ) {
					h = 0;
				} else {
					h = CG_StatusHandle( cgs.currentOrder );
				}
			}   else {
				h = CG_StatusHandle( ci->teamTask );
			}

			if ( h ) {
				CG_DrawPic( xx, y, PIC_WIDTH, PIC_WIDTH, h );
			}

			xx += PIC_WIDTH + 1;

			leftOver = rect->w - xx;
			maxx = xx + leftOver / 3;



			CG_Text_Paint_Limit( &maxx, xx, y + text_y, UI_FONT_DEFAULT, scale, color, ci->name, 0, 0 );

			p = CG_ConfigString( CS_LOCATIONS + ci->location );
			if ( !p || !*p ) {
				p = "unknown";
			}

			xx += leftOver / 3 + 2;
			maxx = rect->w - 4;

			CG_Text_Paint_Limit( &maxx, xx, y + text_y, UI_FONT_DEFAULT, scale, color, p, 0, 0 );
			y += text_y + 2;
			if ( y + text_y + 2 > rect->y + rect->h ) {
				break;
			}

		}
	}
#endif  // #ifdef MISSIONPACK
}


void CG_DrawTeamSpectators( rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader ) {
	if ( cg.spectatorLen ) {
		float maxX;

		if ( cg.spectatorWidth == -1 ) {
			cg.spectatorWidth = 0;
			cg.spectatorPaintX = rect->x + 1;
			cg.spectatorPaintX2 = -1;
		}

		if ( cg.spectatorOffset > cg.spectatorLen ) {
			cg.spectatorOffset = 0;
			cg.spectatorPaintX = rect->x + 1;
			cg.spectatorPaintX2 = -1;
		}

		if ( cg.time > cg.spectatorTime ) {
			cg.spectatorTime = cg.time + 10;
			if ( cg.spectatorPaintX <= rect->x + 2 ) {
				if ( cg.spectatorOffset < cg.spectatorLen ) {
					cg.spectatorPaintX += CG_Text_Width( &cg.spectatorList[cg.spectatorOffset], font, scale, 1 ) - 1;
					cg.spectatorOffset++;
				} else {
					cg.spectatorOffset = 0;
					if ( cg.spectatorPaintX2 >= 0 ) {
						cg.spectatorPaintX = cg.spectatorPaintX2;
					} else {
						cg.spectatorPaintX = rect->x + rect->w - 2;
					}
					cg.spectatorPaintX2 = -1;
				}
			} else {
				cg.spectatorPaintX--;
				if ( cg.spectatorPaintX2 >= 0 ) {
					cg.spectatorPaintX2--;
				}
			}
		}

		maxX = rect->x + rect->w - 2;
		CG_Text_Paint_Limit( &maxX, cg.spectatorPaintX, rect->y + rect->h - 3, UI_FONT_DEFAULT, scale, color, &cg.spectatorList[cg.spectatorOffset], 0, 0 );
		if ( cg.spectatorPaintX2 >= 0 ) {
			float maxX2 = rect->x + rect->w - 2;
			CG_Text_Paint_Limit( &maxX2, cg.spectatorPaintX2, rect->y + rect->h - 3, UI_FONT_DEFAULT, scale, color, cg.spectatorList, 0, cg.spectatorOffset );
		}
		if ( cg.spectatorOffset && maxX > 0 ) {
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if ( cg.spectatorPaintX2 == -1 ) {
				cg.spectatorPaintX2 = rect->x + rect->w - 2;
			}
		} else {
			cg.spectatorPaintX2 = -1;
		}

	}
}



void CG_DrawMedal( int ownerDraw, rectDef_t *rect, int font, float scale, vec4_t color, qhandle_t shader ) {
	score_t *score = &cg.scores[cg.selectedScore];
	float value = 0;
	char *text = NULL;
	color[3] = 0.25;

	switch ( ownerDraw ) {
	case CG_ACCURACY:
		value = score->accuracy;
		break;
	case CG_ASSISTS:
		value = score->assistCount;
		break;
	case CG_DEFEND:
		value = score->defendCount;
		break;
	case CG_EXCELLENT:
		value = score->excellentCount;
		break;
	case CG_IMPRESSIVE:
		value = score->impressiveCount;
		break;
	case CG_PERFECT:
		value = score->perfect;
		break;
	case CG_GAUNTLET:
		value = score->guantletCount;
		break;
	case CG_CAPTURES:
		value = score->captures;
		break;
	}

	if ( value > 0 ) {
		if ( ownerDraw != CG_PERFECT ) {
			if ( ownerDraw == CG_ACCURACY ) {
				text = va( "%i%%", (int)value );
				if ( value > 50 ) {
					color[3] = 1.0;
				}
			} else {
				text = va( "%i", (int)value );
				color[3] = 1.0;
			}
		} else {
			if ( value ) {
				color[3] = 1.0;
			}
			text = "Wow";
		}
	}

	trap_R_SetColor( color );
	CG_DrawPic( rect->x, rect->y, rect->w, rect->h, shader );

	if ( text ) {
		color[3] = 1.0;
		value = CG_Text_Width( text, font, scale, 0 );
		CG_Text_Paint( rect->x + ( rect->w - value ) / 2, rect->y + rect->h + 10, font, scale, color, text, 0, 0, 0 );
	}
	trap_R_SetColor( NULL );

}


/*
==============
CG_DrawWeapStability
	draw a bar showing current stability level (0-255), max at current weapon/ability, and 'perfect' reference mark

	probably only drawn for scoped weapons
==============
*/
void CG_DrawWeapStability( rectDef_t *rect, vec4_t color, int align ) {
	vec4_t goodColor = {0, 1, 0, 0.5f}, badColor = {1, 0, 0, 0.5f};

	if ( !cg_drawSpreadScale.integer ) {
		return;
	}

	if ( cg_drawSpreadScale.integer == 1 && !( cg.weaponSelect == WP_SNOOPERSCOPE || cg.weaponSelect == WP_SNIPERRIFLE || cg.weaponSelect == WP_FG42SCOPE ) ) {
		// cg_drawSpreadScale of '1' means only draw for scoped weapons, '2' means draw all the time (for debugging)
		return;
	}

	if ( !( cg.snap->ps.aimSpreadScale ) ) {
		return;
	}

	CG_FilledBar( rect->x, rect->y, rect->w, rect->h, goodColor, badColor, NULL, (float)cg.snap->ps.aimSpreadScale / 255.0f, 2 | 4 | 256 ); // flags (BAR_CENTER|BAR_VERT|BAR_LERP_COLOR)
}


/*
==============
CG_DrawWeapHeat
==============
*/
void CG_DrawWeapHeat( rectDef_t *rect, int align ) {
	vec4_t color = {1, 0, 0, 0.2f}, color2 = {1, 0, 0, 0.5f};
	int flags = 0;

	if ( !( cg.snap->ps.curWeapHeat ) ) {
		return;
	}

	if ( align != HUD_HORIZONTAL ) {
		flags |= 4;   // BAR_VERT

	}
	flags |= 1;       // BAR_LEFT			- this is hardcoded now, but will be decided by the menu script
	flags |= 16;      // BAR_BG			- draw the filled contrast box
//	flags|=32;		// BAR_BGSPACING_X0Y5	- different style

	flags |= 256;     // BAR_COLOR_LERP
	CG_FilledBar( rect->x, rect->y, rect->w, rect->h, color, color2, NULL, (float)cg.snap->ps.curWeapHeat / 255.0f, flags );
}


/*
==============
CG_DrawFatigue
==============
*/

static void CG_DrawFatigue( rectDef_t *rect, vec4_t color, int align ) {
	//	vec4_t	color = {0, 1, 0, 1}, color2 = {1, 0, 0, 1};
	vec4_t colorBonus = {1, 1, 0, 0.45f};   // yellow (a little more solid for the 'bonus' stamina)
	float barFrac;  //, omBarFrac;
	int flags = 0;
	float chargeTime;       // DHM - Nerve

	barFrac = (float)cg.snap->ps.sprintTime / SPRINTTIME;
//	omBarFrac = 1.0f-barFrac;

	if ( align != HUD_HORIZONTAL ) {
		flags |= 4;   // BAR_VERT
		flags |= 1;   // BAR_LEFT (left, when vertical means grow 'up')
	}

	CG_FilledBar( rect->x, rect->y, rect->w, rect->h, color, NULL, NULL, (float)cg.snap->ps.sprintTime / SPRINTTIME, flags );

	// fill in the left side of the bar with the counter for the nofatigue powerup
	if ( cg.snap->ps.powerups[PW_NOFATIGUE] ) {
		CG_FilledBar( rect->x, rect->y, rect->w / 2, rect->h, colorBonus, NULL, NULL, cg.snap->ps.powerups[PW_NOFATIGUE] / BONUSTIME, flags );
	}
// JPW NERVE -- added drawWeaponPercent in multiplayer, drawn to left
	if ( cgs.gametype != GT_SINGLE_PLAYER ) {

		if ( cg.snap->ps.stats[ STAT_PLAYER_CLASS ] == PC_ENGINEER ) {
			chargeTime = cg_engineerChargeTime.value;
		} else if ( cg.snap->ps.stats[ STAT_PLAYER_CLASS ] == PC_MEDIC ) {
			chargeTime = cg_medicChargeTime.value;
		} else if ( cg.snap->ps.stats[ STAT_PLAYER_CLASS ] == PC_LT ) {
			chargeTime = cg_LTChargeTime.value;
		} else {
			chargeTime = cg_soldierChargeTime.value;
		}

		barFrac = (float)( cg.time - cg.snap->ps.classWeaponTime ) / chargeTime;

		if ( barFrac > 1.0 ) {
			barFrac = 1.0;
		}

		color[0] = 1.0f;
		color[1] = color[2] = barFrac;
		color[3] = 0.25 + barFrac * 0.5;

		CG_FilledBar( rect->x - rect->w, rect->y, rect->w / 2, rect->h, color, NULL, NULL, barFrac, flags );
	}
// jpw
}


/*
==============
CG_OwnerDraw
==============
*/
void CG_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	rectDef_t rect;

	if ( cg_drawStatus.integer == 0 ) {
		return;
	}

	//if (ownerDrawFlags != 0 && !CG_OwnerDrawVisible(ownerDrawFlags)) {
	//	return;
	//}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw ) {
	case CG_PLAYER_WEAPON_ICON2D:
		CG_DrawPlayerWeaponIcon( &rect, ownerDrawFlags & CG_SHOW_HIGHLIGHTED, align );
		break;
	case CG_PLAYER_ARMOR_ICON:
		CG_DrawPlayerArmorIcon( &rect, ownerDrawFlags & CG_SHOW_2DONLY );
		break;
	case CG_PLAYER_ARMOR_ICON2D:
		CG_DrawPlayerArmorIcon( &rect, qtrue );
		break;
	case CG_PLAYER_ARMOR_VALUE:
		CG_DrawPlayerArmorValue( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_PLAYER_AMMO_ICON:
		CG_DrawPlayerAmmoIcon( &rect, ownerDrawFlags & CG_SHOW_2DONLY );
		break;
	case CG_PLAYER_AMMO_ICON2D:
		CG_DrawPlayerAmmoIcon( &rect, qtrue );
		break;
	case CG_PLAYER_AMMO_VALUE:
		CG_DrawPlayerAmmoValue( &rect, font, scale, color, shader, textStyle, 0 );
		break;
	case CG_CURSORHINT:
		CG_DrawCursorhint( &rect );
		break;
//----(SA)	added
	case CG_NEWMESSAGE:
		CG_DrawMessageIcon( &rect );
		break;
//----(SA)	end
	case CG_PLAYER_AMMOCLIP_VALUE:
		CG_DrawPlayerAmmoValue( &rect, font, scale, color, shader, textStyle, 1 );
		break;
	case CG_SELECTEDPLAYER_HEAD:
		CG_DrawSelectedPlayerHead( &rect, ownerDrawFlags & CG_SHOW_2DONLY, qfalse );
		break;
	case CG_VOICE_HEAD:
		CG_DrawSelectedPlayerHead( &rect, ownerDrawFlags & CG_SHOW_2DONLY, qtrue );
		break;
	case CG_VOICE_NAME:
		CG_DrawSelectedPlayerName( &rect, font, scale, color, qtrue, textStyle );
		break;
	case CG_SELECTEDPLAYER_STATUS:
		CG_DrawSelectedPlayerStatus( &rect );
		break;
	case CG_SELECTEDPLAYER_ARMOR:
		CG_DrawSelectedPlayerArmor( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_SELECTEDPLAYER_HEALTH:
		CG_DrawSelectedPlayerHealth( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_SELECTEDPLAYER_NAME:
		CG_DrawSelectedPlayerName( &rect, font, scale, color, qfalse, textStyle );
		break;
	case CG_SELECTEDPLAYER_LOCATION:
		CG_DrawSelectedPlayerLocation( &rect, font, scale, color, textStyle );
		break;
	case CG_SELECTEDPLAYER_WEAPON:
		CG_DrawSelectedPlayerWeapon( &rect );
		break;
	case CG_SELECTEDPLAYER_POWERUP:
		CG_DrawSelectedPlayerPowerup( &rect, ownerDrawFlags & CG_SHOW_2DONLY );
		break;
	case CG_PLAYER_WEAPON_HEAT:
		CG_DrawWeapHeat( &rect, align );
		break;
	case CG_PLAYER_WEAPON_STABILITY:
		CG_DrawWeapStability( &rect, color, align );
		break;
	case CG_STAMINA:
		CG_DrawFatigue( &rect, color, align );
		break;
	case CG_PLAYER_HEAD:
		CG_DrawPlayerHead( &rect, ownerDrawFlags & CG_SHOW_2DONLY );
		break;
	case CG_PLAYER_HOLDABLE:
		CG_DrawHoldableItem( &rect, font, scale, ownerDrawFlags & CG_SHOW_2DONLY );
		break;
	case CG_PLAYER_ITEM:
		CG_DrawPlayerItem( &rect, font, scale, ownerDrawFlags & CG_SHOW_2DONLY );
		break;
	case CG_PLAYER_SCORE:
		CG_DrawPlayerScore( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_PLAYER_HEALTH:
		CG_DrawPlayerHealth( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_RED_SCORE:
		CG_DrawRedScore( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_BLUE_SCORE:
		CG_DrawBlueScore( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_RED_NAME:
		CG_DrawRedName( &rect, font, scale, color, textStyle );
		break;
	case CG_BLUE_NAME:
		CG_DrawBlueName( &rect, font, scale, color, textStyle );
		break;
	case CG_BLUE_FLAGHEAD:
		CG_DrawBlueFlagHead( &rect );
		break;
	case CG_BLUE_FLAGSTATUS:
		CG_DrawBlueFlagStatus( &rect, shader );
		break;
	case CG_BLUE_FLAGNAME:
		CG_DrawBlueFlagName( &rect, font, scale, color, textStyle );
		break;
	case CG_RED_FLAGHEAD:
		CG_DrawRedFlagHead( &rect );
		break;
	case CG_RED_FLAGSTATUS:
		CG_DrawRedFlagStatus( &rect, shader );
		break;
	case CG_RED_FLAGNAME:
		CG_DrawRedFlagName( &rect, font, scale, color, textStyle );
		break;
	case CG_HARVESTER_SKULLS:
		CG_HarvesterSkulls( &rect, font, scale, color, qfalse, textStyle );
		break;
	case CG_HARVESTER_SKULLS2D:
		CG_HarvesterSkulls( &rect, font, scale, color, qtrue, textStyle );
		break;
	case CG_ONEFLAG_STATUS:
		CG_OneFlagStatus( &rect );
		break;
	case CG_PLAYER_LOCATION:
		CG_DrawPlayerLocation( &rect, font, scale, color, textStyle );
		break;
	case CG_TEAM_COLOR:
		CG_DrawTeamColor( &rect, color );
		break;
	case CG_CTF_POWERUP:
		CG_DrawCTFPowerUp( &rect );
		break;
	case CG_AREA_WEAPON:
		CG_DrawAreaWeapons( &rect, align, special, font, scale, color );
		break;
	case CG_AREA_HOLDABLE:
		CG_DrawAreaHoldable( &rect, align, special, font, scale, color );
		break;
	case CG_AREA_POWERUP:
		CG_DrawAreaPowerUp( &rect, align, special, font, scale, color );
		break;
	case CG_PLAYER_STATUS:
		CG_DrawPlayerStatus( &rect );
		break;
	case CG_PLAYER_HASFLAG:
		CG_DrawPlayerHasFlag( &rect, qfalse );
		break;
	case CG_PLAYER_HASFLAG2D:
		CG_DrawPlayerHasFlag( &rect, qtrue );
		break;
	case CG_AREA_SYSTEMCHAT:
		CG_DrawAreaSystemChat( &rect, font, scale, color, shader );
		break;
	case CG_AREA_TEAMCHAT:
		CG_DrawAreaTeamChat( &rect, font, scale, color, shader );
		break;
	case CG_AREA_CHAT:
		CG_DrawAreaChat( &rect, font, scale, color, shader );
		break;
	case CG_GAME_TYPE:
		CG_DrawGameType( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_GAME_STATUS:
		CG_DrawGameStatus( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_KILLER:
		CG_DrawKiller( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_ACCURACY:
	case CG_ASSISTS:
	case CG_DEFEND:
	case CG_EXCELLENT:
	case CG_IMPRESSIVE:
	case CG_PERFECT:
	case CG_GAUNTLET:
	case CG_CAPTURES:
		CG_DrawMedal( ownerDraw, &rect, font, scale, color, shader );
		break;
	case CG_SPECTATORS:
		CG_DrawTeamSpectators( &rect, font, scale, color, shader );
		break;
	case CG_TEAMINFO:
		if ( cg_currentSelectedPlayer.integer == numSortedTeamPlayers ) {
			CG_DrawNewTeamInfo( &rect, text_x, text_y, font, scale, color, shader );
		}
		break;
	case CG_CAPFRAGLIMIT:
		CG_DrawCapFragLimit( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_1STPLACE:
		CG_Draw1stPlace( &rect, font, scale, color, shader, textStyle );
		break;
	case CG_2NDPLACE:
		CG_Draw2ndPlace( &rect, font, scale, color, shader, textStyle );
		break;
	default:
		break;
	}
}

void CG_MouseEvent( int x, int y ) {
	int n;

	if ( ( cg.predictedPlayerState.pm_type == PM_NORMAL || cg.predictedPlayerState.pm_type == PM_SPECTATOR ) && cg.showScores == qfalse ) {
		trap_Key_SetCatcher( 0 );
		return;
	}

	cgs.cursorX += x;
	if ( cgs.cursorX < 0 ) {
		cgs.cursorX = 0;
	} else if ( cgs.cursorX > 640 ) {
		cgs.cursorX = 640;
	}

	cgs.cursorY += y;
	if ( cgs.cursorY < 0 ) {
		cgs.cursorY = 0;
	} else if ( cgs.cursorY > 480 ) {
		cgs.cursorY = 480;
	}

	n = Display_CursorType( cgs.cursorX, cgs.cursorY );
	cgs.activeCursor = 0;
	if ( n == CURSOR_ARROW ) {
		cgs.activeCursor = cgs.media.selectCursor;
	} else if ( n == CURSOR_SIZER ) {
		cgs.activeCursor = cgs.media.sizeCursor;
	}

	if ( cgs.capturedItem ) {
		Display_MouseMove( cgs.capturedItem, x, y );
	} else {
		Display_MouseMove( NULL, cgs.cursorX, cgs.cursorY );
	}

}

/*
==================
CG_HideTeamMenus
==================

*/
void CG_HideTeamMenu() {
	Menus_CloseByName( "teamMenu" );
	Menus_CloseByName( "getMenu" );
}

/*
==================
CG_ShowTeamMenus
==================

*/
void CG_ShowTeamMenu() {
	Menus_OpenByName( "teamMenu" );
}




/*
==================
CG_EventHandling
==================
 type 0 - no event handling
	  1 - team menu
	  2 - hud editor

*/
void CG_EventHandling( int type ) {
	cgs.eventHandling = type;
	if ( type == CGAME_EVENT_NONE ) {
		CG_HideTeamMenu();
	} else if ( type == CGAME_EVENT_TEAMMENU ) {
		//CG_ShowTeamMenu();
	} else if ( type == CGAME_EVENT_SCOREBOARD ) {
	}

}



void CG_KeyEvent( int key, qboolean down ) {

	if ( !down ) {
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_NORMAL || ( cg.predictedPlayerState.pm_type == PM_SPECTATOR && cg.showScores == qfalse ) ) {
		CG_EventHandling( CGAME_EVENT_NONE );
		trap_Key_SetCatcher( 0 );
		return;
	}

	//if (key == trap_Key_GetKey("teamMenu") || !Display_CaptureItem(cgs.cursorX, cgs.cursorY)) {
	// if we see this then we should always be visible
	//  CG_EventHandling(CGAME_EVENT_NONE);
	//  trap_Key_SetCatcher(0);
	//}



	Display_HandleKey( key, down, cgs.cursorX, cgs.cursorY );

	if ( cgs.capturedItem ) {
		cgs.capturedItem = NULL;
	}   else {
		if ( key == K_MOUSE2 && down ) {
			cgs.capturedItem = Display_CaptureItem( cgs.cursorX, cgs.cursorY );
		}
	}
}

int CG_ClientNumFromName( const char *p ) {
	int i;
	for ( i = 0; i < cgs.maxclients; i++ ) {
		if ( cgs.clientinfo[i].infoValid && Q_stricmp( cgs.clientinfo[i].name, p ) == 0 ) {
			return i;
		}
	}
	return -1;
}

void CG_ShowResponseHead() {
	Menus_OpenByName( "voiceMenu" );
	trap_Cvar_Set( "cl_conXOffset", "72" );
	cg.voiceTime = cg.time;
}

void CG_RunMenuScript( char **args ) {
}


void CG_GetTeamColor( vec4_t *color ) {
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		( *color )[0] = 1;
		( *color )[3] = .25f;
		( *color )[1] = ( *color )[2] = 0;
	} else if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_BLUE ) {
		( *color )[0] = ( *color )[1] = 0;
		( *color )[2] = 1;
		( *color )[3] = .25f;
	} else {
		( *color )[0] = ( *color )[2] = 0;
		( *color )[1] = .17f;
		( *color )[3] = .25f;
	}
}

