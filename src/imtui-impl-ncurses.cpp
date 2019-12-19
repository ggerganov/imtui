/*! \file imtui-impl-ncurses.cpp
 *  \brief Enter description here.
 */

#include "imtui/imtui.h"
#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-impl-text.h"

#include <ncurses.h>

#include <map>
#include <vector>
#include <string>

bool ImTui_ImplNcurses_Init(bool mouseSupport) {
    initscr();
    use_default_colors();
    start_color();
    cbreak();
    noecho();
    curs_set(0);
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    if (mouseSupport) {
        mouseinterval(0);
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
        printf("\033[?1003h\n");
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

	int screenSizeX = 0;
	int screenSizeY = 0;

	getmaxyx(stdscr, screenSizeY, screenSizeX);
	ImGui::GetIO().DisplaySize = ImVec2(screenSizeX, screenSizeY);

    return true;
}

void ImTui_ImplNcurses_Shutdown() {
    endwin();
}

void ImTui_ImplNcurses_NewFrame() {
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

    static bool lastKeysDown[512];

    std::fill(lastKeysDown, lastKeysDown + 512, 0);

    ImGui::GetIO().KeyCtrl = false;

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
    ImGui::GetIO().MouseDown[1] = rbut;
}

// state
int nColPairs = 1;
ImTui::TScreen screenPrev;
std::vector<uint8_t> curs;
std::array<std::pair<bool, int>, 256*256> colPairs;

void ImTui_ImplNcurses_DrawScreen(const ImTui::TScreen & screen) {
    int nx = screen.nx;
    int ny = screen.ny;

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
                if (screenPrev.data[y*nx + x] != screen.data[y*nx + x]) {
                    isSame = false;
                    break;
                }
            }
        }
        if (isSame) continue;

        int lastp = 0xFFFFFFFF;
        move(y, 0);
        for (int x = 0; x < nx; ++x) {
            auto cell = screen.data[y*nx + x];
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
            memcpy(screenPrev.data + y*nx, screen.data + y*nx, nx*sizeof(ImTui::TCell));
        }
    }

    if (!compare) {
        memcpy(screenPrev.data, screen.data, nx*ny*sizeof(ImTui::TCell));
    }
}

bool ImTui_ImplNcurses_ProcessEvent() {
    return true;
}
