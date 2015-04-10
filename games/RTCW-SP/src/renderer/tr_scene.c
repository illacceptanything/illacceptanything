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

int r_firstSceneDrawSurf;

int r_numdlights;
int r_firstSceneDlight;

int r_numcoronas;
int r_firstSceneCorona;

int r_numentities;
int r_firstSceneEntity;

int r_numpolys;
int r_firstScenePoly;

int r_numpolyverts;

int skyboxportal;
int drawskyboxportal;

/*
====================
R_ToggleSmpFrame

====================
*/
void R_ToggleSmpFrame( void ) {
	if ( r_smp->integer ) {
		// use the other buffers next frame, because another CPU
		// may still be rendering into the current ones
		tr.smpFrame ^= 1;
	} else {
		tr.smpFrame = 0;
	}

	backEndData[tr.smpFrame]->commands.used = 0;

	r_firstSceneDrawSurf = 0;

	r_numdlights = 0;
	r_firstSceneDlight = 0;

	r_numcoronas = 0;
	r_firstSceneCorona = 0;

	r_numentities = 0;
	r_firstSceneEntity = 0;

	r_numpolys = 0;
	r_firstScenePoly = 0;

	r_numpolyverts = 0;
}


/*
====================
RE_ClearScene

====================
*/
void RE_ClearScene( void ) {
	r_firstSceneDlight = r_numdlights;
	r_firstSceneCorona = r_numcoronas;
	r_firstSceneEntity = r_numentities;
	r_firstScenePoly = r_numpolys;
}

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonSurfaces( void ) {
	int i;
	shader_t    *sh;
	srfPoly_t   *poly;

	tr.currentEntityNum = ENTITYNUM_WORLD;
	tr.shiftedEntityNum = tr.currentEntityNum << QSORT_ENTITYNUM_SHIFT;

	for ( i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys ; i++, poly++ ) {
		sh = R_GetShaderByHandle( poly->hShader );
// GR - not tessellated
		R_AddDrawSurf( ( void * )poly, sh, poly->fogIndex, qfalse, ATI_TESS_NONE );
	}
}

/*
=====================
RE_AddPolyToScene

=====================
*/
void RE_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts ) {
	srfPoly_t   *poly;
	int i;
	int fogIndex;
	fog_t       *fog;
	vec3_t bounds[2];

	if ( !tr.registered ) {
		return;
	}

	if ( !hShader ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_AddPolyToScene: NULL poly shader\n" );
		return;
	}

	if ( ( ( r_numpolyverts + numVerts ) > max_polyverts ) || ( r_numpolys >= max_polys ) ) {
		return;
	}

	poly = &backEndData[tr.smpFrame]->polys[r_numpolys];
	poly->surfaceType = SF_POLY;
	poly->hShader = hShader;
	poly->numVerts = numVerts;
	poly->verts = &backEndData[tr.smpFrame]->polyVerts[r_numpolyverts];

	memcpy( poly->verts, verts, numVerts * sizeof( *verts ) );
	// Ridah
	if ( glConfig.hardwareType == GLHW_RAGEPRO ) {
		poly->verts->modulate[0] = 255;
		poly->verts->modulate[1] = 255;
		poly->verts->modulate[2] = 255;
		poly->verts->modulate[3] = 255;
	}
	// done.
	r_numpolys++;
	r_numpolyverts += numVerts;

	// see if it is in a fog volume
	if ( tr.world->numfogs == 1 ) {
		fogIndex = 0;
	} else {
		// find which fog volume the poly is in
		VectorCopy( poly->verts[0].xyz, bounds[0] );
		VectorCopy( poly->verts[0].xyz, bounds[1] );
		for ( i = 1 ; i < poly->numVerts ; i++ ) {
			AddPointToBounds( poly->verts[i].xyz, bounds[0], bounds[1] );
		}
		for ( fogIndex = 1 ; fogIndex < tr.world->numfogs ; fogIndex++ ) {
			fog = &tr.world->fogs[fogIndex];
			if ( bounds[1][0] >= fog->bounds[0][0]
				 && bounds[1][1] >= fog->bounds[0][1]
				 && bounds[1][2] >= fog->bounds[0][2]
				 && bounds[0][0] <= fog->bounds[1][0]
				 && bounds[0][1] <= fog->bounds[1][1]
				 && bounds[0][2] <= fog->bounds[1][2] ) {
				break;
			}
		}
		if ( fogIndex == tr.world->numfogs ) {
			fogIndex = 0;
		}
	}
	poly->fogIndex = fogIndex;
}

// Ridah
/*
=====================
RE_AddPolysToScene

=====================
*/
void RE_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys ) {
	srfPoly_t   *poly;
	int i;
	int fogIndex;
	fog_t       *fog;
	vec3_t bounds[2];
	int j;

	if ( !tr.registered ) {
		return;
	}

	if ( !hShader ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_AddPolysToScene: NULL poly shader\n" );
		return;
	}

	for ( j = 0; j < numPolys; j++ ) {
		if ( r_numpolyverts + numVerts > max_polyverts || r_numpolys >= max_polys ) {
//			ri.Printf( PRINT_WARNING, "WARNING: RE_AddPolysToScene: MAX_POLYS or MAX_POLYVERTS reached\n");
			return;
		}

		poly = &backEndData[tr.smpFrame]->polys[r_numpolys];
		poly->surfaceType = SF_POLY;
		poly->hShader = hShader;
		poly->numVerts = numVerts;
		poly->verts = &backEndData[tr.smpFrame]->polyVerts[r_numpolyverts];

		memcpy( poly->verts, &verts[numVerts * j], numVerts * sizeof( *verts ) );
		// Ridah
		if ( glConfig.hardwareType == GLHW_RAGEPRO ) {
			poly->verts->modulate[0] = 255;
			poly->verts->modulate[1] = 255;
			poly->verts->modulate[2] = 255;
			poly->verts->modulate[3] = 255;
		}
		// done.
		r_numpolys++;
		r_numpolyverts += numVerts;

		// if no world is loaded
		if ( tr.world == NULL ) {
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if ( tr.world->numfogs == 1 ) {
			fogIndex = 0;
		} else {
			// find which fog volume the poly is in
			VectorCopy( poly->verts[0].xyz, bounds[0] );
			VectorCopy( poly->verts[0].xyz, bounds[1] );
			for ( i = 1 ; i < poly->numVerts ; i++ ) {
				AddPointToBounds( poly->verts[i].xyz, bounds[0], bounds[1] );
			}
			for ( fogIndex = 1 ; fogIndex < tr.world->numfogs ; fogIndex++ ) {
				fog = &tr.world->fogs[fogIndex];
				if ( bounds[1][0] >= fog->bounds[0][0]
					 && bounds[1][1] >= fog->bounds[0][1]
					 && bounds[1][2] >= fog->bounds[0][2]
					 && bounds[0][0] <= fog->bounds[1][0]
					 && bounds[0][1] <= fog->bounds[1][1]
					 && bounds[0][2] <= fog->bounds[1][2] ) {
					break;
				}
			}
			if ( fogIndex == tr.world->numfogs ) {
				fogIndex = 0;
			}
		}
		poly->fogIndex = fogIndex;
	}
}
// done.


//=================================================================================


/*
=====================
RE_AddRefEntityToScene

=====================
*/
void RE_AddRefEntityToScene( const refEntity_t *ent ) {
	if ( !tr.registered ) {
		return;
	}
	// show_bug.cgi?id=402
	if ( r_numentities >= ENTITYNUM_WORLD ) {
		return;
	}
	if ( ent->reType < 0 || ent->reType >= RT_MAX_REF_ENTITY_TYPE ) {
		ri.Error( ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType );
	}

	backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
	backEndData[tr.smpFrame]->entities[r_numentities].lightingCalculated = qfalse;

	r_numentities++;
}

// Ridah, added support for overdraw field
/*
=====================
RE_AddLightToScene

=====================
*/
void RE_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b, int overdraw ) {
	dlight_t    *dl;

	if ( !tr.registered ) {
		return;
	}
	if ( r_numdlights >= MAX_DLIGHTS ) {
		return;
	}
	if ( intensity <= 0 ) {
		return;
	}
	// these cards don't have the correct blend mode
	if ( glConfig.hardwareType == GLHW_RIVA128 || glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		return;
	}
	// RF, allow us to force some dlights under all circumstances
	if ( !( overdraw & REF_FORCE_DLIGHT ) ) {
		if ( r_dynamiclight->integer == 0 ) {
			return;
		}
		if ( r_dynamiclight->integer == 2 && !( backEndData[tr.smpFrame]->dlights[r_numdlights].forced ) ) {
			return;
		}
	}

	if ( r_dlightScale->value <= 0 ) { //----(SA)	added
		return;
	}

	overdraw &= ~REF_FORCE_DLIGHT;
	overdraw &= ~REF_JUNIOR_DLIGHT;

	dl = &backEndData[tr.smpFrame]->dlights[r_numdlights++];
	VectorCopy( org, dl->origin );

	dl->radius = intensity * r_dlightScale->value;  //----(SA)	modified
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->dlshader = NULL;
	dl->overdraw = 0;

	if ( overdraw == 10 ) { // sorry, hijacking 10 for a quick hack (SA)
		dl->dlshader = R_GetShaderByHandle( RE_RegisterShader( "negdlightshader" ) );
	} else if ( overdraw == 11 ) { // 11 is flames
		dl->dlshader = R_GetShaderByHandle( RE_RegisterShader( "flamedlightshader" ) );
	} else {
		dl->overdraw = overdraw;
	}
}
// done.


/*
==============
RE_AddCoronaToScene
==============
*/
void RE_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, int flags ) {
	corona_t    *cor;

	if ( !tr.registered ) {
		return;
	}
	if ( r_numcoronas >= MAX_CORONAS ) {
		return;
	}

	cor = &backEndData[tr.smpFrame]->coronas[r_numcoronas++];
	VectorCopy( org, cor->origin );
	cor->color[0] = r;
	cor->color[1] = g;
	cor->color[2] = b;
	cor->scale = scale;
	cor->id = id;
	cor->flags = flags;
}

/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/
void RE_RenderScene( const refdef_t *fd ) {
	viewParms_t parms;
	int startTime;

	if ( !tr.registered ) {
		return;
	}
	GLimp_LogComment( "====== RE_RenderScene =====\n" );

	if ( r_norefresh->integer ) {
		return;
	}

	startTime = ri.Milliseconds();

	if ( !tr.world && !( fd->rdflags & RDF_NOWORLDMODEL ) ) {
		ri.Error( ERR_DROP, "R_RenderScene: NULL worldmodel" );
	}

	memcpy( tr.refdef.text, fd->text, sizeof( tr.refdef.text ) );

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy( fd->vieworg, tr.refdef.vieworg );
	VectorCopy( fd->viewaxis[0], tr.refdef.viewaxis[0] );
	VectorCopy( fd->viewaxis[1], tr.refdef.viewaxis[1] );
	VectorCopy( fd->viewaxis[2], tr.refdef.viewaxis[2] );

	tr.refdef.time = fd->time;
	tr.refdef.rdflags = fd->rdflags;

	if ( fd->rdflags & RDF_SKYBOXPORTAL ) {
		skyboxportal = 1;
	}

	if ( fd->rdflags & RDF_DRAWSKYBOX ) {
		drawskyboxportal = 1;
	} else {
		drawskyboxportal = 0;
	}

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = qfalse;
	if ( !( tr.refdef.rdflags & RDF_NOWORLDMODEL ) ) {
		int areaDiff;
		int i;

		// compare the area bits
		areaDiff = 0;
		for ( i = 0 ; i < MAX_MAP_AREA_BYTES / 4 ; i++ ) {
			areaDiff |= ( (int *)tr.refdef.areamask )[i] ^ ( (int *)fd->areamask )[i];
			( (int *)tr.refdef.areamask )[i] = ( (int *)fd->areamask )[i];
		}

		if ( areaDiff ) {
			// a door just opened or something
			tr.refdef.areamaskModified = qtrue;
		}
	}


	// derived info

	tr.refdef.floatTime = tr.refdef.time * 0.001f;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData[tr.smpFrame]->drawSurfs;

	tr.refdef.num_entities = r_numentities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData[tr.smpFrame]->entities[r_firstSceneEntity];

	tr.refdef.num_dlights = r_numdlights - r_firstSceneDlight;
	tr.refdef.dlights = &backEndData[tr.smpFrame]->dlights[r_firstSceneDlight];

	tr.refdef.num_coronas = r_numcoronas - r_firstSceneCorona;
	tr.refdef.coronas = &backEndData[tr.smpFrame]->coronas[r_firstSceneCorona];

	tr.refdef.numPolys = r_numpolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData[tr.smpFrame]->polys[r_firstScenePoly];

	// turn off dynamic lighting globally by clearing all the
	// dlights if it needs to be disabled or if vertex lighting is enabled
	if ( /*r_dynamiclight->integer == 0 ||*/    // RF, disabled so we can force things like lightning dlights
		r_vertexLight->integer == 1 ||
		glConfig.hardwareType == GLHW_PERMEDIA2 ) {
		tr.refdef.num_dlights = 0;
	}

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	memset( &parms, 0, sizeof( parms ) );
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - ( tr.refdef.y + tr.refdef.height );
	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;
	parms.isPortal = qfalse;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	VectorCopy( fd->vieworg, parms.or.origin );
	VectorCopy( fd->viewaxis[0], parms.or.axis[0] );
	VectorCopy( fd->viewaxis[1], parms.or.axis[1] );
	VectorCopy( fd->viewaxis[2], parms.or.axis[2] );

	VectorCopy( fd->vieworg, parms.pvsOrigin );

	R_RenderView( &parms );

	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneEntity = r_numentities;
	r_firstSceneDlight = r_numdlights;
	r_firstScenePoly = r_numpolys;

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}
