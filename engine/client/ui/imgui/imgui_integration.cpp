#include "imgui_integration.h"
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

