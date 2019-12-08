/*! \file imtui_demo.h
 *  \brief Enter description here.
 */

#pragma once

#include "imgui/imgui.h"

namespace ImTui {

void ShowAboutWindow(bool*);
void ShowDemoWindow(bool*);
void ShowUserGuide();
void ShowStyleEditor(ImGuiStyle* style = nullptr);

}
