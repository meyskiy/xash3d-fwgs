#pragma once
// Include C++ standard library headers FIRST to avoid STL conflicts on Android
// On Android NDK, we need to ensure we use the new libc++ and not the old STL
#if __ANDROID__
// Prevent inclusion of old STL headers that conflict with new libc++
#define _LIBCPP_VERSION 1
#endif
#include <vector>

#include "imgui_window.h"

class CImGuiWindowSystem
{
    friend class CImGuiManager;
public:
    void AddWindow(IImGuiWindow *window);

private:
    CImGuiWindowSystem();
    ~CImGuiWindowSystem() {};
    CImGuiWindowSystem(const CImGuiWindowSystem &) = delete;
    CImGuiWindowSystem &operator=(const CImGuiWindowSystem &) = delete;

    void LinkWindows();
    void Initialize();
    void VidInitialize();
    void Terminate();
    void NewFrame();
    bool KeyInput(bool keyDown, int keyNumber, const char *bindName);
    bool CursorRequired();

    std::vector<IImGuiWindow*> m_WindowList;
};

