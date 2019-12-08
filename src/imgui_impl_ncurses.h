/*! \file imgui_impl_ncurses.h
 *  \brief Enter description here.
 */

#pragma once

struct TScreen;

IMGUI_IMPL_API bool     ImGui_ImplNcurses_Init();
IMGUI_IMPL_API void     ImGui_ImplNcurses_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplNcurses_NewFrame(TScreen & screen);
IMGUI_IMPL_API void     ImGui_ImplNcurses_DrawScreen(TScreen & screen);
IMGUI_IMPL_API bool     ImGui_ImplNcurses_ProcessEvent();
