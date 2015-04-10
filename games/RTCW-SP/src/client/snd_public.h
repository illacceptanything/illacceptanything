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


#ifdef DOOMSOUND    ///// (SA) DOOMSOUND
#ifdef __cplusplus
extern "C" {
#endif
#endif  ///// (SA) DOOMSOUND

void S_Init( void );
void S_Shutdown( void );
void S_UpdateThread( void );

// if origin is NULL, the sound will be dynamically sourced from the entity
void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx );
void S_StartSoundEx( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx, int flags );
void S_StartLocalSound( sfxHandle_t sfx, int channelNum );

void S_StartBackgroundTrack( const char *intro, const char *loop, int fadeupTime );
void S_StopBackgroundTrack( void );
void S_QueueBackgroundTrack( const char *loop );            //----(SA)	added
void S_FadeStreamingSound( float targetvol, int time, int ssNum );  //----(SA)	added
void S_FadeAllSounds( float targetvol, int time );    //----(SA)	added

void S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation );
void S_StopStreamingSound( int index );
void S_StopEntStreamingSound( int entNum ); //----(SA)	added

// cinematics and voice-over-network will send raw samples
// 1.0 volume will be direct output of source samples
void S_RawSamples( int samples, int rate, int width, int s_channels,
				   const byte *data, float lvol, float rvol, int streamingIndex );

// stop all sounds and the background track
void S_StopAllSounds( void );

// all continuous looping sounds must be added before calling S_Update
void S_ClearLoopingSounds( void );
void S_ClearSounds( qboolean clearStreaming, qboolean clearMusic ); //----(SA)	modified
void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfxHandle, int volume );
void S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfx );
void S_StopLoopingSound( int entityNum );

#ifdef DOOMSOUND    ///// (SA) DOOMSOUND
void S_ClearSoundBuffer( void );
#endif ///// (SA) DOOMSOUND
// recompute the reletive volumes for all running sounds
// reletive to the given entityNum / orientation
void S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );

// let the sound system know where an entity currently is
void S_UpdateEntityPosition( int entityNum, const vec3_t origin );

void S_Update( void );

void S_DisableSounds( void );

void S_BeginRegistration( void );

// RegisterSound will allways return a valid sample, even if it
// has to create a placeholder.  This prevents continuous filesystem
// checks for missing files
#ifdef DOOMSOUND    ///// (SA) DOOMSOUND
sfxHandle_t S_RegisterSound( const char *sample );
#else
sfxHandle_t S_RegisterSound( const char *sample, qboolean compressed );
#endif ///// (SA) DOOMSOUND

void S_DisplayFreeMemory( void );

//
int S_GetVoiceAmplitude( int entityNum );


#ifdef DOOMSOUND    ///// (SA) DOOMSOUND
#ifdef __cplusplus
}
#endif
#endif  ///// (SA) DOOMSOUND
