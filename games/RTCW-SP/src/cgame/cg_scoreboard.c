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

// cg_scoreboard -- draw the scoreboard on top of the game screen
#include "cg_local.h"


#define SCOREBOARD_WIDTH    ( 31 * BIGCHAR_WIDTH )

/*
=================
CG_DrawScoreboard
=================
*/
static void CG_DrawClientScore( int x, int y, score_t *score, float *color, float fade ) {
	char string[1024];
	vec3_t headAngles;
	clientInfo_t    *ci;

	if ( score->client < 0 || score->client >= cgs.maxclients ) {
		Com_Printf( "Bad score->client: %i\n", score->client );
		return;
	}

	ci = &cgs.clientinfo[score->client];

	// draw the handicap or bot skill marker
	if ( ci->botSkill > 0 && ci->botSkill <= 5 ) {
		CG_DrawPic( 0, y - 8, 32, 32, cgs.media.botSkillShaders[ ci->botSkill - 1 ] );
	} else if ( ci->handicap < 100 ) {
		Com_sprintf( string, sizeof( string ), "%i", ci->handicap );
		CG_DrawSmallStringColor( 8, y, string, color );
	}

	// draw the wins / losses
	if ( cgs.gametype == GT_TOURNAMENT ) {
		Com_sprintf( string, sizeof( string ), "%i/%i", ci->wins, ci->losses );
		CG_DrawSmallStringColor( x + SCOREBOARD_WIDTH + 2, y, string, color );
	}

	// draw the face
	VectorClear( headAngles );
	headAngles[YAW] = 180;

	CG_DrawHead( x - ICON_SIZE, y - ( ICON_SIZE - BIGCHAR_HEIGHT ) / 2, ICON_SIZE, ICON_SIZE,
				 score->client, headAngles );

	if ( ci->powerups & ( 1 << PW_REDFLAG ) ) {
		CG_DrawFlagModel( x - ICON_SIZE - ICON_SIZE / 2, y - ( ICON_SIZE - BIGCHAR_HEIGHT ) / 2, ICON_SIZE, ICON_SIZE,
						  TEAM_RED );
	} else if ( ci->powerups & ( 1 << PW_BLUEFLAG ) ) {
		CG_DrawFlagModel( x - ICON_SIZE - ICON_SIZE / 2, y - ( ICON_SIZE - BIGCHAR_HEIGHT ) / 2, ICON_SIZE, ICON_SIZE,
						  TEAM_BLUE );
	}

	// draw the score line
	if ( score->ping == -1 ) {
		Com_sprintf( string, sizeof( string ),
					 "connecting     %s", ci->name );
	} else if ( ci->team == TEAM_SPECTATOR ) {
		Com_sprintf( string, sizeof( string ),
					 "SPECT %4i %4i %s", score->ping, score->time, ci->name );
	} else {
		Com_sprintf( string, sizeof( string ),
					 "%5i %4i %4i %s", score->score, score->ping, score->time, ci->name );
	}

	// highlight your position
	if ( score->client == cg.snap->ps.clientNum ) {
		float hcolor[4];
		int rank;

		if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR
			 || cgs.gametype >= GT_TEAM ) {
			rank = -1;
		} else {
			rank = cg.snap->ps.persistant[PERS_RANK] & ~RANK_TIED_FLAG;
		}

		if ( rank == 0 ) {
			hcolor[0] = 0;
			hcolor[1] = 0;
			hcolor[2] = 0.7;
		} else if ( rank == 1 ) {
			hcolor[0] = 0.7;
			hcolor[1] = 0;
			hcolor[2] = 0;
		} else if ( rank == 2 ) {
			hcolor[0] = 0.7;
			hcolor[1] = 0.7;
			hcolor[2] = 0;
		} else {
			hcolor[0] = 0.7;
			hcolor[1] = 0.7;
			hcolor[2] = 0.7;
		}

		hcolor[3] = fade * 0.7;
		CG_FillRect( x - 2, y,  SCOREBOARD_WIDTH, BIGCHAR_HEIGHT + 1, hcolor );
	}

	CG_DrawBigString( x, y, string, fade );

	// add the "ready" marker for intermission exiting
	if ( cg.snap->ps.stats[ STAT_CLIENTS_READY ] & ( 1 << score->client ) ) {
		CG_DrawBigStringColor( 0, y, "READY", color );
	}
}

/*
=================
CG_TeamScoreboard
=================
*/
static int CG_TeamScoreboard( int x, int y, team_t team, float fade ) {
	int i;
	score_t *score;
	float color[4];
	int count;
	int lineHeight;
	clientInfo_t    *ci;

	color[0] = color[1] = color[2] = 1.0;
	color[3] = fade;

	count = 0;
	lineHeight = 40;
	// don't draw more than 9 rows
	for ( i = 0 ; i < cg.numScores && count < 9 ; i++ ) {
		score = &cg.scores[i];
		ci = &cgs.clientinfo[ score->client ];

		if ( team != ci->team ) {
			continue;
		}

		CG_DrawClientScore( x, y + lineHeight * count, score, color, fade );
		count++;
	}

	return y + count * lineHeight + 20;
}

// NERVE - SMF
/*
=================
WM_DrawClientScore
=================
*/
static int INFO_PLAYER_WIDTH    = 300;
static int INFO_SCORE_WIDTH     = 50;
static int INFO_LATENCY_WIDTH   = 80;
static int INFO_TEAM_HEIGHT     = 24;
static int INFO_BORDER          = 2;

static void WM_DrawClientScore( int x, int y, score_t *score, float *color, float fade ) {
	float tempx;
	vec4_t hcolor;
	clientInfo_t *ci;

	if ( y + SMALLCHAR_HEIGHT >= 440 ) {
		return;
	}

	if ( score->client == cg.snap->ps.clientNum ) {
		tempx = x;

		hcolor[3] = fade * 0.3;
		VectorSet( hcolor, 0.4452, 0.1172, 0.0782 );            // DARK-RED

		CG_FillRect( tempx, y + 1, INFO_PLAYER_WIDTH - INFO_BORDER, SMALLCHAR_HEIGHT - 1, hcolor );
		tempx += INFO_PLAYER_WIDTH;

		CG_FillRect( tempx, y + 1, INFO_SCORE_WIDTH - INFO_BORDER, SMALLCHAR_HEIGHT - 1, hcolor );
		tempx += INFO_SCORE_WIDTH;

		CG_FillRect( tempx, y + 1, INFO_LATENCY_WIDTH - INFO_BORDER, SMALLCHAR_HEIGHT - 1, hcolor );
		tempx += INFO_LATENCY_WIDTH;
	}

	tempx = x;
	ci = &cgs.clientinfo[score->client];

	CG_DrawSmallString( tempx, y, ci->name, fade );
	tempx += INFO_PLAYER_WIDTH;

	CG_DrawSmallString( tempx, y, va( "%4i", score->score ), fade );
	tempx += INFO_SCORE_WIDTH;

	CG_DrawSmallString( tempx, y, va( "%4i", score->ping ), fade );
	tempx += INFO_LATENCY_WIDTH;
}

/*
=================
WM_TeamScoreboard
=================
*/
static int WM_TeamScoreboard( int x, int y, team_t team, float fade ) {
	vec4_t hcolor;
	float tempx;
	int i;

	hcolor[3] = fade;
	if ( team == TEAM_RED ) {
		VectorSet( hcolor, 0.4452, 0.1172, 0.0782 );        // LIGHT-RED
	} else if ( team == TEAM_BLUE )                                                             {
		VectorSet( hcolor, 0.1836, 0.2422, 0.1680 );        // LIGHT-GREEN
	} else {
		VectorSet( hcolor, 0.2, 0.2, 0.2 );                 // DARK-GREY

	}
	// dont draw spectator if there are none
	for ( i = 0; i < cg.numScores; i++ ) {
		if ( team == cgs.clientinfo[ cg.scores[i].client ].team ) {
			break;
		}
	}
	if ( team == TEAM_SPECTATOR && i == cg.numScores ) {
		return y;
	}

	// draw team header
	if ( y + SMALLCHAR_HEIGHT >= 440 ) {
		return y;
	}

	tempx = x;

	CG_FillRect( tempx, y, INFO_PLAYER_WIDTH - INFO_BORDER, INFO_TEAM_HEIGHT, hcolor );
	if ( team == TEAM_RED ) {
		CG_DrawSmallString( tempx, y, "Axis", fade );
	} else if ( team == TEAM_BLUE ) {
		CG_DrawSmallString( tempx, y, "Allies", fade );
	} else {
		CG_DrawSmallString( tempx, y, "Spectators", fade );
	}
	tempx += INFO_PLAYER_WIDTH;

	CG_FillRect( tempx, y, INFO_SCORE_WIDTH - INFO_BORDER, INFO_TEAM_HEIGHT, hcolor );
	tempx += INFO_SCORE_WIDTH;

	CG_FillRect( tempx, y, INFO_LATENCY_WIDTH - INFO_BORDER, INFO_TEAM_HEIGHT, hcolor );
	tempx += INFO_LATENCY_WIDTH;

	// draw player info
	VectorSet( hcolor, 1, 1, 1 );
	hcolor[3] = fade;

	y += INFO_TEAM_HEIGHT + INFO_BORDER;

	for ( i = 0; i < cg.numScores; i++ ) {
		if ( team != cgs.clientinfo[ cg.scores[i].client ].team ) {
			continue;
		}

		WM_DrawClientScore( x, y, &cg.scores[i], hcolor, fade );
		y += SMALLCHAR_HEIGHT;
	}

	y += 4;

	return y;
}

/*
=================
WM_DrawObjectives
=================
*/
int WM_DrawObjectives( int x, int y, int width, float fade ) {
	const char *s, *buf, *str;
	char teamstr[32];
	int i, num, strwidth, status;

	y += 32;

	// determine character's team
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_RED ) {
		strcpy( teamstr, "axis_desc" );
	} else {
		strcpy( teamstr, "allied_desc" );
	}

	s = CG_ConfigString( CS_MULTI_INFO );
	buf = Info_ValueForKey( s, "numobjectives" );

	if ( buf && atoi( buf ) ) {
		num = atoi( buf );

		for ( i = 0; i < num; i++ ) {
			s = CG_ConfigString( CS_MULTI_OBJECTIVE1 + i );
			buf = Info_ValueForKey( s, teamstr );

			// draw text
			str = va( "%s", buf );
			strwidth = CG_DrawStrlen( str ) * SMALLCHAR_WIDTH;
			CG_DrawSmallString( x + width / 2 - strwidth / 2 - 12, y, str, fade );

			// draw status flags
			status = atoi( Info_ValueForKey( s, "status" ) );

			if ( status == 0 ) {
				CG_DrawPic( x + width / 2 - strwidth / 2 - 16 - 24, y, 24, 16, trap_R_RegisterShaderNoMip( "ui/assets/ger_flag.tga" ) );
				CG_DrawPic( x + width / 2 + strwidth / 2 - 12 + 4, y, 24, 16, trap_R_RegisterShaderNoMip( "ui/assets/ger_flag.tga" ) );
			} else if ( status == 1 )   {
				CG_DrawPic( x + width / 2 - strwidth / 2 - 16 - 24, y, 24, 16, trap_R_RegisterShaderNoMip( "ui/assets/usa_flag.tga" ) );
				CG_DrawPic( x + width / 2 + strwidth / 2 - 12 + 4, y, 24, 16, trap_R_RegisterShaderNoMip( "ui/assets/usa_flag.tga" ) );
			}

			y += 16;
		}
	}

	return y;
}

/*
=================
WM_ScoreboardOverlay
=================
*/
int WM_ScoreboardOverlay( int x, int y, float fade ) {
	vec4_t hcolor;
	int width;
	char        *s; // JPW NERVE
	int msec, mins, seconds, tens; // JPW NERVE

	width = INFO_PLAYER_WIDTH + INFO_LATENCY_WIDTH + INFO_SCORE_WIDTH + 25;

	VectorSet( hcolor, 0, 0, 0 );
	hcolor[3] = 0.7 * fade;

	// draw background
	CG_FillRect( x - 12, y, width, 400, hcolor );

	// draw title frame
	VectorSet( hcolor, 0.0039, 0.0039, 0.2461 );
	hcolor[3] = 1 * fade;
	CG_FillRect( x - 12, y, width, 30, hcolor );
	CG_DrawRect( x - 12, y, width, 400, 2, hcolor );

	if ( cg.snap->ps.pm_type == PM_INTERMISSION ) {
		const char *s, *buf;

		s = CG_ConfigString( CS_MULTI_INFO );
		buf = Info_ValueForKey( s, "winner" );

		if ( atoi( buf ) ) {
			CG_DrawSmallString( x - 12 + 5, y, "ALLIES WIN!", fade );
		} else {
			CG_DrawSmallString( x - 12 + 5, y, "AXIS WIN!", fade );
		}
	}
// JPW NERVE -- mission time & reinforce time
	else {
		msec = ( cgs.timelimit * 60.f * 1000.f ) - ( cg.time - cgs.levelStartTime );

		seconds = msec / 1000;
		mins = seconds / 60;
		seconds -= mins * 60;
		tens = seconds / 10;
		seconds -= tens * 10;

		s = va( "Mission time:   %2.0f:%i%i", (float)mins, tens, seconds ); // float cast to line up with reinforce time
		CG_DrawSmallString( x - 7,y,s,fade );

		if ( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_RED ) {
			msec = cg_redlimbotime.integer - ( cg.time % cg_redlimbotime.integer );
		} else if ( cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_BLUE )     {
			msec = cg_bluelimbotime.integer - ( cg.time % cg_bluelimbotime.integer );
		} else { // no team (spectator mode)
			msec = 0;
		}

		if ( msec ) {
			seconds = msec / 1000;
			mins = seconds / 60;
			seconds -= mins * 60;
			tens = seconds / 10;
			seconds -= tens * 10;

			s = va( "Reinforce time: %2.0f:%i%i", (float)mins, tens, seconds );
			CG_DrawSmallString( x - 7,y + 16,s,fade );
		}
	}
// jpw
//	CG_DrawSmallString( x - 12 + 5, y, "Wolfenstein Multiplayer", fade ); // old one

	y = WM_DrawObjectives( x, y, width, fade );
	y += 5;

	// draw field names
	CG_DrawSmallString( x, y, "Players", fade );
	x += INFO_PLAYER_WIDTH;

	CG_DrawSmallString( x, y, "Score", fade );
	x += INFO_SCORE_WIDTH;

	CG_DrawSmallString( x, y, "Latency", fade );
	x += INFO_LATENCY_WIDTH;

	y += 20;

	return y;
}
// -NERVE - SMF

/*
=================
CG_DrawScoreboard

Draw the normal in-game scoreboard
=================
*/
qboolean CG_DrawScoreboard( void ) {
	int x = 0, y = 0, w;     // TTimo init
	float fade;
	float   *fadeColor;
	char    *s;

	// don't draw amuthing if the menu or console is up
	if ( cg_paused.integer ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// still need to see 'mission failed' message in SP
	if ( cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_DEAD ) {
		return qfalse;
	}

	if ( cgs.gametype == GT_SINGLE_PLAYER && cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		cg.deferredPlayerLoading = 0;
		return qfalse;
	}

	// don't draw scoreboard during death while warmup up
	if ( cg.warmup && !cg.showScores ) {
		return qfalse;
	}

	if ( cg.showScores || cg.predictedPlayerState.pm_type == PM_DEAD ||
		 cg.predictedPlayerState.pm_type == PM_INTERMISSION ) {
		fade = 1.0;
		fadeColor = colorWhite;
	} else {
		fadeColor = CG_FadeColor( cg.scoreFadeTime, FADE_TIME );

		if ( !fadeColor ) {
			// next time scoreboard comes up, don't print killer
			cg.deferredPlayerLoading = 0;
			cg.killerName[0] = 0;
			return qfalse;
		}
		fade = *fadeColor;
	}


	// fragged by ... line
	if ( cg.killerName[0] ) {
		s = va( "Killed by %s", cg.killerName );
		w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
		x = ( SCREEN_WIDTH - w ) / 2;
		y = 40;
		CG_DrawBigString( x, y, s, fade );
	}

	// current rank

	//----(SA) enclosed this so it doesn't draw for SP
	if ( cgs.gametype != GT_SINGLE_PLAYER && cgs.gametype != GT_WOLF ) { // NERVE - SMF - added wolf multiplayer check
		if ( cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ) {
			if ( cgs.gametype < GT_TEAM ) {
				s = va( "%s place with %i",
						CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
						cg.snap->ps.persistant[PERS_SCORE] );
				w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
				x = ( SCREEN_WIDTH - w ) / 2;
				y = 60;
				CG_DrawBigString( x, y, s, fade );
			} else {
				if ( cg.teamScores[0] == cg.teamScores[1] ) {
					s = va( "Teams are tied at %i", cg.teamScores[0] );
				} else if ( cg.teamScores[0] >= cg.teamScores[1] ) {
					s = va( "Red leads %i to %i",cg.teamScores[0], cg.teamScores[1] );
				} else {
					s = va( "Blue leads %i to %i",cg.teamScores[1], cg.teamScores[0] );
				}

				w = CG_DrawStrlen( s ) * BIGCHAR_WIDTH;
				x = ( SCREEN_WIDTH - w ) / 2;
				y = 60;
				CG_DrawBigString( x, y, s, fade );
			}
		}

		// scoreboard
		x = 320 - SCOREBOARD_WIDTH / 2;
		y = 86;

	#if 0
		CG_DrawBigStringColor( x, y, "SCORE PING TIME NAME", fadeColor );
		CG_DrawBigStringColor( x, y + 12, "----- ---- ---- ---------------", fadeColor );
	#endif
		CG_DrawPic( x + 1 * 16, y, 64, 32, cgs.media.scoreboardScore );
		CG_DrawPic( x + 6 * 16 + 8, y, 64, 32, cgs.media.scoreboardPing );
		CG_DrawPic( x + 11 * 16 + 8, y, 64, 32, cgs.media.scoreboardTime );
		CG_DrawPic( x + 16 * 16, y, 64, 32, cgs.media.scoreboardName );

		y += 32;
	}

	// NERVE - SMF
	if ( cgs.gametype == GT_WOLF ) {
		//
		// teamplay scoreboard
		//
		x = 320 - SCOREBOARD_WIDTH / 2 + 20 + 20;
		y = 40;

		y = WM_ScoreboardOverlay( x, y, fade );

		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			y = WM_TeamScoreboard( x, y, TEAM_RED, fade );
			y = WM_TeamScoreboard( x, y, TEAM_BLUE, fade );
		} else {
			y = WM_TeamScoreboard( x, y, TEAM_BLUE, fade );
			y = WM_TeamScoreboard( x, y, TEAM_RED, fade );
		}
		y = WM_TeamScoreboard( x, y, TEAM_SPECTATOR, fade );
	}
	// -NERVE - SMF
	else if ( cgs.gametype >= GT_TEAM ) {
		//
		// teamplay scoreboard
		//
		if ( cg.teamScores[0] >= cg.teamScores[1] ) {
			y = CG_TeamScoreboard( x, y, TEAM_RED, fade );
			y = CG_TeamScoreboard( x, y, TEAM_BLUE, fade );
		} else {
			y = CG_TeamScoreboard( x, y, TEAM_BLUE, fade );
			y = CG_TeamScoreboard( x, y, TEAM_RED, fade );
		}
		y = CG_TeamScoreboard( x, y, TEAM_SPECTATOR, fade );

	} else if ( cgs.gametype != GT_SINGLE_PLAYER ) {   //----(SA) modified
		//
		// free for all scoreboard
		//
		y = CG_TeamScoreboard( x, y, TEAM_FREE, fade );
		y = CG_TeamScoreboard( x, y, TEAM_SPECTATOR, fade );
	}

	// load any models that have been deferred
	if ( ++cg.deferredPlayerLoading > 1 ) {
		CG_LoadDeferredPlayers();
	}

	return qtrue;
}

//================================================================================

/*
================
CG_CenterGiantLine
================
*/
static void CG_CenterGiantLine( float y, const char *string ) {
	float x;
	vec4_t color;

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	x = 0.5 * ( 640 - GIANT_WIDTH * CG_DrawStrlen( string ) );

	CG_DrawStringExt( x, y, string, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
}

/*
=================
CG_DrawTourneyScoreboard

Draw the oversize scoreboard for tournements
=================
*/
void CG_DrawTourneyScoreboard( void ) {
	const char      *s;
	vec4_t color;
	int min, tens, ones;
	clientInfo_t    *ci;
	int y;
	int i;

	// request more scores regularly
	if ( cg.scoresRequestTime + 2000 < cg.time ) {
		cg.scoresRequestTime = cg.time;
		trap_SendClientCommand( "score" );
	}

	color[0] = 1;
	color[1] = 1;
	color[2] = 1;
	color[3] = 1;

	// draw the dialog background
	color[0] = color[1] = color[2] = 0;
	color[3] = 1;
	CG_FillRect( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, color );

	// print the mesage of the day
	s = CG_ConfigString( CS_MOTD );
	if ( !s[0] ) {
		s = "Scoreboard";
	}

	// print optional title
	CG_CenterGiantLine( 8, s );

	// print server time
	ones = cg.time / 1000;
	min = ones / 60;
	ones %= 60;
	tens = ones / 10;
	ones %= 10;
	s = va( "%i:%i%i", min, tens, ones );

	CG_CenterGiantLine( 64, s );


	// print the two scores

	y = 160;
	if ( cgs.gametype >= GT_TEAM ) {
		//
		// teamplay scoreboard
		//
		CG_DrawStringExt( 8, y, "Red Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		s = va( "%i", cg.teamScores[0] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen( s ), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );

		y += 64;

		CG_DrawStringExt( 8, y, "Blue Team", color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
		s = va( "%i", cg.teamScores[1] );
		CG_DrawStringExt( 632 - GIANT_WIDTH * strlen( s ), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
	} else {
		//
		// free for all scoreboard
		//
		for ( i = 0 ; i < MAX_CLIENTS ; i++ ) {
			ci = &cgs.clientinfo[i];
			if ( !ci->infoValid ) {
				continue;
			}
			if ( ci->team != TEAM_FREE ) {
				continue;
			}

			CG_DrawStringExt( 8, y, ci->name, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			s = va( "%i", ci->score );
			CG_DrawStringExt( 632 - GIANT_WIDTH * strlen( s ), y, s, color, qtrue, qtrue, GIANT_WIDTH, GIANT_HEIGHT, 0 );
			y += 64;
		}
	}


}

