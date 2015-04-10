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
// Name:			botlib.h
// Function:		bot AI Library
// Programmer:		Mr Elusive (MrElusive@idsoftware.com)
// Last update:		1999-08-18
// Tab Size:		3
//===========================================================================

#define BOTLIB_API_VERSION      2

struct aas_clientmove_s;
struct aas_entityinfo_s;
struct bot_consolemessage_s;
struct bot_match_s;
struct bot_goal_s;
struct bot_moveresult_s;
struct bot_initmove_s;
struct weaponinfo_s;


//debug line colors
#define LINECOLOR_NONE          -1
#define LINECOLOR_RED           1 //0xf2f2f0f0L
#define LINECOLOR_GREEN         2 //0xd0d1d2d3L
#define LINECOLOR_BLUE          3 //0xf3f3f1f1L
#define LINECOLOR_YELLOW        4 //0xdcdddedfL
#define LINECOLOR_ORANGE        5 //0xe0e1e2e3L

//Print types
#define PRT_MESSAGE             1
#define PRT_WARNING             2
#define PRT_ERROR               3
#define PRT_FATAL               4
#define PRT_EXIT                5

//console message types
#define CMS_NORMAL              0
#define CMS_CHAT                1

//botlib error codes
#define BLERR_NOERROR                   0   //no error
#define BLERR_LIBRARYNOTSETUP           1   //library not setup
#define BLERR_LIBRARYALREADYSETUP       2   //BotSetupLibrary: library already setup
#define BLERR_INVALIDCLIENTNUMBER       3   //invalid client number
#define BLERR_INVALIDENTITYNUMBER       4   //invalid entity number
#define BLERR_NOAASFILE                 5   //BotLoadMap: no AAS file available
#define BLERR_CANNOTOPENAASFILE         6   //BotLoadMap: cannot open AAS file
#define BLERR_CANNOTSEEKTOAASFILE       7   //BotLoadMap: cannot seek to AAS file
#define BLERR_CANNOTREADAASHEADER       8   //BotLoadMap: cannot read AAS header
#define BLERR_WRONGAASFILEID            9   //BotLoadMap: incorrect AAS file id
#define BLERR_WRONGAASFILEVERSION       10  //BotLoadMap: incorrect AAS file version
#define BLERR_CANNOTREADAASLUMP         11  //BotLoadMap: cannot read AAS file lump
#define BLERR_NOBSPFILE                 12  //BotLoadMap: no BSP file available
#define BLERR_CANNOTOPENBSPFILE         13  //BotLoadMap: cannot open BSP file
#define BLERR_CANNOTSEEKTOBSPFILE       14  //BotLoadMap: cannot seek to BSP file
#define BLERR_CANNOTREADBSPHEADER       15  //BotLoadMap: cannot read BSP header
#define BLERR_WRONGBSPFILEID            16  //BotLoadMap: incorrect BSP file id
#define BLERR_WRONGBSPFILEVERSION       17  //BotLoadMap: incorrect BSP file version
#define BLERR_CANNOTREADBSPLUMP         18  //BotLoadMap: cannot read BSP file lump
#define BLERR_AICLIENTNOTSETUP          19  //BotAI: client not setup
#define BLERR_AICLIENTALREADYSETUP      20  //BotSetupClient: client already setup
#define BLERR_AIMOVEINACTIVECLIENT      21  //BotMoveClient: cannot move inactive client
#define BLERR_AIMOVETOACTIVECLIENT      22  //BotMoveClient: cannot move to active client
#define BLERR_AICLIENTALREADYSHUTDOWN   23  //BotShutdownClient: client not setup
#define BLERR_AIUPDATEINACTIVECLIENT    24  //BotUpdateClient: called for inactive client
#define BLERR_AICMFORINACTIVECLIENT     25  //BotConsoleMessage: called for inactive client
#define BLERR_SETTINGSINACTIVECLIENT    26  //BotClientSettings: called for inactive client
#define BLERR_CANNOTLOADICHAT           27  //BotSetupClient: cannot load initial chats
#define BLERR_CANNOTLOADITEMWEIGHTS     28  //BotSetupClient: cannot load item weights
#define BLERR_CANNOTLOADITEMCONFIG      29  //BotSetupLibrary: cannot load item config
#define BLERR_CANNOTLOADWEAPONWEIGHTS   30  //BotSetupClient: cannot load weapon weights
#define BLERR_CANNOTLOADWEAPONCONFIG    31  //BotSetupLibrary: cannot load weapon config
#define BLERR_INVALIDSOUNDINDEX         32  //BotAddSound: invalid sound index value

//action flags
#define ACTION_ATTACK           1
#define ACTION_USE              2
#define ACTION_RESPAWN          4
#define ACTION_JUMP             8
#define ACTION_MOVEUP           8
#define ACTION_CROUCH           16
#define ACTION_MOVEDOWN         16
#define ACTION_MOVEFORWARD      32
#define ACTION_MOVEBACK         64
#define ACTION_MOVELEFT         128
#define ACTION_MOVERIGHT        256
#define ACTION_DELAYEDJUMP      512
#define ACTION_TALK             1024
#define ACTION_GESTURE          2048
#define ACTION_WALK             4096

//the bot input, will be converted to an usercmd_t
typedef struct bot_input_s
{
	float thinktime;        //time since last output (in seconds)
	vec3_t dir;             //movement direction
	float speed;            //speed in the range [0, 400]
	vec3_t viewangles;      //the view angles
	int actionflags;        //one of the ACTION_? flags
	int weapon;             //weapon to use
} bot_input_t;

#ifndef BSPTRACE

//bsp_trace_t hit surface
typedef struct bsp_surface_s
{
	char name[16];
	int flags;
	int value;
} bsp_surface_t;

//remove the bsp_trace_s structure definition l8r on
//a trace is returned when a box is swept through the world
typedef struct bsp_trace_s
{
	qboolean allsolid;          // if true, plane is not valid
	qboolean startsolid;        // if true, the initial point was in a solid area
	float fraction;             // time completed, 1.0 = didn't hit anything
	vec3_t endpos;              // final position
	cplane_t plane;             // surface normal at impact
	float exp_dist;             // expanded plane distance
	int sidenum;                // number of the brush side hit
	bsp_surface_t surface;      // the hit point surface
	int contents;               // contents on other side of surface hit
	int ent;                    // number of entity hit
} bsp_trace_t;

#define BSPTRACE
#endif  // BSPTRACE

//entity state
typedef struct bot_entitystate_s
{
	int type;               // entity type
	int flags;              // entity flags
	vec3_t origin;          // origin of the entity
	vec3_t angles;          // angles of the model
	vec3_t old_origin;      // for lerping
	vec3_t mins;            // bounding box minimums
	vec3_t maxs;            // bounding box maximums
	int groundent;          // ground entity
	int solid;              // solid type
	int modelindex;         // model used
	int modelindex2;        // weapons, CTF flags, etc
	int frame;              // model frame number
	int event;              // impulse events -- muzzle flashes, footsteps, etc
	int eventParm;          // even parameter
	int powerups;           // bit flags
	int weapon;             // determines weapon and flash model, etc
	int legsAnim;           // mask off ANIM_TOGGLEBIT
	int torsoAnim;          // mask off ANIM_TOGGLEBIT
//	int		weapAnim;		// mask off ANIM_TOGGLEBIT	//----(SA)	added
//----(SA)	didn't want to comment in as I wasn't sure of any implications of changing this structure.
} bot_entitystate_t;

//bot AI library exported functions
typedef struct botlib_import_s
{
	//print messages from the bot library
	void ( QDECL * Print )( int type, char *fmt, ... );
	//trace a bbox through the world
	void ( *Trace )( bsp_trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int passent, int contentmask );
	//trace a bbox against a specific entity
	void ( *EntityTrace )( bsp_trace_t *trace, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int entnum, int contentmask );
	//retrieve the contents at the given point
	int ( *PointContents )( vec3_t point );
	//check if the point is in potential visible sight
	int ( *inPVS )( vec3_t p1, vec3_t p2 );
	//retrieve the BSP entity data lump
	char        *( *BSPEntityData )( void );
	//
	void ( *BSPModelMinsMaxsOrigin )( int modelnum, vec3_t angles, vec3_t mins, vec3_t maxs, vec3_t origin );
	//send a bot client command
	void ( *BotClientCommand )( int client, char *command );
	//memory allocation
	void        *( *GetMemory )( int size );
	void ( *FreeMemory )( void *ptr );
	void ( *FreeZoneMemory )( void );
	void        *( *HunkAlloc )( int size );
	//file system access
	int ( *FS_FOpenFile )( const char *qpath, fileHandle_t *file, fsMode_t mode );
	int ( *FS_Read )( void *buffer, int len, fileHandle_t f );
	int ( *FS_Write )( const void *buffer, int len, fileHandle_t f );
	void ( *FS_FCloseFile )( fileHandle_t f );
	int ( *FS_Seek )( fileHandle_t f, long offset, int origin );
	//debug visualisation stuff
	int ( *DebugLineCreate )( void );
	void ( *DebugLineDelete )( int line );
	void ( *DebugLineShow )( int line, vec3_t start, vec3_t end, int color );
	//
	int ( *DebugPolygonCreate )( int color, int numPoints, vec3_t *points );
	void ( *DebugPolygonDelete )( int id );
	//
	// Ridah, Cast AI stuff
	qboolean ( *AICast_VisibleFromPos )(   vec3_t srcpos, int srcnum,
										   vec3_t destpos, int destnum, qboolean updateVisPos );
	qboolean ( *AICast_CheckAttackAtPos )( int entnum, int enemy, vec3_t pos, qboolean ducking, qboolean allowHitWorld );
	// done.
} botlib_import_t;

typedef struct aas_export_s
{
	//-----------------------------------
	// be_aas_entity.h
	//-----------------------------------
	void ( *AAS_EntityInfo )( int entnum, struct aas_entityinfo_s *info );
	//-----------------------------------
	// be_aas_main.h
	//-----------------------------------
	int ( *AAS_Initialized )( void );
	void ( *AAS_PresenceTypeBoundingBox )( int presencetype, vec3_t mins, vec3_t maxs );
	float ( *AAS_Time )( void );
	//--------------------------------------------
	// be_aas_sample.c
	//--------------------------------------------
	int ( *AAS_PointAreaNum )( vec3_t point );
	int ( *AAS_TraceAreas )( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas );
	//--------------------------------------------
	// be_aas_bspq3.c
	//--------------------------------------------
	int ( *AAS_PointContents )( vec3_t point );
	int ( *AAS_NextBSPEntity )( int ent );
	int ( *AAS_ValueForBSPEpairKey )( int ent, char *key, char *value, int size );
	int ( *AAS_VectorForBSPEpairKey )( int ent, char *key, vec3_t v );
	int ( *AAS_FloatForBSPEpairKey )( int ent, char *key, float *value );
	int ( *AAS_IntForBSPEpairKey )( int ent, char *key, int *value );
	//--------------------------------------------
	// be_aas_reach.c
	//--------------------------------------------
	int ( *AAS_AreaReachability )( int areanum );
	//--------------------------------------------
	// be_aas_route.c
	//--------------------------------------------
	int ( *AAS_AreaTravelTimeToGoalArea )( int areanum, vec3_t origin, int goalareanum, int travelflags );
	//--------------------------------------------
	// be_aas_move.c
	//--------------------------------------------
	int ( *AAS_Swimming )( vec3_t origin );
	int ( *AAS_PredictClientMovement )( struct aas_clientmove_s *move,
										int entnum, vec3_t origin,
										int presencetype, int onground,
										vec3_t velocity, vec3_t cmdmove,
										int cmdframes,
										int maxframes, float frametime,
										int stopevent, int stopareanum, int visualize );

	// Ridah, route-tables
	//--------------------------------------------
	// be_aas_routetable.c
	//--------------------------------------------
	void ( *AAS_RT_ShowRoute )( vec3_t srcpos, int srcnum, int destnum );
	qboolean ( *AAS_RT_GetHidePos )( vec3_t srcpos, int srcnum, int srcarea, vec3_t destpos, int destnum, int destarea, vec3_t returnPos );
	int ( *AAS_FindAttackSpotWithinRange )( int srcnum, int rangenum, int enemynum, float rangedist, int travelflags, float *outpos );
	void ( *AAS_SetAASBlockingEntity )( vec3_t absmin, vec3_t absmax, qboolean blocking );
	// done.

	// Ridah
	void ( *AAS_SetCurrentWorld )( int index );
	// done.

} aas_export_t;

typedef struct ea_export_s
{
	//ClientCommand elementary actions
	void ( *EA_Say )( int client, char *str );
	void ( *EA_SayTeam )( int client, char *str );
	void ( *EA_UseItem )( int client, char *it );
	void ( *EA_DropItem )( int client, char *it );
	void ( *EA_UseInv )( int client, char *inv );
	void ( *EA_DropInv )( int client, char *inv );
	void ( *EA_Gesture )( int client );
	void ( *EA_Command )( int client, char *command );
	//regular elementary actions
	void ( *EA_SelectWeapon )( int client, int weapon );
	void ( *EA_Talk )( int client );
	void ( *EA_Attack )( int client );
	void ( *EA_Use )( int client );
	void ( *EA_Respawn )( int client );
	void ( *EA_Jump )( int client );
	void ( *EA_DelayedJump )( int client );
	void ( *EA_Crouch )( int client );
	void ( *EA_MoveUp )( int client );
	void ( *EA_MoveDown )( int client );
	void ( *EA_MoveForward )( int client );
	void ( *EA_MoveBack )( int client );
	void ( *EA_MoveLeft )( int client );
	void ( *EA_MoveRight )( int client );
	void ( *EA_Move )( int client, vec3_t dir, float speed );
	void ( *EA_View )( int client, vec3_t viewangles );
	//send regular input to the server
	void ( *EA_EndRegular )( int client, float thinktime );
	void ( *EA_GetInput )( int client, float thinktime, bot_input_t *input );
	void ( *EA_ResetInput )( int client, bot_input_t *init );
} ea_export_t;

typedef struct ai_export_s
{
	//-----------------------------------
	// be_ai_char.h
	//-----------------------------------
	int ( *BotLoadCharacter )( char *charfile, int skill );
	void ( *BotFreeCharacter )( int character );
	float ( *Characteristic_Float )( int character, int index );
	float ( *Characteristic_BFloat )( int character, int index, float min, float max );
	int ( *Characteristic_Integer )( int character, int index );
	int ( *Characteristic_BInteger )( int character, int index, int min, int max );
	void ( *Characteristic_String )( int character, int index, char *buf, int size );
	//-----------------------------------
	// be_ai_chat.h
	//-----------------------------------
	int ( *BotAllocChatState )( void );
	void ( *BotFreeChatState )( int handle );
	void ( *BotQueueConsoleMessage )( int chatstate, int type, char *message );
	void ( *BotRemoveConsoleMessage )( int chatstate, int handle );
	int ( *BotNextConsoleMessage )( int chatstate, struct bot_consolemessage_s *cm );
	int ( *BotNumConsoleMessages )( int chatstate );
	void ( *BotInitialChat )( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
	int ( *BotNumInitialChats )( int chatstate, char *type );
	int ( *BotReplyChat )( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
	int ( *BotChatLength )( int chatstate );
	void ( *BotEnterChat )( int chatstate, int client, int sendto );
	void ( *BotGetChatMessage )( int chatstate, char *buf, int size );
	int ( *StringContains )( char *str1, char *str2, int casesensitive );
	int ( *BotFindMatch )( char *str, struct bot_match_s *match, unsigned long int context );
	void ( *BotMatchVariable )( struct bot_match_s *match, int variable, char *buf, int size );
	void ( *UnifyWhiteSpaces )( char *string );
	void ( *BotReplaceSynonyms )( char *string, unsigned long int context );
	int ( *BotLoadChatFile )( int chatstate, char *chatfile, char *chatname );
	void ( *BotSetChatGender )( int chatstate, int gender );
	void ( *BotSetChatName )( int chatstate, char *name );
	//-----------------------------------
	// be_ai_goal.h
	//-----------------------------------
	void ( *BotResetGoalState )( int goalstate );
	void ( *BotResetAvoidGoals )( int goalstate );
	void ( *BotRemoveFromAvoidGoals )( int goalstate, int number );
	void ( *BotPushGoal )( int goalstate, struct bot_goal_s *goal );
	void ( *BotPopGoal )( int goalstate );
	void ( *BotEmptyGoalStack )( int goalstate );
	void ( *BotDumpAvoidGoals )( int goalstate );
	void ( *BotDumpGoalStack )( int goalstate );
	void ( *BotGoalName )( int number, char *name, int size );
	int ( *BotGetTopGoal )( int goalstate, struct bot_goal_s *goal );
	int ( *BotGetSecondGoal )( int goalstate, struct bot_goal_s *goal );
	int ( *BotChooseLTGItem )( int goalstate, vec3_t origin, int *inventory, int travelflags );
	int ( *BotChooseNBGItem )( int goalstate, vec3_t origin, int *inventory, int travelflags,
							   struct bot_goal_s *ltg, float maxtime );
	int ( *BotTouchingGoal )( vec3_t origin, struct bot_goal_s *goal );
	int ( *BotItemGoalInVisButNotVisible )( int viewer, vec3_t eye, vec3_t viewangles, struct bot_goal_s *goal );
	int ( *BotGetLevelItemGoal )( int index, char *classname, struct bot_goal_s *goal );
	int ( *BotGetNextCampSpotGoal )( int num, struct bot_goal_s *goal );
	int ( *BotGetMapLocationGoal )( char *name, struct bot_goal_s *goal );
	float ( *BotAvoidGoalTime )( int goalstate, int number );
	void ( *BotInitLevelItems )( void );
	void ( *BotUpdateEntityItems )( void );
	int ( *BotLoadItemWeights )( int goalstate, char *filename );
	void ( *BotFreeItemWeights )( int goalstate );
	void ( *BotInterbreedGoalFuzzyLogic )( int parent1, int parent2, int child );
	void ( *BotSaveGoalFuzzyLogic )( int goalstate, char *filename );
	void ( *BotMutateGoalFuzzyLogic )( int goalstate, float range );
	int ( *BotAllocGoalState )( int client );
	void ( *BotFreeGoalState )( int handle );
	//-----------------------------------
	// be_ai_move.h
	//-----------------------------------
	void ( *BotResetMoveState )( int movestate );
	void ( *BotMoveToGoal )( struct bot_moveresult_s *result, int movestate, struct bot_goal_s *goal, int travelflags );
	int ( *BotMoveInDirection )( int movestate, vec3_t dir, float speed, int type );
	void ( *BotResetAvoidReach )( int movestate );
	void ( *BotResetLastAvoidReach )( int movestate );
	int ( *BotReachabilityArea )( vec3_t origin, int testground );
	int ( *BotMovementViewTarget )( int movestate, struct bot_goal_s *goal, int travelflags, float lookahead, vec3_t target );
	int ( *BotPredictVisiblePosition )( vec3_t origin, int areanum, struct bot_goal_s *goal, int travelflags, vec3_t target );
	int ( *BotAllocMoveState )( void );
	void ( *BotFreeMoveState )( int handle );
	void ( *BotInitMoveState )( int handle, struct bot_initmove_s *initmove );
	// Ridah
	void ( *BotInitAvoidReach )( int handle );
	// done.
	//-----------------------------------
	// be_ai_weap.h
	//-----------------------------------
	int ( *BotChooseBestFightWeapon )( int weaponstate, int *inventory );
	void ( *BotGetWeaponInfo )( int weaponstate, int weapon, struct weaponinfo_s *weaponinfo );
	int ( *BotLoadWeaponWeights )( int weaponstate, char *filename );
	int ( *BotAllocWeaponState )( void );
	void ( *BotFreeWeaponState )( int weaponstate );
	void ( *BotResetWeaponState )( int weaponstate );
	//-----------------------------------
	// be_ai_gen.h
	//-----------------------------------
	int ( *GeneticParentsAndChildSelection )( int numranks, float *ranks, int *parent1, int *parent2, int *child );
} ai_export_t;

//bot AI library imported functions
typedef struct botlib_export_s
{
	//Area Awareness System functions
	aas_export_t aas;
	//Elementary Action functions
	ea_export_t ea;
	//AI functions
	ai_export_t ai;
	//setup the bot library, returns BLERR_
	int ( *BotLibSetup )( void );
	//shutdown the bot library, returns BLERR_
	int ( *BotLibShutdown )( void );
	//sets a library variable returns BLERR_
	int ( *BotLibVarSet )( char *var_name, char *value );
	//gets a library variable returns BLERR_
	int ( *BotLibVarGet )( char *var_name, char *value, int size );
	//sets a C-like define returns BLERR_
	int ( *BotLibDefine )( char *string );
	//start a frame in the bot library
	int ( *BotLibStartFrame )( float time );
	//load a new map in the bot library
	int ( *BotLibLoadMap )( const char *mapname );
	//entity updates
	int ( *BotLibUpdateEntity )( int ent, bot_entitystate_t *state );
	//just for testing
	int ( *Test )( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 );
} botlib_export_t;

//linking of bot library
botlib_export_t *GetBotLibAPI( int apiVersion, botlib_import_t *import );

/* Library variables:

name:						default:			module(s):			description:

"basedir"					""					l_utils.c			Quake2 base directory
"gamedir"					""					l_utils.c			Quake2 game directory
"cddir"						""					l_utils.c			Quake2 CD directory

"autolaunchbspc"			"0"					be_aas_load.c		automatically launch (Win)BSPC
"log"						"0"					l_log.c				enable/disable creating a log file
"maxclients"				"4"					be_interface.c		maximum number of clients
"maxentities"				"1024"				be_interface.c		maximum number of entities

"sv_friction"				"6"					be_aas_move.c		ground friction
"sv_stopspeed"				"100"				be_aas_move.c		stop speed
"sv_gravity"				"800"				be_aas_move.c		gravity value
"sv_waterfriction"			"1"					be_aas_move.c		water friction
"sv_watergravity"			"400"				be_aas_move.c		gravity in water
"sv_maxvelocity"			"300"				be_aas_move.c		maximum velocity
"sv_maxwalkvelocity"		"300"				be_aas_move.c		maximum walk velocity
"sv_maxcrouchvelocity"		"100"				be_aas_move.c		maximum crouch velocity
"sv_maxswimvelocity"		"150"				be_aas_move.c		maximum swim velocity
"sv_walkaccelerate"			"10"				be_aas_move.c		walk acceleration
"sv_airaccelerate"			"1"					be_aas_move.c		air acceleration
"sv_swimaccelerate"			"4"					be_aas_move.c		swim acceleration
"sv_maxstep"				"18"				be_aas_move.c		maximum step height
"sv_maxbarrier"				"32"				be_aas_move.c		maximum barrier height
"sv_maxsteepness"			"0.7"				be_aas_move.c		maximum floor steepness
"sv_jumpvel"				"270"				be_aas_move.c		jump z velocity
"sv_maxwaterjump"			"20"				be_aas_move.c		maximum waterjump height

"max_aaslinks"				"4096"				be_aas_sample.c		maximum links in the AAS
"max_bsplinks"				"4096"				be_aas_bsp.c		maximum links in the BSP

"notspawnflags"				"2048"				be_ai_goal.c		entities with these spawnflags will be removed
"itemconfig"				"items.c"			be_ai_goal.c		item configuration file
"weaponconfig"				"weapons.c"			be_ai_weap.c		weapon configuration file
"synfile"					"syn.c"				be_ai_chat.c		file with synonyms
"rndfile"					"rnd.c"				be_ai_chat.c		file with random strings
"matchfile"					"match.c"			be_ai_chat.c		file with match strings
"max_messages"				"1024"				be_ai_chat.c		console message heap size
"max_weaponinfo"			"32"				be_ai_weap.c		maximum number of weapon info
"max_projectileinfo"		"32"				be_ai_weap.c		maximum number of projectile info
"max_iteminfo"				"256"				be_ai_goal.c		maximum number of item info
"max_levelitems"			"256"				be_ai_goal.c		maximum number of level items
"framereachability"			""					be_aas_reach.c		number of reachabilities to calucate per frame
"forceclustering"			"0"					be_aas_main.c		force recalculation of clusters
"forcereachability"			"0"					be_aas_main.c		force recalculation of reachabilities
"forcewrite"				"0"					be_aas_main.c		force writing of aas file
"nooptimize"				"0"					be_aas_main.c		no aas optimization

"laserhook"					"0"					be_ai_move.c		0 = CTF hook, 1 = laser hook

*/

