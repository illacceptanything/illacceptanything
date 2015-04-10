/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

//===========================================================================
//
// Name:			ai_cast_characters.c
// Function:		Wolfenstein AI Characters
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

#include "g_local.h"
#include "../game/botlib.h"      //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../botai/botai.h"          //bot ai interface

#include "ai_cast.h"

//---------------------------------------------------------------------------
// Character specific attributes (defaults, these can be altered in the editor (TODO!))
AICharacterDefaults_t aiDefaults[NUM_CHARACTERS] = {
	//AICHAR_NONE
	{0},
	//AICHAR_SOLDIER
	{
		"Soldier",
		{
			220,        // running speed
			90,         // walking speed
			80,         // crouching speed
			90,         // Field of View
			200,        // Yaw Speed	// RF change
			0.0,        // leader
			0.5,        // aim skill
			0.5,        // aim accuracy
			0.75,       // attack skill
			0.5,        // reaction time
			0.4,        // attack crouch
			0.0,        // idle crouch
			0.5,        // aggression
			0.8,        // tactical
			0.0,        // camper
			16000,      // alertness
			100,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"infantrySightPlayer",
		"infantryAttackPlayer",
		"infantryOrders",
		"infantryDeath",
		"infantrySilentDeath",   //----(SA)	added
		"infantryPain",
		"infantryStay",          // stay - you're told to stay put
		"infantryFollow",        // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"infantryOrdersDeny",    // deny - refuse orders (doing something else)
		AITEAM_NAZI,                        // team
		"infantryss/default",                    // default model/skin
		{WP_MP40,WP_GRENADE_LAUNCHER},      // starting weapons
		BBOX_SMALL, {32,48},                // bbox, crouch/stand height
		AIFL_CATCH_GRENADE | AIFL_STAND_IDLE2, // flags
		NULL, NULL, NULL,                   // special attack routine
		NULL,                               // looping sound
		AISTATE_RELAXED
	},
	//AICHAR_AMERICAN
	{
		"American",
		{
			220,        // running speed
			90,         // walking speed
			80,         // crouching speed
			90,         // Field of View
			200,        // Yaw Speed	// RF change
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.5,        // reaction time
			0.3,        // attack crouch
			0.0,        // idle crouch
			0.5,        // aggression
			0.8,        // tactical
			0.0,        // camper
			16000,      // alertness
			100,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"americanSightPlayer",
		"americanAttackPlayer",
		"americanOrders",
		"americanDeath",
		"americanDeath",     //----(SA)	added
		"americanPain",
		"americanStay",          // stay - you're told to stay put
		"americanFollow",        // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"americanOrdersDeny",    // deny - refuse orders (doing something else)
		AITEAM_ALLIES,
		"american/default",
		{WP_THOMPSON,WP_GRENADE_PINEAPPLE},
		BBOX_SMALL, {32,48},
		AIFL_CATCH_GRENADE | AIFL_STAND_IDLE2,
		NULL, NULL, NULL,
		NULL,
		AISTATE_RELAXED
	},
	//AICHAR_ZOMBIE
	{
		"Zombie",
		{
			200,        // running speed		//----(SA)	DK requested change
			60,         // walking speed		//----(SA)	DK requested change
			80,         // crouching speed
			90,         // Field of View
			350,        // Yaw Speed
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.1,        // reaction time
			0.0,        // attack crouch
			0.0,        // idle crouch
			1.0,        // aggression
			0.0,        // tactical
			0.0,        // camper
			16000,      // alertness
			180,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"zombieSightPlayer",
		"zombieAttackPlayer",
		"zombieOrders",
		"zombieDeath",
		"zombieDeath",       //----(SA)	added
		"zombiePain",
		"zombieStay",        // stay - you're told to stay put
		"zombieFollow",      // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"zombieOrdersDeny",  // deny - refuse orders (doing something else)
		AITEAM_MONSTER,
		"zombie/default",
		{WP_GAUNTLET,WP_MONSTER_ATTACK2},
		BBOX_SMALL, {32,48},
		/*AIFL_NOPAIN|*/ AIFL_WALKFORWARD | AIFL_NO_RELOAD,
		AIFunc_ZombieFlameAttackStart, AIFunc_ZombieAttack2Start, NULL,
		NULL,
		AISTATE_ALERT
	},

//----(SA)	added
	//AICHAR_WARZOMBIE
	{
		"WarriorZombie",
		{
			250,        // running speed	(SA) upped from 200->250 per Mike/DK
			60,         // walking speed
			80,         // crouching speed
			90,         // Field of View
			350,        // Yaw Speed
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.1,        // reaction time
			0.0,        // attack crouch
			0.0,        // idle crouch
			1.0,        // aggression
			0.0,        // tactical
			0.0,        // camper
			16000,      // alertness
			180,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"warzombieSightPlayer",
		"warzombieAttackPlayer",
		"warzombieOrders",
		"warzombieDeath",
		"warzombieDeath",        //----(SA)	added
		"warzombiePain",
		"sound/weapons/melee/fstatck.wav",       // stay - you're told to stay put
		"warzombieFollow",       // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"warzombieOrdersDeny",   // deny - refuse orders (doing something else)
		AITEAM_MONSTER,
		"warrior/default",
		{WP_MONSTER_ATTACK1,WP_MONSTER_ATTACK2,WP_MONSTER_ATTACK3},
		BBOX_SMALL, {10,48},    // very low defense position
		AIFL_NO_RELOAD,
		AIFunc_WarriorZombieMeleeStart, /*AIFunc_WarriorZombieSightStart*/ NULL, AIFunc_WarriorZombieDefenseStart,
		NULL,
		AISTATE_ALERT
	},
//----(SA)	end

	//AICHAR_FEMZOMBIE
	{
		"FemZombie",
		{
			90,         // running speed
			30,         // walking speed
			80,         // crouching speed
			90,         // Field of View
			200,        // Yaw Speed	// RF change
			0.0,        // leader
			0.5,        // aim skill
			0.5,        // aim accuracy
			0.75,       // attack skill
			0.8,        // reaction time
			0.0,        // attack crouch
			0.0,        // idle crouch
			1.0,        // aggression
			0.8,        // tactical
			0.0,        // camper
			16000,      // alertness
			180,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"zombieFemSightPlayer",
		"zombieFemAttackPlayer",
		"zombieFemOrders",
		"zombieFemDeath",
		"zombieFemDeath",        //----(SA)	added
		"zombieFemPain",
		"zombieFemStay",     // stay - you're told to stay put
		"zombieFemFollow",       // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"zombieFemOrdersDeny",   // deny - refuse orders (doing something else)
		AITEAM_MONSTER,                     // team
		"femzombie/default",                 // default model/skin
		{WP_GAUNTLET,WP_MONSTER_ATTACK2},       // starting weapons
		BBOX_SMALL, {32,48},                    // bbox, crouch/stand height
		AIFL_NO_FLAME_DAMAGE | AIFL_STAND_IDLE2 | AIFL_WALKFORWARD | AIFL_NO_RELOAD,      // flags
		AIFunc_FZombie_LightningAttackStart, AIFunc_FZombie_HandLightningAttackStart, NULL,                     // special attack routine
		NULL,
		AISTATE_ALERT
	},


//----(SA)	end

	//AICHAR_UNDEAD
	{
		"Undead",
		{
			70,         // running speed
			70,         // walking speed
			80,         // crouching speed
			90,         // Field of View
			100,        // Yaw Speed
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.8,        // reaction time
			0.0,        // attack crouch
			0.0,        // idle crouch
			1.0,        // aggression
			0.0,        // tactical
			0.0,        // camper
			16000,      // alertness
			100,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"undeadSightPlayer",
		"undeadAttackPlayer",
		"undeadOrders",
		"undeadDeath",
		"undeadDeath",       //----(SA)	added
		"undeadPain",
		"undeadStay",        // stay - you're told to stay put
		"undeadFollow",      // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"undeadOrdersDeny",  // deny - refuse orders (doing something else)
		AITEAM_MONSTER,
		"undead/default",
		{WP_GAUNTLET},
		BBOX_SMALL, {32,48},
		AIFL_WALKFORWARD | AIFL_NO_RELOAD,
		NULL, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_VENOM
	{
		"Venom",
		{
			110,        // running speed
			100,        // walking speed
			80,         // crouching speed
			90,         // Field of View
			200,        // Yaw Speed
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.5,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			0.9,        // aggression
			0.2,        // tactical
			0.0,        // camper
			16000,      // alertness
			240,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"venomSightPlayer",
		"venomAttackPlayer",
		"venomOrders",
		"venomDeath",
		"venomDeath",        //----(SA)	added
		"venomPain",
		"venomStay",     // stay - you're told to stay put
		"venomFollow",       // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"venomOrdersDeny",   // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"venom/default",
		{WP_VENOM,WP_VENOM_FULL,WP_FLAMETHROWER},
		BBOX_SMALL, {32,48},
		AIFL_NO_FLAME_DAMAGE | AIFL_WALKFORWARD | AIFL_NO_RELOAD | AIFL_NO_HEADSHOT_DMG,
		NULL, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_LOPER
	{
		"Loper",
		{
			220,        // running speed
			70,         // walking speed
			220,        // crouching speed
			90,         // Field of View
			200,        // Yaw Speed
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.8,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			1.0,        // aggression
			0.0,        // tactical
			0.0,        // camper
			16000,      // alertness
			500,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"loperSightPlayer",
		"loperAttackPlayer",
		"loperOrders",
		"loperDeath",
		"loperDeath",        //----(SA)	added
		"loperPain",
		"loperStay",     // stay - you're told to stay put
		"loperFollow",       // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"loperOrdersDeny",   // deny - refuse orders (doing something else)
		AITEAM_MONSTER,
		"loper/default",
		{WP_MONSTER_ATTACK1,WP_MONSTER_ATTACK2,WP_MONSTER_ATTACK3},
		BBOX_LARGE, {48,48},        // large is for wide characters
		AIFL_NO_RELOAD,
		AIFunc_LoperAttack1Start, AIFunc_LoperAttack2Start, AIFunc_LoperAttack3Start,
		"sound/world/electloop.wav",
		AISTATE_ALERT
	},
	//AICHAR_SEALOPER

	//----(SA)	no 'correct' values/weapons/etc.  have been set.  only adding character basics
	{
		"SeaLoper",
		{
			220,        // running speed
			70,         // walking speed
			220,        // crouching speed
			90,         // Field of View
			200,        // Yaw Speed
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.8,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			1.0,        // aggression
			0.0,        // tactical
			0.0,        // camper
			16000,      // alertness
			500,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"sealoperSightPlayer",
		"sealoperAttackPlayer",
		"sealoperOrders",
		"sealoperDeath",
		"sealoperDeath",     //----(SA)	added
		"sealoperPain",
		"sealoperStay",          // stay - you're told to stay put
		"sealoperFollow",        // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"sealoperOrdersDeny",    // deny - refuse orders (doing something else)
		AITEAM_MONSTER,
		"sealoper/default",

		{WP_MONSTER_ATTACK1,WP_MONSTER_ATTACK2,WP_MONSTER_ATTACK3},
		BBOX_LARGE, {48,48},        // large is for wide characters
		AIFL_NO_RELOAD,
		AIFunc_LoperAttack1Start, AIFunc_LoperAttack2Start, AIFunc_LoperAttack3Start,
		"sound/world/electloop.wav",
		AISTATE_ALERT

	},

	//AICHAR_ELITEGUARD
	{
		"Elite Guard",
		{
			230,        // running speed
			90,         // walking speed
			100,        // crouching speed
			90,         // Field of View
			200,        // Yaw Speed	// RF change
			0.0,        // leader
			0.5,        // aim skill
			1.0,        // aim accuracy
			0.9,        // attack skill
			0.3,        // reaction time
			0.4,        // attack crouch
			0.0,        // idle crouch
			0.5,        // aggression
			1.0,        // tactical
			0.0,        // camper
			16000,      // alertness
			120,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"eliteGuardSightPlayer",
		"eliteGuardAttackPlayer",
		"eliteGuardOrders",
		"eliteGuardDeath",
		"eliteGuardDeath",       //----(SA)	added
		"eliteGuardPain",
		"eliteGuardStay",        // stay - you're told to stay put
		"eliteGuardFollow",      // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"eliteGuardOrdersDeny",  // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"eliteguard/default",
		{WP_SILENCER},      //----(SA)	TODO: replace w/ "silenced luger"
		BBOX_SMALL, {32,48},
		AIFL_CATCH_GRENADE | AIFL_STAND_IDLE2,
		NULL, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},

	//AICHAR_STIMSOLDIER1
	{
		"Stim Soldier",
		{
			170,        // running speed
			100,        // walking speed
			90,         // crouching speed
			90,         // Field of View
			150,        // Yaw Speed
			0.0,        // leader
			0.7,        // aim skill
			1.0,        // aim accuracy
			0.9,        // attack skill
			0.6,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			0.9,        // aggression
			0.1,        // tactical
			0.0,        // camper
			16000,      // alertness
			300,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"stimSoldierSightPlayer",
		"stimSoldierAttackPlayer",
		"stimSoldierOrders",
		"stimSoldierDeath",
		"stimSoldierDeath",      //----(SA)	added
		"stimSoldierPain",
		"stimSoldierStay",           // stay - you're told to stay put
		"stimSoldierFollow",     // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"stimSoldierOrdersDeny", // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"stim/default",
		{WP_MONSTER_ATTACK2},   // TODO: dual machinegun attack
		BBOX_LARGE, {48,64},
		AIFL_NO_RELOAD,
		NULL, AIFunc_StimSoldierAttack2Start, NULL,
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_STIMSOLDIER2
	{
		"Stim Soldier",
		{
			170,        // running speed
			100,        // walking speed
			90,         // crouching speed
			90,         // Field of View
			150,        // Yaw Speed
			0.0,        // leader
			0.7,        // aim skill
			1.0,        // aim accuracy
			0.9,        // attack skill
			0.6,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			0.9,        // aggression
			0.1,        // tactical
			0.0,        // camper
			16000,      // alertness
			300,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"stimSoldierSightPlayer",
		"stimSoldierAttackPlayer",
		"stimSoldierOrders",
		"stimSoldierDeath",
		"stimSoldierDeath",      //----(SA)	added
		"stimSoldierPain",
		"stimSoldierStay",           // stay - you're told to stay put
		"stimSoldierFollow",     // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"stimSoldierOrdersDeny", // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"stim/default",
		{WP_MP40, WP_ROCKET_LAUNCHER, WP_MONSTER_ATTACK1},  // attack1 is leaping rocket attack
		BBOX_LARGE, {48,64},
		AIFL_NO_RELOAD,
		AIFunc_StimSoldierAttack1Start, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_STIMSOLDIER3
	{
		"Stim Soldier",
		{
			170,        // running speed
			100,        // walking speed
			90,         // crouching speed
			90,         // Field of View
			150,        // Yaw Speed
			0.0,        // leader
			0.7,        // aim skill
			1.0,        // aim accuracy
			0.9,        // attack skill
			0.6,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			0.9,        // aggression
			0.1,        // tactical
			0.0,        // camper
			16000,      // alertness
			300,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"stimSoldierSightPlayer",
		"stimSoldierAttackPlayer",
		"stimSoldierOrders",
		"stimSoldierDeath",
		"stimSoldierDeath",      //----(SA)	added
		"stimSoldierPain",
		"stimSoldierStay",           // stay - you're told to stay put
		"stimSoldierFollow",     // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"stimSoldierOrdersDeny", // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"stim/default",
		{WP_MP40, WP_TESLA},    // no monster_attack1, since that's only used for the jumping rocket attack
		BBOX_LARGE, {48,64},
		AIFL_NO_RELOAD,
		AIFunc_StimSoldierAttack1Start, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_SUPERSOLDIER
	{
		"Super Soldier",
		{
			170,        // running speed
			100,        // walking speed
			90,         // crouching speed
			90,         // Field of View
			150,        // Yaw Speed
			0.0,        // leader
			0.7,        // aim skill
			1.0,        // aim accuracy
			0.9,        // attack skill
			0.6,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			0.9,        // aggression
			0.1,        // tactical
			0.0,        // camper
			16000,      // alertness
			300,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			2.0,        // pain threshold multiplier
		},
		"superSoldierSightPlayer",
		"superSoldierAttackPlayer",
		"superSoldierOrders",
		"superSoldierDeath",
		"superSoldierDeath",     //----(SA)	added
		"superSoldierPain",
		"superSoldierStay",          // stay - you're told to stay put
		"superSoldierFollow",        // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"superSoldierOrdersDeny",    // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"supersoldier/default",
		{WP_VENOM},
		BBOX_LARGE, {48,64},
		AIFL_NO_RELOAD | AIFL_NO_FLAME_DAMAGE | AIFL_NO_TESLA_DAMAGE,
		NULL, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_BLACKGUARD
	{
		"Black Guard",
		{
			220,        // running speed
			90,         // walking speed
			100,        // crouching speed
			90,         // Field of View
			300,        // Yaw Speed
			0.0,        // leader
			0.5,        // aim skill
			0.8,        // aim accuracy
			0.9,        // attack skill
			0.3,        // reaction time
			0.4,        // attack crouch
			0.0,        // idle crouch
			0.5,        // aggression
			1.0,        // tactical
			0.0,        // camper
			16000,      // alertness
			120,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"blackGuardSightPlayer",
		"blackGuardAttackPlayer",
		"blackGuardOrders",
		"blackGuardDeath",
		"blackGuardDeath",       //----(SA)	added
		"blackGuardPain",
		"blackGuardStay",        // stay - you're told to stay put
		"blackGuardFollow",      // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"blackGuardOrdersDeny",  // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"blackguard/default",
//		{WP_MP40, WP_GRENADE_LAUNCHER, WP_MONSTER_ATTACK1},	// attack1 is melee kick
		{WP_FG42, WP_FG42SCOPE, WP_GRENADE_LAUNCHER, WP_MONSTER_ATTACK1},   // attack1 is melee kick
		BBOX_SMALL, {32,48},
		AIFL_CATCH_GRENADE | AIFL_FLIP_ANIM | AIFL_STAND_IDLE2,
		AIFunc_BlackGuardAttack1Start, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_PROTOSOLDIER
	{
		"Protosoldier",
		{
			170,        // running speed
			100,        // walking speed
			90,         // crouching speed
			90,         // Field of View
			230,        // Yaw Speed
			0.0,        // leader
			0.7,        // aim skill
			1.0,        // aim accuracy
			0.9,        // attack skill
			0.2,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			0.9,        // aggression
			0.1,        // tactical
			0.0,        // camper
			16000,      // alertness
			300,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			2.0,        // pain threshold multiplier
		},
		"protoSoldierSightPlayer",
		"protoSoldierAttackPlayer",
		"protoSoldierOrders",
		"protoSoldierDeath",
		"protoSoldierDeath",     //----(SA)	added
		"protoSoldierPain",
		"protoSoldierStay",          // stay - you're told to stay put
		"protoSoldierFollow",        // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"protoSoldierOrdersDeny",    // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"protosoldier/default",
//		{WP_TESLA},
		{WP_VENOM_FULL},
		BBOX_LARGE, {48,64},
		AIFL_NO_TESLA_DAMAGE | AIFL_NO_FLAME_DAMAGE | AIFL_WALKFORWARD | AIFL_NO_RELOAD,
		NULL, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_REJECTX
	{
		"Reject X Creature",
		{
			// (SA) trying to set basics up per DM's requests
			300,        // running speed	// 220 * 1.35 (DM number)
			150,        // walking speed
			100,        // crouching speed
			90,         // Field of View
			150,        // Yaw Speed
			0.0,        // leader
			0.7,        // aim skill
			1.0,        // aim accuracy
			0.9,        // attack skill
			0.6,        // reaction time
			0.0,        // attack crouch
			0.2,        // idle crouch
			0.6,        // aggression
			0.02,       // tactical
			0.0,        // camper
			16000,      // alertness
			500,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"rejectXSightPlayer",
		"rejectXAttackPlayer",
		"rejectXOrders",
		"rejectXDeath",
		"rejectXDeath",      //----(SA)	added
		"rejectXPain",
		"rejectXStay",
		"rejectXFollow",
		"rejectXOrdersDeny",
		AITEAM_NAZI,
		"rejectx/default",
		{WP_MONSTER_ATTACK1},   // ,WP_FLAMETHROWER},
		BBOX_SMALL, {32, 48},
		AIFL_STAND_IDLE2 | AIFL_NO_RELOAD,
		AIFunc_RejectAttack1Start, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
// AICHAR_FROGMAN
	{
		"Frogman",
		{
			170,        // running speed
			100,        // walking speed
			90,         // crouching speed
			90,         // Field of View
			150,        // Yaw Speed
			0.0,        // leader
			0.7,        // aim skill
			1.0,        // aim accuracy
			0.9,        // attack skill
			0.6,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			0.9,        // aggression
			0.1,        // tactical
			0.0,        // camper
			16000,      // alertness
			200,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"frogmanSightPlayer",
		"frogmanAttackPlayer",
		"frogmanOrders",
		"frogmanDeath",
		"frogmanDeath",      //----(SA)	added
		"frogmanPain",
		"frogmanStay",           // stay - you're told to stay put
		"frogmanFollow",     // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"frogmanOrdersDeny", // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"frogman/default",
		{WP_SPEARGUN},
		BBOX_SMALL, {32,48},    // bbox, crouch/stand height
		0,
		NULL, NULL, NULL,
		NULL,
		AISTATE_RELAXED
	},
	//AICHAR_HELGA
	{
		"Helga",
		{
			140,        // running speed
			90,         // walking speed
			80,         // crouching speed
			90,         // Field of View
			300,        // Yaw Speed
			0.0,        // leader
			0.5,        // aim skill
			0.5,        // aim accuracy
			0.75,       // attack skill
			0.5,        // reaction time
			0.0,        // attack crouch
			0.0,        // idle crouch
			0.5,        // aggression
			0.8,        // tactical
			0.0,        // camper
			16000,      // alertness
			100,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"helgaSightPlayer",
		"helgaAttackPlayer",
		"helgaOrders",
		"helgaDeath",
		"helgaDeath",        //----(SA)	added
		"helgaPain",
		"helgaStay",     // stay - you're told to stay put
		"helgaFollow",       // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"helgaOrdersDeny",   // deny - refuse orders (doing something else)
		AITEAM_MONSTER,                     // team
		"helga/default",                 // default model/skin
		{WP_LUGER},                         // starting weapons
		BBOX_SMALL, {32,48},                // bbox, crouch/stand height
//		AIFL_STAND_IDLE2,					// flags
		0,
		NULL, NULL, NULL,                   // special attack routine
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_HEINRICH
	{
		"Heinrich",
		{
			170,        // running speed
			100,        // walking speed
			90,         // crouching speed
			90,         // Field of View
			230,        // Yaw Speed
			0.0,        // leader
			0.7,        // aim skill
			1.0,        // aim accuracy
			0.9,        // attack skill
			0.2,        // reaction time
			0.05,       // attack crouch
			0.0,        // idle crouch
			0.9,        // aggression
			0.1,        // tactical
			0.0,        // camper
			16000,      // alertness
			300,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"heinrichSightPlayer",
		"heinrichAttackPlayer",
		"heinrichOrders",
		"heinrichDeath",
		"heinrichDeath",
		"heinrichPain",
		"heinrichStay",          // stay - you're told to stay put
		"heinrichFollow",        // follow - go with ordering player ("i'm with you" rather than "yes sir!")
		"heinrichOrdersDeny",    // deny - refuse orders (doing something else)
		AITEAM_NAZI,
		"heinrich/default",
		{WP_VENOM_FULL},
		BBOX_LARGE, {110,140},  // (SA) height is not exact.  just eyeballed.
		AIFL_WALKFORWARD | AIFL_NO_RELOAD,
		NULL, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
	//AICHAR_PARTISAN
	{
		"Partisan",
		{
			220,        // running speed
			90,         // walking speed
			80,         // crouching speed
			90,         // Field of View
			300,        // Yaw Speed
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.5,        // reaction time
			0.3,        // attack crouch
			0.0,        // idle crouch
			0.5,        // aggression
			0.8,        // tactical
			0.0,        // camper
			16000,      // alertness
			100,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"partisanSightPlayer",
		"partisanAttackPlayer",
		"partisanOrders",
		"partisanDeath",
		"partisanDeath",     //----(SA)	added
		"partisanPain",
		"partisanStay",
		"partisanFollow",
		"partisanOrdersDeny",
		AITEAM_ALLIES,  //----(SA)	changed affiliation for DK
		"partisan/default",
		{WP_THOMPSON},
		BBOX_SMALL, {32,48},
		AIFL_CATCH_GRENADE | AIFL_STAND_IDLE2,
		NULL, NULL, NULL,
		NULL,
		AISTATE_RELAXED
	},
	//AICHAR_CIVILIAN
	{
		"Civilian",
		{
			220,        // running speed
			90,         // walking speed
			80,         // crouching speed
			90,         // Field of View
			300,        // Yaw Speed
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.5,        // reaction time
			0.3,        // attack crouch
			0.0,        // idle crouch
			0.5,        // aggression
			0.8,        // tactical
			0.0,        // camper
			16000,      // alertness
			100,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"civilianSightPlayer",
		"civilianAttackPlayer",
		"civilianOrders",
		"civilianDeath",
		"civilianDeath",     //----(SA)	added
		"civilianPain",
		"civilianStay",
		"civilianFollow",
		"civilianOrdersDeny",
		AITEAM_NEUTRAL, //----(SA)	changed affiliation for DK
		"civilian/default",
		{0},
		BBOX_SMALL, {32,48},
		AIFL_CATCH_GRENADE | AIFL_STAND_IDLE2,
		NULL, NULL, NULL,
		NULL,
		AISTATE_RELAXED
	},
	//AICHAR_CHIMP
	{
		"Chimp",
		{
			220,        // running speed
			90,         // walking speed
			80,         // crouching speed
			90,         // Field of View
			300,        // Yaw Speed
			0.0,        // leader
			0.70,       // aim skill
			0.70,       // aim accuracy
			0.75,       // attack skill
			0.5,        // reaction time
			0.3,        // attack crouch
			0.0,        // idle crouch
			0.5,        // aggression
			0.8,        // tactical
			0.0,        // camper
			16000,      // alertness
			100,        // starting health
			1.0,        // hearing range
			512,        // relaxec detection radius
			1.0,        // pain threshold multiplier
		},
		"chimpSightPlayer",
		"chimpAttackPlayer",
		"chimpOrders",
		"chimpDeath",
		"chimpDeath",        //----(SA)	added
		"chimpPain",
		"chimpStay",
		"chimpFollow",
		"chimpOrdersDeny",
		AITEAM_ALLIES,  //----(SA)	changed affiliation for DK
		"chimp/default",
		{0},
		BBOX_SMALL, {16,24},
		AIFL_CATCH_GRENADE | AIFL_STAND_IDLE2 | AIFL_NO_RELOAD,
		NULL, NULL, NULL,
		NULL,
		AISTATE_ALERT
	},
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Bounding boxes
static vec3_t bbmins[2] = {{-18, -18, -24},{-32,-32,-24}};
static vec3_t bbmaxs[2] = {{ 18,  18,  48},{ 32, 32, 68}};
// TTimo unused
//static float crouchMaxZ[2] = {32,48};	// same as player, will head be ok?
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Weapon info
cast_weapon_info_t weaponInfo;
//---------------------------------------------------------------------------

/*
============
AIChar_SetBBox

  FIXME: pass a maxZ into this so we can tailor the height for each character,
  since height isn't important for the AAS routing (whereas width is very important)
============
*/
void AIChar_SetBBox( gentity_t *ent, cast_state_t *cs ) {
	VectorCopy( bbmins[cs->aasWorldIndex], ent->client->ps.mins );
	VectorCopy( bbmaxs[cs->aasWorldIndex], ent->client->ps.maxs );
	ent->client->ps.maxs[2] = aiDefaults[cs->aiCharacter].crouchstandZ[1];
	VectorCopy( ent->client->ps.mins, ent->r.mins );
	VectorCopy( ent->client->ps.maxs, ent->r.maxs );
	ent->client->ps.crouchMaxZ = aiDefaults[cs->aiCharacter].crouchstandZ[0];
	ent->s.density = cs->aasWorldIndex;
}

/*
============
AIChar_Death
============
*/
void AIChar_Death( gentity_t *ent, gentity_t *attacker, int damage, int mod ) { //----(SA)	added mod
	// need this check otherwise sound will overwrite gib message
	if ( ent->health > GIB_HEALTH  ) {
		if ( ent->client->ps.eFlags & EF_HEADSHOT ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].quietDeathSoundScript ) );
		} else {
			switch ( mod ) {               //----(SA)	modified to add 'quiet' deaths
			case MOD_KNIFE_STEALTH:
			case MOD_SNIPERRIFLE:
			case MOD_SNOOPERSCOPE:
				G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].quietDeathSoundScript ) );
				break;
			default:
				G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].deathSoundScript ) );
				break;
			}
		}
	}
}

/*
=============
AIChar_GetPainLocation
=============
*/
int AIChar_GetPainLocation( gentity_t *ent, vec3_t point ) {
	static char *painTagNames[] = {
		"tag_head",
		"tag_chest",
		"tag_torso",
		"tag_groin",
		"tag_armright",
		"tag_armleft",
		"tag_legright",
		"tag_legleft",
		NULL,
	};

	int tagIndex, bestTag;
	float bestDist, dist;
	orientation_t or;

	// first make sure the client is able to retrieve tag information
	// TTimo gcc: warning: comparison is always false due to limited range of data type
	// initial line: if (trap_GetTag( ent->s.number, painTagNames[0], &or ) < 0)
	if ( !trap_GetTag( ent->s.number, painTagNames[0], &or ) ) {
		return 0;
	}

	// find a correct animation to play, based on the body orientation at previous frame
	for ( tagIndex = 0, bestDist = 0, bestTag = -1; painTagNames[tagIndex]; tagIndex++ ) {
		// grab the tag with this name
		// TTimo gcc: warning: comparison is always true due to limited range of data type
		// initial line: if (trap_GetTag( ent->s.number, painTagNames[tagIndex], &or ) >= 0)
		if ( trap_GetTag( ent->s.number, painTagNames[tagIndex], &or ) ) {
			dist = VectorDistance( or.origin, point );
			if ( !bestDist || dist < bestDist ) {
				bestTag = tagIndex;
				bestDist = dist;
			}
		}
	}

	if ( bestTag >= 0 ) {
		return bestTag + 1;
	}

	return 0;
}

/*
============
AIChar_Pain
============
*/
void AIChar_Pain( gentity_t *ent, gentity_t *attacker, int damage, vec3_t point ) {
	#define PAIN_THRESHOLD      25
	#define STUNNED_THRESHOLD   30
	cast_state_t    *cs;
	float dist;
	qboolean forceStun = qfalse;
	float painThreshold, stunnedThreshold;

	cs = AICast_GetCastState( ent->s.number );

	if ( g_testPain.integer == 1 ) {
		ent->health = ent->client->pers.maxHealth;  // debugging
	}

	if ( g_testPain.integer != 2 ) {
		if ( level.time < cs->painSoundTime ) {
			return;
		}
	}

	painThreshold = PAIN_THRESHOLD * cs->attributes[PAIN_THRESHOLD_SCALE];
	stunnedThreshold = STUNNED_THRESHOLD * cs->attributes[PAIN_THRESHOLD_SCALE];

	// if they are already playing another animation, we might get confused and cut it off, so don't play a pain
	if ( ent->client->ps.torsoTimer || ent->client->ps.legsTimer ) {
		return;
	}

	// if we are waiting for our weapon to fire (throwing a grenade)
	if ( ent->client->ps.weaponDelay ) {
		return;
	}

	// HACK: if the attacker is using the flamethrower, don't do any special pain anim or sound
	// FIXME: we should pass in the MOD here, since they could have fired a grenade, then switched weapons
	if ( attacker->s.weapon == WP_FLAMETHROWER ) {
		return;
	}

	if ( !Q_stricmp( attacker->classname, "props_statue" ) ) {
		damage = 99999; // try and force a stun
		forceStun = qtrue;
	}

	// now check the damageQuota to see if we should play a pain animation
	// first reduce the current damageQuota with time
	if ( cs->damageQuotaTime && cs->damageQuota > 0 ) {
		cs->damageQuota -= (int)( ( 1.0 + ( g_gameskill.value / GSKILL_MAX ) ) * ( (float)( level.time - cs->damageQuotaTime ) / 1000 ) * ( 7.5 + cs->attributes[ATTACK_SKILL] * 10.0 ) );
		if ( cs->damageQuota < 0 ) {
			cs->damageQuota = 0;
		}
	}

	// if it's been a long time since our last pain, scale it up
	if ( cs->painSoundTime < level.time - 1000 ) {
		float scale;
		scale = (float)( level.time - cs->painSoundTime - 1000 ) / 1000.0;
		if ( scale > 4.0 ) {
			scale = 4.0;
		}
		damage = (int)( (float)damage * ( 1.0 + ( scale * ( 1.0 - 0.5 * g_gameskill.value / GSKILL_MAX ) ) ) );
	}

	// adjust the new damage with distance, if they are really close, scale it down, to make it
	// harder to get through the game by continually rushing the enemies
	if ( ( dist = VectorDistance( ent->r.currentOrigin, attacker->r.currentAngles ) ) < 384 ) {
		damage -= (int)( (float)damage * ( 1.0 - ( dist / 384.0 ) ) * ( 0.5 + 0.5 * g_gameskill.value / GSKILL_MAX ) );
	}

	// add the new damage
	cs->damageQuota += damage;
	cs->damageQuotaTime = level.time;

	if ( forceStun ) {
		damage = 99999; // try and force a stun
		cs->damageQuota = painThreshold + 1;
	}

	// if it's over the threshold, play a pain

	// don't do this if crouching, or we might clip through the world

	if ( g_testPain.integer == 2 || ( cs->damageQuota > painThreshold ) ) {
		int delay;

		// stunned?
		if ( damage > stunnedThreshold && ( forceStun || ( rand() % 2 ) ) ) {   // stunned
			BG_UpdateConditionValue( ent->s.number, ANIM_COND_STUNNED, qtrue, qfalse );
		}
		// enemy weapon
		if ( attacker->client ) {
			BG_UpdateConditionValue( ent->s.number, ANIM_COND_ENEMY_WEAPON, attacker->s.weapon, qtrue );
		}
		if ( point ) {
			// location
			BG_UpdateConditionValue( ent->s.number, ANIM_COND_IMPACT_POINT, AIChar_GetPainLocation( ent, point ), qtrue );
		} else {
			BG_UpdateConditionValue( ent->s.number, ANIM_COND_IMPACT_POINT, 0, qfalse );
		}

		// pause while we play a pain
		delay = BG_AnimScriptEvent( &ent->client->ps, ANIM_ET_PAIN, qfalse, qtrue );

		// turn off temporary conditions
		BG_UpdateConditionValue( ent->s.number, ANIM_COND_STUNNED, 0, qfalse );
		BG_UpdateConditionValue( ent->s.number, ANIM_COND_ENEMY_WEAPON, 0, qfalse );
		BG_UpdateConditionValue( ent->s.number, ANIM_COND_IMPACT_POINT, 0, qfalse );

		if ( delay >= 0 ) {
			// setup game stuff to handle the character movements, etc
			cs->pauseTime = level.time + delay + 250;
			cs->lockViewAnglesTime = cs->pauseTime;
			// make sure we stop crouching
			cs->bs->attackcrouch_time = 0;
			// don't fire while in pain?
			cs->triggerReleaseTime = cs->pauseTime;
			// stay crouching if we were before the pain
			if ( cs->bs->cur_ps.viewheight > cs->bs->cur_ps.crouchViewHeight ) {
				cs->bs->attackcrouch_time = trap_AAS_Time() + (float)( cs->pauseTime - level.time ) / 1000.0 + 0.5;
			}
		}

		// if we didnt just play a scripted sound, then play one of the default sounds
		if ( cs->lastScriptSound < level.time ) {
			G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].painSoundScript ) );
		}

		// reset the quota
		cs->damageQuota = 0;
		cs->damageQuotaTime = 0;
		//
		cs->painSoundTime = cs->pauseTime + (int)( 1000 * ( g_gameskill.value / GSKILL_MAX ) );     // add a bit more of a buffer before the next one
	}

}

/*
============
AIChar_Sight
============
*/
void AIChar_Sight( gentity_t *ent, gentity_t *other, int lastSight ) {
	cast_state_t    *cs;

	cs = AICast_GetCastState( ent->s.number );

	// if we are in noattack mode, don't make sounds
	if ( cs->castScriptStatus.scriptNoAttackTime >= level.time ) {
		return;
	}
	if ( cs->noAttackTime >= level.time ) {
		return;
	}

	// if they have recently played a script sound, then ignore this
	if ( cs->lastScriptSound > level.time - 4000 ) {
		return;
	}

	if ( !AICast_SameTeam( cs, other->s.number ) ) {
		if ( !cs->firstSightTime || cs->firstSightTime < ( level.time - 15000 ) ) {
			//G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].sightSoundScript ) );
		}
		cs->firstSightTime = level.time;
	}

}

/*
=====================
AIChar_AttackSND

  NOTE: this should just lookup a sound script for this character/weapon combo
=====================
*/
void AIChar_AttackSound( cast_state_t *cs ) {

	gentity_t *ent;

	ent = &g_entities [cs->entityNum];

	if ( cs->attackSNDtime > level.time ) {
		return;
	}

	// if we are in noattack mode, don't make sounds
	if ( cs->castScriptStatus.scriptNoAttackTime >= level.time ) {
		return;
	}
	if ( cs->noAttackTime >= level.time ) {
		return;
	}

	// Ridah, only yell when throwing grenades every now and then, since it's not very "stealthy"
	if ( cs->bs->weaponnum == WP_GRENADE_LAUNCHER && rand() % 5 ) {
		return;
	}

	cs->attackSNDtime = level.time + 5000 + ( 1000 * rand() % 10 );

	AICast_ScriptEvent( cs, "attacksound", ent->aiName );
	if ( cs->aiFlags & AIFL_DENYACTION ) {
		return;
	}

	if ( cs->bs->weaponnum == WP_LUGER ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].ordersSoundScript ) );
	} else {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( aiDefaults[ent->aiCharacter].attackSoundScript ) );
	}

}

/*
============
AIChar_spawn
============
*/
void AIChar_spawn( gentity_t *ent ) {
	gentity_t       *newent;
	cast_state_t    *cs;
	AICharacterDefaults_t *aiCharDefaults;
	int i;
	static int lastCall;
	static int numCalls;

	// if there are other cast's waiting to spawn before us, wait for them
	for ( i = MAX_CLIENTS, newent = &g_entities[MAX_CLIENTS]; i < MAX_GENTITIES; i++, newent++ ) {
		if ( !newent->inuse ) {
			continue;
		}
		if ( newent->think != AIChar_spawn ) {
			continue;
		}
		if ( newent == ent ) {
			break;      // we are the first in line
		}
		// still waiting for someone else
		ent->nextthink = level.time + FRAMETIME;
		return;
	}

	// if the client hasn't connected yet, wait around
	if ( !AICast_FindEntityForName( "player" ) ) {
		ent->nextthink = level.time + FRAMETIME;
		return;
	}

	if ( lastCall == level.time ) {
		if ( numCalls++ > 2 ) {
			ent->nextthink = level.time + FRAMETIME;
			return;     // spawned enough this frame already
		}
	} else {
		numCalls = 0;
	}
	lastCall = level.time;

	aiCharDefaults = &aiDefaults[ent->aiCharacter];

	// ............................
	// setup weapon info
	//
	// starting weapons/ammo
	memset( &weaponInfo, 0, sizeof( weaponInfo ) );
	for ( i = 0; aiCharDefaults->weapons[i]; i++ ) {
		//weaponInfo.startingWeapons[(aiCharDefaults->weapons[i] / 32)] |= ( 1 << aiCharDefaults->weapons[i] );
		//weaponInfo.startingWeapons[0] |= ( 1 << aiCharDefaults->weapons[i] );
		COM_BitSet( weaponInfo.startingWeapons, aiCharDefaults->weapons[i] );
		if ( aiCharDefaults->weapons[i] == WP_GRENADE_LAUNCHER ) { // give them a bunch of grenades, but not an unlimited supply
			weaponInfo.startingAmmo[BG_FindAmmoForWeapon( aiCharDefaults->weapons[i] )] = 6;
		} else {
			weaponInfo.startingAmmo[BG_FindAmmoForWeapon( aiCharDefaults->weapons[i] )] = 999;
		}
	}
	//
	// use the default skin if nothing specified
	if ( !ent->aiSkin || !strlen( ent->aiSkin ) ) {
		ent->aiSkin = aiCharDefaults->skin;
	}
	// ............................
	//
	// create the character

	// (there will always be an ent->aiSkin (SA))
	newent = AICast_CreateCharacter( ent, aiCharDefaults->attributes, &weaponInfo, aiCharDefaults->name, ent->aiSkin, ent->aihSkin, "m", "7", "100" );

	if ( !newent ) {
		G_FreeEntity( ent );
		return;
	}
	// copy any character-specific information to the new entity (like editor fields, etc)
	//
	// copy this across so killing ai can trigger a target
	newent->target = ent->target;
	//
	newent->classname = ent->classname;
	newent->r.svFlags |= ( ent->r.svFlags & SVF_NOFOOTSTEPS );
	newent->aiCharacter = ent->aiCharacter;
	newent->client->ps.aiChar = ent->aiCharacter;
	newent->spawnflags = ent->spawnflags;
	newent->aiTeam = ent->aiTeam;
	if ( newent->aiTeam < 0 ) {
		newent->aiTeam = aiCharDefaults->aiTeam;
	}
	newent->client->ps.teamNum = newent->aiTeam;
	if ( newent->aiCharacter == AICHAR_FEMZOMBIE ) {
		newent->flags |= FL_NO_KNOCKBACK;
	}
	//
	// kill the old entity
	G_FreeEntity( ent );
	// attach to the new entity
	ent = newent;
	//
	// precache any specific sounds
	//
	// ...
	//
	// get the cast state
	cs = AICast_GetCastState( ent->s.number );
	//
	// setup any character specific cast_state variables
	cs->deathfunc = AIChar_Death;
	cs->painfunc = AIChar_Pain;
	cs->aiFlags |= aiCharDefaults->aiFlags;
	cs->aiState = aiCharDefaults->aiState;
	//
	cs->queryCountValidTime = -1;
	//
	// randomly choose idle animation
	if ( cs->aiFlags & AIFL_STAND_IDLE2 ) {
		newent->client->ps.eFlags |= EF_STAND_IDLE2;
	}
	//
	// attach any event specific functions (pain, death, etc)
	//
	//cs->getDeathAnim = AIChar_getDeathAnim;
	cs->sightfunc = AIChar_Sight;
	if ( ent->aiTeam == AITEAM_ALLIES || ent->aiTeam == AITEAM_NEUTRAL ) { // friendly
		cs->activate = AICast_ProcessActivate;
	} else {
		cs->activate = NULL;
	}
	cs->aifuncAttack1 = aiCharDefaults->aifuncAttack1;
	cs->aifuncAttack2 = aiCharDefaults->aifuncAttack2;
	cs->aifuncAttack3 = aiCharDefaults->aifuncAttack3;
	//
	// looping sound?
	if ( aiCharDefaults->loopingSound ) {
		ent->s.loopSound = G_SoundIndex( aiCharDefaults->loopingSound );
	}
	//
	// precache sounds for this character
	G_SoundIndex( aiDefaults[ent->aiCharacter].attackSoundScript );
	G_SoundIndex( aiDefaults[ent->aiCharacter].sightSoundScript );
	G_SoundIndex( aiDefaults[ent->aiCharacter].ordersSoundScript );
	G_SoundIndex( aiDefaults[ent->aiCharacter].deathSoundScript );
	G_SoundIndex( aiDefaults[ent->aiCharacter].quietDeathSoundScript ); //----(SA)	added
	G_SoundIndex( aiDefaults[ent->aiCharacter].painSoundScript );
	G_SoundIndex( aiDefaults[ent->aiCharacter].staySoundScript );
	G_SoundIndex( aiDefaults[ent->aiCharacter].followSoundScript );
	G_SoundIndex( aiDefaults[ent->aiCharacter].ordersDenySoundScript );
	//
	// special spawnflag stuff
	if ( ent->spawnflags & 2 ) {
		cs->secondDeadTime = qtrue;
	}
	//
	// init scripting
	cs->castScriptStatus.castScriptEventIndex = -1;
	cs->castScriptStatus.scriptAttackEnt = -1;
	//
	// set crouch move speed
	ent->client->ps.crouchSpeedScale = cs->attributes[CROUCHING_SPEED] / cs->attributes[RUNNING_SPEED];
	//
	// check for some anims which we can use for special behaviours
	if ( BG_GetAnimScriptEvent( &ent->client->ps, ANIM_ET_ROLL ) >= 0 ) {
		cs->aiFlags |= AIFL_ROLL_ANIM;
	}
	if ( BG_GetAnimScriptEvent( &ent->client->ps, ANIM_ET_FLIP ) >= 0 ) {
		cs->aiFlags |= AIFL_FLIP_ANIM;
	}
	if ( BG_GetAnimScriptEvent( &ent->client->ps, ANIM_ET_DIVE ) >= 0 ) {
		cs->aiFlags |= AIFL_DIVE_ANIM;
	}
	//
	// check for no headshot damage
	if ( cs->aiFlags & AIFL_NO_HEADSHOT_DMG ) {
		ent->headshotDamageScale = 0.0;
	}
	//
	if ( !ent->aiInactive ) {
		// trigger a spawn script event
		AICast_ScriptEvent( cs, "spawn", "" );
	} else {
		trap_UnlinkEntity( ent );
	}

}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_soldier (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
soldier entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'infantryss/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/
/*
-------- MODEL FOR RADIANT ONLY - DO NOT SET THIS AS A KEY --------
model="models\mapobjects\characters\test\nazi.md3"
*/
/*
============
SP_ai_soldier
============
*/
void SP_ai_soldier( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_SOLDIER );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_american (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
american entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'american/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_american
============
*/
void SP_ai_american( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_AMERICAN );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_zombie (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive PortalZombie
zombie entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'zombie/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_zombie
============
*/
void SP_ai_zombie( gentity_t *ent ) {
	ent->r.svFlags |= SVF_NOFOOTSTEPS;
	AICast_DelayedSpawnCast( ent, AICHAR_ZOMBIE );
}


//----(SA)	added
//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_warzombie (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive PortalZombie
warrior zombie entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'warrior/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_zombie
============
*/
void SP_ai_warzombie( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_WARZOMBIE );
}


//----(SA)	end



//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_femzombie (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
zombie entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'femzombie/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_femzombie
============
*/
void SP_ai_femzombie( gentity_t *ent ) {
	ent->r.svFlags |= SVF_NOFOOTSTEPS;
	AICast_DelayedSpawnCast( ent, AICHAR_FEMZOMBIE );
}




//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_undead (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
undead entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'undead/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_undead
============
*/
void SP_ai_undead( gentity_t *ent ) {
	ent->r.svFlags |= SVF_NOFOOTSTEPS;
	AICast_DelayedSpawnCast( ent, AICHAR_UNDEAD );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_venom (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
venom entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'venom/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_venom
============
*/
void SP_ai_venom( gentity_t *ent ) {
	ent->r.svFlags |= SVF_NOFOOTSTEPS;
	AICast_DelayedSpawnCast( ent, AICHAR_VENOM );
}


//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_loper (1 0.25 0) (-32 -32 -24) (32 32 48) TriggerSpawn NoRevive
loper entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'loper/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_loper
============
*/
void SP_ai_loper( gentity_t *ent ) {
	ent->r.svFlags |= SVF_NOFOOTSTEPS;
	AICast_DelayedSpawnCast( ent, AICHAR_LOPER );
	//
	level.loperZapSound = G_SoundIndex( "loperZap" );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_sealoper (1 0.25 0) (-32 -32 -24) (32 32 48) TriggerSpawn NoRevive
loper entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'sealoper/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_sealoper
============
*/
void SP_ai_sealoper( gentity_t *ent ) {
	ent->r.svFlags |= SVF_NOFOOTSTEPS;
	AICast_DelayedSpawnCast( ent, AICHAR_SEALOPER );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_boss_helga (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
helga entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'helga/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_boss_helga
============
*/
void SP_ai_boss_helga( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_HELGA );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_boss_heinrich (1 0.25 0) (-32 -32 -24) (32 32 156) TriggerSpawn NoRevive
heinrich entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'helga/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_boss_heinrich
============
*/
void SP_ai_boss_heinrich( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_HEINRICH );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_partisan (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'partisan/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_partisan
============
*/
void SP_ai_partisan( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_PARTISAN );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_civilian (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'civilian/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_civilian
============
*/
void SP_ai_civilian( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_CIVILIAN );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_chimp (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'chimp/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_chimp
============
*/
void SP_ai_chimp( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_CHIMP );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_eliteguard (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
elite guard entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'eliteguard/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_eliteguard
============
*/
void SP_ai_eliteguard( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_ELITEGUARD );
}


//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_frogman (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
elite guard entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'frogman/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_frogman
============
*/
void SP_ai_frogman( gentity_t *ent ) {
	ent->r.svFlags |= SVF_NOFOOTSTEPS;
	AICast_DelayedSpawnCast( ent, AICHAR_FROGMAN );
}


//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_stimsoldier_dual (1 0.25 0) (-32 -32 -24) (32 32 64) TriggerSpawn NoRevive
stim soldier entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'stim/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_stimsoldier_dual
============
*/
void SP_ai_stimsoldier_dual( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_STIMSOLDIER1 );
	//
	level.stimSoldierFlySound = G_SoundIndex( "sound/stimsoldier/flyloop.wav" );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_stimsoldier_rocket (1 0.25 0) (-32 -32 -24) (32 32 64) TriggerSpawn NoRevive
stim soldier entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'stim/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_stimsoldier_rocket
============
*/
void SP_ai_stimsoldier_rocket( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_STIMSOLDIER2 );
	//
	level.stimSoldierFlySound = G_SoundIndex( "sound/stimsoldier/flyloop.wav" );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_stimsoldier_tesla (1 0.25 0) (-32 -32 -24) (32 32 64) TriggerSpawn NoRevive
stim soldier entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'stim/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_stimsoldier_tesla
============
*/
void SP_ai_stimsoldier_tesla( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_STIMSOLDIER3 );
	//
	level.stimSoldierFlySound = G_SoundIndex( "sound/stimsoldier/flyloop.wav" );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_supersoldier (1 0.25 0) (-32 -32 -24) (32 32 64) TriggerSpawn NoRevive
supersoldier entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'supersoldier/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_supersoldier
============
*/
void SP_ai_supersoldier( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_SUPERSOLDIER );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_protosoldier (1 0.25 0) (-32 -32 -24) (32 32 64) TriggerSpawn NoRevive
protosoldier entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'protosoldier/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_protosoldier
============
*/
void SP_ai_protosoldier( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_PROTOSOLDIER );
}


//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_rejectxcreature (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
Reject X creature
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'rejectx/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

void SP_ai_rejectxcreature( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_REJECTX );
}

//----------------------------------------------------------------------------------------------------------------------------
/*QUAKED ai_blackguard (1 0.25 0) (-16 -16 -24) (16 16 64) TriggerSpawn NoRevive
black guard entity
"skin" the .skin file to use for this character (must exist in the player characters directory, otherwise 'blackguard/default' is used)
"head" the .skin file to use for his head (must exist in the pc's dir, otherwise 'default' is used)
"ainame" name of AI
*/

/*
============
SP_ai_blackguard
============
*/
void SP_ai_blackguard( gentity_t *ent ) {
	AICast_DelayedSpawnCast( ent, AICHAR_BLACKGUARD );
}
