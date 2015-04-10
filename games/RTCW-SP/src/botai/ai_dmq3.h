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
 * name:		ai_dmq3.h
 *
 * desc:		Quake3 bot AI
 *
 *
 *****************************************************************************/

//setup the deathmatch AI
void BotSetupDeathmatchAI( void );
//shutdown the deathmatch AI
void BotShutdownDeathmatchAI( void );
//let the bot live within it's deathmatch AI net
void BotDeathmatchAI( bot_state_t *bs, float thinktime );
//free waypoints
void BotFreeWaypoints( bot_waypoint_t *wp );
//choose a weapon
void BotChooseWeapon( bot_state_t *bs );
//setup movement stuff
void BotSetupForMovement( bot_state_t *bs );
//update the inventory
void BotUpdateInventory( bot_state_t *bs );
//update the inventory during battle
void BotUpdateBattleInventory( bot_state_t *bs, int enemy );
//use holdable items during battle
void BotBattleUseItems( bot_state_t *bs );
//return true if the bot is dead
qboolean BotIsDead( bot_state_t *bs );
//returns true if the bot is in observer mode
qboolean BotIsObserver( bot_state_t *bs );
//returns true if the bot is in the intermission
qboolean BotIntermission( bot_state_t *bs );
//returns true if the bot is in lava
qboolean BotInLava( bot_state_t *bs );
//returns true if the bot is in slime
qboolean BotInSlime( bot_state_t *bs );
//returns true if the entity is dead
qboolean EntityIsDead( aas_entityinfo_t *entinfo );
//returns true if the entity is invisible
qboolean EntityIsInvisible( aas_entityinfo_t *entinfo );
//returns true if the entity is shooting
qboolean EntityIsShooting( aas_entityinfo_t *entinfo );
//returns the name of the client
char *ClientName( int client, char *name, int size );
//returns an simplyfied client name
char *EasyClientName( int client, char *name, int size );
//returns the skin used by the client
char *ClientSkin( int client, char *skin, int size );
//returns the aggression of the bot in the range [0, 100]
float BotAggression( bot_state_t *bs );
//returns true if the bot wants to retreat
int BotWantsToRetreat( bot_state_t *bs );
//returns true if the bot wants to chase
int BotWantsToChase( bot_state_t *bs );
//returns true if the bot wants to help
int BotWantsToHelp( bot_state_t *bs );
//returns true if the bot can and wants to rocketjump
int BotCanAndWantsToRocketJump( bot_state_t *bs );
//returns true if the bot wants to and goes camping
int BotWantsToCamp( bot_state_t *bs );
//the bot will perform attack movements
bot_moveresult_t BotAttackMove( bot_state_t *bs, int tfl );
//returns true if the bot and the entity are in the same team
int BotSameTeam( bot_state_t *bs, int entnum );
//returns true if teamplay is on
int TeamPlayIsOn( void );
//returns true and sets the .enemy field when an enemy is found
int BotFindEnemy( bot_state_t *bs, int curenemy );
//returns a roam goal
void BotRoamGoal( bot_state_t *bs, vec3_t goal );
//returns entity visibility in the range [0, 1]
float BotEntityVisible( int viewer, vec3_t eye, vec3_t viewangles, float fov, int ent );
//the bot will aim at the current enemy
void BotAimAtEnemy( bot_state_t *bs );
//check if the bot should attack
void BotCheckAttack( bot_state_t *bs );
//AI when the bot is blocked
void BotAIBlocked( bot_state_t *bs, bot_moveresult_t *moveresult, int activate );
//returns the CTF team the bot is in
int BotCTFTeam( bot_state_t *bs );
//returns the flag the bot is carrying (CTFFLAG_?)
int BotCTFCarryingFlag( bot_state_t *bs );
//set ctf goals (defend base, get enemy flag) during seek
void BotCTFSeekGoals( bot_state_t *bs );
//set ctf goals (defend base, get enemy flag) during retreat
void BotCTFRetreatGoals( bot_state_t *bs );
//create a new waypoint
bot_waypoint_t *BotCreateWayPoint( char *name, vec3_t origin, int areanum );
//find a waypoint with the given name
bot_waypoint_t *BotFindWayPoint( bot_waypoint_t *waypoints, char *name );
//strstr but case insensitive
char *stristr( char *str, char *charset );
//returns the number of the client with the given name
int ClientFromName( char *name );
//
int BotPointAreaNum( vec3_t origin );
//
void BotMapScripts( bot_state_t *bs );

//ctf flags
#define CTF_FLAG_NONE       0
#define CTF_FLAG_RED        1
#define CTF_FLAG_BLUE       2
//CTF skins
#define CTF_SKIN_REDTEAM    "red"
#define CTF_SKIN_BLUETEAM   "blue"
//CTF teams
#define CTF_TEAM_NONE       0
#define CTF_TEAM_RED        1
#define CTF_TEAM_BLUE       2

extern int dmflags;         //deathmatch flags
extern int gametype;        //game type

// Rafael gameskill
extern int gameskill;
// done

extern vmCvar_t bot_grapple;
extern vmCvar_t bot_rocketjump;
extern vmCvar_t bot_fastchat;
extern vmCvar_t bot_nochat;
extern vmCvar_t bot_testrchat;

extern bot_goal_t ctf_redflag;
extern bot_goal_t ctf_blueflag;

