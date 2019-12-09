/*! \file imtui-impl-ncurses.h
 *  \brief Enter description here.
 */

#pragma once

namespace ImTui {
struct TScreen;
}

IMGUI_IMPL_API bool     ImTui_ImplNcurses_Init();
IMGUI_IMPL_API void     ImTui_ImplNcurses_Shutdown();
IMGUI_IMPL_API void     ImTui_ImplNcurses_NewFrame(ImTui::TScreen & screen);
IMGUI_IMPL_API void     ImTui_ImplNcurses_DrawScreen(ImTui::TScreen & screen);
IMGUI_IMPL_API bool     ImTui_ImplNcurses_ProcessEvent();
