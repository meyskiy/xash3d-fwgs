#include "imgui_integration.h"

#if __ANDROID__
// Prevent old STL from being included by defining its guards BEFORE any includes
#define _STL_PAIR_H
#define _STL_UTILITY_H
#define _STL_CONFIG_H
#endif

#include "imgui_manager.h"

extern CImGuiManager &g_ImGuiManager;

void ImGui_Init(void)
{
    g_ImGuiManager.Initialize();
}

void ImGui_VidInit(void)
{
    g_ImGuiManager.VidInitialize();
}

void ImGui_Shutdown(void)
{
    g_ImGuiManager.Terminate();
}

void ImGui_NewFrame(void)
{
    g_ImGuiManager.NewFrame();
}

int ImGui_KeyInput(int keyDown, int keyNumber, const char *bindName)
{
    return g_ImGuiManager.KeyInput(keyDown != 0, keyNumber, bindName) ? 1 : 0;
}

