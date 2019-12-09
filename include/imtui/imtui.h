/*! \file imtui.h
 *  \brief Enter description here.
 */

#pragma once

// simply expose the existing Dear ImGui API
#include "imgui/imgui.h"

#include "imtui/imtui-impl-text.h"

#include <vector>

namespace ImTui {

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

}
