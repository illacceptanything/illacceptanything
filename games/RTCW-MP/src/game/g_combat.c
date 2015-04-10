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

/*
 * name:		g_combat.c
 *
 * desc:
 *
*/

#include "g_local.h"


/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore( gentity_t *ent, int score ) {
	if ( !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}

	// Ridah, no scoring during single player
	// DHM - Nerve :: fix typo
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		return;
	}
	// done.


	ent->client->ps.persistant[PERS_SCORE] += score;
	if ( g_gametype.integer >= GT_TEAM ) { // JPW NERVE changed to >=
		level.teamScores[ ent->client->ps.persistant[PERS_TEAM] ] += score;
	}
	CalculateRanks();
}

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems( gentity_t *self ) {
	gitem_t     *item;
	int weapon;
	gentity_t   *drop = 0;

	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a client
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.

	// (SA) always drop what you were switching to
	if ( 1 ) {
		if ( self->client->ps.weaponstate == WEAPON_DROPPING ) {
			weapon = self->client->pers.cmd.weapon;
		}
		if ( !( COM_BitCheck( self->client->ps.weapons, weapon ) ) ) {
			weapon = WP_NONE;
		}
	}

	// JPW NERVE don't drop these weapon types
	if ( ( weapon == WP_FLAMETHROWER ) || ( weapon == WP_GARAND ) || ( weapon == WP_MAUSER ) || ( weapon == WP_VENOM ) ) {
		weapon = WP_NONE;
	}
	// jpw

	if ( weapon > WP_NONE && weapon < WP_MONSTER_ATTACK1 && self->client->ps.ammo[ BG_FindAmmoForWeapon( weapon )] ) {
		// find the item type for this weapon
		item = BG_FindItemForWeapon( weapon );
		// spawn the item

		// Rafael
		if ( !( self->client->ps.persistant[PERS_HWEAPON_USE] ) ) {
			drop = Drop_Item( self, item, 0, qfalse );
			// JPW NERVE -- fix ammo counts
			drop->count = self->client->ps.ammoclip[BG_FindClipForWeapon( weapon )];
			drop->item->quantity = self->client->ps.ammoclip[BG_FindClipForWeapon( weapon )];
			// jpw
		}
	}
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t dir;
	vec3_t angles;

	if ( attacker && attacker != self ) {
		VectorSubtract( attacker->s.pos.trBase, self->s.pos.trBase, dir );
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract( inflictor->s.pos.trBase, self->s.pos.trBase, dir );
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw( dir );

	angles[YAW] = vectoyaw( dir );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
}


/*
==============
GibHead
==============
*/
void GibHead( gentity_t *self, int killer ) {
	G_AddEvent( self, EV_GIB_HEAD, killer );
}

/*
==================
GibEntity
==================
*/
void GibEntity( gentity_t *self, int killer ) {
	gentity_t *other = &g_entities[killer];
	vec3_t dir;

	VectorClear( dir );
	if ( other->inuse ) {
		if ( other->client ) {
			VectorSubtract( self->r.currentOrigin, other->r.currentOrigin, dir );
			VectorNormalize( dir );
		} else if ( !VectorCompare( other->s.pos.trDelta, vec3_origin ) ) {
			VectorNormalize2( other->s.pos.trDelta, dir );
		}
	}

	G_AddEvent( self, EV_GIB_PLAYER, DirToByte( dir ) );
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

/*
==================
body_die
==================
*/
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( self->health > GIB_HEALTH ) {
		return;
	}

	GibEntity( self, 0 );
}


// these are just for logging, the client prints its own messages
char    *modNames[] = {
	"MOD_UNKNOWN",
	"MOD_SHOTGUN",
	"MOD_GAUNTLET",
	"MOD_MACHINEGUN",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_RAILGUN",
	"MOD_LIGHTNING",
	"MOD_BFG",
	"MOD_BFG_SPLASH",
	"MOD_KNIFE",
	"MOD_KNIFE2",
	"MOD_KNIFE_STEALTH",
	"MOD_LUGER",
	"MOD_COLT",
	"MOD_MP40",
	"MOD_THOMPSON",
	"MOD_STEN",
	"MOD_MAUSER",
	"MOD_SNIPERRIFLE",
	"MOD_GARAND",
	"MOD_SNOOPERSCOPE",
	"MOD_SILENCER", //----(SA)
	"MOD_AKIMBO",    //----(SA)
	"MOD_BAR",   //----(SA)
	"MOD_FG42",
	"MOD_FG42SCOPE",
	"MOD_PANZERFAUST",
	"MOD_ROCKET_LAUNCHER",
	"MOD_GRENADE_LAUNCHER",
	"MOD_VENOM",
	"MOD_VENOM_FULL",
	"MOD_FLAMETHROWER",
	"MOD_TESLA",
	"MOD_SPEARGUN",
	"MOD_SPEARGUN_CO2",
	"MOD_GRENADE_PINEAPPLE",
	"MOD_CROSS",
	"MOD_MORTAR",
	"MOD_MORTAR_SPLASH",
	"MOD_KICKED",
	"MOD_GRABBER",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
	"MOD_GRAPPLE",
	"MOD_EXPLOSIVE",
	"MOD_POISONGAS",
	"MOD_ZOMBIESPIT",
	"MOD_ZOMBIESPIT_SPLASH",
	"MOD_ZOMBIESPIRIT",
	"MOD_ZOMBIESPIRIT_SPLASH",
	"MOD_LOPER_LEAP",
	"MOD_LOPER_GROUND",
	"MOD_LOPER_HIT",
// JPW NERVE
	"MOD_LT_ARTILLERY",
	"MOD_LT_AIRSTRIKE",
	"MOD_ENGINEER",  // not sure if we'll use
	"MOD_MEDIC",     // these like this or not
// jpw
	"MOD_BAT"
};

/*
==================
player_die
==================
*/
void limbo( gentity_t *ent, qboolean makeCorpse ); // JPW NERVE

void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t   *ent;
	// TTimo might be used uninitialized
	int contents = 0;
	int killer;
	int i;
	char        *killerName, *obit;
	qboolean nogib = qtrue;
	gitem_t     *item = NULL; // JPW NERVE for flag drop
	vec3_t launchvel,launchspot;      // JPW NERVE
	gentity_t   *flag; // JPW NERVE

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}

	self->client->ps.pm_type = PM_DEAD;

	G_AddEvent( self, EV_STOPSTREAMINGSOUND, 0 );

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->client ) {
			killerName = attacker->client->pers.netname;
		} else {
			killerName = "<non-client>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= sizeof( modNames ) / sizeof( modNames[0] ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[ meansOfDeath ];
	}

	G_LogPrintf( "Kill: %i %i %i: %s killed %s by %s\n",
				 killer, self->s.number, meansOfDeath, killerName,
				 self->client->pers.netname, obit );

	// broadcast the death event to everyone
	ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

// JPW NERVE -- if player is holding ticking grenade, drop it
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		if ( ( self->client->ps.grenadeTimeLeft ) && ( self->s.weapon != WP_DYNAMITE ) ) {
			launchvel[0] = crandom();
			launchvel[1] = crandom();
			launchvel[2] = random();
			VectorScale( launchvel, 160, launchvel );
			VectorCopy( self->r.currentOrigin, launchspot );
			launchspot[2] += 40;
			fire_grenade( self, launchspot, launchvel, self->s.weapon );

		}
	}
// jpw

	if ( attacker && attacker->client ) {
		if ( attacker == self || OnSameTeam( self, attacker ) ) {

			// DHM - Nerve :: Complaint lodging
			if ( attacker != self && level.warmupTime <= 0 ) {
				if ( attacker->client->pers.localClient ) {
					trap_SendServerCommand( self - g_entities, "complaint -4" );
				} else {
					trap_SendServerCommand( self - g_entities, va( "complaint %i", attacker->s.number ) );
					self->client->pers.complaintClient = attacker->s.clientNum;
					self->client->pers.complaintEndTime = level.time + 20500;
				}
			}
			// dhm

			// JPW NERVE
			if ( g_gametype.integer >= GT_WOLF ) { // high penalty to offset medic heal
				AddScore( attacker, WOLF_FRIENDLY_PENALTY );
			} else {
				// jpw
				AddScore( attacker, -1 );
			}
		} else {
			// JPW NERVE -- mostly added as conveneience so we can tweak from the #defines all in one place
			if ( g_gametype.integer >= GT_WOLF ) {
				AddScore( attacker, WOLF_FRAG_BONUS );
			} else {
				// jpw
				AddScore( attacker, 1 );
			}

			attacker->client->lastKillTime = level.time;
		}
	} else {
		AddScore( self, -1 );
	}

	// Add team bonuses
	Team_FragBonuses( self, inflictor, attacker );

	// if client is in a nodrop area, don't drop anything
// JPW NERVE new drop behavior
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {   // only drop here in single player; in multiplayer, drop @ limbo
		contents = trap_PointContents( self->r.currentOrigin, -1 );
		if ( !( contents & CONTENTS_NODROP ) ) {
			TossClientItems( self );
		}
	}

	// drop flag regardless
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		if ( self->client->ps.powerups[PW_REDFLAG] ) {
			item = BG_FindItem( "Red Flag" );
			if ( !item ) {
				item = BG_FindItem( "Objective" );
			}

			self->client->ps.powerups[PW_REDFLAG] = 0;
		}
		if ( self->client->ps.powerups[PW_BLUEFLAG] ) {
			item = BG_FindItem( "Blue Flag" );
			if ( !item ) {
				item = BG_FindItem( "Objective" );
			}

			self->client->ps.powerups[PW_BLUEFLAG] = 0;
		}

		if ( item ) {
			launchvel[0] = crandom() * 20;
			launchvel[1] = crandom() * 20;
			launchvel[2] = 10 + random() * 10;

			flag = LaunchItem( item,self->r.currentOrigin,launchvel,self->s.number );
			flag->s.modelindex2 = self->s.otherEntityNum2; // JPW NERVE FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
			flag->message = self->message;  // DHM - Nerve :: also restore item name
			// Clear out player's temp copies
			self->s.otherEntityNum2 = 0;
			self->message = NULL;
		}

		// send a fancy "MEDIC!" scream.  Sissies, ain' they?
		if ( self->client != NULL ) {
			if ( self->health > GIB_HEALTH && meansOfDeath != MOD_SUICIDE ) {

				if ( self->client->sess.sessionTeam == TEAM_RED ) {
					if ( random() > 0.5 ) {
						G_AddEvent( self, EV_GENERAL_SOUND, G_SoundIndex( "sound/multiplayer/axis/g-medic2.wav" ) );
					} else {
						G_AddEvent( self, EV_GENERAL_SOUND, G_SoundIndex( "sound/multiplayer/axis/g-medic3.wav" ) );
					}
				} else {
					if ( random() > 0.5 ) {
						G_AddEvent( self, EV_GENERAL_SOUND, G_SoundIndex( "sound/multiplayer/allies/a-medic3.wav" ) );
					} else {
						G_AddEvent( self, EV_GENERAL_SOUND, G_SoundIndex( "sound/multiplayer/allies/a-medic2.wav" ) );
					}
				}
			}
		}
	}
// jpw

	Cmd_Score_f( self );        // show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t   *client;

		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if ( client->sess.spectatorClient == self->s.number ) {
			Cmd_Score_f( g_entities + i );
		}
	}

	self->takedamage = qtrue;   // can still be gibbed
	self->r.contents = CONTENTS_CORPSE;

	self->s.powerups = 0;
// JPW NERVE -- only corpse in SP; in MP, need CONTENTS_BODY so medic can operate
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		self->s.weapon = WP_NONE;
		self->s.angles[0] = 0;
	} else {
		self->client->limboDropWeapon = self->s.weapon; // store this so it can be dropped in limbo
	}
// jpw
	self->s.angles[2] = 0;
	LookAtKiller( self, inflictor, attacker );

	VectorCopy( self->s.angles, self->client->ps.viewangles );
	self->s.loopSound = 0;

	trap_UnlinkEntity( self );
	self->r.maxs[2] = 0;
	self->client->ps.maxs[2] = 0;
	trap_LinkEntity( self );

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + 800;

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof( self->client->ps.powerups ) );

	// never gib in a nodrop
	if ( self->health <= GIB_HEALTH && !( contents & CONTENTS_NODROP ) ) {
		GibEntity( self, killer );
		nogib = qfalse;
	}

	if ( nogib ) {
		// normal death
		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if ( self->health <= GIB_HEALTH ) {
			self->health = GIB_HEALTH + 1;
		}

// JPW NERVE for medic
		self->client->medicHealAmt = 0;
// jpw

		// DHM - Play death animation, and set pm_time to delay 'fallen' animation
		self->client->ps.pm_time = BG_AnimScriptEvent( &self->client->ps, ANIM_ET_DEATH, qfalse, qtrue );

		G_AddEvent( self, EV_DEATH1 + 1, killer );

		// the body can still be gibbed
		self->die = body_die;
	}

	trap_LinkEntity( self );

	if ( g_gametype.integer >= GT_WOLF && meansOfDeath == MOD_SUICIDE ) {
		limbo( self, qtrue );
	}
}


/*
================
CheckArmor
================
*/
int CheckArmor( gentity_t *ent, int damage, int dflags ) {
	gclient_t   *client;
	int save;
	int count;

	if ( !damage ) {
		return 0;
	}

	client = ent->client;

	if ( !client ) {
		return 0;
	}

	if ( dflags & DAMAGE_NO_ARMOR ) {
		return 0;
	}

	// armor
	count = client->ps.stats[STAT_ARMOR];
	save = ceil( damage * ARMOR_PROTECTION );
	if ( save >= count ) {
		save = count;
	}

	if ( !save ) {
		return 0;
	}

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}

qboolean IsHeadShotWeapon( int mod, qboolean aicharacter ) {
	if ( aicharacter ) {       // ai's are allowed headshots from these weapons
		if ( mod == MOD_SNIPERRIFLE ||
			 mod == MOD_SNOOPERSCOPE ) {
			return qtrue;
		}

		return qfalse;
	}

	// players are allowed headshots from these weapons
	if (    mod == MOD_LUGER ||
			mod == MOD_COLT ||
			mod == MOD_AKIMBO ||    //----(SA)	added
			mod == MOD_MP40 ||
			mod == MOD_THOMPSON ||
			mod == MOD_STEN ||
			mod == MOD_BAR ||
			mod == MOD_FG42 ||
			mod == MOD_FG42SCOPE ||
			mod == MOD_MAUSER ||
			mod == MOD_GARAND || // JPW NERVE this was left out
			mod == MOD_SNIPERRIFLE ||
			mod == MOD_SNOOPERSCOPE ||
			mod == MOD_SILENCER ||  //----(SA)	modified
			mod == MOD_SNIPERRIFLE ) {
		return qtrue;
	}

	return qfalse;
}

qboolean IsHeadShot( gentity_t *targ, qboolean isAICharacter, vec3_t dir, vec3_t point, int mod ) {
	gentity_t   *head;
	trace_t tr;
	vec3_t start, end;
	gentity_t   *traceEnt;
	orientation_t or;           // DHM - Nerve

	qboolean head_shot_weapon = qfalse;

	// not a player or critter so bail
	if ( !( targ->client ) ) {
		return qfalse;
	}

	if ( targ->health <= 0 ) {
		return qfalse;
	}

	head_shot_weapon = IsHeadShotWeapon( mod, isAICharacter );

	if ( head_shot_weapon ) {
		head = G_Spawn();

		if ( trap_GetTag( targ->s.number, "tag_head", &or ) ) {
			G_SetOrigin( head, or.origin );
		} else {
			float height, dest;
			vec3_t v, angles, forward, up, right;

			G_SetOrigin( head, targ->r.currentOrigin );

			if ( targ->client->ps.pm_flags & PMF_DUCKED ) { // closer fake offset for 'head' box when crouching
				height = targ->client->ps.crouchViewHeight - 12;
			} else {
				height = targ->client->ps.viewheight;
			}

			// NERVE - SMF - this matches more closely with WolfMP models
			VectorCopy( targ->client->ps.viewangles, angles );
			if ( angles[PITCH] > 180 ) {
				dest = ( -360 + angles[PITCH] ) * 0.75;
			} else {
				dest = angles[PITCH] * 0.75;
			}
			angles[PITCH] = dest;

			AngleVectors( angles, forward, right, up );
			VectorScale( forward, 5, v );
			VectorMA( v, 18, up, v );

			VectorAdd( v, head->r.currentOrigin, head->r.currentOrigin );
			head->r.currentOrigin[2] += height / 2;
			// -NERVE - SMF
		}

		VectorCopy( head->r.currentOrigin, head->s.origin );
		VectorCopy( targ->r.currentAngles, head->s.angles );
		VectorCopy( head->s.angles, head->s.apos.trBase );
		VectorCopy( head->s.angles, head->s.apos.trDelta );
		VectorSet( head->r.mins, -6, -6, -2 ); // JPW NERVE changed this z from -12 to -6 for crouching, also removed standing offset
		VectorSet( head->r.maxs, 6, 6, 10 ); // changed this z from 0 to 6
		head->clipmask = CONTENTS_SOLID;
		head->r.contents = CONTENTS_SOLID;

		trap_LinkEntity( head );

		// trace another shot see if we hit the head
		VectorCopy( point, start );
		VectorMA( start, 64, dir, end );
		trap_Trace( &tr, start, NULL, NULL, end, targ->s.number, MASK_SHOT );

		traceEnt = &g_entities[ tr.entityNum ];

		if ( g_debugBullets.integer >= 3 ) {   // show hit player head bb
			gentity_t *tent;
			vec3_t b1, b2;
			VectorCopy( head->r.currentOrigin, b1 );
			VectorCopy( head->r.currentOrigin, b2 );
			VectorAdd( b1, head->r.mins, b1 );
			VectorAdd( b2, head->r.maxs, b2 );
			tent = G_TempEntity( b1, EV_RAILTRAIL );
			VectorCopy( b2, tent->s.origin2 );
			tent->s.dmgFlags = 1;

			// show headshot trace
			// end the headshot trace at the head box if it hits
			if ( tr.fraction != 1 ) {
				VectorMA( start, ( tr.fraction * 64 ), dir, end );
			}
			tent = G_TempEntity( start, EV_RAILTRAIL );
			VectorCopy( end, tent->s.origin2 );
			tent->s.dmgFlags = 0;
		}

		G_FreeEntity( head );

		if ( traceEnt == head ) {
			level.totalHeadshots++;         // NERVE - SMF
			return qtrue;
		} else {
			level.missedHeadshots++;    // NERVE - SMF
		}
	}

	return qfalse;
}

gentity_t* G_BuildHead( gentity_t *ent ) {
	gentity_t* head;
	orientation_t or;           // DHM - Nerve

	head = G_Spawn();

	if ( trap_GetTag( ent->s.number, "tag_head", &or ) ) {
		G_SetOrigin( head, or.origin );
	} else {
		float height, dest;
		vec3_t v, angles, forward, up, right;

		G_SetOrigin( head, ent->r.currentOrigin );

		if ( ent->client->ps.pm_flags & PMF_DUCKED ) { // closer fake offset for 'head' box when crouching
			height = ent->client->ps.crouchViewHeight - 12;
		} else {
			height = ent->client->ps.viewheight;
		}

		// NERVE - SMF - this matches more closely with WolfMP models
		VectorCopy( ent->client->ps.viewangles, angles );
		if ( angles[PITCH] > 180 ) {
			dest = ( -360 + angles[PITCH] ) * 0.75;
		} else {
			dest = angles[PITCH] * 0.75;
		}
		angles[PITCH] = dest;

		AngleVectors( angles, forward, right, up );
		VectorScale( forward, 5, v );
		VectorMA( v, 18, up, v );

		VectorAdd( v, head->r.currentOrigin, head->r.currentOrigin );
		head->r.currentOrigin[2] += height / 2;
		// -NERVE - SMF
	}

	VectorCopy( head->r.currentOrigin, head->s.origin );
	VectorCopy( ent->r.currentAngles, head->s.angles );
	VectorCopy( head->s.angles, head->s.apos.trBase );
	VectorCopy( head->s.angles, head->s.apos.trDelta );
	VectorSet( head->r.mins, -6, -6, -2 ); // JPW NERVE changed this z from -12 to -6 for crouching, also removed standing offset
	VectorSet( head->r.maxs, 6, 6, 10 ); // changed this z from 0 to 6
	head->clipmask = CONTENTS_SOLID;
	head->r.contents = CONTENTS_SOLID;
	head->parent = ent;
	head->s.eType = ET_TEMPHEAD;

	trap_LinkEntity( head );

	return head;
}

/*
==============
G_ArmorDamage
	brokeparts is how many should be broken off now
	curbroke is how many are broken
	the difference is how many to pop off this time
==============
*/
void G_ArmorDamage( gentity_t *targ ) {
	int brokeparts, curbroke;
	int numParts;
	int dmgbits = 16;   // 32/2;
	int i;

	if ( !targ->client ) {
		return;
	}

	if ( targ->s.aiChar == AICHAR_PROTOSOLDIER ) {
		numParts = 9;
	} else if ( targ->s.aiChar == AICHAR_SUPERSOLDIER ) {
		numParts = 14;
	} else if ( targ->s.aiChar == AICHAR_HEINRICH ) {
		numParts = 20;
	} else {
		return;
	}

	if ( numParts > dmgbits ) {
		numParts = dmgbits; // lock this down so it doesn't overwrite any bits that it shouldn't.  TODO: fix this


	}
	// determined here (on server) by location of hit and existing armor, you're updating here so
	// the client knows which pieces are still in place, and by difference with previous state, which
	// pieces to play an effect where the part is blown off.
	// Need to do it here so we have info on where the hit registered (head, torso, legs or if we go with more detail; arm, leg, chest, codpiece, etc)

	// ... Ick, just discovered that the refined hit detection ("hit nearest to which tag") is clientside...

	// For now, I'll randomly pick a part that hasn't been cleared.  This might end up looking okay, and we won't need the refined hits.
	//	however, we still have control on the server-side of which parts come off, regardless of what shceme is used.

	brokeparts = (int)( ( 1 - ( (float)( targ->health ) / (float)( targ->client->ps.stats[STAT_MAX_HEALTH] ) ) ) * numParts );

	if ( brokeparts && ( ( targ->s.dmgFlags & ( ( 1 << numParts ) - 1 ) ) != ( 1 << numParts ) - 1 ) ) {   // there are still parts left to clear

		// how many are removed already?
		curbroke = 0;
		for ( i = 0; i < numParts; i++ ) {
			if ( targ->s.dmgFlags & ( 1 << i ) ) {
				curbroke++;
			}
		}

		// need to remove more
		if ( brokeparts - curbroke >= 1 && curbroke < numParts ) {
			for ( i = 0; i < ( brokeparts - curbroke ); i++ ) {
				int remove = rand() % ( numParts );

				if ( !( ( targ->s.dmgFlags & ( ( 1 << numParts ) - 1 ) ) != ( 1 << numParts ) - 1 ) ) { // no parts are available any more
					break;
				}

				// FIXME: lose the 'while' loop.  Still should be safe though, since the check above verifies that it will eventually find a valid part
				while ( targ->s.dmgFlags & ( 1 << remove ) ) {
					remove = rand() % ( numParts );
				}

				targ->s.dmgFlags |= ( 1 << remove );    // turn off 'undamaged' part
				if ( (int)( random() + 0.5 ) ) {                       // choose one of two possible replacements
					targ->s.dmgFlags |= ( 1 << ( numParts + remove ) );
				}
			}
		}
	}
}

/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t   *client;
	int take;
	int save;
	int asave;
	int knockback;

	if ( !targ->takedamage ) {
		return;
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued || g_gamestate.integer != GS_PLAYING ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

// JPW NERVE
	if ( ( targ->waterlevel >= 3 ) && ( mod == MOD_FLAMETHROWER ) ) {
		return;
	}
// jpw

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER && !( targ->aiName ) && !( targ->isProp ) && !targ->scriptName ) {
		if ( targ->use && targ->moverState == MOVER_POS1 ) {
			targ->use( targ, inflictor, attacker );
		}
		return;
	}

	if ( targ->s.eType == ET_MOVER && targ->aiName && !( targ->isProp ) && !targ->scriptName ) {
		switch ( mod ) {
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			break;
		default:
			return; // no damage from other weapons
		}
	} else if ( targ->s.eType == ET_EXPLOSIVE )   {
		// 32 Explosive
		// 64 Dynamite only
		if ( ( targ->spawnflags & 32 ) || ( targ->spawnflags & 64 ) ) {
			switch ( mod ) {
			case MOD_GRENADE:
			case MOD_GRENADE_SPLASH:
			case MOD_ROCKET:
			case MOD_ROCKET_SPLASH:
			case MOD_AIRSTRIKE:
			case MOD_GRENADE_PINEAPPLE:
			case MOD_MORTAR:
			case MOD_MORTAR_SPLASH:
			case MOD_EXPLOSIVE:
				if ( targ->spawnflags & 64 ) {
					return;
				}

				break;

			case MOD_DYNAMITE:
			case MOD_DYNAMITE_SPLASH:
				break;

			default:
				return;
			}
		}
	}

	client = targ->client;

	if ( client ) {
		if ( client->noclip ) {
			return;
		}
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize( dir );
	}

	knockback = damage;
	if ( knockback > 200 ) {
		knockback = 200;
	}
	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client && ( g_friendlyFire.integer || !OnSameTeam( targ, attacker ) ) ) {
		vec3_t kvel;
		float mass;

		mass = 200;

		if ( mod == MOD_LIGHTNING && !( ( level.time + targ->s.number * 50 ) % 400 ) ) {
			knockback = 60;
			dir[2] = 0.3;
		}

		VectorScale( dir, g_knockback.value * (float)knockback / mass, kvel );
		VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );

		if ( targ == attacker && !(  mod != MOD_ROCKET &&
									 mod != MOD_ROCKET_SPLASH &&
									 mod != MOD_GRENADE &&
									 mod != MOD_GRENADE_SPLASH &&
									 mod != MOD_DYNAMITE ) ) {
			targ->client->ps.velocity[2] *= 0.25;
		}

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !targ->client->ps.pm_time ) {
			int t;

			t = knockback * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if ( !( dflags & DAMAGE_NO_PROTECTION ) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( targ != attacker && OnSameTeam( targ, attacker )  ) {
			if ( !g_friendlyFire.integer ) {
				return;
			}
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return;
		}

		// RF, warzombie defense position is basically godmode for the time being
		if ( targ->flags & FL_DEFENSE_GUARD ) {
			return;
		}

		// check for invulnerability // (SA) moved from below so DAMAGE_NO_PROTECTION will still work
		if ( client && client->ps.powerups[PW_INVULNERABLE] ) { //----(SA)	added
			return;
		}

	}

	// battlesuit protects from all radius damage (but takes knockback)
	// and protects 50% against all damage
	if ( client && client->ps.powerups[PW_BATTLESUIT] ) {
		G_AddEvent( targ, EV_POWERUP_BATTLESUIT, 0 );
		if ( dflags & DAMAGE_RADIUS ) {
			return;
		}
		damage *= 0.5;
	}

	// add to the attacker's hit counter
	if ( attacker->client && targ != attacker && targ->health > 0 ) {
		if ( OnSameTeam( targ, attacker ) ) {
			attacker->client->ps.persistant[PERS_HITS] -= damage;
		} else {
			attacker->client->ps.persistant[PERS_HITS] += damage;
		}
	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;
	save = 0;

	// save some from armor
	asave = CheckArmor( targ, take, dflags );
	take -= asave;

	if ( IsHeadShot( targ, qfalse, dir, point, mod ) ) {

		if ( take * 2 < 50 ) { // head shots, all weapons, do minimum 50 points damage
			take = 50;
		} else {
			take *= 2; // sniper rifles can do full-kill (and knock into limbo)

		}
		if ( !( targ->client->ps.eFlags & EF_HEADSHOT ) ) {  // only toss hat on first headshot
			G_AddEvent( targ, EV_LOSE_HAT, DirToByte( dir ) );
		}

		targ->client->ps.eFlags |= EF_HEADSHOT;
	}

	if ( g_debugDamage.integer ) {
		G_Printf( "client:%i health:%i damage:%i armor:%i\n", targ->s.number,
				  targ->health, take, asave );
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;

		if ( dir ) {
			VectorCopy( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
	Team_CheckHurtCarrier( targ, attacker );

	if ( targ->client ) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	// do the damage
	if ( take ) {
		targ->health = targ->health - take;

		// Ridah, can't gib with bullet weapons (except VENOM)
		if ( mod != MOD_VENOM && attacker == inflictor && targ->health <= GIB_HEALTH ) {
			if ( targ->aiCharacter != AICHAR_ZOMBIE ) { // zombie needs to be able to gib so we can kill him (although he doesn't actually GIB, he just dies)
				targ->health = GIB_HEALTH + 1;
			}
		}

// JPW NERVE overcome previous chunk of code for making grenades work again
		if ( ( g_gametype.integer != GT_SINGLE_PLAYER ) && ( take > 190 ) ) { // 190 is greater than 2x mauser headshot, so headshots don't gib
			targ->health = GIB_HEALTH - 1;
		}
// jpw
		//G_Printf("health at: %d\n", targ->health);
		if ( targ->health <= 0 ) {
			if ( client ) {
				targ->flags |= FL_NO_KNOCKBACK;
// JPW NERVE -- repeated shooting sends to limbo
				if ( g_gametype.integer >= GT_WOLF ) {
					if ( ( targ->health < FORCE_LIMBO_HEALTH ) && ( targ->health > GIB_HEALTH ) && ( !( targ->client->ps.pm_flags & PMF_LIMBO ) ) ) {
						limbo( targ, qtrue );
					}
				}
// jpw
			}

			if ( targ->health < -999 ) {
				targ->health = -999;
			}

			targ->enemy = attacker;
			if ( targ->die ) { // Ridah, mg42 doesn't have die func (FIXME)
				targ->die( targ, inflictor, attacker, take, mod );
			}

			// if we freed ourselves in death function
			if ( !targ->inuse ) {
				return;
			}

			// RF, entity scripting
			if ( targ->s.number >= MAX_CLIENTS && targ->health <= 0 ) { // might have revived itself in death function
				G_Script_ScriptEvent( targ, "death", "" );
			}

		} else if ( targ->pain ) {
			if ( dir ) {  // Ridah, had to add this to fix NULL dir crash
				VectorCopy( dir, targ->rotate );
				VectorCopy( point, targ->pos3 ); // this will pass loc of hit
			} else {
				VectorClear( targ->rotate );
				VectorClear( targ->pos3 );
			}

			targ->pain( targ, attacker, take, point );

			// RF, entity scripting
			if ( targ->s.number >= MAX_CLIENTS ) {
				G_Script_ScriptEvent( targ, "pain", va( "%d %d", targ->health, targ->health + take ) );
			}
		}

		//G_ArmorDamage(targ);	//----(SA)	moved out to separate routine

		// Ridah, this needs to be done last, incase the health is altered in one of the event calls
		if ( targ->client ) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}
	}

}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage( gentity_t *targ, vec3_t origin ) {
	vec3_t dest;
	trace_t tr;
	vec3_t midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd( targ->r.absmin, targ->r.absmax, midpoint );
	VectorScale( midpoint, 0.5, midpoint );

	VectorCopy( midpoint, dest );
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}



	if ( &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	VectorCopy( midpoint, dest );
	dest[0] += 15.0;
	dest[1] += 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}


	return qfalse;
}


/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius,
						 gentity_t *ignore, int mod ) {
	float points, dist;
	gentity_t   *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	int i, e;
	qboolean hitClient = qfalse;
// JPW NERVE
	float boxradius;
	vec3_t dest;
	trace_t tr;
	vec3_t midpoint;
// jpw


	if ( radius < 1 ) {
		radius = 1;
	}

	boxradius = 1.41421356 * radius; // radius * sqrt(2) for bounding box enlargement --
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - boxradius; // JPW NERVE
		maxs[i] = origin[i] + boxradius; // JPW NERVE
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if ( ent == ignore ) {
			continue;
		}
		if ( !ent->takedamage ) {
			continue;
		}

/* JPW NERVE -- we can put this back if we need to, but it kinna sucks for human-sized bboxes
		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}
*/
// JPW NERVE
		if ( !ent->r.bmodel ) {
			VectorSubtract( ent->r.currentOrigin,origin,v ); // JPW NERVE simpler centroid check that doesn't have box alignment weirdness
		} else {
			for ( i = 0 ; i < 3 ; i++ ) {
				if ( origin[i] < ent->r.absmin[i] ) {
					v[i] = ent->r.absmin[i] - origin[i];
				} else if ( origin[i] > ent->r.absmax[i] ) {
					v[i] = origin[i] - ent->r.absmax[i];
				} else {
					v[i] = 0;
				}
			}
		}
// jpw
		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

// JPW NERVE -- different radiusdmg behavior for MP -- big explosions should do less damage (over less distance) through failed traces
		if ( CanDamage( ent, origin ) ) {
			if ( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage( ent, NULL, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod );
		}
// JPW NERVE --  MP weapons should do 1/8 damage through walls over 1/8th distance
		else {
			if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
				VectorAdd( ent->r.absmin, ent->r.absmax, midpoint );
				VectorScale( midpoint, 0.5, midpoint );
				VectorCopy( midpoint, dest );

				trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
				if ( tr.fraction < 1.0 ) {
					VectorSubtract( dest,origin,dest );
					dist = VectorLength( dest );
					if ( dist < radius * 0.2f ) { // closer than 1/4 dist
						if ( LogAccuracyHit( ent, attacker ) ) {
							hitClient = qtrue;
						}
						VectorSubtract( ent->r.currentOrigin, origin, dir );
						dir[2] += 24;
						G_Damage( ent, NULL, attacker, dir, origin, (int)( points * 0.1f ), DAMAGE_RADIUS, mod );
					}
				}
			}
		}
// jpw
	}
	return hitClient;
}
