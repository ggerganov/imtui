/*! \file imtui-impl-emscripten.h
 *  \brief Enter description here.
 */

#pragma once

namespace ImTui {
struct TScreen;
}

ImTui::TScreen * ImTui_ImplEmscripten_Init(bool mouseSupport);
void ImTui_ImplEmscripten_Shutdown();
void ImTui_ImplEmscripten_NewFrame();

extern "C" {
    void set_mouse_pos(float x, float y);
    void set_mouse_down(int but, float x, float y);
    void set_mouse_up(int but, float x, float y);
    void set_mouse_wheel(float x, float y);
    void set_screen_size(int nx, int ny);

    void set_key_down(int key);
    void set_key_up(int key);
    void set_key_press(int key);

    void get_screen(char * buffer);
}
