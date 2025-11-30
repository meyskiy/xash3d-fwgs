/*
gl_rmain.c - renderer main loop
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "gl_local.h"
#include "xash3d_mathlib.h"
#include "library.h"
#include "beamdef.h"
#include "particledef.h"
#include "entity_types.h"
#include "pmtrace.h"
#include "pm_defs.h"


#define IsLiquidContents( cnt )	( cnt == CONTENTS_WATER || cnt == CONTENTS_SLIME || cnt == CONTENTS_LAVA )

float		gldepthmin, gldepthmax;
ref_instance_t	RI;

// External cvars from gl_opengl.c
extern cvar_t kek_esp;
extern cvar_t kek_esp_box;
extern cvar_t kek_esp_name;
extern cvar_t kek_esp_weapon;

// External function from gl_context.c
extern void CL_FillRGBA( int rendermode, float _x, float _y, float _w, float _h, byte r, byte g, byte b, byte a );

extern cvar_t kek_esp_r;
extern cvar_t kek_esp_g;
extern cvar_t kek_esp_b;
extern cvar_t kek_esp_visible_r;
extern cvar_t kek_esp_visible_g;
extern cvar_t kek_esp_visible_b;
extern cvar_t kek_esp_name_r;
extern cvar_t kek_esp_name_g;
extern cvar_t kek_esp_name_b;
extern cvar_t kek_esp_weapon_r;
extern cvar_t kek_esp_weapon_g;
extern cvar_t kek_esp_weapon_b;

extern cvar_t kek_aimbot;
extern cvar_t kek_aimbot_fov;
extern cvar_t kek_aimbot_smooth;
extern cvar_t kek_aimbot_visible_only;
extern cvar_t kek_aimbot_draw_fov;
extern cvar_t kek_aimbot_max_distance;
extern cvar_t kek_aimbot_psilent;
extern cvar_t kek_aimbot_dm;
extern cvar_t kek_aimbot_x;
extern cvar_t kek_aimbot_y;
extern cvar_t kek_aimbot_z;
extern cvar_t kek_aimbot_fov_r;
extern cvar_t kek_aimbot_fov_g;
extern cvar_t kek_aimbot_fov_b;

extern cvar_t kek_antiaim;
extern cvar_t kek_antiaim_mode;
extern cvar_t kek_antiaim_speed;
extern cvar_t kek_antiaim_jitter_range;
extern cvar_t kek_antiaim_fake_angle;

extern cvar_t kek_debug;

extern cvar_t kek_custom_fov;
extern cvar_t kek_custom_fov_value;

extern cvar_t kek_viewmodel_glow;
extern cvar_t kek_viewmodel_glow_mode;
extern cvar_t kek_viewmodel_glow_r;
extern cvar_t kek_viewmodel_glow_g;
extern cvar_t kek_viewmodel_glow_b;
extern cvar_t kek_viewmodel_glow_alpha;


// Function prototypes
void kek_RenderViewModelGlow( void );
static void kek_ApplyAntiAim( ref_viewpass_t *rvp );

static qboolean kek_UserinfoValueForKey( const char *userinfo, const char *key, char *out, size_t out_size )
{
	char current_key[64];
	size_t key_len;

	if( !out || !out_size )
		return false;

	out[0] = '\0';

	if( !userinfo || !userinfo[0] || !key || !key[0] )
		return false;

	while( *userinfo )
	{
		if( *userinfo == '\\' )
			userinfo++;

		key_len = 0;
		while( userinfo[key_len] && userinfo[key_len] != '\\' && key_len < sizeof( current_key ) - 1 )
		{
			current_key[key_len] = userinfo[key_len];
			key_len++;
		}
		current_key[key_len] = '\0';
		userinfo += key_len;

		if( *userinfo == '\\' )
			userinfo++;

		const char *value_start = userinfo;
		while( *userinfo && *userinfo != '\\' )
			userinfo++;

		if( !Q_stricmp( current_key, key ))
		{
			size_t value_len = Q_min( (size_t)(userinfo - value_start), out_size - 1 );
			Q_strncpy( out, value_start, value_len + 1 );
			out[value_len] = '\0';
			return (out[0] != '\0');
		}
	}

	return false;
}

static cvar_t *kek_cl_debug_cvar = NULL;

typedef enum
{
	KEK_MODEL_TEAM_UNKNOWN = 0,
	KEK_MODEL_TEAM_CT,
	KEK_MODEL_TEAM_T
} kek_model_team_t;

typedef struct
{
	qboolean has_numeric;
	int numeric;
	qboolean has_team_str;
	char team_str[32];
	qboolean has_model_team;
	kek_model_team_t model_team;
} kek_team_info_t;

static float kek_GetClDebugValue( void )
{
	if( !kek_cl_debug_cvar )
		kek_cl_debug_cvar = gEngfuncs.pfnGetCvarPointer( "cl_debug", 0 );

	if( kek_cl_debug_cvar )
		return kek_cl_debug_cvar->value;

	return gEngfuncs.pfnGetCvarFloat( "cl_debug" );
}

static void kek_NormalizeModelName( const char *model, char *out, size_t out_size )
{
	const char *name;
	char *dot;

	if( !out || !out_size )
		return;

	out[0] = '\0';

	if( !model || !model[0] )
		return;

	name = model;

	const char *sep = Q_strrchr( name, '/' );
	if( sep )
		name = sep + 1;

	sep = Q_strrchr( name, '\\' );
	if( sep )
		name = sep + 1;

	Q_strncpy( out, name, out_size );

	dot = Q_strrchr( out, '.' );
	if( dot )
		*dot = '\0';

	Q_strnlwr( out, out, out_size );
}

static qboolean kek_StringMatchesList( const char *value, const char *const *list, size_t count )
{
	size_t i;

	if( !value || !value[0] )
		return false;

	for( i = 0; i < count; ++i )
	{
		if( !Q_stricmp( value, list[i] ))
			return true;
	}

	return false;
}

static kek_model_team_t kek_DetectTeamFromModel( const char *model )
{
	static const char *ct_models[] = {
		"urban", "gign", "gsg9", "sas", "seal", "swat",
		"spetsnaz", "idf", "gendarmerie", "fbi", "diver",
		"ctm_fbi", "ctm_gign", "ctm_gsg9", "ctm_idf", "ctm_sas",
		"ctm_st6", "ctm_swat", "ctm_diver"
	};
	static const char *t_models[] = {
		"leet", "terror", "guerilla", "militia", "arctic", "anarchist",
		"pirate", "phoenix", "separatist", "professional", "tm_anarchist",
		"tm_leet", "tm_pirate", "tm_phoenix", "tm_professional", "tm_balkan",
		"tm_jungle"
	};
	char normalized[64];

	kek_NormalizeModelName( model, normalized, sizeof( normalized ));

	if( !normalized[0] )
		return KEK_MODEL_TEAM_UNKNOWN;

	if( !Q_strncmp( normalized, "ct_", 3 ) || !Q_strncmp( normalized, "ctm_", 4 ) || Q_strstr( normalized, "_ct" ))
		return KEK_MODEL_TEAM_CT;

	if( !Q_strncmp( normalized, "t_", 2 ) || !Q_strncmp( normalized, "tm_", 3 ) || Q_strstr( normalized, "_t" ))
		return KEK_MODEL_TEAM_T;

	if( kek_StringMatchesList( normalized, ct_models, ARRAYSIZE( ct_models ) ))
		return KEK_MODEL_TEAM_CT;

	if( kek_StringMatchesList( normalized, t_models, ARRAYSIZE( t_models ) ))
		return KEK_MODEL_TEAM_T;

	return KEK_MODEL_TEAM_UNKNOWN;
}

static void kek_FillTeamInfo( cl_entity_t *ent, kek_team_info_t *info )
{
	int player_index;
	player_info_t *pinfo;

	memset( info, 0, sizeof( *info ));

	if( !ent )
		return;

	if( ent->curstate.team > 0 )
	{
		info->has_numeric = true;
		info->numeric = ent->curstate.team;
	}

	player_index = ent->index - 1;
	if( player_index < 0 || player_index >= gp_cl->maxclients )
		return;

	pinfo = gEngfuncs.pfnPlayerInfo( player_index );
	if( !pinfo )
		return;

	if( pinfo->userinfo[0] )
	{
		char team_buf[sizeof( info->team_str )];
		if( kek_UserinfoValueForKey( pinfo->userinfo, "team", team_buf, sizeof( team_buf )) && team_buf[0] )
		{
			Q_strnlwr( team_buf, info->team_str, sizeof( info->team_str ));
			info->has_team_str = true;
		}
	}

	if( pinfo->model[0] )
	{
		kek_model_team_t model_team = kek_DetectTeamFromModel( pinfo->model );
		if( model_team != KEK_MODEL_TEAM_UNKNOWN )
		{
			info->model_team = model_team;
			info->has_model_team = true;
		}
	}
}

// KEK debug tracking variables
static cl_entity_t *kek_GetLocalPlayerEntity( void )
{
	if( !gp_cl )
		return NULL;

	int local_index = gp_cl->playernum + 1;
	if( local_index <= 0 || local_index >= tr.max_entities )
		return NULL;

	return CL_GetEntityByIndex( local_index );
}

static int kek_esp_total_count = 0;
static int kek_esp_visible_count = 0;
static double kek_esp_debug_last_print = 0.0;
static double kek_aimbot_debug_last_print = 0.0;
static int kek_aimbot_last_target = -1;
static float kek_aimbot_target_time = 0.0f;
static qboolean kek_aimbot_last_visible = false;
static float kek_aimbot_last_distance = 0.0f;
static float kek_aimbot_last_fov = 0.0f;
static vec3_t kek_aimbot_last_angles = { 0.0f, 0.0f, 0.0f };

static int R_RankForRenderMode( int rendermode )
{
	switch( rendermode )
	{
	case kRenderTransTexture:
		return 1;	// draw second
	case kRenderTransAdd:
		return 2;	// draw third
	case kRenderGlow:
		return 3;	// must be last!
	}
	return 0;
}

void R_AllowFog( qboolean allowed )
{
	if( allowed )
	{
		if( glState.isFogEnabled && gl_fog.value )
			pglEnable( GL_FOG );
	}
	else
	{
		if( glState.isFogEnabled )
			pglDisable( GL_FOG );
	}
}

/*
===============
R_OpaqueEntity

Opaque entity can be brush or studio model but sprite
===============
*/
qboolean R_OpaqueEntity( cl_entity_t *ent )
{
	if( R_GetEntityRenderMode( ent ) == kRenderNormal )
	{
		switch( ent->curstate.renderfx )
		{
		case kRenderFxNone:
		case kRenderFxDeadPlayer:
		case kRenderFxLightMultiplier:
		case kRenderFxExplode:
			return true;
		}
	}
	return false;
}

/*
===============
R_TransEntityCompare

Sorting translucent entities by rendermode then by distance
===============
*/
static int R_TransEntityCompare( const void *a, const void *b )
{
	cl_entity_t	*ent1, *ent2;
	vec3_t		vecLen, org;
	float		dist1, dist2;
	int		rendermode1;
	int		rendermode2;

	ent1 = *(cl_entity_t **)a;
	ent2 = *(cl_entity_t **)b;
	rendermode1 = R_GetEntityRenderMode( ent1 );
	rendermode2 = R_GetEntityRenderMode( ent2 );

	// sort by distance
	if( ent1->model->type != mod_brush || rendermode1 != kRenderTransAlpha )
	{
		VectorAverage( ent1->model->mins, ent1->model->maxs, org );
		VectorAdd( ent1->origin, org, org );
		VectorSubtract( RI.vieworg, org, vecLen );
		dist1 = DotProduct( vecLen, vecLen );
	}
	else dist1 = 1000000000;

	if( ent2->model->type != mod_brush || rendermode2 != kRenderTransAlpha )
	{
		VectorAverage( ent2->model->mins, ent2->model->maxs, org );
		VectorAdd( ent2->origin, org, org );
		VectorSubtract( RI.vieworg, org, vecLen );
		dist2 = DotProduct( vecLen, vecLen );
	}
	else dist2 = 1000000000;

	if( dist1 > dist2 )
		return -1;
	if( dist1 < dist2 )
		return 1;

	// then sort by rendermode
	if( R_RankForRenderMode( rendermode1 ) > R_RankForRenderMode( rendermode2 ))
		return 1;
	if( R_RankForRenderMode( rendermode1 ) < R_RankForRenderMode( rendermode2 ))
		return -1;

	return 0;
}

/*
===============
R_WorldToScreen

Convert a given point from world into screen space
Returns true if we behind to screen
===============
*/
int R_WorldToScreen( const vec3_t point, vec3_t screen )
{
	matrix4x4	worldToScreen;
	qboolean	behind;
	float	w;

	if( !point || !screen )
		return true;

	Matrix4x4_Copy( worldToScreen, RI.worldviewProjectionMatrix );
	screen[0] = worldToScreen[0][0] * point[0] + worldToScreen[0][1] * point[1] + worldToScreen[0][2] * point[2] + worldToScreen[0][3];
	screen[1] = worldToScreen[1][0] * point[0] + worldToScreen[1][1] * point[1] + worldToScreen[1][2] * point[2] + worldToScreen[1][3];
	w = worldToScreen[3][0] * point[0] + worldToScreen[3][1] * point[1] + worldToScreen[3][2] * point[2] + worldToScreen[3][3];
	screen[2] = 0.0f; // just so we have something valid here

	if( w < 0.001f )
	{
		behind = true;
	}
	else
	{
		float invw = 1.0f / w;
		screen[0] *= invw;
		screen[1] *= invw;
		behind = false;
	}

	return behind;
}

/*
===============
R_ScreenToWorld

Convert a given point from screen into world space
===============
*/
void R_ScreenToWorld( const vec3_t screen, vec3_t point )
{
	matrix4x4	screenToWorld;
	float	w;

	if( !point || !screen )
		return;

	Matrix4x4_Invert_Full( screenToWorld, RI.worldviewProjectionMatrix );

	point[0] = screen[0] * screenToWorld[0][0] + screen[1] * screenToWorld[0][1] + screen[2] * screenToWorld[0][2] + screenToWorld[0][3];
	point[1] = screen[0] * screenToWorld[1][0] + screen[1] * screenToWorld[1][1] + screen[2] * screenToWorld[1][2] + screenToWorld[1][3];
	point[2] = screen[0] * screenToWorld[2][0] + screen[1] * screenToWorld[2][1] + screen[2] * screenToWorld[2][2] + screenToWorld[2][3];
	w = screen[0] * screenToWorld[3][0] + screen[1] * screenToWorld[3][1] + screen[2] * screenToWorld[3][2] + screenToWorld[3][3];
	if( w != 0.0f ) VectorScale( point, ( 1.0f / w ), point );
}

/*
===============
R_PushScene
===============
*/
void R_PushScene( void )
{
	if( ++tr.draw_stack_pos >= MAX_DRAW_STACK )
		gEngfuncs.Host_Error( "draw stack overflow\n" );

	tr.draw_list = &tr.draw_stack[tr.draw_stack_pos];
}

/*
===============
R_PopScene
===============
*/
void R_PopScene( void )
{
	if( --tr.draw_stack_pos < 0 )
		gEngfuncs.Host_Error( "draw stack underflow\n" );
	tr.draw_list = &tr.draw_stack[tr.draw_stack_pos];
}

/*
===============
R_ClearScene
===============
*/
void R_ClearScene( void )
{
	tr.draw_list->num_solid_entities = 0;
	tr.draw_list->num_trans_entities = 0;
	tr.draw_list->num_beam_entities = 0;

	// clear the scene befor start new frame
	if( gEngfuncs.drawFuncs->R_ClearScene != NULL )
		gEngfuncs.drawFuncs->R_ClearScene();

}

/*
===============
R_AddEntity
===============
*/
qboolean R_AddEntity( struct cl_entity_s *clent, int type )
{
	if( !r_drawentities->value )
		return false; // not allow to drawing

	if( !clent || !clent->model )
		return false; // if set to invisible, skip

	if( FBitSet( clent->curstate.effects, EF_NODRAW ))
		return false; // done

	if( !R_ModelOpaque( clent->curstate.rendermode ) && CL_FxBlend( clent ) <= 0 )
		return true; // invisible

	switch( type )
	{
	case ET_FRAGMENTED:
		r_stats.c_client_ents++;
		break;
	case ET_TEMPENTITY:
		r_stats.c_active_tents_count++;
		break;
	default: break;
	}

	if( R_OpaqueEntity( clent ))
	{
		// opaque
		if( tr.draw_list->num_solid_entities >= MAX_VISIBLE_PACKET )
			return false;

		tr.draw_list->solid_entities[tr.draw_list->num_solid_entities] = clent;
		tr.draw_list->num_solid_entities++;
	}
	else
	{
		// translucent
		if( tr.draw_list->num_trans_entities >= MAX_VISIBLE_PACKET )
			return false;

		tr.draw_list->trans_entities[tr.draw_list->num_trans_entities] = clent;
		tr.draw_list->num_trans_entities++;
	}

	return true;
}

/*
=============
R_Clear
=============
*/
static void R_Clear( int bitMask )
{
	int	bits;

	if( ENGINE_GET_PARM( PARM_DEV_OVERVIEW ))
		pglClearColor( 0.0f, 1.0f, 0.0f, 1.0f ); // green background (Valve rules)
	else pglClearColor( 0.5f, 0.5f, 0.5f, 1.0f );

	bits = GL_DEPTH_BUFFER_BIT;

	if( glState.stencilEnabled )
		bits |= GL_STENCIL_BUFFER_BIT;

	bits &= bitMask;

	pglClear( bits );

	// change ordering for overview
	if( RI.drawOrtho )
	{
		gldepthmin = 1.0f;
		gldepthmax = 0.0f;
	}
	else
	{
		gldepthmin = 0.0f;
		gldepthmax = 1.0f;
	}

	pglDepthFunc( GL_LEQUAL );
	pglDepthRange( gldepthmin, gldepthmax );
}

//=============================================================================
/*
===============
R_GetFarClip
===============
*/
static float R_GetFarClip( void )
{
	if( WORLDMODEL && RI.drawWorld )
		return tr.movevars->zmax * 1.73f;
	return 2048.0f;
}

/*
===============
R_SetupFrustum
===============
*/
void R_SetupFrustum( void )
{
	const ref_overview_t	*ov = gEngfuncs.GetOverviewParms();

	if( RP_NORMALPASS() && ( ENGINE_GET_PARM( PARM_WATER_LEVEL ) >= 3 ) && ENGINE_GET_PARM( PARM_QUAKE_COMPATIBLE ))
	{
		RI.fov_x = atan( tan( DEG2RAD( RI.fov_x ) / 2 ) * ( 0.97f + sin( gp_cl->time * 1.5f ) * 0.03f )) * 2 / (M_PI_F / 180.0f);
		RI.fov_y = atan( tan( DEG2RAD( RI.fov_y ) / 2 ) * ( 1.03f - sin( gp_cl->time * 1.5f ) * 0.03f )) * 2 / (M_PI_F / 180.0f);
	}

	// build the transformation matrix for the given view angles
	AngleVectors( RI.viewangles, RI.vforward, RI.vright, RI.vup );

	if( !r_lockfrustum.value )
	{
		VectorCopy( RI.vieworg, RI.cullorigin );
		VectorCopy( RI.vforward, RI.cull_vforward );
		VectorCopy( RI.vright, RI.cull_vright );
		VectorCopy( RI.vup, RI.cull_vup );
	}

	if( RI.drawOrtho )
		GL_FrustumInitOrtho( &RI.frustum, ov->xLeft, ov->xRight, ov->yTop, ov->yBottom, ov->zNear, ov->zFar );
	else GL_FrustumInitProj( &RI.frustum, 0.0f, R_GetFarClip(), RI.fov_x, RI.fov_y ); // NOTE: we ignore nearplane here (mirrors only)
}

/*
=============
R_SetupProjectionMatrix
=============
*/
static void R_SetupProjectionMatrix( matrix4x4 m )
{
	GLfloat	xMin, xMax, yMin, yMax, zNear, zFar;

	if( RI.drawOrtho )
	{
		const ref_overview_t *ov = gEngfuncs.GetOverviewParms();
		Matrix4x4_CreateOrtho( m, ov->xLeft, ov->xRight, ov->yTop, ov->yBottom, ov->zNear, ov->zFar );
		return;
	}

	RI.farClip = R_GetFarClip();

	zNear = 4.0f;
	zFar = Q_max( 256.0f, RI.farClip );

	yMax = zNear * tan( RI.fov_y * M_PI_F / 360.0f );
	yMin = -yMax;

	xMax = zNear * tan( RI.fov_x * M_PI_F / 360.0f );
	xMin = -xMax;

	if( tr.rotation & 1 )
	{
		Matrix4x4_CreateProjection( m, yMax, yMin, xMax, xMin, zNear, zFar );
	}
	else
	{
		Matrix4x4_CreateProjection( m, xMax, xMin, yMax, yMin, zNear, zFar );
	}
}

/*
=============
R_SetupModelviewMatrix
=============
*/
static void R_SetupModelviewMatrix( matrix4x4 m )
{
	Matrix4x4_CreateModelview( m );
	if( tr.rotation & 1 )
	{
		Matrix4x4_ConcatRotate( m, anglemod( -RI.viewangles[2] + 90 ), 1, 0, 0 );
		Matrix4x4_ConcatRotate( m, -RI.viewangles[0], 0, 1, 0 );
		Matrix4x4_ConcatRotate( m, -RI.viewangles[1], 0, 0, 1 );
	}
	else
	{
		Matrix4x4_ConcatRotate( m, -RI.viewangles[2], 1, 0, 0 );
		Matrix4x4_ConcatRotate( m, -RI.viewangles[0], 0, 1, 0 );
		Matrix4x4_ConcatRotate( m, -RI.viewangles[1], 0, 0, 1 );
	}
	Matrix4x4_ConcatTranslate( m, -RI.vieworg[0], -RI.vieworg[1], -RI.vieworg[2] );
}

/*
=============
R_LoadIdentity
=============
*/
void R_LoadIdentity( void )
{
	if( tr.modelviewIdentity ) return;

	Matrix4x4_LoadIdentity( RI.objectMatrix );
	Matrix4x4_Copy( RI.modelviewMatrix, RI.worldviewMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.modelviewMatrix );
	tr.modelviewIdentity = true;
}

/*
=============
R_RotateForEntity
=============
*/
void R_RotateForEntity( cl_entity_t *e )
{
	float	scale = 1.0f;

	if( e == CL_GetEntityByIndex( 0 ))
	{
		R_LoadIdentity();
		return;
	}

	if( e->model->type != mod_brush && e->curstate.scale > 0.0f )
		scale = e->curstate.scale;

	Matrix4x4_CreateFromEntity( RI.objectMatrix, e->angles, e->origin, scale );
	Matrix4x4_ConcatTransforms( RI.modelviewMatrix, RI.worldviewMatrix, RI.objectMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.modelviewMatrix );
	tr.modelviewIdentity = false;
}

/*
=============
R_TranslateForEntity
=============
*/
void R_TranslateForEntity( cl_entity_t *e )
{
	float	scale = 1.0f;

	if( e == CL_GetEntityByIndex( 0 ))
	{
		R_LoadIdentity();
		return;
	}

	if( e->model->type != mod_brush && e->curstate.scale > 0.0f )
		scale = e->curstate.scale;

	Matrix4x4_CreateFromEntity( RI.objectMatrix, vec3_origin, e->origin, scale );
	Matrix4x4_ConcatTransforms( RI.modelviewMatrix, RI.worldviewMatrix, RI.objectMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.modelviewMatrix );
	tr.modelviewIdentity = false;
}

/*
===============
R_FindViewLeaf
===============
*/
void R_FindViewLeaf( void )
{
	RI.oldviewleaf = RI.viewleaf;
	RI.viewleaf = gEngfuncs.Mod_PointInLeaf( RI.pvsorigin, WORLDMODEL->nodes );
}

/*
===============
R_SetupFrame
===============
*/
static void R_SetupFrame( void )
{
	// setup viewplane dist
	RI.viewplanedist = DotProduct( RI.vieworg, RI.vforward );

	// NOTE: this request is the fps-killer on some NVidia drivers
	glState.isFogEnabled = pglIsEnabled( GL_FOG );

	if( !gl_nosort.value )
	{
		// sort translucents entities by rendermode and distance
		qsort( tr.draw_list->trans_entities, tr.draw_list->num_trans_entities, sizeof( cl_entity_t* ), R_TransEntityCompare );
	}

	// current viewleaf
	if( RI.drawWorld )
	{
		RI.isSkyVisible = false; // unknown at this moment
		R_FindViewLeaf();
	}
}

/*
=============
R_SetupGL
=============
*/
void R_SetupGL( qboolean set_gl_state )
{
	R_SetupModelviewMatrix( RI.worldviewMatrix );
	R_SetupProjectionMatrix( RI.projectionMatrix );

	Matrix4x4_Concat( RI.worldviewProjectionMatrix, RI.projectionMatrix, RI.worldviewMatrix );

	if( !set_gl_state ) return;

	if( RP_NORMALPASS( ))
	{
		int x, x2, y, y2;

		// set up viewport (main, playersetup)
		x = floor( RI.viewport[0] * gpGlobals->width / gpGlobals->width );
		x2 = ceil(( RI.viewport[0] + RI.viewport[2] ) * gpGlobals->width / gpGlobals->width );
		y = floor( gpGlobals->height - RI.viewport[1] * gpGlobals->height / gpGlobals->height );
		y2 = ceil( gpGlobals->height - ( RI.viewport[1] + RI.viewport[3] ) * gpGlobals->height / gpGlobals->height );

		if( tr.rotation & 1 )
			pglViewport( y2, x, y - y2, x2 - x );
		else pglViewport( x, y2, x2 - x, y - y2 );
	}
	else
	{
		// envpass, mirrorpass
		pglViewport( RI.viewport[0], RI.viewport[1], RI.viewport[2], RI.viewport[3] );
	}

	pglMatrixMode( GL_PROJECTION );
	GL_LoadMatrix( RI.projectionMatrix );

	pglMatrixMode( GL_MODELVIEW );
	GL_LoadMatrix( RI.worldviewMatrix );

	if( FBitSet( RI.params, RP_CLIPPLANE ))
	{
		GLdouble	clip[4];
		mplane_t	*p = &RI.clipPlane;

		clip[0] = p->normal[0];
		clip[1] = p->normal[1];
		clip[2] = p->normal[2];
		clip[3] = -p->dist;

		pglClipPlane( GL_CLIP_PLANE0, clip );
		pglEnable( GL_CLIP_PLANE0 );
	}

	GL_Cull( GL_FRONT );

	pglDisable( GL_BLEND );
	pglDisable( GL_ALPHA_TEST );
	pglColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
}

/*
=============
R_EndGL
=============
*/
static void R_EndGL( void )
{
	if( RI.params & RP_CLIPPLANE )
		pglDisable( GL_CLIP_PLANE0 );
}

/*
=============
R_RecursiveFindWaterTexture

using to find source waterleaf with
watertexture to grab fog values from it
=============
*/
static gl_texture_t *R_RecursiveFindWaterTexture( const mnode_t *node, const mnode_t *ignore, qboolean down )
{
	gl_texture_t *tex = NULL;
	mnode_t *children[2];

	// assure the initial node is not null
	// we could check it here, but we would rather check it
	// outside the call to get rid of one additional recursion level
	Assert( node != NULL );

	// ignore solid nodes
	if( node->contents == CONTENTS_SOLID )
		return NULL;

	if( node->contents < 0 )
	{
		mleaf_t		*pleaf;
		msurface_t	**mark;
		int		i, c;

		// ignore non-liquid leaves
		if( node->contents != CONTENTS_WATER && node->contents != CONTENTS_LAVA && node->contents != CONTENTS_SLIME )
			 return NULL;

		// find texture
		pleaf = (mleaf_t *)node;
		mark = pleaf->firstmarksurface;
		c = pleaf->nummarksurfaces;

		for( i = 0; i < c; i++, mark++ )
		{
			if( (*mark)->flags & SURF_DRAWTURB && (*mark)->texinfo && (*mark)->texinfo->texture )
				return R_GetTexture( (*mark)->texinfo->texture->gl_texturenum );
		}

		// texture not found
		return NULL;
	}

	// this is a regular node
	// traverse children
	node_children( children, node, WORLDMODEL );

	if( children[0] && ( children[0] != ignore ))
	{
		tex = R_RecursiveFindWaterTexture( children[0], node, true );
		if( tex ) return tex;
	}

	if( children[1] && ( children[1] != ignore ))
	{
		tex = R_RecursiveFindWaterTexture( children[1], node, true );
		if( tex )	return tex;
	}

	// for down recursion, return immediately
	if( down ) return NULL;

	// texture not found, step up if any
	if( node->parent )
		return R_RecursiveFindWaterTexture( node->parent, node, false );

	// top-level node, bail out
	return NULL;
}

/*
=============
R_CheckFog

check for underwater fog
Using backward recursion to find waterline leaf
from underwater leaf (idea: XaeroX)
=============
*/
static void R_CheckFog( void )
{
	cl_entity_t	*ent;
	gl_texture_t	*tex;
	int		i, cnt, count;

	// quake global fog
	if( ENGINE_GET_PARM( PARM_QUAKE_COMPATIBLE ))
	{
		if( !tr.movevars->fog_settings )
		{
			if( pglIsEnabled( GL_FOG ))
				pglDisable( GL_FOG );
			RI.fogEnabled = false;
			return;
		}

		// quake-style global fog
		RI.fogColor[0] = ((tr.movevars->fog_settings & 0xFF000000) >> 24) / 255.0f;
		RI.fogColor[1] = ((tr.movevars->fog_settings & 0xFF0000) >> 16) / 255.0f;
		RI.fogColor[2] = ((tr.movevars->fog_settings & 0xFF00) >> 8) / 255.0f;
		RI.fogDensity = ((tr.movevars->fog_settings & 0xFF) / 255.0f) * 0.01f;
		RI.fogStart = RI.fogEnd = 0.0f;
		RI.fogColor[3] = 1.0f;
		RI.fogCustom = false;
		RI.fogEnabled = true;
		RI.fogSkybox = true;
		return;
	}

	RI.fogEnabled = false;

	if( RI.onlyClientDraw || ENGINE_GET_PARM( PARM_WATER_LEVEL ) < 3 || !RI.drawWorld || !RI.viewleaf )
	{
		if( RI.cached_waterlevel == 3 )
		{
			// in some cases waterlevel jumps from 3 to 1. Catch it
			RI.cached_waterlevel = ENGINE_GET_PARM( PARM_WATER_LEVEL );
			RI.cached_contents = CONTENTS_EMPTY;
			if( !RI.fogCustom )
			{
				glState.isFogEnabled = false;
				pglDisable( GL_FOG );
			}
		}
		return;
	}

	ent = gEngfuncs.CL_GetWaterEntity( RI.vieworg );
	if( ent && ent->model && ent->model->type == mod_brush && ent->curstate.skin < 0 )
		cnt = ent->curstate.skin;
	else cnt = RI.viewleaf->contents;

	RI.cached_waterlevel = ENGINE_GET_PARM( PARM_WATER_LEVEL );

	if( !IsLiquidContents( RI.cached_contents ) && IsLiquidContents( cnt ))
	{
		tex = NULL;

		// check for water texture
		if( ent && ent->model && ent->model->type == mod_brush )
		{
			msurface_t	*surf;

			count = ent->model->nummodelsurfaces;

			for( i = 0, surf = &ent->model->surfaces[ent->model->firstmodelsurface]; i < count; i++, surf++ )
			{
				if( surf->flags & SURF_DRAWTURB && surf->texinfo && surf->texinfo->texture )
				{
					tex = R_GetTexture( surf->texinfo->texture->gl_texturenum );
					RI.cached_contents = ent->curstate.skin;
					break;
				}
			}
		}
		else
		{
			tex = R_RecursiveFindWaterTexture( RI.viewleaf->parent, NULL, false );
			if( tex ) RI.cached_contents = RI.viewleaf->contents;
		}

		if( !tex ) return;	// no valid fogs

		// copy fog params
		RI.fogColor[0] = tex->fogParams[0] / 255.0f;
		RI.fogColor[1] = tex->fogParams[1] / 255.0f;
		RI.fogColor[2] = tex->fogParams[2] / 255.0f;
		RI.fogDensity = tex->fogParams[3] * 0.000025f;
		RI.fogStart = RI.fogEnd = 0.0f;
		RI.fogColor[3] = 1.0f;
		RI.fogCustom = false;
		RI.fogEnabled = true;
		RI.fogSkybox = true;
	}
	else
	{
		RI.fogCustom = false;
		RI.fogEnabled = true;
		RI.fogSkybox = true;
	}
}

/*
=============
R_CheckGLFog

special condition for Spirit 1.9
that used direct calls of glFog-functions
=============
*/
static void R_CheckGLFog( void )
{
#ifdef HACKS_RELATED_HLMODS
	if(( !RI.fogEnabled && !RI.fogCustom ) && pglIsEnabled( GL_FOG ) && VectorIsNull( RI.fogColor ))
	{
		// fill the fog color from GL-state machine
		pglGetFloatv( GL_FOG_COLOR, RI.fogColor );
		RI.fogSkybox = true;
	}
#endif
}

/*
=============
R_DrawFog

=============
*/
void R_DrawFog( void )
{
	if( !RI.fogEnabled || !gl_fog.value )
		return;

	pglEnable( GL_FOG );
	if( ENGINE_GET_PARM( PARM_QUAKE_COMPATIBLE ))
		pglFogi( GL_FOG_MODE, GL_EXP2 );
	else pglFogi( GL_FOG_MODE, GL_EXP );
	pglFogf( GL_FOG_DENSITY, RI.fogDensity );
	pglFogfv( GL_FOG_COLOR, RI.fogColor );
	pglHint( GL_FOG_HINT, GL_NICEST );
}

/*
=============
R_DrawEntitiesOnList
=============
*/
static void R_DrawEntitiesOnList( void )
{
	int	i;

	tr.blend = 1.0f;
	GL_CheckForErrors();

	// first draw solid entities
	for( i = 0; i < tr.draw_list->num_solid_entities && !RI.onlyClientDraw; i++ )
	{
		RI.currententity = tr.draw_list->solid_entities[i];
		RI.currentmodel = RI.currententity->model;

		Assert( RI.currententity != NULL );
		Assert( RI.currentmodel != NULL );

		switch( RI.currentmodel->type )
		{
		case mod_brush:
			R_DrawBrushModel( RI.currententity );
			break;
		case mod_alias:
			R_DrawAliasModel( RI.currententity );
			break;
		case mod_studio:
			R_DrawStudioModel( RI.currententity );
			break;
		default:
			break;
		}
	}

	GL_CheckForErrors();

	// quake-specific feature
	R_DrawAlphaTextureChains();

	GL_CheckForErrors();

	// draw sprites seperately, because of alpha blending
	for( i = 0; i < tr.draw_list->num_solid_entities && !RI.onlyClientDraw; i++ )
	{
		RI.currententity = tr.draw_list->solid_entities[i];
		RI.currentmodel = RI.currententity->model;

		Assert( RI.currententity != NULL );
		Assert( RI.currentmodel != NULL );

		switch( RI.currentmodel->type )
		{
		case mod_sprite:
			R_DrawSpriteModel( RI.currententity );
			break;
		}
	}

	GL_CheckForErrors();

	if( !RI.onlyClientDraw )
	{
		gEngfuncs.CL_DrawEFX( tr.frametime, false );
	}

	GL_CheckForErrors();

	if( RI.drawWorld )
		gEngfuncs.pfnDrawNormalTriangles();

	GL_CheckForErrors();

	// then draw translucent entities
	for( i = 0; i < tr.draw_list->num_trans_entities && !RI.onlyClientDraw; i++ )
	{
		RI.currententity = tr.draw_list->trans_entities[i];
		RI.currentmodel = RI.currententity->model;

		// handle studiomodels with custom rendermodes on texture
		if( RI.currententity->curstate.rendermode != kRenderNormal )
			tr.blend = CL_FxBlend( RI.currententity ) / 255.0f;
		else tr.blend = 1.0f; // draw as solid but sorted by distance

		if( tr.blend <= 0.0f ) continue;

		Assert( RI.currententity != NULL );
		Assert( RI.currentmodel != NULL );

		switch( RI.currentmodel->type )
		{
		case mod_brush:
			R_DrawBrushModel( RI.currententity );
			break;
		case mod_alias:
			R_DrawAliasModel( RI.currententity );
			break;
		case mod_studio:
			R_DrawStudioModel( RI.currententity );
			break;
		case mod_sprite:
			R_DrawSpriteModel( RI.currententity );
			break;
		default:
			break;
		}
	}

	GL_CheckForErrors();

	if( RI.drawWorld )
	{
		pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
		gEngfuncs.pfnDrawTransparentTriangles ();
	}

	GL_CheckForErrors();

	if( !RI.onlyClientDraw )
	{
		R_AllowFog( false );
		gEngfuncs.CL_DrawEFX( tr.frametime, true );
		R_AllowFog( true );
	}

	GL_CheckForErrors();

	pglDisable( GL_BLEND );	// Trinity Render issues

	if( !RI.onlyClientDraw )
	{
		// Kek custom FOV for viewmodel (arms distancing)
		float original_projection[16];
		float original_fov_x = RI.fov_x;
		float original_fov_y = RI.fov_y;

		// Save current projection matrix
		pglGetFloatv( GL_PROJECTION_MATRIX, original_projection );

		if( kek_custom_fov.value > 0.0f )
		{
			float custom_fov_value = kek_custom_fov_value.value;
			if( custom_fov_value > 0.0f )
			{
				// Increase FOV for viewmodel
				RI.fov_x = Q_min( RI.fov_x + custom_fov_value, 170.0f );
				RI.fov_y = Q_min( RI.fov_y + custom_fov_value, 170.0f );

				// Re-setup projection matrix with new FOV
				R_SetupProjectionMatrix( RI.projectionMatrix );
				pglMatrixMode( GL_PROJECTION );
				GL_LoadMatrix( RI.projectionMatrix );
				pglMatrixMode( GL_MODELVIEW );
			}
		}

		R_DrawViewModel();

		// Kek viewmodel glow effect
		if( kek_viewmodel_glow.value > 0.0f && tr.viewent && tr.viewent->model )
		{
			kek_RenderViewModelGlow();
		}

		// Restore original projection matrix
		pglMatrixMode( GL_PROJECTION );
		pglLoadMatrixf( original_projection );
		pglMatrixMode( GL_MODELVIEW );

		// Restore original FOV values
		RI.fov_x = original_fov_x;
		RI.fov_y = original_fov_y;
	}
	gEngfuncs.CL_ExtraUpdate();

	GL_CheckForErrors();
}


/*
================
kek_GetPlayerPosition

Get current player position with extrapolation if update is stale.
Returns true if position is valid and recent enough to display.
================
*/
static qboolean kek_GetPlayerPosition( cl_entity_t *ent, vec3_t out_origin )
{
	float current_time = gp_cl->time;
	float last_update_time;
	float time_since_update;
	vec3_t extrapolated_origin;
	
	// Get base position first
	if( !VectorIsNull( ent->origin ))
		VectorCopy( ent->origin, out_origin );
	else if( !VectorIsNull( ent->curstate.origin ))
		VectorCopy( ent->curstate.origin, out_origin );
	else
		return false; // No valid position
	
	// Get last update time (use msg_time if available, otherwise animtime)
	if( ent->curstate.msg_time > 0.0f )
		last_update_time = ent->curstate.msg_time;
	else if( ent->curstate.animtime > 0.0f )
		last_update_time = ent->curstate.animtime;
	else
	{
		// No timestamp available - assume position is recent if we have valid origin
		// This handles cases where timestamp might not be set
		return true;
	}
	
	// Calculate time since last update
	time_since_update = current_time - last_update_time;
	
	// Handle negative time (shouldn't happen, but be safe)
	if( time_since_update < 0.0f )
		time_since_update = 0.0f;
	
	// If update is too old (more than 5 seconds), don't show ESP
	// This prevents showing stale positions when player is out of PVS
	if( time_since_update > 5.0f )
		return false;
	
	// If update is recent (less than 0.5 seconds), use position as-is
	if( time_since_update <= 0.5f )
		return true;
	
	// If update is stale but not too old, extrapolate based on velocity
	// Only extrapolate if we have valid velocity and update is not too old
	if( !VectorIsNull( ent->curstate.velocity ) && time_since_update <= 3.0f )
	{
		VectorCopy( out_origin, extrapolated_origin );
		VectorMA( extrapolated_origin, time_since_update, ent->curstate.velocity, out_origin );
		return true;
	}
	
	// For moderately stale updates (0.5-5 seconds) without velocity, use last known position
	// This is better than showing nothing, but position may be inaccurate
	return true;
}

/*
================
kek_IsPlayerVisible

Check if player is visible (not behind wall)
================
*/
static qboolean kek_IsPlayerVisible( cl_entity_t *ent )
{
	vec3_t start, end, player_origin;
	pmtrace_t trace;

	// Get view origin
	VectorCopy( RI.vieworg, start );
	
	// Get current player position with extrapolation if needed
		if( !kek_GetPlayerPosition( ent, player_origin ))
		return false; // Position is too stale or invalid
	
	// Get player origin (center of bounding box)
	VectorCopy( player_origin, end );
	
	// Adjust end to center of player
	vec3_t mins, maxs;
	VectorCopy( ent->curstate.mins, mins );
	VectorCopy( ent->curstate.maxs, maxs );
	if( VectorIsNull( mins ) && VectorIsNull( maxs ))
	{
		mins[0] = -16; mins[1] = -16; mins[2] = -36;
		maxs[0] = 16; maxs[1] = 16; maxs[2] = 36;
	}
	end[2] += (mins[2] + maxs[2]) * 0.5f;

	// Trace line from view to player
	trace = gEngfuncs.CL_TraceLine( start, end, PM_STUDIO_BOX );
	
	// If trace hit something, player is behind wall
	return (trace.fraction >= 1.0f);
}

/*
================
kek_IsPlayerAlive

Check if player is alive (not dead)
================
*/
static qboolean kek_IsPlayerAlive( cl_entity_t *ent )
{
	// БАЗОВАЯ ПРОВЕРКА #1: Проверяем, что это действительно игрок
	if( !ent || !ent->player )
		return false;

	// БАЗОВАЯ ПРОВЕРКА #2: Проверяем индекс
	if( ent->index < 1 || (gp_cl && ent->index > gp_cl->maxclients) )
		return false;

	// КРИТИЧЕСКАЯ ПРОВЕРКА #1: Проверяем spectator флаг
	// Спектаторы не являются живыми игроками
	if( ent->curstate.spectator == 1 )
		return false;

	// КРИТИЧЕСКАЯ ПРОВЕРКА #2: Проверяем, что игрок не помечен как невидимый
	if( FBitSet( ent->curstate.effects, EF_NODRAW ))
		return false;
	
	// КРИТИЧЕСКАЯ ПРОВЕРКА #3: Проверяем solid - мертвые игроки часто имеют SOLID_NOT
	// Но не блокируем только по этому признаку, так как плагины могут устанавливать SOLID_NOT
	// Проверяем в комбинации с другими признаками
	if( ent->curstate.solid == SOLID_NOT )
	{
		// Если solid == NOT И нет валидной позиции - вероятно мертв
	if( VectorIsNull( ent->origin ) && VectorIsNull( ent->curstate.origin ))
		return false;
	}

	// КРИТИЧЕСКАЯ ПРОВЕРКА #4: Проверяем валидность позиции
	// Мертвые игроки часто имеют нулевую позицию
	if( VectorIsNull( ent->origin ) && VectorIsNull( ent->curstate.origin ))
		return false;

	// КРИТИЧЕСКАЯ ПРОВЕРКА #5: Проверяем модель - призраки могут иметь другую модель
	if( !ent->model || ent->model->type == mod_bad )
		return false;

	// КРИТИЧЕСКАЯ ПРОВЕРКА #6: Проверяем комбинацию признаков мертвого игрока
	// Мертвые игроки часто имеют: нет оружия + тип движения NONE + solid NOT
	if( ent->curstate.weaponmodel == 0 && 
	    ent->curstate.movetype == MOVETYPE_NONE &&
	    ent->curstate.solid == SOLID_NOT )
		return false;

	// Если все проверки пройдены - игрок ЖИВ
	return true;
}

/*
================
kek_IsSameTeam

Check if entity is on the same team as local player
================
*/
/*
================
kek_IsSameTeam

Определяет, является ли сущность тимейтом локального игрока.
Использует несколько методов определения команды, так как в разных модах
информация о командах хранится по-разному или отсутствует вовсе.

Методы проверки (в порядке приоритета):
1. curstate.team - основной метод, но работает только если сервер передает эту информацию
2. Модель игрока - тимейты часто имеют одинаковые модели (работает в CS, HLDM и т.д.)
3. Цвета игрока - менее надежный метод, но может помочь в некоторых случаях

Возвращает true если игроки в одной команде, false в противном случае.
================
*/
static qboolean kek_IsSameTeam( cl_entity_t *ent )
{
	cl_entity_t *local = kek_GetLocalPlayerEntity();
	kek_team_info_t ent_info, local_info;

	if( !local || !ent )
		return false;

	if( ent->index == local->index )
		return true;

	kek_FillTeamInfo( ent, &ent_info );
	kek_FillTeamInfo( local, &local_info );

	if( ent_info.has_numeric && local_info.has_numeric && ent_info.numeric == local_info.numeric )
		return true;

	if( ent_info.has_team_str && local_info.has_team_str && !Q_stricmp( ent_info.team_str, local_info.team_str ))
		return true;

	if( ent_info.has_model_team && local_info.has_model_team && ent_info.model_team == local_info.model_team )
		return true;

	return false;
}

/*
================
kek_DrawESPName

Draw player name above ESP box
================
*/
static void kek_DrawESPName( cl_entity_t *ent, float screen_x, float screen_y, float color[3] )
{
	player_info_t *info;
	vec3_t origin, screen_pos;
	const char *name;
	
	// Get player info - index is 1-based for entities, but player info is 0-based
	int player_index = ent->index - 1;
	if( player_index < 0 || player_index >= gp_cl->maxclients )
		return;
		
	info = gEngfuncs.pfnPlayerInfo( player_index );
	if( !info )
		return;
	
	// Get name - check different possible fields
	if( info->name && info->name[0] != '\0' )
		name = info->name;
	else
		return; // No name available
	
	// Get current position with extrapolation if needed
	if( !kek_GetPlayerPosition( ent, origin ))
		return; // Position is too stale or invalid
	
	vec3_t mins, maxs;
	VectorCopy( ent->curstate.mins, mins );
	VectorCopy( ent->curstate.maxs, maxs );
	if( VectorIsNull( mins ) && VectorIsNull( maxs ))
	{
		mins[2] = -36;
		maxs[2] = 36;
	}
	origin[2] += maxs[2] + 15.0f; // Above head (increased offset)
	
	// Convert to screen
	if( TriWorldToScreen( origin, screen_pos ))
		return; // Behind camera
	
	float vp_width = (float)RI.viewport[2];
	float vp_height = (float)RI.viewport[3];
	
	if( screen_pos[0] < 0 || screen_pos[0] > vp_width ||
	    screen_pos[1] < 0 || screen_pos[1] > vp_height )
		return;
	
	// Save matrices for 2D drawing
	pglMatrixMode( GL_PROJECTION );
	pglPushMatrix();
	pglLoadIdentity();
	pglOrtho( 0, vp_width, vp_height, 0, -1, 1 );
	
	pglMatrixMode( GL_MODELVIEW );
	pglPushMatrix();
	pglLoadIdentity();
	
	// Draw name using engine function
	rgba_t name_color;
	name_color[0] = (byte)(color[0] * 255.0f);
	name_color[1] = (byte)(color[1] * 255.0f);
	name_color[2] = (byte)(color[2] * 255.0f);
	name_color[3] = 255;
	
	// Center text
	int text_width, text_height;
	gEngfuncs.Con_DrawStringLen( name, &text_width, &text_height );
	gEngfuncs.Con_DrawString( (int)(screen_pos[0] - text_width * 0.5f), (int)screen_pos[1], name, name_color );
	
	// Restore matrices
	pglPopMatrix();
	pglMatrixMode( GL_PROJECTION );
	pglPopMatrix();
	pglMatrixMode( GL_MODELVIEW );
}

/*
================
kek_DrawESPWeapon

Draw weapon name below ESP box as text
================
*/
static void kek_DrawESPWeapon( cl_entity_t *ent, float screen_x, float screen_y, float color[3] )
{
	vec3_t origin, screen_pos;
	model_t *weapon_model;
	int weapon_index;
	const char *weapon_name = "Unknown";
	
	// Get weapon model index
	weapon_index = ent->curstate.weaponmodel;
	if( weapon_index <= 0 )
		return;
	
	// Check if weapon model index is valid
	if( weapon_index >= gp_cl->nummodels )
		return;
	
	weapon_model = CL_ModelHandle( weapon_index );
	if( !weapon_model )
		return;
	
	// Get weapon name from model path
	if( weapon_model->name && weapon_model->name[0] != '\0' )
	{
		// Extract weapon name from path (e.g., "models/w_ak47.mdl" -> "ak47")
		const char *name = weapon_model->name;
		const char *slash = Q_strrchr( name, '/' );
		const char *backslash = Q_strrchr( name, '\\' );
		const char *last_sep = (slash > backslash) ? slash : backslash;
		
		if( last_sep )
			name = last_sep + 1;
		
		// Remove extension and extract name
		static char weapon_buf[64];
		int name_len = 0;
		const char *p = name;
		while( *p != '\0' && *p != '.' && name_len < sizeof( weapon_buf ) - 1 )
		{
			weapon_buf[name_len++] = *p++;
		}
		weapon_buf[name_len] = '\0';
		
		// Remove "w_" prefix if present
		if( weapon_buf[0] == 'w' && weapon_buf[1] == '_' )
			weapon_name = weapon_buf + 2;
		else
			weapon_name = weapon_buf;
	}
	
	// Get current position with extrapolation if needed
	if( !kek_GetPlayerPosition( ent, origin ))
		return; // Position is too stale or invalid
	
	vec3_t mins, maxs;
	VectorCopy( ent->curstate.mins, mins );
	VectorCopy( ent->curstate.maxs, maxs );
	if( VectorIsNull( mins ) && VectorIsNull( maxs ))
	{
		mins[2] = -36;
		maxs[2] = 36;
	}
	origin[2] += mins[2] - 10.0f; // Below feet
	
	// Convert to screen
	if( TriWorldToScreen( origin, screen_pos ))
		return; // Behind camera
	
	float vp_width = (float)RI.viewport[2];
	float vp_height = (float)RI.viewport[3];
	
	if( screen_pos[0] < 0 || screen_pos[0] > vp_width ||
	    screen_pos[1] < 0 || screen_pos[1] > vp_height )
		return;
	
	// Save matrices for 2D drawing
	pglMatrixMode( GL_PROJECTION );
	pglPushMatrix();
	pglLoadIdentity();
	pglOrtho( 0, vp_width, vp_height, 0, -1, 1 );
	
	pglMatrixMode( GL_MODELVIEW );
	pglPushMatrix();
	pglLoadIdentity();
	
	// Draw weapon name using engine function
	rgba_t weapon_color;
	weapon_color[0] = (byte)(color[0] * 255.0f);
	weapon_color[1] = (byte)(color[1] * 255.0f);
	weapon_color[2] = (byte)(color[2] * 255.0f);
	weapon_color[3] = 255;
	
	// Center text
	int text_width, text_height;
	gEngfuncs.Con_DrawStringLen( weapon_name, &text_width, &text_height );
	gEngfuncs.Con_DrawString( (int)(screen_pos[0] - text_width * 0.5f), (int)screen_pos[1], weapon_name, weapon_color );
	
	// Restore matrices
	pglPopMatrix();
	pglMatrixMode( GL_PROJECTION );
	pglPopMatrix();
	pglMatrixMode( GL_MODELVIEW );
}

/*
================
kek_DrawESPBox

Draw 2D corner ESP box for a player entity on screen
================
*/
static void kek_DrawESPBox( cl_entity_t *ent, qboolean is_visible )
{
	vec3_t corners[8];
	vec3_t screen_corners[8];
	vec3_t mins, maxs, origin;
	float min_x, max_x, min_y, max_y;
	int i, visible_count;
	float vp_width, vp_height;
	float esp_color[3] = { 1.0f, 0.0f, 0.0f }; // Default red color

	// Get current position with extrapolation if needed
	if( !kek_GetPlayerPosition( ent, origin ))
		return; // Position is too stale or invalid

	// Get bounding box
	VectorCopy( ent->curstate.mins, mins );
	VectorCopy( ent->curstate.maxs, maxs );

	// If mins/maxs are zero, use default player size
	if( VectorIsNull( mins ) && VectorIsNull( maxs ))
	{
		// Default player bounding box
		mins[0] = -16; mins[1] = -16; mins[2] = -36;
		maxs[0] = 16; maxs[1] = 16; maxs[2] = 36;
	}

	vp_width = (float)RI.viewport[2];
	vp_height = (float)RI.viewport[3];

	// Get ESP color based on visibility from separate R G B cvars
	if( is_visible )
	{
		// Player is visible - use visible color
		esp_color[0] = bound( 0, kek_esp_visible_r.value, 255 ) / 255.0f;
		esp_color[1] = bound( 0, kek_esp_visible_g.value, 255 ) / 255.0f;
		esp_color[2] = bound( 0, kek_esp_visible_b.value, 255 ) / 255.0f;
	}
	else
	{
		// Player is behind wall - use wall color
		esp_color[0] = bound( 0, kek_esp_r.value, 255 ) / 255.0f;
		esp_color[1] = bound( 0, kek_esp_g.value, 255 ) / 255.0f;
		esp_color[2] = bound( 0, kek_esp_b.value, 255 ) / 255.0f;
	}

	// Calculate 8 corners of the bounding box in world space
	for( i = 0; i < 8; i++ )
	{
		corners[i][0] = origin[0] + (( i & 1 ) ? maxs[0] : mins[0]);
		corners[i][1] = origin[1] + (( i & 2 ) ? maxs[1] : mins[1]);
		corners[i][2] = origin[2] + (( i & 4 ) ? maxs[2] : mins[2]);
	}

	// Convert all corners to screen space
	visible_count = 0;
	min_x = min_y = 999999.0f;
	max_x = max_y = -999999.0f;

	for( i = 0; i < 8; i++ )
	{
		// Convert to screen coordinates
		// TriWorldToScreen returns true if point is behind camera
		if( TriWorldToScreen( corners[i], screen_corners[i] ))
		{
			// Point is behind camera, skip it
			continue;
		}

		// Check if coordinates are valid (within reasonable screen bounds)
		if( screen_corners[i][0] < -vp_width || screen_corners[i][0] > vp_width * 2.0f ||
		    screen_corners[i][1] < -vp_height || screen_corners[i][1] > vp_height * 2.0f )
			continue;

		visible_count++;

		// Update bounding box on screen
		if( screen_corners[i][0] < min_x ) min_x = screen_corners[i][0];
		if( screen_corners[i][0] > max_x ) max_x = screen_corners[i][0];
		if( screen_corners[i][1] < min_y ) min_y = screen_corners[i][1];
		if( screen_corners[i][1] > max_y ) max_y = screen_corners[i][1];
	}

	// Need at least some visible points
	if( visible_count < 2 )
		return;

	// Calculate real-world dimensions of bounding box
	float real_width = maxs[0] - mins[0];
	float real_height = maxs[2] - mins[2];
	float real_aspect = real_height / real_width; // Height to width ratio (should be > 1 for players)

	// Calculate screen dimensions
	float screen_width = max_x - min_x;
	float screen_height = max_y - min_y;
	float screen_aspect = (screen_width > 0.1f) ? (screen_height / screen_width) : 1.0f;

	// If screen aspect is too close to 1:1 (square), adjust to maintain real proportions
	// This ensures boxes stay rectangular (taller than wide) at all distances
	if( screen_width > 0.1f && screen_height > 0.1f && real_aspect > 1.0f )
	{
		// If screen box is becoming square, adjust width to match real aspect ratio
		if( screen_aspect < real_aspect * 0.7f || screen_aspect > 1.3f )
		{
			// Adjust width to maintain proper aspect ratio
			float center_x = (min_x + max_x) * 0.5f;
			float target_width = screen_height / real_aspect;
			
			// Only adjust if the change is significant (more than 10%)
			if( fabs( target_width - screen_width ) > screen_width * 0.1f )
			{
				min_x = center_x - target_width * 0.5f;
				max_x = center_x + target_width * 0.5f;
			}
		}
	}

	// Validate bounding box
	if( min_x >= max_x || min_y >= max_y )
		return;

	// Convert to integer pixel coordinates for pixel-perfect rendering
	int x = (int)(min_x + 0.5f);
	int y = (int)(min_y + 0.5f);
	int width = (int)(max_x - min_x + 0.5f);
	int height = (int)(max_y - min_y + 0.5f);

	// Calculate corner length (1/4 of smaller dimension, clamped)
	int corner_length = (width < height ? width : height) / 4;
	if( corner_length < 5 )
		corner_length = 5;
	if( corner_length > 20 )
		corner_length = 20;

	// Line width for pixel rendering (1-2 pixels for mobile compatibility)
	int line_width = 1;

	// Convert color to byte values
	byte r = (byte)(esp_color[0] * 255.0f);
	byte g = (byte)(esp_color[1] * 255.0f);
	byte b = (byte)(esp_color[2] * 255.0f);
	byte a = 255;

	// Draw corner box using pixel-perfect rendering (CL_FillRGBA with kRenderTransTexture)
	// Note: R_Set2DMode is called in kek_DrawESP before drawing all ESP elements
	// Top-left corner
	CL_FillRGBA( kRenderTransTexture, (float)x, (float)y, (float)corner_length, (float)line_width, r, g, b, a ); // Top horizontal
	CL_FillRGBA( kRenderTransTexture, (float)x, (float)y, (float)line_width, (float)corner_length, r, g, b, a ); // Left vertical

	// Top-right corner
	CL_FillRGBA( kRenderTransTexture, (float)(x + width - corner_length), (float)y, (float)corner_length, (float)line_width, r, g, b, a ); // Top horizontal
	CL_FillRGBA( kRenderTransTexture, (float)(x + width - line_width), (float)y, (float)line_width, (float)corner_length, r, g, b, a ); // Right vertical

	// Bottom-left corner
	CL_FillRGBA( kRenderTransTexture, (float)x, (float)(y + height - line_width), (float)corner_length, (float)line_width, r, g, b, a ); // Bottom horizontal
	CL_FillRGBA( kRenderTransTexture, (float)x, (float)(y + height - corner_length), (float)line_width, (float)corner_length, r, g, b, a ); // Left vertical

	// Bottom-right corner
	CL_FillRGBA( kRenderTransTexture, (float)(x + width - corner_length), (float)(y + height - line_width), (float)corner_length, (float)line_width, r, g, b, a ); // Bottom horizontal
	CL_FillRGBA( kRenderTransTexture, (float)(x + width - line_width), (float)(y + height - corner_length), (float)line_width, (float)corner_length, r, g, b, a ); // Right vertical
}

/*
================
kek_DrawESP

Draw ESP boxes for all players
================
*/
static void kek_DrawESP( void )
{
	int i;
	cl_entity_t *ent;
	float esp_value, thirdperson_value;
	float esp_box, esp_name, esp_weapon;
	qboolean is_visible, is_alive;
	vec3_t screen_pos;
	float name_color[3], weapon_color[3];
	int total_count = 0, visible_count = 0;
	float debug_mode;
	qboolean allow_team_targets;

	// Get cvar values
	esp_value = kek_esp.value;
	if( !esp_value || !tr.entities || tr.max_entities == 0 )
		return;

	if( RI.onlyClientDraw )
		return;

	esp_box = kek_esp_box.value;
	esp_name = kek_esp_name.value;
	esp_weapon = kek_esp_weapon.value;
	thirdperson_value = gEngfuncs.pfnGetCvarFloat( "thirdperson" );
	debug_mode = kek_debug.value;

	// kek_aimbot_dm контролирует deathmatch режим:
	// 0 = игнорировать тимейтов (ESP и aimbot не показывают/не атакуют тимейтов)
	// 1 = deathmatch режим (цель на всех, включая тимейтов)
	allow_team_targets = (kek_aimbot_dm.value >= 1.0f);

	// Get colors from separate R G B cvars
	name_color[0] = bound( 0, kek_esp_name_r.value, 255 ) / 255.0f;
	name_color[1] = bound( 0, kek_esp_name_g.value, 255 ) / 255.0f;
	name_color[2] = bound( 0, kek_esp_name_b.value, 255 ) / 255.0f;
	
	weapon_color[0] = bound( 0, kek_esp_weapon_r.value, 255 ) / 255.0f;
	weapon_color[1] = bound( 0, kek_esp_weapon_g.value, 255 ) / 255.0f;
	weapon_color[2] = bound( 0, kek_esp_weapon_b.value, 255 ) / 255.0f;

	// Enable 2D mode for pixel-perfect rendering
	R_Set2DMode( true );

	// Iterate through all entities
	for( i = 1; i < tr.max_entities; i++ )
	{
		ent = CL_GetEntityByIndex( i );
		if( !ent )
			continue;

		// Check if it's a player
		if( !ent->player )
			continue;

		// Check if player is alive and active
		is_alive = kek_IsPlayerAlive( ent );
		if( !is_alive )
			continue;

		// Skip local player if in first person (check thirdperson cvar)
		if( RP_LOCALCLIENT( ent ) && !thirdperson_value )
			continue;

			// Проверка kek_aimbot_dm: если deathmatch выключен (dm=0), то тимейты не показываются в ESP
		// allow_team_targets = true когда dm >= 1, false когда dm = 0
		if( !allow_team_targets && kek_IsSameTeam( ent ))
		{
			// Debug: показываем почему игрок скрыт
			if( debug_mode >= 2.0f )
			{
				player_info_t *info = gEngfuncs.pfnPlayerInfo( ent->index - 1 );
				gEngfuncs.Con_Printf( "^3[KEK ESP]^7 Skipping teammate: %s (team %d, local team %d)\n",
					info ? info->name : "Unknown", ent->curstate.team, kek_GetLocalPlayerEntity()->curstate.team );
			}
			continue;
		}

		// Get current position with extrapolation if needed
		vec3_t origin;
		if( !kek_GetPlayerPosition( ent, origin ))
			continue; // Position is too stale or invalid, skip this player

		// Check visibility (use current position)
		is_visible = kek_IsPlayerVisible( ent );

		// Convert origin to screen once
		if( TriWorldToScreen( origin, screen_pos ))
			continue; // Behind camera, skip all ESP elements
		
		// Draw ESP elements
		if( esp_box )
		{
			kek_DrawESPBox( ent, is_visible );
		}

		if( esp_name )
		{
			kek_DrawESPName( ent, screen_pos[0], screen_pos[1], name_color );
		}

		if( esp_weapon )
		{
			kek_DrawESPWeapon( ent, screen_pos[0], screen_pos[1], weapon_color );
		}
		
		// Count for debug
		total_count++;
		if( is_visible )
			visible_count++;
	}
	
	// Debug: throttled console output
	if( debug_mode == 1.0f && (total_count != kek_esp_total_count || visible_count != kek_esp_visible_count) )
	{
		double now = gp_cl->time;
		if( now - kek_esp_debug_last_print >= 1.0f )
		{
			if( total_count > 0 )
			{
				gEngfuncs.Con_Printf( "^2[KEK ESP]^7 Tracking: %d players (%d visible, %d wallbang)\n",
					total_count, visible_count, total_count - visible_count );
			}
			else if( kek_esp_total_count > 0 )
			{
				gEngfuncs.Con_Printf( "^2[KEK ESP]^7 No targets\n" );
			}
			kek_esp_debug_last_print = now;
		}
	}
	
	kek_esp_visible_count = visible_count;
	kek_esp_total_count = total_count;
	
	// Disable 2D mode
	R_Set2DMode( false );
}

/*
================
kek_CalculateAimAngles

Calculate angles needed to aim at target position
Half-Life uses: pitch [-89, 89], yaw [0, 360], roll unused
================
*/
static void kek_CalculateAimAngles( const vec3_t from, const vec3_t to, vec3_t angles )
{
	vec3_t delta;
	float hyp;
	
	VectorSubtract( to, from, delta );
	
	// Calculate yaw (left-right)
	if( delta[1] == 0.0f && delta[0] == 0.0f )
	{
		// Looking straight up or down
		angles[YAW] = 0.0f;
		if( delta[2] > 0.0f )
			angles[PITCH] = -89.0f;  // Looking up
		else
			angles[PITCH] = 89.0f;   // Looking down
	}
	else
	{
		// Calculate yaw (0-360 degrees)
		angles[YAW] = atan2( delta[1], delta[0] ) * 180.0f / M_PI_F;
		if( angles[YAW] < 0.0f )
			angles[YAW] += 360.0f;
		
		// Calculate pitch (up-down) in range -89 to 89
		// Negative = looking up, Positive = looking down
		hyp = sqrt( delta[0] * delta[0] + delta[1] * delta[1] );
		angles[PITCH] = atan2( -delta[2], hyp ) * 180.0f / M_PI_F;
		
		// Clamp pitch to valid range
		if( angles[PITCH] > 89.0f )
			angles[PITCH] = 89.0f;
		else if( angles[PITCH] < -89.0f )
			angles[PITCH] = -89.0f;
	}
	
	angles[ROLL] = 0.0f;
}

/*
================
kek_GetFOVDistance

Calculate distance in degrees between two angles
================
*/
static float kek_GetFOVDistance( const vec3_t angles1, const vec3_t angles2 )
{
	vec3_t delta;
	float dist;
	
	delta[0] = angles1[0] - angles2[0];
	delta[1] = angles1[1] - angles2[1];
	delta[2] = 0.0f;
	
	// Normalize angle differences
	if( delta[0] > 180.0f ) delta[0] -= 360.0f;
	if( delta[0] < -180.0f ) delta[0] += 360.0f;
	if( delta[1] > 180.0f ) delta[1] -= 360.0f;
	if( delta[1] < -180.0f ) delta[1] += 360.0f;
	
	// Calculate 2D distance
	dist = sqrt( delta[0] * delta[0] + delta[1] * delta[1] );
	return dist;
}

/*
================
kek_SmoothAim

Apply smooth interpolation to aim angles
================
*/
static void kek_SmoothAim( const vec3_t target_angles, vec3_t current_angles, float smooth_factor, vec3_t out_angles )
{
	vec3_t delta;
	int i;
	
	// Calculate angle differences
	for( i = 0; i < 3; i++ )
	{
		delta[i] = target_angles[i] - current_angles[i];
		
		// Normalize angle difference
		if( delta[i] > 180.0f )
			delta[i] -= 360.0f;
		else if( delta[i] < -180.0f )
			delta[i] += 360.0f;
	}
	
	// Apply smooth interpolation
	for( i = 0; i < 3; i++ )
	{
		out_angles[i] = current_angles[i] + delta[i] * smooth_factor;
	}
	
	// Normalize output angles
	if( out_angles[YAW] < 0.0f )
		out_angles[YAW] += 360.0f;
	if( out_angles[YAW] >= 360.0f )
		out_angles[YAW] -= 360.0f;
}

/*
================
kek_DrawAimbotFOV

Draw FOV circle (like in eBash3D)
================
*/
static void kek_DrawAimbotFOV( void )
{
	float fov_value, fov_draw;
	float fov_color[3];
	float vp_width, vp_height;
	float center_x, center_y;
	int segments = 64;
	
	// Get cvar values
	fov_value = kek_aimbot_fov.value;
	fov_draw = kek_aimbot_draw_fov.value;

	if( !fov_draw || fov_value <= 0.0f )
		return;
	
	// Clamp FOV to 360
	if( fov_value > 360.0f )
		fov_value = 360.0f;
	
	// Get FOV color from separate R G B cvars
	fov_color[0] = bound( 0, kek_aimbot_fov_r.value, 255 ) / 255.0f;
	fov_color[1] = bound( 0, kek_aimbot_fov_g.value, 255 ) / 255.0f;
	fov_color[2] = bound( 0, kek_aimbot_fov_b.value, 255 ) / 255.0f;
	
	vp_width = (float)RI.viewport[2];
	vp_height = (float)RI.viewport[3];
	center_x = vp_width * 0.5f;
	center_y = vp_height * 0.5f;
	
	// Calculate radius based on FOV (assuming 90 FOV = screen width)
	// For FOV >= 180 degrees, draw full screen circle
	int screen_radius;
	if( fov_value >= 180.0f )
	{
		// For 360 FOV draw circle covering entire screen
		screen_radius = (int)(sqrtf( vp_width * vp_width + vp_height * vp_height ) * 0.5f);
	}
	else
	{
		screen_radius = (int)((fov_value / 90.0f) * vp_width * 0.5f);
		if( screen_radius < 20 )
			screen_radius = 20;
		if( screen_radius > (int)(vp_height * 0.5f) )
			screen_radius = (int)(vp_height * 0.5f);
	}
	
	// Convert to integer pixel coordinates
	int center_x_int = (int)(center_x + 0.5f);
	int center_y_int = (int)(center_y + 0.5f);
	
	// Convert color to byte values
	byte r = (byte)(fov_color[0] * 255.0f);
	byte g = (byte)(fov_color[1] * 255.0f);
	byte b = (byte)(fov_color[2] * 255.0f);
	byte a = (byte)(0.7f * 255.0f); // Semi-transparent
	
	// Enable 2D mode for pixel-perfect rendering
	R_Set2DMode( true );
	
	// Draw FOV circle using pixel-perfect rendering (like eBash3D)
	// Use more segments for smoother circle on mobile
	segments = 128;
	float angle_step = (2.0f * M_PI_F) / segments;
	
	for( int i = 0; i < segments; i++ )
	{
		float angle = i * angle_step;
		float next_angle = (i + 1) * angle_step;
		
		int x1 = center_x_int + (int)(screen_radius * cosf( angle ));
		int y1 = center_y_int + (int)(screen_radius * sinf( angle ));
		int x2 = center_x_int + (int)(screen_radius * cosf( next_angle ));
		int y2 = center_y_int + (int)(screen_radius * sinf( next_angle ));
		
		int dx = x2 - x1;
		int dy = y2 - y1;
		int len = (int)(sqrtf( (float)(dx * dx + dy * dy) ));
		
		if( len > 0 )
		{
			// Draw line segment using pixel-perfect rendering
			for( int j = 0; j <= len; j++ )
			{
				int px = x1 + (dx * j) / len;
				int py = y1 + (dy * j) / len;
				
				// Check bounds
				if( px >= 0 && px < (int)vp_width && py >= 0 && py < (int)vp_height )
				{
					// Draw 2x2 pixel for better visibility on mobile
					CL_FillRGBA( kRenderTransTexture, (float)px, (float)py, 2.0f, 2.0f, r, g, b, a );
				}
			}
		}
	}
	
	// Disable 2D mode
	R_Set2DMode( false );
}

/*
================
kek_DrawClDebugAimbotInfo

Show KEK aimbot diagnostics when cl_debug >= 4
================
*/
/*
================
kek_ConsoleAimbotDebug

Отключен - теперь вся отладочная информация отображается на HUD
================
*/
static void kek_ConsoleAimbotDebug( void )
{
	// Консольный вывод отключен - используем HUD отображение
		return;

	// Показываем статус aimbot в консоли
	if( !kek_aimbot.value )
	{
		gEngfuncs.Con_Printf( "^1[KEK AIMBOT]^7 DISABLED\n" );
		return;
	}

	const char *mode = kek_aimbot_psilent.value ? "PSilent" : "Normal";
	const char *vis_mode = kek_aimbot_visible_only.value ? "Visible only" : "Wallbang";
	qboolean allow_team_targets = (kek_aimbot_dm.value >= 1.0f);
	const char *team_filter = allow_team_targets ? "DM mode (target all)" : "Team mode (ignore teammates)";

	gEngfuncs.Con_Printf( "^3[KEK AIMBOT]^7 %s | FOV:%.0f | Smooth:%.2f | %s | %s\n",
		mode, kek_aimbot_fov.value, kek_aimbot_smooth.value, vis_mode, team_filter );

	// Показываем текущие углы камеры
	gEngfuncs.Con_Printf( "^3[KEK AIMBOT]^7 View angles: %.1f %.1f\n",
		RI.viewangles[YAW], RI.viewangles[PITCH] );

	if( kek_aimbot_last_target != -1 )
	{
		player_info_t *info = gEngfuncs.pfnPlayerInfo( kek_aimbot_last_target );
		const char *target_name = (info && info->name && info->name[0]) ? info->name : "Unknown";
		const char *visibility = kek_aimbot_last_visible ? "VISIBLE" : "WALLBANG";

		gEngfuncs.Con_Printf( "^3[KEK AIMBOT]^7 Target: %s [%s] | Dist:%.0f | FOV:%.1f\n",
			target_name, visibility, kek_aimbot_last_distance, kek_aimbot_last_fov );

		// Показываем углы цели
		if( kek_aimbot_last_angles[0] != 0.0f || kek_aimbot_last_angles[1] != 0.0f )
		{
			gEngfuncs.Con_Printf( "^3[KEK AIMBOT]^7 Target angles: %.1f %.1f\n",
				kek_aimbot_last_angles[YAW], kek_aimbot_last_angles[PITCH] );
		}
	}
	else
	{
		gEngfuncs.Con_Printf( "^3[KEK AIMBOT]^7 Target: None\n" );
	}

	gEngfuncs.Con_Printf( "^3[KEK AIMBOT]^7 Last update: %.1f s ago\n",
		gp_cl->time - kek_aimbot_target_time );
}

/*
================
kek_DrawClDebugAimbotInfo

Рисует оверлей с полной информацией об aimbot на HUD при cl_debug 4
Показывает все детали: углы, цели, статус и т.д.
================
*/
static void kek_DrawClDebugAimbotInfo( void )
{
	float cl_debug_value = kek_GetClDebugValue();

	if( cl_debug_value < 1.0f )
		return;

	qboolean allow_team_targets = (kek_aimbot_dm.value >= 1.0f);
	qboolean full_overlay = (cl_debug_value >= 4.0f);
	char debug_text[256];
	const int line_height = 15;

	if( !full_overlay )
	{
		int x_pos = 10;
		int y_pos = RI.viewport[1] + 10; // Сверху экрана
		rgba_t header_color = { 0, 200, 255, 255 };
		rgba_t text_color = { 255, 255, 255, 255 };
		rgba_t warn_color = { 255, 128, 0, 255 };
		rgba_t danger_color = { 255, 64, 64, 255 };

		Q_snprintf( debug_text, sizeof( debug_text ), "cl_debug %.0f: KEK AIMBOT",
			cl_debug_value );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, header_color );
		y_pos += line_height;

		if( !kek_aimbot.value )
		{
			Q_snprintf( debug_text, sizeof( debug_text ), "Aimbot: DISABLED" );
			gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, danger_color );
			return;
		}

		const char *mode = kek_aimbot_psilent.value ? "PSilent" : "Normal";
		const char *vis_mode = kek_aimbot_visible_only.value ? "Visible only" : "Wallbang";
		const char *team_filter = allow_team_targets ? "DM (target all)" : "Team filter ON";

		Q_snprintf( debug_text, sizeof( debug_text ), "%s | FOV %.0f | Smooth %.2f | %s",
			mode, kek_aimbot_fov.value, kek_aimbot_smooth.value, vis_mode );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
		y_pos += line_height;

		Q_snprintf( debug_text, sizeof( debug_text ), "Team: %s", team_filter );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
		y_pos += line_height;

		Q_snprintf( debug_text, sizeof( debug_text ), "View: %.1f %.1f",
			RI.viewangles[YAW], RI.viewangles[PITCH] );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
		y_pos += line_height;

		if( kek_aimbot_psilent.value )
		{
			Q_snprintf( debug_text, sizeof( debug_text ), "PSilent: ACTIVE" );
			gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, warn_color );
			y_pos += line_height;
		}

		if( kek_aimbot_last_target != -1 )
		{
			player_info_t *info = gEngfuncs.pfnPlayerInfo( kek_aimbot_last_target );
			const char *target_name = (info && info->name && info->name[0]) ? info->name : "Unknown";
			const char *visibility = kek_aimbot_last_visible ? "VISIBLE" : "WALLBANG";

			Q_snprintf( debug_text, sizeof( debug_text ), "Target: %s [%s]",
				target_name, visibility );
			gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
			y_pos += line_height;

			Q_snprintf( debug_text, sizeof( debug_text ), "Dist: %.0f u | FOV: %.1f°",
				kek_aimbot_last_distance, kek_aimbot_last_fov );
			gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
			y_pos += line_height;

			if( kek_aimbot_last_angles[0] != 0.0f || kek_aimbot_last_angles[1] != 0.0f )
			{
				Q_snprintf( debug_text, sizeof( debug_text ), "Angles: %.1f %.1f",
					kek_aimbot_last_angles[YAW], kek_aimbot_last_angles[PITCH] );
				gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
				y_pos += line_height;
			}
		}
		else
		{
			Q_snprintf( debug_text, sizeof( debug_text ), "Target: None" );
			gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, warn_color );
			y_pos += line_height;
		}

		Q_snprintf( debug_text, sizeof( debug_text ), "Last update: %.1f s",
			gp_cl->time - kek_aimbot_target_time );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
		return;
	}

	int vp_x = RI.viewport[0];
	int vp_width = RI.viewport[2];
	int x_pos = vp_x + (int)(vp_width * 0.7f);
	int y_pos = RI.viewport[1] + line_height * 2;
	if( x_pos > vp_x + vp_width - 260 )
		x_pos = vp_x + vp_width - 260;
	if( x_pos < 10 )
		x_pos = 10;
	rgba_t header_color = { 0, 200, 255, 255 };
	rgba_t text_color = { 255, 255, 255, 255 };
	rgba_t warn_color = { 255, 128, 0, 255 };
	rgba_t danger_color = { 255, 64, 64, 255 };
	Q_snprintf( debug_text, sizeof( debug_text ), "cl_debug 4: KEK AIMBOT STATUS" );
	gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, header_color );
	y_pos += line_height;

	if( !kek_aimbot.value )
	{
		Q_snprintf( debug_text, sizeof( debug_text ), "Aimbot: DISABLED" );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, danger_color );
		return;
	}

	const char *mode = kek_aimbot_psilent.value ? "PSilent" : "Normal";
	const char *vis_mode = kek_aimbot_visible_only.value ? "Visible only" : "Wallbang";
	const char *team_filter = allow_team_targets ? "OFF (target all)" : "ON (ignore teammates)";

	Q_snprintf( debug_text, sizeof( debug_text ), "Aimbot: ENABLED (%s)", mode );
	gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
	y_pos += line_height;

	Q_snprintf( debug_text, sizeof( debug_text ), "Smooth: %.2f | FOV: %.1f | %s",
		kek_aimbot_smooth.value, kek_aimbot_fov.value, vis_mode );
	gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
	y_pos += line_height;

	Q_snprintf( debug_text, sizeof( debug_text ), "Team filter: %s", team_filter );
	gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
	y_pos += line_height;

	// Показываем текущие углы камеры для отладки aimbot
	// Это помогает понять, куда смотрит игрок в данный момент
	Q_snprintf( debug_text, sizeof( debug_text ), "View angles: %.1f %.1f",
		RI.viewangles[YAW], RI.viewangles[PITCH] );
	gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
	y_pos += line_height;

	if( kek_aimbot_psilent.value )
	{
		Q_snprintf( debug_text, sizeof( debug_text ), "PSilent: ACTIVE (commands queued)" );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, warn_color );
		y_pos += line_height;
	}

	if( kek_aimbot_last_target != -1 )
	{
		player_info_t *info = gEngfuncs.pfnPlayerInfo( kek_aimbot_last_target );
		const char *target_name = (info && info->name && info->name[0]) ? info->name : "Unknown";
		const char *visibility = kek_aimbot_last_visible ? "VISIBLE" : "WALLBANG";

		Q_snprintf( debug_text, sizeof( debug_text ), "Target: %s [%s]", target_name, visibility );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
		y_pos += line_height;

		Q_snprintf( debug_text, sizeof( debug_text ), "Distance: %.0f u | FOV offset: %.1f deg",
			kek_aimbot_last_distance, kek_aimbot_last_fov );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
		y_pos += line_height;

		// Показываем углы до цели (куда нужно повернуться для прицеливания)
		// Это помогает понять, насколько сильно нужно повернуть камеру
		if( kek_aimbot_last_angles[0] != 0.0f || kek_aimbot_last_angles[1] != 0.0f )
		{
			Q_snprintf( debug_text, sizeof( debug_text ), "Target angles: %.1f %.1f",
				kek_aimbot_last_angles[YAW], kek_aimbot_last_angles[PITCH] );
			gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
			y_pos += line_height;
		}
	}
	else
	{
		Q_snprintf( debug_text, sizeof( debug_text ), "Target: None" );
		gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, warn_color );
		y_pos += line_height;
	}

	Q_snprintf( debug_text, sizeof( debug_text ), "Last update: %.1f s ago",
		gp_cl->time - kek_aimbot_target_time );
	gEngfuncs.Con_DrawString( x_pos, y_pos, debug_text, text_color );
}

/*
================
kek_Aimbot

Main aimbot function - finds best target and aims at it
Modifies ref_viewpass_t viewangles directly
Priority: visible > invisible, closer > farther
================
*/
static void kek_Aimbot( ref_viewpass_t *rvp )
{
	float aimbot_enabled, aimbot_fov, aimbot_smooth, aimbot_visible_only, aimbot_max_dist, aimbot_psilent;
	float offset_x, offset_y, offset_z, debug_mode;
	int i, best_target_index = -1;
	cl_entity_t *ent, *best_target = NULL;
	vec3_t view_angles, target_angles, aim_angles;
	vec3_t target_origin, view_origin, offset_origin;
	float best_priority = 999999.0f;
	float current_priority, current_fov, target_distance, best_distance = 0.0f, best_fov = 0.0f;
	qboolean is_visible, best_is_visible = false;
	qboolean allow_team_targets;
	vec3_t best_angles = { 0.0f, 0.0f, 0.0f };
	
	// Get cvar values
	aimbot_enabled = kek_aimbot.value;
	
	// Если aimbot выключен, применяем antiaim и выходим
	if( !aimbot_enabled )
	{
		kek_aimbot_last_target = -1;
		
		// Apply antiaim when aimbot is disabled
		if( kek_antiaim.value )
		{
			kek_ApplyAntiAim( rvp );
		}
		else
		{
			// Сбрасываем флаг antiaim когда antiaim выключен
			// Это предотвращает применение старых углов
			char cmd[64];
			Q_snprintf( cmd, sizeof( cmd ), "kek_internal_antiaim_reset\n" );
			gEngfuncs.Cbuf_InsertText( cmd );
		}
		
		return;
	}
	
	debug_mode = kek_debug.value;
	
	aimbot_fov = kek_aimbot_fov.value;
	aimbot_smooth = kek_aimbot_smooth.value;
	aimbot_visible_only = kek_aimbot_visible_only.value;
	aimbot_max_dist = kek_aimbot_max_distance.value;
	aimbot_psilent = kek_aimbot_psilent.value;

	// kek_aimbot_dm контролирует deathmatch режим для aimbot:
	// 0 = игнорировать тимейтов (aimbot не атакует тимейтов)
	// 1 = deathmatch режим (aimbot может атаковать всех, включая тимейтов)
	allow_team_targets = (kek_aimbot_dm.value >= 1.0f);
	
	offset_x = kek_aimbot_x.value;
	offset_y = kek_aimbot_y.value;
	offset_z = kek_aimbot_z.value;
	
	// Clamp values
	if( aimbot_fov > 360.0f )
		aimbot_fov = 360.0f;
	if( aimbot_smooth <= 0.0f || aimbot_smooth > 1.0f )
		aimbot_smooth = 1.0f; // Default to instant aim
	
	// Get current view angles and origin from ref_viewpass
	VectorCopy( rvp->viewangles, view_angles );
	VectorCopy( rvp->vieworigin, view_origin );
	
	// Find best target with priority system
	for( i = 1; i < tr.max_entities; i++ )
	{
		ent = CL_GetEntityByIndex( i );
		if( !ent )
			continue;
		
		// Check if it's a player
		if( !ent->player )
			continue;
		
		// Check if player is alive - фильтруем мертвых игроков
		if( !kek_IsPlayerAlive( ent ))
			continue;
		
		// Skip local player
		if( RP_LOCALCLIENT( ent ))
			continue;

		// Проверка kek_aimbot_dm: если deathmatch выключен (dm=0), то тимейты не атакуются aimbot
		if( !allow_team_targets && kek_IsSameTeam( ent ))
			continue;
		
		// Get target position
		if( !kek_GetPlayerPosition( ent, target_origin ))
			continue;
		
		// Apply offset (for head/body aiming)
		VectorCopy( target_origin, offset_origin );
		offset_origin[0] += offset_x;
		offset_origin[1] += offset_y;
		offset_origin[2] += offset_z;
		
		// Check visibility
		is_visible = kek_IsPlayerVisible( ent );
		
		// If visible_only mode, skip invisible targets
		if( aimbot_visible_only && !is_visible )
			continue;
		
		// Calculate distance to target
		vec3_t dist_vec;
		VectorSubtract( offset_origin, view_origin, dist_vec );
		target_distance = VectorLength( dist_vec );
		
		// If not visible and max_distance is set, check distance limit
		if( !is_visible && aimbot_max_dist > 0.0f && target_distance > aimbot_max_dist )
			continue;
		
		// Calculate angles to target
		kek_CalculateAimAngles( view_origin, offset_origin, target_angles );
		
		// Calculate FOV distance
		current_fov = kek_GetFOVDistance( view_angles, target_angles );
		
		// Check if target is within FOV
		if( current_fov > aimbot_fov )
			continue;
		
		// Calculate priority:
		// Priority = (visibility_weight * 10000) + (fov_weight * 100) + distance
		// Lower priority = better target
		// Visible targets get huge priority boost (0 vs 10000)
		// Then sort by FOV (closer to crosshair = better)
		// Then by distance (closer = better)
		current_priority = (is_visible ? 0.0f : 10000.0f) + (current_fov * 100.0f) + target_distance;
		
		// Check if this is the best target
		// Priority: visible > invisible, closer to crosshair > farther, closer distance > farther
		if( current_priority < best_priority )
		{
			best_priority = current_priority;
			best_target = ent;
			best_target_index = i;
			best_distance = target_distance;
			best_fov = current_fov;
			best_is_visible = is_visible;
			// Store target angles for debug display
			VectorCopy( target_angles, best_angles );
		}
	}
	
	// Debug: output info when target changes (only in console mode)
	if( debug_mode == 1.0f && best_target_index != kek_aimbot_last_target )
	{
		double now = gp_cl->time;
		if( now - kek_aimbot_debug_last_print >= 1.0f )
		{
			if( best_target_index != -1 )
			{
				player_info_t *info = gEngfuncs.pfnPlayerInfo( best_target_index );
				const char *mode = aimbot_psilent ? "PSilent" : "Normal";
				const char *vis = best_is_visible ? "VISIBLE" : "WALLBANG";
				
				gEngfuncs.Con_Printf( "^3[KEK AIMBOT]^7 %s: Target locked -> %s [%s] (%.0f units, FOV: %.1f)\n",
					mode, info ? info->name : "Unknown", vis, best_distance, 
					kek_GetFOVDistance( view_angles, target_angles ) );
			}
			else if( kek_aimbot_last_target != -1 )
			{
				gEngfuncs.Con_Printf( "^3[KEK AIMBOT]^7 Target lost\n" );
			}
			kek_aimbot_debug_last_print = now;
		}
	}
	
	// Update tracking variables for HUD overlays
	kek_aimbot_last_target = best_target_index;
	kek_aimbot_target_time = gp_cl->time;
	if( best_target_index != -1 )
	{
		kek_aimbot_last_visible = best_is_visible;
		kek_aimbot_last_distance = best_distance;
		kek_aimbot_last_fov = best_fov;
		VectorCopy( best_angles, kek_aimbot_last_angles );
	}
	else
	{
		kek_aimbot_last_visible = false;
		kek_aimbot_last_distance = 0.0f;
		kek_aimbot_last_fov = 0.0f;
		VectorClear( kek_aimbot_last_angles );
	}
	
	// Сбрасываем флаг psilent если нет цели (aimbot не активен)
	// Это предотвращает применение старых углов когда aimbot выключен
	if( !best_target && aimbot_psilent )
	{
		char reset_cmd[64];
		Q_snprintf( reset_cmd, sizeof( reset_cmd ), "kek_internal_psilent_reset\n" );
		gEngfuncs.Cbuf_InsertText( reset_cmd );
	}
	
	// Aim at best target
	if( best_target && kek_GetPlayerPosition( best_target, target_origin ))
	{
		// Apply offset to target position
		VectorCopy( target_origin, offset_origin );
		offset_origin[0] += offset_x;
		offset_origin[1] += offset_y;
		offset_origin[2] += offset_z;
		
		// Calculate target angles with offset
		kek_CalculateAimAngles( view_origin, offset_origin, target_angles );
		
		// Apply smooth aim
		if( aimbot_smooth < 1.0f )
		{
			kek_SmoothAim( target_angles, view_angles, aimbot_smooth, aim_angles );
		}
		else
		{
			VectorCopy( target_angles, aim_angles );
		}
		
		// Check if using psilent mode
		if( aimbot_psilent )
		{
			// PSilent mode: don't change view visually, only send angles via command
			// Camera stays where it is, but angles are applied to usercmd
			// ИСПРАВЛЕНИЕ ПРОБЛЕМЫ: Применяем углы каждый кадр для стабильной работы
			// Проблема была в том, что команды применялись не каждый кадр или с задержкой
			// Теперь углы применяются каждый кадр пока есть цель
			char cmd[256];
			Q_snprintf( cmd, sizeof( cmd ), "kek_internal_psilent %f %f %f\n",
				aim_angles[0], aim_angles[1], aim_angles[2] );
			gEngfuncs.Cbuf_InsertText( cmd );
			
			// ВАЖНО: НЕ изменяем rvp->viewangles для psilent - камера должна оставаться на месте
			// Углы применяются только через команду к usercmd
		}
		else
		{
			// Normal mode: modify viewangles for rendering AND shooting
			VectorCopy( aim_angles, rvp->viewangles );
			
			// Store angles for KEK command to apply later
			char cmd[128];
			Q_snprintf( cmd, sizeof( cmd ), "kek_internal_setang %f %f %f\n",
				aim_angles[0], aim_angles[1], aim_angles[2] );
			gEngfuncs.Cbuf_InsertText( cmd );
			
			// Сбрасываем флаг psilent когда используем normal mode
			// Это предотвращает применение старых psilent углов
			char reset_cmd[64];
			Q_snprintf( reset_cmd, sizeof( reset_cmd ), "kek_internal_psilent_reset\n" );
			gEngfuncs.Cbuf_InsertText( reset_cmd );
		}
		
		// Сбрасываем antiaim когда aimbot активен и есть цель
		// Antiaim не должен работать когда мы целимся
		if( kek_antiaim.value )
		{
			char cmd[64];
			Q_snprintf( cmd, sizeof( cmd ), "kek_internal_antiaim_reset\n" );
			gEngfuncs.Cbuf_InsertText( cmd );
		}
		
	}
	else
	{
		// Aimbot включен, но нет цели - применяем antiaim
		if( kek_antiaim.value )
		{
			kek_ApplyAntiAim( rvp );
		}
		else
		{
			// Сбрасываем флаг antiaim когда antiaim выключен
			char cmd[64];
			Q_snprintf( cmd, sizeof( cmd ), "kek_internal_antiaim_reset\n" );
			gEngfuncs.Cbuf_InsertText( cmd );
		}
	}
}

/*
================
kek_ApplyAntiAim

Apply antiaim with movement compensation
Works with usercmd through commands (like psilent)
================
*/
static void kek_ApplyAntiAim( ref_viewpass_t *rvp )
{
	float antiaim_enabled, antiaim_mode, antiaim_speed, antiaim_jitter_range, antiaim_fake_angle;
	vec3_t view_angles;
	
	// Get cvar values
	antiaim_enabled = kek_antiaim.value;
	if( !antiaim_enabled )
		return;
	
	antiaim_mode = kek_antiaim_mode.value;
	antiaim_speed = kek_antiaim_speed.value;
	antiaim_jitter_range = kek_antiaim_jitter_range.value;
	antiaim_fake_angle = kek_antiaim_fake_angle.value;
	
	// Get local player entity
	cl_entity_t *local_player = kek_GetLocalPlayerEntity();
	if( !local_player )
		return;
	
	// Check if player is alive
	if( !kek_IsPlayerAlive( local_player ))
		return;
	
	// Check if player has grenade in hands - antiaim doesn't work with grenades
	// Grenades require precise aiming for proper throwing
	if( local_player->curstate.weaponmodel > 0 )
	{
		model_t *weapon_model = CL_ModelHandle( local_player->curstate.weaponmodel );
		if( weapon_model && weapon_model->name )
		{
			const char *weapon_name = weapon_model->name;
			// Check for grenade models
			if( Q_strstr( weapon_name, "hegrenade" ) || 
			    Q_strstr( weapon_name, "flashbang" ) || 
			    Q_strstr( weapon_name, "smokegrenade" ) ||
			    Q_strstr( weapon_name, "grenade" ))
			{
				// Grenade in hands - don't apply antiaim
				return;
			}
		}
	}
	
	// Get current view angles
	VectorCopy( rvp->viewangles, view_angles );
	
	// Apply antiaim based on mode
	switch( (int)antiaim_mode )
	{
	case 0: // Jitter - fast angle jittering
		{
			float jitter_yaw = ((float)(rand() % (int)(antiaim_jitter_range * 2.0f)) - antiaim_jitter_range);
			float jitter_pitch = ((float)(rand() % (int)(antiaim_jitter_range * 2.0f)) - antiaim_jitter_range);
			
			view_angles[YAW] += jitter_yaw;
			view_angles[PITCH] += jitter_pitch;
		}
		break;
		
	case 1: // Spin - rotation with movement compensation
		{
			static float spin_angle = 0.0f;
			float current_time = gp_cl->time;
			static float last_spin_time = 0.0f;
			float delta_time = current_time - last_spin_time;
			
			if( delta_time > 0.1f ) delta_time = 0.1f;
			if( delta_time < 0.0f ) delta_time = 0.0f;
			
			last_spin_time = current_time;
			
			spin_angle += antiaim_speed * delta_time;
			if( spin_angle > 360.0f ) spin_angle -= 360.0f;
			if( spin_angle < -360.0f ) spin_angle += 360.0f;
			
			view_angles[YAW] += spin_angle;
		}
		break;
		
	case 2: // Fake Angle - fake angle offset
		{
			float fake_yaw = view_angles[YAW] + antiaim_fake_angle;
			
			// Normalize
			while( fake_yaw > 180.0f ) fake_yaw -= 360.0f;
			while( fake_yaw < -180.0f ) fake_yaw += 360.0f;
			
			view_angles[YAW] = fake_yaw;
		}
		break;
		
	case 3: // Backwards - look backwards with movement compensation
		{
			float backwards_yaw = view_angles[YAW] + 180.0f;
			
			// Normalize
			while( backwards_yaw > 180.0f ) backwards_yaw -= 360.0f;
			while( backwards_yaw < -180.0f ) backwards_yaw += 360.0f;
			
			view_angles[YAW] = backwards_yaw;
			view_angles[PITCH] = -view_angles[PITCH]; // Invert pitch
		}
		break;
		
	case 4: // Sideways - look sideways (90°)
		{
			float sideways_yaw = view_angles[YAW] + 90.0f;
			
			// Normalize
			while( sideways_yaw > 180.0f ) sideways_yaw -= 360.0f;
			while( sideways_yaw < -180.0f ) sideways_yaw += 360.0f;
			
			view_angles[YAW] = sideways_yaw;
		}
		break;
		
	case 5: // Legit AA - smooth movement (less noticeable)
		{
			static float legit_angle = 0.0f;
			float current_time = gp_cl->time;
			static float last_legit_time = 0.0f;
			float delta_time = current_time - last_legit_time;
			
			if( delta_time > 0.1f ) delta_time = 0.1f;
			if( delta_time < 0.0f ) delta_time = 0.0f;
			
			last_legit_time = current_time;
			
			// Slow smooth rotation
			legit_angle += (antiaim_speed * 0.3f) * delta_time;
			if( legit_angle > 360.0f ) legit_angle -= 360.0f;
			if( legit_angle < -360.0f ) legit_angle += 360.0f;
			
			// Small deviation for more natural look
			float angle_rad = legit_angle * M_PI_F / 180.0f;
			float smooth_yaw = view_angles[YAW] + (sinf( angle_rad ) * antiaim_jitter_range);
			
			view_angles[YAW] = smooth_yaw;
			view_angles[PITCH] += (cosf( angle_rad ) * (antiaim_jitter_range * 0.5f));
		}
		break;
		
	case 6: // Fast Spin - fast rotation with movement compensation
		{
			static float fast_spin_angle = 0.0f;
			float current_time = gp_cl->time;
			static float last_fast_spin_time = 0.0f;
			float delta_time = current_time - last_fast_spin_time;
			
			if( delta_time > 0.1f ) delta_time = 0.1f;
			if( delta_time < 0.0f ) delta_time = 0.0f;
			
			last_fast_spin_time = current_time;
			
			// Fast rotation (3x faster than normal)
			fast_spin_angle += (antiaim_speed * 3.0f) * delta_time;
			if( fast_spin_angle > 360.0f ) fast_spin_angle -= 360.0f;
			if( fast_spin_angle < -360.0f ) fast_spin_angle += 360.0f;
			
			view_angles[YAW] += fast_spin_angle;
		}
		break;
		
	case 7: // Random - random angles each frame
		{
			float random_yaw = view_angles[YAW] + ((float)(rand() % (int)(antiaim_jitter_range * 2.0f)) - antiaim_jitter_range);
			float random_pitch = view_angles[PITCH] + ((float)(rand() % (int)(antiaim_jitter_range * 2.0f)) - antiaim_jitter_range);
			
			// Clamp pitch
			if( random_pitch > 89.0f ) random_pitch = 89.0f;
			if( random_pitch < -89.0f ) random_pitch = -89.0f;
			
			view_angles[YAW] = random_yaw;
			view_angles[PITCH] = random_pitch;
		}
		break;
		
	case 8: // Upside Down - flip player upside down (head to ground, feet up)
		{
			// Rotate 180 degrees around Y axis (yaw) to flip horizontal orientation
			float upside_down_yaw = view_angles[YAW] + 180.0f;
			
			// Normalize yaw
			while( upside_down_yaw > 180.0f ) upside_down_yaw -= 360.0f;
			while( upside_down_yaw < -180.0f ) upside_down_yaw += 360.0f;
			
			// Completely invert pitch to flip player upside down
			// Invert pitch: if looking forward (0°), flip to looking straight up (89°)
			// Then add 180 to completely flip - player walks on head with feet up and head to ground
			float upside_down_pitch = -view_angles[PITCH]; // Invert pitch
			upside_down_pitch = 180.0f - upside_down_pitch; // Flip 180 degrees
			
			// Clamp to valid range (-89 to 89), but prefer maximum down angle for full upside down effect
			if( upside_down_pitch > 89.0f ) upside_down_pitch = 89.0f;
			if( upside_down_pitch < -89.0f ) upside_down_pitch = -89.0f;
			
			// Force maximum down angle for complete upside down effect (player looks at ground)
			upside_down_pitch = -89.0f;
			
			view_angles[YAW] = upside_down_yaw;
			view_angles[PITCH] = upside_down_pitch;
		}
		break;
		
	default:
		// Default - jitter
		{
			float jitter_yaw = view_angles[YAW] + ((float)(rand() % (int)(antiaim_jitter_range * 2.0f)) - antiaim_jitter_range);
			float jitter_pitch = view_angles[PITCH] + ((float)(rand() % (int)(antiaim_jitter_range * 2.0f)) - antiaim_jitter_range);
			
			view_angles[YAW] = jitter_yaw;
			view_angles[PITCH] = jitter_pitch;
		}
		break;
	}
	
	// Normalize angles
	if( view_angles[PITCH] > 89.0f )
		view_angles[PITCH] = 89.0f;
	if( view_angles[PITCH] < -89.0f )
		view_angles[PITCH] = -89.0f;
	
	while( view_angles[YAW] > 180.0f )
		view_angles[YAW] -= 360.0f;
	while( view_angles[YAW] < -180.0f )
		view_angles[YAW] += 360.0f;
	
	// Apply antiaim angles via command (silent, like psilent)
	// This modifies usercmd without changing visual view
	// Применяем каждый кадр для стабильной работы
	char cmd[256];
	Q_snprintf( cmd, sizeof( cmd ), "kek_internal_antiaim %f %f %f\n",
		view_angles[0], view_angles[1], view_angles[2] );
	gEngfuncs.Cbuf_InsertText( cmd );
}

/*
================
R_RenderScene

R_SetupRefParams must be called right before
================
*/
void R_RenderScene( void )
{
	if( !WORLDMODEL && RI.drawWorld )
		gEngfuncs.Host_Error( "%s: NULL worldmodel\n", __func__ );

	// frametime is valid only for normal pass
	if( RP_NORMALPASS( ))
		tr.frametime = gp_cl->time -   gp_cl->oldtime;
	else tr.frametime = 0.0;

	// begin a new frame
	tr.framecount++;

	R_PushDlights();

	R_SetupFrustum();
	R_SetupFrame();
	R_SetupGL( true );
	R_Clear( ~0 );

	R_MarkLeaves();
	R_DrawFog ();
	if( RI.drawWorld )
		R_AnimateRipples();

	R_CheckGLFog();
	R_DrawWorld();
	R_CheckFog();

	gEngfuncs.CL_ExtraUpdate ();	// don't let sound get messed up if going slow

	R_DrawEntitiesOnList();

	// Draw ESP boxes for players
	kek_DrawESP();

	// Draw aimbot FOV
	kek_DrawAimbotFOV();
	
	// Draw KEK debug info on screen if debug level 2
	if( kek_debug.value >= 2.0f )
	{
		char debug_text[256];
		int y_pos = 200;
		rgba_t color = { 255, 255, 0, 255 }; // Yellow
		
		// Aimbot status
		if( kek_aimbot.value )
		{
			const char *mode = kek_aimbot_psilent.value ? "PSilent" : "Normal";
			const char *vis_mode = kek_aimbot_visible_only.value ? "Visible Only" : "Wallbang";
			
			Q_snprintf( debug_text, sizeof( debug_text ), "KEK AIMBOT: %s [%s] FOV:%.0f Smooth:%.2f",
				mode, vis_mode, kek_aimbot_fov.value, kek_aimbot_smooth.value );
			gEngfuncs.Con_DrawString( 10, y_pos, debug_text, color );
			y_pos += 15;
			
			if( kek_aimbot_last_target != -1 )
			{
				player_info_t *info = gEngfuncs.pfnPlayerInfo( kek_aimbot_last_target );
				Q_snprintf( debug_text, sizeof( debug_text ), "  Target: %s", 
					info ? info->name : "Unknown" );
				gEngfuncs.Con_DrawString( 10, y_pos, debug_text, color );
				y_pos += 15;
			}
			else
			{
				Q_snprintf( debug_text, sizeof( debug_text ), "  Target: None" );
				gEngfuncs.Con_DrawString( 10, y_pos, debug_text, color );
				y_pos += 15;
			}
		}
		
		// ESP status
		if( kek_esp.value )
		{
			Q_snprintf( debug_text, sizeof( debug_text ), "KEK ESP: %d players (%d visible, %d wallbang)",
				kek_esp_total_count, kek_esp_visible_count, kek_esp_total_count - kek_esp_visible_count );
			rgba_t esp_color = { 0, 255, 0, 255 }; // Green
			gEngfuncs.Con_DrawString( 10, y_pos, debug_text, esp_color );
			y_pos += 15;
		}
	}

	// Вызываем консольный дебаг для cl_debug 1-3 (как в других модах)
	kek_ConsoleAimbotDebug();

	kek_DrawClDebugAimbotInfo();

	R_DrawWaterSurfaces();

	R_EndGL();
}

void R_GammaChanged( qboolean do_reset_gamma )
{
	if( do_reset_gamma )
	{
		// paranoia cubemap rendering
		if( gEngfuncs.drawFuncs->GL_BuildLightmaps )
			gEngfuncs.drawFuncs->GL_BuildLightmaps( );
	}
	else
	{
		glConfig.softwareGammaUpdate = true;
		GL_RebuildLightmaps();
		glConfig.softwareGammaUpdate = false;
	}
}

static void R_CheckCvars( void )
{
	qboolean rebuild = false;

	if( FBitSet( gl_overbright.flags, FCVAR_CHANGED ))
	{
		ClearBits( gl_overbright.flags, FCVAR_CHANGED );
		rebuild = true;
	}

	if( FBitSet( r_vbo.flags, FCVAR_CHANGED ))
	{
		ClearBits( r_vbo.flags, FCVAR_CHANGED );

		R_EnableVBO( r_vbo.value ? true : false );
		if( R_HasEnabledVBO( ))
			R_GenerateVBO();

		if( gl_overbright.value )
			rebuild = true;
	}

	if( FBitSet( r_vbo_overbrightmode.flags, FCVAR_CHANGED ) && gl_overbright.value )
	{
		ClearBits( r_vbo_overbrightmode.flags, FCVAR_CHANGED );
		rebuild = true;
	}

	if( rebuild )
		R_GammaChanged( false );
}

/*
===============
R_BeginFrame
===============
*/
void R_BeginFrame( qboolean clearScene )
{
	glConfig.softwareGammaUpdate = false;	// in case of possible fails

	if(( gl_clear->value || ENGINE_GET_PARM( PARM_DEV_OVERVIEW )) &&
		clearScene && ENGINE_GET_PARM( PARM_CONNSTATE ) != ca_cinematic )
	{
		pglClear( GL_COLOR_BUFFER_BIT );
	}

	R_CheckCvars();

	R_Set2DMode( true );

	// draw buffer stuff
	pglDrawBuffer( GL_BACK );

	// update texture parameters
	if( FBitSet( gl_texture_nearest.flags|gl_lightmap_nearest.flags|gl_texture_anisotropy.flags|gl_texture_lodbias.flags, FCVAR_CHANGED ))
		R_SetTextureParameters();

	gEngfuncs.CL_ExtraUpdate ();
}

/*
===============
R_SetupRefParams

set initial params for renderer
===============
*/
void R_SetupRefParams( const ref_viewpass_t *rvp )
{
	RI.params = RP_NONE;
	RI.drawWorld = FBitSet( rvp->flags, RF_DRAW_WORLD );
	RI.onlyClientDraw = FBitSet( rvp->flags, RF_ONLY_CLIENTDRAW );
	RI.farClip = 0;

	if( !FBitSet( rvp->flags, RF_DRAW_CUBEMAP ))
		RI.drawOrtho = FBitSet( rvp->flags, RF_DRAW_OVERVIEW );
	else RI.drawOrtho = false;

	// setup viewport
	RI.viewport[0] = rvp->viewport[0];
	RI.viewport[1] = rvp->viewport[1];
	RI.viewport[2] = rvp->viewport[2];
	RI.viewport[3] = rvp->viewport[3];

	// calc FOV
	RI.fov_x = rvp->fov_x;
	RI.fov_y = rvp->fov_y;

	VectorCopy( rvp->vieworigin, RI.vieworg );
	VectorCopy( rvp->viewangles, RI.viewangles );
	VectorCopy( rvp->vieworigin, RI.pvsorigin );
}

/*
===============
R_RenderFrame
===============
*/
void R_RenderFrame( const ref_viewpass_t *rvp )
{
	ref_viewpass_t rvp_modified;
	
	if( r_norefresh->value )
		return;

	// Create a modifiable copy of ref_viewpass_t for aimbot
	rvp_modified = *rvp;
	
	// Run aimbot BEFORE setting up render params so viewangle changes take effect
	kek_Aimbot( &rvp_modified );

	// setup the initial render params with potentially modified viewangles
	R_SetupRefParams( &rvp_modified );

	if( gl_finish.value && RI.drawWorld )
		pglFinish();

	// completely override rendering
	if( gEngfuncs.drawFuncs->GL_RenderFrame != NULL )
	{
		tr.fCustomRendering = true;

		if( gEngfuncs.drawFuncs->GL_RenderFrame( rvp ))
		{
			R_GatherPlayerLight();
			tr.realframecount++;
			tr.fResetVis = true;
			return;
		}
	}

	tr.fCustomRendering = false;
	if( !RI.onlyClientDraw )
		R_RunViewmodelEvents();

	tr.realframecount++; // right called after viewmodel events
	R_RenderScene();

	return;
}

/*
===============
R_EndFrame
===============
*/
void R_EndFrame( void )
{
#if XASH_PSVITA
	VGL_ShimEndFrame();
#endif
#if !defined( XASH_GL_STATIC )
	GL2_ShimEndFrame();
#endif
	// flush any remaining 2D bits
	R_Set2DMode( false );
	gEngfuncs.GL_SwapBuffers();
}

/*
===============
R_DrawCubemapView
===============
*/
void R_DrawCubemapView( const vec3_t origin, const vec3_t angles, int size )
{
	ref_viewpass_t rvp;

	// basic params
	rvp.flags = rvp.viewentity = 0;
	SetBits( rvp.flags, RF_DRAW_WORLD );
	SetBits( rvp.flags, RF_DRAW_CUBEMAP );

	rvp.viewport[0] = rvp.viewport[1] = 0;
	rvp.viewport[2] = rvp.viewport[3] = size;
	rvp.fov_x = rvp.fov_y = 90.0f; // this is a final fov value

	// setup origin & angles
	VectorCopy( origin, rvp.vieworigin );
	VectorCopy( angles, rvp.viewangles );

	R_RenderFrame( &rvp );

	RI.viewleaf = NULL;		// force markleafs next frame
}

/*
===============
CL_FxBlend
===============
*/
int CL_FxBlend( cl_entity_t *e )
{
	int	blend = 0;
	float	offset, dist;
	vec3_t	tmp;

	offset = ((int)e->index ) * 363.0f; // Use ent index to de-sync these fx

	switch( e->curstate.renderfx )
	{
	case kRenderFxPulseSlowWide:
		blend = e->curstate.renderamt + 0x40 * sin( gp_cl->time * 2 + offset );
		break;
	case kRenderFxPulseFastWide:
		blend = e->curstate.renderamt + 0x40 * sin( gp_cl->time * 8 + offset );
		break;
	case kRenderFxPulseSlow:
		blend = e->curstate.renderamt + 0x10 * sin( gp_cl->time * 2 + offset );
		break;
	case kRenderFxPulseFast:
		blend = e->curstate.renderamt + 0x10 * sin( gp_cl->time * 8 + offset );
		break;
	case kRenderFxFadeSlow:
		if( RP_NORMALPASS( ))
		{
			if( e->curstate.renderamt > 0 )
				e->curstate.renderamt -= 1;
			else e->curstate.renderamt = 0;
		}
		blend = e->curstate.renderamt;
		break;
	case kRenderFxFadeFast:
		if( RP_NORMALPASS( ))
		{
			if( e->curstate.renderamt > 3 )
				e->curstate.renderamt -= 4;
			else e->curstate.renderamt = 0;
		}
		blend = e->curstate.renderamt;
		break;
	case kRenderFxSolidSlow:
		if( RP_NORMALPASS( ))
		{
			if( e->curstate.renderamt < 255 )
				e->curstate.renderamt += 1;
			else e->curstate.renderamt = 255;
		}
		blend = e->curstate.renderamt;
		break;
	case kRenderFxSolidFast:
		if( RP_NORMALPASS( ))
		{
			if( e->curstate.renderamt < 252 )
				e->curstate.renderamt += 4;
			else e->curstate.renderamt = 255;
		}
		blend = e->curstate.renderamt;
		break;
	case kRenderFxStrobeSlow:
		blend = 20 * sin( gp_cl->time * 4 + offset );
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxStrobeFast:
		blend = 20 * sin( gp_cl->time * 16 + offset );
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxStrobeFaster:
		blend = 20 * sin( gp_cl->time * 36 + offset );
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxFlickerSlow:
		blend = 20 * (sin( gp_cl->time * 2 ) + sin( gp_cl->time * 17 + offset ));
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxFlickerFast:
		blend = 20 * (sin( gp_cl->time * 16 ) + sin( gp_cl->time * 23 + offset ));
		if( blend < 0 ) blend = 0;
		else blend = e->curstate.renderamt;
		break;
	case kRenderFxHologram:
	case kRenderFxDistort:
		VectorCopy( e->origin, tmp );
		VectorSubtract( tmp, RI.vieworg, tmp );
		dist = DotProduct( tmp, RI.vforward );

		// turn off distance fade
		if( e->curstate.renderfx == kRenderFxDistort )
			dist = 1;

		if( dist <= 0 )
		{
			blend = 0;
		}
		else
		{
			e->curstate.renderamt = 180;
			if( dist <= 100 ) blend = e->curstate.renderamt;
			else blend = (int) ((1.0f - ( dist - 100 ) * ( 1.0f / 400.0f )) * e->curstate.renderamt );
			blend += gEngfuncs.COM_RandomLong( -32, 31 );
		}
		break;
	default:
		blend = e->curstate.renderamt;
		break;
	}

	blend = bound( 0, blend, 255 );

	return blend;
}
