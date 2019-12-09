#include "imtui/imtui.h"

#include "imtui/imtui-impl-ncurses.h"

#include "imtui-common.h"

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    VSync vsync;
    ImTui::TScreen screen;

    ImTui_ImplNcurses_Init();
    ImTui_ImplText_Init();

    int nframes = 0;
    float fval = 1.23f;

    while (true) {
        ImTui_ImplNcurses_NewFrame(screen);
        ImTui_ImplText_NewFrame();

        ImGui::GetIO().DeltaTime = vsync.delta_s();

        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(4, 2), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(50.0, 10.0), ImGuiCond_Once);
        ImGui::Begin("Hello, world!");
        ImGui::Text("NFrames = %d", nframes++);
        ImGui::Text("Mouse Pos : x = %g, y = %g", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y);
        ImGui::Text("Time per frame %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Text("Float:");
        ImGui::SameLine();
        ImGui::SliderFloat("##float", &fval, 0.0f, 10.0f);

        ImGui::End();

        ImGui::Render();

        ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
        ImTui_ImplNcurses_DrawScreen(screen);

        vsync.wait();
    }

    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();

    return 0;
}
