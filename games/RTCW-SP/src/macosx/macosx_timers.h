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

#ifdef OMNI_TIMER

#import <OmniTimer/OmniTimer.h>

#define OTSTART( node ) OTStackPush( node )
#define OTSTOP( node )  OTStackPop()

extern OTStackNode *rootNode;
extern OTStackNode *markFragmentsNode1;
extern OTStackNode *markFragmentsNode2;
extern OTStackNode *markFragmentsGrid;
extern OTStackNode *markFragmentsNode4;
extern OTStackNode *addMarkFragmentsNode;
extern OTStackNode *chopPolyNode;
extern OTStackNode *boxTraceNode;
extern OTStackNode *boxOnPlaneSideNode;
extern OTStackNode *recursiveWorldNode;
extern OTStackNode *surfaceAnimNode;
extern OTStackNode *surfaceFaceNode;
extern OTStackNode *surfaceMeshNode;
extern OTStackNode *surfaceEndNode;
extern OTStackNode *shadowEndNode;
extern OTStackNode *stageIteratorGenericNode;
extern OTStackNode *computeColorAndTexNode;
extern OTStackNode *mp3DecodeNode;

extern void InitializeTimers();

#else

#define OTSTART( node )
#define OTSTOP( node )

#endif

