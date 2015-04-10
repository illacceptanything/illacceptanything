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
 *  idSound.h
 *  osxDOOM
 *
 *  Created by zaphod on Thu Mar 08 2001.
 *
 */


/*
 * idSound is the class that actually stores a sound.  It also has a limited public API that allows you to signal
 * a sound from the game code or an entity.  We need to use signals since the sound code runs in it's own thread
 * and the entity class doesn't need to become thread safe just for the sound.
 *
 * to register a sound use from any thread, you can specify as many flags as you like:
 *
 * handle = idSound::register( "coolfile.wav", idSound::LOOPING, idSound::DOPPLER );
 *
 * to play a sound from anywhere, and from any thread.
 *
 * idsound::signal( handle, idSound::PLAY );
 *
 * from an entity, you'll need to signal the sound from the entity startup with:
 *
 * idsound::signal( handle, idSound::PLAY, this );
 *
 * and from the main loop as:
 *
 * idsound::signal( handle, idSound::UPDATE, this);
 *
 * and when the entity dies, or stops:
 *
 * idsound::signal( handle, idSound::STOP );
 *
 * Sounds started here will store their own state in a similar fashion to the
 * renderer so they can be, for the most part, updated without bothering the game thread.
 *
 */


typedef int idSoundHandle;

class idSound {

public:
static void        start();
static void        finish();

idSound( const char *name = NULL );                 // register me!
~idSound();
void        play( const int dwFlags = 0 );
void        play3D( const int dwFlags, const idVec3& pos, const idVec3& vel );
void        stop();
void        update3D( const idVec3& pos, const idVec3& vel );

protected:
private:
friend class idSpeaker;
friend class idWaveform;

int lastTimeUsed;

idAudioBuffer   *pSound;
int bIndex;
};


