#include "imtui/imtui.h"
#include "imtui/imtui-impl-emscripten.h"

#include "imtui-demo.h"

#include <emscripten.h>

ImTui::TScreen * g_screen = nullptr;

extern "C" {
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

            bool showDemoWindow = true;
            ImTui::ShowDemoWindow(&showDemoWindow);

            ImGui::Render();

            ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), g_screen);
        }
}

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImTui_ImplText_Init();
    g_screen = ImTui_ImplEmscripten_Init(true);

    return 0;
}
