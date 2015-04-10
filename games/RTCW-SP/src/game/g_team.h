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


#define CTF_CAPTURE_BONUS       5       // what you get for capture
#define CTF_TEAM_BONUS          0       // what your team gets for capture
#define CTF_RECOVERY_BONUS      1       // what you get for recovery
#define CTF_FLAG_BONUS          0       // what you get for picking up enemy flag
#define CTF_FRAG_CARRIER_BONUS  2       // what you get for fragging enemy flag carrier
#define CTF_FLAG_RETURN_TIME    40000   // seconds until auto return

#define CTF_CARRIER_DANGER_PROTECT_BONUS    2   // bonus for fraggin someone who has recently hurt your flag carrier
#define CTF_CARRIER_PROTECT_BONUS           1   // bonus for fraggin someone while either you or your target are near your flag carrier
#define CTF_FLAG_DEFENSE_BONUS              1   // bonus for fraggin someone while either you or your target are near your flag
#define CTF_RETURN_FLAG_ASSIST_BONUS        1   // awarded for returning a flag that causes a capture to happen almost immediately
#define CTF_FRAG_CARRIER_ASSIST_BONUS       2   // award for fragging a flag carrier if a capture happens almost immediately

#define CTF_TARGET_PROTECT_RADIUS           400 // the radius around an object being defended where a target will be worth extra frags
#define CTF_ATTACKER_PROTECT_RADIUS         400 // the radius around an object being defended where an attacker will get extra frags when making kills

#define CTF_CARRIER_DANGER_PROTECT_TIMEOUT  8
#define CTF_FRAG_CARRIER_ASSIST_TIMEOUT     10000
#define CTF_RETURN_FLAG_ASSIST_TIMEOUT      10000

#define CTF_GRAPPLE_SPEED                   750 // speed of grapple in flight
#define CTF_GRAPPLE_PULL_SPEED              750 // speed player is pulled at

// Prototypes

int OtherTeam( int team );
const char *TeamName( int team );
const char *OtherTeamName( int team );
const char *TeamColorString( int team );

void Team_DroppedFlagThink( gentity_t *ent );
void Team_FragBonuses( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker );
void Team_CheckHurtCarrier( gentity_t *targ, gentity_t *attacker );
void Team_InitGame( void );
void Team_ReturnFlag( int team );
void Team_FreeEntity( gentity_t *ent );
gentity_t *SelectCTFSpawnPoint( team_t team, int teamstate, vec3_t origin, vec3_t angles );
gentity_t *Team_GetLocation( gentity_t *ent );
qboolean Team_GetLocationMsg( gentity_t *ent, char *loc, int loclen );
void TeamplayInfoMessage( gentity_t *ent );
void CheckTeamStatus( void );

int Pickup_Team( gentity_t *ent, gentity_t *other );
