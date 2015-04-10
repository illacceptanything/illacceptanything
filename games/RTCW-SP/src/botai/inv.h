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
 * name:		inv.h
 *
 * desc:
 *
*/


#define INVENTORY_NONE              0
//armor
#define INVENTORY_ARMOR             1
//weapons
#define INVENTORY_LUGER             4
#define INVENTORY_MAUSER            5
#define INVENTORY_MP40              6
#define INVENTORY_SP5               7
#define INVENTORY_ROCKETLAUNCHER    8
#define INVENTORY_GRENADELAUNCHER   9
#define INVENTORY_VENOM             10
#define INVENTORY_FLAMETHROWER      11
#define INVENTORY_CROSS             12

#define INVENTORY_GAUNTLET          13


// please leave these open up to 27 (INVENTORY_9MM) (and double check defines when merging)
// the inventory max (MAX_ITEMS) is 256, so we aren't too concerned about running out of space

//ammo
#define INVENTORY_9MM               27
#define INVENTORY_792MM             28
#define INVENTORY_SP5AMMO           29
#define INVENTORY_ROCKETS           30
#define INVENTORY_GRENADES          31
#define INVENTORY_127MM             32
#define INVENTORY_FUEL              33
#define INVENTORY_CHARGES           34

// please leave these open up to 48 (INVENTORY_HEALTH) (and double check defines when merging)
// the inventory max (MAX_ITEMS) is 256, so we aren't too concerned about running out of space

//powerups
#define INVENTORY_HEALTH            48
#define INVENTORY_TELEPORTER        49
#define INVENTORY_MEDKIT            50
#define INVENTORY_QUAD              51
#define INVENTORY_ENVIRONMENTSUIT   52
#define INVENTORY_HASTE             53
#define INVENTORY_INVISIBILITY      54
#define INVENTORY_REGEN             55
#define INVENTORY_FLIGHT            56
#define INVENTORY_REDFLAG           57
#define INVENTORY_BLUEFLAG          58
//enemy stuff
#define ENEMY_HORIZONTAL_DIST       200
#define ENEMY_HEIGHT                201
#define NUM_VISIBLE_ENEMIES         202
#define NUM_VISIBLE_TEAMMATES       203

//item numbers (make sure they are in sync with bg_itemlist in bg_misc.c)
#define MODELINDEX_ARMORSHARD       1
#define MODELINDEX_ARMORCOMBAT      2
#define MODELINDEX_ARMORBODY        3
#define MODELINDEX_HEALTHSMALL      4
#define MODELINDEX_HEALTH           5
#define MODELINDEX_HEALTHLARGE      6
#define MODELINDEX_HEALTHMEGA       7

#define MODELINDEX_GAUNTLET         8
#define MODELINDEX_SHOTGUN          9
#define MODELINDEX_MACHINEGUN       10
#define MODELINDEX_GRENADELAUNCHER  11
#define MODELINDEX_ROCKETLAUNCHER   12
#define MODELINDEX_LIGHTNING        13
#define MODELINDEX_RAILGUN          14
#define MODELINDEX_SP5              15
#define MODELINDEX_BFG10K           16
#define MODELINDEX_GRAPPLINGHOOK    17

#define MODELINDEX_SHELLS           18
#define MODELINDEX_BULLETS          19
#define MODELINDEX_GRENADES         20
#define MODELINDEX_CELLS            21
#define MODELINDEX_LIGHTNINGAMMO    22
#define MODELINDEX_ROCKETS          23
#define MODELINDEX_SLUGS            24
#define MODELINDEX_BFGAMMO          25

#define MODELINDEX_TELEPORTER       26
#define MODELINDEX_MEDKIT           27
#define MODELINDEX_QUAD             28
#define MODELINDEX_ENVIRONMENTSUIT  29
#define MODELINDEX_HASTE            30
#define MODELINDEX_INVISIBILITY     31
#define MODELINDEX_REGEN            32
#define MODELINDEX_FLIGHT           33
#define MODELINDEX_REDFLAG          34
#define MODELINDEX_BLUEFLAG         35


