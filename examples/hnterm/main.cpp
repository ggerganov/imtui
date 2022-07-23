/*! \file main.cpp
 *  \brief HNTerm - browse Hacker News in the terminal
 */

#include "imtui/imtui.h"

#include "hn-state.h"

#ifdef __EMSCRIPTEN__

#include "imtui/imtui-impl-emscripten.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#else

#define EMSCRIPTEN_KEEPALIVE

#include "imtui/imtui-impl-ncurses.h"

#endif

#include <array>
#include <map>
#include <vector>
#include <string>
#include <functional>

// global vars
bool g_updated = false;
ImTui::TScreen * g_screen = nullptr;

// platform specific functions
extern bool hnInit();
extern void hnFree();
extern int openInBrowser(std::string uri);

// helper functions
namespace {

[[maybe_unused]]
std::map<std::string, std::string> parseCmdArguments(int argc, char ** argv) {
    int last = argc;
    std::map<std::string, std::string> res;
    for (int i = 1; i < last; ++i) {
        res[argv[i]] = "";
        if (argv[i][0] == '-') {
            if (strlen(argv[i]) > 1) {
                res[std::string(1, argv[i][1])] = strlen(argv[i]) > 2 ? argv[i] + 2 : "";
            }
        }
    }

    return res;
}

}

namespace UI {

enum class WindowContent : int {
    Top,
    //Best,
    Show,
    Ask,
    New,
    Count,
};

enum class StoryListMode : int {
    Micro,
    Normal,
    Spread,
    COUNT,
};

enum class ColorScheme : int {
    Default,
    Dark,
    Green,
    COUNT,
};

std::map<WindowContent, std::string> kContentStr = {
    { WindowContent::Top, "Top" },
    //{ WindowContent::Best, "Best" },
    { WindowContent::Show, "Show" },
    { WindowContent::Ask, "Ask" },
    { WindowContent::New, "New" },
};

struct WindowData {
    WindowContent content;
    bool showComments = false;
    HN::ItemId selectedStoryId = 0;
    int hoveredStoryId = 0;
    int hoveredCommentId = 0;
    int maxStories = 10;
};

struct State {
    int hoveredWindowId = 0;
    int statusWindowHeight = 4;

    ColorScheme colorScheme = ColorScheme::Dark;
    StoryListMode storyListMode = StoryListMode::Normal;
#ifdef __EMSCRIPTEN__
    bool showHelpWelcome = true;
#else
    bool showHelpWelcome = false;
#endif
    bool showHelpModal = false;
    bool showStatusWindow = true;

    int nWindows = 2;

    char statusWindowHeader[512];

    std::map<int, bool> collapsed;

    void changeColorScheme(bool inc = true) {
        if (inc) {
            colorScheme = (ColorScheme)(((int) colorScheme + 1) % ((int)ColorScheme::COUNT));
        }

        ImVec4* colors = ImGui::GetStyle().Colors;
        switch (colorScheme) {
            case ColorScheme::Default:
                {
                    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
                    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
                    colors[ImGuiCol_WindowBg]               = ImVec4(0.96f, 0.96f, 0.94f, 1.00f);
                    colors[ImGuiCol_TitleBg]                = ImVec4(1.00f, 0.40f, 0.00f, 1.00f);
                    colors[ImGuiCol_TitleBgActive]          = ImVec4(1.00f, 0.40f, 0.00f, 1.00f);
                    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.69f, 0.25f, 0.00f, 1.00f);
                    colors[ImGuiCol_ChildBg]                = ImVec4(0.96f, 0.96f, 0.94f, 1.00f);
                    colors[ImGuiCol_PopupBg]                = ImVec4(0.96f, 0.96f, 0.94f, 1.00f);
                    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
                }
                break;
            case ColorScheme::Dark:
                {
                    colors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
                    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
                    colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
                    colors[ImGuiCol_TitleBg]                = ImVec4(1.00f, 0.40f, 0.00f, 0.50f);
                    colors[ImGuiCol_TitleBgActive]          = ImVec4(1.00f, 0.40f, 0.00f, 0.50f);
                    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.69f, 0.25f, 0.00f, 0.50f);
                    colors[ImGuiCol_ChildBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
                    colors[ImGuiCol_PopupBg]                = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
                    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
                }
                break;
            case ColorScheme::Green:
                {
                    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 1.00f, 0.00f, 1.00f);
                    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
                    colors[ImGuiCol_WindowBg]               = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
                    colors[ImGuiCol_TitleBg]                = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
                    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
                    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.50f, 1.00f, 0.50f, 1.00f);
                    colors[ImGuiCol_ChildBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
                    colors[ImGuiCol_PopupBg]                = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
                    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
                }
                break;
            default:
                {
                }
        }
    }

    std::array<WindowData, 3> windows { {
        {
            WindowContent::Top,
            false,
            0, 0, 0, 10,
        },
        {
            WindowContent::Show,
            false,
            0, 0, 0, 10,
        },
        {
            WindowContent::New,
            false,
            0, 0, 0, 10,
        },
    } };
};

}

// HackerNews content
HN::State stateHN;

// UI state
UI::State stateUI;

extern "C" {
    EMSCRIPTEN_KEEPALIVE
        bool render_frame() {
            HN::ItemIds toUpdate;
            HN::ItemIds toRefresh;

#ifdef __EMSCRIPTEN__
            ImTui_ImplEmscripten_NewFrame();
#else
            bool isActive = g_updated;
            isActive |= ImTui_ImplNcurses_NewFrame();
#endif
            ImTui_ImplText_NewFrame();

            ImGui::NewFrame();

            if (ImGui::GetIO().DisplaySize.x < 50) {
                stateUI.nWindows = 1;
            }

            for (int windowId = 0; windowId < stateUI.nWindows; ++windowId) {
                const auto & items = stateHN.items;
                auto & window = stateUI.windows[windowId];

                {
                    auto wSize = ImGui::GetIO().DisplaySize;
                    wSize.x /= stateUI.nWindows;
                    if (stateUI.showStatusWindow) {
                        wSize.y -= stateUI.statusWindowHeight;
                    }
                    wSize.x = int(wSize.x);
                    ImGui::SetNextWindowPos(ImVec2(wSize.x*windowId, 0), ImGuiCond_Always);

                    if (windowId < stateUI.nWindows - 1) {
                        wSize.x -= 1.1;
                    }
                    ImGui::SetNextWindowSize(wSize, ImGuiCond_Always);
                }

                std::string title = "[Y] Hacker News (" + UI::kContentStr[window.content] + ")##" + std::to_string(windowId);
                ImGui::Begin(title.c_str(), nullptr,
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollbar);

                ImGui::Text("%s", "");
                if (window.showComments == false) {
                    const auto & storyIds =
                        (window.content == UI::WindowContent::Top) ? stateHN.idsTop :
                        (window.content == UI::WindowContent::Show) ? stateHN.idsShow :
                        (window.content == UI::WindowContent::New) ? stateHN.idsNew :
                        stateHN.idsTop;

                    int nShow = std::min(window.maxStories, (int) storyIds.size());
                    for (int i = 0; i < nShow; ++i) {
                        const auto & id = storyIds[i];

                        toRefresh.push_back(id);
                        if (items.find(id) == items.end() || (
                                        std::holds_alternative<HN::Story>(items.at(id).data) == false &&
                                        std::holds_alternative<HN::Job>(items.at(id).data) == false)) {
                            continue;
                        }

                        const auto & item = items.at(id);

                        bool isHovered = false;

                        if (std::holds_alternative<HN::Story>(item.data)) {
                            const HN::Story & story = std::get<HN::Story>(item.data);

                            auto p0 = ImGui::GetCursorScreenPos();

                            // draw text to be able to calculate the final text size
                            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvailWidth());
                            ImGui::Text("%2d.", i + 1);
                            ImGui::SameLine();
                            ImGui::Text("%s", story.title.c_str());

                            // draw hovered story highlight
                            if (windowId == stateUI.hoveredWindowId && i == window.hoveredStoryId) {
                                auto col0 = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                                auto col1 = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                                ImGui::PushStyleColor(ImGuiCol_Text, col1);
                                ImGui::PushStyleColor(ImGuiCol_TextDisabled, col0);

                                auto p1 = ImGui::GetCursorScreenPos();
                                p1.y -= 1;
                                if (p1.y > p0.y) {
                                    p1.x += ImGui::GetContentRegionAvailWidth() - 1;
                                } else {
                                    p1.x += ImGui::CalcTextSize(story.title.c_str()).x + 5;
                                }

                                // highlight rectangle
                                ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, ImGui::GetColorU32(col0));

                                if (ImGui::IsKeyPressed('o', false) || ImGui::IsKeyPressed('O', false)) {
                                    openInBrowser(story.url);
                                }
                            }

                            // go back to original position and redraw text over the highlight
                            ImGui::SetCursorScreenPos(p0);

                            ImGui::Text("%2d.", i + 1);
                            isHovered |= ImGui::IsItemHovered();
                            ImGui::SameLine();
                            ImGui::Text("%s", story.title.c_str());
                            isHovered |= ImGui::IsItemHovered();

                            ImGui::PopTextWrapPos();
                            ImGui::SameLine();

                            if (windowId == stateUI.hoveredWindowId && i == window.hoveredStoryId) {
                                ImGui::PopStyleColor(2);
                            }

                            ImGui::TextDisabled(" (%s)", story.domain.c_str());

                            if (stateUI.storyListMode != UI::StoryListMode::Micro) {
                                ImGui::TextDisabled("    %d points by %s %s ago | %d comments", story.score, story.by.c_str(), stateHN.timeSince(story.time).c_str(), story.descendants);
                                isHovered |= ImGui::IsItemHovered();
                            }
                        } else if (std::holds_alternative<HN::Job>(item.data)) {
                            const HN::Job & job = std::get<HN::Job>(item.data);

                            if (windowId == stateUI.hoveredWindowId && i == window.hoveredStoryId) {
                                auto col0 = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                                auto col1 = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                                ImGui::PushStyleColor(ImGuiCol_Text, col1);
                                ImGui::PushStyleColor(ImGuiCol_TextDisabled, col0);

                                auto p0 = ImGui::GetCursorScreenPos();
                                p0.x += 1;
                                auto p1 = p0;
                                p1.x += ImGui::CalcTextSize(job.title.c_str()).x + 4;

                                ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, ImGui::GetColorU32(col0));

                                if (ImGui::IsKeyPressed('o', false) || ImGui::IsKeyPressed('O', false)) {
                                    openInBrowser(job.url);
                                }
                            }

                            ImGui::Text("%2d.", i + 1);
                            isHovered |= ImGui::IsItemHovered();
                            ImGui::SameLine();
                            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvailWidth());
                            ImGui::Text("%s", job.title.c_str());
                            isHovered |= ImGui::IsItemHovered();
                            ImGui::PopTextWrapPos();
                            ImGui::SameLine();

                            if (windowId == stateUI.hoveredWindowId && i == window.hoveredStoryId) {
                                ImGui::PopStyleColor(2);
                            }

                            ImGui::TextDisabled(" (%s)", job.domain.c_str());

                            if (stateUI.storyListMode != UI::StoryListMode::Micro) {
                                ImGui::TextDisabled("    %d points by %s %s ago", job.score, job.by.c_str(), stateHN.timeSince(job.time).c_str());
                                isHovered |= ImGui::IsItemHovered();
                            }
                        }

                        if (isHovered) {
                            stateUI.hoveredWindowId = windowId;
                            window.hoveredStoryId = i;
                        }

                        if (stateUI.storyListMode == UI::StoryListMode::Spread) {
                            ImGui::Text("%s", "");
                        }

                        if (ImGui::GetCursorScreenPos().y + 3 > ImGui::GetWindowSize().y) {
                            window.maxStories = i + 1;
                            break;
                        } else {
                            if ((i == window.maxStories - 1) && (ImGui::GetCursorScreenPos().y + 2 < ImGui::GetWindowSize().y)) {
                                ++window.maxStories;
                            }
                        }
                    }

                    if (windowId == stateUI.hoveredWindowId) {
                        if (ImGui::IsMouseDoubleClicked(0) ||
                            ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter], false)) {
                            if (stateUI.showHelpModal == false && window.hoveredStoryId < (int) storyIds.size()) {
                                window.showComments = true;
                                window.selectedStoryId = storyIds[window.hoveredStoryId];
                            }
                        }

                        if (ImGui::IsKeyPressed('r', false) && window.hoveredStoryId < (int) storyIds.size()) {
                            toUpdate.push_back(storyIds[window.hoveredStoryId]);
                        }

                        if (ImGui::IsKeyPressed('k', true) ||
                            ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_UpArrow], true)) {
                            window.hoveredStoryId = std::max(0, window.hoveredStoryId - 1);
                        }

                        if (ImGui::IsKeyPressed('j', true) ||
                            ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_DownArrow], true)) {
                            window.hoveredStoryId = std::max(0, std::min(nShow - 1, window.hoveredStoryId + 1));
                        }

                        if (ImGui::IsKeyPressed('g', true)) {
                            window.hoveredStoryId = 0;
                        }

                        if (ImGui::IsKeyPressed('G', true)) {
                            window.hoveredStoryId = nShow - 1;
                        }

                        if (ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Tab])) {
                            stateUI.windows[stateUI.hoveredWindowId].content =
                                (UI::WindowContent) ((int(stateUI.windows[stateUI.hoveredWindowId].content) + 1)%int(UI::WindowContent::Count));
                        }
                    }
                } else {
                    if (std::holds_alternative<HN::Story>(items.at(window.selectedStoryId).data) == false) {
                        window.showComments = false;
                    } else {
                        const auto & story = std::get<HN::Story>(items.at(window.selectedStoryId).data);

                        toRefresh.push_back(story.id);

                        ImGui::Text("%s", story.title.c_str());
                        ImGui::TextDisabled("%d points by %s %s ago | %d comments", story.score, story.by.c_str(), stateHN.timeSince(story.time).c_str(), story.descendants);
                        if (story.text.empty() == false) {
                            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvailWidth());
                            ImGui::Text("%s", story.text.c_str());
                            ImGui::PopTextWrapPos();
                        }

                        ImGui::Text("%s", "");

                        int curCommentId = 0;
                        bool forceUpdate = false;

                        std::function<void(const HN::ItemIds & commentIds, int indent)> renderComments;
                        renderComments = [&](const HN::ItemIds & commentIds, int indent) {
                            const int nComments = commentIds.size();
                            for (int i = 0; i < nComments; ++i) {
                                const auto & id = commentIds[i];

                                if (forceUpdate) {
                                    toUpdate.push_back(id);
                                }

                                toRefresh.push_back(commentIds[i]);
                                if (items.find(id) == items.end() || std::holds_alternative<HN::Comment>(items.at(id).data) == false) {
                                    continue;
                                }

                                const auto & item = items.at(id);
                                const auto & comment = std::get<HN::Comment>(item.data);

                                char header[128];
                                snprintf(header, 128, "%*s %s %s ago [%s]", indent, "", comment.by.c_str(), stateHN.timeSince(comment.time).c_str(), stateUI.collapsed[id] ? "+" : "-");

                                if (windowId == stateUI.hoveredWindowId && curCommentId == window.hoveredCommentId) {
                                    auto col0 = ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                                    auto col1 = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                                    ImGui::PushStyleColor(ImGuiCol_TextDisabled, col1);

                                    auto p0 = ImGui::GetCursorScreenPos();
                                    p0.x += 1 + indent;
                                    auto p1 = p0;
                                    p1.x += ImGui::CalcTextSize(header).x - indent;

                                    ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, ImGui::GetColorU32(col0));

                                    if (ImGui::GetCursorScreenPos().y < 4) {
                                        ImGui::SetScrollHereY(0.0f);
                                    }
                                }

                                bool isHovered = false;

                                ImGui::TextDisabled("%s", header);
                                isHovered |= ImGui::IsItemHovered();

                                if (windowId == stateUI.hoveredWindowId && curCommentId == window.hoveredCommentId) {
                                    ImGui::PopStyleColor(1);
                                }

                                if (stateUI.collapsed[id] == false) {
                                    ImGui::PushTextWrapPos(ImGui::GetContentRegionAvailWidth());
                                    ImGui::Text("%*s", indent, "");
                                    ImGui::SameLine();
                                    ImGui::Text("%s", comment.text.c_str());
                                    isHovered |= ImGui::IsItemHovered();
                                    ImGui::PopTextWrapPos();
                                }

                                if (windowId == stateUI.hoveredWindowId && curCommentId == window.hoveredCommentId) {
                                    if (ImGui::GetCursorScreenPos().y > ImGui::GetWindowSize().y + 4) {
                                        ImGui::SetScrollHereY(1.0f);
                                    }

                                    if (ImGui::IsMouseDoubleClicked(0) ||
                                        ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter], false)) {
                                        stateUI.collapsed[id] = !stateUI.collapsed[id];
                                    }
                                }

                                if (isHovered) {
                                    window.hoveredCommentId = curCommentId;
                                }

                                ImGui::Text("%s", "");

                                ++curCommentId;

                                if (stateUI.collapsed[id] == false ) {
                                    if (comment.kids.size() > 0) {
                                        renderComments(comment.kids, indent + 1);
                                    }
                                } else {
                                    //curCommentId += comment.kids.size();
                                }
                            }
                        };

                        if (windowId == stateUI.hoveredWindowId) {
                            if (ImGui::IsKeyPressed('r', false)) {
                                toUpdate.push_back(story.id);
                                forceUpdate = true;
                            }
                        }

                        ImGui::BeginChild("##comments", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);
                        renderComments(story.kids, 0);
                        ImGui::EndChild();

                        if (windowId == stateUI.hoveredWindowId) {
                            if (ImGui::IsKeyPressed('k', true) ||
                                ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_UpArrow], true)) {
                                window.hoveredCommentId = std::max(0, window.hoveredCommentId - 1);
                            }

                            if (ImGui::IsKeyPressed('j', true) ||
                                ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_DownArrow], true)) {
                                window.hoveredCommentId = std::min(curCommentId - 1, window.hoveredCommentId + 1);
                            }

                            if (ImGui::IsKeyPressed('g', true)) {
                                window.hoveredCommentId = 0;
                            }

                            if (ImGui::IsKeyPressed('G', true)) {
                                window.hoveredCommentId = curCommentId - 1;
                            }

                            if (ImGui::IsKeyPressed('o', false) || ImGui::IsKeyPressed('O', false)) {
                                openInBrowser((std::string("https://news.ycombinator.com/item?id=") + std::to_string(story.id)).c_str());
                            }

                            if (ImGui::IsMouseClicked(1) ||
                                ImGui::IsMouseClicked(2) ||
                                ImGui::IsKeyPressed('q', false) ||
                                ImGui::IsKeyPressed('b', false) ||
                                ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Backspace], false)) {
                                window.showComments = false;
                            }
                        }

                        window.hoveredCommentId = std::min(curCommentId - 1, window.hoveredCommentId);
                    }
                }

                ImGui::End();
            }

            if (ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow], true)) {
                stateUI.hoveredWindowId = std::max(0, stateUI.hoveredWindowId - 1);
            }

            if (ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_RightArrow], true)) {
                stateUI.hoveredWindowId = std::min(stateUI.nWindows - 1, stateUI.hoveredWindowId + 1);
            }

            if (stateUI.showStatusWindow) {
                {
                    auto wSize = ImGui::GetIO().DisplaySize;
                    ImGui::SetNextWindowPos(ImVec2(0, wSize.y - stateUI.statusWindowHeight), ImGuiCond_Always);
                    ImGui::SetNextWindowSize(ImVec2(wSize.x, stateUI.statusWindowHeight), ImGuiCond_Always);
                }
                snprintf(stateUI.statusWindowHeader, 512, "Status | Story List Mode : %d |", (int) (stateUI.storyListMode));
                ImGui::Begin(stateUI.statusWindowHeader, nullptr,
                             ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove);
                ImGui::Text(" API requests     : %d / %d B (next update in %d s)", stateHN.nFetches, (int) stateHN.totalBytesDownloaded, stateHN.nextUpdate);
                ImGui::Text(" Last API request : %s", stateHN.curURI);
                ImGui::Text(" Source code      : https://github.com/ggerganov/hnterm");
                ImGui::End();
            }

            if (ImGui::IsKeyPressed('s', false)) {
                stateUI.showStatusWindow = !stateUI.showStatusWindow;
            }

            if (ImGui::IsKeyPressed('1', false)) {
                stateUI.nWindows = 1;
            }

            if (ImGui::IsKeyPressed('2', false)) {
                stateUI.nWindows = 2;
            }

            if (ImGui::IsKeyPressed('3', false)) {
                stateUI.nWindows = 3;
            }

            if (ImGui::IsKeyPressed('v', false)) {
                stateUI.storyListMode = (UI::StoryListMode)(((int)(stateUI.storyListMode) + 1)%((int)(UI::StoryListMode::COUNT)));
            }

            if (ImGui::IsKeyPressed('c', false)) {
                stateUI.changeColorScheme();
            }

            if (ImGui::IsKeyPressed('Q', false)) {
                return false;
            }

            stateUI.hoveredWindowId = std::min(stateUI.nWindows - 1, stateUI.hoveredWindowId);

            if (stateUI.showHelpWelcome || (stateUI.showHelpModal == false && (ImGui::IsKeyReleased('h') || ImGui::IsKeyReleased('H')))) {
                stateUI.showHelpWelcome = false;
                ImGui::OpenPopup("###Help");
                auto col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                col.w *= 0.9;
                ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = col;
            }

            if (ImGui::BeginPopupModal("HNTerm v0.3###Help", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
#ifdef __EMSCRIPTEN__
                ImGui::Text(" ");
                ImGui::Text("---------------------------------------------");
                ImGui::Text("Emscripten port of HNTerm");
                ImGui::Text("This demo is not suitable for mobile devices!");
                ImGui::Text("---------------------------------------------");
#endif
                ImGui::Text(" ");
                ImGui::Text("Interactive browsing of https://news.ycombinator.com/");
                ImGui::Text("Content is automatically updated - no need to refresh ");
                ImGui::Text(" ");
                ImGui::Text("    h/H         - toggle Help window    ");
                ImGui::Text("    s           - toggle Status window    ");
                ImGui::Text("    g           - go to top    ");
                ImGui::Text("    G           - go to end    ");
                ImGui::Text("    o/O         - open in browser    ");
                ImGui::Text("    up/down/j/k - navigate items    ");
                ImGui::Text("    left/right  - navigate windows    ");
                ImGui::Text("    Tab         - change current window content    ");
                ImGui::Text("    1/2/3       - change number of windows    ");
                ImGui::Text("    q/b/bkspc   - close comments    ");
                ImGui::Text("    v           - toggle story view mode    ");
                ImGui::Text("    r           - force refresh story    ");
                ImGui::Text("    c           - change colors    ");
                ImGui::Text("    Q           - quit    ");
                ImGui::Text(" ");

                if (stateUI.showHelpModal) {
                    for (int i = 0; i < 512; ++i) {
                        if (ImGui::IsKeyReleased(i) || ImGui::IsMouseDown(0)) {
                            ImGui::CloseCurrentPopup();
                            auto col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                            col.w = 1.0;
                            ImGui::GetStyle().Colors[ImGuiCol_WindowBg] = col;
                            stateUI.showHelpModal = false;
                            break;
                        }
                    }
                } else {
                    stateUI.showHelpModal = true;
                }

                ImGui::EndPopup();
            }

            ImGui::Render();

            ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), g_screen);

#ifndef __EMSCRIPTEN__
            ImTui_ImplNcurses_DrawScreen(isActive);
#endif

            stateHN.forceUpdate(toUpdate);
            g_updated = stateHN.update(toRefresh);

            return true;
        }
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char ** argv) {
#ifndef __EMSCRIPTEN__
    auto argm = parseCmdArguments(argc, argv);
    int mouseSupport = argm.find("--mouse") != argm.end() || argm.find("m") != argm.end();
    if (argm.find("--help") != argm.end() || argm.find("-h") != argm.end()) {
        printf("Usage: hnterm [-m] [-h]\n");
        printf("    -m, --mouse : ncurses mouse support\n");
        printf("    -h, --help  : print this help\n");
        return -1;
    }
#endif

    if (hnInit() == false) {
        fprintf(stderr, "Failed to initialize. Aborting\n");
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

#ifdef __EMSCRIPTEN__
    g_screen = ImTui_ImplEmscripten_Init(true);
#else
    // when no changes occured - limit frame rate to 3.0 fps to save CPU
    g_screen = ImTui_ImplNcurses_Init(mouseSupport != 0, 60.0, 3.0);
#endif
    ImTui_ImplText_Init();

    stateUI.changeColorScheme(false);

#ifndef __EMSCRIPTEN__
    while (true) {
        if (render_frame() == false) break;
    }

    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();
    hnFree();
#endif

    return 0;
}

