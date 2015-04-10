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

#include <float.h>

#include "../client/snd_local.h"
#include "win_local.h"

HRESULT ( WINAPI * pDirectSoundCreate )( GUID FAR *lpGUID, LPDIRECTSOUND FAR *lplpDS, IUnknown FAR *pUnkOuter );
#define iDirectSoundCreate( a,b,c )   pDirectSoundCreate( a,b,c )

#define SECONDARY_BUFFER_SIZE   0x10000


static qboolean dsound_init;
static int sample16;
static DWORD gSndBufSize;
static DWORD locksize;
static LPDIRECTSOUND pDS;
static LPDIRECTSOUNDBUFFER pDSBuf, pDSPBuf;
static HINSTANCE hInstDS;
static qboolean snd_iswave = qfalse;

static const char *DSoundError( int error ) {
	switch ( error ) {
	case DSERR_BUFFERLOST:
		return "DSERR_BUFFERLOST";
	case DSERR_INVALIDCALL:
		return "DSERR_INVALIDCALLS";
	case DSERR_INVALIDPARAM:
		return "DSERR_INVALIDPARAM";
	case DSERR_PRIOLEVELNEEDED:
		return "DSERR_PRIOLEVELNEEDED";
	}

	return "unknown";
}

HGLOBAL hWaveHdr;
LPWAVEHDR lpWaveHdr;

HWAVEOUT hWaveOut;

WAVEOUTCAPS wavecaps;

DWORD gSndBufSize;

MMTIME mmstarttime;

HANDLE hData;
HPSTR lpData, lpData2;

static qboolean wav_init = qfalse;
// starts at 0 for disabled
static int snd_buffer_count = 0;
static int sample16;
static int snd_sent, snd_completed;


// 64K is > 1 second at 16-bit, 22050 Hz
#define WAV_BUFFERS             64
#define WAV_MASK                0x3F
#define WAV_BUFFER_SIZE         0x0400

extern cvar_t       *s_wavonly;
/*
==================
FreeSound
==================
*/
void FreeSound( void ) {
	int i;

	Com_DPrintf( "Shutting down sound system\n" );

	if ( hWaveOut ) {
		Com_DPrintf( "...resetting waveOut\n" );
		waveOutReset( hWaveOut );

		if ( lpWaveHdr ) {
			Com_DPrintf( "...unpreparing headers\n" );
			for ( i = 0 ; i < WAV_BUFFERS ; i++ )
				waveOutUnprepareHeader( hWaveOut, lpWaveHdr + i, sizeof( WAVEHDR ) );
		}

		Com_DPrintf( "...closing waveOut\n" );
		waveOutClose( hWaveOut );

		if ( hWaveHdr ) {
			Com_DPrintf( "...freeing WAV header\n" );
			GlobalUnlock( hWaveHdr );
			GlobalFree( hWaveHdr );
		}

		if ( hData ) {
			Com_DPrintf( "...freeing WAV buffer\n" );
			GlobalUnlock( hData );
			GlobalFree( hData );
		}

	}

	hWaveOut = 0;
	hData = 0;
	hWaveHdr = 0;
	lpData = NULL;
	lpWaveHdr = NULL;
	wav_init = qfalse;
}

/*
==================
SNDDM_InitWav

Crappy windows multimedia base
==================
*/
qboolean SNDDMA_InitWav( void ) {
	WAVEFORMATEX format;
	int i;
	HRESULT hr;

	Com_Printf( "Initializing wave sound\n" );

	snd_sent = 0;
	snd_completed = 0;

	dma.channels = 2;
	dma.samplebits = 16;

	if ( s_khz->value == 44 ) {
		dma.speed = 44100;
	}
	if ( s_khz->value == 22 ) {
		dma.speed = 22050;
	} else {
		dma.speed = 11025;
	}

	memset( &format, 0, sizeof( format ) );
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels
						 * format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec
							 * format.nBlockAlign;

	/* Open a waveform device for output using window callback. */
	Com_DPrintf( "...opening waveform device: " );
	while ( ( hr = waveOutOpen( (LPHWAVEOUT)&hWaveOut, WAVE_MAPPER,
								&format,
								0, 0L, CALLBACK_NULL ) ) != MMSYSERR_NOERROR )
	{
		if ( hr != MMSYSERR_ALLOCATED ) {
			Com_Printf( "failed\n" );
			return qfalse;
		}

		if ( MessageBox( NULL,
						 "The sound hardware is in use by another app.\n\n"
						 "Select Retry to try to start sound again or Cancel to run Quake 2 with no sound.",
						 "Sound not available",
						 MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION ) != IDRETRY ) {
			Com_Printf( "hw in use\n" );
			return qfalse;
		}
	}
	Com_DPrintf( "ok\n" );

	/*
	 * Allocate and lock memory for the waveform data. The memory
	 * for waveform data must be globally allocated with
	 * GMEM_MOVEABLE and GMEM_SHARE flags.

	*/
	Com_DPrintf( "...allocating waveform buffer: " );
	gSndBufSize = WAV_BUFFERS * WAV_BUFFER_SIZE;
	hData = GlobalAlloc( GMEM_MOVEABLE | GMEM_SHARE, gSndBufSize );
	if ( !hData ) {
		Com_Printf( " failed\n" );
		FreeSound();
		return qfalse;
	}
	Com_DPrintf( "ok\n" );

	Com_DPrintf( "...locking waveform buffer: " );
	lpData = GlobalLock( hData );
	if ( !lpData ) {
		Com_Printf( " failed\n" );
		FreeSound();
		return qfalse;
	}
	memset( lpData, 0, gSndBufSize );
	Com_DPrintf( "ok\n" );

	/*
	 * Allocate and lock memory for the header. This memory must
	 * also be globally allocated with GMEM_MOVEABLE and
	 * GMEM_SHARE flags.
	 */
	Com_DPrintf( "...allocating waveform header: " );
	hWaveHdr = GlobalAlloc( GMEM_MOVEABLE | GMEM_SHARE,
							(DWORD) sizeof( WAVEHDR ) * WAV_BUFFERS );

	if ( hWaveHdr == NULL ) {
		Com_Printf( "failed\n" );
		FreeSound();
		return qfalse;
	}
	Com_DPrintf( "ok\n" );

	Com_DPrintf( "...locking waveform header: " );
	lpWaveHdr = (LPWAVEHDR) GlobalLock( hWaveHdr );

	if ( lpWaveHdr == NULL ) {
		Com_Printf( "failed\n" );
		FreeSound();
		return qfalse;
	}
	memset( lpWaveHdr, 0, sizeof( WAVEHDR ) * WAV_BUFFERS );
	Com_DPrintf( "ok\n" );

	/* After allocation, set up and prepare headers. */
	Com_DPrintf( "...preparing headers: " );
	for ( i = 0 ; i < WAV_BUFFERS ; i++ )
	{
		lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE;
		lpWaveHdr[i].lpData = lpData + i * WAV_BUFFER_SIZE;

		if ( waveOutPrepareHeader( hWaveOut, lpWaveHdr + i, sizeof( WAVEHDR ) ) !=
			 MMSYSERR_NOERROR ) {
			Com_Printf( "failed\n" );
			FreeSound();
			return qfalse;
		}
	}
	Com_DPrintf( "ok\n" );

	dma.samples = gSndBufSize / ( dma.samplebits / 8 );
	dma.samplepos = 0;
	dma.submission_chunk = 512;
	dma.buffer = (unsigned char *) lpData;
	sample16 = ( dma.samplebits / 8 ) - 1;

	wav_init = qtrue;

	return qtrue;
}


/*
==================
SNDDMA_Shutdown
==================
*/
void SNDDMA_Shutdown( void ) {
	Com_DPrintf( "Shutting down sound system\n" );

	if ( wav_init ) {
		FreeSound();
		return;
		wav_init = qfalse;
	}

	if ( pDS ) {
		Com_DPrintf( "Destroying DS buffers\n" );
		if ( pDS ) {
			Com_DPrintf( "...setting NORMAL coop level\n" );
			pDS->lpVtbl->SetCooperativeLevel( pDS, g_wv.hWnd, DSSCL_NORMAL );
		}

		if ( pDSBuf ) {
			Com_DPrintf( "...stopping and releasing sound buffer\n" );
			pDSBuf->lpVtbl->Stop( pDSBuf );
			pDSBuf->lpVtbl->Release( pDSBuf );
		}

		// only release primary buffer if it's not also the mixing buffer we just released
		if ( pDSPBuf && ( pDSBuf != pDSPBuf ) ) {
			Com_DPrintf( "...releasing primary buffer\n" );
			pDSPBuf->lpVtbl->Release( pDSPBuf );
		}
		pDSBuf = NULL;
		pDSPBuf = NULL;

		dma.buffer = NULL;

		Com_DPrintf( "...releasing DS object\n" );
		pDS->lpVtbl->Release( pDS );
	}

	if ( hInstDS ) {
		Com_DPrintf( "...freeing DSOUND.DLL\n" );
		FreeLibrary( hInstDS );
		hInstDS = NULL;
	}

	pDS = NULL;
	pDSBuf = NULL;
	pDSPBuf = NULL;
	dsound_init = qfalse;
	memset( (void *)&dma, 0, sizeof( dma ) );
}

/*
==================
SNDDMA_Init

Initialize direct sound
Returns false if failed
==================
*/
int SNDDMA_Init( void ) {

	memset( (void *)&dma, 0, sizeof( dma ) );
	dsound_init = 0;


	if ( !SNDDMA_InitDS() ) {
		return 0;
	}

	dsound_init = qtrue;

	Com_DPrintf( "Completed successfully\n" );


	return 1;
}


int SNDDMA_InitDS() {
	HRESULT hresult;
	qboolean pauseTried;
	DSBUFFERDESC dsbuf;
	DSBCAPS dsbcaps;
	WAVEFORMATEX format;

	if ( s_wavonly->integer ) {

		snd_iswave = SNDDMA_InitWav();

		if ( snd_iswave ) {
			Com_Printf( "Wave sound init succeeded\n" );
		}
		return 1;
	}

	Com_Printf( "Initializing DirectSound\n" );

	if ( !hInstDS ) {
		Com_DPrintf( "...loading dsound.dll: " );

		hInstDS = LoadLibrary( "dsound.dll" );

		if ( hInstDS == NULL ) {
			Com_Printf( "failed\n" );
			return 0;
		}

		Com_DPrintf( "ok\n" );
		pDirectSoundCreate = ( long ( __stdcall * )( struct _GUID *,struct IDirectSound **,struct IUnknown * ) )
							 GetProcAddress( hInstDS,"DirectSoundCreate" );

		if ( !pDirectSoundCreate ) {
			Com_Printf( "*** couldn't get DS proc addr ***\n" );
			return 0;
		}
	}

	Com_DPrintf( "...creating DS object: " );
	pauseTried = qfalse;
	while ( ( hresult = iDirectSoundCreate( NULL, &pDS, NULL ) ) != DS_OK ) {
		if ( hresult != DSERR_ALLOCATED ) {
			Com_Printf( "failed\n" );
			return 0;
		}

		if ( pauseTried ) {
			Com_Printf( "failed, hardware already in use\n" );
			return 0;
		}
		// first try just waiting five seconds and trying again
		// this will handle the case of a sysyem beep playing when the
		// game starts
		Com_DPrintf( "retrying...\n" );
		Sleep( 3000 );
		pauseTried = qtrue;
	}
	Com_DPrintf( "ok\n" );

	Com_DPrintf( "...setting DSSCL_NORMAL coop level: " );

	if ( DS_OK != pDS->lpVtbl->SetCooperativeLevel( pDS, g_wv.hWnd, DSSCL_NORMAL ) ) {
		Com_Printf( "failed\n" );
		SNDDMA_Shutdown();
		return qfalse;
	}
	Com_DPrintf( "ok\n" );


	// create the secondary buffer we'll actually work with
	dma.channels = 2;
	dma.samplebits = 16;

	if ( s_khz->integer == 44 ) {
		dma.speed = 44100;
	} else if ( s_khz->integer == 22 ) {
		dma.speed = 22050;
	} else {
		dma.speed = 11025;
	}

	memset( &format, 0, sizeof( format ) );
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = dma.channels;
	format.wBitsPerSample = dma.samplebits;
	format.nSamplesPerSec = dma.speed;
	format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	memset( &dsbuf, 0, sizeof( dsbuf ) );
	dsbuf.dwSize = sizeof( DSBUFFERDESC );

	// Micah: take advantage of 2D hardware.if available.

#define idDSBCAPS_GETCURRENTPOSITION2 0x00010000

	dsbuf.dwFlags = DSBCAPS_LOCHARDWARE ;           //DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCHARDWARE;
	dsbuf.dwBufferBytes = SECONDARY_BUFFER_SIZE;
	dsbuf.lpwfxFormat = &format;

	memset( &dsbcaps, 0, sizeof( dsbcaps ) );
	dsbcaps.dwSize = sizeof( dsbcaps );

	Com_DPrintf( "...creating secondary buffer: " );
	if ( DS_OK == pDS->lpVtbl->CreateSoundBuffer( pDS, &dsbuf, &pDSBuf, NULL ) ) {
		Com_Printf( "locked hardware.  ok\n" );
	} else {
		// Couldn't get hardware, fallback to software.
		dsbuf.dwFlags = DSBCAPS_LOCSOFTWARE;    //DSBCAPS_CTRLFREQUENCY | DSBCAPS_LOCSOFTWARE;
		if ( DS_OK != pDS->lpVtbl->CreateSoundBuffer( pDS, &dsbuf, &pDSBuf, NULL ) ) {
			Com_Printf( "failed\n" );
			SNDDMA_Shutdown();
			return qfalse;
		}
		Com_DPrintf( "forced to software.  ok\n" );
	}

	// Make sure mixer is active
	if ( DS_OK != pDSBuf->lpVtbl->Play( pDSBuf, 0, 0, DSBPLAY_LOOPING ) ) {
		Com_Printf( "*** Looped sound play failed ***\n" );
		SNDDMA_Shutdown();
		return qfalse;
	}

	// get the returned buffer size
	if ( DS_OK != pDSBuf->lpVtbl->GetCaps( pDSBuf, &dsbcaps ) ) {
		Com_Printf( "*** GetCaps failed ***\n" );
		SNDDMA_Shutdown();
		return qfalse;
	}

	gSndBufSize = dsbcaps.dwBufferBytes;

	dma.channels = format.nChannels;
	dma.samplebits = format.wBitsPerSample;
	dma.speed = format.nSamplesPerSec;
	dma.samples = gSndBufSize / ( dma.samplebits / 8 );
	dma.submission_chunk = 1;
	dma.buffer = NULL;          // must be locked first

	sample16 = ( dma.samplebits / 8 ) - 1;
	return 1;
}
/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos( void ) {
	MMTIME mmtime;
	int s;
	DWORD dwWrite;

	if ( wav_init ) {
		s = snd_sent * WAV_BUFFER_SIZE;
		s >>= sample16;

		s &= ( dma.samples - 1 );

		return s;
	}


	if ( !dsound_init || !pDSBuf ) {
		return 0;
	}


	mmtime.wType = TIME_SAMPLES;
	pDSBuf->lpVtbl->GetCurrentPosition( pDSBuf, &mmtime.u.sample, &dwWrite );

	s = mmtime.u.sample;

	s >>= sample16;

	s &= ( dma.samples - 1 );

	return s;
}

/*
==============
SNDDMA_BeginPainting

Makes sure dma.buffer is valid
===============
*/
void SNDDMA_BeginPainting( void ) {
	int reps;
	DWORD dwSize2;
	DWORD   *pbuf, *pbuf2;
	HRESULT hresult;
	DWORD dwStatus;

	if ( !pDSBuf ) {
		return;
	}

	// if the buffer was lost or stopped, restore it and/or restart it
	if ( pDSBuf->lpVtbl->GetStatus( pDSBuf, &dwStatus ) != DS_OK ) {
		Com_Printf( "Couldn't get sound buffer status\n" );
	}

	if ( dwStatus & DSBSTATUS_BUFFERLOST ) {
		pDSBuf->lpVtbl->Restore( pDSBuf );
	}

	if ( !( dwStatus & DSBSTATUS_PLAYING ) ) {
		pDSBuf->lpVtbl->Play( pDSBuf, 0, 0, DSBPLAY_LOOPING );
	}

	// lock the dsound buffer

	reps = 0;
	dma.buffer = NULL;

	while ( ( hresult = pDSBuf->lpVtbl->Lock( pDSBuf, 0, gSndBufSize, &pbuf, &locksize,
											  &pbuf2, &dwSize2, 0 ) ) != DS_OK )
	{
		if ( hresult != DSERR_BUFFERLOST ) {
			Com_Printf( "SNDDMA_BeginPainting: Lock failed with error '%s'\n", DSoundError( hresult ) );
			S_Shutdown();
			return;
		} else
		{
			pDSBuf->lpVtbl->Restore( pDSBuf );
		}

		if ( ++reps > 2 ) {
			return;
		}
	}
	dma.buffer = (unsigned char *)pbuf;
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
Also unlocks the dsound buffer
===============
*/
void SNDDMA_Submit( void ) {
	LPWAVEHDR h;
	int wResult;

	// unlock the dsound buffer
	if ( pDSBuf ) {
		pDSBuf->lpVtbl->Unlock( pDSBuf, dma.buffer, locksize, NULL, 0 );
	}
	if ( !wav_init ) {
		return;
	}

	//
	// find which sound blocks have completed
	//
	while ( 1 )
	{
		if ( snd_completed == snd_sent ) {
			Com_DPrintf( "Sound overrun\n" );
			break;
		}

		if ( !( lpWaveHdr[ snd_completed & WAV_MASK].dwFlags & WHDR_DONE ) ) {
			break;
		}

		snd_completed++;    // this buffer has been played
	}

	// submit a few new sound blocks
	//
	while ( ( ( snd_sent - snd_completed ) >> sample16 ) < 8 )
	{
		h = lpWaveHdr + ( snd_sent & WAV_MASK );
		if ( s_paintedtime / 256 <= snd_sent ) {
			break;  //	Com_Printf ("submit overrun\n");
		}
		snd_sent++;
		/*
		 * Now the data block can be sent to the output device. The
		 * waveOutWrite function returns immediately and waveform
		 * data is sent to the output device in the background.
		 */
		wResult = waveOutWrite( hWaveOut, h, sizeof( WAVEHDR ) );

		if ( wResult != MMSYSERR_NOERROR ) {
			Com_Printf( "Failed to write block to device\n" );
			FreeSound();
			return;
		}
	}
}


/*
=================
SNDDMA_Activate

When we change windows we need to do this
=================
*/
void SNDDMA_Activate( void ) {
	if ( !pDS ) {
		return;
	}

	if ( DS_OK != pDS->lpVtbl->SetCooperativeLevel( pDS, g_wv.hWnd, DSSCL_NORMAL ) ) {
		Com_Printf( "sound SetCooperativeLevel failed\n" );
		SNDDMA_Shutdown();
	}
}


