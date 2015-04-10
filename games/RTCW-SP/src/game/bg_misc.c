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
 * name:		bg_misc.c
 *
 * desc:		both games misc functions, all completely stateless
 *
*/


#include "q_shared.h"
#include "bg_public.h"


#ifdef CGAMEDLL
extern vmCvar_t cg_gameType;
#endif
#ifdef GAMEDLL
extern vmCvar_t g_gametype;
#endif


// NOTE: weapons that share ammo (ex. colt/thompson) need to share max ammo, but not necessarily uses or max clip
#define MAX_AMMO_45     300
#define MAX_AMMO_9MM    300
#define MAX_AMMO_VENOM  1000
#define MAX_AMMO_MAUSER 200
#define MAX_AMMO_GARAND 10
#define MAX_AMMO_FG42   MAX_AMMO_MAUSER
#define MAX_AMMO_BAR    200


// these defines are matched with the character torso animations
#define DELAY_LOW       100 // machineguns, tesla, spear, flame
#define DELAY_HIGH      100 // mauser, garand
#define DELAY_PISTOL    100 // colt, luger, sp5, cross
#define DELAY_SHOULDER  50  // rl
#define DELAY_THROW     250 // grenades, dynamite

// [0] = maxammo		-	max player ammo carrying capacity.
// [1] = uses			-	how many 'rounds' it takes/costs to fire one cycle.
// [2] = maxclip		-	max 'rounds' in a clip.
// [3] = reloadTime		-	time from start of reload until ready to fire.
// [4] = fireDelayTime	-	time from pressing 'fire' until first shot is fired. (used for delaying fire while weapon is 'readied' in animation)
// [5] = nextShotTime	-	when firing continuously, this is the time between shots
// [6] = maxHeat		-	max active firing time before weapon 'overheats' (at which point the weapon will fail for a moment)
// [7] = coolRate		-	how fast the weapon cools down.
// [8] = mod			-	means of death

// potential inclusions in the table:
// damage			-
// splashDamage		-
// soundRange		-	distance which ai can hear the weapon
// ammoWarning		-	amount we give the player a 'low on ammo' warning (just a HUD color change or something)
// clipWarning		-	amount we give the player a 'low in clip' warning (just a HUD color change or something)
// maxclip2			-	allow the player to (mod/powerup) upgrade clip size when aplicable (luger has 8 round standard clip and 32 round snail magazine, for ex.)
//
//
//

ammotable_t ammoTable[] = {
	//	MAX				USES	MAX		RELOAD	FIRE			NEXT	HEAT,	COOL,	MOD,	...
	//	AMMO			AMT.	CLIP	TIME	DELAY			SHOT
	{   0,              0,      0,      0,      50,             0,      0,      0,      0                       },  //	WP_NONE					// 0

	{   999,            0,      999,    0,      50,             200,    0,      0,      MOD_KNIFE               },  //	WP_KNIFE				// 1

	{   MAX_AMMO_9MM,   1,      8,      1500,   DELAY_PISTOL,   400,    0,      0,      MOD_LUGER               },  //	WP_LUGER				// 2	// NOTE: also 32 round 'snail' magazine
	{   MAX_AMMO_9MM,   1,      32,     2600,   DELAY_LOW,      100,    0,      0,      MOD_MP40                },  //	WP_MP40					// 3
	{   MAX_AMMO_MAUSER,1,      10,     2500,   DELAY_HIGH,     1200,   0,      0,      MOD_MAUSER              },  //	WP_MAUSER				// 4	// NOTE: authentic clips are 5/10/25 rounds
	{   MAX_AMMO_FG42,  1,      20,     2000,   DELAY_LOW,      200,    0,      0,      MOD_FG42                },  //	WP_FG42					// 5
	{   15,             1,      15,     1000,   DELAY_THROW,    1600,   0,      0,      MOD_GRENADE_LAUNCHER    },  //	WP_GRENADE_LAUNCHER		// 6
	{   5,              1,      1,      1000,   DELAY_SHOULDER, 2000,   0,      0,      MOD_PANZERFAUST         },  //	WP_PANZERFAUST			// 7
//	{	MAX_AMMO_VENOM,	1,		500,	3000,	750,			30,		5000,	200,	MOD_VENOM				},	//	WP_VENOM				// -
	{   MAX_AMMO_VENOM, 1,      500,    3000,   750,            45,     5000,   200,    MOD_VENOM               },  //	WP_VENOM				// 8	// JPW NOTE: changed next_shot 50->45 to genlock firing to every server frame (fire rate shouldn't be framerate dependent now)
	{   150,            1,      150,    1000,   DELAY_LOW,      50,     0,      0,      MOD_FLAMETHROWER        },  //	WP_FLAMETHROWER			// 9
	{   300,            1,      300,    1000,   DELAY_LOW,      0,      0,      0,      MOD_TESLA               },  //	WP_TESLA				// 10
//	{	50,				1,		50,		1000,	DELAY_LOW,		1200,	0,		0,		MOD_SPEARGUN			},	//	WP_SPEARGUN				// 11

//	{	999,			0,		999,	0,		50,				200,	0,		0,		MOD_KNIFE2				},	//	WP_KNIFE2				// 12
	{   MAX_AMMO_45,    1,      8,      1500,   DELAY_PISTOL,   400,    0,      0,      MOD_COLT                },  //	WP_COLT					// 13
	{   MAX_AMMO_45,    1,      30,     2400,   DELAY_LOW,      120,    0,      0,      MOD_THOMPSON            },  //	WP_THOMPSON				// 14	// NOTE: also 50 round drum magazine
	{   MAX_AMMO_GARAND,1,      5,      2500,   DELAY_HIGH,     1200,   0,      0,      MOD_GARAND              },  //	WP_GARAND				// 15	// NOTE: always 5 round clips
//	{	MAX_AMMO_BAR,	1,		20,		2000,	DELAY_LOW,		200,	0,		0,		MOD_BAR					},	//	WP_BAR					// 16
	{   15,             1,      15,     1000,   DELAY_THROW,    1600,   0,      0,      MOD_GRENADE_PINEAPPLE   },  //	WP_GRENADE_PINEAPPLE	// 17
//	{	5,				1,		5,		1000,	DELAY_SHOULDER,	1200,	0,		0,		MOD_ROCKET_LAUNCHER		},	//	WP_ROCKET_LAUNCHER		// 18

	{   MAX_AMMO_MAUSER,1,      10,     3000,   0,              1700,   0,      0,      MOD_SNIPERRIFLE         },  //	WP_SNIPER_GER			// 19
	{   MAX_AMMO_GARAND,1,      5,      3000,   0,              1200,   0,      0,      MOD_SNOOPERSCOPE        },  //	WP_SNIPER_AM			// 20
//	{	MAX_AMMO_VENOM,	10,		300,	3000,	1200,			1200,	0,		0,		MOD_VENOM_FULL			},	//	WP_VENOM_FULL			// -
//	{	MAX_AMMO_VENOM,	10,		300,	3000,	1000,			1000,	0,		0,		MOD_VENOM_FULL			},	//	WP_VENOM_FULL			// 21
//	{	20,				1,		20,		1000,	DELAY_LOW,		1200,	0,		0,		MOD_SPEARGUN_CO2		},	//	WP_SPEARGUN_CO2			// 22

	{   MAX_AMMO_FG42,  1,      20,     2000,   DELAY_LOW,      200,    0,      0,      MOD_FG42SCOPE           },  //	WP_FG42SCOPE			// 23
//	{	MAX_AMMO_BAR,	1,		20,		2000,	DELAY_LOW,		90,		0,		0,		MOD_BAR					},	//	WP_BAR2					// 24
	{   MAX_AMMO_9MM,   1,      32,     3100,   DELAY_LOW,      110,    700,    300,    MOD_STEN                },  //	WP_STEN					// 25
	{   MAX_AMMO_9MM,   1,      8,      1500,   DELAY_PISTOL,   400,    0,      0,      MOD_SILENCER            },  //	WP_SILENCER				// 26
	{   MAX_AMMO_45,    1,      8,      2700,   DELAY_PISTOL,   200,    0,      0,      MOD_AKIMBO              },  //	WP_AKIMBO				// 27

	{   999,            0,      999,    0,      50,             0,      0,      0,      0                       },  //	WP_CLASS_SPECIAL		// 28	//	class_special
//	{	100,			1,		100,	1000,	DELAY_PISTOL,	900,	0,		0,		MOD_CROSS				},	//	WP_CROSS				// 29
	{   10,             1,      10,     1000,   DELAY_THROW,    1600,   0,      0,      MOD_DYNAMITE            },  //	WP_DYNAMITE				// 30
//	{	10,				1,		10,		1000,	DELAY_THROW,	1600,	0,		0,		MOD_DYNAMITE			},	//	WP_DYNAMITE2			// 31

// stubs for some "not-real" weapons (so they always return "yes, you have enough ammo for that gauntlet", etc.)
//	{	5,				1,		5,		1000,	DELAY_SHOULDER,	1200,	0,		0,		0 /*mod_prox*/			},	//	WP_PROX					// 32
	{   999,            0,      999,    0,      50,             0,      0,      0,      0                       },  //	WP_MONSTER_ATTACK1		// 33
	{   999,            0,      999,    0,      50,             0,      0,      0,      0                       },  //	WP_MONSTER_ATTACK2		// 34
	{   999,            0,      999,    0,      50,             0,      0,      0,      0                       },  //	WP_MONSTER_ATTACK3		// 35
	{   999,            0,      999,    0,      50,             0,      0,      0,      0                       }   //	WP_GAUNTLET				// 36
};


//----(SA)	moved in here so both games can get to it
int weapAlts[] = {
	WP_NONE,            // 0 WP_NONE
	WP_NONE,            // 1 WP_KNIFE
	WP_SILENCER,        // 2 WP_LUGER
	WP_NONE,            // 3 WP_MP40
	WP_SNIPERRIFLE,     // 4 WP_MAUSER
	WP_FG42SCOPE,       // 5 WP_FG42	// was SP5
	WP_NONE,            // 6 WP_GRENADE_LAUNCHER
	WP_NONE,            // 7 WP_PANZERFAUST
	WP_NONE,            // 8 WP_VENOM
	WP_NONE,            // 9 WP_FLAMETHROWER
	WP_NONE,            // 10 WP_TESLA
//	WP_SPEARGUN_CO2,	// 11 WP_SPEARGUN
//	WP_NONE,			// 12 WP_KNIFE2
	WP_AKIMBO,          // 13 WP_COLT		//----(SA)	new
	WP_NONE,            // 14 WP_THOMPSON
	WP_SNOOPERSCOPE,    // 15 WP_GARAND
//	WP_BAR2,			// 16 WP_BAR		//----(SA)	modified
	WP_NONE,            // 17 WP_GRENADE_PINEAPPLE
//	WP_NONE,			// 18 WP_ROCKET_LAUNCHER
	WP_MAUSER,          // 19 WP_SNIPERRIFLE
	WP_GARAND,          // 20 WP_SNOOPERSCOPE
//	WP_VENOM,			// 21 WP_VENOM_FULL
//	WP_SPEARGUN,		// 22 WP_SPEARGUN_CO2
	WP_FG42,            // 23 WP_FG42SCOPE
//	WP_BAR,				// 24 WP_BAR2		//----(SA)	new
	WP_NONE,            // 25 WP_STEN
	WP_LUGER,           // 26 WP_SILENCER	//----(SA)	was sp5
	WP_COLT,            // 27 WP_AKIMBO		//----(SA)	new
	WP_NONE,            // 28 WP_CLASS_SPECIAL
//	WP_NONE,			// 29 WP_CROSS
	WP_NONE             // 30 WP_DYNAMITE	//----(SA)	modified (not in rotation yet)
//	WP_DYNAMITE			// 31 WP_DYNAMITE2	//----(SA)	new
};


// new (10/18/00)
char *animStrings[] = {
	"BOTH_DEATH1",
	"BOTH_DEAD1",
	"BOTH_DEAD1_WATER",
	"BOTH_DEATH2",
	"BOTH_DEAD2",
	"BOTH_DEAD2_WATER",
	"BOTH_DEATH3",
	"BOTH_DEAD3",
	"BOTH_DEAD3_WATER",

	"BOTH_CLIMB",
	"BOTH_CLIMB_DOWN",
	"BOTH_CLIMB_DISMOUNT",

	"BOTH_SALUTE",

	"BOTH_PAIN1",
	"BOTH_PAIN2",
	"BOTH_PAIN3",
	"BOTH_PAIN4",
	"BOTH_PAIN5",
	"BOTH_PAIN6",
	"BOTH_PAIN7",
	"BOTH_PAIN8",

	"BOTH_GRAB_GRENADE",

	"BOTH_ATTACK1",
	"BOTH_ATTACK2",
	"BOTH_ATTACK3",
	"BOTH_ATTACK4",
	"BOTH_ATTACK5",

	"BOTH_EXTRA1",
	"BOTH_EXTRA2",
	"BOTH_EXTRA3",
	"BOTH_EXTRA4",
	"BOTH_EXTRA5",
	"BOTH_EXTRA6",
	"BOTH_EXTRA7",
	"BOTH_EXTRA8",
	"BOTH_EXTRA9",
	"BOTH_EXTRA10",
	"BOTH_EXTRA11",
	"BOTH_EXTRA12",
	"BOTH_EXTRA13",
	"BOTH_EXTRA14",
	"BOTH_EXTRA15",
	"BOTH_EXTRA16",
	"BOTH_EXTRA17",
	"BOTH_EXTRA18",
	"BOTH_EXTRA19",
	"BOTH_EXTRA20",

	"TORSO_GESTURE",
	"TORSO_GESTURE2",
	"TORSO_GESTURE3",
	"TORSO_GESTURE4",

	"TORSO_DROP",

	"TORSO_RAISE",   // (low)
	"TORSO_ATTACK",
	"TORSO_STAND",
	"TORSO_STAND_ALT1",
	"TORSO_STAND_ALT2",
	"TORSO_READY",
	"TORSO_RELAX",

	"TORSO_RAISE2",  // (high)
	"TORSO_ATTACK2",
	"TORSO_STAND2",
	"TORSO_STAND2_ALT1",
	"TORSO_STAND2_ALT2",
	"TORSO_READY2",
	"TORSO_RELAX2",

	"TORSO_RAISE3",  // (pistol)
	"TORSO_ATTACK3",
	"TORSO_STAND3",
	"TORSO_STAND3_ALT1",
	"TORSO_STAND3_ALT2",
	"TORSO_READY3",
	"TORSO_RELAX3",

	"TORSO_RAISE4",  // (shoulder)
	"TORSO_ATTACK4",
	"TORSO_STAND4",
	"TORSO_STAND4_ALT1",
	"TORSO_STAND4_ALT2",
	"TORSO_READY4",
	"TORSO_RELAX4",

	"TORSO_RAISE5",  // (throw)
	"TORSO_ATTACK5",
	"TORSO_ATTACK5B",
	"TORSO_STAND5",
	"TORSO_STAND5_ALT1",
	"TORSO_STAND5_ALT2",
	"TORSO_READY5",
	"TORSO_RELAX5",

	"TORSO_RELOAD1", // (low)
	"TORSO_RELOAD2", // (high)
	"TORSO_RELOAD3", // (pistol)
	"TORSO_RELOAD4", // (shoulder)

	"TORSO_MG42",        // firing tripod mounted weapon animation

	"TORSO_MOVE",        // torso anim to play while moving and not firing (swinging arms type thing)
	"TORSO_MOVE_ALT",        // torso anim to play while moving and not firing (swinging arms type thing)

	"TORSO_EXTRA",
	"TORSO_EXTRA2",
	"TORSO_EXTRA3",
	"TORSO_EXTRA4",
	"TORSO_EXTRA5",
	"TORSO_EXTRA6",
	"TORSO_EXTRA7",
	"TORSO_EXTRA8",
	"TORSO_EXTRA9",
	"TORSO_EXTRA10",

	"LEGS_WALKCR",
	"LEGS_WALKCR_BACK",
	"LEGS_WALK",
	"LEGS_RUN",
	"LEGS_BACK",
	"LEGS_SWIM",
	"LEGS_SWIM_IDLE",

	"LEGS_JUMP",
	"LEGS_JUMPB",
	"LEGS_LAND",

	"LEGS_IDLE",
	"LEGS_IDLE_ALT", //	"LEGS_IDLE2"
	"LEGS_IDLECR",

	"LEGS_TURN",

	"LEGS_BOOT",     // kicking animation

	"LEGS_EXTRA1",
	"LEGS_EXTRA2",
	"LEGS_EXTRA3",
	"LEGS_EXTRA4",
	"LEGS_EXTRA5",
	"LEGS_EXTRA6",
	"LEGS_EXTRA7",
	"LEGS_EXTRA8",
	"LEGS_EXTRA9",
	"LEGS_EXTRA10",
};


// old
char *animStringsOld[] = {
	"BOTH_DEATH1",
	"BOTH_DEAD1",
	"BOTH_DEATH2",
	"BOTH_DEAD2",
	"BOTH_DEATH3",
	"BOTH_DEAD3",

	"BOTH_CLIMB",
	"BOTH_CLIMB_DOWN",
	"BOTH_CLIMB_DISMOUNT",

	"BOTH_SALUTE",

	"BOTH_PAIN1",
	"BOTH_PAIN2",
	"BOTH_PAIN3",
	"BOTH_PAIN4",
	"BOTH_PAIN5",
	"BOTH_PAIN6",
	"BOTH_PAIN7",
	"BOTH_PAIN8",

	"BOTH_EXTRA1",
	"BOTH_EXTRA2",
	"BOTH_EXTRA3",
	"BOTH_EXTRA4",
	"BOTH_EXTRA5",

	"TORSO_GESTURE",
	"TORSO_GESTURE2",
	"TORSO_GESTURE3",
	"TORSO_GESTURE4",

	"TORSO_DROP",

	"TORSO_RAISE",   // (low)
	"TORSO_ATTACK",
	"TORSO_STAND",
	"TORSO_READY",
	"TORSO_RELAX",

	"TORSO_RAISE2",  // (high)
	"TORSO_ATTACK2",
	"TORSO_STAND2",
	"TORSO_READY2",
	"TORSO_RELAX2",

	"TORSO_RAISE3",  // (pistol)
	"TORSO_ATTACK3",
	"TORSO_STAND3",
	"TORSO_READY3",
	"TORSO_RELAX3",

	"TORSO_RAISE4",  // (shoulder)
	"TORSO_ATTACK4",
	"TORSO_STAND4",
	"TORSO_READY4",
	"TORSO_RELAX4",

	"TORSO_RAISE5",  // (throw)
	"TORSO_ATTACK5",
	"TORSO_ATTACK5B",
	"TORSO_STAND5",
	"TORSO_READY5",
	"TORSO_RELAX5",

	"TORSO_RELOAD1", // (low)
	"TORSO_RELOAD2", // (high)
	"TORSO_RELOAD3", // (pistol)
	"TORSO_RELOAD4", // (shoulder)

	"TORSO_MG42",        // firing tripod mounted weapon animation

	"TORSO_MOVE",        // torso anim to play while moving and not firing (swinging arms type thing)

	"TORSO_EXTRA2",
	"TORSO_EXTRA3",
	"TORSO_EXTRA4",
	"TORSO_EXTRA5",

	"LEGS_WALKCR",
	"LEGS_WALKCR_BACK",
	"LEGS_WALK",
	"LEGS_RUN",
	"LEGS_BACK",
	"LEGS_SWIM",

	"LEGS_JUMP",
	"LEGS_LAND",

	"LEGS_IDLE",
	"LEGS_IDLE2",
	"LEGS_IDLECR",

	"LEGS_TURN",

	"LEGS_BOOT",     // kicking animation

	"LEGS_EXTRA1",
	"LEGS_EXTRA2",
	"LEGS_EXTRA3",
	"LEGS_EXTRA4",
	"LEGS_EXTRA5",
};

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) SUSPENDED SPIN PERSISTANT
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
SUSPENDED - will allow items to hang in the air, otherwise they are dropped to the next surface.
SPIN - will allow items to spin in place.
PERSISTANT - some items (ex. clipboards) can be picked up, but don't disappear

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
"stand" if the item has a stand (ex: mp40_stand.md3) this specifies which stand tag to attach the weapon to ("stand":"4" would mean "tag_stand4" for example)  only weapons support stands currently
*/

// JOSEPH 5-2-00
//----(SA) the addition of the 'ammotype' field was added by me, not removed by id (SA)
gitem_t bg_itemlist[] =
{
	{
		NULL,
		NULL,
		{ NULL,
		  NULL,
		  0, 0, 0},
		NULL,   // icon
		NULL,   // ammo icon
		NULL,   // pickup
		0,
		0,
		0,
		0,          // ammotype
		0,          // cliptype
		"",          // precache
		"",          // sounds
		{0,0,0,0}
	},  // leave index 0 alone



/*QUAKED item_clipboard (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN PERSISTANT
"model" - model to display in the world.  defaults to 'models/powerups/clipboard/clipboard.md3' (the clipboard laying flat is 'clipboard2.md3')
"popup" - menu to popup.  no default since you won't want the same clipboard laying around. (clipboard will display a 'put popup here' message)
"notebookpage" - when clipboard is picked up, this page (menu) will be added to your notebook (FIXME: TODO: more info goes here)

We currently use:
clip_interrogation
clip_codeddispatch
clip_alertstatus

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/clipboard/clipboard.md3"
*/
/*
"scriptName"
*/
	{
		"item_clipboard",
		"sound/pickup/armor/body_pickup.wav",
		{   "models/powerups/clipboard/clipboard.md3",
			0,
			0,
			0, 0 },
		"icons/iconh_small",
		NULL,                   // ammo icon
		"",
		1,
		IT_CLIPBOARD,
		0,
		0,
		0,
		"",
		"",
		{0,0,0,0}
	},

/*QUAKED item_treasure (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN
Items the player picks up that are just used to tally a score at end-level
"model" defaults to 'models/powerups/treasure/goldbar.md3'
"noise" sound to play on pickup.  defaults to 'sound/pickup/treasure/gold.wav'
"message" what to call the item when it's picked up.  defaults to "Treasure Item" (SA: temp)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/treasure/goldbar.md3"
*/
/*
"scriptName"
*/
	{
		"item_treasure",
		"sound/pickup/treasure/gold.wav",
		{   "models/powerups/treasure/goldbar.md3",
			0,
			0,
			0, 0 },
		"icons/iconh_small", // (SA) placeholder
		NULL,                   // ammo icon
		"Treasure Item",     // (SA) placeholder
		5,
		IT_TREASURE,
		0,
		0,
		0,
		"",
		"",
		{0,0,0,0}
	},


	//
	// ARMOR/HEALTH/STAMINA
	//


/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_s.md3"
*/
	{
		"item_health_small",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_s.md3",
			0,
			0, 0,  0 },
		"icons/iconh_small",
		NULL,   // ammo icon
		"Small Health",
		5,
		IT_HEALTH,
		0,
		0,
		0,
		"",
		"",
		{10,10,5,5}
	},

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_m.md3"
*/
	{
		"item_health",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_m.md3",
			0,
			0, 0,  0 },
		"icons/iconh_med",
		NULL,   // ammo icon
		"Med Health",
		25,
		IT_HEALTH,
		0,
		0,
		0,
		"",
		"",
		{25,25,15,15}
	},

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_l.md3"
*/
	{
		"item_health_large",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_l.md3",
			0, 0, 0,   0 },
		"icons/iconh_large",
		NULL,   // ammo icon
		"Large Health",
		50,
		IT_HEALTH,
		0,
		0,
		0,
		"",
		"",
		{50,50,30,20}
	},

/*QUAKED item_health_turkey (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
multi-stage health item.
gives amount on first use based on skill:
skill 1: 50
skill 2: 50
skill 3: 50
skill 4: 40
skill 5: 30

then gives 15 on "finishing up"

player will only eat what he needs.  health at 90, turkey fills up and leaves remains (leaving 15).  health at 5 you eat the whole thing.
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_t1.md3"
*/
	{
		"item_health_turkey",
		"sound/pickup/health/hot_pickup.wav",
		{   "models/powerups/health/health_t3.md3",  // just plate (should now be destructable)
			"models/powerups/health/health_t2.md3",  // half eaten
			"models/powerups/health/health_t1.md3",  // whole turkey
			0, 0 },
		"icons/iconh_turkey",
		NULL,   // ammo icon
		"Hot Meal",
		15,                 // amount given in last stage
		IT_HEALTH,
		0,
		0,
		0,
		"",
		"",
		{45,35,25,15}   // amount given in first stage based on gameskill level
	},

/*QUAKED item_health_breadandmeat (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
multi-stage health item.
gives amount on first use based on skill:
skill 1: 30
skill 2: 30
skill 3: 30
skill 4: 20
skill 5: 10

then gives 10 on "finishing up"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_b1.md3"
*/
	{
		"item_health_breadandmeat",
		"sound/pickup/health/cold_pickup.wav",
		{   "models/powerups/health/health_b3.md3",  // just plate (should now be destructable)
			"models/powerups/health/health_b2.md3",  // half eaten
			"models/powerups/health/health_b1.md3",  // whole turkey
			0, 0 },
		"icons/iconh_breadandmeat",
		NULL,   // ammo icon
		"Cold Meal",
		10,                 // amount given in last stage
		IT_HEALTH,
		0,
		0,
		0,
		"",
		"",
		{30,30,20,15}   // amount given in first stage based on gameskill level
	},

/*QUAKED item_health_wall_box (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED - - RESPAWN
single use health with dual state model.
please set the suspended flag to keep it from falling on the ground
defaults to 50 pts health
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_wallbox.md3"
*/
	{
		"item_health_wall_box",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_wallbox2.md3",
			"models/powerups/health/health_wallbox1.md3",
			0, 0, 0},
		"icons/iconh_wall",
		NULL,   // ammo icon
		"Health",
		25,
		IT_HEALTH,
		0,
		0,
		0,
		"",
		"",
		{25,25,25,25}
	},

/*QUAKED item_health_wall (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED - - RESPAWN
defaults to 50 pts health
you will probably want to check the 'suspended' box to keep it from falling to the ground
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/health/health_w.md3"
*/
	{
		"item_health_wall",
		"sound/pickup/health/health_pickup.wav",
		{   "models/powerups/health/health_w.md3",
			0, 0, 0,   0 },
		"icons/iconh_wall",
		NULL,   // ammo icon
		"Health",
		25,
		IT_HEALTH,
		0,
		0,
		0,
		"",
		"",
		{25,25,15,15}
	},

	//
	// STAMINA
	//


/*QUAKED item_stamina_stein (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
defaults to 30 sec stamina boost
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/instant/stamina_stein.md3"
*/

	{
		"item_stamina_stein",
		"sound/pickup/health/stamina_pickup.wav",
		{   "models/powerups/instant/stamina_stein.md3",
			0, 0, 0,   0 },
		"icons/icons_stein",
		NULL,   // ammo icon
		"Stamina",
		25,
		IT_POWERUP,
		PW_NOFATIGUE,
		0,
		0,
		"",
		"",
		{30,25,20,15}
	},


/*QUAKED item_stamina_brandy (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
defaults to 30 sec stamina boost

multi-stage health item.
gives amount on first use based on skill:
skill 1: 50
skill 2: 50
skill 3: 50
skill 4: 40
skill 5: 30

then gives 15 on "finishing up"

player will only eat what he needs.  health at 90, turkey fills up and leaves remains (leaving 15).  health at 5 you eat the whole thing.
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/instant/stamina_brandy1.md3"
*/

	{
		"item_stamina_brandy",
		"sound/sound/pickup/health/stamina_pickup.wav",
		{   "models/powerups/instant/stamina_brandy2.md3",
			"models/powerups/instant/stamina_brandy1.md3",
			0, 0,  0 },
		"icons/icons_brandy",
		NULL,   // ammo icon
		"Stamina",
		25,
		IT_POWERUP,
		PW_NOFATIGUE,
		0,
		0,
		"",
		"",
		{30,25,20,15}
	},


	//
	// ARMOR
	//


/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/armor/armor_body1.md3"
*/
	{
		"item_armor_body",
		"sound/pickup/armor/body_pickup.wav",
		{   "models/powerups/armor/armor_body1.md3",
			0, 0, 0,   0 },
		"icons/iconr_body",
		NULL,   // ammo icon
		"Flak Jacket",
		75,
		IT_ARMOR,
		0,
		0,
		0,
		"",
		"",
		{75,75,75,75}
	},

/*QUAKED item_armor_body_hang (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED - - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/armor/armor_body2.md3"
*/
	{
		"item_armor_body_hang",
		"sound/pickup/armor/body_pickup.wav",
		{   "models/powerups/armor/armor_body2.md3",
			0, 0, 0,   0 },
		"icons/iconr_bodyh",
		NULL,   // ammo icon
		"Flak Jacket",
		75,
		IT_ARMOR,
		0,
		0,
		0,
		"",
		"",
		{75,75,75,75}
	},

/*QUAKED item_armor_head (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/armor/armor_head1.md3"
*/
	{
		"item_armor_head",
		"sound/pickup/armor/head_pickup.wav",
		{   "models/powerups/armor/armor_head1.md3",
			0, 0, 0,   0 },
		"icons/iconr_head",
		NULL,   // ammo icon
		"Armored Helmet",
		25,
		IT_ARMOR,
		0,
		0,
		0,
		"",
		"",
		{25,25,25,25}
	},



	//
	// WEAPONS
	//

/*
weapon_gauntlet
*/
	{
		"weapon_gauntlet",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/gauntlet/gauntlet.md3",
			0, 0, 0,    0 },
		"",  // icon
		"icons/ammo1",           // ammo icon
		"Gauntlet",              // pickup
		0,
		IT_WEAPON,
		WP_GAUNTLET,
		WP_GAUNTLET,
		WP_GAUNTLET,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},



	// wolf weapons (SA)

/*QUAKED weapon_knife (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/knife/knife.md3"
*/
	{
		"weapon_knife",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/knife/knife.md3",
			"models/weapons2/knife/v_knife.md3",
			"models/weapons2/knife/pu_knife.md3",
			0, 0 },

		"icons/iconw_knife_1",   // icon
		"icons/ammo2",           // ammo icon
		"Knife",             // pickup
		50,
		IT_WEAPON,
		WP_KNIFE,
		WP_KNIFE,
		WP_KNIFE,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},


/* weapon_knife2 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/knife2/knife2.md3"
*/
/*
	{
		"weapon_knife2",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/knife2/knife2.md3",
			"models/weapons2/knife2/v_knife2.md3",
			"models/weapons2/knife2/pu_knife2.md3",
			0, 0 },

		"icons/iconw_knife2_1",	// icon
		"icons/ammo2",			// ammo icon
		"Other Knife",			// pickup
		50,
		IT_WEAPON,
		WP_KNIFE2,
		WP_KNIFE2,
		WP_KNIFE2,
		"",						// precache
		"",						// sounds
		{0,0,0,0}
	},
*/


/*QUAKED weapon_luger (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/luger/luger.md3"
*/
	{
		"weapon_luger",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/luger/luger.md3",
			"models/weapons2/luger/v_luger.md3",
			"models/weapons2/luger/pu_luger.md3",
			0, 0 },

		"icons/iconw_luger_1",   // icon
		"icons/ammo2",           // ammo icon
		"Luger",             // pickup
		50,
		IT_WEAPON,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},


/*QUAKED weapon_mauserRifle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mauser/mauser.md3"
*/
	{
		"weapon_mauserRifle",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/mauser/mauser.md3",
			"models/weapons2/mauser/v_mauser.md3",
			"models/weapons2/mauser/pu_mauser.md3",
			0, 0  },

		"icons/iconw_mauser_1",  // icon
		"icons/ammo3",           // ammo icon
		"Mauser Rifle",          // pickup
		50,
		IT_WEAPON,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

/*QUAKED weapon_thompson (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/thompson/thompson.md3"
*/
	{
		"weapon_thompson",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/thompson/thompson.md3",
			"models/weapons2/thompson/v_thompson.md3",
			"models/weapons2/thompson/pu_thompson.md3",
			0, 0 },

		"icons/iconw_thompson_1",    // icon
		"icons/ammo2",           // ammo icon
		"Thompson",              // pickup
		30,
		IT_WEAPON,
		WP_THOMPSON,
		WP_COLT,
		WP_THOMPSON,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

/*QUAKED weapon_sten (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/sten/sten.md3"
*/
	{
		"weapon_sten",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/sten/sten.md3",
			"models/weapons2/sten/v_sten.md3",
			"models/weapons2/sten/pu_sten.md3",
			0,0 },
		"icons/iconw_sten_1",    // icon
		"icons/ammo2",           // ammo icon
		"Sten",                  // pickup
		30,
		IT_WEAPON,
		WP_STEN,
		WP_LUGER,
		WP_STEN,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

/*weapon_akimbo
dual colts
*/
	{
		"weapon_akimbo",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/colt2/colt2.md3",
			"models/weapons2/colt2/v_colt2.md3",
			"models/weapons2/colt2/pu_colt2.md3",
			0, 0 },

		"icons/iconw_colt_1",    // icon
		"icons/ammo2",           // ammo icon
		"Dual Colts",            // pickup
		50,
		IT_WEAPON,
		WP_AKIMBO,
		WP_COLT,
//		WP_COLT,
		WP_AKIMBO,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

/*QUAKED weapon_colt (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/colt/colt.md3"
*/
	{
		"weapon_colt",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/colt/colt.md3",
			"models/weapons2/colt/v_colt.md3",
			"models/weapons2/colt/pu_colt.md3",
			0, 0 },

		"icons/iconw_colt_1",    // icon
		"icons/ammo2",           // ammo icon
		"Colt",                  // pickup
		50,
		IT_WEAPON,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},


// (SA) snooper is the parent, so 'garand' is no longer available as a stand-alone weap w/ an optional scope
/*
weapon_garandRifle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/garand/garand.md3"
*/
	{
		"NOT_weapon_garandRifle",    //----(SA)	modified so it can no longer be given individually
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/garand/garand.md3",
			"models/weapons2/garand/v_garand.md3",
			"models/weapons2/garand/pu_garand.md3",
			0, 0 },

		"icons/iconw_garand_1",  // icon
		"icons/ammo3",           // ammo icon
		"",                      // pickup			//----(SA)	modified so it can no longer be given individually
		50,
		IT_WEAPON,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

/*QUAKED weapon_mp40 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
"stand" values:
	no value:	laying in a default position on it's side (default)
	2:			upright, barrel pointing up, slightly angled (rack mount)
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models\weapons2\mp40\mp40.md3"
*/
	{
		"weapon_mp40",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/mp40/mp40.md3",
			"models/weapons2/mp40/v_mp40.md3",
			"models/weapons2/mp40/pu_mp40.md3",
			0, 0 },

		"icons/iconw_mp40_1",    // icon
		"icons/ammo2",       // ammo icon
		"MP40",              // pickup
		30,
		IT_WEAPON,
		WP_MP40,
		WP_LUGER,
		WP_MP40,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},



/*QUAKED weapon_fg42 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/fg42/fg42.md3"
*/
	{
		"weapon_fg42",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/fg42/fg42.md3",
			"models/weapons2/fg42/v_fg42.md3",
			"models/weapons2/fg42/pu_fg42.md3",
			0, 0 },

		"icons/iconw_fg42_1",    // icon
		"icons/ammo5",          // ammo icon
		"FG42 Paratroop Rifle",      // pickup
		10,
		IT_WEAPON,
		WP_FG42,
		WP_MAUSER,
		WP_FG42,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

//----(SA)	added

/* weapon_bar (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/bar/bar.md3"
*/
/*
	{
		"weapon_bar",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/bar/bar.md3",
			"models/weapons2/bar/v_bar.md3",
			"models/weapons2/bar/pu_bar.md3",
			0, 0 },

		"icons/iconw_bar_1",	// icon
			"icons/ammo5",		// ammo icon
		"BAR",		// pickup
		10,
		IT_WEAPON,
		WP_BAR,
		WP_BAR,
		WP_BAR,
		"",					// precache
		"",					// sounds
		{0,0,0,0}
	},

*/
/* weapon_bar2
*/
/*
	{
		"weapon_bar2",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/bar/bar.md3",
			"models/weapons2/bar/v_bar.md3",
			"models/weapons2/bar/pu_bar.md3",
			0, 0 },

		"icons/iconw_bar_1",	// icon
			"icons/ammo5",		// ammo icon
		"BAR",		// pickup
		10,
		IT_WEAPON,
		WP_BAR2,
		WP_BAR,
		WP_BAR,
		"",					// precache
		"",					// sounds
		{0,0,0,0}
	},
*/

//----(SA)	end


//----(SA)	modified sp5 to be silencer mod for luger
/*QUAKED weapon_silencer (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/sp5/sp5.md3"
*/
	{
		"weapon_silencer",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/silencer/silencer.md3", //----(SA)	changed 10/25
			"models/weapons2/silencer/v_silencer.md3",
			"models/weapons2/silencer/pu_silencer.md3",
			0, 0},

		"icons/iconw_silencer_1",    // icon
		"icons/ammo5",       // ammo icon
//		"Silencer",		// pickup
		"sp5 pistol",
		10,
		IT_WEAPON,
		WP_SILENCER,
		WP_LUGER,
		WP_LUGER,
		"",                  // precache
		"",                  // sounds
		{0,0,0,0}
	},

/*QUAKED weapon_panzerfaust (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/panzerfaust/pf.md3"
*/
	{
		"weapon_panzerfaust",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/panzerfaust/pf.md3",
			"models/weapons2/panzerfaust/v_pf.md3",
			"models/weapons2/panzerfaust/pu_pf.md3",
			0, 0 },

		"icons/iconw_panzerfaust_1", // icon
		"icons/ammo6",       // ammo icon
		"Panzerfaust",               // pickup
		1,
		IT_WEAPON,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

/* weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/rocketl/rocketl.md3"
*/
/*
	{
		"weapon_rocketlauncher",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/rocketl/rocketl.md3",
			"models/weapons2/rocketl/v_rocketl.md3",
			"models/weapons2/rocketl/pu_rocketl.md3",
			0, 0 },

		"icons/iconw_rocket_1",	// icon
		"icons/ammo6",		// ammo icon
		"Bazooka",				// pickup
		5,
		IT_WEAPON,
		WP_ROCKET_LAUNCHER,
		WP_ROCKET_LAUNCHER,
		WP_ROCKET_LAUNCHER,
		"",						// precache
		"",						// sounds
		{0,0,0,0}
	},
*/

/* weapon_proxlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/prox/prox.md3"
*/

/*
	{
		"weapon_proxlauncher",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/rocketl/rocketl.md3",
			"models/weapons2/rocketl/v_rocketl.md3",
			"models/weapons2/rocketl/pu_rocketl.md3",
			0, 0 },

		"icons/iconw_rocket_1",	// icon
		"icons/ammo6",		// ammo icon
		"Bazooka",				// pickup
		5,
		IT_WEAPON,
		WP_PROX,
		WP_PROX,
		WP_PROX,
		"",						// precache
		"",						// sounds
		{0,0,0,0}
	},
*/

//----(SA)	removed the quaked for this.  we don't actually have a grenade launcher as such.  It's given implicitly
//			by virtue of getting grenade ammo.  So we don't need to have them in maps
/*
weapon_grenadelauncher
*/
	{
		"weapon_grenadelauncher",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/grenade/grenade.md3",
			"models/weapons2/grenade/v_grenade.md3",
			"models/weapons2/grenade/pu_grenade.md3",
			0, 0 },

		"icons/iconw_grenade_1", // icon
		"icons/icona_grenade",   // ammo icon
		"Grenade",               // pickup
		6,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		"",                      // precache
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",             // sounds
		{0,0,0,0}
	},

/*
weapon_grenadePineapple
*/
	{
		"weapon_grenadepineapple",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/grenade/pineapple.md3",
			"models/weapons2/grenade/v_pineapple.md3",
			"models/weapons2/grenade/pu_pineapple.md3",
			0, 0 },

		"icons/iconw_pineapple_1",   // icon
		"icons/icona_pineapple", // ammo icon
		"Pineapple",             // pickup
		6,
		IT_WEAPON,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		"",                      // precache
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",             // sounds
		{0,0,0,0}
	},

/* JPW NERVE
weapon_grenadesmoke
*/
/*	// just commented out to remove the registration errors in SP (we register every weapon in every map in SP)
	{
		"weapon_grenadesmoke",
		"sound/misc/w_pkup.wav",
		{	"models/multiplayer/smokegrenade/smokegrenade.md3",
			"models/multiplayer/smokegrenade/v_smokegrenade.md3",
			"models/multiplayer/smokegrenade/pu_smokegrenade.md3",
			0, 0},

		"icons/iconw_smokegrenade_1",	// icon
		"icons/ammo2",	// ammo icon
		"smokeGrenade",				// pickup
		50,
		IT_WEAPON,
		WP_GRENADE_SMOKE,
		WP_GRENADE_SMOKE,
		WP_GRENADE_SMOKE,
		"",						// precache
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav",				// sounds
		{0,0,0,0}
	},
// jpw
*/
// DHM - Nerve
/*
weapon_medic_heal
*/
/*	{
		"weapon_medic_heal",
		"sound/misc/w_pkup.wav",
		{	"models/multiplayer/medpack/medpack.md3",
			"models/multiplayer/medpack/v_medpack.md3",
			"models/multiplayer/medpack/pu_medpack.md3",
			0, 0 },

		"icons/iconw_medheal_1",	// icon
		"icons/ammo2",			// ammo icon
		"Medic Heal",			// pickup
		50,
		IT_WEAPON,
		WP_MEDIC_HEAL,
		WP_MEDIC_HEAL,
		WP_MEDIC_HEAL,
		"",						// precache
		"",						// sounds
		{0,0,0,0}
	},
// dhm
*/
/*
weapon_dynamite
*/
	{
		"weapon_dynamite",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/dynamite/dynamite.md3",
			"models/weapons2/dynamite/v_dynamite.md3",
			"models/weapons2/dynamite/pu_dynamite.md3",
			0, 0 },

		"icons/iconw_dynamite_1",    // icon
		"icons/ammo9",           // ammo icon
		"Dynamite Weapon",       // pickup
		7,
		IT_WEAPON,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},


/*
weapon_dynamite2
*/
/*
	{
		"weapon_dynamite2",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/dynamite/dynamite.md3",
			"models/weapons2/dynamite/v_dynamite.md3",
			"models/weapons2/dynamite/pu_dynamite.md3",
			0, 0 },

		"icons/iconw_dynamite_1",	// icon
		"icons/ammo9",			// ammo icon
		"Dynamite Weapon",		// pickup
		7,
		IT_WEAPON,
		WP_DYNAMITE2,
		WP_DYNAMITE,
		WP_DYNAMITE,
		"",						// precache
		"",						// sounds
		{0,0,0,0}
	},
*/



/*QUAKED weapon_venom (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/venom/pu_venom.md3"
*/
	{
		"weapon_venom",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/venom/venom.md3",
			"models/weapons2/venom/v_venom.md3",
			"models/weapons2/venom/pu_venom.md3",
			0, 0 },

		"icons/iconw_venom_1",   // icon
		"icons/ammo8",           // ammo icon
		"Venom",             // pickup
		700,
		IT_WEAPON,
		WP_VENOM,
		WP_VENOM,
		WP_VENOM,
		"",                      // precache
		"",                      // sounds
		{0,0,0,0}
	},

/* weapon_venom_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/venom/pu_venom.md3"
*/
/*
	{
		"weapon_venom_shotgun",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/venom/venom.md3",
			"models/weapons2/venom/v_venom.md3",
			"models/weapons2/venom/pu_venom.md3",
			0, 0 },

		"icons/iconw_venom_1",	// icon
		"icons/ammo8",			// ammo icon
		"Venom Shotgun",		// pickup
		700,
		IT_WEAPON,
		WP_VENOM_FULL,
		WP_VENOM,
		WP_VENOM_FULL,
		"",						// precache
		"",						// sounds
		{0,0,0,0}
	},
*/

/* weapon_cross(.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/cross/cross.md3"
*/
/*
	{
		"weapon_cross",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/cross/cross.md3",
			"models/weapons2/cross/v_cross.md3",
			"models/weapons2/cross/pu_cross.md3",
			0, 0 },

		"icons/iconw_cross_1",	// icon
		"icons/ammo9",			// ammo icon
		"This dont exist",	// pickup
		7,
		IT_WEAPON,
		WP_CROSS,
		WP_CROSS,
		WP_CROSS,
		"",						// precache
		"",						// sounds
		{0,0,0,0}
	},
*/


/*QUAKED weapon_flamethrower (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/flamethrower/pu_flamethrower.md3"
*/
	{
		"weapon_flamethrower",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/flamethrower/flamethrower.md3",
			"models/weapons2/flamethrower/v_flamethrower.md3",
			"models/weapons2/flamethrower/pu_flamethrower.md3",
			0, 0 },

		"icons/iconw_flamethrower_1",    // icon
		"icons/ammo10",              // ammo icon
		"Flamethrower",              // pickup
		200,
		IT_WEAPON,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},


/*QUAKED weapon_tesla (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/tesla/pu_tesla.md3"
*/
	{
		"weapon_tesla",
		"sound/misc/w_pkup.wav",

		{   "models/weapons2/tesla/tesla.md3",
			"models/weapons2/tesla/v_tesla.md3",
			"models/weapons2/tesla/pu_tesla.md3",
			0, 0 },

		"icons/iconw_tesla_1",   // icon
		"icons/ammo10",              // ammo icon
		"Tesla Gun",             // pickup
		200,
		IT_WEAPON,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},


//----(SA)	new weaps

/* weapon_speargun (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/speargun/pu_speargun.md3"
*/
/*
	{
		"weapon_speargun",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/speargun/speargun.md3",
			"models/weapons2/speargun/v_speargun.md3",
			"models/weapons2/speargun/pu_speargun.md3",
			0, 0 },
		"icons/iconw_speargun_1",	// icon
		"icons/ammo10",				// ammo icon
		"Speargun",					// pickup
		50,
		IT_WEAPON,
		WP_SPEARGUN,
		WP_SPEARGUN,
		WP_SPEARGUN,
		"",							// precache
		"",							// sounds
		{0,0,0,0}
	},
	*/

/* weapon_speargun_c02 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/speargun/pu_speargun.md3"
*/
/*
	{
		"weapon_speargun_co2",
		"sound/misc/w_pkup.wav",
		{	"models/weapons2/speargun/speargun.md3",
			"models/weapons2/speargun/v_speargun.md3",
			"models/weapons2/speargun/pu_speargun.md3",
			0, 0 },

		"icons/iconw_speargun_1",	// icon
		"icons/ammo10",				// ammo icon
		"CO2 Spears",					// pickup
		50,
		IT_WEAPON,
		WP_SPEARGUN_CO2,
		WP_SPEARGUN_CO2,
		WP_SPEARGUN_CO2,
		"",							// precache
		"",							// sounds
		{0,0,0,0}
	},
*/

/*QUAKED weapon_sniperScope (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/mauser/pu_mauser_scope.md3"
*/
	{
		"weapon_sniperScope",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/mauser/mauser.md3",
			"models/weapons2/mauser/v_mauser.md3",
//			"models/weapons2/mauser/v_mauser_scope.md3",
			"models/weapons2/mauser/pu_mauser_scope.md3",
			0, 0 },

//		"icons/iconw_sniper_1",	// icon
		"icons/iconw_mauser_1",  // icon
		"icons/ammo10",              // ammo icon
		"Sniper Scope",              // pickup
		200,
		IT_WEAPON,
		WP_SNIPERRIFLE,
		WP_MAUSER,
		WP_MAUSER,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

/*QUAKED weapon_snooperrifle (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/weapons2/garand/garand.md3"
*/
	{
		"weapon_snooperrifle",   //----(SA)	modified
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/garand/garand.md3",
			"models/weapons2/garand/v_garand.md3",
			"models/weapons2/garand/pu_garand.md3",
			0, 0 },

		"icons/iconw_garand_1",  // icon
		"icons/ammo10",              // ammo icon
		"Snooper Rifle",             // pickup		//----(SA)	modified
		20,
		IT_WEAPON,
		WP_SNOOPERSCOPE,
		WP_GARAND,
		WP_GARAND,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

/* weapon_fg42scope
*/
	{
		"weapon_fg42scope",  //----(SA)	modified
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/fg42/fg42.md3",
			"models/weapons2/fg42/v_fg42.md3",
			"models/weapons2/fg42/pu_fg42.md3",
			0, 0},

		"icons/iconw_fg42_1",    // icon
		"icons/ammo5",               // ammo icon
		"FG42 Scope",                // pickup		//----(SA)	modified
		0,
		IT_WEAPON,
		WP_FG42SCOPE,   // this weap
		WP_MAUSER,      // shares ammo w/
		WP_FG42,        // shares clip w/
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},


// Ridah, need this for the scripting
/*
weapon_monster_attack1 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_monster_attack1",
		"",
		{   "",
			"",
			0, 0},
		"",  // icon
		NULL,   // ammo icon
		"MonsterAttack1",            // pickup
		100,
		IT_WEAPON,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,         // ammo
		WP_MONSTER_ATTACK1,         //
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},
/*
weapon_monster_attack2 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_monster_attack2",
		"",
		{   "",
			"",
			0, 0},
		"",  // icon
		NULL,   // ammo icon
		"MonsterAttack2",            // pickup
		100,
		IT_WEAPON,
		WP_MONSTER_ATTACK2,
		WP_MONSTER_ATTACK2,         // ammo
		WP_MONSTER_ATTACK2,         //
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},
/*
weapon_monster_attack3 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_monster_attack3",
		"",
		{   "",
			"",
			0, 0},
		"",  // icon
		NULL,   // ammo icon
		"MonsterAttack3",            // pickup
		100,
		IT_WEAPON,
		WP_MONSTER_ATTACK3,
		WP_MONSTER_ATTACK3,         // ammo
		WP_MONSTER_ATTACK3,
		"",                          // precache
		"",                          // sounds
		{0,0,0,0}
	},

/*
weapon_mortar (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_mortar",
		"sound/misc/w_pkup.wav",
		{   "models/weapons2/grenade/grenade.md3",
			"models/weapons2/grenade/v_grenade.md3",
			"models/weapons2/grenade/pu_grenade.md3",
			0, 0},
		"icons/iconw_grenade_1", // icon
		"icons/icona_grenade",   // ammo icon
		"nopickup(WP_MORTAR)",       // pickup
		6,
		IT_WEAPON,
		WP_MORTAR,
		WP_MORTAR,
		WP_MORTAR,
		"",                      // precache
		"sound/weapons/mortar/mortarf1.wav",             // sounds
		{0,0,0,0}
	},


// JPW NERVE -- class-specific multiplayer weapon, can't be picked up, dropped, or placed in map
/*
weapon_class_special (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
/*
	{
		"weapon_class_special",
		"sound/misc/w_pkup.wav",
		{	"models/multiplayer/hammer/hammer.md3",
			"models/multiplayer/hammer/v_hammer.md3",
			"models/multiplayer/hammer/pu_hammer.md3",
			0, 0, },

		"icons/iconw_hammer_1",	// icon
		"icons/ammo2",			// ammo icon
		"Special",				// pickup
		50, // this should never be picked up
		IT_WEAPON,
		WP_CLASS_SPECIAL,
		WP_CLASS_SPECIAL,
		WP_CLASS_SPECIAL,
		"",						// precache
		"",						// sounds
		{0,0,0,0}
	},
// jpw
*/




	//
	// AMMO ITEMS
	//



/*QUAKED ammo_9mm_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Luger pistol, MP40 machinegun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am9mm_s.md3"
*/
	{
		"ammo_9mm_small",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am9mm_s.md3",
		  0, 0, 0, 0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		"9mm Rounds",        // pickup
		30,
		IT_AMMO,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                  // precache
		"",                  // sounds
		{32,24,16,16}
	},
/*QUAKED ammo_9mm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Luger pistol, MP40 machinegun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am9mm_m.md3"
*/
	{
		"ammo_9mm",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am9mm_m.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		"9mm",           // pickup			//----(SA)	changed
		60,
		IT_AMMO,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                  // precache
		"",                  // sounds
		{64,48,32,32}
	},
/*QUAKED ammo_9mm_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Luger pistol, MP40 machinegun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am9mm_l.md3"
*/
	{
		"ammo_9mm_large",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am9mm_l.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		"9mm Box",           // pickup
		100,
		IT_AMMO,
		WP_LUGER,
		WP_LUGER,
		WP_LUGER,
		"",                  // precache
		"",                  // sounds
		{96,64,48,48}
	},


/*QUAKED ammo_45cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Thompson, Colt

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am45cal_s.md3"
*/
	{
		"ammo_45cal_small",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am45cal_s.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		".45cal Rounds", // pickup
		20,
		IT_AMMO,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                  // precache
		"",                  // sounds
		{30,20,15,15}
	},
/*QUAKED ammo_45cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Thompson, Colt

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am45cal_m.md3"
*/
	{
		"ammo_45cal",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am45cal_m.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		".45cal",        // pickup			//----(SA)	changed
		60,
		IT_AMMO,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                  // precache
		"",                  // sounds
		{60,45,30,30}
	},
/*QUAKED ammo_45cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Thompson, Colt

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am45cal_l.md3"
*/
	{
		"ammo_45cal_large",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am45cal_l.md3",
		  0, 0, 0,    0 },
		"icons/iconw_luger_1", // icon
		NULL,               // ammo icon
		".45cal Box",        // pickup
		100,
		IT_AMMO,
		WP_COLT,
		WP_COLT,
		WP_COLT,
		"",                  // precache
		"",                  // sounds
		{90,60,45,45}
	},




/*QUAKED ammo_792mm_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Mauser rifle, FG42

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am792mm_s.md3"
*/
	{
		"ammo_792mm_small",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am792mm_s.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		"7.92mm Rounds",         // pickup
		50,
		IT_AMMO,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                          // precache
		"",                          // sounds
		{16,12,8,8}
	},
/*QUAKED ammo_792mm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Mauser rifle, FG42

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am792mm_m.md3"
*/
	{
		"ammo_792mm",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am792mm_m.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		"7.92mm",                // pickup			//----(SA)	changed
		10,
		IT_AMMO,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                          // precache
		"",                          // sounds
		{32,24,16,16}
	},
/*QUAKED ammo_792mm_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Mauser rifle, FG42

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am792mm_l.md3"
*/
	{
		"ammo_792mm_large",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am792mm_l.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		"7.92mm Box",                // pickup
		50,
		IT_AMMO,
		WP_MAUSER,
		WP_MAUSER,
		WP_MAUSER,
		"",                          // precache
		"",                          // sounds
		{48,32,24,24}
	},




/*QUAKED ammo_30cal_small (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Garand rifle

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am30cal_s.md3"
*/
	{
		"ammo_30cal_small",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am30cal_s.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		".30cal Rounds",         // pickup
		50,
		IT_AMMO,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		"",                          // precache
		"",                          // sounds
		{5,2,2,2}
	},
/*QUAKED ammo_30cal (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Garand rifle

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am30cal_m.md3"
*/
	{
		"ammo_30cal",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am30cal_m.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		".30cal",                // pickup			//----(SA)	changed
		50,
		IT_AMMO,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		"",                          // precache
		"",                          // sounds
		{5,5,5,5    }
	},
/*QUAKED ammo_30cal_large (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Garand rifle

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am30cal_l.md3"
*/
	{
		"ammo_30cal_large",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am30cal_l.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		".30cal Box",                // pickup
		50,
		IT_AMMO,
		WP_GARAND,
		WP_GARAND,
		WP_GARAND,
		"",                          // precache
		"",                          // sounds
		{10,10,10,5}
	},




/*QUAKED ammo_127mm (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Venom gun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/am127mm.md3"
*/
	{
		"ammo_127mm",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/am127mm.md3",
		  0, 0, 0,    0 },
		"icons/icona_machinegun",    // icon
		NULL,                       // ammo icon
		"12.7mm",                    // pickup
		100,
		IT_AMMO,
		WP_VENOM,
		WP_VENOM,
		WP_VENOM,
		"",                          // precache
		"",                          // sounds
		{100,75,50,50}
	},

/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amgren_bag.md3"
*/
	{
		"ammo_grenades",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amgren_bag.md3",
		  0, 0, 0,    0 },
		"icons/icona_grenade",   // icon
		NULL,                   // ammo icon
		"Grenades",              // pickup
		5,
		IT_AMMO,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		WP_GRENADE_LAUNCHER,
		"",                      // precache
		"",                      // sounds
		{5,4,3,3}
	},

/*QUAKED ammo_grenades_american (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amgrenus_bag.md3"
*/
	{
		"ammo_grenades_american",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amgrenus_bag.md3",
		  0, 0, 0,    0 },
		"icons/icona_pineapple", // icon
		NULL,                   // ammo icon
		"Pineapples",            // pickup
		5,
		IT_AMMO,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		WP_GRENADE_PINEAPPLE,
		"",                      // precache
		"",                      // sounds
		{5,4,3,3}
	},

/*QUAKED ammo_dynamite (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN

 -------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/dynamite.md3"
*/
	{
		"ammo_dynamite",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/dynamite.md3",
		  0, 0, 0,    0 },
		"icons/icona_dynamite",  // icon
		NULL,                   // ammo icon
		"Dynamite",              // pickup
		1,
		IT_AMMO,
		WP_DYNAMITE,
		WP_DYNAMITE,
		WP_DYNAMITE,
		"",                      // precache
		"",                      // sounds
		{1,1,1,1}
	},


/*QUAKED ammo_cell (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Tesla

Boosts recharge on Tesla
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amcell.md3"
*/
	{
		"ammo_cell",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amcell.md3",
		  0, 0, 0,    0 },
		"icons/icona_cell",  // icon
		NULL,               // ammo icon
		"Cell",              // pickup
		500,
		IT_AMMO,
		WP_TESLA,
		WP_TESLA,
		WP_TESLA,
		"",                  // precache
		"",                  // sounds
		{100,75,50,50}
	},



/*QUAKED ammo_fuel (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Flamethrower

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amfuel.md3"
*/
	{
		"ammo_fuel",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amfuel.md3",
		  0, 0, 0,    0 },
		"icons/icona_fuel",  // icon
		NULL,               // ammo icon
		"Fuel",              // pickup
		100,
		IT_AMMO,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		WP_FLAMETHROWER,
		"",                  // precache
		"",                  // sounds
		{100,75,50,50}
	},


/* ammo_speargun (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Speargun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amspear.md3"
*/
/*
	{
		"ammo_speargun",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amspear.md3",
		0, 0, 0,	0 },
		"icons/icona_spear",	// icon
		NULL,					// ammo icon
		"Speargun Bolts",			// pickup
		10,
		IT_AMMO,
		WP_SPEARGUN,
		WP_SPEARGUN,
		WP_SPEARGUN,
		"",					// precache
		"",					// sounds
		{100,75,50,50}
	},
*/


/* ammo_speargun_co2 (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
CO2 tipped speargun bolts

used by: Speargun

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amspear.md3"
*/
/*
	{
		"ammo_speargun_co2",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amspear.md3",
		0, 0, 0,	0 },
		"icons/icona_spear",	// icon
		NULL,					// ammo icon
		"C02 Speargun Bolts",	// pickup
		10,
		IT_AMMO,
		WP_SPEARGUN_CO2,
		WP_SPEARGUN_CO2,
		WP_SPEARGUN_CO2,
		"",					// precache
		"",					// sounds
		{100,75,50,50}
	},
*/


//----(SA)	removed ammo_sniper(_n)

//----(SA)	removed ammo_snooper(_n)


/*QUAKED ammo_panzerfaust (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: German Panzerfaust

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/ampf.md3"
*/
	{
		"ammo_panzerfaust",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/ampf.md3",
		  0, 0, 0,    0 },
		"icons/icona_panzerfaust",   // icon
		NULL,                   // ammo icon
		"Panzerfaust Rockets",               // pickup
		5,
		IT_AMMO,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		WP_PANZERFAUST,
		"",                      // precache
		"",                      // sounds
		{4,3,2,2}
	},


/* ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Allied Rocket Launcher (bazooka)

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amrocket.md3"
*/
/*
	{
		"ammo_rockets",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amrocket.md3",
		0, 0, 0,	0 },
		"icons/icona_rocket",	// icon
		NULL,					// ammo icon
		"Rockets",				// pickup
		5,
		IT_AMMO,
		WP_ROCKET_LAUNCHER,
		WP_ROCKET_LAUNCHER,
		WP_ROCKET_LAUNCHER,
		"",						// precache
		"",						// sounds
		{4,3,2,2}
	},
*/

/* ammo_charges (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN
used by: Cross of Coronado

-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/ammo/amcharges.md3"
*/
/*
	{
		"ammo_charges",
		"sound/misc/am_pkup.wav",
		{ "models/powerups/ammo/amcharges.md3",
		0, 0, 0,	0 },
		"icons/icona_charges",	// icon
		NULL,					// ammo icon
		"Charges",				// pickup
		2,
		IT_AMMO,
		WP_CROSS,
		WP_CROSS,
		WP_CROSS,
		"",						// precache
		"",						// sounds
		{4,2,2,1}
	},
*/

//----(SA)	hopefully it doesn't need to be a quaked thing.
//			apologies if it does and I'll put it back.
/*
ammo_monster_attack1 (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
used by: Monster Attack 1 (specific to each monster)
*/
	{
		"ammo_monster_attack1",
		"",
		{ "",
		  0, 0, 0},
		"",                      // icon
		NULL,                   // ammo icon
		"MonsterAttack1",        // pickup
		60,
		IT_AMMO,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		WP_MONSTER_ATTACK1,
		"",
		"",
		{0,0,0,0}
	},


	//
	// HOLDABLE ITEMS
	//

//----(SA)	updated a number of powerup items (11/6/00)

/* holdable_medkit (.3 .3 1) (-16 -16 -16) (16 16 16) SUSPENDED SPIN - RESPAWN

pickup sound : "sound/pickup/holdable/get_medkit.wav"
use sound : "sound/pickup/holdable/get_medkit.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/medkit.md3"
*/
/*
	{
		"holdable_medkit",
		"sound/pickup/holdable/get_medkit.wav",
		{
		"models/powerups/holdable/medkit.md3",
		"models/powerups/holdable/medkit_sphere.md3",
		0, 0,	0 },
		"icons/medkit",	// icon
		NULL,			// ammo icon
		"Medkit",		// pickup
		1,
		IT_HOLDABLE,
		HI_MEDKIT,
		0,
		0,
		"",				// precache
		"sound/pickup/holdable/use_medkit.wav",	// sounds
		{0,0,0,0}
	},
*/

/*QUAKED holdable_wine (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN

pickup sound : "sound/pickup/holdable/get_wine.wav"
use sound : "sound/pickup/holdable/use_wine.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/wine.md3"
*/
	{
		"holdable_wine",
		"sound/pickup/holdable/get_wine.wav",
		{
			"models/powerups/holdable/wine.md3",
			0, 0, 0,    0
		},
		"icons/wine",                    // icon
		NULL,                           // ammo icon
		"1921 Chateau Lafite",           // pickup
		1,
		IT_HOLDABLE,
		HI_WINE,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_wine.wav",        // sounds
		{3,0,0,0}
	},


/* holdable_skull (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Skull of Invulnerability
Protection from all attacks

pickup sound : "sound/pickup/holdable/get_skull.wav"
use sound : "sound/pickup/holdable/use_skull.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/skull.md3"
*/
/*
	{
		"holdable_skull",
		"sound/pickup/holdable/get_skull.wav",
		{
			"models/powerups/holdable/skull.md3",
			0, 0, 0
		,	0 },
		"icons/skull",					// icon
		NULL,							// ammo icon
		"Skull of Invulnerability",		// pickup
		1,
		IT_HOLDABLE,
		HI_SKULL,
		0,
		0,
		"",								// precache
		"sound/pickup/holdable/use_skull.wav",	// sounds
		{0,0,0,0}
	},
*/


/* holdable_p_water (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from drowning for n seconds
"time" (in seconds)  How much extra underwater time is given

pickup sound : "sound/pickup/holdable/get_water.wav"
use sound : "sound/pickup/holdable/use_water.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/water.md3"
*/
/*
	{
		"holdable_p_water",
		"sound/pickup/holdable/get_water.wav",
		{
			"models/powerups/holdable/water.md3",
			0, 0, 0
		,	0 },
		"icons/water",					// icon
		NULL,							// ammo icon
		"Breather",					// pickup
		1,
		IT_HOLDABLE,
		HI_WATER,
		0,
		0,
		"",								// precache
		"sound/pickup/holdable/use_water.wav",	// sounds
		{0,0,0,0}
	},
*/

/* holdable_p_elec (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from electric attacks
Absorbs "dmg" points of electric damage

pickup sound : "sound/pickup/holdable/get_elec.wav"
use sound : "sound/pickup/holdable/use_elec.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/elec.md3"
*/
/*
	{
		"holdable_p_elec",
		"sound/pickup/holdable/get_elec.wav",
		{
			"models/powerups/holdable/elec.md3",
			0, 0, 0
		,	0 },
		"icons/elec",					// icon
		NULL,							// ammo icon
		"Electric Protection",			// pickup
		1,
		IT_HOLDABLE,
		HI_ELECTRIC,
		0,
		0,
		"",								// precache
		"sound/pickup/holdable/use_elec.wav",	// sounds
		{0,0,0,0}
	},
*/

/* holdable_p_fire (.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fire attacks
Absorbs "dmg" points of fire damage

pickup sound : "sound/pickup/holdable/get_fire.wav"
use sound : "sound/pickup/holdable/use_fire.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/fire.md3"
*/
/*
	{
		"holdable_p_fire",
		"sound/pickup/holdable/get_fire.wav",
		{
			"models/powerups/holdable/fire.md3",
			0, 0, 0
		,	0 },
		"icons/fire",					// icon
		NULL,							// ammo icon
		"Fire Protection",				// pickup
		1,
		IT_HOLDABLE,
		HI_FIRE,
		0,
		0,
		"",								// precache
		"sound/pickup/holdable/use_fire.wav",	// sounds
		{0,0,0,0}
	},
*/

/*QUAKED holdable_stamina(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Protection from fatigue
Using the "sprint" key will not fatigue the character

pickup sound : "sound/pickup/holdable/get_stamina.wav"
use sound : "sound/pickup/holdable/use_stamina.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/stamina.md3"
*/
	{
		"holdable_stamina",
		"sound/pickup/holdable/get_stamina.wav",
		{
			"models/powerups/holdable/stamina.md3",
			0, 0, 0
			,   0
		},
		"icons/stamina",             // icon
		NULL,                           // ammo icon
		"Added Stamina",             // pickup
		1,
		IT_HOLDABLE,
		HI_STAMINA,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_stamina.wav", // sounds
		{0,0,0,0}
	},



/*QUAKED holdable_book1(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/venom_book.md3"
*/
	{
		"holdable_book1",
		"sound/pickup/holdable/get_book1.wav",
		{
			"models/powerups/holdable/venom_book.md3",
			0, 0, 0
			,   0
		},
		"icons/icon_vbook",              // icon
		NULL,                       // ammo icon
		"Venom Tech Manual",     // pickup
		1,
		IT_HOLDABLE,
		HI_BOOK1,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_book.wav",    // sounds
		{0,0,0,0}
	},


/*QUAKED holdable_book2(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/paranormal_book.md3"
*/
	{
		"holdable_book2",
		"sound/pickup/holdable/get_book2.wav",
		{
			"models/powerups/holdable/paranormal_book.md3",
			0, 0, 0
			,   0
		},
		"icons/icon_pbook",              // icon
		NULL,                           // ammo icon
		"Project Book",                  // pickup
		1,
		IT_HOLDABLE,
		HI_BOOK2,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_book.wav",    // sounds
		{0,0,0,0}
	},


/*QUAKED holdable_book3(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/zemphr_book.md3"
*/
	{
		"holdable_book3",
		"sound/pickup/holdable/get_book3.wav",
		{
			"models/powerups/holdable/zemphr_book.md3",
			0, 0, 0
			,   0
		},
		"icons/icon_zbook",              // icon
		NULL,                       // ammo icon
		"Dr. Zemph's Journal",       // pickup
		1,
		IT_HOLDABLE,
		HI_BOOK3,
		0,
		0,
		"",                              // precache
		"sound/pickup/holdable/use_book.wav",    // sounds
		{0,0,0,0}
	},




/* holdable_11(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/11.md3"
*/
/*
	{
		"holdable_11",
		"sound/pickup/holdable/get_11.wav",
		{
			"models/powerups/holdable/11.md3",
			0, 0, 0
		,	0 },
		"icons/11",				// icon
		NULL,							// ammo icon
		"11",					// pickup
		1,
		IT_HOLDABLE,
		HI_11,
		0,
		0,
		"",								// precache
		"sound/pickup/holdable/use_11.wav",	// sounds
		{0,0,0,0}
	},
*/

/* holdable_12(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/12.md3"
*/
/*
	{
		"holdable_12",
		"sound/pickup/holdable/get_12.wav",
		{
			"models/powerups/holdable/12.md3",
			0, 0, 0
		,	0 },
		"icons/12",				// icon
		NULL,							// ammo icon
		"12",					// pickup
		1,
		IT_HOLDABLE,
		HI_12,
		0,
		0,
		"",								// precache
		"sound/pickup/holdable/use_12.wav",	// sounds
		{0,0,0,0}
	},
*/

/* holdable_13(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/13.md3"
*/
/*
	{
		"holdable_13",
		"sound/pickup/holdable/get_13.wav",
		{
			"models/powerups/holdable/13.md3",
			0, 0, 0
		,	0 },
		"icons/13",				// icon
		NULL,							// ammo icon
		"13",					// pickup
		1,
		IT_HOLDABLE,
		HI_13,
		0,
		0,
		"",								// precache
		"sound/pickup/holdable/use_13.wav",	// sounds
		{0,0,0,0}
	},
*/

/* holdable_14(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/holdable/14.md3"
*/
/*
	{
		"holdable_14",
		"sound/pickup/holdable/get_14.wav",
		{
			"models/powerups/holdable/14.md3",
			0, 0, 0
		,	0 },
		"icons/14",				// icon
		NULL,							// ammo icon
		"14",					// pickup
		1,
		IT_HOLDABLE,
		HI_14,
		0,
		0,
		"",								// precache
		"sound/pickup/holdable/use_14.wav",	// sounds
		{0,0,0,0}
	},
*/

/* holdable_15(.3 .3 1) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
/
	{
		"holdable_15",
		"sound/pickup/holdable/get_15.wav",
		{
			"models/powerups/holdable/15.md3",
			0, 0, 0
		,	0 },
		"icons/15",				// icon
		NULL,							// ammo icon
		"15",					// pickup
		1,
		IT_HOLDABLE,
		HI_15,
		0,
		"",								// precache
		"sound/pickup/holdable/use_15.wav",	// sounds
		{0,0,0,0}
	},

*/



	//
	// POWERUP ITEMS
	//

/*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
Only in CTF games
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/flags/r_flag.md3"
*/
	{
		"team_CTF_redflag",
		"sound/teamplay/flagtk_red.wav",
		{ "models/flags/r_flag.md3",
		  0, 0, 0,   0 },
		"icons/iconf_red",   // icon
		NULL,               // ammo icon
		"Red Flag",          // pickup
		0,
		IT_TEAM,
		PW_REDFLAG,
		0,
		0,
		"",                  // precache
		"sound/teamplay/flagcap_red.wav sound/teamplay/flagtk_red.wav sound/teamplay/flagret_red.wav",   // sounds
		{0,0,0,0}
	},

/*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
Only in CTF games
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/flags/b_flag.md3"
*/
	{
		"team_CTF_blueflag",
		"sound/teamplay/flagtk_blu.wav",
		{ "models/flags/b_flag.md3",
		  0, 0, 0,   0 },
		"icons/iconf_blu",   // icon
		NULL,               // ammo icon
		"Blue Flag",     // pickup
		0,
		IT_TEAM,
		PW_BLUEFLAG,
		0,
		0,
		"",                  // precache
		"sound/teamplay/flagcap_blu.wav sound/teamplay/flagtk_blu.wav sound/teamplay/flagret_blu.wav",   // sounds
		{0,0,0,0}
	},


	//---- (SA) Wolf keys

/* key_skull1 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 1

pickup sound : "sound/pickup/keys/skull.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/skull.md3"
*/
/*
	{
		"key_skull1",
		"sound/pickup/keys/skull.wav",
		{
			"models/powerups/keys/skull.md3",
			0, 0, 0
		,	0 },
		"icons/iconk_skull",	// icon
		NULL,					// ammo icon
		"Crystal Skull",		// pickup
		0,
		IT_KEY,
		KEY_1,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_chalice2 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 2

pickup sound : "sound/pickup/keys/chalice.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/chalice.md3"
*/
/*
	{
		"key_chalice2",
		"sound/pickup/keys/chalice.wav",
		{
			"models/powerups/keys/chalice.md3",
			0, 0, 0
		,	0 },
		"icons/iconk_chalice",	// icon
		NULL,					// ammo icon
		"Chalice",				// pickup
		0,
		IT_KEY,
		KEY_2,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_eye3 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 3

pickup sound : "sound/pickup/keys/eye.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/eye.md3"
*/
/*
	{
		"key_eye3",
		"sound/pickup/keys/eye.wav",
		{
			"models/powerups/keys/eye.md3",
			0, 0, 0
		,	0 },
		"icons/iconk_eye",		// icon
		NULL,					// ammo icon
		"Eye of Isis",			// pickup
		0,
		IT_KEY,
		KEY_3,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_radio4 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 4

pickup sound : "sound/pickup/keys/radio.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/radio_port.md3"
*/
/*
	{
		"key_radio4",
		"sound/pickup/keys/radio.wav",
		{
			"models/powerups/keys/radio_port.md3",
			0, 0, 0
		,	0 },
		"icons/iconk_radio",	// icon
		NULL,					// ammo icon
		"Field Radio",			// pickup
		0,
		IT_KEY,
		KEY_4,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_satchelcharge5 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 5

pickup sound : "sound/pickup/keys/satchelcharge.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/satchel_charge.md3"
*/
/*
	{
		"key_satchelcharge5",
		"sound/pickup/keys/satchelcharge.wav",
		{
			"models/powerups/keys/satchel_charge.md3",
			0, 0, 0
		,	0 },
		"icons/iconk_satchel",	// icon
		NULL,					// ammo icon
		"Satchel Charge",		// pickup
		0,
		IT_KEY,
		KEY_5,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/*QUAKED key_binocs (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
Binoculars.

pickup sound : "sound/pickup/keys/binocs.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/binoculars.md3"
*/
	{
		"key_binocs",
		"sound/pickup/keys/binocs.wav",
		{
			"models/powerups/keys/binoculars.md3",
			0, 0, 0
			,   0
		},
		"icons/binocs",          // icon
		NULL,                   // ammo icon
		"Binoculars",            // pickup
		0,
		IT_KEY,
		INV_BINOCS,
		0,
		0,
		"",                      // precache
		"models/keys/key.wav",   // sounds
		{0,0,0,0}
	},

/* key_7 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 7

pickup sound : "sound/pickup/keys/key7.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key7.md3"
*/
/*
	{
		"key_key7",
		"sound/pickup/keys/key7.wav",
		{
			"models/powerups/keys/key7.md3",
			0, 0, 0
		,	0 },
		"icons/key7",			// icon
		NULL,					// ammo icon
		"Key 7",				// pickup
		0,
		IT_KEY,
		KEY_7,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_key8 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 8

pickup sound : "sound/pickup/keys/key8.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key.md3"
*/
/*
	{
		"key_key8",
		"sound/pickup/keys/key8.wav",
		{
			"models/powerups/keys/key.md3",
			0, 0, 0
		,	0 },
		"icons/key8",			// icon
		NULL,					// ammo icon
		"Key 8",				// pickup
		0,
		IT_KEY,
		KEY_8,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_key9 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 9

pickup sound : "sound/pickup/keys/key9.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key.md3"
*/
/*
	{
		"key_key9",
		"sound/pickup/keys/key9.wav",
		{
			"models/powerups/keys/key.md3",
			0, 0, 0
		,	0 },
		"icons/key9",			// icon
		NULL,					// ammo icon
		"Key 9",				// pickup
		0,
		IT_KEY,
		KEY_9,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_key10 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 10

pickup sound : "sound/pickup/keys/key10.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key.md3"
*/
/*
	{
		"key_key10",
		"sound/pickup/keys/key10.wav",
		{
			"models/powerups/keys/key.md3",
			0, 0, 0
		,	0 },
		"icons/key10",			// icon
		NULL,					// ammo icon
		"Key 10",				// pickup
		0,
		IT_KEY,
		KEY_10,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_key11 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 11

pickup sound : "sound/pickup/keys/key11.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key.md3"
*/
/*
	{
		"key_key11",
		"sound/pickup/keys/key11.wav",
		{
			"models/powerups/keys/key.md3",
			0, 0, 0
		,	0 },
		"icons/key11",			// icon
		NULL,					// ammo icon
		"Key 11",				// pickup
		0,
		IT_KEY,
		KEY_11,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_key12 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 12

pickup sound : "sound/pickup/keys/key12.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key.md3"
*/
/*
	{
		"key_key12",
		"sound/pickup/keys/key12.wav",
		{
			"models/powerups/keys/key.md3",
			0, 0, 0
		,	0 },
		"icons/key12",			// icon
		NULL,					// ammo icon
		"Key 12",				// pickup
		0,
		IT_KEY,
		KEY_12,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_key13 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 13

pickup sound : "sound/pickup/keys/key13.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key.md3"
*/
/*
	{
		"key_key13",
		"sound/pickup/keys/key13.wav",
		{
			"models/powerups/keys/key.md3",
			0, 0, 0
		,	0 },
		"icons/key13",			// icon
		NULL,					// ammo icon
		"Key 13",				// pickup
		0,
		IT_KEY,
		KEY_13,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_key14 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 14

pickup sound : "sound/pickup/keys/key14.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key.md3"
*/
/*
	{
		"key_key14",
		"sound/pickup/keys/key14.wav",
		{
			"models/powerups/keys/key.md3",
			0, 0, 0
		,	0 },
		"icons/key14",			// icon
		NULL,					// ammo icon
		"Key 14",				// pickup
		0,
		IT_KEY,
		KEY_14,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_key15 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 15

pickup sound : "sound/pickup/keys/key15.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key.md3"
*/
/*
	{
		"key_key15",
		"sound/pickup/keys/key15.wav",
		{
			"models/powerups/keys/key.md3",
			0, 0, 0
		,	0 },
		"icons/key15",			// icon
		NULL,					// ammo icon
		"Key 15",				// pickup
		0,
		IT_KEY,
		KEY_15,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/

/* key_key16 (1 1 0) (-8 -8 -8) (8 8 8) SUSPENDED SPIN - RESPAWN
key 16

pickup sound : "sound/pickup/keys/key16.wav"
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models/powerups/keys/key.md3"
*/
/*
	{
		"key_key16",
		"sound/pickup/keys/key16.wav",
		{
			"models/powerups/keys/key.md3",
			0, 0, 0
		,	0 },
		"icons/key16",			// icon
		NULL,					// ammo icon
		"Key 16",				// pickup
		0,
		IT_KEY,
		KEY_16,
		0,
		0,
		"",						// precache
		"models/keys/key.wav",	// sounds
		{0,0,0,0}
	},
*/



	// end of list marker
	{NULL}
};
// END JOSEPH

int bg_numItems = sizeof( bg_itemlist ) / sizeof( bg_itemlist[0] ) - 1;


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t *BG_FindItemForPowerup( powerup_t pw ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( ( bg_itemlist[i].giType == IT_POWERUP ||
			   bg_itemlist[i].giType == IT_TEAM ) &&
			 bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t *BG_FindItemForHoldable( holdable_t pw ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

//	Com_Error( ERR_DROP, "HoldableItem not found" );

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t *BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t *it;
	int i;
	const int NUM_TABLE_ELEMENTS = WP_NUM_WEAPONS;
	static gitem_t  *lookupTable[WP_NUM_WEAPONS];
	static qboolean lookupTableInit = qtrue;

	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}

	if ( weapon < 0 || weapon > NUM_TABLE_ELEMENTS ) {
		Com_Error( ERR_DROP, "BG_FindItemForWeapon: weapon out of range %i", weapon );
	}

	if ( !lookupTable[weapon] ) {
		Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon );
	}

	// get the weapon from the lookup table
	return lookupTable[weapon];
}

//----(SA) added

#define DEATHMATCH_SHARED_AMMO 0


/*
==============
BG_FindClipForWeapon
==============
*/
weapon_t BG_FindClipForWeapon( weapon_t weapon ) {
	gitem_t *it;
	int i;
	const int NUM_TABLE_ELEMENTS = WP_NUM_WEAPONS;
	static weapon_t lookupTable[WP_NUM_WEAPONS];
	static qboolean lookupTableInit = qtrue;

	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it->giClipIndex;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}

	if ( weapon < 0 || weapon > NUM_TABLE_ELEMENTS ) {
		Com_Error( ERR_DROP, "BG_FindClipForWeapon: weapon out of range %i", weapon );
	}

	// get the weapon from the lookup table
	return lookupTable[weapon];
}



/*
==============
BG_FindAmmoForWeapon
==============
*/
weapon_t BG_FindAmmoForWeapon( weapon_t weapon ) {
	gitem_t *it;
	int i;
	const int NUM_TABLE_ELEMENTS = WP_NUM_WEAPONS;
	static weapon_t lookupTable[WP_NUM_WEAPONS];
	static qboolean lookupTableInit = qtrue;

	if ( lookupTableInit ) {
		for ( i = 0; i < NUM_TABLE_ELEMENTS; i++ ) {
			lookupTable[i] = 0; // default value for no match found
			for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
				if ( it->giType == IT_WEAPON && it->giTag == i ) {
					lookupTable[i] = it->giAmmoIndex;
				}
			}
		}
		// table is created
		lookupTableInit = qfalse;
	}

	if ( weapon < 0 || weapon > NUM_TABLE_ELEMENTS ) {
		Com_Error( ERR_DROP, "BG_FindAmmoForWeapon: weapon out of range %i", weapon );
	}

	// get the weapon from the lookup table
	return lookupTable[weapon];
}

/*
==============
BG_AkimboFireSequence
	returns 'true' if it's the left hand's turn to fire, 'false' if it's the right hand's turn
==============
*/
//qboolean BG_AkimboFireSequence( playerState_t *ps ) {
qboolean BG_AkimboFireSequence( int weapon, int akimboClip, int coltClip ) {
	// NOTE: this doesn't work when clips are turned off (dmflags 64)

	if ( weapon != WP_AKIMBO ) {
		return qfalse;
	}

	if ( !akimboClip ) {
		return qfalse;
	}

	// no ammo in colt, must be akimbo turn
	if ( !coltClip ) {
		return qtrue;
	}

	// at this point, both have ammo

	// now check 'cycle'   // (removed old method 11/5/2001)
	if ( ( akimboClip + coltClip ) & 1 ) {
		return qfalse;
	}

	return qtrue;
}

//----(SA) end

//----(SA) Added keys
/*
==============
BG_FindItemForKey
==============
*/
gitem_t *BG_FindItemForKey( wkey_t k, int *indexreturn ) {
	int i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_KEY && bg_itemlist[i].giTag == k ) {
			{
				if ( indexreturn ) {
					*indexreturn = i;
				}
				return &bg_itemlist[i];
			}
		}
	}

	Com_Error( ERR_DROP, "Key %d not found", k );
	return NULL;
}
//----(SA) end


//----(SA) added
/*
==============
BG_FindItemForAmmo
==============
*/
gitem_t *BG_FindItemForAmmo( int ammo ) {
	int i = 0;

	for (; i < bg_numItems; i++ )
	{
		if ( bg_itemlist[i].giType == IT_AMMO && bg_itemlist[i].giAmmoIndex == ammo ) {
			return &bg_itemlist[i];
		}
	}
	Com_Error( ERR_DROP, "Item not found for ammo: %d", ammo );
	return NULL;
}
//----(SA) end


/*
===============
BG_FindItem

===============
*/
gitem_t *BG_FindItem( const char *pickupName ) {
	gitem_t *it;

	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, pickupName ) ) {
			return it;
		}
	}

	return NULL;
}

/*
==============
BG_FindItem2
	also check classname
==============
*/
gitem_t *BG_FindItem2( const char *name ) {
	gitem_t *it;
	char *name2;

	name2 = (char*)name;

	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, name ) ) {
			return it;
		}

		if ( !Q_strcasecmp( it->classname, name2 ) ) {
			return it;
		}
	}

	Com_Printf( "BG_FindItem2(): unable to locate item '%s'\n", name );

	return NULL;
}

//----(SA)	added
/*
==============
BG_PlayerSeesItem
	Try to quickly determine if an item should be highlighted as per the current cg_drawCrosshairPickups.integer value.
	pvs check should have already been done by the time we get in here, so we shouldn't have to check
==============
*/

//----(SA)	not used
/*
qboolean BG_PlayerSeesItem(playerState_t *ps, entityState_t *item, int atTime)
{
   vec3_t	vorigin, eorigin, viewa, dir;
   float	dot, dist, foo;

   BG_EvaluateTrajectory( &item->pos, atTime, eorigin );

   VectorCopy(ps->origin, vorigin);
   vorigin[2] += ps->viewheight;			// get the view loc up to the viewheight
   eorigin[2] += 16;						// and subtract the item's offset (that is used to place it on the ground)
   VectorSubtract(vorigin, eorigin, dir);

   dist = VectorNormalize(dir);			// dir is now the direction from the item to the player

   if(dist > 255)
	   return qfalse;						// only run the remaining stuff on items that are close enough

   // (SA) FIXME: do this without AngleVectors.
   //		It'd be nice if the angle vectors for the player
   //		have already been figured at this point and I can
   //		just pick them up.  (if anybody is storing this somewhere,
   //		for the current frame please let me know so I don't
   //		have to do redundant calcs)
   AngleVectors(ps->viewangles, viewa, 0, 0);
   dot = DotProduct(viewa, dir );

   // give more range based on distance (the hit area is wider when closer)

   foo = -0.94f - (dist/255.0f) * 0.057f;	// (ranging from -0.94 to -0.997) (it happened to be a pretty good range)

//	Com_Printf("test: if(%f > %f) return qfalse (dot > foo)\n", dot, foo);
   if(dot > foo)
	   return qfalse;

   return qtrue;
}
*/
//----(SA)	end


/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/

extern int trap_Cvar_VariableIntegerValue( const char *var_name );

qboolean    BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		 || ps->origin[0] - origin[0] < -50
		 || ps->origin[1] - origin[1] > 36
		 || ps->origin[1] - origin[1] < -36
		 || ps->origin[2] - origin[2] > 36
		 || ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}



#define AMMOFORWEAP BG_FindAmmoForWeapon( item->giTag )
/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/

qboolean isClipOnly( int weap ) {
	switch ( weap ) {
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_TESLA:
	case WP_FLAMETHROWER:
		return qtrue;
	}
	return qfalse;
}


qboolean    BG_CanItemBeGrabbed( const entityState_t *ent, const playerState_t *ps ) {
	gitem_t *item;
	int ammoweap;
	qboolean multiplayer = qfalse;

	if ( ent->modelindex < 1 || ent->modelindex >= bg_numItems ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

//----(SA)	check for mp
#ifdef GAMEDLL
	if ( g_gametype.integer == GT_WOLF )
#endif
#ifdef CGAMEDLL
	if ( cg_gameType.integer == GT_WOLF )
#endif
	multiplayer = qtrue;
//----(SA)	end

	item = &bg_itemlist[ent->modelindex];

	switch ( item->giType ) {

	case IT_WEAPON:
		// JPW NERVE -- medics & engineers can only pick up same weapon type
		if ( multiplayer ) {
			if ( ( ps->stats[STAT_PLAYER_CLASS] == PC_MEDIC ) || ( ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER ) ) {
				if ( !COM_BitCheck( ps->weapons, item->giTag ) ) {
					return qfalse;
				}
			}
		} else {
			if ( COM_BitCheck( ps->weapons, item->giTag ) ) {               // you have the weap
				if ( isClipOnly( item->giTag ) ) {
					if ( ps->ammoclip[item->giAmmoIndex] >= ammoTable[item->giAmmoIndex].maxclip ) {
						return qfalse;
					}
				} else {
					if ( ps->ammo[item->giAmmoIndex] >= ammoTable[item->giAmmoIndex].maxammo ) { // you are loaded with the ammo
						return qfalse;
					}
				}
			}
		}
		// JPW
		return qtrue;

	case IT_AMMO:
		ammoweap = BG_FindAmmoForWeapon( item->giTag );

		if ( isClipOnly( ammoweap ) ) {
			if ( ps->ammoclip[ammoweap] >= ammoTable[ammoweap].maxclip ) {
				return qfalse;
			}
		}

		if ( ps->ammo[ammoweap] >= ammoTable[ammoweap].maxammo ) {
			return qfalse;
		}

		return qtrue;

	case IT_ARMOR:
		// we also clamp armor to the maxhealth for handicapping
//			if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
		if ( ps->stats[STAT_ARMOR] >= 100 ) {
			return qfalse;
		}
		return qtrue;

	case IT_HEALTH:
		if ( ent->density == ( 1 << 9 ) ) { // density tracks how many uses left
			return qfalse;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
		if ( ent->density == ( 1 << 9 ) ) { // density tracks how many uses left
			return qfalse;
		}

		if ( ps->powerups[PW_NOFATIGUE] == 60000 ) { // full
			return qfalse;
		}

		return qtrue;

	case IT_TEAM:     // team items, such as flags

		// DHM - Nerve :: otherEntity2 is now used instead of modelindex2
		// ent->modelindex2 is non-zero on items if they are dropped
		// we need to know this because we can pick up our dropped flag (and return it)
		// but we can't pick up our flag at base
		if ( ps->persistant[PERS_TEAM] == TEAM_RED ) {
			if ( item->giTag == PW_BLUEFLAG ||
				 ( item->giTag == PW_REDFLAG && ent->otherEntityNum2 /*ent->modelindex2*/ ) ||
				 ( item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG] ) ) {
				return qtrue;
			}
		} else if ( ps->persistant[PERS_TEAM] == TEAM_BLUE ) {
			if ( item->giTag == PW_REDFLAG ||
				 ( item->giTag == PW_BLUEFLAG && ent->otherEntityNum2 /*ent->modelindex2*/ ) ||
				 ( item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG] ) ) {
				return qtrue;
			}
		}
		return qfalse;


	case IT_HOLDABLE:
		return qtrue;

	case IT_TREASURE:       // treasure always picked up
		return qtrue;

	case IT_CLIPBOARD:      // clipboards always picked up
		return qtrue;

		//---- (SA) Wolf keys
	case IT_KEY:
		return qtrue;       // keys are always picked up

	case IT_BAD:
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );

	}

	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float deltaTime;
	float phase;
	vec3_t v;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
	case TR_GRAVITY_PAUSED: //----(SA)
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
//----(SA)	removed
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;     // FIXME: local gravity...
		break;
		// Ridah
	case TR_GRAVITY_LOW:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * ( DEFAULT_GRAVITY * 0.3 ) * deltaTime * deltaTime;     // FIXME: local gravity...
		break;
		// done.
//----(SA)
	case TR_GRAVITY_FLOAT:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * ( DEFAULT_GRAVITY * 0.2 ) * deltaTime;
		break;
//----(SA)	end
		// RF, acceleration
	case TR_ACCELERATE:     // trDelta is the ultimate speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		// phase is the acceleration constant
		phase = VectorLength( tr->trDelta ) / ( tr->trDuration * 0.001 );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time
		VectorMA( tr->trBase, phase * 0.5 * deltaTime * deltaTime, result, result );
		break;
	case TR_DECCELERATE:    // trDelta is the starting speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		// phase is the breaking constant
		phase = VectorLength( tr->trDelta ) / ( tr->trDuration * 0.001 );
		// trDelta at least gives us the acceleration direction
		VectorNormalize2( tr->trDelta, result );
		// get distance travelled at current time (without breaking)
		VectorMA( tr->trBase, deltaTime, tr->trDelta, v );
		// subtract breaking force
		VectorMA( v, -phase * 0.5 * deltaTime * deltaTime, result, result );
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float deltaTime;
	float phase;

	switch ( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );    // derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
//----(SA)	removed
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;       // FIXME: local gravity...
		break;
		// Ridah
	case TR_GRAVITY_LOW:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= ( DEFAULT_GRAVITY * 0.3 ) * deltaTime;       // FIXME: local gravity...
		break;
		// done.
//----(SA)
	case TR_GRAVITY_FLOAT:
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= ( DEFAULT_GRAVITY * 0.2 ) * deltaTime;
		break;
//----(SA)	end
		// RF, acceleration
	case TR_ACCELERATE: // trDelta is eventual speed
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		phase = deltaTime / (float)tr->trDuration;
		VectorScale( tr->trDelta, deltaTime * deltaTime, result );
		break;
	case TR_DECCELERATE:    // trDelta is breaking force
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;    // milliseconds to seconds
		VectorScale( tr->trDelta, deltaTime, result );
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
============
BG_GetMarkDir

  used to find a good directional vector for a mark projection, which will be more likely
  to wrap around adjacent surfaces

  dir is the direction of the projectile or trace that has resulted in a surface being hit
============
*/
void BG_GetMarkDir( const vec3_t dir, const vec3_t normal, vec3_t out ) {
	vec3_t ndir, lnormal;
	float minDot = 0.3;

	if ( VectorLength( normal ) < 1.0 ) {
		VectorSet( lnormal, 0, 0, 1 );
	} else {
		VectorCopy( normal, lnormal );
	}

	VectorNegate( dir, ndir );
	VectorNormalize( ndir );
	if ( normal[2] > 0.8 ) {
		minDot = 0.7;
	}
	// make sure it makrs the impact surface
	while ( DotProduct( ndir, lnormal ) < minDot ) {
		VectorMA( ndir, 0.5, lnormal, ndir );
		VectorNormalize( ndir );
	}

	VectorCopy( ndir, out );
}


char *eventnames[] = {
	"EV_NONE",
	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSTEP_WOOD",
	"EV_FOOTSTEP_GRASS",
	"EV_FOOTSTEP_GRAVEL",
	"EV_FOOTSTEP_ROOF",
	"EV_FOOTSTEP_SNOW",
	"EV_FOOTSTEP_CARPET",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",
	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",
	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",
	"EV_FALL_NDIE",
	"EV_FALL_DMG_10",
	"EV_FALL_DMG_15",
	"EV_FALL_DMG_25",
	"EV_FALL_DMG_50",
	"EV_JUMP_PAD",           // boing sound at origin, jump sound on player
	"EV_JUMP",
	"EV_WATER_TOUCH",    // foot touches
	"EV_WATER_LEAVE",    // foot leaves
	"EV_WATER_UNDER",    // head touches
	"EV_WATER_CLEAR",    // head leaves
	"EV_ITEM_PICKUP",            // normal item pickups are predictable
	"EV_ITEM_PICKUP_QUIET",  // (SA) same, but don't play the default pickup sound as it was specified in the ent
	"EV_GLOBAL_ITEM_PICKUP", // powerup / team sounds are broadcast to everyone
	"EV_NOITEM",
	"EV_NOAMMO",
	"EV_EMPTYCLIP",
	"EV_FILL_CLIP",
	"EV_WEAP_OVERHEAT",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_FIRE_WEAPONB",
	"EV_FIRE_WEAPON_LASTSHOT",
	"EV_FIRE_QUICKGREN", // "Quickgrenade"
	"EV_NOFIRE_UNDERWATER",
	"EV_FIRE_WEAPON_MG42",
	"EV_SUGGESTWEAP",        //----(SA)	added
	"EV_GRENADE_SUICIDE",    //----(SA)	added
	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",
	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",
	"EV_GRENADE_BOUNCE",     // eventParm will be the soundindex
	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",       // no attenuation
	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",
	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_RAILTRAIL",
	"EV_VENOM",
	"EV_VENOMFULL",
	"EV_BULLET",             // otherEntity is the shooter
	"EV_LOSE_HAT",
	"EV_GIB_HEAD",           // only blow off the head
	"EV_PAIN",
	"EV_CROUCH_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_ENTDEATH",           //----(SA)	added
	"EV_OBITUARY",
	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",
	"EV_GIB_PLAYER",         // gib a previously living player
	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_STOPSTREAMINGSOUND", //----(SA)	added
	"EV_TAUNT",
	"EV_SMOKE",
	"EV_SPARKS",
	"EV_SPARKS_ELECTRIC",
	"EV_BATS",
	"EV_BATS_UPDATEPOSITION",
	"EV_BATS_DEATH",
	"EV_EXPLODE",        // func_explosive
	"EV_EFFECT",     // target_effect
	"EV_MORTAREFX",  // mortar firing
	"EV_SPINUP", // JPW NERVE panzerfaust preamble for MP balance
	"EV_SNOW_ON",
	"EV_SNOW_OFF",
	"EV_MISSILE_MISS_SMALL",
	"EV_MISSILE_MISS_LARGE",
	"EV_WOLFKICK_HIT_FLESH",
	"EV_WOLFKICK_HIT_WALL",
	"EV_WOLFKICK_MISS",
	"EV_SPIT_HIT",
	"EV_SPIT_MISS",
	"EV_SHARD",
	"EV_JUNK",
	"EV_EMITTER",    //----(SA)	added
	"EV_OILPARTICLES",
	"EV_OILSLICK",
	"EV_OILSLICKREMOVE",
	"EV_MG42EFX",
	"EV_FLAMEBARREL_BOUNCE",
	"EV_FLAKGUN1",
	"EV_FLAKGUN2",
	"EV_FLAKGUN3",
	"EV_FLAKGUN4",
	"EV_EXERT1",
	"EV_EXERT2",
	"EV_EXERT3",
	"EV_SNOWFLURRY",
	"EV_CONCUSSIVE",
	"EV_DUST",
	"EV_RUMBLE_EFX",
	"EV_GUNSPARKS",
	"EV_FLAMETHROWER_EFFECT",
	"EV_SNIPER_SOUND",
	"EV_POPUP",
	"EV_POPUPBOOK",
	"EV_GIVEPAGE",
	"EV_CLOSEMENU",

	"EV_MAX_EVENTS"
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void    trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer( "showevents", buf, sizeof( buf ) );
		if ( atof( buf ) != 0 ) {
#ifdef QAGAME
			Com_Printf( " game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm );
#else
			Com_Printf( "Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm );
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & ( MAX_EVENTS - 1 )] = newEvent;
	ps->eventParms[ps->eventSequence & ( MAX_EVENTS - 1 )] = eventParm;
	ps->eventSequence++;
}


/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_flags & PMF_LIMBO ) { // JPW NERVE limbo
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	if ( ps->movementDir > 128 ) {
		s->angles2[YAW] = (float)ps->movementDir - 256;
	} else {
		s->angles2[YAW] = ps->movementDir;
	}

	s->legsAnim     = ps->legsAnim;
	s->torsoAnim    = ps->torsoAnim;
	s->clientNum    = ps->clientNum;    // ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	// Ridah, let clients know if this person is using a mounted weapon
	// so they don't show any client muzzle flashes

	// (SA) moved up since it needs to set the ps->eFlags too.
	//		Seems like this could be the problem Raf was
	//		encountering with the EF_DEAD flag below when guys
	//		dead flags weren't sticking

	if ( ps->persistant[PERS_HWEAPON_USE] ) {
		ps->eFlags |= EF_MG42_ACTIVE;
	} else {
		ps->eFlags &= ~EF_MG42_ACTIVE;
	}

	s->eFlags = ps->eFlags;

	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

// from MP
	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}
		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}
// end
	// Ridah, now using a circular list of events for all entities
	// add any new events that have been added to the playerState_t
	// (possibly overwriting entityState_t events)
	for ( i = ps->oldEventSequence; i != ps->eventSequence; i++ ) {
		s->events[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->events[i & ( MAX_EVENTS - 1 )];
		s->eventParms[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->eventParms[i & ( MAX_EVENTS - 1 )];
		s->eventSequence++;
	}
	ps->oldEventSequence = ps->eventSequence;

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->aiChar = ps->aiChar; // Ridah
//	s->loopSound = ps->loopSound;
	s->teamNum = ps->teamNum;
	s->aiState = ps->aiState;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_flags & PMF_LIMBO ) { // JPW NERVE limbo
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;       // ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS ) {
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}
		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	// Ridah, now using a circular list of events for all entities
	// add any new events that have been added to the playerState_t
	// (possibly overwriting entityState_t events)
	if ( ps->oldEventSequence > ps->eventSequence ) {
		ps->oldEventSequence = ps->eventSequence;
	}
	for ( i = ps->oldEventSequence; i != ps->eventSequence; i++ ) {
		s->events[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->events[i & ( MAX_EVENTS - 1 )];
		s->eventParms[s->eventSequence & ( MAX_EVENTS - 1 )] = ps->eventParms[i & ( MAX_EVENTS - 1 )];
		s->eventSequence++;
	}
	ps->oldEventSequence = ps->eventSequence;

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

//	s->loopSound = ps->loopSound;
//	s->generic1 = ps->generic1;
	s->aiChar = ps->aiChar; // Ridah
	s->teamNum = ps->teamNum;
	s->aiState = ps->aiState;
}
