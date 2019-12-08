/*! \file imgui_impl_text.h
 *  \brief Enter description here.
 */

#pragma once

#include <vector>

using TChar = unsigned char;
using TColor = unsigned char;

struct TCell {
    TChar c;
    TColor f = 0;
    TColor b = 0;
};

struct TScreen {
    std::vector<std::vector<TCell>> data;
};

// Backend API
IMGUI_IMPL_API bool     ImGui_ImplText_Init();
IMGUI_IMPL_API void     ImGui_ImplText_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplText_NewFrame();
IMGUI_IMPL_API void     ImGui_ImplText_RenderDrawData(ImDrawData* draw_data, TScreen & screen);
