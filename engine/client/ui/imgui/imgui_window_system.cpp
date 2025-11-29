// Include C++ standard library headers FIRST, before any engine headers
// This prevents conflicts between old STL and new libc++ on Android
#if __ANDROID__
// On Android, prevent old STL from being included by defining its guards BEFORE any includes
// The old STL is in sources/cxx-stl/system/include and conflicts with new libc++
#define _STL_PAIR_H
#define _STL_UTILITY_H
#define _STL_CONFIG_H
// Explicitly include new libc++ headers first to establish the namespace
#include <__config>
#include <__utility/pair.h>
// Now include vector which should use the new libc++ pair we just included
#include <vector>
#else
#include <vector>
#endif

// Include imgui headers
#include "imgui.h"
#include "imgui_window_system.h"
#include "keydefs.h"

// Include engine headers after imgui to avoid conflicts
// const.h must be included first to define basic types (uint, word, byte, BIT, color24, colorVec, etc.)
// cvardef.h must be included before ref_api.h to define cvar_t
// vid_common.h must be included before common.h to define window_mode_e before platform.h uses it
// ref_api.h must be included before common.h to define enum types before platform.h uses theme
extern "C" {
#include "const.h"
#include "cvardef.h"
#include "vid_common.h"
#include "ref_api.h"
#include "common.h"
}

#include "ui_demo_window.h"
#include "ui_cheat_menu.h"

static CImGuiDemoWindow g_DemoWindow;
static CImGuiCheatMenu g_CheatMenu;

void CImGuiWindowSystem::Initialize()
{
    LinkWindows();
    for (IImGuiWindow *window : m_WindowList) {
        window->Initialize();
    }
}

void CImGuiWindowSystem::VidInitialize()
{
    for (IImGuiWindow *window : m_WindowList) {
        window->VidInitialize();
    }
}

void CImGuiWindowSystem::Terminate()
{
    for (IImGuiWindow *window : m_WindowList) {
        window->Terminate();
    }
}

void CImGuiWindowSystem::NewFrame()
{
    for (IImGuiWindow *window : m_WindowList)
    {
        window->Think();
        if (window->Active()) {
            window->Draw();
        }
    }
}

void CImGuiWindowSystem::AddWindow(IImGuiWindow *window)
{
    m_WindowList.push_back(window);
}

CImGuiWindowSystem::CImGuiWindowSystem()
{
    m_WindowList.clear();
}

bool CImGuiWindowSystem::KeyInput(bool keyDown, int keyNumber, const char *bindName)
{
    if (keyDown && keyNumber == K_INS)
    {
        for (IImGuiWindow *window : m_WindowList)
        {
            bool handled = window->HandleKey(keyDown, keyNumber, bindName);
            if (!handled) {
                return false;
            }
        }
        return true;
    }
    
    for (IImGuiWindow *window : m_WindowList)
    {
        if (window->Active()) {
            bool handled = window->HandleKey(keyDown, keyNumber, bindName);
            if (!handled) {
                return false;
            }
        }
    }
    
    return true;
}

bool CImGuiWindowSystem::CursorRequired()
{
    for (IImGuiWindow *window : m_WindowList)
    {
        if (window->Active() && window->CursorRequired()) {
            return true;
        }
    }
    return false;
}

void CImGuiWindowSystem::LinkWindows()
{
    AddWindow(&g_DemoWindow);
    AddWindow(&g_CheatMenu);
}

