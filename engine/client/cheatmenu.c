/*
cheatmenu.c - Cheat menu system for managing cheat functions
Copyright (C) 2024

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include "common.h"
#include "client.h"

// Forward declarations
static void CheatMenu_ShowMainMenu( void );
static void CheatMenu_ShowESPMenu( void );
static void CheatMenu_ShowAimbotMenu( void );
static void CheatMenu_ShowMovementMenu( void );
static void CheatMenu_ShowMiscMenu( void );
static void CheatMenu_ToggleCvar( const char *cvar_name, const char *description );
static void CheatMenu_SetCvar( const char *cvar_name, const char *value, const char *description );

/*
==================
CheatMenu_ToggleCvar

Toggle a cvar between 0 and 1
==================
*/
static void CheatMenu_ToggleCvar( const char *cvar_name, const char *description )
{
	float current = Cvar_VariableValue( cvar_name );
	float new_value = ( current > 0.0f ) ? 0.0f : 1.0f;
	Cvar_Set( cvar_name, va( "%.0f", new_value ) );
	Con_Printf( "%s: %s = %.0f\n", description ? description : cvar_name, cvar_name, new_value );
}

/*
==================
CheatMenu_SetCvar

Set a cvar to a specific value
==================
*/
static void CheatMenu_SetCvar( const char *cvar_name, const char *value, const char *description )
{
	Cvar_Set( cvar_name, value );
	Con_Printf( "%s: %s = %s\n", description ? description : cvar_name, cvar_name, value );
}

/*
==================
CheatMenu_ShowMainMenu

Show the main cheat menu
==================
*/
static void CheatMenu_ShowMainMenu( void )
{
	Con_Printf( "\n" );
	Con_Printf( "============ CHEAT MENU ============\n" );
	Con_Printf( "1. ESP Menu (kek_esp_*)\n" );
	Con_Printf( "2. Aimbot Menu (kek_aimbot_*)\n" );
	Con_Printf( "3. Movement Menu (ebash3d_*)\n" );
	Con_Printf( "4. Miscellaneous Menu\n" );
	Con_Printf( "5. Quick Toggle Commands\n" );
	Con_Printf( "6. Show All Cheat Cvars\n" );
	Con_Printf( "===================================\n" );
	Con_Printf( "Usage: cheatmenu <1-6>\n" );
	Con_Printf( "       cheatmenu toggle <cvar_name>\n" );
	Con_Printf( "       cheatmenu set <cvar_name> <value>\n" );
	Con_Printf( "       cheatmenu get <cvar_name>\n" );
	Con_Printf( "\n" );
}

/*
==================
CheatMenu_ShowESPMenu

Show ESP-related cheat options
==================
*/
static void CheatMenu_ShowESPMenu( void )
{
	Con_Printf( "\n" );
	Con_Printf( "============ ESP MENU ============\n" );
	
	float esp_enable = Cvar_VariableValue( "kek_esp" );
	float esp_box = Cvar_VariableValue( "kek_esp_box" );
	float esp_name = Cvar_VariableValue( "kek_esp_name" );
	float esp_weapon = Cvar_VariableValue( "kek_esp_weapon" );
	
	Con_Printf( "ESP Status:\n" );
	Con_Printf( "  kek_esp: %.0f %s\n", esp_enable, esp_enable > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  kek_esp_box: %.0f %s\n", esp_box, esp_box > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  kek_esp_name: %.0f %s\n", esp_name, esp_name > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  kek_esp_weapon: %.0f %s\n", esp_weapon, esp_weapon > 0 ? "(ON)" : "(OFF)" );
	
	Con_Printf( "\nColor Settings:\n" );
	Con_Printf( "  kek_esp_r/g/b: RGB for hidden players\n" );
	Con_Printf( "  kek_esp_visible_r/g/b: RGB for visible players\n" );
	Con_Printf( "  kek_esp_name_r/g/b: RGB for player names\n" );
	Con_Printf( "  kek_esp_weapon_r/g/b: RGB for weapon names\n" );
	
	Con_Printf( "\nQuick Commands:\n" );
	Con_Printf( "  cheatmenu toggle kek_esp\n" );
	Con_Printf( "  cheatmenu toggle kek_esp_box\n" );
	Con_Printf( "  cheatmenu toggle kek_esp_name\n" );
	Con_Printf( "  cheatmenu toggle kek_esp_weapon\n" );
	Con_Printf( "\n" );
}

/*
==================
CheatMenu_ShowAimbotMenu

Show aimbot-related cheat options
==================
*/
static void CheatMenu_ShowAimbotMenu( void )
{
	Con_Printf( "\n" );
	Con_Printf( "============ AIMBOT MENU ============\n" );
	
	float aimbot_enable = Cvar_VariableValue( "kek_aimbot" );
	float aimbot_fov = Cvar_VariableValue( "kek_aimbot_fov" );
	float aimbot_smooth = Cvar_VariableValue( "kek_aimbot_smooth" );
	float aimbot_visible_only = Cvar_VariableValue( "kek_aimbot_visible_only" );
	float aimbot_psilent = Cvar_VariableValue( "kek_aimbot_psilent" );
	float aimbot_dm = Cvar_VariableValue( "kek_aimbot_dm" );
	float antiaim_enable = Cvar_VariableValue( "kek_antiaim" );
	
	Con_Printf( "Aimbot Status:\n" );
	Con_Printf( "  kek_aimbot: %.0f %s\n", aimbot_enable, aimbot_enable > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  kek_aimbot_fov: %.1f\n", aimbot_fov );
	Con_Printf( "  kek_aimbot_smooth: %.2f\n", aimbot_smooth );
	Con_Printf( "  kek_aimbot_visible_only: %.0f %s\n", aimbot_visible_only, aimbot_visible_only > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  kek_aimbot_psilent: %.0f %s\n", aimbot_psilent, aimbot_psilent > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  kek_aimbot_dm: %.0f %s\n", aimbot_dm, aimbot_dm > 0 ? "(Deathmatch)" : "(Team)" );
	Con_Printf( "  kek_antiaim: %.0f %s\n", antiaim_enable, antiaim_enable > 0 ? "(ON)" : "(OFF)" );
	
	Con_Printf( "\nAnti-Aim Settings:\n" );
	float antiaim_mode = Cvar_VariableValue( "kek_antiaim_mode" );
	float antiaim_speed = Cvar_VariableValue( "kek_antiaim_speed" );
	Con_Printf( "  kek_antiaim_mode: %.0f\n", antiaim_mode );
	Con_Printf( "  kek_antiaim_speed: %.1f\n", antiaim_speed );
	
	Con_Printf( "\nQuick Commands:\n" );
	Con_Printf( "  cheatmenu toggle kek_aimbot\n" );
	Con_Printf( "  cheatmenu set kek_aimbot_fov 90\n" );
	Con_Printf( "  cheatmenu set kek_aimbot_smooth 0.5\n" );
	Con_Printf( "  cheatmenu toggle kek_aimbot_psilent\n" );
	Con_Printf( "  cheatmenu toggle kek_antiaim\n" );
	Con_Printf( "\n" );
}

/*
==================
CheatMenu_ShowMovementMenu

Show movement-related cheat options
==================
*/
static void CheatMenu_ShowMovementMenu( void )
{
	Con_Printf( "\n" );
	Con_Printf( "============ MOVEMENT MENU ============\n" );
	
	float speed_mult = Cvar_VariableValue( "ebash3d_speed_multiplier" );
	float auto_strafe = Cvar_VariableValue( "ebash3d_auto_strafe" );
	float ground_strafe = Cvar_VariableValue( "ebash3d_ground_strafe" );
	float fakelag = Cvar_VariableValue( "ebash3d_fakelag" );
	
	Con_Printf( "Movement Status:\n" );
	Con_Printf( "  ebash3d_speed_multiplier: %.1f\n", speed_mult );
	Con_Printf( "  ebash3d_auto_strafe: %.0f %s\n", auto_strafe, auto_strafe > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  ebash3d_ground_strafe: %.0f %s\n", ground_strafe, ground_strafe > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  ebash3d_fakelag: %.0f ms\n", fakelag );
	
	Con_Printf( "\nQuick Commands:\n" );
	Con_Printf( "  ebash3d_speed - Toggle speed boost\n" );
	Con_Printf( "  ebash3d_strafe - Toggle autostrafe\n" );
	Con_Printf( "  ebash3d_gstrafe - Toggle ground strafe\n" );
	Con_Printf( "  cheatmenu set ebash3d_speed_multiplier 7.0\n" );
	Con_Printf( "  cheatmenu set ebash3d_fakelag 50\n" );
	Con_Printf( "\n" );
}

/*
==================
CheatMenu_ShowMiscMenu

Show miscellaneous cheat options
==================
*/
static void CheatMenu_ShowMiscMenu( void )
{
	Con_Printf( "\n" );
	Con_Printf( "============ MISCELLANEOUS MENU ============\n" );
	
	float custom_fov = Cvar_VariableValue( "kek_custom_fov" );
	float custom_fov_value = Cvar_VariableValue( "kek_custom_fov_value" );
	float viewmodel_glow = Cvar_VariableValue( "kek_viewmodel_glow" );
	float cmd_block = Cvar_VariableValue( "ebash3d_cmd_block" );
	float debug = Cvar_VariableValue( "kek_debug" );
	
	Con_Printf( "Misc Status:\n" );
	Con_Printf( "  kek_custom_fov: %.0f %s\n", custom_fov, custom_fov > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  kek_custom_fov_value: %.1f\n", custom_fov_value );
	Con_Printf( "  kek_viewmodel_glow: %.0f %s\n", viewmodel_glow, viewmodel_glow > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  ebash3d_cmd_block: %.0f %s\n", cmd_block, cmd_block > 0 ? "(ON)" : "(OFF)" );
	Con_Printf( "  kek_debug: %.0f\n", debug );
	
	Con_Printf( "\nID Commands:\n" );
	Con_Printf( "  ebash3d_get_id - Get current player ID\n" );
	Con_Printf( "  ebash3d_change_id [new_id] - Change player ID\n" );
	
	Con_Printf( "\nQuick Commands:\n" );
	Con_Printf( "  cheatmenu toggle kek_custom_fov\n" );
	Con_Printf( "  cheatmenu toggle kek_viewmodel_glow\n" );
	Con_Printf( "  cheatmenu toggle ebash3d_cmd_block\n" );
	Con_Printf( "\n" );
}

/*
==================
CheatMenu_ListAllCvars

List all cheat-related cvars
==================
*/
static void CheatMenu_ListAllCvars( void )
{
	Con_Printf( "\n" );
	Con_Printf( "============ ALL CHEAT CVARS ============\n" );
	Con_Printf( "\nESP CVars:\n" );
	Con_Printf( "  kek_esp, kek_esp_box, kek_esp_name, kek_esp_weapon\n" );
	Con_Printf( "  kek_esp_r, kek_esp_g, kek_esp_b\n" );
	Con_Printf( "  kek_esp_visible_r, kek_esp_visible_g, kek_esp_visible_b\n" );
	Con_Printf( "  kek_esp_name_r, kek_esp_name_g, kek_esp_name_b\n" );
	Con_Printf( "  kek_esp_weapon_r, kek_esp_weapon_g, kek_esp_weapon_b\n" );
	
	Con_Printf( "\nAimbot CVars:\n" );
	Con_Printf( "  kek_aimbot, kek_aimbot_fov, kek_aimbot_smooth\n" );
	Con_Printf( "  kek_aimbot_visible_only, kek_aimbot_draw_fov\n" );
	Con_Printf( "  kek_aimbot_max_distance, kek_aimbot_psilent, kek_aimbot_dm\n" );
	Con_Printf( "  kek_aimbot_x, kek_aimbot_y, kek_aimbot_z\n" );
	Con_Printf( "  kek_aimbot_fov_r, kek_aimbot_fov_g, kek_aimbot_fov_b\n" );
	
	Con_Printf( "\nAnti-Aim CVars:\n" );
	Con_Printf( "  kek_antiaim, kek_antiaim_mode, kek_antiaim_speed\n" );
	Con_Printf( "  kek_antiaim_jitter_range, kek_antiaim_fake_angle\n" );
	
	Con_Printf( "\nMovement CVars:\n" );
	Con_Printf( "  ebash3d_speed_multiplier, ebash3d_auto_strafe\n" );
	Con_Printf( "  ebash3d_ground_strafe, ebash3d_fakelag\n" );
	Con_Printf( "  ebash3d_cmd_block\n" );
	
	Con_Printf( "\nMisc CVars:\n" );
	Con_Printf( "  kek_custom_fov, kek_custom_fov_value\n" );
	Con_Printf( "  kek_viewmodel_glow, kek_viewmodel_glow_mode\n" );
	Con_Printf( "  kek_viewmodel_glow_r, kek_viewmodel_glow_g, kek_viewmodel_glow_b\n" );
	Con_Printf( "  kek_viewmodel_glow_alpha, kek_debug\n" );
	
	Con_Printf( "\nID Commands:\n" );
	Con_Printf( "  ebash3d_get_id, ebash3d_change_id\n" );
	
	Con_Printf( "\nUse 'cvarlist kek_' or 'cvarlist ebash3d_' to see current values\n" );
	Con_Printf( "==========================================\n" );
	Con_Printf( "\n" );
}

/*
==================
CheatMenu_QuickToggle

Quick toggle for common cheat functions
==================
*/
static void CheatMenu_QuickToggle( void )
{
	if( Cmd_Argc() < 2 )
	{
		Con_Printf( "\n" );
		Con_Printf( "============ QUICK TOGGLE ============\n" );
		Con_Printf( "Usage: cheatmenu qt <option>\n" );
		Con_Printf( "\nAvailable options:\n" );
		Con_Printf( "  esp - Toggle ESP\n" );
		Con_Printf( "  aimbot - Toggle Aimbot\n" );
		Con_Printf( "  antiaim - Toggle Anti-Aim\n" );
		Con_Printf( "  speed - Toggle Speed Boost\n" );
		Con_Printf( "  strafe - Toggle Auto Strafe\n" );
		Con_Printf( "  glow - Toggle Viewmodel Glow\n" );
		Con_Printf( "\n" );
		return;
	}
	
	const char *option = Cmd_Argv( 1 );
	
	if( !Q_stricmp( option, "esp" ) )
	{
		CheatMenu_ToggleCvar( "kek_esp", "ESP" );
	}
	else if( !Q_stricmp( option, "aimbot" ) )
	{
		CheatMenu_ToggleCvar( "kek_aimbot", "Aimbot" );
	}
	else if( !Q_stricmp( option, "antiaim" ) )
	{
		CheatMenu_ToggleCvar( "kek_antiaim", "Anti-Aim" );
	}
	else if( !Q_stricmp( option, "speed" ) )
	{
		float current = Cvar_VariableValue( "ebash3d_speed_multiplier" );
		if( current == 1.0f )
			CheatMenu_SetCvar( "ebash3d_speed_multiplier", "7.0", "Speed Boost" );
		else
			CheatMenu_SetCvar( "ebash3d_speed_multiplier", "1.0", "Speed Boost" );
	}
	else if( !Q_stricmp( option, "strafe" ) )
	{
		CheatMenu_ToggleCvar( "ebash3d_auto_strafe", "Auto Strafe" );
	}
	else if( !Q_stricmp( option, "glow" ) )
	{
		CheatMenu_ToggleCvar( "kek_viewmodel_glow", "Viewmodel Glow" );
	}
	else
	{
		Con_Printf( S_ERROR "Unknown option: %s\n", option );
		Con_Printf( "Use 'cheatmenu qt' to see available options\n" );
	}
}

/*
==================
CheatMenu_f

Main cheat menu command handler
Usage:
  cheatmenu - Show main menu
  cheatmenu <1-6> - Show specific submenu
  cheatmenu toggle <cvar> - Toggle a cvar
  cheatmenu set <cvar> <value> - Set a cvar value
  cheatmenu get <cvar> - Get a cvar value
  cheatmenu qt <option> - Quick toggle common functions
==================
*/
static void CheatMenu_f( void )
{
	// Open visual menu instead of console menu
	extern void CheatMenu_Toggle( void );
	CheatMenu_Toggle();
	return;
	
	// Keep old console menu code for fallback
	if( Cmd_Argc() < 2 )
	{
		CheatMenu_ShowMainMenu();
		return;
	}
	
	const char *action = Cmd_Argv( 1 );
	
	if( !Q_stricmp( action, "1" ) || !Q_stricmp( action, "esp" ) )
	{
		CheatMenu_ShowESPMenu();
	}
	else if( !Q_stricmp( action, "2" ) || !Q_stricmp( action, "aimbot" ) )
	{
		CheatMenu_ShowAimbotMenu();
	}
	else if( !Q_stricmp( action, "3" ) || !Q_stricmp( action, "movement" ) )
	{
		CheatMenu_ShowMovementMenu();
	}
	else if( !Q_stricmp( action, "4" ) || !Q_stricmp( action, "misc" ) )
	{
		CheatMenu_ShowMiscMenu();
	}
	else if( !Q_stricmp( action, "5" ) || !Q_stricmp( action, "qt" ) || !Q_stricmp( action, "quicktoggle" ) )
	{
		CheatMenu_QuickToggle();
	}
	else if( !Q_stricmp( action, "6" ) || !Q_stricmp( action, "list" ) )
	{
		CheatMenu_ListAllCvars();
	}
	else if( !Q_stricmp( action, "toggle" ) )
	{
		if( Cmd_Argc() < 3 )
		{
			Con_Printf( S_USAGE "cheatmenu toggle <cvar_name>\n" );
			return;
		}
		const char *cvar_name = Cmd_Argv( 2 );
		CheatMenu_ToggleCvar( cvar_name, NULL );
	}
	else if( !Q_stricmp( action, "set" ) )
	{
		if( Cmd_Argc() < 4 )
		{
			Con_Printf( S_USAGE "cheatmenu set <cvar_name> <value>\n" );
			return;
		}
		const char *cvar_name = Cmd_Argv( 2 );
		const char *value = Cmd_Argv( 3 );
		CheatMenu_SetCvar( cvar_name, value, NULL );
	}
	else if( !Q_stricmp( action, "get" ) )
	{
		if( Cmd_Argc() < 3 )
		{
			Con_Printf( S_USAGE "cheatmenu get <cvar_name>\n" );
			return;
		}
		const char *cvar_name = Cmd_Argv( 2 );
		float value = Cvar_VariableValue( cvar_name );
		const char *string = Cvar_VariableString( cvar_name );
		Con_Printf( "%s = %s (value: %.2f)\n", cvar_name, string, value );
	}
	else
	{
		Con_Printf( S_ERROR "Unknown action: %s\n", action );
		Con_Printf( "Use 'cheatmenu' to see available options\n" );
	}
}

/*
==================
CheatMenu_Init

Register cheat menu commands
==================
*/
void CheatMenu_Init( void )
{
	Cmd_AddCommand( "cheatmenu", CheatMenu_f, "Cheat configuration menu - use 'cheatmenu' for help" );
	Con_DPrintf( "Cheat menu system initialized\n" );
}

/*
==================
CheatMenu_Shutdown

Unregister cheat menu commands
==================
*/
void CheatMenu_Shutdown( void )
{
	Cmd_RemoveCommand( "cheatmenu" );
}
