#include "imgui/imgui.h"

#include "imgui_impl_text.h"
#include "imgui_impl_ncurses.h"

#include "imtui_common.h"

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    VSync vsync;
    TScreen screen;

    ImGui_ImplNcurses_Init();
    ImGui_ImplText_Init();

    int nframes = 0;
    float fval = 1.23f;

    while (true) {
        ImGui_ImplNcurses_NewFrame(screen);
        ImGui_ImplText_NewFrame();

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

        ImGui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
        ImGui_ImplNcurses_DrawScreen(screen);

        vsync.wait();
    }

    ImGui_ImplText_Shutdown();
    ImGui_ImplNcurses_Shutdown();

    return 0;
}
