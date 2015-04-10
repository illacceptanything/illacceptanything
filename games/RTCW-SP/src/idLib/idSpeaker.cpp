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


// CM_FastPointTrace( trace_t *results, const idVec3 &start, const idVec3 &end );

#include "idAudio.h"
#include "idAudioHardware.h"

#define NUM_PLAY_NOTIFICATIONS  16


idStreamingBuffer*  g_pStreamingSound[MAX_STREAMING_SOUNDS];
HANDLE g_hNotificationEvent[MAX_STREAMING_SOUNDS];

dword g_dwNotifyThreadID      = 0;
HANDLE g_hNotifyThread         = NULL;
HINSTANCE g_hInst                 = NULL;
CRITICAL_SECTION g_crit;

//-----------------------------------------------------------------------------
// Name: NotificationProc()
// Desc: Handles dsound notifcation events
//-----------------------------------------------------------------------------
dword WINAPI NotificationProc( LPVOID lpParameter ) {
	HWND hDlg = (HWND) lpParameter;
	MSG msg;
	dword dwResult;
	bool bDone = false;

	while ( !bDone )
	{
		dwResult = MsgWaitForMultipleObjects( MAX_STREAMING_SOUNDS, g_hNotificationEvent,
											  false, INFINITE, QS_ALLEVENTS );
		dwResult -= WAIT_OBJECT_0;

		if ( dwResult < MAX_STREAMING_SOUNDS ) {
			EnterCriticalSection( &g_crit );
			if ( g_pStreamingSound[dwResult] && g_pStreamingSound[dwResult]->IsSoundPlaying() ) {
				g_pStreamingSound[dwResult]->HandleWaveStreamNotification();
			}
			LeaveCriticalSection( &g_crit );
		} else if ( dwResult == MAX_STREAMING_SOUNDS ) {
			while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
			{
				if ( msg.message == WM_QUIT ) {
					bDone = true;
				}
			}
		}
	}

	return 0;
}


idSpeaker::idSpeaker() {
	HRESULT hr;

	hw = new idAudioHardware();
	bool init = true;
	if ( FAILED( hr = hw->Initialize( DSSCL_PRIORITY, 2, 22050, 16 ) ) ) {
		init = false;           // do more here
		return;
	}

	int i;
	for ( i = 0; i < MAX_STREAMING_SOUNDS; i++ ) {
		g_pStreamingSound[i] = NULL;
		g_hNotificationEvent[i] = CreateEvent( NULL, false, false, NULL );
		if ( g_hNotificationEvent[i] == NULL ) {
			Com_Error( ERR_DROP, "could not create all windows events" );
		}
	}
	InitializeCriticalSection( &g_crit );

	// Create a thread to handle DSound notifications
	g_hNotifyThread = CreateThread( NULL, 0, NotificationProc,
									g_wv.hWnd, 0, &g_dwNotifyThreadID );

	// Get the 3D listener, so we can control its params
	hw->Get3DListenerInterface( &pDSListener );

	// Get listener parameters
	dsListenerParams.dwSize = sizeof( DS3DLISTENER );
	pDSListener->GetAllParameters( &dsListenerParams );

	s_dopplerFactor = Cvar_Get( "s_dopplerFactor", "1.0", CVAR_ARCHIVE );
	s_distanceFactor = Cvar_Get( "s_distanceFactor", "1.0", CVAR_ARCHIVE );
	s_rolloffFactor = Cvar_Get( "s_rolloffFactor", "1.0", CVAR_ARCHIVE );
	s_minDistance = Cvar_Get( "s_minDistance", "1.0", CVAR_ARCHIVE );
	s_maxDistance = Cvar_Get( "s_maxDistance", "40.0", CVAR_ARCHIVE );
	s_musicVolume = Cvar_Get( "s_musicVolume", "0.25", CVAR_ARCHIVE );

	listener.Zero();
	listenerTop.Zero();
	listenerFront.Zero();

	Com_Printf( "Sound system underway and doing nicely\n" );
}

bool idSpeaker::loop( const char *intro, const char *filename, int channel, bool looping, bool in3d, bool useNotification ) {
	HRESULT hr;
	idWavefile waveFile;
	dword dwNotifySize;
	int flags;
	int cflags;

	GUID algo;

	algo = GUID_NULL;
	flags = 0;
	cflags = DSBCAPS_GETCURRENTPOSITION2;

	if ( in3d ) {
		cflags |= DSBCAPS_CTRL3D;
		algo = DS3DALG_NO_VIRTUALIZATION;
	}
	if ( looping ) {
		flags = DSBPLAY_LOOPING;
	}
	if ( useNotification ) {
		cflags |= DSBCAPS_CTRLPOSITIONNOTIFY;
	}

	bool success = false;
	if ( useNotification ) {
		EnterCriticalSection( &g_crit );
	}
	// Load the wave file
	if ( SUCCEEDED( hr = waveFile.Open( intro, NULL, WAVEFILE_READ ) ) ) {
		if ( waveFile.GetSize() != 0 && waveFile.m_pwfx != NULL ) {

			// The wave file is valid, and waveFile.m_pwfx is the wave's format
			// so we are done with the reader.
			waveFile.Close();

			// This samples works by dividing a 4 second streaming buffer into
			// NUM_PLAY_NOTIFICATIONS (16) pieces.  It creates a notification for each
			// piece and when a notification arrives then it fills the circular streaming
			// buffer with new wav data over the sound data which was just played

			// Determine the g_dwNotifySize.  It should be an integer multiple of nBlockAlign
			dword nBlockAlign = (dword)waveFile.m_pwfx->nBlockAlign;
			INT nSamplesPerSec = waveFile.m_pwfx->nSamplesPerSec;
			dwNotifySize = nSamplesPerSec * 3 * nBlockAlign / NUM_PLAY_NOTIFICATIONS;
			dwNotifySize -= dwNotifySize % nBlockAlign;

			if ( g_pStreamingSound[channel] ) {
				g_pStreamingSound[channel]->Stop( 0 );            // streaming is always 0
			}
			// Create a new sound
			SAFE_DELETE( g_pStreamingSound[channel] );

			// Set up the direct sound buffer.  Request the NOTIFY flag, so
			// that we are notified as the sound buffer plays.  Note, that using this flag
			// may limit the amount of hardware acceleration that can occur.
			if ( SUCCEEDED( hr = hw->CreateStreaming( &g_pStreamingSound[channel], intro, filename,
													  cflags,
													  algo, useNotification, NUM_PLAY_NOTIFICATIONS,
													  dwNotifySize, g_hNotificationEvent[channel] ) ) ) {
				g_pStreamingSound[channel]->looping = looping;
				g_pStreamingSound[channel]->in3D = in3d;
				if ( in3d ) {
					g_pStreamingSound[channel]->Get3DBufferInterfaces();
				}
			}
			success = true;
		}
	}
	if ( useNotification ) {
		LeaveCriticalSection( &g_crit );
	}
	return success;
}

void idSpeaker::stopLoop( int channel ) {
	if ( this && g_pStreamingSound[channel] ) {
		g_pStreamingSound[channel]->Stop( 0 );                // looping is always channel 0
	}
}

void idSpeaker::updateLoop() {
	if ( this ) {
		int i;
		if ( g_pStreamingSound[0] ) {
			g_pStreamingSound[0]->SetVolume( 0, idSpeak->getMusicVolume() );
		}
		for ( i = 0; i < MAX_STREAMING_SOUNDS; i++ ) {
			if ( g_pStreamingSound[i] && !g_pStreamingSound[i]->UsingNotification() ) {
				g_pStreamingSound[i]->HandleWaveStreamNotification();
			}
		}
	}
}

idSpeaker::~idSpeaker() {

	int i;
	// Close down notification thread
	PostThreadMessage( g_dwNotifyThreadID, WM_QUIT, 0, 0 );
	WaitForSingleObject( g_hNotifyThread, INFINITE );
	CloseHandle( g_hNotifyThread );
	if ( g_hNotificationEvent ) {
		for ( i = 0; i < MAX_STREAMING_SOUNDS; i++ ) {
			if ( g_hNotificationEvent[i] ) {
				CloseHandle( g_hNotificationEvent[i] );
			}
		}
	}

	delete hw;
}

void idSpeaker::process() {

	pDSListener->SetPosition( listener.x, listener.y, listener.z, DS3D_DEFERRED );
	pDSListener->SetOrientation( listenerFront.x, listenerFront.y, listenerFront.z, listenerTop.x, listenerTop.y, listenerTop.z, DS3D_DEFERRED );


	pDSListener->SetDopplerFactor( s_dopplerFactor->value, DS3D_DEFERRED );
	pDSListener->SetDistanceFactor( s_distanceFactor->value, DS3D_DEFERRED );
	pDSListener->SetRolloffFactor( s_rolloffFactor->value, DS3D_DEFERRED );

	pDSListener->CommitDeferredSettings();
}

int idSpeaker::getAudioHardwareChannel() {
	return 0;
}

void idSpeaker::LOCK() {
}

void idSpeaker::UNLOCK() {
}


