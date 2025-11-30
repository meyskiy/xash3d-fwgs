#include "ui_cheat_menu.h"
#include "imgui.h"
#include "keydefs.h"

// Include engine headers after imgui to avoid conflicts
extern "C" {
#include "const.h"
#include "cvardef.h"
#include "vid_common.h"
#include "ref_api.h"
#include "common.h"
}

bool CImGuiCheatMenu::m_ShowWindow = false;

static void CmdShowCheatMenu_f(void)
{
    CImGuiCheatMenu::CmdShowCheatMenu();
}

void CImGuiCheatMenu::Initialize()
{
    Cmd_AddCommand("kek_menu", CmdShowCheatMenu_f, "Show eBash3D Recode cheat menu");
}

void CImGuiCheatMenu::VidInitialize()
{
}

void CImGuiCheatMenu::Terminate()
{
}

void CImGuiCheatMenu::Think()
{
}

void CImGuiCheatMenu::Draw()
{
    if (!m_ShowWindow)
        return;
    
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("eBash3D Recode", &m_ShowWindow, ImGuiWindowFlags_None))
    {
        // Tab bar
        if (ImGui::BeginTabBar("CheatMenuTabs"))
        {
            if (ImGui::BeginTabItem("Aimbot"))
            {
                DrawAimbotTab();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Visual"))
            {
                DrawVisualTab();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Antiaim"))
            {
                DrawAntiaimTab();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Misc"))
            {
                DrawMiscTab();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

bool CImGuiCheatMenu::Active()
{
    return m_ShowWindow;
}

bool CImGuiCheatMenu::CursorRequired()
{
    return true;
}

bool CImGuiCheatMenu::HandleKey(bool keyDown, int keyNumber, const char *bindName)
{
    if (keyNumber == K_ESCAPE && keyDown)
    {
        m_ShowWindow = false;
        return true;
    }
    return false;
}

void CImGuiCheatMenu::CmdShowCheatMenu()
{
    m_ShowWindow = !m_ShowWindow;
}

void CImGuiCheatMenu::DrawCheckbox(const char* label, convar_t* cvar)
{
    if (!cvar) return;
    
    bool value = cvar->value != 0.0f;
    if (ImGui::Checkbox(label, &value))
    {
        Cvar_SetValue(cvar->name, value ? 1.0f : 0.0f);
    }
    if (ImGui::IsItemHovered() && cvar->desc)
    {
        ImGui::SetTooltip("%s", cvar->desc);
    }
}

void CImGuiCheatMenu::DrawSliderFloat(const char* label, convar_t* cvar, float min, float max, const char* format)
{
    if (!cvar) return;
    
    float value = cvar->value;
    if (ImGui::SliderFloat(label, &value, min, max, format))
    {
        Cvar_SetValue(cvar->name, value);
    }
    if (ImGui::IsItemHovered() && cvar->desc)
    {
        ImGui::SetTooltip("%s", cvar->desc);
    }
}

void CImGuiCheatMenu::DrawSliderInt(const char* label, convar_t* cvar, int min, int max, const char* format)
{
    if (!cvar) return;
    
    int value = (int)cvar->value;
    if (ImGui::SliderInt(label, &value, min, max, format))
    {
        Cvar_SetValue(cvar->name, (float)value);
    }
    if (ImGui::IsItemHovered() && cvar->desc)
    {
        ImGui::SetTooltip("%s", cvar->desc);
    }
}

void CImGuiCheatMenu::DrawColorPicker3(const char* label, convar_t* cvar_r, convar_t* cvar_g, convar_t* cvar_b)
{
    if (!cvar_r || !cvar_g || !cvar_b) return;
    
    float color[3] = { cvar_r->value / 255.0f, cvar_g->value / 255.0f, cvar_b->value / 255.0f };
    if (ImGui::ColorEdit3(label, color))
    {
        Cvar_SetValue(cvar_r->name, color[0] * 255.0f);
        Cvar_SetValue(cvar_g->name, color[1] * 255.0f);
        Cvar_SetValue(cvar_b->name, color[2] * 255.0f);
    }
}

void CImGuiCheatMenu::DrawAimbotTab()
{
    ImGui::Text("Aimbot Settings");
    ImGui::Separator();
    
    convar_t* cvar;
    
    cvar = Cvar_FindVar("kek_aimbot");
    DrawCheckbox("Enable Aimbot", cvar);
    
    cvar = Cvar_FindVar("kek_aimbot_fov");
    DrawSliderFloat("FOV", cvar, 0.0f, 360.0f, "%.0f");
    
    cvar = Cvar_FindVar("kek_aimbot_smooth");
    DrawSliderFloat("Smooth", cvar, 0.0f, 1.0f, "%.2f");
    
    cvar = Cvar_FindVar("kek_aimbot_visible_only");
    DrawCheckbox("Visible Only", cvar);
    
    cvar = Cvar_FindVar("kek_aimbot_draw_fov");
    DrawCheckbox("Draw FOV", cvar);
    
    cvar = Cvar_FindVar("kek_aimbot_max_distance");
    DrawSliderFloat("Max Distance", cvar, 0.0f, 10000.0f, "%.0f");
    
    cvar = Cvar_FindVar("kek_aimbot_psilent");
    DrawCheckbox("Perfect Silent", cvar);
    
    cvar = Cvar_FindVar("kek_aimbot_dm");
    DrawCheckbox("Deathmatch Mode", cvar);
    
    ImGui::Separator();
    ImGui::Text("Target Offset");
    
    cvar = Cvar_FindVar("kek_aimbot_x");
    DrawSliderFloat("X Offset", cvar, -50.0f, 50.0f, "%.1f");
    
    cvar = Cvar_FindVar("kek_aimbot_y");
    DrawSliderFloat("Y Offset", cvar, -50.0f, 50.0f, "%.1f");
    
    cvar = Cvar_FindVar("kek_aimbot_z");
    DrawSliderFloat("Z Offset (Height)", cvar, -50.0f, 50.0f, "%.1f");
    
    ImGui::Separator();
    ImGui::Text("FOV Indicator Color");
    
    cvar = Cvar_FindVar("kek_aimbot_fov_r");
    convar_t* cvar_g = Cvar_FindVar("kek_aimbot_fov_g");
    convar_t* cvar_b = Cvar_FindVar("kek_aimbot_fov_b");
    DrawColorPicker3("FOV Color", cvar, cvar_g, cvar_b);
}

void CImGuiCheatMenu::DrawVisualTab()
{
    ImGui::Text("Visual Settings");
    ImGui::Separator();
    
    convar_t* cvar;
    
    ImGui::Text("ESP Settings");
    cvar = Cvar_FindVar("kek_esp");
    DrawCheckbox("Enable ESP", cvar);
    
    cvar = Cvar_FindVar("kek_esp_box");
    DrawCheckbox("Draw Boxes", cvar);
    
    cvar = Cvar_FindVar("kek_esp_name");
    DrawCheckbox("Draw Names", cvar);
    
    cvar = Cvar_FindVar("kek_esp_weapon");
    DrawCheckbox("Draw Weapons", cvar);
    
    ImGui::Separator();
    ImGui::Text("ESP Colors - Behind Walls");
    
    cvar = Cvar_FindVar("kek_esp_r");
    convar_t* cvar_g = Cvar_FindVar("kek_esp_g");
    convar_t* cvar_b = Cvar_FindVar("kek_esp_b");
    DrawColorPicker3("ESP Color (Behind Walls)", cvar, cvar_g, cvar_b);
    
    ImGui::Separator();
    ImGui::Text("ESP Colors - Visible");
    
    cvar = Cvar_FindVar("kek_esp_visible_r");
    cvar_g = Cvar_FindVar("kek_esp_visible_g");
    cvar_b = Cvar_FindVar("kek_esp_visible_b");
    DrawColorPicker3("ESP Color (Visible)", cvar, cvar_g, cvar_b);
    
    ImGui::Separator();
    ImGui::Text("ESP Name Color");
    
    cvar = Cvar_FindVar("kek_esp_name_r");
    cvar_g = Cvar_FindVar("kek_esp_name_g");
    cvar_b = Cvar_FindVar("kek_esp_name_b");
    DrawColorPicker3("Name Color", cvar, cvar_g, cvar_b);
    
    ImGui::Separator();
    ImGui::Text("ESP Weapon Color");
    
    cvar = Cvar_FindVar("kek_esp_weapon_r");
    cvar_g = Cvar_FindVar("kek_esp_weapon_g");
    cvar_b = Cvar_FindVar("kek_esp_weapon_b");
    DrawColorPicker3("Weapon Color", cvar, cvar_g, cvar_b);
    
    ImGui::Separator();
    ImGui::Text("Viewmodel Settings");
    
    cvar = Cvar_FindVar("kek_custom_fov");
    DrawCheckbox("Custom Viewmodel FOV", cvar);
    
    cvar = Cvar_FindVar("kek_custom_fov_value");
    DrawSliderFloat("FOV Value", cvar, 0.0f, 50.0f, "%.1f");
    
    ImGui::Separator();
    ImGui::Text("Viewmodel Glow");
    
    cvar = Cvar_FindVar("kek_viewmodel_glow");
    DrawCheckbox("Enable Glow", cvar);
    
    cvar = Cvar_FindVar("kek_viewmodel_glow_r");
    cvar_g = Cvar_FindVar("kek_viewmodel_glow_g");
    cvar_b = Cvar_FindVar("kek_viewmodel_glow_b");
    DrawColorPicker3("Glow Color", cvar, cvar_g, cvar_b);
    
    cvar = Cvar_FindVar("kek_viewmodel_glow_alpha");
    DrawSliderInt("Glow Intensity", cvar, 0, 255, "%d");
    
    ImGui::Separator();
    ImGui::Text("Other Visual");
    
    cvar = Cvar_FindVar("ebash3d_wallhack");
    DrawCheckbox("Wallhack", cvar);
}

void CImGuiCheatMenu::DrawAntiaimTab()
{
    ImGui::Text("Antiaim Settings");
    ImGui::Separator();
    
    convar_t* cvar;
    
    cvar = Cvar_FindVar("kek_antiaim");
    DrawCheckbox("Enable Antiaim", cvar);
    
    cvar = Cvar_FindVar("kek_antiaim_mode");
    DrawSliderInt("Mode", cvar, 0, 7, "%d");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("0=jitter, 1=spin, 2=fake, 3=backwards, 4=sideways, 5=legit, 6=fast spin, 7=random");
    }
    
    cvar = Cvar_FindVar("kek_antiaim_speed");
    DrawSliderFloat("Rotation Speed", cvar, 0.0f, 200.0f, "%.1f");
    
    cvar = Cvar_FindVar("kek_antiaim_jitter_range");
    DrawSliderFloat("Jitter Range", cvar, 0.0f, 180.0f, "%.1f");
    
    cvar = Cvar_FindVar("kek_antiaim_fake_angle");
    DrawSliderFloat("Fake Angle Offset", cvar, -180.0f, 180.0f, "%.1f");
}

void CImGuiCheatMenu::DrawMiscTab()
{
    ImGui::Text("Miscellaneous Settings");
    ImGui::Separator();
    
    convar_t* cvar;
    
    ImGui::Text("Fakelag");
    cvar = Cvar_FindVar("kek_fakelag");
    DrawCheckbox("Enable Fakelag", cvar);
    
    cvar = Cvar_FindVar("kek_fakelag_delay");
    DrawSliderInt("Fakelag Delay (ms)", cvar, 0, 1000, "%d");
    
    ImGui::Separator();
    ImGui::Text("eBash3D Features");
    
    cvar = Cvar_FindVar("ebash3d_cmd_block");
    DrawCheckbox("Block Server Commands", cvar);
    
    cvar = Cvar_FindVar("ebash3d_speed_multiplier");
    DrawSliderFloat("Speed Multiplier", cvar, 0.1f, 10.0f, "%.2f");
    
    cvar = Cvar_FindVar("ebash3d_auto_strafe");
    DrawCheckbox("Auto Strafe", cvar);
    
    cvar = Cvar_FindVar("ebash3d_ground_strafe");
    DrawCheckbox("Ground Strafe", cvar);
    
    cvar = Cvar_FindVar("ebash3d_fakelag");
    DrawSliderInt("eBash3D Fakelag (ms)", cvar, 0, 1000, "%d");
    
    ImGui::Separator();
    ImGui::Text("Debug");
    
    cvar = Cvar_FindVar("kek_debug");
    DrawSliderInt("Debug Level", cvar, 0, 2, "%d");
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("0 = off, 1 = console, 2 = screen");
    }
}

