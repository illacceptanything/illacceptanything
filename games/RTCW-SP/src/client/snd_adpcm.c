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

#include "snd_local.h"


void S_AdpcmEncode( short indata[], char outdata[], int len, struct adpcm_state *state ) {
	// LordHavoc: removed 4-clause BSD code for Intel ADPCM codec
}


void S_AdpcmDecode( const char indata[], short *outdata, int len, struct adpcm_state *state ) {
	// LordHavoc: removed 4-clause BSD code for Intel ADPCM codec
}


/*
====================
S_AdpcmMemoryNeeded

Returns the amount of memory (in bytes) needed to store the samples in out internal adpcm format
====================
*/
int S_AdpcmMemoryNeeded( const wavinfo_t *info ) {
	float scale;
	int scaledSampleCount;
	int sampleMemory;
	int blockCount;
	int headerMemory;

	// determine scale to convert from input sampling rate to desired sampling rate
	scale = (float)info->rate / dma.speed;

	// calc number of samples at playback sampling rate
	scaledSampleCount = info->samples / scale;

	// calc memory need to store those samples using ADPCM at 4 bits per sample
	sampleMemory = scaledSampleCount / 2;

	// calc number of sample blocks needed of PAINTBUFFER_SIZE
	blockCount = scaledSampleCount / PAINTBUFFER_SIZE;
	if ( scaledSampleCount % PAINTBUFFER_SIZE ) {
		blockCount++;
	}

	// calc memory needed to store the block headers
	headerMemory = blockCount * sizeof( adpcm_state_t );

	return sampleMemory + headerMemory;
}


/*
====================
S_AdpcmGetSamples
====================
*/
void S_AdpcmGetSamples( sndBuffer *chunk, short *to ) {
	adpcm_state_t state;
	byte            *out;

	// get the starting state from the block header
	state.index = chunk->adpcm.index;
	state.sample = chunk->adpcm.sample;

	out = (byte *)chunk->sndChunk;
	// get samples
	S_AdpcmDecode( (const char*)out, to, SND_CHUNK_SIZE_BYTE * 2, &state );       //DAJ added (const char*)
}


/*
====================
S_AdpcmEncodeSound
====================
*/
void S_AdpcmEncodeSound( sfx_t *sfx, short *samples ) {
	adpcm_state_t state;
	int inOffset;
	int count;
	int n;
	sndBuffer       *newchunk, *chunk;
	byte            *out;

	inOffset = 0;
	count = sfx->soundLength;
	state.index = 0;
	state.sample = samples[0];

	chunk = NULL;
	while ( count ) {
		n = count;
		if ( n > SND_CHUNK_SIZE_BYTE * 2 ) {
			n = SND_CHUNK_SIZE_BYTE * 2;
		}

		newchunk = SND_malloc();
		if ( sfx->soundData == NULL ) {
			sfx->soundData = newchunk;
		} else {
			chunk->next = newchunk;
		}
		chunk = newchunk;

		// output the header
		chunk->adpcm.index  = state.index;
		chunk->adpcm.sample = state.sample;

		out = (byte *)chunk->sndChunk;

		// encode the samples
		S_AdpcmEncode( samples + inOffset, (char*)out, n, &state );     //DAJ added (char*)

		inOffset += n;
		count -= n;
	}
}