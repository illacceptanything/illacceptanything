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



// cg_event.c -- handle entity events at snapshot or playerstate transitions

#include "cg_local.h"
#include "../ui/ui_shared.h" // for Menus_CloseAll()

extern int hWeaponSnd;

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
	char        *message;
	char        *message2;
	const char  *targetInfo;
	const char  *attackerInfo;
	char targetName[32];
	char attackerName[32];
	gender_t gender;
	clientInfo_t    *ci;

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

	// check for single client messages

	switch ( mod ) {
	case MOD_SUICIDE:
		message = "suicides";
		break;
	case MOD_FALLING:
		message = "cratered";
		break;
	case MOD_CRUSH:
		message = "was squished";
		break;
	case MOD_WATER:
		message = "sank like a rock";
		break;
	case MOD_SLIME:
		message = "melted";
		break;
	case MOD_LAVA:
		message = "does a back flip into the lava";
		break;
	case MOD_TARGET_LASER:
		message = "saw the light";
		break;
	case MOD_TRIGGER_HURT:
		message = "was in the wrong place";
		break;
	default:
		message = NULL;
		break;
	}

	if ( attacker == target ) {
		gender = ci->modelInfo->gender;
		switch ( mod ) {
		case MOD_GRENADE_SPLASH:
			if ( gender == GENDER_FEMALE ) {
				message = "tripped on her own grenade";
			} else if ( gender == GENDER_NEUTER ) {
				message = "tripped on its own grenade";
			} else {
				message = "tripped on his own grenade";
			}
			break;
		case MOD_ROCKET_SPLASH:
			if ( gender == GENDER_FEMALE ) {
				message = "blew herself up";
			} else if ( gender == GENDER_NEUTER ) {
				message = "blew itself up";
			} else {
				message = "blew himself up";
			}
			break;
		case MOD_BFG_SPLASH:
			message = "should have used a smaller gun";
			break;
		case MOD_EXPLOSIVE:
			message = "died in an explosion";
			break;
		default:
			if ( gender == GENDER_FEMALE ) {
				message = "killed herself";
			} else if ( gender == GENDER_NEUTER ) {
				message = "killed itself";
			} else {
				message = "killed himself";
			}
			break;
		}
	}

	if ( message ) {
		CG_Printf( "%s %s.\n", targetName, message );
		return;
	}

	// check for kill messages from the current clientNum
	if ( attacker == cg.snap->ps.clientNum ) {
		char    *s;

		if ( cgs.gametype < GT_TEAM ) {
			s = va( "You fragged %s\n%s place with %i", targetName,
					CG_PlaceString( cg.snap->ps.persistant[PERS_RANK] + 1 ),
					cg.snap->ps.persistant[PERS_SCORE] );
		} else {
			s = va( "You fragged %s", targetName );
		}
		CG_CenterPrint( s, SCREEN_HEIGHT * 0.25, BIGCHAR_WIDTH );

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
			message = "was knifed by";
			break;
		case MOD_LUGER:
			message = "was killed (luger) by";
			break;
		case MOD_COLT:
			message = "was killed (colt) by";
			break;
		case MOD_MP40:
			message = "was killed (mp40) by";
			break;
		case MOD_THOMPSON:
			message = "was killed (thompson) by";
			break;
		case MOD_STEN:
			message = "was killed (sten) by";
			break;
		case MOD_MAUSER:
			message = "was killed (mauser) by";
			break;
		case MOD_SNIPERRIFLE:
			message = "was killed (sniper) by";
			break;
		case MOD_GARAND:
			message = "was killed (garand) by";
			break;
		case MOD_SNOOPERSCOPE:
			message = "was killed (snooper) by";
			break;
		case MOD_AKIMBO:
			message = "was killed (dual colts) by";
			break;
		case MOD_ROCKET_LAUNCHER:
			message = "was killed (rl) by";
			break;
		case MOD_GRENADE_LAUNCHER:
			message = "was killed (gren - gm) by";
			break;
		case MOD_VENOM:
			message = "was killed (venom) by";
			break;
		case MOD_VENOM_FULL:
			message = "was killed (venom shot) by";
			break;
		case MOD_FLAMETHROWER:
			message = "was killed (flamethrower) by";
			break;
		case MOD_TESLA:
			message = "was killed (tesla) by";
			break;
		case MOD_SPEARGUN:
			message = "was killed (spear) by";
			break;
		case MOD_SPEARGUN_CO2:
			message = "was killed (co2 spear) by";
			break;
		case MOD_GRENADE_PINEAPPLE:
			message = "was killed (gren - am) by";
			break;
		case MOD_CROSS:
			message = "was killed (cross) by";
			break;
// JPW NERVE
		case MOD_AIRSTRIKE:
			message = "stood under";
			message2 = "'s air strike";
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

		if ( message ) {
			CG_Printf( "%s %s %s%s\n",
					   targetName, message, attackerName, message2 );
			return;
		}
	}

	// we don't know what it was
// JPW NERVE added mod check for machinegun (prolly mortar here too)
	switch ( mod ) {
	case MOD_MACHINEGUN:
		CG_Printf( "%s was riddled by machinegun fire\n",targetName );
		break;
	default:
		CG_Printf( "%s died.\n", targetName );
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
	int itemNum;
	gitem_t     *item;
	entityState_t *es;

	es = &cent->currentState;

	// itemNum = es->event - EV_USE_ITEM0;
	// JCash bluesnews reported fix
	itemNum = ( es->event & ~EV_EVENT_BITS ) - EV_USE_ITEM0;

	if ( itemNum < 0 || itemNum > HI_NUM_HOLDABLE ) {
		itemNum = 0;
	}

	// print a message if the local player
	if ( es->number == cg.snap->ps.clientNum ) {
		if ( !itemNum ) {
			CG_CenterPrint( "noitem", SCREEN_HEIGHT - ( SCREEN_HEIGHT * 0.25 ), SMALLCHAR_WIDTH ); //----(SA)	modified
		} else {
			item = BG_FindItemForHoldable( itemNum );

			if ( item ) {
				cg.holdableSelectTime = cg.time;    // show remaining items

				switch ( itemNum ) {
				case HI_BOOK1:
				case HI_BOOK2:
				case HI_BOOK3:
					break;
				case HI_WINE:
					CG_CenterPrint( "drankwine", SCREEN_HEIGHT - ( SCREEN_HEIGHT * 0.25 ), SMALLCHAR_WIDTH );
					break;
				default:
					CG_CenterPrint( va( "Use %s", cgs.itemPrintNames[item - bg_itemlist] ), SCREEN_HEIGHT - ( SCREEN_HEIGHT * 0.25 ), SMALLCHAR_WIDTH );
					break;
				}
			}
		}
	}

	switch ( itemNum ) {
	default:
	case HI_NONE:
		trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.useNothingSound );
		break;

	case HI_BOOK1:
	case HI_BOOK2:
	case HI_BOOK3:
		trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.bookSound );
		break;

	case HI_WINE:
		trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.wineSound );
		break;

	case HI_STAMINA:
		trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.staminaSound );
		break;
	}

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
	qboolean selectIt;

	itemid = bg_itemlist[itemNum].giTag;

	cg.itemPickup           = itemNum;
	cg.itemPickupTime       = cg.time;
	cg.itemPickupBlendTime  = cg.time;

	// see if it should be the grabbed weapon
	if ( bg_itemlist[itemNum].giType == IT_WEAPON ) {
		int weapon;

		selectIt = qfalse;

		weapon = itemid;

		if ( weapon == WP_COLT ) {
			if ( COM_BitCheck( cg.snap->ps.weapons, weapon ) ) {
				weapon = WP_AKIMBO; // you have colt, now get akimbo (second)
			}
		}

		if ( cg_autoswitch.integer && cg.predictedPlayerState.weaponstate != WEAPON_RELOADING ) {

			//	0 - "Off"
			//	1 - "Always Switch"
			//	2 - "If New"
			//	3 - "If Better"
			//	4 - "New or Better"
			//	5 - "New and Better"

			// don't ever autoswitch to secondary fire weapons
			if ( weapon != WP_SNIPERRIFLE && weapon != WP_SNOOPERSCOPE && weapon != WP_FG42SCOPE ) {  //----(SA)	modified

				// no weap currently selected, always just select the new one
				if ( !cg.weaponSelect ) {
					selectIt = qtrue;
				}
				// 1 - "Always Switch" - always switch to new weap (Q3A default)
				else if ( cg_autoswitch.integer == 1 ) {
					selectIt = qtrue;
				} else {

					// 2 - "If New" - switch to weap if it's not already in the player's inventory (Wolf default)
					// 4 - either 2 or 3
					// 5 - both 2 and 3

					// FIXME:	this works fine for predicted pickups (when you walk over the weapon), but not for
					//			manual pickups (activate item)
					if ( cg_autoswitch.integer == 2 || cg_autoswitch.integer == 4 || cg_autoswitch.integer == 5 ) {
						if ( !COM_BitCheck( cg.snap->ps.weapons, weapon ) ) {
							selectIt = qtrue;
						}
					}   // end 2/4/5

					// 3 - "If Better" - switch to weap if it's in a bank greater than the current weap
					// 4 - either 2 or 3
					// 5 - both 2 and 3
					if ( cg_autoswitch.integer == 3 || cg_autoswitch.integer == 4 || cg_autoswitch.integer == 5 ) {
						// switch away only if a primary weapon is selected (read: don't switch away if current weap is a secondary mode)
						if ( CG_WeaponIndex( cg.weaponSelect, &wpbank_cur, NULL ) ) {
							if ( CG_WeaponIndex( weapon, &wpbank_pickup, NULL ) ) {
								if ( wpbank_pickup > wpbank_cur ) {
									if ( cg_autoswitch.integer == 5 ) {    // 'new /and/ better'
										if ( !selectIt ) {
											selectIt = qfalse;  // if it isn't selected because it's new, then this isn't "both" new /and/ better
										}
									} else {
										selectIt = qtrue;
									}
								} else {    // not better
									if ( cg_autoswitch.integer == 5 ) {
										selectIt = qfalse;
									}
								}
							}
						}
					}   // end 3/4/5

				}   // end cg_autoswitch.integer != 1

			}   // end weapon != WP_SNIPERRIFLE && ...

		}   // end cg_autoswitch.integer

		// only select one-handed weaps if you've got a chair
		if ( cg.snap->ps.eFlags & EF_MELEE_ACTIVE ) {
			if ( !( ( 1 << weapon ) & WEAPS_ONE_HANDED ) ) {
				selectIt = qfalse;
			}
		}

		if ( selectIt ) {
			cg.weaponSelectTime = cg.time;
			cg.weaponSelect     = weapon;
		}

	}   // end bg_itemlist[itemNum].giType == IT_WEAPON


	if ( bg_itemlist[itemNum].giType == IT_HOLDABLE ) {
		cg.holdableSelectTime   = cg.time;  // show holdables when a new one is picked up
		cg.holdableSelect       = itemid;   // and select the new one
	}

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




void CG_Explodef( vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader, int parent, qboolean damage );

/*
==============
CG_Explode
	the old cent-based explode calls will still work with this pass-through
==============
*/
void CG_Explode( centity_t *cent, vec3_t origin, vec3_t dir, qhandle_t shader ) {
	vec3_t pos;
	qhandle_t inheritmodel = 0;


//	VectorCopy(origin, pos);
	VectorCopy( cent->currentState.origin2, pos );

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

	CG_Explodef(    pos,
					dir,
					cent->currentState.density,         // mass
//					cent->currentState.time2,			// type
					cent->currentState.effect3Time,                 //----(SA)	needed .time
					cent->currentState.dl_intensity,    // sound
					cent->currentState.weapon,          // forceLowGrav
					shader,
					cent->currentState.number,
					cent->currentState.teamNum
					);

}


/*
==============
CG_Explodef
	made this more generic for spawning hits and breaks without needing a *cent
==============
*/
void CG_Explodef( vec3_t origin, vec3_t dir, int mass, int type, qhandle_t sound, int forceLowGrav, qhandle_t shader, int parent, qboolean damage ) {
	int i;
	localEntity_t   *le;
	refEntity_t     *re;
	int howmany, total;
	int pieces[6];              // how many of each piece
	qhandle_t modelshader = 0;
	float materialmul = 1;              // multiplier for different types

	memset( &pieces, 0, sizeof( pieces ) );

	if ( type == 5 && damage ) {
		vec3_t vec, org;
		centity_t *boss;
		// set the default pos to the viewpos
		VectorCopy( cg.refdef.vieworg, org );
		// find the boss
		for ( boss = cg_entities; boss < &cg_entities[MAX_CLIENTS]; boss++ ) {
			if ( !boss->currentValid ) {
				continue;
			}
			if ( boss->currentState.aiChar == AICHAR_HEINRICH ) {
				VectorCopy( boss->lerpOrigin, org );
				break;
			}
		}
		// optimization, ignore if too far away
		VectorSubtract( origin, org, vec ); // whoops, that way wouldn't work for cameras...
		vec[2] = 0;
		if ( VectorLength( vec ) > 800 ) {
			return;
		}
		mass = (int)( 2.0 * mass * ( 1.0 - ( 0.6 + 0.4 * ( VectorLength( vec ) / 800.0 ) ) ) );
	}

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

	// cap end map debris (optimization)
	if ( type == 5 && damage ) {
		pieces[0] = 5;
		pieces[1] = 5;
		pieces[2] = 4;
		pieces[3] = 4;
		pieces[4] = 4;
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

			// RF, debris rocks last longer in the boss map (is thee a better way of doing this at this late stage?)
			if ( snd == LEBS_ROCK && damage ) {
				snd = 0;
				if ( damage ) {
					le->leFlags |= LEF_PLAYER_DAMAGE;
				}
				le->endTime = le->startTime + 7000 + random() * 5000;
			}

			if ( parent ) {
				le->leFlags     |= LEF_NOTOUCHPARENT;   //----(SA)	added
				le->ownerNum    = parent;
			}

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
			le->leFlags     |= LEF_TUMBLE;
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

			switch ( type ) {
			case 6:     // fabric
				le->pos.trType = TR_GRAVITY_FLOAT;      // the fabric stuff will change to use something that looks better
				le->bounceFactor    = 0.0f;
				materialmul         = 0.3f;     // rotation speed
				break;
			default:
				if ( !forceLowGrav && rand() & 1 ) {    // if low gravity is not forced and die roll goes our way use regular grav
					le->pos.trType = TR_GRAVITY;
				} else {
					le->pos.trType = TR_GRAVITY_LOW;
				}
				le->bounceFactor    = 0.2f;     // RF, rubble in end is like rubber, also ID requestsed this
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
			if ( ( le->angles.trBase[0] == 1 || le->angles.trBase[1] == 1 || le->angles.trBase[2] == 1 ) && type != 6 ) {    // not for fabric
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

				if ( type == 5 && damage ) {
					VectorScale( le->pos.trDelta, 4.0, le->pos.trDelta );
					// dont let them fly out too fast
					while ( VectorLength( le->pos.trDelta ) > 800 ) {
						VectorScale( le->pos.trDelta, 0.5, le->pos.trDelta );
					}
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
	Quake ed -> target_effect (0 .5 .8) (-6 -6 -6) (6 6 6) TNT explode smoke debris gore lowgrav
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
		vec3_t sprVel, sprOrg;
		int i,j;

		// RF, sprVel is used without being set, so to be sure, I'm going to clear out the vector
		VectorClear( sprVel );

		for ( i = 0; i < 5; i++ ) {
			for ( j = 0; j < 3; j++ )
				sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
			sprVel[2] += rand() % 50;
			CG_ParticleExplosion( "blacksmokeanimb", sprOrg, sprVel,
								  3500 + rand() % 250,          // duration
								  10,                           // startsize
								  250 + rand() % 60 );          // endsize
		}

		VectorMA( origin, 16, dir, sprOrg );
		VectorScale( dir, 100, sprVel );

		// trying this one just for now just for variety
		CG_ParticleExplosion( "explode1", sprOrg, sprVel,
							  1200,         // duration
							  9,            // startsize
							  300 );        // endsize

		CG_AddDebris( origin, dir,
					  280,              // speed
					  1400,             // duration
					  7 + rand() % 2 ); // count

		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.sfx_dynamiteexp );
		trap_S_StartLocalSound( cgs.media.sfx_dynamiteexpDist, CHAN_AUTO );
		CG_ImpactMark( cgs.media.burnMarkShader, origin, dir, random() * 360, 1,1,1,1, qfalse, 64, qfalse, -1 );
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
			CG_ParticleExplosion( "blacksmokeanimb", sprOrg, sprVel, 3500 + rand() % 250, 10, 250 + rand() % 60 );
		}
	}


	if ( cent->currentState.eventParm & 2 ) {  // explode
		vec3_t sprVel, sprOrg;
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.sfx_rockexp );

		// new explode	(from rl)
		VectorMA( origin, 16, dir, sprOrg );
		VectorScale( dir, 100, sprVel );
		CG_ParticleExplosion( "expblue", sprOrg, sprVel, 500, 20, 160 );
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
		le->leFlags             |= LEF_TUMBLE;
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
	le->leFlags             |= LEF_TUMBLE;
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

void CG_SpawnSpirit( centity_t *cent ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t enemyPos, v, ang;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	re->hModel = cgs.media.ssSpiritSkullModel;
	re->backlerp = 0;
	re->renderfx = RF_NOSHADOW | RF_MINLIGHT;   //----(SA)
	//re->customShader = cgs.media.ssSpiritSkullModel;
	re->reType = RT_MODEL;

	le->leType = LE_HELGA_SPIRIT;
	le->startTime = cg.time;
	le->endTime = cg.time + 6000; //cent->currentState.time;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = cg.time;
	VectorCopy( cent->currentState.origin, le->pos.trBase );
	VectorClear( le->pos.trDelta );

	le->effectWidth = 600;
	le->radius = 30.0;
	le->lastTrailTime = cg.time;
	le->headJuncIndex = -1;
	le->loopingSound = cgs.media.zombieSpiritLoopSound;

	le->ownerNum = cent->currentState.number;

	re->fadeStartTime = le->endTime - 2000;
	re->fadeEndTime = le->endTime;
	re->shaderTime = cg.time;

	// get direction to enemy
	if ( cg_entities[le->ownerNum].currentState.otherEntityNum2 == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, enemyPos );
		enemyPos[2] += cg.snap->ps.viewheight;
	} else {
		VectorCopy( cg_entities[le->ownerNum].currentState.origin2, enemyPos );
	}

	// set angles
	VectorSubtract( enemyPos, le->pos.trBase, v );
	VectorNormalize( v );
	vectoangles( v, ang );
	AnglesToAxis( ang, re->axis );
	VectorScale( v, 350, le->pos.trDelta );

}

/*
==============
CG_EntityEvent

An entity has an event value
also called by CG_CheckPlayerstateEvents
==============
*/
#define DEBUGNAME( x ) if ( cg_debugEvents.integer ) {CG_Printf( x "\n" );}
void CG_EntityEvent( centity_t *cent, vec3_t position ) {
	entityState_t   *es;
	int event;
	vec3_t dir;
	const char      *s;
	int clientNum;
	clientInfo_t    *ci;
	//char			tempStr[MAX_QPATH];

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

	if ( !ci->modelInfo ) {   // not ready yet?
		return;
	}

	switch ( event ) {
		//
		// movement generated events
		//

		// TODO: change all this to sound scripts

	case EV_FOOTSTEP:
		DEBUGNAME( "EV_FOOTSTEP" );
		if ( cg_footsteps.integer ) {
			if ( cent->currentState.aiChar == AICHAR_ELITEGUARD ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_ELITE_STEP ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_ZOMBIE ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_ZOMBIE_STEP ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_LOPER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_LOPER_STEP ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_PROTOSOLDIER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_PROTOSOLDIER_STEP ][footstepcnt] );
				CG_StartShakeCamera( 0.05, 400, es->pos.trBase, 512 );
			} else if ( cent->currentState.aiChar == AICHAR_SUPERSOLDIER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_STEP ][footstepcnt] );
				CG_StartShakeCamera( 0.08, 500, es->pos.trBase, 800 );
			} else if ( cent->currentState.aiChar == AICHAR_HEINRICH ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_HEINRICH ][footstepcnt] );
				CG_StartShakeCamera( 0.08, 500, es->pos.trBase, 800 );
			} else if ( cent->currentState.aiChar == AICHAR_HELGA ) {
				CG_SoundPlayIndexedScript( cgs.media.footsteps[FOOTSTEP_BEAST][0], NULL, es->number );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ ci->modelInfo->footsteps ][footstepcnt] );
			}
		}
		break;
	case EV_FOOTSTEP_METAL:
		DEBUGNAME( "EV_FOOTSTEP_METAL" );
		if ( cg_footsteps.integer ) {
			if ( cent->currentState.aiChar == AICHAR_ELITEGUARD ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_ELITE_METAL ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_LOPER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_LOPER_METAL ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_PROTOSOLDIER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_PROTOSOLDIER_METAL ][footstepcnt] );
				CG_StartShakeCamera( 0.05, 400, es->pos.trBase, 512 );
			} else if ( cent->currentState.aiChar == AICHAR_SUPERSOLDIER || cent->currentState.aiChar == AICHAR_HEINRICH ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_METAL ][0] );
				CG_StartShakeCamera( 0.08, 500, es->pos.trBase, 800 );
			} else if ( cent->currentState.aiChar == AICHAR_HELGA ) {
				CG_SoundPlayIndexedScript( cgs.media.footsteps[FOOTSTEP_BEAST][0], NULL, es->number );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_METAL ][footstepcnt] );
			}
		}
		break;
	case EV_FOOTSTEP_WOOD:
		DEBUGNAME( "EV_FOOTSTEP_WOOD" );
		if ( cg_footsteps.integer ) {
			if ( cent->currentState.aiChar == AICHAR_ELITEGUARD ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_ELITE_WOOD ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_ZOMBIE ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_ZOMBIE_WOOD ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_LOPER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_LOPER_WOOD ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_PROTOSOLDIER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_PROTOSOLDIER_WOOD ][footstepcnt] );
				CG_StartShakeCamera( 0.05, 400, es->pos.trBase, 512 );
			} else if ( cent->currentState.aiChar == AICHAR_SUPERSOLDIER || cent->currentState.aiChar == AICHAR_HEINRICH ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_WOOD ][footstepcnt] );
				CG_StartShakeCamera( 0.08, 500, es->pos.trBase, 800 );
			} else if ( cent->currentState.aiChar == AICHAR_HELGA ) {
				CG_SoundPlayIndexedScript( cgs.media.footsteps[FOOTSTEP_BEAST][0], NULL, es->number );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_WOOD ][footstepcnt] );
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
			if ( cent->currentState.aiChar == AICHAR_PROTOSOLDIER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_PROTOSOLDIER_GRASS ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_SUPERSOLDIER || cent->currentState.aiChar == AICHAR_HEINRICH ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_GRASS ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_HELGA ) {
				CG_SoundPlayIndexedScript( cgs.media.footsteps[FOOTSTEP_BEAST][0], NULL, es->number );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_GRASS ][footstepcnt] );
			}
		}
		break;
	case EV_FOOTSTEP_GRAVEL:
		DEBUGNAME( "EV_FOOTSTEP_GRAVEL" );
		if ( cg_footsteps.integer ) {
			if ( cent->currentState.aiChar == AICHAR_ELITEGUARD ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_ELITE_GRAVEL ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_ZOMBIE ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_ZOMBIE_GRAVEL ][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_PROTOSOLDIER ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_PROTOSOLDIER_GRAVEL][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_SUPERSOLDIER || cent->currentState.aiChar == AICHAR_HEINRICH ) {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_SUPERSOLDIER_GRAVEL][footstepcnt] );
			} else if ( cent->currentState.aiChar == AICHAR_HELGA ) {
				CG_SoundPlayIndexedScript( cgs.media.footsteps[FOOTSTEP_BEAST][0], NULL, es->number );
			} else {
				trap_S_StartSound( NULL, es->number, CHAN_BODY, cgs.media.footsteps[ FOOTSTEP_GRAVEL ][footstepcnt] );
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

	case EV_FALL_DMG_10:
		DEBUGNAME( "EV_FALL_DMG_10" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		// use normal pain sound trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
			CG_StartShakeCamera( 0.03, 200, cg.predictedPlayerState.origin, 200 );
		}
		break;

	case EV_FALL_DMG_15:
		DEBUGNAME( "EV_FALL_DMG_15" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall1.wav" ) );
		// use normal pain sound trap_S_StartSound( NULL, es->number, CHAN_VOICE, CG_CustomSound( es->number, "*pain100_1.wav" ) );
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -16;
			cg.landTime = cg.time;
			CG_StartShakeCamera( 0.04, 200, cg.predictedPlayerState.origin, 200 );
		}
		break;

	case EV_FALL_DMG_25:
		DEBUGNAME( "EV_FALL_DMG_25" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall2.wav" ) );
		cent->pe.painTime = cg.time;    // don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
			CG_StartShakeCamera( 0.05, 200, cg.predictedPlayerState.origin, 200 );
		}
		break;

	case EV_FALL_DMG_50:
		DEBUGNAME( "EV_FALL_DMG_50" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*fall2.wav" ) );
		cent->pe.painTime = cg.time;    // don't play a pain sound right after this
		if ( clientNum == cg.predictedPlayerState.clientNum ) {
			// smooth landing z changes
			cg.landChange = -24;
			cg.landTime = cg.time;
			CG_StartShakeCamera( 0.08, 200, cg.predictedPlayerState.origin, 200 );
		}
		break;

	case EV_FALL_NDIE:
		DEBUGNAME( "EV_FALL_NDIE" );
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
//			trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_WATER, 200, 0, 0, 0, 0);
			char buff[64];
			trap_Cvar_VariableStringBuffer( "r_waterFogColor", buff, sizeof( buff ) );
			trap_SendClientCommand( va( "fogswitch %s", buff ) );
		}
		break;
	case EV_WATER_CLEAR:
		DEBUGNAME( "EV_WATER_CLEAR" );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, CG_CustomSound( es->number, "*gasp.wav" ) );

		// DHM - Nerve :: causes problems in multiplayer...
		if ( cgs.gametype == GT_SINGLE_PLAYER && clientNum == cg.predictedPlayerState.clientNum ) {
//			trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 400,0,0,0,0);
			char buff[64];
			trap_Cvar_VariableStringBuffer( "r_mapFogColor", buff, sizeof( buff ) );
			trap_SendClientCommand( va( "fogswitch %s", buff ) );
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
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, trap_S_RegisterSound( "sound/items/n_health.wav" ) );
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
			if ( ( cg.predictedPlayerState.weapAnim & ~ANIM_TOGGLEBIT ) == WEAP_ATTACK1 ) { // if attacking, go idle while overheating
				cg.predictedPlayerState.weapAnim = ( ( cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | WEAP_IDLE1;
			}
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

			// hope this does not cause trouble.  changed it to chan_weapon so i could kill the sound if the guy dies while reloading
			// can re-work if this causes trouble
//			trap_S_StartSound (NULL, es->number, CHAN_AUTO, cg_weapons[es->weapon].reloadSound );
			trap_S_StartSound( NULL, es->number, CHAN_WEAPON, cg_weapons[es->weapon].reloadSound );
		}
		break;


	case EV_NOAMMO:
		DEBUGNAME( "EV_NOAMMO" );
		if ( ( es->weapon != WP_GRENADE_LAUNCHER ) && ( es->weapon != WP_GRENADE_PINEAPPLE ) && ( es->weapon != WP_DYNAMITE ) ) {
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

		// TTimo
		// show_bug.cgi?id=417
		if ( ( newweap ) && ( cgs.gametype != GT_WOLF ) ) {
			CG_FinishWeaponChange( es->weapon, newweap );
		}

	}
	break;

//----(SA)	added
	case EV_SUGGESTWEAP:
		CG_WeaponSuggest( es->eventParm );
		break;
//----(SA)	end

	case EV_FIRE_WEAPON_MG42:
		// shake the camera a bit
		CG_StartShakeCamera( 0.05, 100, cent->lerpOrigin, 100 );
		trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, hWeaponSnd );
		DEBUGNAME( "EV_FIRE_WEAPON" );
		CG_FireWeapon( cent );
		break;
	case EV_FIRE_WEAPON:
	case EV_FIRE_WEAPONB:
		DEBUGNAME( "EV_FIRE_WEAPON" );
		CG_FireWeapon( cent );
		if ( event == EV_FIRE_WEAPONB ) {  // akimbo firing colt
			cent->akimboFire = qtrue;
		} else {
			cent->akimboFire = qfalse;
		}
		break;
	case EV_FIRE_WEAPON_LASTSHOT:
		DEBUGNAME( "EV_FIRE_WEAPON_LASTSHOT" );
		CG_FireWeapon( cent );
		break;

//----(SA)	added
	case EV_GRENADE_SUICIDE:
		DEBUGNAME( "EV_GRENADE_SUICIDE" );
		CG_MissileHitWall( WP_GRENADE_LAUNCHER, 0, position, dir, 0 );  // (SA) modified to send missilehitwall surface parameters
		break;
//----(SA)	end

//----(SA)	added
	case EV_FIRE_QUICKGREN:
		// testing.  no client side effect yet
		break;
//----(SA)	end
//----(SA)	added
	case EV_NOFIRE_UNDERWATER:
		DEBUGNAME( "EV_NOFIRE_UNDERWATER" );
		if ( es->number == cg.snap->ps.clientNum ) {   // reset client-side weapon animation
			cg.predictedPlayerState.weapAnim = ( ( cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | WEAP_IDLE1;
		}
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

//		CG_Printf("bounce on: %d\n", es->eventParm);

		// DYNAMITE
		if ( es->weapon == WP_DYNAMITE ) {
			trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.dynamitebounce1 );
		} else {
			int flags;
			// GRENADES
			// es->eventParm - surfaceparms
//#define	SURF_METAL              0x1000	// clanking footsteps

			flags = es->eventParm;
			flags = ( flags << 12 );
			if ( flags & SURF_WOOD ) { // SURF_WOOD
				if ( rand() & 1 ) {
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[GRENBOUNCE_WOOD][0] );
				} else { trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[GRENBOUNCE_WOOD][1] );}
			} else if ( flags & ( SURF_METAL | SURF_ROOF | SURF_GLASS ) ) { //	SURF_METAL | SURF_ROOF | SURF_GLASS
				if ( rand() & 1 ) {
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[GRENBOUNCE_METAL][0] );
				} else { trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[GRENBOUNCE_METAL][1] );}
			} else if ( flags & ( SURF_GRASS | SURF_GRAVEL | SURF_SNOW | SURF_CARPET ) ) {  //SURF_GRASS | SURF_GRAVEL | SURF_SNOW | SURF_CARPET
				if ( rand() & 1 ) {
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[GRENBOUNCE_DIRT][0] );
				} else { trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[GRENBOUNCE_DIRT][1] );}
			} else {
				if ( rand() & 1 ) {
					trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[GRENBOUNCE_DEFAULT][0] );
				} else { trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.grenadebounce[GRENBOUNCE_DEFAULT][1] );}
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

	case EV_BULLET_HIT_WALL:
		DEBUGNAME( "EV_BULLET_HIT_WALL" );
		ByteToDir( es->eventParm, dir );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD, qfalse, es->otherEntityNum2 );
		break;

	case EV_BULLET_HIT_FLESH:
		DEBUGNAME( "EV_BULLET_HIT_FLESH" );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm, qfalse, es->otherEntityNum2 );
		break;

	case EV_WOLFKICK_HIT_WALL:
		DEBUGNAME( "EV_WOLFKICK_HIT_WALL" );
		ByteToDir( es->eventParm, dir );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qfalse, ENTITYNUM_WORLD, qtrue, es->otherEntityNum2 );
		trap_S_StartSound( NULL, es->number, CHAN_AUTO, cgs.media.fkickwall );
		break;

	case EV_WOLFKICK_HIT_FLESH:
		DEBUGNAME( "EV_WOLFKICK_HIT_FLESH" );
		CG_Bullet( es->pos.trBase, es->otherEntityNum, dir, qtrue, es->eventParm, qtrue, es->otherEntityNum2 );
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

	case EV_CLOSEMENU:
		Menus_CloseAll();
		break;

	case EV_GIVEPAGE:
	{
		int havepages = cg_notebookpages.integer;
		havepages |= es->eventParm;
		trap_Cvar_Set( "cg_notebookpages", va( "%d", havepages ) );  // store new current page name for the ui to pick up
		trap_Cvar_Set( "cg_youGotMail", "1" );  //----(SA)	added
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
			break;  // RF, all sounds should have extension
			//Q_strncpyz( tempStr, s, sizeof(tempStr) );
			//Q_strcat( tempStr, sizeof(tempStr), ".wav" );
			//s = tempStr;
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
			break;  // RF, all sounds should have extension
			//Q_strncpyz( tempStr, s, sizeof(tempStr) );
			//Q_strcat( tempStr, sizeof(tempStr), ".wav" );
			//s = tempStr;
		}
		// done.
		if ( cgs.gameSounds[ es->eventParm ] ) {
			trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, cgs.gameSounds[ es->eventParm ] );
		} else {
			s = CG_ConfigString( CS_SOUNDS + es->eventParm );
			trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_AUTO, CG_CustomSound( es->number, s ) );
		}
		break;


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

//		trap_S_StartSoundEx(NULL, cent->currentState.number, CHAN_WEAPON, 0, SND_CUTOFF_ALL);	// kill weapon sound (could be reloading)
//		trap_S_StopStreamingSound( es->number );												// kill speech

		trap_S_StartSound( NULL, es->number, CHAN_VOICE,
						   CG_CustomSound( es->number, va( "*death%i.wav", event - EV_DEATH1 + 1 ) ) );
		break;

	case EV_ENTDEATH:
		DEBUGNAME( "EV_ENTDEATH" );
		switch ( es->eventParm ) {
//			case ET_SPOTLIGHT:
//				CG_Explodef(cent->lerpOrigin, normalized_direction, 50, 1, cgs.media.sfx_bullet_glasshit[0], 1, 0, cent->currentState.number, qfalse);
//				break;

		case ET_ALARMBOX:
			// all this crap shouldn't be in here, but we don't have a generic entry_point into
			// explosions that's reasonable.  This will be an early thing to get fixed in future work
		{
			int i, j;
			vec3_t sprVel, sprOrg;

			VectorCopy( cent->lerpAngles, dir );
			VectorNormalize( dir );

			// explosion sprite animation
//					VectorScale( dir, 16, sprVel );
			VectorScale( dir, 6, sprVel );

			for ( i = 0; i < 5; i++ ) {
				for ( j = 0; j < 3; j++ )
					sprOrg[j] = cent->lerpOrigin[j] + 2 * dir[j] + 4 * crandom();
				sprVel[2] += rand() % 10;
				CG_ParticleExplosion( "blacksmokeanimb", sprOrg, sprVel, 3500 + rand() % 250, 4, 50 + rand() % 20 );
			}

			CG_AddDebris(   cent->lerpOrigin,
							dir,
							80,                 // speed
							1000,                   // duration
							3 + rand() % 2 );       // count
		}
		break;
		}

		break;

	case EV_OBITUARY:
		DEBUGNAME( "EV_OBITUARY" );
		CG_Obituary( es );
		break;

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
			trap_S_StartSound( es->pos.trBase, es->number, CHAN_VOICE, cgs.media.zombieDeathSound );
		} else {
			trap_S_StartSound( es->pos.trBase, es->number, CHAN_VOICE, cgs.media.gibSound );
		}
		ByteToDir( es->eventParm, dir );
		CG_GibPlayer( cent, cent->lerpOrigin, dir );
		break;

//----(SA)	added
	case EV_STOPSTREAMINGSOUND:
		DEBUGNAME( "EV_STOPLOOPINGSOUND" );
		trap_S_StopStreamingSound( es->number );

		// hope this does not cause trouble.
		// can re-work if this causes trouble
		// this is only called on death now, so stop the weapon sound now too
		trap_S_StartSoundEx( NULL, es->number, CHAN_WEAPON, 0, SND_CUTOFF_ALL );  // kill weapon sound (could be reloading)

		break;
//----(SA)	end

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
		if ( cent->currentState.density == 3 ) {          // cannon
			CG_ParticleSmoke( cgs.media.smokePuffShaderdirty, cent );
		} else if ( cent->currentState.density == 7 ) {    // steam
			// steam from panzerfaust casing
			CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, cent->currentState.origin, tv( 0,0,1 ), 8, 1000, 8, 20, 20, 0.25f );
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
//				CG_FireFlameChunks( cent, cent->currentState.origin, cent->lerpAngles, 0.1, qfalse, 1 );
//				CG_FireFlameChunks( cent, cent->currentState.origin, cent->currentState.apos.trBase, 0.1, qfalse, 1 );
//			}
//			else
//				CG_FireFlameChunks( cent, cent->currentState.origin, cent->lerpAngles, 0.6, 2, 1 );
		CG_FireFlameChunks( cent, cent->currentState.origin, cent->currentState.apos.trBase, 0.6, 2, 1 );

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

// generic particle emitter that uses client-side particle scripts
	case EV_EMITTER:
	{
		localEntity_t   *le;
		le = CG_AllocLocalEntity();
		le->leType = LE_EMITTER;
		le->startTime = cg.time;
		le->endTime = le->startTime + cent->currentState.time;      // 'time' stores lifetime
		le->pos.trType = TR_STATIONARY;
		VectorCopy( cent->currentState.origin, le->pos.trBase );
		VectorCopy( cent->currentState.origin2, le->angles.trBase );
		le->ownerNum = 0;
		le->radius = cent->currentState.density;        //	'density' stores pressure
		le->headJuncIndex = cent->currentState.teamNum;     // 'type'
	}
	break;

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

	case EV_SPAWN_SPIRIT:
		CG_SpawnSpirit( cent );
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
			goto skipEvent;
			//return;	// already fired
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

skipEvent:

	// check the sequencial list
	// if the eventSequence is zero, then there are no events
	if ( !cent->currentState.eventSequence ) {
		cent->previousEventSequence = 0;
	}
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
