#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ImGui_Init(void);
void ImGui_VidInit(void);
void ImGui_Shutdown(void);
void ImGui_NewFrame(void);
int ImGui_KeyInput(int keyDown, int keyNumber, const char *bindName);

#ifdef __cplusplus
}
#endif

