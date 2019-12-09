/*! \file imtui-impl-text.h
 *  \brief Enter description here.
 */

#pragma once

namespace ImTui {
struct TScreen;
}

bool ImTui_ImplText_Init();
void ImTui_ImplText_Shutdown();
void ImTui_ImplText_NewFrame();
void ImTui_ImplText_RenderDrawData(ImDrawData* draw_data, ImTui::TScreen & screen);
