// Include imgui headers FIRST to establish namespace before STL
#include "imgui.h"

// Include C++ standard library headers after imgui to avoid STL conflicts on Android
#if __ANDROID__
// Prevent old STL from being included by defining its guards
#define _STL_PAIR_H
#define _STL_UTILITY_H
#define _STL_CONFIG_H
#endif
#include <vector>

// Continue with other headers
#include "imgui_window_system.h"
#include "keydefs.h"

// Include engine headers after imgui to avoid conflicts
// const.h must be included first to define basic types (uint, word, byte, BIT, color24, colorVec, etc.)
// cvardef.h must be included before ref_api.h to define cvar_t
// vid_common.h must be included before common.h to define window_mode_e before platform.h uses it
// ref_api.h must be included before common.h to define enum types before platform.h uses them
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
#if __ANDROID__
    for (IImGuiWindow *window : *m_WindowList) {
        window->Initialize();
    }
#else
    for (IImGuiWindow *window : m_WindowList) {
        window->Initialize();
    }
#endif
}

void CImGuiWindowSystem::VidInitialize()
{
#if __ANDROID__
    for (IImGuiWindow *window : *m_WindowList) {
        window->VidInitialize();
    }
#else
    for (IImGuiWindow *window : m_WindowList) {
        window->VidInitialize();
    }
#endif
}

void CImGuiWindowSystem::Terminate()
{
#if __ANDROID__
    for (IImGuiWindow *window : *m_WindowList) {
        window->Terminate();
    }
#else
    for (IImGuiWindow *window : m_WindowList) {
        window->Terminate();
    }
#endif
}

void CImGuiWindowSystem::NewFrame()
{
#if __ANDROID__
    for (IImGuiWindow *window : *m_WindowList)
    {
        window->Think();
        if (window->Active()) {
            window->Draw();
        }
    }
#else
    for (IImGuiWindow *window : m_WindowList)
    {
        window->Think();
        if (window->Active()) {
            window->Draw();
        }
    }
#endif
}

void CImGuiWindowSystem::AddWindow(IImGuiWindow *window)
{
#if __ANDROID__
    if (!m_WindowList) {
        m_WindowList = new std::vector<IImGuiWindow*>();
    }
    m_WindowList->push_back(window);
#else
    m_WindowList.push_back(window);
#endif
}

CImGuiWindowSystem::CImGuiWindowSystem()
{
#if __ANDROID__
    m_WindowList = new std::vector<IImGuiWindow*>();
#else
    m_WindowList.clear();
#endif
}

CImGuiWindowSystem::~CImGuiWindowSystem()
{
#if __ANDROID__
    delete m_WindowList;
    m_WindowList = nullptr;
#endif
}

bool CImGuiWindowSystem::KeyInput(bool keyDown, int keyNumber, const char *bindName)
{
#if __ANDROID__
    if (keyDown && keyNumber == K_INS)
    {
        for (IImGuiWindow *window : *m_WindowList)
        {
            bool handled = window->HandleKey(keyDown, keyNumber, bindName);
            if (!handled) {
                return false;
            }
        }
        return true;
    }
    
    for (IImGuiWindow *window : *m_WindowList)
    {
        if (window->Active()) {
            bool handled = window->HandleKey(keyDown, keyNumber, bindName);
            if (!handled) {
                return false;
            }
        }
    }
#else
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
#endif
    return true;
}

bool CImGuiWindowSystem::CursorRequired()
{
#if __ANDROID__
    for (IImGuiWindow *window : *m_WindowList)
    {
        if (window->Active() && window->CursorRequired()) {
            return true;
        }
    }
#else
    for (IImGuiWindow *window : m_WindowList)
    {
        if (window->Active() && window->CursorRequired()) {
            return true;
        }
    }
#endif
    return false;
}

void CImGuiWindowSystem::LinkWindows()
{
    AddWindow(&g_DemoWindow);
    AddWindow(&g_CheatMenu);
}

