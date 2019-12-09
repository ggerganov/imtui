/*! \file imtui-demo.h
 *  \brief Enter description here.
 */

#pragma once

#include "imtui/imtui.h"

namespace ImTui {

void ShowAboutWindow(bool*);
void ShowDemoWindow(bool*);
void ShowUserGuide();
void ShowStyleEditor(ImGuiStyle* style = nullptr);

}
