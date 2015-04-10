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
 *  idSpeaker.h
 *  osxDOOM
 *
 *  Created by zaphod on Thu Mar 08 2001.
 *
 */

/*
 * idSpeaker is the container class for all sound stuff.  It has static members that allow you to start, stop, and
 * so forth, and private members that will not be exposed to the public API.  This class will start a thread that
 * plays independently of the game API.
 *
 * idSpeaker can handle various output devices, ranging from headphones, to speakers, to 5.1.  The default setup
 * is Dolby 5.1 through at least six speakers (four surround, one front, and sub).
 *
 */

#define MAX_STREAMING_SOUNDS    24

class idSpeaker {

public:
void        process();

idSpeaker();
~idSpeaker();

bool        activate();
bool        loop( const char *intro, const char *filename, int channel, bool looping, bool in3D, bool useNotification );
void        stopLoop( int channel );
void        updateLoop();

idAudioHardware     *hw;

idAudioBuffer*  find( const char* fname ) {
	idAudioBuffer **mat = NULL;

	// see if it has been asked for before
	if ( hashTable.Get( fname, &mat ) ) {
		return ( *mat );
	}
	return NULL;
}

void        setEntityVolume( const int entityNum, const float v ) {
	if ( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
		entVol[entityNum] = v;
	}
}

void        setEntityPosition( const int entityNum, const float x, const float y, const float z ) {
	if ( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
		entPos[entityNum].x = x;
		entPos[entityNum].y = y;
		entPos[entityNum].z = z;
	}
}

void        setEntityVelocity( const int entityNum, const float x, const float y, const float z ) {
	if ( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
		entVel[entityNum].x = x;
		entVel[entityNum].y = y;
		entVel[entityNum].z = z;
	}
}

float       getEntityVolume( const int entityNum ) {
	if ( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
		return entVol[entityNum];
	} else {
		return getMusicVolume();
	}
}

idVec3&     getEntityPosition( const int entityNum ) {
	if ( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
		return entPos[entityNum];
	} else {
		return listener;
	}
}

idVec3&     getEntityVelocity( const int entityNum ) {
	if ( entityNum >= 0 && entityNum < MAX_GENTITIES ) {
		return entVel[entityNum];
	} else {
		return listener;
	}
}

void        setListenerPosition( const float x, const float y, const float z ) {
	listener.x = x;
	listener.y = y;
	listener.z = z;
}

void        setListenerAxis( const vec3_t axis0, const vec3_t axis1 ) {
	listenerFront.x = axis0[0]; listenerFront.y = axis0[1]; listenerFront.z = axis0[2];
	listenerTop.x = axis1[0]; listenerTop.y = axis1[1]; listenerTop.z = axis1[2];
}

idVec3&     getListenerPosition() {
	return listener;
}

float       getMusicVolume() {  return s_musicVolume->value; }
float       getMinDistance() {  return s_minDistance->value; }
float       getMaxDistance() {  return s_maxDistance->value; }

void        bind( const char *name, idAudioBuffer *nis ) {
	hashTable.Set( name, nis );
}
void        unbind( const char *name ) {
	hashTable.Remove( name );
}

private:

void        LOCK();
void        UNLOCK();
bool        initAudioHardware();
bool        releaseAudioHardware();
int         getAudioHardwareChannel();
int         getDMAPos( int channel );
bool        beginPainting( int channel );
bool        endPainting( int channel );
idVec3 entPos[MAX_GENTITIES];
idVec3 entVel[MAX_GENTITIES];
float entVol[MAX_GENTITIES];
idVec3 listener;
idVec3 listenerFront;
idVec3 listenerTop;

cvar_t      *s_dopplerFactor;
cvar_t      *s_distanceFactor;
cvar_t      *s_rolloffFactor;
cvar_t      *s_minDistance;
cvar_t      *s_maxDistance;
cvar_t      *s_musicVolume;

idHashTable<idAudioBuffer*> hashTable;                          // hashed version for loaded sounds

friend class idSound;
friend class idAudioChannel;

LPDIRECTSOUND3DLISTENER pDSListener;
DS3DLISTENER dsListenerParams;                            // Listener properties

};

extern idSpeaker* idSpeak;

