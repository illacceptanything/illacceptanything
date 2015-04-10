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
 * name:	cg_local.h
 *
 * desc:	The entire cgame module is unloaded and reloaded on each level change,
 *			so there is NO persistant data between levels on the client side.
 *			If you absolutely need something stored, it can either be kept
 *			by the server in the server stored userinfos, or stashed in a cvar.

 *
*/

#include "../game/q_shared.h"
#include "tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"


#define POWERUP_BLINKS      5

#define POWERUP_BLINK_TIME  1000
#define FADE_TIME           200
#define PULSE_TIME          200
#define DAMAGE_DEFLECT_TIME 100
#define DAMAGE_RETURN_TIME  400
#define DAMAGE_TIME         500
#define LAND_DEFLECT_TIME   150
#define LAND_RETURN_TIME    300
#define STEP_TIME           200
#define DUCK_TIME           100
#define PAIN_TWITCH_TIME    200
#define WEAPON_SELECT_TIME  1400
#define HOLDABLE_SELECT_TIME 1400   //----(SA)	for drawing holdable icons
#define ITEM_SCALEUP_TIME   1000
#define ZOOM_TIME           150
#define ITEM_BLOB_TIME      200
#define MUZZLE_FLASH_TIME   30      //----(SA)
#define SINK_TIME           1000        // time for fragments to sink into ground before going away
#define ATTACKER_HEAD_TIME  10000
#define REWARD_TIME         3000

#define PULSE_SCALE         1.5         // amount to scale up the icons when activating

#define MAX_STEP_CHANGE     32

#define MAX_VERTS_ON_POLY   10
#define MAX_MARK_POLYS      1024

#define STAT_MINUS          10  // num frame for '-' stats digit

#define ICON_SIZE           48
#define CHAR_WIDTH          32
#define CHAR_HEIGHT         48
#define TEXT_ICON_SPACE     4

#define TEAMCHAT_WIDTH      80
#define TEAMCHAT_HEIGHT     8

// very large characters
#define GIANT_WIDTH         32
#define GIANT_HEIGHT        48

#define NUM_CROSSHAIRS      10

// Ridah, trails
#define STYPE_STRETCH   0
#define STYPE_REPEAT    1

#define TJFL_FADEIN     ( 1 << 0 )
#define TJFL_CROSSOVER  ( 1 << 1 )
#define TJFL_NOCULL     ( 1 << 2 )
#define TJFL_FIXDISTORT ( 1 << 3 )
#define TJFL_SPARKHEADFLARE ( 1 << 4 )
#define TJFL_NOPOLYMERGE    ( 1 << 5 )
// done.

// NERVE - SMF - limbo mode 3d view position
#define LIMBO_3D_X  10
#define LIMBO_3D_Y  120
#define LIMBO_3D_W  420
#define LIMBO_3D_H  330
// -NERVE - SMF

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int oldFrame;
	int oldFrameTime;               // time when ->oldFrame was exactly on

	int frame;
	int frameTime;                  // time when ->frame will be exactly on

	float backlerp;

	float yawAngle;
	qboolean yawing;
	float pitchAngle;
	qboolean pitching;

	int animationNumber;            // may include ANIM_TOGGLEBIT
	int oldAnimationNumber;         // may include ANIM_TOGGLEBIT
	animation_t *animation;
	int animationTime;              // time when the first frame of the animation will be exact

	// Ridah, variable speed anims
	vec3_t oldFramePos;
	float animSpeedScale;
	int oldFrameSnapshotTime;
	headAnimation_t *headAnim;
	// done.

	animation_t *cgAnim;    // pointer to the root of the animation array to use (*animation above points at the current sequence)	//----(SA)	added
} lerpFrame_t;

// Ridah, effect defines
#define MAX_ZOMBIE_SPIRITS          4
#define MAX_ZOMBIE_DEATH_TRAILS     16

#define MAX_LOPER_LIGHTNING_POINTS  24

#define MAX_TESLA_BOLTS             4

#define MAX_EFFECT_ENTS             20

typedef struct {
	lerpFrame_t legs, torso;

	// Ridah, talking animations
	lerpFrame_t head;
	// done.

	lerpFrame_t weap;       //----(SA)	autonomous weapon animations

	int lastTime;                   // last time we were processed/ If the time goes backwards, reset.

	int painTime;
	int painDuration;
	int painDirection;              // flip from 0 to 1
	int painAnimTorso;
	int painAnimLegs;
	int lightningFiring;

	// railgun trail spawning
	vec3_t railgunImpact;
	qboolean railgunFlash;

	// machinegun spinning
	float barrelAngle;
	int barrelTime;
	qboolean barrelSpinning;

	//----(SA) machinegun bolt sliding
	float boltPosition;
	int boltTime;
	int boltSliding;
	//----(SA) end

	//----(SA) 'spinner' spinning (body part)
	float spinnerAngle;
	int spinnerTime;
	qboolean spinnerSpinning;
	//----(SA)	end

	// Ridah, so we can do fast tag grabbing
	refEntity_t legsRefEnt, torsoRefEnt, headRefEnt, gunRefEnt;
	int gunRefEntFrame;

	float animSpeed;            // for manual adjustment

	// Zombie spirit effect
	// !!FIXME: these effects will be restarted by a *_restart command, can we save this data somehow?
	qboolean cueZombieSpirit;               // if this is qfalse, and the zombie effect flag is set, then we need to start a new attack
	int zombieSpiritStartTime;              // time the effect was started, so we can fade things in
	int zombieSpiritTrailHead[MAX_ZOMBIE_SPIRITS];
	int zombieSpiritRotationTimes[MAX_ZOMBIE_SPIRITS];
	int zombieSpiritRadiusCycleTimes[MAX_ZOMBIE_SPIRITS];
	int lastZombieSpirit;
	int nextZombieSpiritSound;
	int zombieSpiritEndTime;                // time the effect was disabled
	vec3_t zombieSpiritPos[MAX_ZOMBIE_SPIRITS];
	vec3_t zombieSpiritDir[MAX_ZOMBIE_SPIRITS];
	float zombieSpiritSpeed[MAX_ZOMBIE_SPIRITS];
	int zombieSpiritStartTimes[MAX_ZOMBIE_SPIRITS];

	// Zombie death effect
	// !!FIXME: these effects will be restarted by a *_restart command, can we save this data somehow?
	qboolean cueZombieDeath;            // if this is qfalse, and the zombie effect flag is set, then we need to start a new attack
	int zombieDeathStartTime;               // time the effect was started, so we can fade things in
	int zombieDeathEndTime;             // time the effect was disabled
	int lastZombieDeath;
	int zombieDeathFadeStart;
	int zombieDeathFadeEnd;
	int zombieDeathTrailHead[MAX_ZOMBIE_DEATH_TRAILS];
	int zombieDeathRotationTimes[MAX_ZOMBIE_DEATH_TRAILS];
	int zombieDeathRadiusCycleTimes[MAX_ZOMBIE_DEATH_TRAILS];

	// loper effects
	int loperLastGroundChargeTime;
	byte loperGroundChargeToggle;
	int loperGroundValidTime;

	vec3_t headLookIdeal;
	vec3_t headLookOffset;
	float headLookSpeed;
	int headLookStopTime;
	float headLookSpeedMax;

	// tesla coil effects
	vec3_t teslaEndPoints[MAX_TESLA_BOLTS];
	int teslaEndPointTimes[MAX_TESLA_BOLTS];            // time the bolt stays valid
	vec3_t teslaOffsetDirs[MAX_TESLA_BOLTS];            // bending direction from center or direct beam
	float teslaOffsets[MAX_TESLA_BOLTS];                // amount to offset from center
	int teslaOffsetTimes[MAX_TESLA_BOLTS];              // time the offset stays valid
	int teslaEnemy[MAX_TESLA_BOLTS];
	int teslaDamageApplyTime;

	int teslaDamagedTime;                   // time we were last hit by a tesla bolt

	// misc effects
	int effectEnts[MAX_EFFECT_ENTS];
	int numEffectEnts;
	int effect1EndTime;
	vec3_t lightningPoints[MAX_LOPER_LIGHTNING_POINTS];
	int lightningTimes[MAX_LOPER_LIGHTNING_POINTS];
	int lightningSoundTime;

	qboolean forceLOD;

} playerEntity_t;

//----(SA)
typedef struct {
	char type[MAX_QPATH];           // md3_lower, md3_lbelt, md3_rbelt, etc.
	char model[MAX_QPATH];          // lower.md3, belt1.md3, etc.
} skinModel_t;
//----(SA) end


//=================================================



// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t currentState;     // from cg.frame
	entityState_t nextState;        // from cg.nextFrame, if available
	qboolean interpolate;           // true if next is valid to interpolate to
	qboolean currentValid;          // true if cg.frame holds this entity

	int muzzleFlashTime;                // move to playerEntity?
	int overheatTime;
	int previousEvent;
	int previousEventSequence;              // Ridah
	int teleportFlag;

	int trailTime;                  // so missile trails can handle dropped initial packets
	int miscTime;

	playerEntity_t pe;

	int errorTime;                  // decay the error from this time
	vec3_t errorOrigin;
	vec3_t errorAngles;

	qboolean extrapolated;          // false if origin / angles is an interpolation
	vec3_t rawOrigin;
	vec3_t rawAngles;

	vec3_t beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t lerpOrigin;
	vec3_t lerpAngles;

	vec3_t lastLerpAngles;          // (SA) for remembering the last position when a state changes

	// Ridah, trail effects
	int headJuncIndex, headJuncIndex2;
	int lastTrailTime;
	// done.

	// Ridah
	float loopSoundVolume;
	vec3_t fireRiseDir;             // if standing still this will be up, otherwise it'll point away from movement dir
	int lastWeaponClientFrame;
	int lastFuseSparkTime;
	vec3_t lastFuseSparkOrg;

	// client side dlights
	int dl_frame;
	int dl_oldframe;
	float dl_backlerp;
	int dl_time;
	char dl_stylestring[64];
	int dl_sound;
	int dl_atten;

	lerpFrame_t lerpFrame;      //----(SA)	added
	vec3_t highlightOrigin;             // center of the geometry.  for things like corona placement on treasure
	qboolean usehighlightOrigin;

	refEntity_t refEnt;
	int processedFrame;                 // frame we were last added to the scene

	// client-side lightning
	int boltTimes[MAX_TESLA_BOLTS];
	vec3_t boltLocs[MAX_TESLA_BOLTS];
	vec3_t boltCrawlDirs[MAX_TESLA_BOLTS];

	// item highlighting

	int highlightTime;
	qboolean highlighted;

	animation_t centAnim[2];

	// (SA) added to help akimbo effects attach to the correct model
	qboolean akimboFire;
} centity_t;


//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s   *prevMark, *nextMark;
	int time;
	qhandle_t markShader;
	qboolean alphaFade;         // fade alpha instead of rgb
	float color[4];
	poly_t poly;
	polyVert_t verts[MAX_VERTS_ON_POLY];

	int duration;           // Ridah
} markPoly_t;

//----(SA)	moved in from cg_view.c
typedef enum {
	ZOOM_NONE,
	ZOOM_BINOC,
	ZOOM_SNIPER,
	ZOOM_SNOOPER,
	ZOOM_FG42SCOPE,
	ZOOM_MG42,
	ZOOM_MAX_ZOOMS
} EZoom_t;

typedef enum {
	ZOOM_OUT,   // widest angle
	ZOOM_IN // tightest angle (approaching 0)
} EZoomInOut_t;

extern float zoomTable[ZOOM_MAX_ZOOMS][2];

//----(SA)	end

typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SPARK,
	LE_DEBRIS,
	LE_BLOOD,
	LE_FUSE_SPARK,
	LE_ZOMBIE_SPIRIT,
	LE_HELGA_SPIRIT,
	LE_ZOMBIE_BAT,
	LE_MOVING_TRACER,
	LE_EMITTER,
	LE_SPIRIT_VIEWFLASH
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE  = 0x0001           // do not scale size over time
	,LEF_TUMBLE          = 0x0002           // tumble over time, used for ejecting shells
	,LEF_NOFADEALPHA     = 0x0004           // Ridah, sparks
	,LEF_SMOKING         = 0x0008           // (SA) smoking
	,LEF_NOTOUCHPARENT   = 0x0010           // (SA) when tracing to eval trajectory, ignore parent cent
	,LEF_PLAYER_DAMAGE   = 0x0020           // hurt the player on impact
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BLOOD
} leMarkType_t;         // fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_ROCK,
	LEBS_WOOD,
	LEBS_BRASS,
	LEBS_BONE
} leBounceSoundType_t;  // fragment local entities can make sounds on impacts

#define MAX_OLD_POS     3

typedef struct localEntity_s {
	struct localEntity_s    *prev, *next;
	leType_t leType;
	int leFlags;

	int startTime;
	int endTime;
	int fadeInTime;

	float lifeRate;                     // 1.0 / (endTime - startTime)

	trajectory_t pos;
	trajectory_t angles;

	float bounceFactor;                 // 0.0 = no bounce, 1.0 = perfect

	float color[4];

	float radius;

	float light;
	vec3_t lightColor;

	leMarkType_t leMarkType;            // mark to leave on fragment impact
	leBounceSoundType_t leBounceSoundType;

	refEntity_t refEntity;

	// Ridah
	int lightOverdraw;
	int lastTrailTime;
	int headJuncIndex, headJuncIndex2;
	float effectWidth;
	int effectFlags;
	struct localEntity_s    *chain;     // used for grouping entities (like for flamethrower junctions)
	int onFireStart, onFireEnd;
	int ownerNum;
	int lastSpiritDmgTime;

	int loopingSound;

	int breakCount;                     // break-up this many times before we can break no more
	float sizeScale;

	char validOldPos[MAX_OLD_POS];
	vec3_t oldPos[MAX_OLD_POS];
	int oldPosHead;
	// done.

} localEntity_t;

//======================================================================


typedef struct {
	int client;
	int score;
	int ping;
	int time;
	int scoreFlags;
	int powerUps;
	int accuracy;
	int impressiveCount;
	int excellentCount;
	int guantletCount;
	int defendCount;
	int assistCount;
	int captures;
	qboolean perfect;
	int team;
} score_t;


typedef enum {
	ACC_BELT_LEFT,  // belt left (lower)
	ACC_BELT_RIGHT, // belt right (lower)
	ACC_BELT,       // belt (upper)
	ACC_BACK,       // back (upper)
	ACC_WEAPON,     // weapon (upper)
	ACC_WEAPON2,    // weapon2 (upper)
	ACC_HAT,        // hat (head)
	ACC_MOUTH2,     //
	ACC_MOUTH3,     //
	//
	ACC_MAX     // this is bound by network limits, must change network stream to increase this
				// (SA) No, really?  that's not true is it?  isn't this client-side only?
} accType_t;

#define ACC_NUM_MOUTH 3 // matches the above count (hat/mouth2/mouth3)




// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define MAX_CUSTOM_SOUNDS   32
#define MAX_GIB_MODELS      16
typedef struct {
	qboolean infoValid;

	int clientNum;

	char name[MAX_QPATH];
	team_t team;

	int botSkill;                   // 0 = not bot, 1-5 = bot

	vec3_t color;

	int score;                      // updated by score servercmds
	int location;                   // location index for team mode
	int health;                     // you only get this info about your teammates
	int armor;
	int curWeapon;

	int handicap;
	int wins, losses;               // in tourney mode

	int powerups;                   // so can display quad/flag status

	int breathPuffTime;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char modelName[MAX_QPATH];
	char skinName[MAX_QPATH];
	char hSkinName[MAX_QPATH];
	qboolean deferred;

	qhandle_t legsModel;
	qhandle_t legsSkin;

	qhandle_t torsoModel;
	qhandle_t torsoSkin;

	qboolean isSkeletal;

	//----(SA) added accessory models/skins for belts/backpacks/etc.
	qhandle_t accModels[ACC_MAX];       // see def of ACC_MAX for index descriptions
	qhandle_t accSkins[ACC_MAX];        // FIXME: put the #define for number of accessory models somewhere. (SA)

	//----(SA)	additional parts for specialized characters (the loper's spinning trunk for example)
	qhandle_t partModels[9];        // [0-7] are optionally called in scripts, [8] is reserved for internal use
	qhandle_t partSkins[9];
	//----(SA)	end

	qhandle_t headModel;
	qhandle_t headSkin;

	qhandle_t modelIcon;

	// RF, may be shared by multiple clients/characters
	animModelInfo_t *modelInfo;

	sfxHandle_t sounds[MAX_CUSTOM_SOUNDS];

	qhandle_t gibModels[MAX_GIB_MODELS];

	vec3_t playermodelScale;            //----(SA)	set in the skin.  client-side only

	int blinkTime;              //----(SA)
} clientInfo_t;



typedef enum {
	W_PART_1,
	W_PART_2,
	W_PART_3,
	W_PART_4,
	W_PART_5,
	W_PART_6,
	W_PART_7,
	W_MAX_PARTS
} barrelType_t;

typedef enum {
	W_TP_MODEL,         //	third person model
	W_FP_MODEL,         //	first person model
	W_PU_MODEL,         //	pickup model
	W_FP_MODEL_SWAP,    //	swap out model
	W_SKTP_MODEL,       //	SKELETAL version third person model
	W_NUM_TYPES
} modelViewType_t;

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean registered;
	gitem_t         *item;

//----(SA)	weapon animation sequences loaded from the weapon.cfg
	animation_t weapAnimations[MAX_WP_ANIMATIONS];
//----(SA)	end

	qhandle_t handsModel;               // the hands don't actually draw, they just position the weapon

	qhandle_t standModel;               // not drawn.  tags used for positioning weapons for pickup

//----(SA) mod for 1st/3rd person weap views
	qhandle_t weaponModel[W_NUM_TYPES];
	qhandle_t wpPartModels[W_NUM_TYPES][W_MAX_PARTS];
	qhandle_t flashModel[W_NUM_TYPES];
	qhandle_t modModel[W_NUM_TYPES];        // like the scope for the rifles
//----(SA) end

	pose_t position;                    // wolf locations (high, low, knife, pistol, shoulder, throw)  defines are WPOS_HIGH, WPOS_LOW, WPOS_KNIFE, WPOS_PISTOL, WPOS_SHOULDER, WPOS_THROW

	vec3_t weaponMidpoint;              // so it will rotate centered instead of by tag

	float flashDlight;
	vec3_t flashDlightColor;
	sfxHandle_t flashSound[4];          // fast firing weapons randomly choose
	sfxHandle_t flashEchoSound[4];      //----(SA)	added - distant gun firing sound
	sfxHandle_t lastShotSound[4];       // sound of the last shot can be different (mauser doesn't have bolt action on last shot for example)

	sfxHandle_t switchSound[4];     //----(SA)	added

	qhandle_t weaponIcon[2];            //----(SA)	[0] is weap icon, [1] is highlight icon
	qhandle_t ammoIcon;

	qhandle_t ammoModel;

	qhandle_t missileModel;
	sfxHandle_t missileSound;
	void ( *missileTrailFunc )( centity_t *, const struct weaponInfo_s *wi );
	float missileDlight;
	vec3_t missileDlightColor;
	int missileRenderfx;

	void ( *ejectBrassFunc )( centity_t * );

	float trailRadius;
	float wiTrailTime;

	sfxHandle_t readySound;             // an amibient sound the weapon makes when it's /not/ firing
	sfxHandle_t firingSound;
	sfxHandle_t overheatSound;
	sfxHandle_t reloadSound;

	sfxHandle_t spinupSound;        //----(SA)	added // sound started when fire button goes down, and stepped on when the first fire event happens
	sfxHandle_t spindownSound;      //----(SA)	added // sound called if the above is running but player doesn't follow through and fire
} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean registered;
	qhandle_t models[MAX_ITEM_MODELS];
	qhandle_t icons[MAX_ITEM_ICONS];
} itemInfo_t;


typedef struct {
	int itemNum;
} powerupInfo_t;

#define MAX_VIEWDAMAGE  8
typedef struct {
	int damageTime, damageDuration;
	float damageX, damageY, damageValue;
} viewDamage_t;

#define MAX_CAMERA_SHAKE    4
typedef struct {
	int time;
	float scale;
	float length;
	float radius;
	vec3_t src;
} cameraShake_t;

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS    16

typedef struct {
	int clientFrame;                // incremented each frame

	int clientNum;

	qboolean demoPlayback;
	qboolean levelShot;             // taking a level menu screenshot
	int deferredPlayerLoading;
	qboolean loading;               // don't defer players at initial startup
	qboolean intermissionStarted;       // don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int latestSnapshotNum;          // the number of snapshots the client system has received
	int latestSnapshotTime;         // the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t  *snap;              // cg.snap->serverTime <= cg.time
	snapshot_t  *nextSnap;          // cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t activeSnapshots[2];

	float frameInterpolation;       // (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean thisFrameTeleport;
	qboolean nextFrameTeleport;

	int frametime;              // cg.time - cg.oldTime

	int time;                   // this is the time value that the client
								// is rendering at.
	int oldTime;                // time at last frame, used for missile trails and prediction checking

	int physicsTime;            // either cg.snap->time or cg.nextSnap->time

	int timelimitWarnings;          // 5 min, 1 min, overtime
	int fraglimitWarnings;

	qboolean mapRestart;            // set on a map restart to set back the weapon

	qboolean renderingThirdPerson;          // during deaths, chasecams, etc

	// prediction state
	qboolean hyperspace;                // true if prediction has hit a trigger_teleport
	playerState_t predictedPlayerState;
	centity_t predictedPlayerEntity;
	qboolean validPPS;                  // clear until the first call to CG_PredictPlayerState
	int predictedErrorTime;
	vec3_t predictedError;

	int eventSequence;
	int predictableEvents[MAX_PREDICTED_EVENTS];

	float stepChange;                   // for stair up smoothing
	int stepTime;

	float duckChange;                   // for duck viewheight smoothing
	int duckTime;

	float landChange;                   // for landing hard
	int landTime;

	// input state sent to server
	int weaponSelect;
	int holdableSelect;                 // (SA) which holdable item is currently held ("selected").  When the client is ready to use it, send "use item <holdableSelect>"

	// auto rotating items
	vec3_t autoAnglesSlow;
	vec3_t autoAxisSlow[3];
	vec3_t autoAngles;
	vec3_t autoAxis[3];
	vec3_t autoAnglesFast;
	vec3_t autoAxisFast[3];

	// view rendering
	refdef_t refdef;
	vec3_t refdefViewAngles;            // will be converted to refdef.viewaxis

	// zoom key
	qboolean zoomed;
	qboolean zoomedBinoc;
	int zoomedScope;            //----(SA)	changed to int
	int zoomTime;
	float zoomSensitivity;
	float zoomval;


	// information screen text during loading
	unsigned char infoScreenText[MAX_STRING_CHARS];

	// scoreboard
	int scoresRequestTime;
	int numScores;
	int selectedScore;
	int teamScores[2];
	score_t scores[MAX_CLIENTS];
	qboolean showScores;
	qboolean scoreBoardShowing;
	int scoreFadeTime;
	char killerName[MAX_NAME_LENGTH];
	char spectatorList[MAX_STRING_CHARS];                   // list of names
	int spectatorLen;                                                           // length of list
	float spectatorWidth;                                                   // width in device units
	int spectatorTime;                                                      // next time to offset
	int spectatorPaintX;                                                    // current paint x
	int spectatorPaintX2;                                                   // current paint x
	int spectatorOffset;                                                    // current offset from start
	int spectatorPaintLen;                                              // current offset from start

	qboolean showItems;
	int itemFadeTime;

	qboolean lightstylesInited;

	// centerprinting
	int centerPrintTime;
	int centerPrintCharWidth;
	int centerPrintY;
	unsigned char centerPrint[1024];
	int centerPrintLines;

	// fade in/out
	int fadeTime;
	float fadeRate;
	vec4_t fadeColor1;
	vec4_t fadeColor2;

//----(SA)	added
	// game stats
	int exitStatsTime;
	int exitStatsFade;
	// just a copy of what's on the server, updated by configstring.  better way to communicate/store this I'm sure
	int playTimeH;
	int playTimeM;
	int playTimeS;
	int attempts;
	int numObjectives;
	int numObjectivesFound;
	int numSecrets;
	int numSecretsFound;
	int numTreasure;
	int numTreasureFound;
	int numArtifacts;
	int numArtifactsFound;
//----(SA)	end


	// low ammo warning state
	int lowAmmoWarning;             // 1 = low, 2 = empty

	// kill timers for carnage reward
	int lastKillTime;

	// crosshair client ID
	int crosshairClientNum;
	int crosshairClientTime;

	int crosshairPowerupNum;
	int crosshairPowerupTime;

//----(SA)	added
	// cursorhints
	int cursorHintIcon;
	int cursorHintTime;
	int cursorHintFade;
	int cursorHintValue;

//----(SA)	end

	// powerup active flashing
	int powerupActive;
	int powerupTime;

	// attacking player
	int attackerTime;
	int voiceTime;

	// reward medals
	int rewardTime;
	int rewardCount;
	qhandle_t rewardShader;

	// warmup countdown
	int warmup;
	int warmupCount;

	// message icon popup time	//----(SA)	added
	int yougotmailTime;

	//==========================

	int itemPickup;
	int itemPickupTime;
	int itemPickupBlendTime;            // the pulse around the crosshair is timed seperately

	int holdableSelectTime;             //----(SA)	for holdable item icon drawing

	int weaponSelectTime;
	int weaponAnimation;
	int weaponAnimationTime;

	// blend blobs
	viewDamage_t viewDamage[MAX_VIEWDAMAGE];
	float damageTime;           // last time any kind of damage was recieved
	int damageIndex;            // slot that was filled in
	float damageX, damageY, damageValue;
	float viewFade;

	int grenLastTime;

	int switchbackWeapon;
	int lastFiredWeapon;
	int lastWeapSelInBank[MAX_WEAP_BANKS];          // remember which weapon was last selected in a bank for 'weaponbank' commands //----(SA)	added
// JPW FIXME NOTE: max_weap_banks > max_weap_banks_mp so this should be OK, but if that changes, change this too

	// status bar head
	float headYaw;
	float headEndPitch;
	float headEndYaw;
	int headEndTime;
	float headStartPitch;
	float headStartYaw;
	int headStartTime;

	// view movement
	float v_dmg_time;
	float v_dmg_pitch;
	float v_dmg_roll;

	vec3_t kick_angles;         // weapon kicks
	vec3_t kick_origin;

	// RF, view flames when getting burnt
	int v_fireTime, v_noFireTime;
	vec3_t v_fireRiseDir;

	// temp working variables for player view
	float bobfracsin;
	int bobcycle;
	float xyspeed;
	int nextOrbitTime;

	// development tool
	refEntity_t testModelEntity;
	char testModelName[MAX_QPATH];
	qboolean testGun;

	// RF, new kick angles
	vec3_t kickAVel;            // for damage feedback, weapon recoil, etc
								// This is the angular velocity, to give a smooth
								// rotational feedback, rather than sudden jerks
	vec3_t kickAngles;          // for damage feedback, weapon recoil, etc
								// NOTE: this is not transmitted through MSG.C stream
								// since weapon kicks are client-side, and damage feedback
								// is rare enough that we can transmit that as an event
	float recoilPitch, recoilPitchAngle;

	// Duffy
	qboolean cameraMode;        // if rendering from a camera
	// Duffy end

	unsigned int cld;           // NERVE - SMF
	qboolean limboMenu;         // NERVE - SMF

	// NERVE - SMF - Objective info display
	int oidTeam;
	int oidPrintTime;
	int oidPrintCharWidth;
	int oidPrintY;
	char oidPrint[1024];
	int oidPrintLines;
	// -NERVE - SMF

	cameraShake_t cameraShake[MAX_CAMERA_SHAKE];
	float cameraShakePhase;
	vec3_t cameraShakeAngles;

	float rumbleScale;          //RUMBLE FX using new shakeCamera code

} cg_t;

#define NUM_FUNNEL_SPRITES  21
#define MAX_LOCKER_DEBRIS   5

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t charsetShader;
	// JOSEPH 4-17-00
	qhandle_t menucharsetShader;
	// END JOSEPH
	qhandle_t charsetProp;
	qhandle_t charsetPropGlow;
	qhandle_t charsetPropB;
	qhandle_t whiteShader;

	qhandle_t redFlagModel;
	qhandle_t blueFlagModel;

	qhandle_t armorModel;

	qhandle_t teamStatusBar;

	qhandle_t deferShader;

	// gib explosions
	qhandle_t gibAbdomen;
	qhandle_t gibArm;
	qhandle_t gibChest;
	qhandle_t gibFist;
	qhandle_t gibFoot;
	qhandle_t gibForearm;
	qhandle_t gibIntestine;
	qhandle_t gibLeg;
	qhandle_t gibSkull;
	qhandle_t gibBrain;

	// debris
	qhandle_t debBlock[6];
	qhandle_t debRock[3];
	qhandle_t debFabric[3];
	qhandle_t debWood[6];

	qhandle_t targetEffectExplosionShader;

	qhandle_t machinegunBrassModel;
	qhandle_t panzerfaustBrassModel;    //----(SA)	added

	// Rafael
	qhandle_t smallgunBrassModel;

	qhandle_t shotgunBrassModel;

	qhandle_t railRingsShader;
	qhandle_t railCoreShader;

	qhandle_t lightningShader;

	qhandle_t friendShader;

//	qhandle_t	medicReviveShader;	//----(SA)	commented out from MP
	qhandle_t balloonShader;
	qhandle_t connectionShader;

	qhandle_t aiStateShaders[MAX_AISTATES];

	qhandle_t selectShader;
	qhandle_t viewBloodShader;
	qhandle_t tracerShader;
	qhandle_t crosshairShader[NUM_CROSSHAIRS];
	qhandle_t crosshairFriendly;    //----(SA)	added
	qhandle_t lagometerShader;
	qhandle_t backTileShader;
	qhandle_t noammoShader;

	qhandle_t reticleShader;
//	qhandle_t	reticleShaderSimple;
	qhandle_t reticleShaderSimpleQ;
//	qhandle_t	snooperShader;
	qhandle_t snooperShaderSimple;
//	qhandle_t	binocShaderSimple;
	qhandle_t binocShaderSimpleQ;   // same as above, but quartered.  (trying to save texture space)

	qhandle_t smokePuffShader;
	qhandle_t smokePuffRageProShader;
	qhandle_t shotgunSmokePuffShader;
	qhandle_t waterBubbleShader;
	qhandle_t bloodTrailShader;

	qhandle_t nailPuffShader;

//----(SA)	modified

	// cursor hints
	// would be nice to specify these in the menu scripts instead of permanent handles...
	qhandle_t hintShaders[HINT_NUM_HINTS];

	qhandle_t youGotMailShader;         // '!' - new entry in notebook
	qhandle_t youGotObjectiveShader;    // '<checkmark> - you completed objective
//----(SA)	end

	// Rafael
	qhandle_t snowShader;
	qhandle_t oilParticle;
	qhandle_t oilSlick;
	// done.

	// Rafael - cannon
	qhandle_t smokePuffShaderdirty;
	qhandle_t smokePuffShaderb1;
	qhandle_t smokePuffShaderb2;
	qhandle_t smokePuffShaderb3;
	qhandle_t smokePuffShaderb4;
	qhandle_t smokePuffShaderb5;
	// done

	// Rafael - blood pool
	qhandle_t bloodPool;

	// Ridah, viewscreen blood animation
	qhandle_t viewBloodAni[5];
	qhandle_t viewFlashBlood;
	qhandle_t viewFlashFire[16];
	// done

	// Rafael bats
	qhandle_t bats[10];
	// done

	// Rafael shards
	qhandle_t shardGlass1;
	qhandle_t shardGlass2;
	qhandle_t shardWood1;
	qhandle_t shardWood2;
	qhandle_t shardMetal1;
	qhandle_t shardMetal2;
	qhandle_t shardCeramic1;
	qhandle_t shardCeramic2;
	// done

	qhandle_t shardRubble1;
	qhandle_t shardRubble2;
	qhandle_t shardRubble3;


	qhandle_t shardJunk[MAX_LOCKER_DEBRIS];

	qhandle_t numberShaders[11];

	qhandle_t shadowMarkShader;
	qhandle_t shadowFootShader;
	qhandle_t shadowTorsoShader;

	qhandle_t botSkillShaders[5];

	// wall mark shaders
	qhandle_t wakeMarkShader;
	qhandle_t wakeMarkShaderAnim;
	qhandle_t bloodMarkShaders[5];
	qhandle_t bloodDotShaders[5];
	qhandle_t bulletMarkShader;
	qhandle_t bulletMarkShaderMetal;
	qhandle_t bulletMarkShaderWood;
	qhandle_t bulletMarkShaderCeramic;
	qhandle_t bulletMarkShaderGlass;
	qhandle_t burnMarkShader;
	qhandle_t holeMarkShader;
	qhandle_t energyMarkShader;

	// powerup shaders
	qhandle_t quadShader;
	qhandle_t redQuadShader;
	qhandle_t quadWeaponShader;
	qhandle_t invisShader;
	qhandle_t regenShader;
	qhandle_t battleSuitShader;
	qhandle_t battleWeaponShader;
	qhandle_t hastePuffShader;

	// weapon effect models
	qhandle_t spearModel;   //----(SA)

	qhandle_t bulletFlashModel;
	qhandle_t ringFlashModel;
	qhandle_t dishFlashModel;
	qhandle_t lightningExplosionModel;

	qhandle_t zombieLoogie;
	qhandle_t flamebarrel;
	qhandle_t mg42muzzleflash;
	//qhandle_t	mg42muzzleflashgg;
	qhandle_t planemuzzleflash;

	// Rafael
	qhandle_t crowbar;

	qhandle_t waterSplashModel;
	qhandle_t waterSplashShader;

	qhandle_t thirdPersonBinocModel;    //----(SA)	added
	qhandle_t cigModel;     //----(SA)	added

	qhandle_t batModel;
	qhandle_t spiritSkullModel;
	qhandle_t helgaGhostModel;

	// weapon effect shaders
	qhandle_t railExplosionShader;
	qhandle_t bulletExplosionShader;
	qhandle_t rocketExplosionShader;
	qhandle_t grenadeExplosionShader;
	qhandle_t bfgExplosionShader;
	qhandle_t bloodExplosionShader;

	qhandle_t flameThrowerhitShader;

	// special effects models
	qhandle_t teleportEffectModel;
	qhandle_t teleportEffectShader;

	// scoreboard headers
	qhandle_t scoreboardName;
	qhandle_t scoreboardPing;
	qhandle_t scoreboardScore;
	qhandle_t scoreboardTime;
	// Ridah
	qhandle_t bloodCloudShader;
	qhandle_t sparkParticleShader;
	qhandle_t smokeTrailShader;
	qhandle_t fireTrailShader;
	qhandle_t lightningBoltShader;
	qhandle_t lightningBoltShaderGreen;
	qhandle_t flamethrowerFireStream;
	qhandle_t flamethrowerBlueStream;
	qhandle_t flamethrowerFuelStream;
	qhandle_t flamethrowerFuelShader;
	qhandle_t onFireShader, onFireShader2;
	//qhandle_t	dripWetShader, dripWetShader2;
	qhandle_t viewFadeBlack;
	qhandle_t sparkFlareShader;
	qhandle_t funnelFireShader[NUM_FUNNEL_SPRITES];

	qhandle_t spotLightShader;
	qhandle_t spotLightBeamShader;
	qhandle_t spotLightBaseModel;       //----(SA)	added
	qhandle_t spotLightLightModel;      //----(SA)	added
	qhandle_t spotLightLightModelBroke;     //----(SA)	added

	qhandle_t lightningHitWallShader;
	qhandle_t lightningWaveShader;
	qhandle_t bulletParticleTrailShader;
	qhandle_t smokeParticleShader;

	// DHM - Nerve :: bullet hitting dirt
	qhandle_t dirtParticle1Shader;
	qhandle_t dirtParticle2Shader;
	qhandle_t dirtParticle3Shader;

	qhandle_t zombieSpiritWallShader;
	qhandle_t zombieSpiritTrailShader;
	qhandle_t zombieSpiritSkullShader;
	qhandle_t zombieDeathDustShader;
	qhandle_t zombieBodyFadeShader;
	qhandle_t zombieHeadFadeShader;

	qhandle_t helgaSpiritSkullShader;
	qhandle_t helgaSpiritTrailShader;

	qhandle_t ssSpiritSkullModel;

	qhandle_t skeletonSkinShader;
	qhandle_t skeletonLegsModel;
	qhandle_t skeletonTorsoModel;
	qhandle_t skeletonHeadModel;
	qhandle_t skeletonLegsSkin;
	qhandle_t skeletonTorsoSkin;
	qhandle_t skeletonHeadSkin;

	qhandle_t loperGroundChargeShader;

	qhandle_t teslaDamageEffectShader;
	qhandle_t teslaAltDamageEffectShader;
	qhandle_t viewTeslaDamageEffectShader;
	qhandle_t viewTeslaAltDamageEffectShader;
	// done.

//----(SA)
	// proto/super/heini armor parts
	qhandle_t protoArmor[9 * 3];        // 9 parts, 3 sections each	(nodam, dam1, dam2)
	qhandle_t superArmor[16 * 3];       // 14 parts, 3 sections each
	qhandle_t heinrichArmor[22 * 3];    // 20 parts, 3 sections each
//----(SA)	end

	// medals shown during gameplay
	qhandle_t medalImpressive;
	qhandle_t medalExcellent;
	qhandle_t medalGauntlet;

	// sounds
	sfxHandle_t n_health;
	sfxHandle_t noFireUnderwater;
	sfxHandle_t snipersound;
	sfxHandle_t quadSound;
	sfxHandle_t tracerSound;
	sfxHandle_t selectSound;
	sfxHandle_t useNothingSound;
	sfxHandle_t wearOffSound;
	sfxHandle_t footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t sfx_lghit1;
	sfxHandle_t sfx_lghit2;
	sfxHandle_t sfx_lghit3;
	sfxHandle_t sfx_ric1;
	sfxHandle_t sfx_ric2;
	sfxHandle_t sfx_ric3;
	sfxHandle_t sfx_railg;
	sfxHandle_t sfx_rockexp;
	sfxHandle_t sfx_dynamiteexp;
	sfxHandle_t sfx_dynamiteexpDist;    //----(SA)	added
	sfxHandle_t sfx_spearhit;
	sfxHandle_t sfx_knifehit[5];
	sfxHandle_t sfx_bullet_metalhit[3];
	sfxHandle_t sfx_bullet_woodhit[3];
	sfxHandle_t sfx_bullet_roofhit[3];
	sfxHandle_t sfx_bullet_ceramichit[3];
	sfxHandle_t sfx_bullet_glasshit[3];
	sfxHandle_t gibSound;
	sfxHandle_t gibBounce1Sound;
	sfxHandle_t gibBounce2Sound;
	sfxHandle_t gibBounce3Sound;
	sfxHandle_t teleInSound;
	sfxHandle_t teleOutSound;
	sfxHandle_t noAmmoSound;
	sfxHandle_t respawnSound;
	sfxHandle_t talkSound;
	sfxHandle_t landSound;
	sfxHandle_t fallSound;
	sfxHandle_t jumpPadSound;

	sfxHandle_t oneMinuteSound;
	sfxHandle_t fiveMinuteSound;
	sfxHandle_t suddenDeathSound;

	sfxHandle_t threeFragSound;
	sfxHandle_t twoFragSound;
	sfxHandle_t oneFragSound;

	sfxHandle_t hitSound;
	sfxHandle_t hitTeamSound;
	sfxHandle_t impressiveSound;
	sfxHandle_t excellentSound;
	sfxHandle_t deniedSound;
	sfxHandle_t humiliationSound;

	sfxHandle_t takenLeadSound;
	sfxHandle_t tiedLeadSound;
	sfxHandle_t lostLeadSound;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

//	sfxHandle_t flightSound;
	sfxHandle_t underWaterSound;
	sfxHandle_t medkitSound;
	sfxHandle_t wineSound;
	sfxHandle_t bookSound;      //----(SA)	added
	sfxHandle_t staminaSound;   //----(SA)	added
	sfxHandle_t elecSound;
	sfxHandle_t fireSound;
	sfxHandle_t waterSound;

	// teamplay sounds
	sfxHandle_t redLeadsSound;
	sfxHandle_t blueLeadsSound;
	sfxHandle_t teamsTiedSound;

	// tournament sounds
	sfxHandle_t count3Sound;
	sfxHandle_t count2Sound;
	sfxHandle_t count1Sound;
	sfxHandle_t countFightSound;
	sfxHandle_t countPrepareSound;

	//----(SA) added
	sfxHandle_t debBounce1Sound;
	sfxHandle_t debBounce2Sound;
	sfxHandle_t debBounce3Sound;
	//----(SA) end

	//----(SA)	added
	sfxHandle_t grenadePulseSound4;
	sfxHandle_t grenadePulseSound3;
	sfxHandle_t grenadePulseSound2;
	sfxHandle_t grenadePulseSound1;
	//----(SA)

//----(SA)	added
	sfxHandle_t sparkSounds[2];
//----(SA)

	// Ridah
	sfxHandle_t flameSound;
	sfxHandle_t flameBlowSound;
	sfxHandle_t flameStartSound;
	sfxHandle_t flameStreamSound;
	sfxHandle_t lightningSounds[3];
	sfxHandle_t lightningZap;
	sfxHandle_t flameCrackSound;
	sfxHandle_t boneBounceSound;

	sfxHandle_t zombieSpiritSound;
	sfxHandle_t zombieSpiritLoopSound;
	sfxHandle_t zombieDeathSound;

	sfxHandle_t helgaSpiritLoopSound;
	sfxHandle_t helgaSpiritSound;
	sfxHandle_t helgaGaspSound;

	sfxHandle_t heinrichArmorBreak; //----(SA)
	sfxHandle_t protoArmorBreak;    //----(SA)
	sfxHandle_t superArmorBreak;    //----(SA)


	sfxHandle_t debrisHitSound;

	sfxHandle_t loperLightningSounds[3];
	sfxHandle_t loperLightningZap;

	sfxHandle_t lightningClap[5];

	sfxHandle_t batsFlyingLoopSound;

//	sfxHandle_t grenadebounce1;
//	sfxHandle_t grenadebounce2;
	sfxHandle_t grenadebounce[GRENBOUNCE_TOTAL][2]; //----(SA)	modified

	sfxHandle_t dynamitebounce1;    //----(SA)	added

	sfxHandle_t fbarrelexp1;
	sfxHandle_t fbarrelexp2;

	sfxHandle_t fkickwall;
	sfxHandle_t fkickflesh;
	sfxHandle_t fkickmiss;

	int bulletHitFleshScript;
	int bulletHitFleshMetalScript;

	int teslaZapScript;
	sfxHandle_t teslaLoopSound;
	// done.

	qhandle_t cursor;
	qhandle_t selectCursor;
	qhandle_t sizeCursor;

} cgMedia_t;


//
// SOUND SCRIPTING
//

typedef struct soundScriptSound_s
{
	char filename[MAX_QPATH];
	sfxHandle_t sfxHandle;
	int lastPlayed;

	struct soundScriptSound_s   *next;
} soundScriptSound_t;


#define MAX_SOUND_SCRIPT_SOUNDS 8192
extern soundScriptSound_t soundScriptSounds[MAX_SOUND_SCRIPT_SOUNDS];
//DAJ defined in cg_sound.c int	numSoundScriptSounds;


typedef struct soundScript_s
{
	int index;
	char name[MAX_QPATH];
	int channel;
	int attenuation;
	qboolean streaming;
	qboolean looping;
	float shakeScale;
	float shakeRadius;
	int shakeDuration;
	qboolean random;    // TODO
	int numSounds;
	soundScriptSound_t  *soundList;         // pointer into the global list of soundScriptSounds (defined below)

	struct soundScript_s    *nextHash;      // next soundScript in our hashTable list position
} soundScript_t;

// we have to define these static lists, since we can't alloc memory within the cgame

#define FILE_HASH_SIZE          1024
extern soundScript_t*      hashTable[FILE_HASH_SIZE];

#define MAX_SOUND_SCRIPTS       4096
extern soundScript_t soundScripts[MAX_SOUND_SCRIPTS];
//DAJ defined in cg_sound.c int	numSoundScripts;

extern soundScript_t soundScripts[MAX_SOUND_SCRIPTS];





// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t gameState;              // gamestate from server
	glconfig_t glconfig;                // rendering configuration
	float screenXScale;                 // derived from glconfig
	float screenYScale;
	float screenXBias;

	int serverCommandSequence;              // reliable command stream counter
	int processedSnapshotNum;            // the number of snapshots cgame has requested

	qboolean localServer;               // detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t gametype;

	// Rafael gameskill
	gameskill_t gameskill;
	// done

	int dmflags;
	int teamflags;
	int fraglimit;
	int capturelimit;
	int timelimit;
	int maxclients;
	char mapname[MAX_QPATH];
	char redTeam[MAX_QPATH];                // A team
	char blueTeam[MAX_QPATH];               // B team

	int voteTime;
	int voteYes;
	int voteNo;
	qboolean voteModified;                  // beep whenever changed
	char voteString[MAX_STRING_TOKENS];

	int teamVoteTime[2];
	int teamVoteYes[2];
	int teamVoteNo[2];
	qboolean teamVoteModified[2];           // beep whenever changed
	char teamVoteString[2][MAX_STRING_TOKENS];

	int levelStartTime;

	int scores1, scores2;                   // from configstrings

	//
	// locally derived information from gamestate
	//
	qhandle_t gameModels[MAX_MODELS];

	sfxHandle_t gameSounds[MAX_SOUNDS];
	int gameSoundTypes[MAX_SOUNDS];             //----(SA)	added

	int numInlineModels;
	qhandle_t inlineDrawModel[MAX_MODELS];
	vec3_t inlineModelMidpoints[MAX_MODELS];

	clientInfo_t clientinfo[MAX_CLIENTS];

	// teamchat width is *3 because of embedded color codes
	char teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH * 3 + 1];
	int teamChatMsgTimes[TEAMCHAT_HEIGHT];
	int teamChatPos;
	int teamLastChatPos;

	char itemPrintNames[MAX_ITEMS][32];             //----(SA)	added

	int cursorX;
	int cursorY;
	qboolean eventHandling;
	qboolean mouseCaptured;
	qboolean sizingHud;
	void *capturedItem;
	qhandle_t activeCursor;

	// screen fading
	//----(SA)	modified just in name so global searching is easier to narrow down (added 'scrF')
	float scrFadeAlpha, scrFadeAlphaCurrent;
	int scrFadeStartTime;
	int scrFadeDuration;

	// media
	cgMedia_t media;

	// player/AI model scripting (client repository)
	animScriptData_t animScriptData;

} cgs_t;

//==============================================================================

extern cgs_t cgs;
extern cg_t cg;
extern centity_t cg_entities[MAX_GENTITIES];
extern weaponInfo_t cg_weapons[MAX_WEAPONS];
extern itemInfo_t cg_items[MAX_ITEMS];
extern markPoly_t cg_markPolys[MAX_MARK_POLYS];

extern vmCvar_t cg_centertime;
extern vmCvar_t cg_runpitch;
extern vmCvar_t cg_runroll;
extern vmCvar_t cg_bobup;
extern vmCvar_t cg_bobpitch;
extern vmCvar_t cg_bobroll;
extern vmCvar_t cg_swingSpeed;
extern vmCvar_t cg_shadows;
extern vmCvar_t cg_gibs;
extern vmCvar_t cg_drawTimer;
extern vmCvar_t cg_drawFPS;
extern vmCvar_t cg_drawSnapshot;
extern vmCvar_t cg_draw3dIcons;
extern vmCvar_t cg_drawIcons;
extern vmCvar_t cg_youGotMail;          //----(SA)	added
extern vmCvar_t cg_drawAmmoWarning;
extern vmCvar_t cg_drawCrosshair;
extern vmCvar_t cg_drawCrosshairNames;
extern vmCvar_t cg_drawCrosshairPickups;
extern vmCvar_t cg_hudAlpha;
extern vmCvar_t cg_useWeapsForZoom;
extern vmCvar_t cg_weaponCycleDelay;            //----(SA)	added
extern vmCvar_t cg_cycleAllWeaps;
extern vmCvar_t cg_drawAllWeaps;
extern vmCvar_t cg_drawRewards;
extern vmCvar_t cg_drawTeamOverlay;
extern vmCvar_t cg_crosshairX;
extern vmCvar_t cg_crosshairY;
extern vmCvar_t cg_crosshairSize;
extern vmCvar_t cg_crosshairAlpha;          //----(SA)	added
extern vmCvar_t cg_crosshairHealth;
extern vmCvar_t cg_drawStatus;
extern vmCvar_t cg_draw2D;
extern vmCvar_t cg_drawFrags;
extern vmCvar_t cg_animSpeed;
extern vmCvar_t cg_debugAnim;
extern vmCvar_t cg_debugPosition;
extern vmCvar_t cg_debugEvents;
extern vmCvar_t cg_drawSpreadScale;
extern vmCvar_t cg_railTrailTime;
extern vmCvar_t cg_errorDecay;
extern vmCvar_t cg_nopredict;
extern vmCvar_t cg_noPlayerAnims;
extern vmCvar_t cg_showmiss;
extern vmCvar_t cg_footsteps;
extern vmCvar_t cg_markTime;
extern vmCvar_t cg_brassTime;
extern vmCvar_t cg_gun_frame;
extern vmCvar_t cg_gun_x;
extern vmCvar_t cg_gun_y;
extern vmCvar_t cg_gun_z;
extern vmCvar_t cg_drawGun;
extern vmCvar_t cg_drawFPGun;
extern vmCvar_t cg_drawGamemodels;
extern vmCvar_t cg_cursorHints;
extern vmCvar_t cg_hintFadeTime;            //----(SA)	added
extern vmCvar_t cg_viewsize;
extern vmCvar_t cg_letterbox;           //----(SA)	added
extern vmCvar_t cg_tracerChance;
extern vmCvar_t cg_tracerWidth;
extern vmCvar_t cg_tracerLength;
extern vmCvar_t cg_tracerSpeed;
extern vmCvar_t cg_autoswitch;
extern vmCvar_t cg_ignore;
extern vmCvar_t cg_simpleItems;
extern vmCvar_t cg_fov;
extern vmCvar_t cg_zoomFov;
extern vmCvar_t cg_zoomDefaultBinoc;
extern vmCvar_t cg_zoomDefaultSniper;
extern vmCvar_t cg_zoomDefaultFG;
extern vmCvar_t cg_zoomDefaultSnooper;
extern vmCvar_t cg_zoomStepBinoc;
extern vmCvar_t cg_zoomStepSniper;
extern vmCvar_t cg_zoomStepSnooper;
extern vmCvar_t cg_zoomStepFG;
extern vmCvar_t cg_reticles;
extern vmCvar_t cg_reticleBrightness;
extern vmCvar_t cg_thirdPersonRange;
extern vmCvar_t cg_thirdPersonAngle;
extern vmCvar_t cg_thirdPerson;
extern vmCvar_t cg_stereoSeparation;
extern vmCvar_t cg_lagometer;
extern vmCvar_t cg_drawAttacker;
extern vmCvar_t cg_synchronousClients;
extern vmCvar_t cg_teamChatTime;
extern vmCvar_t cg_teamChatHeight;
extern vmCvar_t cg_stats;
extern vmCvar_t cg_forceModel;
extern vmCvar_t cg_coronafardist;
extern vmCvar_t cg_coronas;
extern vmCvar_t cg_buildScript;
extern vmCvar_t cg_paused;
extern vmCvar_t cg_blood;
extern vmCvar_t cg_predictItems;
extern vmCvar_t cg_deferPlayers;
extern vmCvar_t cg_teamChatsOnly;
extern vmCvar_t cg_enableBreath;
extern vmCvar_t cg_autoactivate;
extern vmCvar_t cg_emptyswitch;
extern vmCvar_t cg_useSuggestedWeapons;         //----(SA)	added
extern vmCvar_t cg_particleDist;
extern vmCvar_t cg_particleLOD;
extern vmCvar_t cg_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

extern vmCvar_t cg_cameraOrbit;
extern vmCvar_t cg_cameraOrbitDelay;
extern vmCvar_t cg_timescaleFadeEnd;
extern vmCvar_t cg_timescaleFadeSpeed;
extern vmCvar_t cg_timescale;
extern vmCvar_t cg_cameraMode;
extern vmCvar_t cg_smallFont;
extern vmCvar_t cg_bigFont;

extern vmCvar_t cg_blinktime;           //----(SA)	added

extern vmCvar_t cg_currentSelectedPlayer;
extern vmCvar_t cg_currentSelectedPlayerName;

// Rafael - particle switch
extern vmCvar_t cg_wolfparticles;
// done

// Ridah
extern vmCvar_t cg_gameType;
extern vmCvar_t cg_bloodTime;
extern vmCvar_t cg_norender;
extern vmCvar_t cg_skybox;

// Rafael gameskill
extern vmCvar_t cg_gameSkill;
// done

extern vmCvar_t cg_reloading;           //----(SA)	added

// JPW NERVE
extern vmCvar_t cg_medicChargeTime;
extern vmCvar_t cg_engineerChargeTime;
extern vmCvar_t cg_LTChargeTime;
extern vmCvar_t cg_soldierChargeTime;
extern vmCvar_t cg_redlimbotime;
extern vmCvar_t cg_bluelimbotime;
// jpw

extern vmCvar_t cg_hunkUsed;
extern vmCvar_t cg_soundAdjust;
extern vmCvar_t cg_expectedhunkusage;

extern vmCvar_t cg_showAIState;

extern vmCvar_t cg_notebook;
extern vmCvar_t cg_notebookpages;           // bitflags for the currently accessable pages.  if they wanna cheat, let 'em.  Most won't, or will wait 'til they actually play it.

extern vmCvar_t cg_animState;
extern vmCvar_t cg_missionStats;
extern vmCvar_t cg_waitForFire;

extern vmCvar_t cg_loadWeaponSelect;

// NERVE - SMF - Wolf multiplayer configuration cvars
extern vmCvar_t mp_playerType;
extern vmCvar_t mp_weapon;
extern vmCvar_t mp_item1;
extern vmCvar_t mp_item2;
extern vmCvar_t mp_mapDesc;
extern vmCvar_t mp_mapTitle;
// -NERVE - SMF

//
// cg_main.c
//
const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );

void QDECL CG_Printf( const char *msg, ... );
void QDECL CG_Error( const char *msg, ... );

void CG_StartMusic( void );
void CG_QueueMusic( void ); //----(SA)	added

void CG_UpdateCvars( void );

int CG_CrosshairPlayer( void );
int CG_LastAttacker( void );
void CG_LoadMenus( const char *menuFile );
void CG_KeyEvent( int key, qboolean down );
void CG_MouseEvent( int x, int y );
void CG_EventHandling( int type );

qboolean CG_GetTag( int clientNum, char *tagname, orientation_t * or );
qboolean CG_GetWeaponTag( int clientNum, char *tagname, orientation_t * or );

//
// cg_view.c
//
void CG_TestModel_f( void );
void CG_TestGun_f( void );
void CG_TestModelNextFrame_f( void );
void CG_TestModelPrevFrame_f( void );
void CG_TestModelNextSkin_f( void );
void CG_TestModelPrevSkin_f( void );
void CG_ZoomDown_f( void );
void CG_ZoomIn_f( void );
void CG_ZoomOut_f( void );
void CG_ZoomUp_f( void );

void CG_DrawActiveFrame( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );

void CG_Concussive( centity_t *cent );
//
// cg_drawtools.c
//
void CG_AdjustFrom640( float *x, float *y, float *w, float *h );
void CG_FillRect( float x, float y, float width, float height, const float *color );
void CG_HorizontalPercentBar( float x, float y, float width, float height, float percent );
void CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void CG_FilledBar( float x, float y, float w, float h, const float *startColorIn, float *endColor, const float *bgColor, float frac, int flags );
// JOSEPH 10-26-99
void CG_DrawStretchPic( float x, float y, float width, float height, qhandle_t hShader );
// END JOSEPH
void CG_DrawString( float x, float y, const char *string,
					float charWidth, float charHeight, const float *modulate );


void CG_DrawStringExt( int x, int y, const char *string, const float *setColor,
					   qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars );
// JOSEPH 4-17-00
void CG_DrawStringExt2( int x, int y, const char *string, const float *setColor,
						qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars );
// END JOSEPH
void CG_DrawBigString( int x, int y, const char *s, float alpha );
void CG_DrawBigStringColor( int x, int y, const char *s, vec4_t color );
void CG_DrawSmallString( int x, int y, const char *s, float alpha );
void CG_DrawSmallStringColor( int x, int y, const char *s, vec4_t color );
// JOSEPH 4-25-00
void CG_DrawBigString2( int x, int y, const char *s, float alpha );
void CG_DrawBigStringColor2( int x, int y, const char *s, vec4_t color );
// END JOSEPH
int CG_DrawStrlen( const char *str );

float   *CG_FadeColor( int startMsec, int totalMsec );
float *CG_TeamColor( int team );
void CG_TileClear( void );
void CG_ColorForHealth( vec4_t hcolor );
void CG_GetColorForHealth( int health, int armor, vec4_t hcolor );

void UI_DrawProportionalString( int x, int y, const char* str, int style, vec4_t color );

// new hud stuff
void CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void CG_DrawSides( float x, float y, float w, float h, float size );
void CG_DrawTopBottom( float x, float y, float w, float h, float size );



//
// cg_draw.c, cg_newDraw.c
//
extern int sortedTeamPlayers[TEAM_MAXOVERLAY];
extern int numSortedTeamPlayers;
extern int drawTeamOverlayModificationCount;
extern char systemChat[256];
extern char teamChat1[256];
extern char teamChat2[256];

void CG_AddLagometerFrameInfo( void );
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_CenterPrint( const char *str, int y, int charWidth );
void CG_ObjectivePrint( const char *str, int charWidth, int team );     // NERVE - SMF
void CG_DrawHead( float x, float y, float w, float h, int clientNum, vec3_t headAngles );
void CG_DrawActive( stereoFrame_t stereoView );
void CG_DrawFlagModel( float x, float y, float w, float h, int team );

void CG_DrawTeamBackground( int x, int y, int w, int h, float alpha, int team );
void CG_OwnerDraw( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, int font, float scale, vec4_t color, qhandle_t shader, int textStyle );
void CG_Text_Paint( float x, float y, int font, float scale, vec4_t color, const char *text, float adjust, int limit, int style );    //----(SA)	modified
int CG_Text_Width( const char *text, int font, float scale, int limit );
int CG_Text_Height( const char *text, int font, float scale, int limit );
void CG_SelectPrevPlayer();
void CG_SelectNextPlayer();
float CG_GetValue( int ownerDraw, int type ); // 'type' is relative or absolute (fractional-'0.5' or absolute- '50' health)
qboolean CG_OwnerDrawVisible( int flags );
void CG_RunMenuScript( char **args );
void CG_ShowResponseHead();
void CG_SetPrintString( int type, const char *p );
void CG_InitTeamChat();
void CG_GetTeamColor( vec4_t *color );
const char *CG_GetGameStatusText();
const char *CG_GetKillerText();
void CG_Draw3DModel( float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles );
void CG_Text_PaintChar( float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader );
void CG_CheckOrderPending();
const char *CG_GameTypeString();
qboolean CG_YourTeamHasFlag();
qboolean CG_OtherTeamHasFlag();
qhandle_t CG_StatusHandle( int task );
void CG_Fade( int r, int g, int b, int a, int time, int duration ); //----(SA)	modified

void CG_CalcShakeCamera();
void CG_ApplyShakeCamera();



//
// cg_player.c
//
qboolean CG_EntOnFire( centity_t *cent );    // Ridah
void CG_Player( centity_t *cent );
void CG_ResetPlayerEntity( centity_t *cent );
void CG_AddRefEntityWithPowerups( refEntity_t *ent, int powerups, int team, entityState_t *es, const vec3_t fireRiseDir );
void CG_NewClientInfo( int clientNum );
sfxHandle_t CG_CustomSound( int clientNum, const char *soundName );

// Rafael particles
extern qboolean initparticles;
int CG_NewParticleArea( int num );

//
// cg_predict.c
//
void CG_BuildSolidList( void );
int CG_PointContents( const vec3_t point, int passEntityNum );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
			   int skipNumber, int mask );
void CG_PredictPlayerState( void );
void CG_LoadDeferredPlayers( void );


//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
const char  *CG_PlaceString( int rank );
void CG_EntityEvent( centity_t *cent, vec3_t position );
void CG_PainEvent( centity_t *cent, int health, qboolean crouching );


//
// cg_ents.c
//
void CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities( void );
void CG_Beam( centity_t *cent );
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t outDeltaAngles );

void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							 char *tagName, int startIndex, vec3_t *offset );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, char *tagName );


//----(SA)
void CG_AttachedPartChange( centity_t *cent );
void CG_NextItem_f( void );
void CG_PrevItem_f( void );
void CG_Item_f( void );
//----(SA)	end


//
// cg_weapons.c
//
void CG_LastWeaponUsed_f( void );     //----(SA)	added
void CG_NextWeaponInBank_f( void );   //----(SA)	added
void CG_PrevWeaponInBank_f( void );   //----(SA)	added
void CG_AltWeapon_f( void );
void CG_NextWeapon_f( void );
void CG_PrevWeapon_f( void );
void CG_Weapon_f( void );
void CG_WeaponBank_f( void );
void CG_WeaponSuggest( int weap );

void CG_FinishWeaponChange( int lastweap, int newweap );

void CG_RegisterWeapon( int weaponNum );
void CG_RegisterItemVisuals( int itemNum );

void CG_FireWeapon( centity_t *cent );   //----(SA)	modified.
//void CG_EndFireWeapon( centity_t *cent, int firemode );	//----(SA)	added
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, int surfaceFlags );   //	(SA) modified to send missilehitwall surface parameters

void CG_MissileHitWallSmall( int weapon, int clientNum, vec3_t origin, vec3_t dir );
void CG_DrawTracer( vec3_t start, vec3_t finish );

// Rafael
void CG_MG42EFX( centity_t *cent );

void CG_FLAKEFX( centity_t *cent, int whichgun );

void CG_MortarEFX( centity_t *cent );

// Ridah
qboolean CG_MonsterUsingWeapon( centity_t *cent, int aiChar, int weaponNum );

// Rafael
void CG_MissileHitWall2( int weapon, int clientNum, vec3_t origin, vec3_t dir );
// done

void CG_MissileHitPlayer( centity_t *cent, int weapon, vec3_t origin, vec3_t dir, int entityNum );
//----(SA)
void CG_VenomFire( entityState_t *es, qboolean fullmode );
//----(SA)
void CG_Bullet( vec3_t origin, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum, qboolean wolfkick, int otherEntNum2 );

void CG_RailTrail( clientInfo_t *ci, vec3_t start, vec3_t end, int type );   //----(SA)	added 'type'
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi );
void CG_AddViewWeapon( playerState_t *ps );
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent );
void CG_DrawWeaponSelect( void );
void CG_DrawHoldableSelect( void );

void CG_OutOfAmmoChange( void );
void CG_HoldableUsedupChange( void ); //----(SA)	added

//----(SA) added to header to access from outside cg_weapons.c
void CG_AddDebris( vec3_t origin, vec3_t dir, int speed, int duration, int count );
//----(SA) done

void CG_ClientDamage( int entnum, int enemynum, int id );

void CG_AddBulletParticles( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale );

//
// cg_marks.c
//
void    CG_InitMarkPolys( void );
void    CG_AddMarks( void );
void    CG_ImpactMark( qhandle_t markShader,
					   const vec3_t origin, const vec3_t dir,
					   float orientation,
					   float r, float g, float b, float a,
					   qboolean alphaFade,
					   float radius, qboolean temporary, int duration );

// Rafael particles
//
// cg_particles.c
//
void    CG_ClearParticles( void );
void    CG_AddParticles( void );
void    CG_ParticleSnow( qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum );
void    CG_ParticleSmoke( qhandle_t pshader, centity_t *cent );
void    CG_AddParticleShrapnel( localEntity_t *le );
void    CG_ParticleSnowFlurry( qhandle_t pshader, centity_t *cent );
void    CG_ParticleBulletDebris( vec3_t org, vec3_t vel, int duration );
void    CG_ParticleDirtBulletDebris( vec3_t org, vec3_t vel, int duration );     // DHM - Nerve
void    CG_ParticleDirtBulletDebris_Core( vec3_t org, vec3_t vel, int duration, float width, float height, float alpha, char *shadername );  // NERVE - SMF // JPW addtnl params
void    CG_ParticleSparks( vec3_t org, vec3_t vel, int duration, float x, float y, float speed );
void    CG_ParticleDust( centity_t *cent, vec3_t origin, vec3_t dir );
void    CG_ParticleMisc( qhandle_t pshader, vec3_t origin, int size, int duration, float alpha );

// Ridah
void CG_ParticleExplosion( char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd );

// Rafael snow pvs check
void    CG_SnowLink( centity_t *cent, qboolean particleOn );
// done.

// Rafael bats
void CG_ParticleBat( centity_t *cent );
void    CG_ParticleBats( qhandle_t pshader, centity_t *cent );
void    CG_BatsUpdatePosition( centity_t *cent );
void CG_ParticleImpactSmokePuff( qhandle_t pshader, vec3_t origin );
void CG_ParticleImpactSmokePuffExtended( qhandle_t pshader, vec3_t origin, vec3_t dir, int radius, int lifetime, int vel, int acc, int maxroll, float alpha );       // (SA) so I can add more parameters without screwing up the one that's there
void CG_Particle_Bleed( qhandle_t pshader, vec3_t start, vec3_t dir, int fleshEntityNum, int duration );
void CG_GetBleedOrigin( vec3_t head_origin, vec3_t torso_origin, vec3_t legs_origin, int fleshEntityNum );
void CG_Particle_OilParticle( qhandle_t pshader, vec3_t origin, vec3_t origin2, int ptime, int snum );
void CG_Particle_OilSlick( qhandle_t pshader, centity_t *cent );
void CG_OilSlickRemove( centity_t *cent );
void CG_BloodPool( localEntity_t *le, qhandle_t pshader, trace_t *tr );
void CG_ParticleBloodCloudZombie( centity_t *cent, vec3_t origin, vec3_t dir );
void CG_ParticleBloodCloud( centity_t *cent, vec3_t origin, vec3_t dir );
// done

// Ridah, trails
//
// cg_trails.c
//
int CG_AddTrailJunc( int headJuncIndex, qhandle_t shader, int spawnTime, int sType, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth, int flags, vec3_t colorStart, vec3_t colorEnd, float sRatio, float animSpeed );
int CG_AddSparkJunc( int headJuncIndex, qhandle_t shader, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth );
int CG_AddSmokeJunc( int headJuncIndex, qhandle_t shader, vec3_t pos, int trailLife, float alpha, float startWidth, float endWidth );
int CG_AddFireJunc( int headJuncIndex, qhandle_t shader, vec3_t pos, int trailLife, float alpha, float startWidth, float endWidth );
void CG_AddTrails( void );
void CG_ClearTrails( void );
// done.

// Ridah, sound scripting
int CG_SoundScriptPrecache( const char *name );
qboolean CG_SoundPlaySoundScript( const char *name, vec3_t org, int entnum );
void CG_SoundPlayIndexedScript( int index, vec3_t org, int entnum );
void CG_SoundInit( void );
// done.

// Ridah, flamethrower
void CG_FireFlameChunks( centity_t *cent, vec3_t origin, vec3_t angles, float speedScale, qboolean firing, int flags ); //----(SA)	added 'flags'
void CG_InitFlameChunks( void );
void CG_AddFlameChunks( void );
void CG_UpdateFlamethrowerSounds( void );
void CG_FlameDamage( int owner, vec3_t org, float radius );
// done.

//
// cg_localents.c
//
void    CG_InitLocalEntities( void );
localEntity_t   *CG_AllocLocalEntity( void );
void    CG_AddLocalEntities( void );

//
// cg_effects.c
//
int CG_GetOriginForTag( centity_t * cent, refEntity_t * parent, char *tagName, int startIndex, vec3_t org, vec3_t axis[3] );
localEntity_t *CG_SmokePuff( const vec3_t p,
							 const vec3_t vel,
							 float radius,
							 float r, float g, float b, float a,
							 float duration,
							 int startTime,
							 int fadeInTime,
							 int leFlags,
							 qhandle_t hShader );

void CG_BubbleTrail( vec3_t start, vec3_t end, float size, float spacing );
void CG_SpawnEffect( vec3_t org );
void CG_GibPlayer( centity_t *cent, vec3_t playerOrigin, vec3_t gdir );
void CG_LoseHat( centity_t *cent, vec3_t dir );         //----(SA)	added
void CG_GibHead( vec3_t headOrigin );

void CG_Bleed( vec3_t origin, int entityNum );

localEntity_t *CG_MakeExplosion( vec3_t origin, vec3_t dir,
								 qhandle_t hModel, qhandle_t shader, int msec,
								 qboolean isSprite );
// Ridah
void CG_DynamicLightningBolt( qhandle_t shader, vec3_t start, vec3_t pend, int numBolts, float maxWidth, qboolean fade, float startAlpha, int recursion, int randseed );
void CG_SparklerSparks( vec3_t origin, int count );
void CG_ClearFlameChunks( void );
void CG_ProjectedSpotLight( vec3_t start, vec3_t dir );
// done.

//----(SA)
void CG_Spotlight( centity_t *cent, float *color, vec3_t start, vec3_t dir, int segs, float range, int startWidth, float coneAngle, int flags );
#define SL_NOTRACE          0x001   // don't do a trace check for shortening the beam, always draw at full 'range' length
#define SL_NODLIGHT         0x002   // don't put a dlight at the end
#define SL_NOSTARTCAP       0x004   // dont' cap the start circle
#define SL_LOCKTRACETORANGE 0x010   // only trace out as far as the specified range (rather than to max spot range)
#define SL_NOFLARE          0x020   // don't draw a flare when the light is pointing at the camera
#define SL_NOIMPACT         0x040   // don't draw the impact mark
#define SL_LOCKUV           0x080   // lock the texture coordinates at the 'true' length of the requested beam.
#define SL_NOCORE           0x100   // don't draw the center 'core' beam
#define SL_TRACEWORLDONLY   0x200
//----(SA)	done

void CG_RumbleEfx( float pitch, float yaw );

//
// cg_snapshot.c
//
void CG_ProcessSnapshots( void );

//
// cg_info.c
//
void CG_LoadingString( const char *s );
void CG_LoadingItem( int itemNum );
void CG_LoadingClient( int clientNum );
void CG_DrawInformation( void );
const char *CG_translateString( const char *str );

//
// cg_scoreboard.c
//
qboolean CG_DrawScoreboard( void );
void CG_DrawTourneyScoreboard( void );

//
// cg_consolecmds.c
//
qboolean CG_ConsoleCommand( void );
void CG_InitConsoleCommands( void );

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands( int latestSequence );
void CG_ParseServerinfo( void );
void CG_SetConfigValues( void );
void CG_ShaderStateChanged( void );
void CG_SendMoveSpeed( animation_t *animList, int numAnims, char *modelName );

//
// cg_playerstate.c
//
void CG_Respawn( void );
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );
void CG_LoadClientInfo( clientInfo_t *ci );


//===============================================

//
// system traps
// These functions are how the cgame communicates with the main game system
//

// print message on the local console
void        trap_Print( const char *fmt );

// abort the game
void        trap_Error( const char *fmt );

// exit game to main menu (credits/etc)
void        trap_Endgame( void );   //----(SA)	added

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int         trap_Milliseconds( void );

// console variable interaction
void        trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void        trap_Cvar_Update( vmCvar_t *vmCvar );
void        trap_Cvar_Set( const char *var_name, const char *value );
void        trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

// ServerCommand and ConsoleCommand parameter access
int         trap_Argc( void );
void        trap_Argv( int n, char *buffer, int bufferLength );
void        trap_Args( char *buffer, int bufferLength );

// filesystem access
// returns length of file
int         trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void        trap_FS_Read( void *buffer, int len, fileHandle_t f );
void        trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void        trap_FS_FCloseFile( fileHandle_t f );
void        trap_FS_CopyFile( char *from, char *to );   //DAJ

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void        trap_SendConsoleCommand( const char *text );

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void        trap_AddCommand( const char *cmdName );

// send a string to the server over the network
void        trap_SendClientCommand( const char *s );

// force a screen update, only used during gamestate load
void        trap_UpdateScreen( void );

// model collision
void        trap_CM_LoadMap( const char *mapname );
int         trap_CM_NumInlineModels( void );
clipHandle_t trap_CM_InlineModel( int index );      // 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );
clipHandle_t trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs );
int         trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int         trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void        trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
							  const vec3_t mins, const vec3_t maxs,
							  clipHandle_t model, int brushmask );
void        trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
										 const vec3_t mins, const vec3_t maxs,
										 clipHandle_t model, int brushmask,
										 const vec3_t origin, const vec3_t angles );

void        trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
								  const vec3_t mins, const vec3_t maxs,
								  clipHandle_t model, int brushmask );
void        trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end,
											 const vec3_t mins, const vec3_t maxs,
											 clipHandle_t model, int brushmask,
											 const vec3_t origin, const vec3_t angles );

// Returns the projection of a polygon onto the solid brushes in the world
int         trap_CM_MarkFragments( int numPoints, const vec3_t *points,
								   const vec3_t projection,
								   int maxPoints, vec3_t pointBuffer,
								   int maxFragments, markFragment_t *fragmentBuffer );

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void        trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
void        trap_S_StartSoundEx( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags );
void        trap_S_StopLoopingSound( int entnum );

void        trap_S_StopStreamingSound( int entnum );  // usually AI.  character is talking and needs to be shut up /now/

// a local sound is always played full volume
void        trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void        trap_S_ClearLoopingSounds( qboolean killall );
void        trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int volume );
void        trap_S_AddRangedLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range );
void        trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void        trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );

// Ridah, talking animations
int         trap_S_GetVoiceAmplitude( int entityNum );
// done.

// repatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
sfxHandle_t trap_S_RegisterSound( const char *sample );     // returns buzz if not found
void        trap_S_StartBackgroundTrack( const char *intro, const char *loop, int fadeupTime ); // empty name stops music
void        trap_S_StopBackgroundTrack( void );
void        trap_S_FadeBackgroundTrack( float targetvol, int time, int sound );  //----(SA)	added
void        trap_S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation );
void        trap_S_FadeAllSound( float targetvol, int time ); //----(SA)	added

void        trap_R_LoadWorldMap( const char *mapname );

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t   trap_R_RegisterModel( const char *name );           // returns rgb axis if not found
qhandle_t   trap_R_RegisterSkin( const char *name );            // returns all white if not found
qhandle_t   trap_R_RegisterShader( const char *name );          // returns all white if not found
qhandle_t   trap_R_RegisterShaderNoMip( const char *name );         // returns all white if not found

qboolean    trap_R_GetSkinModel( qhandle_t skinid, const char *type, char *name );   //----(SA) added
qhandle_t   trap_R_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap );   //----(SA)	added

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void        trap_R_ClearScene( void );
void        trap_R_AddRefEntityToScene( const refEntity_t *re );

// polys are intended for simple wall marks, not really for doing
// significant construction
void        trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
// Ridah
void        trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
void        trap_RB_ZombieFXAddNewHit( int entityNum, const vec3_t hitPos, const vec3_t hitDir );
// done.
void        trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw );
void        trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int flags );  //----(SA)	modified
void        trap_R_RenderScene( const refdef_t *fd );
void        trap_R_SetColor( const float *rgba );   // NULL = 1,1,1,1
void        trap_R_DrawStretchPic( float x, float y, float w, float h,
								   float s1, float t1, float s2, float t2, qhandle_t hShader );
void        trap_R_DrawStretchPicGradient( float x, float y, float w, float h,
										   float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType );

void        trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int         trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void        trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );

//----(SA)
void    trap_R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density );

//----(SA)

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void        trap_GetGlconfig( glconfig_t *glconfig );

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void        trap_GetGameState( gameState_t *gamestate );

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void        trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean    trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean    trap_GetServerCommand( int serverCommandNumber );

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int         trap_GetCurrentCmdNumber( void );

qboolean    trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );

// used for the weapon/holdable select and zoom
void        trap_SetUserCmdValue( int stateValue, int holdValue, float sensitivityScale, int cld );     // NERVE - SMF - added cld

// aids for VM testing
void        testPrintInt( char *string, int i );
void        testPrintFloat( char *string, float f );

int         trap_MemoryRemaining( void );
void        trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font );
qboolean    trap_Key_IsDown( int keynum );
int         trap_Key_GetCatcher( void );
void        trap_Key_SetCatcher( int catcher );
int         trap_Key_GetKey( const char *binding );

// RF
void trap_SendMoveSpeedsToGame( int entnum, char *movespeeds );

typedef enum {
	SYSTEM_PRINT,
	CHAT_PRINT,
	TEAMCHAT_PRINT
} q3print_t; // bk001201 - warning: useless keyword or type name in empty declaration

void trap_UI_Popup( const char *arg0 );   //----(SA)	added
void trap_UI_ClosePopup( const char *arg0 );     // NERVE - SMF
void trap_UI_LimboChat( const char *arg0 );     // NERVE - SMF

int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
e_status trap_CIN_StopCinematic( int handle );
e_status trap_CIN_RunCinematic( int handle );
void trap_CIN_DrawCinematic( int handle );
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h );

void trap_SnapVector( float *v );

qboolean    trap_GetEntityToken( char *buffer, int bufferSize );

// Duffy, camera stuff
#define CAM_PRIMARY 0   // the main camera for cutscenes, etc.
qboolean    trap_loadCamera( int camNum, const char *name );
void        trap_startCamera( int camNum, int time );
void        trap_stopCamera( int camNum );    //----(SA)	added
qboolean    trap_getCameraInfo( int camNum, int time, vec3_t *origin, vec3_t *angles, float *fov );
void        CG_StartCamera( const char *name, qboolean startBlack );
void        CG_StopCamera( void );

//----(SA)	added
int         CG_LoadCamera( const char *name );
void        CG_FreeCamera( int camNum );
//----(SA)	end

void CG_StartShakeCamera( float p, int duration, vec3_t src, float radius );

qboolean    trap_GetModelInfo( int clientNum, char *modelName, animModelInfo_t **modelInfo );