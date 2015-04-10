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

// cl.input.c  -- builds an intended movement command to send to the server

#include "client.h"

unsigned frame_msec;
int old_com_frameTime;

/*
===============================================================================

KEY BUTTONS

Continuous button event tracking is complicated by the fact that two different
input sources (say, mouse button 1 and the control key) can both press the
same button, but the button should only be released when both of the
pressing key have been released.

When a key event issues a button command (+forward, +attack, etc), it appends
its key number as argv(1) so it can be matched up with the release.

argv(2) will be set to the time the event happened, which allows exact
control even at low framerates when the down and up events may both get qued
at the same time.

===============================================================================
*/

static kbutton_t kb[NUM_BUTTONS];

void IN_MLookDown( void ) {
	kb[KB_MLOOK].active = qtrue;
}

void IN_MLookUp( void ) {
	kb[KB_MLOOK].active = qfalse;
	if ( !cl_freelook->integer ) {
		IN_CenterView();
	}
}

void IN_KeyDown( kbutton_t *b ) {
	int k;
	char    *c;

	c = Cmd_Argv( 1 );
	if ( c[0] ) {
		k = atoi( c );
	} else {
		k = -1;     // typed manually at the console for continuous down
	}

	if ( k == b->down[0] || k == b->down[1] ) {
		return;     // repeating key
	}

	if ( !b->down[0] ) {
		b->down[0] = k;
	} else if ( !b->down[1] ) {
		b->down[1] = k;
	} else {
		Com_Printf( "Three keys down for a button!\n" );
		return;
	}

	if ( b->active ) {
		return;     // still down
	}

	// save timestamp for partial frame summing
	c = Cmd_Argv( 2 );
	b->downtime = atoi( c );

	b->active = qtrue;
	b->wasPressed = qtrue;
}

void IN_KeyUp( kbutton_t *b ) {
	int k;
	char    *c;
	unsigned uptime;

	c = Cmd_Argv( 1 );
	if ( c[0] ) {
		k = atoi( c );
	} else {
		// typed manually at the console, assume for unsticking, so clear all
		b->down[0] = b->down[1] = 0;
		b->active = qfalse;
		return;
	}

	if ( b->down[0] == k ) {
		b->down[0] = 0;
	} else if ( b->down[1] == k ) {
		b->down[1] = 0;
	} else {
		return;     // key up without coresponding down (menu pass through)
	}
	if ( b->down[0] || b->down[1] ) {
		return;     // some other key is still holding it down
	}

	b->active = qfalse;

	// save timestamp for partial frame summing
	c = Cmd_Argv( 2 );
	uptime = atoi( c );
	if ( uptime ) {
		b->msec += uptime - b->downtime;
	} else {
		b->msec += frame_msec / 2;
	}

	b->active = qfalse;
}



/*
===============
CL_KeyState

Returns the fraction of the frame that the key was down
===============
*/
float CL_KeyState( kbutton_t *key ) {
	float val;
	int msec;

	msec = key->msec;
	key->msec = 0;

	if ( key->active ) {
		// still down
		if ( !key->downtime ) {
			msec = com_frameTime;
		} else {
			msec += com_frameTime - key->downtime;
		}
		key->downtime = com_frameTime;
	}

#if 0
	if ( msec ) {
		Com_Printf( "%i ", msec );
	}
#endif

	val = (float)msec / frame_msec;
	if ( val < 0 ) {
		val = 0;
	}
	if ( val > 1 ) {
		val = 1;
	}

	return val;
}



void IN_UpDown( void ) {IN_KeyDown( &kb[KB_UP] );}
void IN_UpUp( void ) {IN_KeyUp( &kb[KB_UP] );}
void IN_DownDown( void ) {IN_KeyDown( &kb[KB_DOWN] );}
void IN_DownUp( void ) {IN_KeyUp( &kb[KB_DOWN] );}
void IN_LeftDown( void ) {IN_KeyDown( &kb[KB_LEFT] );}
void IN_LeftUp( void ) {IN_KeyUp( &kb[KB_LEFT] );}
void IN_RightDown( void ) {IN_KeyDown( &kb[KB_RIGHT] );}
void IN_RightUp( void ) {IN_KeyUp( &kb[KB_RIGHT] );}
void IN_ForwardDown( void ) {IN_KeyDown( &kb[KB_FORWARD] );}
void IN_ForwardUp( void ) {IN_KeyUp( &kb[KB_FORWARD] );}
void IN_BackDown( void ) {IN_KeyDown( &kb[KB_BACK] );}
void IN_BackUp( void ) {IN_KeyUp( &kb[KB_BACK] );}
void IN_LookupDown( void ) {IN_KeyDown( &kb[KB_LOOKUP] );}
void IN_LookupUp( void ) {IN_KeyUp( &kb[KB_LOOKUP] );}
void IN_LookdownDown( void ) {IN_KeyDown( &kb[KB_LOOKDOWN] );}
void IN_LookdownUp( void ) {IN_KeyUp( &kb[KB_LOOKDOWN] );}
void IN_MoveleftDown( void ) {IN_KeyDown( &kb[KB_MOVELEFT] );}
void IN_MoveleftUp( void ) {IN_KeyUp( &kb[KB_MOVELEFT] );}
void IN_MoverightDown( void ) {IN_KeyDown( &kb[KB_MOVERIGHT] );}
void IN_MoverightUp( void ) {IN_KeyUp( &kb[KB_MOVERIGHT] );}

void IN_SpeedDown( void ) {IN_KeyDown( &kb[KB_SPEED] );}
void IN_SpeedUp( void ) {IN_KeyUp( &kb[KB_SPEED] );}
void IN_StrafeDown( void ) {IN_KeyDown( &kb[KB_STRAFE] );}
void IN_StrafeUp( void ) {IN_KeyUp( &kb[KB_STRAFE] );}

void IN_Button0Down( void ) {IN_KeyDown( &kb[KB_BUTTONS0] );}
void IN_Button0Up( void ) {IN_KeyUp( &kb[KB_BUTTONS0] );}
void IN_Button1Down( void ) {IN_KeyDown( &kb[KB_BUTTONS1] );}
void IN_Button1Up( void ) {IN_KeyUp( &kb[KB_BUTTONS1] );}
void IN_UseItemDown( void ) {IN_KeyDown( &kb[KB_BUTTONS2] );}
void IN_UseItemUp( void ) {IN_KeyUp( &kb[KB_BUTTONS2] );}
void IN_Button3Down( void ) {IN_KeyDown( &kb[KB_BUTTONS3] );}
void IN_Button3Up( void ) {IN_KeyUp( &kb[KB_BUTTONS3] );}
void IN_Button4Down( void ) {IN_KeyDown( &kb[KB_BUTTONS4] );}
void IN_Button4Up( void ) {IN_KeyUp( &kb[KB_BUTTONS4] );}
// void IN_Button5Down(void) {IN_KeyDown(&kb[KB_BUTTONS5]);}
// void IN_Button5Up(void) {IN_KeyUp(&kb[KB_BUTTONS5]);}

// void IN_Button6Down(void) {IN_KeyDown(&kb[KB_BUTTONS6]);}
// void IN_Button6Up(void) {IN_KeyUp(&kb[KB_BUTTONS6]);}

// Rafael activate
void IN_ActivateDown( void ) {IN_KeyDown( &kb[KB_BUTTONS6] );}
void IN_ActivateUp( void ) {IN_KeyUp( &kb[KB_BUTTONS6] );}
// done.

// Rafael Kick
void IN_KickDown( void ) {IN_KeyDown( &kb[KB_KICK] );}
void IN_KickUp( void ) {IN_KeyUp( &kb[KB_KICK] );}
// done.

void IN_SprintDown( void ) {IN_KeyDown( &kb[KB_BUTTONS5] );}
void IN_SprintUp( void ) {IN_KeyUp( &kb[KB_BUTTONS5] );}


// wbuttons (wolf buttons)
void IN_Wbutton0Down( void )  { IN_KeyDown( &kb[KB_WBUTTONS0] );    }   //----(SA) secondary fire button
void IN_Wbutton0Up( void )    { IN_KeyUp( &kb[KB_WBUTTONS0] );  }
void IN_ZoomDown( void )      { IN_KeyDown( &kb[KB_WBUTTONS1] );    }   //----(SA)	zoom key
void IN_ZoomUp( void )        { IN_KeyUp( &kb[KB_WBUTTONS1] );  }
void IN_QuickGrenDown( void ) { IN_KeyDown( &kb[KB_WBUTTONS2] );    }   //----(SA)	"Quickgrenade"
void IN_QuickGrenUp( void )   { IN_KeyUp( &kb[KB_WBUTTONS2] );  }
void IN_ReloadDown( void )    { IN_KeyDown( &kb[KB_WBUTTONS3] );    }   //----(SA)	manual weapon re-load
void IN_ReloadUp( void )      { IN_KeyUp( &kb[KB_WBUTTONS3] );  }
void IN_LeanLeftDown( void )  { IN_KeyDown( &kb[KB_WBUTTONS4] );    }   //----(SA)	lean left
void IN_LeanLeftUp( void )    { IN_KeyUp( &kb[KB_WBUTTONS4] );  }
void IN_LeanRightDown( void ) { IN_KeyDown( &kb[KB_WBUTTONS5] );    }   //----(SA)	lean right
void IN_LeanRightUp( void )   { IN_KeyUp( &kb[KB_WBUTTONS5] );  }

// unused
void IN_Wbutton6Down( void )  { IN_KeyDown( &kb[KB_WBUTTONS6] );    }
void IN_Wbutton6Up( void )    { IN_KeyUp( &kb[KB_WBUTTONS6] );  }
void IN_Wbutton7Down( void )  { IN_KeyDown( &kb[KB_WBUTTONS7] );    }
void IN_Wbutton7Up( void )    { IN_KeyUp( &kb[KB_WBUTTONS7] );  }




void IN_ButtonDown( void ) {
	IN_KeyDown( &kb[KB_BUTTONS1] );
}
void IN_ButtonUp( void ) {
	IN_KeyUp( &kb[KB_BUTTONS1] );
}

void IN_CenterView( void ) {
	cl.viewangles[PITCH] = -SHORT2ANGLE( cl.snap.ps.delta_angles[PITCH] );
}

void IN_Notebook( void ) {
	if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
		Cvar_Set( "cg_youGotMail", "0" ); // clear icon	//----(SA)	added
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_NOTEBOOK );    // startup notebook
	}
}

void IN_Help( void ) {
	if ( cls.state == CA_ACTIVE && !clc.demoplaying ) {
		VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_HELP );        // startup help system
	}
}


//==========================================================================

cvar_t  *cl_upspeed;
cvar_t  *cl_forwardspeed;
cvar_t  *cl_sidespeed;

cvar_t  *cl_yawspeed;
cvar_t  *cl_pitchspeed;

cvar_t  *cl_run;

cvar_t  *cl_anglespeedkey;

cvar_t  *cl_recoilPitch;

/*
================
CL_AdjustAngles

Moves the local angle positions
================
*/
void CL_AdjustAngles( void ) {
	float speed;

	if ( kb[KB_SPEED].active ) {
		speed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		speed = 0.001 * cls.frametime;
	}

	if ( !kb[KB_STRAFE].active ) {
		cl.viewangles[YAW] -= speed * cl_yawspeed->value * CL_KeyState( &kb[KB_RIGHT] );
		cl.viewangles[YAW] += speed * cl_yawspeed->value * CL_KeyState( &kb[KB_LEFT] );
	}

	cl.viewangles[PITCH] -= speed * cl_pitchspeed->value * CL_KeyState( &kb[KB_LOOKUP] );
	cl.viewangles[PITCH] += speed * cl_pitchspeed->value * CL_KeyState( &kb[KB_LOOKDOWN] );
}

/*
================
CL_KeyMove

Sets the usercmd_t based on key states
================
*/
void CL_KeyMove( usercmd_t *cmd ) {
	int movespeed;
	int forward, side, up;
	// Rafael Kick
	int kick;
	// done

	//
	// adjust for speed key / running
	// the walking flag is to keep animations consistant
	// even during acceleration and develeration
	//
	if ( kb[KB_SPEED].active ^ cl_run->integer ) {
		movespeed = 127;
		cmd->buttons &= ~BUTTON_WALKING;
	} else {
		cmd->buttons |= BUTTON_WALKING;
		movespeed = 64;
	}

	forward = 0;
	side = 0;
	up = 0;
	if ( kb[KB_STRAFE].active ) {
		side += movespeed * CL_KeyState( &kb[KB_RIGHT] );
		side -= movespeed * CL_KeyState( &kb[KB_LEFT] );
	}

	side += movespeed * CL_KeyState( &kb[KB_MOVERIGHT] );
	side -= movespeed * CL_KeyState( &kb[KB_MOVELEFT] );

//----(SA)	added
	if ( cmd->buttons & BUTTON_ACTIVATE ) {
		if ( side > 0 ) {
			cmd->wbuttons |= WBUTTON_LEANRIGHT;
		} else if ( side < 0 ) {
			cmd->wbuttons |= WBUTTON_LEANLEFT;
		}

		side = 0;   // disallow the strafe when holding 'activate'
	}
//----(SA)	end

	up += movespeed * CL_KeyState( &kb[KB_UP] );
	up -= movespeed * CL_KeyState( &kb[KB_DOWN] );

	forward += movespeed * CL_KeyState( &kb[KB_FORWARD] );
	forward -= movespeed * CL_KeyState( &kb[KB_BACK] );

	// Rafael Kick
	kick = CL_KeyState( &kb[KB_KICK] );
	// done

	if ( !( cl.snap.ps.persistant[PERS_HWEAPON_USE] ) ) {
		cmd->forwardmove = ClampChar( forward );
		cmd->rightmove = ClampChar( side );
		cmd->upmove = ClampChar( up );

		// Rafael - Kick
		cmd->wolfkick = ClampChar( kick );
		// done

	}
}

/*
=================
CL_MouseEvent
=================
*/
void CL_MouseEvent( int dx, int dy, int time ) {
	if ( cls.keyCatchers & KEYCATCH_UI ) {
		VM_Call( uivm, UI_MOUSE_EVENT, dx, dy );
	} else if ( cls.keyCatchers & KEYCATCH_CGAME ) {
		VM_Call( cgvm, CG_MOUSE_EVENT, dx, dy );
	} else {
		cl.mouseDx[cl.mouseIndex] += dx;
		cl.mouseDy[cl.mouseIndex] += dy;
	}
}

/*
=================
CL_JoystickEvent

Joystick values stay set until changed
=================
*/
void CL_JoystickEvent( int axis, int value, int time ) {
	if ( axis < 0 || axis >= MAX_JOYSTICK_AXIS ) {
		Com_Error( ERR_DROP, "CL_JoystickEvent: bad axis %i", axis );
	}
	cl.joystickAxis[axis] = value;
}

/*
=================
CL_JoystickMove
=================
*/
void CL_JoystickMove( usercmd_t *cmd ) {
	int movespeed;
	float anglespeed;

	if ( kb[KB_SPEED].active ^ cl_run->integer ) {
		movespeed = 2;
	} else {
		movespeed = 1;
		cmd->buttons |= BUTTON_WALKING;
	}

	if ( kb[KB_SPEED].active ) {
		anglespeed = 0.001 * cls.frametime * cl_anglespeedkey->value;
	} else {
		anglespeed = 0.001 * cls.frametime;
	}

#ifdef __MACOS__
	cmd->rightmove = ClampChar( cmd->rightmove + cl.joystickAxis[AXIS_SIDE] );
#else
	if ( !kb[KB_STRAFE].active ) {
		cl.viewangles[YAW] += anglespeed * cl_yawspeed->value * cl.joystickAxis[AXIS_SIDE];
	} else {
		cmd->rightmove = ClampChar( cmd->rightmove + cl.joystickAxis[AXIS_SIDE] );
	}
#endif
	if ( kb[KB_MLOOK].active ) {
		cl.viewangles[PITCH] += anglespeed * cl_pitchspeed->value * cl.joystickAxis[AXIS_FORWARD];
	} else {
		cmd->forwardmove = ClampChar( cmd->forwardmove + cl.joystickAxis[AXIS_FORWARD] );
	}

	cmd->upmove = ClampChar( cmd->upmove + cl.joystickAxis[AXIS_UP] );
}

/*
=================
CL_MouseMove
=================
*/
void CL_MouseMove( usercmd_t *cmd ) {
	float mx, my;
	float accelSensitivity;
	float rate;

	// allow mouse smoothing
	if ( m_filter->integer ) {
		mx = ( cl.mouseDx[0] + cl.mouseDx[1] ) * 0.5;
		my = ( cl.mouseDy[0] + cl.mouseDy[1] ) * 0.5;
	} else {
		mx = cl.mouseDx[cl.mouseIndex];
		my = cl.mouseDy[cl.mouseIndex];
	}
	cl.mouseIndex ^= 1;
	cl.mouseDx[cl.mouseIndex] = 0;
	cl.mouseDy[cl.mouseIndex] = 0;

	rate = sqrt( mx * mx + my * my ) / (float)frame_msec;
	accelSensitivity = cl_sensitivity->value + rate * cl_mouseAccel->value;

	// scale by FOV
	accelSensitivity *= cl.cgameSensitivity;

/*	NERVE - SMF - this has moved to CG_CalcFov to fix zoomed-in/out transition movement bug
	if ( cl.snap.ps.stats[STAT_ZOOMED_VIEW] ) {
		if(cl.snap.ps.weapon == WP_SNIPERRIFLE) {
			accelSensitivity *= 0.1;
		}
		else if(cl.snap.ps.weapon == WP_SNOOPERSCOPE) {
			accelSensitivity *= 0.2;
		}
	}
*/
	if ( rate && cl_showMouseRate->integer ) {
		Com_Printf( "%f : %f\n", rate, accelSensitivity );
	}

// Ridah, experimenting with a slow tracking gun

	// Rafael - mg42
	if ( cl.snap.ps.persistant[PERS_HWEAPON_USE] ) {
		mx *= 2.5; //(accelSensitivity * 0.1);
		my *= 2; //(accelSensitivity * 0.075);
	} else
	{
		mx *= accelSensitivity;
		my *= accelSensitivity;
	}

	if ( !mx && !my ) {
		return;
	}

	// add mouse X/Y movement to cmd
	if ( kb[KB_STRAFE].active ) {
		cmd->rightmove = ClampChar( cmd->rightmove + m_side->value * mx );
	} else {
		cl.viewangles[YAW] -= m_yaw->value * mx;
	}

	if ( ( kb[KB_MLOOK].active || cl_freelook->integer ) && !kb[KB_STRAFE].active ) {
		cl.viewangles[PITCH] += m_pitch->value * my;
	} else {
		cmd->forwardmove = ClampChar( cmd->forwardmove - m_forward->value * my );
	}
}


/*
==============
CL_CmdButtons
==============
*/
void CL_CmdButtons( usercmd_t *cmd ) {
	int i;

	//
	// figure button bits
	// send a button bit even if the key was pressed and released in
	// less than a frame
	//
	for ( i = 0 ; i < 7 ; i++ ) {
		if ( kb[KB_BUTTONS0 + i].active || kb[KB_BUTTONS0 + i].wasPressed ) {
			cmd->buttons |= 1 << i;
		}
		kb[KB_BUTTONS0 + i].wasPressed = qfalse;
	}

	for ( i = 0 ; i < 7 ; i++ ) {
		if ( kb[KB_WBUTTONS0 + i].active || kb[KB_WBUTTONS0 + i].wasPressed ) {
			cmd->wbuttons |= 1 << i;
		}
		kb[KB_WBUTTONS0 + i].wasPressed = qfalse;
	}

	if ( cls.keyCatchers ) {
		cmd->buttons |= BUTTON_TALK;
	}

	// allow the game to know if any key at all is
	// currently pressed, even if it isn't bound to anything
	if ( anykeydown && !cls.keyCatchers ) {
		cmd->buttons |= BUTTON_ANY;
	}
}


/*
==============
CL_FinishMove
==============
*/
void CL_FinishMove( usercmd_t *cmd ) {
	int i;

	// copy the state that the cgame is currently sending
	cmd->weapon = cl.cgameUserCmdValue;

	cmd->holdable = cl.cgameUserHoldableValue;  //----(SA)	modified

	// send the current server time so the amount of movement
	// can be determined without allowing cheating
	cmd->serverTime = cl.serverTime;

	for ( i = 0 ; i < 3 ; i++ ) {
		cmd->angles[i] = ANGLE2SHORT( cl.viewangles[i] );
	}
}


/*
=================
CL_CreateCmd
=================
*/
usercmd_t CL_CreateCmd( void ) {
	usercmd_t cmd;
	vec3_t oldAngles;
	float recoilAdd;

	VectorCopy( cl.viewangles, oldAngles );

	// keyboard angle adjustment
	CL_AdjustAngles();

	memset( &cmd, 0, sizeof( cmd ) );

	CL_CmdButtons( &cmd );

	// get basic movement from keyboard
	CL_KeyMove( &cmd );

	// get basic movement from mouse
	CL_MouseMove( &cmd );

	// get basic movement from joystick
	CL_JoystickMove( &cmd );

	// check to make sure the angles haven't wrapped
	if ( cl.viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] + 90;
	} else if ( oldAngles[PITCH] - cl.viewangles[PITCH] > 90 ) {
		cl.viewangles[PITCH] = oldAngles[PITCH] - 90;
	}

	// RF, set the kickAngles so aiming is effected
	recoilAdd = cl_recoilPitch->value;
	if ( fabs( cl.viewangles[PITCH] + recoilAdd ) < 40 ) {
		cl.viewangles[PITCH] += recoilAdd;
	}
	// the recoilPitch has been used, so clear it out
	cl_recoilPitch->value = 0;

	// store out the final values
	CL_FinishMove( &cmd );

	// draw debug graphs of turning for mouse testing
	if ( cl_debugMove->integer ) {
		if ( cl_debugMove->integer == 1 ) {
			SCR_DebugGraph( abs( cl.viewangles[YAW] - oldAngles[YAW] ), 0 );
		}
		if ( cl_debugMove->integer == 2 ) {
			SCR_DebugGraph( abs( cl.viewangles[PITCH] - oldAngles[PITCH] ), 0 );
		}
	}

	cmd.cld = cl.cgameCld;          // NERVE - SMF

	return cmd;
}


/*
=================
CL_CreateNewCommands

Create a new usercmd_t structure for this frame
=================
*/
void CL_CreateNewCommands( void ) {
	usercmd_t   *cmd;
	int cmdNum;

	// no need to create usercmds until we have a gamestate
	if ( cls.state < CA_PRIMED ) {
		return;
	}

	frame_msec = com_frameTime - old_com_frameTime;

	// if running less than 5fps, truncate the extra time to prevent
	// unexpected moves after a hitch
	if ( frame_msec > 200 ) {
		frame_msec = 200;
	}
	old_com_frameTime = com_frameTime;


	// generate a command for this frame
	cl.cmdNumber++;
	cmdNum = cl.cmdNumber & CMD_MASK;
	cl.cmds[cmdNum] = CL_CreateCmd();
	cmd = &cl.cmds[cmdNum];
}

/*
=================
CL_ReadyToSendPacket

Returns qfalse if we are over the maxpackets limit
and should choke back the bandwidth a bit by not sending
a packet this frame.  All the commands will still get
delivered in the next packet, but saving a header and
getting more delta compression will reduce total bandwidth.
=================
*/
qboolean CL_ReadyToSendPacket( void ) {
	int oldPacketNum;
	int delta;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return qfalse;
	}

	// If we are downloading, we send no less than 50ms between packets
	if ( *clc.downloadTempName &&
		 cls.realtime - clc.lastPacketSentTime < 50 ) {
		return qfalse;
	}

	// if we don't have a valid gamestate yet, only send
	// one packet a second
	if ( cls.state != CA_ACTIVE &&
		 cls.state != CA_PRIMED &&
		 !*clc.downloadTempName &&
		 cls.realtime - clc.lastPacketSentTime < 1000 ) {
		return qfalse;
	}

	// send every frame for loopbacks
	if ( clc.netchan.remoteAddress.type == NA_LOOPBACK ) {
		return qtrue;
	}

	// send every frame for LAN
	if ( Sys_IsLANAddress( clc.netchan.remoteAddress ) ) {
		return qtrue;
	}

	// check for exceeding cl_maxpackets
	if ( cl_maxpackets->integer < 15 ) {
		Cvar_Set( "cl_maxpackets", "15" );
	} else if ( cl_maxpackets->integer > 100 ) {
		Cvar_Set( "cl_maxpackets", "100" );
	}
	oldPacketNum = ( clc.netchan.outgoingSequence - 1 ) & PACKET_MASK;
	delta = cls.realtime -  cl.outPackets[ oldPacketNum ].p_realtime;
	if ( delta < 1000 / cl_maxpackets->integer ) {
		// the accumulated commands will go out in the next packet
		return qfalse;
	}

	return qtrue;
}

/*
===================
CL_WritePacket

Create and send the command packet to the server
Including both the reliable commands and the usercmds

During normal gameplay, a client packet will contain something like:

4	sequence number
2	qport
4	serverid
4	acknowledged sequence number
4	clc.serverCommandSequence
<optional reliable commands>
1	clc_move or clc_moveNoDelta
1	command count
<count * usercmds>

===================
*/
void CL_WritePacket( void ) {
	msg_t buf;
	byte data[MAX_MSGLEN];
	int i, j;
	usercmd_t   *cmd, *oldcmd;
	usercmd_t nullcmd;
	int packetNum;
	int oldPacketNum;
	int count, key;

	// don't send anything if playing back a demo
	if ( clc.demoplaying || cls.state == CA_CINEMATIC ) {
		return;
	}

	memset( &nullcmd, 0, sizeof( nullcmd ) );
	oldcmd = &nullcmd;

	MSG_Init( &buf, data, sizeof( data ) );

	MSG_Bitstream( &buf );
	// write the current serverId so the server
	// can tell if this is from the current gameState
	MSG_WriteLong( &buf, cl.serverId );

	// write the last message we received, which can
	// be used for delta compression, and is also used
	// to tell if we dropped a gamestate
	MSG_WriteLong( &buf, clc.serverMessageSequence );

	// write the last reliable message we received
	MSG_WriteLong( &buf, clc.serverCommandSequence );

	// write any unacknowledged clientCommands
	for ( i = clc.reliableAcknowledge + 1 ; i <= clc.reliableSequence ; i++ ) {
		MSG_WriteByte( &buf, clc_clientCommand );
		MSG_WriteLong( &buf, i );
		MSG_WriteString( &buf, clc.reliableCommands[ i & ( MAX_RELIABLE_COMMANDS - 1 ) ] );
	}

	// we want to send all the usercmds that were generated in the last
	// few packet, so even if a couple packets are dropped in a row,
	// all the cmds will make it to the server
	if ( cl_packetdup->integer < 0 ) {
		Cvar_Set( "cl_packetdup", "0" );
	} else if ( cl_packetdup->integer > 5 ) {
		Cvar_Set( "cl_packetdup", "5" );
	}
	oldPacketNum = ( clc.netchan.outgoingSequence - 1 - cl_packetdup->integer ) & PACKET_MASK;
	count = cl.cmdNumber - cl.outPackets[ oldPacketNum ].p_cmdNumber;
	if ( count > MAX_PACKET_USERCMDS ) {
		count = MAX_PACKET_USERCMDS;
		Com_Printf( "MAX_PACKET_USERCMDS\n" );
	}
	if ( count >= 1 ) {
		if ( cl_showSend->integer ) {
			Com_Printf( "(%i)", count );
		}

		// begin a client move command
		if ( cl_nodelta->integer || !cl.snap.valid || clc.demowaiting
			 || clc.serverMessageSequence != cl.snap.messageNum ) {
			MSG_WriteByte( &buf, clc_moveNoDelta );
		} else {
			MSG_WriteByte( &buf, clc_move );
		}

		// write the command count
		MSG_WriteByte( &buf, count );

		// use the checksum feed in the key
		key = clc.checksumFeed;
		// also use the message acknowledge
		key ^= clc.serverMessageSequence;
		// also use the last acknowledged server command in the key
		key ^= Com_HashKey( clc.serverCommands[ clc.serverCommandSequence & ( MAX_RELIABLE_COMMANDS - 1 ) ], 32 );

		// write all the commands, including the predicted command
		for ( i = 0 ; i < count ; i++ ) {
			j = ( cl.cmdNumber - count + i + 1 ) & CMD_MASK;
			cmd = &cl.cmds[j];
			MSG_WriteDeltaUsercmdKey( &buf, key, oldcmd, cmd );
			oldcmd = cmd;
		}
	}

	//
	// deliver the message
	//
	packetNum = clc.netchan.outgoingSequence & PACKET_MASK;
	cl.outPackets[ packetNum ].p_realtime = cls.realtime;
	cl.outPackets[ packetNum ].p_serverTime = oldcmd->serverTime;
	cl.outPackets[ packetNum ].p_cmdNumber = cl.cmdNumber;
	clc.lastPacketSentTime = cls.realtime;

	if ( cl_showSend->integer ) {
		Com_Printf( "%i ", buf.cursize );
	}
//	Netchan_Transmit (&clc.netchan, buf.cursize, buf.data);
	CL_Netchan_Transmit( &clc.netchan, &buf );

	// clients never really should have messages large enough
	// to fragment, but in case they do, fire them all off
	// at once
	while ( clc.netchan.unsentFragments ) {
		CL_Netchan_TransmitNextFragment( &clc.netchan );
	}
}

/*
=================
CL_SendCmd

Called every frame to builds and sends a command packet to the server.
=================
*/
void CL_SendCmd( void ) {
	// don't send any message if not connected
	if ( cls.state < CA_CONNECTED ) {
		return;
	}

	// don't send commands if paused
	if ( com_sv_running->integer && sv_paused->integer && cl_paused->integer ) {
		return;
	}

	// we create commands even if a demo is playing,
	CL_CreateNewCommands();

	// don't send a packet if the last packet was sent too recently
	if ( !CL_ReadyToSendPacket() ) {
		if ( cl_showSend->integer ) {
			Com_Printf( ". " );
		}
		return;
	}

	CL_WritePacket();
}

/*
============
CL_InitInput
============
*/
void CL_InitInput( void ) {
	Cmd_AddCommand( "centerview",IN_CenterView );

	Cmd_AddCommand( "+moveup",IN_UpDown );
	Cmd_AddCommand( "-moveup",IN_UpUp );
	Cmd_AddCommand( "+movedown",IN_DownDown );
	Cmd_AddCommand( "-movedown",IN_DownUp );
	Cmd_AddCommand( "+left",IN_LeftDown );
	Cmd_AddCommand( "-left",IN_LeftUp );
	Cmd_AddCommand( "+right",IN_RightDown );
	Cmd_AddCommand( "-right",IN_RightUp );
	Cmd_AddCommand( "+forward",IN_ForwardDown );
	Cmd_AddCommand( "-forward",IN_ForwardUp );
	Cmd_AddCommand( "+back",IN_BackDown );
	Cmd_AddCommand( "-back",IN_BackUp );
	Cmd_AddCommand( "+lookup", IN_LookupDown );
	Cmd_AddCommand( "-lookup", IN_LookupUp );
	Cmd_AddCommand( "+lookdown", IN_LookdownDown );
	Cmd_AddCommand( "-lookdown", IN_LookdownUp );
	Cmd_AddCommand( "+strafe", IN_StrafeDown );
	Cmd_AddCommand( "-strafe", IN_StrafeUp );
	Cmd_AddCommand( "+moveleft", IN_MoveleftDown );
	Cmd_AddCommand( "-moveleft", IN_MoveleftUp );
	Cmd_AddCommand( "+moveright", IN_MoverightDown );
	Cmd_AddCommand( "-moveright", IN_MoverightUp );
	Cmd_AddCommand( "+speed", IN_SpeedDown );
	Cmd_AddCommand( "-speed", IN_SpeedUp );

	Cmd_AddCommand( "+attack", IN_Button0Down );   // ---- id   (primary firing)
	Cmd_AddCommand( "-attack", IN_Button0Up );
//	Cmd_AddCommand ("+button0", IN_Button0Down);
//	Cmd_AddCommand ("-button0", IN_Button0Up);

	Cmd_AddCommand( "+button1", IN_Button1Down );
	Cmd_AddCommand( "-button1", IN_Button1Up );

	Cmd_AddCommand( "+useitem", IN_UseItemDown );
	Cmd_AddCommand( "-useitem", IN_UseItemUp );

	Cmd_AddCommand( "+salute",   IN_Button3Down ); //----(SA) salute
	Cmd_AddCommand( "-salute",   IN_Button3Up );
//	Cmd_AddCommand ("+button3", IN_Button3Down);
//	Cmd_AddCommand ("-button3", IN_Button3Up);

	Cmd_AddCommand( "+button4", IN_Button4Down );
	Cmd_AddCommand( "-button4", IN_Button4Up );
	//Cmd_AddCommand ("+button5", IN_Button5Down);
	//Cmd_AddCommand ("-button5", IN_Button5Up);

	//Cmd_AddCommand ("+button6", IN_Button6Down);
	//Cmd_AddCommand ("-button6", IN_Button6Up);

	// Rafael Activate
	Cmd_AddCommand( "+activate", IN_ActivateDown );
	Cmd_AddCommand( "-activate", IN_ActivateUp );
	// done.

	// Rafael Kick
	Cmd_AddCommand( "+kick", IN_KickDown );
	Cmd_AddCommand( "-kick", IN_KickUp );
	// done

	Cmd_AddCommand( "+sprint", IN_SprintDown );
	Cmd_AddCommand( "-sprint", IN_SprintUp );


	// wolf buttons
	Cmd_AddCommand( "+attack2",      IN_Wbutton0Down );   //----(SA) secondary firing
	Cmd_AddCommand( "-attack2",      IN_Wbutton0Up );
	Cmd_AddCommand( "+zoom",     IN_ZoomDown );       //
	Cmd_AddCommand( "-zoom",     IN_ZoomUp );
	Cmd_AddCommand( "+quickgren",    IN_QuickGrenDown );  //
	Cmd_AddCommand( "-quickgren",    IN_QuickGrenUp );
	Cmd_AddCommand( "+reload",       IN_ReloadDown );     //
	Cmd_AddCommand( "-reload",       IN_ReloadUp );
	Cmd_AddCommand( "+leanleft", IN_LeanLeftDown );
	Cmd_AddCommand( "-leanleft", IN_LeanLeftUp );
	Cmd_AddCommand( "+leanright",    IN_LeanRightDown );
	Cmd_AddCommand( "-leanright",    IN_LeanRightUp );
	Cmd_AddCommand( "+wbutton6", IN_Wbutton6Down );   //
	Cmd_AddCommand( "-wbutton6", IN_Wbutton6Up );
	Cmd_AddCommand( "+wbutton7", IN_Wbutton7Down );   //
	Cmd_AddCommand( "-wbutton7", IN_Wbutton7Up );
//----(SA) end

	Cmd_AddCommand( "+mlook", IN_MLookDown );
	Cmd_AddCommand( "-mlook", IN_MLookUp );

	Cmd_AddCommand( "notebook",IN_Notebook );
//	Cmd_AddCommand ("help",IN_Help);

	cl_nodelta = Cvar_Get( "cl_nodelta", "0", 0 );
	cl_debugMove = Cvar_Get( "cl_debugMove", "0", 0 );
}


/*
============
CL_ClearKeys
============
*/
void CL_ClearKeys( void ) {
	memset( kb, 0, sizeof( kb ) );
}
