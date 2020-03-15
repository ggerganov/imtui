/*! \file imtui-impl-ncurses.cpp
 *  \brief Enter description here.
 */

#include "imtui/imtui.h"
#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-impl-text.h"

#include <ncurses.h>

#include <array>
#include <chrono>
#include <map>
#include <vector>
#include <string>
#include <thread>

namespace {
    struct VSync {
        VSync(double fps_active = 60.0, double fps_idle = 60.0) :
            tStepActive_us(1000000.0/fps_active),
            tStepIdle_us(1000000.0/fps_idle) {}

        uint64_t tStepActive_us;
        uint64_t tStepIdle_us;
        uint64_t tLast_us = t_us();
        uint64_t tNext_us = tLast_us;

        inline uint64_t t_us() const {
            return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(); // duh ..
        }

        inline void wait(bool active) {
            uint64_t tNow_us = t_us();

            auto tStep_us = active ? tStepActive_us : tStepIdle_us;
            auto tNextCur_us = tNext_us + tStep_us;

            wtimeout(stdscr, 0);
            while (tNow_us < tNextCur_us - 100) {
                if (tNow_us + 0.5*tStepActive_us < tNextCur_us) {
                    int ch = wgetch(stdscr);

                    if (ch != ERR) {
                        ungetch(ch);
                        tNextCur_us = tNow_us;
                        wtimeout(stdscr, 1);

                        return;
                    }
                }

                std::this_thread::sleep_for(std::chrono::microseconds(
                        std::min((uint64_t)(0.9*tStepActive_us),
                                 (uint64_t) (0.9*(tNextCur_us - tNow_us))
                                 )));

                tNow_us = t_us();
            }

            tNext_us += tStep_us;
            wtimeout(stdscr, 1);
        }

        inline float delta_s() {
            uint64_t tNow_us = t_us();
            uint64_t res = tNow_us - tLast_us;
            tLast_us = tNow_us;

            return float(res)/1e6f;
        }
    };
}

static VSync g_vsync;
static ImTui::TScreen * g_screen = nullptr;

ImTui::TScreen * ImTui_ImplNcurses_Init(bool mouseSupport, float fps_active, float fps_idle) {
    if (g_screen == nullptr) {
        g_screen = new ImTui::TScreen();
    }

    if (fps_idle < 0.0) {
        fps_idle = fps_active;
    }
    fps_idle = std::min(fps_active, fps_idle);
    g_vsync = VSync(fps_active, fps_idle);

    initscr();
    use_default_colors();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    wtimeout(stdscr, 1);
    set_escdelay(25);

    if (mouseSupport) {
        keypad(stdscr, true);
        mouseinterval(0);
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        printf("\033[?1003h\n");
    } else {
        keypad(stdscr, false);
    }

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

    ImGui::GetIO().KeyRepeatDelay = 0.050;
    ImGui::GetIO().KeyRepeatRate = 0.050;

	int screenSizeX = 0;
	int screenSizeY = 0;

	getmaxyx(stdscr, screenSizeY, screenSizeX);
	ImGui::GetIO().DisplaySize = ImVec2(screenSizeX, screenSizeY);

    return g_screen;
}

void ImTui_ImplNcurses_Shutdown() {
    endwin();

    if (g_screen) {
        delete g_screen;
    }

    g_screen = nullptr;
}

bool ImTui_ImplNcurses_NewFrame() {
    bool hasInput = false;

	int screenSizeX = 0;
	int screenSizeY = 0;

	getmaxyx(stdscr, screenSizeY, screenSizeX);
	ImGui::GetIO().DisplaySize = ImVec2(screenSizeX, screenSizeY);

    static int mx = 0;
    static int my = 0;
    static int lbut = 0;
    static int rbut = 0;
    static unsigned long mstate = 0;
    static char input[3];

    input[2] = 0;

    auto & keysDown = ImGui::GetIO().KeysDown;
    std::fill(keysDown, keysDown + 512, 0);

    ImGui::GetIO().KeyCtrl = false;
    ImGui::GetIO().KeyShift = false;

    while (true) {
        int c = wgetch(stdscr);

        if (c == ERR) {
            if ((mstate & 0xf) == 0x1) {
                lbut = 0;
                rbut = 0;
            }
            break;
        } else if (c == KEY_MOUSE) {
            MEVENT event;
            if (getmouse(&event) == OK) {
                mx = event.x;
                my = event.y;
                mstate = event.bstate;
                if ((mstate & 0x000f) == 0x0002) lbut = 1;
                if ((mstate & 0x000f) == 0x0001) lbut = 0;
                if ((mstate & 0xf000) == 0x2000) rbut = 1;
                if ((mstate & 0xf000) == 0x1000) rbut = 0;
                //printf("mstate = 0x%016lx\n", mstate);
                ImGui::GetIO().KeyCtrl |= ((mstate & 0x0F000000) == 0x01000000);
            }
        } else {
            input[0] = (c & 0x000000FF);
            input[1] = (c & 0x0000FF00) >> 8;
            //printf("c = %d, c0 = %d, c1 = %d xxx\n", c, input[0], input[1]);
            if (c < 128) {
                if (c != ImGui::GetIO().KeyMap[ImGuiKey_Enter]) {
                    ImGui::GetIO().AddInputCharactersUTF8(input);
                }
            }
            if (c == 330) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Delete]] = true;
            } else if (c == KEY_BACKSPACE || c == KEY_DC || c == 127) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_Backspace]] = true;
            // Shift + arrows (probably not portable :()
            } else if (c == 393) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow]] = true;
                ImGui::GetIO().KeyShift = true;
            } else if (c == 402) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_RightArrow]] = true;
                ImGui::GetIO().KeyShift = true;
            } else if (c == 337) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_UpArrow]] = true;
                ImGui::GetIO().KeyShift = true;
            } else if (c == 336) {
                ImGui::GetIO().KeysDown[ImGui::GetIO().KeyMap[ImGuiKey_DownArrow]] = true;
                ImGui::GetIO().KeyShift = true;
            } else {
                keysDown[c] = true;
            }
        }

        hasInput = true;
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
    ImGui::GetIO().MouseDown[1] = rbut;

    ImGui::GetIO().DeltaTime = g_vsync.delta_s();

    return hasInput;
}

// state
int nColPairs = 1;
int nActiveFrames = 10;
ImTui::TScreen screenPrev;
std::vector<uint8_t> curs;
std::array<std::pair<bool, int>, 256*256> colPairs;

void ImTui_ImplNcurses_DrawScreen(bool active) {
    if (active) nActiveFrames = 10;

    wrefresh(stdscr);

    int nx = g_screen->nx;
    int ny = g_screen->ny;

    bool compare = true;

    if (screenPrev.nx != nx || screenPrev.ny != ny) {
        screenPrev.resize(nx, ny);
        compare = false;
    }

    int ic = 0;
    curs.resize(nx + 1);

    for (int y = 0; y < ny; ++y) {
        bool isSame = compare;
        if (compare) {
            for (int x = 0; x < nx; ++x) {
                if (screenPrev.data[y*nx + x] != g_screen->data[y*nx + x]) {
                    isSame = false;
                    break;
                }
            }
        }
        if (isSame) continue;

        int lastp = 0xFFFFFFFF;
        move(y, 0);
        for (int x = 0; x < nx; ++x) {
            auto cell = g_screen->data[y*nx + x];
            uint16_t f = (cell & 0x00FF0000) >> 16;
            uint16_t b = (cell & 0xFF000000) >> 24;
            uint16_t p = b*256 + f;

            if (colPairs[p].first == false) {
                init_pair(nColPairs, f, b);
                colPairs[p].first = true;
                colPairs[p].second = nColPairs;
                ++nColPairs;
            }

            if (lastp != (int) p) {
                if (curs.size() > 0) {
                    curs[ic] = 0;
                    addstr((char *) curs.data());
                    ic = 0;
                    curs[0] = 0;
                }
                attron(COLOR_PAIR(colPairs[p].second));
                lastp = p;
            }

            uint16_t c = cell & 0x0000FFFF;
            curs[ic++] = c > 0 ? c : '.';
        }

        if (curs.size() > 0) {
            curs[ic] = 0;
            addstr((char *) curs.data());
            ic = 0;
            curs[0] = 0;
        }

        if (compare) {
            memcpy(screenPrev.data + y*nx, g_screen->data + y*nx, nx*sizeof(ImTui::TCell));
        }
    }

    if (!compare) {
        memcpy(screenPrev.data, g_screen->data, nx*ny*sizeof(ImTui::TCell));
    }

    g_vsync.wait(nActiveFrames --> 0);
}

bool ImTui_ImplNcurses_ProcessEvent() {
    return true;
}
