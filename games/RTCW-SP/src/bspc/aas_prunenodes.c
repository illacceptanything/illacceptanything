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
// Name:				aas_prunenodes.c
// Function:		Prune Nodes
// Programmer:		Mr Elusive (MrElusive@demigod.demon.nl)
// Last update:	1997-12-04
// Tab Size:		3
//===========================================================================

#include "qbsp.h"
#include "../botlib/aasfile.h"
#include "aas_create.h"

int c_numprunes;

//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
tmp_node_t *AAS_PruneNodes_r( tmp_node_t *tmpnode ) {
	tmp_area_t *tmparea1, *tmparea2;

	//if it is a solid leaf
	if ( !tmpnode ) {
		return NULL;
	}
	//
	if ( tmpnode->tmparea ) {
		return tmpnode;
	}
	//process the children first
	tmpnode->children[0] = AAS_PruneNodes_r( tmpnode->children[0] );
	tmpnode->children[1] = AAS_PruneNodes_r( tmpnode->children[1] );
	//if both children are areas
	if ( tmpnode->children[0] && tmpnode->children[1] &&
		 tmpnode->children[0]->tmparea && tmpnode->children[1]->tmparea ) {
		tmparea1 = tmpnode->children[0]->tmparea;
		while ( tmparea1->mergedarea ) tmparea1 = tmparea1->mergedarea;

		tmparea2 = tmpnode->children[1]->tmparea;
		while ( tmparea2->mergedarea ) tmparea2 = tmparea2->mergedarea;

		if ( tmparea1 == tmparea2 ) {
			c_numprunes++;
			tmpnode->tmparea = tmparea1;
			tmpnode->planenum = 0;
			AAS_FreeTmpNode( tmpnode->children[0] );
			AAS_FreeTmpNode( tmpnode->children[1] );
			tmpnode->children[0] = NULL;
			tmpnode->children[1] = NULL;
			return tmpnode;
		} //end if
	} //end if
	  //if both solid leafs
	if ( !tmpnode->children[0] && !tmpnode->children[1] ) {
		c_numprunes++;
		AAS_FreeTmpNode( tmpnode );
		return NULL;
	} //end if
	  //
	return tmpnode;
} //end of the function AAS_PruneNodes_r
//===========================================================================
//
// Parameter:				-
// Returns:					-
// Changes Globals:		-
//===========================================================================
void AAS_PruneNodes( void ) {
	Log_Write( "AAS_PruneNodes\r\n" );
	AAS_PruneNodes_r( tmpaasworld.nodes );
	Log_Print( "%6d nodes pruned\r\n", c_numprunes );
} //end of the function AAS_PruneNodes
