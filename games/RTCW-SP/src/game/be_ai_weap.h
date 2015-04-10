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
 * name:		be_ai_weap.h
 *
 * desc:		weapon AI
 *
 *
 *****************************************************************************/

//projectile flags
#define PFL_WINDOWDAMAGE            1       //projectile damages through window
#define PFL_RETURN                  2       //set when projectile returns to owner
//weapon flags
#define WFL_FIRERELEASED            1       //set when projectile is fired with key-up event
//damage types
#define DAMAGETYPE_IMPACT           1       //damage on impact
#define DAMAGETYPE_RADIAL           2       //radial damage
#define DAMAGETYPE_VISIBLE          4       //damage to all entities visible to the projectile

typedef struct projectileinfo_s
{
	char name[MAX_STRINGFIELD];
	char model[MAX_STRINGFIELD];
	int flags;
	float gravity;
	int damage;
	float radius;
	int visdamage;
	int damagetype;
	int healthinc;
	float push;
	float detonation;
	float bounce;
	float bouncefric;
	float bouncestop;
} projectileinfo_t;

typedef struct weaponinfo_s
{
	int valid;                  //true if the weapon info is valid
	int number;                                 //number of the weapon
	char name[MAX_STRINGFIELD];
	char model[MAX_STRINGFIELD];
	int level;
	int weaponindex;
	int flags;
	char projectile[MAX_STRINGFIELD];
	int numprojectiles;
	float hspread;
	float vspread;
	float speed;
	float acceleration;
	vec3_t recoil;
	vec3_t offset;
	vec3_t angleoffset;
	float extrazvelocity;
	int ammoamount;
	int ammoindex;
	float activate;
	float reload;
	float spinup;
	float spindown;
	projectileinfo_t proj;                      //pointer to the used projectile
} weaponinfo_t;

//setup the weapon AI
int BotSetupWeaponAI( void );
//shut down the weapon AI
void BotShutdownWeaponAI( void );
//returns the best weapon to fight with
int BotChooseBestFightWeapon( int weaponstate, int *inventory );
//returns the information of the current weapon
void BotGetWeaponInfo( int weaponstate, int weapon, weaponinfo_t *weaponinfo );
//loads the weapon weights
int BotLoadWeaponWeights( int weaponstate, char *filename );
//returns a handle to a newly allocated weapon state
int BotAllocWeaponState( void );
//frees the weapon state
void BotFreeWeaponState( int weaponstate );
//resets the whole weapon state
void BotResetWeaponState( int weaponstate );
