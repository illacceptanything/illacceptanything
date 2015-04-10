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

#include "../splines/q_shared.h"
#include "../splines/util_str.h"

#pragma once

#define GENTITYNUM_BITS     11      // don't need to send any more
#define MAX_GENTITIES       ( 1 << GENTITYNUM_BITS )

#define Com_ClearedAllocate( x ) calloc( x,1 );
#define Com_Dealloc free
#define Com_Allocate malloc

typedef unsigned char byte;
typedef unsigned long dword;
typedef unsigned short word;
typedef unsigned int uint;
typedef unsigned long ulong;

#ifdef __cplusplus
extern "C" {
#endif

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];

#include "../client/snd_public.h"

int     FS_FOpenFileRead( const char *qpath, fileHandle_t *file, qboolean uniqueFILE );
// if uniqueFILE is true, then a new FILE will be fopened even if the file
// is found in an already open pak file.  If uniqueFILE is false, you must call
// FS_FCloseFile instead of fclose, otherwise the pak FILE would be improperly closed
// It is generally safe to always set uniqueFILE to true, because the majority of
// file IO goes through FS_ReadFile, which Does The Right Thing already.

int     FS_FileIsInPAK( const char *filename, int *pChecksum );
// returns 1 if a file is in the PAK file, otherwise -1

int     FS_Write( const void *buffer, int len, fileHandle_t f );

int     FS_Read( void *buffer, int len, fileHandle_t f );
// properly handles partial reads and reads from other dlls

void    FS_FCloseFile( fileHandle_t f );
// note: you can't just fclose from another DLL, due to MS libc issues

int     FS_ReadFile( const char *qpath, void **buffer );
// returns the length of the file
// a null buffer will just return the file length without loading
// as a quick check for existance. -1 length == not present
// A 0 byte will always be appended at the end, so string ops are safe.
// the buffer should be considered read-only, because it may be cached
// for other uses.

void    FS_ForceFlush( fileHandle_t f );
// forces flush on files we're writing to.

void    FS_FreeFile( void *buffer );
// frees the memory returned by FS_ReadFile

void    FS_WriteFile( const char *qpath, const void *buffer, int size );
// writes a complete file, creating any subdirectories needed

int     FS_filelength( fileHandle_t f );
// doesn't work for files that are opened from a pack file

int     FS_FTell( fileHandle_t f );
// where are we?

void    FS_Flush( fileHandle_t f );

void QDECL FS_Printf( fileHandle_t f, const char *fmt, ... );
// like fprintf

int     FS_FOpenFileByMode( const char *qpath, fileHandle_t *f, fsMode_t mode );
// opens a file for reading, writing, or appending depending on the value of mode

int     FS_Seek( fileHandle_t f, long offset, int origin );
// seek on a file (doesn't work for zip files!!!!!!!!)

int Com_Milliseconds( void );
void Com_Memset( void* dest, const int val, const size_t count );

// Sys_Milliseconds should only be used for profiling purposes,
// any game related timing information should come from event timestamps
int     Sys_Milliseconds( void );


// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
	char        *name;
	char        *string;
	char        *resetString;       // cvar_restart will reset to this value
	char        *latchedString;     // for CVAR_LATCH vars
	int flags;
	qboolean modified;              // set each time the cvar is changed
	int modificationCount;          // incremented each time the cvar is changed
	float value;                    // atof( string )
	int integer;                    // atoi( string )
	struct cvar_s *next;
	struct cvar_s *hashNext;
} cvar_t;

cvar_t *Cvar_Get( const char *var_name, const char *value, int flags );
// creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in
// that allows variables to be unarchived without needing bitflags
// if value is "", the value will not override a previously set value.

#define CVAR_ARCHIVE        1   // set to cause it to be saved to vars.rc
								// used for system variables, not for player
								// specific configurations

#ifdef __cplusplus
}
#endif


template <class IndexType>
class idFlags
{
public:
typedef unsigned int T;

idFlags         ( void ) {
	flag = 0;
}
idFlags         ( const char *debugnames );
idFlags         ( const T val ) {
	flag = val;
}
idFlags&        operator=( const idFlags& src )                { flag = (T)src.flag;   return *this;       }
idFlags&        operator=( const T& val )                      { flag = val;       return *this;           }
void            clearAll( void )                              { flag = 0;                                 }
void            clear( IndexType index )                   { flag &= ~( 1 << ( unsigned int )( index ) );      }
void            clearMask( const T& f )                        { flag &= ~f;                               }
void            set( IndexType index )                   { flag |= ( 1 << ( unsigned int )( index ) );       }
void            set( IndexType index, bool on )      { if ( on ) {
														   flag |= ( 1 << ( unsigned int )( index ) );
													   } else { flag &= ~( 1 << ( unsigned int )( index ) );} }
void            setAll( void )                              { flag = 0xffffffff;                        }
void            setMask( const T& mask )                     { flag |= mask;                             }
void            setVal( const T& f )                        { flag = f;                                 }
bool        testAny( void ) const { return flag != 0;                           }
bool        test( IndexType index ) const { return (bool)( ( flag & ( 1 << ( unsigned int )( index ) ) ) != 0 );  }
bool        testMask( const T& mask ) const { return flag & mask;                       }
bool        testMaskAll( const T& mask ) const { return ( flag & mask ) == mask;             }
void            toggle( IndexType index )                   { flag ^= ( 1 << ( unsigned int )( index ) );       }
operator T( void ) const { return flag;                              }
unsigned int    getNumBits( void ) const { return sizeof( T ) * 8; }

static const char *getDebugNames()                                  { return debugNames;    }
private:
static const char      *debugNames;                                     // list of flag names
T flag;                                                                 // flag data bitmask
};

template <class IndexType> idFlags<IndexType>::idFlags( const char *debugnames ) {
	clearAll();
	if ( !idFlags::debugNames ) {
		idFlags::debugNames = debugnames;
	}
}

template <class IndexType> const char * idFlags<IndexType>::debugNames = 0;

template< class Type >
class idHashTable {
private:
struct hashnode_s {
	idStr key;
	Type value;
	hashnode_s  *next;

	hashnode_s( const idStr &k, Type v, hashnode_s *n ) : key( k ), value( v ), next( n ) {
	};
	hashnode_s( const char *k, Type v, hashnode_s *n ) : key( k ), value( v ), next( n ) {
	};
};

hashnode_s      **m_heads;

unsigned m_tablesize;
unsigned m_numentries;
unsigned m_tablesizemask;

unsigned        GetHash( const char *key );

public:
void            Clear( void );
void            DeleteContents( void );
unsigned        Num( void ) const;

void            Set( const char *key, Type &value );
bool            Get( const char *key, Type **value = NULL );
bool            Remove( const char *key );

~idHashTable<Type>();

idHashTable( unsigned tablesize = 1024 ) : m_tablesize( tablesize ) {
	int i;
	int bits;

	assert( m_tablesize > 0 );

	m_heads = new hashnode_s *[ m_tablesize ];
	memset( m_heads, 0, sizeof( *m_heads ) * m_tablesize );

	m_numentries = 0;
	m_tablesizemask = 0;

	bits = 0;
	for ( i = 0; i < 32; i++ ) {
		if ( m_tablesize & ( 1 << i ) ) {
			bits++;
		}
	}

	if ( bits == 1 ) {
		m_tablesizemask = m_tablesize - 1;
	}
};

idHashTable( idHashTable<Type> &map ) : m_tablesize( map.m_tablesize ) {
	unsigned i;
	hashnode_s  *node;
	hashnode_s  **prev;

	assert( m_tablesize > 0 );

	m_heads         = new hashnode_s *[ m_tablesize ];
	m_numentries    = map.m_numentries;
	m_tablesizemask = map.m_tablesizemask;

	for ( i = 0; i < m_tablesize; i++ ) {
		if ( !map.m_heads[ i ] ) {
			m_heads[ i ] = NULL;
			continue;
		}

		prev = &m_heads[ i ];
		for ( node = map.m_heads[ i ]; node != NULL; node = node->next ) {
			*prev = new hashnode_s( node->key, node->value, NULL );
			prev = &( *prev )->next;
		}
	}
};
};

/*
================
idHashTable<Type>::~idHashTable<Type>()
================
*/
template< class Type >
inline idHashTable<Type>::~idHashTable() {
	Clear();
	delete[] m_heads;
}

/*
================
idHashTable<Type>::GetHash
================
*/
template< class Type >
inline unsigned idHashTable<Type>::GetHash( const char *key ) {
	unsigned h;
	const char  *v;

	if ( m_tablesizemask ) {
		h = 0;
		for ( v = key; *v != '\0'; v++ ) {
			h = ( 64 * h + unsigned( *v ) ) & m_tablesizemask;
		}
	} else {
		h = 0;
		for ( v = key; *v != '\0'; v++ ) {
			h = ( 64 * h + unsigned( *v ) ) % m_tablesize;
		}
	}

	return h;
}

/*
================
idHashTable<Type>::Set
================
*/
template< class Type >
inline void idHashTable<Type>::Set( const char *key, Type &value ) {
	hashnode_s  **head;
	hashnode_s  *node;
	unsigned hash;

	// FIXME
	// if list was always sorted, we could just insert new node as soon as we get
	// a node whose key is greater than the search key
	hash = GetHash( key );
	head = &m_heads[ hash ];
	if ( *head ) {
		for ( node = *head; node != NULL; node = node->next ) {
			if ( node->key == key ) {
				node->value = value;
				return;
			}
		}
	}

	m_numentries++;

	*head = new hashnode_s( key, value, *head );
}

/*
================
idHashTable<Type>::Get
================
*/
template< class Type >
inline bool idHashTable<Type>::Get( const char *key, Type **value ) {
	hashnode_s  **head;
	hashnode_s  *node;
	unsigned hash;

	// FIXME
	// if list was always sorted, we could just insert new node as soon as we get
	// a node whose key is greater than the search key
	hash = GetHash( key );
	head = &m_heads[ hash ];
	if ( *head ) {
		for ( node = *head; node != NULL; node = node->next ) {
			if ( node->key == key ) {
				if ( value ) {
					*value = &node->value;
				}
				return true;
			}
		}
	}

	if ( value ) {
		*value = NULL;
	}

	return false;
}

/*
================
idHashTable<Type>::Remove
================
*/
template< class Type >
inline bool idHashTable<Type>::Remove( const char *key ) {
	hashnode_s  **head;
	hashnode_s  *node;
	hashnode_s  *prev;
	unsigned hash;

	hash = GetHash( key );
	head = &m_heads[ hash ];
	if ( *head ) {
		for ( prev = NULL, node = *head; node != NULL; prev = node, node = node->next ) {
			if ( node->key == key ) {
				if ( prev ) {
					prev->next = node->next;
				} else {
					*head = node->next;
				}

				delete node;
				return true;
			}
		}
	}

	return false;
}

/*
================
idHashTable<Type>::Clear
================
*/
template< class Type >
inline void idHashTable<Type>::Clear( void ) {
	unsigned i;
	hashnode_s  *node;
	hashnode_s  *next;

	for ( i = 0; i < m_tablesize; i++ ) {
		next = m_heads[ i ];
		while ( next != NULL ) {
			node = next;
			next = next->next;
			delete node;
		}

		m_heads[ i ] = NULL;
	}

	m_numentries = 0;
}

/*
================
idHashTable<Type>::DeleteContents
================
*/
template< class Type >
inline void idHashTable<Type>::DeleteContents( void ) {
	unsigned i;
	hashnode_s  *node;
	hashnode_s  *next;

	for ( i = 0; i < m_tablesize; i++ ) {
		next = m_heads[ i ];
		while ( next != NULL ) {
			node = next;
			next = next->next;
			delete node->value;
			delete node;
		}

		m_heads[ i ] = NULL;
	}

	m_numentries = 0;
}



/*
================
idHashTable<Type>::Num
================
*/
template< class Type >
inline unsigned idHashTable<Type>::Num( void ) const {
	return m_numentries;
}

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{

	HINSTANCE reflib_library;           // Handle to refresh DLL
	qboolean reflib_active;

	HWND hWnd;
	HINSTANCE hInstance;
	qboolean activeApp;
	qboolean isMinimized;
	OSVERSIONINFO osversion;

	// when we get a windows message, we store the time off so keyboard processing
	// can know the exact time of an event
	unsigned sysMsgTime;
} WinVars_t;

extern WinVars_t g_wv;

//#define	SND_NORMAL			0x000	// (default) Allow sound to be cut off only by the same sound on this channel
#define     SND_OKTOCUT         0x001   // Allow sound to be cut off by any following sounds on this channel
#define     SND_REQUESTCUT      0x002   // Allow sound to be cut off by following sounds on this channel only for sounds who request cutoff
#define     SND_CUTOFF          0x004   // Cut off sounds on this channel that are marked 'SND_REQUESTCUT'
#define     SND_CUTOFF_ALL      0x008   // Cut off all sounds on this channel
#define     SND_NOCUT           0x010   // Don't cut off.  Always let finish (overridden by SND_CUTOFF_ALL)

#ifdef __cplusplus
}
#endif


