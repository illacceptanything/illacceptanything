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

//===========================================================================
//
// Name:			_files.c
// Function:
// Programmer:		Mr Elusive
// Last update:		1999-12-02
// Tab Size:		4
//===========================================================================

/*

aas_areamerging.c			//AAS area merging
aas_cfg.c					//AAS configuration for different games
aas_create.c				//AAS creating
aas_edgemelting.c			//AAS edge melting
aas_facemerging.c			//AAS face merging
aas_file.c					//AAS file writing
aas_gsubdiv.c				//AAS gravitational and ladder subdivision
aas_map.c					//AAS map brush creation
aas_prunenodes.c			//AAS node pruning
aas_store.c					//AAS file storing

map.c						//map file loading and writing
map_hl.c					//Half-Life map loading
map_q1.c					//Quake1 map loading
map_q2.c					//Quake2 map loading
map_q3.c					//Quake3 map loading
map_sin.c					//Sin map loading
tree.c						//BSP tree management + node pruning			(*)
brushbsp.c					//brush bsp creation							(*)
portals.c					//BSP portal creation and leaf filling			(*)
csg.c						//Constructive Solid Geometry brush chopping	(*)
leakfile.c					//leak file writing								(*)
textures.c					//Quake2 BSP textures							(*)

l_bsp_ent.c					//BSP entity parsing
l_bsp_hl.c					//Half-Life BSP loading and writing
l_bsp_q1.c					//Quake1 BSP loading and writing
l_bsp_q2.c					//Quake2 BSP loading and writing
l_bsp_q3.c					//Quake2 BSP loading and writing
l_bsp_sin.c					//Sin BSP loading and writing
l_cmd.c						//cmd library
l_log.c						//log file library
l_math.c					//math library
l_mem.c						//memory management library
l_poly.c					//polygon (winding) library
l_script.c					//script file parsing library
l_threads.c					//multi-threading library
l_utils.c					//utility library
l_qfiles.c					//loading of quake files

gldraw.c					//GL drawing									(*)
glfile.c					//GL file writing								(*)
nodraw.c					//no draw module								(*)

bspc.c						//BSPC Win32 console version
winbspc.c					//WinBSPC Win32 GUI version
win32_terminal.c			//Win32 terminal output
win32_qfiles.c				//Win32 game file management (also .pak .sin)
win32_font.c				//Win32 fonts
win32_folder.c				//Win32 folder dialogs

*/
