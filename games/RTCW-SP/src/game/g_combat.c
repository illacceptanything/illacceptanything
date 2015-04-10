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
	if ( g_gametype.integer == GT_TEAM ) {
		level.teamScores[ ent->client->ps.persistant[PERS_TEAM] ] += score;
	}
	CalculateRanks();
}



extern qboolean G_ThrowChair( gentity_t *ent, vec3_t dir, qboolean force );

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems( gentity_t *self ) {
	gitem_t     *item;
	vec3_t forward;
	int weapon;
	float angle;
	int i;
	gentity_t   *drop = 0;

	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

	switch ( self->aiCharacter ) {
	case AICHAR_ZOMBIE:
	case AICHAR_WARZOMBIE:
	case AICHAR_LOPER:
		return;         //----(SA)	removed DK's special case
	default:
		break;
	}

	AngleVectors( self->r.currentAngles, forward, NULL, NULL );

	G_ThrowChair( self, forward, qtrue ); // drop chair if you're holding one  //----(SA)	added

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a client
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.

// (SA) always drop what you were switching to
	if ( 1 ) {
//	if ( weapon == WP_MACHINEGUN || weapon == WP_GRAPPLING_HOOK ) {
		if ( self->client->ps.weaponstate == WEAPON_DROPPING || self->client->ps.weaponstate == WEAPON_DROPPING_TORELOAD ) {
			weapon = self->client->pers.cmd.weapon;
		}
		if ( !( COM_BitCheck( self->client->ps.weapons, weapon ) ) ) {
			weapon = WP_NONE;
		}
	}

//----(SA)	added
	if ( weapon == WP_SNOOPERSCOPE ) {
		weapon = WP_GARAND;
	}
	if ( weapon == WP_FG42SCOPE ) {
		weapon = WP_FG42;
	}
	if ( weapon == WP_AKIMBO ) { //----(SA)	added
		weapon = WP_COLT;
	}
//----(SA)	end


	if ( weapon > WP_NONE && weapon < WP_MONSTER_ATTACK1 && self->client->ps.ammo[ BG_FindAmmoForWeapon( weapon )] ) {
		// find the item type for this weapon
		item = BG_FindItemForWeapon( weapon );
		// spawn the item

		// Rafael
		if ( !( self->client->ps.persistant[PERS_HWEAPON_USE] ) ) {
			drop = Drop_Item( self, item, 0, qfalse );
		}
	}

	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {  // dropped items stay forever in SP
		if ( drop ) {
			drop->nextthink = 0;
		}
	}

	if ( g_gametype.integer != GT_TEAM ) {  // drop all the powerups if not in teamplay
		angle = 45;
		for ( i = 1 ; i < PW_NUM_POWERUPS ; i++ ) {
			if ( self->client->ps.powerups[ i ] > level.time ) {
				item = BG_FindItemForPowerup( i );
				if ( !item ) {
					continue;
				}
				drop = Drop_Item( self, item, angle, qfalse );
				// decide how many seconds it has left
				drop->count = ( self->client->ps.powerups[ i ] - level.time ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}
				drop->nextthink = 0;    // stay forever
				angle += 45;
			}
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
	if ( !g_blood.integer ) {
		self->health = GIB_HEALTH + 1;
		return;
	}
	if ( self->aiCharacter == AICHAR_HEINRICH || self->aiCharacter == AICHAR_HELGA || self->aiCharacter == AICHAR_SUPERSOLDIER || self->aiCharacter == AICHAR_PROTOSOLDIER ) {
		if ( self->health <= GIB_HEALTH ) {
			self->health = -1;
			return;
		}
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
	"MOD_DYNAMITE",
	"MOD_DYNAMITE_SPLASH",
	"MOD_AIRSTRIKE", // JPW NERVE
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
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t   *ent;
	int anim;
	int contents = 0;
	int killer;
	int i;
	char        *killerName, *obit;
	qboolean nogib = qtrue;
	gitem_t     *item = NULL; // JPW NERVE for flag drop
	vec3_t launchvel;      // JPW NERVE
	gentity_t   *flag; // JPW NERVE

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}

//----(SA) commented out as we have no hook
//	if (self->client && self->client->hook)
//		Weapon_HookFree(self->client->hook);

	self->client->ps.pm_type = PM_DEAD;

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

	if ( attacker && attacker->client ) {
		if ( attacker == self || OnSameTeam( self, attacker ) ) {
			AddScore( attacker, -1 );
		} else {
			AddScore( attacker, 1 );

			// Ridah, not in single player
			if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
				// done.
				if ( meansOfDeath == MOD_GAUNTLET ) {
					attacker->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;
					attacker->client->ps.persistant[PERS_REWARD] = REWARD_GAUNTLET;
					attacker->client->ps.persistant[PERS_REWARD_COUNT]++;

					// add the sprite over the player's head
//					attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT /*| EF_AWARD_GAUNTLET*/ );
					//attacker->client->ps.eFlags |= EF_AWARD_GAUNTLET;
					attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

					// also play humiliation on target
					self->client->ps.persistant[PERS_REWARD] = REWARD_GAUNTLET;
					self->client->ps.persistant[PERS_REWARD_COUNT]++;
				}

				// check for two kills in a short amount of time
				// if this is close enough to the last kill, give a reward sound
				if ( level.time - attacker->client->lastKillTime < CARNAGE_REWARD_TIME ) {
					attacker->client->ps.persistant[PERS_REWARD_COUNT]++;
					attacker->client->ps.persistant[PERS_REWARD] = REWARD_EXCELLENT;
					attacker->client->ps.persistant[PERS_EXCELLENT_COUNT]++;

					// add the sprite over the player's head
//					attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT /*| EF_AWARD_GAUNTLET*/ );
//					attacker->client->ps.eFlags |= EF_AWARD_EXCELLENT;
					attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
				}
				// Ridah
			}
			// done.
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
		}
		if ( self->client->ps.powerups[PW_BLUEFLAG] ) {
			item = BG_FindItem( "Blue Flag" );
		}
		launchvel[0] = crandom() * 20;
		launchvel[1] = crandom() * 20;
		launchvel[2] = 10 + random() * 10;
		if ( item ) {
			flag = LaunchItem( item,self->r.currentOrigin,launchvel );
			flag->s.modelindex2 = self->s.otherEntityNum2; // JPW NERVE FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
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

	self->s.powerups = 0;
// JPW NERVE -- only corpse in SP; in MP, need CONTENTS_BODY so medic can operate
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		self->r.contents = CONTENTS_CORPSE;
		self->s.weapon = WP_NONE;
	} else {
		self->client->limboDropWeapon = self->s.weapon; // store this so it can be dropped in limbo
	}
// jpw
	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller( self, inflictor, attacker );

	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->s.loopSound = 0;

	self->r.maxs[2] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + 1700;

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof( self->client->ps.powerups ) );

	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		trap_SendServerCommand( -1, "mu_play sound/music/l_failed_1.wav 0\n" );
		trap_SetConfigstring( CS_MUSIC_QUEUE, "" );  // clear queue so it'll be quiet after hit
		trap_SendServerCommand( -1, "cp missionfail0" );
	}


	// never gib in a nodrop
	if ( self->health <= GIB_HEALTH && !( contents & CONTENTS_NODROP ) && g_blood.integer ) {
//		if(self->client->ps.eFlags & EF_HEADSHOT)
//		{
//			GibHead(self, killer);
//		}
//		else	// gib death
//		{
		GibEntity( self, killer );
		nogib = qfalse;
//		}
	}

	if ( nogib ) {
		// normal death
		static int i;

		switch ( i ) {
		case 0:
			anim = BOTH_DEATH1;
			break;
		case 1:
			anim = BOTH_DEATH2;
			break;
		case 2:
		default:
			anim = BOTH_DEATH3;
			break;
		}

		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if ( self->health <= GIB_HEALTH ) {
			self->health = GIB_HEALTH + 1;
		}

// JPW NERVE for medic
		self->client->medicHealAmt = 0;
// jpw

		self->client->ps.legsAnim =
			( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		self->client->ps.torsoAnim =
			( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		G_AddEvent( self, EV_DEATH1 + 1, killer );

		// the body can still be gibbed
		self->die = body_die;

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;
	}

	trap_LinkEntity( self );

	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		AICast_ScriptEvent( AICast_GetCastState( self->s.number ), "death", "" );
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


/*
==============
IsHeadShotWeapon
==============
*/
qboolean IsHeadShotWeapon( int mod, gentity_t *targ, gentity_t *attacker ) {
	// distance rejection
	if ( DistanceSquared( targ->r.currentOrigin, attacker->r.currentOrigin )  >  ( g_headshotMaxDist.integer * g_headshotMaxDist.integer ) ) {
		return qfalse;
	}

	if ( attacker->aiCharacter ) {
		// ai's are always allowed headshots from these weapons
		if ( mod == MOD_SNIPERRIFLE ||
			 mod == MOD_SNOOPERSCOPE ) {
			return qtrue;
		}

		if ( g_gameskill.integer != GSKILL_MAX ) {
			// ai's allowed headshots in skill==GSKILL_MAX
			return qfalse;
		}
	}

	switch ( targ->aiCharacter ) {
		// get out quick for ai's that don't take headshots
	case AICHAR_ZOMBIE:
	case AICHAR_WARZOMBIE:
	case AICHAR_HELGA:      // boss1 (beast)
	case AICHAR_LOPER:
	case AICHAR_VENOM:      //----(SA)	added
		return qfalse;
	default:
		break;
	}

	switch ( mod ) {
		// players are allowed headshots from these weapons
	case MOD_LUGER:
	case MOD_COLT:
	case MOD_AKIMBO:
	case MOD_MP40:
	case MOD_THOMPSON:
	case MOD_STEN:
	case MOD_BAR:
	case MOD_FG42:
	case MOD_MAUSER:
	case MOD_GARAND:
	case MOD_SILENCER:
	case MOD_FG42SCOPE:
	case MOD_SNOOPERSCOPE:
	case MOD_SNIPERRIFLE:
		return qtrue;
	}

	return qfalse;
}


/*
==============
IsHeadShot
==============
*/
qboolean IsHeadShot( gentity_t *targ, gentity_t *attacker, vec3_t dir, vec3_t point, int mod ) {
	gentity_t   *head;
	trace_t tr;
	vec3_t start, end;
	gentity_t   *traceEnt;
	orientation_t or;

	qboolean head_shot_weapon = qfalse;

	// not a player or critter so bail
	if ( !( targ->client ) ) {
		return qfalse;
	}

	if ( targ->health <= 0 ) {
		return qfalse;
	}

	head_shot_weapon = IsHeadShotWeapon( mod, targ, attacker );

	if ( head_shot_weapon ) {
		head = G_Spawn();

		G_SetOrigin( head, targ->r.currentOrigin );

		// RF, if there is a valid tag_head for this entity, then use that
		if ( ( targ->r.svFlags & SVF_CASTAI ) && trap_GetTag( targ->s.number, "tag_head", &or ) ) {
			VectorCopy( or.origin, head->r.currentOrigin );
			VectorMA( head->r.currentOrigin, 6, or.axis[2], head->r.currentOrigin );    // tag is at base of neck
		} else if ( targ->client->ps.pm_flags & PMF_DUCKED ) { // closer fake offset for 'head' box when crouching
			head->r.currentOrigin[2] += targ->client->ps.crouchViewHeight + 8; // JPW NERVE 16 is kludge to get head height to match up
		}
		//else if(targ->client->ps.legsAnim == LEGS_IDLE && targ->aiCharacter == AICHAR_SOLDIER)	// standing with legs bent (about a head shorter than other leg poses)
		//	head->r.currentOrigin[2] += targ->client->ps.viewheight;
		else {
			head->r.currentOrigin[2] += targ->client->ps.viewheight; // JPW NERVE pulled this	// 6 is fudged "head height" value

		}
		VectorCopy( head->r.currentOrigin, head->s.origin );
		VectorCopy( targ->r.currentAngles, head->s.angles );
		VectorCopy( head->s.angles, head->s.apos.trBase );
		VectorCopy( head->s.angles, head->s.apos.trDelta );
		VectorSet( head->r.mins, -6, -6, -6 ); // JPW NERVE changed this z from -12 to -6 for crouching, also removed standing offset
		VectorSet( head->r.maxs, 6, 6, 6 ); // changed this z from 0 to 6
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
			return qtrue;
		}
	}

	return qfalse;
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

	// RF, remove flame protection after enough parts gone
	if ( AICast_NoFlameDamage( targ->s.number ) && ( (float)brokeparts / (float)numParts >= 5.0 / 6.0 ) ) { // figure from DM
		AICast_SetFlameDamage( targ->s.number, qfalse );
	}

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

//----(SA)	added
	if ( g_gametype.integer == GT_SINGLE_PLAYER && !targ->aiCharacter && targ->client && targ->client->cameraPortal ) {
		// get out of damage in sp if in cutscene.
		return;
	}
//----(SA)	end

//	if (reloading || saveGamePending) {	// map transition is happening, don't do anything
	if ( g_reloading.integer || saveGamePending ) {
		return;
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued ) {
		return;
	}

	// RF, track pain for player
	// This is used by AI to determine how long it has been since their enemy was injured

	if ( attacker ) { // (SA) whoops, for falling damage there's no attacker
		if ( targ->client && attacker->client && !( targ->r.svFlags & SVF_CASTAI ) && ( attacker->r.svFlags & SVF_CASTAI ) ) {
			AICast_RegisterPain( targ->s.number );
		}
	}

	if ( ( g_gametype.integer == GT_SINGLE_PLAYER ) && !( targ->r.svFlags & SVF_CASTAI ) ) { // the player
		switch ( mod )
		{
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			// Rafael - had to change this since the
			// we added a new lvl of diff
			if ( g_gameskill.integer == GSKILL_EASY ) {
				damage *= 0.25;
			} else if ( g_gameskill.integer == GSKILL_MEDIUM ) {
				damage *= 0.75;
			} else if ( g_gameskill.integer == GSKILL_HARD ) {
				damage *= 0.9;
			} else {
				damage *= 0.9;
			}
		default:
			break;
		}
	}

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

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

	// reduce damage by the attacker's handicap value
	// unless they are rocket jumping

	// Ridah, not in single player (skill levels?)
// JPW NERVE pulled this from multiplayer too
/*
	if (g_gametype.integer != GT_SINGLE_PLAYER)
	// done.
	if ( attacker->client && attacker != targ ) {
		damage = damage * attacker->client->ps.stats[STAT_MAX_HEALTH] / 100;
	}
*/
// jpw

	// Ridah, Cast AI's don't hurt other Cast AI's as much
	if ( ( attacker->r.svFlags & SVF_CASTAI ) && ( targ->r.svFlags & SVF_CASTAI ) ) {
		if ( !AICast_AIDamageOK( AICast_GetCastState( targ->s.number ), AICast_GetCastState( attacker->s.number ) ) ) {
			return;
		}
		damage = (int)( ceil( (float)damage * 0.5 ) );
	}
	// done.

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

//	if ( knockback > 200 )
//		knockback = 200;
	if ( knockback > 60 ) { // /way/ lessened for SP.  keeps dynamite-jumping potential down
		knockback = 60;
	}

	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client ) {
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

	// Ridah, don't play these in single player
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		// done.
		// add to the attacker's hit counter
		if ( attacker->client && targ != attacker && targ->health > 0 ) {
			if ( OnSameTeam( targ, attacker ) ) {
				attacker->client->ps.persistant[PERS_HITS] -= damage;
			} else {
				attacker->client->ps.persistant[PERS_HITS] += damage;
			}
		}
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {     // JPW NERVE -- removed from multiplayer -- plays havoc with pfaust & demolition balancing

		qboolean dynamite = (qboolean)( mod == MOD_DYNAMITE || mod == MOD_DYNAMITE_SPLASH );

		if ( targ == attacker ) {
			if ( !dynamite ) {
				damage *= 0.5;
			}
		}

		if ( dynamite && targ->aiCharacter == AICHAR_HELGA ) {
			//helga gets special dynamite damage
			damage *= 0.5;
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


	if ( IsHeadShot( targ, attacker, dir, point, mod ) ) {
		// JPW NERVE -- different headshot behavior in multiplayer
		if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
			if ( take * 2 < 50 ) { // head shots, all weapons, do minimum 50 points damage
				take = 50;
			} else {
				take *= 2; // sniper rifles can do full-kill (and knock into limbo)
			}
			if ( !( targ->client->ps.eFlags & EF_HEADSHOT ) ) {  // only toss hat on first headshot
				G_AddEvent( targ, EV_LOSE_HAT, DirToByte( dir ) );
			}
		} // jpw
		else {
			// by default, a headshot means damage x2
			take *= 2;

			// RF, allow headshot damage multiplier (helmets, etc)
			// yes, headshotDamageScale of 0 gives no damage, thats because
			// the bullet hit the head which is fully protected.
			take *= targ->headshotDamageScale;

			// player only code
			if ( !attacker->aiCharacter ) {
				// (SA) id reqests one-shot kills for head shots on common humanoids

				// (SA) except pistols.
				// first pistol head shot does normal 2x damage and flings hat, second gets kill
				//			if((mod != MOD_LUGER && mod != MOD_COLT ) || (targ->client->ps.eFlags & EF_HEADSHOT))	{	// (SA) DM requests removing double shot pistol head shots (3/19)

				// (SA) removed BG for DM.

				if ( !( dflags & DAMAGE_PASSTHRU ) ) {     // ignore headshot 2x damage and snooper-instant-death if the bullet passed through something.  just do reg damage.
					switch ( targ->aiCharacter ) {
					case AICHAR_BLACKGUARD:
						if ( !( targ->client->ps.eFlags & EF_HEADSHOT ) ) { // only obliterate him after he's lost his helmet
							break;
						}
					case AICHAR_SOLDIER:
					case AICHAR_AMERICAN:
					case AICHAR_ELITEGUARD:
					case AICHAR_PARTISAN:
					case AICHAR_CIVILIAN:
						take = 200;
						break;
					default:
						break;
					}
				}

				if ( !( targ->client->ps.eFlags & EF_HEADSHOT ) ) {  // only toss hat on first headshot
					G_AddEvent( targ, EV_LOSE_HAT, DirToByte( dir ) );
				}
			}
		} // JPW

		// shared by both player and ai
		targ->client->ps.eFlags |= EF_HEADSHOT;

	} else {    // non headshot

		if ( !( dflags & DAMAGE_PASSTHRU ) ) {     // ignore headshot 2x damage and snooper-instant-death if the bullet passed through something.  just do reg damage.
			// snooper kills these types in one shot with any contact
			if ( ( mod == MOD_SNOOPERSCOPE || mod == MOD_GARAND ) && !( attacker->aiCharacter ) ) {
				switch ( targ->aiCharacter ) {
				case AICHAR_SOLDIER:
				case AICHAR_AMERICAN:
				case AICHAR_ELITEGUARD:
				case AICHAR_BLACKGUARD:
				case AICHAR_PARTISAN:
				case AICHAR_CIVILIAN:
					take = 200;
					break;
				default:
					break;
				}
			}
		}
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
		if ( targ->client ) {
			if ( mod != MOD_VENOM && attacker == inflictor && targ->health <= GIB_HEALTH ) {
				if ( targ->aiCharacter != AICHAR_ZOMBIE ) { // zombie needs to be able to gib so we can kill him (although he doesn't actually GIB, he just dies)
					targ->health = GIB_HEALTH + 1;
				}
			}
		}

		//G_Printf("health at: %d\n", targ->health);
		if ( targ->health <= 0 ) {
			if ( client ) {
				targ->flags |= FL_NO_KNOCKBACK;
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
		}

		G_ArmorDamage( targ );    //----(SA)	moved out to separate routine

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
