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



// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"

extern int hWeaponSnd;
extern int hWeaponEchoSnd;      // JPW NERVE nasty kludge, referenced from cg_weapons.c

extern void CG_Tracer( vec3_t source, vec3_t dest, int sparks );
//==========================================================================

/*
===================
CG_PlaceString

Also called by scoreboard drawing
===================
*/
const char  *CG_PlaceString( int rank ) {
	static char str[64];
	char    *s, *t;

	if ( rank & RANK_TIED_FLAG ) {
		rank &= ~RANK_TIED_FLAG;
		t = "Tied for ";
	} else {
		t = "";
	}

	if ( rank == 1 ) {
		s = S_COLOR_BLUE "1st" S_COLOR_WHITE;        // draw in blue
	} else if ( rank == 2 ) {
		s = S_COLOR_RED "2nd" S_COLOR_WHITE;     // draw in red
	} else if ( rank == 3 ) {
		s = S_COLOR_YELLOW "3rd" S_COLOR_WHITE;      // draw in yellow
	} else if ( rank == 11 ) {
		s = "11th";
	} else if ( rank == 12 ) {
		s = "12th";
	} else if ( rank == 13 ) {
		s = "13th";
	} else if ( rank % 10 == 1 ) {
		s = va( "%ist", rank );
	} else if ( rank % 10 == 2 ) {
		s = va( "%ind", rank );
	} else if ( rank % 10 == 3 ) {
		s = va( "%ird", rank );
	} else {
		s = va( "%ith", rank );
	}

	Com_sprintf( str, sizeof( str ), "%s%s", t, s );
	return str;
}

/*
=============
CG_Obituary
=============
*/
static void CG_Obituary( entityState_t *ent ) {
	int mod;
	int target, attacker;
	int killtype = 0;               // DHM - Nerve :: 0==Axis; 1==Allied; 2==your kill
	char        *message;
	char        *message2;
	const char  *targetInfo;
	const char  *attackerInfo;
	char targetName[32];
	char attackerName[32];
	clientInfo_t    *ci, *ca; // JPW NERVE ca = attacker

	// Ridah, no obituaries in single player
	if ( cgs.gametype == GT_SINGLE_PLAYER ) {
		return;
	}

	target = ent->otherEntityNum;
	attacker = ent->otherEntityNum2;
	mod = ent->eventParm;

	if ( target < 0 || target >= MAX_CLIENTS ) {
		CG_Error( "CG_Obituary: target out of range" );
	}
	ci = &cgs.clientinfo[target];
	ca = &cgs.clientinfo[attacker];

	if ( attacker < 0 || attacker >= MAX_CLIENTS ) {
		attacker = ENTITYNUM_WORLD;
		attackerInfo = NULL;
	} else {
		attackerInfo = CG_ConfigString( CS_PLAYERS + attacker );
	}

	targetInfo = CG_ConfigString( CS_PLAYERS + target );
	if ( !targetInfo ) {
		return;
	}
	Q_strncpyz( targetName, Info_ValueForKey( targetInfo, "n" ), sizeof( targetName ) - 2 );
	strcat( targetName, S_COLOR_WHITE );

	message2 = "";

	// DHM - Nerve :: Set killtype
	if ( attacker == cg.snap->ps.clientNum ) {
		killtype = 2;
	} else if ( ci->team == TEAM_BLUE ) {
		killtype = 1;
	} else {
		killtype = 0;
	}

	// check for single client messages

	switch ( mod ) {
	case MOD_SUICIDE:
		message = "committed suicide";
		break;
	case MOD_FALLING:
		message = "fell to his death";
		break;
	case MOD_CRUSH:
		message = "was crushed";
		break;
	case MOD_WATER:
		message = "drowned";
		break;
	case MOD_SLIME:
		message = "died by toxic materials";
		break;
		//case MOD_LAVA:
		//message = "does a back flip into the lava";
		//break;
		//case MOD_TARGET_LASER:
		//message = "saw the light";
		//break;
	case MOD_TRIGGER_HURT:
		message = "was killed";
		break;
	default:
		message = NULL;
		break;
	}

	if ( attacker == target ) {
		switch ( mod ) {
// JPW NERVE per atvi req
		case MOD_DYNAMITE:
		case MOD_DYNAMITE_SPLASH:
			message = "dynamited himself to pieces";
			break;
// jpw
		case MOD_GRENADE_SPLASH:
			message = "dove on his own grenade";
			break;
		case MOD_ROCKET_SPLASH:
			message = "vaporized himself";
			break;
		case MOD_AIRSTRIKE:
			message = "obliterated himself";
			break;
			//case MOD_BFG_SPLASH:
			//message = "should have used a smaller gun";
			//break;
		case MOD_EXPLOSIVE:
			message = "died in his own explosion";
			break;
		default:
			message = "killed himself";
			break;
		}
	}
	if ( message ) {
		message = CG_TranslateString( message );
		CG_Printf( "[cgnotify]%s %s.\n", targetName, message );
		return;
	}

	// check for kill messages from the current clientNum
	if ( attacker == cg.snap->ps.clientNum ) {
		char    *s;

		if ( cgs.gametype < GT_TEAM ) {
			s = va( "You killed %s\n%s place with %i", targetName,
					CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
					cg.snap->ps.persistant[PERS_SCORE] );
		} else {
			if ( ci->team == ca->team ) {
				s = va( "%s %s", CG_TranslateString( "You killed ^1TEAMMATE^7" ), targetName );
			} else {
				s = va( "%s %s", CG_TranslateString( "You killed" ), targetName );
			}
		}
		CG_PriorityCenterPrint( s, SCREEN_HEIGHT * 0.75, BIGCHAR_WIDTH * 0.6, 1 );
		// print the text message as well
	}

	// check for double client messages
	if ( !attackerInfo ) {
		attacker = ENTITYNUM_WORLD;
		strcpy( attackerName, "noname" );
	} else {
		Q_strncpyz( attackerName, Info_ValueForKey( attackerInfo, "n" ), sizeof( attackerName ) - 2 );
		strcat( attackerName, S_COLOR_WHITE );
		// check for kill messages about the current clientNum
		if ( target == cg.snap->ps.clientNum ) {
			Q_strncpyz( cg.killerName, attackerName, sizeof( cg.killerName ) );
		}
	}

	if ( attacker != ENTITYNUM_WORLD ) {
		switch ( mod ) {

			// TODO: put real text here.  these are just placeholders

		case MOD_KNIFE_STEALTH:
		case MOD_KNIFE:
		case MOD_KNIFE2:
			//message = "was stabbed to death by";
			message = "was stabbed by";
			message2 = "'s knife";
			break;
		case MOD_LUGER:
			//message = "borrowed some bullets from ";
			//message2 = "'s Luger Parabellum 9mm";
			message = "was killed by";
			message2 = "'s Luger 9mm";
			break;
		case MOD_COLT:
			//message = "'s face was the unwilling recipient of";
			//message2 = "'s .45ACP 1911 rounds";
			message = "was killed by";
			message2 = " 's .45ACP 1911";
			break;
		case MOD_MP40:
			//message = "was force fed a magazine of";
			//message2 = "'s MP40 slugs";
			message = "was killed by";
			message2 = "'s MP40";
			break;
		case MOD_THOMPSON:
			//message = "was hit by a fusillade from";
			//message2 = "'s Thompson";
			message = "was killed by";
			message2 = "'s Thompson";
			break;
		case MOD_STEN:
			//message = "was quietly zapped by";
			//message2 = "'s Sten";
			message = "was killed by";
			message2 = "'s Sten";
			break;
		case MOD_MAUSER:
			//message = "took a rifle gut shot from";
			message = "was killed by";
			message2 = "'s Mauser";
			break;
		case MOD_SNIPERRIFLE:
			//message = "caught a sniper round in the teeth from";
			message = "was killed by";
			message2 = "'s sniper rifle";
			break;
		case MOD_GARAND:
			//message = "was killed (garand) by";
			break;
		case MOD_SNOOPERSCOPE:
			//message = "was killed (snooper) by";
			break;
		case MOD_AKIMBO:
			//message = "was killed (dual colts) by";
			break;
// JPW NERVE - per atvi req
		case MOD_DYNAMITE:
		case MOD_DYNAMITE_SPLASH:
			message = "was blasted by";
			message2 = "'s dynamite";
			break;
// jpw
		case MOD_ROCKET_LAUNCHER:
		case MOD_ROCKET_SPLASH:
			//message = "was vaporized by";
			//message2 = "'s Panzerfaust";
			message = "was blasted by";
			message2 = "'s Panzerfaust";
			break;
		case MOD_GRENADE_LAUNCHER:
		case MOD_GRENADE_SPLASH:
		case MOD_GRENADE_PINEAPPLE:
			//message = "was blown to bits by";
			//message2 = "'s potato masher";
			message = "was exploded by";
			message2 = "'s grenade";
			break;
		case MOD_VENOM:
			//message = "was ventilated by";
			message = "was ventilated by";
			message2 = "'s Venom";
			break;
		case MOD_VENOM_FULL:
			//message = "was killed (venom shot) by";
			break;
		case MOD_FLAMETHROWER:
			//message = "was roasted on the barbie by";
			message = "was cooked by";
			message2 = "'s flamethrower";
			break;
		case MOD_TESLA:
			//message = "was killed (tesla) by";
			break;
		case MOD_SPEARGUN:
			//message = "was killed (spear) by";
			break;
		case MOD_SPEARGUN_CO2:
			//message = "was killed (co2 spear) by";
			break;
		case MOD_MACHINEGUN:
			//message = "was perforated by";
			//message2 = "'s crew-served mg42";
			message = "was perforated by";
			message2 = "'s crew-served MG42";
			break;
		case MOD_CROSS:
			//message = "was killed (cross) by";
			break;
// JPW NERVE
		case MOD_AIRSTRIKE:
			//message = "stood under";
			//message2 = "'s air strike";
			message = "was blasted by";
			message2 = "'s support fire"; // JPW NERVE changed since it gets called for both air strikes and artillery
			break;
// jpw
// (SA) leaving a sample of two part obit's
//		case MOD_ROCKET:
//			message = "ate";
//			message2 = "'s rocket";
//			break;
//		case MOD_ROCKET_SPLASH:
//			message = "almost dodged";
//			message2 = "'s rocket";
//			break;
		default:
			message = "was killed by";
			break;
		}

// JPW NERVE if attacker != target but on same team
		if ( ci->team == ca->team ) {
			message = "^1WAS KILLED BY TEAMMATE^7";
			message2 = "";
		}
// jpw

		if ( message ) {
			message = CG_TranslateString( message );
			if ( message2 ) {
				message2 = CG_TranslateString( message2 );
			}
			CG_Printf( "[cgnotify]%s %s %s%s\n", targetName, message, attackerName, message2 );
			return;
		}
	}

	// we don't know what it was
// JPW NERVE added mod check for machinegun (prolly mortar here too)
	switch ( mod ) {
	case MOD_MACHINEGUN:
		CG_Printf( "[cgnotify]%s was riddled by machinegun fire\n",targetName );
		break;
	default:
		CG_Printf( "[cgnotify]%s died.\n", targetName );
		break;
	}
// jpw
}

//==========================================================================

/*
===============
CG_UseItem
===============
*/
static void CG_UseItem( centity_t *cent ) {
/*
	int			itemNum;
	gitem_t		*item;
	entityState_t *es;

	es = &cent->currentState;

	// itemNum = es->event - EV_USE_ITEM0;
	// JCash bluesnews reported fix
	itemNum = (es->event &~EV_EVENT_BITS) - EV_USE_ITEM0;

	if ( itemNum < 0 || itemNum > HI_NUM_HOLDABLE ) {
		itemNum = 0;
	}

	// print a message if the local player
	if ( es->number == cg.snap->ps.clientNum ) {
		if ( !itemNum ) {
			CG_CenterPrint( "No item to use", SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );
		} else {
			item = BG_FindItemForHoldable( itemNum );

			if(item) {
				cg.holdableSelectTime = cg.time;	// show remaining items

				switch(itemNum) {
					case HI_BOOK1:
					case HI_BOOK2:
					case HI_BOOK3:
						break;
					case HI_WINE:
						CG_CenterPrint( "You drank the wine", SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );
						break;
					default:
						CG_CenterPrint( va("Use %s", item->pickup_name), SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );
						break;
				}
			}
		}
	}

	switch ( itemNum ) {
	default:
	case HI_NONE:
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.useNothingSound );
		break;

	case HI_MEDKIT:
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.medkitSound );
		break;

	case HI_WINE:
		trap_S_StartSound (NULL, es->number, CHAN_BODY, cgs.media.wineSound );
		break;
	}
*/
}

// from cg_weapons.c
extern int CG_WeaponIndex( int weapnum, int *bank, int *cycle );


/*
================
CG_ItemPickup

A new item was picked up this frame
================
*/
static void CG_ItemPickup( int itemNum ) {
	int itemid;
	int wpbank_cur, wpbank_pickup;

	itemid = bg_itemlist[itemNum].giTag;

	cg.itemPickup           = itemNum;
	cg.itemPickupTime       = cg.time;
	cg.itemPickupBlendTime  = cg.time;

	// see if it should be the grabbed weapon
	if ( bg_itemlist[itemNum].giType == IT_WEAPON ) {

		if ( cg_autoswitch.integer && cg.predictedPlayerState.weaponstate != WEAPON_RELOADING ) {

			//	0 - "Off"
			//	1 - "Always Switch"
			//	2 - "If New"
			//	3 - "If Better"
			//	4 - "New or Better"

			// don't ever autoswitch to secondary fire weapons
			if ( itemid != WP_SNIPERRIFLE && itemid != WP_SNOOPERSCOPE && itemid != WP_VENOM_FULL && itemid != WP_FG42SCOPE && itemid != WP_AMMO ) { //----(SA)	modified

				// no weap currently selected, always just select the new one
				if ( !cg.weaponSelect ) {
					cg.weaponSelectTime = cg.time;
					cg.weaponSelect     = itemid;
				}
				// 1 - always switch to new weap (Q3A default)
				else if ( cg_autoswitch.integer == 1 ) {
					cg.weaponSelectTime = cg.time;
					cg.weaponSelect     = itemid;
				} else {

					// 2 - switch to weap if it's not already in the player's inventory (Wolf default)
					// 4 - both 2 and 3

					// FIXME:	this works fine for predicted pickups (when you walk over the weapon), but not for
					//			manual pickups (activate item)
					if ( cg_autoswitch.integer == 2 || cg_autoswitch.integer == 4 ) {
						if ( !COM_BitCheck( cg.snap->ps.weapons, itemid ) ) {
							cg.weaponSelectTime = cg.time;
							cg.weaponSelect     = itemid;
						}
					}   // end 2

					// 3 - switch to weap if it's in a bank greater than the current weap
					// 4 - both 2 and 3
					if ( cg_autoswitch.integer == 3 || cg_autoswitch.integer == 4 ) {
						// switch away only if a primary weapon is selected (read: don't switch away if current weap is a secondary mode)
						if ( CG_WeaponIndex( cg.weaponSelect, &wpbank_cur, NULL ) ) {
							if ( CG_WeaponIndex( itemid, &wpbank_pickup, NULL ) ) {
								if ( wpbank_pickup > wpbank_cur ) {
									cg.weaponSelectTime = cg.time;
									cg.weaponSelect     = itemid;
								}
							}
						}
					}   // end 3

				}   // end cg_autoswitch.integer != 1

			}   // end itemid != WP_SNIPERRIFLE && ...

		}   // end cg_autoswitch.integer

	}   // end bg_itemlist[itemNum].giType == IT_WEAPON


	//if (bg_itemlist[itemNum].giType == IT_HOLDABLE ) {
	//cg.holdableSelectTime	= cg.time;	// show holdables when a new one is picked up
	//cg.holdableSelect		= itemid;	// and select the new one
	//}

}


/*
================
CG_PainEvent

Also called by playerstate transition
================
*/
typedef struct {
	char *tag;
	int refEntOfs;
	int anim;
} painAnimForTag_t;

#define PEFOFS( x ) ( (int)&( ( (playerEntity_t *)0 )->x ) )

void CG_PainEvent( centity_t *cent, int health, qboolean crouching ) {
	char    *snd;

	#define STUNNED_ANIM    BOTH_PAIN8
	painAnimForTag_t tagAnims[] = {
		{"tag_head", PEFOFS( torsoRefEnt ),    BOTH_PAIN1},
		{"tag_chest",    PEFOFS( torsoRefEnt ),    BOTH_PAIN2},
		{"tag_groin",    PEFOFS( legsRefEnt ),     BOTH_PAIN3},
		{"tag_armright",PEFOFS( torsoRefEnt ), BOTH_PAIN4},
		{"tag_armleft",  PEFOFS( torsoRefEnt ),    BOTH_PAIN5},
		{"tag_legright",PEFOFS( legsRefEnt ),      BOTH_PAIN6},
		{"tag_legleft",  PEFOFS( legsRefEnt ),     BOTH_PAIN7},
		{NULL,0,0},
	};
	vec3_t tagOrg;
	int tagIndex, bestTag, oldPainAnim;
	float bestDist, dist;

	// Rafael
	if ( cent->currentState.aiChar && cgs.gametype == GT_SINGLE_PLAYER ) {

		if ( cent->pe.painTime > cg.time - 1000 ) {
			oldPainAnim = cent->pe.painAnimTorso;
		} else {
			oldPainAnim = -1;
		}

		// Ridah, health is actually time to spend playing the animation
		cent->pe.painTime = cg.time;
		cent->pe.painDuration = health << 4;
		cent->pe.painDirection ^= 1;
		cent->pe.painAnimLegs = -1;
		cent->pe.painAnimTorso = -1;

		if ( VectorLength( cent->currentState.origin2 ) > 1 ) {
			// find a correct animation to play, based on the body orientation at previous frame
			for ( tagIndex = 0, bestDist = 0, bestTag = -1; tagAnims[tagIndex].tag; tagIndex++ ) {
				if ( oldPainAnim >= 0 && tagAnims[tagIndex].anim == oldPainAnim ) {
					continue;
				}
				// grab the tag with this name
				if ( CG_GetOriginForTag( cent, ( refEntity_t * )( ( (byte *)&cent->pe ) + tagAnims[tagIndex].refEntOfs ), tagAnims[tagIndex].tag, 0, tagOrg, NULL ) >= 0 ) {
					dist = VectorDistance( tagOrg, cent->currentState.origin2 );
					if ( !bestDist || dist < bestDist ) {
						bestTag = tagIndex;
						bestDist = dist;
					}
				}
			}

			if ( bestTag >= 0 ) {
				if ( !crouching ) {
					cent->pe.painAnimLegs = tagAnims[bestTag].anim;
				}
				cent->pe.painAnimTorso = tagAnims[bestTag].anim;
			}
		}

		if ( cent->pe.painAnimTorso < 0 && cent->pe.painDuration > 1000 ) {   // stunned
			if ( !crouching ) {
				cent->pe.painAnimLegs = STUNNED_ANIM;
			}
			cent->pe.painAnimTorso = STUNNED_ANIM;
		}

		if ( cent->pe.painAnimTorso < 0 ) {
			// pick a random anim
			for ( tagIndex = 0; tagAnims[tagIndex].tag; tagIndex++ ) {};
			bestTag = rand() % tagIndex;
			if ( !crouching ) {
				cent->pe.painAnimLegs = tagAnims[bestTag].anim;
			}
			cent->pe.painAnimTorso = tagAnims[bestTag].anim;
		}

		// adjust the animation speed
		{
			animation_t *anim;
			clientInfo_t *ci;

			ci = &cgs.clientinfo[ cent->currentState.number ];
			anim = &ci->modelInfo->animations[ cent->pe.painAnimTorso ];

			cent->pe.animSpeed = ( anim->frameLerp * anim->numFrames ) / (float)cent->pe.painDuration;
		}

		return;
	}

	// don't do more than two pain sounds a second
	if ( cg.time - cent->pe.painTime < 500 ) {
		return;
	}

	if ( health < 25 ) {
		snd = "*pain25_1.wav";
	} else if ( health < 50 ) {
		snd = "*pain50_1.wav";
	} else if ( health < 75 ) {
		snd = "*pain75_1.wav";
	} else {
		snd = "*pain100_1.wav";
	}
	trap_S_StartSound( NULL, cent->currentState.number, CHAN_VOICE,
					   CG_CustomSound( cent->currentState.number, snd ) );

	// save pain time for programitic twitch animation
	cent->pe.painTime = cg.time;
	cent->pe.painDirection ^= 1;
}





/*
==============
CG_Explode


	if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])

==============
*/

#define POSSIBLE_PIECES 6




void CG_Explodef( vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader );

/*
==============
CG_Explode
	the old cent-based explode calls will still work with this pass-through
==============
*/
void CG_Explode( centity_t *cent, vec3_t origin, vec3_t dir, qhandle_t shader ) {

	qhandle_t inheritmodel = 0;

	// inherit shader
	// (SA) FIXME: do this at spawn time rather than explode time so any new necessary shaders are created earlier
	if ( cent->currentState.eFlags & EF_INHERITSHADER ) {
		if ( !shader ) {
//			inheritmodel = cent->currentState.modelindex;
			inheritmodel = cgs.inlineDrawModel[cent->currentState.modelindex];  // okay, this should be better.
			if ( inheritmodel ) {
				shader = trap_R_GetShaderFromModel( inheritmodel, 0, 0 );
			}
		}
	}


	CG_Explodef(    origin,
					dir,
					cent->currentState.density,         // mass
					cent->currentState.frame,           // type
					cent->currentState.dl_intensity,    // sound
					cent->currentState.weapon,          // forceLowGrav
					shader
					);

}


/*
==============
CG_Explodef
	made this more generic for spawning hits and breaks without needing a *cent
==============
*/
void CG_Explodef( vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader ) {
	int i;
	localEntity_t   *le;
	refEntity_t     *re;
	int howmany, total;
	int pieces[6];              // how many of each piece
	qhandle_t modelshader = 0;
	float materialmul = 1;              // multiplier for different types

	memset( &pieces, 0, sizeof( pieces ) );

	pieces[5]   = (int)( mass / 250.0f );
	pieces[4]   = (int)( mass / 76.0f );
	pieces[3]   = (int)( mass / 37.0f );  // so 2 per 75
	pieces[2]   = (int)( mass / 15.0f );
	pieces[1]   = (int)( mass / 10.0f );
	pieces[0]   = (int)( mass / 5.0f );

	if ( pieces[0] > 20 ) {
		pieces[0] = 20;                 // cap some of the smaller bits so they don't get out of control
	}
	if ( pieces[1] > 15 ) {
		pieces[1] = 15;
	}
	if ( pieces[2] > 10 ) {
		pieces[2] = 10;
	}

	if ( type == 0 ) {    // cap wood even more since it's often grouped, and the small splinters can add up
		if ( pieces[0] > 10 ) {
			pieces[0] = 10;
		}
		if ( pieces[1] > 10 ) {
			pieces[1] = 10;
		}
		if ( pieces[2] > 10 ) {
			pieces[2] = 10;
		}
	}

	total = pieces[5] + pieces[4] + pieces[3] + pieces[2] + pieces[1] + pieces[0];

	if ( sound ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.gameSounds[sound] );
	}

	if ( shader ) { // shader passed in to use
		modelshader = shader;
	}

	for ( i = 0; i < POSSIBLE_PIECES; i++ ) {
		leBounceSoundType_t snd = LEBS_NONE;
		int hmodel = 0;
		float scale;
		int endtime;
		for ( howmany = 0; howmany < pieces[i]; howmany++ ) {

			scale = 1.0f;
			endtime = 0;    // set endtime offset for faster/slower fadeouts

			switch ( type ) {
			case 0: // "wood"
				snd = LEBS_WOOD;
				hmodel = cgs.media.debWood[i];

				if ( i == 0 ) {
					scale = 0.5f;
				} else if ( i == 1 ) {
					scale = 0.6f;
				} else if ( i == 2 )                           {
					scale = 0.7f;
				} else if ( i == 3 )                                                               {
					scale = 0.5f;
				}
				//					else
				//						scale = cg_forceModel.value;
				//					else goto pass;

				if ( i < 3 ) {
					endtime = -3000;    // small bits live 3 sec shorter than normal
				}
				break;

			case 1: // "glass"
				snd = LEBS_NONE;
				if ( i == 5 ) {
					hmodel = cgs.media.shardGlass1;
				} else if ( i == 4 ) {
					hmodel = cgs.media.shardGlass2;
				} else if ( i == 2 )                                             {
					hmodel = cgs.media.shardGlass2;
				} else if ( i == 1 )                                                                                                   {
					hmodel = cgs.media.shardGlass2;
					scale = 0.5f;
				} else {goto pass;}
				break;

			case 2: // "metal"
				snd = LEBS_BRASS;
				if ( i == 5 ) {
					hmodel = cgs.media.shardMetal1;
				} else if ( i == 4 ) {
					hmodel = cgs.media.shardMetal2;
				} else if ( i == 2 )                                             {
					hmodel = cgs.media.shardMetal2;
				} else if ( i == 1 )                                                                                                   {
					hmodel = cgs.media.shardMetal2;
					scale = 0.5f;
				} else {goto pass;}
				break;

			case 3: // "gibs"
				snd = LEBS_BLOOD;
				if ( i == 5 ) {
					hmodel = cgs.media.gibIntestine;
				} else if ( i == 4 ) {
					hmodel = cgs.media.gibLeg;
				} else if ( i == 2 )                                        {
					hmodel = cgs.media.gibChest;
				} else { goto pass;}
				break;

			case 4: // "brick"
				snd = LEBS_ROCK;
				hmodel = cgs.media.debBlock[i];
				break;

			case 5: // "rock"
				snd = LEBS_ROCK;
				if ( i == 5 ) {
					hmodel = cgs.media.debRock[2];                  // temporarily use the next smallest rock piece
				} else if ( i == 4 )                                                              {
					hmodel = cgs.media.debRock[2];
				} else if ( i == 3 )                                                                                                                   {
					hmodel = cgs.media.debRock[1];
				} else if ( i == 2 )                                                                                                                                                                        {
					hmodel = cgs.media.debRock[0];
				} else if ( i == 1 )                                                                                                                                                                                                                             {
					hmodel = cgs.media.debBlock[1];                 // temporarily use the small block pieces
				} else { hmodel = cgs.media.debBlock[0];            // temporarily use the small block pieces
				}
				if ( i <= 2 ) {
					endtime = -2000;    // small bits live 2 sec shorter than normal
				}
				break;

			case 6: // "fabric"
				if ( i == 5 ) {
					hmodel = cgs.media.debFabric[0];
				} else if ( i == 4 ) {
					hmodel = cgs.media.debFabric[1];
				} else if ( i == 2 )                                              {
					hmodel = cgs.media.debFabric[2];
				} else if ( i == 1 )                                                                                                     {
					hmodel = cgs.media.debFabric[2];
					scale = 0.5;
				} else {goto pass;  // (only do 5, 4, 2 and 1)
				}
				break;
			}

			le = CG_AllocLocalEntity();
			re = &le->refEntity;

			le->leType              = LE_FRAGMENT;
			le->startTime           = cg.time;

			le->endTime             = ( le->startTime + 5000 + random() * 5000 ) + endtime;

			// as it turns out, i'm not sure if setting the re->axis here will actually do anything
			//			AxisClear(re->axis);
			//			re->axis[0][0] =
			//			re->axis[1][1] =
			//			re->axis[2][2] = scale;
			//
			//			if(scale != 1.0)
			//				re->nonNormalizedAxes = qtrue;

			le->sizeScale = scale;

			if ( type == 1 ) { // glass
				// Rafael added this because glass looks funky when it fades out
				// TBD: need to look into this so that they fade out correctly
				re->fadeStartTime       = le->endTime;
				re->fadeEndTime         = le->endTime;
			} else {
				re->fadeStartTime       = le->endTime - 4000;
				re->fadeEndTime         = le->endTime;
			}


			le->lifeRate    = 1.0 / ( le->endTime - le->startTime );
			le->leFlags     = LEF_TUMBLE;
			le->leMarkType  = 0;

			VectorCopy( origin, re->origin );
			AxisCopy( axisDefault, re->axis );

			le->leBounceSoundType = snd;
			re->hModel = hmodel;

			// inherit shader
			if ( modelshader ) {
				re->customShader = modelshader;
			}

			re->radius = 1000;

			// trying to make this a little more interesting
			if ( type == 6 ) { // "fabric"
				le->pos.trType = TR_GRAVITY_FLOAT;  // the fabric stuff will change to use something that looks better
			} else {
				if ( !forceLowGrav && rand() & 1 ) {    // if low gravity is not forced and die roll goes our way use regular grav
					le->pos.trType = TR_GRAVITY;
				} else {
					le->pos.trType = TR_GRAVITY_LOW;
				}
			}

			switch ( type ) {
			case 6: // fabric
				le->bounceFactor    = 0.0;
				materialmul         = 0.3;  // rotation speed
				break;
			default:
				le->bounceFactor    = 0.4;
				break;
			}


			// rotation
			le->angles.trType = TR_LINEAR;
			le->angles.trTime = cg.time;
			le->angles.trBase[0] = rand() & 31;
			le->angles.trBase[1] = rand() & 31;
			le->angles.trBase[2] = rand() & 31;
			le->angles.trDelta[0] = ( ( 100 + ( rand() & 500 ) ) - 300 ) * materialmul;
			le->angles.trDelta[1] = ( ( 100 + ( rand() & 500 ) ) - 300 ) * materialmul;
			le->angles.trDelta[2] = ( ( 100 + ( rand() & 500 ) ) - 300 ) * materialmul;


			//			if(type == 6)	// fabric
			//				materialmul = 1;		// translation speed


			VectorCopy( origin, le->pos.trBase );
			VectorNormalize( dir );
			le->pos.trTime = cg.time;

			// (SA) hoping that was just intended to represent randomness
			//			if (cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2])
			if ( le->angles.trBase[0] == 1 || le->angles.trBase[1] == 1 || le->angles.trBase[2] == 1 ) {
				le->pos.trType = TR_GRAVITY;
				VectorScale( dir, 10 * 8, le->pos.trDelta );
				le->pos.trDelta[0] += ( ( random() * 100 ) - 50 );
				le->pos.trDelta[1] += ( ( random() * 100 ) - 50 );
				le->pos.trDelta[2] = ( random() * 200 ) + 200;

			} else {
				// location
				VectorScale( dir, 200 + mass, le->pos.trDelta );
				le->pos.trDelta[0] += ( ( random() * 100 ) - 50 );
				le->pos.trDelta[1] += ( ( random() * 100 ) - 50 );

				if ( dir[2] ) {
					le->pos.trDelta[2] = random() * 200 * materialmul;  // randomize sort of a lot so they don't all land together
				} else {
					le->pos.trDelta[2] = random() * 20;
				}
			}
		}
pass:
		continue;
	}

}


/*
==============
CG_Effect
	Quake ed -> target_effect (0 .5 .8) (-6 -6 -6) (6 6 6) fire explode smoke debris gore lowgrav
==============
*/
void CG_Effect( centity_t *cent, vec3_t origin, vec3_t dir ) {
	localEntity_t   *le;
	refEntity_t     *re;
//	int				howmany;
	int mass;
//	int				large, small;

	VectorSet( dir, 0, 0, 1 );    // straight up.

	mass = cent->currentState.density;

//		1 large per 100, 1 small per 24
//	large	= (int)(mass / 100);
//	small	= (int)(mass / 24) + 1;

	if ( cent->currentState.eventParm & 1 ) {  // fire
	}

	// (SA) right now force smoke on any explosions
//	if(cent->currentState.eventParm & 4)	// smoke
	if ( cent->currentState.eventParm & 6 ) {
		int i, j;
		vec3_t sprVel, sprOrg;
		// explosion sprite animation
		VectorScale( dir, 16, sprVel );
		for ( i = 0; i < 5; i++ ) {
			for ( j = 0; j < 3; j++ )
				sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
			sprVel[2] += rand() % 50;
//			CG_ParticleExplosion( 2, sprOrg, sprVel, 1000+rand()%250, 20, 40+rand()%60 );
			CG_ParticleExplosion( "blacksmokeanim", sprOrg, sprVel, 3500 + rand() % 250, 10, 250 + rand() % 60 ); // JPW NERVE was smokeanimb
		}
	}


	if ( cent->currentState.eventParm & 2 ) {  // explode
		vec3_t sprVel, sprOrg;
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.sfx_rockexp );

		// new explode	(from rl)
		VectorMA( origin, 16, dir, sprOrg );
		VectorScale( dir, 100, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 500, 20, 160 );
		//CG_ParticleExplosion( "blueexp", sprOrg, sprVel, 1200, 9, 300 );

		// (SA) this is done only if the level designer has it marked in the entity.
		//		(see "cent->currentState.eventParm & 64" below)

		// RF, throw some debris
//		CG_AddDebris( origin, dir,
//						280,	// speed
//						1400,	// duration
//						// 15 + rand()%5 );	// count
//						7 + rand()%2 );	// count

		CG_ImpactMark( cgs.media.burnMarkShader, origin, dir, random() * 360, 1,1,1,1, qfalse, 64, qfalse, 0xffffffff );
	}


	if ( cent->currentState.eventParm & 8 ) {  // rubble
		// share the cg_explode code with func_explosives
		const char *s;
		qhandle_t sh = 0;   // shader handle

		vec3_t newdir = {0, 0, 0};

		if ( cent->currentState.angles2[0] || cent->currentState.angles2[1] || cent->currentState.angles2[2] ) {
			VectorCopy( cent->currentState.angles2, newdir );
		}

		s = CG_ConfigString( CS_TARGETEFFECT ); // see if ent has a shader specified
		if ( s && strlen( s ) > 0 ) {
			sh = trap_R_RegisterShader( va( "textures/%s", s ) );    // FIXME: don't do this here.  only for testing

		}
		cent->currentState.eFlags &= ~EF_INHERITSHADER; // don't try to inherit shader
		cent->currentState.dl_intensity = 0;        // no sound
		CG_Explode( cent, origin, newdir, sh );
//void CG_Explodef(vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader);
	}


	if ( cent->currentState.eventParm & 16 ) { // gore
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + 5000 + random() * 3000;
//----(SA)	fading out
		re->fadeStartTime       = le->endTime - 4000;
		re->fadeEndTime         = le->endTime;
//----(SA)	end

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );
		//	re->hModel = hModel;
		re->hModel = cgs.media.gibIntestine;
		le->pos.trType = TR_GRAVITY;
		VectorCopy( origin, le->pos.trBase );

		//	VectorCopy( velocity, le->pos.trDelta );
		VectorNormalize( dir );
		VectorMA( dir, 200, dir, le->pos.trDelta );

		le->pos.trTime = cg.time;

		le->bounceFactor = 0.3;

		le->leBounceSoundType = LEBS_BLOOD;
		le->leMarkType = LEMT_BLOOD;
	}


	if ( cent->currentState.eventParm & 64 ) { // debris trails (the black strip that Ryan did)
		CG_AddDebris( origin, dir,
					  280,      // speed
					  1400,     // duration
					  // 15 + rand()%5 );	// count
					  7 + rand() % 2 ); // count
	}
}






/*
CG_Shard

	We should keep this separate since there will be considerable differences
	in the physical properties of shard vrs debris. not to mention the fact
	there is no way we can quantify what type of effects the designers will
	potentially desire. If it is still possible to merge the functionality of
	cg_shard into cg_explode at a latter time I would have no problem with that
	but for now I want to keep it separate
*/
void CG_Shard( centity_t *cent, vec3_t origin, vec3_t dir ) {
	localEntity_t   *le;
	refEntity_t     *re;
	int type;
	int howmany;
	int i;
	int rval;

	qboolean isflyingdebris = qfalse;

	type = cent->currentState.density;
	howmany = cent->currentState.frame;

	for ( i = 0; i < howmany; i++ )
	{
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		le->leType              = LE_FRAGMENT;
		le->startTime           = cg.time;
		le->endTime             = le->startTime + 5000 + random() * 5000;

//----(SA)	fading out
		re->fadeStartTime       = le->endTime - 1000;
		re->fadeEndTime         = le->endTime;
//----(SA)	end

		if ( type == 999 ) {
			le->startTime           = cg.time;
			le->endTime             = le->startTime + 100;
			re->fadeStartTime       = le->endTime - 100;
			re->fadeEndTime         = le->endTime;
			type = 1;

			isflyingdebris = qtrue;
		}


		le->lifeRate            = 1.0 / ( le->endTime - le->startTime );
		le->leFlags             = LEF_TUMBLE;
		le->bounceFactor        = 0.4;
		// le->leBounceSoundType	= LEBS_WOOD;
		le->leMarkType          = 0;

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );

		rval = rand() % 2;

		if ( type == 0 ) { // glass
			if ( rval ) {
				re->hModel = cgs.media.shardGlass1;
			} else {
				re->hModel = cgs.media.shardGlass2;
			}
		} else if ( type == 1 )     { // wood
			if ( rval ) {
				re->hModel = cgs.media.shardWood1;
			} else {
				re->hModel = cgs.media.shardWood2;
			}
		} else if ( type == 2 )     { // metal
			if ( rval ) {
				re->hModel = cgs.media.shardMetal1;
			} else {
				re->hModel = cgs.media.shardMetal2;
			}
		} else if ( type == 3 )     { // ceramic
			if ( rval ) {
				re->hModel = cgs.media.shardCeramic1;
			} else {
				re->hModel = cgs.media.shardCeramic2;
			}
		} else if ( type == 4 )     { // rubble
			rval = rand() % 3;

			if ( rval == 1 ) {
				re->hModel = cgs.media.shardRubble1;
			} else if ( rval == 2 ) {
				re->hModel = cgs.media.shardRubble2;
			} else {
				re->hModel = cgs.media.shardRubble3;
			}

		} else {
			CG_Printf( "CG_Debris has an unknown type\n" );
		}

		// location
		if ( isflyingdebris ) {
			le->pos.trType = TR_GRAVITY_LOW;
		} else {
			le->pos.trType = TR_GRAVITY;
		}

		VectorCopy( origin, le->pos.trBase );
		VectorNormalize( dir );
		VectorScale( dir, 10 * howmany, le->pos.trDelta );
		le->pos.trTime = cg.time;
		le->pos.trDelta[0] += ( ( random() * 100 ) - 50 );
		le->pos.trDelta[1] += ( ( random() * 100 ) - 50 );
		if ( type ) {
			le->pos.trDelta[2] = ( random() * 200 ) + 100;    // randomize sort of a lot so they don't all land together
		} else { // glass
			le->pos.trDelta[2] = ( random() * 100 ) + 50; // randomize sort of a lot so they don't all land together

		}
		// rotation
		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand() & 31;
		le->angles.trBase[1] = rand() & 31;
		le->angles.trBase[2] = rand() & 31;
		le->angles.trDelta[0] = ( 100 + ( rand() & 500 ) ) - 300;
		le->angles.trDelta[1] = ( 100 + ( rand() & 500 ) ) - 300;
		le->angles.trDelta[2] = ( 100 + ( rand() & 500 ) ) - 300;

	}

}


void CG_ShardJunk( centity_t *cent, vec3_t origin, vec3_t dir ) {
	localEntity_t   *le;
	refEntity_t     *re;
	int type;

	type = cent->currentState.density;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType              = LE_FRAGMENT;
	le->startTime           = cg.time;
	le->endTime             = le->startTime + 5000 + random() * 5000;

	re->fadeStartTime       = le->endTime - 1000;
	re->fadeEndTime         = le->endTime;

	le->lifeRate            = 1.0 / ( le->endTime - le->startTime );
	le->leFlags             = LEF_TUMBLE;
	le->bounceFactor        = 0.4;
	le->leMarkType          = 0;

	VectorCopy( origin, re->origin );
	AxisCopy( axisDefault, re->axis );

	re->hModel = cgs.media.shardJunk[rand() % MAX_LOCKER_DEBRIS];

	le->pos.trType = TR_GRAVITY;

	VectorCopy( origin, le->pos.trBase );
	VectorNormalize( dir );
	VectorScale( dir, 10 * 8, le->pos.trDelta );
	le->pos.trTime = cg.time;
	le->pos.trDelta[0] += ( ( random() * 100 ) - 50 );
	le->pos.trDelta[1] += ( ( random() * 100 ) - 50 );

	le->pos.trDelta[2] = ( random() * 100 ) + 50; // randomize sort of a lot so they don't all land together

	// rotation
	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	//le->angles.trBase[0] = rand()&31;
	//le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand() & 31;

	//le->angles.trDelta[0] = (100 + (rand()&500)) - 300;
	//le->angles.trDelta[1] = (100 + (rand()&500)) - 300;
	le->angles.trDelta[2] = ( 100 + ( rand() & 500 ) ) - 300;

}

void CG_BatDeath( centity_t *cent ) {
	CG_ParticleExplosion( "blood", cent->lerpOrigin, vec3_origin, 400, 20, 30 );
}


/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
extern void CG_AddBulletParticles( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale );
// JPW NERVE
void CG_MachineGunEjectBrass( centity_t *cent );
void CG_MachineGunEjectBrassNew( centity_t *cent );
// jpw
#define DEBUGNAME( x ) if ( cg_debugEvents.integer ) {CG_Printf( x "\n" );}
void CG_EntityEvent( centity_t *cent, vec3_t position ) {
	entityState_t   *es;
	int event;
	vec3_t dir;
	const char      *s;
	int clientNum;
	clientInfo_t    *ci;
	char tempStr[MAX_QPATH];

// JPW NERVE copied here for mg42 SFX event
	vec3_t porg, gorg, norm;    // player/gun origin
	float gdist;
// jpw

	static int footstepcnt = 0;
	static int splashfootstepcnt = 0;

	es = &cent->currentState;
	event = es->event & ~EV_EVENT_BITS;

	if ( cg_debugEvents.integer ) {
		CG_Printf( "ent:%3i  event:%3i ", es->number, event );
	}

	if ( !event ) {
		DEBUGNAME( "ZEROEVENT" );
		return;
	}

	clientNum = es->clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	if ( cgs.gametype == GT_SINGLE_PLAYER && !ci->modelInfo ) {   // not ready yet?
		return;
	}

	switch ( event ) {
		//
		// movement generated events
		//
	case EV_FOOTSTEP:
		DEBUGNAME( "EV_FOOTSTEP" );
		if ( cg_footsteps.integer ) {
			if ( cent->currentState.aiChar == AICHAR_ELITEGUARD ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_ELITE_STEP ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_ZOMBIE ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_ZOMBIE_STEP ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_LOPER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_LOPER_STEP ][footstepcnt] );
			} else if ( cent->currentState.aiChar >= AICHAR_STIMSOLDIER1 && cent->currentState.aiChar <= AICHAR_STIMSOLDIER3 ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_STEP ][footstepcnt] );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ ci->modelInfo->footsteps ][footstepcnt] );
			}
		}
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME( "EV_FOOTSTEP_METAL" );
		if ( cg_footsteps.integer ) {
			if ( cent->currentState.aiChar == AICHAR_ELITEGUARD ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_ELITE_METAL ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_LOPER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_LOPER_METAL ][footstepcnt] );
			} else if ( cent->currentState.aiChar >= AICHAR_STIMSOLDIER1 && cent->currentState.aiChar <= AICHAR_STIMSOLDIER3 ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_METAL ][footstepcnt] );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_METAL ][footstepcnt] );
			}
		}
		break;
	case EV_FOOTSTEP_WOOD:
		DEBUGNAME( "EV_FOOTSTEP_WOOD" );
		if ( cg_footsteps.integer ) {
			if ( cent->currentState.aiChar == AICHAR_ELITEGUARD ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_ELITE_WOOD ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_ZOMBIE ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_ZOMBIE_WOOD ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_LOPER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_LOPER_WOOD ][footstepcnt] );
			} else if ( cent->currentState.aiChar >= AICHAR_STIMSOLDIER1 && cent->currentState.aiChar <= AICHAR_STIMSOLDIER3 ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_WOOD ][footstepcnt] );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_WOOD ][footstepcnt] );
			}

		}
		break;
	case EV_FOOTSTEP_GRASS:
		DEBUGNAME( "EV_FOOTSTEP_GRASS" );
		if ( cg_footsteps.integer ) {
			//if (cent->currentState.aiChar == AICHAR_ELITEGUARD)
			//	trap_S_StartSound (NULL, es->number, CHAN_BODY,
			//		cgs.media.footsteps[ FOOTSTEP_ELITE_STEP ][footstepcnt] );
			//else
			if ( cent->currentState.aiChar >= AICHAR_STIMSOLDIER1 && cent->currentState.aiChar <= AICHAR_STIMSOLDIER3 ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_GRASS ][footstepcnt] );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_GRASS ][footstepcnt] );
			}
		}
		break;
	case EV_FOOTSTEP_GRAVEL:
		DEBUGNAME( "EV_FOOTSTEP_GRAVEL" );
		if ( cg_footsteps.integer ) {
			if ( cent->currentState.aiChar == AICHAR_ELITEGUARD ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_ELITE_GRAVEL ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_ZOMBIE ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_ZOMBIE_GRAVEL ][footstepcnt] );
			} else if ( cent->currentState.aiChar >= AICHAR_STIMSOLDIER1 && cent->currentState.aiChar <= AICHAR_STIMSOLDIER3 ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_GRAVEL][footstepcnt] );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY,
								   cgs.media.footsteps[ FOOTSTEP_GRAVEL ][footstepcnt] );
			}
		}
		break;

	case EV_FOOTSTEP_ROOF:  // tile sound
		DEBUGNAME( "EV_FOOTSTEP_ROOF" );
		if ( cg_footsteps.integer ) {
			//if (cent->currentState.aiChar == AICHAR_ELITEGUARD)
			//	trap_S_StartSound (NULL, es->number, CHAN_BODY,
			//		cgs.media.footsteps[ FOOTSTEP_ELITE_ROOF ][footstepcnt] );
			//else
			trap_S_StartSound( NULL, es->number, CHAN_BODY,
							   cgs.media.footsteps[ FOOTSTEP_ROOF ][footstepcnt] );
		}
		break;
	case EV_FOOTSTEP_SNOW:
		DEBUGNAME( "EV_FOOTSTEP_SNOW" );
		if ( cg_footsteps.integer ) {
			//if (cent->currentState.aiChar == AICHAR_ELITEGUARD)
			//	trap_S_StartSound (NULL, es->number, CHAN_BODY,
			//		cgs.media.footsteps[ FOOTSTEP_ELITE_STEP ][footstepcnt] );
			//else
			trap_S_StartSound( NULL, es->number, CHAN_BODY,
							   cgs.media.footsteps[ FOOTSTEP_SNOW ][footstepcnt] );
		}
		break;

//----(SA)	added
	case EV_FOOTSTEP_CARPET:
		DEBUGNAME( "EV_FOOTSTEP_CARPET" );
		if ( cg_footsteps.integer ) {
			trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_CARPET ][footstepcnt] );
		}
		break;


//----(SA)	end

	case EV_FOOTSPLASH:
		DEBUGNAME( "EV_FOOTSPLASH" );
		if ( cg_footsteps.integer ) {
			trap_S_StartSound( NULL, es->number, CHAN_BODY,
							   cgs.media.footsteps[ FOOTSTEP_SPLASH ][splashfootstepcnt] );
		}
		break;
	case EV_FOOTWADE:
		DEBUGNAME( "EV_FOOTWADE" );
		if ( cg_footsteps.integer ) {
			trap_S_StartSound( NULL, es->number, CHAN_BODY,
							   cgs.media.footsteps[ FOOTSTEP_SPLASH ][splashfootstepcnt] );
		}
		break;
	case EV_SWIM:
		DEBUGNAME( "EV_SWIM" );
		if ( cg_footsteps.integer ) {
			trap_S_StartSound( NULL, es->number, CHAN_BODY,
							   cgs.media.footsteps[ FOOTSTEP_SPLASH ][footstepcnt] );
		}
		break;


	case EV_FALL_SHORT:
		DEBUGNAME( "EV_FALL_SHORT" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.landSound );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -8;
			cg.landTime = cg.time;
		}
		break;
/*
	case EV_FALL_MEDIUM:
		DEBUGNAME("EV_FALL_MEDIUM");
		// use normal pain sound trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_FAR:
		DEBUGNAME("EV_FALL_FAR");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;
*/
	case EV_FALL_DMG_10:
		DEBUGNAME( "EV_FALL_DMG_10" );
		// use normal pain sound trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "sound/multiplayer/land_hurt.wav" ) ); // JPW NERVE
		cent->pe.painTime = cg.time;    // don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_DMG_15:
		DEBUGNAME( "EV_FALL_DMG_15" );
		// use normal pain sound trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "sound/multiplayer/land_hurt.wav" ) ); // JPW NERVE
		cent->pe.painTime = cg.time;    // don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
		}
		break;
	case EV_FALL_DMG_25:
		DEBUGNAME( "EV_FALL_DMG_25" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "sound/multiplayer/land_hurt.wav" ) ); // JPW NERVE
		cent->pe.painTime = cg.time;    // don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;
/*
	case EV_FALL_DMG_30:
		DEBUGNAME("EV_FALL_DMG_30");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;
*/
	case EV_FALL_DMG_50:
		DEBUGNAME( "EV_FALL_DMG_50" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "sound/multiplayer/land_hurt.wav" ) ); // JPW NERVE
		cent->pe.painTime = cg.time;    // don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;
/*
	case EV_FALL_DMG_75:
		DEBUGNAME("EV_FALL_DMG_75");
		trap_S_StartSound (NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		cent->pe.painTime = cg.time;	// don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
		}
		break;
*/
	case EV_FALL_NDIE:
		DEBUGNAME( "EV_FALL_NDIE" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "sound/multiplayer/land_hurt.wav" ) ); // JPW NERVE
		cent->pe.painTime = cg.time;    // don't play a pain sound right after this
		// splat
		break;

	case EV_EXERT1:
		DEBUGNAME( "EV_EXERT1" );
		trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*exert1.wav" ) );
		break;
	case EV_EXERT2:
		DEBUGNAME( "EV_EXERT2" );
		trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*exert2.wav" ) );
		break;
	case EV_EXERT3:
		DEBUGNAME( "EV_EXERT3" );
		trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*exert3.wav" ) );
		break;

	case EV_STEP_4:
	case EV_STEP_8:
	case EV_STEP_12:
	case EV_STEP_16:        // smooth out step up transitions
		DEBUGNAME( "EV_STEP" );
		{
			float oldStep;
			int delta;
			int step;

			if ( clientNum != cg.predictedPlayerState.clientNum ) {
				break;
			}
			// if we are interpolating, we don't need to smooth steps
			if ( cg.demoPlayback || ( cg.snap->ps.pm_flags & PMF_FOLLOW ) ||
				 cg_nopredict.integer || cg_synchronousClients.integer ) {
				break;
			}
			// check for stepping up before a previous step is completed
			delta = cg.time - cg.stepTime;
			if ( delta < STEP_TIME ) {
				oldStep = cg.stepChange * ( STEP_TIME - delta ) / STEP_TIME;
			} else {
				oldStep = 0;
			}

			// add this amount
			step = 4 * ( event - EV_STEP_4 + 1 );
			cg.stepChange = oldStep + step;
			if ( cg.stepChange > MAX_STEP_CHANGE ) {
				cg.stepChange = MAX_STEP_CHANGE;
			}
			cg.stepTime = cg.time;
			break;
		}

	case EV_JUMP_PAD:
		DEBUGNAME( "EV_JUMP_PAD" );
		// boing sound at origin, jump sound on player
		trap_S_StartSound( cent->lerpOrigin, -1, CHAN_VOICE, cgs.media.jumpPadSound );
		trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;

	case EV_JUMP:
		DEBUGNAME( "EV_JUMP" );
		trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*jump1.wav" ) );
		break;
	case EV_TAUNT:
		DEBUGNAME( "EV_TAUNT" );
		trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*taunt.wav" ) );
		break;
	case EV_WATER_TOUCH:
		DEBUGNAME( "EV_WATER_TOUCH" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrInSound );
		break;
	case EV_WATER_LEAVE:
		DEBUGNAME( "EV_WATER_LEAVE" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrOutSound );
		break;
	case EV_WATER_UNDER:
		DEBUGNAME( "EV_WATER_UNDER" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.watrUnSound );
//----(SA)	this fog stuff for underwater is really just a test for feasibility of creating the under-water effect that way.
//----(SA)	the related issues of load/savegames, death underwater, etc. are not handled at all.
//----(SA)	the actual problem, of course, is doing underwater stuff when the water is very turbulant and you can't simply
//----(SA)	do things based on the players head being above/below the water brushes top surface. (since the waves can potentially be /way/ above/below that)

		// DHM - Nerve :: causes problems in multiplayer...
		if ( cgs.gametype == GT_SINGLE_PLAYER && clientNum == cg.predictedPlayerState.clientNum ) {
//			trap_R_SetFog(FOG_WATER, 0, 400, .1, .1, .1, 111);
			trap_R_SetFog( FOG_CMD_SWITCHFOG, FOG_WATER, 200, 0, 0, 0, 0 );
		}
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME( "EV_WATER_CLEAR" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*gasp.wav" ) );

		// DHM - Nerve :: causes problems in multiplayer...
		if ( cgs.gametype == GT_SINGLE_PLAYER && clientNum == cg.predictedPlayerState.clientNum ) {
			trap_R_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP, 400,0,0,0,0 );
		}
		break;

	case EV_ITEM_PICKUP:
	case EV_ITEM_PICKUP_QUIET:
		DEBUGNAME( "EV_ITEM_PICKUP" );
		{
			gitem_t *item;
			int index;

			index = es->eventParm;      // player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];

			if ( event == EV_ITEM_PICKUP ) { // not quiet
				// powerups and team items will have a separate global sound, this one
				// will be played at prediction time
				if ( item->giType == IT_POWERUP || item->giType == IT_TEAM ) {
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, trap_S_RegisterSound( "sound/misc/w_pkup.wav" ) );
				} else {
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, trap_S_RegisterSound( item->pickup_sound ) );
				}
			}

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}

//----(SA)	draw the HUD items for a sec since this is a special item
			if ( item->giType == IT_KEY ) {
				cg.itemFadeTime = cg.time + 1000;
			}

		}
		break;

	case EV_GLOBAL_ITEM_PICKUP:
		DEBUGNAME( "EV_GLOBAL_ITEM_PICKUP" );
		{
			gitem_t *item;
			int index;

			index = es->eventParm;      // player predicted

			if ( index < 1 || index >= bg_numItems ) {
				break;
			}
			item = &bg_itemlist[ index ];
			// powerup pickups are global
			trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, trap_S_RegisterSound( item->pickup_sound ) );

			// show icon and name on status bar
			if ( es->number == cg.snap->ps.clientNum ) {
				CG_ItemPickup( index );
			}
		}
		break;

		//
		// weapon events
		//
	case EV_VENOM:
		DEBUGNAME( "EV_VENOM" );
		CG_VenomFire( es, qfalse );
		break;
	case EV_VENOMFULL:
		DEBUGNAME( "EV_VENOMFULL" );
		CG_VenomFire( es, qtrue );
		break;

	case EV_NOITEM:
		DEBUGNAME( "EV_NOITEM" );
		if ( es->number == cg.snap->ps.clientNum ) {
			CG_HoldableUsedupChange();
		}
		break;

	case EV_WEAP_OVERHEAT:
		DEBUGNAME( "EV_WEAP_OVERHEAT" );

		// start weapon idle animation
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.predictedPlayerState.weapAnim = ( ( cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | WEAP_IDLE1;
			cent->overheatTime = cg.time;   // used to make the barrels smoke when overheated
		}

		if ( cg_weapons[es->weapon].overheatSound ) {
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cg_weapons[es->weapon].overheatSound );
		}
		break;

// JPW NERVE
	case EV_SPINUP:
		DEBUGNAME( "EV_SPINUP" );
		if ( cg_gameType.integer != GT_SINGLE_PLAYER ) {
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cg_weapons[es->weapon].spinupSound );
		}
		break;
// jpw
	case EV_EMPTYCLIP:
		DEBUGNAME( "EV_EMPTYCLIP" );
		break;

	case EV_FILL_CLIP:
		DEBUGNAME( "EV_FILL_CLIP" );
		if ( cg_weapons[es->weapon].reloadSound ) {
			trap_S_StartSound( NULL, es->number, CHAN_WEAPON, cg_weapons[es->weapon].reloadSound ); // JPW NERVE following sherman's SP fix, should allow killing reload sound when player dies
		}
		break;

// JPW NERVE play a sound when engineer fixes MG42
	case EV_MG42_FIXED:
		DEBUGNAME( "EV_MG42_FIXED" );
		trap_S_StartSound( NULL,es->number,CHAN_WEAPON,cg_weapons[WP_MAUSER].reloadSound );
		break;
// jpw

	case EV_NOAMMO:
		DEBUGNAME( "EV_NOAMMO" );
		if ( ( es->weapon != WP_GRENADE_LAUNCHER ) && ( es->weapon != WP_GRENADE_PINEAPPLE ) && ( es->weapon != WP_DYNAMITE )  && ( es->weapon != WP_DYNAMITE2 ) ) {
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.noAmmoSound );
		}
		if ( es->number == cg.snap->ps.clientNum ) {
			CG_OutOfAmmoChange();
		}
		break;
	case EV_CHANGE_WEAPON:
	{

		int newweap = 0;

		DEBUGNAME( "EV_CHANGE_WEAPON" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.selectSound );

		// client will get this message if reloading while using an alternate weapon
		// client should voluntarily switch back to primary at that point
		switch ( es->weapon ) {
		case WP_SNOOPERSCOPE:
			newweap = WP_GARAND;
			break;
		case WP_SNIPERRIFLE:
			newweap = WP_MAUSER;
			break;
		case WP_FG42SCOPE:
			newweap = WP_FG42;
			break;
		default:
			break;
		}

		if ( ( newweap ) && ( cgs.gametype < GT_WOLF ) ) {    // NERVE - SMF - we don't want this in multiplayer
			CG_FinishWeaponChange( es->weapon, newweap );
		}

	}
	break;

	case EV_FIRE_WEAPON_MG42:
		//trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, hWeaponSnd );
// JPW NERVE -- nasty kludge because there's no WP_MG42 struct to hold echosound, so we pull it from GM's predefined globals hweaponSnd & hEchoweaponsnd
		VectorCopy( cent->currentState.pos.trBase, gorg );
		VectorCopy( cg.refdef.vieworg, porg );
		VectorSubtract( gorg, porg, norm );
		gdist = VectorNormalize( norm );
		if ( gdist > 512 && gdist < 4096 ) {
			VectorMA( cg.refdef.vieworg, 64, norm, gorg );
			trap_S_StartSoundEx( gorg, cent->currentState.number, CHAN_WEAPON, hWeaponEchoSnd, SND_NOCUT );
		}
// jpw
		DEBUGNAME( "EV_FIRE_WEAPON" );
		CG_FireWeapon( cent );
		break;
	case EV_FIRE_WEAPON:
// JPW NERVE
		if ( cg.snap->ps.eFlags & EF_ZOOMING ) { // to stop airstrike sfx
			break;
		}
// jpw
	case EV_FIRE_WEAPONB:
		DEBUGNAME( "EV_FIRE_WEAPON" );
		CG_FireWeapon( cent );
		break;
	case EV_FIRE_WEAPON_LASTSHOT:
		DEBUGNAME( "EV_FIRE_WEAPON_LASTSHOT" );
		CG_FireWeapon( cent );
		break;

//----(SA)	added
	case EV_FIRE_QUICKGREN:
		// testing.  no client side effect yet
		break;
//----(SA)	end
//----(SA)	added
	case EV_NOFIRE_UNDERWATER:
		DEBUGNAME( "EV_NOFIRE_UNDERWATER" );
		if ( cgs.media.noFireUnderwater ) {
			trap_S_StartSound( NULL, es->number, CHAN_WEAPON, cgs.media.noFireUnderwater );
		}
		break;
//----(SA)	end
	case EV_USE_ITEM0:
		DEBUGNAME( "EV_USE_ITEM0" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM1:
		DEBUGNAME( "EV_USE_ITEM1" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM2:
		DEBUGNAME( "EV_USE_ITEM2" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM3:
		DEBUGNAME( "EV_USE_ITEM3" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM4:
		DEBUGNAME( "EV_USE_ITEM4" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM5:
		DEBUGNAME( "EV_USE_ITEM5" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM6:
		DEBUGNAME( "EV_USE_ITEM6" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM7:
		DEBUGNAME( "EV_USE_ITEM7" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM8:
		DEBUGNAME( "EV_USE_ITEM8" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM9:
		DEBUGNAME( "EV_USE_ITEM9" );
		CG_UseItem( cent );
		break;
// JPW NERVE -- this looks reasonable
	case EV_TESTID1:
		cg_fxflags |= 2;
		break;
	case EV_TESTID2:
		cg_fxflags |= 1;
		break;
	case EV_ENDTEST:
		cg_fxflags = 0;
		break;
// jpw
	case EV_USE_ITEM10:
		DEBUGNAME( "EV_USE_ITEM10" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM11:
		DEBUGNAME( "EV_USE_ITEM11" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM12:
		DEBUGNAME( "EV_USE_ITEM12" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM13:
		DEBUGNAME( "EV_USE_ITEM13" );
		CG_UseItem( cent );
		break;
	case EV_USE_ITEM14:
		DEBUGNAME( "EV_USE_ITEM14" );
		CG_UseItem( cent );
		break;

		//=================================================================

		//
		// other events
		//
	case EV_PLAYER_TELEPORT_IN:
		DEBUGNAME( "EV_PLAYER_TELEPORT_IN" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.teleInSound );
		CG_SpawnEffect( position );
		break;

	case EV_PLAYER_TELEPORT_OUT:
		DEBUGNAME( "EV_PLAYER_TELEPORT_OUT" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.teleOutSound );
		CG_SpawnEffect(  position );
		break;

	case EV_ITEM_POP:
		DEBUGNAME( "EV_ITEM_POP" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;
	case EV_ITEM_RESPAWN:
		DEBUGNAME( "EV_ITEM_RESPAWN" );
		cent->miscTime = cg.time;   // scale up from this
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.respawnSound );
		break;

	case EV_GRENADE_BOUNCE:
		DEBUGNAME( "EV_GRENADE_BOUNCE" );

		// DYNAMITE
		if ( es->weapon == WP_DYNAMITE || es->weapon == WP_DYNAMITE2 ) {
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.dynamitebounce1 );
		} else {
			// GRENADES
			if ( rand() & 1 ) {
				trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce1 );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_AUTO,  cgs.media.grenadebounce2 );
			}
		}
		break;

	case EV_FLAMEBARREL_BOUNCE:
		DEBUGNAME( "EV_FLAMEBARREL_BOUNCE" );
		if ( rand() & 1 ) {
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.fbarrelexp1 );
		} else {
			trap_S_StartSound( NULL, es->number, CHAN_AUTO,  cgs.media.fbarrelexp2 );
		}
		break;

	case EV_RAILTRAIL:
		// ev_railtrail is now sent standalone rather than by a player entity
		CG_RailTrail( &cgs.clientinfo[ es->otherEntityNum2 ], es->origin2, es->pos.trBase, es->dmgFlags );   //----(SA)	added 'type' field
		break;
		//
		// missile impacts
		//
	case EV_MISSILE_HIT:
		DEBUGNAME( "EV_MISSILE_HIT" );
		ByteToDir( es->eventParm, dir );
		CG_MissileHitPlayer( cent, es->weapon, position, dir, es->otherEntityNum );
		break;

	case EV_MISSILE_MISS_SMALL:
		DEBUGNAME( "EV_MISSILE_MISS" );
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWallSmall( es->weapon, 0, position, dir );
		break;

	case EV_MISSILE_MISS:
		DEBUGNAME( "EV_MISSILE_MISS" );
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWall( es->weapon, 0, position, dir, 0 );   // (SA) modified to send missilehitwall surface parameters
		break;

	case EV_MISSILE_MISS_LARGE:
		DEBUGNAME( "EV_MISSILE_MISS_LARGE" );
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWall( VERYBIGEXPLOSION, 0, position, dir, 0 );  // (SA) modified to send missilehitwall surface parameters
		break;

	case EV_SPIT_MISS:
	case EV_SPIT_HIT:
		DEBUGNAME( "EV_SPIT_MISS" );
		ByteToDir( es->eventParm, dir );
		CG_MissileHitWall( es->weapon, 0, position, dir, 0 ); // (SA) modified to send missilehitwall surface parameters
		break;

	case EV_MG42BULLET_HIT_WALL:
		DEBUGNAME( "EV_MG42BULLET_HIT_WALL" );
		ByteToDir( es->eventParm, dir );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD, qfalse, es->otherEntityNum2, es->effect1Time );
		break;

	case EV_MG42BULLET_HIT_FLESH:
		DEBUGNAME( "EV_MG42BULLET_HIT_FLESH" );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm, qfalse, es->otherEntityNum2, es->effect1Time );
		break;

	case EV_BULLET_HIT_WALL:
		DEBUGNAME( "EV_BULLET_HIT_WALL" );
		ByteToDir( es->eventParm, dir );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD, qfalse, es->otherEntityNum2, 0 );
//		CG_MachineGunEjectBrass(cent); // JPW NERVE
		break;

	case EV_BULLET_HIT_FLESH:
		DEBUGNAME( "EV_BULLET_HIT_FLESH" );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm, qfalse, es->otherEntityNum2, 0 );
		break;

	case EV_WOLFKICK_HIT_WALL:
		DEBUGNAME( "EV_WOLFKICK_HIT_WALL" );
		ByteToDir( es->eventParm, dir );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD, qtrue, es->otherEntityNum2, 0 );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.fkickwall );
		break;

	case EV_WOLFKICK_HIT_FLESH:
		DEBUGNAME( "EV_WOLFKICK_HIT_FLESH" );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm, qtrue, es->otherEntityNum2, 0 );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.fkickflesh );
		break;

	case EV_WOLFKICK_MISS:
		DEBUGNAME( "EV_WOLFKICK_MISS" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.fkickmiss );
		break;

	case EV_POPUPBOOK:
		trap_UI_Popup( va( "hbook%d", es->eventParm ) );
		break;

	case EV_POPUP:
		s = CG_ConfigString( CS_CLIPBOARDS + es->eventParm );
		// 's' is now the name of the menu script to run
		trap_Cvar_Set( "cg_clipboardName", s );    // store new current page name for the ui to pick up
		trap_UI_Popup( s );
		break;

	case EV_GIVEPAGE:
	{
		int havepages = cg_notebookpages.integer;
		havepages |= es->eventParm;
		trap_Cvar_Set( "cg_notebookpages", va( "%d", havepages ) );  // store new current page name for the ui to pick up
	}
	break;

	case EV_GENERAL_SOUND:
		DEBUGNAME( "EV_GENERAL_SOUND" );
		// Ridah, check for a sound script
		s = CG_ConfigString( CS_SOUNDS + es->eventParm );
		if ( !strstr( s, ".wav" ) ) {
			if ( CG_SoundPlaySoundScript( s, NULL, es->number ) ) {
				break;
			}
			// try with .wav
			Q_strncpyz( tempStr, s, sizeof( tempStr ) );
			Q_strcat( tempStr, sizeof( tempStr ), ".wav" );
			s = tempStr;
		}
		// done.
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, s ) );
		}
		break;

	case EV_GLOBAL_SOUND:   // play from the player's head so it never diminishes
		DEBUGNAME( "EV_GLOBAL_SOUND" );
		// Ridah, check for a sound script
		s = CG_ConfigString( CS_SOUNDS + es->eventParm );
		if ( !strstr( s, ".wav" ) ) {
			if ( CG_SoundPlaySoundScript( s, NULL, es->number ) ) {
				break;
			}
			// try with .wav
			Q_strncpyz( tempStr, s, sizeof( tempStr ) );
			Q_strcat( tempStr, sizeof( tempStr ), ".wav" );
			s = tempStr;
		}
		// done.
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, s ) );
		}
		break;

		// DHM - Nerve
	case EV_GLOBAL_CLIENT_SOUND:
		DEBUGNAME( "EV_GLOBAL_CLIENT_SOUND" );

		if ( cg.snap->ps.clientNum == es->teamNum ) {
			if ( cgs.gameSounds[ es->eventParm ] ) {
				trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
			}
		}

		break;
		// dhm - end

	case EV_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME( "EV_PAIN" );
		if ( cent->currentState.number != cg.snap->ps.clientNum ) {
			CG_PainEvent( cent, es->eventParm, qfalse );
		}
		break;

	case EV_CROUCH_PAIN:
		// local player sounds are triggered in CG_CheckLocalSounds,
		// so ignore events on the player
		DEBUGNAME( "EV_PAIN" );
		if ( cent->currentState.number != cg.snap->ps.clientNum ) {
			CG_PainEvent( cent, es->eventParm, qtrue );
		}
		break;

	case EV_DEATH1:
	case EV_DEATH2:
	case EV_DEATH3:
		DEBUGNAME( "EV_DEATHx" );
		trap_S_StartSound( NULL, es->number, CHAN_VOICE,
						   CG_CustomSound( es->number, va( "*death%i.wav", event - EV_DEATH1 + 1 ) ) );
		break;


	case EV_OBITUARY:
		DEBUGNAME( "EV_OBITUARY" );
		CG_Obituary( es );
		break;

// JPW NERVE -- swiped from SP/Sherman
	case EV_STOPSTREAMINGSOUND:
		DEBUGNAME( "EV_STOPLOOPINGSOUND" );
//		trap_S_StopStreamingSound( es->number );
		trap_S_StartSoundEx( NULL, es->number, CHAN_WEAPON, 0, SND_CUTOFF_ALL );  // kill weapon sound (could be reloading)
		break;
// jpw

		//
		// powerup events
		//
	case EV_POWERUP_QUAD:
		DEBUGNAME( "EV_POWERUP_QUAD" );
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_QUAD;
			cg.powerupTime = cg.time;
		}
		trap_S_StartSound( NULL, es->number, CHAN_ITEM, cgs.media.quadSound );
		break;
	case EV_POWERUP_BATTLESUIT:
		DEBUGNAME( "EV_POWERUP_BATTLESUIT" );
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_BATTLESUIT;
			cg.powerupTime = cg.time;
		}
		trap_S_StartSound( NULL, es->number, CHAN_ITEM, trap_S_RegisterSound( "sound/items/protect3.wav" ) );
		break;
	case EV_POWERUP_REGEN:
		DEBUGNAME( "EV_POWERUP_REGEN" );
		if ( es->number == cg.snap->ps.clientNum ) {
			cg.powerupActive = PW_REGEN;
			cg.powerupTime = cg.time;
		}
		trap_S_StartSound( NULL, es->number, CHAN_ITEM, trap_S_RegisterSound( "sound/items/regen.wav" ) );
		break;

	case EV_LOSE_HAT:
		DEBUGNAME( "EV_LOSE_HAT" );
		ByteToDir( es->eventParm, dir );
		// (SA) okay, some events not getting through, so I'm still testing.  Works except for that tho.
//		CG_Printf("lose had dir: %2.4f   %2.4f   %2.4f\n", dir[0], dir[1], dir[2]);
		CG_LoseHat( cent, dir );
		break;

	case EV_GIB_HEAD:
		DEBUGNAME( "EV_GIB_HEAD" );
		trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.gibSound );
		{
			vec3_t vhead, vtorso, vlegs;
			CG_GetBleedOrigin( vhead, vtorso, vlegs, cent->currentState.clientNum );
			CG_GibHead( vhead );
		}
		break;

	case EV_GIB_PLAYER:
		DEBUGNAME( "EV_GIB_PLAYER" );
		if ( es->aiChar == AICHAR_ZOMBIE ) {
			trap_S_StartSound( es->pos.trBase, -1, CHAN_AUTO, cgs.media.zombieDeathSound );
		} else {
			trap_S_StartSound( es->pos.trBase, -1, CHAN_AUTO, cgs.media.gibSound );
		}
		ByteToDir( es->eventParm, dir );
		CG_GibPlayer( cent, cent->lerpOrigin, dir );
		break;

	case EV_STOPLOOPINGSOUND:
		DEBUGNAME( "EV_STOPLOOPINGSOUND" );
		trap_S_StopLoopingSound( es->number );
		es->loopSound = 0;
		break;

	case EV_DEBUG_LINE:
		DEBUGNAME( "EV_DEBUG_LINE" );
		CG_Beam( cent );
		break;

		// Rafael particles
	case EV_SMOKE:
		DEBUGNAME( "EV_SMOKE" );
		if ( cent->currentState.density == 3 ) { // cannon
			CG_ParticleSmoke( cgs.media.smokePuffShaderdirty, cent );
		} else if ( !( cent->currentState.density ) ) {
			CG_ParticleSmoke( cgs.media.smokePuffShader, cent );
		} else {
			CG_ParticleSmoke( cgs.media.smokePuffShader, cent );
		}
		break;
		// done.

	case EV_FLAMETHROWER_EFFECT:
	{
		int old;
		old = cent->currentState.aiChar;
		cent->currentState.aiChar = AICHAR_ZOMBIE;

		// shoot this only in bursts

		// (SA) this first one doesn't seem to do anything.  ?

//			if ((cg.time+cent->currentState.number*100)%1000 > 200) {
//				CG_FireFlameChunks( cent, cent->currentState.origin, cent->lerpAngles, 0.1, qfalse );
//				CG_FireFlameChunks( cent, cent->currentState.origin, cent->currentState.apos.trBase, 0.1, qfalse );
//			}
//			else
//				CG_FireFlameChunks( cent, cent->currentState.origin, cent->lerpAngles, 0.6, 2 );
		CG_FireFlameChunks( cent, cent->currentState.origin, cent->currentState.apos.trBase, 0.6, 2 );

		cent->currentState.aiChar = old;
	}
	break;

	case EV_DUST:
		CG_ParticleDust( cent, cent->currentState.origin, cent->currentState.angles );
		break;

	case EV_RUMBLE_EFX:
	{
		float pitch, yaw;
		pitch = cent->currentState.angles[0];
		yaw = cent->currentState.angles[1];
		CG_RumbleEfx( pitch, yaw );
	}
	break;

	case EV_CONCUSSIVE:
		CG_Concussive( cent );
		break;

//----(SA)	added	// generic particle emitter that uses client-side particle scripts
	case EV_EMITTER:
	{
		localEntity_t   *le;
		le = CG_AllocLocalEntity();
		le->leType = LE_EMITTER;
		le->startTime = cg.time;
		le->endTime = le->startTime + 20000;
		le->pos.trType = TR_STATIONARY;
		VectorCopy( cent->currentState.origin, le->pos.trBase );
		VectorCopy( cent->currentState.origin2, le->angles.trBase );
		le->ownerNum = 0;
	}
	break;
//----(SA)

	case EV_OILPARTICLES:
		CG_Particle_OilParticle( cgs.media.oilParticle, cent->currentState.origin, cent->currentState.origin2, cent->currentState.time, cent->currentState.density );
		break;
	case EV_OILSLICK:
		CG_Particle_OilSlick( cgs.media.oilSlick, cent );
		break;
	case EV_OILSLICKREMOVE:
		CG_OilSlickRemove( cent );
		break;

	case EV_MG42EFX:
		CG_MG42EFX( cent );
		break;

	case EV_FLAKGUN1:
		CG_FLAKEFX( cent, 1 );
		break;
	case EV_FLAKGUN2:
		CG_FLAKEFX( cent, 2 );
		break;
	case EV_FLAKGUN3:
		CG_FLAKEFX( cent, 3 );
		break;
	case EV_FLAKGUN4:
		CG_FLAKEFX( cent, 4 );
		break;

	case EV_SPARKS_ELECTRIC:
	case EV_SPARKS:
	{
		int numsparks;
		int i;
		int duration;
		float x,y;
		float speed;
		vec3_t source, dest;

		if ( !( cent->currentState.density ) ) {
			cent->currentState.density = 1;
		}
		numsparks = rand() % cent->currentState.density;
		duration = cent->currentState.frame;
		x = cent->currentState.angles2[0];
		y = cent->currentState.angles2[1];
		speed = cent->currentState.angles2[2];

		if ( !numsparks ) {
			numsparks = 1;
		}
		for ( i = 0; i < numsparks; i++ )
		{

			if ( event == EV_SPARKS_ELECTRIC ) {
				VectorCopy( cent->currentState.origin, source );

				VectorCopy( source, dest );
				dest[0] += ( ( rand() & 31 ) - 16 );
				dest[1] += ( ( rand() & 31 ) - 16 );
				dest[2] += ( ( rand() & 31 ) - 16 );

				CG_Tracer( source, dest, 1 );
			} else {
				CG_ParticleSparks( cent->currentState.origin, cent->currentState.angles, duration, x, y, speed );
			}

		}

	}
	break;

	case EV_GUNSPARKS:
	{
		int numsparks;
		int speed;
		//int	count;

		numsparks = cent->currentState.density;
		speed = cent->currentState.angles2[2];

		CG_AddBulletParticles( cent->currentState.origin, cent->currentState.angles, speed, 800, numsparks, 1.0f );

	}
	break;

		// Rafael bats
	case EV_BATS:
	{
		int i;
		for ( i = 0; i < cent->currentState.density; i++ )
			CG_ParticleBats( cgs.media.bats[0], cent );
	}
	break;

	case EV_BATS_UPDATEPOSITION:
		CG_BatsUpdatePosition( cent );
		break;

	case EV_BATS_DEATH:
		CG_BatDeath( cent );
		break;

		// Rafael snow pvs check
	case EV_SNOW_ON:
		CG_SnowLink( cent, qtrue );
		break;

	case EV_SNOW_OFF:
		CG_SnowLink( cent, qfalse );
		break;


	case EV_SNOWFLURRY:
		CG_ParticleSnowFlurry( cgs.media.snowShader, cent );
		break;

		//----(SA)

		// for func_exploding
	case EV_EXPLODE:
		DEBUGNAME( "EV_EXPLODE" );
		ByteToDir( es->eventParm, dir );
		CG_Explode( cent, position, dir, 0 );
		break;

		// for target_effect
	case EV_EFFECT:
		DEBUGNAME( "EV_EFFECT" );
//		ByteToDir( es->eventParm, dir );
		CG_Effect( cent, position, dir );
		break;

		//----(SA) done

	case EV_MORTAREFX:  // mortar firing
		DEBUGNAME( "EV_MORTAREFX" );
		CG_MortarEFX( cent );
		break;

	case EV_SHARD:
		ByteToDir( es->eventParm, dir );
		CG_Shard( cent, position, dir );
		break;

	case EV_JUNK:
		ByteToDir( es->eventParm, dir );
		{
			int i;
			int rval;

			rval = rand() % 3 + 3;

			for ( i = 0; i < rval; i++ )
				CG_ShardJunk( cent, position, dir );
		}
		break;

	case EV_SNIPER_SOUND:
		// trap_S_StartSound( es->pos.trBase, -1, CHAN_AUTO, cgs.media.snipersound );
		// trap_S_StartSound (NULL, es->number, CHAN_AUTO,	cgs.media.snipersound );
		trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cgs.media.snipersound );
		break;

	default:
		DEBUGNAME( "UNKNOWN" );
		CG_Error( "Unknown event: %i", event );
		break;
	}


	{
		int rval;

		rval = rand() & 3;

		if ( splashfootstepcnt != rval ) {
			splashfootstepcnt = rval;
		} else {
			splashfootstepcnt++;
		}

		if ( splashfootstepcnt > 3 ) {
			splashfootstepcnt = 0;
		}


		if ( footstepcnt != rval ) {
			footstepcnt = rval;
		} else {
			footstepcnt++;
		}

		if ( footstepcnt > 3 ) {
			footstepcnt = 0;
		}
	}
}


/*
==============
CG_CheckEvents

==============
*/
void CG_CheckEvents( centity_t *cent ) {
	int i, event;

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
	CG_SetEntitySoundPosition( cent );

	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( cent->previousEvent ) {
			//goto skipEvent;
			return; // already fired
		}
		// if this is a player event set the entity number of the client entity number
//(SA) note: EF_PLAYER_EVENT never set
//		if ( cent->currentState.eFlags & EF_PLAYER_EVENT ) {
//			cent->currentState.number = cent->currentState.otherEntityNum;
//		}

		cent->previousEvent = 1;

		cent->currentState.event = cent->currentState.eType - ET_EVENTS;
	} else {

		// DHM - Nerve :: Entities that make it here are Not TempEntities.
		//		As far as we could tell, for all non-TempEntities, the
		//		circular 'events' list contains the valid events.  So we
		//		skip processing the single 'event' field and go straight
		//		to the circular list.

		goto skipEvent;
		/*
		// check for events riding with another entity
		if ( cent->currentState.event == cent->previousEvent ) {
			goto skipEvent;
			//return;
		}
		cent->previousEvent = cent->currentState.event;
		if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == 0 ) {
			goto skipEvent;
			//return;
		}
		*/
		// dhm - end
	}

	CG_EntityEvent( cent, cent->lerpOrigin );
	// DHM - Nerve :: Temp ents return after processing
	return;

skipEvent:

	// check the sequencial list
	// if we've added more events than can fit into the list, make sure we only add them once
	if ( cent->currentState.eventSequence < cent->previousEventSequence ) {
		cent->previousEventSequence -= ( 1 << 8 );    // eventSequence is sent as an 8-bit through network stream
	}
	if ( cent->currentState.eventSequence - cent->previousEventSequence > MAX_EVENTS ) {
		cent->previousEventSequence = cent->currentState.eventSequence - MAX_EVENTS;
	}
	for ( i = cent->previousEventSequence ; i != cent->currentState.eventSequence; i++ ) {
		event = cent->currentState.events[ i & ( MAX_EVENTS - 1 ) ];

		cent->currentState.event = event;
		cent->currentState.eventParm = cent->currentState.eventParms[ i & ( MAX_EVENTS - 1 ) ];
		CG_EntityEvent( cent, cent->lerpOrigin );
	}
	cent->previousEventSequence = cent->currentState.eventSequence;

	// set the event back so we don't think it's changed next frame (unless it really has)
	cent->currentState.event = cent->previousEvent;
}


/*
void CG_CheckEvents( centity_t *cent ) {
	int i, event;

	// calculate the position at exactly the frame time
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, cent->lerpOrigin );
	CG_SetEntitySoundPosition( cent );

	// check for event-only entities
	if ( cent->currentState.eType > ET_EVENTS ) {
		if ( !cent->previousEvent ) {
			cent->previousEvent = 1;
			cent->currentState.event = cent->currentState.eType - ET_EVENTS;
			CG_EntityEvent( cent, cent->lerpOrigin );
		}
	} else {
		// check for events riding with another entity
		if ( cent->currentState.event != cent->previousEvent ) {
			cent->previousEvent = cent->currentState.event;
			if ( cent->currentState.event & ~EV_EVENT_BITS ) {
				CG_EntityEvent( cent, cent->lerpOrigin );
			}
		}
	}

	// check the sequencial list
	// if we've added more events than can fit into the list, make sure we only add them once
	if (cent->currentState.eventSequence < cent->previousEventSequence) {
		cent->previousEventSequence -= (1 << 8);	// eventSequence is sent as an 8-bit through network stream
	}
	if (cent->currentState.eventSequence - cent->previousEventSequence > MAX_EVENTS) {
		cent->previousEventSequence = cent->currentState.eventSequence - MAX_EVENTS;
	}
	for ( i = cent->previousEventSequence ; i != cent->currentState.eventSequence; i++ ) {
		event = cent->currentState.events[ i & (MAX_EVENTS-1) ];

		cent->currentState.event = event;
		cent->currentState.eventParm = cent->currentState.eventParms[ i & (MAX_EVENTS-1) ];
		CG_EntityEvent( cent, cent->lerpOrigin );
	}
	cent->previousEventSequence = cent->currentState.eventSequence;

	// set the event back so we don't think it's changed next frame (unless it really has)
	cent->currentState.event = cent->previousEvent;
}
*/
