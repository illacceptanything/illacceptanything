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

//===========================================================================
//
// Name:			ai_cast_global.h
// Function:		Global AI Cast defines
// Programmer:		Ridah
// Tab Size:		4 (real tabs)
//===========================================================================

// TTimo no typedef, "warning: useless keyword or type name in empty declaration"
struct cast_state_s;

#define AICAST_AIM_SPREAD   2048.0  // a really bad shooter will offset a maximum of this per shot, from the end point of the 8192 trace length

#define DANGER_MISSILE      ( 1 << 0 )
#define DANGER_CLIENTAIM    ( 1 << 1 )
#define DANGER_FLAMES       ( 1 << 2 )

extern qboolean saveGamePending;

qboolean AICast_SameTeam( struct cast_state_s *cs, int enemynum );
struct cast_state_s *AICast_GetCastState( int entitynum );
void AICast_ScriptLoad( void );
void AICast_ScriptEvent( struct cast_state_s *cs, char *eventStr, char *params );
void AICast_ForceScriptEvent( struct cast_state_s *cs, char *eventStr, char *params );
qboolean AICast_AIDamageOK( struct cast_state_s *cs, struct cast_state_s *ocs );
gentity_t *AICast_FindEntityForName( char *name );
gentity_t *AICast_TravEntityForName( gentity_t *startent, char *name );
void AICast_ScriptParse( struct cast_state_s *cs );
void AICast_StartFrame( int time );
void AICast_StartServerFrame( int time );
void AICast_RecordWeaponFire( gentity_t *ent );
void AICast_AIDoor_Touch( gentity_t *ent, gentity_t *aidoor_trigger, gentity_t *door );
float AICast_GetAccuracy( int entnum );
void AICast_Activate( int activatorNum, int entNum );
void AICast_CheckDangerousEntity( gentity_t *ent, int dangerFlags, float dangerDist, float tacticalLevel, float aggressionLevel, qboolean hurtFriendly );
qboolean AICast_NoFlameDamage( int entNum );
void AICast_SetFlameDamage( int entNum, qboolean status );
qboolean AICast_HasFiredWeapon( int entNum, int weapon );
void G_SetAASBlockingEntity( gentity_t *ent, qboolean blocking );
qboolean AICast_InFieldOfVision( vec3_t viewangles, float fov, vec3_t angles );
qboolean AICast_VisibleFromPos( vec3_t srcpos, int srcnum,
								vec3_t destpos, int destnum, qboolean updateVisPos );
qboolean AICast_AllowFlameDamage( int entNum );
void AICast_AdjustIdealYawForMover( int entnum, float yaw );
void AICast_AgePlayTime( int entnum );
int AICast_NoReload( int entnum );
void AICast_RecordScriptSound( int client );
void AICast_UpdateVisibility( gentity_t *srcent, gentity_t *destent, qboolean shareVis, qboolean directview );
void AICast_ProcessBullet( gentity_t *attacker, vec3_t start, vec3_t end );
void AICast_AudibleEvent( int srcnum, vec3_t pos, float range );

//----(SA)	added
int AICast_PlayTime( int entnum );
int AICast_NumAttempts( int entnum );
//----(SA)	end

void AICast_RegisterPain( int entnum );
