#pragma once
#include "imgui_window.h"

#if __ANDROID__
// Prevent old STL from being included by defining its guards BEFORE any includes
#define _STL_PAIR_H
#define _STL_UTILITY_H
#define _STL_CONFIG_H
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

