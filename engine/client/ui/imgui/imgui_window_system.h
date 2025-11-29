#pragma once
#include "imgui_window.h"

// On Android, we need to be careful about STL includes to avoid conflicts
// between old STL (sources/cxx-stl/system/include) and new libc++
#if __ANDROID__
// Prevent old STL headers from being included by defining their guards
// This must be done BEFORE any standard library includes
#define _STL_PAIR_H
#define _STL_UTILITY_H
#define _STL_CONFIG_H
// Use namespace alias to explicitly use new libc+++
namespace std_ndk1 = std::__ndk1;
#endif
#include <vector>

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
