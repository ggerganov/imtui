#include "imtui/imtui.h"
#include "imtui/imtui-impl-emscripten.h"

#include "imtui-common.h"
#include "imtui-demo.h"

#include <emscripten.h>

bool show_demo_window = true;

ImTui::TScreen screen;

extern "C" {
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_KEEPALIVE
        void get_screen(char * buffer) {
            int nx = screen.nx;
            int ny = screen.ny;

            int idx = 0;
            for (int y = 0; y < ny; ++y) {
                for (int x = 0; x < nx; ++x) {
                    const auto & cell = screen.data[y*nx + x];
                    buffer[idx] = cell & 0x000000FF; ++idx;
                    buffer[idx] = (cell & 0x00FF0000) >> 16; ++idx;
                    buffer[idx] = (cell & 0xFF000000) >> 24; ++idx;
                }
            }
        }

    EMSCRIPTEN_KEEPALIVE
        void set_screen_size(int nx, int ny) {
            ImGui::GetIO().DisplaySize.x = nx;
            ImGui::GetIO().DisplaySize.y = ny;
        }
#endif

    EMSCRIPTEN_KEEPALIVE
        void render_frame() {
            ImTui_ImplText_NewFrame();
            ImTui_ImplEmscripten_NewFrame();

            ImGui::NewFrame();

            ImGui::SetNextWindowPos(ImVec2(8, 28), ImGuiCond_Once);
            ImGui::SetNextWindowSize(ImVec2(50.0, 10.0), ImGuiCond_Once);
            ImGui::Begin("Hello, world!");
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

    ImTui_ImplText_Init();
    ImTui_ImplEmscripten_Init(true);

    return 0;
}
