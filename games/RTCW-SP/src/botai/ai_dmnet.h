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


/*****************************************************************************
 * name:		ai_dmnet.h
 *
 * desc:		Quake3 bot AI
 *
 *
 *****************************************************************************/

#define MAX_NODESWITCHES    50

void AIEnter_Intermission( bot_state_t *bs );
void AIEnter_Observer( bot_state_t *bs );
void AIEnter_Respawn( bot_state_t *bs );
void AIEnter_Stand( bot_state_t *bs );
void AIEnter_Seek_ActivateEntity( bot_state_t *bs );
void AIEnter_Seek_NBG( bot_state_t *bs );
void AIEnter_Seek_LTG( bot_state_t *bs );
void AIEnter_Seek_Camp( bot_state_t *bs );
void AIEnter_Battle_Fight( bot_state_t *bs );
void AIEnter_Battle_Chase( bot_state_t *bs );
void AIEnter_Battle_Retreat( bot_state_t *bs );
void AIEnter_Battle_NBG( bot_state_t *bs );
int AINode_Intermission( bot_state_t *bs );
int AINode_Observer( bot_state_t *bs );
int AINode_Respawn( bot_state_t *bs );
int AINode_Stand( bot_state_t *bs );
int AINode_Seek_ActivateEntity( bot_state_t *bs );
int AINode_Seek_NBG( bot_state_t *bs );
int AINode_Seek_LTG( bot_state_t *bs );
int AINode_Battle_Fight( bot_state_t *bs );
int AINode_Battle_Chase( bot_state_t *bs );
int AINode_Battle_Retreat( bot_state_t *bs );
int AINode_Battle_NBG( bot_state_t *bs );

void BotResetNodeSwitches( void );
void BotDumpNodeSwitches( bot_state_t *bs );

