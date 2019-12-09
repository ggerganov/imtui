/*! \file imtui-impl-text.h
 *  \brief Enter description here.
 */

#pragma once

namespace ImTui {
struct TScreen;
}

// Backend API
IMGUI_IMPL_API bool     ImTui_ImplText_Init();
IMGUI_IMPL_API void     ImTui_ImplText_Shutdown();
IMGUI_IMPL_API void     ImTui_ImplText_NewFrame();
IMGUI_IMPL_API void     ImTui_ImplText_RenderDrawData(ImDrawData* draw_data, ImTui::TScreen & screen);
