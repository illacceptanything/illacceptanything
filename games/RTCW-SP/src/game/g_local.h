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



// g_local.h -- local definitions for game module

#include "q_shared.h"
#include "bg_public.h"
#include "g_public.h"

//==================================================================

// the "gameversion" client command will print this plus compile date
//----(SA) Wolfenstein
#define GAMEVERSION "main"
// done.

#define BODY_QUEUE_SIZE     8

#define INFINITE            1000000

#define FRAMETIME           100                 // msec
#define EVENT_VALID_MSEC    1000
#define CARNAGE_REWARD_TIME 3000
#define REWARD_SPRITE_TIME  2000

#define INTERMISSION_DELAY_TIME 1000


// gentity->flags
#define FL_GODMODE              0x00000010
#define FL_NOTARGET             0x00000020
#define FL_DEFENSE_CROUCH       0x00000100  // warzombie defense pose
#define FL_TEAMSLAVE            0x00000400  // not the first on the team
#define FL_NO_KNOCKBACK         0x00000800
#define FL_DROPPED_ITEM         0x00001000
#define FL_NO_BOTS              0x00002000  // spawn point not for bot use
#define FL_NO_HUMANS            0x00004000  // spawn point just for bots
#define FL_AI_GRENADE_KICK      0x00008000  // an AI has already decided to kick this grenade
// Rafael
#define FL_NOFATIGUE            0x00010000  // cheat flag no fatigue

#define FL_TOGGLE               0x00020000  //----(SA)	ent is toggling (doors use this for ex.)
#define FL_KICKACTIVATE         0x00040000  //----(SA)	ent has been activated by a kick (doors use this too for ex.)
#define FL_SOFTACTIVATE         0x00000040  //----(SA)	ent has been activated while 'walking' (doors use this too for ex.)
#define FL_DEFENSE_GUARD        0x00080000  // warzombie defense pose

#define FL_PARACHUTE            0x00100000
#define FL_WARZOMBIECHARGE      0x00200000
#define FL_NO_MONSTERSLICK      0x00400000
#define FL_NO_HEADCHECK         0x00800000

#define FL_NODRAW               0x01000000
#define FL_DOORNOISE            0x02000000  //----(SA)	added

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_POS3,
	MOVER_1TO2,
	MOVER_2TO1,
	// JOSEPH 1-26-00
	MOVER_2TO3,
	MOVER_3TO2,
	// END JOSEPH

	// Rafael
	MOVER_POS1ROTATE,
	MOVER_POS2ROTATE,
	MOVER_1TO2ROTATE,
	MOVER_2TO1ROTATE
} moverState_t;


// door AI sound ranges
#define HEAR_RANGE_DOOR_LOCKED      128 // really close since this is a cruel check
#define HEAR_RANGE_DOOR_KICKLOCKED  512
#define HEAR_RANGE_DOOR_OPEN        256
#define HEAR_RANGE_DOOR_KICKOPEN    768


#define SP_PODIUM_MODEL     "models/mapobjects/podium/podium4.md3"

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

//====================================================================
//
// Scripting, these structure are not saved into savegames (parsed each start)
typedef struct
{
	char    *actionString;
	qboolean ( *actionFunc )( gentity_t *ent, char *params );
} g_script_stack_action_t;
//
typedef struct
{
	//
	// set during script parsing
	g_script_stack_action_t     *action;            // points to an action to perform
	char                        *params;
} g_script_stack_item_t;
//
#define G_MAX_SCRIPT_STACK_ITEMS    64
//
typedef struct
{
	g_script_stack_item_t items[G_MAX_SCRIPT_STACK_ITEMS];
	int numItems;
} g_script_stack_t;
//
typedef struct
{
	int eventNum;                           // index in scriptEvents[]
	char                *params;            // trigger targetname, etc
	g_script_stack_t stack;
} g_script_event_t;
//
typedef struct
{
	char        *eventStr;
	qboolean ( *eventMatch )( g_script_event_t *event, char *eventParm );
} g_script_event_define_t;
//
// Script Flags
#define SCFL_GOING_TO_MARKER    0x1
#define SCFL_ANIMATING          0x2
#define SCFL_WAITING_RESTORE    0x4
//
// Scripting Status (NOTE: this MUST NOT contain any pointer vars)
typedef struct
{
	int scriptStackHead, scriptStackChangeTime;
	int scriptEventIndex;       // current event containing stack of actions to perform
	// scripting system variables
	int scriptId;                   // incremented each time the script changes
	int scriptFlags;
	char    *animatingParams;
} g_script_status_t;
//
#define G_MAX_SCRIPT_ACCUM_BUFFERS  8
//
void G_Script_ScriptEvent( gentity_t *ent, char *eventStr, char *params );
//====================================================================


#define CFOFS( x ) ( (int)&( ( (gclient_t *)0 )->x ) )

struct gentity_s {
	entityState_t s;                // communicated by server to clients
	entityShared_t r;               // shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s    *client;            // NULL if not a client

	qboolean inuse;

	char        *classname;         // set in QuakeEd
	int spawnflags;                 // set in QuakeEd

	qboolean neverFree;             // if true, FreeEntity will only unlink
									// bodyque uses this

	int flags;                      // FL_* variables

	char        *model;
	char        *model2;
	int freetime;                   // level.time when the object was freed

	int eventTime;                  // events will be cleared EVENT_VALID_MSEC after set
	qboolean freeAfterEvent;
	qboolean unlinkAfterEvent;

	qboolean physicsObject;         // if true, it can be pushed by movers and fall off edges
									// all game items are physicsObjects,
	float physicsBounce;            // 1.0 = continuous bounce, 0.0 = no bounce
	int clipmask;                   // brushes with this content value will be collided against
									// when moving.  items and corpses do not collide against
									// players, for instance

	// movers
	moverState_t moverState;
	int soundPos1;
	int sound1to2;
	int sound2to1;
	int soundPos2;
	int soundLoop;
	// JOSEPH 1-26-00
	int sound2to3;
	int sound3to2;
	int soundPos3;
	// END JOSEPH

	int soundKicked;
	int soundKickedEnd;

	int soundSoftopen;
	int soundSoftendo;
	int soundSoftclose;
	int soundSoftendc;

	gentity_t   *parent;
	gentity_t   *nextTrain;
	gentity_t   *prevTrain;
	// JOSEPH 1-26-00
	vec3_t pos1, pos2, pos3;
	// END JOSEPH

	char        *message;

	int timestamp;              // body queue sinking, etc

	float angle;                // set in editor, -1 = up, -2 = down
	char        *target;
	char        *targetdeath;   // fire this on death exclusively //----(SA)	added
	char        *targetname;
	char        *team;
	char        *targetShaderName;
	char        *targetShaderNewName;
	gentity_t   *target_ent;

	float speed;
	float closespeed;           // for movers that close at a different speed than they open
	vec3_t movedir;

	int gDuration;
	int gDurationBack;
	vec3_t gDelta;
	vec3_t gDeltaBack;

	int nextthink;
	void ( *think )( gentity_t *self );
	void ( *reached )( gentity_t *self );       // movers call this when hitting endpoint
	void ( *blocked )( gentity_t *self, gentity_t *other );
	void ( *touch )( gentity_t *self, gentity_t *other, trace_t *trace );
	void ( *use )( gentity_t *self, gentity_t *other, gentity_t *activator );
	void ( *pain )( gentity_t *self, gentity_t *attacker, int damage, vec3_t point );
	void ( *die )( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );

	int pain_debounce_time;
	int fly_sound_debounce_time;            // wind tunnel
	int last_move_time;

	int health;

	qboolean takedamage;

	int damage;
	int splashDamage;           // quad will increase this without increasing radius
	int splashRadius;
	int methodOfDeath;
	int splashMethodOfDeath;

	int count;

	gentity_t   *chain;
	gentity_t   *enemy;
	gentity_t   *activator;
	gentity_t   *teamchain;     // next entity in team
	gentity_t   *teammaster;    // master of the team

	int watertype;
	int waterlevel;

	int noise_index;

	// timing variables
	float wait;
	float random;

	// Rafael - sniper variable
	// sniper uses delay, random, radius
	int radius;
	float delay;

	// JOSEPH 10-11-99
	int TargetFlag;
	float duration;
	vec3_t rotate;
	vec3_t TargetAngles;
	// END JOSEPH

	gitem_t     *item;          // for bonus items

	// Ridah, AI fields
	char        *aiAttributes;
	char        *aiName;
	int aiTeam;
	void ( *AIScript_AlertEntity )( gentity_t *ent );
	qboolean aiInactive;
	int aiCharacter;            // the index of the type of character we are (from aicast_soldier.c)
	// done.

	char        *aiSkin;
	char        *aihSkin;

	vec3_t dl_color;
	char        *dl_stylestring;
	char        *dl_shader;
	int dl_atten;


	int key;                    // used by:  target_speaker->nopvs,

	qboolean active;
	qboolean botDelayBegin;

	// Rafael - mg42
	float harc;
	float varc;

	//----(SA)	added
	float activateArc;              // right now just for mg42, but available for setting what angle this ent can be touched/killed from

	int props_frame_state;

	// Ridah
	int missionLevel;                   // highest mission level completed (for previous level de-briefings)
	int missionObjectives;              // which objectives for the current level have been met
										// gets reset each new level

	int numSecretsFound;                //----(SA)	added to get into savegame
	int numTreasureFound;               //----(SA)	added to get into savegame

	// done.

	// Rafael
	qboolean is_dead;
	// done

	int start_size;
	int end_size;

	// Rafael props

	qboolean isProp;

	int mg42BaseEnt;

	gentity_t   *melee;

	char        *spawnitem;

	qboolean nopickup;

	int flameQuota, flameQuotaTime, flameBurnEnt;

	int count2;

	int grenadeExplodeTime;         // we've caught a grenade, which was due to explode at this time
	int grenadeFired;               // the grenade entity we last fired

	int mg42ClampTime;              // time to wait before an AI decides to ditch the mg42

	char        *track;

	// entity scripting system
	char                *scriptName;

	int numScriptEvents;
	g_script_event_t    *scriptEvents;  // contains a list of actions to perform for each event type
	g_script_status_t scriptStatus;     // current status of scripting
	g_script_status_t scriptStatusBackup;
	// the accumulation buffer
	int scriptAccumBuffer[G_MAX_SCRIPT_ACCUM_BUFFERS];

	qboolean AASblocking;
	float accuracy;

	char        *tagName;       // name of the tag we are attached to
	gentity_t   *tagParent;

	float headshotDamageScale;

	g_script_status_t scriptStatusCurrent;      // had to go down here to keep savegames compatible

	int emitID;                 //----(SA)	added
	int emitNum;                //----(SA)	added
	int emitPressure;           //----(SA)	added
	int emitTime;               //----(SA)	added

	// -------------------------------------------------------------------------------------------
	// if working on a post release patch, new variables should ONLY be inserted after this point
};

// Ridah
#include "ai_cast_global.h"
// done.

typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum {
	TEAM_BEGIN,     // Beginning a team game, spawn at base
	TEAM_ACTIVE     // Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t state;

	int location;

	int captures;
	int basedefense;
	int carrierdefense;
	int flagrecovery;
	int fragcarrier;
	int assists;

	float lasthurtcarrier;
	float lastreturnedflag;
	float flagsince;
	float lastfraggedcarrier;
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define FOLLOW_ACTIVE1  -1
#define FOLLOW_ACTIVE2  -2

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t sessionTeam;
	int spectatorTime;              // for determining next-in-line to play
	spectatorState_t spectatorState;
	int spectatorClient;            // for chasecam and follow mode
	int wins, losses;               // tournament stats
	int playerType;                 // DHM - Nerve :: for GT_WOLF
	int playerWeapon;               // DHM - Nerve :: for GT_WOLF
	int playerPistol;               // DHM - Nerve :: for GT_WOLF
	int playerItem;                 // DHM - Nerve :: for GT_WOLF
	int playerSkin;                 // DHM - Nerve :: for GT_WOLF
} clientSession_t;

//
#define MAX_NETNAME         36
#define MAX_VOTE_COUNT      3

#define PICKUP_ACTIVATE 0   // pickup items only when using "+activate"
#define PICKUP_TOUCH    1   // pickup items when touched
#define PICKUP_FORCE    2   // pickup the next item when touched (and reset to PICKUP_ACTIVATE when done)

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t connected;
	usercmd_t cmd;                  // we would lose angles if not persistant
	usercmd_t oldcmd;               // previous command processed by pmove()
	qboolean localClient;           // true if "ip" info key is "localhost"
	qboolean initialSpawn;          // the first spawn should be at a cool location
	qboolean predictItemPickup;     // based on cg_predictItems userinfo
	qboolean pmoveFixed;            //
	char netname[MAX_NETNAME];

	int autoActivate;               // based on cg_autoactivate userinfo		(uses the PICKUP_ values above)
	int emptySwitch;                // based on cg_emptyswitch userinfo (means "switch my weapon for me when ammo reaches '0' rather than -1)

	int maxHealth;                  // for handicapping
	int enterTime;                  // level.time the client entered the game
	playerTeamState_t teamState;    // status in teamplay games
	int voteCount;                  // to prevent people from constantly calling votes
	int teamVoteCount;              // to prevent people from constantly calling votes
	qboolean teamInfo;              // send team overlay updates?
} clientPersistant_t;


// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t ps;               // communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t pers;
	clientSession_t sess;

	qboolean readyToExit;           // wishes to leave the intermission

	qboolean noclip;

	int lastCmdTime;                // level.time of last usercmd_t, for EF_CONNECTION
									// we can't just use pers.lastCommand.time, because
									// of the g_sycronousclients case
	int buttons;
	int oldbuttons;
	int latched_buttons;

	int wbuttons;
	int oldwbuttons;
	int latched_wbuttons;
	vec3_t oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_armor;               // damage absorbed by armor
	int damage_blood;               // damage taken out of health
	int damage_knockback;           // impact damage
	vec3_t damage_from;             // origin for vector calculation
	qboolean damage_fromWorld;      // if true, don't use the damage_from vector

	int accurateCount;              // for "impressive" reward sound

	int accuracy_shots;             // total number of shots
	int accuracy_hits;              // total number of hits

	//
	int lastkilled_client;          // last client that this client killed
	int lasthurt_client;            // last client that damaged this client
	int lasthurt_mod;               // type of damage the client did

	// timers
	int respawnTime;                // can respawn when time > this, force after g_forcerespwan
	int inactivityTime;             // kick players when time > this
	qboolean inactivityWarning;     // qtrue if the five seoond warning has been given
	int rewardTime;                 // clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int airOutTime;

	int lastKillTime;               // for multiple kill rewards

	qboolean fireHeld;              // used for hook
	gentity_t   *hook;              // grapple hook if out

	int switchTeamTime;             // time the player switched teams

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int timeResidual;

	float currentAimSpreadScale;

	int medicHealAmt;

	// RF, may be shared by multiple clients/characters
	animModelInfo_t *modelInfo;

	// -------------------------------------------------------------------------------------------
	// if working on a post release patch, new variables should ONLY be inserted after this point

	gentity_t   *persistantPowerup;
	int portalID;
	int ammoTimes[WP_NUM_WEAPONS];
	int invulnerabilityTime;

	gentity_t   *cameraPortal;              // grapple hook if out
	vec3_t cameraOrigin;

	int limboDropWeapon;         // JPW NERVE weapon to drop in limbo
	int deployQueueNumber;         // JPW NERVE player order in reinforcement FIFO queue
	int sniperRifleFiredTime;         // JPW NERVE last time a sniper rifle was fired (for muzzle flip effects)
	float sniperRifleMuzzleYaw;       // JPW NERVE for time-dependent muzzle flip in multiplayer
	float sniperRifleMuzzlePitch;       // (SA) added

	int saved_persistant[MAX_PERSISTANT];           // DHM - Nerve :: Save ps->persistant here during Limbo
};



//
// this structure is cleared as each map is entered
//
#define MAX_SPAWN_VARS          64
#define MAX_SPAWN_VARS_CHARS    2048

typedef struct {
	struct gclient_s    *clients;       // [maxclients]

	struct gentity_s    *gentities;
	int gentitySize;
	int num_entities;               // current number, <= MAX_GENTITIES

	int warmupTime;                 // restart match at this time

	fileHandle_t logFile;

	// store latched cvars here that we want to get at often
	int maxclients;

	int framenum;
	int time;                           // in msec
	int previousTime;                   // so movers can back up when blocked

	int startTime;                      // level.time the map was started

	int teamScores[TEAM_NUM_TEAMS];
	int lastTeamLocationTime;               // last time of client team location update

	qboolean newSession;                // don't use any old session data, because
										// we changed gametype

	qboolean restarted;                 // waiting for a map_restart to fire

	int numConnectedClients;
	int numNonSpectatorClients;         // includes connecting clients
	int numPlayingClients;              // connected, non-spectators
	int sortedClients[MAX_CLIENTS];             // sorted by score
	int follow1, follow2;               // clientNums for auto-follow spectators

	int snd_fry;                        // sound index for standing in lava

	int warmupModificationCount;            // for detecting if g_warmup is changed

	// voting state
	char voteString[MAX_STRING_CHARS];
	char voteDisplayString[MAX_STRING_CHARS];
	int voteTime;                       // level.time vote was called
	int voteExecuteTime;                // time the vote is executed
	int voteYes;
	int voteNo;
	int numVotingClients;               // set by CalculateRanks

	// team voting state
	char teamVoteString[2][MAX_STRING_CHARS];
	int teamVoteTime[2];                // level.time vote was called
	int teamVoteYes[2];
	int teamVoteNo[2];
	int numteamVotingClients[2];        // set by CalculateRanks

	// spawn variables
	qboolean spawning;                  // the G_Spawn*() functions are valid
	int numSpawnVars;
	char        *spawnVars[MAX_SPAWN_VARS][2];  // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int intermissionQueued;             // intermission was qualified, but
										// wait INTERMISSION_DELAY_TIME before
										// actually going there so the last
										// frag can be watched.  Disable future
										// kills during this delay
	int intermissiontime;               // time the intermission was started
	char        *changemap;
	qboolean readyToExit;               // at least one client wants to exit
	int exitTime;
	vec3_t intermission_origin;         // also used for spectator spawns
	vec3_t intermission_angle;

	qboolean locationLinked;            // target_locations get linked
	gentity_t   *locationHead;          // head of the location list
	int bodyQueIndex;                   // dead bodies
	gentity_t   *bodyQue[BODY_QUEUE_SIZE];

	int portalSequence;
	// Ridah
	char        *scriptAI;
	int reloadPauseTime;                // don't think AI/client's until this time has elapsed
	int reloadDelayTime;                // don't start loading the savegame until this has expired

	int lastGrenadeKick;

	int loperZapSound;
	int stimSoldierFlySound;
	int bulletRicochetSound;
	// done.

	int snipersound;

//----(SA)	added
	int numSecrets;
	int numTreasure;
	int numArtifacts;
	int numObjectives;
//----(SA)	end

	int knifeSound[4];

// JPW NERVE
	int redReinforceTime, blueReinforceTime;         // last time reinforcements arrived in ms
	int redNumWaiting, blueNumWaiting;         // number of reinforcements in queue
	vec3_t spawntargets[MAX_MULTI_SPAWNTARGETS];      // coordinates of spawn targets
	int numspawntargets;         // # spawntargets in this map
// jpw

	// RF, entity scripting
	char        *scriptEntity;

	// player/AI model scripting (server repository)
	animScriptData_t animScriptData;

	// next map to load
	char nextMap[MAX_STRING_CHARS];

	// RF, record last time we loaded, so we can hack around sighting issues on reload
	int lastLoadTime;

} level_locals_t;

//extern    qboolean	reloading;				// loading up a savegame

//
// g_spawn.c
//
qboolean    G_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean    G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean    G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean    G_SpawnVector( const char *key, const char *defaultString, float *out );
void        G_SpawnEntitiesFromString( void );
char *G_NewString( const char *string );
// Ridah
qboolean G_CallSpawn( gentity_t *ent );
// done.

//
// g_cmds.c
//
void Cmd_Score_f( gentity_t *ent );
void StopFollowing( gentity_t *ent );
//void BroadcastTeamChange( gclient_t *client, int oldTeam );
void SetTeam( gentity_t *ent, char *s );
void SetWolfData( gentity_t *ent, char *ptype, char *weap, char *pistol, char *grenade, char *skinnum );    // DHM - Nerve
void Cmd_FollowCycle_f( gentity_t *ent, int dir );

//
// g_items.c
//
void G_CheckTeamItems( void );
void G_RunItem( gentity_t *ent );
void RespawnItem( gentity_t *ent );

void UseHoldableItem( gentity_t *ent, int item );
void PrecacheItem( gitem_t *it );
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle, qboolean novelocity );
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity );
void SetRespawn( gentity_t *ent, float delay );
void G_SpawnItem( gentity_t *ent, gitem_t *item );
void FinishSpawningItem( gentity_t *ent );
void Think_Weapon( gentity_t *ent );
int ArmorIndex( gentity_t *ent );
void Fill_Clip( playerState_t *ps, int weapon );
void    Add_Ammo( gentity_t *ent, int weapon, int count, qboolean fillClip );
void Touch_Item( gentity_t *ent, gentity_t *other, trace_t *trace );

// Touch_Item_Auto is bound by the rules of autoactivation (if cg_autoactivate is 0, only touch on "activate")
void Touch_Item_Auto( gentity_t *ent, gentity_t *other, trace_t *trace );

void ClearRegisteredItems( void );
void RegisterItem( gitem_t *item );
void SaveRegisteredItems( void );
void Prop_Break_Sound( gentity_t *ent );
void Spawn_Shard( gentity_t *ent, gentity_t *inflictor, int quantity, int type );

//
// g_utils.c
//
// Ridah
int G_FindConfigstringIndex( const char *name, int start, int max, qboolean create );
// done.
int G_ModelIndex( char *name );
int     G_SoundIndex( const char *name );
void    G_TeamCommand( team_t team, char *cmd );
void    G_KillBox( gentity_t *ent );
gentity_t *G_Find( gentity_t *from, int fieldofs, const char *match );
gentity_t *G_PickTarget( char *targetname );
void    G_UseTargets( gentity_t *ent, gentity_t *activator );
void    G_SetMovedir( vec3_t angles, vec3_t movedir );

void    G_InitGentity( gentity_t *e );
gentity_t   *G_Spawn( void );
gentity_t *G_TempEntity( vec3_t origin, int event );
void    G_Sound( gentity_t *ent, int soundIndex );
void    G_AnimScriptSound( int soundIndex, vec3_t org, int client );
void    G_FreeEntity( gentity_t *e );
//qboolean	G_EntitiesFree( void );

void    G_TouchTriggers( gentity_t *ent );
void    G_TouchSolids( gentity_t *ent );

float   *tv( float x, float y, float z );
char    *vtos( const vec3_t v );

void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void G_AddEvent( gentity_t *ent, int event, int eventParm );
void G_SetOrigin( gentity_t *ent, vec3_t origin );
void AddRemap( const char *oldShader, const char *newShader, float timeOffset );
const char *BuildShaderStateConfig();
void G_SetAngle( gentity_t *ent, vec3_t angle );

qboolean infront( gentity_t *self, gentity_t *other );

void G_ProcessTagConnect( gentity_t *ent, qboolean clearAngles );

//
// g_combat.c
//
qboolean CanDamage( gentity_t *targ, vec3_t origin );
void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod );
qboolean G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod );
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
void TossClientItems( gentity_t *self );

// damage flags
#define DAMAGE_RADIUS               0x00000001  // damage was indirect
#define DAMAGE_NO_ARMOR             0x00000002  // armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK         0x00000008  // do not affect velocity, just view angles
#define DAMAGE_NO_TEAM_PROTECTION   0x00000010  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_PROTECTION        0x00000020  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_PASSTHRU             0x00000040  // damage came through an explosive, or other player, or has in some way already given damage to something

//
// g_missile.c
//
void G_RunMissile( gentity_t *ent );
int G_PredictMissile( gentity_t *ent, int duration, vec3_t endPos, qboolean allowBounce );

// Rafael zombiespit
void G_RunSpit( gentity_t *ent );
void G_RunDebris( gentity_t *ent );

void G_RunCrowbar( gentity_t *ent );

//----(SA) removed unused q3a weapon firing
gentity_t *fire_grenade( gentity_t *self, vec3_t start, vec3_t aimdir, int grenadeWPID );
gentity_t *fire_rocket( gentity_t *self, vec3_t start, vec3_t dir );


// Rafael sniper
void fire_lead( gentity_t *self,  vec3_t start, vec3_t dir, int damage );
qboolean visible( gentity_t *self, gentity_t *other );

gentity_t *fire_mortar( gentity_t *self, vec3_t start, vec3_t dir );

gentity_t *fire_zombiespit( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_zombiespirit( gentity_t *self, gentity_t *bolt, vec3_t start, vec3_t dir );
gentity_t *fire_crowbar( gentity_t *self, vec3_t start, vec3_t dir );
gentity_t *fire_flamebarrel( gentity_t *self, vec3_t start, vec3_t dir );
// done

//
// g_mover.c
//
void G_RunMover( gentity_t *ent );
void Use_BinaryMover( gentity_t *ent, gentity_t *other, gentity_t *activator );
void G_Activate( gentity_t *ent, gentity_t *activator );

void G_TryDoor( gentity_t *ent, gentity_t *other, gentity_t *activator ); //----(SA)	added

void InitMoverRotate( gentity_t *ent );

void InitMover( gentity_t *ent );
void SetMoverState( gentity_t *ent, moverState_t moverState, int time );

//
// g_tramcar.c
//
void Reached_Tramcar( gentity_t *ent );


//
// g_misc.c
//
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles );


//
// g_weapon.c
//
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker );
void CalcMuzzlePoint( gentity_t *ent, int weapon, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
void SnapVectorTowards( vec3_t v, vec3_t to );
trace_t *CheckMeleeAttack( gentity_t *ent, float dist, qboolean isTest );
gentity_t *weapon_grenadelauncher_fire( gentity_t *ent, int grenadeWPID );
// Rafael
gentity_t *weapon_crowbar_throw( gentity_t *ent );

void CalcMuzzlePoints( gentity_t *ent, int weapon );
//----(SA) commented out as we have no hook
//void Weapon_HookFree (gentity_t *ent);
//void Weapon_HookThink (gentity_t *ent);

// Rafael - for activate
void CalcMuzzlePointForActivate( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
// done.

//
// g_client.c
//
team_t PickTeam( int ignoreClientNum );
void SetClientViewAngle( gentity_t *ent, vec3_t angle );
gentity_t *SelectSpawnPoint( vec3_t avoidPoint, vec3_t origin, vec3_t angles );
void respawn( gentity_t *ent );
void BeginIntermission( void );
void InitClientPersistant( gclient_t *client );
void InitClientResp( gclient_t *client );
void InitBodyQue( void );
void ClientSpawn( gentity_t *ent );
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod );
void AddScore( gentity_t *ent, int score );
void CalculateRanks( void );
qboolean SpotWouldTelefrag( gentity_t *spot );
qboolean G_GetModelInfo( int clientNum, char *modelName, animModelInfo_t **modelInfo );

//
// g_svcmds.c
//
qboolean    ConsoleCommand( void );
void G_ProcessIPBans( void );
qboolean G_FilterPacket( char *from );

//
// g_weapon.c
//
void FireWeapon( gentity_t *ent );

//
// p_hud.c
//
void MoveClientToIntermission( gentity_t *client );
void G_SetStats( gentity_t *ent );
void DeathmatchScoreboardMessage( gentity_t *client );

//
// g_cmds.c
//
void G_SayTo( gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message ); // JPW NERVE removed static declaration so it would link

//
// g_pweapon.c
//


//
// g_main.c
//
void FindIntermissionPoint( void );
void G_RunThink( gentity_t *ent );
void QDECL G_LogPrintf( const char *fmt, ... );
void SendScoreboardMessageToAllClients( void );
void QDECL G_Printf( const char *fmt, ... );
void QDECL G_DPrintf( const char *fmt, ... );
void QDECL G_Error( const char *fmt, ... );
//----(SA)	added
void G_EndGame( void );
int G_SendMissionStats( void );   // return '0' if objectives not met, '1' if met
void G_ChangeLevel( char *mapName );
//----(SA)	end

//
// g_client.c
//
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot );
void ClientUserinfoChanged( int clientNum );
void ClientDisconnect( int clientNum );
void ClientBegin( int clientNum );
void ClientCommand( int clientNum );

//
// g_active.c
//
void ClientThink( int clientNum );
void ClientEndFrame( gentity_t *ent );
void G_RunClient( gentity_t *ent );

//
// g_team.c
//
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 );


//
// g_mem.c
//
void *G_Alloc( int size );
void G_InitMemory( void );
void Svcmd_GameMem_f( void );

//
// g_session.c
//
void G_ReadSessionData( gclient_t *client );
void G_InitSessionData( gclient_t *client, char *userinfo );

void G_InitWorldSession( void );
void G_WriteSessionData( void );

//
// g_bot.c
//
//void G_InitBots( qboolean restart );
char *G_GetBotInfoByNumber( int num );
char *G_GetBotInfoByName( const char *name );
void G_CheckBotSpawn( void );
void G_QueueBotBegin( int clientNum );
qboolean G_BotConnect( int clientNum, qboolean restart );
void Svcmd_AddBot_f( void );

// ai_main.c
#define MAX_FILEPATH            144

//bot settings
typedef struct bot_settings_s
{
	char characterfile[MAX_FILEPATH];
	float skill;
	char team[MAX_FILEPATH];
} bot_settings_t;

int BotAISetup( int restart );
int BotAIShutdown( int restart );
int BotAILoadMap( int restart );
int BotAISetupClient( int client, struct bot_settings_s *settings );
int BotAIShutdownClient( int client );
int BotAIStartFrame( int time );
void BotTestAAS( vec3_t origin );


// g_cmd.c
void Cmd_Activate_f( gentity_t *ent );
int Cmd_WolfKick_f( gentity_t *ent );
// Ridah

// g_save.c
qboolean G_SaveGame( char *username );
void G_LoadGame( char *username );
qboolean G_SavePersistant( char *nextmap );
void G_LoadPersistant( void );

// g_script.c
void G_Script_ScriptParse( gentity_t *ent );
qboolean G_Script_ScriptRun( gentity_t *ent );
void G_Script_ScriptEvent( gentity_t *ent, char *eventStr, char *params );
void G_Script_ScriptLoad( void );

float AngleDifference( float ang1, float ang2 );

// g_props.c
void Props_Chair_Skyboxtouch( gentity_t *ent );

#include "g_team.h" // teamplay specific stuff


extern level_locals_t level;
extern gentity_t g_entities[MAX_GENTITIES];
extern gentity_t       *g_camEnt;

#define FOFS( x ) ( (int)&( ( (gentity_t *)0 )->x ) )

extern vmCvar_t g_gametype;

// Rafael gameskill
extern vmCvar_t g_gameskill;
// done

extern vmCvar_t g_reloading;        //----(SA)	added

extern vmCvar_t g_dedicated;
extern vmCvar_t g_cheats;
extern vmCvar_t g_maxclients;               // allow this many total, including spectators
extern vmCvar_t g_maxGameClients;           // allow this many active
extern vmCvar_t g_restarted;

extern vmCvar_t g_dmflags;
extern vmCvar_t g_fraglimit;
extern vmCvar_t g_timelimit;
extern vmCvar_t g_capturelimit;
extern vmCvar_t g_friendlyFire;
extern vmCvar_t g_password;
extern vmCvar_t g_needpass;
extern vmCvar_t g_gravity;
extern vmCvar_t g_speed;
extern vmCvar_t g_knockback;
extern vmCvar_t g_quadfactor;
extern vmCvar_t g_forcerespawn;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_debugAlloc;
extern vmCvar_t g_debugDamage;
extern vmCvar_t g_debugBullets;     //----(SA)	added
extern vmCvar_t g_debugAudibleEvents;       //----(SA)	added
extern vmCvar_t g_headshotMaxDist;      //----(SA)	added
extern vmCvar_t g_weaponRespawn;
extern vmCvar_t g_syncronousClients;
extern vmCvar_t g_motd;
extern vmCvar_t g_warmup;
extern vmCvar_t g_blood;
extern vmCvar_t g_allowVote;

extern vmCvar_t g_needpass;
extern vmCvar_t g_weaponTeamRespawn;
extern vmCvar_t g_doWarmup;
extern vmCvar_t g_teamAutoJoin;
extern vmCvar_t g_teamForceBalance;
extern vmCvar_t g_banIPs;
extern vmCvar_t g_filterBan;
extern vmCvar_t g_rankings;
extern vmCvar_t g_enableBreath;
extern vmCvar_t g_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

//Rafael
extern vmCvar_t g_autoactivate;

extern vmCvar_t g_testPain;

extern vmCvar_t g_missionStats;
extern vmCvar_t ai_scriptName;          // name of AI script file to run (instead of default for that map)
extern vmCvar_t g_scriptName;           // name of script file to run (instead of default for that map)

extern vmCvar_t g_scriptDebug;

extern vmCvar_t g_userAim;

extern vmCvar_t g_forceModel;

extern vmCvar_t g_mg42arc;

extern vmCvar_t g_totalPlayTime;
extern vmCvar_t g_attempts;

extern vmCvar_t g_footstepAudibleRange;
// JPW NERVE multiplayer
extern vmCvar_t g_redlimbotime;
extern vmCvar_t g_bluelimbotime;
extern vmCvar_t g_medicChargeTime;
extern vmCvar_t g_engineerChargeTime;
extern vmCvar_t g_LTChargeTime;
extern vmCvar_t g_soldierChargeTime;
// jpw

extern vmCvar_t g_playerStart;      //----(SA)	added


void    trap_Printf( const char *fmt );
void    trap_Error( const char *fmt );
void    trap_Endgame( void );   //----(SA)	added
int     trap_Milliseconds( void );
int     trap_Argc( void );
void    trap_Argv( int n, char *buffer, int bufferLength );
void    trap_Args( char *buffer, int bufferLength );
int     trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void    trap_FS_Read( void *buffer, int len, fileHandle_t f );
int     trap_FS_Write( const void *buffer, int len, fileHandle_t f );
int     trap_FS_Rename( const char *from, const char *to );
void    trap_FS_FCloseFile( fileHandle_t f );
int     trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
void    trap_SendConsoleCommand( int exec_when, const char *text );
void    trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void    trap_Cvar_Update( vmCvar_t *cvar );
void    trap_Cvar_Set( const char *var_name, const char *value );
int     trap_Cvar_VariableIntegerValue( const char *var_name );
float   trap_Cvar_VariableValue( const char *var_name );
void    trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void    trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *gameClients, int sizeofGameClient );
void    trap_DropClient( int clientNum, const char *reason );
void    trap_SendServerCommand( int clientNum, const char *text );
void    trap_SetConfigstring( int num, const char *string );
void    trap_GetConfigstring( int num, char *buffer, int bufferSize );
void    trap_GetUserinfo( int num, char *buffer, int bufferSize );
void    trap_SetUserinfo( int num, const char *buffer );
void    trap_GetServerinfo( char *buffer, int bufferSize );
void    trap_SetBrushModel( gentity_t *ent, const char *name );
void    trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
void    trap_TraceCapsule( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
int     trap_PointContents( const vec3_t point, int passEntityNum );
qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 );
qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );
void    trap_AdjustAreaPortalState( gentity_t *ent, qboolean open );
qboolean trap_AreasConnected( int area1, int area2 );
void    trap_LinkEntity( gentity_t *ent );
void    trap_UnlinkEntity( gentity_t *ent );
int     trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount );
qboolean trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
qboolean trap_EntityContactCapsule( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
int     trap_BotAllocateClient( void );
void    trap_BotFreeClient( int clientNum );
void    trap_GetUsercmd( int clientNum, usercmd_t *cmd );
qboolean    trap_GetEntityToken( char *buffer, int bufferSize );
qboolean trap_GetTag( int clientNum, char *tagName, orientation_t * or );

int     trap_DebugPolygonCreate( int color, int numPoints, vec3_t *points );
void    trap_DebugPolygonDelete( int id );

int     trap_BotLibSetup( void );
int     trap_BotLibShutdown( void );
int     trap_BotLibVarSet( char *var_name, char *value );
int     trap_BotLibVarGet( char *var_name, char *value, int size );
int     trap_BotLibDefine( char *string );
int     trap_BotLibStartFrame( float time );
int     trap_BotLibLoadMap( const char *mapname );
int     trap_BotLibUpdateEntity( int ent, void /* struct bot_updateentity_s */ *bue );
int     trap_BotLibTest( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 );

int     trap_BotGetSnapshotEntity( int clientNum, int sequence );
int     trap_BotGetServerCommand( int clientNum, char *message, int size );
//int		trap_BotGetConsoleMessage(int clientNum, char *message, int size);
void    trap_BotUserCommand( int client, usercmd_t *ucmd );

void        trap_AAS_EntityInfo( int entnum, void /* struct aas_entityinfo_s */ *info );

int         trap_AAS_Initialized( void );
void        trap_AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs );
float       trap_AAS_Time( void );

// Ridah
void        trap_AAS_SetCurrentWorld( int index );
// done.

int         trap_AAS_PointAreaNum( vec3_t point );
int         trap_AAS_TraceAreas( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas );

int         trap_AAS_PointContents( vec3_t point );
int         trap_AAS_NextBSPEntity( int ent );
int         trap_AAS_ValueForBSPEpairKey( int ent, char *key, char *value, int size );
int         trap_AAS_VectorForBSPEpairKey( int ent, char *key, vec3_t v );
int         trap_AAS_FloatForBSPEpairKey( int ent, char *key, float *value );
int         trap_AAS_IntForBSPEpairKey( int ent, char *key, int *value );

int         trap_AAS_AreaReachability( int areanum );

int         trap_AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags );

int         trap_AAS_Swimming( vec3_t origin );
int         trap_AAS_PredictClientMovement( void /* aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize );

// Ridah, route-tables
void        trap_AAS_RT_ShowRoute( vec3_t srcpos, int srcnum, int destnum );
qboolean    trap_AAS_RT_GetHidePos( vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos );
int         trap_AAS_FindAttackSpotWithinRange( int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float *outpos );
qboolean    trap_AAS_GetRouteFirstVisPos( vec3_t srcpos, vec3_t destpos, int travelflags, vec3_t retpos );
void        trap_AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, qboolean blocking );
// done.

void    trap_EA_Say( int client, char *str );
void    trap_EA_SayTeam( int client, char *str );
void    trap_EA_UseItem( int client, char *it );
void    trap_EA_DropItem( int client, char *it );
void    trap_EA_UseInv( int client, char *inv );
void    trap_EA_DropInv( int client, char *inv );
void    trap_EA_Gesture( int client );
void    trap_EA_Command( int client, char *command );

void    trap_EA_SelectWeapon( int client, int weapon );
void    trap_EA_Talk( int client );
void    trap_EA_Attack( int client );
void    trap_EA_Reload( int client );
void    trap_EA_Use( int client );
void    trap_EA_Respawn( int client );
void    trap_EA_Jump( int client );
void    trap_EA_DelayedJump( int client );
void    trap_EA_Crouch( int client );
void    trap_EA_MoveUp( int client );
void    trap_EA_MoveDown( int client );
void    trap_EA_MoveForward( int client );
void    trap_EA_MoveBack( int client );
void    trap_EA_MoveLeft( int client );
void    trap_EA_MoveRight( int client );
void    trap_EA_Move( int client, vec3_t dir, float speed );
void    trap_EA_View( int client, vec3_t viewangles );

void    trap_EA_EndRegular( int client, float thinktime );
void    trap_EA_GetInput( int client, float thinktime, void /* struct bot_input_s */ *input );
void    trap_EA_ResetInput( int client, void *init );


int     trap_BotLoadCharacter( char *charfile, int skill );
void    trap_BotFreeCharacter( int character );
float   trap_Characteristic_Float( int character, int index );
float   trap_Characteristic_BFloat( int character, int index, float min, float max );
int     trap_Characteristic_Integer( int character, int index );
int     trap_Characteristic_BInteger( int character, int index, int min, int max );
void    trap_Characteristic_String( int character, int index, char *buf, int size );

int     trap_BotAllocChatState( void );
void    trap_BotFreeChatState( int handle );
void    trap_BotQueueConsoleMessage( int chatstate, int type, char *message );
void    trap_BotRemoveConsoleMessage( int chatstate, int handle );
int     trap_BotNextConsoleMessage( int chatstate, void /* struct bot_consolemessage_s */ *cm );
int     trap_BotNumConsoleMessages( int chatstate );
void    trap_BotInitialChat( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int     trap_BotNumInitialChats( int chatstate, char *type );
int     trap_BotReplyChat( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int     trap_BotChatLength( int chatstate );
void    trap_BotEnterChat( int chatstate, int client, int sendto );
void    trap_BotGetChatMessage( int chatstate, char *buf, int size );
int     trap_StringContains( char *str1, char *str2, int casesensitive );
int     trap_BotFindMatch( char *str, void /* struct bot_match_s */ *match, unsigned long int context );
void    trap_BotMatchVariable( void /* struct bot_match_s */ *match, int variable, char *buf, int size );
void    trap_UnifyWhiteSpaces( char *string );
void    trap_BotReplaceSynonyms( char *string, unsigned long int context );
int     trap_BotLoadChatFile( int chatstate, char *chatfile, char *chatname );
void    trap_BotSetChatGender( int chatstate, int gender );
void    trap_BotSetChatName( int chatstate, char *name );
void    trap_BotResetGoalState( int goalstate );
void    trap_BotRemoveFromAvoidGoals( int goalstate, int number );
void    trap_BotResetAvoidGoals( int goalstate );
void    trap_BotPushGoal( int goalstate, void /* struct bot_goal_s */ *goal );
void    trap_BotPopGoal( int goalstate );
void    trap_BotEmptyGoalStack( int goalstate );
void    trap_BotDumpAvoidGoals( int goalstate );
void    trap_BotDumpGoalStack( int goalstate );
void    trap_BotGoalName( int number, char *name, int size );
int     trap_BotGetTopGoal( int goalstate, void /* struct bot_goal_s */ *goal );
int     trap_BotGetSecondGoal( int goalstate, void /* struct bot_goal_s */ *goal );
int     trap_BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags );
int     trap_BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime );
int     trap_BotTouchingGoal( vec3_t origin, void /* struct bot_goal_s */ *goal );
int     trap_BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal );
int     trap_BotGetNextCampSpotGoal( int num, void /* struct bot_goal_s */ *goal );
int     trap_BotGetMapLocationGoal( char *name, void /* struct bot_goal_s */ *goal );
int     trap_BotGetLevelItemGoal( int index, char *classname, void /* struct bot_goal_s */ *goal );
float   trap_BotAvoidGoalTime( int goalstate, int number );
void    trap_BotInitLevelItems( void );
void    trap_BotUpdateEntityItems( void );
int     trap_BotLoadItemWeights( int goalstate, char *filename );
void    trap_BotFreeItemWeights( int goalstate );
void    trap_BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child );
void    trap_BotSaveGoalFuzzyLogic( int goalstate, char *filename );
void    trap_BotMutateGoalFuzzyLogic( int goalstate, float range );
int     trap_BotAllocGoalState( int state );
void    trap_BotFreeGoalState( int handle );

void    trap_BotResetMoveState( int movestate );
void    trap_BotMoveToGoal( void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags );
int     trap_BotMoveInDirection( int movestate, vec3_t dir, float speed, int type );
void    trap_BotResetAvoidReach( int movestate );
void    trap_BotResetLastAvoidReach( int movestate );
int     trap_BotReachabilityArea( vec3_t origin, int testground );
int     trap_BotMovementViewTarget( int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target );
int     trap_BotPredictVisiblePosition( vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target );
int     trap_BotAllocMoveState( void );
void    trap_BotFreeMoveState( int handle );
void    trap_BotInitMoveState( int handle, void /* struct bot_initmove_s */ *initmove );
// Ridah
void    trap_BotInitAvoidReach( int handle );
// done.

int     trap_BotChooseBestFightWeapon( int weaponstate, int *inventory );
void    trap_BotGetWeaponInfo( int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo );
int     trap_BotLoadWeaponWeights( int weaponstate, char *filename );
int     trap_BotAllocWeaponState( void );
void    trap_BotFreeWeaponState( int weaponstate );
void    trap_BotResetWeaponState( int weaponstate );

int     trap_GeneticParentsAndChildSelection( int numranks, float *ranks, int *parent1, int *parent2, int *child );

void    trap_SnapVector( float *v );

typedef enum
{
	shard_glass = 0,
	shard_wood,
	shard_metal,
	shard_ceramic,
	shard_rubble
} shards_t;
