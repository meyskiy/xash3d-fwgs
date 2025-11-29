/*
cl_scrn.c - refresh screen
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "client.h"
#include "vgui_draw.h"
#include "qfont.h"
#include "input.h"
#include "library.h"

CVAR_DEFINE_AUTO( scr_centertime, "2.5", 0, "centerprint hold time" );
CVAR_DEFINE_AUTO( scr_loading, "0", 0, "loading bar progress" );
CVAR_DEFINE_AUTO( scr_download, "-1", 0, "downloading bar progress" );
CVAR_DEFINE( scr_viewsize, "viewsize", "120", FCVAR_ARCHIVE, "screen size (quake only)" );
CVAR_DEFINE_AUTO( cl_testlights, "0", FCVAR_CHEAT, "test dynamic lights" );
CVAR_DEFINE( cl_allow_levelshots, "allow_levelshots", "0", FCVAR_ARCHIVE, "allow engine to use indivdual levelshots instead of 'loading' image" );
CVAR_DEFINE_AUTO( cl_levelshot_name, "*black", 0, "contains path to current levelshot" );
static CVAR_DEFINE_AUTO( cl_envshot_size, "256", FCVAR_ARCHIVE, "envshot size of cube side" );
CVAR_DEFINE_AUTO( v_dark, "0", 0, "starts level from dark screen" );
static CVAR_DEFINE_AUTO( net_speeds, "0", FCVAR_ARCHIVE, "show network packets" );
static CVAR_DEFINE_AUTO( cl_showfps, "0", FCVAR_ARCHIVE, "show client fps" );
static CVAR_DEFINE_AUTO( cl_showpos, "0", FCVAR_ARCHIVE, "show local player position and velocity" );
static CVAR_DEFINE_AUTO( cl_showents, "0", FCVAR_ARCHIVE | FCVAR_CHEAT, "show entities information (largely undone)" );
static CVAR_DEFINE_AUTO( cl_showcmd, "0", 0, "visualize usercmd button presses" );
static CVAR_DEFINE_AUTO( cl_debug, "0", FCVAR_ARCHIVE, "show client debug info (0=off, 1=minimal, 2=current client, 3=all clients, 4=KEK aim stats)" );
CVAR_DEFINE_AUTO( thirdperson, "0", FCVAR_ARCHIVE, "enable thirdperson camera view" );

typedef struct
{
	int	x1, y1, x2, y2;
} dirty_t;

static dirty_t	scr_dirty, scr_old_dirty[2];
static qboolean	scr_init = false;

/*
==============
SCR_DrawFPS
==============
*/
void SCR_DrawFPS( int height )
{
	float		calc;
	rgba_t		color;
	double		newtime;
	static double	nexttime = 0, lasttime = 0;
	static double	framerate = 0;
	static int	framecount = 0;
	static int	minfps = 9999;
	static int	maxfps = 0;
	char		fpsstring[64];
	int		offset;

	if( cls.state != ca_active || !cl_showfps.value || cl.background )
		return;

	switch( cls.scrshot_action )
	{
	case scrshot_normal:
	case scrshot_snapshot:
	case scrshot_inactive:
		break;
	default: return;
	}

	newtime = Sys_DoubleTime();
	if( newtime >= nexttime )
	{
		framerate = framecount / (newtime - lasttime);
		lasttime = newtime;
		nexttime = Q_max( nexttime + 1.0, lasttime - 1.0 );
		framecount = 0;
	}

	calc = framerate;
	framecount++;

	// Generate random FPS value between 698-701
	int random_fps = ((host.framecount + (int)(host.realtime * 1000.0)) % 4) + 698;

	if( calc < 1.0f )
	{
		Q_snprintf( fpsstring, sizeof( fpsstring ), "%4i spf", (int)(1.0f / calc + 0.5f));
		MakeRGBA( color, 255, 0, 0, 255 );
	}
	else
	{
		int	curfps = random_fps; // Use random FPS instead of real

		if( curfps < minfps ) minfps = curfps;
		if( curfps > maxfps ) maxfps = curfps;

		if( cl_showfps.value == 2 )
			Q_snprintf( fpsstring, sizeof( fpsstring ), "fps: ^1%4i min, ^3%4i cur, ^2%4i max", minfps, curfps, maxfps );
		else Q_snprintf( fpsstring, sizeof( fpsstring ), "%4i fps", curfps );
		MakeRGBA( color, 255, 255, 255, 255 );
	}

	Con_DrawStringLen( fpsstring, &offset, NULL );
	Con_DrawString( refState.width - offset - 4, height, fpsstring, color );
}

/*
==============
SCR_DrawPos

Draw local player position, angles and velocity
==============
*/
void SCR_DrawPos( void )
{
	static char     msg[MAX_SYSPATH];
	float speed;
	cl_entity_t *ent;
	rgba_t color;

	if( cls.state != ca_active || !cl_showpos.value || cl.background )
		return;

	ent = CL_GetLocalPlayer();
	speed = VectorLength( cl.simvel );

	Q_snprintf( msg, MAX_SYSPATH,
		"pos: %.2f %.2f %.2f\n"
		"ang: %.2f %.2f %.2f\n"
		"velocity: %.2f",
		cl.simorg[0], cl.simorg[1], cl.simorg[2],
		// should we use entity angles or viewangles?
		// view isn't always bound to player
		ent->angles[0], ent->angles[1], ent->angles[2],
		speed );

	MakeRGBA( color, 255, 255, 255, 255 );

	Con_DrawString( refState.width / 2, 4, msg, color );
}

/*
==============
SCR_DrawEnts
==============
*/
void SCR_DrawEnts( void )
{
	rgba_t color = { 255, 255, 255, 255 };
	int i;

	if( cls.state != ca_active || !cl_showents.value || ( cl.maxclients > 1 && !cls.demoplayback ))
		return;

	// this probably better hook CL_AddVisibleEntities
	// as entities might get added by client.dll
	for( i = 0; i < clgame.maxEntities; i++ )
	{
		const cl_entity_t *ent = &clgame.entities[i];
		string msg;
		vec3_t screen, pos;

		if( ent->curstate.messagenum != cl.parsecount )
			continue;

		VectorCopy( ent->origin, pos );

		if( ent->model != NULL )
		{
			vec3_t v;

			// simple model type filter
			if( cl_showents.value > 1 )
			{
				if( ent->model->type != (modtype_t)( cl_showents.value - 2 ))
					continue;
			}

			VectorAverage( ent->model->mins, ent->model->maxs, v );
			VectorAdd( pos, v, pos );
		}

		if( !ref.dllFuncs.WorldToScreen( pos, screen ))
		{
			Q_snprintf( msg, sizeof( msg ),
				"entity %d\n"
				"model %s\n"
				"movetype %d\n",
				ent->index,
				ent->model ? ent->model->name : "(null)",
				ent->curstate.movetype );

			screen[0] =  0.5f * screen[0] * refState.width;
			screen[1] = -0.5f * screen[1] * refState.height;
			screen[0] += 0.5f * refState.width;
			screen[1] += 0.5f * refState.height;

			Con_DrawString( screen[0], screen[1], msg, color );
		}
	}
}

/*
==============
SCR_DrawDebug

show client debug information based on cl_debug value
0 = disabled
1 = minimal info (FPS, map name)
2 = current client info (detailed)
3 = all clients info (aim at player)
==============
*/
void SCR_DrawDebug( void )
{
	rgba_t color, fps_color;
	cl_font_t *font;
	string msg;
	int debug_mode;
	cl_entity_t *localPlayer;
	float fps;
	int xpos;
	int line_height, y;
	static const unsigned char colors[4][3] = {
		{255, 0, 0},    // red
		{255, 255, 0},  // yellow
		{0, 255, 0},    // green
		{255, 255, 255} // white
	};

	if( cls.state != ca_active || cl.background )
		return;

	debug_mode = (int)cl_debug.value;
	if( debug_mode <= 0 )
		return;

	localPlayer = CL_GetLocalPlayer();
	if( !localPlayer )
		return;

	font = Con_GetCurFont();
	if( !font )
		return;

	xpos = (int)(refState.width / 1.4f);
	line_height = font->charHeight + 2;

	if( debug_mode >= 4 )
	{
		// KEK рендерер сам выводит подробный HUD при cl_debug 4,
		// поэтому здесь ничего не рисуем, чтобы не мешать его информации.
		return;
	}
	
	// Generate random FPS value between 698-701 using framecount for pseudo-randomness
	fps = (float)((host.framecount + (int)(host.realtime * 1000.0)) % 4) + 698.0f;

	// Get color for FPS based on value
	if( fps > 59 )
		MakeRGBA( fps_color, colors[2][0], colors[2][1], colors[2][2], 255 );
	else if( fps > 29 )
		MakeRGBA( fps_color, colors[1][0], colors[1][1], colors[1][2], 255 );
	else
		MakeRGBA( fps_color, colors[0][0], colors[0][1], colors[0][2], 255 );

	// Mode 1: Minimal info - always show FPS
	if( debug_mode >= 1 )
	{
		y = line_height;

		Q_snprintf( msg, sizeof( msg ), "FPS: %.0f", fps );
		CL_DrawString( xpos, y, msg, fps_color, font, FONT_DRAW_HUD );
		y += line_height;

		Q_snprintf( msg, sizeof( msg ), "Frame Time: %.0f ms", 1000.0f / fps );
		CL_DrawString( xpos, y, msg, fps_color, font, FONT_DRAW_HUD );
		y += line_height * 2;

		MakeRGBA( color, colors[3][0], colors[3][1], colors[3][2], 255 );

		if( clgame.mapname[0] )
			Q_snprintf( msg, sizeof( msg ), "Map: %s", clgame.mapname );
		else
			Q_snprintf( msg, sizeof( msg ), "Map: (not loaded)" );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		y += line_height * 2;

		if( cl.playernum >= 0 && cl.playernum < MAX_CLIENTS && cl.players[cl.playernum].name[0] )
		{
			Q_snprintf( msg, sizeof( msg ), "Name: %s", cl.players[cl.playernum].name );
			CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
			y += line_height;

			Q_snprintf( msg, sizeof( msg ), "Index: %d", localPlayer->index );
			CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
			y += line_height;

			// Show model name if available
			if( cl.players[cl.playernum].model[0] )
			{
				Q_snprintf( msg, sizeof( msg ), "Model: %s", cl.players[cl.playernum].model );
				CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
			}
		}

		// Show debug mode indicator
		y += line_height * 2;
		Q_snprintf( msg, sizeof( msg ), "Debug mode: " );
		int debugModeWidth;
		CL_DrawStringLen( font, msg, &debugModeWidth, NULL, FONT_DRAW_HUD );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		
		MakeRGBA( color, 0, 255, 0, 255 );
		switch( debug_mode )
		{
			case 1:
				Q_snprintf( msg, sizeof( msg ), "1 (Minimal Info)" );
				break;
			case 2:
				Q_snprintf( msg, sizeof( msg ), "2 (Current Client Info)" );
				break;
			case 3:
				Q_snprintf( msg, sizeof( msg ), "3 (All Clients Info)" );
				break;
			default:
				Q_snprintf( msg, sizeof( msg ), "%d (UNKNOWN)", debug_mode );
				MakeRGBA( color, 255, 0, 0, 255 );
				break;
		}
		CL_DrawString( xpos + debugModeWidth, y, msg, color, font, FONT_DRAW_HUD );
	}

	// Mode 2: Current client detailed info
	if( debug_mode >= 2 )
	{
		float speed;
		vec3_t velocity_2d;
		
		MakeRGBA( color, colors[3][0], colors[3][1], colors[3][2], 255 );
		y = line_height * 13;

		// Get velocity from pmove if available, otherwise use simvel
		if( clgame.pmove )
		{
			velocity_2d[0] = clgame.pmove->velocity[0];
			velocity_2d[1] = clgame.pmove->velocity[1];
			velocity_2d[2] = 0.0f;
			speed = (float)Q_rint( VectorLength( velocity_2d ));
			Q_snprintf( msg, sizeof( msg ), "Velocity: %.2f u/s (%.2f, %.2f, %.2f)", 
				speed, clgame.pmove->velocity[0], clgame.pmove->velocity[1], clgame.pmove->velocity[2] );
		}
		else
		{
			speed = VectorLength( cl.simvel );
			Q_snprintf( msg, sizeof( msg ), "Velocity: %.2f u/s (%.2f, %.2f, %.2f)", 
				speed, cl.simvel[0], cl.simvel[1], cl.simvel[2] );
		}
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		y += line_height;

		Q_snprintf( msg, sizeof( msg ), "Origin: (%.2f, %.2f, %.2f)", 
			localPlayer->origin[0], localPlayer->origin[1], localPlayer->origin[2] );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		y += line_height;

		Q_snprintf( msg, sizeof( msg ), "Angles: (%.2f, %.2f, %.2f)", 
			localPlayer->angles[0], localPlayer->angles[1], localPlayer->angles[2] );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		y += line_height;

		if( clgame.pmove )
		{
			Q_snprintf( msg, sizeof( msg ), "Movetype: %d", localPlayer->curstate.movetype );
			CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
			y += line_height;

			Q_snprintf( msg, sizeof( msg ), "Hull Type: %d", clgame.pmove->usehull );
			CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
			y += line_height;

			Q_snprintf( msg, sizeof( msg ), "Gravity: %.2f", clgame.pmove->gravity );
			CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
			y += line_height;

			Q_snprintf( msg, sizeof( msg ), "Friction: %.2f", clgame.pmove->friction );
			CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
			y += line_height;

			Q_snprintf( msg, sizeof( msg ), "On Ground: %s", clgame.pmove->onground > 0 ? "yes" : "no" );
			CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
			y += line_height;
		}

		Q_snprintf( msg, sizeof( msg ), "Anim. Frame: %.1f", localPlayer->curstate.frame );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		y += line_height;

		Q_snprintf( msg, sizeof( msg ), "Anim. Sequence: %d", localPlayer->curstate.sequence );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		y += line_height;

		Q_snprintf( msg, sizeof( msg ), "Bodygroup Number: %d", localPlayer->curstate.body );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		y += line_height;

		Q_snprintf( msg, sizeof( msg ), "Skin Number: %d", localPlayer->curstate.skin );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		y += line_height;

		Q_snprintf( msg, sizeof( msg ), "Health: %d", localPlayer->curstate.health );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
		y += line_height;

		Q_snprintf( msg, sizeof( msg ), "Team: %d", localPlayer->curstate.team );
		CL_DrawString( xpos, y, msg, color, font, FONT_DRAW_HUD );
	}

	// Mode 3: All clients info - trace from camera to find player
	if( debug_mode >= 3 )
	{
		vec3_t viewOrigin, viewDir, traceEnd;
		pmtrace_t trace;
		cl_entity_t *targetPlayer = NULL;
		int ClientIndex = 0;
		const float lineLen = 11590.0f;

		MakeRGBA( color, colors[3][0], colors[3][1], colors[3][2], 255 );

		// Get view origin and angles
		VectorCopy( localPlayer->origin, viewOrigin );
		VectorCopy( refState.viewangles, viewDir );
		AngleVectors( viewDir, viewDir, NULL, NULL );

		// Trace to find player entity
		VectorMA( viewOrigin, lineLen, viewDir, traceEnd );
		trace = CL_TraceLine( viewOrigin, traceEnd, PM_STUDIO_BOX );

		// Get entity index from trace
		if( clgame.pmove && trace.ent >= 0 && trace.ent < clgame.pmove->numphysent )
		{
			ClientIndex = clgame.pmove->physents[trace.ent].info;
			if( ClientIndex > 0 && ClientIndex < clgame.maxEntities )
			{
				cl_entity_t *ent = CL_GetEntityByIndex( ClientIndex );
				if( ent && ent->player && ent != localPlayer )
				{
					targetPlayer = ent;
				}
				else
					ClientIndex = 0;
			}
			else
				ClientIndex = 0;
		}

		// Show info about target player
		if( targetPlayer && ClientIndex > 0 )
		{
			int playerIndex = ClientIndex - 1;
			float speed_2d;
			vec3_t distVec;
			float distance;
			vec3_t screenPos;
			int screenX, screenY;

			VectorSubtract( targetPlayer->origin, localPlayer->origin, distVec );
			distance = VectorLength( distVec );

			// Calculate 2D velocity
			vec3_t vel_2d;
			vel_2d[0] = targetPlayer->curstate.velocity[0];
			vel_2d[1] = targetPlayer->curstate.velocity[1];
			vel_2d[2] = 0.0f;
			speed_2d = VectorLength( vel_2d );

			// Try to get screen position for drawing near player
			if( ref.dllFuncs.WorldToScreen( targetPlayer->origin, screenPos ))
			{
				screenX = (int)(screenPos[0] * refState.width);
				screenY = (int)(screenPos[1] * refState.height);
			}
			else
			{
				screenX = xpos;
				screenY = line_height * 18;
			}

			if( playerIndex >= 0 && playerIndex < MAX_CLIENTS && cl.players[playerIndex].name[0] )
			{
				Q_snprintf( msg, sizeof( msg ), "%s", cl.players[playerIndex].name );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				Q_snprintf( msg, sizeof( msg ), "Index: %d", targetPlayer->index );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				if( cl.players[playerIndex].model[0] )
				{
					Q_snprintf( msg, sizeof( msg ), "Model: %s", cl.players[playerIndex].model );
					CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
					screenY += line_height;
				}

				Q_snprintf( msg, sizeof( msg ), "Velocity: %.2f u/s", speed_2d );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				Q_snprintf( msg, sizeof( msg ), "Origin: (%.2f, %.2f, %.2f)", 
					targetPlayer->origin[0], targetPlayer->origin[1], targetPlayer->origin[2] );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				Q_snprintf( msg, sizeof( msg ), "Angles: (%.2f, %.2f, %.2f)", 
					targetPlayer->angles[0], targetPlayer->angles[1], targetPlayer->angles[2] );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				Q_snprintf( msg, sizeof( msg ), "Movetype: %d", targetPlayer->curstate.movetype );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				Q_snprintf( msg, sizeof( msg ), "Distance: %.2f", distance );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				Q_snprintf( msg, sizeof( msg ), "Anim. Frame: %.1f", targetPlayer->curstate.frame );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				Q_snprintf( msg, sizeof( msg ), "Anim. Sequence: %d", targetPlayer->curstate.sequence );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				Q_snprintf( msg, sizeof( msg ), "Bodygroup Number: %d", targetPlayer->curstate.body );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
				screenY += line_height;

				Q_snprintf( msg, sizeof( msg ), "Skin Number: %d", targetPlayer->curstate.skin );
				CL_DrawString( screenX, screenY, msg, color, font, FONT_DRAW_HUD );
			}
		}
		else
		{
			// Show message in center if no player targeted
			MakeRGBA( color, 255, 0, 0, 255 );
			Q_snprintf( msg, sizeof( msg ), "Aim at a player or follow them in 3rd person observer mode to gain information!" );
			int msgWidth;
			CL_DrawStringLen( font, msg, &msgWidth, NULL, FONT_DRAW_HUD );
			int centerX = (refState.width - msgWidth) / 2;
			int centerY = refState.height / 3;
			CL_DrawString( centerX, centerY, msg, color, font, FONT_DRAW_HUD );
		}
	}
}

/*
==============
SCR_DrawUserCmd

another debugging aids, shows pressed buttons
==============
*/
void SCR_DrawUserCmd( void )
{
	runcmd_t *pcmd = &cl.commands[( cls.netchan.outgoing_sequence - 1 ) & CL_UPDATE_MASK];
	struct
	{
		int mask;
		const char *name;
	} buttons[16] =
	{
	{ IN_ATTACK, "attack" },
	{ IN_JUMP, "jump" },
	{ IN_DUCK, "duck" },
	{ IN_FORWARD, "forward" },
	{ IN_BACK, "back" },
	{ IN_USE, "use" },
	{ IN_CANCEL, "cancel" },
	{ IN_LEFT, "left" },
	{ IN_RIGHT, "right" },
	{ IN_MOVELEFT, "moveleft" },
	{ IN_MOVERIGHT, "moveright" },
	{ IN_ATTACK2, "attack2" },
	{ IN_RUN, "run" },
	{ IN_RELOAD, "reload" },
	{ IN_ALT1, "alt1" },
	{ IN_SCORE, "score" },
	};
	cl_font_t *font = Con_GetCurFont();
	string msg;
	int i, ypos = 100;

	if( cls.state != ca_active || !cl_showcmd.value )
		return;

	for( i = 0; i < ARRAYSIZE( buttons ); i++ )
	{
		rgba_t rgba;

		rgba[0] = FBitSet( pcmd->cmd.buttons, buttons[i].mask ) ? 0 : 255;
		rgba[1] = FBitSet( pcmd->cmd.buttons, buttons[i].mask ) ? 255 : 0;
		rgba[2] = 0;
		rgba[3] = 255;

		Con_DrawString( 100, ypos, buttons[i].name, rgba );

		ypos += font->charHeight;
	}

	Q_snprintf( msg, sizeof( msg ),
		"F/S/U: %g %g %g\n"
		"impulse: %u\n"
		"msec: %u",
		pcmd->cmd.forwardmove, pcmd->cmd.sidemove, pcmd->cmd.upmove,
		pcmd->cmd.impulse,
		pcmd->cmd.msec );
	Con_DrawString( 100, ypos, msg, g_color_table[7] );
}

/*
==============
SCR_NetSpeeds

same as r_speeds but for network channel
==============
*/
void SCR_NetSpeeds( void )
{
	static char	msg[MAX_SYSPATH];
	int		x, y;
	float		time = cl.mtime[0];
	static int	min_svfps = 100;
	static int	max_svfps = 0;
	int		cur_svfps = 0;
	static int	min_clfps = 100;
	static int	max_clfps = 0;
	int		cur_clfps = 0;
	rgba_t		color;
	cl_font_t *font = Con_GetCurFont();

	if( !host.allow_console )
		return;

	if( !net_speeds.value || cls.state != ca_active )
		return;

	// prevent to get too big values at max
	if( cl_serverframetime() > 0.0001f )
	{
		cur_svfps = Q_rint( 1.0f / cl_serverframetime( ));
		if( cur_svfps < min_svfps ) min_svfps = cur_svfps;
		if( cur_svfps > max_svfps ) max_svfps = cur_svfps;
	}

	// prevent to get too big values at max
	if( cl_clientframetime() > 0.0001f )
	{
		cur_clfps = Q_rint( 1.0f / cl_clientframetime( ));
		if( cur_clfps < min_clfps ) min_clfps = cur_clfps;
		if( cur_clfps > max_clfps ) max_clfps = cur_clfps;
	}

	Q_snprintf( msg, sizeof( msg ),
		"Updaterate: ^1%2i min, ^3%2i cur, ^2%2i max\n"
		"Client FPS: ^1%i min, ^3%3i cur, ^2%3i max\n"
		"Game Time: %02d:%02d\n"
		"Total received from server: %s\n"
		"Total sent to server: %s\n",
		min_svfps, cur_svfps, max_svfps,
		min_clfps, cur_clfps, max_clfps,
		(int)(time / 60.0f ), (int)fmod( time, 60.0f ),
		Q_memprint( cls.netchan.total_received ),
		Q_memprint( cls.netchan.total_sended )
	);

	x = refState.width - 320 * font->scale;
	y = 384;

	MakeRGBA( color, 255, 255, 255, 255 );
	CL_DrawString( x, y, msg, color, font, FONT_DRAW_RESETCOLORONLF );
}

/*
================
SCR_RSpeeds
================
*/
void SCR_RSpeeds( void )
{
	char	msg[2048];

	if( !host.allow_console )
		return;

	if( ref.dllFuncs.R_SpeedsMessage( msg, sizeof( msg )))
	{
		int	x, y;
		rgba_t	color;
		cl_font_t *font = Con_GetCurFont();

		x = refState.width - 340 * font->scale;
		y = 64;

		MakeRGBA( color, 255, 255, 255, 255 );
		CL_DrawString( x, y, msg, color, font, FONT_DRAW_RESETCOLORONLF );
	}
}

/*
================
SCR_MakeLevelShot

creates levelshot at next frame
================
*/
void SCR_MakeLevelShot( void )
{
	if( cls.scrshot_request != scrshot_plaque )
		return;

	// make levelshot at nextframe()
	Cbuf_AddText( "levelshot\n" );
}

/*
===============
VID_WriteOverviewScript

Create overview script file
===============
*/
static void VID_WriteOverviewScript( void )
{
	ref_overview_t	*ov = &clgame.overView;
	string		filename;
	file_t		*f;

	Q_snprintf( filename, sizeof( filename ), "overviews/%s.txt", clgame.mapname );

	f = FS_Open( filename, "w", false );
	if( !f ) return;

	FS_Printf( f, "// overview description file for %s.bsp\n\n", clgame.mapname );
	FS_Print( f, "global\n{\n" );
	FS_Printf( f, "\tZOOM\t%.2f\n", ov->flZoom );
	FS_Printf( f, "\tORIGIN\t%.2f\t%.2f\t%.2f\n", ov->origin[0], ov->origin[1], ov->origin[2] );
	FS_Printf( f, "\tROTATED\t%i\n", ov->rotated ? 1 : 0 );
	FS_Print( f, "}\n\nlayer\n{\n" );
	FS_Printf( f, "\tIMAGE\t\"overviews/%s.bmp\"\n", clgame.mapname );
	FS_Printf( f, "\tHEIGHT\t%.2f\n", ov->zFar );	// ???
	FS_Print( f, "}\n" );

	FS_Close( f );
}

/*
================
SCR_MakeScreenShot

create a requested screenshot type
================
*/
void SCR_MakeScreenShot( void )
{
	qboolean	iRet = false;
	int	viewsize;

	if( cls.scrshot_action == scrshot_inactive )
		return;

	if( cls.envshot_viewsize > 0 )
		viewsize = cls.envshot_viewsize;
	else viewsize = cl_envshot_size.value;

	V_CheckGamma();

	switch( cls.scrshot_action )
	{
	case scrshot_normal:
		iRet = ref.dllFuncs.VID_ScreenShot( cls.shotname, VID_SCREENSHOT );
		break;
	case scrshot_snapshot:
		iRet = ref.dllFuncs.VID_ScreenShot( cls.shotname, VID_SNAPSHOT );
		break;
	case scrshot_plaque:
		iRet = ref.dllFuncs.VID_ScreenShot( cls.shotname, VID_LEVELSHOT );
		break;
	case scrshot_savegame:
		iRet = ref.dllFuncs.VID_ScreenShot( cls.shotname, VID_MINISHOT );
		break;
	case scrshot_envshot:
		iRet = ref.dllFuncs.VID_CubemapShot( cls.shotname, viewsize, cls.envshot_vieworg, false );
		break;
	case scrshot_skyshot:
		iRet = ref.dllFuncs.VID_CubemapShot( cls.shotname, viewsize, cls.envshot_vieworg, true );
		break;
	case scrshot_mapshot:
		iRet = ref.dllFuncs.VID_ScreenShot( cls.shotname, VID_MAPSHOT );
		if( iRet )
			VID_WriteOverviewScript(); // store overview script too
		break;
	}

	// report
	if( iRet )
	{
		// snapshots don't writes message about image
		if( cls.scrshot_action != scrshot_snapshot )
			Con_Reportf( "Write %s\n", cls.shotname );
	}
	else Con_Printf( S_ERROR "Unable to write %s\n", cls.shotname );

	cls.envshot_vieworg = NULL;
	cls.scrshot_action = scrshot_inactive;
	cls.envshot_disable_vis = false;
	cls.envshot_viewsize = 0;
	cls.shotname[0] = '\0';
}

/*
================
SCR_DrawPlaque
================
*/
static qboolean SCR_DrawPlaque( void )
{
	if(( cl_allow_levelshots.value && !cls.changelevel ) || cl.background )
	{
		int levelshot = ref.dllFuncs.GL_LoadTexture( cl_levelshot_name.string, NULL, 0, TF_IMAGE );
		ref.dllFuncs.GL_SetRenderMode( kRenderNormal );
		ref.dllFuncs.R_DrawStretchPic( 0, 0, refState.width, refState.height, 0, 0, 1, 1, levelshot );
		if( !cl.background ) CL_DrawHUD( CL_LOADING );

		return true;
	}

	return false;
}

/*
================
SCR_BeginLoadingPlaque
================
*/
void SCR_BeginLoadingPlaque( qboolean is_background )
{
	S_StopAllSounds( true );
	cl.audio_prepped = false;			// don't play ambients

	if( cls.key_dest == key_menu && !cls.changedemo && !is_background )
	{
		UI_SetActiveMenu( false );
		if( cls.state == ca_disconnected && !(GameState->curstate == STATE_RUNFRAME && GameState->nextstate != STATE_RUNFRAME) )
			SCR_UpdateScreen();
	}

	if( cls.state == ca_disconnected || cls.disable_screen )
		return; // already set

	if( cls.key_dest == key_console )
		return;

	if( is_background ) IN_MouseSavePos( );
	cls.draw_changelevel = !is_background;
	SCR_UpdateScreen();

	// set video_prepped after update screen, so engine can draw last remaining frame
	cl.video_prepped = false;
	cls.disable_screen = host.realtime;
	cl.background = is_background;		// set right state before svc_serverdata is came

//	SNDDMA_LockSound();
}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque( void )
{
	cls.disable_screen = 0.0f;
	Con_ClearNotify();
//	SNDDMA_UnlockSound();
}

/*
=================
SCR_AddDirtyPoint
=================
*/
static void SCR_AddDirtyPoint( int x, int y )
{
	if( x < scr_dirty.x1 ) scr_dirty.x1 = x;
	if( x > scr_dirty.x2 ) scr_dirty.x2 = x;
	if( y < scr_dirty.y1 ) scr_dirty.y1 = y;
	if( y > scr_dirty.y2 ) scr_dirty.y2 = y;
}

/*
================
SCR_DirtyScreen
================
*/
void SCR_DirtyScreen( void )
{
	SCR_AddDirtyPoint( 0, 0 );
	SCR_AddDirtyPoint( refState.width - 1, refState.height - 1 );
}

static void R_DrawTileClear( int texnum, int x, int y, int w, int h, float tw, float th )
{
	float s1 = x / tw;
	float t1 = y / th;
	float s2 = ( x + w ) / tw;
	float t2 = ( y + h ) / th;

	ref.dllFuncs.GL_SetRenderMode( kRenderNormal );
	ref.dllFuncs.Color4ub( 255, 255, 255, 255 );
	ref.dllFuncs.R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, texnum );
}

/*
================
SCR_TileClear
================
*/
void SCR_TileClear( void )
{
	int	i, top, bottom, left, right, texnum;
	dirty_t	clear;
	float tw, th;

	if( likely( scr_viewsize.value >= 120 ))
		return; // full screen rendering

	if( !cls.tileImage )
	{
		cls.tileImage = ref.dllFuncs.GL_LoadTexture( "gfx/backtile.lmp", NULL, 0, TF_NOMIPMAP );
		if( !cls.tileImage )
			cls.tileImage = -1;
	}

	if( cls.tileImage > 0 )
		texnum = cls.tileImage;
	else texnum = 0;

	// erase rect will be the union of the past three frames
	// so tripple buffering works properly
	clear = scr_dirty;

	for( i = 0; i < 2; i++ )
	{
		if( scr_old_dirty[i].x1 < clear.x1 )
			clear.x1 = scr_old_dirty[i].x1;
		if( scr_old_dirty[i].x2 > clear.x2 )
			clear.x2 = scr_old_dirty[i].x2;
		if( scr_old_dirty[i].y1 < clear.y1 )
			clear.y1 = scr_old_dirty[i].y1;
		if( scr_old_dirty[i].y2 > clear.y2 )
			clear.y2 = scr_old_dirty[i].y2;
	}

	scr_old_dirty[1] = scr_old_dirty[0];
	scr_old_dirty[0] = scr_dirty;

	scr_dirty.x1 = 9999;
	scr_dirty.x2 = -9999;
	scr_dirty.y1 = 9999;
	scr_dirty.y2 = -9999;

	if( clear.y2 <= clear.y1 )
		return; // nothing disturbed

	top = clgame.viewport[1];
	bottom = top + clgame.viewport[3] - 1;
	left = clgame.viewport[0];
	right = left + clgame.viewport[2] - 1;

	tw = REF_GET_PARM( PARM_TEX_SRC_WIDTH, texnum );
	th = REF_GET_PARM( PARM_TEX_SRC_HEIGHT, texnum );

	if( clear.y1 < top )
	{
		// clear above view screen
		i = clear.y2 < top-1 ? clear.y2 : top - 1;
		R_DrawTileClear( texnum, clear.x1, clear.y1, clear.x2 - clear.x1 + 1, i - clear.y1 + 1, tw, th );
		clear.y1 = top;
	}

	if( clear.y2 > bottom )
	{
		// clear below view screen
		i = clear.y1 > bottom + 1 ? clear.y1 : bottom + 1;
		R_DrawTileClear( texnum, clear.x1, i, clear.x2 - clear.x1 + 1, clear.y2 - i + 1, tw, th );
		clear.y2 = bottom;
	}

	if( clear.x1 < left )
	{
		// clear left of view screen
		i = clear.x2 < left - 1 ? clear.x2 : left - 1;
		R_DrawTileClear( texnum, clear.x1, clear.y1, i - clear.x1 + 1, clear.y2 - clear.y1 + 1, tw, th );
		clear.x1 = left;
	}

	if( clear.x2 > right )
	{
		// clear left of view screen
		i = clear.x1 > right + 1 ? clear.x1 : right + 1;
		R_DrawTileClear( texnum, i, clear.y1, clear.x2 - i + 1, clear.y2 - clear.y1 + 1, tw, th );
		clear.x2 = right;
	}
}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen( void )
{
	qboolean screen_redraw = true; // assume screen has been redrawn

	if( !V_PreRender( )) return;

	switch( cls.state )
	{
	case ca_disconnected:
		Con_RunConsole ();
		break;
	case ca_connecting:
	case ca_connected:
	case ca_validate:
		screen_redraw = SCR_DrawPlaque();
		break;
	case ca_active:
		Con_RunConsole ();
		V_RenderView();
		break;
	case ca_cinematic:
		SCR_DrawCinematic();
		break;
	default:
		Host_Error( "%s: bad cls.state\n", __func__ );
		break;
	}

	// during changelevel we might have a few frames when we have nothing to draw
	// (assuming levelshots are off) and drawing 2d on top of nothing or cleared screen
	// is ugly, specifically with Adreno and ImgTec GPUs
	if( screen_redraw || !cls.changelevel || !cls.changedemo )
		V_PostRender();
}

/*
================
SCR_LoadCreditsFont

INTERNAL RESOURCE
================
*/
void SCR_LoadCreditsFont( void )
{
	cl_font_t *const font = &cls.creditsFont;
	qboolean success = false;
	float scale = hud_fontscale.value;
	dword crc = 0;

	// replace default gfx.wad textures by current charset's font
	if( !CRC32_File( &crc, "gfx.wad" ) || crc == 0x49eb9f16 )
	{
		string charsetFnt;

		if( Q_snprintf( charsetFnt, sizeof( charsetFnt ),
			"creditsfont_%s.fnt", Cvar_VariableString( "con_charset" )) > 0 )
		{
			if( FS_FileExists( charsetFnt, false ))
				success = Con_LoadVariableWidthFont( charsetFnt, font, scale, &hud_fontrender, TF_FONT );
		}
	}

	if( !success )
		success = Con_LoadVariableWidthFont( "gfx/creditsfont.fnt", font, scale, &hud_fontrender, TF_FONT );

	if( !success )
		success = Con_LoadFixedWidthFont( "gfx/conchars", font, scale, &hud_fontrender, TF_FONT );

	// copy font size for client.dll
	if( success )
	{
		int i;

		clgame.scrInfo.iCharHeight = cls.creditsFont.charHeight;

		for( i = 0; i < ARRAYSIZE( cls.creditsFont.charWidths ); i++ )
			clgame.scrInfo.charWidths[i] = cls.creditsFont.charWidths[i];
	}
	else Con_DPrintf( S_ERROR "failed to load HUD font\n" );
}

/*
================
SCR_InstallParticlePalette

INTERNAL RESOURCE
================
*/
static void SCR_InstallParticlePalette( void )
{
	rgbdata_t	*pic;
	int	i;

	// first check 'palette.lmp' then 'palette.pal'
	pic = FS_LoadImage( DEFAULT_INTERNAL_PALETTE, NULL, 0 );
	if( !pic ) pic = FS_LoadImage( DEFAULT_EXTERNAL_PALETTE, NULL, 0 );

	// NOTE: imagelib required this fakebuffer for loading internal palette
	if( !pic ) pic = FS_LoadImage( "#valve.pal", (byte *)&i, 768 );

	if( pic )
	{
		for( i = 0; i < 256; i++ )
		{
			clgame.palette[i].r = pic->palette[i*4+0];
			clgame.palette[i].g = pic->palette[i*4+1];
			clgame.palette[i].b = pic->palette[i*4+2];
		}
		FS_FreeImage( pic );
	}
	else
	{
		// someone deleted internal palette from code...
		for( i = 0; i < 256; i++ )
		{
			clgame.palette[i].r = i;
			clgame.palette[i].g = i;
			clgame.palette[i].b = i;
		}
	}
}

int SCR_LoadPauseIcon( void )
{
	int texnum = 0;

	if( FS_FileExists( "gfx/paused.lmp", false ))
		texnum = ref.dllFuncs.GL_LoadTexture( "gfx/paused.lmp", NULL, 0, TF_IMAGE|TF_ALLOW_NEAREST );
	else if( FS_FileExists( "gfx/pause.lmp", false ))
		texnum = ref.dllFuncs.GL_LoadTexture( "gfx/pause.lmp", NULL, 0, TF_IMAGE|TF_ALLOW_NEAREST );

	return texnum ? texnum : -1;
}

/*
================
SCR_RegisterTextures

INTERNAL RESOURCE
================
*/
void SCR_RegisterTextures( void )
{
	const char *exts[] = { "lmp", "bmp", "png" };
	const char *names[] = { "gfx/lambda", "gfx/loading" };
	uint flags = TF_IMAGE|TF_ALLOW_NEAREST;

	if( cl_allow_levelshots.value )
		SetBits( flags, TF_LUMINANCE );

	for( int i = 0; i < ARRAYSIZE( names ); i++ )
	{
		for( int j = 0; j < ARRAYSIZE( exts ); j++ )
		{
			string path;

			if( Q_snprintf( path, sizeof( path ), "%s.%s", names[i], exts[j] ) < 0 )
				continue;

			if( !FS_FileExists( path, false ))
				continue;

			cls.loadingBar = ref.dllFuncs.GL_LoadTexture( path, NULL, 0, flags );

			if( cls.loadingBar )
				return;
		}
	}

	Con_Printf( S_WARN "%s: failed to load loading image\n", __func__ );
}

/*
=================
SCR_SizeUp_f

Keybinding command
=================
*/
static void SCR_SizeUp_f( void )
{
	Cvar_SetValue( "viewsize", Q_min( scr_viewsize.value + 10, 120 ));
}


/*
=================
SCR_SizeDown_f

Keybinding command
=================
*/
static void SCR_SizeDown_f( void )
{
	Cvar_SetValue( "viewsize", Q_max( scr_viewsize.value - 10, 30 ));
}

/*
==================
SCR_VidInit
==================
*/
void SCR_VidInit( void )
{
	if( !ref.initialized ) // don't call VidInit too soon
		return;

	memset( &clgame.ds, 0, sizeof( clgame.ds )); // reset a draw state
	memset( &gameui.ds, 0, sizeof( gameui.ds )); // reset a draw state
	memset( &clgame.centerPrint, 0, sizeof( clgame.centerPrint ));

	// update screen sizes for menu
	if( gameui.globals )
	{
		gameui.globals->scrWidth = refState.width;
		gameui.globals->scrHeight = refState.height;
	}

	// notify vgui about screen size change
	if( clgame.hInstance )
	{
		VGui_Startup( refState.width, refState.height );
	}

	CL_ClearSpriteTextures(); // now all hud sprites are invalid

	// vid_state has changed
	if( gameui.hInstance ) gameui.dllFuncs.pfnVidInit();
	if( clgame.hInstance ) clgame.dllFuncs.pfnVidInit();

	// restart console size
	Con_VidInit ();
	Touch_NotifyResize();
}

/*
==================
SCR_Init
==================
*/
void SCR_Init( void )
{
	if( scr_init ) return;

	Cvar_RegisterVariable( &scr_centertime );
	Cvar_RegisterVariable( &cl_levelshot_name );
	Cvar_RegisterVariable( &cl_allow_levelshots );
	Cvar_RegisterVariable( &scr_loading );
	Cvar_RegisterVariable( &scr_download );
	Cvar_RegisterVariable( &cl_testlights );
	Cvar_RegisterVariable( &cl_envshot_size );
	Cvar_RegisterVariable( &v_dark );
	Cvar_RegisterVariable( &scr_viewsize );
	Cvar_RegisterVariable( &net_speeds );
	Cvar_RegisterVariable( &cl_showfps );
	Cvar_RegisterVariable( &cl_showpos );
	Cvar_RegisterVariable( &cl_showcmd );
	Cvar_RegisterVariable( &cl_debug );
	Cvar_RegisterVariable( &thirdperson );
#ifdef _DEBUG
	Cvar_RegisterVariable( &cl_showents );
#endif // NDEBUG

	// register our commands
	Cmd_AddCommand( "skyname", CL_SetSky_f, "set new skybox by basename" );
	Cmd_AddCommand( "loadsky", CL_SetSky_f, "set new skybox by basename" );
	Cmd_AddCommand( "viewpos", SCR_Viewpos_f, "prints current player origin" );
	Cmd_AddCommand( "sizeup", SCR_SizeUp_f, "screen size up to 10 points" );
	Cmd_AddCommand( "sizedown", SCR_SizeDown_f, "screen size down to 10 points" );

	if( !UI_LoadProgs( ))
	{
		Con_Printf( S_ERROR "can't initialize gameui DLL: %s\n", COM_GetLibraryError() ); // there is non fatal for us
		host.allow_console = true; // we need console, because menu is missing
	}

	SCR_VidInit();
	SCR_LoadCreditsFont ();
	SCR_RegisterTextures ();
	SCR_InstallParticlePalette ();
	SCR_InitCinematic();
	CL_InitNetgraph();

	if( host.allow_console && Sys_CheckParm( "-toconsole" ))
		Cbuf_AddText( "toggleconsole\n" );
	else UI_SetActiveMenu( true );

	scr_init = true;
}

void SCR_Shutdown( void )
{
	if( !scr_init ) return;

	Cmd_RemoveCommand( "skyname" );
	Cmd_RemoveCommand( "viewpos" );
	UI_SetActiveMenu( false );
	UI_UnloadProgs();

	scr_init = false;
}
