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
 * This is the hardware abstraction layer.  Right now, it's not even
 * a little abstract, because I've got MS code in here for dealing with
 * DirectSound and .wav files, but it will be very very soon
 *
 */

#ifndef _idAudioHardware_h_
#define _idAudioHardware_h_

//#define DIRECTSOUND_VERSION 0x0700

// we will soon not be including these at all
#include "../mssdk/include/dsound.h"
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
// promise

#define MAX_CHANNELS    32

/* general extended waveform format structure
   Use this for all NON PCM formats
   (information common to all formats)
*/

typedef struct {
	word wFormatTag;           /* format type */
	word nChannels;            /* number of channels (i.e. mono, stereo...) */
	dword nSamplesPerSec;      /* sample rate */
	dword nAvgBytesPerSec;     /* for buffer estimation */
	word nBlockAlign;          /* block size of data */
	word wBitsPerSample;       /* Number of bits per sample of mono data */
	word cbSize;               /* The count in bytes of the size of
									extra information (after cbSize) */
} waveformatex;

/* RIFF chunk information data structure */
typedef struct {
	FOURCC ckid;                    /* chunk ID */
	dword cksize;                   /* chunk size */
	FOURCC fccType;                 /* form type or list type */
	dword dwDataOffset;             /* offset of data portion of chunk */
} mminfo;

//-----------------------------------------------------------------------------
// Classes used by this header
//-----------------------------------------------------------------------------
class idAudioHardware;
class idAudioBuffer;
class idStreamingBuffer;
class idWavefile;

//-----------------------------------------------------------------------------
// Typing macros
//-----------------------------------------------------------------------------
#define WAVEFILE_READ   1
#define WAVEFILE_WRITE  2

//-----------------------------------------------------------------------------
// Miscellaneous helper functions
//-----------------------------------------------------------------------------
#define SAFE_DELETE( p )       { if ( p ) { delete ( p );     ( p ) = NULL; } }
#define SAFE_DELETE_ARRAY( p ) { if ( p ) { delete[] ( p );   ( p ) = NULL; } }
#define SAFE_RELEASE( p )      { if ( p ) { ( p )->Release(); ( p ) = NULL; } }


//-----------------------------------------------------------------------------
// Name: class idAudioHardware
// Desc:
//-----------------------------------------------------------------------------
class idAudioHardware {
protected:
LPDIRECTSOUND8 m_pDS;
bool eax;
LPDIRECTSOUNDBUFFER pDSBPrimary;
LPKSPROPERTYSET pEAXListener;
public:
idAudioHardware();
~idAudioHardware();

int Initialize( dword dwCoopLevel, dword dwPrimaryChannels, dword dwPrimaryFreq, dword dwPrimaryBitRate );
inline LPDIRECTSOUND GetDirectSound() { return m_pDS; }
int SetPrimaryBufferFormat( dword dwPrimaryChannels, dword dwPrimaryFreq, dword dwPrimaryBitRate );
int Get3DListenerInterface( LPDIRECTSOUND3DLISTENER* ppDSListener );

bool    GetEAX( LPDIRECTSOUND3DBUFFER genBuf );

int Create( idAudioBuffer** ppSound, const char* strWaveFileName, dword dwCreationFlags = 0, GUID guid3DAlgorithm = GUID_NULL, dword dwNumBuffers = 1, dword dwMaxNumBuffers = MAX_CHANNELS );
int CreateFromMemory( idAudioBuffer** ppSound, byte* pbData, ulong ulDataSize, waveformatex *pwfx, dword dwCreationFlags = 0, GUID guid3DAlgorithm = GUID_NULL, dword dwNumBuffers = 1 );
int CreateStreaming( idStreamingBuffer** ppStreamingSound, const char* strIntroWaveFileName, const char* strWaveFileName, dword dwCreationFlags, GUID guid3DAlgorithm, bool useNotification, dword dwNotifyCount, dword dwNotifySize, HANDLE hNotifyEvent );
};




//-----------------------------------------------------------------------------
// Name: class idAudioBuffer
// Desc: Encapsulates functionality of a DirectSound buffer.
//-----------------------------------------------------------------------------
class idAudioBuffer {
protected:
LPDIRECTSOUNDBUFFER*    m_apDSBuffer;
LPDIRECTSOUND3DBUFFER*  m_pDS3DBuffer;                      // 3D sound buffer

dword m_dwDSBufferSize;
idWavefile*          m_pWaveFile;
idWavefile*          m_qWaveFile;
dword m_dwNumBuffers;
dword m_dwMaxNumBuffers;

int RestoreBuffer( LPDIRECTSOUNDBUFFER pDSB, bool* pbWasRestored );

public:
idAudioBuffer( LPDIRECTSOUNDBUFFER* apDSBuffer, dword dwDSBufferSize, dword dwNumBuffers, dword dwMaxNumBuffers, idWavefile* pWaveFile );
virtual ~idAudioBuffer();

void Get3DBufferInterfaces( float minDistance = 1.0f, float maxDistance = 400.0f );
int FillBufferWithSound( LPDIRECTSOUNDBUFFER pDSB, bool bRepeatWavIfBufferLarger );

void SetRange( int index, float min, float max );
void SetPosition( int index, float x, float y, float z );
void SetVelocity( int index, float x, float y, float z );
void SetVolume( int index, float volume );

LPDIRECTSOUNDBUFFER GetFreeBuffer();
LPDIRECTSOUNDBUFFER GetBuffer( dword dwIndex );
LPDIRECTSOUND3DBUFFER Get3DBuffer( dword dwIndex );

int     Play( dword index, dword dwPriority, dword dwFlags );
int     Pause( dword index );
int     Stop( dword index );
int     Reset();
bool    IsSoundPlaying();
bool    AllChannelsPlaying();
int     GetFreeIndex();
void    Make2D( dword index );
int     GetNumBuffers() { return m_dwNumBuffers; }
idStr               *name;

int entnum[MAX_CHANNELS];
int entchannel[MAX_CHANNELS];
int flags[MAX_CHANNELS];
};




//-----------------------------------------------------------------------------
// Name: class idStreamingBuffer
// Desc: Encapsulates functionality to play a wave file with DirectSound.
//       The Create() method loads a chunk of wave file into the buffer,
//       and as sound plays more is written to the buffer by calling
//       HandleWaveStreamNotification() whenever hNotifyEvent is signaled.
//-----------------------------------------------------------------------------
class idStreamingBuffer : public idAudioBuffer {
protected:
dword m_dwLastPlayPos;
dword m_dwPlayProgress;
dword m_dwNotifySize;
dword m_dwNextWriteOffset;
dword m_dwNextEvent;
bool m_bFillNextNotificationWithSilence;
bool m_bUsingNotification;

idWavefile*          m_qWaveFile;

public:

idStreamingBuffer( LPDIRECTSOUNDBUFFER pDSBuffer,
				   dword dwDSBufferSize,
				   idWavefile* pWaveFile,
				   idWavefile* oWaveFile, bool useNotification, dword dwNotifySize );
~idStreamingBuffer();

int HandleWaveStreamNotification();
int Reset();
bool    UsingNotification() { return m_bUsingNotification; }

int attenuation;
bool looping;
bool in3D;
};


//-----------------------------------------------------------------------------
// Name: class idWavefile
// Desc: Encapsulates reading or writing sound data to or from a wave file
//-----------------------------------------------------------------------------
class idWavefile {
public:
waveformatex*   m_pwfx;            // Pointer to waveformatex structure
fileHandle_t m_hmmio;              // MM I/O handle for the WAVE
mminfo m_ck;                       // Multimedia RIFF chunk
mminfo m_ckRiff;                   // Use in opening a WAVE file
dword m_dwSize;                    // The size of the wave file
dword m_dwFlags;
bool m_bIsReadingFromMemory;
byte*           m_pbData;
byte*           m_pbDataCur;
ulong m_ulDataSize;

protected:
int ReadMMIO();

public:
idWavefile();
~idWavefile();

int Open( const char* strFileName, waveformatex* pwfx, dword dwFlags );
int OpenFromMemory( byte* pbData, ulong ulDataSize, waveformatex* pwfx, dword dwFlags );
int Close();

int Read( byte* pBuffer, dword dwSizeToRead, dword* pdwSizeRead );

dword   GetSize();
int     ResetFile();
waveformatex* GetFormat() { return m_pwfx; };
};

#endif // _idAudioHardware_h_
