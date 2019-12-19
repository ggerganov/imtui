/*! \file imtui-impl-ncurses.h
 *  \brief Enter description here.
 */

#pragma once

namespace ImTui {
struct TScreen;
}

bool ImTui_ImplNcurses_Init(bool mouseSupport);
void ImTui_ImplNcurses_Shutdown();
void ImTui_ImplNcurses_NewFrame();
void ImTui_ImplNcurses_DrawScreen(const ImTui::TScreen & screen);
bool ImTui_ImplNcurses_ProcessEvent();
