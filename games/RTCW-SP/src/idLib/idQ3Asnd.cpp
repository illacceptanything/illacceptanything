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
 *  idSpeaker.cpp
 *  osxDOOM
 *
 *  Created by zaphod on Thu Mar 08 2001.
 *
 */


#include "idAudio.h"
#include "idAudioHardware.h"

static int numLoopSounds;
static int oldNumLoopSounds;

#define MAX_LOOPSOUNDS 1024
#define UNDERWATER_BIT  8

typedef struct {
	idAudioBuffer*  sfx;
	float volume;
	idVec3 origin;
	idVec3 velocity;
	int range;
	bool loudUnderWater;
} loopSound_t;

typedef struct loopSound_s {
	idAudioBuffer*  sfx;
	int index;
} loopIndex_t;

static loopSound_t loopSounds[MAX_LOOPSOUNDS];
static loopIndex_t indexList[MAX_LOOPSOUNDS];
static loopIndex_t newIndexList[MAX_LOOPSOUNDS];

idSpeaker       *idSpeak = NULL;

extern idStreamingBuffer*   g_pStreamingSound[MAX_STREAMING_SOUNDS];


void S_Init( void ) {
	idSpeak = new idSpeaker();
}

void S_Shutdown( void ) {
	delete idSpeak;
}

void S_StartSoundEx( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx, int flags ) {
	idVec3 pos;
	idAudioBuffer *pSound = (idAudioBuffer*)sfx;
	int bIndex, i;

	// check for a streaming sound that this entity is playing in this channel
	// kill it if it exists
	if ( entnum >= 0 ) {
		for ( i = 1; i < MAX_STREAMING_SOUNDS; i++ ) {    // track 0 is music/cinematics
			if ( !g_pStreamingSound[i] ) {
				continue;
			}
			// check to see if this character currently has another sound streaming on the same channel
			if ( ( entchannel != CHAN_AUTO ) && ( g_pStreamingSound[i]->entnum[0] >= 0 ) && ( g_pStreamingSound[i]->entchannel[0] == entchannel ) && ( g_pStreamingSound[i]->entnum[0] == entnum ) ) {
				// found a match, override this channel
				g_pStreamingSound[i]->Stop( 0 );
				break;
			}
		}
	}

//----(SA)	modified

	if ( pSound ) {
		// shut off other sounds on this channel if necessary
		for ( i = 0 ; i < pSound->GetNumBuffers() ; i++ ) {
			if ( pSound->entnum[i] == entnum && pSound->entchannel[i] == entchannel ) {

				// cutoff all on channel
				if ( flags & SND_CUTOFF_ALL ) {
					pSound->Stop( i );
					continue;
				}

				if ( pSound->flags[i] & SND_NOCUT ) {
					continue;
				}

				// cutoff sounds that expect to be overwritten
				if ( pSound->flags[i] & SND_OKTOCUT ) {
					pSound->Stop( i );
					continue;
				}

				// cutoff 'weak' sounds on channel
				if ( flags & SND_CUTOFF ) {
					if ( pSound->flags[i] & SND_REQUESTCUT ) {
						pSound->Stop( i );
						continue;
					}
				}

			}
		}

//----(SA)	end

		if ( origin ) {
			pos.x = origin[0] * QUAKE_TO_DS3D;
			pos.y = origin[1] * QUAKE_TO_DS3D;
			pos.z = origin[2] * QUAKE_TO_DS3D;

			bIndex = pSound->GetFreeIndex();

			if ( bIndex != -1 ) {
				pSound->entnum[bIndex] = entnum;
				pSound->entchannel[bIndex] = entchannel;
				pSound->flags[bIndex] = flags;

				idSpeak->setEntityPosition( entnum, pos.x, pos.y, pos.z );
				pSound->SetPosition( bIndex, pos.x, pos.y, pos.z );
				pSound->Play( bIndex, 0, 0 );
			}
		} else {
			bIndex = pSound->GetFreeIndex();

			if ( bIndex != -1 ) {
				pSound->entnum[bIndex] = entnum;
				pSound->entchannel[bIndex] = entchannel;
				pSound->flags[bIndex] = flags;

				pSound->Make2D( bIndex );
				pSound->Play( bIndex, 0, 0 );            // index, priority, flags
			}
		}
	}
}

void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx ) {
	S_StartSoundEx( origin, entnum, entchannel, sfx, 0 );
}

void S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	if ( sfx ) {
		idAudioBuffer *pSound = (idAudioBuffer*)sfx;
		int bIndex = pSound->GetFreeIndex();
		if ( bIndex != -1 ) {
			pSound->Make2D( bIndex );
			pSound->Play( bIndex, 0, 0 );            // index, priority, flags
		}
	}
}

// "extension" should include the dot: ".map"
void defaultExtension( char *path, int maxSize, const char *extension ) {
	char oldPath[MAX_QPATH];
	char    *src;

//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen( path ) - 1;

	while ( *src != '/' && src != path ) {
		if ( *src == '.' ) {
			return;                 // it has an extension
		}
		src--;
	}

	Q_strncpyz( oldPath, path, sizeof( oldPath ) );
	Com_sprintf( path, maxSize, "%s%s", oldPath, extension );
}

void S_StartBackgroundTrack( const char *intro, const char *loop ) {
	char name[MAX_QPATH];
	char band[MAX_QPATH];

	if ( !intro ) {
		intro = "";
	}
	if ( !loop || !loop[0] ) {
		loop = intro;
	}
	Com_DPrintf( "S_StartBackgroundTrack( %s, %s )...\n", intro, loop );

	Q_strncpyz( name, intro, sizeof( name ) - 4 );
	defaultExtension( name, sizeof( name ), ".wav" );
	if ( loop && loop[0] ) {
		Q_strncpyz( band, loop, sizeof( band ) - 4 );
		defaultExtension( band, sizeof( band ), ".wav" );
	} else {
		band[0] = 0;
	}

	bool looping = false;
	if ( loop && loop[0] ) {
		looping = true;
	}

	if ( idSpeak->loop( name, band, 0, looping, false, true ) ) {
		g_pStreamingSound[0]->Play( 0, 0, looping ? DSBPLAY_LOOPING : 0 );
	}
}

void S_StopBackgroundTrack( void ) {
	g_pStreamingSound[0]->Stop( 0 );
}

void S_UpdateBackgroundTrack( void ) {
}

/*
======================
S_StartStreamingSound

  FIXME: record the starting cg.time of the sound, so we can determine the
  position by looking at the current cg.time, this way pausing or loading a
  savegame won't screw up the timing of important sounds
======================
*/

void S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation ) {
	int i;
	int ss;

	if ( !intro || !intro[0] ) {
		if ( loop && loop[0] ) {
			intro = loop;
		} else {
			intro = "";
		}
	}
	// look for a free track, but first check for overriding a currently playing sound for this entity
	ss = 0;
	if ( entnum >= 0 ) {
		for ( i = 1; i < MAX_STREAMING_SOUNDS; i++ ) {    // track 0 is music/cinematics
			if ( g_pStreamingSound[i] == NULL ) {
				continue;
			}
			// check to see if this character currently has another sound streaming on the same channel
			if ( ( channel != CHAN_AUTO ) && ( g_pStreamingSound[i]->entnum [0] >= 0 ) && ( g_pStreamingSound[i]->entchannel[0] == channel ) && ( g_pStreamingSound[i]->entnum[0] == entnum ) ) {
				// found a match, override this channel
				idSpeak->stopLoop( i );
				ss = i; // use this track to start the new stream
				break;
			}
		}
	}
	if ( !ss ) {
		// no need to override a current stream, so look for a free track
		for ( i = 1; i < MAX_STREAMING_SOUNDS; i++ ) {    // track 0 is music/cinematics
			if ( !g_pStreamingSound[i] || !g_pStreamingSound[i]->IsSoundPlaying() ) {
				ss = i;
				break;
			}
		}
	}

	if ( !ss ) {
		return;
	}

	bool looping = false;
	bool in3D = false;

	if ( attenuation ) {
		in3D = true;
	}
	if ( loop && loop[0] ) {
		looping = true;
	}

	if ( idSpeak->loop( intro, loop, ss, looping, in3D, false ) ) {
		if ( g_pStreamingSound[ss] != NULL ) {
			idVec3 pos = idSpeak->getEntityPosition( entnum );

			g_pStreamingSound[ss]->entnum[0] = entnum;
			g_pStreamingSound[ss]->entchannel[0] = channel;
			g_pStreamingSound[ss]->attenuation = attenuation;
			g_pStreamingSound[ss]->SetPosition( 0, pos.x, pos.y, pos.z );           // streaming sound is always channel 0
			g_pStreamingSound[ss]->Play( 0, 0, looping ? DSBPLAY_LOOPING : 0 );
			g_pStreamingSound[ss]->SetRange( 0, idSpeak->getMinDistance(), idSpeak->getMaxDistance() * 4 );
		}
	}
}

void S_RawSamples( int samples, int rate, int width, int channels, const byte *data, float volume ) {
}

void S_StopAllSounds( void ) {
	int i;
	for ( i = 0; i < MAX_STREAMING_SOUNDS; i++ ) {
		idSpeak->stopLoop( i );
	}
	S_ClearLoopingSounds();
}

void S_ClearLoopingSounds( void ) {
	int i;
	for ( i = 0 ; i < numLoopSounds ; i++ ) {
		loopSounds[i].sfx = NULL;
	}
	oldNumLoopSounds = numLoopSounds;
	numLoopSounds = 0;
}

void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, const int range, sfxHandle_t sfx, int volume ) {
	idAudioBuffer *pSound = (idAudioBuffer*)sfx;
	float fvolume;

	idSpeak->setEntityPosition( entityNum, origin[0] * QUAKE_TO_DS3D, origin[1] * QUAKE_TO_DS3D, origin[2] * QUAKE_TO_DS3D );
	idSpeak->setEntityVelocity( entityNum, velocity[0] * QUAKE_TO_DS3D * ( 1.0f / 20.0f ), velocity[1] * QUAKE_TO_DS3D * ( 1.0f / 20.0f ), velocity[2] * QUAKE_TO_DS3D  * ( 1.0f / 20.0f ) );

	fvolume = volume;
	fvolume *= ( 1.0f / 255.0f );

	idSpeak->setEntityVolume( entityNum, fvolume );

	loopSounds[numLoopSounds].sfx = pSound;

	loopSounds[numLoopSounds].origin.x = origin[0] * QUAKE_TO_DS3D;;
	loopSounds[numLoopSounds].origin.y = origin[1] * QUAKE_TO_DS3D;;
	loopSounds[numLoopSounds].origin.z = origin[2] * QUAKE_TO_DS3D;;

	loopSounds[numLoopSounds].velocity.x = velocity[0] * QUAKE_TO_DS3D * ( 1.0f / 20.0f );
	loopSounds[numLoopSounds].velocity.y = velocity[1] * QUAKE_TO_DS3D * ( 1.0f / 20.0f );
	loopSounds[numLoopSounds].velocity.z = velocity[2] * QUAKE_TO_DS3D * ( 1.0f / 20.0f );

	loopSounds[numLoopSounds].volume = fvolume;

	if ( range ) {
		loopSounds[numLoopSounds].range = ( (float)range ) * QUAKE_TO_DS3D;
	} else {
		loopSounds[numLoopSounds].range = idSpeak->getMaxDistance();
	}

	if ( volume & 1 << UNDERWATER_BIT ) {
		loopSounds[numLoopSounds].loudUnderWater = true;
	}

	numLoopSounds++;
}

void oldVectorRotate( vec3_t in, vec3_t matrix[3], vec3_t out ) {
	out[0] = DotProduct( in, matrix[0] );
	out[1] = DotProduct( in, matrix[1] );
	out[2] = DotProduct( in, matrix[2] );
}


void S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater ) {
	vec3_t front, newFront, top, newTop;

	front[0] = 0; front[1] = 0; front[2] = 1;
	top[0] = 0; top[1] = 1; top[2] = 0;

	oldVectorRotate( front, axis, newFront );
	oldVectorRotate( top, axis, newTop );

	idSpeak->setListenerPosition( origin[0] * QUAKE_TO_DS3D, origin[1] * QUAKE_TO_DS3D, origin[2] * QUAKE_TO_DS3D );
	idSpeak->setListenerAxis( newFront, newTop );
}

void S_UpdateEntityPosition( int entityNum, const vec3_t origin ) {
	idSpeak->setEntityPosition( entityNum, origin[0] * QUAKE_TO_DS3D, origin[1] * QUAKE_TO_DS3D, origin[2] * QUAKE_TO_DS3D );
}

void S_Update( void ) {
	int i, j;
	idAudioBuffer *iab;

	for ( i = 0 ; i < numLoopSounds ; i++ ) {
		if ( loopSounds[i].sfx ) {
			iab = loopSounds[i].sfx;
			int index = -1;
			for ( j = 0;  j < oldNumLoopSounds; j++ ) {
				if ( indexList[j].sfx == iab ) {
					indexList[j].sfx = NULL;
					index = indexList[j].index;
					iab->SetPosition( index, loopSounds[i].origin.x, loopSounds[i].origin.y, loopSounds[i].origin.z );
					iab->SetVelocity( index, loopSounds[i].velocity.x, loopSounds[i].velocity.y, loopSounds[i].velocity.z );
					iab->SetVolume( index, loopSounds[i].volume );
					iab->SetRange( index, idSpeak->getMinDistance(), loopSounds[i].range );
					newIndexList[i].index = index;
					newIndexList[i].sfx = iab;
					break;
				}
			}
			if ( index == -1 ) {
				index = iab->GetFreeIndex();
				if ( index != -1 ) {
					iab->SetPosition( index, loopSounds[i].origin.x, loopSounds[i].origin.y, loopSounds[i].origin.z );
					iab->SetVelocity( index, loopSounds[i].velocity.x, loopSounds[i].velocity.y, loopSounds[i].velocity.z );
					iab->SetVolume( index, loopSounds[i].volume );
					iab->SetRange( index, idSpeak->getMinDistance(), loopSounds[i].range );
					iab->Play( index, 0, DSBPLAY_LOOPING );
					newIndexList[i].index = index;
					newIndexList[i].sfx = iab;
				}
			}
		}
	}
	for ( i = 0;  i < oldNumLoopSounds; i++ ) {
		if ( indexList[i].sfx ) {
			indexList[i].sfx->Stop( indexList[i].index );
		}
	}

	memcpy( indexList, newIndexList, numLoopSounds * sizeof( loopIndex_t ) );

	idSpeak->updateLoop();
	idSpeak->process();
}

void S_DisableSounds( void ) {
}


void S_BeginRegistration( void ) {
}

static bool initEAX = false;

sfxHandle_t S_RegisterSound( const char *sample ) {
	idAudioBuffer *iab;
	sfxHandle_t poop;
	int hr;

	if ( ( iab = idSpeak->find( sample ) ) ) {
		poop = (int)iab;
	} else {
		iab = NULL;
		// Load the wave file into a DirectSound buffer
		hr = idSpeak->hw->Create( &iab, sample, DSBCAPS_CTRL3D | DSBCAPS_LOCHARDWARE, DS3DALG_DEFAULT  );
		if ( hr != E_FAIL ) {
			// Get the 3D buffer from the secondary buffer
			iab->Get3DBufferInterfaces( idSpeak->getMinDistance(), idSpeak->getMaxDistance() );
			if ( !initEAX ) {
				initEAX = true;
				idSpeak->hw->GetEAX( iab->Get3DBuffer( 0 ) );
			}
			poop = (int)iab;
			idSpeak->bind( sample, iab );
			iab->name = new idStr( sample );
		} else {
			SAFE_DELETE( iab );
			poop = 0;
		}
	}
	return poop;
}

void SNDDMA_Activate( void ) {
}

void S_ClearSoundBuffer( void ) {
}

/*
===================
S_GetVoiceAmplitude
===================
*/
int S_GetVoiceAmplitude( int entityNum ) {
	return 0;
}

