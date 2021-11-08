/*! \file imtui-impl-emscripten.cpp
 *  \brief Enter description here.
 */

#include "imtui/imtui.h"
#include "imtui/imtui-impl-emscripten.h"
#include "imtui/imtui-impl-text.h"

#include <emscripten.h>

// client input
static bool ignoreMouse = false;
static ImVec2 lastMousePos = { 0.0, 0.0 };
static bool  lastMouseDown[5] = { false, false, false, false, false };
static float lastMouseWheel = 0.0;
static float lastMouseWheelH = 0.0;

static ImTui::TScreen * g_screen = nullptr;
static char lastAddText[8];
static bool lastKeysDown[512];

ImTui::TScreen * ImTui_ImplEmscripten_Init(bool mouseSupport) {
    if (g_screen == nullptr) {
        g_screen = new ImTui::TScreen();
    }

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

    return g_screen;
}

void ImTui_ImplEmscripten_Shutdown() {
    if (g_screen) {
        delete g_screen;
    }

    g_screen = nullptr;
}

void ImTui_ImplEmscripten_NewFrame() {
    ImGui::GetIO().MousePos = lastMousePos;
    ImGui::GetIO().MouseWheelH = lastMouseWheelH;
    ImGui::GetIO().MouseWheel = lastMouseWheel;
    ImGui::GetIO().MouseDown[0] = lastMouseDown[0];
    ImGui::GetIO().MouseDown[1] = lastMouseDown[2]; // right-click in browser is 2
    ImGui::GetIO().MouseDown[2] = lastMouseDown[1]; // scroll-click in browser is 1
    ImGui::GetIO().MouseDown[3] = lastMouseDown[3];
    ImGui::GetIO().MouseDown[4] = lastMouseDown[4];

    if (strlen(lastAddText) > 0 && (!(lastAddText[0] & 0x80)) ) {
        ImGui::GetIO().AddInputCharactersUTF8(lastAddText);
    }

    for (int i = 0; i < 512; ++i) {
        ImGui::GetIO().KeysDown[i] = lastKeysDown[i];
    }

    lastMouseWheelH = 0.0;
    lastMouseWheel = 0.0;
    lastAddText[0] = 0;
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
void get_screen(char * buffer) {
    int nx = g_screen->nx;
    int ny = g_screen->ny;

    int idx = 0;
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            const auto & cell = g_screen->data[y*nx + x];
            buffer[idx] = cell & 0x000000FF; ++idx;
            buffer[idx] = (cell & 0x00FF0000) >> 16; ++idx;
            buffer[idx] = (cell & 0xFF000000) >> 24; ++idx;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void set_key_down(int key) {
    lastAddText[0] = 0;
    lastAddText[1] = 0;

    if (lastKeysDown[17]) {
        (key == 65) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_A]] = true);
        (key == 67) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_C]] = true);
        (key == 86) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_V]] = true);
        (key == 88) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_X]] = true);
        (key == 89) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Y]] = true);
        (key == 90) && (lastKeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Z]] = true);
    } else {
        bool isSpecial = false;
        for (int i = 0; i < ImGuiKey_COUNT; ++i) {
            if (key == ImGui::GetIO().KeyMap[i] && key != ImGui::GetIO().KeyMap[ImGuiKey_Space]) {
                isSpecial = true;
                break;
            }
        }

        if (key == 189) { // minus '-' sign
            key = 45;
        }

        if (isSpecial == false) {
            lastAddText[0] = key;
        }
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
    if (key > 0) {
        lastAddText[0] = key;
        lastAddText[1] = 0;
    }
}

EMSCRIPTEN_KEEPALIVE
void set_screen_size(int nx, int ny) {
    ImGui::GetIO().DisplaySize.x = nx;
    ImGui::GetIO().DisplaySize.y = ny;
}
