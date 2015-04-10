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
=======================================================================

  USER INTERFACE MAIN

=======================================================================
*/

#include "ui_local.h"

uiInfo_t uiInfo;

static const char *MonthAbbrev[] = {
	"Jan","Feb","Mar",
	"Apr","May","Jun",
	"Jul","Aug","Sep",
	"Oct","Nov","Dec"
};


static const char *skillLevels[] = {
	"I Can Win",
	"Bring It On",
	"Hurt Me Plenty",
	"Hardcore",
	"Nightmare"
};

static const int numSkillLevels = sizeof( skillLevels ) / sizeof( const char* );


static const char *netSources[] = {
	"Local",
	"Mplayer",
	"Internet",
	"Favorites"
};
static const int numNetSources = sizeof( netSources ) / sizeof( const char* );

static const serverFilter_t serverFilters[] = {
	{"All", "" },
	{"Quake 3 Arena", "" },
	{"Team Arena", "missionpack" },
	{"Rocket Arena", "arena" },
	{"Alliance", "alliance" },
};

static const char *teamArenaGameTypes[] = {
	"FFA",
	"TOURNAMENT",
	"SP",
	"TEAM DM",
	"CTF",
	"1FCTF",
	"OVERLOAD",
	"HARVESTER",
	"TEAMTOURNAMENT"
};

static int const numTeamArenaGameTypes = sizeof( teamArenaGameTypes ) / sizeof( const char* );


static const char *teamArenaGameNames[] = {
	"Free For All",
	"Tournament",
	"Single Player",
	"Team Deathmatch",
	"Capture the Flag",
	"One Flag CTF",
	"Overload",
	"Harvester",
	"Team Tournament",
};

static int const numTeamArenaGameNames = sizeof( teamArenaGameNames ) / sizeof( const char* );


static const int numServerFilters = sizeof( serverFilters ) / sizeof( serverFilter_t );

static const char *sortKeys[] = {
	"Server Name",
	"Map Name",
	"Open Player Spots",
	"Game Type",
	"Ping Time"
};
static const int numSortKeys = sizeof( sortKeys ) / sizeof( const char* );

static char* netnames[] = {
	"???",
	"UDP",
	"IPX",
	NULL
};

// TTimo: unused
//static char quake3worldMessage[] = "Visit www.quake3world.com - News, Community, Events, Files";

static int gamecodetoui[] = {4,2,3,0,5,1,6};
static int uitogamecode[] = {4,6,2,3,1,5,7};


// NERVE - SMF - enabled for multiplayer
static void UI_StartServerRefresh( qboolean full );
static void UI_StopServerRefresh( void );
static void UI_DoServerRefresh( void );
static void UI_FeederSelection( float feederID, int index );
static void UI_BuildServerDisplayList( qboolean force );
static void UI_BuildServerStatus( qboolean force );
static void UI_BuildFindPlayerList( qboolean force );
static int QDECL UI_ServersQsortCompare( const void *arg1, const void *arg2 );
static int UI_MapCountByGameType( qboolean singlePlayer );
static const char *UI_SelectedMap( int index, int *actual );
static int UI_GetIndexFromSelection( int actual );
// -NERVE - SMF - enabled for multiplayer

static void UI_ParseGameInfo( const char *teamFile );
//static void UI_ParseTeamInfo(const char *teamFile);

//int ProcessNewUI( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6 );

static uiMenuCommand_t menutype = UIMENU_NONE;

// externs
extern displayContextDef_t *DC;


//----(SA)	added for savegame sorting
static int QDECL UI_SavegamesQsortCompare( const void *arg1, const void *arg2 );
//----(SA)	end

void Text_PaintCenter( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust );


/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .qvm file
================
*/
vmCvar_t ui_new;
vmCvar_t ui_debug;
vmCvar_t ui_initialized;
vmCvar_t ui_WolfFirstRun;

void _UI_Init( qboolean );
void _UI_Shutdown( void );
void _UI_KeyEvent( int key, qboolean down );
void _UI_MouseEvent( int dx, int dy );
void _UI_Refresh( int realtime );
qboolean _UI_IsFullscreen( void );
#if defined( __MACOS__ )
#pragma export on
#endif
int vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
#if defined( __MACOS__ )
#pragma export off
#endif
	switch ( command ) {
	case UI_GETAPIVERSION:
		return UI_API_VERSION;

	case UI_INIT:
		_UI_Init( arg0 );
		return 0;

	case UI_SHUTDOWN:
		_UI_Shutdown();
		return 0;

	case UI_KEY_EVENT:
		_UI_KeyEvent( arg0, arg1 );
		return 0;

	case UI_MOUSE_EVENT:
		_UI_MouseEvent( arg0, arg1 );
		return 0;

	case UI_REFRESH:
		_UI_Refresh( arg0 );
		return 0;

	case UI_IS_FULLSCREEN:
		return _UI_IsFullscreen();

	case UI_SET_ACTIVE_MENU:
		_UI_SetActiveMenu( arg0 );
		return 0;

	case UI_GET_ACTIVE_MENU:
		return _UI_GetActiveMenu();

	case UI_CONSOLE_COMMAND:
		return UI_ConsoleCommand( arg0 );

	case UI_DRAW_CONNECT_SCREEN:
		UI_DrawConnectScreen( arg0 );
		return 0;
	case UI_HASUNIQUECDKEY:             // mod authors need to observe this
		return qtrue;

	}

	return -1;
}



void AssetCache() {
	int n;
	//if (Assets.textFont == NULL) {
	//}
	//Assets.background = trap_R_RegisterShaderNoMip( ASSET_BACKGROUND );
	//Com_Printf("Menu Size: %i bytes\n", sizeof(Menus));
	uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( ASSET_GRADIENTBAR );
	uiInfo.uiDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip( ART_FX_BASE );
	uiInfo.uiDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip( ART_FX_RED );
	uiInfo.uiDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip( ART_FX_YELLOW );
	uiInfo.uiDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip( ART_FX_GREEN );
	uiInfo.uiDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip( ART_FX_TEAL );
	uiInfo.uiDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip( ART_FX_BLUE );
	uiInfo.uiDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip( ART_FX_CYAN );
	uiInfo.uiDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip( ART_FX_WHITE );
	uiInfo.uiDC.Assets.scrollBar = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR );
	uiInfo.uiDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWDOWN );
	uiInfo.uiDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWUP );
	uiInfo.uiDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWLEFT );
	uiInfo.uiDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip( ASSET_SCROLLBAR_ARROWRIGHT );
	uiInfo.uiDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip( ASSET_SCROLL_THUMB );
	uiInfo.uiDC.Assets.sliderBar = trap_R_RegisterShaderNoMip( ASSET_SLIDER_BAR );
	uiInfo.uiDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip( ASSET_SLIDER_THUMB );

	for ( n = 0; n < NUM_CROSSHAIRS; n++ ) {
		uiInfo.uiDC.Assets.crosshairShader[n] = trap_R_RegisterShaderNoMip( va( "gfx/2d/crosshair%c", 'a' + n ) );
	}

//	uiInfo.newHighScoreSound = trap_S_RegisterSound("sound/feedback/voc_newhighscore.wav");
}

void _UI_DrawSides( float x, float y, float w, float h, float size ) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic( x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}

void _UI_DrawTopBottom( float x, float y, float w, float h, float size ) {
	UI_AdjustFrom640( &x, &y, &w, &h );
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic( x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
	trap_R_DrawStretchPic( x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader );
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect( float x, float y, float width, float height, float size, const float *color ) {
	trap_R_SetColor( color );

	_UI_DrawTopBottom( x, y, width, height, size );
	_UI_DrawSides( x, y, width, height, size );

	trap_R_SetColor( NULL );
}




int Text_Width( const char *text, int font, float scale, int limit ) {
	int count,len;
	float out;
	glyphInfo_t *glyph;
	float useScale;
	const char *s = text;

	fontInfo_t *fnt = &uiInfo.uiDC.Assets.textFont;
	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= ui_smallFont.value ) {
			fnt = &uiInfo.uiDC.Assets.smallFont;
		} else if ( scale > ui_bigFont.value ) {
			fnt = &uiInfo.uiDC.Assets.bigFont;
		}
	} else if ( font == UI_FONT_BIG ) {
		fnt = &uiInfo.uiDC.Assets.bigFont;
	} else if ( font == UI_FONT_SMALL ) {
		fnt = &uiInfo.uiDC.Assets.smallFont;
	} else if ( font == UI_FONT_HANDWRITING ) {
		fnt = &uiInfo.uiDC.Assets.handwritingFont;
	}

	useScale = scale * fnt->glyphScale;
	out = 0;
	if ( text ) {
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = &fnt->glyphs[(unsigned char)*s];
				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}
	return out * useScale;
}

int Text_Height( const char *text, int font, float scale, int limit ) {
	int len, count;
	float max;
	glyphInfo_t *glyph;
	float useScale;
	const unsigned char *s = text;

	fontInfo_t *fnt = &uiInfo.uiDC.Assets.textFont;
	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= ui_smallFont.value ) {
			fnt = &uiInfo.uiDC.Assets.smallFont;
		} else if ( scale > ui_bigFont.value ) {
			fnt = &uiInfo.uiDC.Assets.bigFont;
		}
	} else if ( font == UI_FONT_BIG ) {
		fnt = &uiInfo.uiDC.Assets.bigFont;
	} else if ( font == UI_FONT_SMALL ) {
		fnt = &uiInfo.uiDC.Assets.smallFont;
	} else if ( font == UI_FONT_HANDWRITING ) {
		fnt = &uiInfo.uiDC.Assets.handwritingFont;
	}

	useScale = scale * fnt->glyphScale;
	max = 0;
	if ( text ) {
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			if ( Q_IsColorString( s ) ) {
				s += 2;
				continue;
			} else {
				glyph = &fnt->glyphs[*s];
				if ( max < glyph->height ) {
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

void Text_PaintChar( float x, float y, float width, float height, int font, float scale, float s, float t, float s2, float t2, qhandle_t hShader ) {
	float w, h;
	w = width * scale;
	h = height * scale;
	UI_AdjustFrom640( &x, &y, &w, &h );
	trap_R_DrawStretchPic( x, y, w, h, s, t, s2, t2, hShader );
}

void Text_Paint( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust, int limit, int style ) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	float useScale;

	fontInfo_t *fnt = &uiInfo.uiDC.Assets.textFont;
	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= ui_smallFont.value ) {
			fnt = &uiInfo.uiDC.Assets.smallFont;
		} else if ( scale > ui_bigFont.value ) {
			fnt = &uiInfo.uiDC.Assets.bigFont;
		}
	} else if ( font == UI_FONT_BIG ) {
		fnt = &uiInfo.uiDC.Assets.bigFont;
	} else if ( font == UI_FONT_SMALL ) {
		fnt = &uiInfo.uiDC.Assets.smallFont;
	} else if ( font == UI_FONT_HANDWRITING ) {
		fnt = &uiInfo.uiDC.Assets.handwritingFont;
	}

	useScale = scale * fnt->glyphScale;
	if ( text ) {
		const unsigned char *s = text;
		trap_R_SetColor( color );
		memcpy( &newColor[0], &color[0], sizeof( vec4_t ) );
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		while ( s && *s && count < len ) {
			glyph = &fnt->glyphs[*s];
			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE ) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					Text_PaintChar( x + ofs, y - yadj + ofs,
									glyph->imageWidth,
									glyph->imageHeight,
									font,
									useScale,
									glyph->s,
									glyph->t,
									glyph->s2,
									glyph->t2,
									glyph->glyph );
					trap_R_SetColor( newColor );
					colorBlack[3] = 1.0;
				}
				Text_PaintChar( x, y - yadj,
								glyph->imageWidth,
								glyph->imageHeight,
								font,
								useScale,
								glyph->s,
								glyph->t,
								glyph->s2,
								glyph->t2,
								glyph->glyph );

				x += ( glyph->xSkip * useScale ) + adjust;
				s++;
				count++;
			}
		}
		trap_R_SetColor( NULL );
	}
}

void Text_PaintWithCursor( float x, float y, int font, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style ) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph, *glyph2;
	float yadj;
	float useScale;

	fontInfo_t *fnt = &uiInfo.uiDC.Assets.textFont;
	if ( font == UI_FONT_DEFAULT ) {
		if ( scale <= ui_smallFont.value ) {
			fnt = &uiInfo.uiDC.Assets.smallFont;
		} else if ( scale > ui_bigFont.value ) {
			fnt = &uiInfo.uiDC.Assets.bigFont;
		}
	} else if ( font == UI_FONT_BIG ) {
		fnt = &uiInfo.uiDC.Assets.bigFont;
	} else if ( font == UI_FONT_SMALL ) {
		fnt = &uiInfo.uiDC.Assets.smallFont;
	} else if ( font == UI_FONT_HANDWRITING ) {
		fnt = &uiInfo.uiDC.Assets.handwritingFont;
	}

	useScale = scale * fnt->glyphScale;
	if ( text ) {
		const unsigned char *s = text;
		trap_R_SetColor( color );
		memcpy( &newColor[0], &color[0], sizeof( vec4_t ) );
		len = strlen( text );
		if ( limit > 0 && len > limit ) {
			len = limit;
		}
		count = 0;
		glyph2 = &fnt->glyphs[(unsigned char)cursor];
		while ( s && *s && count < len ) {
			glyph = &fnt->glyphs[*s];
			//int yadj = Assets.textFont.glyphs[text[i]].bottom + Assets.textFont.glyphs[text[i]].top;
			//float yadj = scale * (Assets.textFont.glyphs[text[i]].imageHeight - Assets.textFont.glyphs[text[i]].height);
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex( *( s + 1 ) )], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				yadj = useScale * glyph->top;
				if ( style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE ) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor( colorBlack );
					Text_PaintChar( x + ofs, y - yadj + ofs,
									glyph->imageWidth,
									glyph->imageHeight,
									font,
									useScale,
									glyph->s,
									glyph->t,
									glyph->s2,
									glyph->t2,
									glyph->glyph );
					colorBlack[3] = 1.0;
					trap_R_SetColor( newColor );
				}
				Text_PaintChar( x, y - yadj,
								glyph->imageWidth,
								glyph->imageHeight,
								font,
								useScale,
								glyph->s,
								glyph->t,
								glyph->s2,
								glyph->t2,
								glyph->glyph );

				// CG_DrawPic(x, y - yadj, scale * uiDC.Assets.textFont.glyphs[text[i]].imageWidth, scale * uiDC.Assets.textFont.glyphs[text[i]].imageHeight, uiDC.Assets.textFont.glyphs[text[i]].glyph);
				yadj = useScale * glyph2->top;
				if ( count == cursorPos && !( ( uiInfo.uiDC.realTime / BLINK_DIVISOR ) & 1 ) ) {
					Text_PaintChar( x, y - yadj,
									glyph2->imageWidth,
									glyph2->imageHeight,
									font,
									useScale,
									glyph2->s,
									glyph2->t,
									glyph2->s2,
									glyph2->t2,
									glyph2->glyph );
				}

				x += ( glyph->xSkip * useScale );
				s++;
				count++;
			}
		}
		// need to paint cursor at end of text
		if ( cursorPos == len && !( ( uiInfo.uiDC.realTime / BLINK_DIVISOR ) & 1 ) ) {
			yadj = useScale * glyph2->top;
			Text_PaintChar( x, y - yadj,
							glyph2->imageWidth,
							glyph2->imageHeight,
							font,
							useScale,
							glyph2->s,
							glyph2->t,
							glyph2->s2,
							glyph2->t2,
							glyph2->glyph );

		}


		trap_R_SetColor( NULL );
	}
}

// TTimo: unused
/*
static void Text_Paint_Limit(float *maxX, float x, float y, int font, float scale, vec4_t color, const char* text, float adjust, int limit) {
	int len, count;
	vec4_t newColor;
	glyphInfo_t *glyph;
	if (text) {
		const unsigned char *s = text;
		float max = *maxX;
		float useScale;

		fontInfo_t *fnt = &uiInfo.uiDC.Assets.textFont;
		if(font == UI_FONT_DEFAULT) {
			if (scale <= ui_smallFont.value) {
				fnt = &uiInfo.uiDC.Assets.smallFont;
			} else if (scale > ui_bigFont.value) {
				fnt = &uiInfo.uiDC.Assets.bigFont;
			}
		} else if(font == UI_FONT_BIG) {
			fnt = &uiInfo.uiDC.Assets.bigFont;
		} else if(font == UI_FONT_SMALL) {
			fnt = &uiInfo.uiDC.Assets.smallFont;
		} else if(font == UI_FONT_HANDWRITING) {
			fnt = &uiInfo.uiDC.Assets.handwritingFont;
		}

		useScale = scale * fnt->glyphScale;
		trap_R_SetColor( color );
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &fnt->glyphs[*s];
			if ( Q_IsColorString( s ) ) {
				memcpy( newColor, g_color_table[ColorIndex(*(s+1))], sizeof( newColor ) );
				newColor[3] = color[3];
				trap_R_SetColor( newColor );
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (Text_Width(s, font, useScale, 1) + x > max) {
					*maxX = 0;
					break;
				}
				Text_PaintChar(x, y - yadj,
							 glyph->imageWidth,
							 glyph->imageHeight,
							 font,
							 useScale,
							 glyph->s,
							 glyph->t,
							 glyph->s2,
							 glyph->t2,
							 glyph->glyph);
				x += (glyph->xSkip * useScale) + adjust;
				*maxX = x;
				count++;
				s++;
			}
		}
		trap_R_SetColor( NULL );
	}

}
*/

void UI_ShowPostGame( qboolean newHigh ) {
	trap_Cvar_Set( "cg_cameraOrbit", "0" );
	trap_Cvar_Set( "cg_thirdPerson", "0" );
	trap_Cvar_Set( "sv_killserver", "1" );
	uiInfo.soundHighScore = newHigh;
	_UI_SetActiveMenu( UIMENU_POSTGAME );
}
/*
=================
_UI_Refresh
=================
*/

void UI_DrawCenteredPic( qhandle_t image, int w, int h ) {
	int x, y;
	x = ( SCREEN_WIDTH - w ) / 2;
	y = ( SCREEN_HEIGHT - h ) / 2;
	UI_DrawHandlePic( x, y, w, h, image );
}

int frameCount = 0;
int startTime;

#define UI_FPS_FRAMES   4
void _UI_Refresh( int realtime ) {
	static int index;
	static int previousTimes[UI_FPS_FRAMES];

	//if ( !( trap_Key_GetCatcher() & KEYCATCH_UI ) ) {
	//	return;
	//}

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if ( index > UI_FPS_FRAMES ) {
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for ( i = 0 ; i < UI_FPS_FRAMES ; i++ ) {
			total += previousTimes[i];
		}
		if ( !total ) {
			total = 1;
		}
		uiInfo.uiDC.FPS = 1000 * UI_FPS_FRAMES / total;
	}



	UI_UpdateCvars();

	if ( Menu_Count() > 0 ) {
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
		UI_DoServerRefresh();
		// refresh server status
		UI_BuildServerStatus( qfalse );
		// refresh find player list
		UI_BuildFindPlayerList( qfalse );
	}

	// draw cursor
	UI_SetColor( NULL );
	if ( Menu_Count() > 0 ) {
		uiMenuCommand_t mymenu = _UI_GetActiveMenu();
		if ( mymenu != UIMENU_BRIEFING ) {
			UI_DrawHandlePic( uiInfo.uiDC.cursorx - 16, uiInfo.uiDC.cursory - 16, 32, 32, uiInfo.uiDC.Assets.cursor );
		}
	}

#ifndef NDEBUG
	if ( uiInfo.uiDC.debug ) {
		// cursor coordinates
		//FIXME
		//UI_DrawString( 0, 0, va("(%d,%d)",uis.cursorx,uis.cursory), UI_LEFT|UI_SMALLFONT, colorRed );
	}
#endif

}

/*
=================
_UI_Shutdown
=================
*/
void _UI_Shutdown( void ) {
	trap_LAN_SaveCachedServers();
}

char *defaultMenu = NULL;

char *GetMenuBuffer( const char *filename ) {
	int len;
	fileHandle_t f;
	static char buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "menu file not found: %s, using default\n", filename ) );
		return defaultMenu;
	}
	if ( len >= MAX_MENUFILE ) {
		trap_Print( va( S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, MAX_MENUFILE ) );
		trap_FS_FCloseFile( f );
		return defaultMenu;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	//COM_Compress(buf);
	return buf;

}

qboolean Asset_Parse( int handle ) {
	pc_token_t token;
	const char *tempStr;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}
	if ( Q_stricmp( token.string, "{" ) != 0 ) {
		return qfalse;
	}

	while ( 1 ) {

		memset( &token, 0, sizeof( pc_token_t ) );

		if ( !trap_PC_ReadToken( handle, &token ) ) {
			return qfalse;
		}

		if ( Q_stricmp( token.string, "}" ) == 0 ) {
			return qtrue;
		}

		// font
		if ( Q_stricmp( token.string, "font" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle,&pointSize ) ) {
				return qfalse;
			}
			trap_R_RegisterFont( tempStr, pointSize, &uiInfo.uiDC.Assets.textFont );
			uiInfo.uiDC.Assets.fontRegistered = qtrue;
			continue;
		}

		if ( Q_stricmp( token.string, "smallFont" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle,&pointSize ) ) {
				return qfalse;
			}
			trap_R_RegisterFont( tempStr, pointSize, &uiInfo.uiDC.Assets.smallFont );
			continue;
		}

		if ( Q_stricmp( token.string, "bigFont" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle,&pointSize ) ) {
				return qfalse;
			}
			trap_R_RegisterFont( tempStr, pointSize, &uiInfo.uiDC.Assets.bigFont );
			continue;
		}

		// handwriting
		if ( Q_stricmp( token.string, "handwritingFont" ) == 0 ) {
			int pointSize;
			if ( !PC_String_Parse( handle, &tempStr ) || !PC_Int_Parse( handle,&pointSize ) ) {
				return qfalse;
			}
			trap_R_RegisterFont( tempStr, pointSize, &uiInfo.uiDC.Assets.handwritingFont );
			continue;
		}


		// gradientbar
		if ( Q_stricmp( token.string, "gradientbar" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip( tempStr );
			continue;
		}

		// enterMenuSound
		if ( Q_stricmp( token.string, "menuEnterSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// exitMenuSound
		if ( Q_stricmp( token.string, "menuExitSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// itemFocusSound
		if ( Q_stricmp( token.string, "itemFocusSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		// menuBuzzSound
		if ( Q_stricmp( token.string, "menuBuzzSound" ) == 0 ) {
			if ( !PC_String_Parse( handle, &tempStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound( tempStr );
			continue;
		}

		if ( Q_stricmp( token.string, "cursor" ) == 0 ) {
			if ( !PC_String_Parse( handle, &uiInfo.uiDC.Assets.cursorStr ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = trap_R_RegisterShaderNoMip( uiInfo.uiDC.Assets.cursorStr );
			continue;
		}

		if ( Q_stricmp( token.string, "fadeClamp" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeClamp ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "fadeCycle" ) == 0 ) {
			if ( !PC_Int_Parse( handle, &uiInfo.uiDC.Assets.fadeCycle ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "fadeAmount" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.fadeAmount ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowX" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowX ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowY" ) == 0 ) {
			if ( !PC_Float_Parse( handle, &uiInfo.uiDC.Assets.shadowY ) ) {
				return qfalse;
			}
			continue;
		}

		if ( Q_stricmp( token.string, "shadowColor" ) == 0 ) {
			if ( !PC_Color_Parse( handle, &uiInfo.uiDC.Assets.shadowColor ) ) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.shadowFadeClamp = uiInfo.uiDC.Assets.shadowColor[3];
			continue;
		}

	}
	return qfalse;
}

void Font_Report() {
	int i;
	Com_Printf( "Font Info\n" );
	Com_Printf( "=========\n" );
	for ( i = 32; i < 96; i++ ) {
		Com_Printf( "Glyph handle %i: %i\n", i, uiInfo.uiDC.Assets.textFont.glyphs[i].glyph );
	}
}

void UI_Report() {
	String_Report();
	//Font_Report();

}

void UI_ParseMenu( const char *menuFile ) {
	int handle;
	pc_token_t token;

	Com_Printf( "Parsing menu file:%s\n", menuFile );

	handle = trap_PC_LoadSource( menuFile );
	if ( !handle ) {
		return;
	}

	while ( 1 ) {
		memset( &token, 0, sizeof( pc_token_t ) );
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//	Com_Printf( "Missing { in menu file\n" );
		//	break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//	Com_Printf( "Too many menus!\n" );
		//	break;
		//}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( Q_stricmp( token.string, "assetGlobalDef" ) == 0 ) {
			if ( Asset_Parse( handle ) ) {
				continue;
			} else {
				break;
			}
		}

		if ( Q_stricmp( token.string, "menudef" ) == 0 ) {
			// start a new menu
			Menu_New( handle );
		}
	}
	trap_PC_FreeSource( handle );
}

qboolean Load_Menu( int handle ) {
	pc_token_t token;

	if ( !trap_PC_ReadToken( handle, &token ) ) {
		return qfalse;
	}
	if ( token.string[0] != '{' ) {
		return qfalse;
	}

	while ( 1 ) {

		if ( !trap_PC_ReadToken( handle, &token ) ) {
			return qfalse;
		}

		if ( token.string[0] == 0 ) {
			return qfalse;
		}

		if ( token.string[0] == '}' ) {
			return qtrue;
		}

		UI_ParseMenu( token.string );
	}
	return qfalse;
}

void UI_LoadMenus( const char *menuFile, qboolean reset ) {
	pc_token_t token;
	int handle;
	int start;

	start = trap_Milliseconds();

	handle = trap_PC_LoadSource( menuFile );
	if ( !handle ) {
		trap_Error( va( S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile ) );
#ifdef WOLF_SP_DEMO
		handle = trap_PC_LoadSource( "ui/menus.txt" );
#else
		handle = trap_PC_LoadSource( "ui/demomenus.txt" );
#endif
		if ( !handle ) {
			trap_Error( va( S_COLOR_RED "default menu file not found: ui/menus.txt, unable to continue!\n", menuFile ) );
		}
	}

	ui_new.integer = 1;

	if ( reset ) {
		Menu_Reset();
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}
		if ( token.string[0] == 0 || token.string[0] == '}' ) {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( Q_stricmp( token.string, "loadmenu" ) == 0 ) {
			if ( Load_Menu( handle ) ) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_Printf( "UI menu load time = %d milli seconds\n", trap_Milliseconds() - start );

	trap_PC_FreeSource( handle );
}



/*
==============
UI_LoadTranslationStrings
==============
*/
#define MAX_BUFFER          20000
static void UI_LoadTranslationStrings( void ) {
	char buffer[MAX_BUFFER];
	char *text;
	char filename[MAX_QPATH];
	fileHandle_t f;
	int len, i, numStrings;
	char *token;

	Com_sprintf( filename, MAX_QPATH, "text/strings.txt" );
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
//		CG_Printf( S_COLOR_RED "WARNING: string translation file (strings.txt not found in main/text)\n" );
		return;
	}
	if ( len > MAX_BUFFER ) {
//		CG_Error( "%s is too big, make it smaller (max = %i bytes)\n", filename, MAX_BUFFER );
	}

	// load the file into memory
	trap_FS_Read( buffer, len, f );
	buffer[len] = 0;
	trap_FS_FCloseFile( f );
	// parse the list
	text = buffer;

	numStrings = sizeof( translateStrings ) / sizeof( translateStrings[0] ) - 1;

	for ( i = 0; i < numStrings; i++ ) {
		token = COM_ParseExt( &text, qtrue );
		if ( !token[0] ) {
			break;
		}
		translateStrings[i].localname = malloc( strlen( token ) + 1 );
		strcpy( translateStrings[i].localname, token );
	}
}


void UI_Load() {
	char lastName[1024];
	menuDef_t *menu = Menu_GetFocused();
	char *menuSet = UI_Cvar_VariableString( "ui_menuFiles" );
	if ( menu && menu->window.name ) {
		strcpy( lastName, menu->window.name );
	}
	if ( menuSet == NULL || menuSet[0] == '\0' ) {
#ifdef WOLF_SP_DEMO
		menuSet = "ui/demomenus.txt";
#else
		menuSet = "ui/menus.txt";
#endif
	}

	String_Init();

	// load translation text
	UI_LoadTranslationStrings();

//	UI_ParseGameInfo("gameinfo.txt");
//	UI_LoadArenas();

	UI_LoadMenus( menuSet, qtrue );
	Menus_CloseAll();
	Menus_ActivateByName( lastName );

}

static const char *handicapValues[] = {"None","95","90","85","80","75","70","65","60","55","50","45","40","35","30","25","20","15","10","5",NULL};
// TTimo: unused
//static int numHandicaps = sizeof(handicapValues) / sizeof(const char*);

static void UI_DrawHandicap( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	int i, h;

	h = Com_Clamp( 5, 100, trap_Cvar_VariableValue( "handicap" ) );
	i = 20 - h / 5;

	Text_Paint( rect->x, rect->y, font, scale, color, handicapValues[i], 0, 0, textStyle );
}

//----(SA)	added
/*
==============
UI_DrawSavegameName
==============
*/
static void UI_DrawSavegameName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	Text_PaintCenter( rect->x, rect->y, font, scale, color, ui_savegameName.string, textStyle );
}



/*
==============
UI_DrawClanName
==============
*/
static void UI_DrawClanName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	Text_Paint( rect->x, rect->y, font, scale, color, UI_Cvar_VariableString( "ui_teamName" ), 0, 0, textStyle );
}


static void UI_SetCapFragLimits( qboolean uiVars ) {
	int cap = 5;
	int frag = 10;
#ifdef MISSIONPACK
	if ( uiInfo.gameTypes[ui_gameType.integer].gtEnum == GT_OBELISK ) {
		cap = 4;
	} else if ( uiInfo.gameTypes[ui_gameType.integer].gtEnum == GT_HARVESTER ) {
		cap = 15;
	}
#endif  // #ifdef MISSIONPACK
	if ( uiVars ) {
		trap_Cvar_Set( "ui_captureLimit", va( "%d", cap ) );
		trap_Cvar_Set( "ui_fragLimit", va( "%d", frag ) );
	} else {
		trap_Cvar_Set( "capturelimit", va( "%d", cap ) );
		trap_Cvar_Set( "fraglimit", va( "%d", frag ) );
	}
}
// ui_gameType assumes gametype 0 is -1 ALL and will not show
static void UI_DrawGameType( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	Text_Paint( rect->x, rect->y, font, scale, color, uiInfo.gameTypes[ui_gameType.integer].gameType, 0, 0, textStyle );
}

static void UI_DrawNetGameType( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	if ( ui_netGameType.integer < 0 || ui_netGameType.integer > uiInfo.numGameTypes ) {
		trap_Cvar_Set( "ui_netGameType", "0" );
		trap_Cvar_Set( "ui_actualNetGameType", "0" );
	}
	Text_Paint( rect->x, rect->y, font, scale, color, uiInfo.gameTypes[ui_netGameType.integer].gameType, 0, 0, textStyle );
}

static void UI_DrawJoinGameType( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	if ( ui_joinGameType.integer < 0 || ui_joinGameType.integer > uiInfo.numJoinGameTypes ) {
		trap_Cvar_Set( "ui_joinGameType", "0" );
	}
	Text_Paint( rect->x, rect->y, font, scale, color, uiInfo.joinGameTypes[ui_joinGameType.integer].gameType, 0, 0, textStyle );
}


/*
==============
UI_SavegameIndexFromName
	sorted index in feeder
==============
*/
static int UI_SavegameIndexFromName( const char *name ) {
	int i;
	if ( name && *name ) {
		for ( i = 0; i < uiInfo.savegameCount; i++ ) {
//			if (Q_stricmp(name, uiInfo.savegameList[i].savegameName) == 0) {
			if ( Q_stricmp( name, uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[i]].savegameName ) == 0 ) {
				return i;
			}
		}
	}
	return 0;
}

/*
==============
UI_SavegameIndexFromName2
	return the index in t
==============
*/
static int UI_SavegameIndexFromName2( const char *name ) {
	int index;
	index = UI_SavegameIndexFromName( name );

	return uiInfo.savegameStatus.displaySavegames[index];
}

/*
==============
UI_DrawSaveGameShot
==============
*/
static void UI_DrawSaveGameShot( rectDef_t *rect, float scale, vec4_t color ) {
	int i;
	qhandle_t image;

	trap_R_SetColor( color );

	if ( !strlen( ui_savegameName.string ) || ui_savegameName.string[0] == '0' ) {
		image = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
	} else {
		i = UI_SavegameIndexFromName2( ui_savegameName.string );
//	mapName
		if ( uiInfo.savegameList[i].sshotImage == -1 ) {
			uiInfo.savegameList[i].sshotImage = trap_R_RegisterShaderNoMip( uiInfo.savegameList[i].imageName );
		}

		image = uiInfo.savegameList[i].sshotImage;
	}

	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, image );
	trap_R_SetColor( NULL );

}


/*
==============
UI_DrawMessageIcon
==============
*/
// TTimo: unused
/*
//----(SA)	added
static void UI_DrawMessageIcon(rectDef_t *rect, float scale, vec4_t color) {
	trap_R_SetColor( color );
//	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.savegameList[uiInfo.savegameIndex].sshotImage);
	trap_R_SetColor(NULL);
}
*/

static int UI_TeamIndexFromName( const char *name ) {
	int i;

	if ( name && *name ) {
		for ( i = 0; i < uiInfo.teamCount; i++ ) {
			if ( Q_stricmp( name, uiInfo.teamList[i].teamName ) == 0 ) {
				return i;
			}
		}
	}

	return 0;

}


/*
==============
UI_DrawClanLogo
==============
*/
static void UI_DrawClanLogo( rectDef_t *rect, float scale, vec4_t color ) {
	int i;
	i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
	if ( i >= 0 && i < uiInfo.teamCount ) {
		trap_R_SetColor( color );

		if ( uiInfo.teamList[i].teamIcon == -1 ) {
			uiInfo.teamList[i].teamIcon         = trap_R_RegisterShaderNoMip( uiInfo.teamList[i].imageName );
			uiInfo.teamList[i].teamIcon_Metal   = trap_R_RegisterShaderNoMip( va( "%s_metal",uiInfo.teamList[i].imageName ) );
			uiInfo.teamList[i].teamIcon_Name    = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[i].imageName ) );
		}

		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon );
		trap_R_SetColor( NULL );
	}
}

/*
==============
UI_DrawClanCinematic
==============
*/
static void UI_DrawClanCinematic( rectDef_t *rect, float scale, vec4_t color ) {
	int i;
	i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
	if ( i >= 0 && i < uiInfo.teamCount ) {

		if ( uiInfo.teamList[i].cinematic >= -2 ) {
			if ( uiInfo.teamList[i].cinematic == -1 ) {
				uiInfo.teamList[i].cinematic = trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.teamList[i].imageName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
			}
			if ( uiInfo.teamList[i].cinematic >= 0 ) {
				trap_CIN_RunCinematic( uiInfo.teamList[i].cinematic );
				trap_CIN_SetExtents( uiInfo.teamList[i].cinematic, rect->x, rect->y, rect->w, rect->h );
				trap_CIN_DrawCinematic( uiInfo.teamList[i].cinematic );
			} else {
				uiInfo.teamList[i].cinematic = -2;
			}
		} else {
			trap_R_SetColor( color );
			UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon );
			trap_R_SetColor( NULL );
		}
	}

}


static void UI_DrawPregameCinematic( rectDef_t *rect, float scale, vec4_t color ) {
	if ( uiInfo.previewMovie > -2 ) {
		uiInfo.previewMovie = trap_CIN_PlayCinematic( va( "%s.roq", "assault" ), 0, 0, 0, 0, ( CIN_loop | CIN_silent | CIN_system ) );
		if ( uiInfo.previewMovie >= 0 ) {
			trap_CIN_RunCinematic( uiInfo.previewMovie );
			trap_CIN_SetExtents( uiInfo.previewMovie, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic( uiInfo.previewMovie );
		} else {
			uiInfo.previewMovie = -2;
		}
	}

}

static void UI_DrawPreviewCinematic( rectDef_t *rect, float scale, vec4_t color ) {
	if ( uiInfo.previewMovie > -2 ) {
		uiInfo.previewMovie = trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.movieList[uiInfo.movieIndex] ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		if ( uiInfo.previewMovie >= 0 ) {
			trap_CIN_RunCinematic( uiInfo.previewMovie );
			trap_CIN_SetExtents( uiInfo.previewMovie, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic( uiInfo.previewMovie );
		} else {
			uiInfo.previewMovie = -2;
		}
	}

}


static void UI_DrawSkill( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	int i;
	i = trap_Cvar_VariableValue( "g_spSkill" );
	if ( i < 1 || i > numSkillLevels ) {
		i = 1;
	}
	Text_Paint( rect->x, rect->y, font, scale, color, skillLevels[i - 1],0, 0, textStyle );
}


static void UI_DrawTeamName( rectDef_t *rect, int font, float scale, vec4_t color, qboolean blue, int textStyle ) {
	int i;
	i = UI_TeamIndexFromName( UI_Cvar_VariableString( ( blue ) ? "ui_blueTeam" : "ui_redTeam" ) );
	if ( i >= 0 && i < uiInfo.teamCount ) {
		Text_Paint( rect->x, rect->y, font, scale, color, va( "%s: %s", ( blue ) ? "Blue" : "Red", uiInfo.teamList[i].teamName ),0, 0, textStyle );
	}
}

static void UI_DrawTeamMember( rectDef_t *rect, int font, float scale, vec4_t color, qboolean blue, int num, int textStyle ) {
#ifdef MISSIONPACK
	// 0 - None
	// 1 - Human
	// 2..NumCharacters - Bot
	int value = trap_Cvar_VariableValue( va( blue ? "ui_blueteam%i" : "ui_redteam%i", num ) );
	const char *text;
	if ( value <= 0 ) {
		text = "Closed";
	} else if ( value == 1 ) {
		text = "Human";
	} else {
		value -= 2;

		if ( ui_actualNetGameType.integer >= GT_TEAM ) {
			if ( value >= uiInfo.characterCount ) {
				value = 0;
			}
			text = uiInfo.characterList[value].name;
		} else {
			if ( value >= UI_GetNumBots() ) {
				value = 0;
			}
			text = UI_GetBotNameByNumber( value );
		}
	}
	Text_Paint( rect->x, rect->y, font, scale, color, text, 0, 0, textStyle );
#endif  // #ifdef MISSIONPACK
}

static void UI_DrawEffects( rectDef_t *rect, float scale, vec4_t color ) {
	UI_DrawHandlePic( rect->x, rect->y - 14, 128, 8, uiInfo.uiDC.Assets.fxBasePic );
	UI_DrawHandlePic( rect->x + uiInfo.effectsColor * 16 + 8, rect->y - 16, 16, 12, uiInfo.uiDC.Assets.fxPic[uiInfo.effectsColor] );
}

//----(SA)	added
/*
==============
UI_DrawMapLevelshot
	Draws the levelshot for the current map.
==============
*/
static void UI_DrawMapLevelshot( rectDef_t *rect ) {
	char levelname[64];
	qhandle_t levelshot = 0;

	DC->getCVarString( "mapname", levelname, sizeof( levelname ) );

	if ( levelname && levelname[0] != 0 ) {
		levelshot = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga", levelname ) );
	}

	if ( !levelshot ) {
		levelshot = trap_R_RegisterShaderNoMip( "menu/art/unknownmap" );
	}

	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, levelshot );
}

/*
==============
CG_HorizontalPercentBar
	Generic routine for pretty much all status indicators that show a fractional
	value to the palyer by virtue of how full a drawn box is.

flags:
	left		- 1
	center		- 2		// direction is 'right' by default and orientation is 'horizontal'
	vert		- 4
	nohudalpha	- 8		// don't adjust bar's alpha value by the cg_hudalpha value
	bg			- 16	// background contrast box (bg set with bgColor of 'NULL' means use default bg color (1,1,1,0.25)
	spacing		- 32	// some bars use different sorts of spacing when drawing both an inner and outer box

	lerp color	- 256	// use an average of the start and end colors to set the fill color
==============
*/


// TODO: these flags will be shared, but it was easier to work on stuff if I wasn't changing header files a lot
#define BAR_LEFT        0x0001
#define BAR_CENTER      0x0002
#define BAR_VERT        0x0004
#define BAR_NOHUDALPHA  0x0008
#define BAR_BG          0x0010
// different spacing modes for use w/ BAR_BG
#define BAR_BGSPACING_X0Y5  0x0020
#define BAR_BGSPACING_X0Y0  0x0040

#define BAR_LERP_COLOR  0x0100

#define BAR_BORDERSIZE 2

void UI_FilledBar( float x, float y, float w, float h, float *startColor, float *endColor, const float *bgColor, float frac, int flags ) {
	vec4_t backgroundcolor = {1, 1, 1, 0.25f}, colorAtPos;  // colorAtPos is the lerped color if necessary
	int indent = BAR_BORDERSIZE;

	if ( ( flags & BAR_BG ) && bgColor ) { // BAR_BG set, and color specified, use specified bg color
		Vector4Copy( bgColor, backgroundcolor );
	}

	// hud alpha
	if ( !( flags & BAR_NOHUDALPHA ) ) {
		startColor[3] *= ui_hudAlpha.value;
		if ( endColor ) {
			endColor[3] *= ui_hudAlpha.value;
		}
		if ( backgroundcolor ) {
			backgroundcolor[3] *= ui_hudAlpha.value;
		}
	}

	if ( flags & BAR_LERP_COLOR ) {
		Vector4Average( startColor, endColor, frac, colorAtPos );
	}

	// background
	if ( ( flags & BAR_BG ) ) {
		// draw background at full size and shrink the remaining box to fit inside with a border.  (alternate border may be specified by a BAR_BGSPACING_xx)
		UI_FillRect(   x,
					   y,
					   w,
					   h,
					   backgroundcolor );

		if ( flags & BAR_BGSPACING_X0Y0 ) {          // fill the whole box (no border)

		} else if ( flags & BAR_BGSPACING_X0Y5 ) {   // spacing created for weapon heat
			indent *= 3;
			y += indent;
			h -= ( 2 * indent );

		} else {                                // default spacing of 2 units on each side
			x += indent;
			y += indent;
			w -= ( 2 * indent );
			h -= ( 2 * indent );
		}
	}


	// adjust for horiz/vertical and draw the fractional box
	if ( flags & BAR_VERT ) {
		if ( flags & BAR_LEFT ) {    // TODO: remember to swap colors on the ends here
			y += ( h * ( 1 - frac ) );
		} else if ( flags & BAR_CENTER ) {
			y += ( h * ( 1 - frac ) / 2 );
		}

		if ( flags & BAR_LERP_COLOR ) {
			UI_FillRect( x, y, w, h * frac, colorAtPos );
		} else {
			UI_FillRect( x, y, w, h * frac, startColor );
		}

	} else {

		if ( flags & BAR_LEFT ) {    // TODO: remember to swap colors on the ends here
			x += ( w * ( 1 - frac ) );
		} else if ( flags & BAR_CENTER ) {
			x += ( w * ( 1 - frac ) / 2 );
		}

		if ( flags & BAR_LERP_COLOR ) {
			UI_FillRect( x, y, w * frac, h, colorAtPos );
		} else {
			UI_FillRect( x, y, w * frac, h, startColor );
		}
	}

}



/*
==============
UI_DrawLoadStatus
==============
*/
static void UI_DrawLoadStatus( rectDef_t *rect, vec4_t color, int align ) {
	int expectedHunk;
	float percentDone = 0.0f;
	char hunkBuf[MAX_QPATH];
	int flags = 0;

	if ( align != HUD_HORIZONTAL ) {
		flags |= 4;   // BAR_VERT
//		flags|=1;	// BAR_LEFT (left, when vertical means grow 'up')
	}

	flags |= 16;      // BAR_BG			- draw the filled contrast box

	trap_Cvar_VariableStringBuffer( "com_expectedhunkusage", hunkBuf, MAX_QPATH );
	expectedHunk = atoi( hunkBuf );

	if ( expectedHunk > 0 ) {
		percentDone = (float)( ui_hunkUsed.integer ) / (float)( expectedHunk );
		if ( percentDone > 0.97 ) { // never actually show 100%, since we are not in the game yet
			percentDone = 0.97;
		}

		UI_FilledBar( rect->x, rect->y, rect->w, rect->h, color, NULL, NULL, percentDone, flags ); // flags (BAR_CENTER|BAR_VERT|BAR_LERP_COLOR)
	} else {
//		Text_Paint( rect->x, rect->y, UI_FONT_DEFAULT, 0.2f, color, "Please Wait...", 0, 0, 0);
		Text_Paint( rect->x, rect->y, UI_FONT_DEFAULT, 0.2f, color, DC->getTranslatedString( "pleasewait" ), 0, 0, 0 );
	}

}
//----(SA)	end


static void UI_DrawMapPreview( rectDef_t *rect, float scale, vec4_t color, qboolean net ) {
	int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if ( map < 0 || map > uiInfo.mapCount ) {
		if ( net ) {
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set( "ui_currentNetMap", "0" );
		} else {
			ui_currentMap.integer = 0;
			trap_Cvar_Set( "ui_currentMap", "0" );
		}
		map = 0;
	}

	if ( uiInfo.mapList[map].levelShot == -1 ) {
		uiInfo.mapList[map].levelShot = trap_R_RegisterShaderNoMip( uiInfo.mapList[map].imageName );
	}

	if ( uiInfo.mapList[map].levelShot > 0 ) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.mapList[map].levelShot );
	} else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "menu/art/unknownmap" ) );
	}
}


static void UI_DrawMapTimeToBeat( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	int minutes, seconds, time;
	if ( ui_currentMap.integer < 0 || ui_currentMap.integer > uiInfo.mapCount ) {
		ui_currentMap.integer = 0;
		trap_Cvar_Set( "ui_currentMap", "0" );
	}

	time = uiInfo.mapList[ui_currentMap.integer].timeToBeat[uiInfo.gameTypes[ui_gameType.integer].gtEnum];

	minutes = time / 60;
	seconds = time % 60;

	Text_Paint( rect->x, rect->y, font, scale, color, va( "%02i:%02i", minutes, seconds ), 0, 0, textStyle );
}



static void UI_DrawMapCinematic( rectDef_t *rect, float scale, vec4_t color, qboolean net ) {

	int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if ( map < 0 || map > uiInfo.mapCount ) {
		if ( net ) {
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set( "ui_currentNetMap", "0" );
		} else {
			ui_currentMap.integer = 0;
			trap_Cvar_Set( "ui_currentMap", "0" );
		}
		map = 0;
	}

	if ( uiInfo.mapList[map].cinematic >= -1 ) {
		if ( uiInfo.mapList[map].cinematic == -1 ) {
			uiInfo.mapList[map].cinematic = trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.mapList[map].mapLoadName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		}
		if ( uiInfo.mapList[map].cinematic >= 0 ) {
			trap_CIN_RunCinematic( uiInfo.mapList[map].cinematic );
			trap_CIN_SetExtents( uiInfo.mapList[map].cinematic, rect->x, rect->y, rect->w, rect->h );
			trap_CIN_DrawCinematic( uiInfo.mapList[map].cinematic );
		} else {
			uiInfo.mapList[map].cinematic = -2;
		}
	} else {
		UI_DrawMapPreview( rect, scale, color, net );
	}
}



static qboolean updateModel = qtrue;
static qboolean q3Model = qfalse;

static void UI_DrawPlayerModel( rectDef_t *rect ) {
	static playerInfo_t info;
	char model[MAX_QPATH];
	char team[256];
	char head[256];
	vec3_t viewangles;
	static vec3_t moveangles = { 0, 0, 0 };

	if ( trap_Cvar_VariableValue( "ui_Q3Model" ) ) {
		//	  strcpy(model, UI_Cvar_VariableString("model"));
		strcpy( model, "multi" );
		strcpy( head, UI_Cvar_VariableString( "headmodel" ) );
		if ( !q3Model ) {
			q3Model = qtrue;
			updateModel = qtrue;
		}
		team[0] = '\0';
	} else {
		strcpy( model, UI_Cvar_VariableString( "team_model" ) );
		strcpy( head, UI_Cvar_VariableString( "team_headmodel" ) );
		strcpy( team, UI_Cvar_VariableString( "ui_teamName" ) );
		if ( q3Model ) {
			q3Model = qfalse;
			updateModel = qtrue;
		}
	}

	moveangles[YAW] += 1;       // NERVE - SMF - TEMPORARY

	// compare new cvars to old cvars and see if we need to update
	{
		int v1, v2;

		v1 = trap_Cvar_VariableValue( "mp_team" );
		v2 = trap_Cvar_VariableValue( "ui_prevTeam" );
		if ( v1 != v2 ) {
			trap_Cvar_Set( "ui_prevTeam", va( "%i", v1 ) );
			updateModel = qtrue;
		}

		v1 = trap_Cvar_VariableValue( "mp_playerType" );
		v2 = trap_Cvar_VariableValue( "ui_prevClass" );
		if ( v1 != v2 ) {
			trap_Cvar_Set( "ui_prevClass", va( "%i", v1 ) );
			updateModel = qtrue;
		}

		v1 = trap_Cvar_VariableValue( "mp_weapon" );
		v2 = trap_Cvar_VariableValue( "ui_prevWeapon" );
		if ( v1 != v2 ) {
			trap_Cvar_Set( "ui_prevWeapon", va( "%i", v1 ) );
			updateModel = qtrue;
		}
	}

	if ( updateModel ) {      // NERVE - SMF - TEMPORARY
		memset( &info, 0, sizeof( playerInfo_t ) );
		viewangles[YAW]   = 180 - 10;
		viewangles[PITCH] = 0;
		viewangles[ROLL]  = 0;
		//      VectorClear( moveangles );
#ifdef MISSIONPACK
		UI_PlayerInfo_SetModel( &info, model, head, team );
#else
		UI_PlayerInfo_SetModel( &info, model );
#endif  // MISSIONPACK
		UI_PlayerInfo_SetInfo( &info, LEGS_IDLE, TORSO_STAND, viewangles, moveangles, -1, qfalse );
		//		UI_RegisterClientModelname( &info, model, head, team);
		updateModel = qfalse;
	} else {
		VectorCopy( moveangles, info.moveAngles );
	}

	//	info.moveAngles[YAW] += 1;
	//   UI_PlayerInfo_SetInfo( &info, LEGS_IDLE, TORSO_STAND, viewangles, moveangles, WP_MP40, qfalse );
	UI_DrawPlayer( rect->x, rect->y, rect->w, rect->h, &info, uiInfo.uiDC.realTime / 2 );

}

static void UI_DrawNetSource( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	if ( ui_netSource.integer < 0 || ui_netSource.integer > uiInfo.numGameTypes ) {
		ui_netSource.integer = 0;
	}
	Text_Paint( rect->x, rect->y, font, scale, color, va( "Source: %s", netSources[ui_netSource.integer] ), 0, 0, textStyle );
}

static void UI_DrawNetMapPreview( rectDef_t *rect, float scale, vec4_t color ) {

	if ( uiInfo.serverStatus.currentServerPreview > 0 ) {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview );
	} else {
		UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip( "menu/art/unknownmap" ) );
	}
}

static void UI_DrawNetMapCinematic( rectDef_t *rect, float scale, vec4_t color ) {
	if ( ui_currentNetMap.integer < 0 || ui_currentNetMap.integer > uiInfo.mapCount ) {
		ui_currentNetMap.integer = 0;
		trap_Cvar_Set( "ui_currentNetMap", "0" );
	}

	if ( uiInfo.serverStatus.currentServerCinematic >= 0 ) {
		trap_CIN_RunCinematic( uiInfo.serverStatus.currentServerCinematic );
		trap_CIN_SetExtents( uiInfo.serverStatus.currentServerCinematic, rect->x, rect->y, rect->w, rect->h );
		trap_CIN_DrawCinematic( uiInfo.serverStatus.currentServerCinematic );
	} else {
		UI_DrawNetMapPreview( rect, scale, color );
	}
}



static void UI_DrawNetFilter( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	if ( ui_serverFilterType.integer < 0 || ui_serverFilterType.integer > numServerFilters ) {
		ui_serverFilterType.integer = 0;
	}
	Text_Paint( rect->x, rect->y, font, scale, color, va( "Filter: %s", serverFilters[ui_serverFilterType.integer].description ), 0, 0, textStyle );
}


static void UI_DrawTier( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	int i;
	i = trap_Cvar_VariableValue( "ui_currentTier" );
	if ( i < 0 || i >= uiInfo.tierCount ) {
		i = 0;
	}
	Text_Paint( rect->x, rect->y, font, scale, color, va( "Tier: %s", uiInfo.tierList[i].tierName ),0, 0, textStyle );
}

static void UI_DrawTierMap( rectDef_t *rect, int index ) {
	int i;
	i = trap_Cvar_VariableValue( "ui_currentTier" );
	if ( i < 0 || i >= uiInfo.tierCount ) {
		i = 0;
	}

	if ( uiInfo.tierList[i].mapHandles[index] == -1 ) {
		uiInfo.tierList[i].mapHandles[index] = trap_R_RegisterShaderNoMip( va( "levelshots/%s", uiInfo.tierList[i].maps[index] ) );
	}

	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.tierList[i].mapHandles[index] );
}

static const char *UI_EnglishMapName( const char *map ) {
	int i;
	for ( i = 0; i < uiInfo.mapCount; i++ ) {
		if ( Q_stricmp( map, uiInfo.mapList[i].mapLoadName ) == 0 ) {
			return uiInfo.mapList[i].mapName;
		}
	}
	return "";
}

static void UI_DrawTierMapName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	int i, j;
	i = trap_Cvar_VariableValue( "ui_currentTier" );
	if ( i < 0 || i >= uiInfo.tierCount ) {
		i = 0;
	}
	j = trap_Cvar_VariableValue( "ui_currentMap" );
	if ( j < 0 || j > MAPS_PER_TIER ) {
		j = 0;
	}

	Text_Paint( rect->x, rect->y, font, scale, color, UI_EnglishMapName( uiInfo.tierList[i].maps[j] ), 0, 0, textStyle );
}

static void UI_DrawTierGameType( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	int i, j;
	i = trap_Cvar_VariableValue( "ui_currentTier" );
	if ( i < 0 || i >= uiInfo.tierCount ) {
		i = 0;
	}
	j = trap_Cvar_VariableValue( "ui_currentMap" );
	if ( j < 0 || j > MAPS_PER_TIER ) {
		j = 0;
	}

	Text_Paint( rect->x, rect->y, font, scale, color, uiInfo.gameTypes[uiInfo.tierList[i].gameTypes[j]].gameType, 0, 0, textStyle );
}

// TTimo: unused
/*
static const char *UI_OpponentLeaderName() {
	int i = UI_TeamIndexFromName(UI_Cvar_VariableString("ui_opponentName"));
	return uiInfo.teamList[i].teamMembers[0];
}

static const char *UI_AIFromName(const char *name) {
	int j;
	for (j = 0; j < uiInfo.aliasCount; j++) {
		if (Q_stricmp(uiInfo.aliasList[j].name, name) == 0) {
			return uiInfo.aliasList[j].ai;
		}
	}
	return "James";
}

static const int UI_AIIndex(const char *name) {
	int j;
	for (j = 0; j < uiInfo.characterCount; j++) {
		if (Q_stricmp(name, uiInfo.characterList[j].name) == 0) {
			return j;
		}
	}
	return 0;
}

static const int UI_AIIndexFromName(const char *name) {
	int j;
	for (j = 0; j < uiInfo.aliasCount; j++) {
		if (Q_stricmp(uiInfo.aliasList[j].name, name) == 0) {
			return UI_AIIndex(uiInfo.aliasList[j].ai);
		}
	}
	return 0;
}

static const char *UI_OpponentLeaderHead() {
	const char *leader = UI_OpponentLeaderName();
	return UI_AIFromName(leader);
}

static const char *UI_OpponentLeaderModel() {
	int i;
	const char *head = UI_OpponentLeaderHead();
	for (i = 0; i < uiInfo.characterCount; i++) {
		if (Q_stricmp(head, uiInfo.characterList[i].name) == 0) {
			if (uiInfo.characterList[i].female) {
				return "Janet";
			} else {
				return "James";
			}
		}
	}
	return "James";
}
*/

static qboolean updateOpponentModel = qtrue;
static void UI_DrawOpponent( rectDef_t *rect ) {
	static playerInfo_t info2;
	char model[MAX_QPATH];
	char headmodel[MAX_QPATH];
	char team[256];
	vec3_t viewangles;
	vec3_t moveangles;

	if ( updateOpponentModel ) {

		strcpy( model, UI_Cvar_VariableString( "ui_opponentModel" ) );
		strcpy( headmodel, UI_Cvar_VariableString( "ui_opponentModel" ) );
		team[0] = '\0';

		memset( &info2, 0, sizeof( playerInfo_t ) );
		viewangles[YAW]   = 180 - 10;
		viewangles[PITCH] = 0;
		viewangles[ROLL]  = 0;
		VectorClear( moveangles );
#ifdef MISSIONPACK
		UI_PlayerInfo_SetModel( &info2, model, headmodel, "" );
#else
		UI_PlayerInfo_SetModel( &info2, model );
#endif  // #ifdef MISSIONPACK
		UI_PlayerInfo_SetInfo( &info2, LEGS_IDLE, TORSO_STAND, viewangles, vec3_origin, WP_MP40, qfalse );
#ifdef MISSIONPACK
		UI_RegisterClientModelname( &info2, model, headmodel, team );
#else
		UI_RegisterClientModelname( &info2, model );
#endif  // #ifdef MISSIONPACK
		updateOpponentModel = qfalse;
	}

	UI_DrawPlayer( rect->x, rect->y, rect->w, rect->h, &info2, uiInfo.uiDC.realTime / 2 );

}

static void UI_NextOpponent() {
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );
	int j = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
	i++;
	if ( i >= uiInfo.teamCount ) {
		i = 0;
	}
	if ( i == j ) {
		i++;
		if ( i >= uiInfo.teamCount ) {
			i = 0;
		}
	}
	trap_Cvar_Set( "ui_opponentName", uiInfo.teamList[i].teamName );
}

static void UI_PriorOpponent() {
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );
	int j = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
	i--;
	if ( i < 0 ) {
		i = uiInfo.teamCount - 1;
	}
	if ( i == j ) {
		i--;
		if ( i < 0 ) {
			i = uiInfo.teamCount - 1;
		}
	}
	trap_Cvar_Set( "ui_opponentName", uiInfo.teamList[i].teamName );
}

static void UI_DrawPlayerLogo( rectDef_t *rect, vec3_t color ) {
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );

	if ( uiInfo.teamList[i].teamIcon == -1 ) {
		uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[i].imageName );
		uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal",uiInfo.teamList[i].imageName ) );
		uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[i].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon );
	trap_R_SetColor( NULL );
}

static void UI_DrawPlayerLogoMetal( rectDef_t *rect, vec3_t color ) {
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
	if ( uiInfo.teamList[i].teamIcon == -1 ) {
		uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[i].imageName );
		uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal",uiInfo.teamList[i].imageName ) );
		uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[i].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal );
	trap_R_SetColor( NULL );
}

static void UI_DrawPlayerLogoName( rectDef_t *rect, vec3_t color ) {
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
	if ( uiInfo.teamList[i].teamIcon == -1 ) {
		uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[i].imageName );
		uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal",uiInfo.teamList[i].imageName ) );
		uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[i].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Name );
	trap_R_SetColor( NULL );
}

static void UI_DrawOpponentLogo( rectDef_t *rect, vec3_t color ) {
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );
	if ( uiInfo.teamList[i].teamIcon == -1 ) {
		uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[i].imageName );
		uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal",uiInfo.teamList[i].imageName ) );
		uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[i].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon );
	trap_R_SetColor( NULL );
}

static void UI_DrawOpponentLogoMetal( rectDef_t *rect, vec3_t color ) {
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );
	if ( uiInfo.teamList[i].teamIcon == -1 ) {
		uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[i].imageName );
		uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal",uiInfo.teamList[i].imageName ) );
		uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[i].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Metal );
	trap_R_SetColor( NULL );
}

static void UI_DrawOpponentLogoName( rectDef_t *rect, vec3_t color ) {
	int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );
	if ( uiInfo.teamList[i].teamIcon == -1 ) {
		uiInfo.teamList[i].teamIcon = trap_R_RegisterShaderNoMip( uiInfo.teamList[i].imageName );
		uiInfo.teamList[i].teamIcon_Metal = trap_R_RegisterShaderNoMip( va( "%s_metal",uiInfo.teamList[i].imageName ) );
		uiInfo.teamList[i].teamIcon_Name = trap_R_RegisterShaderNoMip( va( "%s_name", uiInfo.teamList[i].imageName ) );
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y, rect->w, rect->h, uiInfo.teamList[i].teamIcon_Name );
	trap_R_SetColor( NULL );
}

static void UI_DrawAllMapsSelection( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle, qboolean net ) {
#ifdef MISSIONPACK
	int map = ( net ) ? ui_currentNetMap.integer : ui_currentMap.integer;
	if ( map >= 0 && map < uiInfo.mapCount ) {
		Text_Paint( rect->x, rect->y, font, scale, color, uiInfo.mapList[map].mapName, 0, 0, textStyle );
	}
#endif  // #ifdef MISSIONPACK
}

static void UI_DrawOpponentName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	Text_Paint( rect->x, rect->y, font, scale, color, UI_Cvar_VariableString( "ui_opponentName" ), 0, 0, textStyle );
}


static int UI_OwnerDrawWidth( int ownerDraw, int font, float scale ) {
	int i, h, value;
	const char *text;
	const char *s = NULL;

	switch ( ownerDraw ) {
	case UI_HANDICAP:
		h = Com_Clamp( 5, 100, trap_Cvar_VariableValue( "handicap" ) );
		i = 20 - h / 5;
		s = handicapValues[i];
		break;

//----(SA)	added
	case UI_SAVEGAMENAME:
		s = ui_savegameName.string;
		break;

	case UI_SAVEGAMEINFO:
		i = UI_SavegameIndexFromName2( ui_savegameName.string );
		s = uiInfo.savegameList[i].savegameInfoText;
		break;
//----(SA)	end

	case UI_CLANNAME:
		s = UI_Cvar_VariableString( "ui_teamName" );
		break;
	case UI_GAMETYPE:
		s = uiInfo.gameTypes[ui_gameType.integer].gameType;
		break;
	case UI_SKILL:
		i = trap_Cvar_VariableValue( "g_spSkill" );
		if ( i < 1 || i > numSkillLevels ) {
			i = 1;
		}
		s = skillLevels[i - 1];
		break;
	case UI_BLUETEAMNAME:
		i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_blueTeam" ) );
		if ( i >= 0 && i < uiInfo.teamCount ) {
			s = va( "%s: %s", "Blue", uiInfo.teamList[i].teamName );
		}
		break;
	case UI_REDTEAMNAME:
		i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_redTeam" ) );
		if ( i >= 0 && i < uiInfo.teamCount ) {
			s = va( "%s: %s", "Red", uiInfo.teamList[i].teamName );
		}
		break;
	case UI_BLUETEAM1:
	case UI_BLUETEAM2:
	case UI_BLUETEAM3:
	case UI_BLUETEAM4:
	case UI_BLUETEAM5:
		value = trap_Cvar_VariableValue( va( "ui_blueteam%i", ownerDraw - UI_BLUETEAM1 + 1 ) );
		if ( value <= 0 ) {
			text = "Closed";
		} else if ( value == 1 ) {
			text = "Human";
		} else {
			value -= 2;
			if ( value >= uiInfo.aliasCount ) {
				value = 0;
			}
			text = uiInfo.aliasList[value].name;
		}
		s = va( "%i. %s", ownerDraw - UI_BLUETEAM1 + 1, text );
		break;
	case UI_REDTEAM1:
	case UI_REDTEAM2:
	case UI_REDTEAM3:
	case UI_REDTEAM4:
	case UI_REDTEAM5:
		value = trap_Cvar_VariableValue( va( "ui_redteam%i", ownerDraw - UI_REDTEAM1 + 1 ) );
		if ( value <= 0 ) {
			text = "Closed";
		} else if ( value == 1 ) {
			text = "Human";
		} else {
			value -= 2;
			if ( value >= uiInfo.aliasCount ) {
				value = 0;
			}
			text = uiInfo.aliasList[value].name;
		}
		s = va( "%i. %s", ownerDraw - UI_REDTEAM1 + 1, text );
		break;
	case UI_NETSOURCE:
		if ( ui_netSource.integer < 0 || ui_netSource.integer > uiInfo.numJoinGameTypes ) {
			ui_netSource.integer = 0;
		}
		s = va( "Source: %s", netSources[ui_netSource.integer] );
		break;
	case UI_NETFILTER:
		if ( ui_serverFilterType.integer < 0 || ui_serverFilterType.integer > numServerFilters ) {
			ui_serverFilterType.integer = 0;
		}
		s = va( "Filter: %s", serverFilters[ui_serverFilterType.integer].description );
		break;
	case UI_TIER:
		break;
	case UI_TIER_MAPNAME:
		break;
	case UI_TIER_GAMETYPE:
		break;
	case UI_ALLMAPS_SELECTION:
		break;
	case UI_OPPONENT_NAME:
		break;
	case UI_KEYBINDSTATUS:
		if ( Display_KeyBindPending() ) {
//			s = "Waiting for new key... Press ESCAPE to cancel";
			s = DC->getTranslatedString( "keywait" );
		} else {
//			s = "Press ENTER or CLICK to change, Press BACKSPACE to clear";
			s = DC->getTranslatedString( "keychange" );
		}
		break;
	case UI_SERVERREFRESHDATE:
#ifdef MISSIONPACK
		s = UI_Cvar_VariableString( va( "ui_lastServerRefresh_%i", ui_netSource.integer ) );
#endif  // #ifdef MISSIONPACK
		break;
	default:
		break;
	}

	if ( s ) {
		return Text_Width( s, font, scale, 0 );
	}
	return 0;
}

static void UI_DrawBotName( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
#ifdef MISSIONPACK
	int value = uiInfo.botIndex;
	int game = trap_Cvar_VariableValue( "g_gametype" );
	const char *text = "";
	if ( game >= GT_TEAM ) {
		if ( value >= uiInfo.characterCount ) {
			value = 0;
		}
		text = uiInfo.characterList[value].name;
	} else {
		if ( value >= UI_GetNumBots() ) {
			value = 0;
		}
		text = UI_GetBotNameByNumber( value );
	}
	Text_Paint( rect->x, rect->y, font, scale, color, text, 0, 0, textStyle );
#endif  // #ifdef MISSIONPACK
}

static void UI_DrawBotSkill( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	if ( uiInfo.skillIndex >= 0 && uiInfo.skillIndex < numSkillLevels ) {
		Text_Paint( rect->x, rect->y, font, scale, color, skillLevels[uiInfo.skillIndex], 0, 0, textStyle );
	}
}

static void UI_DrawRedBlue( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	Text_Paint( rect->x, rect->y, font, scale, color, ( uiInfo.redBlue == 0 ) ? "Red" : "Blue", 0, 0, textStyle );
}

static void UI_DrawCrosshair( rectDef_t *rect, float scale, vec4_t color ) {
	int ch;
	ch = ( uiInfo.currentCrosshair % NUM_CROSSHAIRS );

	if ( !ch ) {
		return;
	}

	trap_R_SetColor( color );
	UI_DrawHandlePic( rect->x, rect->y - rect->h, rect->w, rect->h, uiInfo.uiDC.Assets.crosshairShader[ch] );
	trap_R_SetColor( NULL );
}

/*
===============
UI_BuildPlayerList
===============
*/
static void UI_BuildPlayerList() {
	uiClientState_t cs;
	int n, count, team, team2, playerTeamNumber;
	char info[MAX_INFO_STRING];

	trap_GetClientState( &cs );
	trap_GetConfigString( CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING );
	uiInfo.playerNumber = cs.clientNum;
	uiInfo.teamLeader = atoi( Info_ValueForKey( info, "tl" ) );
	team = atoi( Info_ValueForKey( info, "t" ) );
	trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) );
	count = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	playerTeamNumber = 0;
	for ( n = 0; n < count; n++ ) {
		trap_GetConfigString( CS_PLAYERS + n, info, MAX_INFO_STRING );

		if ( info[0] ) {
			Q_strncpyz( uiInfo.playerNames[uiInfo.playerCount], Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
			Q_CleanStr( uiInfo.playerNames[uiInfo.playerCount] );
			uiInfo.playerCount++;
			team2 = atoi( Info_ValueForKey( info, "t" ) );
			if ( team2 == team ) {
				Q_strncpyz( uiInfo.teamNames[uiInfo.myTeamCount], Info_ValueForKey( info, "n" ), MAX_NAME_LENGTH );
				Q_CleanStr( uiInfo.teamNames[uiInfo.myTeamCount] );
				uiInfo.teamClientNums[uiInfo.myTeamCount] = n;
				if ( uiInfo.playerNumber == n ) {
					playerTeamNumber = uiInfo.myTeamCount;
				}
				uiInfo.myTeamCount++;
			}
		}
	}

	if ( !uiInfo.teamLeader ) {
		trap_Cvar_Set( "cg_selectedPlayer", va( "%d", playerTeamNumber ) );
	}

	n = trap_Cvar_VariableValue( "cg_selectedPlayer" );
	if ( n < 0 || n > uiInfo.myTeamCount ) {
		n = 0;
	}
	if ( n < uiInfo.myTeamCount ) {
		trap_Cvar_Set( "cg_selectedPlayerName", uiInfo.teamNames[n] );
	}
}


static void UI_DrawSelectedPlayer( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh ) {
		uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
		UI_BuildPlayerList();
	}
	Text_Paint( rect->x, rect->y, font, scale, color, ( uiInfo.teamLeader ) ? UI_Cvar_VariableString( "cg_selectedPlayerName" ) : UI_Cvar_VariableString( "name" ), 0, 0, textStyle );
}

static void UI_DrawServerRefreshDate( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
#ifdef MISSIONPACK
	if ( uiInfo.serverStatus.refreshActive ) {
		vec4_t lowLight, newColor;
		lowLight[0] = 0.8 * color[0];
		lowLight[1] = 0.8 * color[1];
		lowLight[2] = 0.8 * color[2];
		lowLight[3] = 0.8 * color[3];
		LerpColor( color,lowLight,newColor,0.5 + 0.5 * sin( uiInfo.uiDC.realTime / PULSE_DIVISOR ) );
		Text_Paint( rect->x, rect->y, font, scale, newColor, va( "Getting info for %d servers (ESC to cancel)", trap_LAN_GetServerCount( ui_netSource.integer ) ), 0, 0, textStyle );
	} else {
		char buff[64];
		Q_strncpyz( buff, UI_Cvar_VariableString( va( "ui_lastServerRefresh_%i", ui_netSource.integer ) ), 64 );
		Text_Paint( rect->x, rect->y, font, scale, color, va( "Refresh Time: %s", buff ), 0, 0, textStyle );
	}
#endif  // #ifdef MISSIONPACK
}

static void UI_DrawServerMOTD( rectDef_t *rect, int font, float scale, vec4_t color ) {
#ifdef MISSIONPACK
	if ( uiInfo.serverStatus.motdLen ) {
		float maxX;

		if ( uiInfo.serverStatus.motdWidth == -1 ) {
			uiInfo.serverStatus.motdWidth = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if ( uiInfo.serverStatus.motdOffset > uiInfo.serverStatus.motdLen ) {
			uiInfo.serverStatus.motdOffset = 0;
			uiInfo.serverStatus.motdPaintX = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if ( uiInfo.uiDC.realTime > uiInfo.serverStatus.motdTime ) {
			uiInfo.serverStatus.motdTime = uiInfo.uiDC.realTime + 10;
			if ( uiInfo.serverStatus.motdPaintX <= rect->x + 2 ) {
				if ( uiInfo.serverStatus.motdOffset < uiInfo.serverStatus.motdLen ) {
					uiInfo.serverStatus.motdPaintX += Text_Width( &uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], font, scale, 1 ) - 1;
					uiInfo.serverStatus.motdOffset++;
				} else {
					uiInfo.serverStatus.motdOffset = 0;
					if ( uiInfo.serverStatus.motdPaintX2 >= 0 ) {
						uiInfo.serverStatus.motdPaintX = uiInfo.serverStatus.motdPaintX2;
					} else {
						uiInfo.serverStatus.motdPaintX = rect->x + rect->w - 2;
					}
					uiInfo.serverStatus.motdPaintX2 = -1;
				}
			} else {
				//serverStatus.motdPaintX--;
				uiInfo.serverStatus.motdPaintX -= 2;
				if ( uiInfo.serverStatus.motdPaintX2 >= 0 ) {
					//serverStatus.motdPaintX2--;
					uiInfo.serverStatus.motdPaintX2 -= 2;
				}
			}
		}

		maxX = rect->x + rect->w - 2;
		Text_Paint_Limit( &maxX, uiInfo.serverStatus.motdPaintX, rect->y + rect->h - 3, font, scale, color, &uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], 0, 0 );
		if ( uiInfo.serverStatus.motdPaintX2 >= 0 ) {
			float maxX2 = rect->x + rect->w - 2;
			Text_Paint_Limit( &maxX2, uiInfo.serverStatus.motdPaintX2, rect->y + rect->h - 3, font, scale, color, uiInfo.serverStatus.motd, 0, uiInfo.serverStatus.motdOffset );
		}
		if ( uiInfo.serverStatus.motdOffset && maxX > 0 ) {
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if ( uiInfo.serverStatus.motdPaintX2 == -1 ) {
				uiInfo.serverStatus.motdPaintX2 = rect->x + rect->w - 2;
			}
		} else {
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

	}
#endif  // #ifdef MISSIONPACK
}

static void UI_DrawKeyBindStatus( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	//int ofs = 0; // TTimo: unused
	if ( Display_KeyBindPending() ) {
//		Text_Paint(rect->x, rect->y, font, scale, color, "Waiting for new key... Press ESCAPE to cancel", 0, 0, textStyle);
		Text_Paint( rect->x, rect->y, font, scale, color, DC->getTranslatedString( "keywait" ), 0, 0, textStyle );
	} else {
//		Text_Paint(rect->x, rect->y, font, scale, color, "Press ENTER or CLICK to change, Press BACKSPACE to clear", 0, 0, textStyle);
		Text_Paint( rect->x, rect->y, font, scale, color, DC->getTranslatedString( "keychange" ), 0, 0, textStyle );

	}
}

static void UI_DrawGLInfo( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	char * eptr;
	char buff[4096];
	const char *lines[64];
	int y, numLines, i;

	Text_Paint( rect->x + 2, rect->y, font, scale, color, va( "VENDOR: %s", uiInfo.uiDC.glconfig.vendor_string ), 0, 30, textStyle );
	Text_Paint( rect->x + 2, rect->y + 15, font, scale, color, va( "VERSION: %s: %s", uiInfo.uiDC.glconfig.version_string,uiInfo.uiDC.glconfig.renderer_string ), 0, 30, textStyle );
	Text_Paint( rect->x + 2, rect->y + 30, font, scale, color, va( "PIXELFORMAT: color(%d-bits) Z(%d-bits) stencil(%d-bits)", uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits ), 0, 30, textStyle );

	// build null terminated extension strings
	Q_strncpyz( buff, uiInfo.uiDC.glconfig.extensions_string, 4096 );
	eptr = buff;
	y = rect->y + 45;
	numLines = 0;
	while ( y < rect->y + rect->h && *eptr )
	{
		while ( *eptr && *eptr == ' ' )
			*eptr++ = '\0';

		// track start of valid string
		if ( *eptr && *eptr != ' ' ) {
			lines[numLines++] = eptr;
		}

		while ( *eptr && *eptr != ' ' )
			eptr++;
	}

	i = 0;
	while ( i < numLines ) {
		Text_Paint( rect->x + 2, y, font, scale, color, lines[i++], 0, 36, textStyle );
		if ( i < numLines ) {
			Text_Paint( rect->x + rect->w / 3.0f, y, font, scale, color, lines[i++], 0, 36, textStyle );
		}
		if ( i < numLines ) {
			Text_Paint( rect->x + ( 2.0f * ( rect->w / 3.0f ) ), y, font, scale, color, lines[i++], 0, 36, textStyle );
		}
		y += 10;
		if ( y > rect->y + rect->h - 11 ) {
			break;
		}
	}

//	i = 0;
//	while (i < numLines) {
//		Text_Paint(rect->x + 2, y, font, scale, color, lines[i++], 0, 20, textStyle);
//		if (i < numLines) {
//			Text_Paint(rect->x + rect->w / 2, y, font, scale, color, lines[i++], 0, 20, textStyle);
//		}
//		y += 10;
//		if (y > rect->y + rect->h - 11) {
//			break;
//		}
//	}


}

// NERVE - SMF
static void UI_DrawLimboChat( rectDef_t *rect, int font, float scale, vec4_t color, int textStyle ) {
	int i, count;
	char buf[140];
	float x, y, w, h;

	memset( buf, 0, 140 );

	// first count strings
	for ( i = 0, count = 0; ; i++, count++ ) {
		if ( !trap_GetLimboString( i, buf ) ) {
			break;
		}
	}

	// now draw strings
	for ( i = 0; i < count; i++ ) {
		x = rect->x;
		y = rect->y + 9 * i;
		w = 1;
		h = 1;

		trap_GetLimboString( i, buf );
		Text_Paint( x, y, font, scale, color, buf, 0, 0, textStyle );
	}
}
// -NERVE - SMF

// FIXME: table drive
//
static void UI_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, int font, float scale, vec4_t color, qhandle_t shader, int textStyle ) {
	rectDef_t rect;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw ) {
	case UI_HANDICAP:
		UI_DrawHandicap( &rect, font, scale, color, textStyle );
		break;
	case UI_EFFECTS:
		UI_DrawEffects( &rect, scale, color );
		break;
	case UI_PLAYERMODEL:
		UI_DrawPlayerModel( &rect );
		break;
	case UI_CLANNAME:
		UI_DrawClanName( &rect, font, scale, color, textStyle );
		break;


	case UI_SAVEGAME_SHOT:  // (SA)
		UI_DrawSaveGameShot( &rect, scale, color );
		break;

	case UI_CLANLOGO:
		UI_DrawClanLogo( &rect, scale, color );
		break;
	case UI_CLANCINEMATIC:
		UI_DrawClanCinematic( &rect, scale, color );
		break;
	case UI_STARTMAPCINEMATIC:
		UI_DrawPregameCinematic( &rect, scale, color );
		break;
	case UI_PREVIEWCINEMATIC:
		UI_DrawPreviewCinematic( &rect, scale, color );
		break;
	case UI_GAMETYPE:
		UI_DrawGameType( &rect, font, scale, color, textStyle );
		break;
	case UI_NETGAMETYPE:
		UI_DrawNetGameType( &rect, font, scale, color, textStyle );
		break;
	case UI_JOINGAMETYPE:
		UI_DrawJoinGameType( &rect, font, scale, color, textStyle );
		break;
	case UI_MAPPREVIEW:
		UI_DrawMapPreview( &rect, scale, color, qtrue );
		break;

//----(SA)	added
	case UI_SAVEGAMENAME:
		UI_DrawSavegameName( &rect, font, scale, color, textStyle );
		break;
	case UI_SAVEGAMEINFO:
		break;
	case UI_LEVELSHOT:
		UI_DrawMapLevelshot( &rect );
		break;
	case UI_LOADSTATUSBAR:
		UI_DrawLoadStatus( &rect, color, align );
		break;
//----(SA)	end

	case UI_MAP_TIMETOBEAT:
		UI_DrawMapTimeToBeat( &rect, font, scale, color, textStyle );
		break;
	case UI_MAPCINEMATIC:
		UI_DrawMapCinematic( &rect, scale, color, qfalse );
		break;
	case UI_SKILL:
		UI_DrawSkill( &rect, font, scale, color, textStyle );
		break;
	case UI_BLUETEAMNAME:
		UI_DrawTeamName( &rect, font, scale, color, qtrue, textStyle );
		break;
	case UI_REDTEAMNAME:
		UI_DrawTeamName( &rect, font, scale, color, qfalse, textStyle );
		break;
	case UI_BLUETEAM1:
	case UI_BLUETEAM2:
	case UI_BLUETEAM3:
	case UI_BLUETEAM4:
	case UI_BLUETEAM5:
		UI_DrawTeamMember( &rect, font, scale, color, qtrue, ownerDraw - UI_BLUETEAM1 + 1, textStyle );
		break;
	case UI_REDTEAM1:
	case UI_REDTEAM2:
	case UI_REDTEAM3:
	case UI_REDTEAM4:
	case UI_REDTEAM5:
		UI_DrawTeamMember( &rect, font, scale, color, qfalse, ownerDraw - UI_REDTEAM1 + 1, textStyle );
		break;
	case UI_NETSOURCE:
		UI_DrawNetSource( &rect, font, scale, color, textStyle );
		break;
	case UI_NETMAPPREVIEW:
		UI_DrawNetMapPreview( &rect, scale, color );
		break;
	case UI_NETMAPCINEMATIC:
		UI_DrawNetMapCinematic( &rect, scale, color );
		break;
	case UI_NETFILTER:
		UI_DrawNetFilter( &rect, font, scale, color, textStyle );
		break;
	case UI_TIER:
		UI_DrawTier( &rect, font, scale, color, textStyle );
		break;
	case UI_OPPONENTMODEL:
		UI_DrawOpponent( &rect );
		break;
	case UI_TIERMAP1:
		UI_DrawTierMap( &rect, 0 );
		break;
	case UI_TIERMAP2:
		UI_DrawTierMap( &rect, 1 );
		break;
	case UI_TIERMAP3:
		UI_DrawTierMap( &rect, 2 );
		break;
	case UI_PLAYERLOGO:
		UI_DrawPlayerLogo( &rect, color );
		break;
	case UI_PLAYERLOGO_METAL:
		UI_DrawPlayerLogoMetal( &rect, color );
		break;
	case UI_PLAYERLOGO_NAME:
		UI_DrawPlayerLogoName( &rect, color );
		break;
	case UI_OPPONENTLOGO:
		UI_DrawOpponentLogo( &rect, color );
		break;
	case UI_OPPONENTLOGO_METAL:
		UI_DrawOpponentLogoMetal( &rect, color );
		break;
	case UI_OPPONENTLOGO_NAME:
		UI_DrawOpponentLogoName( &rect, color );
		break;
	case UI_TIER_MAPNAME:
		UI_DrawTierMapName( &rect, font, scale, color, textStyle );
		break;
	case UI_TIER_GAMETYPE:
		UI_DrawTierGameType( &rect, font, scale, color, textStyle );
		break;
	case UI_ALLMAPS_SELECTION:
		UI_DrawAllMapsSelection( &rect, font, scale, color, textStyle, qtrue );
		break;
	case UI_MAPS_SELECTION:
		UI_DrawAllMapsSelection( &rect, font, scale, color, textStyle, qfalse );
		break;
	case UI_OPPONENT_NAME:
		UI_DrawOpponentName( &rect, font, scale, color, textStyle );
		break;
	case UI_BOTNAME:
		UI_DrawBotName( &rect, font, scale, color, textStyle );
		break;
	case UI_BOTSKILL:
		UI_DrawBotSkill( &rect, font, scale, color, textStyle );
		break;
	case UI_REDBLUE:
		UI_DrawRedBlue( &rect, font, scale, color, textStyle );
		break;
	case UI_CROSSHAIR:
		UI_DrawCrosshair( &rect, scale, color );
		break;
	case UI_SELECTEDPLAYER:
		UI_DrawSelectedPlayer( &rect, font, scale, color, textStyle );
		break;
	case UI_SERVERREFRESHDATE:
		UI_DrawServerRefreshDate( &rect, font, scale, color, textStyle );
		break;
	case UI_SERVERMOTD:
		UI_DrawServerMOTD( &rect, font, scale, color );
		break;
	case UI_GLINFO:
		UI_DrawGLInfo( &rect, font, scale, color, textStyle );
		break;
	case UI_KEYBINDSTATUS:
		UI_DrawKeyBindStatus( &rect, font, scale, color, textStyle );
		break;
		// NERVE - SMF
	case UI_LIMBOCHAT:
		UI_DrawLimboChat( &rect, font, scale, color, textStyle );
		break;
		// -NERVE - SMF

	default:
		break;
	}
}

static qboolean UI_OwnerDrawVisible( int flags ) {
	qboolean vis = qtrue;

	while ( flags ) {

		if ( flags & UI_SHOW_FFA ) {
			if ( trap_Cvar_VariableValue( "g_gametype" ) != GT_FFA ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FFA;
		}

		if ( flags & UI_SHOW_NOTFFA ) {
			if ( trap_Cvar_VariableValue( "g_gametype" ) == GT_FFA ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFFA;
		}

		if ( flags & UI_SHOW_LEADER ) {
			// these need to show when this client can give orders to a player or a group
			if ( !uiInfo.teamLeader ) {
				vis = qfalse;
			} else {
				// if showing yourself
				if ( ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber ) {
					vis = qfalse;
				}
			}
			flags &= ~UI_SHOW_LEADER;
		}
		if ( flags & UI_SHOW_NOTLEADER ) {
			// these need to show when this client is assigning their own status or they are NOT the leader
			if ( uiInfo.teamLeader ) {
				// if not showing yourself
				if ( !( ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber ) ) {
					vis = qfalse;
				}
			}
			flags &= ~UI_SHOW_NOTLEADER;
		}
		if ( flags & UI_SHOW_FAVORITESERVERS ) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if ( ui_netSource.integer != AS_FAVORITES ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FAVORITESERVERS;
		}
		if ( flags & UI_SHOW_NOTFAVORITESERVERS ) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if ( ui_netSource.integer == AS_FAVORITES ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		}
		if ( flags & UI_SHOW_ANYTEAMGAME ) {
			if ( uiInfo.gameTypes[ui_gameType.integer].gtEnum <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYTEAMGAME;
		}
		if ( flags & UI_SHOW_ANYNONTEAMGAME ) {
			if ( uiInfo.gameTypes[ui_gameType.integer].gtEnum > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_ANYNONTEAMGAME;
		}
		if ( flags & UI_SHOW_NETANYTEAMGAME ) {
			if ( uiInfo.gameTypes[ui_netGameType.integer].gtEnum <= GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYTEAMGAME;
		}
		if ( flags & UI_SHOW_NETANYNONTEAMGAME ) {
			if ( uiInfo.gameTypes[ui_netGameType.integer].gtEnum > GT_TEAM ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NETANYNONTEAMGAME;
		}
		if ( flags & UI_SHOW_NEWHIGHSCORE ) {
			if ( uiInfo.newHighScoreTime < uiInfo.uiDC.realTime ) {
				vis = qfalse;
			} else {
				if ( uiInfo.soundHighScore ) {
					if ( trap_Cvar_VariableValue( "sv_killserver" ) == 0 ) {
						// wait on server to go down before playing sound
						trap_S_StartLocalSound( uiInfo.newHighScoreSound, CHAN_ANNOUNCER );
						uiInfo.soundHighScore = qfalse;
					}
				}
			}
			flags &= ~UI_SHOW_NEWHIGHSCORE;
		}
		if ( flags & UI_SHOW_NEWBESTTIME ) {
			if ( uiInfo.newBestTime < uiInfo.uiDC.realTime ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NEWBESTTIME;
		}
		if ( flags & UI_SHOW_DEMOAVAILABLE ) {
			if ( !uiInfo.demoAvailable ) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_DEMOAVAILABLE;
		} else {
			flags = 0;
		}
	}
	return vis;
}

static qboolean UI_Handicap_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		int h;
		h = Com_Clamp( 5, 100, trap_Cvar_VariableValue( "handicap" ) );
		if ( key == K_MOUSE2 ) {
			h -= 5;
		} else {
			h += 5;
		}
		if ( h > 100 ) {
			h = 5;
		} else if ( h < 0 ) {
			h = 100;
		}
		trap_Cvar_Set( "handicap", va( "%i", h ) );
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Effects_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {

		if ( key == K_MOUSE2 ) {
			uiInfo.effectsColor--;
		} else {
			uiInfo.effectsColor++;
		}

		if ( uiInfo.effectsColor > 6 ) {
			uiInfo.effectsColor = 0;
		} else if ( uiInfo.effectsColor < 0 ) {
			uiInfo.effectsColor = 6;
		}

		trap_Cvar_SetValue( "color", uitogamecode[uiInfo.effectsColor] );
		return qtrue;
	}
	return qfalse;
}


//----(SA)	added
static qboolean UI_SavegameName_HandleKey( int flags, float *special, int key ) {

	// disable
	return qfalse;

	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		int i;
		i = UI_SavegameIndexFromName( ui_savegameName.string );

		if ( key == K_MOUSE2 ) {
			i--;
		} else {
			i++;
		}
		if ( i >= uiInfo.savegameCount ) {
			i = 0;
		} else if ( i < 0 ) {
			i = uiInfo.savegameCount - 1;
		}

		// set feeder highlight
		Menu_SetFeederSelection( NULL, FEEDER_SAVEGAMES, i, NULL );

		trap_Cvar_Set( "ui_savegameName", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[i]].savegameName );
		trap_Cvar_Set( "ui_savegameInfo", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[i]].savegameInfoText );
		return qtrue;
	}
	return qfalse;
}
//----(SA)	end


static qboolean UI_ClanName_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		int i;
		i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
		if ( uiInfo.teamList[i].cinematic >= 0 ) {
			trap_CIN_StopCinematic( uiInfo.teamList[i].cinematic );
			uiInfo.teamList[i].cinematic = -1;
		}
		if ( key == K_MOUSE2 ) {
			i--;
		} else {
			i++;
		}
		if ( i >= uiInfo.teamCount ) {
			i = 0;
		} else if ( i < 0 ) {
			i = uiInfo.teamCount - 1;
		}
		trap_Cvar_Set( "ui_teamName", uiInfo.teamList[i].teamName );
		updateModel = qtrue;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_GameType_HandleKey( int flags, float *special, int key, qboolean resetMap ) {
#ifdef MISSIONPACK
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		int oldCount = UI_MapCountByGameType( qtrue );

		// hard coded mess here
		if ( key == K_MOUSE2 ) {
			ui_gameType.integer--;
			if ( ui_gameType.integer == 2 ) {
				ui_gameType.integer = 1;
			} else if ( ui_gameType.integer < 2 ) {
				ui_gameType.integer = uiInfo.numGameTypes - 1;
			}
		} else {
			ui_gameType.integer++;
			if ( ui_gameType.integer >= uiInfo.numGameTypes ) {
				ui_gameType.integer = 1;
			} else if ( ui_gameType.integer == 2 ) {
				ui_gameType.integer = 3;
			}
		}

		if ( uiInfo.gameTypes[ui_gameType.integer].gtEnum == GT_TOURNAMENT ) {
			trap_Cvar_Set( "ui_Q3Model", "1" );
		} else {
			trap_Cvar_Set( "ui_Q3Model", "0" );
		}

		trap_Cvar_Set( "ui_gameType", va( "%d", ui_gameType.integer ) );
		UI_SetCapFragLimits( qtrue );
		UI_LoadBestScores( uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum );
		if ( resetMap && oldCount != UI_MapCountByGameType( qtrue ) ) {
			trap_Cvar_Set( "ui_currentMap", "0" );
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, 0, NULL );
		}
		return qtrue;
	}
#endif  // #ifdef MISSIONPACK
	return qfalse;
}

static qboolean UI_NetGameType_HandleKey( int flags, float *special, int key ) {
#ifdef MISSIONPACK
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {

		if ( key == K_MOUSE2 ) {
			ui_netGameType.integer--;
		} else {
			ui_netGameType.integer++;
		}

		if ( ui_netGameType.integer < 0 ) {
			ui_netGameType.integer = uiInfo.numGameTypes - 1;
		} else if ( ui_netGameType.integer >= uiInfo.numGameTypes ) {
			ui_netGameType.integer = 0;
		}

		trap_Cvar_Set( "ui_netGameType", va( "%d", ui_netGameType.integer ) );
		trap_Cvar_Set( "ui_actualnetGameType", va( "%d", uiInfo.gameTypes[ui_netGameType.integer].gtEnum ) );
		trap_Cvar_Set( "ui_currentNetMap", "0" );
		UI_MapCountByGameType( qfalse );
		Menu_SetFeederSelection( NULL, FEEDER_ALLMAPS, 0, NULL );
		return qtrue;
	}
#endif  // #ifdef MISSIONPACK
	return qfalse;
}

static qboolean UI_JoinGameType_HandleKey( int flags, float *special, int key ) {
#ifdef MISSIONPACK
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {

		if ( key == K_MOUSE2 ) {
			ui_joinGameType.integer--;
		} else {
			ui_joinGameType.integer++;
		}

		if ( ui_joinGameType.integer < 0 ) {
			ui_joinGameType.integer = uiInfo.numJoinGameTypes - 1;
		} else if ( ui_joinGameType.integer >= uiInfo.numJoinGameTypes ) {
			ui_joinGameType.integer = 0;
		}

		trap_Cvar_Set( "ui_joinGameType", va( "%d", ui_joinGameType.integer ) );
		UI_BuildServerDisplayList( qtrue );
		return qtrue;
	}
#endif  // #ifdef MISSIONPACK
	return qfalse;
}



static qboolean UI_Skill_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		int i = trap_Cvar_VariableValue( "g_spSkill" );

		if ( key == K_MOUSE2 ) {
			i--;
		} else {
			i++;
		}

		if ( i < 1 ) {
			i = numSkillLevels;
		} else if ( i > numSkillLevels ) {
			i = 1;
		}

		trap_Cvar_Set( "g_spSkill", va( "%i", i ) );
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_TeamName_HandleKey( int flags, float *special, int key, qboolean blue ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		int i;
		i = UI_TeamIndexFromName( UI_Cvar_VariableString( ( blue ) ? "ui_blueTeam" : "ui_redTeam" ) );

		if ( key == K_MOUSE2 ) {
			i--;
		} else {
			i++;
		}

		if ( i >= uiInfo.teamCount ) {
			i = 0;
		} else if ( i < 0 ) {
			i = uiInfo.teamCount - 1;
		}

		trap_Cvar_Set( ( blue ) ? "ui_blueTeam" : "ui_redTeam", uiInfo.teamList[i].teamName );

		return qtrue;
	}
	return qfalse;
}

static qboolean UI_TeamMember_HandleKey( int flags, float *special, int key, qboolean blue, int num ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		// 0 - None
		// 1 - Human
		// 2..NumCharacters - Bot
		char *cvar = va( blue ? "ui_blueteam%i" : "ui_redteam%i", num );
		int value = trap_Cvar_VariableValue( cvar );

		if ( key == K_MOUSE2 ) {
			value--;
		} else {
			value++;
		}

		if ( ui_actualNetGameType.integer >= GT_TEAM ) {
			if ( value >= uiInfo.characterCount + 2 ) {
				value = 0;
			} else if ( value < 0 ) {
				value = uiInfo.characterCount + 2 - 1;
			}
		} else {
			if ( value >= UI_GetNumBots() + 2 ) {
				value = 0;
			} else if ( value < 0 ) {
				value = UI_GetNumBots() + 2 - 1;
			}
		}

		trap_Cvar_Set( cvar, va( "%i", value ) );
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_NetSource_HandleKey( int flags, float *special, int key ) {
#ifdef MISSIONPACK
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {

		if ( key == K_MOUSE2 ) {
			ui_netSource.integer--;
		} else {
			ui_netSource.integer++;
		}

		if ( ui_netSource.integer >= numNetSources ) {
			ui_netSource.integer = 0;
		} else if ( ui_netSource.integer < 0 ) {
			ui_netSource.integer = numNetSources - 1;
		}

		UI_BuildServerDisplayList( qtrue );
		if ( ui_netSource.integer != AS_GLOBAL ) {
			UI_StartServerRefresh( qtrue );
		}
		trap_Cvar_Set( "ui_netSource", va( "%d", ui_netSource.integer ) );
		return qtrue;
	}
#endif  // #ifdef MISSIONPACK
	return qfalse;
}

static qboolean UI_NetFilter_HandleKey( int flags, float *special, int key ) {
#ifdef MISSIONPACK
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {

		if ( key == K_MOUSE2 ) {
			ui_serverFilterType.integer--;
		} else {
			ui_serverFilterType.integer++;
		}

		if ( ui_serverFilterType.integer >= numServerFilters ) {
			ui_serverFilterType.integer = 0;
		} else if ( ui_serverFilterType.integer < 0 ) {
			ui_serverFilterType.integer = numServerFilters - 1;
		}
		UI_BuildServerDisplayList( qtrue );
		return qtrue;
	}
#endif  // #ifdef MISSIONPACK
	return qfalse;
}

static qboolean UI_OpponentName_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		if ( key == K_MOUSE2 ) {
			UI_PriorOpponent();
		} else {
			UI_NextOpponent();
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_BotName_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		int game = trap_Cvar_VariableValue( "g_gametype" );
		int value = uiInfo.botIndex;

		if ( key == K_MOUSE2 ) {
			value--;
		} else {
			value++;
		}

		if ( game >= GT_TEAM ) {
			if ( value >= uiInfo.characterCount + 2 ) {
				value = 0;
			} else if ( value < 0 ) {
				value = uiInfo.characterCount + 2 - 1;
			}
		} else {
			if ( value >= UI_GetNumBots() + 2 ) {
				value = 0;
			} else if ( value < 0 ) {
				value = UI_GetNumBots() + 2 - 1;
			}
		}
		uiInfo.botIndex = value;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_BotSkill_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		if ( key == K_MOUSE2 ) {
			uiInfo.skillIndex--;
		} else {
			uiInfo.skillIndex++;
		}
		if ( uiInfo.skillIndex >= numSkillLevels ) {
			uiInfo.skillIndex = 0;
		} else if ( uiInfo.skillIndex < 0 ) {
			uiInfo.skillIndex = numSkillLevels - 1;
		}
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_RedBlue_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		uiInfo.redBlue ^= 1;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Crosshair_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		if ( key == K_MOUSE2 ) {
			uiInfo.currentCrosshair--;
		} else {
			uiInfo.currentCrosshair++;
		}

		if ( uiInfo.currentCrosshair >= NUM_CROSSHAIRS ) {
			uiInfo.currentCrosshair = 0;
		} else if ( uiInfo.currentCrosshair < 0 ) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		trap_Cvar_Set( "cg_drawCrosshair", va( "%d", uiInfo.currentCrosshair ) );
		return qtrue;
	}
	return qfalse;
}



static qboolean UI_SelectedPlayer_HandleKey( int flags, float *special, int key ) {
	if ( key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER ) {
		int selected;

		UI_BuildPlayerList();
		if ( !uiInfo.teamLeader ) {
			return qfalse;
		}
		selected = trap_Cvar_VariableValue( "cg_selectedPlayer" );

		if ( key == K_MOUSE2 ) {
			selected--;
		} else {
			selected++;
		}

		if ( selected > uiInfo.myTeamCount ) {
			selected = 0;
		} else if ( selected < 0 ) {
			selected = uiInfo.myTeamCount;
		}

		if ( selected == uiInfo.myTeamCount ) {
			trap_Cvar_Set( "cg_selectedPlayerName", "Everyone" );
		} else {
			trap_Cvar_Set( "cg_selectedPlayerName", uiInfo.teamNames[selected] );
		}
		trap_Cvar_Set( "cg_selectedPlayer", va( "%d", selected ) );
	}
	return qfalse;
}


static qboolean UI_OwnerDrawHandleKey( int ownerDraw, int flags, float *special, int key ) {
	switch ( ownerDraw ) {
	case UI_HANDICAP:
		return UI_Handicap_HandleKey( flags, special, key );
		break;
	case UI_EFFECTS:
		return UI_Effects_HandleKey( flags, special, key );
		break;
//----(SA)	added
	case UI_SAVEGAMENAME:
	case UI_SAVEGAMEINFO:
		return UI_SavegameName_HandleKey( flags, special, key );

//----(SA)	end
	case UI_CLANNAME:
		return UI_ClanName_HandleKey( flags, special, key );
		break;
	case UI_GAMETYPE:
		return UI_GameType_HandleKey( flags, special, key, qtrue );
		break;
	case UI_NETGAMETYPE:
		return UI_NetGameType_HandleKey( flags, special, key );
		break;
	case UI_JOINGAMETYPE:
		return UI_JoinGameType_HandleKey( flags, special, key );
		break;
	case UI_SKILL:
		return UI_Skill_HandleKey( flags, special, key );
		break;
	case UI_BLUETEAMNAME:
		return UI_TeamName_HandleKey( flags, special, key, qtrue );
		break;
	case UI_REDTEAMNAME:
		return UI_TeamName_HandleKey( flags, special, key, qfalse );
		break;
	case UI_BLUETEAM1:
	case UI_BLUETEAM2:
	case UI_BLUETEAM3:
	case UI_BLUETEAM4:
	case UI_BLUETEAM5:
		UI_TeamMember_HandleKey( flags, special, key, qtrue, ownerDraw - UI_BLUETEAM1 + 1 );
		break;
	case UI_REDTEAM1:
	case UI_REDTEAM2:
	case UI_REDTEAM3:
	case UI_REDTEAM4:
	case UI_REDTEAM5:
		UI_TeamMember_HandleKey( flags, special, key, qfalse, ownerDraw - UI_REDTEAM1 + 1 );
		break;
	case UI_NETSOURCE:
		UI_NetSource_HandleKey( flags, special, key );
		break;
	case UI_NETFILTER:
		UI_NetFilter_HandleKey( flags, special, key );
		break;
	case UI_OPPONENT_NAME:
		UI_OpponentName_HandleKey( flags, special, key );
		break;
	case UI_BOTNAME:
		return UI_BotName_HandleKey( flags, special, key );
		break;
	case UI_BOTSKILL:
		return UI_BotSkill_HandleKey( flags, special, key );
		break;
	case UI_REDBLUE:
		UI_RedBlue_HandleKey( flags, special, key );
		break;
	case UI_CROSSHAIR:
		UI_Crosshair_HandleKey( flags, special, key );
		break;
	case UI_SELECTEDPLAYER:
		UI_SelectedPlayer_HandleKey( flags, special, key );
		break;
	default:
		break;
	}

	return qfalse;
}


static float UI_GetValue( int ownerDraw, int type ) {
	return 0;
}

/*
=================
UI_ServersQsortCompare
=================
*/
static int QDECL UI_ServersQsortCompare( const void *arg1, const void *arg2 ) {
#ifdef MISSIONPACK
	return trap_LAN_CompareServers( ui_netSource.integer, uiInfo.serverStatus.sortKey, uiInfo.serverStatus.sortDir, *(int*)arg1, *(int*)arg2 );
#else
	return qfalse;
#endif  // #ifdef MISSIONPACK
}


/*
=================
UI_ServersSort
=================
*/
void UI_ServersSort( int column, qboolean force ) {

	if ( !force ) {
		if ( uiInfo.serverStatus.sortKey == column ) {
			return;
		}
	}

	uiInfo.serverStatus.sortKey = column;
	qsort( &uiInfo.serverStatus.displayServers[0], uiInfo.serverStatus.numDisplayServers, sizeof( int ), UI_ServersQsortCompare );
}


//----(SA)	added


/*
==============
UI_SavegamesQsortCompare
==============
*/
static int QDECL UI_SavegamesQsortCompare( const void *arg1, const void *arg2 ) {
	int *ea, *eb, ret;

	savegameInfo *sg, *sg2;
	int i, j;

	ea = (int *)arg1;
	eb = (int *)arg2;

	if ( *ea == *eb ) {
		return 0;
	}

	sg = &uiInfo.savegameList[*eb];
	sg2 = &uiInfo.savegameList[*ea];

	if ( uiInfo.savegameStatus.sortKey == SORT_SAVENAME ) {
		ret = Q_stricmp( &sg->savegameName[0], &sg2->savegameName[0] );

	} else if ( uiInfo.savegameStatus.sortKey == SORT_SAVETIME ) {

// (SA) better way to do this?  (i was adding up seconds, but that seems slower than a bunch of comparisons)
		i = sg->tm.tm_year;
		j = sg2->tm.tm_year;
		if ( i < j ) {
			ret = -1;
		} else if ( i > j ) {
			ret = 1;
		} else {
			i = sg->tm.tm_yday;
			j = sg2->tm.tm_yday;
			if ( i < j ) {
				ret = -1;
			} else if ( i > j ) {
				ret = 1;
			} else {
				i = sg->tm.tm_hour;
				j = sg2->tm.tm_hour;
				if ( i < j ) {
					ret = -1;
				} else if ( i > j ) {
					ret = 1;
				} else {
					i = sg->tm.tm_min;
					j = sg2->tm.tm_min;
					if ( i < j ) {
						ret = -1;
					} else if ( i > j ) {
						ret = 1;
					} else {
						i = sg->tm.tm_sec;
						j = sg2->tm.tm_sec;
						if ( i < j ) {
							ret = -1;
						} else if ( i > j ) {
							ret = 1;
						} else { ret = 0;}
					}
				}
			}
		}
	} else {
		ret = 0;
	}

	if ( uiInfo.savegameStatus.sortDir ) {
		return ret;
	} else {
		return -ret;
	}
}

/*
==============
UI_SavegameSort
==============
*/
void UI_SavegameSort( int column, qboolean force ) {
	int cursel;

	if ( !force ) {
		if ( uiInfo.savegameStatus.sortKey == column ) {
			return;
		}
	}
	uiInfo.savegameStatus.sortKey = column;

	if ( uiInfo.savegameCount ) {
		qsort( &uiInfo.savegameStatus.displaySavegames[0], uiInfo.savegameCount, sizeof( int ), UI_SavegamesQsortCompare );

		// re-select the one that was selected before sorting
		cursel = UI_SavegameIndexFromName( ui_savegameName.string );
		UI_FeederSelection( FEEDER_SAVEGAMES, cursel );
		Menu_SetFeederSelection( NULL, FEEDER_SAVEGAMES, cursel, NULL );

		// and clear out the text entry
		trap_Cvar_Set( "ui_savegame", "" );

	} else {
		trap_Cvar_Set( "ui_savegameName", "" );
		trap_Cvar_Set( "ui_savegameInfo", "(no savegames)" );
	}

}
//----(SA)	end


/*
===============
UI_LoadMods
===============
*/
static void UI_LoadMods() {
	int numdirs;
	char dirlist[2048];
	char    *dirptr;
	char  *descptr;
	int i;
	int dirlen;

	uiInfo.modCount = 0;
	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof( dirlist ) );
	dirptr  = dirlist;
	for ( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
		descptr = dirptr + dirlen;
		uiInfo.modList[uiInfo.modCount].modName = String_Alloc( dirptr );
		uiInfo.modList[uiInfo.modCount].modDescr = String_Alloc( descptr );
		dirptr += dirlen + strlen( descptr ) + 1;
		uiInfo.modCount++;
		if ( uiInfo.modCount >= MAX_MODS ) {
			break;
		}
	}

}


/*
===============
UI_LoadTeams
===============
*/
// TTimo: unused
/*
static void UI_LoadTeams() {
	char	teamList[4096];
	char	*teamName;
	int		i, len, count;

	count = trap_FS_GetFileList( "", "team", teamList, 4096 );

	if (count) {
		teamName = teamList;
		for ( i = 0; i < count; i++ ) {
			len = strlen( teamName );
			UI_ParseTeamInfo(teamName);
			teamName += len + 1;
		}
	}

}
*/

/*
==============
UI_DelSavegame
==============
*/
static void UI_DelSavegame() {
	int ret, i;

	i = UI_SavegameIndexFromName2( ui_savegameName.string );

	ret = trap_FS_Delete( va( "save/%s.svg", uiInfo.savegameList[i].savegameFile ) );

	if ( ret ) {
		Com_Printf( "Deleted savegame: %s.svg\n", uiInfo.savegameList[i].savegameName );
	} else {
		Com_Printf( "Unable to delete savegame: %s.svg\n", uiInfo.savegameList[i].savegameName );
	}


	UI_SavegameSort( uiInfo.savegameStatus.sortKey, qtrue );  // re-sort
}





#define SAVE_INFOSTRING_LENGTH  256     // defined in g_save.c


/*
==============
UI_ParseSavegame
==============
*/

static char *monthStr[12] =
{
	"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

/*
==============
UI_ParseSavegame
==============
*/
void UI_ParseSavegame( int index ) {
	fileHandle_t f;
	qtime_t         *tm;
	int i, ver;
	static char buf[SAVE_INFOSTRING_LENGTH];
	char mapname[MAX_QPATH];

	trap_FS_FOpenFile( va( "save/%s.svg", uiInfo.savegameList[index].savegameFile ), &f, FS_READ );
	if ( !f ) {
		return;
	}

	// read the version
	trap_FS_Read( &ver, sizeof( i ), f );


	// 'info' if > 11
	// 'mission' if > 12
	// 'skill' if >13
	// 'time' if > 14

	// if the version is wrong, just set some defaults and get out
	if ( ver < 9 ) {  // don't try anything for really old savegames
		trap_FS_FCloseFile( f );
		uiInfo.savegameList[index].mapName          = "unknownmap";
		uiInfo.savegameList[index].episode          = -1;
		uiInfo.savegameList[index].savegameInfoText = "Gametime: (unknown)\nHealth: (unknown)\n(old savegame)";

		uiInfo.savegameList[index].date = "temp_date";
		uiInfo.savegameList[index].time = "(old savegame)";

		memset( &uiInfo.savegameList[index].tm, 0, sizeof( qtime_t ) );
		uiInfo.savegameList[index].time = String_Alloc( va( "(old savegame ver: %d)", ver ) );
		return;
	}

	// read the mapname
	trap_FS_Read( mapname, MAX_QPATH, f );
	uiInfo.savegameList[index].mapName = String_Alloc( &mapname[0] );

	// read the level time
	trap_FS_Read( &i, sizeof( i ), f );

	// read the totalPlayTime
	trap_FS_Read( &i, sizeof( i ), f );

	// read the episode
	trap_FS_Read( &i, sizeof( i ), f );
	uiInfo.savegameList[index].episode = i;

	if ( ver < 12 ) {
		trap_FS_FCloseFile( f );
		uiInfo.savegameList[index].savegameInfoText = "Gametime: (unknown)\nHealth: (unknown)\n(old savegame)";
		uiInfo.savegameList[index].date = "temp_date";
		memset( &uiInfo.savegameList[index].tm, 0, sizeof( qtime_t ) );
		uiInfo.savegameList[index].time = String_Alloc( va( "(old savegame ver: %d)", ver ) );
		return;
	}

	// read the info string length
	trap_FS_Read( &i, sizeof( i ), f );

	// read the info string
	trap_FS_Read( buf, i, f );
	buf[i] = '\0';        //DAJ made it a char
	uiInfo.savegameList[index].savegameInfoText = String_Alloc( buf );

	// time
	if ( ver > 14 ) {
		tm = &uiInfo.savegameList[index].tm;
		trap_FS_Read( &tm->tm_sec, sizeof( tm->tm_sec ), f );          // secs after the min
		trap_FS_Read( &tm->tm_min, sizeof( tm->tm_min ), f );          // mins after the hour
		trap_FS_Read( &tm->tm_hour, sizeof( tm->tm_hour ), f );        // hrs since midnight
		trap_FS_Read( &tm->tm_mday, sizeof( tm->tm_mday ), f );
		trap_FS_Read( &tm->tm_mon, sizeof( tm->tm_mon ), f );
		trap_FS_Read( &tm->tm_year, sizeof( tm->tm_year ), f );        // yrs from 1900
		trap_FS_Read( &tm->tm_wday, sizeof( tm->tm_wday ), f );
		trap_FS_Read( &tm->tm_yday, sizeof( tm->tm_yday ), f );        // days since jan1 (0-365)
		trap_FS_Read( &tm->tm_isdst, sizeof( tm->tm_isdst ), f );
		uiInfo.savegameList[index].time = String_Alloc( va( "%s %i, %i   %02i:%02i", monthStr[tm->tm_mon], tm->tm_mday, 1900 + tm->tm_year, tm->tm_hour, tm->tm_min ) );
	} else {
		memset( &uiInfo.savegameList[index].tm, 0, sizeof( qtime_t ) );
		uiInfo.savegameList[index].time = String_Alloc( va( "(old save ver: %d)", ver ) );
	}

	trap_FS_FCloseFile( f );
}

/*
==============
UI_LoadSavegames
==============
*/
static void UI_LoadSavegames( char *dir ) {
	char sglist[4096];
	char    *sgname;
	int i, len;

	if ( dir ) {
		uiInfo.savegameCount = trap_FS_GetFileList( va( "save/%s", dir ), "svg", sglist, 4096 );
	} else {
		uiInfo.savegameCount = trap_FS_GetFileList( "save", "svg", sglist, 4096 );
	}

	if ( uiInfo.savegameCount ) {
		if ( uiInfo.savegameCount > MAX_SAVEGAMES ) {
			uiInfo.savegameCount = MAX_SAVEGAMES;
		}
		sgname = sglist;
		for ( i = 0; i < uiInfo.savegameCount; i++ ) {

			len = strlen( sgname );

			if ( !Q_stricmp( sgname, "current.svg" ) ) {    // ignore some savegames that have special uses and shouldn't be loaded by the user directly
				i--;
				uiInfo.savegameCount -= 1;
				sgname += len + 1;
				continue;
			}

			if ( !Q_stricmp( sgname +  len - 4,".svg" ) ) {
				sgname[len - 4] = '\0';
			}
//			Q_strupr(sgname);
			if ( dir ) {
				uiInfo.savegameList[i].savegameFile = String_Alloc( va( "%s/%s", dir, sgname ) );
			} else {
				uiInfo.savegameList[i].savegameFile = String_Alloc( sgname );
			}

			uiInfo.savegameList[i].savegameName = String_Alloc( sgname );

			// get string into list for sorting too
			uiInfo.savegameStatus.displaySavegames[i] = i;
//			qsort( &uiInfo.savegameStatus.displaySavegames[0], uiInfo.savegameCount, sizeof(int), UI_SavegamesQsortCompare);

			// read savegame and get needed info
			UI_ParseSavegame( i );

			if ( uiInfo.savegameList[i].episode != -1 ) {
				uiInfo.savegameList[i].sshotImage = trap_R_RegisterShaderNoMip( va( "levelshots/episodeshots/e%d.tga", uiInfo.savegameList[i].episode + 1 ) );
			} else {
				uiInfo.savegameList[i].sshotImage = trap_R_RegisterShaderNoMip( "levelshots/episodeshots/e_unknown.tga" );
			}



			sgname += len + 1;
		}

		// sort it
		UI_SavegameSort( 0, qtrue );

		// set current selection
//		i = UI_SavegameIndexFromName(ui_savegameName.string);
//		Menu_SetFeederSelection(NULL, FEEDER_SAVEGAMES, i, NULL);
	}
}


/*
===============
UI_LoadMovies
===============
*/
static void UI_LoadMovies() {
	char movielist[4096];
	char    *moviename;
	int i, len;

	uiInfo.movieCount = trap_FS_GetFileList( "video", "roq", movielist, 4096 );

	if ( uiInfo.movieCount ) {
		if ( uiInfo.movieCount > MAX_MOVIES ) {
			uiInfo.movieCount = MAX_MOVIES;
		}
		moviename = movielist;
		for ( i = 0; i < uiInfo.movieCount; i++ ) {
			len = strlen( moviename );
			if ( !Q_stricmp( moviename +  len - 4,".roq" ) ) {
				moviename[len - 4] = '\0';
			}
			Q_strupr( moviename );
			uiInfo.movieList[i] = String_Alloc( moviename );
			moviename += len + 1;
		}
	}

}



/*
===============
UI_LoadDemos
===============
*/
static void UI_LoadDemos() {
	char demolist[4096];
	char demoExt[32];
	char    *demoname;
	int i, len;

	Com_sprintf( demoExt, sizeof( demoExt ), "dm_%d", (int)trap_Cvar_VariableValue( "protocol" ) );

	uiInfo.demoCount = trap_FS_GetFileList( "demos", demoExt, demolist, 4096 );

	Com_sprintf( demoExt, sizeof( demoExt ), ".dm_%d", (int)trap_Cvar_VariableValue( "protocol" ) );

	if ( uiInfo.demoCount ) {
		if ( uiInfo.demoCount > MAX_DEMOS ) {
			uiInfo.demoCount = MAX_DEMOS;
		}
		demoname = demolist;
		for ( i = 0; i < uiInfo.demoCount; i++ ) {
			len = strlen( demoname );
			if ( !Q_stricmp( demoname +  len - strlen( demoExt ), demoExt ) ) {
				demoname[len - strlen( demoExt )] = '\0';
			}
			Q_strupr( demoname );
			uiInfo.demoList[i] = String_Alloc( demoname );
			demoname += len + 1;
		}
	}

}


/*
==============
UI_SetNextMap
==============
*/
// TTimo: unused
/*
static qboolean UI_SetNextMap(int actual, int index) {
	int i;
	for (i = actual + 1; i < uiInfo.mapCount; i++) {
		if (uiInfo.mapList[i].active) {
			Menu_SetFeederSelection(NULL, FEEDER_MAPS, index + 1, "skirmish");
			return qtrue;
		}
	}
	return qfalse;
}
*/

/*
==============
UI_StartSkirmish
==============
*/
static void UI_StartSkirmish( qboolean next ) {
#ifdef MISSIONPACK
	int i, k, g, delay, temp;
	float skill;
	char buff[MAX_STRING_CHARS];

	if ( next ) {
		int actual;
		int index = trap_Cvar_VariableValue( "ui_mapIndex" );
		UI_MapCountByGameType( qtrue );
		UI_SelectedMap( index, &actual );
		if ( UI_SetNextMap( actual, index ) ) {
		} else {
			UI_GameType_HandleKey( 0, 0, K_MOUSE1, qfalse );
			UI_MapCountByGameType( qtrue );
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, 0, "skirmish" );
		}
	}

	g = uiInfo.gameTypes[ui_gameType.integer].gtEnum;
	trap_Cvar_SetValue( "g_gametype", g );
	trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentMap.integer].mapLoadName ) );
	skill = trap_Cvar_VariableValue( "g_spSkill" );
	trap_Cvar_Set( "ui_scoreMap", uiInfo.mapList[ui_currentMap.integer].mapName );

	k = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_opponentName" ) );

	trap_Cvar_Set( "ui_singlePlayerActive", "1" );

	// set up sp overrides, will be replaced on postgame
	temp = trap_Cvar_VariableValue( "capturelimit" );
	trap_Cvar_Set( "ui_saveCaptureLimit", va( "%i", temp ) );
	temp = trap_Cvar_VariableValue( "fraglimit" );
	trap_Cvar_Set( "ui_saveFragLimit", va( "%i", temp ) );

	UI_SetCapFragLimits( qfalse );

	temp = trap_Cvar_VariableValue( "cg_drawTimer" );
	trap_Cvar_Set( "ui_drawTimer", va( "%i", temp ) );
	temp = trap_Cvar_VariableValue( "g_doWarmup" );
	trap_Cvar_Set( "ui_doWarmup", va( "%i", temp ) );
	temp = trap_Cvar_VariableValue( "g_friendlyFire" );
	trap_Cvar_Set( "ui_friendlyFire", va( "%i", temp ) );
	temp = trap_Cvar_VariableValue( "sv_maxClients" );
	trap_Cvar_Set( "ui_maxClients", va( "%i", temp ) );
	temp = trap_Cvar_VariableValue( "g_warmup" );
	trap_Cvar_Set( "ui_Warmup", va( "%i", temp ) );
	temp = trap_Cvar_VariableValue( "sv_pure" );
	trap_Cvar_Set( "ui_pure", va( "%i", temp ) );

	trap_Cvar_Set( "cg_cameraOrbit", "0" );
	trap_Cvar_Set( "cg_thirdPerson", "0" );
	trap_Cvar_Set( "cg_drawTimer", "1" );
	trap_Cvar_Set( "g_doWarmup", "1" );
	trap_Cvar_Set( "g_warmup", "15" );
	trap_Cvar_Set( "sv_pure", "0" );
	trap_Cvar_Set( "g_friendlyFire", "0" );
	trap_Cvar_Set( "g_redTeam", UI_Cvar_VariableString( "ui_teamName" ) );
	trap_Cvar_Set( "g_blueTeam", UI_Cvar_VariableString( "ui_opponentName" ) );

	if ( trap_Cvar_VariableValue( "ui_recordSPDemo" ) ) {
		Com_sprintf( buff, MAX_STRING_CHARS, "%s_%i", uiInfo.mapList[ui_currentMap.integer].mapLoadName, g );
		trap_Cvar_Set( "ui_recordSPDemoName", buff );
	}

	delay = 500;

	if ( g == GT_TOURNAMENT ) {
		trap_Cvar_Set( "sv_maxClients", "2" );
		Com_sprintf( buff, sizeof( buff ), "wait ; addbot %s %f " ", %i \n", uiInfo.mapList[ui_currentMap.integer].opponentName, skill, delay );
		trap_Cmd_ExecuteText( EXEC_APPEND, buff );
	} else {
		temp = uiInfo.mapList[ui_currentMap.integer].teamMembers * 2;
		trap_Cvar_Set( "sv_maxClients", va( "%d", temp ) );
		for ( i = 0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers; i++ ) {
			Com_sprintf( buff, sizeof( buff ), "addbot %s %f %s %i %s\n", UI_AIFromName( uiInfo.teamList[k].teamMembers[i] ), skill, ( g == GT_FFA ) ? "" : "Blue", delay, uiInfo.teamList[k].teamMembers[i] );
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
		k = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
		for ( i = 0; i < uiInfo.mapList[ui_currentMap.integer].teamMembers - 1; i++ ) {
			Com_sprintf( buff, sizeof( buff ), "addbot %s %f %s %i %s\n", UI_AIFromName( uiInfo.teamList[k].teamMembers[i] ), skill, ( g == GT_FFA ) ? "" : "Red", delay, uiInfo.teamList[k].teamMembers[i] );
			trap_Cmd_ExecuteText( EXEC_APPEND, buff );
			delay += 500;
		}
	}
	if ( g >= GT_TEAM ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, "wait 5; team Red\n" );
	}
#endif  // #ifdef MISSIONPACK
}

// NERVE - SMF
/*
==============
WM_ChangePlayerType
==============
*/
itemDef_t *Menu_FindItemByName( menuDef_t *menu, const char *p );
void Menu_ShowItemByName( menuDef_t *menu, const char *p, qboolean bShow );

#define ITEM_GRENADES       1
#define ITEM_MEDKIT         2

#define ITEM_PISTOL         1

#define DEFAULT_PISTOL

#define PT_KNIFE            ( 1 )
#define PT_PISTOL           ( 1 << 2 )
#define PT_RIFLE            ( 1 << 3 )
#define PT_LIGHTONLY        ( 1 << 4 )
#define PT_GRENADES         ( 1 << 5 )
#define PT_EXPLOSIVES       ( 1 << 6 )
#define PT_MEDKIT           ( 1 << 7 )

typedef struct {
	const char  *name;
	int items;
} playerType_t;

static playerType_t playerTypes[] = {
	{ "player_window_soldier",       PT_KNIFE | PT_PISTOL | PT_RIFLE | PT_GRENADES },
	{ "player_window_medic",     PT_KNIFE | PT_PISTOL | PT_MEDKIT },
	{ "player_window_engineer",      PT_KNIFE | PT_PISTOL | PT_LIGHTONLY | PT_EXPLOSIVES | PT_GRENADES },
	{ "player_window_lieutenant",    PT_KNIFE | PT_PISTOL | PT_RIFLE | PT_EXPLOSIVES }
};

typedef struct {
	int weapindex;

	const char  *desc;
	int flags;
	const char  *cvar;
	int value;
	const char  *name;

	const char  *torso_anim;
	const char  *legs_anim;
} weaponType_t;

// NERVE - SMF - this is the weapon info list [what can and can't be used by character classes]
//   - This list is seperate from the actual text names in the listboxes for localization purposes.
//   - The list boxes look up this list by the cvar value.
static weaponType_t weaponTypes[] = {
	{ 0, "NULL", 0, "none", 0, "none", "", "" },

	{ WP_COLT,  "1911 pistol",   PT_PISTOL,  "mp_weapon", 0, "ui/assets/weapon_colt1911.tga",      "firing_pistolB_1",      "stand_pistolB" },
	{ WP_LUGER, "Luger pistol",  PT_PISTOL,  "mp_weapon", 1, "ui/assets/weapon_luger.tga",     "firing_pistolB_1",      "stand_pistolB" },
	//	{ 0,		"Medkit",		PT_MEDKIT,	"mp_item2",		2, "ui/assets/item_medkit.tga",			"firing_pistolB_1",		"stand_pistolB" },

	{ WP_MP40,              "MP40 submachinegun",        PT_LIGHTONLY | PT_RIFLE,    "mp_weapon", 3, "ui/assets/weapon_mp40.tga",          "relaxed_idle_2h_1", "relaxed_idle_2h_1" },
	{ WP_THOMPSON,          "Thompson submachinegun",    PT_LIGHTONLY | PT_RIFLE,    "mp_weapon", 4, "ui/assets/weapon_thompson.tga",      "relaxed_idle_2h_1", "relaxed_idle_2h_1" },
	{ WP_STEN,              "Sten submachinegun",        PT_LIGHTONLY | PT_RIFLE,    "mp_weapon", 5, "ui/assets/weapon_sten.tga",          "relaxed_idle_2h_1", "relaxed_idle_2h_1" },

	{ WP_MAUSER,            "Mauser sniper rifle",       PT_RIFLE,                   "mp_weapon", 6, "ui/assets/weapon_mauser.tga",        "stand_rifle",           "stand_rifle" },
	{ WP_GARAND,            "Garand rifle",              PT_RIFLE,                   "mp_weapon", 7, "ui/assets/weapon_mauser.tga",        "stand_rifle",           "stand_rifle" },
	{ WP_PANZERFAUST,       "Panzerfaust",               PT_RIFLE,                   "mp_weapon", 8, "ui/assets/weapon_panzerfaust.tga",   "stand_machinegun",      "stand_machinegun" },
	{ WP_VENOM,             "Minigun",                   PT_RIFLE,                   "mp_weapon", 9, "ui/assets/weapon_venom.tga",     "stand_machinegun",      "stand_machinegun" },
	{ WP_FLAMETHROWER,      "Flamethrower",              PT_RIFLE,                   "mp_weapon", 10, "ui/assets/weapon_flamethrower.tga","stand_machinegun",       "stand_machinegun" },

	{ WP_GRENADE_PINEAPPLE, "Pineapple grenade",     PT_GRENADES,                "mp_item1",      11, "ui/assets/weapon_grenade.tga",      "firing_pistolB_1",      "stand_pistolB" },
	{ WP_GRENADE_LAUNCHER,  "Stick grenade",         PT_GRENADES,                "mp_item1",      12, "ui/assets/weapon_grenade_ger.tga",  "firing_pistolB_1",      "stand_pistolB" },

	{ WP_DYNAMITE,          "Explosives",                PT_EXPLOSIVES,              "mp_item2",      13, "ui/assets/weapon_dynamite.tga", "firing_pistolB_1",      "stand_pistolB" },

	{ 0, NULL, 0, NULL, 0, NULL, NULL, NULL }
};

typedef struct {
	char        *name;
	int flags;
	char        *shader;
} uiitemType_t;

#define UI_KNIFE_PIC    "window_knife_pic"
#define UI_PISTOL_PIC   "window_pistol_pic"
#define UI_WEAPON_PIC   "window_weapon_pic"
#define UI_ITEM1_PIC    "window_item1_pic"
#define UI_ITEM2_PIC    "window_item2_pic"

static uiitemType_t itemTypes[] = {
	{ UI_KNIFE_PIC,     PT_KNIFE,       "ui/assets/weapon_knife.tga" },
	{ UI_PISTOL_PIC,    PT_PISTOL,      "ui/assets/weapon_colt1911.tga" },

	{ UI_WEAPON_PIC,    PT_RIFLE,       "ui/assets/weapon_mauser.tga" },

	{ UI_ITEM1_PIC,     PT_MEDKIT,      "ui/assets/item_medkit.tga" },

	{ UI_ITEM1_PIC,     PT_GRENADES,    "ui/assets/weapon_grenade.tga" },
	{ UI_ITEM2_PIC,     PT_EXPLOSIVES,  "ui/assets/weapon_dynamite.tga" },

	{ NULL, 0, NULL }
};


int WM_getWeaponIndex() {
	int lookupIndex, i;

	lookupIndex = trap_Cvar_VariableValue( "mp_weapon" );

	for ( i = 1; weaponTypes[i].name; i++ ) {
		if ( weaponTypes[i].value == lookupIndex ) {
			return weaponTypes[i].weapindex;
		}
	}

	return 0;
}

void WM_getWeaponAnim( const char **torso_anim, const char **legs_anim ) {
	int lookupIndex, i;

	lookupIndex = trap_Cvar_VariableValue( "mp_weapon" );

	for ( i = 1; weaponTypes[i].name; i++ ) {
		if ( weaponTypes[i].value == lookupIndex ) {
			*torso_anim = weaponTypes[i].torso_anim;
			*legs_anim = weaponTypes[i].legs_anim;
			return;
		}
	}
}

static void WM_ChangePlayerType() {
	int i, j, playerType;
	menuDef_t *menu = Menu_GetFocused();
	itemDef_t *itemdef, *itemdef2;

	playerType = trap_Cvar_VariableValue( "mp_playerType" );

	for ( i = 0; i < 4; i++ ) {
		itemdef = Menu_FindItemByName( menu, playerTypes[i].name );
		if ( !itemdef ) {
			continue;
		}

		if ( i == playerType ) {
			Menu_ShowItemByName( itemdef->parent, playerTypes[i].name, qtrue );
		} else {
			Menu_ShowItemByName( itemdef->parent, playerTypes[i].name, qfalse );
		}

		// selected only settings
		if ( i != playerType ) {
			continue;
		}

		// force all to none first
		for ( j = 0; itemTypes[j].name; j++ ) {
			itemdef2 = Menu_FindItemByName( menu, itemTypes[j].name );
			if ( itemdef2 ) {
				itemdef2->window.background = DC->registerShaderNoMip( "ui/assets/item_none.tga" );
			}
		}

		// set values
		for ( j = 0; itemTypes[j].name; j++ ) {
			itemdef2 = Menu_FindItemByName( menu, itemTypes[j].name );
			if ( itemdef2 && ( playerTypes[i].items & itemTypes[j].flags ) ) {
				itemdef2->window.background = DC->registerShaderNoMip( itemTypes[j].shader );
			}
		}
	}
}

void WM_GetSpawnPoints() {
	char cs[MAX_STRING_CHARS];
	const char *s;
	int i;

	trap_GetConfigString( CS_MULTI_INFO, cs, sizeof( cs ) );
	s = Info_ValueForKey( cs, "numspawntargets" );

	if ( !s ) {
		return;
	}

	// first index is for autopicking
	Q_strncpyz( uiInfo.spawnPoints[0], "Auto Pick", MAX_SPAWNDESC );

	uiInfo.spawnCount = atoi( s ) + 1;

	for ( i = 1; i < uiInfo.spawnCount; i++ ) {
		trap_GetConfigString( CS_MULTI_SPAWNTARGETS + i, cs, sizeof( cs ) );

		s = Info_ValueForKey( cs, "spawn_targ" );
		if ( !s ) {
			return;
		}

		Q_strncpyz( uiInfo.spawnPoints[i], s, MAX_SPAWNDESC );
	}
}

void WM_HideItems() {
	menuDef_t *menu = Menu_GetFocused();

	Menu_ShowItemByName( menu, "window_pickplayer", qfalse );
	Menu_ShowItemByName( menu, "window_weap", qfalse );
	Menu_ShowItemByName( menu, "weap_*", qfalse );
	Menu_ShowItemByName( menu, "pistol_*", qfalse );
	Menu_ShowItemByName( menu, "grenade_*", qfalse );
	Menu_ShowItemByName( menu, "player_type", qfalse );
}

void WM_PickItem( int selectionType, int itemIndex ) {
	menuDef_t *menu = Menu_GetFocused();
	int playerType;

	if ( selectionType == WM_SELECT_TEAM ) {
		switch ( itemIndex ) {
		case WM_AXIS:
			trap_Cvar_Set( "mp_team", "0" );
			break;
		case WM_ALLIES:
			trap_Cvar_Set( "mp_team", "1" );
			break;
		case WM_SPECTATOR:
			trap_Cvar_Set( "mp_team", "2" );
			break;
		}
	} else if ( selectionType == WM_SELECT_CLASS )   {
		switch ( itemIndex ) {
		case WM_START_SELECT:
			break;
		case WM_SOLDIER:
			trap_Cvar_Set( "mp_playerType", "0" );
			trap_Cvar_Set( "mp_weapon", "0" );
			break;
		case WM_MEDIC:
			trap_Cvar_Set( "mp_playerType", "1" );
			trap_Cvar_Set( "mp_weapon", "0" );
			break;
		case WM_ENGINEER:
			trap_Cvar_Set( "mp_playerType", "2" );
			trap_Cvar_Set( "mp_weapon", "0" );
			break;
		case WM_LIEUTENANT:
			trap_Cvar_Set( "mp_playerType", "3" );
			trap_Cvar_Set( "mp_weapon", "0" );
			break;
		}
	} else if ( selectionType == WM_SELECT_WEAPON )   {
		if ( itemIndex == WM_START_SELECT ) {
		} else {
			trap_Cvar_Set( weaponTypes[itemIndex].cvar, va( "%i", weaponTypes[itemIndex].value ) );
		}
	} else if ( selectionType == WM_SELECT_PISTOL )   {
		if ( itemIndex == WM_START_SELECT ) {
			// hide all other menus first
			WM_HideItems();

			// show this menu
			Menu_ShowItemByName( menu, "window_weap", qtrue );
			Menu_ShowItemByName( menu, "pistol_*", qtrue );
		} else {
			itemDef_t *itemdef = Menu_FindItemByName( menu, UI_PISTOL_PIC );

			trap_Cvar_Set( weaponTypes[itemIndex].cvar, va( "%i", weaponTypes[itemIndex].value ) );

			if ( itemdef ) {
				itemdef->window.background = DC->registerShaderNoMip( weaponTypes[itemIndex].name );
			}

			Menu_ShowItemByName( menu, "window_weap", qfalse );
			Menu_ShowItemByName( menu, "pistol_*", qfalse );
		}
	} else if ( selectionType == WM_SELECT_GRENADE )   {
		if ( itemIndex == WM_START_SELECT ) {
			// hide all other menus first
			WM_HideItems();

			// show this menu
			playerType = trap_Cvar_VariableValue( "mp_playerType" );

			if ( playerType == 1 || playerType == 3 ) {
				return;
			}

			Menu_ShowItemByName( menu, "window_weap", qtrue );
			Menu_ShowItemByName( menu, "grenade_*", qtrue );
		} else {
			itemDef_t *itemdef = Menu_FindItemByName( menu, UI_ITEM1_PIC );

			trap_Cvar_Set( weaponTypes[itemIndex].cvar, va( "%i", weaponTypes[itemIndex].value ) );

			if ( itemdef ) {
				itemdef->window.background = DC->registerShaderNoMip( weaponTypes[itemIndex].name );
			}

			Menu_ShowItemByName( menu, "window_weap", qfalse );
			Menu_ShowItemByName( menu, "grenade_*", qfalse );
		}
	}
}

void WM_LimboChat() {
	char buf[200];

	trap_Cvar_VariableStringBuffer( "ui_cmd", buf, 200 );

	if ( strlen( buf ) ) {
		trap_Cmd_ExecuteText( EXEC_APPEND, va( "say_limbo %s\n", buf ) );
	}

	trap_Cvar_Set( "ui_cmd", "" );
}

extern qboolean g_waitingForKey;
extern qboolean g_editingField;
extern itemDef_t *g_editItem;

void WM_ActivateLimboChat() {
	menuDef_t *menu;
	itemDef_t *itemdef;

	menu = Menu_GetFocused();
	menu = Menus_ActivateByName( "wm_limboChat" );

	if ( !menu || g_editItem ) {
		return;
	}

	itemdef = Menu_FindItemByName( menu, "window_limbo_chat" );

	if ( itemdef ) {
		itemdef->cursorPos = 0;
		g_editingField = qtrue;
		g_editItem = itemdef;
		DC->setOverstrikeMode( qtrue );
	}
}
// -NERVE - SMF

/*
==============
UI_Update
==============
*/
static void UI_Update( const char *name ) {
	int val = trap_Cvar_VariableValue( name );

	if ( Q_stricmp( name, "ui_SetName" ) == 0 ) {
		trap_Cvar_Set( "name", UI_Cvar_VariableString( "ui_Name" ) );
	} else if ( Q_stricmp( name, "ui_setRate" ) == 0 ) {
		float rate = trap_Cvar_VariableValue( "rate" );
		if ( rate >= 5000 ) {
			trap_Cvar_Set( "cl_maxpackets", "30" );
			trap_Cvar_Set( "cl_packetdup", "1" );
		} else if ( rate >= 4000 ) {
			trap_Cvar_Set( "cl_maxpackets", "15" );
			trap_Cvar_Set( "cl_packetdup", "2" );       // favor less prediction errors when there's packet loss
		} else {
			trap_Cvar_Set( "cl_maxpackets", "15" );
			trap_Cvar_Set( "cl_packetdup", "1" );       // favor lower bandwidth
		}
	} else if ( Q_stricmp( name, "ui_GetName" ) == 0 ) {
		trap_Cvar_Set( "ui_Name", UI_Cvar_VariableString( "name" ) );
	} else if ( Q_stricmp( name, "r_colorbits" ) == 0 ) {
		switch ( val ) {
		case 0:
			trap_Cvar_SetValue( "r_depthbits", 0 );
			trap_Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 16:
			trap_Cvar_SetValue( "r_depthbits", 16 );
			trap_Cvar_SetValue( "r_stencilbits", 0 );
			break;
		case 32:
			trap_Cvar_SetValue( "r_depthbits", 24 );
			break;
		}
	} else if ( Q_stricmp( name, "r_lodbias" ) == 0 ) {
		switch ( val ) {
		case 0:
			trap_Cvar_SetValue( "r_subdivisions", 4 );
			break;
		case 1:
			trap_Cvar_SetValue( "r_subdivisions", 12 );
			break;
		case 2:
			trap_Cvar_SetValue( "r_subdivisions", 20 );
			break;
		}
	} else if ( Q_stricmp( name, "ui_glCustom" ) == 0 ) {
		switch ( val ) {
		case 0: // high quality
			trap_Cvar_SetValue( "r_fullScreen", 1 );
			trap_Cvar_SetValue( "r_subdivisions", 4 );
			trap_Cvar_SetValue( "r_vertexlight", 0 );
			trap_Cvar_SetValue( "r_lodbias", 0 );
			trap_Cvar_SetValue( "r_colorbits", 32 );
			trap_Cvar_SetValue( "r_depthbits", 24 );
			trap_Cvar_SetValue( "r_picmip", 0 );
			trap_Cvar_SetValue( "r_picmip2", 0 );
			trap_Cvar_SetValue( "r_mode", 4 );
			trap_Cvar_SetValue( "r_texturebits", 32 );
			trap_Cvar_SetValue( "r_fastSky", 0 );
			trap_Cvar_SetValue( "r_inGameVideo", 1 );
			trap_Cvar_SetValue( "cg_shadows", 1 );
			trap_Cvar_SetValue( "cg_brassTime", 2500 );
			trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_LINEAR" );
			break;
		case 1: // normal
			trap_Cvar_SetValue( "r_fullScreen", 1 );
			trap_Cvar_SetValue( "r_subdivisions", 12 );
			trap_Cvar_SetValue( "r_vertexlight", 0 );
			trap_Cvar_SetValue( "r_lodbias", 0 );
			trap_Cvar_SetValue( "r_colorbits", 0 );
			trap_Cvar_SetValue( "r_depthbits", 24 );
			trap_Cvar_SetValue( "r_picmip", 0 );
			trap_Cvar_SetValue( "r_picmip2", 1 );
			trap_Cvar_SetValue( "r_mode", 3 );
			trap_Cvar_SetValue( "r_texturebits", 0 );
			trap_Cvar_SetValue( "r_fastSky", 0 );
			trap_Cvar_SetValue( "r_inGameVideo", 1 );
			trap_Cvar_SetValue( "cg_brassTime", 2500 );
			trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" ); //----(SA)	modified so wolf never sets trilinear automatically
			trap_Cvar_SetValue( "cg_shadows", 0 );
			break;
		case 2: // fast
			trap_Cvar_SetValue( "r_fullScreen", 1 );
			trap_Cvar_SetValue( "r_subdivisions", 8 );
			trap_Cvar_SetValue( "r_vertexlight", 0 );
			trap_Cvar_SetValue( "r_lodbias", 1 );
			trap_Cvar_SetValue( "r_colorbits", 0 );
			trap_Cvar_SetValue( "r_depthbits", 0 );
			trap_Cvar_SetValue( "r_picmip", 1 );
			trap_Cvar_SetValue( "r_picmip2", 2 );
			trap_Cvar_SetValue( "r_mode", 3 );
			trap_Cvar_SetValue( "r_texturebits", 0 );
			trap_Cvar_SetValue( "cg_shadows", 0 );
			trap_Cvar_SetValue( "r_fastSky", 1 );
			trap_Cvar_SetValue( "r_inGameVideo", 0 );
			trap_Cvar_SetValue( "cg_brassTime", 0 );
			trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;
		case 3: // fastest
			trap_Cvar_SetValue( "r_fullScreen", 1 );
			trap_Cvar_SetValue( "r_subdivisions", 20 );
			trap_Cvar_SetValue( "r_vertexlight", 1 );
			trap_Cvar_SetValue( "r_lodbias", 2 );
			trap_Cvar_SetValue( "r_colorbits", 16 );
			trap_Cvar_SetValue( "r_depthbits", 16 );
			trap_Cvar_SetValue( "r_mode", 3 );
			trap_Cvar_SetValue( "r_picmip", 2 );
			trap_Cvar_SetValue( "r_picmip2", 3 );
			trap_Cvar_SetValue( "r_texturebits", 16 );
			trap_Cvar_SetValue( "cg_shadows", 0 );
			trap_Cvar_SetValue( "cg_brassTime", 0 );
			trap_Cvar_SetValue( "r_fastSky", 1 );
			trap_Cvar_SetValue( "r_inGameVideo", 0 );
			trap_Cvar_Set( "r_texturemode", "GL_LINEAR_MIPMAP_NEAREST" );
			break;

		case 999:   // 999 is reserved for having set default values ("recommended")
			break;
		}
	} else if ( Q_stricmp( name, "ui_mousePitch" ) == 0 ) {
		if ( val == 0 ) {
			trap_Cvar_SetValue( "m_pitch", 0.022f );
		} else {
			trap_Cvar_SetValue( "m_pitch", -0.022f );
		}

//----(SA)	added
	} else if ( Q_stricmp( name, "ui_savegameListAutosave" ) == 0 ) {
		if ( val == 0 ) {
			UI_LoadSavegames( NULL );
		} else {
			// TODO: get this from a cvar, so more complicated
			// directory structures can be set up later if desired
//			cvar = getcvar("ui_savegameSubdir");
//			UI_LoadSavegames(cvar.value);
			UI_LoadSavegames( "autosave" );    // get from default directory 'main/save/autosave/*.svg'
		}
	}
//----(SA)	end

}


/*
==============
UI_RunMenuScript
==============
*/

static void UI_RunMenuScript( char **args ) {
	const char *name, *name2;
	char buff[1024];

	if ( String_Parse( args, &name ) ) {
		//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
		if ( Q_stricmp( name, "StartServer" ) == 0 ) {
			int i, clients;
			float skill;
			trap_Cvar_Set( "cg_thirdPerson", "0" );
			trap_Cvar_Set( "cg_cameraOrbit", "0" );
			trap_Cvar_Set( "ui_singlePlayerActive", "0" );
			trap_Cvar_SetValue( "dedicated", Com_Clamp( 0, 2, ui_dedicated.integer ) );
			trap_Cvar_SetValue( "g_gametype", Com_Clamp( 0, 8, uiInfo.gameTypes[ui_netGameType.integer].gtEnum ) );
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "wait ; wait ; map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName ) );
			skill = trap_Cvar_VariableValue( "g_spSkill" );
			// set max clients based on spots
			clients = 0;
			for ( i = 0; i < PLAYERS_PER_TEAM; i++ ) {
				int bot = trap_Cvar_VariableValue( va( "ui_blueteam%i", i + 1 ) );
				if ( bot >= 0 ) {
					clients++;
				}
				bot = trap_Cvar_VariableValue( va( "ui_redteam%i", i + 1 ) );
				if ( bot >= 0 ) {
					clients++;
				}
			}
			if ( clients == 0 ) {
				clients = 8;
			}
			trap_Cvar_Set( "sv_maxClients", va( "%d",clients ) );

			for ( i = 0; i < PLAYERS_PER_TEAM; i++ ) {
				int bot = trap_Cvar_VariableValue( va( "ui_blueteam%i", i + 1 ) );
				if ( bot > 1 ) {
					if ( ui_actualNetGameType.integer >= GT_TEAM ) {
						Com_sprintf( buff, sizeof( buff ), "addbot %s %f %s\n", uiInfo.characterList[bot - 2].name, skill, "Blue" );
					} else {
						// NERVE - SMF - no bots in wolf multiplayer
						//						Com_sprintf( buff, sizeof(buff), "addbot %s %f \n", UI_GetBotNameByNumber(bot-2), skill);
					}
					trap_Cmd_ExecuteText( EXEC_APPEND, buff );
				}
				bot = trap_Cvar_VariableValue( va( "ui_redteam%i", i + 1 ) );
				if ( bot > 1 ) {
					if ( ui_actualNetGameType.integer >= GT_TEAM ) {
						Com_sprintf( buff, sizeof( buff ), "addbot %s %f %s\n", uiInfo.characterList[bot - 2].name, skill, "Red" );
					} else {
						// NERVE - SMF - no bots in wolf multiplayer
						//						Com_sprintf( buff, sizeof(buff), "addbot %s %f \n", UI_GetBotNameByNumber(bot-2), skill);
					}
					trap_Cmd_ExecuteText( EXEC_APPEND, buff );
				}
			}
		} else if ( Q_stricmp( name, "updateSPMenu" ) == 0 ) {
			UI_SetCapFragLimits( qtrue );
			UI_MapCountByGameType( qtrue );
			ui_mapIndex.integer = UI_GetIndexFromSelection( ui_currentMap.integer );
			trap_Cvar_Set( "ui_mapIndex", va( "%d", ui_mapIndex.integer ) );
			Menu_SetFeederSelection( NULL, FEEDER_MAPS, ui_mapIndex.integer, "skirmish" );
			UI_GameType_HandleKey( 0, 0, K_MOUSE1, qfalse );
			UI_GameType_HandleKey( 0, 0, K_MOUSE2, qfalse );
		} else if ( Q_stricmp( name, "resetDefaults" ) == 0 ) {
// SP was like...
//			trap_Cmd_ExecuteText( EXEC_APPEND, "exec default.cfg\n");
//			trap_Cmd_ExecuteText( EXEC_APPEND, "cvar_restart\n");
//			Controls_SetDefaults();
//			trap_Cvar_Set("com_introPlayed", "1" );
//			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );

// from MP 11/12/01
			trap_Cmd_ExecuteText( EXEC_NOW, "cvar_restart\n" );            // NERVE - SMF - changed order
			trap_Cmd_ExecuteText( EXEC_NOW, "exec default.cfg\n" );
			trap_Cmd_ExecuteText( EXEC_NOW, "exec language.cfg\n" );       // NERVE - SMF
			trap_Cmd_ExecuteText( EXEC_NOW, "setRecommended\n" );     // NERVE - SMF
			Controls_SetDefaults();
			trap_Cvar_Set( "com_introPlayed", "1" );
			trap_Cvar_Set( "com_recommendedSet", "1" );                   // NERVE - SMF
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart\n" );
// end from MP
		} else if ( Q_stricmp( name, "getCDKey" ) == 0 ) {
			char out[17];
			trap_GetCDKey( buff, 17 );
			trap_Cvar_Set( "cdkey1", "" );
			trap_Cvar_Set( "cdkey2", "" );
			trap_Cvar_Set( "cdkey3", "" );
			trap_Cvar_Set( "cdkey4", "" );
			if ( strlen( buff ) == CDKEY_LEN ) {
				Q_strncpyz( out, buff, 5 );
				trap_Cvar_Set( "cdkey1", out );
				Q_strncpyz( out, buff + 4, 5 );
				trap_Cvar_Set( "cdkey2", out );
				Q_strncpyz( out, buff + 8, 5 );
				trap_Cvar_Set( "cdkey3", out );
				Q_strncpyz( out, buff + 12, 5 );
				trap_Cvar_Set( "cdkey4", out );
			}

		} else if ( Q_stricmp( name, "verifyCDKey" ) == 0 ) {
			buff[0] = '\0';
			Q_strcat( buff, 1024, UI_Cvar_VariableString( "cdkey1" ) );
			Q_strcat( buff, 1024, UI_Cvar_VariableString( "cdkey2" ) );
			Q_strcat( buff, 1024, UI_Cvar_VariableString( "cdkey3" ) );
			Q_strcat( buff, 1024, UI_Cvar_VariableString( "cdkey4" ) );
			trap_Cvar_Set( "cdkey", buff );
			if ( trap_VerifyCDKey( buff, UI_Cvar_VariableString( "cdkeychecksum" ) ) ) {
				trap_Cvar_Set( "ui_cdkeyvalid", "CD Key Appears to be valid." );
				trap_SetCDKey( buff );
			} else {
				trap_Cvar_Set( "ui_cdkeyvalid", "CD Key does not appear to be valid." );
			}
			//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
		} else if ( Q_stricmp( name, "loadArenas" ) == 0 ) {
			UI_LoadArenas();
			UI_MapCountByGameType( qfalse );
			Menu_SetFeederSelection( NULL, FEEDER_ALLMAPS, 0, "createserver" );
			//#endif	// #ifdef MISSIONPACK
		} else if ( Q_stricmp( name, "saveControls" ) == 0 ) {
			Controls_SetConfig( qtrue );
		} else if ( Q_stricmp( name, "loadControls" ) == 0 ) {
			Controls_GetConfig();
		} else if ( Q_stricmp( name, "clearError" ) == 0 ) {
			trap_Cvar_Set( "com_errorMessage", "" );
			//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
		} else if ( Q_stricmp( name, "loadGameInfo" ) == 0 ) {
			UI_ParseGameInfo( "gameinfo.txt" );
			UI_LoadBestScores( uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum );
			//#endif	// #ifdef MISSIONPACK
		} else if ( Q_stricmp( name, "resetScores" ) == 0 ) {
			UI_ClearScores();
			//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
		} else if ( Q_stricmp( name, "RefreshServers" ) == 0 ) {
			UI_StartServerRefresh( qtrue );
			UI_BuildServerDisplayList( qtrue );
		} else if ( Q_stricmp( name, "RefreshFilter" ) == 0 ) {
			UI_StartServerRefresh( qfalse );
			UI_BuildServerDisplayList( qtrue );
		} else if ( Q_stricmp( name, "RunSPDemo" ) == 0 ) {
			if ( uiInfo.demoAvailable ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo %s_%i", uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum ) );
			}
			//#endif	// #ifdef MISSIONPACK`
		} else if ( Q_stricmp( name, "LoadDemos" ) == 0 ) {
			UI_LoadDemos();
		} else if ( Q_stricmp( name, "LoadMovies" ) == 0 ) {
			UI_LoadMovies();

			//----(SA)	added
		} else if ( Q_stricmp( name, "LoadSaveGames" ) == 0 ) {  // get the list
			UI_LoadSavegames( NULL );
		} else if ( Q_stricmp( name, "Loadgame" ) == 0 ) {
			int i = UI_SavegameIndexFromName2( ui_savegameName.string );
			// in developer, don't actually load the game
			if ( DC->getCVarValue( "developer" ) ) {
				Com_Printf( "would load game (developer 0):\n   %s\n", uiInfo.savegameList[i].savegameFile );
			} else {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "loadgame %s\n", uiInfo.savegameList[i].savegameFile ) );
			}

			// save.  throw dialog box if file exists
		} else if ( Q_stricmp( name, "Savegame" ) == 0 ) {
			int i;
			char name[MAX_NAME_LENGTH];

			name[0] = '\0';
			Q_strncpyz( name, UI_Cvar_VariableString( "ui_savegame" ), MAX_NAME_LENGTH );

			if ( !strlen( name ) ) {
				Menus_OpenByName( "save_name_popmenu" );
			} else {
				// find out if there's an existing savegame with that name
				for ( i = 0; i < uiInfo.savegameCount; i++ ) {
					if ( Q_stricmp( name, uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[i]].savegameName ) == 0 ) {
						Menus_OpenByName( "save_overwrite_popmenu" );
						break;
					}
				}

				if ( i == uiInfo.savegameCount ) {
					trap_Cmd_ExecuteText( EXEC_APPEND, va( "savegame %s\n", UI_Cvar_VariableString( "ui_savegame" ), MAX_NAME_LENGTH ) );
					Menus_CloseAll();
				}
			}
			// save with no confirm for overwrite
		} else if ( Q_stricmp( name, "Savegame2" ) == 0 ) {
			if ( !strlen( name ) ) {
				Menus_OpenByName( "save_name_popmenu" );
			} else {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "savegame %s\n", UI_Cvar_VariableString( "ui_savegame" ), MAX_NAME_LENGTH ) );
				Menus_CloseAll();
			}
		} else if ( Q_stricmp( name, "DelSavegame" ) == 0 ) {
			int i = UI_SavegameIndexFromName2( ui_savegameName.string );
			if ( DC->getCVarValue( "developer" ) ) {
				Com_Printf( "would delete game (developer 0):\n   %s\n", uiInfo.savegameList[i].savegameFile );
			} else {
				UI_DelSavegame();
			}
		} else if ( Q_stricmp( name, "SavegameSort" ) == 0 ) {
			int sortColumn;
			if ( Int_Parse( args, &sortColumn ) ) {
				// if same column we're already sorting on then flip the direction
				if ( sortColumn == uiInfo.savegameStatus.sortKey ) {
					uiInfo.savegameStatus.sortDir = !uiInfo.savegameStatus.sortDir;
				}
				// make sure we sort again
				UI_SavegameSort( sortColumn, qtrue );
			}
			//----(SA)	end

			//----(SA)	added
		} else if ( Q_stricmp( name, "playerstart" ) == 0 ) {
			trap_Cmd_ExecuteText( EXEC_APPEND, "fade 0 0 0 0 3\n" );    // fade screen up
			trap_Cvar_Set( "g_playerstart", "1" );                 // set cvar which will trigger "playerstart" in script
			Menus_CloseAll();
			//----(SA)	end

		} else if ( Q_stricmp( name, "LoadMods" ) == 0 ) {
			UI_LoadMods();
		} else if ( Q_stricmp( name, "playMovie" ) == 0 ) {
			if ( uiInfo.previewMovie >= 0 ) {
				trap_CIN_StopCinematic( uiInfo.previewMovie );
			}
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "cinematic %s.roq 2\n", uiInfo.movieList[uiInfo.movieIndex] ) );
		} else if ( Q_stricmp( name, "RunMod" ) == 0 ) {
			trap_Cvar_Set( "fs_game", uiInfo.modList[uiInfo.modIndex].modName );
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
		} else if ( Q_stricmp( name, "RunDemo" ) == 0 ) {
			trap_Cmd_ExecuteText( EXEC_APPEND, va( "demo %s\n", uiInfo.demoList[uiInfo.demoIndex] ) );
		} else if ( Q_stricmp( name, "Wolf" ) == 0 ) {
			trap_Cvar_Set( "fs_game", "" );
			trap_Cmd_ExecuteText( EXEC_APPEND, "vid_restart;" );
			//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
		} else if ( Q_stricmp( name, "closeJoin" ) == 0 ) {
			if ( uiInfo.serverStatus.refreshActive ) {
				UI_StopServerRefresh();
				uiInfo.serverStatus.nextDisplayRefresh = 0;
				uiInfo.nextServerStatusRefresh = 0;
				uiInfo.nextFindPlayerRefresh = 0;
				UI_BuildServerDisplayList( qtrue );
			} else {
				Menus_CloseByName( "joinserver" );
				Menus_OpenByName( "main" );
			}
			//#endif
		} else if ( Q_stricmp( name, "StopRefresh" ) == 0 ) {
			//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
			UI_StopServerRefresh();
			uiInfo.serverStatus.nextDisplayRefresh = 0;
			uiInfo.nextServerStatusRefresh = 0;
			uiInfo.nextFindPlayerRefresh = 0;
		} else if ( Q_stricmp( name, "UpdateFilter" ) == 0 ) {
			if ( ui_netSource.integer == AS_LOCAL ) {
				UI_StartServerRefresh( qtrue );
			}
			UI_BuildServerDisplayList( qtrue );
			UI_FeederSelection( FEEDER_SERVERS, 0 );
		} else if ( Q_stricmp( name, "ServerStatus" ) == 0 ) {
			trap_LAN_GetServerAddressString( ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], uiInfo.serverStatusAddress, sizeof( uiInfo.serverStatusAddress ) );
			UI_BuildServerStatus( qtrue );
		} else if ( Q_stricmp( name, "FoundPlayerServerStatus" ) == 0 ) {
			Q_strncpyz( uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof( uiInfo.serverStatusAddress ) );
			UI_BuildServerStatus( qtrue );
			Menu_SetFeederSelection( NULL, FEEDER_FINDPLAYER, 0, NULL );
		} else if ( Q_stricmp( name, "FindPlayer" ) == 0 ) {
			UI_BuildFindPlayerList( qtrue );
			// clear the displayed server status info
			uiInfo.serverStatusInfo.numLines = 0;
			Menu_SetFeederSelection( NULL, FEEDER_FINDPLAYER, 0, NULL );
		} else if ( Q_stricmp( name, "JoinServer" ) == 0 ) {
			trap_Cvar_Set( "cg_thirdPerson", "0" );
			trap_Cvar_Set( "cg_cameraOrbit", "0" );
			trap_Cvar_Set( "ui_singlePlayerActive", "0" );
			if ( uiInfo.serverStatus.currentServer >= 0 && uiInfo.serverStatus.currentServer < uiInfo.serverStatus.numDisplayServers ) {
				trap_LAN_GetServerAddressString( ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, 1024 );
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", buff ) );
			}
		} else if ( Q_stricmp( name, "FoundPlayerJoinServer" ) == 0 ) {
			trap_Cvar_Set( "ui_singlePlayerActive", "0" );
			if ( uiInfo.currentFoundPlayerServer >= 0 && uiInfo.currentFoundPlayerServer < uiInfo.numFoundPlayerServers ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "connect %s\n", uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer] ) );
			}
			//#endif	// #ifdef MISSIONPACK
		} else if ( Q_stricmp( name, "Quit" ) == 0 ) {
			trap_Cvar_Set( "ui_singlePlayerActive", "0" );
			trap_Cmd_ExecuteText( EXEC_NOW, "quit" );
		} else if ( Q_stricmp( name, "Controls" ) == 0 ) {
			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "setup_menu2" );
		} else if ( Q_stricmp( name, "Leave" ) == 0 ) {
			trap_Cmd_ExecuteText( EXEC_APPEND, "disconnect\n" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "main" );
		} else if ( Q_stricmp( name, "ServerSort" ) == 0 ) {
			int sortColumn;
			if ( Int_Parse( args, &sortColumn ) ) {
				// if same column we're already sorting on then flip the direction
				if ( sortColumn == uiInfo.serverStatus.sortKey ) {
					uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
				}
				// make sure we sort again
				UI_ServersSort( sortColumn, qtrue );
			}
		} else if ( Q_stricmp( name, "nextSkirmish" ) == 0 ) {
			UI_StartSkirmish( qtrue );
		} else if ( Q_stricmp( name, "SkirmishStart" ) == 0 ) {
			UI_StartSkirmish( qfalse );
		} else if ( Q_stricmp( name, "closeingame" ) == 0 ) {
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();
			//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
		} else if ( Q_stricmp( name, "voteMap" ) == 0 ) {
			if ( ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.mapCount ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote map %s\n",uiInfo.mapList[ui_currentNetMap.integer].mapLoadName ) );
			}
			//#endif	// #ifdef MISSIONPACK
		} else if ( Q_stricmp( name, "voteKick" ) == 0 ) {
			if ( uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote kick %s\n",uiInfo.playerNames[uiInfo.playerIndex] ) );
			}
		} else if ( Q_stricmp( name, "voteGame" ) == 0 ) {
			if ( ui_netGameType.integer >= 0 && ui_netGameType.integer < uiInfo.numGameTypes ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callvote g_gametype %i\n",uiInfo.gameTypes[ui_netGameType.integer].gtEnum ) );
			}
		} else if ( Q_stricmp( name, "voteLeader" ) == 0 ) {
			if ( uiInfo.teamIndex >= 0 && uiInfo.teamIndex < uiInfo.myTeamCount ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "callteamvote leader %s\n",uiInfo.teamNames[uiInfo.teamIndex] ) );
			}
			//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
		} else if ( Q_stricmp( name, "addBot" ) == 0 ) {
			if ( trap_Cvar_VariableValue( "g_gametype" ) >= GT_TEAM ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "addbot %s %i %s\n", uiInfo.characterList[uiInfo.botIndex].name, uiInfo.skillIndex + 1, ( uiInfo.redBlue == 0 ) ? "Red" : "Blue" ) );
			} else {
				// NERVE - SMF - no bots in wolf multiplayer
				//				trap_Cmd_ExecuteText( EXEC_APPEND, va("addbot %s %i %s\n", UI_GetBotNameByNumber(uiInfo.botIndex), uiInfo.skillIndex+1, (uiInfo.redBlue == 0) ? "Red" : "Blue") );
			}
		} else if ( Q_stricmp( name, "addFavorite" ) == 0 ) {
			if ( ui_netSource.integer != AS_FAVORITES ) {
				char name[MAX_NAME_LENGTH];
				char addr[MAX_NAME_LENGTH];
				int res;

				trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS );
				name[0] = addr[0] = '\0';
				Q_strncpyz( name,    Info_ValueForKey( buff, "hostname" ), MAX_NAME_LENGTH );
				Q_strncpyz( addr,    Info_ValueForKey( buff, "addr" ), MAX_NAME_LENGTH );
				if ( strlen( name ) > 0 && strlen( addr ) > 0 ) {
					res = trap_LAN_AddServer( AS_FAVORITES, name, addr );
					if ( res == 0 ) {
						// server already in the list
						Com_Printf( "Favorite already in list\n" );
					} else if ( res == -1 )     {
						// list full
						Com_Printf( "Favorite list full\n" );
					} else {
						// successfully added
						Com_Printf( "Added favorite server %s\n", addr );
					}
				}
			}
		} else if ( Q_stricmp( name, "deleteFavorite" ) == 0 ) {
			if ( ui_netSource.integer == AS_FAVORITES ) {
				char addr[MAX_NAME_LENGTH];
				trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS );
				addr[0] = '\0';
				Q_strncpyz( addr,    Info_ValueForKey( buff, "addr" ), MAX_NAME_LENGTH );
				if ( strlen( addr ) > 0 ) {
					trap_LAN_RemoveServer( AS_FAVORITES, addr );
				}
			}
		} else if ( Q_stricmp( name, "createFavorite" ) == 0 ) {
			if ( ui_netSource.integer == AS_FAVORITES ) {
				char name[MAX_NAME_LENGTH];
				char addr[MAX_NAME_LENGTH];
				int res;

				name[0] = addr[0] = '\0';
				Q_strncpyz( name,    UI_Cvar_VariableString( "ui_favoriteName" ), MAX_NAME_LENGTH );
				Q_strncpyz( addr,    UI_Cvar_VariableString( "ui_favoriteAddress" ), MAX_NAME_LENGTH );
				if ( strlen( name ) > 0 && strlen( addr ) > 0 ) {
					res = trap_LAN_AddServer( AS_FAVORITES, name, addr );
					if ( res == 0 ) {
						// server already in the list
						Com_Printf( "Favorite already in list\n" );
					} else if ( res == -1 )     {
						// list full
						Com_Printf( "Favorite list full\n" );
					} else {
						// successfully added
						Com_Printf( "Added favorite server %s\n", addr );
					}
				}
			}
		} else if ( Q_stricmp( name, "orders" ) == 0 ) {
			const char *orders;
			if ( String_Parse( args, &orders ) ) {
				int selectedPlayer = trap_Cvar_VariableValue( "cg_selectedPlayer" );
				if ( selectedPlayer < uiInfo.myTeamCount ) {
					strcpy( buff, orders );
					trap_Cmd_ExecuteText( EXEC_APPEND, va( buff, uiInfo.teamClientNums[selectedPlayer] ) );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				} else {
					int i;
					for ( i = 0; i < uiInfo.myTeamCount; i++ ) {
						if ( Q_stricmp( UI_Cvar_VariableString( "name" ), uiInfo.teamNames[i] ) == 0 ) {
							continue;
						}
						strcpy( buff, orders );
						trap_Cmd_ExecuteText( EXEC_APPEND, va( buff, uiInfo.teamNames[i] ) );
						trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
					}
				}
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		} else if ( Q_stricmp( name, "voiceOrdersTeam" ) == 0 ) {
			const char *orders;
			if ( String_Parse( args, &orders ) ) {
				int selectedPlayer = trap_Cvar_VariableValue( "cg_selectedPlayer" );
				if ( selectedPlayer == uiInfo.myTeamCount ) {
					trap_Cmd_ExecuteText( EXEC_APPEND, orders );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				}
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
		} else if ( Q_stricmp( name, "voiceOrders" ) == 0 ) {
			const char *orders;
			if ( String_Parse( args, &orders ) ) {
				int selectedPlayer = trap_Cvar_VariableValue( "cg_selectedPlayer" );
				if ( selectedPlayer < uiInfo.myTeamCount ) {
					strcpy( buff, orders );
					trap_Cmd_ExecuteText( EXEC_APPEND, va( buff, uiInfo.teamClientNums[selectedPlayer] ) );
					trap_Cmd_ExecuteText( EXEC_APPEND, "\n" );
				}
				trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
				trap_Key_ClearStates();
				trap_Cvar_Set( "cl_paused", "0" );
				Menus_CloseAll();
			}
			//#endif	// #ifdef MISSIONPACK
		} else if ( Q_stricmp( name, "glCustom" ) == 0 ) {
			trap_Cvar_Set( "ui_glCustom", "4" );
		} else if ( Q_stricmp( name, "update" ) == 0 ) {
			if ( String_Parse( args, &name2 ) ) {
				UI_Update( name2 );
			}
			// NERVE - SMF
//----(SA)	// start other .exe
		} else if ( Q_stricmp( name, "startSingleplayer" ) == 0 ) {  // so it doesn't barf if it gets a mp menu
			trap_Cmd_ExecuteText( EXEC_APPEND, "startMultiplayer\n" );
		} else if ( Q_stricmp( name, "startMultiplayer" ) == 0 ) {
			trap_Cmd_ExecuteText( EXEC_APPEND, "startMultiplayer\n" );
//----(SA)
		} else if ( Q_stricmp( name, "wm_showPickPlayer" ) == 0 ) {
			Menus_CloseAll();
			Menus_OpenByName( "wm_pickplayer" );
		} else if ( Q_stricmp( name, "wm_showPickTeam" ) == 0 ) {
			Menus_CloseAll();
			Menus_OpenByName( "wm_pickteam" );
		} else if ( Q_stricmp( name, "changePlayerType" ) == 0 ) {
			WM_ChangePlayerType();
		} else if ( Q_stricmp( name, "getSpawnPoints" ) == 0 ) {
			WM_GetSpawnPoints();
		} else if ( Q_stricmp( name, "wm_pickitem2" ) == 0 ) {
			const char *param, *param2;
			int selectType = 0, itemIndex = 0;

			if ( String_Parse( args, &param ) &&  String_Parse( args, &param2 ) ) {
				selectType = atoi( param );
				itemIndex = atoi( param2 );
				WM_PickItem( selectType, itemIndex );
			}
		} else if ( Q_stricmp( name, "startMultiplayer" ) == 0 ) {
			int team, playerType, weapon, pistol, item1, i;
			const char *teamStr, *classStr, *weapStr;

			Menus_CloseAll();

			// get cvars
			team = trap_Cvar_VariableValue( "mp_team" );
			playerType = trap_Cvar_VariableValue( "mp_playerType" );
			weapon = trap_Cvar_VariableValue( "mp_weapon" );
			pistol = trap_Cvar_VariableValue( "mp_pistol" );
			item1 = trap_Cvar_VariableValue( "mp_item1" );

			// print center message
			if ( team == 0 ) {
				teamStr = "nazi";
			} else if ( team == 1 ) {
				teamStr = "allied";
			} else {
				teamStr = "spectator";
			}

			if ( playerType == 0 ) {
				classStr = "soldier";
			} else if ( playerType == 1 ) {
				classStr = "medic";
			} else if ( playerType == 2 ) {
				classStr = "engineer";
			} else {
				classStr = "lieutenant";
			}

			weapStr = "";
			for ( i = 0; weaponTypes[i].name; i++ ) {
				if ( weaponTypes[i].value == weapon ) {
					weapStr = weaponTypes[i].desc;
				}
			}

			if ( team != 2 ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "limbomessage %s %s %s\n", teamStr, classStr, weapStr ) );
			}

			// join team
			if ( team == 0 ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "team %s %i %i %i %i 1\n", "red", playerType, weapon, pistol, item1 ) );
			} else if ( team == 1 ) {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "team %s %i %i %i %i 1\n", "blue", playerType, weapon, pistol, item1 ) );
			} else {
				trap_Cmd_ExecuteText( EXEC_APPEND, va( "team %s %i %i %i %i 1\n", "s", playerType, weapon, pistol, item1 ) );
			}

		} else if ( Q_stricmp( name, "limboChat" ) == 0 ) {
			WM_LimboChat();
		} else if ( Q_stricmp( name, "activateLimboChat" ) == 0 ) {
			WM_ActivateLimboChat();
			// -NERVE - SMF
		} else if ( Q_stricmp( name, "setrecommended" ) == 0 ) {
			trap_Cmd_ExecuteText( EXEC_APPEND, "setRecommended 1\n" );
		} else {
			Com_Printf( "unknown UI script %s\n", name );
		}
	}
}

static void UI_GetTeamColor( vec4_t *color ) {
}

/*
==================
UI_MapCountByGameType
==================
*/
static int UI_MapCountByGameType( qboolean singlePlayer ) {
	int i, c, game;
	c = 0;
	game = singlePlayer ? uiInfo.gameTypes[ui_gameType.integer].gtEnum : uiInfo.gameTypes[ui_netGameType.integer].gtEnum;
	if ( game == GT_SINGLE_PLAYER ) {
		game++;
	}
	if ( game == GT_TEAM ) {
		game = GT_FFA;
	}

	for ( i = 0; i < uiInfo.mapCount; i++ ) {
		uiInfo.mapList[i].active = qfalse;
		if ( uiInfo.mapList[i].typeBits & ( 1 << game ) ) {
			if ( singlePlayer ) {
				if ( !( uiInfo.mapList[i].typeBits & ( 1 << GT_SINGLE_PLAYER ) ) ) {
					continue;
				}
			}
			c++;
			uiInfo.mapList[i].active = qtrue;
		}
	}
	return c;
}

/*
==================
UI_InsertServerIntoDisplayList
==================
*/
static void UI_InsertServerIntoDisplayList( int num, int position ) {
	int i;

	if ( position < 0 || position > uiInfo.serverStatus.numDisplayServers ) {
		return;
	}
	//
	uiInfo.serverStatus.numDisplayServers++;
	for ( i = uiInfo.serverStatus.numDisplayServers; i > position; i-- ) {
		uiInfo.serverStatus.displayServers[i] = uiInfo.serverStatus.displayServers[i - 1];
	}
	uiInfo.serverStatus.displayServers[position] = num;
}

/*
==================
UI_RemoveServerFromDisplayList
==================
*/
static void UI_RemoveServerFromDisplayList( int num ) {
	int i, j;

	for ( i = 0; i < uiInfo.serverStatus.numDisplayServers; i++ ) {
		if ( uiInfo.serverStatus.displayServers[i] == num ) {
			uiInfo.serverStatus.numDisplayServers--;
			for ( j = i; j < uiInfo.serverStatus.numDisplayServers; j++ ) {
				uiInfo.serverStatus.displayServers[j] = uiInfo.serverStatus.displayServers[j + 1];
			}
			return;
		}
	}
}

/*
==================
UI_BinaryServerInsertion
==================
*/
static void UI_BinaryServerInsertion( int num ) {
	//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
	int mid, offset, res, len;

	// use binary search to insert server
	len = uiInfo.serverStatus.numDisplayServers;
	mid = len;
	offset = 0;
	res = 0;
	while ( mid > 0 ) {
		mid = len >> 1;
		//
		res = trap_LAN_CompareServers( ui_netSource.integer, uiInfo.serverStatus.sortKey,
									   uiInfo.serverStatus.sortDir, num, uiInfo.serverStatus.displayServers[offset + mid] );
		// if equal
		if ( res == 0 ) {
			UI_InsertServerIntoDisplayList( num, offset + mid );
			return;
		}
		// if larger
		else if ( res == 1 ) {
			offset += mid;
			len -= mid;
		}
		// if smaller
		else {
			len -= mid;
		}
	}
	if ( res == 1 ) {
		offset++;
	}
	UI_InsertServerIntoDisplayList( num, offset );
	//#endif	// #ifdef MISSIONPACK
}

/*
==================
UI_BuildServerDisplayList
==================
*/
static void UI_BuildServerDisplayList( qboolean force ) {
	//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
	int i, count, clients, maxClients, ping, game, len, visible;
	char info[MAX_STRING_CHARS];
	//qboolean startRefresh = qtrue;// TTimo: unused
	static int numinvisible;

	game = 0;       // NERVE - SMF - shut up compiler warning

	if ( !( force || uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh ) ) {
		return;
	}
	// if we shouldn't reset
	if ( force == 2 ) {
		force = 0;
	}

	// do motd updates here too
	trap_Cvar_VariableStringBuffer( "cl_motdString", uiInfo.serverStatus.motd, sizeof( uiInfo.serverStatus.motd ) );
	len = strlen( uiInfo.serverStatus.motd );
	if ( len == 0 ) {
		strcpy( uiInfo.serverStatus.motd, "Welcome to Team Arena!" );
		len = strlen( uiInfo.serverStatus.motd );
	}
	if ( len != uiInfo.serverStatus.motdLen ) {
		uiInfo.serverStatus.motdLen = len;
		uiInfo.serverStatus.motdWidth = -1;
	}

	if ( force ) {
		numinvisible = 0;
		// clear number of displayed servers
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		// set list box index to zero
		Menu_SetFeederSelection( NULL, FEEDER_SERVERS, 0, NULL );
		// mark all servers as visible so we store ping updates for them
		trap_LAN_MarkServerVisible( ui_netSource.integer, -1, qtrue );
	}

	// get the server count (comes from the master)
	count = trap_LAN_GetServerCount( ui_netSource.integer );
	if ( count == -1 || ( ui_netSource.integer == AS_LOCAL && count == 0 ) ) {
		// still waiting on a response from the master
		uiInfo.serverStatus.numDisplayServers = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 500;
		return;
	}

	visible = qfalse;
	for ( i = 0; i < count; i++ ) {
		// if we already got info for this server
		if ( !trap_LAN_ServerIsVisible( ui_netSource.integer, i ) ) {
			continue;
		}
		visible = qtrue;
		// get the ping for this server
		ping = trap_LAN_GetServerPing( ui_netSource.integer, i );
		if ( ping > 0 || ui_netSource.integer == AS_FAVORITES ) {

			trap_LAN_GetServerInfo( ui_netSource.integer, i, info, MAX_STRING_CHARS );

			clients = atoi( Info_ValueForKey( info, "clients" ) );
			uiInfo.serverStatus.numPlayersOnServers += clients;

			if ( ui_browserShowEmpty.integer == 0 ) {
				if ( clients == 0 ) {
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}

			if ( ui_browserShowFull.integer == 0 ) {
				maxClients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
				if ( clients == maxClients ) {
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}
			/*			// NERVE - SMF - comment out for now, not recognizing "gametype" properly
			if (uiInfo.joinGameTypes[ui_joinGameType.integer].gtEnum != -1) {
			game = atoi(Info_ValueForKey(info, "gametype"));
			if (game != uiInfo.joinGameTypes[ui_joinGameType.integer].gtEnum) {
			trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
			continue;
			}
			}
			*/

			if ( ui_serverFilterType.integer > 0 ) {
				if ( Q_stricmp( Info_ValueForKey( info, "game" ), serverFilters[ui_serverFilterType.integer].basedir ) != 0 ) {
					trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
					continue;
				}
			}
			// make sure we never add a favorite server twice
			if ( ui_netSource.integer == AS_FAVORITES ) {
				UI_RemoveServerFromDisplayList( i );
			}
			// insert the server into the list
			UI_BinaryServerInsertion( i );
			// done with this server
			if ( ping > 0 ) {
				trap_LAN_MarkServerVisible( ui_netSource.integer, i, qfalse );
				numinvisible++;
			}
		}
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime;

	// if there were no servers visible for ping updates
	if ( !visible ) {
		//		UI_StopServerRefresh();
		//		uiInfo.serverStatus.nextDisplayRefresh = 0;
	}
	//#endif	// #ifdef MISSIONPACK
}

typedef struct
{
	char *name, *altName;
} serverStatusCvar_t;

serverStatusCvar_t serverStatusCvars[] = {
	{"sv_hostname", "Name"},
	{"Address", ""},
	{"gamename", "Game name"},
	{"g_gametype", "Game type"},
	{"mapname", "Map"},
	{"version", ""},
	{"protocol", ""},
	{"timelimit", ""},
	{"fraglimit", ""},
	{NULL, NULL}
};

/*
==================
UI_SortServerStatusInfo
==================
*/
static void UI_SortServerStatusInfo( serverStatusInfo_t *info ) {
	int i, j, index;
	char *tmp1, *tmp2;

	// FIXME: if "gamename" == "baseq3" or "missionpack" then
	// replace the gametype number by FFA, CTF etc.
	//
	index = 0;
	for ( i = 0; serverStatusCvars[i].name; i++ ) {
		for ( j = 0; j < info->numLines; j++ ) {
			if ( !info->lines[j][1] || info->lines[j][1][0] ) {
				continue;
			}
			if ( !Q_stricmp( serverStatusCvars[i].name, info->lines[j][0] ) ) {
				// swap lines
				tmp1 = info->lines[index][0];
				tmp2 = info->lines[index][3];
				info->lines[index][0] = info->lines[j][0];
				info->lines[index][3] = info->lines[j][3];
				info->lines[j][0] = tmp1;
				info->lines[j][3] = tmp2;
				//
				if ( strlen( serverStatusCvars[i].altName ) ) {
					info->lines[index][0] = serverStatusCvars[i].altName;
				}
				index++;
			}
		}
	}
}

/*
==================
UI_GetServerStatusInfo
==================
*/
static int UI_GetServerStatusInfo( const char *serverAddress, serverStatusInfo_t *info ) {
	//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
	char *p, *score, *ping, *name;
	int i, len;

	if ( !info ) {
		trap_LAN_ServerStatus( serverAddress, NULL, 0 );
		return qfalse;
	}
	memset( info, 0, sizeof( *info ) );
	if ( trap_LAN_ServerStatus( serverAddress, info->text, sizeof( info->text ) ) ) {
		Q_strncpyz( info->address, serverAddress, sizeof( info->address ) );
		p = info->text;
		info->numLines = 0;
		info->lines[info->numLines][0] = "Address";
		info->lines[info->numLines][1] = "";
		info->lines[info->numLines][2] = "";
		info->lines[info->numLines][3] = info->address;
		info->numLines++;
		// get the cvars
		while ( p && *p ) {
			p = strchr( p, '\\' );
			if ( !p ) {
				break;
			}
			*p++ = '\0';
			if ( *p == '\\' ) {
				break;
			}
			info->lines[info->numLines][0] = p;
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			p = strchr( p, '\\' );
			if ( !p ) {
				break;
			}
			*p++ = '\0';
			info->lines[info->numLines][3] = p;

			info->numLines++;
			if ( info->numLines >= MAX_SERVERSTATUS_LINES ) {
				break;
			}
		}
		// get the player list
		if ( info->numLines < MAX_SERVERSTATUS_LINES - 3 ) {
			// empty line
			info->lines[info->numLines][0] = "";
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			info->lines[info->numLines][3] = "";
			info->numLines++;
			// header
			info->lines[info->numLines][0] = "num";
			info->lines[info->numLines][1] = "score";
			info->lines[info->numLines][2] = "ping";
			info->lines[info->numLines][3] = "name";
			info->numLines++;
			// parse players
			i = 0;
			len = 0;
			while ( p && *p ) {
				if ( *p == '\\' ) {
					*p++ = '\0';
				}
				if ( !p ) {
					break;
				}
				score = p;
				p = strchr( p, ' ' );
				if ( !p ) {
					break;
				}
				*p++ = '\0';
				ping = p;
				p = strchr( p, ' ' );
				if ( !p ) {
					break;
				}
				*p++ = '\0';
				name = p;
				Com_sprintf( &info->pings[len], sizeof( info->pings ) - len, "%d", i );
				info->lines[info->numLines][0] = &info->pings[len];
				len += strlen( &info->pings[len] ) + 1;
				info->lines[info->numLines][1] = score;
				info->lines[info->numLines][2] = ping;
				info->lines[info->numLines][3] = name;
				info->numLines++;
				if ( info->numLines >= MAX_SERVERSTATUS_LINES ) {
					break;
				}
				p = strchr( p, '\\' );
				if ( !p ) {
					break;
				}
				*p++ = '\0';
				//
				i++;
			}
		}
		UI_SortServerStatusInfo( info );
		return qtrue;
	}
	//#endif	// #ifdef MISSIONPACK
	return qfalse;
}

/*
==================
stristr
==================
*/
static char *stristr( char *str, char *charset ) {
	int i;

	while ( *str ) {
		for ( i = 0; charset[i] && str[i]; i++ ) {
			if ( toupper( charset[i] ) != toupper( str[i] ) ) {
				break;
			}
		}
		if ( !charset[i] ) {
			return str;
		}
		str++;
	}
	return NULL;
}

/*
==================
UI_BuildFindPlayerList
==================
*/
static void UI_BuildFindPlayerList( qboolean force ) {
	//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
	static int numFound, numTimeOuts;
	int i, j, resend;
	serverStatusInfo_t info;
	char name[MAX_NAME_LENGTH + 2];
	char infoString[MAX_STRING_CHARS];

	if ( !force ) {
		if ( !uiInfo.nextFindPlayerRefresh || uiInfo.nextFindPlayerRefresh > uiInfo.uiDC.realTime ) {
			return;
		}
	} else {
		memset( &uiInfo.pendingServerStatus, 0, sizeof( uiInfo.pendingServerStatus ) );
		uiInfo.numFoundPlayerServers = 0;
		uiInfo.currentFoundPlayerServer = 0;
		trap_Cvar_VariableStringBuffer( "ui_findPlayer", uiInfo.findPlayerName, sizeof( uiInfo.findPlayerName ) );
		Q_CleanStr( uiInfo.findPlayerName );
		// should have a string of some length
		if ( !strlen( uiInfo.findPlayerName ) ) {
			uiInfo.nextFindPlayerRefresh = 0;
			return;
		}
		// set resend time
		resend = ui_serverStatusTimeOut.integer / 2 - 10;
		if ( resend < 50 ) {
			resend = 50;
		}
		trap_Cvar_Set( "cl_serverStatusResendTime", va( "%d", resend ) );
		// reset all server status requests
		trap_LAN_ServerStatus( NULL, NULL, 0 );
		//
		uiInfo.numFoundPlayerServers = 1;
		Com_sprintf( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1],
					 sizeof( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1] ),
					 "searching %d...", uiInfo.pendingServerStatus.num );
		numFound = 0;
		numTimeOuts++;
	}
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		// if this pending server is valid
		if ( uiInfo.pendingServerStatus.server[i].valid ) {
			// try to get the server status for this server
			if ( UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, &info ) ) {
				//
				numFound++;
				// parse through the server status lines
				for ( j = 0; j < info.numLines; j++ ) {
					// should have ping info
					if ( !info.lines[j][2] || !info.lines[j][2][0] ) {
						continue;
					}
					// clean string first
					Q_strncpyz( name, info.lines[j][3], sizeof( name ) );
					Q_CleanStr( name );
					// if the player name is a substring
					if ( stristr( name, uiInfo.findPlayerName ) ) {
						// add to found server list if we have space (always leave space for a line with the number found)
						if ( uiInfo.numFoundPlayerServers < MAX_FOUNDPLAYER_SERVERS - 1 ) {
							//
							Q_strncpyz( uiInfo.foundPlayerServerAddresses[uiInfo.numFoundPlayerServers - 1],
										uiInfo.pendingServerStatus.server[i].adrstr,
										sizeof( uiInfo.foundPlayerServerAddresses[0] ) );
							Q_strncpyz( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1],
										uiInfo.pendingServerStatus.server[i].name,
										sizeof( uiInfo.foundPlayerServerNames[0] ) );
							uiInfo.numFoundPlayerServers++;
						} else {
							// can't add any more so we're done
							uiInfo.pendingServerStatus.num = uiInfo.serverStatus.numDisplayServers;
						}
					}
				}
				Com_sprintf( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1],
							 sizeof( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1] ),
							 "searching %d/%d...", uiInfo.pendingServerStatus.num, numFound );
				// retrieved the server status so reuse this spot
				uiInfo.pendingServerStatus.server[i].valid = qfalse;
			}
		}
		// if empty pending slot or timed out
		if ( !uiInfo.pendingServerStatus.server[i].valid ||
			 uiInfo.pendingServerStatus.server[i].startTime < uiInfo.uiDC.realTime - ui_serverStatusTimeOut.integer ) {
			if ( uiInfo.pendingServerStatus.server[i].valid ) {
				numTimeOuts++;
			}
			// reset server status request for this address
			UI_GetServerStatusInfo( uiInfo.pendingServerStatus.server[i].adrstr, NULL );
			// reuse pending slot
			uiInfo.pendingServerStatus.server[i].valid = qfalse;
			// if we didn't try to get the status of all servers in the main browser yet
			if ( uiInfo.pendingServerStatus.num < uiInfo.serverStatus.numDisplayServers ) {
				uiInfo.pendingServerStatus.server[i].startTime = uiInfo.uiDC.realTime;
				trap_LAN_GetServerAddressString( ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num],
												 uiInfo.pendingServerStatus.server[i].adrstr, sizeof( uiInfo.pendingServerStatus.server[i].adrstr ) );
				trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num], infoString, sizeof( infoString ) );
				Q_strncpyz( uiInfo.pendingServerStatus.server[i].name, Info_ValueForKey( infoString, "hostname" ), sizeof( uiInfo.pendingServerStatus.server[0].name ) );
				uiInfo.pendingServerStatus.server[i].valid = qtrue;
				uiInfo.pendingServerStatus.num++;
				Com_sprintf( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1],
							 sizeof( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1] ),
							 "searching %d/%d...", uiInfo.pendingServerStatus.num, numFound );
			}
		}
	}
	for ( i = 0; i < MAX_SERVERSTATUSREQUESTS; i++ ) {
		if ( uiInfo.pendingServerStatus.server[i].valid ) {
			break;
		}
	}
	// if still trying to retrieve server status info
	if ( i < MAX_SERVERSTATUSREQUESTS ) {
		uiInfo.nextFindPlayerRefresh = uiInfo.uiDC.realTime + 25;
	} else {
		// add a line that shows the number of servers found
		if ( !uiInfo.numFoundPlayerServers ) {
			Com_sprintf( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1], sizeof( uiInfo.foundPlayerServerAddresses[0] ), "no servers found" );
		} else {
			Com_sprintf( uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1], sizeof( uiInfo.foundPlayerServerAddresses[0] ),
						 "%d server%s found with player %s", uiInfo.numFoundPlayerServers - 1,
						 uiInfo.numFoundPlayerServers == 2 ? "" : "s", uiInfo.findPlayerName );
		}
		uiInfo.nextFindPlayerRefresh = 0;
		// show the server status info for the selected server
		UI_FeederSelection( FEEDER_FINDPLAYER, uiInfo.currentFoundPlayerServer );
	}
	//#endif	// #ifdef MISSIONPACK
}

/*
==================
UI_BuildServerStatus
==================
*/
static void UI_BuildServerStatus( qboolean force ) {
	//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
	if ( uiInfo.nextFindPlayerRefresh ) {
		return;
	}
	if ( !force ) {
		if ( !uiInfo.nextServerStatusRefresh || uiInfo.nextServerStatusRefresh > uiInfo.uiDC.realTime ) {
			return;
		}
	} else {
		Menu_SetFeederSelection( NULL, FEEDER_SERVERSTATUS, 0, NULL );
		uiInfo.serverStatusInfo.numLines = 0;
		// reset all server status requests
		trap_LAN_ServerStatus( NULL, NULL, 0 );
	}
	if ( uiInfo.serverStatus.currentServer < 0 || uiInfo.serverStatus.currentServer > uiInfo.serverStatus.numDisplayServers || uiInfo.serverStatus.numDisplayServers == 0 ) {
		return;
	}
	if ( UI_GetServerStatusInfo( uiInfo.serverStatusAddress, &uiInfo.serverStatusInfo ) ) {
		uiInfo.nextServerStatusRefresh = 0;
		UI_GetServerStatusInfo( uiInfo.serverStatusAddress, NULL );
	} else {
		uiInfo.nextServerStatusRefresh = uiInfo.uiDC.realTime + 500;
	}
	//#endif	// #ifdef MISSIONPACK
}



/*
==================
UI_FeederCount
==================
*/
static int UI_FeederCount( float feederID ) {
	if ( feederID == FEEDER_HEADS ) {
		return uiInfo.characterCount;
	} else if ( feederID == FEEDER_Q3HEADS ) {
		return uiInfo.q3HeadCount;
	} else if ( feederID == FEEDER_CINEMATICS ) {
		return uiInfo.movieCount;
	} else if ( feederID == FEEDER_SAVEGAMES ) {
		return uiInfo.savegameCount;
	} else if ( feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS ) {
		return UI_MapCountByGameType( feederID == FEEDER_MAPS ? qtrue : qfalse );
	} else if ( feederID == FEEDER_SERVERS ) {
		return uiInfo.serverStatus.numDisplayServers;
	} else if ( feederID == FEEDER_SERVERSTATUS ) {
		return uiInfo.serverStatusInfo.numLines;
	} else if ( feederID == FEEDER_FINDPLAYER ) {
		return uiInfo.numFoundPlayerServers;
	} else if ( feederID == FEEDER_PLAYER_LIST ) {
		if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh ) {
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}
		return uiInfo.playerCount;
	} else if ( feederID == FEEDER_TEAM_LIST ) {
		if ( uiInfo.uiDC.realTime > uiInfo.playerRefresh ) {
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}
		return uiInfo.myTeamCount;
	} else if ( feederID == FEEDER_MODS ) {
		return uiInfo.modCount;
	} else if ( feederID == FEEDER_DEMOS ) {
		return uiInfo.demoCount;
		// NERVE - SMF
	} else if ( feederID == FEEDER_PICKSPAWN ) {
		return uiInfo.spawnCount;
	}
	// -NERVE - SMF
	return 0;
}

static const char *UI_SelectedMap( int index, int *actual ) {
	int i, c;
	c = 0;
	*actual = 0;
	for ( i = 0; i < uiInfo.mapCount; i++ ) {
		if ( uiInfo.mapList[i].active ) {
			if ( c == index ) {
				*actual = i;
				return uiInfo.mapList[i].mapName;
			} else {
				c++;
			}
		}
	}
	return "";
}

static int UI_GetIndexFromSelection( int actual ) {
	int i, c;
	c = 0;
	for ( i = 0; i < uiInfo.mapCount; i++ ) {
		if ( uiInfo.mapList[i].active ) {
			if ( i == actual ) {
				return c;
			}
			c++;
		}
	}
	return 0;
}

static void UI_UpdatePendingPings() {
	//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
	trap_LAN_ResetPings( ui_netSource.integer );
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	//#endif	// #ifdef MISSIONPACK
}

// NERVE - SMF
static void UI_FeederAddItem( float feederID, const char *name, int index ) {

}
// -NERVE - SMF


/*
==============
UI_FileText
==============
*/
static const char *UI_FileText( char *fileName ) {
	int len;
	fileHandle_t f;
	static char buf[MAX_MENUDEFFILE];

	len = trap_FS_FOpenFile( fileName, &f, FS_READ );
	if ( !f ) {
		return NULL;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );
	return &buf[0];
}

//----(SA)	added
/*
==============
UI_translateString
==============
*/
static const char *UI_translateString( const char *inString ) {
	int i, numStrings;

	numStrings = sizeof( translateStrings ) / sizeof( translateStrings[0] ) - 1;

	for ( i = 0; i < numStrings; i++ ) {
		if ( !translateStrings[i].name || !strlen( translateStrings[i].name ) ) {
			return inString;
		}

		if ( !strcmp( inString, translateStrings[i].name ) ) {
			if ( translateStrings[i].localname && strlen( translateStrings[i].localname ) ) {
				return translateStrings[i].localname;
			}
			break;
		}
	}

	return inString;
}
//----(SA)	end


/*
==============
UI_FeederItemText
==============
*/
static const char *UI_FeederItemText( float feederID, int index, int column, qhandle_t *handle ) {
	static char info[MAX_STRING_CHARS];
	static char hostname[1024];
	static char clientBuff[32];
	static int lastServerColumn = -1, lastSaveColumn = -1;
	static int lastServerTime = 0, lastSaveTime = 0;
	*handle = -1;
	if ( feederID == FEEDER_HEADS ) {
		if ( index >= 0 && index < uiInfo.characterCount ) {
			return uiInfo.characterList[index].name;
		}
	} else if ( feederID == FEEDER_Q3HEADS ) {
		if ( index >= 0 && index < uiInfo.q3HeadCount ) {
			return uiInfo.q3HeadNames[index];
		}
	} else if ( feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS ) {
		int actual;
		return UI_SelectedMap( index, &actual );
		//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
	} else if ( feederID == FEEDER_SERVERS ) {
		if ( index >= 0 && index < uiInfo.serverStatus.numDisplayServers ) {
			int ping, game;
			if ( lastServerColumn != column || lastServerTime > uiInfo.uiDC.realTime + 5000 ) {
				trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS );
				lastServerColumn = column;
				lastServerTime = uiInfo.uiDC.realTime;
			}
			ping = atoi( Info_ValueForKey( info, "ping" ) );
			if ( ping == -1 ) {
				// if we ever see a ping that is out of date, do a server refresh
				// UI_UpdatePendingPings();
			}
			switch ( column ) {
			case SORT_HOST:
				if ( ping <= 0 ) {
					return Info_ValueForKey( info, "addr" );
				} else {
					if ( ui_netSource.integer == AS_LOCAL ) {
						Com_sprintf( hostname, sizeof( hostname ), "%s [%s]", Info_ValueForKey( info, "hostname" ), netnames[atoi( Info_ValueForKey( info, "nettype" ) )] );
						return hostname;
					} else {
						return Info_ValueForKey( info, "hostname" );
					}
				}
			case SORT_MAP: return Info_ValueForKey( info, "mapname" );
			case SORT_CLIENTS:
				Com_sprintf( clientBuff, sizeof( clientBuff ), "%s (%s)", Info_ValueForKey( info, "clients" ), Info_ValueForKey( info, "sv_maxclients" ) );
				return clientBuff;
			case SORT_GAME:
				game = atoi( Info_ValueForKey( info, "gametype" ) );
				if ( game >= 0 && game < numTeamArenaGameTypes ) {
					return teamArenaGameTypes[game];
				} else {
					return "Unknown";
				}
			case SORT_PING:
				if ( ping <= 0 ) {
					return "...";
				} else {
					return Info_ValueForKey( info, "ping" );
				}
			}
		}
	} else if ( feederID == FEEDER_SERVERSTATUS ) {
		if ( index >= 0 && index < uiInfo.serverStatusInfo.numLines ) {
			if ( column >= 0 && column < 4 ) {
				return uiInfo.serverStatusInfo.lines[index][column];
			}
		}
	} else if ( feederID == FEEDER_FINDPLAYER ) {
		if ( index >= 0 && index < uiInfo.numFoundPlayerServers ) {
			//return uiInfo.foundPlayerServerAddresses[index];
			return uiInfo.foundPlayerServerNames[index];
		}
		//#endif	// #ifdef MISSIONPACK
	} else if ( feederID == FEEDER_PLAYER_LIST ) {
		if ( index >= 0 && index < uiInfo.playerCount ) {
			return uiInfo.playerNames[index];
		}
	} else if ( feederID == FEEDER_TEAM_LIST ) {
		if ( index >= 0 && index < uiInfo.myTeamCount ) {
			return uiInfo.teamNames[index];
		}
	} else if ( feederID == FEEDER_MODS ) {
		if ( index >= 0 && index < uiInfo.modCount ) {
			if ( uiInfo.modList[index].modDescr && *uiInfo.modList[index].modDescr ) {
				return uiInfo.modList[index].modDescr;
			} else {
				return uiInfo.modList[index].modName;
			}
		}
	} else if ( feederID == FEEDER_CINEMATICS ) {
		if ( index >= 0 && index < uiInfo.movieCount ) {
			return uiInfo.movieList[index];
		}
	} else if ( feederID == FEEDER_SAVEGAMES ) {
		if ( index >= 0 && index < uiInfo.savegameCount ) {
//			int ping, game;
			if ( lastSaveColumn != column ) {
//				trap_LAN_GetServerInfo(ui_netSource.integer, uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS);
				lastSaveColumn = column;
				lastSaveTime = uiInfo.uiDC.realTime;
			}

			switch ( column ) {
			case SORT_SAVENAME:
				return uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameName;
				break;
			case SORT_SAVETIME:
				return uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].time;
				break;

			}
		}
	} else if ( feederID == FEEDER_DEMOS ) {
		if ( index >= 0 && index < uiInfo.demoCount ) {
			return uiInfo.demoList[index];
		}
	}
	// NERVE - SMF
	else if ( feederID == FEEDER_PICKSPAWN ) {
		return uiInfo.spawnPoints[index];
	}
	// -NERVE - SMF
	return "";
}


static qhandle_t UI_FeederItemImage( float feederID, int index ) {
	if ( feederID == FEEDER_HEADS ) {
		if ( index >= 0 && index < uiInfo.characterCount ) {
			if ( uiInfo.characterList[index].headImage == -1 ) {
				uiInfo.characterList[index].headImage = trap_R_RegisterShaderNoMip( uiInfo.characterList[index].imageName );
			}
			return uiInfo.characterList[index].headImage;
		}
	} else if ( feederID == FEEDER_Q3HEADS ) {
		if ( index >= 0 && index < uiInfo.q3HeadCount ) {
			return uiInfo.q3HeadIcons[index];
		}
	} else if ( feederID == FEEDER_ALLMAPS || feederID == FEEDER_MAPS ) {
		int actual;

		UI_SelectedMap( index, &actual );
		index = actual;
		if ( index >= 0 && index < uiInfo.mapCount ) {
			if ( uiInfo.mapList[index].levelShot == -1 ) {
				uiInfo.mapList[index].levelShot = trap_R_RegisterShaderNoMip( uiInfo.mapList[index].imageName );
			}
			return uiInfo.mapList[index].levelShot;
		}
	} else if ( feederID == FEEDER_SAVEGAMES ) {
		if ( index >= 0 && index < uiInfo.savegameCount ) {
			if ( uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].sshotImage == -1 ) {
				uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].sshotImage = trap_R_RegisterShaderNoMip( va( "levelshots/%s.tga",uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].mapName ) );
			}
			return uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].sshotImage;
		}
	}

	return 0;
}

static void UI_FeederSelection( float feederID, int index ) {
	static char info[MAX_STRING_CHARS];
	if ( feederID == FEEDER_HEADS ) {
		if ( index >= 0 && index < uiInfo.characterCount ) {
			trap_Cvar_Set( "team_model", uiInfo.characterList[index].female ? "janet" : "james" );
			trap_Cvar_Set( "team_headmodel", va( "*%s", uiInfo.characterList[index].name ) );
			updateModel = qtrue;
		}
	} else if ( feederID == FEEDER_Q3HEADS ) {
		if ( index >= 0 && index < uiInfo.q3HeadCount ) {
			trap_Cvar_Set( "model", uiInfo.q3HeadNames[index] );
			trap_Cvar_Set( "headmodel", uiInfo.q3HeadNames[index] );
			updateModel = qtrue;
		}
		//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
	} else if ( feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS ) {
		int actual, map;
		map = ( feederID == FEEDER_ALLMAPS ) ? ui_currentNetMap.integer : ui_currentMap.integer;
		if ( uiInfo.mapList[map].cinematic >= 0 ) {
			trap_CIN_StopCinematic( uiInfo.mapList[map].cinematic );
			uiInfo.mapList[map].cinematic = -1;
		}
		UI_SelectedMap( index, &actual );
		trap_Cvar_Set( "ui_mapIndex", va( "%d", index ) );
		ui_mapIndex.integer = index;

		if ( feederID == FEEDER_MAPS ) {
			ui_currentMap.integer = actual;
			trap_Cvar_Set( "ui_currentMap", va( "%d", actual ) );
			uiInfo.mapList[ui_currentMap.integer].cinematic = trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.mapList[ui_currentMap.integer].mapLoadName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
			UI_LoadBestScores( uiInfo.mapList[ui_currentMap.integer].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum );
			trap_Cvar_Set( "ui_opponentModel", uiInfo.mapList[ui_currentMap.integer].opponentName );
			updateOpponentModel = qtrue;
		} else {
			ui_currentNetMap.integer = actual;
			trap_Cvar_Set( "ui_currentNetMap", va( "%d", actual ) );
			uiInfo.mapList[ui_currentNetMap.integer].cinematic = trap_CIN_PlayCinematic( va( "%s.roq", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		}

	} else if ( feederID == FEEDER_SERVERS ) {
		const char *mapName = NULL;
		uiInfo.serverStatus.currentServer = index;
		trap_LAN_GetServerInfo( ui_netSource.integer, uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS );
		uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip( va( "levelshots/%s", Info_ValueForKey( info, "mapname" ) ) );
		if ( uiInfo.serverStatus.currentServerCinematic >= 0 ) {
			trap_CIN_StopCinematic( uiInfo.serverStatus.currentServerCinematic );
			uiInfo.serverStatus.currentServerCinematic = -1;
		}
		mapName = Info_ValueForKey( info, "mapname" );
		if ( mapName && *mapName ) {
			uiInfo.serverStatus.currentServerCinematic = trap_CIN_PlayCinematic( va( "%s.roq", mapName ), 0, 0, 0, 0, ( CIN_loop | CIN_silent ) );
		}
	} else if ( feederID == FEEDER_SERVERSTATUS ) {
		//
	} else if ( feederID == FEEDER_FINDPLAYER ) {
		uiInfo.currentFoundPlayerServer = index;
		//
		if ( index < uiInfo.numFoundPlayerServers - 1 ) {
			// build a new server status for this server
			Q_strncpyz( uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof( uiInfo.serverStatusAddress ) );
			Menu_SetFeederSelection( NULL, FEEDER_SERVERSTATUS, 0, NULL );
			UI_BuildServerStatus( qtrue );
		}
		//#endif	// #ifdef MISSIONPACK
	} else if ( feederID == FEEDER_PLAYER_LIST ) {
		uiInfo.playerIndex = index;
	} else if ( feederID == FEEDER_TEAM_LIST ) {
		uiInfo.teamIndex = index;
	} else if ( feederID == FEEDER_MODS ) {
		uiInfo.modIndex = index;
	} else if ( feederID == FEEDER_CINEMATICS ) {
		uiInfo.movieIndex = index;
		if ( uiInfo.previewMovie >= 0 ) {
			trap_CIN_StopCinematic( uiInfo.previewMovie );
		}
		uiInfo.previewMovie = -1;
	} else if ( feederID == FEEDER_SAVEGAMES ) {
//		menuDef_t *menu;
//		itemDef_t *itemdef;

		if ( index >= 0 && index < uiInfo.savegameCount ) {

			if ( DC->getCVarValue( "developer" ) ) {
				Com_Printf( "setting sel save to: %s\n  (savename: %s)\n", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameName,  uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameFile );
			}

			// the text entry box
			trap_Cvar_Set( "ui_savegame", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameName );

			trap_Cvar_Set( "ui_savegameName", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameName );
			trap_Cvar_Set( "ui_savegameInfo", uiInfo.savegameList[uiInfo.savegameStatus.displaySavegames[index]].savegameInfoText );

//		    menu = Menu_GetFocused();
//			itemdef = Menu_FindItemByName( menu, "savename" );
//			itemdef->cursorPos = 0;
		}
	} else if ( feederID == FEEDER_DEMOS ) {
		uiInfo.demoIndex = index;
		// NERVE - SMF
	} else if ( feederID == FEEDER_PICKSPAWN ) {
		trap_Cmd_ExecuteText( EXEC_NOW, va( "setspawnpt %i\n", index ) );
	}
	// -NERVE - SMF
}

// TTimo: unused
/*
static qboolean Team_Parse(char **p) {
	char *token;
	const char *tempStr;
	int i;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {

		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (token[0] == '{') {
			// seven tokens per line, team name and icon, and 5 team member names
			if (!String_Parse(p, &uiInfo.teamList[uiInfo.teamCount].teamName) || !String_Parse(p, &tempStr)) {
				return qfalse;
			}


			uiInfo.teamList[uiInfo.teamCount].imageName = tempStr;
			uiInfo.teamList[uiInfo.teamCount].teamIcon = trap_R_RegisterShaderNoMip(uiInfo.teamList[uiInfo.teamCount].imageName);
			uiInfo.teamList[uiInfo.teamCount].teamIcon_Metal = trap_R_RegisterShaderNoMip(va("%s_metal",uiInfo.teamList[uiInfo.teamCount].imageName));
			uiInfo.teamList[uiInfo.teamCount].teamIcon_Name = trap_R_RegisterShaderNoMip(va("%s_name", uiInfo.teamList[uiInfo.teamCount].imageName));

			uiInfo.teamList[uiInfo.teamCount].cinematic = -1;

			for (i = 0; i < TEAM_MEMBERS; i++) {
				uiInfo.teamList[uiInfo.teamCount].teamMembers[i] = NULL;
				if (!String_Parse(p, &uiInfo.teamList[uiInfo.teamCount].teamMembers[i])) {
					return qfalse;
				}
			}

			Com_Printf("Loaded team %s with team icon %s.\n", uiInfo.teamList[uiInfo.teamCount].teamName, tempStr);
			if (uiInfo.teamCount < MAX_TEAMS) {
				uiInfo.teamCount++;
			} else {
				Com_Printf("Too many teams, last team replaced!\n");
			}
			token = COM_ParseExt(p, qtrue);
			if (token[0] != '}') {
				return qfalse;
			}
		}
	}

	return qfalse;
}

static qboolean Character_Parse(char **p) {
	char *token;
	const char *tempStr;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}


	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (token[0] == '{') {
			// two tokens per line, character name and sex
			if (!String_Parse(p, &uiInfo.characterList[uiInfo.characterCount].name) || !String_Parse(p, &tempStr)) {
				return qfalse;
			}

			uiInfo.characterList[uiInfo.characterCount].headImage = -1;
			uiInfo.characterList[uiInfo.characterCount].imageName = String_Alloc(va("models/players/heads/%s/icon_default.tga", uiInfo.characterList[uiInfo.characterCount].name));

			if (tempStr && (tempStr[0] == 'f' || tempStr[0] == 'F')) {
				uiInfo.characterList[uiInfo.characterCount].female = qtrue;
			} else {
				uiInfo.characterList[uiInfo.characterCount].female = qfalse;
			}

			Com_Printf("Loaded %s character %s.\n", tempStr, uiInfo.characterList[uiInfo.characterCount].name);
			if (uiInfo.characterCount < MAX_HEADS) {
				uiInfo.characterCount++;
			} else {
				Com_Printf("Too many characters, last character replaced!\n");
			}

			token = COM_ParseExt(p, qtrue);
			if (token[0] != '}') {
				return qfalse;
			}
		}
	}

	return qfalse;
}

static qboolean Alias_Parse(char **p) {
	char *token;

	token = COM_ParseExt(p, qtrue);

	if (token[0] != '{') {
		return qfalse;
	}

	while ( 1 ) {
		token = COM_ParseExt(p, qtrue);

		if (Q_stricmp(token, "}") == 0) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if (token[0] == '{') {
			// three tokens per line, character name, bot alias, and preferred action a - all purpose, d - defense, o - offense
			if (!String_Parse(p, &uiInfo.aliasList[uiInfo.aliasCount].name) || !String_Parse(p, &uiInfo.aliasList[uiInfo.aliasCount].ai) || !String_Parse(p, &uiInfo.aliasList[uiInfo.aliasCount].action)) {
				return qfalse;
			}

			Com_Printf("Loaded character alias %s using character ai %s.\n", uiInfo.aliasList[uiInfo.aliasCount].name, uiInfo.aliasList[uiInfo.aliasCount].ai);
			if (uiInfo.aliasCount < MAX_ALIASES) {
				uiInfo.aliasCount++;
			} else {
				Com_Printf("Too many aliases, last alias replaced!\n");
			}

			token = COM_ParseExt(p, qtrue);
			if (token[0] != '}') {
				return qfalse;
			}
		}
	}

	return qfalse;
}
*/

// mode
// 0 - high level parsing
// 1 - team parsing
// 2 - character parsing
// TTimo: unused
/*
static void UI_ParseTeamInfo(const char *teamFile) {
	char	*token;
	char *p;
	char *buff = NULL;
	//int mode = 0; // TTimo: unused

	buff = GetMenuBuffer(teamFile);
	if (!buff) {
		return;
	}

	p = buff;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if( !token || token[0] == 0 || token[0] == '}') {
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if (Q_stricmp(token, "teams") == 0) {

			if (Team_Parse(&p)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token, "characters") == 0) {
			Character_Parse(&p);
		}

		if (Q_stricmp(token, "aliases") == 0) {
			Alias_Parse(&p);
		}

	}

}
*/

/*
==============
GameType_Parse
==============
*/
static qboolean GameType_Parse( char **p, qboolean join ) {
	char *token;

	token = COM_ParseExt( p, qtrue );

	if ( token[0] != '{' ) {
		return qfalse;
	}

	if ( join ) {
		uiInfo.numJoinGameTypes = 0;
	} else {
		uiInfo.numGameTypes = 0;
	}

	while ( 1 ) {
		token = COM_ParseExt( p, qtrue );

		if ( Q_stricmp( token, "}" ) == 0 ) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if ( token[0] == '{' ) {
			// two tokens per line, character name and sex
			if ( join ) {
				if ( !String_Parse( p, &uiInfo.joinGameTypes[uiInfo.numJoinGameTypes].gameType ) || !Int_Parse( p, &uiInfo.joinGameTypes[uiInfo.numJoinGameTypes].gtEnum ) ) {
					return qfalse;
				}
			} else {
				if ( !String_Parse( p, &uiInfo.gameTypes[uiInfo.numGameTypes].gameType ) || !Int_Parse( p, &uiInfo.gameTypes[uiInfo.numGameTypes].gtEnum ) ) {
					return qfalse;
				}
			}

			if ( join ) {
				if ( uiInfo.numJoinGameTypes < MAX_GAMETYPES ) {
					uiInfo.numJoinGameTypes++;
				} else {
					Com_Printf( "Too many net game types, last one replace!\n" );
				}
			} else {
				if ( uiInfo.numGameTypes < MAX_GAMETYPES ) {
					uiInfo.numGameTypes++;
				} else {
					Com_Printf( "Too many game types, last one replace!\n" );
				}
			}

			token = COM_ParseExt( p, qtrue );
			if ( token[0] != '}' ) {
				return qfalse;
			}
		}
	}
	return qfalse;
}

static qboolean MapList_Parse( char **p ) {
	char *token;

	token = COM_ParseExt( p, qtrue );

	if ( token[0] != '{' ) {
		return qfalse;
	}

	uiInfo.mapCount = 0;

	while ( 1 ) {
		token = COM_ParseExt( p, qtrue );

		if ( Q_stricmp( token, "}" ) == 0 ) {
			return qtrue;
		}

		if ( !token || token[0] == 0 ) {
			return qfalse;
		}

		if ( token[0] == '{' ) {
			if ( !String_Parse( p, &uiInfo.mapList[uiInfo.mapCount].mapName ) || !String_Parse( p, &uiInfo.mapList[uiInfo.mapCount].mapLoadName )
				 || !Int_Parse( p, &uiInfo.mapList[uiInfo.mapCount].teamMembers ) ) {
				return qfalse;
			}

			if ( !String_Parse( p, &uiInfo.mapList[uiInfo.mapCount].opponentName ) ) {
				return qfalse;
			}

			uiInfo.mapList[uiInfo.mapCount].typeBits = 0;

			while ( 1 ) {
				token = COM_ParseExt( p, qtrue );
				if ( Q_isnumeric( token[0] ) ) {
					uiInfo.mapList[uiInfo.mapCount].typeBits |= ( 1 << ( token[0] - 0x030 ) );
					if ( !Int_Parse( p, &uiInfo.mapList[uiInfo.mapCount].timeToBeat[token[0] - 0x30] ) ) {
						return qfalse;
					}
				} else {
					break;
				}
			}

			//mapList[mapCount].imageName = String_Alloc(va("levelshots/%s", mapList[mapCount].mapLoadName));
			//if (uiInfo.mapCount == 0) {
			// only load the first cinematic, selection loads the others
			//  uiInfo.mapList[uiInfo.mapCount].cinematic = trap_CIN_PlayCinematic(va("%s.roq",uiInfo.mapList[uiInfo.mapCount].mapLoadName), qfalse, qfalse, qtrue, 0, 0, 0, 0);
			//}
			uiInfo.mapList[uiInfo.mapCount].cinematic = -1;
			uiInfo.mapList[uiInfo.mapCount].levelShot = trap_R_RegisterShaderNoMip( va( "levelshots/%s_small", uiInfo.mapList[uiInfo.mapCount].mapLoadName ) );

			if ( uiInfo.mapCount < MAX_MAPS ) {
				uiInfo.mapCount++;
			} else {
				Com_Printf( "Too many maps, last one replaced!\n" );
			}
		}
	}
	return qfalse;
}

static void UI_ParseGameInfo( const char *teamFile ) {
	char    *token;
	char *p;
	char *buff = NULL;
	//int mode = 0; // TTimo: unused

	buff = GetMenuBuffer( teamFile );
	if ( !buff ) {
		return;
	}

	p = buff;

	while ( 1 ) {
		token = COM_ParseExt( &p, qtrue );
		if ( !token || token[0] == 0 || token[0] == '}' ) {
			break;
		}

		if ( Q_stricmp( token, "}" ) == 0 ) {
			break;
		}

		if ( Q_stricmp( token, "gametypes" ) == 0 ) {

			if ( GameType_Parse( &p, qfalse ) ) {
				continue;
			} else {
				break;
			}
		}

		if ( Q_stricmp( token, "joingametypes" ) == 0 ) {

			if ( GameType_Parse( &p, qtrue ) ) {
				continue;
			} else {
				break;
			}
		}

		if ( Q_stricmp( token, "maps" ) == 0 ) {
			// start a new menu
			MapList_Parse( &p );
		}

	}
}

static void UI_Pause( qboolean b ) {
	if ( b ) {
		// pause the game and set the ui keycatcher
		trap_Cvar_Set( "cl_paused", "1" );
		trap_Key_SetCatcher( KEYCATCH_UI );
	} else {
		// unpause the game and clear the ui keycatcher
		trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
		trap_Key_ClearStates();
		trap_Cvar_Set( "cl_paused", "0" );
	}
}

// TTimo: unused
/*
static int UI_OwnerDraw_Width(int ownerDraw) {
  // TTimo: end of non-void
  return 0;
}
*/

static int UI_PlayCinematic( const char *name, float x, float y, float w, float h ) {
	return trap_CIN_PlayCinematic( name, x, y, w, h, ( CIN_loop | CIN_silent ) );
}

static void UI_StopCinematic( int handle ) {
	if ( handle >= 0 ) {
		trap_CIN_StopCinematic( handle );
	} else {
		handle = abs( handle );
		if ( handle == UI_MAPCINEMATIC ) {
			if ( uiInfo.mapList[ui_currentMap.integer].cinematic >= 0 ) {
				trap_CIN_StopCinematic( uiInfo.mapList[ui_currentMap.integer].cinematic );
				uiInfo.mapList[ui_currentMap.integer].cinematic = -1;
			}
		} else if ( handle == UI_NETMAPCINEMATIC ) {
			if ( uiInfo.serverStatus.currentServerCinematic >= 0 ) {
				trap_CIN_StopCinematic( uiInfo.serverStatus.currentServerCinematic );
				uiInfo.serverStatus.currentServerCinematic = -1;
			}
		} else if ( handle == UI_CLANCINEMATIC ) {
			int i = UI_TeamIndexFromName( UI_Cvar_VariableString( "ui_teamName" ) );
			if ( i >= 0 && i < uiInfo.teamCount ) {
				if ( uiInfo.teamList[i].cinematic >= 0 ) {
					trap_CIN_StopCinematic( uiInfo.teamList[i].cinematic );
					uiInfo.teamList[i].cinematic = -1;
				}
			}
		}
	}
}

static void UI_DrawCinematic( int handle, float x, float y, float w, float h ) {
	trap_CIN_SetExtents( handle, x, y, w, h );
	trap_CIN_DrawCinematic( handle );
}

static void UI_RunCinematicFrame( int handle ) {
	trap_CIN_RunCinematic( handle );
}



/*
=================
PlayerModel_BuildList
=================
*/
static void UI_BuildQ3Model_List( void ) {
	int numdirs;
	int numfiles;
	char dirlist[2048];
	char filelist[2048];
	char skinname[64];
	char*   dirptr;
	char*   fileptr;
	int i;
	int j;
	int dirlen;
	int filelen;

	uiInfo.q3HeadCount = 0;

	// iterate directory of all player models
	numdirs = trap_FS_GetFileList( "models/players", "/", dirlist, 2048 );
	dirptr  = dirlist;
	for ( i = 0; i < numdirs && uiInfo.q3HeadCount < MAX_PLAYERMODELS; i++,dirptr += dirlen + 1 )
	{
		dirlen = strlen( dirptr );

		if ( dirlen && dirptr[dirlen - 1] == '/' ) {
			dirptr[dirlen - 1] = '\0';
		}

		if ( !strcmp( dirptr,"." ) || !strcmp( dirptr,".." ) ) {
			continue;
		}

		// iterate all skin files in directory
		numfiles = trap_FS_GetFileList( va( "models/players/%s",dirptr ), "tga", filelist, 2048 );
		fileptr  = filelist;
		for ( j = 0; j < numfiles && uiInfo.q3HeadCount < MAX_PLAYERMODELS; j++,fileptr += filelen + 1 )
		{
			filelen = strlen( fileptr );

			COM_StripExtension( fileptr,skinname );

			// look for icon_????
			if ( Q_stricmpn( skinname, "icon_", 5 ) == 0 && !( Q_stricmp( skinname,"icon_blue" ) == 0 || Q_stricmp( skinname,"icon_red" ) == 0 ) ) {
				if ( Q_stricmp( skinname, "icon_default" ) == 0 ) {
					Com_sprintf( uiInfo.q3HeadNames[uiInfo.q3HeadCount], sizeof( uiInfo.q3HeadNames[uiInfo.q3HeadCount] ), dirptr );
				} else {
					Com_sprintf( uiInfo.q3HeadNames[uiInfo.q3HeadCount], sizeof( uiInfo.q3HeadNames[uiInfo.q3HeadCount] ), "%s/%s",dirptr, skinname + 5 );
				}
				uiInfo.q3HeadIcons[uiInfo.q3HeadCount++] = trap_R_RegisterShaderNoMip( va( "models/players/%s/%s",dirptr,skinname ) );
			}

		}
	}

}


/*
=================
UI_Init
=================
*/
void _UI_Init( qboolean inGameLoad ) {
	const char *menuSet;
	int start;

	//uiInfo.inGameLoad = inGameLoad;

	UI_RegisterCvars();
	UI_InitMemory();

	// cache redundant calulations
	trap_GetGlconfig( &uiInfo.uiDC.glconfig );

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * ( 1.0 / 480.0 );
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * ( 1.0 / 640.0 );
	if ( uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640 ) {
		// wide screen
		uiInfo.uiDC.bias = 0.5 * ( uiInfo.uiDC.glconfig.vidWidth - ( uiInfo.uiDC.glconfig.vidHeight * ( 640.0 / 480.0 ) ) );
	} else {
		// no wide screen
		uiInfo.uiDC.bias = 0;
	}


	//UI_Load();
	uiInfo.uiDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	uiInfo.uiDC.setColor = &UI_SetColor;
	uiInfo.uiDC.drawHandlePic = &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic = &trap_R_DrawStretchPic;
	uiInfo.uiDC.drawText = &Text_Paint;
	uiInfo.uiDC.textWidth = &Text_Width;
	uiInfo.uiDC.textHeight = &Text_Height;
	uiInfo.uiDC.registerModel = &trap_R_RegisterModel;
	uiInfo.uiDC.modelBounds = &trap_R_ModelBounds;
	uiInfo.uiDC.fillRect = &UI_FillRect;
	uiInfo.uiDC.drawRect = &_UI_DrawRect;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.drawTopBottom = &_UI_DrawTopBottom;
	uiInfo.uiDC.clearScene = &trap_R_ClearScene;
	uiInfo.uiDC.drawSides = &_UI_DrawSides;
	uiInfo.uiDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene = &trap_R_RenderScene;
	uiInfo.uiDC.registerFont = &trap_R_RegisterFont;
	uiInfo.uiDC.ownerDrawItem = &UI_OwnerDraw;
	uiInfo.uiDC.getValue = &UI_GetValue;
	uiInfo.uiDC.ownerDrawVisible = &UI_OwnerDrawVisible;
	uiInfo.uiDC.runScript = &UI_RunMenuScript;
	uiInfo.uiDC.getTeamColor = &UI_GetTeamColor;
	uiInfo.uiDC.setCVar = trap_Cvar_Set;
	uiInfo.uiDC.getCVarString = trap_Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue = trap_Cvar_VariableValue;
	uiInfo.uiDC.drawTextWithCursor = &Text_PaintWithCursor;
	uiInfo.uiDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound = &trap_S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey = &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.feederCount = &UI_FeederCount;
	uiInfo.uiDC.feederItemImage = &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText = &UI_FeederItemText;
	uiInfo.uiDC.fileText = &UI_FileText;    //----(SA)

	uiInfo.uiDC.getTranslatedString = &UI_translateString;  //----(SA) added

	uiInfo.uiDC.feederSelection = &UI_FeederSelection;
	uiInfo.uiDC.feederAddItem = &UI_FeederAddItem;                  // NERVE - SMF
	uiInfo.uiDC.setBinding = &trap_Key_SetBinding;
	uiInfo.uiDC.getBindingBuf = &trap_Key_GetBindingBuf;
	uiInfo.uiDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;
	uiInfo.uiDC.executeText = &trap_Cmd_ExecuteText;
	uiInfo.uiDC.Error = &Com_Error;
	uiInfo.uiDC.Print = &Com_Printf;
	uiInfo.uiDC.Pause = &UI_Pause;
	uiInfo.uiDC.ownerDrawWidth = &UI_OwnerDrawWidth;
	uiInfo.uiDC.registerSound = &trap_S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	uiInfo.uiDC.playCinematic = &UI_PlayCinematic;
	uiInfo.uiDC.stopCinematic = &UI_StopCinematic;
	uiInfo.uiDC.drawCinematic = &UI_DrawCinematic;
	uiInfo.uiDC.runCinematicFrame = &UI_RunCinematicFrame;

	Init_Display( &uiInfo.uiDC );

	String_Init();

	// load translation text
	UI_LoadTranslationStrings();

//	uiInfo.uiDC.cursor	= trap_R_RegisterShaderNoMip( "menu/art/3_cursor3" );
	uiInfo.uiDC.whiteShader = trap_R_RegisterShaderNoMip( "white" );

	AssetCache();

	start = trap_Milliseconds();

	uiInfo.teamCount = 0;
	uiInfo.characterCount = 0;
	uiInfo.aliasCount = 0;

//	UI_ParseTeamInfo("teaminfo.txt");
//	UI_LoadTeams();
//	UI_ParseGameInfo("gameinfo.txt");

	menuSet = UI_Cvar_VariableString( "ui_menuFiles" );
	if ( menuSet == NULL || menuSet[0] == '\0' ) {
#ifdef WOLF_SP_DEMO
		menuSet = "ui/demomenus.txt";
#else
		menuSet = "ui/menus.txt";
#endif
	}

#if 0
	if ( uiInfo.inGameLoad ) {
		UI_LoadMenus( "ui/ingame.txt", qtrue );
	} else {
	}
#else
	UI_LoadMenus( menuSet, qtrue );
	UI_LoadMenus( "ui/ingame.txt", qfalse );
#endif

	Menus_CloseAll();

//#ifdef MISSIONPACK			// NERVE - SMF - enabled for multiplayer
	trap_LAN_LoadCachedServers();
	UI_LoadBestScores( uiInfo.mapList[0].mapLoadName, uiInfo.gameTypes[ui_gameType.integer].gtEnum );
//#endif	// #ifdef MISSIONPACK

	UI_BuildQ3Model_List();
#ifdef MISSIONPACK
	UI_LoadBots();
#endif  // #ifdef MISSIONPACK

	// sets defaults for ui temp cvars
	uiInfo.effectsColor = gamecodetoui[(int)trap_Cvar_VariableValue( "color" ) - 1];
	uiInfo.currentCrosshair = (int)trap_Cvar_VariableValue( "cg_drawCrosshair" );
	trap_Cvar_Set( "ui_mousePitch", ( trap_Cvar_VariableValue( "m_pitch" ) >= 0 ) ? "0" : "1" );

	uiInfo.serverStatus.currentServerCinematic = -1;
	uiInfo.previewMovie = -1;

	if ( trap_Cvar_VariableValue( "ui_WolfFirstRun" ) == 0 ) {
		trap_Cvar_Set( "s_volume", "0.8" );
		trap_Cvar_Set( "s_musicvolume", "0.8" );
		trap_Cvar_Set( "ui_WolfFirstRun", "1" );
	}

	trap_Cvar_Register( NULL, "debug_protocol", "", 0 );
}


/*
=================
UI_KeyEvent
=================
*/
void _UI_KeyEvent( int key, qboolean down ) {

	if ( Menu_Count() > 0 ) {
		menuDef_t *menu = Menu_GetFocused();
		if ( menu ) {
			if ( key == K_ESCAPE && down && !Menus_AnyFullScreenVisible() ) {
				Menus_CloseAll();
			} else {
				Menu_HandleKey( menu, key, down );
			}
		} else {
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
		}
	}

	//if ((s > 0) && (s != menu_null_sound)) {
	//  trap_S_StartLocalSound( s, CHAN_LOCAL_SOUND );
	//}
}

/*
=================
UI_MouseEvent
=================
*/
void _UI_MouseEvent( int dx, int dy ) {
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;
	if ( uiInfo.uiDC.cursorx < 0 ) {
		uiInfo.uiDC.cursorx = 0;
	} else if ( uiInfo.uiDC.cursorx > SCREEN_WIDTH ) {
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;
	}

	uiInfo.uiDC.cursory += dy;
	if ( uiInfo.uiDC.cursory < 0 ) {
		uiInfo.uiDC.cursory = 0;
	} else if ( uiInfo.uiDC.cursory > SCREEN_HEIGHT ) {
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;
	}

	if ( Menu_Count() > 0 ) {
		//menuDef_t *menu = Menu_GetFocused();
		//Menu_HandleMouseMove(menu, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
		Display_MouseMove( NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory );
	}

}

void UI_LoadNonIngame() {
	const char *menuSet = UI_Cvar_VariableString( "ui_menuFiles" );
	if ( menuSet == NULL || menuSet[0] == '\0' ) {
#ifdef WOLF_SP_DEMO
		menuSet = "ui/demomenus.txt";
#else
		menuSet = "ui/menus.txt";
#endif
	}
	UI_LoadMenus( menuSet, qfalse );
	uiInfo.inGameLoad = qfalse;
}



//----(SA)	added
/*
==============
_UI_GetActiveMenu
==============
*/
uiMenuCommand_t _UI_GetActiveMenu( void ) {
	return menutype;
}

//----(SA)	end

void _UI_SetActiveMenu( uiMenuCommand_t menu ) {
	char buf[256];

	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	if ( Menu_Count() > 0 ) {
		vec3_t v;
		v[0] = v[1] = v[2] = 0;

		if ( menu == UIMENU_BRIEFING && menutype == menu ) { // don't let briefing be set multiple times
			return;
		}

		menutype = menu;    //----(SA)	added

		switch ( menu ) {
		case UIMENU_NONE:
			trap_Key_SetCatcher( trap_Key_GetCatcher() & ~KEYCATCH_UI );
			trap_Key_ClearStates();
			trap_Cvar_Set( "cl_paused", "0" );
			Menus_CloseAll();

			return;
		case UIMENU_MAIN:
			//trap_Cvar_Set( "sv_killserver", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			//trap_S_StartLocalSound( trap_S_RegisterSound("sound/misc/menu_background.wav", qfalse) , CHAN_LOCAL_SOUND );
			//trap_S_StartBackgroundTrack("sound/misc/menu_background.wav", NULL);
			if ( uiInfo.inGameLoad ) {
				UI_LoadNonIngame();
			}
			Menus_CloseAll();
			Menus_ActivateByName( "main" );
			trap_Cvar_VariableStringBuffer( "com_errorMessage", buf, sizeof( buf ) );
			if ( strlen( buf ) ) {
#ifdef MISSIONPACK
				if ( !ui_singlePlayerActive.integer ) {
#endif  // #ifdef MISSIONPACK
				Menus_ActivateByName( "error_popmenu" );
#ifdef MISSIONPACK
			} else {
				trap_Cvar_Set( "com_errorMessage", "" );
			}
#endif  // #ifdef MISSIONPACK
			}
			// ensure sound is there for the menu
			trap_S_FadeAllSound( 1.0f, 1000 );    // make sure sound fades up

			// ensure savegames are loadable
			trap_Cvar_Set( "g_reloading", "0" );

			return;

		case UIMENU_TEAM:
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_ActivateByName( "team" );
			return;

//----(SA)	added
		case UIMENU_ENDGAME:
			// ensure sound is there for the menu
			trap_S_FadeAllSound( 1.0f, 1000 );
			// ensure savegames are loadable
			trap_Cvar_Set( "g_reloading", "0" );

			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_ActivateByName( "credit" );
			return;
//----(SA)	end

		case UIMENU_NEED_CD:
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_ActivateByName( "needcd" );
//			UI_ConfirmMenu( "Insert the CD", NULL, NeedCDAction );
			return;

		case UIMENU_BAD_CD_KEY:
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_ActivateByName( "badcd" );
//			UI_ConfirmMenu( "Bad CD Key", NULL, NeedCDKeyAction );
			return;

		case UIMENU_POSTGAME:
			//trap_Cvar_Set( "sv_killserver", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			if ( uiInfo.inGameLoad ) {
				UI_LoadNonIngame();
			}
			Menus_CloseAll();
			Menus_ActivateByName( "endofgame" );
			//UI_ConfirmMenu( "Bad CD Key", NULL, NeedCDKeyAction );
			return;

		case UIMENU_INGAME:
			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			UI_BuildPlayerList();
			Menus_CloseAll();
			Menus_ActivateByName( "ingame" );
			return;

		case UIMENU_PREGAME:
			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "pregame" );
			return;

		case UIMENU_NOTEBOOK:
			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "notebook" );
			return;

		case UIMENU_BOOK1:
		case UIMENU_BOOK2:
		case UIMENU_BOOK3:
//			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( va( "hbook%d", ( menu - UIMENU_BOOK1 ) + 1 ) );
			return;

		case UIMENU_CLIPBOARD:
			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_ActivateByName( "clipboard" );
			return;

//		case UIMENU_HELP:
//			trap_Cvar_Set( "cl_paused", "1" );
//			trap_Key_SetCatcher( KEYCATCH_UI );
//			Menus_CloseAll();
//			Menus_ActivateByName("help");
//			return;

		case UIMENU_BRIEFING:
			Menus_CloseAll();
			Menus_ActivateByName( "briefing" );
			return;

			// NERVE - SMF
		case UIMENU_WM_PICKTEAM:
			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_OpenByName( "wm_pickteam" );
			return;

		case UIMENU_WM_PICKPLAYER:
			trap_Cvar_Set( "cl_paused", "1" );
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_OpenByName( "wm_pickplayer" );
			return;

		case UIMENU_WM_QUICKMESSAGE:
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_OpenByName( "wm_quickmessage" );
			return;

		case UIMENU_WM_LIMBO:
			trap_Key_SetCatcher( KEYCATCH_UI );
			Menus_CloseAll();
			Menus_OpenByName( "wm_limboView" );
			Menus_OpenByName( "wm_limboChat" );
			Menus_OpenByName( "wm_limboModel" );
			Menus_OpenByName( "wm_limboOptions" );
			Menus_OpenByName( "wm_limboButtonBar" );
			return;
			// -NERVE - SMF
		default:
			break;
		}
	}
}

qboolean _UI_IsFullscreen( void ) {
	return Menus_AnyFullScreenVisible();
}



static connstate_t lastConnState;
static char lastLoadingText[MAX_INFO_VALUE];

static void UI_ReadableSize( char *buf, int bufsize, int value ) {
	if ( value > 1024 * 1024 * 1024 ) { // gigs
		Com_sprintf( buf, bufsize, "%d", value / ( 1024 * 1024 * 1024 ) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d GB",
					 ( value % ( 1024 * 1024 * 1024 ) ) * 100 / ( 1024 * 1024 * 1024 ) );
	} else if ( value > 1024 * 1024 ) { // megs
		Com_sprintf( buf, bufsize, "%d", value / ( 1024 * 1024 ) );
		Com_sprintf( buf + strlen( buf ), bufsize - strlen( buf ), ".%02d MB",
					 ( value % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
	} else if ( value > 1024 ) { // kilos
		Com_sprintf( buf, bufsize, "%d KB", value / 1024 );
	} else { // bytes
		Com_sprintf( buf, bufsize, "%d bytes", value );
	}
}

// Assumes time is in msec
static void UI_PrintTime( char *buf, int bufsize, int time ) {
	time /= 1000;  // change to seconds

	if ( time > 3600 ) { // in the hours range
		Com_sprintf( buf, bufsize, "%d hr %d min", time / 3600, ( time % 3600 ) / 60 );
	} else if ( time > 60 ) { // mins
		Com_sprintf( buf, bufsize, "%d min %d sec", time / 60, time % 60 );
	} else  { // secs
		Com_sprintf( buf, bufsize, "%d sec", time );
	}
}

void Text_PaintCenter( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust ) {
	int len = Text_Width( text, font, scale, 0 );
	Text_Paint( x - len / 2, y, font, scale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );
}


static void UI_DisplayDownloadInfo( const char *downloadName, float centerPoint, float yStart, int font, float scale ) {
	static char dlText[]    = "Downloading:";
	static char etaText[]   = "Estimated time left:";
	static char xferText[]  = "Transfer rate:";

	int downloadSize, downloadCount, downloadTime;
	char dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int xferRate;
	int leftWidth;
	const char *s;

	downloadSize = trap_Cvar_VariableValue( "cl_downloadSize" );
	downloadCount = trap_Cvar_VariableValue( "cl_downloadCount" );
	downloadTime = trap_Cvar_VariableValue( "cl_downloadTime" );

	leftWidth = 320;

	UI_SetColor( colorWhite );
	Text_PaintCenter( centerPoint, yStart + 112, font, scale, colorWhite, dlText, 0 );
	Text_PaintCenter( centerPoint, yStart + 144, font, scale, colorWhite, etaText, 0 );
	Text_PaintCenter( centerPoint, yStart + 208, font, scale, colorWhite, xferText, 0 );

	if ( downloadSize > 0 ) {
		s = va( "%s (%d%%)", downloadName, downloadCount * 100 / downloadSize );
	} else {
		s = downloadName;
	}

	Text_Paint( centerPoint, yStart + 244, font, 0.6f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWEDMORE );

	UI_ReadableSize( dlSizeBuf,     sizeof dlSizeBuf,       downloadCount );
	UI_ReadableSize( totalSizeBuf,  sizeof totalSizeBuf,    downloadSize );

	if ( downloadCount < 4096 || !downloadTime ) {
		Text_PaintCenter( leftWidth, 160, font, 0.6f, colorWhite, "estimating", 0 );
		Text_PaintCenter( leftWidth, 192, font, 0.6f, colorWhite, va( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
	} else {
		if ( ( uiInfo.uiDC.realTime - downloadTime ) / 1000 ) {
			xferRate = downloadCount / ( ( uiInfo.uiDC.realTime - downloadTime ) / 1000 );
		} else {
			xferRate = 0;
		}
		UI_ReadableSize( xferRateBuf, sizeof xferRateBuf, xferRate );

		// Extrapolate estimated completion time
		if ( downloadSize && xferRate ) {
			int n = downloadSize / xferRate; // estimated time for entire d/l in secs

			// We do it in K (/1024) because we'd overflow around 4MB
			UI_PrintTime( dlTimeBuf, sizeof dlTimeBuf,
						  ( n - ( ( ( downloadCount / 1024 ) * n ) / ( downloadSize / 1024 ) ) ) * 1000 );

			Text_PaintCenter( leftWidth, 160, font, 0.6f, colorWhite, dlTimeBuf, 0 );
			Text_PaintCenter( leftWidth, 192, font, 0.6f, colorWhite, va( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
		} else {
			Text_PaintCenter( leftWidth, 160, font, 0.6f, colorWhite, "estimating", 0 );
			if ( downloadSize ) {
				Text_PaintCenter( leftWidth, 160, font, 0.6f, colorWhite, va( "(%s of %s copied)", dlSizeBuf, totalSizeBuf ), 0 );
			} else {
				Text_PaintCenter( leftWidth, 160, font, 0.6f, colorWhite, va( "(%s copied)", dlSizeBuf ), 0 );
			}
		}

		if ( xferRate ) {
			Text_PaintCenter( leftWidth, 160, font, 0.6f, colorWhite, va( "%s/Sec", xferRateBuf ), 0 );
		}
	}
}

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
void UI_DrawConnectScreen( qboolean overlay ) {
	char            *s;
	uiClientState_t cstate;
	char info[MAX_INFO_VALUE];
	char text[256];
	float centerPoint, yStart, scale;

	menuDef_t *menu = Menus_FindByName( "Connect" );


	if ( !overlay && menu ) {
		Menu_Paint( menu, qtrue );
	}

	if ( !overlay ) {
		centerPoint = 320;
		yStart = 130;
		scale = 0.5f;
	} else {
		centerPoint = 320;
		yStart = 32;
		scale = 0.6f;
		return;
	}

	// see what information we should display
	trap_GetClientState( &cstate );

	info[0] = '\0';
	if ( trap_GetConfigString( CS_SERVERINFO, info, sizeof( info ) ) ) {
		Text_PaintCenter( centerPoint, yStart, UI_FONT_DEFAULT, scale, colorWhite, va( "Loading %s", Info_ValueForKey( info, "mapname" ) ), 0 );
	}

	if ( !Q_stricmp( cstate.servername,"localhost" ) ) {
//		Text_PaintCenter(centerPoint, yStart + 48, UI_FONT_DEFAULT, scale, colorWhite, va("Get Psyched!"), ITEM_TEXTSTYLE_SHADOWEDMORE);
	} else {
		strcpy( text, va( "Connecting to %s", cstate.servername ) );
		Text_PaintCenter( centerPoint, yStart + 48, UI_FONT_DEFAULT, scale, colorWhite,text, ITEM_TEXTSTYLE_SHADOWEDMORE );
	}

	//UI_DrawProportionalString( 320, 96, "Press Esc to abort", UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );

	// display global MOTD at bottom
	Text_PaintCenter( centerPoint, 600, UI_FONT_DEFAULT, scale, colorWhite, Info_ValueForKey( cstate.updateInfoString, "motd" ), 0 );
	// print any server info (server full, bad version, etc)
	if ( cstate.connState < CA_CONNECTED ) {
		Text_PaintCenter( centerPoint, yStart + 176, UI_FONT_DEFAULT, scale, colorWhite, cstate.messageString, 0 );
	}

	if ( lastConnState > cstate.connState ) {
		lastLoadingText[0] = '\0';
	}
	lastConnState = cstate.connState;

	switch ( cstate.connState ) {
	case CA_CONNECTING:
		s = va( "Awaiting connection...%i", cstate.connectPacketCount );
		break;
	case CA_CHALLENGING:
		s = va( "Awaiting challenge...%i", cstate.connectPacketCount );
		break;
	case CA_CONNECTED: {
		char downloadName[MAX_INFO_VALUE];

		trap_Cvar_VariableStringBuffer( "cl_downloadName", downloadName, sizeof( downloadName ) );
		if ( *downloadName ) {
			UI_DisplayDownloadInfo( downloadName, centerPoint, yStart, UI_FONT_DEFAULT, scale );
			return;
		}
	}
		s = "Awaiting gamestate...";
		break;
	case CA_LOADING:
		return;
	case CA_PRIMED:
		return;
	default:
		return;
	}


	if ( Q_stricmp( cstate.servername,"localhost" ) ) {
		Text_PaintCenter( centerPoint, yStart + 80, UI_FONT_DEFAULT, scale, colorWhite, s, 0 );
	}

	// password required / connection rejected information goes here
}


/*
================
cvars
================
*/

typedef struct {
	vmCvar_t    *vmCvar;
	char        *cvarName;
	char        *defaultString;
	int cvarFlags;
} cvarTable_t;

vmCvar_t ui_ffa_fraglimit;
vmCvar_t ui_ffa_timelimit;

vmCvar_t ui_tourney_fraglimit;
vmCvar_t ui_tourney_timelimit;

vmCvar_t ui_team_fraglimit;
vmCvar_t ui_team_timelimit;
vmCvar_t ui_team_friendly;

vmCvar_t ui_ctf_capturelimit;
vmCvar_t ui_ctf_timelimit;
vmCvar_t ui_ctf_friendly;

vmCvar_t ui_arenasFile;
vmCvar_t ui_botsFile;
vmCvar_t ui_spScores1;
vmCvar_t ui_spScores2;
vmCvar_t ui_spScores3;
vmCvar_t ui_spScores4;
vmCvar_t ui_spScores5;
vmCvar_t ui_spAwards;
vmCvar_t ui_spVideos;
vmCvar_t ui_spSkill;

vmCvar_t ui_spSelection;
vmCvar_t ui_master;

vmCvar_t ui_brassTime;
vmCvar_t ui_drawCrosshair;
vmCvar_t ui_drawCrosshairNames;
vmCvar_t ui_drawCrosshairPickups;       //----(SA) added
vmCvar_t ui_useSuggestedWeapons;    //----(SA)	added
vmCvar_t ui_marks;
// JOSEPH 12-3-99
vmCvar_t ui_autoactivate;
vmCvar_t ui_emptyswitch;        //----(SA)	added
// END JOSEPH

vmCvar_t ui_server1;
vmCvar_t ui_server2;
vmCvar_t ui_server3;
vmCvar_t ui_server4;
vmCvar_t ui_server5;
vmCvar_t ui_server6;
vmCvar_t ui_server7;
vmCvar_t ui_server8;
vmCvar_t ui_server9;
vmCvar_t ui_server10;
vmCvar_t ui_server11;
vmCvar_t ui_server12;
vmCvar_t ui_server13;
vmCvar_t ui_server14;
vmCvar_t ui_server15;
vmCvar_t ui_server16;

vmCvar_t ui_cdkeychecked;
vmCvar_t ui_smallFont;
vmCvar_t ui_bigFont;

vmCvar_t ui_selectedPlayer;
vmCvar_t ui_selectedPlayerName;
vmCvar_t ui_netSource;
vmCvar_t ui_menuFiles;
vmCvar_t ui_gameType;
vmCvar_t ui_netGameType;
vmCvar_t ui_actualNetGameType;
vmCvar_t ui_joinGameType;
vmCvar_t ui_dedicated;

vmCvar_t ui_notebookCurrentPage;        //----(SA)	added
vmCvar_t ui_clipboardName;          // the name of the group for the current clipboard item //----(SA)	added
vmCvar_t ui_hudAlpha;
vmCvar_t ui_hunkUsed;       //----(SA)	added
vmCvar_t ui_cameraMode;     //----(SA)	added
vmCvar_t ui_savegameListAutosave;       //----(SA)	added
vmCvar_t ui_savegameName;

// NERVE - SMF - cvars for multiplayer
vmCvar_t ui_serverFilterType;
vmCvar_t ui_currentNetMap;
vmCvar_t ui_currentMap;
vmCvar_t ui_mapIndex;

vmCvar_t ui_browserMaster;
vmCvar_t ui_browserGameType;
vmCvar_t ui_browserSortKey;
vmCvar_t ui_browserShowFull;
vmCvar_t ui_browserShowEmpty;

vmCvar_t ui_serverStatusTimeOut;

vmCvar_t ui_Q3Model;
vmCvar_t ui_headModel;
vmCvar_t ui_model;

vmCvar_t ui_limboOptions;

vmCvar_t ui_cmd;

vmCvar_t ui_prevTeam;
vmCvar_t ui_prevClass;
vmCvar_t ui_prevWeapon;

vmCvar_t ui_limboMode;
// -NERVE - SMF

cvarTable_t cvarTable[] = {
	{ &ui_ffa_fraglimit, "ui_ffa_fraglimit", "20", CVAR_ARCHIVE },
	{ &ui_ffa_timelimit, "ui_ffa_timelimit", "0", CVAR_ARCHIVE },

	{ &ui_tourney_fraglimit, "ui_tourney_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_tourney_timelimit, "ui_tourney_timelimit", "15", CVAR_ARCHIVE },

	{ &ui_team_fraglimit, "ui_team_fraglimit", "0", CVAR_ARCHIVE },
	{ &ui_team_timelimit, "ui_team_timelimit", "20", CVAR_ARCHIVE },
	{ &ui_team_friendly, "ui_team_friendly",  "1", CVAR_ARCHIVE },

	{ &ui_ctf_capturelimit, "ui_ctf_capturelimit", "8", CVAR_ARCHIVE },
	{ &ui_ctf_timelimit, "ui_ctf_timelimit", "30", CVAR_ARCHIVE },
	{ &ui_ctf_friendly, "ui_ctf_friendly",  "0", CVAR_ARCHIVE },

	{ &ui_arenasFile, "g_arenasFile", "", CVAR_INIT | CVAR_ROM },
	{ &ui_botsFile, "g_botsFile", "", CVAR_INIT | CVAR_ROM },
	{ &ui_spScores1, "g_spScores1", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores2, "g_spScores2", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores3, "g_spScores3", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores4, "g_spScores4", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spScores5, "g_spScores5", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spAwards, "g_spAwards", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spVideos, "g_spVideos", "", CVAR_ARCHIVE | CVAR_ROM },
	{ &ui_spSkill, "g_spSkill", "2", CVAR_ARCHIVE | CVAR_LATCH },

	{ &ui_spSelection, "ui_spSelection", "", CVAR_ROM },
	{ &ui_master, "ui_master", "0", CVAR_ARCHIVE },

	{ &ui_browserMaster, "ui_browserMaster", "0", CVAR_ARCHIVE },
	{ &ui_browserGameType, "ui_browserGameType", "0", CVAR_ARCHIVE },
	{ &ui_browserSortKey, "ui_browserSortKey", "4", CVAR_ARCHIVE },
	{ &ui_browserShowFull, "ui_browserShowFull", "1", CVAR_ARCHIVE },
	{ &ui_browserShowEmpty, "ui_browserShowEmpty", "1", CVAR_ARCHIVE },

	{ &ui_brassTime, "cg_brassTime", "1250", CVAR_ARCHIVE },
	{ &ui_drawCrosshair, "cg_drawCrosshair", "4", CVAR_ARCHIVE },
	{ &ui_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE },
	{ &ui_drawCrosshairPickups, "cg_drawCrosshairPickups", "1", CVAR_ARCHIVE },   //----(SA) added
	{ &ui_marks, "cg_marktime", "20000", CVAR_ARCHIVE },
	{ &ui_autoactivate, "cg_autoactivate", "1", CVAR_ARCHIVE },
	{ &ui_useSuggestedWeapons, "cg_useSuggestedWeapons", "1", CVAR_ARCHIVE }, //----(SA)	added
	{ &ui_emptyswitch, "cg_emptyswitch", "0", CVAR_ARCHIVE }, //----(SA)	added
	{ &ui_server1, "server1", "", CVAR_ARCHIVE },
	{ &ui_server2, "server2", "", CVAR_ARCHIVE },
	{ &ui_server3, "server3", "", CVAR_ARCHIVE },
	{ &ui_server4, "server4", "", CVAR_ARCHIVE },
	{ &ui_server5, "server5", "", CVAR_ARCHIVE },
	{ &ui_server6, "server6", "", CVAR_ARCHIVE },
	{ &ui_server7, "server7", "", CVAR_ARCHIVE },
	{ &ui_server8, "server8", "", CVAR_ARCHIVE },
	{ &ui_server9, "server9", "", CVAR_ARCHIVE },
	{ &ui_server10, "server10", "", CVAR_ARCHIVE },
	{ &ui_server11, "server11", "", CVAR_ARCHIVE },
	{ &ui_server12, "server12", "", CVAR_ARCHIVE },
	{ &ui_server13, "server13", "", CVAR_ARCHIVE },
	{ &ui_server14, "server14", "", CVAR_ARCHIVE },
	{ &ui_server15, "server15", "", CVAR_ARCHIVE },
	{ &ui_server16, "server16", "", CVAR_ARCHIVE },
	{ &ui_dedicated, "ui_dedicated", "0", CVAR_ARCHIVE },
	{ &ui_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{ &ui_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},
	{ &ui_cdkeychecked, "ui_cdkeychecked", "0", CVAR_ROM },
	{ &ui_selectedPlayer, "cg_selectedPlayer", "0", CVAR_ARCHIVE},
	{ &ui_selectedPlayerName, "cg_selectedPlayerName", "", CVAR_ARCHIVE},
	{ &ui_netSource, "ui_netSource", "0", CVAR_ARCHIVE },
#ifdef WOLF_SP_DEMO
	{ &ui_menuFiles, "ui_menuFiles", "ui/demomenus.txt", CVAR_ARCHIVE },
#else
	{ &ui_menuFiles, "ui_menuFiles", "ui/menus.txt", CVAR_ARCHIVE },
#endif
	{ &ui_gameType, "ui_gametype", "3", CVAR_ARCHIVE },
	{ &ui_joinGameType, "ui_joinGametype", "0", CVAR_ARCHIVE },
	{ &ui_netGameType, "ui_netGametype", "3", CVAR_ARCHIVE },
	{ &ui_actualNetGameType, "ui_actualNetGametype", "0", CVAR_ARCHIVE },
	{ &ui_notebookCurrentPage, "ui_notebookCurrentPage", "1", CVAR_ROM},
	{ &ui_clipboardName, "cg_clipboardName", "", CVAR_ROM },

	// NERVE - SMF - multiplayer cvars
	{ &ui_mapIndex, "ui_mapIndex", "0", CVAR_ARCHIVE },
	{ &ui_currentMap, "ui_currentMap", "0", CVAR_ARCHIVE },
	{ &ui_currentNetMap, "ui_currentNetMap", "0", CVAR_ARCHIVE },

	{ &ui_initialized, "ui_initialized", "0", CVAR_TEMP },
	{ &ui_debug, "ui_debug", "0", CVAR_TEMP },
	{ &ui_WolfFirstRun, "ui_WolfFirstRun", "0", CVAR_ARCHIVE},

	{ &ui_serverStatusTimeOut, "ui_serverStatusTimeOut", "7000", CVAR_ARCHIVE},

//	{ &ui_Q3Model, "ui_Q3Model", "1", 0 },
//	{ &ui_headModel, "headModel", "", 0 },

	{ &ui_limboOptions, "ui_limboOptions", "0", 0 },
	{ &ui_cmd, "ui_cmd", "", 0 },

	{ &ui_prevTeam, "ui_prevTeam", "-1", 0 },
	{ &ui_prevClass, "ui_prevClass", "-1", 0 },
	{ &ui_prevWeapon, "ui_prevWeapon", "-1", 0 },

	{ &ui_limboMode, "ui_limboMode", "0", 0 },
	// -NERVE - SMF

	{ &ui_hudAlpha, "cg_hudAlpha", "0.8", CVAR_ARCHIVE },
	{ &ui_hunkUsed, "com_hunkused", "0", 0 },     //----(SA)	added
	{ &ui_cameraMode, "com_cameraMode", "0", 0},  //----(SA)	added

	{ &ui_savegameName, "ui_savegameName", "", CVAR_ROM}


};

int cvarTableSize = sizeof( cvarTable ) / sizeof( cvarTable[0] );


/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars( void ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Register( cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags );
	}
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars( void ) {
	int i;
	cvarTable_t *cv;

	for ( i = 0, cv = cvarTable ; i < cvarTableSize ; i++, cv++ ) {
		trap_Cvar_Update( cv->vmCvar );
	}
}

// NERVE - SMF
/*
=================
ArenaServers_StopRefresh
=================
*/
static void UI_StopServerRefresh( void ) {
	int count;

	if ( !uiInfo.serverStatus.refreshActive ) {
		// not currently refreshing
		return;
	}
	uiInfo.serverStatus.refreshActive = qfalse;
	Com_Printf( "%d servers listed in browser with %d players.\n",
				uiInfo.serverStatus.numDisplayServers,
				uiInfo.serverStatus.numPlayersOnServers );
	count = trap_LAN_GetServerCount( ui_netSource.integer );
	if ( count - uiInfo.serverStatus.numDisplayServers > 0 ) {
		Com_Printf( "%d servers not listed due to packet loss or pings higher than %d\n",
					count - uiInfo.serverStatus.numDisplayServers,
					(int) trap_Cvar_VariableValue( "cl_maxPing" ) );
	}

}

/*
=================
UI_DoServerRefresh
=================
*/
static void UI_DoServerRefresh( void ) {
	qboolean wait = qfalse;

	if ( !uiInfo.serverStatus.refreshActive ) {
		return;
	}
	if ( ui_netSource.integer != AS_FAVORITES ) {
		if ( ui_netSource.integer == AS_LOCAL ) {
			if ( !trap_LAN_GetServerCount( ui_netSource.integer ) ) {
				wait = qtrue;
			}
		} else {
			if ( trap_LAN_GetServerCount( ui_netSource.integer ) < 0 ) {
				wait = qtrue;
			}
		}
	}

	if ( uiInfo.uiDC.realTime < uiInfo.serverStatus.refreshtime ) {
		if ( wait ) {
			return;
		}
	}

	// if still trying to retrieve pings
	if ( trap_LAN_UpdateVisiblePings( ui_netSource.integer ) ) {
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	} else if ( !wait ) {
		// get the last servers in the list
		UI_BuildServerDisplayList( 2 );
		// stop the refresh
		UI_StopServerRefresh();
	}
	//
	UI_BuildServerDisplayList( qfalse );
}

/*
=================
UI_StartServerRefresh
=================
*/
static void UI_StartServerRefresh( qboolean full ) {
	int i;
	char    *ptr;

	qtime_t q;
	trap_RealTime( &q );
	trap_Cvar_Set( va( "ui_lastServerRefresh_%i", ui_netSource.integer ), va( "%s-%i, %i at %i:%i", MonthAbbrev[q.tm_mon],q.tm_mday, 1900 + q.tm_year,q.tm_hour,q.tm_min ) );

	if ( !full ) {
		UI_UpdatePendingPings();
		return;
	}

	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 1000;
	// clear number of displayed servers
	uiInfo.serverStatus.numDisplayServers = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;
	// mark all servers as visible so we store ping updates for them
	trap_LAN_MarkServerVisible( ui_netSource.integer, -1, qtrue );
	// reset all the pings
	trap_LAN_ResetPings( ui_netSource.integer );
	//
	if ( ui_netSource.integer == AS_LOCAL ) {
		trap_Cmd_ExecuteText( EXEC_NOW, "localservers\n" );
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
		return;
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;
	if ( ui_netSource.integer == AS_GLOBAL || ui_netSource.integer == AS_MPLAYER ) {
		if ( ui_netSource.integer == AS_GLOBAL ) {
			i = 0;
		} else {
			i = 1;
		}

		ptr = UI_Cvar_VariableString( "debug_protocol" );
		if ( strlen( ptr ) ) {
			trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %s full empty\n", i, ptr ) );
		} else {
			trap_Cmd_ExecuteText( EXEC_NOW, va( "globalservers %d %d full empty\n", i, (int)trap_Cvar_VariableValue( "protocol" ) ) );
		}
	}
}
// -NERVE - SMF
