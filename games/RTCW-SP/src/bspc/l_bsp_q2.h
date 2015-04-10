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

#ifndef ME
#define ME
#endif //ME

extern int nummodels;
extern dmodel_t            *dmodels; //[MAX_MAP_MODELS];

extern int visdatasize;
extern byte                *dvisdata; //[MAX_MAP_VISIBILITY];
extern dvis_t          *dvis;

extern int lightdatasize;
extern byte                *dlightdata; //[MAX_MAP_LIGHTING];

extern int entdatasize;
extern char                *dentdata; //[MAX_MAP_ENTSTRING];

extern int numleafs;
extern dleaf_t         *dleafs; //[MAX_MAP_LEAFS];

extern int numplanes;
extern dplane_t            *dplanes; //[MAX_MAP_PLANES];

extern int numvertexes;
extern dvertex_t       *dvertexes; //[MAX_MAP_VERTS];

extern int numnodes;
extern dnode_t         *dnodes; //[MAX_MAP_NODES];

extern int numtexinfo;
extern texinfo_t texinfo[MAX_MAP_TEXINFO];

extern int numfaces;
extern dface_t         *dfaces; //[MAX_MAP_FACES];

extern int numedges;
extern dedge_t         *dedges; //[MAX_MAP_EDGES];

extern int numleaffaces;
extern unsigned short  *dleaffaces; //[MAX_MAP_LEAFFACES];

extern int numleafbrushes;
extern unsigned short  *dleafbrushes; //[MAX_MAP_LEAFBRUSHES];

extern int numsurfedges;
extern int             *dsurfedges; //[MAX_MAP_SURFEDGES];

extern int numareas;
extern darea_t         *dareas; //[MAX_MAP_AREAS];

extern int numareaportals;
extern dareaportal_t   *dareaportals; //[MAX_MAP_AREAPORTALS];

extern int numbrushes;
extern dbrush_t            *dbrushes; //[MAX_MAP_BRUSHES];

extern int numbrushsides;
extern dbrushside_t    *dbrushsides; //[MAX_MAP_BRUSHSIDES];

extern byte dpop[256];

extern char brushsidetextured[MAX_MAP_BRUSHSIDES];

void Q2_AllocMaxBSP( void );
void Q2_FreeMaxBSP( void );

void Q2_DecompressVis( byte *in, byte *decompressed );
int Q2_CompressVis( byte *vis, byte *dest );

void Q2_LoadBSPFile( char *filename, int offset, int length );
void Q2_LoadBSPFileTexinfo( char *filename ); // just for qdata
void Q2_WriteBSPFile( char *filename );
void Q2_PrintBSPFileSizes( void );
void Q2_ParseEntities( void );
void Q2_UnparseEntities( void );

