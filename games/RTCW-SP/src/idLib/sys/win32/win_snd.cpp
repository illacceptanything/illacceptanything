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


#include "../../idLib.h"
#include "../../idAudio.h"

#include "../../../mssdk/include/dxerr8.h"
#include "../../../mssdk/include/dsound.h"

#include "eax.h"

#undef DEFINE_GUID

#define DEFINE_GUID( name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8 ) \
	EXTERN_C const GUID name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID( CLSID_EAXDirectSound8,
			 0xca503b60,
			 0xb176,
			 0x11d4,
			 0xa0, 0x94, 0xd0, 0xc0, 0xbf, 0x3a, 0x56, 0xc );


//-----------------------------------------------------------------------------
// Name: idAudioHardware::idAudioHardware()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
idAudioHardware::idAudioHardware() {
	CoInitialize( 0 );
	m_pDS = NULL;
	pDSBPrimary = NULL;
	pEAXListener = NULL;
	eax = false;
}

//-----------------------------------------------------------------------------
// Name: idAudioHardware::~idAudioHardware()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
idAudioHardware::~idAudioHardware() {
	if ( pEAXListener ) {
		IKsPropertySet_Release( pEAXListener );
	}
	SAFE_RELEASE( pDSBPrimary );
	SAFE_RELEASE( m_pDS );
	CoUninitialize();
}

//-----------------------------------------------------------------------------
// Name: idAudioHardware::Initialize()
// Desc: Initializes the IDirectSound object and also sets the primary buffer
//       format.  This function must be called before any others.
//-----------------------------------------------------------------------------
int idAudioHardware::Initialize( dword dwCoopLevel,
								 dword dwPrimaryChannels,
								 dword dwPrimaryFreq,
								 dword dwPrimaryBitRate ) {
	int hr;

	SAFE_RELEASE( pDSBPrimary );
	SAFE_RELEASE( m_pDS );

	hr = CoCreateInstance( CLSID_EAXDirectSound8, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (void**)&m_pDS );
	if ( FAILED( hr ) ) {
		// Failed to initialize eax.dll, if this happens then we can fallback to use Direct Sound
		if ( FAILED( hr = CoCreateInstance( CLSID_DirectSound8, NULL, CLSCTX_INPROC_SERVER, IID_IDirectSound8, (void**)&m_pDS ) ) ) {
			Com_Error( ERR_DROP, TEXT( "DirectSoundCreate" ) );
		}
	}
	// Need to call the initialize function on Direct Sound when creating through CoCreateInstance instead of
	// using the DirectSoundCreate call.
	if ( m_pDS->Initialize( NULL ) != DS_OK ) {
		Com_Error( ERR_DROP, TEXT( "Initialize" ), hr );
	}

	// Set DirectSound coop level
	if ( FAILED( hr = m_pDS->SetCooperativeLevel( g_wv.hWnd, dwCoopLevel ) ) ) {
		Com_Error( ERR_DROP, TEXT( "SetCooperativeLevel" ), hr );
	}

	// Obtain primary buffer, asking it for 3D control
	// Get the primary buffer
	DSBUFFERDESC dsbdesc;
	memset( &dsbdesc, 0, sizeof( DSBUFFERDESC ) );
	dsbdesc.dwSize = sizeof( DSBUFFERDESC );
	dsbdesc.dwFlags = DSBCAPS_CTRL3D | DSBCAPS_PRIMARYBUFFER;
	if ( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbdesc, &pDSBPrimary, NULL ) ) ) {
		Com_Error( ERR_DROP, TEXT( "Initialize" ), hr );
	}

	// Set primary buffer format
	SetPrimaryBufferFormat( dwPrimaryChannels, dwPrimaryFreq, dwPrimaryBitRate );

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: idAudioHardware::SetPrimaryBufferFormat()
// Desc: Set primary buffer to a specified format
//       For example, to set the primary buffer format to 22kHz stereo, 16-bit
//       then:   dwPrimaryChannels = 2
//               dwPrimaryFreq     = 22050,
//               dwPrimaryBitRate  = 16
//-----------------------------------------------------------------------------
int idAudioHardware::SetPrimaryBufferFormat( dword dwPrimaryChannels,
											 dword dwPrimaryFreq,
											 dword dwPrimaryBitRate ) {
	int hr;

	if ( m_pDS == NULL ) {
		return CO_E_NOTINITIALIZED;
	}

	WAVEFORMATEX wfx;
	memset( &wfx, 0, sizeof( wfx ) );
	wfx.wFormatTag      = WAVE_FORMAT_PCM;
	wfx.nChannels       = (WORD) dwPrimaryChannels;
	wfx.nSamplesPerSec  = dwPrimaryFreq;
	wfx.wBitsPerSample  = (WORD) dwPrimaryBitRate;
	wfx.nBlockAlign     = wfx.wBitsPerSample / 8 * wfx.nChannels;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	if ( FAILED( hr = pDSBPrimary->SetFormat( &wfx ) ) ) {
		Com_Error( ERR_DROP, TEXT( "SetFormat" ), hr );
	}

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: idAudioHardware::Get3DListenerInterface()
// Desc: Returns the 3D listener interface associated with primary buffer.
//-----------------------------------------------------------------------------
int idAudioHardware::Get3DListenerInterface( LPDIRECTSOUND3DLISTENER8* ppDSListener ) {
	int hr;

	if ( ppDSListener == NULL ) {
		return E_INVALIDARG;
	}
	if ( m_pDS == NULL ) {
		return CO_E_NOTINITIALIZED;
	}

	*ppDSListener = NULL;

	if ( FAILED( hr = pDSBPrimary->QueryInterface( IID_IDirectSound3DListener8,
												   (void**)ppDSListener ) ) ) {
		Com_Error( ERR_DROP, TEXT( "QueryInterface" ), hr );
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: idAudioHardware::Get3DListenerInterface()
// Desc: Returns the 3D listener interface associated with primary buffer.
//-----------------------------------------------------------------------------
bool idAudioHardware::GetEAX( LPDIRECTSOUND3DBUFFER genBuf ) {
	static bool tried = false;

	if ( !eax && !tried ) {
		ULONG support = 0;
		tried = true;

		genBuf->QueryInterface( IID_IKsPropertySet, (void**)&pEAXListener );

		if ( FAILED( pEAXListener->QuerySupport( DSPROPSETID_EAX_ListenerProperties, DSPROPERTY_EAXLISTENER_ALLPARAMETERS, &support ) ) ) {
			return false;
		}
		if ( ( support & ( KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET ) ) != ( KSPROPERTY_SUPPORT_GET | KSPROPERTY_SUPPORT_SET ) ) {
			return false;
		}

		eax = true;
	}
	return eax;
}



//-----------------------------------------------------------------------------
// Name: idAudioHardware::Create()
// Desc:
//-----------------------------------------------------------------------------
int idAudioHardware::Create( idAudioBuffer** ppSound,
							 const char* strWaveFileName,
							 dword dwCreationFlags,
							 GUID guid3DAlgorithm,
							 dword dwNumBuffers,
							 dword dwMaxNumBuffers ) {
	int hr;
	int hrRet = S_OK;
	dword i;
	LPDIRECTSOUNDBUFFER* apDSBuffer     = NULL;
	dword dwDSBufferSize = NULL;
	idWavefile*           pWaveFile      = NULL;

	if ( m_pDS == NULL ) {
		return CO_E_NOTINITIALIZED;
	}
	if ( strWaveFileName == NULL || ppSound == NULL || dwNumBuffers < 1 ) {
		return E_INVALIDARG;
	}

	apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
	if ( apDSBuffer == NULL ) {
		hr = E_OUTOFMEMORY;
		goto LFail;
	}

	pWaveFile = new idWavefile();
	if ( pWaveFile == NULL ) {
		hr = E_OUTOFMEMORY;
		goto LFail;
	}

	pWaveFile->Open( strWaveFileName, NULL, WAVEFILE_READ );

	if ( pWaveFile->GetSize() == 0 ) {
		// Wave is blank, so don't create it.
		hr = E_FAIL;
		goto LFail;
	}

	// Make the DirectSound buffer the same size as the wav file
	dwDSBufferSize = pWaveFile->GetSize();

	// Create the direct sound buffer, and only request the flags needed
	// since each requires some overhead and limits if the buffer can
	// be hardware accelerated
	DSBUFFERDESC dsbd;
	memset( &dsbd, 0, sizeof( DSBUFFERDESC ) );
	dsbd.dwSize          = sizeof( DSBUFFERDESC );
	dsbd.dwFlags         = dwCreationFlags;
	dsbd.dwBufferBytes   = dwDSBufferSize;
	dsbd.guid3DAlgorithm = guid3DAlgorithm;
	dsbd.lpwfxFormat     = (WAVEFORMATEX*)pWaveFile->m_pwfx;

	// DirectSound is only guarenteed to play PCM data.  Other
	// formats may or may not work depending the sound card driver.
	hr = m_pDS->CreateSoundBuffer( &dsbd, &apDSBuffer[0], NULL );

	// Be sure to return this error code if it occurs so the
	// callers knows this happened.
	if ( hr == DS_NO_VIRTUALIZATION ) {
		hrRet = DS_NO_VIRTUALIZATION;
	}

	if ( FAILED( hr ) ) {
		// DSERR_BUFFERTOOSMALL will be returned if the buffer is
		// less than DSBSIZE_FX_MIN (100ms) and the buffer is created
		// with DSBCAPS_CTRLFX.
		if ( hr != DSERR_BUFFERTOOSMALL ) {
			Com_Error( ERR_DROP, TEXT( "CreateSoundBuffer" ), hr );
		}

		goto LFail;
	}

	for ( i = 1; i < dwNumBuffers; i++ )     {
		if ( FAILED( hr = m_pDS->DuplicateSoundBuffer( apDSBuffer[0], &apDSBuffer[i] ) ) ) {
			Com_Error( ERR_DROP, TEXT( "DuplicateSoundBuffer" ), hr );
			goto LFail;
		}
	}

	// Create the sound
	*ppSound = new idAudioBuffer( apDSBuffer, dwDSBufferSize, dwNumBuffers, dwMaxNumBuffers, pWaveFile );

	pWaveFile->Close();

	SAFE_DELETE( apDSBuffer );

	return hrRet;

LFail:
	// Cleanup
	SAFE_DELETE( pWaveFile );
	SAFE_DELETE( apDSBuffer );
	return hr;
}









//-----------------------------------------------------------------------------
// Name: idAudioHardware::CreateFromMemory()
// Desc:
//-----------------------------------------------------------------------------
int idAudioHardware::CreateFromMemory( idAudioBuffer** ppSound,
									   byte* pbData,
									   ulong ulDataSize,
									   waveformatex* pwfx,
									   dword dwCreationFlags,
									   GUID guid3DAlgorithm,
									   dword dwNumBuffers ) {
	int hr;
	dword i;
	LPDIRECTSOUNDBUFFER* apDSBuffer     = NULL;
	dword dwDSBufferSize = NULL;
	idWavefile*           pWaveFile      = NULL;

	if ( m_pDS == NULL ) {
		return CO_E_NOTINITIALIZED;
	}
	if ( pbData == NULL || ppSound == NULL || dwNumBuffers < 1 ) {
		return E_INVALIDARG;
	}

	apDSBuffer = new LPDIRECTSOUNDBUFFER[dwNumBuffers];
	if ( apDSBuffer == NULL ) {
		hr = E_OUTOFMEMORY;
		goto LFail;
	}

	pWaveFile = new idWavefile();
	if ( pWaveFile == NULL ) {
		hr = E_OUTOFMEMORY;
		goto LFail;
	}

	pWaveFile->OpenFromMemory( pbData,ulDataSize, pwfx, WAVEFILE_READ );


	// Make the DirectSound buffer the same size as the wav file
	dwDSBufferSize = ulDataSize;

	// Create the direct sound buffer, and only request the flags needed
	// since each requires some overhead and limits if the buffer can
	// be hardware accelerated
	DSBUFFERDESC dsbd;
	memset( &dsbd, 0, sizeof( DSBUFFERDESC ) );
	dsbd.dwSize          = sizeof( DSBUFFERDESC );
	dsbd.dwFlags         = dwCreationFlags;
	dsbd.dwBufferBytes   = dwDSBufferSize;
	dsbd.guid3DAlgorithm = guid3DAlgorithm;
	dsbd.lpwfxFormat     = (WAVEFORMATEX *)pwfx;

	if ( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbd, &apDSBuffer[0], NULL ) ) ) {
		Com_Error( ERR_DROP, TEXT( "CreateSoundBuffer" ), hr );
		goto LFail;
	}

	for ( i = 1; i < dwNumBuffers; i++ ) {
		if ( FAILED( hr = m_pDS->DuplicateSoundBuffer( apDSBuffer[0], &apDSBuffer[i] ) ) ) {
			Com_Error( ERR_DROP, TEXT( "DuplicateSoundBuffer" ), hr );
			goto LFail;
		}
	}

	// Create the sound
	*ppSound = new idAudioBuffer( apDSBuffer, dwDSBufferSize, dwNumBuffers, dwNumBuffers, pWaveFile );

	SAFE_DELETE( apDSBuffer );
	return S_OK;

LFail:
	// Cleanup

	SAFE_DELETE( apDSBuffer );
	return hr;
}





//-----------------------------------------------------------------------------
// Name: idAudioHardware::CreateStreaming()
// Desc:
//-----------------------------------------------------------------------------
int idAudioHardware::CreateStreaming( idStreamingBuffer** ppStreamingSound,
									  const char* strIntroWaveFileName,
									  const char* strWaveFileName,
									  dword dwCreationFlags,
									  GUID guid3DAlgorithm,
									  bool useNotification,
									  dword dwNotifyCount,
									  dword dwNotifySize,
									  HANDLE hNotifyEvent ) {
	int hr;

	if ( m_pDS == NULL ) {
		return CO_E_NOTINITIALIZED;
	}
	if ( strIntroWaveFileName == NULL || ppStreamingSound == NULL || ( useNotification && ( hNotifyEvent == NULL ) ) ) {
		return E_INVALIDARG;
	}

	LPDIRECTSOUNDBUFFER pDSBuffer      = NULL;
	dword dwDSBufferSize = NULL;
	idWavefile*          pWaveFile      = NULL;
	idWavefile*          qWaveFile      = NULL;
	DSBPOSITIONNOTIFY*  aPosNotify     = NULL;
	LPDIRECTSOUNDNOTIFY pDSNotify      = NULL;

	if ( strIntroWaveFileName ) {
		pWaveFile = new idWavefile();
		pWaveFile->Open( strIntroWaveFileName, NULL, WAVEFILE_READ );
	}

	if ( strWaveFileName && strcmp( strIntroWaveFileName, strWaveFileName ) ) {
		qWaveFile = new idWavefile();
		qWaveFile->Open( strWaveFileName, NULL, WAVEFILE_READ );
	}

	// Figure out how big the DSound buffer should be
	dwDSBufferSize = dwNotifySize * dwNotifyCount;

	// Set up the direct sound buffer.  Request the NOTIFY flag, so
	// that we are notified as the sound buffer plays.  Note, that using this flag
	// may limit the amount of hardware acceleration that can occur.
	DSBUFFERDESC dsbd;
	memset( &dsbd, 0, sizeof( DSBUFFERDESC ) );
	dsbd.dwSize          = sizeof( DSBUFFERDESC );
	dsbd.dwFlags         = dwCreationFlags |
						   DSBCAPS_GETCURRENTPOSITION2;

	dsbd.dwBufferBytes   = dwDSBufferSize;
	dsbd.guid3DAlgorithm = guid3DAlgorithm;
	dsbd.lpwfxFormat     = (WAVEFORMATEX *)pWaveFile->m_pwfx;

	if ( FAILED( hr = m_pDS->CreateSoundBuffer( &dsbd, &pDSBuffer, NULL ) ) ) {
		// If wave format isn't then it will return
		// either DSERR_BADFORMAT or E_INVALIDARG
		Com_Error( ERR_DROP, TEXT( "CreateSoundBuffer" ), hr );
	}

	if ( useNotification ) {
		// Create the notification events, so that we know when to fill
		// the buffer as the sound plays.
		if ( FAILED( hr = pDSBuffer->QueryInterface( IID_IDirectSoundNotify,
													 (void**)&pDSNotify ) ) ) {
			SAFE_DELETE( aPosNotify );
			Com_Error( ERR_DROP, TEXT( "QueryInterface" ), hr );
		}

		aPosNotify = new DSBPOSITIONNOTIFY[ dwNotifyCount ];
		if ( aPosNotify == NULL ) {
			return E_OUTOFMEMORY;
		}

		for ( dword i = 0; i < dwNotifyCount; i++ )
		{
			aPosNotify[i].dwOffset     = ( dwNotifySize * i ) + dwNotifySize - 1;
			aPosNotify[i].hEventNotify = hNotifyEvent;
		}

		// Tell DirectSound when to notify us. The notification will come in the from
		// of signaled events that are handled in WinMain()
		if ( FAILED( hr = pDSNotify->SetNotificationPositions( dwNotifyCount,
															   aPosNotify ) ) ) {
			SAFE_RELEASE( pDSNotify );
			SAFE_DELETE( aPosNotify );
			Com_Error( ERR_DROP, TEXT( "SetNotificationPositions" ), hr );
		}

		SAFE_RELEASE( pDSNotify );
		SAFE_DELETE( aPosNotify );
	}
	// Create the sound
	*ppStreamingSound = new idStreamingBuffer( pDSBuffer, dwDSBufferSize, pWaveFile, qWaveFile, useNotification, dwNotifySize );

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: idAudioBuffer::idAudioBuffer()
// Desc: Constructs the class
//-----------------------------------------------------------------------------
idAudioBuffer::idAudioBuffer( LPDIRECTSOUNDBUFFER* apDSBuffer, dword dwDSBufferSize,
							  dword dwNumBuffers, dword dwMaxNumBuffers, idWavefile* pWaveFile ) {
	dword i;

	m_pDS3DBuffer = NULL;
	name = NULL;

	m_apDSBuffer = new LPDIRECTSOUNDBUFFER[dwMaxNumBuffers];
	for ( i = 0; i < dwNumBuffers; i++ ) {
		m_apDSBuffer[i] = apDSBuffer[i];
	}

	m_dwDSBufferSize = dwDSBufferSize;
	m_dwNumBuffers   = dwNumBuffers;
	m_dwMaxNumBuffers = dwMaxNumBuffers;
	m_pWaveFile      = pWaveFile;

	FillBufferWithSound( m_apDSBuffer[0], false );

	// Make DirectSound do pre-processing on sound effects
	for ( i = 0; i < dwNumBuffers; i++ ) {
		m_apDSBuffer[i]->SetCurrentPosition( 0 );
	}

	for ( i = 0; i < MAX_CHANNELS; i++ ) {
		entnum[i] = 0;
		entchannel[i] = 0;
		flags[i] = 0;
	}
}




//-----------------------------------------------------------------------------
// Name: idAudioBuffer::~idAudioBuffer()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
idAudioBuffer::~idAudioBuffer() {
	for ( dword i = 0; i < m_dwNumBuffers; i++ ) {
		SAFE_RELEASE( m_apDSBuffer[i] );
		if ( m_pDS3DBuffer ) {
			SAFE_RELEASE( m_pDS3DBuffer[i] );
		}
	}

	SAFE_DELETE_ARRAY( m_apDSBuffer );
	SAFE_DELETE_ARRAY( m_pDS3DBuffer );
	SAFE_DELETE( m_pWaveFile );
	SAFE_DELETE( name );
}




//-----------------------------------------------------------------------------
// Name: idAudioBuffer::FillBufferWithSound()
// Desc: Fills a DirectSound buffer with a sound file
//-----------------------------------------------------------------------------
int idAudioBuffer::FillBufferWithSound( LPDIRECTSOUNDBUFFER pDSB, bool bRepeatWavIfBufferLarger ) {
	int hr;
	void*   pDSLockedBuffer      = NULL; // Pointer to locked buffer memory
	dword dwDSLockedBufferSize = 0;      // Size of the locked DirectSound buffer
	dword dwWavDataRead        = 0;      // Amount of data read from the wav file

	if ( pDSB == NULL ) {
		return CO_E_NOTINITIALIZED;
	}

	// Make sure we have focus, and we didn't just switch in from
	// an app which had a DirectSound device
	if ( FAILED( hr = RestoreBuffer( pDSB, NULL ) ) ) {
		Com_Error( ERR_DROP, TEXT( "RestoreBuffer" ), hr );
	}

	// Lock the buffer down
	if ( FAILED( hr = pDSB->Lock( 0, m_dwDSBufferSize,
								  &pDSLockedBuffer, &dwDSLockedBufferSize,
								  NULL, NULL, 0L ) ) ) {
		Com_Error( ERR_DROP, TEXT( "Lock" ), hr );
	}

	// Reset the wave file to the beginning
	m_pWaveFile->ResetFile();

	if ( FAILED( hr = m_pWaveFile->Read( (byte*) pDSLockedBuffer,
										 dwDSLockedBufferSize,
										 &dwWavDataRead ) ) ) {
		Com_Error( ERR_DROP, TEXT( "Read" ), hr );
	}

	if ( dwWavDataRead == 0 ) {
		// Wav is blank, so just fill with silence
		memset( pDSLockedBuffer, (byte)( m_pWaveFile->m_pwfx->wBitsPerSample == 8 ? 128 : 0 ), dwDSLockedBufferSize );
	} else if ( dwWavDataRead < dwDSLockedBufferSize )    {
		// If the wav file was smaller than the DirectSound buffer,
		// we need to fill the remainder of the buffer with data
		if ( bRepeatWavIfBufferLarger ) {
			// Reset the file and fill the buffer with wav data
			dword dwReadSoFar = dwWavDataRead;    // From previous call above.
			while ( dwReadSoFar < dwDSLockedBufferSize )
			{
				// This will keep reading in until the buffer is full
				// for very short files
				if ( FAILED( hr = m_pWaveFile->ResetFile() ) ) {
					Com_Error( ERR_DROP, TEXT( "ResetFile" ), hr );
				}

				hr = m_pWaveFile->Read( (byte*)pDSLockedBuffer + dwReadSoFar,
										dwDSLockedBufferSize - dwReadSoFar,
										&dwWavDataRead );
				if ( FAILED( hr ) ) {
					Com_Error( ERR_DROP, TEXT( "Read" ), hr );
				}

				dwReadSoFar += dwWavDataRead;
			}
		} else
		{
			// Don't repeat the wav file, just fill in silence
			memset( (byte*) pDSLockedBuffer + dwWavDataRead,
					(byte)( m_pWaveFile->m_pwfx->wBitsPerSample == 8 ? 128 : 0 ),
					dwDSLockedBufferSize - dwWavDataRead );
		}
	}

	// Unlock the buffer, we don't need it anymore.
	pDSB->Unlock( pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0 );

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: idAudioBuffer::RestoreBuffer()
// Desc: Restores the lost buffer. *pbWasRestored returns qtrue if the buffer was
//       restored.  It can also NULL if the information is not needed.
//-----------------------------------------------------------------------------
int idAudioBuffer::RestoreBuffer( LPDIRECTSOUNDBUFFER pDSB, bool* pbWasRestored ) {
	int hr;

	if ( pDSB == NULL ) {
		return CO_E_NOTINITIALIZED;
	}
	if ( pbWasRestored ) {
		*pbWasRestored = false;
	}

	dword dwStatus;
	if ( FAILED( hr = pDSB->GetStatus( &dwStatus ) ) ) {
		Com_Error( ERR_DROP, TEXT( "GetStatus" ), hr );
	}

	if ( dwStatus & DSBSTATUS_BUFFERLOST ) {
		// Since the app could have just been activated, then
		// DirectSound may not be giving us control yet, so
		// the restoring the buffer may fail.
		// If it does, sleep until DirectSound gives us control.
		do
		{
			hr = pDSB->Restore();
			if ( hr == DSERR_BUFFERLOST ) {
				Sleep( 10 );
			}
		}
		while ( hr = pDSB->Restore() );

		if ( pbWasRestored != NULL ) {
			*pbWasRestored = qtrue;
		}

		return S_OK;
	} else
	{
		return S_FALSE;
	}
}




//-----------------------------------------------------------------------------
// Name: idAudioBuffer::GetFreeBuffer()
// Desc: Checks to see if a buffer is playing and returns qtrue if it is.
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER idAudioBuffer::GetFreeBuffer() {
	bool bIsPlaying = false;

	if ( m_apDSBuffer == NULL ) {
		return NULL;
	}

	for ( dword i = 0; i < m_dwNumBuffers; i++ ) {
		if ( m_apDSBuffer[i] ) {
			dword dwStatus = 0;
			m_apDSBuffer[i]->GetStatus( &dwStatus );
			if ( ( dwStatus & DSBSTATUS_PLAYING ) == 0 ) {
				break;
			}
		}
	}

	if ( i != m_dwNumBuffers ) {
		return m_apDSBuffer[ i ];
	} else {
		return m_apDSBuffer[ rand() % m_dwNumBuffers ];
	}
}




//-----------------------------------------------------------------------------
// Name: idAudioBuffer::GetBuffer()
// Desc:
//-----------------------------------------------------------------------------
LPDIRECTSOUNDBUFFER idAudioBuffer::GetBuffer( dword dwIndex ) {
	if ( m_apDSBuffer == NULL ) {
		return NULL;
	}
	if ( dwIndex >= m_dwNumBuffers ) {
		return NULL;
	}

	return m_apDSBuffer[dwIndex];
}

LPDIRECTSOUND3DBUFFER idAudioBuffer::Get3DBuffer( dword dwIndex ) {
	if ( m_apDSBuffer == NULL ) {
		return NULL;
	}
	if ( dwIndex >= m_dwNumBuffers ) {
		return NULL;
	}

	return m_pDS3DBuffer[dwIndex];
}


void idAudioBuffer::SetPosition( int index, float x, float y, float z ) {
	if ( m_pDS3DBuffer[index] ) {
		m_pDS3DBuffer[index]->SetPosition( x, y, z, DS3D_DEFERRED );
	}
}

void idAudioBuffer::SetRange( int index, float min, float max ) {
	if ( m_pDS3DBuffer[index] ) {
		m_pDS3DBuffer[index]->SetMinDistance( min, DS3D_DEFERRED );
		m_pDS3DBuffer[index]->SetMaxDistance( max, DS3D_DEFERRED );
	}
}

void idAudioBuffer::SetVelocity( int index, float x, float y, float z ) {
	if ( m_pDS3DBuffer[index] ) {
		m_pDS3DBuffer[index]->SetVelocity( x, y, z, DS3D_DEFERRED );
	}
}

void idAudioBuffer::SetVolume( int index, float x ) {
	if ( m_apDSBuffer[index] ) {

		if ( x < 0 ) {
			x = 0;
		}
		if ( x > 1 ) {
			x = 1;
		}
		x = ( 1.0 - x ) * DSBVOLUME_MIN;
		m_apDSBuffer[index]->SetVolume( x );
	}
}

int idAudioBuffer::GetFreeIndex( void ) {
	bool bIsPlaying = false;

	if ( m_apDSBuffer == NULL ) {
		return -1;
	}

	for ( dword i = 0; i < m_dwNumBuffers; i++ ) {
		if ( m_apDSBuffer[i] ) {
			dword dwStatus = 0;
			m_apDSBuffer[i]->GetStatus( &dwStatus );
			if ( ( dwStatus & DSBSTATUS_PLAYING ) == 0 ) {
				break;
			}
		}
	}

	if ( i != m_dwNumBuffers ) {
		return i;
	} else {
		int hr;
		if ( m_dwNumBuffers < m_dwMaxNumBuffers ) {
			DS3DBUFFER g_dsBufferParams;                            // 3D buffer properties
			LPDIRECTSOUND m_pDS = idSpeak->hw->GetDirectSound();
			i = m_dwNumBuffers;
			if ( FAILED( hr = m_pDS->DuplicateSoundBuffer( m_apDSBuffer[0], &m_apDSBuffer[i] ) ) ) {
				Com_Error( ERR_DROP, TEXT( "DuplicateSoundBuffer" ), hr );
			}

			m_apDSBuffer[i]->QueryInterface( IID_IDirectSound3DBuffer8, (void**)&m_pDS3DBuffer[i] );
			// Get the 3D buffer parameters
			g_dsBufferParams.dwSize = sizeof( DS3DBUFFER );
			m_pDS3DBuffer[0]->GetAllParameters( &g_dsBufferParams );
			m_pDS3DBuffer[i]->SetAllParameters( &g_dsBufferParams, DS3D_DEFERRED );

			m_dwNumBuffers++;
			return i;
		}
	}
	return -1;
}


//-----------------------------------------------------------------------------
// Name: idAudioBuffer::Get3DBufferInterfaces()
// Desc:
//-----------------------------------------------------------------------------
void idAudioBuffer::Get3DBufferInterfaces( float minDistance, float maxDistance ) {
	int i;
	DS3DBUFFER g_dsBufferParams;                            // 3D buffer properties

	if ( m_apDSBuffer == NULL ) {
		return;
	}

	m_pDS3DBuffer = new LPDIRECTSOUND3DBUFFER[m_dwMaxNumBuffers];
	for ( i = 0; i < m_dwNumBuffers; i++ ) {
		if ( m_apDSBuffer[i] ) {
			m_apDSBuffer[i]->QueryInterface( IID_IDirectSound3DBuffer8, (void**)&m_pDS3DBuffer[i] );
			// Get the 3D buffer parameters
			g_dsBufferParams.dwSize = sizeof( DS3DBUFFER );
			m_pDS3DBuffer[i]->GetAllParameters( &g_dsBufferParams );

			// Set new 3D buffer parameters
			g_dsBufferParams.dwMode = DS3DMODE_NORMAL;

			g_dsBufferParams.flMinDistance = minDistance;
			g_dsBufferParams.flMaxDistance = maxDistance;
			m_pDS3DBuffer[i]->SetAllParameters( &g_dsBufferParams, DS3D_DEFERRED );


		}
	}
}

void idAudioBuffer::Make2D( dword index ) {
	if ( m_apDSBuffer == NULL ) {
		return ;
	}
	if ( index >= m_dwNumBuffers ) {
		return;
	}
	if ( m_pDS3DBuffer[index] ) {
		m_pDS3DBuffer[index]->SetMode( DS3DMODE_DISABLE, DS3D_DEFERRED );
	}
}

//-----------------------------------------------------------------------------
// Name: idAudioBuffer::Play()
// Desc: Plays the sound using voice management flags.  Pass in DSBPLAY_LOOPING
//       in the dwFlags to loop the sound
//-----------------------------------------------------------------------------
int idAudioBuffer::Play( dword index, dword dwPriority, dword dwFlags ) {
	int hr;
	bool bRestored;

	if ( m_apDSBuffer == NULL ) {
		return CO_E_NOTINITIALIZED;
	}

	if ( index >= m_dwNumBuffers ) {
		return 0;
	}

	LPDIRECTSOUNDBUFFER pDSB = GetBuffer( index );

	if ( pDSB == NULL ) {
		Com_Error( ERR_DROP, TEXT( "GetFreeBuffer" ), E_FAIL );
	}

	// Restore the buffer if it was lost
	if ( FAILED( hr = RestoreBuffer( pDSB, &bRestored ) ) ) {
		Com_Error( ERR_DROP, TEXT( "RestoreBuffer" ), hr );
	}

	if ( bRestored ) {
		// The buffer was restored, so we need to fill it with new data
		if ( FAILED( hr = FillBufferWithSound( pDSB, false ) ) ) {
			Com_Error( ERR_DROP, TEXT( "FillBufferWithSound" ), hr );
		}

		// Make DirectSound do pre-processing on sound effects
		Reset();
	}

	return pDSB->Play( 0, dwPriority, dwFlags );
}




//-----------------------------------------------------------------------------
// Name: idAudioBuffer::Stop()
// Desc: Stops the sound from playing
//-----------------------------------------------------------------------------
int idAudioBuffer::Stop( dword index ) {
	if ( this == NULL || m_apDSBuffer == NULL ) {
		return CO_E_NOTINITIALIZED;
	}

	int hr = 0;

	if ( index < m_dwNumBuffers ) {
		hr = m_apDSBuffer[index]->Stop();
	}

	return hr;
}

//-----------------------------------------------------------------------------
// Name: idAudioBuffer::Reset()
// Desc: Reset all of the sound buffers
//-----------------------------------------------------------------------------
int idAudioBuffer::Reset() {
	if ( m_apDSBuffer == NULL ) {
		return CO_E_NOTINITIALIZED;
	}

	int hr = 0;

	for ( dword i = 0; i < m_dwNumBuffers; i++ )
		hr |= m_apDSBuffer[i]->SetCurrentPosition( 0 );

	return hr;
}




//-----------------------------------------------------------------------------
// Name: idAudioBuffer::IsSoundPlaying()
// Desc: Checks to see if a buffer is playing and returns qtrue if it is.
//-----------------------------------------------------------------------------
bool idAudioBuffer::IsSoundPlaying() {
	bool bIsPlaying = false;

	if ( m_apDSBuffer == NULL ) {
		return false;
	}

	for ( dword i = 0; i < m_dwNumBuffers; i++ )
	{
		if ( m_apDSBuffer[i] ) {
			dword dwStatus = 0;
			m_apDSBuffer[i]->GetStatus( &dwStatus );
			if ( dwStatus & DSBSTATUS_PLAYING ) {
				bIsPlaying = true;
				break;
			}
		}
	}

	return bIsPlaying;
}

//-----------------------------------------------------------------------------
// Name: idAudioBuffer::IsSoundPlaying()
// Desc: Checks to see if a buffer is playing and returns qtrue if it is.
//-----------------------------------------------------------------------------
bool idAudioBuffer::AllChannelsPlaying() {
	if ( m_apDSBuffer == NULL ) {
		return false;
	}

	bool bIsPlaying = true;

	for ( dword i = 0; i < m_dwNumBuffers; i++ )
	{
		if ( m_apDSBuffer[i] ) {
			dword dwStatus = 0;
			m_apDSBuffer[i]->GetStatus( &dwStatus );
			if ( !( dwStatus & DSBSTATUS_PLAYING ) ) {
				bIsPlaying = false;
				break;
			}
		}
	}

	return bIsPlaying;
}




//-----------------------------------------------------------------------------
// Name: idStreamingBuffer::idStreamingBuffer()
// Desc: Setups up a buffer so data can be streamed from the wave file into
//       buffer.  This is very useful for large wav files that would take a
//       while to load.  The buffer is initially filled with data, then
//       as sound is played the notification events are signaled and more data
//       is written into the buffer by calling HandleWaveStreamNotification()
//-----------------------------------------------------------------------------
idStreamingBuffer::idStreamingBuffer( LPDIRECTSOUNDBUFFER pDSBuffer, dword dwDSBufferSize,
									  idWavefile* pWaveFile, idWavefile* qWaveFile, bool useNotification, dword dwNotifySize )
	: idAudioBuffer( &pDSBuffer, dwDSBufferSize, 1, 1, pWaveFile ) {
	m_pWaveFile         = pWaveFile;
	m_qWaveFile         = qWaveFile;
	m_dwLastPlayPos     = 0;
	m_dwPlayProgress    = 0;
	m_dwNotifySize      = dwNotifySize;
	m_dwNextWriteOffset = 0;
	m_dwNextEvent       = Com_Milliseconds() + ( m_dwNotifySize * 1000 ) / ( m_pWaveFile->m_pwfx->nAvgBytesPerSec );
	m_bFillNextNotificationWithSilence = false;
	m_bUsingNotification = useNotification;
}




//-----------------------------------------------------------------------------
// Name: idStreamingBuffer::~idStreamingBuffer()
// Desc: Destroys the class
//-----------------------------------------------------------------------------
idStreamingBuffer::~idStreamingBuffer() {
	SAFE_DELETE( m_pWaveFile );
	SAFE_DELETE( m_qWaveFile );
}



//-----------------------------------------------------------------------------
// Name: idStreamingBuffer::HandleWaveStreamNotification()
// Desc: Handle the notification that tell us to put more wav data in the
//       circular buffer
//-----------------------------------------------------------------------------
int idStreamingBuffer::HandleWaveStreamNotification() {
	if ( !m_bUsingNotification ) {
		int dwRead = Com_Milliseconds();
		if ( dwRead < m_dwNextEvent ) {
			return S_OK;
		}
		m_dwNextEvent = dwRead + ( m_dwNotifySize * 1000 ) / ( m_pWaveFile->m_pwfx->nAvgBytesPerSec );
	}

	int hr;
	dword dwCurrentPlayPos;
	dword dwPlayDelta;
	dword dwBytesWrittenToBuffer;
	void*   pDSLockedBuffer = NULL;
	void*   pDSLockedBuffer2 = NULL;
	dword dwDSLockedBufferSize;
	dword dwDSLockedBufferSize2;

	if ( m_apDSBuffer == NULL || m_pWaveFile == NULL ) {
		return CO_E_NOTINITIALIZED;
	}

	// Restore the buffer if it was lost
	bool bRestored;
	if ( FAILED( hr = RestoreBuffer( m_apDSBuffer[0], &bRestored ) ) ) {
		Com_Error( ERR_DROP, TEXT( "RestoreBuffer" ), hr );
	}

	if ( bRestored ) {
		// The buffer was restored, so we need to fill it with new data
		if ( FAILED( hr = FillBufferWithSound( m_apDSBuffer[0], false ) ) ) {
			Com_Error( ERR_DROP, TEXT( "FillBufferWithSound" ), hr );
		}
		return S_OK;
	}

	// Lock the DirectSound buffer
	if ( FAILED( hr = m_apDSBuffer[0]->Lock( m_dwNextWriteOffset, m_dwNotifySize,
											 &pDSLockedBuffer, &dwDSLockedBufferSize,
											 &pDSLockedBuffer2, &dwDSLockedBufferSize2, 0L ) ) ) {
		Com_Error( ERR_DROP, TEXT( "Lock" ), hr );
	}

	// m_dwDSBufferSize and m_dwNextWriteOffset are both multiples of m_dwNotifySize,
	// it should the second buffer should never be valid
	if ( pDSLockedBuffer2 != NULL ) {
		return E_UNEXPECTED;
	}

	if ( !m_bFillNextNotificationWithSilence ) {
		// Fill the DirectSound buffer with wav data
		if ( FAILED( hr = m_pWaveFile->Read( (byte*) pDSLockedBuffer,
											 dwDSLockedBufferSize,
											 &dwBytesWrittenToBuffer ) ) ) {
			Com_Error( ERR_DROP, TEXT( "Read" ), hr );
		}
	} else
	{
		// Fill the DirectSound buffer with silence
		memset( pDSLockedBuffer,
				(byte)( m_pWaveFile->m_pwfx->wBitsPerSample == 8 ? 128 : 0 ),
				dwDSLockedBufferSize );
		dwBytesWrittenToBuffer = dwDSLockedBufferSize;
	}

	// If the number of bytes written is less than the
	// amount we requested, we have a short file.
	if ( dwBytesWrittenToBuffer < dwDSLockedBufferSize ) {
		if ( !looping ) {
			// Fill in silence for the rest of the buffer.
			memset( (byte*) pDSLockedBuffer + dwBytesWrittenToBuffer,
					(byte)( m_pWaveFile->m_pwfx->wBitsPerSample == 8 ? 128 : 0 ),
					dwDSLockedBufferSize - dwBytesWrittenToBuffer );

			// Any future notifications should just fill the buffer with silence
			m_bFillNextNotificationWithSilence = qtrue;
		} else
		{
			// We are looping, so reset the file and fill the buffer with wav data
			dword dwReadSoFar = dwBytesWrittenToBuffer;    // From previous call above.
			while ( dwReadSoFar < dwDSLockedBufferSize )
			{
				if ( m_qWaveFile != NULL ) {
					m_pWaveFile->Close();
					m_pWaveFile = m_qWaveFile;
					m_qWaveFile = NULL;
				}
				// This will keep reading in until the buffer is full (for very short files).
				if ( FAILED( hr = m_pWaveFile->ResetFile() ) ) {
					Com_Error( ERR_DROP, TEXT( "ResetFile" ), hr );
				}

				if ( FAILED( hr = m_pWaveFile->Read( (byte*)pDSLockedBuffer + dwReadSoFar,
													 dwDSLockedBufferSize - dwReadSoFar,
													 &dwBytesWrittenToBuffer ) ) ) {
					Com_Error( ERR_DROP, TEXT( "Read" ), hr );
				}

				dwReadSoFar += dwBytesWrittenToBuffer;
			}
		}
	}

	// Unlock the DirectSound buffer
	m_apDSBuffer[0]->Unlock( pDSLockedBuffer, dwDSLockedBufferSize, NULL, 0 );

	// Figure out how much data has been played so far.  When we have played
	// passed the end of the file, we will either need to start filling the
	// buffer with silence or starting reading from the beginning of the file,
	// depending if the user wants to loop the sound
	if ( FAILED( hr = m_apDSBuffer[0]->GetCurrentPosition( &dwCurrentPlayPos, NULL ) ) ) {
		Com_Error( ERR_DROP, TEXT( "GetCurrentPosition" ), hr );
	}

	// Check to see if the position counter looped
	if ( dwCurrentPlayPos < m_dwLastPlayPos ) {
		dwPlayDelta = ( m_dwDSBufferSize - m_dwLastPlayPos ) + dwCurrentPlayPos;
	} else {
		dwPlayDelta = dwCurrentPlayPos - m_dwLastPlayPos;
	}

	m_dwPlayProgress += dwPlayDelta;
	m_dwLastPlayPos = dwCurrentPlayPos;

	// If we are now filling the buffer with silence, then we have found the end so
	// check to see if the entire sound has played, if it has then stop the buffer.
	if ( m_bFillNextNotificationWithSilence ) {
		// We don't want to cut off the sound before it's done playing.
		if ( m_dwPlayProgress >= m_pWaveFile->GetSize() ) {
			m_apDSBuffer[0]->Stop();
		}
	}

	// Update where the buffer will lock (for next time)
	m_dwNextWriteOffset += dwDSLockedBufferSize;
	m_dwNextWriteOffset %= m_dwDSBufferSize; // Circular buffer

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: idStreamingBuffer::Reset()
// Desc: Resets the sound so it will begin playing at the beginning
//-----------------------------------------------------------------------------
int idStreamingBuffer::Reset() {
	int hr;

	if ( m_apDSBuffer[0] == NULL || m_pWaveFile == NULL ) {
		return CO_E_NOTINITIALIZED;
	}

	m_dwLastPlayPos     = 0;
	m_dwPlayProgress    = 0;
	m_dwNextWriteOffset = 0;
	m_bFillNextNotificationWithSilence = false;

	// Restore the buffer if it was lost
	bool bRestored;
	if ( FAILED( hr = RestoreBuffer( m_apDSBuffer[0], &bRestored ) ) ) {
		Com_Error( ERR_DROP, TEXT( "RestoreBuffer" ), hr );
	}

	if ( bRestored ) {
		// The buffer was restored, so we need to fill it with new data
		if ( FAILED( hr = FillBufferWithSound( m_apDSBuffer[0], false ) ) ) {
			Com_Error( ERR_DROP, TEXT( "FillBufferWithSound" ), hr );
		}
	}

	m_pWaveFile->ResetFile();

	return m_apDSBuffer[0]->SetCurrentPosition( 0L );
}




//-----------------------------------------------------------------------------
// Name: idWavefile::idWavefile()
// Desc: Constructs the class.  Call Open() to open a wave file for reading.
//       Then call Read() as needed.  Calling the destructor or Close()
//       will close the file.
//-----------------------------------------------------------------------------
idWavefile::idWavefile() {
	m_pwfx    = NULL;
	m_hmmio   = -1;
	m_dwSize  = 0;
	m_bIsReadingFromMemory = false;
}




//-----------------------------------------------------------------------------
// Name: idWavefile::~idWavefile()
// Desc: Destructs the class
//-----------------------------------------------------------------------------
idWavefile::~idWavefile() {
	Close();

	if ( !m_bIsReadingFromMemory ) {
		SAFE_DELETE_ARRAY( m_pwfx );
	}
}




//-----------------------------------------------------------------------------
// Name: idWavefile::Open()
// Desc: Opens a wave file for reading
//-----------------------------------------------------------------------------
int idWavefile::Open( const char* strFileName, waveformatex* pwfx, dword dwFlags ) {
	int hr;
	m_dwFlags = dwFlags;
	m_bIsReadingFromMemory = false;

	if ( m_dwFlags == WAVEFILE_READ ) {
		if ( strFileName == NULL ) {
			return E_INVALIDARG;
		}
		SAFE_DELETE_ARRAY( m_pwfx );

		m_dwSize = FS_FOpenFileRead( strFileName, &m_hmmio, qtrue );
		if ( m_dwSize == 0xffffffff ) {
			m_dwSize = 0;
			return E_FAIL;
		}
		if ( FAILED( hr = ReadMMIO() ) ) {
			// ReadMMIO will fail if its an not a wave file
			FS_FCloseFile( m_hmmio );
			Com_Error( ERR_DROP, TEXT( "ReadMMIO" ), hr );
		}

		if ( FAILED( hr = ResetFile() ) ) {
			Com_Error( ERR_DROP, TEXT( "ResetFile" ), hr );
		}

		// After the reset, the size of the wav file is m_ck.cksize so store it now
		m_dwSize = m_ck.cksize;
	}

	if ( m_dwSize != -1 ) {
		return S_OK;
	}
	return E_FAIL;
}




//-----------------------------------------------------------------------------
// Name: idWavefile::OpenFromMemory()
// Desc: copy data to idWavefile member variable from memory
//-----------------------------------------------------------------------------
int idWavefile::OpenFromMemory( byte* pbData, ulong ulDataSize,
								waveformatex* pwfx, dword dwFlags ) {
	m_pwfx       = pwfx;
	m_ulDataSize = ulDataSize;
	m_pbData     = pbData;
	m_pbDataCur  = m_pbData;
	m_bIsReadingFromMemory = qtrue;

	if ( dwFlags != WAVEFILE_READ ) {
		return E_NOTIMPL;
	}

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: idWavefile::ReadMMIO()
// Desc: Support function for reading from a multimedia I/O stream.
//       m_hmmio must be valid before calling.  This function uses it to
//       update m_ckRiff, and m_pwfx.
//-----------------------------------------------------------------------------
int idWavefile::ReadMMIO() {
	mminfo ckIn;                    // chunk info. for general use.
	PCMWAVEFORMAT pcmWaveFormat;    // Temp PCM structure to load in.

	m_pwfx = NULL;

	FS_Read( &m_ckRiff, 12, m_hmmio );
	m_ckRiff.dwDataOffset = 12;

	// Check to make sure this is a valid wave file
	if ( ( m_ckRiff.ckid != FOURCC_RIFF ) ||
		 ( m_ckRiff.fccType != mmioFOURCC( 'W', 'A', 'V', 'E' ) ) ) {
		Com_Error( ERR_DROP, TEXT( "mmioFOURCC" ), E_FAIL );
	}

	// Search the input file for for the 'fmt ' chunk.
	ckIn.dwDataOffset = 12;
	do {
		if ( 8 != FS_Read( &ckIn, 8, m_hmmio ) ) {
			Com_Error( ERR_DROP, TEXT( "mmioDescend" ), E_FAIL );
		}
		ckIn.dwDataOffset += ckIn.cksize - 8;
	} while ( ckIn.ckid != mmioFOURCC( 'f', 'm', 't', ' ' ) );

	// Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
	// if there are extra parameters at the end, we'll ignore them
	if ( ckIn.cksize < (LONG) sizeof( PCMWAVEFORMAT ) ) {
		Com_Error( ERR_DROP, TEXT( "sizeof(PCMWAVEFORMAT)" ), E_FAIL );
	}

	// Read the 'fmt ' chunk into <pcmWaveFormat>.
	if ( FS_Read( (HPSTR) &pcmWaveFormat, sizeof( pcmWaveFormat ), m_hmmio ) != sizeof( pcmWaveFormat ) ) {
		Com_Error( ERR_DROP, TEXT( "mmioRead" ), E_FAIL );
	}

	// Allocate the waveformatex, but if its not pcm format, read the next
	// word, and thats how many extra bytes to allocate.
	if ( pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM ) {
		m_pwfx = (waveformatex*)new CHAR[ sizeof( waveformatex ) ];
		if ( NULL == m_pwfx ) {
			Com_Error( ERR_DROP, TEXT( "m_pwfx" ), E_FAIL );
		}

		// Copy the bytes from the pcm structure to the waveformatex structure
		memcpy( m_pwfx, &pcmWaveFormat, sizeof( pcmWaveFormat ) );
		m_pwfx->cbSize = 0;
	} else
	{
		// Read in length of extra bytes.
		WORD cbExtraBytes = 0L;
		if ( FS_Read( (CHAR*)&cbExtraBytes, sizeof( WORD ), m_hmmio ) != sizeof( WORD ) ) {
			Com_Error( ERR_DROP, TEXT( "mmioRead" ), E_FAIL );
		}

		m_pwfx = (waveformatex*)new CHAR[ sizeof( waveformatex ) + cbExtraBytes ];
		if ( NULL == m_pwfx ) {
			Com_Error( ERR_DROP, TEXT( "new" ), E_FAIL );
		}

		// Copy the bytes from the pcm structure to the waveformatex structure
		memcpy( m_pwfx, &pcmWaveFormat, sizeof( pcmWaveFormat ) );
		m_pwfx->cbSize = cbExtraBytes;

		// Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
		if ( FS_Read( ( CHAR* )( ( (byte*)&( m_pwfx->cbSize ) ) + sizeof( WORD ) ),
					  cbExtraBytes, m_hmmio ) != cbExtraBytes ) {
			SAFE_DELETE( m_pwfx );
			Com_Error( ERR_DROP, TEXT( "mmioRead" ), E_FAIL );
		}
	}

	return S_OK;
}




//-----------------------------------------------------------------------------
// Name: idWavefile::GetSize()
// Desc: Retuns the size of the read access wave file
//-----------------------------------------------------------------------------
dword idWavefile::GetSize() {
	return m_dwSize;
}




//-----------------------------------------------------------------------------
// Name: idWavefile::ResetFile()
// Desc: Resets the internal m_ck pointer so reading starts from the
//       beginning of the file again
//-----------------------------------------------------------------------------
int idWavefile::ResetFile() {
	if ( m_bIsReadingFromMemory ) {
		m_pbDataCur = m_pbData;
	} else
	{
		if ( m_hmmio == -1 ) {
			return CO_E_NOTINITIALIZED;
		}

		if ( m_dwFlags == WAVEFILE_READ ) {
			// Seek to the data
			if ( -1 == FS_Seek( m_hmmio, m_ckRiff.dwDataOffset + sizeof( FOURCC ),
								FS_SEEK_SET ) ) {
				Com_Error( ERR_DROP, TEXT( "mmioSeek" ), E_FAIL );
			}

			// Search the input file for for the 'fmt ' chunk.
			do {
				if ( 4 != FS_Read( &m_ck, 4, m_hmmio ) ) {
					Com_Error( ERR_DROP, TEXT( "mmioDescend" ), E_FAIL );
				}
			} while ( m_ck.ckid != mmioFOURCC( 'd', 'a', 't', 'a' ) );
			FS_Read( &m_ck.cksize, 4, m_hmmio );
		}
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
// Name: idWavefile::Read()
// Desc: Reads section of data from a wave file into pBuffer and returns
//       how much read in pdwSizeRead, reading not more than dwSizeToRead.
//       This uses m_ck to determine where to start reading from.  So
//       subsequent calls will be continue where the last left off unless
//       Reset() is called.
//-----------------------------------------------------------------------------
int idWavefile::Read( byte* pBuffer, dword dwSizeToRead, dword* pdwSizeRead ) {
	if ( m_bIsReadingFromMemory ) {
		if ( m_pbDataCur == NULL ) {
			return CO_E_NOTINITIALIZED;
		}
		if ( pdwSizeRead != NULL ) {
			*pdwSizeRead = 0;
		}

		if ( ( byte* )( m_pbDataCur + dwSizeToRead ) >
			 ( byte* )( m_pbData + m_ulDataSize ) ) {
			dwSizeToRead = m_ulDataSize - (dword)( m_pbDataCur - m_pbData );
		}

		CopyMemory( pBuffer, m_pbDataCur, dwSizeToRead );

		if ( pdwSizeRead != NULL ) {
			*pdwSizeRead = dwSizeToRead;
		}

		return S_OK;
	} else
	{
		if ( m_hmmio == -1 ) {
			return CO_E_NOTINITIALIZED;
		}
		if ( pBuffer == NULL || pdwSizeRead == NULL ) {
			return E_INVALIDARG;
		}

		if ( pdwSizeRead != NULL ) {
			*pdwSizeRead = 0;
		}

		int cbDataIn = dwSizeToRead;
		if ( cbDataIn > m_ck.cksize ) {
			cbDataIn = m_ck.cksize;
		}
		m_ck.cksize -= cbDataIn;
		*pdwSizeRead = FS_Read( pBuffer, cbDataIn, m_hmmio );
		return S_OK;
	}
}

//-----------------------------------------------------------------------------
// Name: idWavefile::Close()
// Desc: Closes the wave file
//-----------------------------------------------------------------------------
int idWavefile::Close() {
	if ( m_dwFlags == WAVEFILE_READ && m_hmmio != -1 ) {
		FS_FCloseFile( m_hmmio );
		m_hmmio = -1;
	}
	return S_OK;
}



