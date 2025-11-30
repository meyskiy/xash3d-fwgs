#pragma once
#include "imgui_window.h"

// Forward declaration to avoid including <vector> in header on Android
// This prevents STL conflicts between old STL and new libc++
#if __ANDROID__
// Prevent old STL from being included
#define _STL_PAIR_H
#define _STL_UTILITY_H
#define _STL_CONFIG_H
namespace std {
    template<typename T, typename Alloc = void>
    class vector;
}
#else
#include <vector>
#endif

class CImGuiWindowSystem
{
    friend class CImGuiManager;
public:
    void AddWindow(IImGuiWindow *window);

private:
    CImGuiWindowSystem();
    ~CImGuiWindowSystem();
    CImGuiWindowSystem(const CImGuiWindowSystem &) = delete;
    CImGuiWindowSystem &operator=(const CImGuiWindowSystem &) = delete;

    void LinkWindows();
    void Initialize();
    void VidInitialize();
    void Terminate();
    void NewFrame();
    bool KeyInput(bool keyDown, int keyNumber, const char *bindName);
    bool CursorRequired();

#if __ANDROID__
    std::vector<IImGuiWindow*> *m_WindowList;
#else
    std::vector<IImGuiWindow*> m_WindowList;
#endif
};

