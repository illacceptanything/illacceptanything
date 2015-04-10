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
 * name:		be_ai_goal.h
 *
 * desc:		goal AI
 *
 *
 *****************************************************************************/

#define MAX_AVOIDGOALS          64
#define MAX_GOALSTACK           8

#define GFL_NONE                0
#define GFL_ITEM                1
#define GFL_ROAM                2
#define GFL_NOSLOWAPPROACH      4

//a bot goal
typedef struct bot_goal_s
{
	vec3_t origin;              //origin of the goal
	int areanum;                //area number of the goal
	vec3_t mins, maxs;          //mins and maxs of the goal
	int entitynum;              //number of the goal entity
	int number;                 //goal number
	int flags;                  //goal flags
	int iteminfo;               //item information
} bot_goal_t;

//reset the whole goal state, but keep the item weights
void BotResetGoalState( int goalstate );
//reset avoid goals
void BotResetAvoidGoals( int goalstate );
//remove the goal with the given number from the avoid goals
void BotRemoveFromAvoidGoals( int goalstate, int number );
//push a goal
void BotPushGoal( int goalstate, bot_goal_t *goal );
//pop a goal
void BotPopGoal( int goalstate );
//makes the bot's goal stack empty
void BotEmptyGoalStack( int goalstate );
//dump the avoid goals
void BotDumpAvoidGoals( int goalstate );
//dump the goal stack
void BotDumpGoalStack( int goalstate );
//name of the goal
void BotGoalName( int number, char *name, int size );
//get goal from top of stack
int BotGetTopGoal( int goalstate, bot_goal_t *goal );
int BotGetSecondGoal( int goalstate, bot_goal_t *goal );
//choose the best long term goal item for the bot
int BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags );
//choose the best nearby goal item for the bot
int BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags,
					  bot_goal_t *ltg, float maxtime );
//returns true if the bot touches the goal
int BotTouchingGoal( vec3_t origin, bot_goal_t *goal );
//returns true if the goal should be visible but isn't
int BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, bot_goal_t *goal );
//get some info about a level item
int BotGetLevelItemGoal( int index, char *classname, bot_goal_t *goal );
//get the next camp spot in the map
int BotGetNextCampSpotGoal( int num, bot_goal_t *goal );
//get the map location with the given name
int BotGetMapLocationGoal( char *name, bot_goal_t *goal );
//returns the avoid goal time
float BotAvoidGoalTime( int goalstate, int number );
//initializes the items in the level
void BotInitLevelItems( void );
//regularly update dynamic entity items (dropped weapons, flags etc.)
void BotUpdateEntityItems( void );
//interbreed the goal fuzzy logic
void BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child );
//save the goal fuzzy logic to disk
void BotSaveGoalFuzzyLogic( int goalstate, char *filename );
//mutate the goal fuzzy logic
void BotMutateGoalFuzzyLogic( int goalstate, float range );
//loads item weights for the bot
int BotLoadItemWeights( int goalstate, char *filename );
//frees the item weights of the bot
void BotFreeItemWeights( int goalstate );
//returns the handle of a newly allocated goal state
int BotAllocGoalState( int client );
//free the given goal state
void BotFreeGoalState( int handle );
//setup the goal AI
int BotSetupGoalAI( void );
//shut down the goal AI
void BotShutdownGoalAI( void );
