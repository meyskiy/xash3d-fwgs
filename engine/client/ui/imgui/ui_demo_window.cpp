#include "ui_demo_window.h"
#include "imgui.h"
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

bool CImGuiDemoWindow::m_ShowWindow = false;

static void CmdShowDemoWindow_f(void)
{
    CImGuiDemoWindow::CmdShowDemoWindow();
}

void CImGuiDemoWindow::Initialize()
{
    Cmd_AddCommand("ui_imgui_demo", CmdShowDemoWindow_f, "Show ImGui demo window");
}

void CImGuiDemoWindow::VidInitialize()
{
}

void CImGuiDemoWindow::Terminate()
{
}

void CImGuiDemoWindow::Think()
{
}

void CImGuiDemoWindow::Draw()
{
    if (m_ShowWindow) {
        ImGui::ShowDemoWindow(&m_ShowWindow);
    }
}

bool CImGuiDemoWindow::Active()
{
    return m_ShowWindow;
}

bool CImGuiDemoWindow::CursorRequired()
{
    return true;
}

bool CImGuiDemoWindow::HandleKey(bool keyDown, int keyNumber, const char *bindName)
{
    if (keyNumber == K_ESCAPE)
    {
        m_ShowWindow = false;
    }
    return false;
}

void CImGuiDemoWindow::CmdShowDemoWindow()
{
    m_ShowWindow = true;
}

