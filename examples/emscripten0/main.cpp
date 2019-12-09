#include "imtui/imtui.h"

#include "imtui-common.h"
#include "imtui-demo.h"

#include <emscripten.h>

#include <thread>
#include <chrono>
#include <string>
#include <cstdint>

#ifndef EMSCRIPTEN_KEEPALIVE
#define EMSCRIPTEN_KEEPALIVE
#endif

// For clarity, our main loop code is declared at the end.
void main_loop(void*);

int nframes = 0;

bool show_demo_window = true;

ImTui::TScreen screen;

// client input
ImVec2 lastMousePos = { 0.0, 0.0 };
bool  lastMouseDown[5] = { false, false, false, false, false };
float lastMouseWheel = 0.0;
float lastMouseWheelH = 0.0;

std::string lastAddText = "";
bool lastKeysDown[512];

extern "C" {

EMSCRIPTEN_KEEPALIVE
void set_mouse_pos(float x, float y) {
    lastMousePos.x = x;
    lastMousePos.y = y;
}

EMSCRIPTEN_KEEPALIVE
void set_mouse_down(int but, float x, float y) {
    lastMouseDown[but] = true;
    lastMousePos.x = x;
    lastMousePos.y = y;
}

EMSCRIPTEN_KEEPALIVE
void set_mouse_up(int but, float x, float y) {
    lastMouseDown[but] = false;
    lastMousePos.x = x;
    lastMousePos.y = y;
}

EMSCRIPTEN_KEEPALIVE
void set_mouse_wheel(float x, float y) {
    lastMouseWheelH = x;
    lastMouseWheel  = y;
}

EMSCRIPTEN_KEEPALIVE
void set_key_down(int key) {
    if (key > 0) {
        lastKeysDown[key] = true;
    }
}

EMSCRIPTEN_KEEPALIVE
void set_key_up(int key) {
    if (key > 0) {
        lastKeysDown[key] = false;
    }
}

EMSCRIPTEN_KEEPALIVE
void set_key_press(int key) {
    if (key > 0) {
        lastAddText.resize(1);
        lastAddText[0] = key;
    }
}

EMSCRIPTEN_KEEPALIVE
void get_screen(char * buffer) {
    int nx = screen.data[0].size();
    int ny = screen.data.size();

    int idx = 0;
    for (int y = 0; y < ny; ++y) {
        for (int x = 0; x < nx; ++x) {
            buffer[idx] = screen.data[y][x].c; ++idx;
            buffer[idx] = screen.data[y][x].f; ++idx;
            buffer[idx] = screen.data[y][x].b; ++idx;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void set_screen_size(int nx, int ny) {
    ImGui::GetIO().DisplaySize.x = nx;
    ImGui::GetIO().DisplaySize.y = ny;

    ImTui_ImplText_Init();
}

EMSCRIPTEN_KEEPALIVE
void render_frame() {
    ImTui_ImplText_NewFrame();

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

    ImGui::NewFrame();

    ImGui::SetNextWindowPos(ImVec2(8, 28), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(50.0, 10.0), ImGuiCond_Once);
    ImGui::Begin("Hello, world!");
    ImGui::Text("nframes = %d", nframes++);
    ImGui::Text("mx = %g, my = %g", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    ImTui::ShowDemoWindow(&show_demo_window);

    ImGui::Render();

    ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
}
}

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    //emscripten_set_main_loop_arg(main_loop, NULL, 60, true);

    return 0;
}

void main_loop(void *)
{
}
