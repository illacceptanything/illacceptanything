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

//===========================================================================
//
// Name:			ai_cast.h
// Function:		Wolfenstein AI Character Routines
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "../botai/ai_main.h"    // just so we can use the structures
#include "../botai/ai_dmq3.h"    // just so we can use the structures

#include "ai_cast_fight.h"

//
// constants/defines
//
#define MAX_AIFUNCS         15      // if we go over this per frame, likely to have an infinite loop
//
#define SIGHT_PER_SEC       50      // do this many sight iterations per second
//
// Cast AI specific action flags (get translated into ucmd's
#define CASTACTION_WALK     1
//
#define MAX_SCRIPT_ACCUM_BUFFERS    8
//
#define AICAST_PRT_ALWAYS   0
#define AICAST_PRT_DEBUG    1
//
#define DEBUG_FOLLOW_DIST   96
//
#define MAX_LEADER_DIST     256
//
#define AASWORLD_STANDARD   0
#define AASWORLD_LARGE      1
//
// use this for returning the length of an anim
#define ANIMLENGTH( frames,fps )  ( ( frames * 1000 ) / fps )
//
#define AICAST_TFL_DEFAULT  TFL_DEFAULT & ~( TFL_JUMPPAD | TFL_ROCKETJUMP | TFL_BFGJUMP | TFL_GRAPPLEHOOK | TFL_DOUBLEJUMP | TFL_RAMPJUMP | TFL_STRAFEJUMP | TFL_LAVA ) //----(SA)	modified since slime is no longer deadly
//#define AICAST_TFL_DEFAULT	TFL_DEFAULT & ~(TFL_JUMPPAD|TFL_ROCKETJUMP|TFL_BFGJUMP|TFL_GRAPPLEHOOK|TFL_DOUBLEJUMP|TFL_RAMPJUMP|TFL_STRAFEJUMP|TFL_SLIME|TFL_LAVA)
//
// AI flags
#define AIFL_CATCH_GRENADE      0x1
#define AIFL_NO_FLAME_DAMAGE    0x2
#define AIFL_FIRED              0x4
#define AIFL_LAND_ANIM_PLAYED   0x8
#define AIFL_ROLL_ANIM          0x10
#define AIFL_FLIP_ANIM          0x20
#define AIFL_STAND_IDLE2        0x40
#define AIFL_NOAVOID            0x80    // if set, this AI will ignore requests for us to move out the way
#define AIFL_NOPAIN             0x100   // don't stop for pain anims
#define AIFL_WALKFORWARD        0x200   // only walk forward
#define AIFL_DENYACTION         0x400   // used by scripting to prevent dynamic code from executing certain behaviour
#define AIFL_VIEWLOCKED         0x800   // prevent anything outside movement routines from changing view
#define AIFL_CORPSESIGHTING     0x1000  // share information through friendly corpses
#define AIFL_WAITINGTOSPAWN     0x2000  // waiting until space is clear to spawn in to the game
#define AIFL_JUST_SPAWNED       0x4000
#define AIFL_NO_RELOAD          0x8000  // this character doesn't need to reload
#define AIFL_TALKING            0x10000
#define AIFL_NO_HEADLOOK        0x20000
#define AIFL_ATTACK_CROUCH      0x40000
#define AIFL_MISCFLAG1          0x80000     // used various bits of code, temporarily
#define AIFL_MISCFLAG2          0x100000    // used various bits of code, temporarily
#define AIFL_ZOOMING            0x200000
#define AIFL_NO_HEADSHOT_DMG    0x400000
#define AIFL_DIVE_ANIM          0x800000    // able to dive to cover
#define AIFL_NO_TESLA_DAMAGE    0x1000000
#define AIFL_EXPLICIT_ROUTING   0x2000000   // direct routing towards ai_markers, rather than using AAS
#define AIFL_DISMOUNTING        0x4000000
#define AIFL_SPECIAL_FUNC       0x8000000   // prevent external interuption of current think func

//
// predict events
typedef enum
{
	PREDICTSTOP_NONE,
	PREDICTSTOP_HITENT,
	PREDICTSTOP_HITCLIENT
} predictStop_t;
//
typedef enum
{
	AITEAM_NAZI,
	AITEAM_ALLIES,
	AITEAM_MONSTER,
	AITEAM_SPARE1,
	AITEAM_SPARE2,
	AITEAM_SPARE3,
	AITEAM_SPARE4,
	AITEAM_NEUTRAL
} AITeam_t;
//
typedef enum
{
	BBOX_SMALL,
	BBOX_LARGE
} BBoxType_t;
//
// attributes
// !!! NOTE: any changes to this must be reflected in the attributeStrings in ai_cast.c
typedef enum
{
	RUNNING_SPEED,  // max = 300	(running speed)
	WALKING_SPEED,  // max = 300	(walking speed)
	CROUCHING_SPEED, // max = 300	(crouching speed)
	FOV,            // max = 360	(field of view)
	YAW_SPEED,      // max = 300	(yaw speed, so we can make zombie's turn slowly)
	LEADER,         // max = 1.0	(ability to lead an AI squadron)
	AIM_SKILL,      // max = 1.0	(skill while aiming)
	AIM_ACCURACY,   // max = 1.0	(accuracy of firing)
	ATTACK_SKILL,   // max = 1.0	(ability to attack and do other things, like retreat)
	REACTION_TIME,  // max = 1.0	(upon seeing enemy, wait this long before reaction)
	ATTACK_CROUCH,  // max = 1.0	(likely to crouch while firing)
	IDLE_CROUCH,    // max = 1.0	(likely to crouch while idling)
	AGGRESSION,     // max = 1.0	(willingness to fight till the death)
	TACTICAL,       // max = 1.0	(ability to use strategy to their advantage, also behaviour whilst hunting enemy, more likely to creep around)
	CAMPER,         // max = 1.0	(set this to make them stay in the spot they are spawned)
	ALERTNESS,      // max = 1.0	(ability to notice enemies at long range)
	STARTING_HEALTH, // MAX = 999	(starting health)
	HEARING_SCALE,  // max = 999	(multiply default hearing ranges by this)
	HEARING_SCALE_NOT_PVS,  // max = 999 (multiply hearing range by this if outside PVS)
	INNER_DETECTION_RADIUS, // default = 512	(enemies within this range trigger immediate combat mode
	PAIN_THRESHOLD_SCALE,   // default = 1.0

	AICAST_MAX_ATTRIBUTES

} castAttributes_t;
//
typedef enum
{
	SIGHTSOUNDSCRIPT,
	ATTACKSOUNDSCRIPT,
	ORDERSSOUNDSCRIPT,
	DEATHSOUNDSCRIPT,
	QUIETDEATHSOUNDSCRIPT,  //----(SA)	ADDED FOR SILENT DEATHS (SNIPER/KNIFE)
	FLAMEDEATHSOUNDSCRIPT,  //----(SA)	ADDED FOR FLAMING
	PAINSOUNDSCRIPT,

	STAYSOUNDSCRIPT,
	FOLLOWSOUNDSCRIPT,
	ORDERSDENYSOUNDSCRIPT,
	MISC1SOUNDSCRIPT,

	MAX_AI_EVENT_SOUNDS
} AIEventSounds_t;
//
typedef struct {
	char *name;
	float attributes[AICAST_MAX_ATTRIBUTES];

	char    *soundScripts[MAX_AI_EVENT_SOUNDS];

	int aiTeam;
	char *skin;
	int weapons[8];
	int bboxType;
	vec2_t crouchstandZ;
	int aiFlags;

	char    *( *aifuncAttack1 )( struct cast_state_s *cs );     //use this battle aifunc for monster_attack1
	char    *( *aifuncAttack2 )( struct cast_state_s *cs );     //use this battle aifunc for monster_attack2
	char    *( *aifuncAttack3 )( struct cast_state_s *cs );     //use this battle aifunc for monster_attack2

	char *loopingSound;     // play this sound constantly while alive

	aistateEnum_t aiState;
} AICharacterDefaults_t;

//
// script flags
#define SFL_NOCHANGEWEAPON          0x1
#define SFL_NOAIDAMAGE              0x2
#define SFL_FRIENDLYSIGHTCORPSE_TRIGGERED   0x4
#define SFL_WAITING_RESTORE         0x8
#define SFL_FIRST_CALL              0x10
//
// attributes strings (used for per-entity attribute definitions)
// NOTE: these must match the attributes above)
extern char *castAttributeStrings[];
extern AICharacterDefaults_t aiDefaults[NUM_CHARACTERS];
//
// structure defines
//
#define AIVIS_ENEMY                 1
#define AIVIS_INSPECTED             2       // we have inspected them once already
#define AIVIS_INSPECT               4       // we should inspect them when we get a chance
#define AIVIS_PROCESS_SIGHTING      8       // so we know if we have or haven't processed the sighting since they were last seen
#define AIVIS_SIGHT_SCRIPT_CALLED   0x10    // set once sight script has been called.. only call once
//
// share range
#define AIVIS_SHARE_RANGE       384     // if we are within this range of a friendly, share their vis info
//
#define MAX_CHASE_MARKERS       3
#define CHASE_MARKER_INTERVAL   1000
//
#define COMBAT_TIMEOUT          8000
// sight info
typedef struct
{
	int flags;
	int lastcheck_timestamp;
	int real_visible_timestamp;
	int real_update_timestamp;
	int real_notvisible_timestamp;
	int visible_timestamp;          // time we last recorded a sighting
	vec3_t visible_pos;             // position we last knew of them being at (could be hearing, etc)
	vec3_t real_visible_pos;        // position we last physically saw them
	vec3_t visible_vel;             // velocity during last sighting
	int notvisible_timestamp;       // last time we didn't see the entity (used for reaction delay)
	vec3_t chase_marker[MAX_CHASE_MARKERS];
	int chase_marker_count;
	int lastcheck_health;
} cast_visibility_t;
//
// starting weapons, ammo, etc
typedef struct
{
	int startingWeapons[MAX_WEAPONS / ( sizeof( int ) * 8 )];
	int startingAmmo[MAX_WEAPONS];          // starting ammo values for each weapon (set to 999 for unlimited)
} cast_weapon_info_t;
//
// scripting
typedef struct
{
	char    *actionString;
	qboolean ( *actionFunc )( struct cast_state_s *cs, char *params );
} cast_script_stack_action_t;
//
typedef struct
{
	//
	// set during script parsing
	cast_script_stack_action_t      *action;            // points to an action to perform
	char                            *params;
} cast_script_stack_item_t;
//
#define AICAST_MAX_SCRIPT_STACK_ITEMS   64
//
typedef struct
{
	cast_script_stack_item_t items[AICAST_MAX_SCRIPT_STACK_ITEMS];
	int numItems;
} cast_script_stack_t;
//
typedef struct
{
	int eventNum;                           // index in scriptEvents[]
	char                *params;            // trigger targetname, etc
	cast_script_stack_t stack;
} cast_script_event_t;
//
typedef struct
{
	char        *eventStr;
	qboolean ( *eventMatch )( cast_script_event_t *event, char *eventParm );
} cast_script_event_define_t;
//
typedef struct
{
	int castScriptStackHead, castScriptStackChangeTime;
	int castScriptEventIndex;       // current event containing stack of actions to perform
	// scripting system AI variables (set by scripting system, used directly by AI)
	int scriptId;                   // incremented each time the script changes
	int scriptFlags;
	int scriptNoAttackTime;
	int scriptNoMoveTime;
	int playAnimViewlockTime;
	int scriptGotoEnt;              // just goto them, then resume normal behaviour (don't follow)
	int scriptGotoId;
	vec3_t scriptWaitPos;
	int scriptWaitMovetime;
	vec3_t scriptWaitHidePos;
	int scriptWaitHideTime;
	int scriptNoSightTime;
	int scriptAttackEnt;            // we should always attack this AI if they are alive
	vec3_t playanim_viewangles;
} cast_script_status_t;
//
typedef struct
{
	aistateEnum_t currentState;
	aistateEnum_t nextState;
	int nextStateTimer;             // time left until "newState" is reached
} aistate_t;
//
typedef enum
{
	MS_DEFAULT,
	MS_WALK,
	MS_RUN,
	MS_CROUCH
} movestate_t;
//
typedef enum
{
	MSTYPE_NONE,
	MSTYPE_TEMPORARY,
	MSTYPE_PERMANENT
} movestateType_t;
//
//
typedef struct aicast_checkattack_cache_s
{
	int enemy;
	qboolean allowHitWorld;
	int time;
	int weapon;
	qboolean result;
} aicast_checkattack_cache_t;
//
// --------------------------------------------------------------------------------
// the main cast structure
typedef struct cast_state_s
{
	bot_state_t     *bs;
	int entityNum;

	int aasWorldIndex;              // set this according to our bounding box type

	// Cast specific information follows. Add to this as needed, this way the bot_state_t structure
	// remains untouched.

	int aiCharacter;
	int aiFlags;
	int lastThink;                  // time they last thinked, so we can vary the think times
	int actionFlags;                // cast AI specific movement flags
	int lastPain, lastPainDamage;
	int travelflags;
	int thinkFuncChangeTime;

	aistateEnum_t aiState;
	movestate_t movestate;              // walk, run, crouch etc (can be specified in a script)
	movestateType_t movestateType;      // temporary, permanent, etc

	float attributes[AICAST_MAX_ATTRIBUTES];
	// these define the abilities of each cast AI

	// scripting system
	int numCastScriptEvents;
	cast_script_event_t     *castScriptEvents;  // contains a list of actions to perform for each event type
	cast_script_status_t castScriptStatus;      // current status of scripting
	cast_script_status_t castScriptStatusCurrent;       // scripting status to use for backups
	cast_script_status_t castScriptStatusBackup;    // perm backup of status of scripting, only used by backup and restore commands
	int scriptCallIndex;                        // inc'd each time a script is called
	int scriptAnimTime, scriptAnimNum;                          // last time an anim was played using scripting
	// the accumulation buffer
	int scriptAccumBuffer[MAX_SCRIPT_ACCUM_BUFFERS];

	//
	cast_weapon_info_t  *weaponInfo;    // FIXME: make this a list, so they can have multiple weapons?
	cast_visibility_t vislist[MAX_CLIENTS];         // array of all other client entities, allocated at level start-up
	int weaponFireTimes[MAX_WEAPONS];

	char    *( *aifunc )( struct cast_state_s *cs );            //current AI function
	char    *( *oldAifunc )( struct cast_state_s *cs );         // just so we can restore the last aiFunc if required

	char    *( *aifuncAttack1 )( struct cast_state_s *cs );     //use this battle aifunc for monster_attack1
	char    *( *aifuncAttack2 )( struct cast_state_s *cs );     //use this battle aifunc for monster_attack2
	char    *( *aifuncAttack3 )( struct cast_state_s *cs );     //use this battle aifunc for monster_attack2

	void ( *painfunc )( gentity_t *ent, gentity_t *attacker, int damage, vec3_t point );
	void ( *deathfunc )( gentity_t *ent, gentity_t *attacker, int damage, int mod ); //----(SA)	added mod
	void ( *sightfunc )( gentity_t *ent, gentity_t *other, int lastSight );

	//int		(*getDeathAnim)(gentity_t *ent, gentity_t *attacker, int damage);
	void ( *sightEnemy )( gentity_t *ent, gentity_t *other );
	void ( *sightFriend )( gentity_t *ent, gentity_t *other );

	void ( *activate )( int entNum, int activatorNum );

	//
	// !!! NOTE: make sure any entityNum type variables get initialized
	//		to -1 in AICast_CreateCharacter(), or they'll be defaulting to
	//		the player (index 0)
	//

	// goal/AI stuff

	int followEntity;
	float followDist;
	qboolean followIsGoto;      // we are really just going to the entity, but should wait until scripting tells us we can stop
	int followTime;             // if this runs out, the scripting has probably been interupted
	qboolean followSlowApproach;

	int leaderNum;              // entnum of player we are following

	float speedScale;           // so we can vary movement speed

	float combatGoalTime;
	vec3_t combatGoalOrigin;

	int lastGetHidePos;
	int startAttackCount;       // incremented each time we start a standing attack
								// used to make sure we only find a combat spot once per attack
	int combatSpotAttackCount;
	int combatSpotDelayTime;
	int startBattleChaseTime;

	int blockedTime;            // time they were last blocked by a solid entity
	int obstructingTime;        // time that we should move so we are not obstructing someone else
	vec3_t obstructingPos;

	int blockedAvoidTime;
	float blockedAvoidYaw;

	int deathTime;
	int rebirthTime, revivingTime;

	// battle values
	int enemyHeight;
	int enemyDist;

	vec3_t takeCoverPos, takeCoverEnemyPos;
	int takeCoverTime;

	int attackSpotTime;

	int triggerReleaseTime;

	int lastWeaponFired;        // set each time a weapon is fired. used to detect when a weapon has been fired from within scripting
	vec3_t lastWeaponFiredPos;
	int lastWeaponFiredWeaponNum;

	// idle behaviour stuff
	int lastEnemy, nextIdleAngleChange;
	float idleYawChange, idleYaw;

	qboolean crouchHideFlag;

	int doorMarker, doorEntNum;

	// Rafael
	int attackSNDtime;
	int attacksnd;
	int painSoundTime;
	int firstSightTime;
	qboolean secondDeadTime;
	// done

	int startGrenadeFlushTime;
	int lockViewAnglesTime;
	int grenadeFlushEndTime;
	int grenadeFlushFiring;

	int dangerEntity;
	int dangerEntityValidTime;      // dangerEntity is valid until this time expires
	vec3_t dangerEntityPos;         // dangerEntity is predicted to end up here
	int dangerEntityTimestamp;      // time this danger was recorded
	float dangerDist;

	int mountedEntity;              // mg42, etc that we have mounted
	int inspectBodyTime;
	vec3_t startOrigin;

	int damageQuota;
	int damageQuotaTime;

	int dangerLastGetAvoid;
	int lastAvoid;

	int doorMarkerTime, doorMarkerNum, doorMarkerDoor;

	int pauseTime;                  // absolutely don't move move while this is > level.time

	aicast_checkattack_cache_t checkAttackCache;

	int secretsFound;

	int attempts;

	qboolean grenadeGrabFlag;       // if this is set, we need to play the anim before we can grab it

	vec3_t lastMoveToPosGoalOrg;    // if this changes, we should reset the Bot Avoid Reach

	int noAttackTime;               // used by dynamic AI to stop attacking for set time

	int lastRollMove;
	int lastFlipMove;

	vec3_t stimFlyAttackPos;

	int lastDodgeRoll;              // last time we rolled to get out of our enemies direct aim
	int battleRollTime;

	vec3_t viewlock_viewangles;
	int grenadeKickWeapon;

	int animHitCount;               // for stepping through the frames on which to inflict damage

	int totalPlayTime, lastLoadTime;

	int queryStartTime, queryCountValidTime, queryCount, queryAlertSightTime;

	int lastScriptSound;

	int inspectNum;

	int scriptPauseTime;

	int bulletImpactEntity;
	int bulletImpactTime;           // last time we heard/saw a bullet impact
	int bulletImpactIgnoreTime;
	vec3_t bulletImpactStart, bulletImpactEnd;

	int audibleEventTime;
	vec3_t audibleEventOrg;
	int audibleEventEnt;

	int battleChaseMarker, battleChaseMarkerDir;

	int lastBattleHunted;           // last time an enemy decided to hunt us
	int battleHuntPauseTime, battleHuntViewTime;

	int lastAttackCrouch;

	int lastMoveThink;          // last time we ran our ClientThink()

	int numEnemies;             // last count of enemies that are currently pursuing us

	int noReloadTime;           // dont reload prematurely until this time has expired

	int lastValidAreaNum[2];        // last valid area within each AAS world
	int lastValidAreaTime[2];       // time we last got the area

	int weaponNum;              // our current weapon
	int enemyNum;               // our current enemy
	vec3_t ideal_viewangles, viewangles;
	usercmd_t lastucmd;
	int attackcrouch_time;
	int bFlags;

	int deadSinkStartTime;

	int lastActivate;

	vec3_t loperLeapVel;
	// -------------------------------------------------------------------------------------------
	// if working on a post release patch, new variables should ONLY be inserted after this point
	// -------------------------------------------------------------------------------------------

} cast_state_t;
//
#define CSFOFS( x ) ( (int)&( ( (cast_state_t *)0 )->x ) )
//
typedef struct aicast_predictmove_s
{
	vec3_t endpos;              //position at the end of movement prediction
	vec3_t velocity;            //velocity at the end of movement prediction
	int presencetype;           //presence type at end of movement prediction
	int stopevent;              //event that made the prediction stop
	float time;                 //time predicted ahead
	int frames;                 //number of frames predicted ahead
	int numtouch;
	int touchents[MAXTOUCH];
	int groundEntityNum;
} aicast_predictmove_t;
//
// variables/globals
//
//cast states
extern cast_state_t *caststates;
//number of characters
extern int numcast;
//
// minimum time between thinks (maximum is double this)
extern int aicast_thinktime;
// maximum number of character thinks at once
extern int aicast_maxthink;
// maximum clients
extern int aicast_maxclients;
// skill scale
extern float aicast_skillscale;
//
// cvar to enable aicast debugging, set higher for more levels of debugging
extern vmCvar_t aicast_debug;
extern vmCvar_t aicast_debugname;
extern vmCvar_t aicast_scripts;
//
//
// procedure defines
//
// ai_cast.c
void    AIChar_SetBBox( gentity_t *ent, cast_state_t *cs, qboolean useHeadTag );
void    AICast_Printf( int type, const char *fmt, ... );
gentity_t *AICast_CreateCharacter( gentity_t *ent, float *attributes, cast_weapon_info_t *weaponInfo, char *castname, char *model, char *head, char *sex, char *color, char *handicap );
void    AICast_Init( void );
void    AICast_DelayedSpawnCast( gentity_t *ent, int castType );
qboolean AICast_SolidsInBBox( vec3_t pos, vec3_t mins, vec3_t maxs, int entnum, int mask );
void    AICast_CheckLevelAttributes( cast_state_t *cs, gentity_t *ent, char **ppStr );
//
// ai_cast_sight.c
void    AICast_SightUpdate( int numchecks );
qboolean AICast_VisibleFromPos( vec3_t srcpos, int srcnum,
								vec3_t destpos, int destnum, qboolean updateVisPos );
void    AICast_UpdateVisibility( gentity_t *srcent, gentity_t *destent, qboolean shareVis, qboolean directview );
qboolean AICast_CheckVisibility( gentity_t *srcent, gentity_t *destent );
//
// ai_cast_debug.c
void    AICast_DBG_InitAIFuncs( void );
void    AICast_DBG_AddAIFunc( cast_state_t *cs, char *funcname );
void    AICast_DBG_ListAIFuncs( cast_state_t *cs, int numprint );
void    AICast_DBG_RouteTable_f( vec3_t org, char *param );
int     Sys_MilliSeconds( void );
void    AICast_DebugFrame( cast_state_t *cs );
//
// ai_cast_funcs.c
void AICast_SpecialFunc( cast_state_t *cs );
bot_moveresult_t *AICast_MoveToPos( cast_state_t *cs, vec3_t pos, int entnum );
float   AICast_SpeedScaleForDistance( cast_state_t *cs, float startdist, float idealDist );
char    *AIFunc_DefaultStart( cast_state_t *cs );
char    *AIFunc_IdleStart( cast_state_t *cs );
char    *AIFunc_ChaseGoalIdleStart( cast_state_t *cs, int entitynum, float reachdist );
char    *AIFunc_ChaseGoalStart( cast_state_t *cs, int entitynum, float reachdist, qboolean slowApproach );
char    *AIFunc_BattleChaseStart( cast_state_t *cs );
char    *AIFunc_BattleStart( cast_state_t *cs );
char    *AIFunc_DoorMarkerStart( cast_state_t *cs, int doornum, int markernum );
char    *AIFunc_DoorMarker( cast_state_t *cs );
char    *AIFunc_BattleTakeCoverStart( cast_state_t *cs );
char    *AIFunc_GrenadeFlushStart( cast_state_t *cs );
char    *AIFunc_AvoidDangerStart( cast_state_t *cs );
char    *AIFunc_BattleMG42Start( cast_state_t *cs );
char    *AIFunc_InspectBodyStart( cast_state_t *cs );
char    *AIFunc_GrenadeKickStart( cast_state_t *cs );
char    *AIFunc_InspectFriendlyStart( cast_state_t *cs, int entnum );
char    *AIFunc_InspectBulletImpactStart( cast_state_t *cs );
char    *AIFunc_InspectAudibleEventStart( cast_state_t *cs, int entnum );
char    *AIFunc_BattleAmbushStart( cast_state_t *cs );
char    *AIFunc_BattleHuntStart( cast_state_t *cs );
//
// ai_cast_func_attack.c
char    *AIFunc_ZombieFlameAttackStart( cast_state_t *cs );
char    *AIFunc_ZombieAttack2Start( cast_state_t *cs );
char    *AIFunc_ZombieMeleeStart( cast_state_t *cs );
char    *AIFunc_LoperAttack1Start( cast_state_t *cs );
char    *AIFunc_LoperAttack2Start( cast_state_t *cs );
char    *AIFunc_LoperAttack3Start( cast_state_t *cs );
char    *AIFunc_StimSoldierAttack1Start( cast_state_t *cs );
char    *AIFunc_StimSoldierAttack2Start( cast_state_t *cs );
char    *AIFunc_BlackGuardAttack1Start( cast_state_t *cs );
char    *AIFunc_RejectAttack1Start( cast_state_t *cs ); //----(SA)
char    *AIFunc_WarriorZombieMeleeStart( cast_state_t *cs );
char    *AIFunc_WarriorZombieSightStart( cast_state_t *cs );
char    *AIFunc_WarriorZombieDefenseStart( cast_state_t *cs );
//
// ai_cast_func_boss1.c
char    *AIFunc_Helga_SpiritAttack_Start( cast_state_t *cs );
char    *AIFunc_Helga_MeleeStart( cast_state_t *cs );
char    *AIFunc_FlameZombie_PortalStart( cast_state_t *cs );
char    *AIFunc_Heinrich_MeleeStart( cast_state_t *cs );
char    *AIFunc_Heinrich_RaiseDeadStart( cast_state_t *cs );
char    *AIFunc_Heinrich_SpawnSpiritsStart( cast_state_t *cs );
void    AICast_Heinrich_SoundPrecache( void );
//
// ai_cast_fight.c
qboolean AICast_StateChange( cast_state_t *cs, aistateEnum_t newaistate );
void    AICast_WeaponSway( cast_state_t *cs, vec3_t ofs );
int     AICast_ScanForEnemies( cast_state_t *cs, int *enemies );
void    AICast_UpdateBattleInventory( cast_state_t *cs, int enemy );
float   AICast_Aggression( cast_state_t *cs );
int     AICast_WantsToChase( cast_state_t *cs );
int     AICast_WantsToTakeCover( cast_state_t *cs, qboolean attacking );
qboolean AICast_EntityVisible( cast_state_t *cs, int enemynum, qboolean directview );
bot_moveresult_t AICast_CombatMove( cast_state_t *cs, int tfl );
qboolean AICast_AimAtEnemy( cast_state_t *cs );
qboolean AICast_CheckAttackAtPos( int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld );
qboolean AICast_CheckAttack( cast_state_t *cs, int enemy, qboolean allowHitWorld );
void    AICast_ProcessAttack( cast_state_t *cs );
void    AICast_ChooseWeapon( cast_state_t *cs, qboolean battleFunc );
qboolean AICast_GetTakeCoverPos( cast_state_t *cs, int enemyNum, vec3_t enemyPos, vec3_t returnPos );
qboolean AICast_CanMoveWhileFiringWeapon( int weaponnum );
float   AICast_GetWeaponSoundRange( int weapon );
qboolean AICast_StopAndAttack( cast_state_t *cs );
qboolean AICast_WantToRetreat( cast_state_t *cs );
int     AICast_SafeMissileFire( gentity_t *ent, int duration, int enemyNum, vec3_t enemyPos, int selfNum, vec3_t endPos );
void    AIChar_AttackSound( cast_state_t *cs );
qboolean AICast_GotEnoughAmmoForWeapon( cast_state_t *cs, int weapon );
qboolean AICast_HostileEnemy( cast_state_t *cs, int enemynum );
qboolean AICast_QueryEnemy( cast_state_t *cs, int enemynum );
void AICast_AudibleEvent( int srcnum, vec3_t pos, float range );
qboolean AICast_WeaponUsable( cast_state_t *cs, int weaponNum );
float AICast_WeaponRange( cast_state_t *cs, int weaponnum );

//
// ai_cast_events.c
void    AICast_Pain( gentity_t *targ, gentity_t *attacker, int damage, vec3_t point );
void    AICast_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
void    AICast_Sight( gentity_t *ent, gentity_t *other, int lastSight );
void    AICast_EndChase( cast_state_t *cs );
void    AICast_ProcessActivate( int entNum, int activatorNum );
//
// ai_cast_think.c
void AICast_Think( int client, float thinktime );
void AICast_UpdateInput( cast_state_t *cs, int time );
void AICast_InputToUserCommand( cast_state_t * cs, bot_input_t * bi, usercmd_t * ucmd, int delta_angles[3] );
void AICast_PredictMovement( cast_state_t *cs, int numframes, float frametime, aicast_predictmove_t *move, usercmd_t *ucmd, int checkHitEnt );
void AICast_Blocked( cast_state_t *cs, bot_moveresult_t *moveresult, int activate, bot_goal_t *goal );
qboolean AICast_RequestCrouchAttack( cast_state_t *cs, vec3_t org, float time );
qboolean AICast_GetAvoid( cast_state_t *cs, bot_goal_t *goal, vec3_t outpos, qboolean reverse, int blockEnt );
void AICast_QueryThink( cast_state_t *cs );
void AICast_DeadClipWalls( cast_state_t *cs );
void AICast_IdleReload( cast_state_t *cs );
//
// ai_cast_script.c
qboolean AICast_ScriptRun( cast_state_t *cs, qboolean force );
//
// ai_cast_soldier.c
void    AIChar_spawn( gentity_t *ent );
//
// other/external defines
void    BotCheckAir( bot_state_t *bs );
void    BotUpdateInput( bot_state_t *bs, int time );
float   AngleDifference( float ang1, float ang2 );
float   BotChangeViewAngle( float angle, float ideal_angle, float speed );
void BotInputToUserCommand( bot_input_t * bi, usercmd_t * ucmd, int delta_angles[3] );
void    GibEntity( gentity_t *self, int killer );
void    GibHead( gentity_t *self, int killer );
//
extern bot_state_t  *botstates[MAX_CLIENTS];
