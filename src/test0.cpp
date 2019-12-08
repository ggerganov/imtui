/*! \file test0.cpp
 *  \brief Enter description here.
 */

#include "imgui/imgui.h"

#include <ncurses.h>
#include <stdio.h>
#include <stdint.h>

#include <cmath>
#include <vector>
#include <map>

#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <thread>
#include <chrono>

struct VSync {
    VSync(double rate_fps = 60.0) : tStep_us(1000000.0/rate_fps) {}

    uint64_t tStep_us;
    uint64_t tLast_us = t_us();
    uint64_t tNext_us = tLast_us + tStep_us;

    inline uint64_t t_us() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(); // duh ..
    }

    inline void wait() {
        uint64_t tNow_us = t_us();
        while (tNow_us < tNext_us - 100) {
            std::this_thread::sleep_for(std::chrono::microseconds((uint64_t) (0.9*(tNext_us - tNow_us))));
            tNow_us = t_us();
        }

        tNext_us += tStep_us;
    }

    inline float delta_s() {
        uint64_t tNow_us = t_us();
        uint64_t res = tNow_us - tLast_us;
        tLast_us = tNow_us;
        return float(res)/1e6f;
    }
};

using TChar = unsigned char;
using TColor = unsigned char;

struct TCell {
    TChar c;
    TColor f = 0;
    TColor b = 0;
};

using TScreen = std::vector<std::vector<TCell>>;

#define ABS(x) ((x >= 0) ? x : -x)

#define DIFF(uv0, uv1) \
    (std::fabs(uv0.x - uv1.x) > 1e-6 || \
     std::fabs(uv0.x - uv2.x) > 1e-6 || \
     std::fabs(uv1.x - uv2.x) > 1e-6 || \
     std::fabs(uv0.y - uv1.y) > 1e-6 || \
     std::fabs(uv0.y - uv2.y) > 1e-6 || \
     std::fabs(uv1.y - uv2.y) > 1e-6)

void ScanLine(int x1, int y1, int x2, int y2, int ymax, std::vector<int> & xrange) {
    int sx, sy, dx1, dy1, dx2, dy2, x, y, m, n, k, cnt;

    sx = x2 - x1;
    sy = y2 - y1;

    if (sx > 0) dx1 = 1;
    else if (sx < 0) dx1 = -1;
    else dx1 = 0;

    if (sy > 0) dy1 = 1;
    else if (sy < 0) dy1 = -1;
    else dy1 = 0;

    m = ABS(sx);
    n = ABS(sy);
    dx2 = dx1;
    dy2 = 0;

    if (m < n)
    {
        m = ABS(sy);
        n = ABS(sx);
        dx2 = 0;
        dy2 = dy1;
    }

    x = x1; y = y1;
    cnt = m + 1;
    k = n / 2;

    while (cnt--) {
        if ((y >= 0) && (y < ymax)) {
            if (x < xrange[2*y+0]) xrange[2*y+0] = x;
            if (x > xrange[2*y+1]) xrange[2*y+1] = x;
        }

        k += n;
        if (k < m) {
            x += dx2;
            y += dy2;
        } else {
            k -= m;
            x += dx1;
            y += dy1;
        }
    }
}

void drawTriangle(ImVec2 p0, ImVec2 p1, ImVec2 p2, unsigned char col, TScreen & screen) {
    int ymin = std::min(std::min(std::min((float) screen.size(), p0.y), p1.y), p2.y);
    int ymax = std::max(std::max(std::max(0.0f, p0.y), p1.y), p2.y);

    int ydelta = ymax - ymin + 1;

    std::vector<int> xrange(2*ydelta);

    for (int y = 0; y < ydelta; y++) {
        xrange[2*y+0] = 999999;
        xrange[2*y+1] = -999999;
    }

    ScanLine(p0.x, p0.y - ymin, p1.x, p1.y - ymin, ydelta, xrange);
    ScanLine(p1.x, p1.y - ymin, p2.x, p2.y - ymin, ydelta, xrange);
    ScanLine(p2.x, p2.y - ymin, p0.x, p0.y - ymin, ydelta, xrange);

    for (int y = 0; y < ydelta; y++) {
        if (xrange[2*y+1] >= xrange[2*y+0]) {
            int x = xrange[2*y+0];
            int len = 1 + xrange[2*y+1] - xrange[2*y+0];

            while (len--) {
                if (x >= 0 && x < (int) screen[0].size() && y + ymin >= 0 && y + ymin < (int) screen.size()) {
                    screen[y + ymin][x].c = ' ';
                    screen[y + ymin][x].b = col;
                }
                ++x;
            }
        }
    }
}

TColor rgbToAnsi256(ImU32 col, bool doAlpha) {
    TColor r = col & 0x000000FF;
    TColor g = (col & 0x0000FF00) >> 8;
    TColor b = (col & 0x00FF0000) >> 16;

    if (doAlpha) {
        TColor a = (col & 0xFF000000) >> 24;
        float scale = float(a)/255;
        r = std::round(r*scale);
        g = std::round(g*scale);
        b = std::round(b*scale);
    }

    if (r == g && g == b) {
        if (r < 8) {
            return 16;
        }

        if (r > 248) {
            return 231;
        }

        return std::round((float(r - 8) / 247) * 24) + 232;
    }

    TColor res = 16
        + (36 * std::round((float(r) / 255) * 5))
        + (6 * std::round((float(g) / 255) * 5))
        + std::round((float(b) / 255) * 5);

    return res;
}

void ImTui_RenderDrawData(ImDrawData* draw_data) {
    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0)
        return;

    TScreen screen(fb_height);
    for (auto & row : screen) {
        row.resize(fb_width);
    }

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];

        // Upload vertex/index buffers
        //glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
        //glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec4 clip_rect;
                clip_rect.x = (pcmd->ClipRect.x - clip_off.x) * clip_scale.x;
                clip_rect.y = (pcmd->ClipRect.y - clip_off.y) * clip_scale.y;
                clip_rect.z = (pcmd->ClipRect.z - clip_off.x) * clip_scale.x;
                clip_rect.w = (pcmd->ClipRect.w - clip_off.y) * clip_scale.y;

                if (clip_rect.x < fb_width && clip_rect.y < fb_height && clip_rect.z >= 0.0f && clip_rect.w >= 0.0f)
                {
                    float lastCharX = -10000.0f;
                    float lastCharY = -10000.0f;

                    for (unsigned int i = 0; i < pcmd->ElemCount; i += 3) {
                        int vidx0 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 0];
                        int vidx1 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 1];
                        int vidx2 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 2];

                        auto pos0 = cmd_list->VtxBuffer[vidx0].pos;
                        auto pos1 = cmd_list->VtxBuffer[vidx1].pos;
                        auto pos2 = cmd_list->VtxBuffer[vidx2].pos;

                        pos0.x = std::max(std::min(float(clip_rect.z - 1), pos0.x), clip_rect.x);
                        pos1.x = std::max(std::min(float(clip_rect.z - 1), pos1.x), clip_rect.x);
                        pos2.x = std::max(std::min(float(clip_rect.z - 1), pos2.x), clip_rect.x);
                        pos0.y = std::max(std::min(float(clip_rect.w - 1), pos0.y), clip_rect.y);
                        pos1.y = std::max(std::min(float(clip_rect.w - 1), pos1.y), clip_rect.y);
                        pos2.y = std::max(std::min(float(clip_rect.w - 1), pos2.y), clip_rect.y);

                        auto uv0 = cmd_list->VtxBuffer[vidx0].uv;
                        auto uv1 = cmd_list->VtxBuffer[vidx1].uv;
                        auto uv2 = cmd_list->VtxBuffer[vidx2].uv;

                        auto col0 = cmd_list->VtxBuffer[vidx0].col;
                        //auto col1 = cmd_list->VtxBuffer[vidx1].col;
                        //auto col2 = cmd_list->VtxBuffer[vidx2].col;

                        if (DIFF(uv0, uv1) || DIFF(uv0, uv2) || DIFF(uv1, uv2)) {
                            int vvidx0 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 3];
                            int vvidx1 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 4];
                            int vvidx2 = cmd_list->IdxBuffer[pcmd->IdxOffset + i + 5];

                            auto ppos0 = cmd_list->VtxBuffer[vvidx0].pos;
                            auto ppos1 = cmd_list->VtxBuffer[vvidx1].pos;
                            auto ppos2 = cmd_list->VtxBuffer[vvidx2].pos;

                            float x = ((pos0.x + pos1.x + pos2.x + ppos0.x + ppos1.x + ppos2.x)/6.0f);
                            float y = ((pos0.y + pos1.y + pos2.y + ppos0.y + ppos1.y + ppos2.y)/6.0f) - 0.1f;

                            if (std::fabs(y - lastCharY) < 0.5f && std::fabs(x - lastCharX) < 0.5f) {
                                x = lastCharX + 1.0f;
                                y = lastCharY;
                            }

                            lastCharX = x;
                            lastCharY = y;

                            int xx = (x) + 1;
                            int yy = (y) + 0;
                            if (xx < 0 || xx >= fb_width || yy < 0 || yy >= fb_height) {
                            } else {
                                screen[yy][xx].c = (col0 & 0xff000000) >> 24;
                                screen[yy][xx].f = rgbToAnsi256(col0, false);
                            }
                            i += 3;
                        } else {
                            auto ppos0 = pos0;
                            auto ppos1 = pos1;
                            auto ppos2 = pos2;

                            //ppos0.x = std::round(ppos0.x);
                            //ppos1.x = std::round(ppos1.x);
                            //ppos2.x = std::round(ppos2.x);
                            //ppos0.y = std::round(ppos0.y);
                            //ppos1.y = std::round(ppos1.y);
                            //ppos2.y = std::round(ppos2.y);

                            //ppos0.x = (pos0.x < pos1.x || pos0.x < pos2.x) ? std::ceil(pos0.x + 0.5f) : std::floor(pos0.x - 0.5f);
                            //ppos1.x = (pos1.x < pos0.x || pos1.x < pos2.x) ? std::ceil(pos1.x + 0.5f) : std::floor(pos1.x - 0.5f);
                            //ppos2.x = (pos2.x < pos0.x || pos2.x < pos1.x) ? std::ceil(pos2.x + 0.5f) : std::floor(pos2.x - 0.5f);
                            //ppos0.y = (pos0.y < pos1.y || pos0.y < pos2.y) ? std::ceil(pos0.y + 0.5f) : std::floor(pos0.y - 0.5f);
                            //ppos1.y = (pos1.y < pos0.y || pos1.y < pos2.y) ? std::ceil(pos1.y + 0.5f) : std::floor(pos1.y - 0.5f);
                            //ppos2.y = (pos2.y < pos0.y || pos2.y < pos1.y) ? std::ceil(pos2.y + 0.5f) : std::floor(pos2.y - 0.5f);

                            drawTriangle(ppos0, ppos1, ppos2, rgbToAnsi256(col0, true), screen);
                        }

                        //printf("  - command %u %d\n", i, cmd_list->VtxBuffer.Size);
                        //printf("    v: [%10.6f %10.6f], u[%10.6f %10.6f], col = \033[38;5;%dm%#08x\033[0m\n"
                        //       , cmd_list->VtxBuffer[vidx0].pos.x, cmd_list->VtxBuffer[vidx0].pos.y
                        //       , cmd_list->VtxBuffer[vidx0].uv.x,  cmd_list->VtxBuffer[vidx0].uv.y
                        //       , rgbToAnsi256(cmd_list->VtxBuffer[vidx0].col), cmd_list->VtxBuffer[vidx0].col);
                        //printf("    v: [%10.6f %10.6f], u[%10.6f %10.6f], col = %#08x\n"
                        //       , cmd_list->VtxBuffer[vidx1].pos.x, cmd_list->VtxBuffer[vidx1].pos.y
                        //       , cmd_list->VtxBuffer[vidx1].uv.x,  cmd_list->VtxBuffer[vidx1].uv.y
                        //       , cmd_list->VtxBuffer[vidx0].col);
                        //printf("    v: [%10.6f %10.6f], u[%10.6f %10.6f], col = %#08x\n"
                        //       , cmd_list->VtxBuffer[vidx2].pos.x, cmd_list->VtxBuffer[vidx2].pos.y
                        //       , cmd_list->VtxBuffer[vidx2].uv.x,  cmd_list->VtxBuffer[vidx2].uv.y
                        //       , cmd_list->VtxBuffer[vidx0].col);
                    }
                }
            }
        }
    }

    //printf("\033[1;48;5;240mbold red text\033[0m\n");
    if (0)
    {
        for (int x = 0; x < fb_width + 2; ++x) {
            printf("-");
        }
        printf("\n");
        for (int y = 0; y < fb_height; ++y) {
            printf("|");
            for (int x = 0; x < fb_width; ++x) {

                mvprintw(x, y, "\033[48;5;%d;38;5;%dm%c",
                       screen[y][x].b,
                       screen[y][x].f,
                       screen[y][x].c != 0 ? screen[y][x].c : ' ');
            }
            printf("|\n");
        }
        for (int x = 0; x < fb_width + 2; ++x) {
            printf("-");
        }
        printf("\n");
    }
    //printf("\033[0m\n");

    //for (int f = 0; f < 256; ++f) {
    //    for (int b = 0; b < 256; ++b) {
    //        init_pair(f*256 + b + 1, f, b);
    //    }
    //}

    int np = 1;
    std::map<int, int> pairs;

    for (int y = 0; y < fb_height; ++y) {
        for (int x = 0; x < fb_width; ++x) {
            int f = screen[y][x].f;
            int b = screen[y][x].b;
            int p = b*256 + f;

            if (pairs.find(p) == pairs.end()) {
                pairs[p] = np;
                init_pair(np, f, b);
                ++np;
            }

            attron(COLOR_PAIR(pairs[p]));
            mvprintw(y, x, "%c",
                     screen[y][x].c > 0 ? screen[y][x].c : '.');
            //mvprintw(y, x, "\033[48;5;%d;38;5;%dm%c",
            //         screen[y][x].b,
            //         screen[y][x].f,
            //         screen[y][x].c > 0 ? screen[y][x].c : ' ');
        }
    }
}

int main() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto & style = ImGui::GetStyle();

    style.Alpha                   = 1.0f;
    style.WindowPadding           = ImVec2(0.5,0.0);
    style.WindowRounding          = 0.0f;
    style.WindowBorderSize        = 0.0f;
    style.WindowMinSize           = ImVec2(4,2);
    style.WindowTitleAlign        = ImVec2(0.0f,0.0f);
    style.WindowMenuButtonPosition= ImGuiDir_Left;
    style.ChildRounding           = 0.0f;
    style.ChildBorderSize         = 0.0f;
    style.PopupRounding           = 0.0f;
    style.PopupBorderSize         = 0.0f;
    style.FramePadding            = ImVec2(1.0,0.0);
    style.FrameRounding           = 0.0f;
    style.FrameBorderSize         = 0.0f;
    style.ItemSpacing             = ImVec2(1,0.0);
    style.ItemInnerSpacing        = ImVec2(1,0);
    style.TouchExtraPadding       = ImVec2(0.5,0.0);
    style.IndentSpacing           = 1.0f;
    style.ColumnsMinSpacing       = 1.0f;
    style.ScrollbarSize           = 0.5f;
    style.ScrollbarRounding       = 0.0f;
    style.GrabMinSize             = 0.1f;
    style.GrabRounding            = 0.0f;
    style.TabRounding             = 0.0f;
    style.TabBorderSize           = 0.0f;
    style.ColorButtonPosition     = ImGuiDir_Right;
    style.ButtonTextAlign         = ImVec2(0.5f,0.0f);
    style.SelectableTextAlign     = ImVec2(0.0f,0.0f);
    style.DisplayWindowPadding    = ImVec2(0,0);
    style.DisplaySafeAreaPadding  = ImVec2(0,0);
    style.MouseCursorScale        = 1.0f;
    style.AntiAliasedLines        = false;
    style.AntiAliasedFill         = false;
    style.CurveTessellationTol    = 1.25f;

    ImGui::GetIO().KeyMap[ImGuiKey_Tab]         = 9;
    ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow]   = 260;
    ImGui::GetIO().KeyMap[ImGuiKey_RightArrow]  = 261;
    ImGui::GetIO().KeyMap[ImGuiKey_UpArrow]     = 259;
    ImGui::GetIO().KeyMap[ImGuiKey_DownArrow]   = 258;
    ImGui::GetIO().KeyMap[ImGuiKey_PageUp]      = 339;
    ImGui::GetIO().KeyMap[ImGuiKey_PageDown]    = 338;
    ImGui::GetIO().KeyMap[ImGuiKey_Home]        = 262;
    ImGui::GetIO().KeyMap[ImGuiKey_End]         = 360;
    ImGui::GetIO().KeyMap[ImGuiKey_Insert]      = 331;
    ImGui::GetIO().KeyMap[ImGuiKey_Delete]      = 330;
    ImGui::GetIO().KeyMap[ImGuiKey_Backspace]   = 263;
    ImGui::GetIO().KeyMap[ImGuiKey_Space]       = 32;
    ImGui::GetIO().KeyMap[ImGuiKey_Enter]       = 10;
    ImGui::GetIO().KeyMap[ImGuiKey_Escape]      = 27;
    ImGui::GetIO().KeyMap[ImGuiKey_KeyPadEnter] = 343;
    ImGui::GetIO().KeyMap[ImGuiKey_A]           = 1;
    ImGui::GetIO().KeyMap[ImGuiKey_C]           = 3;
    ImGui::GetIO().KeyMap[ImGuiKey_V]           = 22;
    ImGui::GetIO().KeyMap[ImGuiKey_X]           = 24;
    ImGui::GetIO().KeyMap[ImGuiKey_Y]           = 25;
    ImGui::GetIO().KeyMap[ImGuiKey_Z]           = 26;

    ImFontConfig fontConfig;
    //fontConfig.GlyphExtraSpacing.x = 1.9f;
    //fontConfig.PixelSnapH = true;
    //fontConfig.OversampleV = 3;
    fontConfig.GlyphMinAdvanceX = 1.0f;
    fontConfig.SizePixels = 1.00;
    ImGui::GetIO().Fonts->AddFontDefault(&fontConfig);

    ImGui::StyleColorsDark();

    // Build atlas
    unsigned char* tex_pixels = NULL;
    int tex_w, tex_h;
    ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&tex_pixels, &tex_w, &tex_h);

    //ImGui::GetIO().FontGlobalScale = 0.1538462f;
    //ImGui::GetIO().FontGlobalScale = 1.2;
    //ImGui::GetIO().FontGlobalScale = 1.0f/14.0f;

    initscr();
    use_default_colors();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    mouseinterval(0);
    //halfdelay(1);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    printf("\033[?1003h\n"); // Makes the terminal report mouse movement events

    int nframes = 0;

    int mx = 0;
    int my = 0;
    int lbut = 0;
    unsigned long mstate = 0;

    bool showDemoWindow = true;

    char input[3];
    input[2] = 0;

    bool lastKeysDown[512];

    VSync vsync;

    while (true) {
        std::fill(lastKeysDown, lastKeysDown + 512, 0);

        ImGui::GetIO().KeyCtrl = false;

        while (true) {
            int c = wgetch(stdscr);

            if (c == ERR) {
                if ((mstate & 0xf) == 0x1) lbut = 0;
                break;
            } else if (c == KEY_MOUSE) {
                MEVENT event;
                if (getmouse(&event) == OK) {
                    mx = event.x;
                    my = event.y;
                    mstate = event.bstate;
                    if ((mstate & 0xf) == 0x2) lbut = 1;
                    if ((mstate & 0xf) == 0x1) lbut = 0;
                    if ((mstate & 0xf) == 0x2) lbut = 1;
                    if ((mstate & 0xf) == 0x1) lbut = 0;
                    ImGui::GetIO().KeyCtrl |= ((mstate & 0x0F000000) == 0x01000000);
                }
            } else {
                input[0] = c & 0x000000FF;
                input[1] = (c & 0x0000FF00) >> 8;
                if (c < 128) {
                    ImGui::GetIO().AddInputCharactersUTF8(input);
                }
                lastKeysDown[c] = true;
            }
        }
        for (int i = 0; i < 512; ++i) {
            ImGui::GetIO().KeysDown[i] = lastKeysDown[i];
        }

        if (ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_A]]) ImGui::GetIO().KeyCtrl = true;
        if (ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_C]]) ImGui::GetIO().KeyCtrl = true;
        if (ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_V]]) ImGui::GetIO().KeyCtrl = true;
        if (ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_X]]) ImGui::GetIO().KeyCtrl = true;
        if (ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Y]]) ImGui::GetIO().KeyCtrl = true;
        if (ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Z]]) ImGui::GetIO().KeyCtrl = true;

        ImGui::GetIO().MousePos.x = mx;
        ImGui::GetIO().MousePos.y = my;
        ImGui::GetIO().MouseDown[0] = lbut;

        ImGui::GetIO().DisplaySize = ImVec2(200, 60);
        ImGui::GetIO().DeltaTime = vsync.delta_s();
        ImGui::NewFrame();

        if (nframes > 10) {
            ImGui::SetNextWindowPos(ImVec2(10.1, 2.1), ImGuiCond_Once);
            ImGui::ShowDemoWindow(&showDemoWindow);
            ImGui::GetFont()->FontSize = 1.00f;
        }

        ImGui::SetNextWindowPos(ImVec2(61, 1), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(80.0, 20.0), ImGuiCond_Always);
        ImGui::Begin("Hello, world!");

        ImGui::Text("This is some useful text.");

        static float f = 0.5f;
        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);

        static int counter = 0;
        if (ImGui::Button("Button")) counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        ImGui::Text("--------------------------");

        ImGui::Text("nframes = %d", nframes++);
        ImGui::Text("mx = %d, my = %d, but = %d, mstate = 0x%08lx", mx, my, lbut, mstate);
        ImGui::Text("key = %d ", input[0]);
        ImGui::Text("font size = %10.6f", ImGui::GetFont()->FontSize);
        ImGui::End();

        ImGui::Render();

        ImTui_RenderDrawData(ImGui::GetDrawData());

        vsync.wait();
    }

    ImGui::DestroyContext();

    return 0;
}
