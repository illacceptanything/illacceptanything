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


/*****************************************************************************
 * name:		l_memory.h
 *
 * desc:		memory management
 *
 *
 *****************************************************************************/

#ifdef _DEBUG
#ifndef BSPC
	#define MEMDEBUG
#endif
#endif

#ifdef MEMDEBUG
#define GetMemory( size )             GetMemoryDebug( size, # size, __FILE__, __LINE__ );
#define GetClearedMemory( size )      GetClearedMemoryDebug( size, # size, __FILE__, __LINE__ );
//allocate a memory block of the given size
void *GetMemoryDebug( unsigned long size, char *label, char *file, int line );
//allocate a memory block of the given size and clear it
void *GetClearedMemoryDebug( unsigned long size, char *label, char *file, int line );
//
#define GetHunkMemory( size )         GetHunkMemoryDebug( size, # size, __FILE__, __LINE__ );
#define GetClearedHunkMemory( size )  GetClearedHunkMemoryDebug( size, # size, __FILE__, __LINE__ );
//allocate a memory block of the given size
void *GetHunkMemoryDebug( unsigned long size, char *label, char *file, int line );
//allocate a memory block of the given size and clear it
void *GetClearedHunkMemoryDebug( unsigned long size, char *label, char *file, int line );
#else
//allocate a memory block of the given size
void *GetMemory( unsigned long size );
//allocate a memory block of the given size and clear it
void *GetClearedMemory( unsigned long size );
//
#ifdef BSPC
#define GetHunkMemory GetMemory
#define GetClearedHunkMemory GetClearedMemory
#else
//allocate a memory block of the given size
void *GetHunkMemory( unsigned long size );
//allocate a memory block of the given size and clear it
void *GetClearedHunkMemory( unsigned long size );
#endif
#endif

//free the given memory block
void FreeMemory( void *ptr );
//prints the total used memory size
void PrintUsedMemorySize( void );
//print all memory blocks with label
void PrintMemoryLabels( void );
//returns the size of the memory block in bytes
int MemoryByteSize( void *ptr );
//free all allocated memory
void DumpMemory( void );
