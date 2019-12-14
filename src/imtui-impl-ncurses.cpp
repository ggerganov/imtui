/*! \file imtui-impl-ncurses.cpp
 *  \brief Enter description here.
 */

#include "imtui/imtui.h"
#include "imtui/imtui-impl-ncurses.h"
#include "imtui/imtui-impl-text.h"

#include <ncurses.h>

#include <map>

bool ImTui_ImplNcurses_Init() {
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

void ImTui_ImplNcurses_NewFrame(ImTui::TScreen & screen) {
	int screenSizeX = 0;
	int screenSizeY = 0;

	getmaxyx(stdscr, screenSizeY, screenSizeX);
	ImGui::GetIO().DisplaySize = ImVec2(screenSizeX, screenSizeY);

    screen.data.resize(ImGui::GetIO().DisplaySize.y);
    for (auto & row : screen.data) {
        row.clear();
        row.shrink_to_fit();
        row.resize(ImGui::GetIO().DisplaySize.x);
    }

    static int mx = 0;
    static int my = 0;
    static int lbut = 0;
    static unsigned long mstate = 0;
    static char input[3];

    input[2] = 0;

    static bool lastKeysDown[512];

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
}

void ImTui_ImplNcurses_DrawScreen(ImTui::TScreen & screen) {
    int np = 1;
    std::map<int, int> pairs;

    int nx = screen.data[0].size();
    int ny = screen.data.size();

    for (int y = 0; y < ny; ++y) {
        int lastp = -1;
        int lastx = 0;
        std::string curs = "";
        for (int x = 0; x < nx; ++x) {
            int f = screen.data[y][x].f;
            int b = screen.data[y][x].b;
            int p = b*256 + f;

            if (pairs.find(p) == pairs.end()) {
                pairs[p] = np;
                init_pair(np, f, b);
                ++np;
            }

            if (lastp != pairs[p]) {
                if (curs.size() > 0) {
                    mvprintw(y, lastx, "%s", curs.c_str());
                    curs = "";
                }
                attron(COLOR_PAIR(pairs[p]));
                lastx = x;
                lastp = pairs[p];
            }
            curs += screen.data[y][x].c > 0 ? screen.data[y][x].c : '.';
        }

        if (curs.size() > 0) {
            mvprintw(y, lastx, "%s", curs.c_str());
        }
    }
}

bool ImTui_ImplNcurses_ProcessEvent() {
    return true;
}
