#pragma once
#include "imgui.h"

// Forward declarations
struct convar_s;
typedef struct convar_s convar_t;

class CImGuiBackend
{
    convar_t *ui_imgui_scale;

public:
    bool Init();
    void Shutdown();
    void NewFrame();
    void RenderDrawData(ImDrawData *draw_data);
};

// Functions are defined in backends/imgui_impl_opengl2.cpp

