/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).  

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		cg_ents.c
 *
 * desc:		present snapshot entities, happens every single frame
 *
*/


#include "cg_local.h"

///////////////////////
extern int propellerModel;
///////////////////////

/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							 char *tagName, int startIndex, vec3_t *offset ) {
	int i;
	orientation_t lerped;

	// lerp the tag
	trap_R_LerpTag( &lerped, parent, tagName, startIndex );

	// FIXME: allow origin offsets along tag?	//----(SA) Yes! Adding.

	VectorCopy( parent->origin, entity->origin );

	if ( offset ) {
		VectorAdd( lerped.origin, *offset, lerped.origin );
	}
//----(SA) end

	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( lerped.axis, ( (refEntity_t *)parent )->axis, entity->axis );
	// Ridah, not sure why this was here.. causes jittery torso animation, since the torso might have
	// different frame/oldFrame
	//entity->backlerp = parent->backlerp;
}


/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
									char *tagName ) {
	int i;
	orientation_t lerped;
	vec3_t tempAxis[3];

//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag( &lerped, parent, tagName, 0 );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( entity->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, ( (refEntity_t *)parent )->axis, entity->axis );
}


//----(SA)	added
/*
==============
CG_LoseArmor
	maybe better in cg_localents.c
==============
*/
void CG_LoseArmor( centity_t *cent, int index ) {
}

/*
==============
CG_AttachedPartChange
==============
*/
void CG_AttachedPartChange( centity_t *cent ) {
}

//----(SA)	end

/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition( centity_t *cent ) {
	if ( cent->currentState.solid == SOLID_BMODEL ) {
		vec3_t origin;
		float   *v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( cent->lerpOrigin, v, origin );
		trap_S_UpdateEntityPosition( cent->currentState.number, origin );
	} else {
		trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
	}
}




#define LS_FRAMETIME 100 // (ms)  cycle through lightstyle characters at 10fps


/*
==============
CG_SetDlightIntensity

==============
*/
void CG_AddLightstyle( centity_t *cent ) {
	float lightval;
	int cl;
	int r, g, b;
	int stringlength;
	float offset;
	int offsetwhole;
	int otime;
	int lastch, nextch;

	if ( !cent->dl_stylestring ) {
		return;
	}

	otime = cg.time - cent->dl_time;
	stringlength = strlen( cent->dl_stylestring );

	// it's been a long time since you were updated, lets assume a reset
	if ( otime > 2 * LS_FRAMETIME ) {
		otime = 0;
		cent->dl_frame = cent->dl_oldframe = 0;
		cent->dl_backlerp = 0;
	}

	cent->dl_time = cg.time;

	offset = ( (float)otime ) / LS_FRAMETIME;
	offsetwhole = (int)offset;

	cent->dl_backlerp += offset;


	if ( cent->dl_backlerp > 1 ) {                     // we're moving on to the next frame
		cent->dl_oldframe   = cent->dl_oldframe + (int)cent->dl_backlerp;
		cent->dl_frame      = cent->dl_oldframe + 1;
		if ( cent->dl_oldframe >= stringlength ) {
			cent->dl_oldframe = ( cent->dl_oldframe ) % stringlength;
			if ( cent->dl_oldframe < 3 && cent->dl_sound ) { // < 3 so if an alarm comes back into the pvs it will only start a sound if it's going to be closely synced with the light, otherwise wait till the next cycle
				trap_S_StartSound( NULL, cent->currentState.number, CHAN_AUTO, cgs.gameSounds[cent->dl_sound] );
			}
		}

		if ( cent->dl_frame >= stringlength ) {
			cent->dl_frame = ( cent->dl_frame ) % stringlength;
		}

		cent->dl_backlerp = cent->dl_backlerp - (int)cent->dl_backlerp;
	}


	lastch = cent->dl_stylestring[cent->dl_oldframe] - 'a';
	nextch = cent->dl_stylestring[cent->dl_frame] - 'a';

	lightval = ( lastch * ( 1.0 - cent->dl_backlerp ) ) + ( nextch * cent->dl_backlerp );

	lightval = ( lightval * ( 1000.0f / 24.0f ) ) - 200.0f;  // they want 'm' as the "middle" value as 300

	lightval = max( 0.0f,    lightval );
	lightval = min( 1000.0f, lightval );

	cl = cent->currentState.constantLight;
	r = cl & 255;
	g = ( cl >> 8 ) & 255;
	b = ( cl >> 16 ) & 255;

	trap_R_AddLightToScene( cent->lerpOrigin, lightval, (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 0 ); // overdraw forced to 0 for now
}


void CG_GetWindVector( vec3_t dir ); // JPW NERVE

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( centity_t *cent ) {
	static vec3_t dir;
	// update sound origins
	CG_SetEntitySoundPosition( cent );

	// add loop sound
	if ( cent->currentState.loopSound ) {
		//----(SA)	hmm, the above (CG_SetEntitySoundPosition()) sets s_entityPosition[entityNum] with a valid
		//			location, but the looping sound for a bmodel will never get it since that sound is
		//			started with the lerpOriging right here. \/ \/	How do looping sounds ever work for bmodels?
		//			Or have they always been broken and we just never used them?

		if ( cent->currentState.eType == ET_SPEAKER ) {
			/*if(cent->currentState.density == 1) {	// NO_PVS
				trap_S_AddRealLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[ cent->currentState.loopSound ] );
			}
			else*/if ( cent->currentState.dmgFlags ) { // range is set
				trap_S_AddRangedLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[ cent->currentState.loopSound ], cent->currentState.dmgFlags );
			} else {
				trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[ cent->currentState.loopSound ], 255 );
			}
		} else if ( cent->currentState.solid == SOLID_BMODEL )   {
			vec3_t origin;
			float   *v;

			v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
			VectorAdd( cent->lerpOrigin, v, origin );
			trap_S_AddLoopingSound( cent->currentState.number, origin, vec3_origin, cgs.gameSounds[ cent->currentState.loopSound ], 255 );
		} else {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.gameSounds[ cent->currentState.loopSound ], 255 );
		}
	} /*else {
		// stop NO_PVS speakers if they've been turned off
		if(cent->currentState.eType == ET_SPEAKER) {
			if(cent->currentState.density == 1) {
				trap_S_StopLoopingSound(cent->currentState.number);
			}
		}
	}*/


	// constant light glow
	if ( cent->currentState.constantLight ) {
		int cl;
		int i, r, g, b;


		if ( cent->dl_stylestring[0] != 0 ) {  // it's probably a dlight
			CG_AddLightstyle( cent );
		} else
		{
			cl = cent->currentState.constantLight;
			r = cl & 255;
			g = ( cl >> 8 ) & 255;
			b = ( cl >> 16 ) & 255;
			i = ( ( cl >> 24 ) & 255 ) * 4;

			trap_R_AddLightToScene( cent->lerpOrigin, i, (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 0 );
		}
	}

	// Ridah, flaming sounds
	if ( CG_EntOnFire( cent ) ) {
		// play a flame blow sound when moving
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flameBlowSound, (int)( 255.0 * ( 1.0 - fabs( cent->fireRiseDir[2] ) ) ) );
		// play a burning sound when not moving
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flameSound, (int)( 0.3 * 255.0 * ( pow( cent->fireRiseDir[2],2 ) ) ) );
	}


	// DHM - Nerve :: If EF_SMOKING is set, emit smoke
	if ( cgs.gametype >= GT_WOLF && cent->currentState.eFlags & EF_SMOKING ) {
		float rnd = random();

		if ( cent->lastTrailTime < cg.time ) {
			cent->lastTrailTime = cg.time + 100;

// JPW NERVE -- use wind vector for smoke
			CG_GetWindVector( dir );
			VectorScale( dir,20,dir ); // was 75, before that 55
			if ( dir[2] < 10 ) {
				dir[2] += 10;
			}
//			dir[0] = crandom() * 10;
//			dir[1] = crandom() * 10;
//			dir[2] = 10 + rnd * 30;
// jpw
			CG_SmokePuff( cent->lerpOrigin, dir, 15 + ( random() * 10 ),
						  0.3 + rnd, 0.3 + rnd, 0.3 + rnd, 0.4, 1500 + ( rand() % 500 ),
						  cg.time, cg.time + 500, 0, cgs.media.smokePuffShader );
		}
	}
	// dhm - end
// JPW NERVE same thing but for smoking barrels instead of nasty server-side effect from single player
	if ( cgs.gametype >= GT_WOLF && cent->currentState.eFlags & EF_SMOKINGBLACK ) {
		float rnd = random();

		if ( cent->lastTrailTime < cg.time ) {
			cent->lastTrailTime = cg.time + 75;

			CG_GetWindVector( dir );
			VectorScale( dir,50,dir ); // was 75, before that 55
			if ( dir[2] < 50 ) {
				dir[2] += 50;
			}

			CG_SmokePuff( cent->lerpOrigin, dir, 40 + random() * 70, //40+(rnd*40),
						  rnd * 0.1, rnd * 0.1, rnd * 0.1, 1, 2800 + ( rand() % 4000 ), //2500+(random()*1500),
						  cg.time, 0, 0, cgs.media.smokePuffShader );
		}
	}
// jpw



}


/*
==================
CG_General
==================
*/
static void CG_General( centity_t *cent ) {
	refEntity_t ent;
	entityState_t       *s1;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if ( !s1->modelindex ) {
		return;
	}

	memset( &ent, 0, sizeof( ent ) );

	// set frame

	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	if ( ent.frame ) {

		ent.oldframe -= 1;
		ent.backlerp = 1 - cg.frameInterpolation;

		if ( cent->currentState.time ) {
			ent.fadeStartTime = cent->currentState.time;
			ent.fadeEndTime = cent->currentState.time2;
		}

	}

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	ent.hModel = cgs.gameModels[s1->modelindex];

	// player model
	if ( s1->number == cg.snap->ps.clientNum ) {
		ent.renderfx |= RF_THIRD_PERSON;    // only draw from mirrors
	}

	if ( cent->currentState.eType == ET_MG42_BARREL ) {
		// grab angles from first person user or self if not
		// ATVI Wolfenstein Misc #469 - don't track until viewlocked
		if ( cent->currentState.otherEntityNum == cg.snap->ps.clientNum && cg.snap->ps.viewlocked ) {
			AnglesToAxis( cg.predictedPlayerState.viewangles, ent.axis );
		} else {
			AnglesToAxis( cent->lerpAngles, ent.axis );
		}
	} else {
		// convert angles to axis
		AnglesToAxis( cent->lerpAngles, ent.axis );
	}

	// scale gamemodels
	if ( cent->currentState.eType == ET_GAMEMODEL ) {
		VectorScale( ent.axis[0], cent->currentState.angles2[0], ent.axis[0] );
		VectorScale( ent.axis[1], cent->currentState.angles2[1], ent.axis[1] );
		VectorScale( ent.axis[2], cent->currentState.angles2[2], ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;

//----(SA)	testing
		if ( cent->currentState.apos.trType ) {
			ent.reFlags |= REFLAG_ORIENT_LOD;
		}
//----(SA)	end
	}

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );

	memcpy( &cent->refEnt, &ent, sizeof( refEntity_t ) );
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent ) {
	if ( !cent->currentState.clientNum ) {  // FIXME: use something other than clientNum...
		return;     // not auto triggering
	}

	if ( cg.time < cent->miscTime ) {
		return;
	}

	trap_S_StartSound( NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm] );

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}



/*
==============
CG_DrawHoldableSelect

  This, of course, will all change when we've got a hud, but for now it makes the holdable items usable and not bad looking
==============
*/
void CG_DrawHoldableSelect( void ) {
/*
	int		bits;
	int		count;
	int		amount;
	int		i, x, y, w;
	float	*color;
	char	*name;
	gitem_t		*item;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	color = CG_FadeColor( cg.holdableSelectTime, HOLDABLE_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	// showing select clears pickup item display, but not the blend blob
	cg.itemPickupTime = 0;

	//----(SA)	removed

	// count the number of weapons owned
	bits = cg.snap->ps.stats[ STAT_HOLDABLE_ITEM ];
	count = 0;

	for ( i = 1 ; i <= HI_BOOK3; i++ ) {
		if ( bits & ( 1 << i ) ) {
			if(cg.predictedPlayerState.holdable[i])		// don't show ones we're out of
				count++;
		}
	}

	x = 320 - count * 20;
	y = 380;


	for ( i = 1 ; i <= HI_BOOK3 ; i++ ) {
		if ( !( bits & ( 1 << i ) ) ) {
			continue;
		}

		amount = cg.predictedPlayerState.holdable[i];

		if(!amount)
			continue;

		item = BG_FindItemForHoldable(i);
		if(!item)
			continue;

		CG_RegisterItemVisuals(item - bg_itemlist);

		// draw icon
		if(i == HI_WINE) {
			// wine icons have three stages since each bottle has three uses (as opposed to others so far where there's only 1 use)
			int wine = amount;
			if(wine > 3) wine = 3;
			CG_DrawPic( x, y, 32, 32, cg_items[item - bg_itemlist].icons[ 2 - (wine - 1) ] ) ;
		}
		else {
			CG_DrawPic( x, y, 32, 32, cg_items[item - bg_itemlist].icons[0]);
		}

		// draw remaining uses if there's more than one
		if(amount > 1)
			CG_DrawBigStringColor(x, y + 34, va("%d", amount), color);

		// draw selection marker
		if ( i == cg.holdableSelect) {
			CG_DrawPic( x-4, y-4, 40, 40, cgs.media.selectShader );
		}

		x += 40;
	}

	// draw the selected name
	if(cg.holdableSelect) {
		item = BG_FindItemForHoldable(cg.holdableSelect);
		if(item) {
			name = item->pickup_name;
			if ( name ) {
		//----(SA)	trying smaller text
//				w = CG_DrawStrlen( name ) * BIGCHAR_WIDTH;
				w = CG_DrawStrlen( name ) * 10;
				x = ( SCREEN_WIDTH - w ) / 2;
//				CG_DrawBigStringColor(x, y - 22, name, color);
				CG_DrawStringExt2( x, y + 60, name, color, qfalse, qtrue, 10, 10, 0 );
			}
		}
	}

	trap_R_SetColor( NULL );
*/
}


/*
==============
CG_NextItem_f
==============
*/

void CG_NextItem_f( void ) {
/*
	int		i;
	int		original, next;

	if ( !cg.snap ) {
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.holdableSelectTime = cg.time;
	cg.weaponSelectTime = 0;	// (SA) clear weapon selection drawing

	next = original = cg.holdableSelect;

	for ( i = 0 ; i < HI_NUM_HOLDABLE ; i++ ) {
		next++;

		if(next == HI_NUM_HOLDABLE)
			next = 0;

		if(cg.predictedPlayerState.holdable[next]) {	//----(SA)
			break;
		}
	}

	if ( i == HI_NUM_HOLDABLE ) {
		next = original;
	}

	cg.holdableSelect = next;
*/
}

/*
==============
CG_PrevItem_f
==============
*/
void CG_PrevItem_f( void ) {
/*
	int		i;
	int		original, next;

	if ( !cg.snap ) {
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	cg.weaponSelectTime = 0;	// (SA) clear weapon selection drawing
	cg.holdableSelectTime = cg.time;

	next = original = cg.holdableSelect;

	for ( i = 0 ; i < HI_NUM_HOLDABLE ; i++ ) {
		next--;

		if(next == -1)
			next = HI_NUM_HOLDABLE - 1;

		if(cg.predictedPlayerState.holdable[next]) {	//----(SA)
			break;
		}
	}

	if ( i == HI_NUM_HOLDABLE ) {
		next = original;
	}

	cg.holdableSelect = next;
*/
}

/*
==============
CG_Item_f
==============
*/
void CG_Item_f( void ) {
	//int		num;
	//num = atoi( CG_Argv( 1 ) );

	//cg.holdableSelectTime = cg.time;

	//CG_Printf ("Item set to: d\n", num);
}



//----(SA)	added
/*
==============
CG_HoldableUsedupChange
==============
*/
void CG_HoldableUsedupChange( void ) {
	int holding;

	holding = cg.holdableSelect;

	CG_NextItem_f();

	if ( cg.holdableSelect == holding ) {  // nothing else to go to
		cg.holdableSelect = 0;
		cg.weaponSelectTime = 0;
		return;
	}
}
//----(SA)	end



qboolean CG_PlayerSeesItem( playerState_t *ps, entityState_t *item, int atTime, int itemType ) {
	vec3_t vorigin, eorigin, viewa, dir;
	float dot, dist, foo;
	trace_t tr;

	BG_EvaluateTrajectory( &item->pos, atTime, eorigin );

	VectorCopy( ps->origin, vorigin );
	vorigin[2] += ps->viewheight;           // get the view loc up to the viewheight
//	eorigin[2] += 8;						// and subtract the item's offset (that is used to place it on the ground)


	VectorSubtract( vorigin, eorigin, dir );

	dist = VectorNormalize( dir );            // dir is now the direction from the item to the player

	if ( dist > 255 ) {
		return qfalse;                      // only run the remaining stuff on items that are close enough

	}
	// (SA) FIXME: do this without AngleVectors.
	//		It'd be nice if the angle vectors for the player
	//		have already been figured at this point and I can
	//		just pick them up.  (if anybody is storing this somewhere,
	//		for the current frame please let me know so I don't
	//		have to do redundant calcs)
	AngleVectors( ps->viewangles, viewa, 0, 0 );
	dot = DotProduct( viewa, dir );

	// give more range based on distance (the hit area is wider when closer)

//	foo = -0.94f - (dist/255.0f) * 0.057f;	// (ranging from -0.94 to -0.997) (it happened to be a pretty good range)
	foo = -0.94f - ( dist * ( 1.0f / 255.0f ) ) * 0.057f;   // (ranging from -0.94 to -0.997) (it happened to be a pretty good range)

///	Com_Printf("test: if(%f > %f) return qfalse (dot > foo)\n", dot, foo);
	if ( dot > foo ) {
		return qfalse;
	}

	// (SA) okay, everything else is okay, so do a bloody trace. (so coronas on treasure doesn't show through walls) <sigh>
	if ( itemType == IT_TREASURE ) {
		CG_Trace( &tr, vorigin, NULL, NULL, eorigin, -1, MASK_SOLID );

		if ( tr.fraction != 1 ) {
			return qfalse;
		}
	}

	return qtrue;

}


/*
==================
CG_Item
==================
*/
static void CG_Item( centity_t *cent ) {
	refEntity_t ent;
	entityState_t       *es;
	gitem_t             *item;
	float scale;
	qboolean hasStand, highlight;
	float highlightFadeScale = 1.0f;

	es = &cent->currentState;

	hasStand = qfalse;
	highlight = qfalse;

	// (item index is stored in es->modelindex for item)

	if ( es->modelindex >= bg_numItems ) {
		CG_Error( "Bad item index %i on entity", es->modelindex );
	}

	// if set to invisible, skip
	if ( !es->modelindex || ( es->eFlags & EF_NODRAW ) ) {
		return;
	}

	item = &bg_itemlist[ es->modelindex ];

	if ( cg_simpleItems.integer && item->giType != IT_TEAM ) {
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		VectorCopy( cent->lerpOrigin, ent.origin );
		ent.radius = 14;
		ent.customShader = cg_items[es->modelindex].icons[0];
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		trap_R_AddRefEntityToScene( &ent );
		return;
	}

	scale = 0.005 + cent->currentState.number * 0.00001;

	memset( &ent, 0, sizeof( ent ) );

	ent.nonNormalizedAxes = qfalse;

	if ( item->giType == IT_WEAPON ) {
		weaponInfo_t    *weaponInfo = &cg_weapons[item->giTag];

		if ( weaponInfo->standModel ) {
			hasStand = qtrue;
		}

		if ( hasStand ) {                          // first try to put the weapon on it's 'stand'
			refEntity_t stand;

			memset( &stand, 0, sizeof( stand ) );
			stand.hModel = weaponInfo->standModel;

			if ( es->eFlags & EF_SPINNING ) {
				if ( es->groundEntityNum == -1 || !es->groundEntityNum ) { // (SA) spinning with a stand will spin the stand and the attached weap (only when in the air)
					VectorCopy( cg.autoAnglesSlow, cent->lerpAngles );
					VectorCopy( cg.autoAnglesSlow, cent->lastLerpAngles );
				} else {
					VectorCopy( cent->lastLerpAngles, cent->lerpAngles );   // make a tossed weapon sit on the ground in a position that matches how it was yawed
				}
			}

			AnglesToAxis( cent->lerpAngles, stand.axis );
			VectorCopy( cent->lerpOrigin, stand.origin );

			// scale the stand to match the weapon scale ( the weapon will also be scaled inside CG_PositionEntityOnTag() )
			VectorScale( stand.axis[0], 1.5, stand.axis[0] );
			VectorScale( stand.axis[1], 1.5, stand.axis[1] );
			VectorScale( stand.axis[2], 1.5, stand.axis[2] );

//----(SA)	modified
			if ( cent->currentState.frame ) {
				CG_PositionEntityOnTag( &ent, &stand, va( "tag_stand%d", cent->currentState.frame ), 0, NULL );
			} else {
				CG_PositionEntityOnTag( &ent, &stand, "tag_stand", 0, NULL );
			}
//----(SA)	end

			VectorCopy( ent.origin, ent.oldorigin );
			ent.nonNormalizedAxes = qtrue;

		} else {                                // then default to laying it on it's side
			if ( !cg_items[es->modelindex].models[2] ) {
				cent->lerpAngles[2] += 90;
			}

			AnglesToAxis( cent->lerpAngles, ent.axis );

			// increase the size of the weapons when they are presented as items
			VectorScale( ent.axis[0], 1.5, ent.axis[0] );
			VectorScale( ent.axis[1], 1.5, ent.axis[1] );
			VectorScale( ent.axis[2], 1.5, ent.axis[2] );
			ent.nonNormalizedAxes = qtrue;

			VectorCopy( cent->lerpOrigin, ent.origin );
			VectorCopy( cent->lerpOrigin, ent.oldorigin );

			if ( es->eFlags & EF_SPINNING ) {  // spinning will override the angles set by a stand
				if ( es->groundEntityNum == -1 || !es->groundEntityNum ) { // (SA) spinning with a stand will spin the stand and the attached weap (only when in the air)
					VectorCopy( cg.autoAnglesSlow, cent->lerpAngles );
					VectorCopy( cg.autoAnglesSlow, cent->lastLerpAngles );
				} else {
					VectorCopy( cent->lastLerpAngles, cent->lerpAngles );   // make a tossed weapon sit on the ground in a position that matches how it was yawed
				}
			}
		}

	} else {
		AnglesToAxis( cent->lerpAngles, ent.axis );
		VectorCopy( cent->lerpOrigin, ent.origin );
		VectorCopy( cent->lerpOrigin, ent.oldorigin );

		if ( es->eFlags & EF_SPINNING ) {  // spinning will override the angles set by a stand
			VectorCopy( cg.autoAnglesSlow, cent->lerpAngles );
			AxisCopy( cg.autoAxisSlow, ent.axis );
		}
	}


	if ( es->modelindex2 ) {   // modelindex2 was specified for the ent, meaning it probably has an alternate model (as opposed to the one in the itemlist)
							   // try to load it first, and if it fails, default to the itemlist model
		ent.hModel = cgs.gameModels[ es->modelindex2 ];
	} else {
		if ( item->giType == IT_WEAPON && cg_items[es->modelindex].models[2] ) { // check if there's a specific model for weapon pickup placement
			ent.hModel = cg_items[es->modelindex].models[2];
		} else if ( item->giType == IT_HEALTH || item->giType == IT_AMMO || item->giType == IT_POWERUP ) {
			if ( es->density < ( 1 << 9 ) ) {  // (10 bits of data transmission for density)
				ent.hModel = cg_items[es->modelindex].models[es->density];  // multi-state powerups store their state in 'density'
			} else {
				ent.hModel = cg_items[es->modelindex].models[0];
			}
		} else {
			ent.hModel = cg_items[es->modelindex].models[0];
		}
	}

	//----(SA)	find midpoint for highlight corona.
	//			Can't do it when item is registered since it wouldn't know about replacement model
	if ( !( cent->usehighlightOrigin ) ) {
		vec3_t mins, maxs, offset;
		int i;

		trap_R_ModelBounds( ent.hModel, mins, maxs );           // get bounds

		for ( i = 0 ; i < 3 ; i++ ) {
			offset[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );  // find object-space center
		}

		VectorCopy( cent->lerpOrigin, cent->highlightOrigin );        // set 'midpoint' to origin

		for ( i = 0 ; i < 3 ; i++ ) {                           // adjust midpoint by offset and orientation
			cent->highlightOrigin[i] += offset[0] * ent.axis[0][i] +
										offset[1] * ent.axis[1][i] +
										offset[2] * ent.axis[2][i];
		}

		cent->usehighlightOrigin = qtrue;
	}

	// items without glow textures need to keep a minimum light value so they are always visible
//	if ( ( item->giType == IT_WEAPON ) || ( item->giType == IT_ARMOR ) ) {
	ent.renderfx |= RF_MINLIGHT;
//	}

	// highlighting items the player looks at
	if ( cg_drawCrosshairPickups.integer ) {


		if ( cg_drawCrosshairPickups.integer == 2 ) {  // '2' is 'force highlights'
			highlight = qtrue;
		}

		if ( CG_PlayerSeesItem( &cg.predictedPlayerState, es, cg.time, item->giType ) ) {
			highlight = qtrue;

			if ( item->giType == IT_TREASURE ) {
				trap_R_AddCoronaToScene( cent->highlightOrigin, 1, 0.85, 0.5, 2, cent->currentState.number, qtrue ); //----(SA)	add corona to treasure
			}
		} else {
			if ( item->giType == IT_TREASURE ) {
				trap_R_AddCoronaToScene( cent->highlightOrigin, 1, 0.85, 0.5, 2, cent->currentState.number, qfalse );    //----(SA)	"empty corona" for proper fades
			}
		}

//----(SA)	added fixed item highlight fading

		if ( highlight ) {
			if ( !cent->highlighted ) {
				cent->highlighted = qtrue;
				cent->highlightTime = cg.time;
			}
			ent.hilightIntensity = ( ( cg.time - cent->highlightTime ) / 250.0f ) * highlightFadeScale;  // .25 sec to brighten up
		} else {
			if ( cent->highlighted ) {
				cent->highlighted = qfalse;
				cent->highlightTime = cg.time;
			}
			ent.hilightIntensity = 1.0f - ( ( cg.time - cent->highlightTime ) / 1000.0f ) * highlightFadeScale; // 1 sec to dim down (diff in time causes problems if you quickly flip to/away from looking at the item)
		}

		if ( ent.hilightIntensity < 0.25f ) {   // leave a minlight
			ent.hilightIntensity = 0.25f;
		}
		if ( ent.hilightIntensity > 1 ) {
			ent.hilightIntensity = 1.0;
		}
	}
//----(SA)	end


	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

//============================================================================

/*
===============
CG_Smoker
===============
*/
static void CG_Smoker( centity_t *cent ) {
	// this ent has some special setting up
	// time = speed
	// time2 = duration
	// angles2[0] = start_size
	// angles2[1] = end_size
	// angles2[2] = wait
	// dl_intensity = health
	// constantLight = delay
	// origin2 = normal to emit particles along

	if ( cg.time - cent->highlightTime > cent->currentState.constantLight ) {
		// FIXME: make this framerate independant?
		cent->highlightTime = cg.time;  // fire a particle this frame

		if ( cent->currentState.density == 3 ) { // cannon
			CG_ParticleSmoke( cgs.media.smokePuffShaderdirty, cent );
		} else if ( !( cent->currentState.density ) ) {
			CG_ParticleSmoke( cgs.media.smokePuffShader, cent );
		} else {
			CG_ParticleSmoke( cgs.media.smokePuffShader, cent );
		}
	}

	cent->lastTrailTime = cg.time;  // time we were last received at the client
}

/*
===============
CG_Missile
===============
*/

extern void CG_RocketTrail( centity_t *ent, const weaponInfo_t *wi );

static void CG_Missile( centity_t *cent ) {
	refEntity_t ent;
	entityState_t       *s1;
	const weaponInfo_t      *weapon;

	s1 = &cent->currentState;
	if ( s1->weapon > WP_NUM_WEAPONS ) {
		s1->weapon = 0;
	}
	weapon = &cg_weapons[s1->weapon];

	// calculate the axis
	VectorCopy( s1->angles, cent->lerpAngles );

	// add trails
	if ( cent->currentState.eType == ET_FP_PARTS
		 || cent->currentState.eType == ET_FIRE_COLUMN
		 || cent->currentState.eType == ET_FIRE_COLUMN_SMOKE
		 || cent->currentState.eType == ET_RAMJET ) {
		CG_RocketTrail( cent, NULL );
	} else if ( weapon->missileTrailFunc ) {
		weapon->missileTrailFunc( cent, weapon );
	}

	// add dynamic light
	if ( weapon->missileDlight ) {
		trap_R_AddLightToScene( cent->lerpOrigin, weapon->missileDlight,
								weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2], 0 );
	}

//----(SA)	whoops, didn't mean to check it in with the missile flare

	// add missile sound
	if ( weapon->missileSound ) {
		vec3_t velocity;

		BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, velocity );
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, weapon->missileSound, 255 );
	}

	// DHM - Nerve :: Don't tick until armed
	if ( cgs.gametype >= GT_WOLF && cent->currentState.weapon == WP_DYNAMITE ) {
		if ( cent->currentState.teamNum < 4 ) {
			vec3_t velocity;

			BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, velocity );
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, weapon->spindownSound, 255 );
		}
	}

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

//----(SA) removed plasma gun code as sp5 is taking that spot

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;

	if ( cent->currentState.eType == ET_FP_PARTS ) {
		ent.hModel = cgs.gameModels[cent->currentState.modelindex];
	} else if ( cent->currentState.eType == ET_EXPLO_PART )     {
		ent.hModel = cgs.gameModels[cent->currentState.modelindex];
	} else if ( cent->currentState.eType == ET_FLAMEBARREL ) {
		ent.hModel = cgs.media.flamebarrel;
	} else if ( cent->currentState.eType == ET_FIRE_COLUMN || cent->currentState.eType == ET_FIRE_COLUMN_SMOKE ) {
		// it may have a model sometime in the future
		ent.hModel = 0;
	} else if ( cent->currentState.eType == ET_RAMJET ) {
		ent.hModel = 0;
	}
	// ent.hModel = cgs.gameModels[cent->currentState.modelindex];
	else {
		ent.hModel = weapon->missileModel;
	}
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
		ent.axis[0][2] = 1;
	}

	// spin as it moves
	if ( s1->pos.trType != TR_STATIONARY ) {
		RotateAroundDirection( ent.axis, cg.time / 4 );
	} else {
		RotateAroundDirection( ent.axis, s1->time );
	}

	// Rafael
	// Added this since it may be a propExlosion
	if ( ent.hModel ) {
		// add to refresh list, possibly with quad glow
		CG_AddRefEntityWithPowerups( &ent, s1->powerups, TEAM_FREE, s1, vec3_origin );
	}

}

/*
===============
CG_Bat
===============
*/
static void CG_Bat( centity_t *cent ) {
	CG_ParticleBat( cent );
	trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.batsFlyingLoopSound, 5 );
}

//----(SA)	animation_t struct changed, so changes are to keep this working
static animation_t grabberAnims[] = {
	{"", 0,  6,  6,  1000 / 5,     1000 / 5  },  // (main idle)
	{"", 5,  21, 21, 1000 / 7,     1000 / 7  },  // (random idle)
	{"", 25, 11, 0,  1000 / 15,    1000 / 15 },  // (attack big swipe)
	{"", 35, 16, 0,  1000 / 15,    1000 / 15 },  // (attack small swipe)
	{"", 50, 16, 0,  1000 / 15,    1000 / 15 },  // (attack grab)
	{"", 66, 1,  0,  1000 / 15,    1000 / 15 }   // (starting position)
};

//----(SA)	added
static animation_t footlockerAnims[] = {
	{"", 0,  1,  1,  1000 / 5,     1000 / 5  },  // (main idle)
	{"", 0,  5,  5,  1000 / 5,     1000 / 5  },  // (lock rattle)
	{"", 5,  6,  0,  1000 / 5,     1000 / 5  }   // (break open)
};

//----(SA)	end

// DHM - Nerve :: capture and hold flag

static animation_t multi_flagpoleAnims[] = {
	{"", 0,      1,      0,      1000 / 15,    1000 / 15 },  // (no flags, idle)
	{"", 0,      15,     0,      1000 / 15,    1000 / 15 },  // (axis flag rising)
	{"", 490,    15,     0,      1000 / 15,    1000 / 15 },  // (american flag rising)
	{"", 20,     211,    211,    1000 / 15,    1000 / 15 },  // (axis flag raised)
	{"", 255,    211,    211,    1000 / 15,    1000 / 15 },  // (american flag raised)
	{"", 235,    15,     0,      1000 / 15,    1000 / 15 },  // (axis switching to american)
	{"", 470,    15,     0,      1000 / 15,    1000 / 15 },  // (american switching to axis)
	{"", 510,    15,     0,      1000 / 15,    1000 / 15 },  // (axis flag falling)
	{"", 530,    15,     0,      1000 / 15,    1000 / 15 }   // (american flag falling)
};

// dhm - end

extern void CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale );


/*
==============
CG_TrapSetAnim
==============
*/
static void CG_TrapSetAnim( centity_t *cent, lerpFrame_t *lf, int newAnim ) {
	// transition animation
	lf->animationNumber = cent->currentState.frame;

	if ( 0 ) {
		lf->animation = &grabberAnims[cent->currentState.frame];
	} else {
		lf->animation = &footlockerAnims[cent->currentState.frame];
	}

	// DHM - Nerve :: teamNum specifies which set of animations to use (only 1 exists right now)
	if ( cgs.gametype >= GT_WOLF ) {
		switch ( cent->currentState.teamNum ) {

		case 1:
			lf->animation = &multi_flagpoleAnims[ cent->currentState.frame ];
			break;
		default:
			// Keep what was set above
			break;
		}
	}
	// dhm - end

	lf->animationTime = lf->frameTime + lf->animation->initialLerp;
}

/*
==============
CG_Trap
	// TODO: change from 'trap' to something else.  'trap' is a misnomer.  it's actually used for other stuff too
==============
*/
static void CG_Trap( centity_t *cent ) {
	refEntity_t ent;
	entityState_t       *cs;
	lerpFrame_t         *traplf;

	memset( &ent, 0, sizeof( ent ) );

	cs = &cent->currentState;

	traplf = &cent->lerpFrame;

	// initial setup
	if ( !traplf->oldFrameTime ) {
		traplf->frameTime       =
			traplf->oldFrameTime    = cg.time;

		CG_TrapSetAnim( cent, traplf, cs->frame );

		traplf->frame           =
			traplf->oldFrame        = traplf->animation->firstFrame;
	}

	// transition to new anim if requested
	if ( ( traplf->animationNumber != cs->frame ) || !traplf->animation ) {
		CG_TrapSetAnim( cent, traplf, cs->frame );
	}

	CG_RunLerpFrame( NULL, traplf, 0, 1 );    // use existing lerp code rather than re-writing

	ent.frame       = traplf->frame;
	ent.oldframe    = traplf->oldFrame;
	ent.backlerp    = traplf->backlerp;

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	ent.hModel = cgs.gameModels[cs->modelindex];

	AnglesToAxis( cent->lerpAngles, ent.axis );

	trap_R_AddRefEntityToScene( &ent );

	memcpy( &cent->refEnt, &ent, sizeof( refEntity_t ) );
}
//----(SA)	end


/*
==============
CG_Corona
==============
*/
static void CG_Corona( centity_t *cent ) {
	trace_t tr;
	int r, g, b;
	int dli;
	qboolean visible = qfalse,
			 behind = qfalse,
			 toofar = qfalse;

	float dot, dist;
	vec3_t dir;

	if ( cg_coronas.integer == 0 ) {   // if set to '0' no coronas
		return;
	}

	dli = cent->currentState.dl_intensity;
	r = dli & 255;
	g = ( dli >> 8 ) & 255;
	b = ( dli >> 16 ) & 255;

	// only coronas that are in your PVS are being added

	VectorSubtract( cg.refdef.vieworg, cent->lerpOrigin, dir );

	dist = VectorNormalize2( dir, dir );
	if ( dist > cg_coronafardist.integer ) {   // performance variable cg_coronafardist will keep down super long traces
		toofar = qtrue;
	}

	dot = DotProduct( dir, cg.refdef.viewaxis[0] );
	if ( dot >= -0.6 ) {     // assumes ~90 deg fov	(SA) changed value to 0.6 (screen corner at 90 fov)
		behind = qtrue;     // use the dot to at least do trivial removal of those behind you.
	}
	// yeah, I could calc side planes to clip against, but would that be worth it? (much better than dumb dot>= thing?)

//	CG_Printf("dot: %f\n", dot);

	if ( cg_coronas.integer == 2 ) {   // if set to '2' trace everything
		behind = qfalse;
		toofar = qfalse;
	}


	if ( !behind && !toofar ) {
		CG_Trace( &tr, cg.refdef.vieworg, NULL, NULL, cent->lerpOrigin, -1, MASK_SOLID | CONTENTS_BODY ); // added blockage by players.  not sure how this is going to be since this is their bb, not their model (too much blockage)

		if ( tr.fraction == 1 ) {
			visible = qtrue;
		}

		trap_R_AddCoronaToScene( cent->lerpOrigin, (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, (float)cent->currentState.density / 255.0f, cent->currentState.number, visible );
	}
}


/*
==============
CG_Efx
==============
*/
static void CG_Efx( centity_t *cent ) {
	int i;
	float rnd;
	trace_t trace;
	vec3_t perpvec;
	vec3_t stickPoint;
	float movePerUpdate;

	if ( cent->currentState.eType == ET_EF_TESLA ) {
		rnd = cent->currentState.angles2[0];

		for ( i = 0; i < MAX_TESLA_BOLTS; i++ ) {
			if ( cent->boltTimes[i] < cg.time ) {
				VectorSet( cent->boltLocs[i], crandom(), crandom(), crandom() );
				VectorNormalize2( cent->boltLocs[i], cent->boltLocs[i] );
				VectorMA( cent->currentState.origin2, rnd, cent->boltLocs[i], cent->boltLocs[i] );

				cent->boltTimes[i] = cg.time + rand() % cent->currentState.time2;     // hold this position for ~1 second ('stickytime' value is stored in .time2)

				// cut the bolt short if it collides w/ something
				CG_Trace( &trace, cent->currentState.origin, NULL, NULL, cent->boltLocs[i], -1, MASK_SOLID | CONTENTS_BODY );

				if ( trace.fraction < 1 ) {
					// take damage
					if ( trace.entityNum != ENTITYNUM_WORLD ) {
//						CG_ClientDamage(trace.entityNum, cent->currentState.number, CLDMG_TESLA);
//						cg_entities[trace.entityNum].pe.teslaDamagedTime = cg.time;
					}

					VectorCopy( trace.endpos, cent->boltLocs[i] );

					// store perpendicular vector so end can 'crawl'
					PerpendicularVector( perpvec, trace.plane.normal );

					RotatePointAroundVector( stickPoint, trace.plane.normal, perpvec, crandom() * 360 );

					// scale it so it won't move too far with bolts that have long boltTimer's
					movePerUpdate = 1.0f / (float)( cent->boltTimes[i] - cg.time );

					// move a max of 64 away from the 'original' target location
					VectorScale( stickPoint, movePerUpdate * trace.fraction * 64.0f, cent->boltCrawlDirs[i] );

				} else {
					VectorSet( cent->boltCrawlDirs[i], 0, 0, 0 );
				}

			}
		}

		for ( i = 0; i < MAX_TESLA_BOLTS; i++ ) {

			if ( cent->boltCrawlDirs[0] || cent->boltCrawlDirs[1] || cent->boltCrawlDirs[2] ) {
				VectorMA( cent->boltLocs[i], cent->boltTimes[i] - cg.time, cent->boltCrawlDirs[i], perpvec );
			} else {
				VectorCopy( cent->boltLocs[i], perpvec );
			}

			CG_DynamicLightningBolt(    cgs.media.lightningBoltShader,  // shader
										cent->currentState.origin,      // start
										perpvec,                // end
										cent->currentState.density,     // numBolts
										cent->currentState.frame,       // maxWidth
										qtrue,      // fade
										1.0,        // startAlpha
										0,          // recursion
										i * i * 2 );     // randseed
		}

		// add dlight
		if ( cent->currentState.dl_intensity ) {
			int r, g, b;
			int dli;
			int randomness = cent->currentState.time2;

			if ( ( cg.time / 50 ) % ( randomness + ( cg.time % randomness ) ) == 0 ) {
				dli = cent->currentState.dl_intensity;
				r = dli & 255;
				g = ( dli >> 8 ) & 255;
				b = ( dli >> 16 ) & 255;
				trap_R_AddLightToScene( cent->currentState.origin, cent->currentState.time, (float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 0 );
			}
		}
	} else if ( cent->currentState.eType == ET_EF_SPOTLIGHT )     {

		vec3_t targetpos, normalized_direction, direction;
		float dist, fov = 90;
		vec4_t color = {1, 1, 1, .1};
		int splinetarget = 0;
		char    *cs;

		VectorCopy( cent->currentState.origin2, targetpos );

		splinetarget = cent->overheatTime;

		if ( !splinetarget ) {
			cs = (char *)CG_ConfigString( CS_SPLINES + cent->currentState.density );
			cent->overheatTime = splinetarget = CG_LoadCamera( va( "cameras/%s.camera", cs ) );
			if ( splinetarget != -1 ) {
				trap_startCamera( splinetarget, cg.time );
			}
		} else {
			vec3_t angles;
			if ( splinetarget != -1 ) {
				if ( trap_getCameraInfo( splinetarget, cg.time, &targetpos, &angles, &fov ) ) {

				} else {    // loop
					trap_startCamera( splinetarget, cg.time );
					trap_getCameraInfo( splinetarget, cg.time, &targetpos, &angles, &fov );
				}
			}
		}


		normalized_direction[0] = direction[0] = targetpos[0] - cent->currentState.origin[0];
		normalized_direction[1] = direction[1] = targetpos[1] - cent->currentState.origin[1];
		normalized_direction[2] = direction[2] = targetpos[2] - cent->currentState.origin[2];

		dist = VectorNormalize( normalized_direction );

		if ( dist == 0 ) {
			return;
		}

		CG_Spotlight( cent, color, cent->currentState.origin, normalized_direction, 999, 2048, 10, fov, 0 );
	}
}


//----(SA) adding func_explosive

/*
===============
CG_Explosive
	This is currently almost exactly the same as CG_Mover
	It's split out so that any changes or experiments are
	unattached to anything else.
===============
*/
static void CG_Explosive( centity_t *cent ) {
	refEntity_t ent;
	entityState_t       *s1;

	s1 = &cent->currentState;


	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
//	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin );
//	VectorCopy( ent.origin, cent->lerpOrigin);
	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add to refresh list
	// trap_R_AddRefEntityToScene(&ent);

	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		trap_R_AddRefEntityToScene( &ent );
	} else {
		trap_R_AddRefEntityToScene( &ent );
	}

}

//----(SA) done

// declaration for add bullet particles (might as well stick this one in a .h file I think)
extern void CG_AddBulletParticles( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale );

/*
===============
CG_Mover
===============
*/
static void CG_Mover( centity_t *cent ) {
	refEntity_t ent;
	entityState_t       *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );

	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( cent->lerpOrigin, ent.oldorigin );

	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx = RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = 0;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// Rafael
	//  testing for mike to get movers to scale
	if ( cent->currentState.density == ET_MOVERSCALED ) {
		VectorScale( ent.axis[0], cent->currentState.angles2[0], ent.axis[0] );
		VectorScale( ent.axis[1], cent->currentState.angles2[1], ent.axis[1] );
		VectorScale( ent.axis[2], cent->currentState.angles2[2], ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	}


//----(SA)	added
	if ( cent->currentState.eType == ET_ALARMBOX ) {
		ent.renderfx |= RF_MINLIGHT;
	}
//----(SA)	end


	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		ent.frame = s1->frame;
		trap_R_AddRefEntityToScene( &ent );
		memcpy( &cent->refEnt, &ent, sizeof( refEntity_t ) );
	} else {
		trap_R_AddRefEntityToScene( &ent );
	}

	// add propeller and sfx to me109
	if ( cent->currentState.density == 7 || cent->currentState.density == 8 ) {
		refEntity_t propeller;
		vec3_t angles;

		memset( &propeller, 0, sizeof( propeller ) );
		VectorCopy( ent.lightingOrigin, propeller.lightingOrigin );
		propeller.shadowPlane = ent.shadowPlane;
		propeller.renderfx = ent.renderfx;

		propeller.hModel = propellerModel;

		angles[PITCH] = cg.time % 16;

		AnglesToAxis( angles, propeller.axis );

		CG_PositionRotatedEntityOnTag( &propeller, &ent, "tag_prop" );

		trap_R_AddRefEntityToScene( &propeller );

		if ( cent->currentState.density == 8 ) {
			refEntity_t flash;
			vec3_t angles;

			angles[YAW] = 90;
			angles[ROLL] = random() * 90;

			memset( &flash, 0, sizeof( flash ) );
			flash.renderfx = ent.shadowPlane;
			//flash.hModel = cgs.media.mg42muzzleflash;
			flash.hModel = cgs.media.planemuzzleflash;

			AnglesToAxis( angles, flash.axis );
			CG_PositionRotatedEntityOnTag( &flash, &ent, "tag_gun1" );

			trap_R_AddRefEntityToScene( &flash );
			trap_R_AddLightToScene( flash.origin, 200 + ( rand() & 31 ),1.0, 0.6, 0.23, 0 );

			memset( &flash, 0, sizeof( flash ) );
			flash.renderfx = ent.shadowPlane;
			//flash.hModel = cgs.media.mg42muzzleflash;
			flash.hModel = cgs.media.planemuzzleflash;

			AnglesToAxis( angles, flash.axis );
			CG_PositionRotatedEntityOnTag( &flash, &ent, "tag_gun02" );

			trap_R_AddRefEntityToScene( &flash );
			trap_R_AddLightToScene( flash.origin, 200 + ( rand() & 31 ),1.0, 0.6, 0.23, 0 );
		}
	}

	// alarm box spark effects

	if ( cent->currentState.eType == ET_ALARMBOX ) {
		if ( cent->currentState.frame == 2 ) {    // i'm dead
			if ( rand() % 50 == 1 ) {
				vec3_t angNorm;                 // normalized angles
				VectorNormalize2( cent->lerpAngles, angNorm );
				//		(origin, dir, speed, duration, count, 'randscale')
				CG_AddBulletParticles( cent->lerpOrigin, angNorm, 2, 800, 4, 16.0f );
				trap_S_StartSound( NULL, cent->currentState.number, CHAN_AUTO, cgs.media.sparkSounds[0] );
			}
		}
	}
}

/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( centity_t *cent ) {
	refEntity_t ent;
	entityState_t       *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( s1->pos.trBase, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );

	AxisClear( ent.axis );
	ent.reType = RT_BEAM;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_Portal
===============
*/
static void CG_Portal( centity_t *cent ) {
	refEntity_t ent;
	entityState_t       *s1;

	s1 = &cent->currentState;

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	ByteToDir( s1->eventParm, ent.axis[0] );
	PerpendicularVector( ent.axis[1], ent.axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( vec3_origin, ent.axis[1], ent.axis[1] );

	CrossProduct( ent.axis[0], ent.axis[1], ent.axis[2] );
	ent.reType = RT_PORTALSURFACE;
	ent.frame = s1->frame;      // rotation speed
	ent.skinNum = s1->clientNum / 256.0 * 360;    // roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene( &ent );
}

/*
===============
CG_Prop
===============
*/
static void CG_Prop( centity_t *cent ) {
	refEntity_t ent;
	entityState_t       *s1;
	vec3_t angles;
	float scale;

	s1 = &cent->currentState;

	// create the render entity
	memset( &ent, 0, sizeof( ent ) );

	if ( cg.renderingThirdPerson ) {
		VectorCopy( cent->lerpOrigin, ent.origin );
		VectorCopy( cent->lerpOrigin, ent.oldorigin );

		ent.frame = s1->frame;
		ent.oldframe = ent.frame;
		ent.backlerp = 0;
	} else
	{
		VectorCopy( cg.refdef.vieworg, ent.origin );
		VectorCopy( cg.refdefViewAngles, angles );

		if ( cg.bobcycle & 1 ) {
			scale = -cg.xyspeed;
		} else {
			scale = cg.xyspeed;
		}

		// modify angles from bobbing
		angles[ROLL] += scale * cg.bobfracsin * 0.005;
		angles[YAW] += scale * cg.bobfracsin * 0.01;
		angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

		VectorCopy( angles, cent->lerpAngles );

		ent.frame = s1->frame;
		ent.oldframe = ent.frame;
		ent.backlerp = 0;

		if ( cent->currentState.density ) {
			ent.frame = s1->frame + cent->currentState.density;
			ent.oldframe = ent.frame - 1;
			ent.backlerp = 1 - cg.frameInterpolation;
			ent.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;

			//CG_Printf ("frame %d oldframe %d\n", ent.frame, ent.oldframe);
		} else if ( ent.frame )     {
			ent.oldframe -= 1;
			ent.backlerp = 1 - cg.frameInterpolation;
		} else
		{
			ent.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;
		}
	}

	AnglesToAxis( cent->lerpAngles, ent.axis );

	ent.renderfx |= RF_NOSHADOW;

	// flicker between two skins (FIXME?)
	ent.skinNum = ( cg.time >> 6 ) & 1;

	// get the model, either as a bmodel or a modelindex
	if ( s1->solid == SOLID_BMODEL ) {
		ent.hModel = cgs.inlineDrawModel[s1->modelindex];
	} else {
		ent.hModel = cgs.gameModels[s1->modelindex];
	}

	// add the secondary model
	if ( s1->modelindex2 ) {
		ent.skinNum = 0;
		ent.hModel = cgs.gameModels[s1->modelindex2];
		ent.frame = s1->frame;
		trap_R_AddRefEntityToScene( &ent );
		memcpy( &cent->refEnt, &ent, sizeof( refEntity_t ) );
	} else {
		trap_R_AddRefEntityToScene( &ent );
	}

}


/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t outDeltaAngles ) {
	centity_t   *cent;
	vec3_t oldOrigin, origin, deltaOrigin;
	vec3_t oldAngles, angles, deltaAngles;

	if ( outDeltaAngles ) {
		VectorClear( outDeltaAngles );
	}

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
		VectorCopy( in, out );
		return;
	}

	cent = &cg_entities[ moverNum ];

	if ( cent->currentState.eType != ET_MOVER ) {
		VectorCopy( in, out );
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectory( &cent->currentState.pos, toTime, origin );
	BG_EvaluateTrajectory( &cent->currentState.apos, toTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd( in, deltaOrigin, out );
	if ( outDeltaAngles ) {
		VectorCopy( deltaAngles, outDeltaAngles );
	}

	// FIXME: origin change when on a rotating object
}


/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent ) {
	vec3_t current, next;
	float f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == NULL ) {
		// DHM - Nerve :: FIXME? There are some cases when in Limbo mode during a map restart
		//					that were tripping this error.
		//CG_Error( "CG_InterpolateEntityPosition: cg.nextSnap == NULL" );
		//CG_Printf("CG_InterpolateEntityPosition: cg.nextSnap == NULL");
		return;
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions( centity_t *cent ) {
	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// NERVE - SMF - fix for jittery clients in multiplayer
	if ( cgs.gametype != GT_SINGLE_PLAYER ) {
		// first see if we can interpolate between two snaps for
		// linear extrapolated clients
		if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
			 cent->currentState.number < MAX_CLIENTS ) {
			CG_InterpolateEntityPosition( cent );
			return;
		}
	}
	// -NERVE - SMF

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity ) {
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum,
								   cg.snap->serverTime, cg.time, cent->lerpOrigin, NULL );
	}
}

/*
===============
CG_ProcessEntity
===============
*/
static void CG_ProcessEntity( centity_t *cent ) {
	switch ( cent->currentState.eType ) {
	default:
		CG_Error( "Bad entity type: %i\n", cent->currentState.eType );
		break;
	case ET_CONCUSSIVE_TRIGGER:
	case ET_CAMERA:
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
	case ET_OID_TRIGGER:
	case ET_AI_EFFECT:
	case ET_EXPLOSIVE_INDICATOR:        // NERVE - SMF
		break;
	case ET_SPEAKER:
		CG_Speaker( cent );
		break;
	case ET_GAMEMODEL:
		if ( !cg_drawGamemodels.integer ) {
			break;
		}
	case ET_MG42_BARREL:
	case ET_FOOTLOCKER:
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_CORPSE:
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_ITEM:
		CG_Item( cent );
		break;
	case ET_MISSILE:
	case ET_FLAMEBARREL:
	case ET_FP_PARTS:
	case ET_FIRE_COLUMN:
	case ET_FIRE_COLUMN_SMOKE:
	case ET_EXPLO_PART:
	case ET_RAMJET:
		CG_Missile( cent );
		break;
	case ET_EF_TESLA:
	case ET_EF_SPOTLIGHT:
	case ET_EFFECT3:
		CG_Efx( cent );
		break;
	case ET_EXPLOSIVE:
		CG_Explosive( cent );
		break;
	case ET_TRAP:
		CG_Trap( cent );
		break;
	case ET_ALARMBOX:
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_PROP:
		CG_Prop( cent );
		break;
	case ET_BEAM:
		CG_Beam( cent );
		break;
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_CORONA:
		CG_Corona( cent );
		break;
	case ET_BAT:
		CG_Bat( cent );
		break;
	case ET_SMOKER:
		CG_Smoker( cent );
		break;
	}
}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent ) {
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	cent->processedFrame = cg.clientFrame;

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	// add automatic effects
	CG_EntityEffects( cent );

	// call the appropriate function which will add this entity to the view accordingly
	CG_ProcessEntity( cent );
}

/*
==================
CG_AddEntityToTag
==================
*/
static void CG_AddEntityToTag( centity_t *cent ) {
	entityState_t       *s1;
	centity_t           *centParent;
	entityState_t       *sParent;
	refEntity_t ent;
	char *cs, *token = NULL;
	int i, pi;
	vec3_t ang;

	memset( &ent, 0, sizeof( ent ) );

	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	if ( cent->processedFrame == cg.clientFrame ) {
		// already processed this frame
		return;
	}

	// calculate the current origin
	CG_CalcEntityLerpPositions( cent );

	s1 = &cent->currentState;

	// find us in the list of tagged entities
	sParent = NULL;
	centParent = NULL;
	for ( i = CS_TAGCONNECTS + 1; i < CS_TAGCONNECTS + MAX_TAGCONNECTS; i++ ) {   // NOTE: +1 since G_FindConfigStringIndex() starts at index 1 rather than 0 (not sure why)
		cs = (char *)CG_ConfigString( i );
		token = COM_Parse( &cs );
		if ( !token[0] ) {
			break;
		}
		if ( atoi( token ) == s1->number ) {
			token = COM_Parse( &cs );
			if ( !token[0] ) {
				CG_Error( "CG_EntityTagConnected: missing parameter in configstring" );
			}
			pi = atoi( token );
			if ( pi < 0 || pi >= MAX_GENTITIES ) {
				CG_Error( "CG_EntityTagConnected: parent out of range" );
			}
			centParent = &cg_entities[pi];
			sParent = &( cg_entities[pi].currentState );
			token = COM_Parse( &cs );
			if ( !token[0] ) {
				CG_Error( "CG_EntityTagConnected: missing parameter in configstring" );
			}

			// NOTE: token is now the tag name to attach to

			break;
		}
	}

	if ( !sParent ) {
		CG_Error( "CG_EntityTagConnected: unable to find configstring to perform connection" );
	}

	// if parent isn't visible, then don't draw us
	if ( !centParent->currentValid ) {
		return;
	}

	// make sure all parents are added first
	if ( centParent->processedFrame != cg.clientFrame ) {
		if ( sParent->eFlags & EF_TAGCONNECT ) {
			CG_AddEntityToTag( centParent );
		}
	}

	// if there was a higher ranking parent not added to the scene, then don't add us
	if ( centParent->processedFrame != cg.clientFrame ) {
		return;
	}

	cent->processedFrame = cg.clientFrame;

	// start with default axis
	AnglesToAxis( vec3_origin, ent.axis );

	// get the tag position from parent
	CG_PositionEntityOnTag( &ent, &centParent->refEnt, token, 0, NULL );

	VectorCopy( ent.origin, cent->lerpOrigin );
	// we need to add the child's angles to the tag angles
	if ( !cent->currentState.density ) {  // this entity should rotate with it's parent, but can turn around using it's own angles
		AxisToAngles( ent.axis, ang );
		VectorAdd( cent->lerpAngles, ang, cent->lerpAngles );
	} else {    // face our angles exactly
		BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );
	}

	// add automatic effects
	CG_EntityEffects( cent );

	// call the appropriate function which will add this entity to the view accordingly
	CG_ProcessEntity( cent );
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities( void ) {
	int num;
	centity_t           *cent;
	playerState_t       *ps;
	//int					clcount;

	// set cg.frameInterpolation
	if ( cg.nextSnap ) {
		int delta;

		delta = ( cg.nextSnap->serverTime - cg.snap->serverTime );
		if ( delta == 0 ) {
			cg.frameInterpolation = 0;
		} else {
			cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
		}
	} else {
		cg.frameInterpolation = 0;  // actually, it should never be used, because
									// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
	cg.autoAnglesSlow[0] = 0;
	cg.autoAnglesSlow[1] = ( cg.time & 4095 ) * 360 / 4095.0;
	cg.autoAnglesSlow[2] = 0;

	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;

	AnglesToAxis( cg.autoAnglesSlow, cg.autoAxisSlow );
	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
	CG_AddCEntity( &cg.predictedPlayerEntity );

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

	// add each entity sent over by the server

	// NON TAG-CONNECTED ENTITIES
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		if ( !( cent->currentState.eFlags & EF_TAGCONNECT ) ) {
			CG_AddCEntity( cent );
		}
	}

	// TAG-CONNECTED ENTITIES (connected to NON TAG-CONNECTED ENTITIES)
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
		if ( cent->currentState.eFlags & EF_TAGCONNECT ) {
			CG_AddEntityToTag( cent );
		}
	}

	// Ridah, add the flamethrower sounds
	CG_UpdateFlamethrowerSounds();
}
