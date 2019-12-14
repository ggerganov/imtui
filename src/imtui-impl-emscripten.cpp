/*! \file imtui-impl-emscripten.cpp
 *  \brief Enter description here.
 */

#include "imtui/imtui.h"
#include "imtui/imtui-impl-emscripten.h"
#include "imtui/imtui-impl-text.h"

#include <emscripten.h>

#include <string>

// client input
static bool ignoreMouse = false;
static ImVec2 lastMousePos = { 0.0, 0.0 };
static bool  lastMouseDown[5] = { false, false, false, false, false };
static float lastMouseWheel = 0.0;
static float lastMouseWheelH = 0.0;

static std::string lastAddText = "";
static bool lastKeysDown[512];

bool ImTui_ImplEmscripten_Init(bool mouseSupport) {
    ImGui::GetIO().KeyMap[ImGuiKey_Tab]         = 9;
    ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow]   = 37;
    ImGui::GetIO().KeyMap[ImGuiKey_RightArrow]  = 39;
    ImGui::GetIO().KeyMap[ImGuiKey_UpArrow]     = 38;
    ImGui::GetIO().KeyMap[ImGuiKey_DownArrow]   = 40;
    ImGui::GetIO().KeyMap[ImGuiKey_PageUp]      = 33;
    ImGui::GetIO().KeyMap[ImGuiKey_PageDown]    = 34;
    ImGui::GetIO().KeyMap[ImGuiKey_Home]        = 36;
    ImGui::GetIO().KeyMap[ImGuiKey_End]         = 35;
    ImGui::GetIO().KeyMap[ImGuiKey_Insert]      = 45;
    ImGui::GetIO().KeyMap[ImGuiKey_Delete]      = 46;
    ImGui::GetIO().KeyMap[ImGuiKey_Backspace]   = 8;
    ImGui::GetIO().KeyMap[ImGuiKey_Space]       = 32;
    ImGui::GetIO().KeyMap[ImGuiKey_Enter]       = 13;
    ImGui::GetIO().KeyMap[ImGuiKey_Escape]      = 27;
    ImGui::GetIO().KeyMap[ImGuiKey_KeyPadEnter] = 13;
    ImGui::GetIO().KeyMap[ImGuiKey_A]           = 1;
    ImGui::GetIO().KeyMap[ImGuiKey_C]           = 2;
    ImGui::GetIO().KeyMap[ImGuiKey_V]           = 2;
    ImGui::GetIO().KeyMap[ImGuiKey_X]           = 3;
    ImGui::GetIO().KeyMap[ImGuiKey_Y]           = 4;
    ImGui::GetIO().KeyMap[ImGuiKey_Z]           = 5;

    ignoreMouse = !mouseSupport;

    return true;
}

void ImTui_ImplEmscripten_Shutdown() {
}

void ImTui_ImplEmscripten_NewFrame(ImTui::TScreen & screen) {
    ImGui::GetIO().MousePos = lastMousePos;
    ImGui::GetIO().MouseWheelH = lastMouseWheelH;
    ImGui::GetIO().MouseWheel = lastMouseWheel;
    ImGui::GetIO().MouseDown[0] = lastMouseDown[0];
    ImGui::GetIO().MouseDown[1] = lastMouseDown[1];
    ImGui::GetIO().MouseDown[2] = lastMouseDown[2];
    ImGui::GetIO().MouseDown[3] = lastMouseDown[3];
    ImGui::GetIO().MouseDown[4] = lastMouseDown[4];

    if (lastAddText.size() > 0) {
        ImGui::GetIO().AddInputCharactersUTF8(lastAddText.c_str());
    }

    for (int i = 0; i < 512; ++i) {
        ImGui::GetIO().KeysDown[i] = lastKeysDown[i];
    }

    lastMouseWheelH = 0.0;
    lastMouseWheel = 0.0;
    lastAddText = "";

    for (auto & row : screen.data) {
        for (auto & cell : row) {
            cell.c = ' ';
            cell.f = 0;
            cell.b = 0;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void set_mouse_pos(float x, float y) {
    if (ignoreMouse) return;

    lastMousePos.x = x;
    lastMousePos.y = y;
}

EMSCRIPTEN_KEEPALIVE
void set_mouse_down(int but, float x, float y) {
    if (ignoreMouse) return;

    lastMouseDown[but] = true;
    lastMousePos.x = x;
    lastMousePos.y = y;
}

EMSCRIPTEN_KEEPALIVE
void set_mouse_up(int but, float x, float y) {
    if (ignoreMouse) return;

    lastMouseDown[but] = false;
    lastMousePos.x = x;
    lastMousePos.y = y;
}

EMSCRIPTEN_KEEPALIVE
void set_mouse_wheel(float x, float y) {
    if (ignoreMouse) return;

    lastMouseWheelH = x;
    lastMouseWheel  = y;
}

EMSCRIPTEN_KEEPALIVE
void set_key_down(int key) {
    lastAddText.resize(1);
    if (ImGui::GetIO().KeyShift == false && key >= 'A' && key <= 'Z') {
        key += 'a' - 'A';
    }

    if (lastKeysDown[17]) {
        (key == 65) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_A]] = true);
        (key == 67) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_C]] = true);
        (key == 86) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_V]] = true);
        (key == 88) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_X]] = true);
        (key == 89) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Y]] = true);
        (key == 90) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Z]] = true);
    } else {
        lastAddText[0] = key;
    }

    lastKeysDown[key] = true;

    if (key == 16) {
        ImGui::GetIO().KeyShift = true;
    }

    if (key == 17) {
        ImGui::GetIO().KeyCtrl = true;
    }

    if (key == 18) {
        ImGui::GetIO().KeyAlt = true;
    }
}

EMSCRIPTEN_KEEPALIVE
void set_key_up(int key) {
    if (ImGui::GetIO().KeyShift == false && key >= 'A' && key <= 'Z') {
        key += 'a' - 'A';
    }

    lastKeysDown[key] = false;

    if (key == 16) {
        ImGui::GetIO().KeyShift = false;
    }

    if (key == 17) {
        ImGui::GetIO().KeyCtrl = false;
        lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_A]] = false;
        lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_C]] = false;
        lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_V]] = false;
        lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_X]] = false;
        lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Y]] = false;
        lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Z]] = false;
    }

    if (key == 18) {
        ImGui::GetIO().KeyAlt = false;
    }
}

EMSCRIPTEN_KEEPALIVE
void set_key_press(int key) {
    if (ImGui::GetIO().KeyShift == false && key >= 'A' && key <= 'Z') {
        key += 'a' - 'A';
    }

    if (key > 0) {
        lastAddText.resize(1);
        lastAddText[0] = key;
    }
}

