#include "imtui/imtui.h"
#include "imtui/imtui-impl-emscripten.h"

#include "imtui-common.h"
#include "imtui-demo.h"

#include <emscripten.h>

bool show_demo_window = true;

ImTui::TScreen screen;

extern "C" {
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
        }

    EMSCRIPTEN_KEEPALIVE
        void render_frame() {
            ImTui_ImplText_NewFrame();
            ImTui_ImplEmscripten_NewFrame(screen);

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
    ImTui_ImplEmscripten_Init();

    return 0;
}
