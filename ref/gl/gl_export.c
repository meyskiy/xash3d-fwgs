/*
gl_export.c - OpenGL function pointer definitions for Android
This file defines the OpenGL function pointers that are declared as extern in gl_export.h
*/

#if !XASH_GL_STATIC && __ANDROID__

// Include necessary headers first
#include "const.h"
#include "cvardef.h"

// Now we need to include gl_export.h but with APIENTRY_LINKAGE redefined
// The problem is that gl_export.h uses APIENTRY_LINKAGE which is defined as 'extern'
// We need to redefine it to empty string to get definitions instead of declarations

// First, undefine APIENTRY_LINKAGE if it was defined
#ifdef APIENTRY_LINKAGE
#undef APIENTRY_LINKAGE
#endif

// Define it as empty (this will make the declarations become definitions)
#define APIENTRY_LINKAGE

// Now include gl_export.h - but we need to prevent the guard from blocking
// Since gl_export.h has #ifndef GL_EXPORT_H guard, we need to include it differently
// Actually, we can just include it normally since we're in a different compilation unit

// But wait - gl_export.h might have already been included by gl_local.h in other files
// We need to make sure we're getting fresh declarations

// The best approach: include gl_export.h directly, and since APIENTRY_LINKAGE is now empty,
// all the declarations will become definitions
#include "gl_export.h"

#endif
