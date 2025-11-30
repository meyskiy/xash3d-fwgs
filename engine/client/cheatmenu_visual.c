/*
cheatmenu_visual.c - Visual cheat menu system with touch support
eBash3D Recode - Modern cheat menu
Copyright (C) 2024

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.
*/

#include "common.h"
#include "client.h"
#include "ref_api.h"
#include "ref_common.h"
#include "qfont.h"
#include "keydefs.h"
#include "cursor_type.h"
#include "platform/platform.h"

// Menu states
typedef enum
{
	CHEATMENU_CLOSED = 0,
	CHEATMENU_MAIN,
	CHEATMENU_VISUAL,
	CHEATMENU_AIMBOT,
	CHEATMENU_MOVEMENT,
	CHEATMENU_MISC
} cheatmenu_state_t;

static cheatmenu_state_t menu_state = CHEATMENU_CLOSED;
static int selected_item = 0;
static int scroll_offset = 0;
static qboolean mouse_down = false;
static qboolean dragging_slider = false;
static int dragging_slider_index = -1;
static float mouse_x = 0.0f, mouse_y = 0.0f;
static float touch_x = 0.0f, touch_y = 0.0f;
static qboolean touch_active = false;

// Calculate centered menu position

// Menu constants - Modern black/purple theme (centered, wider)
#define MENU_WIDTH 600
#define MENU_HEIGHT 650
#define ITEM_HEIGHT 42
#define ITEM_SPACING 4
#define SLIDER_WIDTH 220
#define SLIDER_HEIGHT 10
#define TOGGLE_WIDTH 70
#define TOGGLE_HEIGHT 30

// Modern color scheme - Black background with purple accents (enhanced)
#define COLOR_BG_R 10
#define COLOR_BG_G 10
#define COLOR_BG_B 15
#define COLOR_BG_A 250

#define COLOR_BG_DARK_R 5
#define COLOR_BG_DARK_G 5
#define COLOR_BG_DARK_B 8
#define COLOR_BG_DARK_A 255

#define COLOR_PURPLE_R 138
#define COLOR_PURPLE_G 43
#define COLOR_PURPLE_B 226
#define COLOR_PURPLE_A 255

#define COLOR_PURPLE_LIGHT_R 167
#define COLOR_PURPLE_LIGHT_G 96
#define COLOR_PURPLE_LIGHT_B 255
#define COLOR_PURPLE_LIGHT_A 255

#define COLOR_PURPLE_DARK_R 100
#define COLOR_PURPLE_DARK_G 30
#define COLOR_PURPLE_DARK_B 180
#define COLOR_PURPLE_DARK_A 255

#define COLOR_BORDER_R 138
#define COLOR_BORDER_G 43
#define COLOR_BORDER_B 226
#define COLOR_BORDER_A 255

#define COLOR_BORDER_GLOW_R 200
#define COLOR_BORDER_GLOW_G 100
#define COLOR_BORDER_GLOW_B 255
#define COLOR_BORDER_GLOW_A 150

#define COLOR_TEXT_R 255
#define COLOR_TEXT_G 255
#define COLOR_TEXT_B 255
#define COLOR_TEXT_A 255

#define COLOR_TEXT_TITLE_R 200
#define COLOR_TEXT_TITLE_G 120
#define COLOR_TEXT_TITLE_B 255
#define COLOR_TEXT_TITLE_A 255

#define COLOR_TEXT_SECONDARY_R 180
#define COLOR_TEXT_SECONDARY_G 180
#define COLOR_TEXT_SECONDARY_B 200
#define COLOR_TEXT_SECONDARY_A 255

#define COLOR_TOGGLE_ON_R 138
#define COLOR_TOGGLE_ON_G 43
#define COLOR_TOGGLE_ON_B 226
#define COLOR_TOGGLE_ON_A 255

#define COLOR_TOGGLE_OFF_R 40
#define COLOR_TOGGLE_OFF_G 40
#define COLOR_TOGGLE_OFF_B 45
#define COLOR_TOGGLE_OFF_A 220

#define COLOR_SLIDER_TRACK_R 30
#define COLOR_SLIDER_TRACK_G 30
#define COLOR_SLIDER_TRACK_B 40
#define COLOR_SLIDER_TRACK_A 255

#define COLOR_SLIDER_FILL_R 138
#define COLOR_SLIDER_FILL_G 43
#define COLOR_SLIDER_FILL_B 226
#define COLOR_SLIDER_FILL_A 200

#define COLOR_SLIDER_THUMB_R 200
#define COLOR_SLIDER_THUMB_G 120
#define COLOR_SLIDER_THUMB_B 255
#define COLOR_SLIDER_THUMB_A 255

#define COLOR_HOVER_R 167
#define COLOR_HOVER_G 96
#define COLOR_HOVER_B 255
#define COLOR_HOVER_A 120

#define COLOR_SHADOW_R 0
#define COLOR_SHADOW_G 0
#define COLOR_SHADOW_B 0
#define COLOR_SHADOW_A 180

// Maximum items visible on screen
#define MAX_VISIBLE_ITEMS ((MENU_HEIGHT - 80) / (ITEM_HEIGHT + ITEM_SPACING))

/*
==================
CheatMenu_GetMenuPosition
Calculate centered menu position
==================
*/
static void CheatMenu_GetMenuPosition( int *x, int *y )
{
	extern ref_globals_t refState;
	*x = ( refState.width - MENU_WIDTH ) / 2;
	*y = ( refState.height - MENU_HEIGHT ) / 2;
}

/*
==================
CheatMenu_DrawRect
Draw a filled rectangle
==================
*/
static void CheatMenu_DrawRect( int x, int y, int w, int h, int r, int g, int b, int a )
{
	ref.dllFuncs.R_Set2DMode( true );
	ref.dllFuncs.FillRGBA( kRenderTransTexture, x, y, w, h, r, g, b, a );
}

/*
==================
CheatMenu_DrawShadow
Draw shadow effect for modern look
==================
*/
static void CheatMenu_DrawShadow( int x, int y, int w, int h, int intensity )
{
	// Draw multiple shadow layers for depth
	for( int i = 0; i < 3; i++ )
	{
		int offset = i + 1;
		int alpha = intensity / ( i + 2 );
		CheatMenu_DrawRect( x + offset, y + offset, w, h, COLOR_SHADOW_R, COLOR_SHADOW_G, COLOR_SHADOW_B, alpha );
	}
}

/*
==================
CheatMenu_DrawText
Draw text at position
==================
*/
static void CheatMenu_DrawText( int x, int y, const char *text, int r, int g, int b )
{
	cl_font_t *font = Con_GetCurFont();
	rgba_t color;
	MakeRGBA( color, r, g, b, 255 );
	CL_DrawString( x, y, text, color, font, FONT_DRAW_UTF8 | FONT_DRAW_HUD );
}

/*
==================
CheatMenu_GetCvarValue
Get cvar value as float
==================
*/
static float CheatMenu_GetCvarValue( const char *cvar_name )
{
	return Cvar_VariableValue( cvar_name );
}

/*
==================
CheatMenu_SetCvarValue
Set cvar value
==================
*/
static void CheatMenu_SetCvarValue( const char *cvar_name, float value )
{
	Cvar_Set( cvar_name, va( "%.2f", value ) );
}

/*
==================
CheatMenu_IsPointInRect
Check if point is inside rectangle
==================
*/
static qboolean CheatMenu_IsPointInRect( float x, float y, int rect_x, int rect_y, int rect_w, int rect_h )
{
	return ( x >= rect_x && x <= ( rect_x + rect_w ) && y >= rect_y && y <= ( rect_y + rect_h ) );
}

/*
==================
CheatMenu_DrawToggle
Draw a modern toggle switch
==================
*/
static void CheatMenu_DrawToggle( int x, int y, int w, int h, qboolean enabled, const char *label, qboolean hover )
{
	// Draw label
	CheatMenu_DrawText( x + 10, y + h / 2 - 8, label, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B );
	
	// Draw toggle switch (modern style)
	int toggle_x = x + w - TOGGLE_WIDTH - 10;
	int toggle_y = y + ( h - TOGGLE_HEIGHT ) / 2;
	
	// Background
	if( enabled )
		CheatMenu_DrawRect( toggle_x, toggle_y, TOGGLE_WIDTH, TOGGLE_HEIGHT, COLOR_TOGGLE_ON_R, COLOR_TOGGLE_ON_G, COLOR_TOGGLE_ON_B, COLOR_TOGGLE_ON_A );
	else
		CheatMenu_DrawRect( toggle_x, toggle_y, TOGGLE_WIDTH, TOGGLE_HEIGHT, COLOR_TOGGLE_OFF_R, COLOR_TOGGLE_OFF_G, COLOR_TOGGLE_OFF_B, COLOR_TOGGLE_OFF_A );
	
	// Toggle button circle
	int circle_size = TOGGLE_HEIGHT - 4;
	int circle_x;
	if( enabled )
		circle_x = toggle_x + TOGGLE_WIDTH - circle_size - 2;
	else
		circle_x = toggle_x + 2;
	int circle_y = toggle_y + 2;
	
	CheatMenu_DrawRect( circle_x, circle_y, circle_size, circle_size, 255, 255, 255, 255 );
	
	// Text
	if( enabled )
		CheatMenu_DrawText( toggle_x + 8, toggle_y + TOGGLE_HEIGHT / 2 - 8, "ON", 255, 255, 255 );
}

/*
==================
CheatMenu_DrawSlider
Draw a modern slider control (working slider)
==================
*/
static void CheatMenu_DrawSlider( int x, int y, int w, int h, float value, float min, float max, const char *label, int slider_index, qboolean hover )
{
	// Draw label and value
	char text[128];
	char value_text[32];
	if( max > 100.0f )
		Q_snprintf( value_text, sizeof( value_text ), "%.0f", value );
	else if( max > 1.0f )
		Q_snprintf( value_text, sizeof( value_text ), "%.1f", value );
	else
		Q_snprintf( value_text, sizeof( value_text ), "%.2f", value );
	Q_snprintf( text, sizeof( text ), "%s: %s", label, value_text );
	CheatMenu_DrawText( x + 15, y + h / 2 - 8, text, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B );
	
	// Draw slider track
	int slider_x = x + w - SLIDER_WIDTH - 15;
	int slider_y = y + h / 2 - SLIDER_HEIGHT / 2;
	
	// Track background with shadow
	CheatMenu_DrawRect( slider_x, slider_y, SLIDER_WIDTH, SLIDER_HEIGHT, COLOR_SLIDER_TRACK_R, COLOR_SLIDER_TRACK_G, COLOR_SLIDER_TRACK_B, COLOR_SLIDER_TRACK_A );
	
	// Draw filled portion with gradient effect
	float normalized = ( value - min ) / ( max - min );
	normalized = bound( 0.0f, normalized, 1.0f );
	int filled_w = (int)( normalized * SLIDER_WIDTH );
	if( filled_w > 0 )
		CheatMenu_DrawRect( slider_x, slider_y, filled_w, SLIDER_HEIGHT, COLOR_SLIDER_FILL_R, COLOR_SLIDER_FILL_G, COLOR_SLIDER_FILL_B, COLOR_SLIDER_FILL_A );
	
	// Draw slider thumb (larger, more modern)
	int thumb_size = 20;
	int thumb_x = slider_x + (int)( normalized * ( SLIDER_WIDTH - thumb_size ) );
	int thumb_y = slider_y - ( thumb_size - SLIDER_HEIGHT ) / 2;
	
	// Shadow for thumb
	CheatMenu_DrawRect( thumb_x + 1, thumb_y + 1, thumb_size, thumb_size, 0, 0, 0, 100 );
	
	// Thumb with gradient effect
	if( dragging_slider && dragging_slider_index == slider_index )
		CheatMenu_DrawRect( thumb_x, thumb_y, thumb_size, thumb_size, COLOR_PURPLE_LIGHT_R, COLOR_PURPLE_LIGHT_G, COLOR_PURPLE_LIGHT_B, COLOR_SLIDER_THUMB_A );
	else
		CheatMenu_DrawRect( thumb_x, thumb_y, thumb_size, thumb_size, COLOR_SLIDER_THUMB_R, COLOR_SLIDER_THUMB_G, COLOR_SLIDER_THUMB_B, COLOR_SLIDER_THUMB_A );
	
	// Inner highlight
	CheatMenu_DrawRect( thumb_x + 2, thumb_y + 2, thumb_size - 4, thumb_size - 4, 255, 255, 255, 80 );
}

/*
==================
CheatMenu_DrawButton
Draw a modern button
==================
*/
static void CheatMenu_DrawButton( int x, int y, int w, int h, const char *label, qboolean selected, qboolean hover )
{
	// Shadow
	CheatMenu_DrawShadow( x, y, w, h, 100 );
	
	// Background with gradient effect
	if( selected || hover )
	{
		CheatMenu_DrawRect( x, y, w, h, COLOR_PURPLE_R, COLOR_PURPLE_G, COLOR_PURPLE_B, 100 );
		// Glow effect
		CheatMenu_DrawRect( x, y, w, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, COLOR_BORDER_GLOW_A );
		CheatMenu_DrawRect( x, y + h - 2, w, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, COLOR_BORDER_GLOW_A );
		CheatMenu_DrawRect( x, y, 2, h, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, COLOR_BORDER_GLOW_A );
		CheatMenu_DrawRect( x + w - 2, y, 2, h, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, COLOR_BORDER_GLOW_A );
	}
	else
	{
		CheatMenu_DrawRect( x, y, w, h, COLOR_BG_DARK_R, COLOR_BG_DARK_G, COLOR_BG_DARK_B, 220 );
	}
	
	// Purple border
	CheatMenu_DrawRect( x, y, w, 2, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y + h - 2, w, 2, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y, 2, h, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x + w - 2, y, 2, h, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	
	// Center text
	int text_len = Q_strlen( label );
	int text_x = x + w / 2 - ( text_len * 6 );
	if( selected || hover )
		CheatMenu_DrawText( text_x, y + h / 2 - 8, label, COLOR_TEXT_TITLE_R, COLOR_TEXT_TITLE_G, COLOR_TEXT_TITLE_B );
	else
		CheatMenu_DrawText( text_x, y + h / 2 - 8, label, COLOR_TEXT_R, COLOR_TEXT_G, COLOR_TEXT_B );
}

/*
==================
CheatMenu_DrawMainMenu
Draw main menu with modern design
==================
*/
static void CheatMenu_DrawMainMenu( void )
{
	int x, y;
	CheatMenu_GetMenuPosition( &x, &y );
	
	// Draw shadow
	CheatMenu_DrawShadow( x, y, MENU_WIDTH, MENU_HEIGHT, 150 );
	
	// Draw main background with gradient
	CheatMenu_DrawRect( x, y, MENU_WIDTH, MENU_HEIGHT, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B, COLOR_BG_A );
	
	// Draw darker top section for title
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 60, COLOR_BG_DARK_R, COLOR_BG_DARK_G, COLOR_BG_DARK_B, COLOR_BG_A );
	
	// Draw purple border with glow
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y + MENU_HEIGHT - 4, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x + MENU_WIDTH - 4, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	
	// Glow effect on borders
	CheatMenu_DrawRect( x - 1, y - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y + MENU_HEIGHT - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x + MENU_WIDTH - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	
	// Draw title "eBash3D Recode" with glow
	CheatMenu_DrawText( x + MENU_WIDTH / 2 - 85, y + 28, "eBash3D Recode", COLOR_TEXT_TITLE_R, COLOR_TEXT_TITLE_G, COLOR_TEXT_TITLE_B );
	
	// Draw menu items
	int item_y = y + 75;
	const char *items[] = { "Visual", "Aimbot", "Movement", "Misc", "Close" };
	int num_items = 5;
	
	for( int i = 0; i < num_items; i++ )
	{
		CheatMenu_DrawButton( x + 20, item_y, MENU_WIDTH - 40, ITEM_HEIGHT, items[i], selected_item == i, false );
		item_y += ITEM_HEIGHT + ITEM_SPACING;
	}
	
	// Draw hint
	CheatMenu_DrawText( x + 25, y + MENU_HEIGHT - 40, "Use arrow keys or mouse to navigate", COLOR_TEXT_SECONDARY_R, COLOR_TEXT_SECONDARY_G, COLOR_TEXT_SECONDARY_B );
	CheatMenu_DrawText( x + 25, y + MENU_HEIGHT - 25, "Press ENTER or click to select", COLOR_TEXT_SECONDARY_R, COLOR_TEXT_SECONDARY_G, COLOR_TEXT_SECONDARY_B );
}

/*
==================
CheatMenu_GetItemCount
Get total item count for current menu
==================
*/
static int CheatMenu_GetItemCount( void )
{
	switch( menu_state )
	{
	case CHEATMENU_VISUAL: return 10; // ESP + viewmodel + colors
	case CHEATMENU_AIMBOT: return 12; // Aimbot settings
	case CHEATMENU_MOVEMENT: return 4; // Movement settings (speed, auto strafe, ground strafe, fakelag)
	case CHEATMENU_MISC: return 2; // Misc settings (cmd_block, debug)
	default: return 0;
	}
}

/*
==================
CheatMenu_DrawVisualMenu
Draw Visual menu (renamed from ESP, includes viewmodel glow)
==================
*/
static void CheatMenu_DrawVisualMenu( void )
{
	int x, y;
	CheatMenu_GetMenuPosition( &x, &y );
	int item_y = y + 70;
	
	// Draw shadow
	CheatMenu_DrawShadow( x, y, MENU_WIDTH, MENU_HEIGHT, 150 );
	
	// Draw background
	CheatMenu_DrawRect( x, y, MENU_WIDTH, MENU_HEIGHT, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B, COLOR_BG_A );
	
	// Draw darker top section
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 60, COLOR_BG_DARK_R, COLOR_BG_DARK_G, COLOR_BG_DARK_B, COLOR_BG_A );
	
	// Draw purple border with glow
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y + MENU_HEIGHT - 4, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x + MENU_WIDTH - 4, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	
	// Glow effect
	CheatMenu_DrawRect( x - 1, y - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y + MENU_HEIGHT - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x + MENU_WIDTH - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	
	// Draw title and back button
	CheatMenu_DrawText( x + 25, y + 28, "Visual Settings", COLOR_TEXT_TITLE_R, COLOR_TEXT_TITLE_G, COLOR_TEXT_TITLE_B );
	CheatMenu_DrawButton( x + MENU_WIDTH - 110, y + 18, 90, 35, "Back", selected_item == 99, false );
	
	int total_items = CheatMenu_GetItemCount();
	int max_items = MAX_VISIBLE_ITEMS;
	
	// Adjust scroll offset
	if( selected_item < scroll_offset )
		scroll_offset = selected_item;
	if( selected_item >= scroll_offset + max_items )
		scroll_offset = selected_item - max_items + 1;
	
	// Draw items with scroll
	int item_index = 0;
	for( int i = scroll_offset; i < total_items && item_index < max_items; i++ )
	{
		int draw_y = item_y + ( item_index * ( ITEM_HEIGHT + ITEM_SPACING ) );
		
		switch( i )
		{
		case 0: // ESP Enabled
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_esp" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "ESP Enabled", selected_item == i );
			break;
		}
		case 1: // ESP Boxes
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_esp_box" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "ESP Boxes", selected_item == i );
			break;
		}
		case 2: // Player Names
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_esp_name" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Player Names", selected_item == i );
			break;
		}
		case 3: // Weapon Names
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_esp_weapon" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Weapon Names", selected_item == i );
			break;
		}
		case 4: // Viewmodel Glow
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_viewmodel_glow" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Viewmodel Glow", selected_item == i );
			break;
		}
		case 5: // Custom FOV
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_custom_fov" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Custom FOV", selected_item == i );
			break;
		}
		case 6: // Custom FOV Value
		{
			float value = CheatMenu_GetCvarValue( "kek_custom_fov_value" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, 60.0f, 150.0f, "FOV Value", 0, selected_item == i );
			break;
		}
		case 7: // Visible Only
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_aimbot_visible_only" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Visible Only", selected_item == i );
			break;
		}
		case 8: // Draw FOV Circle
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_aimbot_draw_fov" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Draw FOV Circle", selected_item == i );
			break;
		}
		case 9: // Max Distance
		{
			float value = CheatMenu_GetCvarValue( "kek_aimbot_max_distance" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, 0.0f, 5000.0f, "Max Distance", 1, selected_item == i );
			break;
		}
		}
		item_index++;
	}
	
	// Draw scroll indicator
	if( total_items > max_items )
	{
		float scroll_pos = (float)scroll_offset / (float)( total_items - max_items );
		int scrollbar_y = y + 70;
		int scrollbar_h = MENU_HEIGHT - 100;
		int scrollbar_x = x + MENU_WIDTH - 15;
		CheatMenu_DrawRect( scrollbar_x, scrollbar_y, 5, scrollbar_h, 60, 60, 65, 200 );
		int thumb_h = Q_max( 20, (int)( scrollbar_h * ( (float)max_items / (float)total_items ) ) );
		int thumb_y = scrollbar_y + (int)( scroll_pos * ( scrollbar_h - thumb_h ) );
		CheatMenu_DrawRect( scrollbar_x, thumb_y, 5, thumb_h, COLOR_PURPLE_R, COLOR_PURPLE_G, COLOR_PURPLE_B, 200 );
	}
}

/*
==================
CheatMenu_DrawAimbotMenu
Draw Aimbot menu
==================
*/
static void CheatMenu_DrawAimbotMenu( void )
{
	int x, y;
	CheatMenu_GetMenuPosition( &x, &y );
	int item_y = y + 70;
	
	// Draw shadow
	CheatMenu_DrawShadow( x, y, MENU_WIDTH, MENU_HEIGHT, 150 );
	
	// Draw background
	CheatMenu_DrawRect( x, y, MENU_WIDTH, MENU_HEIGHT, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B, COLOR_BG_A );
	
	// Draw darker top section
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 60, COLOR_BG_DARK_R, COLOR_BG_DARK_G, COLOR_BG_DARK_B, COLOR_BG_A );
	
	// Draw purple border with glow
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y + MENU_HEIGHT - 4, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x + MENU_WIDTH - 4, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	
	// Glow effect
	CheatMenu_DrawRect( x - 1, y - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y + MENU_HEIGHT - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x + MENU_WIDTH - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	
	// Draw title and back button
	CheatMenu_DrawText( x + 25, y + 28, "Aimbot Settings", COLOR_TEXT_TITLE_R, COLOR_TEXT_TITLE_G, COLOR_TEXT_TITLE_B );
	CheatMenu_DrawButton( x + MENU_WIDTH - 110, y + 18, 90, 35, "Back", selected_item == 99, false );
	
	int total_items = CheatMenu_GetItemCount();
	int max_items = MAX_VISIBLE_ITEMS;
	
	// Adjust scroll offset
	if( selected_item < scroll_offset )
		scroll_offset = selected_item;
	if( selected_item >= scroll_offset + max_items )
		scroll_offset = selected_item - max_items + 1;
	
	// Draw items with scroll
	int item_index = 0;
	for( int i = scroll_offset; i < total_items && item_index < max_items; i++ )
	{
		int draw_y = item_y + ( item_index * ( ITEM_HEIGHT + ITEM_SPACING ) );
		
		switch( i )
		{
		case 0: // Aimbot Enabled
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_aimbot" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Aimbot Enabled", selected_item == i );
			break;
		}
		case 1: // FOV
		{
			float value = CheatMenu_GetCvarValue( "kek_aimbot_fov" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, 0.0f, 180.0f, "FOV", 0, selected_item == i );
			break;
		}
		case 2: // Smooth
		{
			float value = CheatMenu_GetCvarValue( "kek_aimbot_smooth" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, 0.0f, 1.0f, "Smooth", 1, selected_item == i );
			break;
		}
		case 3: // Perfect Silent
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_aimbot_psilent" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Perfect Silent", selected_item == i );
			break;
		}
		case 4: // Anti-Aim
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_antiaim" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Anti-Aim", selected_item == i );
			break;
		}
		case 5: // Anti-Aim Mode
		{
			float value = CheatMenu_GetCvarValue( "kek_antiaim_mode" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, 0.0f, 8.0f, "Anti-Aim Mode", 2, selected_item == i );
			break;
		}
		case 6: // Anti-Aim Speed
		{
			float value = CheatMenu_GetCvarValue( "kek_antiaim_speed" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, 0.0f, 10.0f, "Anti-Aim Speed", 3, selected_item == i );
			break;
		}
		case 7: // Jitter Range
		{
			float value = CheatMenu_GetCvarValue( "kek_antiaim_jitter_range" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, 0.0f, 180.0f, "Jitter Range", 4, selected_item == i );
			break;
		}
		case 8: // Fake Angle
		{
			float value = CheatMenu_GetCvarValue( "kek_antiaim_fake_angle" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, -180.0f, 180.0f, "Fake Angle", 5, selected_item == i );
			break;
		}
		case 9: // Aim X
		{
			float value = CheatMenu_GetCvarValue( "kek_aimbot_x" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, -10.0f, 10.0f, "Aim X Offset", 6, selected_item == i );
			break;
		}
		case 10: // Aim Y
		{
			float value = CheatMenu_GetCvarValue( "kek_aimbot_y" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, -10.0f, 10.0f, "Aim Y Offset", 7, selected_item == i );
			break;
		}
		case 11: // Aim Z
		{
			float value = CheatMenu_GetCvarValue( "kek_aimbot_z" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, -10.0f, 10.0f, "Aim Z Offset", 8, selected_item == i );
			break;
		}
		}
		item_index++;
	}
	
	// Draw scroll indicator
	if( total_items > max_items )
	{
		float scroll_pos = (float)scroll_offset / (float)( total_items - max_items );
		int scrollbar_y = y + 70;
		int scrollbar_h = MENU_HEIGHT - 100;
		int scrollbar_x = x + MENU_WIDTH - 15;
		CheatMenu_DrawRect( scrollbar_x, scrollbar_y, 5, scrollbar_h, 60, 60, 65, 200 );
		int thumb_h = Q_max( 20, (int)( scrollbar_h * ( (float)max_items / (float)total_items ) ) );
		int thumb_y = scrollbar_y + (int)( scroll_pos * ( scrollbar_h - thumb_h ) );
		CheatMenu_DrawRect( scrollbar_x, thumb_y, 5, thumb_h, COLOR_PURPLE_R, COLOR_PURPLE_G, COLOR_PURPLE_B, 200 );
	}
}

/*
==================
CheatMenu_DrawMovementMenu
Draw Movement menu  
==================
*/
static void CheatMenu_DrawMovementMenu( void )
{
	int x, y;
	CheatMenu_GetMenuPosition( &x, &y );
	int item_y = y + 70;
	
	// Draw shadow
	CheatMenu_DrawShadow( x, y, MENU_WIDTH, MENU_HEIGHT, 150 );
	
	// Draw background
	CheatMenu_DrawRect( x, y, MENU_WIDTH, MENU_HEIGHT, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B, COLOR_BG_A );
	
	// Draw darker top section
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 60, COLOR_BG_DARK_R, COLOR_BG_DARK_G, COLOR_BG_DARK_B, COLOR_BG_A );
	
	// Draw purple border with glow
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y + MENU_HEIGHT - 4, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x + MENU_WIDTH - 4, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	
	// Glow effect
	CheatMenu_DrawRect( x - 1, y - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y + MENU_HEIGHT - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x + MENU_WIDTH - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	
	CheatMenu_DrawText( x + 25, y + 28, "Movement Settings", COLOR_TEXT_TITLE_R, COLOR_TEXT_TITLE_G, COLOR_TEXT_TITLE_B );
	CheatMenu_DrawButton( x + MENU_WIDTH - 110, y + 18, 90, 35, "Back", selected_item == 99, false );
	
	int total_items = CheatMenu_GetItemCount();
	int max_items = MAX_VISIBLE_ITEMS;
	if( selected_item < scroll_offset ) scroll_offset = selected_item;
	if( selected_item >= scroll_offset + max_items ) scroll_offset = selected_item - max_items + 1;
	
	int item_index = 0;
	for( int i = scroll_offset; i < total_items && item_index < max_items; i++ )
	{
		int draw_y = item_y + ( item_index * ( ITEM_HEIGHT + ITEM_SPACING ) );
		switch( i )
		{
		case 0:
		{
			float value = CheatMenu_GetCvarValue( "ebash3d_speed_multiplier" );
			value = bound( 1.0f, value, 10.0f );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, 1.0f, 10.0f, "Speed Multiplier", 0, selected_item == i );
			break;
		}
		case 1:
		{
			qboolean enabled = CheatMenu_GetCvarValue( "ebash3d_auto_strafe" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Auto Strafe", selected_item == i );
			break;
		}
		case 2:
		{
			qboolean enabled = CheatMenu_GetCvarValue( "ebash3d_ground_strafe" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Ground Strafe", selected_item == i );
			break;
		}
		case 3:
		{
			float value = CheatMenu_GetCvarValue( "ebash3d_fakelag" );
			CheatMenu_DrawSlider( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, value, 0.0f, 500.0f, "Fake Lag (ms)", 1, selected_item == i );
			break;
		}
		}
		item_index++;
	}
}

/*
==================
CheatMenu_DrawMiscMenu
Draw Misc menu
==================
*/
static void CheatMenu_DrawMiscMenu( void )
{
	int x, y;
	CheatMenu_GetMenuPosition( &x, &y );
	int item_y = y + 70;
	
	// Draw shadow
	CheatMenu_DrawShadow( x, y, MENU_WIDTH, MENU_HEIGHT, 150 );
	
	// Draw background
	CheatMenu_DrawRect( x, y, MENU_WIDTH, MENU_HEIGHT, COLOR_BG_R, COLOR_BG_G, COLOR_BG_B, COLOR_BG_A );
	
	// Draw darker top section
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 60, COLOR_BG_DARK_R, COLOR_BG_DARK_G, COLOR_BG_DARK_B, COLOR_BG_A );
	
	// Draw purple border with glow
	CheatMenu_DrawRect( x, y, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y + MENU_HEIGHT - 4, MENU_WIDTH, 4, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	CheatMenu_DrawRect( x + MENU_WIDTH - 4, y, 4, MENU_HEIGHT, COLOR_BORDER_R, COLOR_BORDER_G, COLOR_BORDER_B, COLOR_BORDER_A );
	
	// Glow effect
	CheatMenu_DrawRect( x - 1, y - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y + MENU_HEIGHT - 1, MENU_WIDTH + 2, 2, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	CheatMenu_DrawRect( x + MENU_WIDTH - 1, y, 2, MENU_HEIGHT, COLOR_BORDER_GLOW_R, COLOR_BORDER_GLOW_G, COLOR_BORDER_GLOW_B, 80 );
	
	CheatMenu_DrawText( x + 25, y + 28, "Misc Settings", COLOR_TEXT_TITLE_R, COLOR_TEXT_TITLE_G, COLOR_TEXT_TITLE_B );
	CheatMenu_DrawButton( x + MENU_WIDTH - 110, y + 18, 90, 35, "Back", selected_item == 99, false );
	
	int total_items = CheatMenu_GetItemCount();
	int max_items = MAX_VISIBLE_ITEMS;
	
	int item_index = 0;
	for( int i = scroll_offset; i < total_items && item_index < max_items; i++ )
	{
		int draw_y = item_y + ( item_index * ( ITEM_HEIGHT + ITEM_SPACING ) );
		switch( i )
		{
		case 0:
		{
			qboolean enabled = CheatMenu_GetCvarValue( "ebash3d_cmd_block" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Block Server Commands", selected_item == i );
			break;
		}
		case 1:
		{
			qboolean enabled = CheatMenu_GetCvarValue( "kek_debug" ) > 0.0f;
			CheatMenu_DrawToggle( x + 20, draw_y, MENU_WIDTH - 40, ITEM_HEIGHT, enabled, "Debug Mode", selected_item == i );
			break;
		}
		}
		item_index++;
	}
}

/*
==================
CheatMenu_ProcessSliderInput
Process slider dragging
==================
*/
static qboolean CheatMenu_ProcessSliderInput( float x, float y, cheatmenu_state_t menu, int item_index, float min_val, float max_val, const char *cvar_name )
{
	extern ref_globals_t refState;
	int screen_x = (int)( x * refState.width );
	int screen_y = (int)( y * refState.height );
	
	int menu_x, menu_y;
	CheatMenu_GetMenuPosition( &menu_x, &menu_y );
	int item_y = menu_y + 70;
	
	int draw_y = item_y + ( ( item_index - scroll_offset ) * ( ITEM_HEIGHT + ITEM_SPACING ) );
	
	int slider_x = menu_x + 20 + ( MENU_WIDTH - 40 ) - SLIDER_WIDTH - 15;
	int slider_y = draw_y + ITEM_HEIGHT / 2 - SLIDER_HEIGHT / 2;
	
	// If dragging, always update based on X position
	// Otherwise check if mouse is over slider
	if( dragging_slider || ( screen_x >= slider_x - 5 && screen_x <= slider_x + SLIDER_WIDTH + 5 && 
	                         screen_y >= slider_y - 10 && screen_y <= slider_y + SLIDER_HEIGHT + 10 ) )
	{
		// Calculate normalized position based on mouse X position
		float normalized = (float)( screen_x - slider_x ) / (float)SLIDER_WIDTH;
		normalized = bound( 0.0f, normalized, 1.0f );
		float new_value = min_val + normalized * ( max_val - min_val );
		new_value = bound( min_val, new_value, max_val );
		CheatMenu_SetCvarValue( cvar_name, new_value );
		return true;
	}
	
	return false;
}

/*
==================
CheatMenu_ProcessMouseInput
Process mouse/touch input for menu interaction (with slider support)
==================
*/
static void CheatMenu_ProcessMouseInput( float x, float y, qboolean pressed, qboolean released )
{
	extern ref_globals_t refState;
	int menu_x, menu_y;
	CheatMenu_GetMenuPosition( &menu_x, &menu_y );
	
	// Convert normalized coordinates to screen coordinates
	int screen_x = (int)( x * refState.width );
	int screen_y = (int)( y * refState.height );
	
	// Handle slider dragging - FIX: stop dragging when mouse is released
	if( dragging_slider )
	{
		if( released )
		{
			// Stop dragging when mouse is released
			dragging_slider = false;
			dragging_slider_index = -1;
			return;
		}
		
		// Continue dragging while mouse is down (update on any mouse movement)
		if( mouse_down )
		{
			switch( menu_state )
			{
			case CHEATMENU_VISUAL:
				if( dragging_slider_index == 0 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 6, 60.0f, 150.0f, "kek_custom_fov_value" );
				else if( dragging_slider_index == 1 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 9, 0.0f, 5000.0f, "kek_aimbot_max_distance" );
				break;
			case CHEATMENU_AIMBOT:
				if( dragging_slider_index == 0 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 1, 0.0f, 180.0f, "kek_aimbot_fov" );
				else if( dragging_slider_index == 1 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 2, 0.0f, 1.0f, "kek_aimbot_smooth" );
				else if( dragging_slider_index == 2 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 5, 0.0f, 8.0f, "kek_antiaim_mode" );
				else if( dragging_slider_index == 3 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 6, 0.0f, 10.0f, "kek_antiaim_speed" );
				else if( dragging_slider_index == 4 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 7, 0.0f, 180.0f, "kek_antiaim_jitter_range" );
				else if( dragging_slider_index == 5 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 8, -180.0f, 180.0f, "kek_antiaim_fake_angle" );
				else if( dragging_slider_index == 6 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 9, -10.0f, 10.0f, "kek_aimbot_x" );
				else if( dragging_slider_index == 7 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 10, -10.0f, 10.0f, "kek_aimbot_y" );
				else if( dragging_slider_index == 8 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 11, -10.0f, 10.0f, "kek_aimbot_z" );
				break;
			case CHEATMENU_MOVEMENT:
				if( dragging_slider_index == 0 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 0, 1.0f, 10.0f, "ebash3d_speed_multiplier" );
				else if( dragging_slider_index == 1 )
					CheatMenu_ProcessSliderInput( x, y, menu_state, 3, 0.0f, 500.0f, "ebash3d_fakelag" );
				break;
			}
		}
		return;
	}
	
	if( !released )
		return;
	
	if( !CheatMenu_IsPointInRect( screen_x, screen_y, menu_x, menu_y, MENU_WIDTH, MENU_HEIGHT ) )
		return;
	
	switch( menu_state )
	{
	case CHEATMENU_MAIN:
	{
		int item_y = menu_y + 70;
		for( int i = 0; i < 5; i++ )
		{
			if( CheatMenu_IsPointInRect( screen_x, screen_y, menu_x + 15, item_y, MENU_WIDTH - 30, ITEM_HEIGHT ) )
			{
				if( i == 0 ) menu_state = CHEATMENU_VISUAL;
				else if( i == 1 ) menu_state = CHEATMENU_AIMBOT;
				else if( i == 2 ) menu_state = CHEATMENU_MOVEMENT;
				else if( i == 3 ) menu_state = CHEATMENU_MISC;
				else if( i == 4 ) menu_state = CHEATMENU_CLOSED;
				selected_item = 0;
				scroll_offset = 0;
				return;
			}
			item_y += ITEM_HEIGHT + ITEM_SPACING;
		}
		break;
	}
	case CHEATMENU_VISUAL:
	{
		if( CheatMenu_IsPointInRect( screen_x, screen_y, menu_x + MENU_WIDTH - 100, menu_y + 15, 80, 30 ) )
		{
			menu_state = CHEATMENU_MAIN;
			selected_item = 0;
			scroll_offset = 0;
			return;
		}
		
		int total_items = CheatMenu_GetItemCount();
		int max_items = MAX_VISIBLE_ITEMS;
		int item_y = menu_y + 65;
		
		for( int i = scroll_offset; i < total_items && i < scroll_offset + max_items; i++ )
		{
			int draw_y = item_y + ( ( i - scroll_offset ) * ( ITEM_HEIGHT + ITEM_SPACING ) );
			int item_x = menu_x + 20;
			int item_w = MENU_WIDTH - 40;
			
			// Check toggle clicks
			if( i <= 5 || i == 7 || i == 8 )
			{
				int toggle_x = item_x + item_w - TOGGLE_WIDTH - 10;
				if( CheatMenu_IsPointInRect( screen_x, screen_y, toggle_x, draw_y, TOGGLE_WIDTH, ITEM_HEIGHT ) )
				{
					const char *cvars[] = {
						"kek_esp", "kek_esp_box", "kek_esp_name", "kek_esp_weapon",
						"kek_viewmodel_glow", "kek_custom_fov",
						NULL, // slider
						"kek_aimbot_visible_only", "kek_aimbot_draw_fov"
					};
					if( cvars[i] )
					{
						float val = CheatMenu_GetCvarValue( cvars[i] );
						CheatMenu_SetCvarValue( cvars[i], val > 0.0f ? 0.0f : 1.0f );
					}
					return;
				}
			}
			
			// Check slider clicks
			if( i == 6 )
			{
				int slider_x = item_x + item_w - SLIDER_WIDTH - 10;
				int slider_y = draw_y + ITEM_HEIGHT / 2 - SLIDER_HEIGHT / 2;
				if( CheatMenu_IsPointInRect( screen_x, screen_y, slider_x - 5, slider_y - 10, SLIDER_WIDTH + 10, SLIDER_HEIGHT + 20 ) )
				{
					dragging_slider = true;
					dragging_slider_index = 0;
					CheatMenu_ProcessSliderInput( x, y, menu_state, i, 60.0f, 150.0f, "kek_custom_fov_value" );
					return;
				}
			}
			else if( i == 9 )
			{
				int slider_x = item_x + item_w - SLIDER_WIDTH - 10;
				int slider_y = draw_y + ITEM_HEIGHT / 2 - SLIDER_HEIGHT / 2;
				if( CheatMenu_IsPointInRect( screen_x, screen_y, slider_x - 5, slider_y - 10, SLIDER_WIDTH + 10, SLIDER_HEIGHT + 20 ) )
				{
					dragging_slider = true;
					dragging_slider_index = 1;
					CheatMenu_ProcessSliderInput( x, y, menu_state, i, 0.0f, 5000.0f, "kek_aimbot_max_distance" );
					return;
				}
			}
		}
		break;
	}
	case CHEATMENU_AIMBOT:
	{
		if( CheatMenu_IsPointInRect( screen_x, screen_y, menu_x + MENU_WIDTH - 100, menu_y + 15, 80, 30 ) )
		{
			menu_state = CHEATMENU_MAIN;
			selected_item = 0;
			scroll_offset = 0;
			return;
		}
		
		int total_items = CheatMenu_GetItemCount();
		int max_items = MAX_VISIBLE_ITEMS;
		int item_y = menu_y + 65;
		
		for( int i = scroll_offset; i < total_items && i < scroll_offset + max_items; i++ )
		{
			int draw_y = item_y + ( ( i - scroll_offset ) * ( ITEM_HEIGHT + ITEM_SPACING ) );
			int item_x = menu_x + 20;
			int item_w = MENU_WIDTH - 40;
			
			// Toggle items: 0, 3, 4
			if( i == 0 || i == 3 || i == 4 )
			{
				int toggle_x = item_x + item_w - TOGGLE_WIDTH - 10;
				if( CheatMenu_IsPointInRect( screen_x, screen_y, toggle_x, draw_y, TOGGLE_WIDTH, ITEM_HEIGHT ) )
				{
					const char *cvars[] = { "kek_aimbot", NULL, NULL, "kek_aimbot_psilent", "kek_antiaim" };
					if( cvars[i] )
					{
						float val = CheatMenu_GetCvarValue( cvars[i] );
						CheatMenu_SetCvarValue( cvars[i], val > 0.0f ? 0.0f : 1.0f );
					}
					return;
				}
			}
			
			// Slider items
			if( i == 1 || i == 2 || ( i >= 5 && i <= 11 ) )
			{
				int slider_x = item_x + item_w - SLIDER_WIDTH - 10;
				int slider_y = draw_y + ITEM_HEIGHT / 2 - SLIDER_HEIGHT / 2;
				if( CheatMenu_IsPointInRect( screen_x, screen_y, slider_x - 5, slider_y - 10, SLIDER_WIDTH + 10, SLIDER_HEIGHT + 20 ) )
				{
					dragging_slider = true;
					dragging_slider_index = ( i == 1 ) ? 0 : ( i == 2 ) ? 1 : ( i == 5 ) ? 2 : ( i == 6 ) ? 3 : ( i == 7 ) ? 4 : ( i == 8 ) ? 5 : ( i == 9 ) ? 6 : ( i == 10 ) ? 7 : 8;
					
					if( i == 1 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, 0.0f, 180.0f, "kek_aimbot_fov" );
					else if( i == 2 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, 0.0f, 1.0f, "kek_aimbot_smooth" );
					else if( i == 5 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, 0.0f, 8.0f, "kek_antiaim_mode" );
					else if( i == 6 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, 0.0f, 10.0f, "kek_antiaim_speed" );
					else if( i == 7 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, 0.0f, 180.0f, "kek_antiaim_jitter_range" );
					else if( i == 8 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, -180.0f, 180.0f, "kek_antiaim_fake_angle" );
					else if( i == 9 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, -10.0f, 10.0f, "kek_aimbot_x" );
					else if( i == 10 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, -10.0f, 10.0f, "kek_aimbot_y" );
					else if( i == 11 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, -10.0f, 10.0f, "kek_aimbot_z" );
					return;
				}
			}
		}
		break;
	}
	case CHEATMENU_MOVEMENT:
	{
		if( CheatMenu_IsPointInRect( screen_x, screen_y, menu_x + MENU_WIDTH - 100, menu_y + 15, 80, 30 ) )
		{
			menu_state = CHEATMENU_MAIN;
			selected_item = 0;
			scroll_offset = 0;
			return;
		}
		
		int total_items = CheatMenu_GetItemCount();
		int max_items = MAX_VISIBLE_ITEMS;
		int item_y = menu_y + 65;
		
		for( int i = scroll_offset; i < total_items && i < scroll_offset + max_items; i++ )
		{
			int draw_y = item_y + ( ( i - scroll_offset ) * ( ITEM_HEIGHT + ITEM_SPACING ) );
			int item_x = menu_x + 20;
			int item_w = MENU_WIDTH - 40;
			
			// Toggle items: 1, 2
			if( i == 1 || i == 2 )
			{
				int toggle_x = item_x + item_w - TOGGLE_WIDTH - 10;
				if( CheatMenu_IsPointInRect( screen_x, screen_y, toggle_x, draw_y, TOGGLE_WIDTH, ITEM_HEIGHT ) )
				{
					const char *cvars[] = { NULL, "ebash3d_auto_strafe", "ebash3d_ground_strafe", NULL };
					if( cvars[i] )
					{
						float val = CheatMenu_GetCvarValue( cvars[i] );
						CheatMenu_SetCvarValue( cvars[i], val > 0.0f ? 0.0f : 1.0f );
					}
					return;
				}
			}
			
			// Slider items: 0, 3
			if( i == 0 || i == 3 )
			{
				int slider_x = item_x + item_w - SLIDER_WIDTH - 10;
				int slider_y = draw_y + ITEM_HEIGHT / 2 - SLIDER_HEIGHT / 2;
				if( CheatMenu_IsPointInRect( screen_x, screen_y, slider_x - 5, slider_y - 10, SLIDER_WIDTH + 10, SLIDER_HEIGHT + 20 ) )
				{
					dragging_slider = true;
					dragging_slider_index = ( i == 0 ) ? 0 : 1;
					
					if( i == 0 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, 1.0f, 10.0f, "ebash3d_speed_multiplier" );
					else if( i == 3 ) CheatMenu_ProcessSliderInput( x, y, menu_state, i, 0.0f, 500.0f, "ebash3d_fakelag" );
					return;
				}
			}
		}
		break;
	}
	case CHEATMENU_MISC:
	{
		if( CheatMenu_IsPointInRect( screen_x, screen_y, menu_x + MENU_WIDTH - 100, menu_y + 15, 80, 30 ) )
		{
			menu_state = CHEATMENU_MAIN;
			selected_item = 0;
			scroll_offset = 0;
			return;
		}
		
		int total_items = CheatMenu_GetItemCount();
		int max_items = MAX_VISIBLE_ITEMS;
		int item_y = menu_y + 65;
		
		for( int i = scroll_offset; i < total_items && i < scroll_offset + max_items; i++ )
		{
			int draw_y = item_y + ( ( i - scroll_offset ) * ( ITEM_HEIGHT + ITEM_SPACING ) );
			int item_x = menu_x + 20;
			int item_w = MENU_WIDTH - 40;
			
			// All items are toggles
			int toggle_x = item_x + item_w - TOGGLE_WIDTH - 10;
			if( CheatMenu_IsPointInRect( screen_x, screen_y, toggle_x, draw_y, TOGGLE_WIDTH, ITEM_HEIGHT ) )
			{
				const char *cvars[] = { "ebash3d_cmd_block", "kek_debug" };
				if( cvars[i] )
				{
					float val = CheatMenu_GetCvarValue( cvars[i] );
					CheatMenu_SetCvarValue( cvars[i], val > 0.0f ? 0.0f : 1.0f );
				}
				return;
			}
		}
		break;
	}
	}
}

/*
==================
CheatMenu_Draw
Main drawing function
==================
*/
void CheatMenu_Draw( void )
{
	if( menu_state == CHEATMENU_CLOSED )
		return;
	
	// Update slider while dragging (called every frame)
	if( dragging_slider && mouse_down )
	{
		CheatMenu_ProcessMouseInput( mouse_x, mouse_y, true, false );
	}
	
	if( touch_active )
	{
		CheatMenu_ProcessMouseInput( touch_x, touch_y, false, true );
		touch_active = false;
	}
	
	if( cls.key_dest == key_game )
		Platform_SetCursorType( dc_arrow );
	
	switch( menu_state )
	{
	case CHEATMENU_MAIN:
		CheatMenu_DrawMainMenu();
		break;
	case CHEATMENU_VISUAL:
		CheatMenu_DrawVisualMenu();
		break;
	case CHEATMENU_AIMBOT:
		CheatMenu_DrawAimbotMenu();
		break;
	case CHEATMENU_MOVEMENT:
		CheatMenu_DrawMovementMenu();
		break;
	case CHEATMENU_MISC:
		CheatMenu_DrawMiscMenu();
		break;
	}
}

/*
==================
CheatMenu_Toggle
==================
*/
void CheatMenu_Toggle( void )
{
	if( menu_state == CHEATMENU_CLOSED )
	{
		menu_state = CHEATMENU_MAIN;
		selected_item = 0;
		scroll_offset = 0;
		Platform_SetCursorType( dc_arrow );
	}
	else
	{
		menu_state = CHEATMENU_CLOSED;
		dragging_slider = false;
		if( cls.key_dest == key_game && cls.state == ca_active )
			Platform_SetCursorType( dc_none );
	}
}

/*
==================
CheatMenu_Open
==================
*/
void CheatMenu_Open( void )
{
	menu_state = CHEATMENU_MAIN;
	selected_item = 0;
	scroll_offset = 0;
}

/*
==================
CheatMenu_Close
==================
*/
void CheatMenu_Close( void )
{
	menu_state = CHEATMENU_CLOSED;
	dragging_slider = false;
}

/*
==================
CheatMenu_HandleKey
==================
*/
qboolean CheatMenu_HandleKey( int key, qboolean down )
{
	if( menu_state == CHEATMENU_CLOSED )
		return false;
	
	if( !down )
		return false;
	
	switch( key )
	{
	case K_ESCAPE:
		if( menu_state == CHEATMENU_MAIN )
			CheatMenu_Close();
		else
		{
			menu_state = CHEATMENU_MAIN;
			selected_item = 0;
			scroll_offset = 0;
		}
		return true;
	case K_ENTER:
	case K_KP_ENTER:
		if( menu_state == CHEATMENU_MAIN )
		{
			if( selected_item == 0 ) menu_state = CHEATMENU_VISUAL;
			else if( selected_item == 1 ) menu_state = CHEATMENU_AIMBOT;
			else if( selected_item == 2 ) menu_state = CHEATMENU_MOVEMENT;
			else if( selected_item == 3 ) menu_state = CHEATMENU_MISC;
			else if( selected_item == 4 ) menu_state = CHEATMENU_CLOSED;
			selected_item = 0;
			scroll_offset = 0;
		}
		return true;
	case K_UPARROW:
	case K_MWHEELUP:
		if( menu_state == CHEATMENU_MAIN )
			selected_item = Q_max( 0, selected_item - 1 );
		else
			selected_item = Q_max( 0, selected_item - 1 );
		return true;
	case K_DOWNARROW:
	case K_MWHEELDOWN:
		if( menu_state == CHEATMENU_MAIN )
			selected_item = Q_min( 4, selected_item + 1 );
		else
		{
			int max_items = CheatMenu_GetItemCount();
			selected_item = Q_min( max_items - 1, selected_item + 1 );
		}
		return true;
	default:
		return false;
	}
}

/*
==================
CheatMenu_HandleTouch
==================
*/
void CheatMenu_HandleTouch( float x, float y )
{
	touch_x = x;
	touch_y = y;
	touch_active = true;
}

/*
==================
CheatMenu_HandleMouse
==================
*/
void CheatMenu_HandleMouse( float x, float y, qboolean down, qboolean up )
{
	mouse_x = x;
	mouse_y = y;
	
	// FIX: Stop dragging immediately when mouse is released
	if( up && dragging_slider )
	{
		mouse_down = false;
		CheatMenu_ProcessMouseInput( x, y, false, true );
		return;
	}
	
	// Update slider while dragging (on mouse move or press)
	if( dragging_slider && mouse_down )
	{
		CheatMenu_ProcessMouseInput( x, y, false, false );
		return;
	}
	
	if( down )
	{
		mouse_down = true;
		CheatMenu_ProcessMouseInput( x, y, true, false );
	}
	
	if( up )
	{
		mouse_down = false;
		CheatMenu_ProcessMouseInput( x, y, false, true );
	}
}

/*
==================
CheatMenu_IsOpen
==================
*/
qboolean CheatMenu_IsOpen( void )
{
	return menu_state != CHEATMENU_CLOSED;
}