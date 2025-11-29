#pragma once
#include "imgui_window.h"

// Forward declaration
struct convar_s;
typedef struct convar_s convar_t;

class CImGuiCheatMenu : public IImGuiWindow
{
public:
    void Initialize();
    void VidInitialize();
    void Terminate();
    void Think();
    void Draw();
    bool Active();
    bool CursorRequired();
    bool HandleKey(bool keyDown, int keyNumber, const char *bindName);
    static void CmdShowCheatMenu();

private:
    static bool m_ShowWindow;
    
    // Helper functions for drawing controls
    void DrawAimbotTab();
    void DrawVisualTab();
    void DrawAntiaimTab();
    void DrawMiscTab();
    
    // Helper to draw checkbox
    void DrawCheckbox(const char* label, convar_t* cvar);
    
    // Helper to draw slider for float
    void DrawSliderFloat(const char* label, convar_t* cvar, float min, float max, const char* format = "%.1f");
    
    // Helper to draw slider for int
    void DrawSliderInt(const char* label, convar_t* cvar, int min, int max, const char* format = "%d");
    
    // Helper to draw color picker (RGB)
    void DrawColorPicker3(const char* label, convar_t* cvar_r, convar_t* cvar_g, convar_t* cvar_b);
};

