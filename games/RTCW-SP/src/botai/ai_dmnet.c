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
 * name:		ai_dmnet.c
 *
 * desc:		Quake3 bot AI
 *
 *
 *****************************************************************************/

#include "../game/g_local.h"
#include "../game/botlib.h"
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_char.h"
#include "../game/be_ai_chat.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../game/be_ai_weap.h"
#include "../botai/botai.h"
//
#include "ai_main.h"
#include "ai_dmq3.h"
#include "ai_chat.h"
#include "ai_cmd.h"
#include "ai_dmnet.h"
//data file headers
#include "chars.h"           //characteristics
#include "inv.h"         //indexes into the inventory
#include "syn.h"         //synonyms
#include "match.h"           //string matching types and vars

//goal flag, see be_ai_goal.h for the other GFL_*
#define GFL_AIR         16

int numnodeswitches;
char nodeswitch[MAX_NODESWITCHES + 1][144];

#define LOOKAHEAD_DISTANCE      300

/*
==================
BotResetNodeSwitches
==================
*/
void BotResetNodeSwitches( void ) {
	numnodeswitches = 0;
}

/*
==================
BotDumpNodeSwitches
==================
*/
void BotDumpNodeSwitches( bot_state_t *bs ) {
	int i;
	char netname[MAX_NETNAME];

	ClientName( bs->client, netname, sizeof( netname ) );
	BotAI_Print( PRT_MESSAGE, "%s at %1.1f switched more than %d AI nodes\n", netname, trap_AAS_Time(), MAX_NODESWITCHES );
	for ( i = 0; i < numnodeswitches; i++ ) {
		BotAI_Print( PRT_MESSAGE, nodeswitch[i] );
	}
	BotAI_Print( PRT_FATAL, "" );
}

/*
==================
BotRecordNodeSwitch
==================
*/
void BotRecordNodeSwitch( bot_state_t *bs, char *node, char *str ) {
	char netname[MAX_NETNAME];

	ClientName( bs->client, netname, sizeof( netname ) );
	Com_sprintf( nodeswitch[numnodeswitches], 144, "%s at %2.1f entered %s: %s\n", netname, trap_AAS_Time(), node, str );
#ifdef DEBUG
	if ( 0 ) {
		BotAI_Print( PRT_MESSAGE, nodeswitch[numnodeswitches] );
	}
#endif //DEBUG
	numnodeswitches++;
}

/*
==================
BotGetAirGoal
==================
*/
int BotGetAirGoal( bot_state_t *bs, bot_goal_t *goal ) {
	bsp_trace_t bsptrace;
	vec3_t end, mins = {-15, -15, -2}, maxs = {15, 15, 2};
	int areanum;

	//trace up until we hit solid
	VectorCopy( bs->origin, end );
	end[2] += 1000;
	BotAI_Trace( &bsptrace, bs->origin, mins, maxs, end, bs->entitynum, CONTENTS_SOLID | CONTENTS_PLAYERCLIP );
	//trace down until we hit water
	VectorCopy( bsptrace.endpos, end );
	BotAI_Trace( &bsptrace, end, mins, maxs, bs->origin, bs->entitynum, CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA );
	//if we found the water surface
	if ( bsptrace.fraction > 0 ) {
		areanum = BotPointAreaNum( bsptrace.endpos );
		if ( areanum ) {
			VectorCopy( bsptrace.endpos, goal->origin );
			goal->origin[2] -= 2;
			goal->areanum = areanum;
			goal->mins[0] = -15;
			goal->mins[1] = -15;
			goal->mins[2] = -1;
			goal->maxs[0] = 15;
			goal->maxs[1] = 15;
			goal->maxs[2] = 1;
			goal->flags = GFL_AIR;
			goal->number = 0;
			goal->iteminfo = 0;
			goal->entitynum = 0;
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
BotGoForAir
==================
*/
int BotGoForAir( bot_state_t *bs, int tfl, bot_goal_t *ltg, float range ) {
	bot_goal_t goal;

	//if the bot needs air
	if ( bs->lastair_time < trap_AAS_Time() - 6 ) {
		//
#ifdef DEBUG
		//BotAI_Print(PRT_MESSAGE, "going for air\n");
#endif //DEBUG
	   //if we can find an air goal
		if ( BotGetAirGoal( bs, &goal ) ) {
			trap_BotPushGoal( bs->gs, &goal );
			return qtrue;
		} else {
			//get a nearby goal outside the water
			while ( trap_BotChooseNBGItem( bs->gs, bs->origin, bs->inventory, tfl, ltg, range ) ) {
				trap_BotGetTopGoal( bs->gs, &goal );
				//if the goal is not in water
				if ( !( trap_AAS_PointContents( goal.origin ) & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
					return qtrue;
				}
				trap_BotPopGoal( bs->gs );
			}
			trap_BotResetAvoidGoals( bs->gs );
		}
	}
	return qfalse;
}

/*
==================
BotNearbyGoal
==================
*/
int BotNearbyGoal( bot_state_t *bs, int tfl, bot_goal_t *ltg, float range ) {
	int ret;

	if ( BotGoForAir( bs, tfl, ltg, range ) ) {
		return qtrue;
	}
	//
	ret = trap_BotChooseNBGItem( bs->gs, bs->origin, bs->inventory, tfl, ltg, range );
	/*
	if (ret)
	{
		char buf[128];
		//get the goal at the top of the stack
		trap_BotGetTopGoal(bs->gs, &goal);
		trap_BotGoalName(goal.number, buf, sizeof(buf));
		BotAI_Print(PRT_MESSAGE, "%1.1f: new nearby goal %s\n", trap_AAS_Time(), buf);
	}
	*/
	return ret;
}

/*
==================
BotReachedGoal
==================
*/
int BotReachedGoal( bot_state_t *bs, bot_goal_t *goal ) {
	if ( goal->flags & GFL_ITEM ) {
		//if touching the goal
		if ( trap_BotTouchingGoal( bs->origin, goal ) ) {
			return qtrue;
		}
		//if the goal isn't there
		if ( trap_BotItemGoalInVisButNotVisible( bs->entitynum, bs->eye, bs->viewangles, goal ) ) {
			return qtrue;
		}
		//if in the goal area and below or above the goal and not swimming
		if ( bs->areanum == goal->areanum ) {
			if ( bs->origin[0] > goal->origin[0] + goal->mins[0] && bs->origin[0] < goal->origin[0] + goal->maxs[0] ) {
				if ( bs->origin[1] > goal->origin[1] + goal->mins[1] && bs->origin[1] < goal->origin[1] + goal->maxs[1] ) {
					if ( !trap_AAS_Swimming( bs->origin ) ) {
						return qtrue;
					}
				}
			}
		}
	} else if ( goal->flags & GFL_AIR )     {
		//if touching the goal
		if ( trap_BotTouchingGoal( bs->origin, goal ) ) {
			return qtrue;
		}
		//if the bot got air
		if ( bs->lastair_time > trap_AAS_Time() - 1 ) {
			return qtrue;
		}
	} else {
		//if touching the goal
		if ( trap_BotTouchingGoal( bs->origin, goal ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

/*
==================
BotGetItemLongTermGoal
==================
*/
int BotGetItemLongTermGoal( bot_state_t *bs, int tfl, bot_goal_t *goal ) {
	//if the bot has no goal
	if ( !trap_BotGetTopGoal( bs->gs, goal ) ) {
		//BotAI_Print(PRT_MESSAGE, "no ltg on stack\n");
		bs->ltg_time = 0;
	}
	//if the bot touches the current goal
	else if ( BotReachedGoal( bs, goal ) ) {
		BotChooseWeapon( bs );
		bs->ltg_time = 0;
	}
	//if it is time to find a new long term goal
	if ( bs->ltg_time < trap_AAS_Time() ) {
		//pop the current goal from the stack
		trap_BotPopGoal( bs->gs );
		//BotAI_Print(PRT_MESSAGE, "%s: choosing new ltg\n", ClientName(bs->client, netname, sizeof(netname)));
		//choose a new goal
		//BotAI_Print(PRT_MESSAGE, "%6.1f client %d: BotChooseLTGItem\n", trap_AAS_Time(), bs->client);
		if ( trap_BotChooseLTGItem( bs->gs, bs->origin, bs->inventory, tfl ) ) {
			/*
			char buf[128];
			//get the goal at the top of the stack
			trap_BotGetTopGoal(bs->gs, goal);
			trap_BotGoalName(goal->number, buf, sizeof(buf));
			BotAI_Print(PRT_MESSAGE, "%1.1f: new long term goal %s\n", trap_AAS_Time(), buf);
			*/
			bs->ltg_time = trap_AAS_Time() + 20;
		} else { //the bot gets sorta stuck with all the avoid timings, shouldn't happen though
				//
#ifdef DEBUG
			char netname[128];

			BotAI_Print( PRT_MESSAGE, "%s: no valid ltg (probably stuck)\n", ClientName( bs->client, netname, sizeof( netname ) ) );
#endif
			//trap_BotDumpAvoidGoals(bs->gs);
			//reset the avoid goals and the avoid reach
			trap_BotResetAvoidGoals( bs->gs );
			trap_BotResetAvoidReach( bs->ms );
		}
		//get the goal at the top of the stack
		return trap_BotGetTopGoal( bs->gs, goal );
	}
	return qtrue;
}

/*
==================
BotGetLongTermGoal

we could also create a seperate AI node for every long term goal type
however this saves us a lot of code
==================
*/
int BotGetLongTermGoal( bot_state_t *bs, int tfl, int retreat, bot_goal_t *goal ) {
	vec3_t target, dir;
	char netname[MAX_NETNAME];
	char buf[MAX_MESSAGE_SIZE];
	int areanum;
	float croucher;
	aas_entityinfo_t entinfo;
	bot_waypoint_t *wp;

	if ( bs->ltgtype == LTG_TEAMHELP && !retreat ) {
		//check for bot typing status message
		if ( bs->teammessage_time && bs->teammessage_time < trap_AAS_Time() ) {
			BotAI_BotInitialChat( bs, "help_start", EasyClientName( bs->teammate, netname, sizeof( netname ) ), NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->teammessage_time = 0;
		}
		//if trying to help the team mate for more than a minute
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
			bs->ltgtype = 0;
		}
		//if the team mate IS visible for quite some time
		if ( bs->teammatevisible_time < trap_AAS_Time() - 10 ) {
			bs->ltgtype = 0;
		}
		//get entity information of the companion
		BotEntityInfo( bs->teammate, &entinfo );
		//if the team mate is visible
		if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->teammate ) ) {
			//if close just stand still there
			VectorSubtract( entinfo.origin, bs->origin, dir );
			if ( VectorLength( dir ) < 100 ) {
				trap_BotResetAvoidReach( bs->ms );
				return qfalse;
			}
		} else {
			//last time the bot was NOT visible
			bs->teammatevisible_time = trap_AAS_Time();
		}
		//if the entity information is valid (entity in PVS)
		if ( entinfo.valid ) {
			areanum = BotPointAreaNum( entinfo.origin );
			if ( areanum && trap_AAS_AreaReachability( areanum ) ) {
				//update team goal
				bs->teamgoal.entitynum = bs->teammate;
				bs->teamgoal.areanum = areanum;
				VectorCopy( entinfo.origin, bs->teamgoal.origin );
				VectorSet( bs->teamgoal.mins, -8, -8, -8 );
				VectorSet( bs->teamgoal.maxs, 8, 8, 8 );
			}
		}
		memcpy( goal, &bs->teamgoal, sizeof( bot_goal_t ) );
		return qtrue;
	}
	//if the bot accompanies someone
	if ( bs->ltgtype == LTG_TEAMACCOMPANY && !retreat ) {
		//check for bot typing status message
		if ( bs->teammessage_time && bs->teammessage_time < trap_AAS_Time() ) {
			BotAI_BotInitialChat( bs, "accompany_start", EasyClientName( bs->teammate, netname, sizeof( netname ) ), NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->teammessage_time = 0;
		}
		//if accompanying the companion for 3 minutes
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
			BotAI_BotInitialChat( bs, "accompany_stop", EasyClientName( bs->teammate, netname, sizeof( netname ) ), NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->ltgtype = 0;
		}
		//get entity information of the companion
		BotEntityInfo( bs->teammate, &entinfo );
		//if the companion is visible
		if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->teammate ) ) {
			//update visible time
			bs->teammatevisible_time = trap_AAS_Time();
			VectorSubtract( entinfo.origin, bs->origin, dir );
			if ( VectorLength( dir ) < bs->formation_dist ) {
				//check if the bot wants to crouch
				//don't crouch if crouched less than 5 seconds ago
				if ( bs->attackcrouch_time < trap_AAS_Time() - 5 ) {
					croucher = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CROUCHER, 0, 1 );
					if ( random() < bs->thinktime * croucher ) {
						bs->attackcrouch_time = trap_AAS_Time() + 5 + croucher * 15;
					}
				}
				//don't crouch when swimming
				if ( trap_AAS_Swimming( bs->origin ) ) {
					bs->attackcrouch_time = trap_AAS_Time() - 1;
				}
				//if not arrived yet or arived some time ago
				if ( bs->arrive_time < trap_AAS_Time() - 2 ) {
					//if not arrived yet
					if ( !bs->arrive_time ) {
						trap_EA_Gesture( bs->client );
						BotAI_BotInitialChat( bs, "accompany_arrive", EasyClientName( bs->teammate, netname, sizeof( netname ) ), NULL );
						trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
						bs->arrive_time = trap_AAS_Time();
					}
					//if the bot wants to crouch
					else if ( bs->attackcrouch_time > trap_AAS_Time() ) {
						trap_EA_Crouch( bs->client );
					}
					//else do some model taunts
					else if ( random() < bs->thinktime * 0.3 ) {
						//do a gesture :)
						trap_EA_Gesture( bs->client );
					}
				}
				//if just arrived look at the companion
				if ( bs->arrive_time > trap_AAS_Time() - 2 ) {
					VectorSubtract( entinfo.origin, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
					bs->ideal_viewangles[2] *= 0.5;
				}
				//else look strategically around for enemies
				else if ( random() < bs->thinktime * 0.8 ) {
					BotRoamGoal( bs, target );
					VectorSubtract( target, bs->origin, dir );
					vectoangles( dir, bs->ideal_viewangles );
					bs->ideal_viewangles[2] *= 0.5;
				}
				//check if the bot wants to go for air
				if ( BotGoForAir( bs, bs->tfl, &bs->teamgoal, 400 ) ) {
					trap_BotResetLastAvoidReach( bs->ms );
					//get the goal at the top of the stack
					//trap_BotGetTopGoal(bs->gs, &tmpgoal);
					//trap_BotGoalName(tmpgoal.number, buf, 144);
					//BotAI_Print(PRT_MESSAGE, "new nearby goal %s\n", buf);
					//time the bot gets to pick up the nearby goal item
					bs->nbg_time = trap_AAS_Time() + 8;
					AIEnter_Seek_NBG( bs );
					return qfalse;
				}
				//
				trap_BotResetAvoidReach( bs->ms );
				return qfalse;
			}
		}
		//if the entity information is valid (entity in PVS)
		if ( entinfo.valid ) {
			areanum = BotPointAreaNum( entinfo.origin );
			if ( areanum && trap_AAS_AreaReachability( areanum ) ) {
				//update team goal so bot will accompany
				bs->teamgoal.entitynum = bs->teammate;
				bs->teamgoal.areanum = areanum;
				VectorCopy( entinfo.origin, bs->teamgoal.origin );
				VectorSet( bs->teamgoal.mins, -8, -8, -8 );
				VectorSet( bs->teamgoal.maxs, 8, 8, 8 );
			}
		}
		//the goal the bot should go for
		memcpy( goal, &bs->teamgoal, sizeof( bot_goal_t ) );
		//if the companion is NOT visible for too long
		if ( bs->teammatevisible_time < trap_AAS_Time() - 60 ) {
			BotAI_BotInitialChat( bs, "accompany_cannotfind", EasyClientName( bs->teammate, netname, sizeof( netname ) ), NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->ltgtype = 0;
		}
		return qtrue;
	}
	//
	if ( bs->ltgtype == LTG_DEFENDKEYAREA ) {
		if ( trap_AAS_AreaTravelTimeToGoalArea( bs->areanum, bs->origin,
												bs->teamgoal.areanum, TFL_DEFAULT ) > bs->defendaway_range ) {
			bs->defendaway_time = 0;
		}
	}
	//if defending a key area
	if ( bs->ltgtype == LTG_DEFENDKEYAREA && !retreat &&
		 bs->defendaway_time < trap_AAS_Time() ) {
		//check for bot typing status message
		if ( bs->teammessage_time && bs->teammessage_time < trap_AAS_Time() ) {
			trap_BotGoalName( bs->teamgoal.number, buf, sizeof( buf ) );
			BotAI_BotInitialChat( bs, "defend_start", buf, NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->teammessage_time = 0;
		}
		//set the bot goal
		memcpy( goal, &bs->teamgoal, sizeof( bot_goal_t ) );
		//stop after 2 minutes
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
			trap_BotGoalName( bs->teamgoal.number, buf, sizeof( buf ) );
			BotAI_BotInitialChat( bs, "defend_stop", buf, NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->ltgtype = 0;
		}
		//if very close... go away for some time
		VectorSubtract( goal->origin, bs->origin, dir );
		if ( VectorLength( dir ) < 70 ) {
			trap_BotResetAvoidReach( bs->ms );
			bs->defendaway_time = trap_AAS_Time() + 2 + 5 * random();
			bs->defendaway_range = 300;
		}
		return qtrue;
	}
	//going to kill someone
	if ( bs->ltgtype == LTG_KILL && !retreat ) {
		//check for bot typing status message
		if ( bs->teammessage_time && bs->teammessage_time < trap_AAS_Time() ) {
			EasyClientName( bs->teamgoal.entitynum, buf, sizeof( buf ) );
			BotAI_BotInitialChat( bs, "kill_start", buf, NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->teammessage_time = 0;
		}
		//
		if ( bs->lastkilledplayer == bs->teamgoal.entitynum ) {
			EasyClientName( bs->teamgoal.entitynum, buf, sizeof( buf ) );
			BotAI_BotInitialChat( bs, "kill_done", buf, NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->lastkilledplayer = -1;
			bs->ltgtype = 0;
		}
		//
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
			bs->ltgtype = 0;
		}
		//just roam around
		return BotGetItemLongTermGoal( bs, tfl, goal );
	}
	//get an item
	if ( bs->ltgtype == LTG_GETITEM && !retreat ) {
		//check for bot typing status message
		if ( bs->teammessage_time && bs->teammessage_time < trap_AAS_Time() ) {
			trap_BotGoalName( bs->teamgoal.number, buf, sizeof( buf ) );
			BotAI_BotInitialChat( bs, "getitem_start", buf, NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->teammessage_time = 0;
		}
		//set the bot goal
		memcpy( goal, &bs->teamgoal, sizeof( bot_goal_t ) );
		//stop after some time
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
			bs->ltgtype = 0;
		}
		//
		if ( trap_BotItemGoalInVisButNotVisible( bs->entitynum, bs->eye, bs->viewangles, goal ) ) {
			trap_BotGoalName( bs->teamgoal.number, buf, sizeof( buf ) );
			BotAI_BotInitialChat( bs, "getitem_notthere", buf, NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->ltgtype = 0;
		} else if ( BotReachedGoal( bs, goal ) )       {
			trap_BotGoalName( bs->teamgoal.number, buf, sizeof( buf ) );
			BotAI_BotInitialChat( bs, "getitem_gotit", buf, NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->ltgtype = 0;
		}
		return qtrue;
	}
	//if camping somewhere
	if ( ( bs->ltgtype == LTG_CAMP || bs->ltgtype == LTG_CAMPORDER ) && !retreat ) {
		//check for bot typing status message
		if ( bs->teammessage_time && bs->teammessage_time < trap_AAS_Time() ) {
			if ( bs->ltgtype == LTG_CAMPORDER ) {
				BotAI_BotInitialChat( bs, "camp_start", EasyClientName( bs->teammate, netname, sizeof( netname ) ), NULL );
				trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			}
			bs->teammessage_time = 0;
		}
		//set the bot goal
		memcpy( goal, &bs->teamgoal, sizeof( bot_goal_t ) );
		//
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
			if ( bs->ltgtype == LTG_CAMPORDER ) {
				BotAI_BotInitialChat( bs, "camp_stop", NULL );
				trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			}
			bs->ltgtype = 0;
		}
		//if really near the camp spot
		VectorSubtract( goal->origin, bs->origin, dir );
		if ( VectorLength( dir ) < 60 ) {
			//if not arrived yet
			if ( !bs->arrive_time ) {
				if ( bs->ltgtype == LTG_CAMPORDER ) {
					BotAI_BotInitialChat( bs, "camp_arrive", EasyClientName( bs->teammate, netname, sizeof( netname ) ), NULL );
					trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
				}
				bs->arrive_time = trap_AAS_Time();
			}
			//look strategically around for enemies
			if ( random() < bs->thinktime * 0.8 ) {
				BotRoamGoal( bs, target );
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
			}
			//check if the bot wants to crouch
			//don't crouch if crouched less than 5 seconds ago
			if ( bs->attackcrouch_time < trap_AAS_Time() - 5 ) {
				croucher = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_CROUCHER, 0, 1 );
				if ( random() < bs->thinktime * croucher ) {
					bs->attackcrouch_time = trap_AAS_Time() + 5 + croucher * 15;
				}
			}
			//if the bot wants to crouch
			if ( bs->attackcrouch_time > trap_AAS_Time() ) {
				trap_EA_Crouch( bs->client );
			}
			//don't crouch when swimming
			if ( trap_AAS_Swimming( bs->origin ) ) {
				bs->attackcrouch_time = trap_AAS_Time() - 1;
			}
			//make sure the bot is not gonna drown
			if ( trap_PointContents( bs->eye,bs->entitynum ) & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
				if ( bs->ltgtype == LTG_CAMPORDER ) {
					BotAI_BotInitialChat( bs, "camp_stop", NULL );
					trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
				}
				bs->ltgtype = 0;
			}
			//
			if ( bs->camp_range > 0 ) {
				//FIXME: move around a bit
			}
			//
			trap_BotResetAvoidReach( bs->ms );
			return qfalse;
		}
		return qtrue;
	}
	//patrolling along several waypoints
	if ( bs->ltgtype == LTG_PATROL && !retreat ) {
		//check for bot typing status message
		if ( bs->teammessage_time && bs->teammessage_time < trap_AAS_Time() ) {
			strcpy( buf, "" );
			for ( wp = bs->patrolpoints; wp; wp = wp->next ) {
				strcat( buf, wp->name );
				if ( wp->next ) {
					strcat( buf, " to " );
				}
			}
			BotAI_BotInitialChat( bs, "patrol_start", buf, NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->teammessage_time = 0;
		}
		//
		if ( !bs->curpatrolpoint ) {
			bs->ltgtype = 0;
			return qfalse;
		}
		//if the bot touches the current goal
		if ( trap_BotTouchingGoal( bs->origin, &bs->curpatrolpoint->goal ) ) {
			if ( bs->patrolflags & PATROL_BACK ) {
				if ( bs->curpatrolpoint->prev ) {
					bs->curpatrolpoint = bs->curpatrolpoint->prev;
				} else {
					bs->curpatrolpoint = bs->curpatrolpoint->next;
					bs->patrolflags &= ~PATROL_BACK;
				}
			} else {
				if ( bs->curpatrolpoint->next ) {
					bs->curpatrolpoint = bs->curpatrolpoint->next;
				} else {
					bs->curpatrolpoint = bs->curpatrolpoint->prev;
					bs->patrolflags |= PATROL_BACK;
				}
			}
		}
		//stop after 5 minutes
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
			BotAI_BotInitialChat( bs, "patrol_stop", NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->ltgtype = 0;
		}
		if ( !bs->curpatrolpoint ) {
			bs->ltgtype = 0;
			return qfalse;
		}
		memcpy( goal, &bs->curpatrolpoint->goal, sizeof( bot_goal_t ) );
		return qtrue;
	}
#ifdef CTF
	//if going for enemy flag
	if ( bs->ltgtype == LTG_GETFLAG ) {
		//check for bot typing status message
		if ( bs->teammessage_time && bs->teammessage_time < trap_AAS_Time() ) {
			BotAI_BotInitialChat( bs, "captureflag_start", NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->teammessage_time = 0;
		}
		//
		switch ( BotCTFTeam( bs ) ) {
		case CTF_TEAM_RED: *goal = ctf_blueflag; break;
		case CTF_TEAM_BLUE: *goal = ctf_redflag; break;
		default: bs->ltgtype = 0; return qfalse;
		}
		//if touching the flag
		if ( trap_BotTouchingGoal( bs->origin, goal ) ) {
			bs->ltgtype = 0;
		}
		//stop after 3 minutes
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
#ifdef DEBUG
			BotAI_Print( PRT_MESSAGE, "%s: I quit getting the flag\n", ClientName( bs->client, netname, sizeof( netname ) ) );
#endif //DEBUG
			bs->ltgtype = 0;
		}
		return qtrue;
	}
	//if rushing to the base
	if ( bs->ltgtype == LTG_RUSHBASE && bs->rushbaseaway_time < trap_AAS_Time() ) {
		switch ( BotCTFTeam( bs ) ) {
		case CTF_TEAM_RED: *goal = ctf_redflag; break;
		case CTF_TEAM_BLUE: *goal = ctf_blueflag; break;
		default: bs->ltgtype = 0; return qfalse;
		}
		//quit rushing after 2 minutes
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
			bs->ltgtype = 0;
		}
		//if touching the base flag the bot should loose the enemy flag
		if ( trap_BotTouchingGoal( bs->origin, goal ) ) {
			//if the bot is still carrying the enemy flag then the
			//base flag is gone, now just walk near the base a bit
			if ( BotCTFCarryingFlag( bs ) ) {
				trap_BotResetAvoidReach( bs->ms );
				bs->rushbaseaway_time = trap_AAS_Time() + 5 + 10 * random();
				//FIXME: add chat to tell the others to get back the flag
			} else {
				bs->ltgtype = 0;
			}
		}
		return qtrue;
	}
	//returning flag
	if ( bs->ltgtype == LTG_RETURNFLAG ) {
		//check for bot typing status message
		if ( bs->teammessage_time && bs->teammessage_time < trap_AAS_Time() ) {
			EasyClientName( bs->teamgoal.entitynum, buf, sizeof( buf ) );
			BotAI_BotInitialChat( bs, "returnflag_start", buf, NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->teammessage_time = 0;
		}
		//
		if ( bs->teamgoal_time < trap_AAS_Time() ) {
			bs->ltgtype = 0;
		}
		//just roam around
		return BotGetItemLongTermGoal( bs, tfl, goal );
	}
#endif //CTF
	   //normal goal stuff
	return BotGetItemLongTermGoal( bs, tfl, goal );
}

/*
==================
BotLongTermGoal
==================
*/
int BotLongTermGoal( bot_state_t *bs, int tfl, int retreat, bot_goal_t *goal ) {
	aas_entityinfo_t entinfo;
	char teammate[MAX_MESSAGE_SIZE];
	float dist;
	int areanum;
	vec3_t dir;

	//FIXME: also have air long term goals?
	//
	//if the bot is leading someone and not retreating
	if ( bs->lead_time > 0 && !retreat ) {
		if ( bs->lead_time < trap_AAS_Time() ) {
			//FIXME: add chat to tell the team mate that he/she's on his/her own
			bs->lead_time = 0;
			return BotGetLongTermGoal( bs, tfl, retreat, goal );
		}
		//
		if ( bs->leadmessage_time < 0 && -bs->leadmessage_time < trap_AAS_Time() ) {
			BotAI_BotInitialChat( bs, "followme", EasyClientName( bs->lead_teammate, teammate, sizeof( teammate ) ), NULL );
			trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
			bs->leadmessage_time = trap_AAS_Time();
		}
		//get entity information of the companion
		BotEntityInfo( bs->lead_teammate, &entinfo );
		//
		if ( entinfo.valid ) {
			areanum = BotPointAreaNum( entinfo.origin );
			if ( areanum && trap_AAS_AreaReachability( areanum ) ) {
				//update team goal
				bs->lead_teamgoal.entitynum = bs->lead_teammate;
				bs->lead_teamgoal.areanum = areanum;
				VectorCopy( entinfo.origin, bs->lead_teamgoal.origin );
				VectorSet( bs->lead_teamgoal.mins, -8, -8, -8 );
				VectorSet( bs->lead_teamgoal.maxs, 8, 8, 8 );
			}
		}
		//if the team mate is visible
		if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->lead_teammate ) ) {
			bs->leadvisible_time = trap_AAS_Time();
		}
		//if the team mate is not visible for 1 seconds
		if ( bs->leadvisible_time < trap_AAS_Time() - 1 ) {
			bs->leadbackup_time = trap_AAS_Time() + 2;
		}
		//distance towards the team mate
		VectorSubtract( bs->origin, bs->lead_teamgoal.origin, dir );
		dist = VectorLength( dir );
		//if backing up towards the team mate
		if ( bs->leadbackup_time > trap_AAS_Time() ) {
			if ( bs->leadmessage_time < trap_AAS_Time() - 20 ) {
				BotAI_BotInitialChat( bs, "followme", EasyClientName( bs->lead_teammate, teammate, sizeof( teammate ) ), NULL );
				trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
				bs->leadmessage_time = trap_AAS_Time();
			}
			//if very close to the team mate
			if ( dist < 100 ) {
				bs->leadbackup_time = 0;
			}
			//the bot should go back to the team mate
			memcpy( goal, &bs->lead_teamgoal, sizeof( bot_goal_t ) );
			return qtrue;
		} else {
			//if quite distant from the team mate
			if ( dist > 500 ) {
				if ( bs->leadmessage_time < trap_AAS_Time() - 20 ) {
					BotAI_BotInitialChat( bs, "followme", EasyClientName( bs->lead_teammate, teammate, sizeof( teammate ) ), NULL );
					trap_BotEnterChat( bs->cs, bs->client, CHAT_TEAM );
					bs->leadmessage_time = trap_AAS_Time();
				}
				//look at the team mate
				VectorSubtract( entinfo.origin, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
				bs->ideal_viewangles[2] *= 0.5;
				//just wait for the team mate
				return qfalse;
			}
		}
	}
	return BotGetLongTermGoal( bs, tfl, retreat, goal );
}

/*
==================
AIEnter_Intermission
==================
*/
void AIEnter_Intermission( bot_state_t *bs ) {
	BotRecordNodeSwitch( bs, "intermission", "" );
	//reset the bot state
	BotResetState( bs );
	//check for end level chat
	if ( BotChat_EndLevel( bs ) ) {
		trap_BotEnterChat( bs->cs, bs->client, bs->chatto );
	}
	bs->ainode = AINode_Intermission;
}

/*
==================
AINode_Intermission
==================
*/
int AINode_Intermission( bot_state_t *bs ) {
	//if the intermission ended
	if ( !BotIntermission( bs ) ) {
		if ( BotChat_StartLevel( bs ) ) {
			bs->stand_time = trap_AAS_Time() + BotChatTime( bs );
		} else {
			bs->stand_time = trap_AAS_Time() + 2;
		}
		AIEnter_Stand( bs );
	}
	return qtrue;
}

/*
==================
AIEnter_Observer
==================
*/
void AIEnter_Observer( bot_state_t *bs ) {
	BotRecordNodeSwitch( bs, "observer", "" );
	//reset the bot state
	BotResetState( bs );
	bs->ainode = AINode_Observer;
}

/*
==================
AINode_Observer
==================
*/
int AINode_Observer( bot_state_t *bs ) {
	//if the bot left observer mode
	if ( !BotIsObserver( bs ) ) {
		AIEnter_Stand( bs );
	}
	return qtrue;
}

/*
==================
AIEnter_Stand
==================
*/
void AIEnter_Stand( bot_state_t *bs ) {
	BotRecordNodeSwitch( bs, "stand", "" );
	bs->standfindenemy_time = trap_AAS_Time() + 1;
	bs->ainode = AINode_Stand;
}

/*
==================
AINode_Stand
==================
*/
int AINode_Stand( bot_state_t *bs ) {

	//if the bot's health decreased
	if ( bs->lastframe_health > bs->inventory[INVENTORY_HEALTH] ) {
		if ( BotChat_HitTalking( bs ) ) {
			bs->standfindenemy_time = trap_AAS_Time() + BotChatTime( bs ) + 0.1;
			bs->stand_time = trap_AAS_Time() + BotChatTime( bs ) + 0.1;
		}
	}
	if ( bs->standfindenemy_time < trap_AAS_Time() ) {
		if ( BotFindEnemy( bs, -1 ) ) {
			AIEnter_Battle_Fight( bs );
			return qfalse;
		}
		bs->standfindenemy_time = trap_AAS_Time() + 1;
	}
	trap_EA_Talk( bs->client );
	if ( bs->stand_time < trap_AAS_Time() ) {
		trap_BotEnterChat( bs->cs, bs->client, bs->chatto );
		AIEnter_Seek_LTG( bs );
		return qfalse;
	}
	//
	return qtrue;
}

/*
==================
AIEnter_Respawn
==================
*/
void AIEnter_Respawn( bot_state_t *bs ) {
	BotRecordNodeSwitch( bs, "respawn", "" );
	//reset some states
	trap_BotResetMoveState( bs->ms );
	trap_BotResetGoalState( bs->gs );
	trap_BotResetAvoidGoals( bs->gs );
	trap_BotResetAvoidReach( bs->ms );
	//if the bot wants to chat
	if ( BotChat_Death( bs ) ) {
		bs->respawn_time = trap_AAS_Time() + BotChatTime( bs );
		bs->respawnchat_time = trap_AAS_Time();
	} else {
		bs->respawn_time = trap_AAS_Time() + 1 + random();
		bs->respawnchat_time = 0;
	}
	//set respawn state
	bs->respawn_wait = qfalse;
	bs->ainode = AINode_Respawn;
}

/*
==================
AINode_Respawn
==================
*/
int AINode_Respawn( bot_state_t *bs ) {
	if ( bs->respawn_wait ) {
		if ( !BotIsDead( bs ) ) {
			AIEnter_Seek_LTG( bs );
		} else {
			trap_EA_Respawn( bs->client );
		}
	} else if ( bs->respawn_time < trap_AAS_Time() )     {
		//wait until respawned
		bs->respawn_wait = qtrue;
		//elementary action respawn
		trap_EA_Respawn( bs->client );
		//
		if ( bs->respawnchat_time ) {
			trap_BotEnterChat( bs->cs, bs->client, bs->chatto );
			bs->enemy = -1;
		}
	}
	if ( bs->respawnchat_time && bs->respawnchat_time < trap_AAS_Time() - 0.5 ) {
		trap_EA_Talk( bs->client );
	}
	//
	return qtrue;
}

/*
==================
AIEnter_Seek_ActivateEntity
==================
*/
void AIEnter_Seek_ActivateEntity( bot_state_t *bs ) {
	BotRecordNodeSwitch( bs, "activate entity", "" );
	bs->ainode = AINode_Seek_ActivateEntity;
}

/*
==================
AINode_Seek_Activate_Entity
==================
*/
int AINode_Seek_ActivateEntity( bot_state_t *bs ) {
	bot_goal_t *goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;

	if ( BotIsObserver( bs ) ) {
		AIEnter_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_Respawn( bs );
		return qfalse;
	}
	//
	bs->tfl = TFL_DEFAULT;
	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//map specific code
	BotMapScripts( bs );
	//no enemy
	bs->enemy = -1;
	//
	goal = &bs->activategoal;
	//if the bot has no goal
	if ( !goal ) {
		bs->activate_time = 0;
	}
	//if the bot touches the current goal
	else if ( trap_BotTouchingGoal( bs->origin, goal ) ) {
		BotChooseWeapon( bs );
#ifdef DEBUG
		BotAI_Print( PRT_MESSAGE, "touched button or trigger\n" );
#endif //DEBUG
		bs->activate_time = 0;
	}
	//
	if ( bs->activate_time < trap_AAS_Time() ) {
		AIEnter_Seek_NBG( bs );
		return qfalse;
	}
	//initialize the movement state
	BotSetupForMovement( bs );
	//move towards the goal
	trap_BotMoveToGoal( &moveresult, bs->ms, goal, bs->tfl );
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		bs->nbg_time = 0;
	}
	//check if the bot is blocked
	BotAIBlocked( bs, &moveresult, qtrue );
	//
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if ( !( bs->flags & BFL_IDEALVIEWSET ) )       {
		if ( trap_BotMovementViewTarget( bs->ms, goal, bs->tfl, 300, target ) ) {
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
		} else {
			//vectoangles(moveresult.movedir, bs->ideal_viewangles);
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	//if the weapon is used for the bot movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTWEAPON ) {
		bs->weaponnum = moveresult.weapon;
	}
	//if there is an enemy
	if ( BotFindEnemy( bs, -1 ) ) {
		if ( BotWantsToRetreat( bs ) ) {
			//keep the current long term goal and retreat
			AIEnter_Battle_NBG( bs );
		} else {
			trap_BotResetLastAvoidReach( bs->ms );
			//empty the goal stack
			trap_BotEmptyGoalStack( bs->gs );
			//go fight
			AIEnter_Battle_Fight( bs );
		}
	}
	return qtrue;
}

/*
==================
AIEnter_Seek_NBG
==================
*/
void AIEnter_Seek_NBG( bot_state_t *bs ) {
	bot_goal_t goal;
	char buf[144];

	if ( trap_BotGetTopGoal( bs->gs, &goal ) ) {
		trap_BotGoalName( goal.number, buf, 144 );
		BotRecordNodeSwitch( bs, "seek NBG", buf );
	} else {
		BotRecordNodeSwitch( bs, "seek NBG", "no goal" );
	}
	bs->ainode = AINode_Seek_NBG;
}

/*
==================
AINode_Seek_NBG
==================
*/
int AINode_Seek_NBG( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;

	if ( BotIsObserver( bs ) ) {
		AIEnter_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_Respawn( bs );
		return qfalse;
	}
	//
	bs->tfl = TFL_DEFAULT;
	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//
	if ( BotCanAndWantsToRocketJump( bs ) ) {
		bs->tfl |= TFL_ROCKETJUMP;
	}
	//map specific code
	BotMapScripts( bs );
	//no enemy
	bs->enemy = -1;
	//if the bot has no goal
	if ( !trap_BotGetTopGoal( bs->gs, &goal ) ) {
		bs->nbg_time = 0;
	}
	//if the bot touches the current goal
	else if ( BotReachedGoal( bs, &goal ) ) {
		BotChooseWeapon( bs );
		bs->nbg_time = 0;
	}
	//
	if ( bs->nbg_time < trap_AAS_Time() ) {
		//pop the current goal from the stack
		trap_BotPopGoal( bs->gs );
		//check for new nearby items right away
		//NOTE: we canNOT reset the check_time to zero because it would create an endless loop of node switches
		bs->check_time = trap_AAS_Time() + 0.05;
		//go back to seek ltg
		AIEnter_Seek_LTG( bs );
		return qfalse;
	}
	//initialize the movement state
	BotSetupForMovement( bs );
	//move towards the goal
	trap_BotMoveToGoal( &moveresult, bs->ms, &goal, bs->tfl );
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		bs->nbg_time = 0;
	}
	//check if the bot is blocked
	BotAIBlocked( bs, &moveresult, qtrue );
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if ( !( bs->flags & BFL_IDEALVIEWSET ) )       {
		if ( !trap_BotGetSecondGoal( bs->gs, &goal ) ) {
			trap_BotGetTopGoal( bs->gs, &goal );
		}
		if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
		}
		//FIXME: look at cluster portals?
		else {vectoangles( moveresult.movedir, bs->ideal_viewangles );}
		bs->ideal_viewangles[2] *= 0.5;
	}
	//if the weapon is used for the bot movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTWEAPON ) {
		bs->weaponnum = moveresult.weapon;
	}
	//if there is an enemy
	if ( BotFindEnemy( bs, -1 ) ) {
		if ( BotWantsToRetreat( bs ) ) {
			//keep the current long term goal and retreat
			AIEnter_Battle_NBG( bs );
		} else {
			trap_BotResetLastAvoidReach( bs->ms );
			//empty the goal stack
			trap_BotEmptyGoalStack( bs->gs );
			//go fight
			AIEnter_Battle_Fight( bs );
		}
	}
	return qtrue;
}

/*
==================
AIEnter_Seek_LTG
==================
*/
void AIEnter_Seek_LTG( bot_state_t *bs ) {
	bot_goal_t goal;
	char buf[144];

	if ( trap_BotGetTopGoal( bs->gs, &goal ) ) {
		trap_BotGoalName( goal.number, buf, 144 );
		BotRecordNodeSwitch( bs, "seek LTG", buf );
	} else {
		BotRecordNodeSwitch( bs, "seek LTG", "no goal" );
	}
	bs->ainode = AINode_Seek_LTG;
}

/*
==================
AINode_Seek_LTG
==================
*/
int AINode_Seek_LTG( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	int range;
	//char buf[128];
	//bot_goal_t tmpgoal;

	if ( BotIsObserver( bs ) ) {
		AIEnter_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_Respawn( bs );
		return qfalse;
	}
	//
	if ( BotChat_Random( bs ) ) {
		bs->stand_time = trap_AAS_Time() + BotChatTime( bs );
		AIEnter_Stand( bs );
		return qfalse;
	}
	//
	bs->tfl = TFL_DEFAULT;
	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//
	if ( BotCanAndWantsToRocketJump( bs ) ) {
		bs->tfl |= TFL_ROCKETJUMP;
	}
	//map specific code
	BotMapScripts( bs );
	//no enemy
	bs->enemy = -1;
	//
	if ( bs->killedenemy_time > trap_AAS_Time() - 2 ) {
		if ( random() < bs->thinktime * 1 ) {
			trap_EA_Gesture( bs->client );
		}
	}
	//if there is an enemy
	if ( BotFindEnemy( bs, -1 ) ) {
		if ( BotWantsToRetreat( bs ) ) {
			//keep the current long term goal and retreat
			AIEnter_Battle_Retreat( bs );
			return qfalse;
		} else {
			trap_BotResetLastAvoidReach( bs->ms );
			//empty the goal stack
			trap_BotEmptyGoalStack( bs->gs );
			//go fight
			AIEnter_Battle_Fight( bs );
			return qfalse;
		}
	}
#ifdef CTF
	if ( gametype == GT_CTF ) {
		//decide what to do in CTF mode
		BotCTFSeekGoals( bs );
	}
#endif //CTF
	   //get the current long term goal
	if ( !BotLongTermGoal( bs, bs->tfl, qfalse, &goal ) ) {
		return qtrue;
	}
	//check for nearby goals periodicly
	if ( bs->check_time < trap_AAS_Time() ) {
		bs->check_time = trap_AAS_Time() + 0.5;
		//check if the bot wants to camp
		BotWantsToCamp( bs );
		//
		if ( bs->ltgtype == LTG_DEFENDKEYAREA ) {
			range = 400;
		} else { range = 150;}
		//
#ifdef CTF
		//if carrying a flag the bot shouldn't be distracted too much
		if ( BotCTFCarryingFlag( bs ) ) {
			range = 50;
		}
#endif //CTF
	   //
		if ( BotNearbyGoal( bs, bs->tfl, &goal, range ) ) {
			trap_BotResetLastAvoidReach( bs->ms );
			//get the goal at the top of the stack
			//trap_BotGetTopGoal(bs->gs, &tmpgoal);
			//trap_BotGoalName(tmpgoal.number, buf, 144);
			//BotAI_Print(PRT_MESSAGE, "new nearby goal %s\n", buf);
			//time the bot gets to pick up the nearby goal item
			bs->nbg_time = trap_AAS_Time() + 4 + range * 0.01;
			AIEnter_Seek_NBG( bs );
			return qfalse;
		}
	}
	//initialize the movement state
	BotSetupForMovement( bs );
	//move towards the goal
	trap_BotMoveToGoal( &moveresult, bs->ms, &goal, bs->tfl );
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked( bs, &moveresult, qtrue );
	//if the viewangles are used for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	}
	//if waiting for something
	else if ( moveresult.flags & MOVERESULT_WAITING ) {
		if ( random() < bs->thinktime * 0.8 ) {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
	} else if ( !( bs->flags & BFL_IDEALVIEWSET ) )       {
		if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
		}
		//FIXME: look at cluster portals?
		else if ( VectorLength( moveresult.movedir ) ) {
			vectoangles( moveresult.movedir, bs->ideal_viewangles );
		} else if ( random() < bs->thinktime * 0.8 )     {
			BotRoamGoal( bs, target );
			VectorSubtract( target, bs->origin, dir );
			vectoangles( dir, bs->ideal_viewangles );
			bs->ideal_viewangles[2] *= 0.5;
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	//if the weapon is used for the bot movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTWEAPON ) {
		bs->weaponnum = moveresult.weapon;
	}
	//
	return qtrue;
}

/*
==================
AIEnter_Battle_Fight
==================
*/
void AIEnter_Battle_Fight( bot_state_t *bs ) {
	BotRecordNodeSwitch( bs, "battle fight", "" );
	trap_BotResetLastAvoidReach( bs->ms );
	bs->ainode = AINode_Battle_Fight;
}

/*
==================
AINode_Battle_Fight
==================
*/
int AINode_Battle_Fight( bot_state_t *bs ) {
	int areanum;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;

	if ( BotIsObserver( bs ) ) {
		AIEnter_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_Respawn( bs );
		return qfalse;
	}
	//if no enemy
	if ( bs->enemy < 0 ) {
		AIEnter_Seek_LTG( bs );
		return qfalse;
	}
	//
	BotEntityInfo( bs->enemy, &entinfo );
	//if the enemy is dead
	if ( bs->enemydeath_time ) {
		if ( bs->enemydeath_time < trap_AAS_Time() - 1.5 ) {
			bs->enemydeath_time = 0;
			if ( bs->enemysuicide ) {
				BotChat_EnemySuicide( bs );
			}
			if ( bs->lastkilledplayer == bs->enemy && BotChat_Kill( bs ) ) {
				bs->stand_time = trap_AAS_Time() + BotChatTime( bs );
				AIEnter_Stand( bs );
			} else {
				bs->ltg_time = 0;
				AIEnter_Seek_LTG( bs );
			}
			return qfalse;
		}
	} else {
		if ( EntityIsDead( &entinfo ) ) {
			bs->enemydeath_time = trap_AAS_Time();
		}
	}
	//if the enemy is invisible and not shooting the bot looses track easily
	if ( EntityIsInvisible( &entinfo ) && !EntityIsShooting( &entinfo ) ) {
		if ( random() < 0.2 ) {
			AIEnter_Seek_LTG( bs );
			return qfalse;
		}
	}
	//update the reachability area and origin if possible
	areanum = BotPointAreaNum( entinfo.origin );
	if ( areanum && trap_AAS_AreaReachability( areanum ) ) {
		VectorCopy( entinfo.origin, bs->lastenemyorigin );
		bs->lastenemyareanum = areanum;
	}
	//update the attack inventory values
	BotUpdateBattleInventory( bs, bs->enemy );
	//if the bot's health decreased
	if ( bs->lastframe_health > bs->inventory[INVENTORY_HEALTH] ) {
		if ( BotChat_HitNoDeath( bs ) ) {
			bs->stand_time = trap_AAS_Time() + BotChatTime( bs );
			AIEnter_Stand( bs );
			return qfalse;
		}
	}
	//if the bot hit someone
	if ( bs->cur_ps.persistant[PERS_HITS] > bs->lasthitcount ) {
		if ( BotChat_HitNoKill( bs ) ) {
			bs->stand_time = trap_AAS_Time() + BotChatTime( bs );
			AIEnter_Stand( bs );
			return qfalse;
		}
	}
	//if the enemy is not visible
	if ( !BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy ) ) {
		if ( BotWantsToChase( bs ) ) {
			AIEnter_Battle_Chase( bs );
			return qfalse;
		} else {
			AIEnter_Seek_LTG( bs );
			return qfalse;
		}
	}
	//use holdable items
	BotBattleUseItems( bs );
	//
	bs->tfl = TFL_DEFAULT;
	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//
	if ( BotCanAndWantsToRocketJump( bs ) ) {
		bs->tfl |= TFL_ROCKETJUMP;
	}
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//do attack movements
	moveresult = BotAttackMove( bs, bs->tfl );
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked( bs, &moveresult, qfalse );
	//aim at the enemy
	BotAimAtEnemy( bs );
	//attack the enemy if possible
	BotCheckAttack( bs );
	//if the bot wants to retreat
	if ( BotWantsToRetreat( bs ) ) {
		AIEnter_Battle_Retreat( bs );
		return qtrue;
	}
	return qtrue;
}

/*
==================
AIEnter_Battle_Chase
==================
*/
void AIEnter_Battle_Chase( bot_state_t *bs ) {
	BotRecordNodeSwitch( bs, "battle chase", "" );
	bs->chase_time = trap_AAS_Time();
	bs->ainode = AINode_Battle_Chase;
}

/*
==================
AINode_Battle_Chase
==================
*/
int AINode_Battle_Chase( bot_state_t *bs ) {
	bot_goal_t goal;
	vec3_t target, dir;
	bot_moveresult_t moveresult;
	float range;

	if ( BotIsObserver( bs ) ) {
		AIEnter_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_Respawn( bs );
		return qfalse;
	}
	//if no enemy
	if ( bs->enemy < 0 ) {
		AIEnter_Seek_LTG( bs );
		return qfalse;
	}
	//if the enemy is visible
	if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy ) ) {
		AIEnter_Battle_Fight( bs );
		return qfalse;
	}
	//if there is another enemy
	if ( BotFindEnemy( bs, -1 ) ) {
		AIEnter_Battle_Fight( bs );
		return qfalse;
	}
	//there is no last enemy area
	if ( !bs->lastenemyareanum ) {
		AIEnter_Seek_LTG( bs );
		return qfalse;
	}
	//
	bs->tfl = TFL_DEFAULT;
	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//
	if ( BotCanAndWantsToRocketJump( bs ) ) {
		bs->tfl |= TFL_ROCKETJUMP;
	}
	//map specific code
	BotMapScripts( bs );
	//create the chase goal
	goal.entitynum = bs->enemy;
	goal.areanum = bs->lastenemyareanum;
	VectorCopy( bs->lastenemyorigin, goal.origin );
	VectorSet( goal.mins, -8, -8, -8 );
	VectorSet( goal.maxs, 8, 8, 8 );
	//if the last seen enemy spot is reached the enemy could not be found
	if ( trap_BotTouchingGoal( bs->origin, &goal ) ) {
		bs->chase_time = 0;
	}
	//if there's no chase time left
	if ( !bs->chase_time || bs->chase_time < trap_AAS_Time() - 10 ) {
		AIEnter_Seek_LTG( bs );
		return qfalse;
	}
	//check for nearby goals periodicly
	if ( bs->check_time < trap_AAS_Time() ) {
		bs->check_time = trap_AAS_Time() + 1;
		range = 150;
		//
		if ( BotNearbyGoal( bs, bs->tfl, &goal, range ) ) {
			//the bot gets 5 seconds to pick up the nearby goal item
			bs->nbg_time = trap_AAS_Time() + 0.1 * range + 1;
			trap_BotResetLastAvoidReach( bs->ms );
			AIEnter_Battle_NBG( bs );
			return qfalse;
		}
	}
	//
	BotUpdateBattleInventory( bs, bs->enemy );
	//initialize the movement state
	BotSetupForMovement( bs );
	//move towards the goal
	trap_BotMoveToGoal( &moveresult, bs->ms, &goal, bs->tfl );
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked( bs, &moveresult, qfalse );
	//
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEWSET | MOVERESULT_MOVEMENTVIEW | MOVERESULT_SWIMVIEW ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	} else if ( !( bs->flags & BFL_IDEALVIEWSET ) )       {
		if ( bs->chase_time > trap_AAS_Time() - 2 ) {
			BotAimAtEnemy( bs );
		} else {
			if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			}
		}
		bs->ideal_viewangles[2] *= 0.5;
	}
	//if the weapon is used for the bot movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTWEAPON ) {
		bs->weaponnum = moveresult.weapon;
	}
	//if the bot is in the area the enemy was last seen in
	if ( bs->areanum == bs->lastenemyareanum ) {
		bs->chase_time = 0;
	}
	//if the bot wants to retreat (the bot could have been damage during the chase)
	if ( BotWantsToRetreat( bs ) ) {
		AIEnter_Battle_Retreat( bs );
		return qtrue;
	}
	return qtrue;
}

/*
==================
AIEnter_Battle_Retreat
==================
*/
void AIEnter_Battle_Retreat( bot_state_t *bs ) {
	BotRecordNodeSwitch( bs, "battle retreat", "" );
	bs->ainode = AINode_Battle_Retreat;
}

/*
==================
AINode_Battle_Retreat
==================
*/
int AINode_Battle_Retreat( bot_state_t *bs ) {
	bot_goal_t goal;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	vec3_t target, dir;
	float attack_skill, range;
	int areanum;

	if ( BotIsObserver( bs ) ) {
		AIEnter_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_Respawn( bs );
		return qfalse;
	}
	//if no enemy
	if ( bs->enemy < 0 ) {
		AIEnter_Seek_LTG( bs );
		return qfalse;
	}
	//
	BotEntityInfo( bs->enemy, &entinfo );
	if ( EntityIsDead( &entinfo ) ) {
		AIEnter_Seek_LTG( bs );
		return qfalse;
	}
	//
	bs->tfl = TFL_DEFAULT;
	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//map specific code
	BotMapScripts( bs );
	//update the attack inventory values
	BotUpdateBattleInventory( bs, bs->enemy );
	//if the bot doesn't want to retreat anymore... probably picked up some nice items
	if ( BotWantsToChase( bs ) ) {
		//empty the goal stack, when chasing, only the enemy is the goal
		trap_BotEmptyGoalStack( bs->gs );
		//go chase the enemy
		AIEnter_Battle_Chase( bs );
		return qfalse;
	}
	//update the last time the enemy was visible
	if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy ) ) {
		bs->enemyvisible_time = trap_AAS_Time();
		//update the reachability area and origin if possible
		areanum = BotPointAreaNum( entinfo.origin );
		if ( areanum && trap_AAS_AreaReachability( areanum ) ) {
			VectorCopy( entinfo.origin, bs->lastenemyorigin );
			bs->lastenemyareanum = areanum;
		}
	}
	//if the enemy is NOT visible for 4 seconds
	if ( bs->enemyvisible_time < trap_AAS_Time() - 4 ) {
		AIEnter_Seek_LTG( bs );
		return qfalse;
	}
	//else if the enemy is NOT visible
	else if ( bs->enemyvisible_time < trap_AAS_Time() ) {
		//if there is another enemy
		if ( BotFindEnemy( bs, -1 ) ) {
			AIEnter_Battle_Fight( bs );
			return qfalse;
		}
	}
	//
#ifdef CTF
	if ( gametype == GT_CTF ) {
		BotCTFRetreatGoals( bs );
	}
#endif //CTF
	   //use holdable items
	BotBattleUseItems( bs );
	//get the current long term goal while retreating
	if ( !BotLongTermGoal( bs, bs->tfl, qtrue, &goal ) ) {
		return qtrue;
	}
	//check for nearby goals periodicly
	if ( bs->check_time < trap_AAS_Time() ) {
		bs->check_time = trap_AAS_Time() + 1;
		range = 150;
#ifdef CTF
		//if carrying a flag the bot shouldn't be distracted too much
		if ( BotCTFCarryingFlag( bs ) ) {
			range = 100;
		}
#endif //CTF
	   //
		if ( BotNearbyGoal( bs, bs->tfl, &goal, range ) ) {
			trap_BotResetLastAvoidReach( bs->ms );
			//time the bot gets to pick up the nearby goal item
			bs->nbg_time = trap_AAS_Time() + range / 100 + 1;
			AIEnter_Battle_NBG( bs );
			return qfalse;
		}
	}
	//initialize the movement state
	BotSetupForMovement( bs );
	//move towards the goal
	trap_BotMoveToGoal( &moveresult, bs->ms, &goal, bs->tfl );
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->ltg_time = 0;
	}
	//
	BotAIBlocked( bs, &moveresult, qfalse );
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//if the view is fixed for the movement
	if ( moveresult.flags & ( MOVERESULT_MOVEMENTVIEW
							  //|MOVERESULT_SWIMVIEW
							  ) ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	} else if ( !( moveresult.flags & MOVERESULT_MOVEMENTVIEWSET )
				&& !( bs->flags & BFL_IDEALVIEWSET ) ) {
		attack_skill = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_ATTACK_SKILL, 0, 1 );
		//if the bot is skilled anough
		if ( attack_skill > 0.3 ) {
			BotAimAtEnemy( bs );
		} else {
			if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	//if the weapon is used for the bot movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTWEAPON ) {
		bs->weaponnum = moveresult.weapon;
	}
	//attack the enemy if possible
	BotCheckAttack( bs );
	//
	return qtrue;
}

/*
==================
AIEnter_Battle_NBG
==================
*/
void AIEnter_Battle_NBG( bot_state_t *bs ) {
	BotRecordNodeSwitch( bs, "battle NBG", "" );
	bs->ainode = AINode_Battle_NBG;
}

/*
==================
AINode_Battle_NBG
==================
*/
int AINode_Battle_NBG( bot_state_t *bs ) {
	int areanum;
	bot_goal_t goal;
	aas_entityinfo_t entinfo;
	bot_moveresult_t moveresult;
	float attack_skill;
	vec3_t target, dir;

	if ( BotIsObserver( bs ) ) {
		AIEnter_Observer( bs );
		return qfalse;
	}
	//if in the intermission
	if ( BotIntermission( bs ) ) {
		AIEnter_Intermission( bs );
		return qfalse;
	}
	//respawn if dead
	if ( BotIsDead( bs ) ) {
		AIEnter_Respawn( bs );
		return qfalse;
	}
	//if no enemy
	if ( bs->enemy < 0 ) {
		AIEnter_Seek_NBG( bs );
		return qfalse;
	}
	//
	BotEntityInfo( bs->enemy, &entinfo );
	if ( EntityIsDead( &entinfo ) ) {
		AIEnter_Seek_NBG( bs );
		return qfalse;
	}
	//
	bs->tfl = TFL_DEFAULT;
	if ( bot_grapple.integer ) {
		bs->tfl |= TFL_GRAPPLEHOOK;
	}
	//if in lava or slime the bot should be able to get out
	if ( BotInLava( bs ) ) {
		bs->tfl |= TFL_LAVA;
	}
	if ( BotInSlime( bs ) ) {
		bs->tfl |= TFL_SLIME;
	}
	//
	if ( BotCanAndWantsToRocketJump( bs ) ) {
		bs->tfl |= TFL_ROCKETJUMP;
	}
	//map specific code
	BotMapScripts( bs );
	//update the last time the enemy was visible
	if ( BotEntityVisible( bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy ) ) {
		bs->enemyvisible_time = trap_AAS_Time();
		//update the reachability area and origin if possible
		areanum = BotPointAreaNum( entinfo.origin );
		if ( areanum && trap_AAS_AreaReachability( areanum ) ) {
			VectorCopy( entinfo.origin, bs->lastenemyorigin );
			bs->lastenemyareanum = areanum;
		}
	}
	//if the bot has no goal or touches the current goal
	if ( !trap_BotGetTopGoal( bs->gs, &goal ) ) {
		bs->nbg_time = 0;
	} else if ( trap_BotTouchingGoal( bs->origin, &goal ) )       {
		bs->nbg_time = 0;
	}
	//
	if ( bs->nbg_time < trap_AAS_Time() ) {
		//pop the current goal from the stack
		trap_BotPopGoal( bs->gs );
		//if the bot still has a goal
		if ( trap_BotGetTopGoal( bs->gs, &goal ) ) {
			AIEnter_Battle_Retreat( bs );
		} else { AIEnter_Battle_Fight( bs );}
		//
		return qfalse;
	}
	//initialize the movement state
	BotSetupForMovement( bs );
	//move towards the goal
	trap_BotMoveToGoal( &moveresult, bs->ms, &goal, bs->tfl );
	//if the movement failed
	if ( moveresult.failure ) {
		//reset the avoid reach, otherwise bot is stuck in current area
		trap_BotResetAvoidReach( bs->ms );
		//BotAI_Print(PRT_MESSAGE, "movement failure %d\n", moveresult.traveltype);
		bs->nbg_time = 0;
	}
	//
	BotAIBlocked( bs, &moveresult, qfalse );
	//update the attack inventory values
	BotUpdateBattleInventory( bs, bs->enemy );
	//choose the best weapon to fight with
	BotChooseWeapon( bs );
	//if the view is fixed for the movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTVIEW ) {
		VectorCopy( moveresult.ideal_viewangles, bs->ideal_viewangles );
	} else if ( !( moveresult.flags & MOVERESULT_MOVEMENTVIEWSET )
				&& !( bs->flags & BFL_IDEALVIEWSET ) ) {
		attack_skill = trap_Characteristic_BFloat( bs->character, CHARACTERISTIC_ATTACK_SKILL, 0, 1 );
		//if the bot is skilled anough and the enemy is visible
		if ( attack_skill > 0.3 ) {
			//&& BotEntityVisible(bs->entitynum, bs->eye, bs->viewangles, 360, bs->enemy)
			BotAimAtEnemy( bs );
		} else {
			if ( trap_BotMovementViewTarget( bs->ms, &goal, bs->tfl, 300, target ) ) {
				VectorSubtract( target, bs->origin, dir );
				vectoangles( dir, bs->ideal_viewangles );
			} else {
				vectoangles( moveresult.movedir, bs->ideal_viewangles );
			}
			bs->ideal_viewangles[2] *= 0.5;
		}
	}
	//if the weapon is used for the bot movement
	if ( moveresult.flags & MOVERESULT_MOVEMENTWEAPON ) {
		bs->weaponnum = moveresult.weapon;
	}
	//attack the enemy if possible
	BotCheckAttack( bs );
	//
	return qtrue;
}
