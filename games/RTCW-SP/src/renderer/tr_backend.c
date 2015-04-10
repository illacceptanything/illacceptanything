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

#include "tr_local.h"

backEndData_t   *backEndData[SMP_FRAMES];
backEndState_t backEnd;


static float s_flipMatrix[16] = {
	// convert from our coordinate system (looking down X)
	// to OpenGL's coordinate system (looking down -Z)
	0, 0, -1, 0,
	-1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 0, 1
};


/*
** GL_Bind
*/
void GL_Bind( image_t *image ) {
	int texnum;

	if ( !image ) {
		ri.Printf( PRINT_WARNING, "GL_Bind: NULL image\n" );
		texnum = tr.defaultImage->texnum;
	} else {
		texnum = image->texnum;
	}

	if ( r_nobind->integer && tr.dlightImage ) {        // performance evaluation option
		texnum = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[glState.currenttmu] != texnum ) {
		image->frameUsed = tr.frameCount;
		glState.currenttextures[glState.currenttmu] = texnum;
		qglBindTexture( GL_TEXTURE_2D, texnum );
	}
}

/*
** GL_SelectTexture
*/
void GL_SelectTexture( int unit ) {
	if ( glState.currenttmu == unit ) {
		return;
	}

	if ( unit == 0 ) {
		qglActiveTextureARB( GL_TEXTURE0_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE0_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE0_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE0_ARB )\n" );
	} else if ( unit == 1 )   {
		qglActiveTextureARB( GL_TEXTURE1_ARB );
		GLimp_LogComment( "glActiveTextureARB( GL_TEXTURE1_ARB )\n" );
		qglClientActiveTextureARB( GL_TEXTURE1_ARB );
		GLimp_LogComment( "glClientActiveTextureARB( GL_TEXTURE1_ARB )\n" );
	} else {
		ri.Error( ERR_DROP, "GL_SelectTexture: unit = %i", unit );
	}

	glState.currenttmu = unit;
}


/*
** GL_BindMultitexture
*/
void GL_BindMultitexture( image_t *image0, GLuint env0, image_t *image1, GLuint env1 ) {
	int texnum0, texnum1;

	texnum0 = image0->texnum;
	texnum1 = image1->texnum;

	if ( r_nobind->integer && tr.dlightImage ) {        // performance evaluation option
		texnum0 = texnum1 = tr.dlightImage->texnum;
	}

	if ( glState.currenttextures[1] != texnum1 ) {
		GL_SelectTexture( 1 );
		image1->frameUsed = tr.frameCount;
		glState.currenttextures[1] = texnum1;
		qglBindTexture( GL_TEXTURE_2D, texnum1 );
	}
	if ( glState.currenttextures[0] != texnum0 ) {
		GL_SelectTexture( 0 );
		image0->frameUsed = tr.frameCount;
		glState.currenttextures[0] = texnum0;
		qglBindTexture( GL_TEXTURE_2D, texnum0 );
	}
}


/*
** GL_Cull
*/
void GL_Cull( int cullType ) {
	if ( glState.faceCulling == cullType ) {
		return;
	}

	glState.faceCulling = cullType;

	if ( cullType == CT_TWO_SIDED ) {
		qglDisable( GL_CULL_FACE );
	} else
	{
		qglEnable( GL_CULL_FACE );

		if ( cullType == CT_BACK_SIDED ) {
			if ( backEnd.viewParms.isMirror ) {
				qglCullFace( GL_FRONT );
			} else
			{
				qglCullFace( GL_BACK );
			}
		} else
		{
			if ( backEnd.viewParms.isMirror ) {
				qglCullFace( GL_BACK );
			} else
			{
				qglCullFace( GL_FRONT );
			}
		}
	}
}

/*
** GL_TexEnv
*/
void GL_TexEnv( int env ) {
	if ( env == glState.texEnv[glState.currenttmu] ) {
		return;
	}

	glState.texEnv[glState.currenttmu] = env;


	switch ( env )
	{
	case GL_MODULATE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		break;
	case GL_REPLACE:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );
		break;
	case GL_DECAL:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );
		break;
	case GL_ADD:
		qglTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );
		break;
	default:
		ri.Error( ERR_DROP, "GL_TexEnv: invalid env '%d' passed\n", env );
		break;
	}
}

/*
** GL_State
**
** This routine is responsible for setting the most commonly changed state
** in Q3.
*/
void GL_State( unsigned long stateBits ) {
	unsigned long diff = stateBits ^ glState.glStateBits;

	if ( !diff ) {
		return;
	}

	//
	// check depthFunc bits
	//
	if ( diff & GLS_DEPTHFUNC_EQUAL ) {
		if ( stateBits & GLS_DEPTHFUNC_EQUAL ) {
			qglDepthFunc( GL_EQUAL );
		} else
		{
			qglDepthFunc( GL_LEQUAL );
		}
	}

	//
	// check blend bits
	//
	if ( diff & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
		GLenum srcFactor, dstFactor;

		if ( stateBits & ( GLS_SRCBLEND_BITS | GLS_DSTBLEND_BITS ) ) {
			switch ( stateBits & GLS_SRCBLEND_BITS )
			{
			case GLS_SRCBLEND_ZERO:
				srcFactor = GL_ZERO;
				break;
			case GLS_SRCBLEND_ONE:
				srcFactor = GL_ONE;
				break;
			case GLS_SRCBLEND_DST_COLOR:
				srcFactor = GL_DST_COLOR;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_COLOR:
				srcFactor = GL_ONE_MINUS_DST_COLOR;
				break;
			case GLS_SRCBLEND_SRC_ALPHA:
				srcFactor = GL_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_SRC_ALPHA:
				srcFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_SRCBLEND_DST_ALPHA:
				srcFactor = GL_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ONE_MINUS_DST_ALPHA:
				srcFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			case GLS_SRCBLEND_ALPHA_SATURATE:
				srcFactor = GL_SRC_ALPHA_SATURATE;
				break;
			default:
				srcFactor = GL_ONE;     // to get warning to shut up
				ri.Error( ERR_DROP, "GL_State: invalid src blend state bits\n" );
				break;
			}

			switch ( stateBits & GLS_DSTBLEND_BITS )
			{
			case GLS_DSTBLEND_ZERO:
				dstFactor = GL_ZERO;
				break;
			case GLS_DSTBLEND_ONE:
				dstFactor = GL_ONE;
				break;
			case GLS_DSTBLEND_SRC_COLOR:
				dstFactor = GL_SRC_COLOR;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_COLOR:
				dstFactor = GL_ONE_MINUS_SRC_COLOR;
				break;
			case GLS_DSTBLEND_SRC_ALPHA:
				dstFactor = GL_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA:
				dstFactor = GL_ONE_MINUS_SRC_ALPHA;
				break;
			case GLS_DSTBLEND_DST_ALPHA:
				dstFactor = GL_DST_ALPHA;
				break;
			case GLS_DSTBLEND_ONE_MINUS_DST_ALPHA:
				dstFactor = GL_ONE_MINUS_DST_ALPHA;
				break;
			default:
				dstFactor = GL_ONE;     // to get warning to shut up
				ri.Error( ERR_DROP, "GL_State: invalid dst blend state bits\n" );
				break;
			}

			qglEnable( GL_BLEND );
			qglBlendFunc( srcFactor, dstFactor );
		} else
		{
			qglDisable( GL_BLEND );
		}
	}

	//
	// check depthmask
	//
	if ( diff & GLS_DEPTHMASK_TRUE ) {
		if ( stateBits & GLS_DEPTHMASK_TRUE ) {
			qglDepthMask( GL_TRUE );
		} else
		{
			qglDepthMask( GL_FALSE );
		}
	}

	//
	// fill/line mode
	//
	if ( diff & GLS_POLYMODE_LINE ) {
		if ( stateBits & GLS_POLYMODE_LINE ) {
			qglPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		} else
		{
			qglPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	//
	// depthtest
	//
	if ( diff & GLS_DEPTHTEST_DISABLE ) {
		if ( stateBits & GLS_DEPTHTEST_DISABLE ) {
			qglDisable( GL_DEPTH_TEST );
		} else
		{
			qglEnable( GL_DEPTH_TEST );
		}
	}

	//
	// alpha test
	//
	if ( diff & GLS_ATEST_BITS ) {
		switch ( stateBits & GLS_ATEST_BITS )
		{
		case 0:
			qglDisable( GL_ALPHA_TEST );
			break;
		case GLS_ATEST_GT_0:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GREATER, 0.0f );
			break;
		case GLS_ATEST_LT_80:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_LESS, 0.5f );
			break;
		case GLS_ATEST_GE_80:
			qglEnable( GL_ALPHA_TEST );
			qglAlphaFunc( GL_GEQUAL, 0.5f );
			break;
		default:
			assert( 0 );
			break;
		}
	}

	glState.glStateBits = stateBits;
}



/*
================
RB_Hyperspace

A player has predicted a teleport, but hasn't arrived yet
================
*/
static void RB_Hyperspace( void ) {
	float c;

	if ( !backEnd.isHyperspace ) {
		// do initialization shit
	}

	c = ( backEnd.refdef.time & 255 ) / 255.0f;
	qglClearColor( c, c, c, 1 );
	qglClear( GL_COLOR_BUFFER_BIT );

	backEnd.isHyperspace = qtrue;
}


static void SetViewportAndScissor( void ) {
	qglMatrixMode( GL_PROJECTION );
	qglLoadMatrixf( backEnd.viewParms.projectionMatrix );
	qglMatrixMode( GL_MODELVIEW );

	// set the window clipping
	qglViewport(    backEnd.viewParms.viewportX,
					backEnd.viewParms.viewportY,
					backEnd.viewParms.viewportWidth,
					backEnd.viewParms.viewportHeight );

// TODO: insert handling for widescreen?  (when looking through camera)
	qglScissor(     backEnd.viewParms.viewportX,
					backEnd.viewParms.viewportY,
					backEnd.viewParms.viewportWidth,
					backEnd.viewParms.viewportHeight );
}

/*
=================
RB_BeginDrawingView

Any mirrored or portaled views have already been drawn, so prepare
to actually render the visible surfaces for this view
=================
*/
void RB_BeginDrawingView( void ) {
	int clearBits = 0;

	// sync with gl if needed
	if ( r_finish->integer == 1 && !glState.finishCalled ) {
		qglFinish();
		glState.finishCalled = qtrue;
	}
	if ( r_finish->integer == 0 ) {
		glState.finishCalled = qtrue;
	}

	// we will need to change the projection matrix before drawing
	// 2D images again
	backEnd.projection2D = qfalse;

	//
	// set the modelview matrix for the viewer
	//
	SetViewportAndScissor();

	// ensures that depth writes are enabled for the depth clear
	GL_State( GLS_DEFAULT );


////////// (SA) modified to ensure one glclear() per frame at most

	// clear relevant buffers
	clearBits = 0;

	if ( r_measureOverdraw->integer || r_shadows->integer == 2 ) {
		clearBits |= GL_STENCIL_BUFFER_BIT;
	}

	if ( r_uiFullScreen->integer ) {
		clearBits = GL_DEPTH_BUFFER_BIT;    // (SA) always just clear depth for menus

	} else if ( skyboxportal ) {
		if ( backEnd.refdef.rdflags & RDF_SKYBOXPORTAL ) {   // portal scene, clear whatever is necessary
			clearBits |= GL_DEPTH_BUFFER_BIT;

			if ( r_fastsky->integer || backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {  // fastsky: clear color

				// try clearing first with the portal sky fog color, then the world fog color, then finally a default
				clearBits |= GL_COLOR_BUFFER_BIT;
				if ( glfogsettings[FOG_PORTALVIEW].registered ) {
					qglClearColor( glfogsettings[FOG_PORTALVIEW].color[0], glfogsettings[FOG_PORTALVIEW].color[1], glfogsettings[FOG_PORTALVIEW].color[2], glfogsettings[FOG_PORTALVIEW].color[3] );
				} else if ( glfogNum > FOG_NONE && glfogsettings[FOG_CURRENT].registered )      {
					qglClearColor( glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3] );
				} else {
//					qglClearColor ( 1.0, 0.0, 0.0, 1.0 );	// red clear for testing portal sky clear
					qglClearColor( 0.5, 0.5, 0.5, 1.0 );
				}
			} else {                                                    // rendered sky (either clear color or draw quake sky)
				if ( glfogsettings[FOG_PORTALVIEW].registered ) {
					qglClearColor( glfogsettings[FOG_PORTALVIEW].color[0], glfogsettings[FOG_PORTALVIEW].color[1], glfogsettings[FOG_PORTALVIEW].color[2], glfogsettings[FOG_PORTALVIEW].color[3] );

					if ( glfogsettings[FOG_PORTALVIEW].clearscreen ) {    // portal fog requests a screen clear (distance fog rather than quake sky)
						clearBits |= GL_COLOR_BUFFER_BIT;
					}
				}

			}
		} else {                                        // world scene with portal sky, don't clear any buffers, just set the fog color if there is one

			clearBits |= GL_DEPTH_BUFFER_BIT;   // this will go when I get the portal sky rendering way out in the zbuffer (or not writing to zbuffer at all)

			if ( glfogNum > FOG_NONE && glfogsettings[FOG_CURRENT].registered ) {
				if ( backEnd.refdef.rdflags & RDF_UNDERWATER ) {
					if ( glfogsettings[FOG_CURRENT].mode == GL_LINEAR ) {
						clearBits |= GL_COLOR_BUFFER_BIT;
					}

				} else if ( !( r_portalsky->integer ) ) {    // portal skies have been manually turned off, clear bg color
					clearBits |= GL_COLOR_BUFFER_BIT;
				}

				qglClearColor( glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3] );
			}
		}
	} else {                                              // world scene with no portal sky
		clearBits |= GL_DEPTH_BUFFER_BIT;

		// NERVE - SMF - we don't want to clear the buffer when no world model is specified
		if ( backEnd.refdef.rdflags & RDF_NOWORLDMODEL ) {
			clearBits &= ~GL_COLOR_BUFFER_BIT;
		}
		// -NERVE - SMF
		// (SA) well, this is silly then
		else if ( r_fastsky->integer ) {   //  || backEnd.refdef.rdflags & RDF_NOWORLDMODEL

			clearBits |= GL_COLOR_BUFFER_BIT;

			if ( glfogsettings[FOG_CURRENT].registered ) { // try to clear fastsky with current fog color
				qglClearColor( glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3] );
			} else {
//				qglClearColor ( 0.0, 0.0, 1.0, 1.0 );	// blue clear for testing world sky clear
				qglClearColor( 0.5, 0.5, 0.5, 1.0 );
			}
		} else {        // world scene, no portal sky, not fastsky, clear color if fog says to, otherwise, just set the clearcolor
			if ( glfogsettings[FOG_CURRENT].registered ) { // try to clear fastsky with current fog color
				qglClearColor( glfogsettings[FOG_CURRENT].color[0], glfogsettings[FOG_CURRENT].color[1], glfogsettings[FOG_CURRENT].color[2], glfogsettings[FOG_CURRENT].color[3] );

				if ( glfogsettings[FOG_CURRENT].clearscreen ) {   // world fog requests a screen clear (distance fog rather than quake sky)
					clearBits |= GL_COLOR_BUFFER_BIT;
				}
			}
		}
	}


	if ( clearBits ) {
		qglClear( clearBits );
	}

//----(SA)	done

	if ( ( backEnd.refdef.rdflags & RDF_HYPERSPACE ) ) {
		RB_Hyperspace();
		return;
	} else
	{
		backEnd.isHyperspace = qfalse;
	}

	glState.faceCulling = -1;       // force face culling to set next time

	// we will only draw a sun if there was sky rendered in this view
	backEnd.skyRenderedThisView = qfalse;

	// clip to the plane of the portal
	if ( backEnd.viewParms.isPortal ) {
		float plane[4];
		double plane2[4];

		plane[0] = backEnd.viewParms.portalPlane.normal[0];
		plane[1] = backEnd.viewParms.portalPlane.normal[1];
		plane[2] = backEnd.viewParms.portalPlane.normal[2];
		plane[3] = backEnd.viewParms.portalPlane.dist;

		plane2[0] = DotProduct( backEnd.viewParms.or.axis[0], plane );
		plane2[1] = DotProduct( backEnd.viewParms.or.axis[1], plane );
		plane2[2] = DotProduct( backEnd.viewParms.or.axis[2], plane );
		plane2[3] = DotProduct( plane, backEnd.viewParms.or.origin ) - plane[3];

		qglLoadMatrixf( s_flipMatrix );
		qglClipPlane( GL_CLIP_PLANE0, plane2 );
		qglEnable( GL_CLIP_PLANE0 );
	} else {
		qglDisable( GL_CLIP_PLANE0 );
	}
}

/*
============
RB_ZombieFX

  This is post-tesselation filtering, made especially for the Zombie.
============
*/

extern void GlobalVectorToLocal( const vec3_t in, vec3_t out );
extern vec_t VectorLengthSquared( const vec3_t v );

#define ZOMBIEFX_MAX_VERTS              2048
#define ZOMBIEFX_FADEOUT_TIME_SEC       ( 0.001 * ZOMBIEFX_FADEOUT_TIME )
#define ZOMBIEFX_MAX_HITS               128
#define ZOMBIEFX_MAX_NEWHITS            4
#define ZOMBIEFX_HIT_OKRANGE_SQR        9   // all verts within this range will be hit
#define ZOMBIEFX_HIT_MAXRANGE_SQR       36  // each bullet that strikes the bounding box, will effect verts inside this range (allowing for projections onto the mesh)
#define ZOMBIEFX_PERHIT_TAKEALPHA       150
#define ZOMBIEFX_MAX_HITS_PER_VERT      2

static char *zombieFxFleshHitSurfaceNames[2] = {"u_body","l_legs"};

// this stores each of the flesh hits for each of the zombies in the game
typedef struct {
	qboolean isHit;
	unsigned short numHits;
	unsigned short vertHits[ZOMBIEFX_MAX_HITS]; // bit flags to represent those verts that have been hit
	int numNewHits;
	vec3_t newHitPos[ZOMBIEFX_MAX_NEWHITS];
	vec3_t newHitDir[ZOMBIEFX_MAX_NEWHITS];
} trZombieFleshHitverts_t;
//
trZombieFleshHitverts_t zombieFleshHitVerts[MAX_SP_CLIENTS][2]; // one for upper, one for lower

void RB_ZombieFXInit( void ) {
	memset( zombieFleshHitVerts, 0, sizeof( zombieFleshHitVerts ) );
}

void RB_ZombieFXAddNewHit( int entityNum, const vec3_t hitPos, const vec3_t hitDir ) {
	int part = 0;

// disabled for E3, are we still going to use this?
	return;

	if ( entityNum == -1 ) {
		// hack, reset data
		RB_ZombieFXInit();
		return;
	}

	if ( entityNum & ( 1 << 30 ) ) {
		part = 1;
		entityNum &= ~( 1 << 30 );
	}

	if ( entityNum >= MAX_SP_CLIENTS ) {
		Com_Printf( "RB_ZombieFXAddNewHit: entityNum (%i) outside allowable range (%i)\n", entityNum, MAX_SP_CLIENTS );
		return;
	}
	if ( zombieFleshHitVerts[entityNum][part].numHits + zombieFleshHitVerts[entityNum][part].numNewHits >= ZOMBIEFX_MAX_HITS ) {
		// already full of hits
		return;
	}
	if ( zombieFleshHitVerts[entityNum][part].numNewHits >= ZOMBIEFX_MAX_NEWHITS ) {
		// just ignore this hit
		return;
	}
	// add it to the list
	VectorCopy( hitPos, zombieFleshHitVerts[entityNum][part].newHitPos[zombieFleshHitVerts[entityNum][part].numNewHits] );
	VectorCopy( hitDir, zombieFleshHitVerts[entityNum][part].newHitDir[zombieFleshHitVerts[entityNum][part].numNewHits] );
	zombieFleshHitVerts[entityNum][part].numNewHits++;
}

void RB_ZombieFXProcessNewHits( trZombieFleshHitverts_t *fleshHitVerts, int oldNumVerts, int numSurfVerts ) {
	float *xyzTrav, *normTrav;
	vec3_t hitPos, hitDir, v, testDir;
	float bestHitDist, thisDist;
	qboolean foundHit;
	int i, j, bestHit;
	unsigned short *hitTrav;
	byte hitCounts[ZOMBIEFX_MAX_VERTS];     // so we can quickly tell if a particular vert has been hit enough times already

// disabled for E3, are we still going to use this?
	return;

	// first build the hitCount list
	memset( hitCounts, 0, sizeof( hitCounts ) );
	for ( i = 0, hitTrav = fleshHitVerts->vertHits; i < fleshHitVerts->numHits; i++, hitTrav++ ) {
		hitCounts[*hitTrav]++;
	}

	// for each new hit
	for ( i = 0; i < fleshHitVerts->numNewHits; i++ ) {
		// calc the local hitPos
		VectorCopy( fleshHitVerts->newHitPos[i], v );
		VectorSubtract( v, backEnd.currentEntity->e.origin, v );
		GlobalVectorToLocal( v, hitPos );
		// calc the local hitDir
		VectorCopy( fleshHitVerts->newHitDir[i], v );
		GlobalVectorToLocal( v, hitDir );

		// look for close matches
		foundHit = qfalse;

		// for each vertex
		for (   j = 0, bestHitDist = -1, xyzTrav = tess.xyz[oldNumVerts], normTrav = tess.normal[oldNumVerts];
				j < numSurfVerts;
				j++, xyzTrav += 4, normTrav += 4 ) {

			// if this vert has been hit enough times already
			if ( hitCounts[j] > ZOMBIEFX_MAX_HITS_PER_VERT ) {
				continue;
			}
			// if this normal faces the wrong way, reject it
			if ( DotProduct( normTrav, hitDir ) > 0 ) {
				continue;
			}
			// get the diff vector
			VectorSubtract( xyzTrav, hitPos, testDir );
			// check for distance within range
			thisDist = VectorLengthSquared( testDir );
			if ( thisDist < ZOMBIEFX_HIT_OKRANGE_SQR ) {
				goto hitCheckDone;
			}
			thisDist = sqrt( thisDist );
			// check for the projection being inside range
			VectorMA( hitPos, thisDist, hitDir, v );
			VectorSubtract( xyzTrav, v, testDir );
			thisDist = VectorLengthSquared( testDir );
			if ( thisDist < ZOMBIEFX_HIT_OKRANGE_SQR ) {
				goto hitCheckDone;
			}
			// if we are still struggling to find a hit, then pick the closest outside the OK range
			if ( !foundHit ) {
				if ( thisDist < ZOMBIEFX_HIT_MAXRANGE_SQR && ( bestHitDist < 0 || thisDist < bestHitDist ) ) {
					bestHitDist = thisDist;
					bestHit = j;
				}
			}

			// if it gets to here, then it failed
			continue;

hitCheckDone:

			// this vertex was hit
			foundHit = qtrue;
			// set the appropriate bit-flag
			fleshHitVerts->isHit = qtrue;
			fleshHitVerts->vertHits[fleshHitVerts->numHits++] = (unsigned short)j;
			//if (fleshHitVerts->numHits == ZOMBIEFX_MAX_HITS)
			//	break;	// only find one close match per shot
			if ( fleshHitVerts->numHits == ZOMBIEFX_MAX_HITS ) {
				break;
			}
		}

		if ( fleshHitVerts->numHits == ZOMBIEFX_MAX_HITS ) {
			break;
		}

		// if we didn't find a hit vertex, grab the closest acceptible match
		if ( !foundHit && bestHitDist >= 0 ) {
			// set the appropriate bit-flag
			fleshHitVerts->isHit = qtrue;
			fleshHitVerts->vertHits[fleshHitVerts->numHits++] = (unsigned short)bestHit;
			if ( fleshHitVerts->numHits == ZOMBIEFX_MAX_HITS ) {
				break;
			}
		}
	}

	// we've processed any new hits
	fleshHitVerts->numNewHits = 0;
}

void RB_ZombieFXShowFleshHits( trZombieFleshHitverts_t *fleshHitVerts, int oldNumVerts, int numSurfVerts ) {
	byte *vertColors;
	unsigned short *vertHits;
	int i;

// disabled for E3, are we still going to use this?
	return;

	vertColors = tess.vertexColors[oldNumVerts];
	vertHits = fleshHitVerts->vertHits;

	// for each hit entry, adjust that verts alpha component
	for ( i = 0; i < fleshHitVerts->numHits; i++, vertHits++ ) {
		if ( vertColors[( *vertHits ) * 4 + 3] < ZOMBIEFX_PERHIT_TAKEALPHA ) {
			vertColors[( *vertHits ) * 4 + 3] = 0;
		} else {
			vertColors[( *vertHits ) * 4 + 3] -= ZOMBIEFX_PERHIT_TAKEALPHA;
		}
	}
}

void RB_ZombieFXDecompose( int oldNumVerts, int numSurfVerts, float deltaTimeScale ) {
	byte *vertColors;
	float   *xyz, *norm;
	int i;
	float alpha;

// disabled for E3, are we still going to use this?
	return;

	vertColors = tess.vertexColors[oldNumVerts];
	xyz = tess.xyz[oldNumVerts];
	norm = tess.normal[oldNumVerts];

	for ( i = 0; i < numSurfVerts; i++, vertColors += 4, xyz += 4, norm += 4 ) {
		alpha = 255.0 * ( (float)( 1 + i % 3 ) / 3.0 ) * deltaTimeScale * 2;
		if ( alpha > 255.0 ) {
			alpha = 255.0;
		}
		if ( (float)vertColors[3] - alpha < 0 ) {
			vertColors[3] = 0;
		} else {
			vertColors[3] -= (byte)alpha;
		}

		// skin shrinks with age
		VectorMA( xyz, -2.0 * deltaTimeScale, norm, xyz );
	}
}

void RB_ZombieFXFullAlpha( int oldNumVerts, int numSurfVerts ) {
	byte *vertColors;
	int i;

	vertColors = tess.vertexColors[oldNumVerts];

	for ( i = 0; i < numSurfVerts; i++, vertColors += 4 ) {
		vertColors[3] = 255;
	}
}

void RB_ZombieFX( int part, drawSurf_t *drawSurf, int oldNumVerts, int oldNumIndex ) {
	int numSurfVerts;
	float deltaTime;
	char    *surfName;
	trZombieFleshHitverts_t *fleshHitVerts;

	// Central point for Zombie post-tess processing. Various effects can be added from this point

// disabled for E3, are we still going to use this?
	return;

	if ( *drawSurf->surface == SF_MD3 ) {
		surfName = ( (md3Surface_t *)drawSurf->surface )->name;
	} else if ( *drawSurf->surface == SF_MDC ) {
		surfName = ( (mdcSurface_t *)drawSurf->surface )->name;
	} else {
		Com_Printf( "RB_ZombieFX: unknown surface type\n" );
		return;
	}

	// ignore all surfaces starting with u_sk (skeleton)
	if ( !Q_strncmp( surfName, "u_sk", 4 ) ) {
		return;
	}
	// legs
	if ( !Q_strncmp( surfName, "l_sk", 4 ) ) {
		return;
	}
	// head
	if ( !Q_strncmp( surfName, "h_sk", 4 ) ) {
		return;
	}

	numSurfVerts = tess.numVertexes - oldNumVerts;

	if ( numSurfVerts > ZOMBIEFX_MAX_VERTS ) {
		Com_Printf( "RB_ZombieFX: exceeded ZOMBIEFX_MAX_VERTS\n" );
		return;
	}

	deltaTime = backEnd.currentEntity->e.shaderTime;
	if ( ZOMBIEFX_FADEOUT_TIME_SEC < deltaTime ) {
		// nothing to do, it's done fading out
		tess.numVertexes = oldNumVerts;
		tess.numIndexes = oldNumIndex;
		return;
	}

	fleshHitVerts = &zombieFleshHitVerts[backEnd.currentEntity->e.entityNum][part];

	// set everything to full alpha
	RB_ZombieFXFullAlpha( oldNumVerts, numSurfVerts );

	// if this is the chest surface, do flesh hits
	if ( !Q_stricmp( surfName, zombieFxFleshHitSurfaceNames[part] ) ) {

		// check for any new bullet impacts that need to be scanned for triangle collisions
		if ( fleshHitVerts->numNewHits ) {
			RB_ZombieFXProcessNewHits( fleshHitVerts, oldNumVerts, numSurfVerts );
		}

		// hide vertices marked as being torn off
		if ( fleshHitVerts->isHit ) {
			RB_ZombieFXShowFleshHits( fleshHitVerts, oldNumVerts, numSurfVerts );
		}
	}

	// decompose?
	if ( deltaTime ) {
		RB_ZombieFXDecompose( oldNumVerts, numSurfVerts, deltaTime / ZOMBIEFX_FADEOUT_TIME_SEC );
	}

}


#define MAC_EVENT_PUMP_MSEC     5

/*
==================
RB_RenderDrawSurfList
==================
*/
void RB_RenderDrawSurfList( drawSurf_t *drawSurfs, int numDrawSurfs ) {
	shader_t        *shader, *oldShader;
	int fogNum, oldFogNum;
	int entityNum, oldEntityNum;
	int dlighted, oldDlighted;
	qboolean depthRange, oldDepthRange;
	int i;
	drawSurf_t      *drawSurf;
	int oldSort;
	float originalTime;
	int oldNumVerts, oldNumIndex;
//GR - tessellation flag
	int atiTess = 0, oldAtiTess;
#ifdef __MACOS__
	int macEventTime;

	Sys_PumpEvents();       // crutch up the mac's limited buffer queue size

	// we don't want to pump the event loop too often and waste time, so
	// we are going to check every shader change
	macEventTime = ri.Milliseconds() + MAC_EVENT_PUMP_MSEC;
#endif

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;

	// clear the z buffer, set the modelview, etc
	RB_BeginDrawingView();

	// draw everything
	oldEntityNum = -1;
	backEnd.currentEntity = &tr.worldEntity;
	oldShader = NULL;
	oldFogNum = -1;
	oldDepthRange = qfalse;
	oldDlighted = qfalse;
	oldSort = -1;
	depthRange = qfalse;
// GR - tessellation also forces to draw everything
	oldAtiTess = -1;

	backEnd.pc.c_surfaces += numDrawSurfs;

	for ( i = 0, drawSurf = drawSurfs ; i < numDrawSurfs ; i++, drawSurf++ ) {
		if ( drawSurf->sort == oldSort ) {
			// fast path, same as previous sort
			oldNumVerts = tess.numVertexes;
			oldNumIndex = tess.numIndexes;

			rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );
/*
			// RF, convert the newly created vertexes into dust particles, and overwrite
			if (backEnd.currentEntity->e.reFlags & REFLAG_ZOMBIEFX) {
				RB_ZombieFX( 0, drawSurf, oldNumVerts, oldNumIndex );
			}
			else if (backEnd.currentEntity->e.reFlags & REFLAG_ZOMBIEFX2) {
				RB_ZombieFX( 1, drawSurf, oldNumVerts, oldNumIndex );
			}
*/
			continue;
		}
		oldSort = drawSurf->sort;
// GR - also extract tesselation flag
		R_DecomposeSort( drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted, &atiTess );

		//
		// change the tess parameters if needed
		// a "entityMergable" shader is a shader that can have surfaces from seperate
		// entities merged into a single batch, like smoke and blood puff sprites
		if ( shader != oldShader || fogNum != oldFogNum || dlighted != oldDlighted
// GR - force draw on tessellation flag change
			 || ( atiTess != oldAtiTess )
			 || ( entityNum != oldEntityNum && !shader->entityMergable ) ) {
			if ( oldShader != NULL ) {
#ifdef __MACOS__    // crutch up the mac's limited buffer queue size
				int t;

				t = ri.Milliseconds();
				if ( t > macEventTime ) {
					macEventTime = t + MAC_EVENT_PUMP_MSEC;
					Sys_PumpEvents();
				}
#endif
// GR - pass tessellation flag to the shader command
//		make sure to use oldAtiTess!!!
				tess.ATI_tess = ( oldAtiTess == ATI_TESS_TRUFORM );

				RB_EndSurface();
			}
			RB_BeginSurface( shader, fogNum );
			oldShader = shader;
			oldFogNum = fogNum;
			oldDlighted = dlighted;
// GR - update old tessellation flag
			oldAtiTess = atiTess;
		}

		//
		// change the modelview matrix if needed
		//
		if ( entityNum != oldEntityNum ) {
			depthRange = qfalse;

			if ( entityNum != ENTITYNUM_WORLD ) {
				backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
				backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;

				// we have to reset the shaderTime as well otherwise image animations start
				// from the wrong frame
//				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				// set up the transformation matrix
				R_RotateForEntity( backEnd.currentEntity, &backEnd.viewParms, &backEnd.or );

				// set up the dynamic lighting if needed
				if ( backEnd.currentEntity->needDlights ) {
					R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
				}

				if ( backEnd.currentEntity->e.renderfx & RF_DEPTHHACK ) {
					// hack the depth range to prevent view model from poking into walls
					depthRange = qtrue;
				}
			} else {
				backEnd.currentEntity = &tr.worldEntity;
				backEnd.refdef.floatTime = originalTime;
				backEnd.or = backEnd.viewParms.world;

				// we have to reset the shaderTime as well otherwise image animations on
				// the world (like water) continue with the wrong frame
//				tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

				R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );
			}

			qglLoadMatrixf( backEnd.or.modelMatrix );

			//
			// change depthrange if needed
			//
			if ( oldDepthRange != depthRange ) {
				if ( depthRange ) {
					qglDepthRange( 0, 0.3 );
				} else {
					qglDepthRange( 0, 1 );
				}
				oldDepthRange = depthRange;
			}

			oldEntityNum = entityNum;
		}

		// RF, ZOMBIEFX, store the tess indexes, so we can grab the calculated
		// vertex positions and normals, and convert them into dust particles
		oldNumVerts = tess.numVertexes;
		oldNumIndex = tess.numIndexes;

		// add the triangles for this surface
		rb_surfaceTable[ *drawSurf->surface ]( drawSurf->surface );

		// RF, convert the newly created vertexes into dust particles, and overwrite
		if ( backEnd.currentEntity->e.reFlags & REFLAG_ZOMBIEFX ) {
			RB_ZombieFX( 0, drawSurf, oldNumVerts, oldNumIndex );
		} else if ( backEnd.currentEntity->e.reFlags & REFLAG_ZOMBIEFX2 )     {
			RB_ZombieFX( 1, drawSurf, oldNumVerts, oldNumIndex );
		}
	}

	// draw the contents of the last shader batch
	if ( oldShader != NULL ) {
// GR - pass tessellation flag to the shader command
//		make sure to use oldAtiTess!!!
		tess.ATI_tess = ( oldAtiTess == ATI_TESS_TRUFORM );

		RB_EndSurface();
	}

	// go back to the world modelview matrix
	backEnd.currentEntity = &tr.worldEntity;
	backEnd.refdef.floatTime = originalTime;
	backEnd.or = backEnd.viewParms.world;
	R_TransformDlights( backEnd.refdef.num_dlights, backEnd.refdef.dlights, &backEnd.or );

	qglLoadMatrixf( backEnd.viewParms.world.modelMatrix );
	if ( depthRange ) {
		qglDepthRange( 0, 1 );
	}

	// (SA) draw sun
	RB_DrawSun();


	// darken down any stencil shadows
	RB_ShadowFinish();

	// add light flares on lights that aren't obscured
	RB_RenderFlares();

#ifdef __MACOS__
	Sys_PumpEvents();       // crutch up the mac's limited buffer queue size
#endif
}


/*
============================================================================

RENDER BACK END THREAD FUNCTIONS

============================================================================
*/

/*
================
RB_SetGL2D

================
*/
void    RB_SetGL2D( void ) {
	backEnd.projection2D = qtrue;

	// set 2D virtual screen size
	qglViewport( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglScissor( 0, 0, glConfig.vidWidth, glConfig.vidHeight );
	qglMatrixMode( GL_PROJECTION );
	qglLoadIdentity();
	qglOrtho( 0, glConfig.vidWidth, glConfig.vidHeight, 0, 0, 1 );
	qglMatrixMode( GL_MODELVIEW );
	qglLoadIdentity();

	GL_State( GLS_DEPTHTEST_DISABLE |
			  GLS_SRCBLEND_SRC_ALPHA |
			  GLS_DSTBLEND_ONE_MINUS_SRC_ALPHA );

	qglDisable( GL_FOG ); //----(SA)	added

	qglDisable( GL_CULL_FACE );
	qglDisable( GL_CLIP_PLANE0 );

	// set time for 2D shaders
	backEnd.refdef.time = ri.Milliseconds();
	backEnd.refdef.floatTime = backEnd.refdef.time * 0.001f;
}


/*
=============
RE_StretchRaw

FIXME: not exactly backend
Stretches a raw 32 bit power of 2 bitmap image over the given screen rectangle.
Used for cinematics.
=============
*/
void RE_StretchRaw( int x, int y, int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty ) {
	int i, j;
	int start, end;

	if ( !tr.registered ) {
		return;
	}
	R_SyncRenderThread();

	// we definately want to sync every frame for the cinematics
	qglFinish();

	start = end = 0;
	if ( r_speeds->integer ) {
		start = ri.Milliseconds();
	}

	// make sure rows and cols are powers of 2
	for ( i = 0 ; ( 1 << i ) < cols ; i++ ) {
	}
	for ( j = 0 ; ( 1 << j ) < rows ; j++ ) {
	}
	if ( ( 1 << i ) != cols || ( 1 << j ) != rows ) {
		ri.Error( ERR_DROP, "Draw_StretchRaw: size not a power of 2: %i by %i", cols, rows );
	}

	GL_Bind( tr.scratchImage[client] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D( GL_TEXTURE_2D, 0, 3, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	} else {
		if ( dirty ) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}

	if ( r_speeds->integer ) {
		end = ri.Milliseconds();
		ri.Printf( PRINT_ALL, "qglTexSubImage2D %i, %i: %i msec\n", cols, rows, end - start );
	}

	RB_SetGL2D();

	qglColor3f( tr.identityLight, tr.identityLight, tr.identityLight );

	qglBegin( GL_QUADS );
	qglTexCoord2f( 0.5f / cols,  0.5f / rows );
	qglVertex2f( x, y );
	qglTexCoord2f( ( cols - 0.5f ) / cols,  0.5f / rows );
	qglVertex2f( x + w, y );
	qglTexCoord2f( ( cols - 0.5f ) / cols, ( rows - 0.5f ) / rows );
	qglVertex2f( x + w, y + h );
	qglTexCoord2f( 0.5f / cols, ( rows - 0.5f ) / rows );
	qglVertex2f( x, y + h );
	qglEnd();
}


void RE_UploadCinematic( int w, int h, int cols, int rows, const byte *data, int client, qboolean dirty ) {

	GL_Bind( tr.scratchImage[client] );

	// if the scratchImage isn't in the format we want, specify it as a new texture
	if ( cols != tr.scratchImage[client]->width || rows != tr.scratchImage[client]->height ) {
		tr.scratchImage[client]->width = tr.scratchImage[client]->uploadWidth = cols;
		tr.scratchImage[client]->height = tr.scratchImage[client]->uploadHeight = rows;
		qglTexImage2D( GL_TEXTURE_2D, 0, 3, cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		qglTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	} else {
		if ( dirty ) {
			// otherwise, just subimage upload it so that drivers can tell we are going to be changing
			// it and don't try and do a texture compression
			qglTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, cols, rows, GL_RGBA, GL_UNSIGNED_BYTE, data );
		}
	}
}


/*
=============
RB_SetColor

=============
*/
const void  *RB_SetColor( const void *data ) {
	const setColorCommand_t *cmd;

	cmd = (const setColorCommand_t *)data;

	backEnd.color2D[0] = cmd->color[0] * 255;
	backEnd.color2D[1] = cmd->color[1] * 255;
	backEnd.color2D[2] = cmd->color[2] * 255;
	backEnd.color2D[3] = cmd->color[3] * 255;

	return (const void *)( cmd + 1 );
}

/*
=============
RB_StretchPic
=============
*/
const void *RB_StretchPic( const void *data ) {
	const stretchPicCommand_t   *cmd;
	shader_t *shader;
	int numVerts, numIndexes;

	cmd = (const stretchPicCommand_t *)data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

	*(int *)tess.vertexColors[ numVerts ] =
		*(int *)tess.vertexColors[ numVerts + 1 ] =
			*(int *)tess.vertexColors[ numVerts + 2 ] =
				*(int *)tess.vertexColors[ numVerts + 3 ] = *(int *)backEnd.color2D;

	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void *)( cmd + 1 );
}


/*
==============
RB_StretchPicGradient
==============
*/
const void *RB_StretchPicGradient( const void *data ) {
	const stretchPicCommand_t   *cmd;
	shader_t *shader;
	int numVerts, numIndexes;

	cmd = (const stretchPicCommand_t *)data;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	shader = cmd->shader;
	if ( shader != tess.shader ) {
		if ( tess.numIndexes ) {
			RB_EndSurface();
		}
		backEnd.currentEntity = &backEnd.entity2D;
		RB_BeginSurface( shader, 0 );
	}

	RB_CHECKOVERFLOW( 4, 6 );
	numVerts = tess.numVertexes;
	numIndexes = tess.numIndexes;

	tess.numVertexes += 4;
	tess.numIndexes += 6;

	tess.indexes[ numIndexes ] = numVerts + 3;
	tess.indexes[ numIndexes + 1 ] = numVerts + 0;
	tess.indexes[ numIndexes + 2 ] = numVerts + 2;
	tess.indexes[ numIndexes + 3 ] = numVerts + 2;
	tess.indexes[ numIndexes + 4 ] = numVerts + 0;
	tess.indexes[ numIndexes + 5 ] = numVerts + 1;

//	*(int *)tess.vertexColors[ numVerts ] =
//		*(int *)tess.vertexColors[ numVerts + 1 ] =
//		*(int *)tess.vertexColors[ numVerts + 2 ] =
//		*(int *)tess.vertexColors[ numVerts + 3 ] = *(int *)backEnd.color2D;

	*(int *)tess.vertexColors[ numVerts ] =
		*(int *)tess.vertexColors[ numVerts + 1 ] = *(int *)backEnd.color2D;

	*(int *)tess.vertexColors[ numVerts + 2 ] =
		*(int *)tess.vertexColors[ numVerts + 3 ] = *(int *)cmd->gradientColor;

	tess.xyz[ numVerts ][0] = cmd->x;
	tess.xyz[ numVerts ][1] = cmd->y;
	tess.xyz[ numVerts ][2] = 0;

	tess.texCoords[ numVerts ][0][0] = cmd->s1;
	tess.texCoords[ numVerts ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 1 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 1 ][1] = cmd->y;
	tess.xyz[ numVerts + 1 ][2] = 0;

	tess.texCoords[ numVerts + 1 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 1 ][0][1] = cmd->t1;

	tess.xyz[ numVerts + 2 ][0] = cmd->x + cmd->w;
	tess.xyz[ numVerts + 2 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 2 ][2] = 0;

	tess.texCoords[ numVerts + 2 ][0][0] = cmd->s2;
	tess.texCoords[ numVerts + 2 ][0][1] = cmd->t2;

	tess.xyz[ numVerts + 3 ][0] = cmd->x;
	tess.xyz[ numVerts + 3 ][1] = cmd->y + cmd->h;
	tess.xyz[ numVerts + 3 ][2] = 0;

	tess.texCoords[ numVerts + 3 ][0][0] = cmd->s1;
	tess.texCoords[ numVerts + 3 ][0][1] = cmd->t2;

	return (const void *)( cmd + 1 );
}


/*
=============
RB_DrawSurfs

=============
*/
const void  *RB_DrawSurfs( const void *data ) {
	const drawSurfsCommand_t    *cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	cmd = (const drawSurfsCommand_t *)data;

	backEnd.refdef = cmd->refdef;
	backEnd.viewParms = cmd->viewParms;

	RB_RenderDrawSurfList( cmd->drawSurfs, cmd->numDrawSurfs );

	return (const void *)( cmd + 1 );
}


/*
=============
RB_DrawBuffer

=============
*/
const void  *RB_DrawBuffer( const void *data ) {
	const drawBufferCommand_t   *cmd;

	cmd = (const drawBufferCommand_t *)data;

	qglDrawBuffer( cmd->buffer );

	// clear screen for debugging
	if ( r_clear->integer ) {
		qglClearColor( 1, 0, 0.5, 1 );
		qglClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	}

	return (const void *)( cmd + 1 );
}

/*
===============
RB_ShowImages

Draw all the images to the screen, on top of whatever
was there.  This is used to test for texture thrashing.

Also called by RE_EndRegistration
===============
*/
void RB_ShowImages( void ) {
	int i;
	image_t *image;
	float x, y, w, h;
	int start, end;

	if ( !backEnd.projection2D ) {
		RB_SetGL2D();
	}

	qglClear( GL_COLOR_BUFFER_BIT );

	qglFinish();


	start = ri.Milliseconds();

	for ( i = 0 ; i < tr.numImages ; i++ ) {
		image = tr.images[i];

		w = glConfig.vidWidth / 40;
		h = glConfig.vidHeight / 30;

		x = i % 40 * w;
		y = i / 30 * h;

		// show in proportional size in mode 2
		if ( r_showImages->integer == 2 ) {
			w *= image->uploadWidth / 512.0f;
			h *= image->uploadHeight / 512.0f;
		}

		GL_Bind( image );
		qglBegin( GL_QUADS );
		qglTexCoord2f( 0, 0 );
		qglVertex2f( x, y );
		qglTexCoord2f( 1, 0 );
		qglVertex2f( x + w, y );
		qglTexCoord2f( 1, 1 );
		qglVertex2f( x + w, y + h );
		qglTexCoord2f( 0, 1 );
		qglVertex2f( x, y + h );
		qglEnd();
	}

	qglFinish();

	end = ri.Milliseconds();
	ri.Printf( PRINT_ALL, "%i msec to draw all images\n", end - start );

}


/*
=============
RB_SwapBuffers

=============
*/
const void  *RB_SwapBuffers( const void *data ) {
	const swapBuffersCommand_t  *cmd;

	// finish any 2D drawing if needed
	if ( tess.numIndexes ) {
		RB_EndSurface();
	}

	// texture swapping test
	if ( r_showImages->integer ) {
		RB_ShowImages();
	}

	cmd = (const swapBuffersCommand_t *)data;

	// we measure overdraw by reading back the stencil buffer and
	// counting up the number of increments that have happened
	if ( r_measureOverdraw->integer ) {
		int i;
		long sum = 0;
		unsigned char *stencilReadback;

		stencilReadback = ri.Hunk_AllocateTempMemory( glConfig.vidWidth * glConfig.vidHeight );
		qglReadPixels( 0, 0, glConfig.vidWidth, glConfig.vidHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencilReadback );

		for ( i = 0; i < glConfig.vidWidth * glConfig.vidHeight; i++ ) {
			sum += stencilReadback[i];
		}

		backEnd.pc.c_overDraw += sum;
		ri.Hunk_FreeTempMemory( stencilReadback );
	}


	if ( !glState.finishCalled ) {
		qglFinish();
	}

	GLimp_LogComment( "***************** RB_SwapBuffers *****************\n\n\n" );

	GLimp_EndFrame();

	backEnd.projection2D = qfalse;

	return (const void *)( cmd + 1 );
}

/*
====================
RB_ExecuteRenderCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
====================
*/
void RB_ExecuteRenderCommands( const void *data ) {
	int t1, t2;

	t1 = ri.Milliseconds();

	if ( !r_smp->integer || data == backEndData[0]->commands.cmds ) {
		backEnd.smpFrame = 0;
	} else {
		backEnd.smpFrame = 1;
	}

	while ( 1 ) {
		switch ( *(const int *)data ) {
		case RC_SET_COLOR:
			data = RB_SetColor( data );
			break;
		case RC_STRETCH_PIC:
			data = RB_StretchPic( data );
			break;
		case RC_STRETCH_PIC_GRADIENT:
			data = RB_StretchPicGradient( data );
			break;
		case RC_DRAW_SURFS:
			data = RB_DrawSurfs( data );
			break;
		case RC_DRAW_BUFFER:
			data = RB_DrawBuffer( data );
			break;
		case RC_SWAP_BUFFERS:
			data = RB_SwapBuffers( data );
			break;

		case RC_END_OF_LIST:
		default:
			// stop rendering on this thread
			t2 = ri.Milliseconds();
			backEnd.pc.msec = t2 - t1;
			return;
		}
	}

}


/*
================
RB_RenderThread
================
*/
void RB_RenderThread( void ) {
	const void  *data;

	// wait for either a rendering command or a quit command
	while ( 1 ) {
		// sleep until we have work to do
		data = GLimp_RendererSleep();

		if ( !data ) {
			return; // all done, renderer is shutting down
		}

		renderThreadActive = qtrue;

		RB_ExecuteRenderCommands( data );

		renderThreadActive = qfalse;
	}
}

