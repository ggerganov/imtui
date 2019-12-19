/*! \file main.cpp
 *  \brief HNTerm - browse Hacker News in the terminal
 */

#include "imtui/imtui.h"

#include "json.h"
#include "imtui-common.h"

#ifdef __EMSCRIPTEN__

#define ENABLE_API_CACHE 0

#include "imtui/imtui-impl-emscripten.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#else

#define ENABLE_API_CACHE 0
#define EMSCRIPTEN_KEEPALIVE

#include "imtui/imtui-impl-ncurses.h"

#endif

#include <map>
#include <vector>
#include <string>
#include <variant>
#include <functional>

// tmp
#include <fstream>

// global vars
char g_curURI[512];
int g_nfetches = 0;
int g_nextUpdate = 0;
uint64_t g_totalBytesDownloaded = 0;

// screen buffer
ImTui::TScreen screen;

// platform specific functions
extern bool hnInit();
extern void hnFree();
extern int openInBrowser(std::string uri);
extern void requestJSONForURI_impl(std::string uri);
extern std::string getJSONForURI_impl(const std::string & uri);
extern void updateRequests_impl();
extern uint64_t t_s();

// helper functions
namespace {

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

void replaceAll(std::string & s, const std::string & search, const std::string & replace) {
    for (size_t pos = 0; ; pos += replace.length()) {
        pos = s.find(search, pos);
        if (pos == std::string::npos) break;
        s.erase(pos, search.length());
        s.insert(pos, replace);
    }
}

inline std::string timeSince(uint64_t t) {
    auto delta = t_s() - t;
    if (delta < 60) return std::to_string(delta) + " seconds";
    if (delta < 3600) return std::to_string(delta/60) + " minutes";
    if (delta < 24*3600) return std::to_string(delta/3600) + " hours";
    return std::to_string(delta/24/3600) + " days";
}

void requestJSONForURI(std::string uri) {
    ++g_nfetches;
    snprintf(g_curURI, 512, "%s", uri.c_str());

    requestJSONForURI_impl(std::move(uri));
}

std::string getJSONForURI(const std::string & uri) {
    auto response_string = getJSONForURI_impl(uri);
    if (response_string == "") return "";

#ifdef ENABLE_API_CACHE
    std::string fname = "cache-" + uri;
    for (auto & ch : fname) {
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9')) {
            continue;
        }
        ch = '-';
    }

    //if (ENABLE_API_CACHE) {
    //    std::ifstream fin(fname);
    //    if (fin.is_open() && fin.good()) {
    //        std::string str((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
    //        fin.close();

    //        return str;
    //    }
    //}

    if (ENABLE_API_CACHE) {
        std::ofstream fout(fname);
        fout.write(response_string.c_str(), response_string.size());
        fout.close();
    }
#endif

    g_totalBytesDownloaded += response_string.size();

    return response_string;
}

}

namespace HN {

using URI = std::string;
using ItemId = int;
using ItemIds = std::vector<ItemId>;
using ItemData = std::map<std::string, std::string>;

static const std::string kCmdPrefix = "curl -s -k ";

static const URI kAPIItem = "https://hacker-news.firebaseio.com/v0/item/";
static const URI kAPITopStories = "https://hacker-news.firebaseio.com/v0/topstories.json";
static const URI kAPINewStories = "https://hacker-news.firebaseio.com/v0/newstories.json";
//static const URI kAPIBestStories = "https://hacker-news.firebaseio.com/v0/beststories.json";
static const URI kAPIAskStories = "https://hacker-news.firebaseio.com/v0/askstories.json";
static const URI kAPIShowStories = "https://hacker-news.firebaseio.com/v0/showstories.json";
static const URI kAPIJobStories = "https://hacker-news.firebaseio.com/v0/jobstories.json";
static const URI kAPIUpdates = "https://hacker-news.firebaseio.com/v0/updates.json";

struct Story {
    std::string by = "";
    int descendants = 0;
    ItemId id = 0;
    ItemIds kids;
    int score = 0;
    uint64_t time = 0;
    std::string text = "";
    std::string title = "";
    std::string url = "";
    std::string domain = "";
};

struct Comment {
    std::string by = "";
    ItemId id = 0;
    ItemIds kids;
    ItemId parent = 0;
    std::string text = "";
    uint64_t time = 0;
};

struct Job {
    std::string by = "";
    ItemId id = 0;
    int score = 0;
    uint64_t time = 0;
    std::string title = "";
    std::string url = "";
    std::string domain = "";
};

struct Poll {};
struct PollOpt {};

enum class ItemType : int {
    Unknown,
    Story,
    Comment,
    Job,
    Poll,
    PollOpt,
};

struct Item {
    ItemType type = ItemType::Unknown;

    bool needUpdate = true;
    bool needRequest = true;

    std::variant<Story, Comment, Job, Poll, PollOpt> data;
};

// todo : optimize this
std::string parseHTML(std::string str) {
    ::replaceAll(str, "<p>", "\n");

    std::string res;
    bool inTag = false;
    for (auto & ch : str) {
        if (ch == '<') {
            inTag = true;
        } else if (ch == '>') {
            inTag = false;
        } else {
            if (inTag == false) {
                res += ch;
            }
        }
    }

    ::replaceAll(res, "&#x2F;", "/");
    ::replaceAll(res, "&#x27;", "'");
    ::replaceAll(res, "&gt;", ">");
    ::replaceAll(res, "–", "-");
    ::replaceAll(res, "“", "\"");
    ::replaceAll(res, "”", "\"");
    ::replaceAll(res, "‘", "'");
    ::replaceAll(res, "’", "'");
    ::replaceAll(res, "„", "'");
    ::replaceAll(res, "&quot;", "\"");
    ::replaceAll(res, "&amp;", "&");
    ::replaceAll(res, "—", "-");

    return res;
}

URI getItemURI(ItemId id) {
    return kAPIItem + std::to_string(id) + ".json";
}

ItemType getItemType(const ItemData & itemData) {
    if (itemData.find("type") == itemData.end()) return ItemType::Unknown;

    auto strType = itemData.at("type");

    if (strType == "story") return ItemType::Story;
    if (strType == "comment") return ItemType::Comment;
    if (strType == "job") return ItemType::Job;
    if (strType == "poll") return ItemType::Poll;
    if (strType == "pollopt") return ItemType::PollOpt;

    return ItemType::Unknown;
}

void parseStory(const ItemData & data, Story & res) {
    try {
        res.by = data.at("by");
    } catch (...) {
        res.by = "[deleted]";
    }
    try {
        res.descendants = std::stoi(data.at("descendants"));
    } catch (...) {
        res.descendants = 0;
    }
    try {
        res.id = std::stoi(data.at("id"));
    } catch (...) {
        res.id = 0;
    }
    try {
        res.kids = JSON::parseIntArray(data.at("kids"));
    } catch (...) {
        res.kids.clear();
    }
    try {
        res.score = std::stoi(data.at("score"));
    } catch (...) {
        res.score = 0;
    }
    try {
        res.time = std::stoll(data.at("time"));
    } catch (...) {
        res.time = 0;
    }
    try {
        res.text = parseHTML(data.at("text"));
    } catch (...) {
        res.text = "";
    }
    try {
        res.title = parseHTML(data.at("title"));
    } catch (...) {
        res.title = "";
    }
    try {
        res.url = data.at("url");
        res.domain = "";
        int slash = 0;
        for (auto & ch : res.url) {
            if (ch == '/') {
                ++slash;
                continue;
            }
            if (slash > 2) break;
            if (slash > 1) res.domain += ch;
        }
    } catch (...) {
        res.url = "";
        res.domain = "";
    }
}

void parseComment(const ItemData & data, Comment & res) {
    try {
        res.by = data.at("by");
    } catch (...) {
        res.by = "[deleted]";
    }
    try {
        res.id = std::stoi(data.at("id"));
    } catch (...) {
        res.id = 0;
    }
    try {
        res.kids = JSON::parseIntArray(data.at("kids"));
    } catch (...) {
        res.kids.clear();
    }
    try {
        res.parent = std::stoi(data.at("parent"));
    } catch (...) {
        res.parent = 0;
    }
    try {
        res.text = parseHTML(data.at("text"));
    } catch (...) {
        res.text = "";
    }
    try {
        res.time = std::stoll(data.at("time"));
    } catch (...) {
        res.time = 0;
    }
}

void parseJob(const ItemData & data, Job & res) {
    try {
        res.by = data.at("by");
    } catch (...) {
        res.by = "[deleted]";
    }
    try {
        res.id = std::stoi(data.at("id"));
    } catch (...) {
        res.id = 0;
    }
    try {
        res.score = std::stoi(data.at("score"));
    } catch (...) {
        res.score = 0;
    }
    try {
        res.time = std::stoll(data.at("time"));
    } catch (...) {
        res.time = 0;
    }
    try {
        res.title = parseHTML(data.at("title"));
    } catch (...) {
        res.title = "";
    }
    try {
        res.url = data.at("url");
        res.domain = "";
        int slash = 0;
        for (auto & ch : res.url) {
            if (ch == '/') {
                ++slash;
                continue;
            }
            if (slash > 2) break;
            if (slash > 1) res.domain += ch;
        }
    } catch (...) {
        res.url = "";
        res.domain = "";
    }
}

ItemIds getStoriesIds(const URI & uri) {
    return JSON::parseIntArray(getJSONForURI(uri));
}

ItemIds getChangedItemsIds() {
    auto data = JSON::parseJSONMap(getJSONForURI(HN::kAPIUpdates));
    return JSON::parseIntArray(data["items"]);
}

struct State {
    bool timeout(uint64_t now, uint64_t last) {
        return now - last > 30;
    }

    void update(const ItemIds & toRefresh) {
        auto now = ::t_s();

        if (timeout(now, lastStoriesPoll_s)) {
            ::requestJSONForURI(HN::kAPITopStories);
            //::requestJSONForURI(HN::kAPIBestStories);
            ::requestJSONForURI(HN::kAPIShowStories);
            ::requestJSONForURI(HN::kAPIAskStories);
            ::requestJSONForURI(HN::kAPINewStories);

            lastStoriesPoll_s = ::t_s();
        } else {
            g_nextUpdate = lastStoriesPoll_s + 30 - now;
        }

        if (timeout(now, lastUpdatePoll_s)) {
            ::requestJSONForURI(HN::kAPIUpdates);

            lastUpdatePoll_s = ::t_s();
        }

        {
            {
                auto ids = HN::getStoriesIds(HN::kAPITopStories);
                if (ids.empty() == false) {
                    idsTop = std::move(ids);
                }
            }

            //{
            //    auto ids = HN::getStoriesIds(HN::kAPIBestStories);
            //    if (ids.empty() == false) {
            //        idsBest = std::move(ids);
            //    }
            //}

            {
                auto ids = HN::getStoriesIds(HN::kAPIShowStories);
                if (ids.empty() == false) {
                    idsShow = std::move(ids);
                }
            }

            {
                auto ids = HN::getStoriesIds(HN::kAPIAskStories);
                if (ids.empty() == false) {
                    idsAsk = std::move(ids);
                }
            }

            {
                auto ids = HN::getStoriesIds(HN::kAPINewStories);
                if (ids.empty() == false) {
                    idsNew = std::move(ids);
                }
            }
        }

        {
            auto ids = HN::getChangedItemsIds();
            for (auto id : ids) {
                if (items.find(id) == items.end()) continue;
                items[id].needUpdate = true;
                items[id].needRequest = true;
            }
        }

        for (auto id : toRefresh) {
            if (items[id].needRequest == false) continue;

            ::requestJSONForURI(getItemURI(id));
            items[id].needRequest = false;
        }

        for (auto id : toRefresh) {
            if (items[id].needUpdate == false) continue;

            const auto json = getJSONForURI(getItemURI(id));
            if (json == "") continue;

            const auto data = JSON::parseJSONMap(json);
            const auto type = getItemType(data);
            auto & item = items[id];
            switch (type) {
                case ItemType::Unknown:
                    {
                    }
                    break;
                case ItemType::Story:
                    {
                        if (std::holds_alternative<Story>(item.data) == false) {
                            item.data = Story();
                        }

                        Story & story = std::get<Story>(item.data);
                        if (item.needUpdate) {
                            parseStory(data, story);
                            item.needUpdate = false;
                        }
                    }
                    break;
                case ItemType::Comment:
                    {
                        if (std::holds_alternative<Comment>(item.data) == false) {
                            item.data = Comment();
                        }

                        Comment & story = std::get<Comment>(item.data);
                        if (item.needUpdate) {
                            parseComment(data, story);
                            item.needUpdate = false;
                        }
                    }
                    break;
                case ItemType::Job:
                    {
                        if (std::holds_alternative<Job>(item.data) == false) {
                            item.data = Job();
                        }

                        Job & job = std::get<Job>(item.data);
                        if (item.needUpdate) {
                            parseJob(data, job);
                            item.needUpdate = false;
                        }
                    }
                    break;
                case ItemType::Poll:
                    {
                        // temp
                        item.needUpdate = false;
                    }
                    break;
                case ItemType::PollOpt:
                    {
                        // temp
                        item.needUpdate = false;
                    }
                    break;
            };

            break;
        }
    }

    ItemIds idsTop;
    //ItemIds idsBest;
    ItemIds idsShow;
    ItemIds idsAsk;
    ItemIds idsNew;

    std::map<ItemId, Item> items;

    uint64_t lastUpdatePoll_s = 0;
    uint64_t lastStoriesPoll_s = 0;
};

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

#ifndef __EMSCRIPTEN__
VSync vsync;
#endif

// HackerNews content
HN::State stateHN;

// items that need to be updated
HN::ItemIds toRefresh;

// UI state
UI::State stateUI;

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
        bool render_frame() {
            stateHN.update(toRefresh);
            updateRequests_impl();

            toRefresh.clear();

#ifdef __EMSCRIPTEN__
            ImTui_ImplEmscripten_NewFrame();
#else
            ImTui_ImplNcurses_NewFrame();
#endif
            ImTui_ImplText_NewFrame();

#ifndef __EMSCRIPTEN__
            ImGui::GetIO().DeltaTime = vsync.delta_s();
#endif

            if (ImGui::GetIO().DisplaySize.x < 50) {
                stateUI.nWindows = 1;
            }

            ImGui::NewFrame();

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

                            if (windowId == stateUI.hoveredWindowId && i == window.hoveredStoryId) {
                                auto col0 = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                                auto col1 = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                                ImGui::PushStyleColor(ImGuiCol_Text, col1);
                                ImGui::PushStyleColor(ImGuiCol_TextDisabled, col0);

                                auto p0 = ImGui::GetCursorScreenPos();
                                p0.x += 1;
                                auto p1 = p0;
                                p1.x += ImGui::CalcTextSize(story.title.c_str()).x + 4;

                                ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, ImGui::GetColorU32(col0));

                                if (ImGui::IsKeyPressed('o', false) || ImGui::IsKeyPressed('O', false)) {
                                    openInBrowser(story.url);
                                }
                            }

                            ImGui::Text("%2d.", i + 1);
                            isHovered |= ImGui::IsItemHovered();
                            ImGui::SameLine();
                            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvailWidth());
                            ImGui::Text("%s", story.title.c_str());
                            isHovered |= ImGui::IsItemHovered();
                            ImGui::PopTextWrapPos();
                            ImGui::SameLine();

                            if (windowId == stateUI.hoveredWindowId && i == window.hoveredStoryId) {
                                ImGui::PopStyleColor(2);
                            }

                            ImGui::TextDisabled(" (%s)", story.domain.c_str());

                            if (stateUI.storyListMode != UI::StoryListMode::Micro) {
                                ImGui::TextDisabled("    %d points by %s %s ago | %d comments", story.score, story.by.c_str(), ::timeSince(story.time).c_str(), story.descendants);
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
                                ImGui::TextDisabled("    %d points by %s %s ago", job.score, job.by.c_str(), ::timeSince(job.time).c_str());
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
                            if (stateUI.showHelpModal == false) {
                                window.showComments = true;
                                window.selectedStoryId = storyIds[window.hoveredStoryId];
                            }
                        }

                        if (ImGui::IsKeyPressed('k', true) ||
                            ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_UpArrow], true)) {
                            window.hoveredStoryId = std::max(0, window.hoveredStoryId - 1);
                        }

                        if (ImGui::IsKeyPressed('j', true) ||
                            ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_DownArrow], true)) {
                            window.hoveredStoryId = std::min(nShow - 1, window.hoveredStoryId + 1);
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
                        ImGui::TextDisabled("%d points by %s %s ago | %d comments", story.score, story.by.c_str(), ::timeSince(story.time).c_str(), story.descendants);
                        if (story.text.empty() == false) {
                            ImGui::PushTextWrapPos(ImGui::GetContentRegionAvailWidth());
                            ImGui::Text("%s", story.text.c_str());
                            ImGui::PopTextWrapPos();
                        }

                        ImGui::Text("%s", "");

                        int curCommentId = 0;

                        std::function<void(const HN::ItemIds & commentIds, int indent)> renderComments;
                        renderComments = [&](const HN::ItemIds & commentIds, int indent) {
                            const int nComments = commentIds.size();
                            for (int i = 0; i < nComments; ++i) {
                                const auto & id = commentIds[i];

                                toRefresh.push_back(commentIds[i]);
                                if (items.find(id) == items.end() || std::holds_alternative<HN::Comment>(items.at(id).data) == false) {
                                    continue;
                                }

                                const auto & item = items.at(id);
                                const auto & comment = std::get<HN::Comment>(item.data);

                                static char header[128];
                                snprintf(header, 128, "%*s %s %s ago [%s]", indent, "", comment.by.c_str(), ::timeSince(comment.time).c_str(), stateUI.collapsed[id] ? "+" : "-");

                                if (windowId == stateUI.hoveredWindowId && curCommentId == window.hoveredCommentId) {
                                    auto col0 = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                                    ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

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

                                if (stateUI.collapsed[id] == false ) {
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
                ImGui::Text(" API requests     : %d / %d B (next update in %d s)", g_nfetches, (int) g_totalBytesDownloaded, g_nextUpdate);
                ImGui::Text(" Last API request : %s", g_curURI);
                ImGui::Text(" Source code      : https://github.com/ggerganov/imtui/tree/master/examples/hnterm");
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

            if (ImGui::IsKeyPressed('c', false)) {
                stateUI.storyListMode = (UI::StoryListMode)(((int)(stateUI.storyListMode) + 1)%((int)(UI::StoryListMode::COUNT)));
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

            if (ImGui::BeginPopupModal("HNTerm v0.1###Help", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
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
                ImGui::Text("    c           - toggle story mode    ");
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

            ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);

#ifndef __EMSCRIPTEN__
            ImTui_ImplNcurses_DrawScreen(screen);
            vsync.wait();
#endif

            return true;
        }
}

int main(int argc, char ** argv) {
#ifndef __EMSCRIPTEN__
    auto argm = parseCmdArguments(argc, argv);
    int mouseSupport = argm.find("--mouse") != argm.end() || argm.find("m") != argm.end();
    if (argm.find("--help") != argm.end() || argm.find("-h") != argm.end()) {
        printf("Usage: %s [-m] [-h]\n", argv[0]);
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
    ImTui_ImplEmscripten_Init(true);
#else
    ImTui_ImplNcurses_Init(mouseSupport != 0);
#endif
    ImTui_ImplText_Init();

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.96f, 0.96f, 0.94f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(1.00f, 0.40f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(1.00f, 0.40f, 0.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.69f, 0.25f, 0.00f, 1.00f);
    colors[ImGuiCol_ChildBg]                = ImVec4(0.96f, 0.96f, 0.94f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.96f, 0.96f, 0.94f, 1.00f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

#ifndef __EMSCRIPTEN__
    while (true) {
        if (render_frame() == false) break;
    }
#endif

    ImTui_ImplText_Shutdown();
#ifdef __EMSCRIPTEN__
    ImTui_ImplEmscripten_Shutdown();
#else
    ImTui_ImplNcurses_Shutdown();
#endif

    hnFree();

    return 0;
}

