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


#include "qbsp.h"

int c_glfaces;

int PortalVisibleSides( portal_t *p ) {
	int fcon, bcon;

	if ( !p->onnode ) {
		return 0;       // outside

	}
	fcon = p->nodes[0]->contents;
	bcon = p->nodes[1]->contents;

	// same contents never create a face
	if ( fcon == bcon ) {
		return 0;
	}

	// FIXME: is this correct now?
	if ( !fcon ) {
		return 1;
	}
	if ( !bcon ) {
		return 2;
	}
	return 0;
}

void OutputWinding( winding_t *w, FILE *glview ) {
	static int level = 128;
	vec_t light;
	int i;

	fprintf( glview, "%i\n", w->numpoints );
	level += 28;
	light = ( level & 255 ) / 255.0;
	for ( i = 0 ; i < w->numpoints ; i++ )
	{
		fprintf( glview, "%6.3f %6.3f %6.3f %6.3f %6.3f %6.3f\n",
				 w->p[i][0],
				 w->p[i][1],
				 w->p[i][2],
				 light,
				 light,
				 light );
	}
	fprintf( glview, "\n" );
}

/*
=============
OutputPortal
=============
*/
void OutputPortal( portal_t *p, FILE *glview ) {
	winding_t   *w;
	int sides;

	sides = PortalVisibleSides( p );
	if ( !sides ) {
		return;
	}

	c_glfaces++;

	w = p->winding;

	if ( sides == 2 ) {   // back side
		w = ReverseWinding( w );
	}

	OutputWinding( w, glview );

	if ( sides == 2 ) {
		FreeWinding( w );
	}
}

/*
=============
WriteGLView_r
=============
*/
void WriteGLView_r( node_t *node, FILE *glview ) {
	portal_t    *p, *nextp;

	if ( node->planenum != PLANENUM_LEAF ) {
		WriteGLView_r( node->children[0], glview );
		WriteGLView_r( node->children[1], glview );
		return;
	}

	// write all the portals
	for ( p = node->portals ; p ; p = nextp )
	{
		if ( p->nodes[0] == node ) {
			OutputPortal( p, glview );
			nextp = p->next[0];
		} else {
			nextp = p->next[1];
		}
	}
}

/*
=============
WriteGLView
=============
*/
void WriteGLView( tree_t *tree, char *source ) {
	char name[1024];
	FILE    *glview;

	c_glfaces = 0;
	sprintf( name, "%s%s.gl",outbase, source );
	printf( "Writing %s\n", name );

	glview = fopen( name, "w" );
	if ( !glview ) {
		Error( "Couldn't open %s", name );
	}
	WriteGLView_r( tree->headnode, glview );
	fclose( glview );

	printf( "%5i c_glfaces\n", c_glfaces );
}

