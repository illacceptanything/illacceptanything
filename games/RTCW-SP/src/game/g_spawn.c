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

/*
 * name:		g_spawn.c
 *
 * desc:
 *
*/

#include "g_local.h"

qboolean    G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int i;

	if ( !level.spawning ) {
		*out = (char *)defaultString;
//		G_Error( "G_SpawnString() called while not spawning" );
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !strcmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean    G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char        *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean    G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char        *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean    G_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char        *s;
	qboolean present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}



//
// fields are needed for spawning from the entity string
//
typedef enum {
	F_INT,
	F_FLOAT,
	F_LSTRING,          // string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,          // string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_ENTITY,           // index on disk, pointer in memory
	F_ITEM,             // index on disk, pointer in memory
	F_CLIENT,           // index on disk, pointer in memory
	F_IGNORE
} fieldtype_t;

typedef struct
{
	char    *name;
	int ofs;
	fieldtype_t type;
	int flags;
} field_t;

field_t fields[] = {
	{"classname",    FOFS( classname ),    F_LSTRING},
	{"origin",       FOFS( s.origin ),     F_VECTOR},
	{"model",        FOFS( model ),        F_LSTRING},
	{"model2",       FOFS( model2 ),       F_LSTRING},
	{"spawnflags",   FOFS( spawnflags ),   F_INT},
	{"speed",        FOFS( speed ),        F_FLOAT},
	{"closespeed",   FOFS( closespeed ),   F_FLOAT},   //----(SA)	added
	{"target",       FOFS( target ),       F_LSTRING},
	{"targetname",   FOFS( targetname ),   F_LSTRING},
	{"targetdeath",  FOFS( targetdeath ),      F_LSTRING}, //----(SA)	added
	{"message",      FOFS( message ),      F_LSTRING},
	{"popup",        FOFS( message ),      F_LSTRING}, // (SA) mutually exclusive from 'message', but makes the ent more logical for the level designer
	{"book",     FOFS( message ),      F_LSTRING}, // (SA) mutually exclusive from 'message', but makes the ent more logical for the level designer
	{"team",     FOFS( team ),         F_LSTRING},
	{"wait",     FOFS( wait ),         F_FLOAT},
	{"random",       FOFS( random ),       F_FLOAT},
	{"count",        FOFS( count ),        F_INT},
	{"health",       FOFS( health ),       F_INT},
	{"light",        0,                  F_IGNORE},
	{"dmg",          FOFS( damage ),       F_INT},
	{"angles",       FOFS( s.angles ),     F_VECTOR},
	{"angle",        FOFS( s.angles ),     F_ANGLEHACK},
	// JOSEPH 9-27-99
	{"duration", FOFS( duration ),     F_FLOAT},
	{"rotate",       FOFS( rotate ),       F_VECTOR},
	// END JOSEPH
	{"degrees",      FOFS( angle ),        F_FLOAT},
	{"time",     FOFS( speed ),        F_FLOAT},

	// Ridah, AI fields
	{"aiattributes", FOFS( aiAttributes ), F_LSTRING},
	{"ainame",       FOFS( aiName ),       F_LSTRING},
	{"aiteam",       FOFS( aiTeam ),       F_INT},
	// done.

	//----(SA) additional ai field
	{"skin",     FOFS( aiSkin ),       F_LSTRING},
	{"head",     FOFS( aihSkin ),      F_LSTRING},

	//----(SA) done

	// (SA) dlight lightstyles (made all these unique variables for testing)
	{"_color",       FOFS( dl_color ),     F_VECTOR},      // color of the light	(the underscore is inserted by the color picker in QER)
	{"color",        FOFS( dl_color ),     F_VECTOR},      // color of the light
	{"stylestring",  FOFS( dl_stylestring ), F_LSTRING},   // user defined stylestring "fffndlsfaaaaaa" for example
	// done

	//----(SA)
	{"shader",       FOFS( dl_shader ), F_LSTRING},    // shader to use for a target_effect or dlight

	// (SA) for target_unlock
	{"key",          FOFS( key ),      F_INT},
	// done

	// (SA) for item placement
	{"stand",        FOFS( s.frame ),      F_INT},
	// (SA)	end

	// Rafael - mg42
	{"harc",     FOFS( harc ),         F_FLOAT},
	{"varc",     FOFS( varc ),         F_FLOAT},
	// done.

	// Rafael - sniper
	{"delay",    FOFS( delay ),        F_FLOAT},
	{"radius",   FOFS( radius ),       F_INT},

	// Ridah, for reloading savegames at correct mission spot
//	{"missionlevel",	FOFS(missionObjectives),	F_INT},

	// Rafel
	{"start_size", FOFS( start_size ), F_INT},
	{"end_size", FOFS( end_size ), F_INT},

	{"shard", FOFS( count ), F_INT},

	// Rafael
	{"spawnitem",        FOFS( spawnitem ),            F_LSTRING},

	{"track",            FOFS( track ),                F_LSTRING},

	{"scriptName",       FOFS( scriptName ),           F_LSTRING},

	{NULL}
};


typedef struct {
	char    *name;
	void ( *spawn )( gentity_t *ent );
} spawn_t;

void SP_info_player_start( gentity_t *ent );
void SP_info_player_deathmatch( gentity_t *ent );
void SP_info_player_intermission( gentity_t *ent );
void SP_info_firstplace( gentity_t *ent );
void SP_info_secondplace( gentity_t *ent );
void SP_info_thirdplace( gentity_t *ent );
void SP_info_podium( gentity_t *ent );

void SP_func_plat( gentity_t *ent );
void SP_func_static( gentity_t *ent );
void SP_func_leaky( gentity_t *ent ); //----(SA)	added
void SP_func_rotating( gentity_t *ent );
void SP_func_bobbing( gentity_t *ent );
void SP_func_pendulum( gentity_t *ent );
void SP_func_button( gentity_t *ent );
void SP_func_explosive( gentity_t *ent );
void SP_func_door( gentity_t *ent );
void SP_func_train( gentity_t *ent );
void SP_func_timer( gentity_t *self );
// JOSEPH 1-26-00
void SP_func_train_rotating( gentity_t *ent );
void SP_func_secret( gentity_t *ent );
// END JOSEPH
// Rafael
void SP_func_door_rotating( gentity_t *ent );
// RF
void SP_func_bats( gentity_t *self );

void SP_trigger_always( gentity_t *ent );
void SP_trigger_multiple( gentity_t *ent );
void SP_trigger_push( gentity_t *ent );
void SP_trigger_teleport( gentity_t *ent );
void SP_trigger_hurt( gentity_t *ent );

//---- (SA) Wolf triggers
void SP_trigger_once( gentity_t *ent );
//---- done

void SP_target_remove_powerups( gentity_t *ent );
void SP_target_give( gentity_t *ent );
void SP_target_delay( gentity_t *ent );
void SP_target_speaker( gentity_t *ent );
void SP_target_print( gentity_t *ent );
void SP_target_laser( gentity_t *self );
void SP_target_character( gentity_t *ent );
void SP_target_score( gentity_t *ent );
void SP_target_teleporter( gentity_t *ent );
void SP_target_relay( gentity_t *ent );
void SP_target_kill( gentity_t *ent );
void SP_target_position( gentity_t *ent );
void SP_target_location( gentity_t *ent );
void SP_target_push( gentity_t *ent );
void SP_target_script_trigger( gentity_t *ent );

//---- (SA) Wolf targets
// targets
void SP_target_alarm( gentity_t *ent );
void SP_target_counter( gentity_t *ent );
void SP_target_lock( gentity_t *ent );
void SP_target_effect( gentity_t *ent );
void SP_target_fog( gentity_t *ent );
void SP_target_autosave( gentity_t *ent );

// entity visibility dummy
void SP_misc_vis_dummy( gentity_t *ent );
void SP_misc_vis_dummy_multiple( gentity_t *ent );

//----(SA) done

void SP_light( gentity_t *self );
void SP_info_null( gentity_t *self );
void SP_info_notnull( gentity_t *self );
void SP_info_notnull_big( gentity_t *ent );  //----(SA)	added
void SP_info_camp( gentity_t *self );
void SP_path_corner( gentity_t *self );

void SP_misc_teleporter_dest( gentity_t *self );
void SP_misc_model( gentity_t *ent );
void SP_misc_gamemodel( gentity_t *ent );
void SP_misc_portal_camera( gentity_t *ent );
void SP_misc_portal_surface( gentity_t *ent );
void SP_misc_light_surface( gentity_t *ent );
void SP_misc_grabber_trap( gentity_t *ent );
void SP_misc_spotlight( gentity_t *ent ); //----(SA)	added

void SP_shooter_rocket( gentity_t *ent );
void SP_shooter_grenade( gentity_t *ent );

void SP_team_CTF_redplayer( gentity_t *ent );
void SP_team_CTF_blueplayer( gentity_t *ent );

void SP_team_CTF_redspawn( gentity_t *ent );
void SP_team_CTF_bluespawn( gentity_t *ent );

// JPW NERVE for multiplayer spawnpoint selection
void SP_team_WOLF_objective( gentity_t *ent );
// jpw

void SP_team_WOLF_checkpoint( gentity_t *ent );     // DHM - Nerve

// JOSEPH 1-18-00
void SP_props_box_32( gentity_t *self );
void SP_props_box_48( gentity_t *self );
void SP_props_box_64( gentity_t *self );
// END JOSEPH

// Ridah
void SP_ai_soldier( gentity_t *ent );
void SP_ai_american( gentity_t *ent );
void SP_ai_zombie( gentity_t *ent );
void SP_ai_warzombie( gentity_t *ent );
void SP_ai_marker( gentity_t *ent );
void SP_ai_effect( gentity_t *ent );
void SP_ai_trigger( gentity_t *ent );
void SP_ai_venom( gentity_t *ent );
void SP_ai_loper( gentity_t *ent );
void SP_ai_boss_helga( gentity_t *ent );
void SP_ai_boss_heinrich( gentity_t *ent ); //----(SA)	added
void SP_ai_eliteguard( gentity_t *ent );
void SP_ai_stimsoldier_dual( gentity_t *ent );
void SP_ai_stimsoldier_rocket( gentity_t *ent );
void SP_ai_stimsoldier_tesla( gentity_t *ent );
void SP_ai_supersoldier( gentity_t *ent );
void SP_ai_blackguard( gentity_t *ent );
void SP_ai_protosoldier( gentity_t *ent );
void SP_ai_frogman( gentity_t *ent );
void SP_ai_partisan( gentity_t *ent );
void SP_ai_civilian( gentity_t *ent );
// done.

// Rafael particles
void SP_Snow( gentity_t *ent );
void SP_target_smoke( gentity_t *ent );
void SP_Bubbles( gentity_t *ent );
// done.

// (SA) dlights
void SP_dlight( gentity_t *ent );
// done
void SP_corona( gentity_t *ent );

// Rafael mg42
void SP_mg42( gentity_t *ent );
// done.

// Rafael sniper
void SP_shooter_sniper( gentity_t *ent );
void SP_sniper_brush( gentity_t *ent );
// done

//----(SA)
void SP_shooter_zombiespit( gentity_t *ent );
void SP_shooter_mortar( gentity_t *ent );
void SP_shooter_tesla( gentity_t *ent );

// alarm
void SP_alarm_box( gentity_t *ent );
//----(SA)	end

void SP_trigger_flagonly( gentity_t *ent );     // DHM - Nerve
void SP_trigger_objective_info( gentity_t *ent );   // DHM - Nerve

void SP_gas( gentity_t *ent );
void SP_target_rumble( gentity_t *ent );
void SP_func_train_particles( gentity_t *ent );


// Rafael
void SP_trigger_aidoor( gentity_t *ent );
void SP_SmokeDust( gentity_t *ent );
void SP_Dust( gentity_t *ent );
void SP_props_sparks( gentity_t *ent );
void SP_props_gunsparks( gentity_t *ent );

// Props
void SP_Props_Bench( gentity_t *ent );
void SP_Props_Radio( gentity_t *ent );
void SP_Props_Chair( gentity_t *ent );
void SP_Props_ChairHiback( gentity_t *ent );
void SP_Props_ChairSide( gentity_t *ent );
void SP_Props_ChairChat( gentity_t *ent );
void SP_Props_ChairChatArm( gentity_t *ent );
void SP_Props_DamageInflictor( gentity_t *ent );
void SP_Props_Locker_Tall( gentity_t *ent );
void SP_Props_Desklamp( gentity_t *ent );
void SP_Props_Flamebarrel( gentity_t *ent );
void SP_crate_64( gentity_t *ent );
void SP_Props_Flipping_Table( gentity_t *ent );
void SP_crate_32( gentity_t *self );
void SP_Props_Crate32x64( gentity_t *ent );
void SP_Props_58x112tablew( gentity_t *ent );
void SP_props_castlebed( gentity_t *ent );
void SP_Props_RadioSEVEN( gentity_t *ent );
void SP_propsFireColumn( gentity_t *ent );
void SP_props_flamethrower( gentity_t *ent );

void SP_func_tramcar( gentity_t *ent );
void func_invisible_user( gentity_t *ent );

void SP_lightJunior( gentity_t *self );

void SP_props_me109( gentity_t *ent );
void SP_misc_flak( gentity_t *ent );
void SP_plane_waypoint( gentity_t *self );

void SP_props_snowGenerator( gentity_t *ent );
void SP_truck_cam( gentity_t *self );

void SP_screen_fade( gentity_t *ent );
void SP_camera_reset_player( gentity_t *ent );
void SP_camera_cam( gentity_t *ent );
void SP_props_decoration( gentity_t *ent );
void SP_props_decorBRUSH( gentity_t *ent );
void SP_props_statue( gentity_t *ent );
void SP_props_statueBRUSH( gentity_t *ent );
void SP_skyportal( gentity_t *ent );

// RF, scripting
void SP_script_model_med( gentity_t *ent );
void SP_script_mover( gentity_t *ent );
void SP_script_multiplayer( gentity_t *ent );         // DHM - Nerve

void SP_props_footlocker( gentity_t *self );
void SP_misc_firetrails( gentity_t *ent );
void SP_misc_tagemitter( gentity_t *ent );   //----(SA)	added
void SP_trigger_deathCheck( gentity_t *ent );
void SP_misc_spawner( gentity_t *ent );
void SP_props_decor_Scale( gentity_t *ent );

spawn_t spawns[] = {
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_intermission", SP_info_player_intermission},
	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},
	{"info_notnull_big", SP_info_notnull_big},   //----(SA)	added

	{"info_camp", SP_info_camp},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_explosive", SP_func_explosive},
	{"func_door", SP_func_door},
	{"func_static", SP_func_static},
	{"func_leaky", SP_func_leaky},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_pendulum", SP_func_pendulum},
	{"func_train", SP_func_train},
	{"func_group", SP_info_null},
	// JOSEPH 1-26-00
	{"func_train_rotating", SP_func_train_rotating},
	{"func_secret", SP_func_secret},
	// END JOSEPH
	// Rafael
	{"func_door_rotating", SP_func_door_rotating},

	{"func_train_particles", SP_func_train_particles},

	{"func_timer", SP_func_timer},           // rename trigger_timer?

	{"func_tramcar", SP_func_tramcar},
	{"func_invisible_user", func_invisible_user},

	{"func_bats", SP_func_bats},

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{"trigger_always", SP_trigger_always},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_push", SP_trigger_push},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_hurt", SP_trigger_hurt},

	//---- (SA) Wolf triggers
	{"trigger_once",     SP_trigger_once},
	//---- done

	// Rafael
	{"trigger_aidoor", SP_trigger_aidoor},
	{"trigger_deathCheck",SP_trigger_deathCheck},

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give},
	{"target_remove_powerups", SP_target_remove_powerups},
	{"target_delay", SP_target_delay},
	{"target_speaker", SP_target_speaker},
	{"target_print", SP_target_print},
	{"target_laser", SP_target_laser},
	{"target_score", SP_target_score},
	{"target_teleporter", SP_target_teleporter},
	{"target_relay", SP_target_relay},
	{"target_kill", SP_target_kill},
	{"target_position", SP_target_position},
	{"target_location", SP_target_location},
	{"target_push", SP_target_push},
	{"target_script_trigger", SP_target_script_trigger},

	//---- (SA) Wolf targets
	{"target_alarm",     SP_target_alarm},
	{"target_counter",       SP_target_counter},
	{"target_lock",          SP_target_lock},
	{"target_effect",        SP_target_effect},
	{"target_fog",           SP_target_fog},
	{"target_autosave",      SP_target_autosave},    //----(SA)	added
	//---- done

	{"target_rumble", SP_target_rumble},


	{"light", SP_light},

	{"lightJunior", SP_lightJunior},

	{"path_corner", SP_path_corner},

	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_gamemodel", SP_misc_gamemodel},
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},

	//----(SA) Wolf misc
	{"misc_vis_dummy",       SP_misc_vis_dummy},
	{"misc_vis_dummy_multiple",      SP_misc_vis_dummy_multiple},
	{"misc_light_surface",   SP_misc_light_surface},
	{"misc_grabber_trap",    SP_misc_grabber_trap},
	{"misc_spotlight",       SP_misc_spotlight}, //----(SA)	added
	//----(SA) end


	// Rafael mg42
	{"misc_mg42", SP_mg42},
	// done.
	{"misc_flak", SP_misc_flak},
	{"misc_firetrails", SP_misc_firetrails},

	{"misc_tagemitter", SP_misc_tagemitter}, //----(SA)	added

	{"shooter_rocket", SP_shooter_rocket},
	{"shooter_grenade", SP_shooter_grenade},

//----(SA)
	{"shooter_zombiespit", SP_shooter_zombiespit},
	{"shooter_mortar", SP_shooter_mortar},
	{"shooter_tesla", SP_shooter_tesla},

	// alarm
	{"alarm_box", SP_alarm_box},
//----(SA)	end

	// Rafael sniper
	{"shooter_sniper", SP_shooter_sniper},
	{"sniper_brush", SP_sniper_brush},
	// done

	{"team_CTF_redplayer", SP_team_CTF_redplayer},
	{"team_CTF_blueplayer", SP_team_CTF_blueplayer},

	{"team_CTF_redspawn", SP_team_CTF_redspawn},
	{"team_CTF_bluespawn", SP_team_CTF_bluespawn},

// JPW NERVE
	{"team_WOLF_objective", SP_team_WOLF_objective},
// jpw

	{"team_WOLF_checkpoint", SP_team_WOLF_checkpoint},       // DHM - Nerve

	// Ridah
	{"ai_soldier", SP_ai_soldier},
	{"ai_american", SP_ai_american},
	{"ai_zombie", SP_ai_zombie},
	{"ai_warzombie", SP_ai_warzombie},
	{"ai_venom", SP_ai_venom},
	{"ai_loper", SP_ai_loper},
	{"ai_boss_helga", SP_ai_boss_helga},
	{"ai_boss_heinrich", SP_ai_boss_heinrich},   //----(SA)
	{"ai_eliteguard", SP_ai_eliteguard},
	{"ai_stimsoldier_dual", SP_ai_stimsoldier_dual},
	{"ai_stimsoldier_rocket", SP_ai_stimsoldier_rocket},
	{"ai_stimsoldier_tesla", SP_ai_stimsoldier_tesla},
	{"ai_supersoldier", SP_ai_supersoldier},
	{"ai_protosoldier", SP_ai_protosoldier},
	{"ai_frogman", SP_ai_frogman},
	{"ai_blackguard", SP_ai_blackguard},
	{"ai_partisan", SP_ai_partisan},
	{"ai_civilian", SP_ai_civilian},


	{"ai_marker", SP_ai_marker},
	{"ai_effect", SP_ai_effect},
	{"ai_trigger", SP_ai_trigger},
	// done.

	// Rafael particles
	{"misc_snow256", SP_Snow},
	{"misc_snow128", SP_Snow},
	{"misc_snow64", SP_Snow},
	{"misc_snow32", SP_Snow},
	{"target_smoke", SP_target_smoke},

	{"misc_bubbles8", SP_Bubbles},
	{"misc_bubbles16", SP_Bubbles},
	{"misc_bubbles32", SP_Bubbles},
	{"misc_bubbles64", SP_Bubbles},
	// done.

	{"misc_spawner", SP_misc_spawner},

	// JOSEPH 1-18-00
	{"props_box_32", SP_props_box_32},
	{"props_box_48", SP_props_box_48},
	{"props_box_64", SP_props_box_64},
	// END JOSEPH

	// Rafael
	{"props_smokedust", SP_SmokeDust},
	{"props_dust", SP_Dust},
	{"props_sparks", SP_props_sparks},
	{"props_gunsparks", SP_props_gunsparks},

	{"plane_waypoint", SP_plane_waypoint},

	{"props_me109", SP_props_me109},

	// Rafael - props
	{"props_bench", SP_Props_Bench},
	{"props_radio", SP_Props_Radio},
	{"props_chair", SP_Props_Chair},
	{"props_chair_hiback", SP_Props_ChairHiback},
	{"props_chair_side", SP_Props_ChairSide},
	{"props_chair_chat", SP_Props_ChairChat},
	{"props_chair_chatarm", SP_Props_ChairChatArm},
	{"props_damageinflictor", SP_Props_DamageInflictor},
	{"props_locker_tall",SP_Props_Locker_Tall},
	{"props_desklamp", SP_Props_Desklamp},
	{"props_flamebarrel", SP_Props_Flamebarrel},
	{"props_crate_64", SP_crate_64},
	{"props_flippy_table", SP_Props_Flipping_Table},
	{"props_crate_32", SP_crate_32},
	{"props_crate_32x64", SP_Props_Crate32x64},
	{"props_58x112tablew", SP_Props_58x112tablew},
	{"props_castlebed", SP_props_castlebed},
	{"props_radioSEVEN", SP_Props_RadioSEVEN},
	{"props_snowGenerator", SP_props_snowGenerator},
	{"props_FireColumn", SP_propsFireColumn},
	{"props_decoration", SP_props_decoration},
	{"props_decorBRUSH", SP_props_decorBRUSH},
	{"props_statue", SP_props_statue},
	{"props_statueBRUSH", SP_props_statueBRUSH},
	{"props_skyportal", SP_skyportal},
	{"props_footlocker", SP_props_footlocker},
	{"props_flamethrower", SP_props_flamethrower},
	{"props_decoration_scale",SP_props_decor_Scale},

	{"truck_cam", SP_truck_cam},

	{"screen_fade", SP_screen_fade},
	{"camera_reset_player", SP_camera_reset_player},
	{"camera_cam",SP_camera_cam},

	// (SA) dlight and dlightstyles
	{"dlight",       SP_dlight},

	//----(SA)	light coronas
	{"corona",       SP_corona},

	{"test_gas", SP_gas},
	{"trigger_flagonly", SP_trigger_flagonly},               // DHM - Nerve
	{"trigger_objective_info", SP_trigger_objective_info},   // DHM - Nerve

	// RF, scripting
	{"script_model_med", SP_script_model_med},
	{"script_mover", SP_script_mover},
	{"script_multiplayer", SP_script_multiplayer},           // DHM - Nerve

	{0, 0}
};

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *ent ) {
	spawn_t *s;
	gitem_t *item;

	if ( !ent->classname ) {
		G_Printf( "G_CallSpawn: NULL classname\n" );
		return qfalse;
	}

	// check item spawn functions
	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( !strcmp( item->classname, ent->classname ) ) {
			// found it
			// DHM - Nerve :: allow flags in GTWOLF
			if ( item->giType == IT_TEAM && ( g_gametype.integer != GT_CTF && g_gametype.integer != GT_WOLF ) ) {
				return qfalse;
			}
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}

	// check normal spawn functions
	for ( s = spawns ; s->name ; s++ ) {
		if ( !strcmp( s->name, ent->classname ) ) {
			// found it
			s->spawn( ent );

			// RF, entity scripting
			if ( ent->s.number >= MAX_CLIENTS && ent->scriptName ) {
				G_Script_ScriptParse( ent );
				G_Script_ScriptEvent( ent, "spawn", "" );
			}

			return qtrue;
		}
	}
	G_Printf( "%s doesn't have a spawn function\n", ent->classname );
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string ) {
	char    *newb, *new_p;
	int i,l;

	l = strlen( string ) + 1;

	newb = G_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i = 0 ; i < l ; i++ ) {
		if ( string[i] == '\\' && i < l - 1 ) {
			i++;
			if ( string[i] == 'n' ) {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}

	return newb;
}




/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *value, gentity_t *ent ) {
	field_t *f;
	byte    *b;
	float v;
	vec3_t vec;

	for ( f = fields ; f->name ; f++ ) {
		if ( !Q_stricmp( f->name, key ) ) {
			// found it
			b = (byte *)ent;

			switch ( f->type ) {
			case F_LSTRING:
				*( char ** )( b + f->ofs ) = G_NewString( value );
				break;
			case F_VECTOR:
				sscanf( value, "%f %f %f", &vec[0], &vec[1], &vec[2] );
				( ( float * )( b + f->ofs ) )[0] = vec[0];
				( ( float * )( b + f->ofs ) )[1] = vec[1];
				( ( float * )( b + f->ofs ) )[2] = vec[2];
				break;
			case F_INT:
				*( int * )( b + f->ofs ) = atoi( value );
				break;
			case F_FLOAT:
				*( float * )( b + f->ofs ) = atof( value );
				break;
			case F_ANGLEHACK:
				v = atof( value );
				( ( float * )( b + f->ofs ) )[0] = 0;
				( ( float * )( b + f->ofs ) )[1] = v;
				( ( float * )( b + f->ofs ) )[2] = 0;
				break;
			default:
			case F_IGNORE:
				break;
			}
			return;
		}
	}
}




/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void ) {
	int i;
	gentity_t   *ent;

	// get the next free entity
	ent = G_Spawn();

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		G_ParseField( level.spawnVars[i][0], level.spawnVars[i][1], ent );
	}

	// check for "notteam" / "notfree" flags
	if ( g_gametype.integer == GT_SINGLE_PLAYER ) {
		G_SpawnInt( "notsingle", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}
	if ( g_gametype.integer >= GT_TEAM ) {
		G_SpawnInt( "notteam", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	} else {
		G_SpawnInt( "notfree", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
	}
}



/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string ) {
	int l;
	char    *dest;

	l = strlen( string );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_VARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l + 1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( void ) {
	char keyname[MAX_TOKEN_CHARS];
	char com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		G_Error( "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}

		// parse value
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return qtrue;
}



/*QUAKED worldspawn (0 0 0) ? sun_cameraflare

Every map should have exactly one worldspawn.
"music"     Music wav file
"gravity"   800 is default gravity
"message" Text to print during connection process
"ambient"  Ambient light value (must use '_color')
"_color"    Ambient light color (must be used with 'ambient')
"sun"        Shader to use for 'sun' image
*/
void SP_worldspawn( void ) {
	char    *s;
	gitem_t *item; // JPW NERVE

	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va( "%i", level.startTime ) );

	G_SpawnString( "music", "", &s );
	trap_SetConfigstring( CS_MUSIC, s );

	G_SpawnString( "message", "", &s );
	trap_SetConfigstring( CS_MESSAGE, s );              // map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );     // message of the day

	G_SpawnString( "gravity", "800", &s );
	trap_Cvar_Set( "g_gravity", s );

	// (SA) FIXME: todo: sun shader set for worldspawn

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "" );
	if ( g_restarted.integer ) {
		trap_Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;
	}

// JPW NERVE change minigun overheat time for single player -- this array gets reloaded every time the server is reset,
// so this is as good a place as any to do stuff like this
	if ( g_gametype.integer != GT_SINGLE_PLAYER ) {
		ammoTable[WP_VENOM].maxHeat *= 0.25;
		ammoTable[WP_DYNAMITE].uses = 0; // regens based on recharge time
		// reset ammo for subs to be distinct for multiplayer (so running out of rifle ammo doesn't deplete sidearm)
		// if player runs out of SMG ammunition, it shouldn't *also* deplete pistol ammunition.  If you change this, change
		// g_spawn.c as well
		item = BG_FindItem( "Thompson" );
		item->giAmmoIndex = WP_THOMPSON;
		item = BG_FindItem( "Sten" );
		item->giAmmoIndex = WP_STEN;
		item = BG_FindItem( "MP40" );
		item->giAmmoIndex = WP_MP40;
	}
// jpw

}


/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void ) {
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() ) {
		G_Error( "SpawnEntities: no entities" );
	}
	SP_worldspawn();

	// parse ents
	while ( G_ParseSpawnVars() ) {
		G_SpawnGEntityFromSpawnVars();
	}

	level.spawning = qfalse;            // any future calls to G_Spawn*() will be errors
}

