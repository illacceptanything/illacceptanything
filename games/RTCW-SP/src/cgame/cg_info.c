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

// cg_info.c -- display information while data is being loading

#include "cg_local.h"
#include "../ui/ui_shared.h"

#define MAX_LOADING_PLAYER_ICONS    16
#define MAX_LOADING_ITEM_ICONS      26

static int loadingPlayerIconCount;
static int loadingItemIconCount;
static qhandle_t loadingPlayerIcons[MAX_LOADING_PLAYER_ICONS];
static qhandle_t loadingItemIcons[MAX_LOADING_ITEM_ICONS];


/*
===================
CG_DrawLoadingIcons
===================
*/
static void CG_DrawLoadingIcons( void ) {
	int n;
	int x, y;

	// JOSEPH 5-2-00 Per MAXX
	return;

	for ( n = 0; n < loadingPlayerIconCount; n++ ) {
		x = 16 + n * 78;
		y = 324;
		CG_DrawPic( x, y, 64, 64, loadingPlayerIcons[n] );
	}

	for ( n = 0; n < loadingItemIconCount; n++ ) {
		y = 400;
		if ( n >= 13 ) {
			y += 40;
		}
		x = 16 + n % 13 * 48;
		CG_DrawPic( x, y, 32, 32, loadingItemIcons[n] );
	}
}


/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString( const char *s ) {
	Q_strncpyz( cg.infoScreenText, s, sizeof( cg.infoScreenText ) );

	if ( s && s[0] != 0 ) {
		CG_Printf( va( "LOADING... %s\n",s ) );   //----(SA)	added so you can see from the console what's going on

	}
	trap_UpdateScreen();
}

/*
===================
CG_LoadingItem
===================
*/
void CG_LoadingItem( int itemNum ) {
	gitem_t     *item;

	item = &bg_itemlist[itemNum];

	if ( item->giType == IT_KEY ) { // do not show keys at level startup //----(SA)
		return;
	}

//----(SA)	Max Kaufman request that we don't show any pacifier stuff for items
	return;
//----(SA)	end


	if ( item->icon && loadingItemIconCount < MAX_LOADING_ITEM_ICONS ) {
		loadingItemIcons[loadingItemIconCount++] = trap_R_RegisterShaderNoMip( item->icon );
	}

	CG_LoadingString( cgs.itemPrintNames[item - bg_itemlist] );
}

/*
===================
CG_LoadingClient
===================
*/
void CG_LoadingClient( int clientNum ) {
	const char      *info;
	char            *skin;
	char personality[MAX_QPATH];
	char model[MAX_QPATH];
	char iconName[MAX_QPATH];

	if ( cgs.gametype == GT_SINGLE_PLAYER  && clientNum > 0 ) { // for now only show the player's icon in SP games
		return;
	}

	info = CG_ConfigString( CS_PLAYERS + clientNum );

	Q_strncpyz( model, Info_ValueForKey( info, "model" ), sizeof( model ) );
	skin = Q_strrchr( model, '/' );
	if ( skin ) {
		*skin++ = '\0';
	} else {
		skin = "default";
	}

	Com_sprintf( iconName, MAX_QPATH, "models/players/%s/icon_%s.tga", model, skin );

// (SA) ignore player icons for the moment
	if ( !( cg_entities[clientNum].currentState.aiChar ) ) {
//		if ( loadingPlayerIconCount < MAX_LOADING_PLAYER_ICONS ) {
//			loadingPlayerIcons[loadingPlayerIconCount++] = trap_R_RegisterShaderNoMip( iconName );
//		}
	}

	Q_strncpyz( personality, Info_ValueForKey( info, "n" ), sizeof( personality ) );
	Q_CleanStr( personality );

	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		trap_S_RegisterSound( va( "sound/player/announce/%s.wav", personality ) );
	}

	CG_LoadingString( personality );
}

/*
====================
CG_DrawStats
====================
*/

typedef struct {
	char    *label;
	int YOfs;
	int labelX;
	int labelFlags;
	vec4_t labelColor;

	char    *format;
	int formatX;
	int formatFlags;
	vec4_t formatColor;

	int numVars;
} statsItem_t;

// this defines the layout of the mission stats
// NOTE: these must match the stats sent in AICast_ScriptAction_ChangeLevel()
static statsItem_t statsItems[] = {
	{ "end_time",        168,    214,        ITEM_TEXTSTYLE_SHADOWEDMORE,    {1.0f,  1.0f,   1.0f,   1.0f},  "%02i:%02i:%02i",    348,    ITEM_TEXTSTYLE_SHADOWEDMORE,    {1.0f,  1.0f,   1.0f,   1.0f},  3 },
	{ "end_objectives",  28,     214,        ITEM_TEXTSTYLE_SHADOWEDMORE,    {1.0f,  1.0f,   1.0f,   1.0f},  "%i/%i",         348,    ITEM_TEXTSTYLE_SHADOWEDMORE,    {1.0f,  1.0f,   1.0f,   1.0f},  2 },
	{ "end_secrets", 28,     214,        ITEM_TEXTSTYLE_SHADOWEDMORE,    {1.0f,  1.0f,   1.0f,   1.0f},  "%i/%i",         348,    ITEM_TEXTSTYLE_SHADOWEDMORE,    {1.0f,  1.0f,   1.0f,   1.0f},  2 },
	{ "end_treasure",    28,     214,        ITEM_TEXTSTYLE_SHADOWEDMORE,    {0.62f, 0.56f,  0.0f,   1.0f},  "%i/%i",         348,    ITEM_TEXTSTYLE_SHADOWEDMORE,    {1.0f,  1.0f,   1.0f,   1.0f},  2 },
	{ "end_attempts",    28,     214,        ITEM_TEXTSTYLE_SHADOWEDMORE,    {1.0f,  1.0f,   1.0f,   1.0f},  "%i",                348,    ITEM_TEXTSTYLE_SHADOWEDMORE,    {1.0f,  1.0f,   1.0f,   1.0f},  1 },

	{ NULL }
};

//	int			numSecrets;
//	int			numTreasure;
//	int			numTreasureFound;
//	int			numArtifacts;
//	int			numObjectives;



/*
==============
CG_DrawStats
==============
*/
void CG_DrawStats( char *stats ) {
	int i, y, v, j;
	#define MAX_STATS_VARS  64
	int vars[MAX_STATS_VARS];
	char *str, *token;
	char *formatStr;
	int varIndex;
	char string[MAX_QPATH];

	UI_DrawProportionalString( 320, 120, "MISSION STATS",
							   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );

	Q_strncpyz( string, stats, sizeof( string ) );
	str = string;
	// convert commas to spaces
	for ( i = 0; str[i]; i++ ) {
		if ( str[i] == ',' ) {
			str[i] = ' ';
		}
	}

	for ( i = 0, y = 0, v = 0; statsItems[i].label; i++ ) {
		y += statsItems[i].YOfs;

//		UI_DrawProportionalString( statsItems[i].labelX, y, statsItems[i].label,
//			statsItems[i].labelFlags, *statsItems[i].labelColor );

		if ( statsItems[i].numVars ) {
			varIndex = v;
			for ( j = 0; j < statsItems[i].numVars; j++ ) {
				token = COM_Parse( &str );
				if ( !token || !token[0] ) {
					CG_Error( "error parsing mission stats\n" );
					return;
				}

				vars[v++] = atoi( token );
			}

			// build the formatStr
			switch ( statsItems[i].numVars ) {
			case 1:
				formatStr = va( statsItems[i].format, vars[varIndex] );
				break;
			case 2:
				formatStr = va( statsItems[i].format, vars[varIndex], vars[varIndex + 1] );
				break;
			case 3:
				formatStr = va( statsItems[i].format, vars[varIndex], vars[varIndex + 1], vars[varIndex + 2] );
				break;
			case 4:
				formatStr = va( statsItems[i].format, vars[varIndex], vars[varIndex + 1], vars[varIndex + 2], vars[varIndex + 3] );
				break;
			}

//			UI_DrawProportionalString( statsItems[i].formatX, y, formatStr,
//				statsItems[i].formatFlags, *statsItems[i].formatColor );
		}
	}
}

/*
==============
CG_DrawExitStats
	pretty much what the game should draw when you're at the exit
	This is not the final deal, but represents the kind of thing
	that will be there
==============
*/

void CG_DrawExitStats( void ) {
	int i, y, v, j;
	float *color;   // faded color based on cursor hint drawing
	float color2[4] = {0, 0, 0, 1};
	const char *str;
	char *mstats, *token;

	#define MAX_STATS_VARS  64
	int vars[MAX_STATS_VARS];
	char *formatStr = NULL; // TTimo: init
	int varIndex = 0;     // TTimo: init

	if ( cg_paused.integer ) {
		// no draw if any menu's are up	 (or otherwise paused)
		return;
	}

	color = CG_FadeColor( cg.cursorHintTime, cg.cursorHintFade );

	if ( !color ) { // currently faded out, don't draw
		return;
	}

	// check for fade up
	if ( cg.time < ( cg.exitStatsTime + cg.exitStatsFade ) ) {
		color[3] = (float)( cg.time - cg.exitStatsTime ) / (float)cg.exitStatsFade;
	}

	color2[3] = color[3];


// parse it
	str = CG_ConfigString( CS_MISSIONSTATS );

	if ( !str || !str[0] ) {
		return;
	}

	// background
	color2[3] *= 0.6f;
	CG_FilledBar( 150, 104, 340, 230, color2, NULL, NULL, 1.0f, 0 );

	color2[0] = color2[1] = color2[2] = 0.3f;
	color2[3] *= 0.6f;

	// border
	CG_FilledBar( 148, 104, 2, 230, color2, NULL, NULL, 1.0f, 0 );    // left
	CG_FilledBar( 490, 104, 2, 230, color2, NULL, NULL, 1.0f, 0 );    // right
	CG_FilledBar( 148, 102, 344, 2, color2, NULL, NULL, 1.0f, 0 );    // top
	CG_FilledBar( 148, 334, 344, 2, color2, NULL, NULL, 1.0f, 0 );    // bot


	// text boxes
	color2[0] = color2[1] = color2[2] = 0.4f;
	for ( i = 0; i < 5; i++ ) {
		CG_FilledBar( 170, 154 + ( 28 * i ), 300, 20, color2, NULL, NULL, 1.0f, 0 );
	}


	// green title
	color2[0] = color2[2] = 0;
	color2[1] = 0.3f;
	CG_FilledBar( 150, 104, 340, 20, color2, NULL, NULL, 1.0f, 0 );

	color2[0] = color2[1] = color2[2] = 0.2f;

	// title
	color2[0] = color2[1] = color2[2] = 1;
	color2[3] = color[3];
//	CG_Text_Paint(280, 120, 2, 0.25f, color2, va("%s", CG_translateString("end_title")), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
	//----(SA)	scale change per MK
	CG_Text_Paint( 270, 120, 2, 0.313f, color2, va( "%s", CG_translateString( "end_title" ) ), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );

	color2[0] = color2[1] = color2[2] = 1;
	if ( cg.cursorHintIcon == HINT_NOEXIT ) {
		// "exit not available"
//		CG_Text_Paint(250, 320, 2, 0.3f, color2, va("%s", CG_translateString("end_noexit")), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		//----(SA)	scale change per MK
		CG_Text_Paint( 260, 320, 2, 0.225f, color2, va( "%s", CG_translateString( "end_noexit" ) ), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
	} else {
		// "forward to proceed"
//		CG_Text_Paint(230, 320, 2, 0.3f, color2, va("%s", CG_translateString("end_exit")), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE);
		//----(SA)	scale change per MK
		CG_Text_Paint( 250, 320, 2, 0.225f, color2, va( "%s", CG_translateString( "end_exit" ) ), 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
	}

	mstats = (char*)str + 2;    // add offset for 's='
	for ( i = 0; mstats[i]; i++ ) {
		if ( mstats[i] == ',' ) {
			mstats[i] = ' ';
		}
	}
	for ( i = 0, y = 0, v = 0; statsItems[i].label; i++ ) {
		y += statsItems[i].YOfs;

		VectorCopy4( statsItems[i].labelColor, color2 );
//		statsItems[i].labelColor[3] = statsItems[i].formatColor[3] = color[3];	// set proper alpha
		color2[3] = statsItems[i].formatColor[3] = color[3];    // set proper alpha


		if ( statsItems[i].numVars ) {
			varIndex = v;
			for ( j = 0; j < statsItems[i].numVars; j++ ) {
				token = COM_Parse( &mstats );
				if ( !token || !token[0] ) {
					CG_Error( "error parsing mission stats\n" );
					return;
				}

				vars[v++] = atoi( token );
			}

			// build the formatStr
			switch ( statsItems[i].numVars ) {
			case 1:
				formatStr = va( statsItems[i].format, vars[varIndex] );
				break;
			case 2:
				formatStr = va( statsItems[i].format, vars[varIndex], vars[varIndex + 1] );
				break;
			case 3:
				formatStr = va( statsItems[i].format, vars[varIndex], vars[varIndex + 1], vars[varIndex + 2] );
				break;
			case 4:
				formatStr = va( statsItems[i].format, vars[varIndex], vars[varIndex + 1], vars[varIndex + 2], vars[varIndex + 3] );
				break;
			}

			CG_Text_Paint( statsItems[i].formatX, y, 2, 0.3, statsItems[i].formatColor, formatStr, 0, 0, statsItems[i].formatFlags );
//			UI_DrawProportionalString( statsItems[i].formatX, y, formatStr, statsItems[i].formatFlags, *statsItems[i].formatColor );
		}

		if ( i == 1 ) {  // 'objectives'
			if ( vars[varIndex] < vars[varIndex + 1] ) { // missing objectives, draw in red
				color2[0] = 1;
				color2[1] = color2[2] = 0;
			}
		}

		if ( i == 3 ) {  // 'treasure'
			if ( vars[varIndex] < vars[varIndex + 1] || !vars[varIndex + 1] ) {    // missing treasure, only draw in white (gold when you got em all)  (unless there's no gold available, then 0/0 shows white)
				color2[0] = color2[1] = color2[2] = 1;  // white
			}
		}

//		UI_DrawProportionalString( statsItems[i].labelX, y, statsItems[i].label, statsItems[i].labelFlags, *statsItems[i].labelColor );

//		CG_Text_Paint(statsItems[i].labelX, y, 2, 0.3, statsItems[i].labelColor, va("%s:", CG_translateString(statsItems[i].label)), 0, 0, statsItems[i].labelFlags);
		CG_Text_Paint( statsItems[i].labelX, y, 2, 0.3, color2, va( "%s:", CG_translateString( statsItems[i].label ) ), 0, 0, statsItems[i].labelFlags );

	}
	token = COM_Parse( &mstats );

// end (parse it)
}


/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation( void ) {
	const char  *s;
	const char  *info;
	const char  *sysInfo;
	int y;
	int value;
	qhandle_t levelshot = 0;   // TTimo: init
//	qhandle_t	detail;
	char buf[1024];
	static int lastDraw = 0;  // Ridah, so we don't draw the screen more often than we need to
	int ms;
	static int callCount = 0;
	float percentDone;

	int expectedHunk;
	char hunkBuf[MAX_QPATH];

	vec4_t color;

	if ( cg.snap && ( strlen( cg_missionStats.string ) <= 1 ) ) {
		return;     // we are in the world, no need to draw information
	}

	if ( callCount ) {    // reject recursive calls
		return;
	}

	ms = trap_Milliseconds();
	if ( ( lastDraw <= ms ) && ( lastDraw > ms - 100 ) ) {
		return;
	}
	lastDraw = ms;

	callCount++;

	info = CG_ConfigString( CS_SERVERINFO );
	sysInfo = CG_ConfigString( CS_SYSTEMINFO );

	trap_Cvar_VariableStringBuffer( "com_expectedhunkusage", hunkBuf, MAX_QPATH );
	expectedHunk = atoi( hunkBuf );


	s = Info_ValueForKey( info, "mapname" );

	//----(SA)	just the briefing now

	if ( s && s[0] != 0 ) {  // there is often no 's'
		levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", s ) );
	}

	if ( !levelshot ) {
		levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
	}

	trap_R_SetColor( NULL );
//	CG_DrawPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, levelshot );

	// blend a detail texture over it
	//detail = trap_R_RegisterShader( "levelShotDetail" );
	//trap_R_DrawStretchPic( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0, 0, 2.5, 2, detail );


// (SA) commented out for Drew
//	UI_DrawProportionalString( 320, 16, va( "Loading %s", Info_ValueForKey( info, "mapname" ) ), UI_SMALLFONT|UI_CENTER|UI_DROPSHADOW, colorWhite );

	// show the loading progress
	VectorSet( color, 0.8, 0.8, 0.8 );
	color[3] = 0.8;

	if ( strlen( cg_missionStats.string ) > 1 && cg_missionStats.string[0] == 's' ) {
		vec2_t xy = { 190, 470 };
		vec2_t wh = { 260, 10 };

		// draw the mission stats while loading

		if ( expectedHunk > 0 ) {
			percentDone = (float)( cg_hunkUsed.integer + cg_soundAdjust.integer ) / (float)( expectedHunk );
			if ( percentDone > 0.97 ) { // never actually show 100%, since we are not in the game yet
				percentDone = 0.97;
			}
			CG_HorizontalPercentBar( xy[0] + 10, xy[1] + wh[1] - 10, wh[0] - 20, 10, percentDone );
		} else {
			UI_DrawProportionalString( 320, xy[1] + wh[1] - 10, "please wait",
									   UI_CENTER | UI_EXSMALLFONT | UI_DROPSHADOW, color );
		}

		trap_UpdateScreen();
		callCount--;
		return;
	}

	// Ridah, in single player, cheats disabled, don't show unnecessary information
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {

		if ( 0 ) { // bar drawn in menu now
			vec2_t xy = { 200, 468 };
			vec2_t wh = { 240, 10 };

			// show the percent complete bar
			if ( expectedHunk > 0 ) {
				percentDone = (float)( cg_hunkUsed.integer + cg_soundAdjust.integer ) / (float)( expectedHunk );
				if ( percentDone > 0.97 ) {
					percentDone = 0.97;
				}

				CG_HorizontalPercentBar( xy[0], xy[1], wh[0], wh[1], percentDone );
			}
		}

		trap_UI_Popup( "briefing" );

		trap_UpdateScreen();
		callCount--;
		return;
	}
	// done.


	// draw the icons of thiings as they are loaded
	CG_DrawLoadingIcons();

	// the first 150 rows are reserved for the client connection
	// screen to write into
	if ( cg.infoScreenText[0] ) {
		UI_DrawProportionalString( 320, 128, va( "Loading... %s", cg.infoScreenText ),
								   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
	} else {
		UI_DrawProportionalString( 320, 128, "Awaiting snapshot...",
								   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
	}

	// draw info string information

	y = 180;

	// don't print server lines if playing a local game
	trap_Cvar_VariableStringBuffer( "sv_running", buf, sizeof( buf ) );
	if ( !atoi( buf ) ) {
		// server hostname
		s = Info_ValueForKey( info, "sv_hostname" );
		UI_DrawProportionalString( 320, y, s,
								   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT;

		// server-specific message of the day
		s = CG_ConfigString( CS_MOTD );
		if ( s[0] ) {
			UI_DrawProportionalString( 320, y, s,
									   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
			y += PROP_HEIGHT;
		}

		// some extra space after hostname and motd
		y += 10;
	}

	// map-specific message (long map name)
	s = CG_ConfigString( CS_MESSAGE );
	if ( s[0] ) {
		UI_DrawProportionalString( 320, y, s,
								   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT;
	}

	// cheats warning
	s = Info_ValueForKey( sysInfo, "sv_cheats" );
	if ( s[0] == '1' ) {
		UI_DrawProportionalString( 320, y, "CHEATS ARE ENABLED",
								   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT;
	}

	// game type
	switch ( cgs.gametype ) {
	case GT_FFA:
		s = "Free For All";
		break;
	case GT_SINGLE_PLAYER:
		s = "Single Player";
		break;
	case GT_TOURNAMENT:
		s = "Tournament";
		break;
	case GT_TEAM:
		s = "Team Deathmatch";
		break;
	case GT_CTF:
		s = "Capture The Flag";
		break;
// JPW NERVE
	case GT_WOLF:
		s = "Wolfenstein Multiplayer";
		break;
// jpw
	default:
		s = "Unknown Gametype";
		break;
	}
	UI_DrawProportionalString( 320, y, s,
							   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
	y += PROP_HEIGHT;

	value = atoi( Info_ValueForKey( info, "timelimit" ) );
	if ( value ) {
		UI_DrawProportionalString( 320, y, va( "timelimit %i", value ),
								   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
		y += PROP_HEIGHT;
	}

	if ( cgs.gametype != GT_CTF && cgs.gametype != GT_SINGLE_PLAYER ) {
		value = atoi( Info_ValueForKey( info, "fraglimit" ) );
		if ( value ) {
			UI_DrawProportionalString( 320, y, va( "fraglimit %i", value ),
									   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
			y += PROP_HEIGHT;
		}
	}

	if ( cgs.gametype == GT_CTF ) {
		value = atoi( Info_ValueForKey( info, "capturelimit" ) );
		if ( value ) {
			UI_DrawProportionalString( 320, y, va( "capturelimit %i", value ),
									   UI_CENTER | UI_SMALLFONT | UI_DROPSHADOW, colorWhite );
			y += PROP_HEIGHT;
		}
	}

	callCount--;
}
