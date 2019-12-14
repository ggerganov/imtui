#include "imtui/imtui.h"

#include "imtui/imtui-impl-ncurses.h"

#include "imtui-common.h"

#include <curl/curl.h>

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include <thread>
#include <variant>
#include <functional>

// tmp
#include <fstream>

std::mutex g_mutex;
bool g_keepRunning = true;
uint64_t g_totalBytesDownloaded = 0;
int g_nfetches = 0;
int g_nextUpdate = 0;
char g_curURI[512];
void * g_curl = nullptr;

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

inline uint64_t t_s() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(); // duh ..
}

inline std::string timeSince(uint64_t t) {
    auto delta = t_s() - t;
    if (delta < 60) return std::to_string(delta) + " seconds";
    if (delta < 3600) return std::to_string(delta/60) + " minutes";
    if (delta < 24*3600) return std::to_string(delta/3600) + " hours";
    return std::to_string(delta/24/3600) + " days";
}

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

bool initCurl() {
    g_curl = curl_easy_init();
    if (g_curl == nullptr) {
        return false;
    }

    curl_easy_setopt(g_curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(g_curl, CURLOPT_USERPWD, "user:pass");
    curl_easy_setopt(g_curl, CURLOPT_USERAGENT, "curl/7.42.0");
    curl_easy_setopt(g_curl, CURLOPT_MAXREDIRS, 50L);
    curl_easy_setopt(g_curl, CURLOPT_TCP_KEEPALIVE, 1L);

    return true;
}

void freeCurl() {
    if (g_curl) {
        curl_easy_cleanup(g_curl);
    }
    g_curl = NULL;
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

struct Job {};
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

    std::variant<Story, Comment> data;
};

ItemIds parseJSONIntArray(const std::string json) {
    ItemIds res;
    if (json[0] != '[') return res;

    int n = json.size();
    int curid = 0;
    for (int i = 1; i < n; ++i) {
        if (json[i] == ' ') continue;
        if (json[i] == ',') {
            res.push_back(curid);
            curid = 0;
            continue;
        }

        if (json[i] == ']') {
            res.push_back(curid);
            break;
        }

        curid = 10*curid + json[i] - 48;
    }

    return res;
}

ItemData parseJSONMap(const std::string json) {
    ItemData res;
    if (json[0] != '{') return res;

    bool hasKey = false;
    bool inToken = false;

    std::string strKey = "";
    std::string strVal = "";

    int n = json.size();
    for (int i = 1; i < n; ++i) {
        if (!inToken) {
            if (json[i] == ' ') continue;
            if (json[i] == '"' && json[i-1] != '\\') {
                inToken = true;
                continue;
            }
        } else {
            if (json[i] == '"' && json[i-1] != '\\') {
                if (hasKey == false) {
                    hasKey = true;
                    ++i;
                    while (json[i] == ' ') ++i;
                    ++i; // :
                    while (json[i] == ' ') ++i;
                    if (json[i] == '[') {
                        while (json[i] != ']') {
                            strVal += json[i++];
                        }
                        strVal += ']';
                        hasKey = false;
                        res[strKey] = strVal;
                        strKey = "";
                        strVal = "";
                    } else if (json[i] != '\"') {
                        while (json[i] != ',' && json[i] != '}') {
                            strVal += json[i++];
                        }
                        hasKey = false;
                        res[strKey] = strVal;
                        strKey = "";
                        strVal = "";
                    } else {
                        inToken = true;
                        continue;
                    }
                } else {
                    hasKey = false;
                    res[strKey] = strVal;
                    strKey = "";
                    strVal = "";
                }
                inToken = false;
                continue;
            }
            if (hasKey == false) {
                strKey += json[i];
            } else {
                strVal += json[i];
            }
        }
    }

    return res;
}

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
    ::replaceAll(res, "&quot;", "\"");
    return res;
}

URI getItemURI(ItemId id) {
    return kAPIItem + std::to_string(id) + ".json";
}

std::string getJSONForURI(URI uri) {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        ++g_nfetches;
        snprintf(g_curURI, 512, "%s", uri.c_str());
    }

    std::string fname = "cache-" + uri;
    for (auto & ch : fname) {
        if ((ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9')) {
            continue;
        }
        ch = '-';
    }

    if (1) {
        std::ifstream fin(fname);
        if (fin.is_open() && fin.good()) {
            //printf("%s\n", uri.c_str());
            std::string str((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
            fin.close();

            return str;
        }
    }

    //printf("Fetching: %s\n", uri.c_str());
    curl_easy_setopt(g_curl, CURLOPT_URL, uri.c_str());

    std::string response_string;
    std::string header_string;
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(g_curl, CURLOPT_HEADERDATA, &header_string);

    char* url;
    long response_code;
    double elapsed;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_getinfo(g_curl, CURLINFO_TOTAL_TIME, &elapsed);
    curl_easy_getinfo(g_curl, CURLINFO_EFFECTIVE_URL, &url);

    curl_easy_perform(g_curl);

    {
        std::ofstream fout(fname);
        fout.write(response_string.c_str(), response_string.size());
        fout.close();
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_totalBytesDownloaded += response_string.size();
    }

    return response_string;
}

ItemData getItemDataFromJSON(std::string json) {
    return parseJSONMap(json);
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
    res.descendants = std::stoi(data.at("descendants"));
    res.id = std::stoi(data.at("id"));
    try {
        res.kids = parseJSONIntArray(data.at("kids"));
    } catch (...) {
        res.kids.clear();
    }
    res.score = std::stoi(data.at("score"));
    res.time = std::stoll(data.at("time"));
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

Story getStory(ItemId id) {
    Story res;

    auto data = parseJSONMap(getJSONForURI(getItemURI(id)));
    parseStory(data, res);

    return res;
}

void parseComment(const ItemData & data, Comment & res) {
    try {
        res.by = data.at("by");
    } catch (...) {
        res.by = "[deleted]";
    }
    res.id = std::stoi(data.at("id"));
    try {
        res.kids = parseJSONIntArray(data.at("kids"));
    } catch (...) {
        res.kids.clear();
    }
    res.parent = std::stoi(data.at("parent"));
    try {
        res.text = parseHTML(data.at("text"));
    } catch (...) {
        res.text = "";
    }
    res.time = std::stoll(data.at("time"));
}

Comment getComment(ItemId id) {
    Comment res;

    auto data = parseJSONMap(getJSONForURI(getItemURI(id)));
    parseComment(data, res);

    return res;
}

ItemIds getStoriesIds(URI uri) {
    std::string json = getJSONForURI(uri);
    return parseJSONIntArray(json);
}

ItemIds getChangedItemsIds() {
    std::string json = getJSONForURI(HN::kAPIUpdates);
    auto data = parseJSONMap(json);
    return parseJSONIntArray(data["items"]);
}

struct State {
    bool timeout(uint64_t now, uint64_t last) {
        return now - last > 30;
    }

    bool update(const ItemIds & toRefresh) {
        auto now = ::t_s();
        bool hasChange = false;

        if (timeout(now, lastStoriesPoll_s)) {
            idsTop = HN::getStoriesIds(HN::kAPITopStories);
            //idsBest = HN::getStoriesIds(HN::kAPIBestStories);
            idsShow = HN::getStoriesIds(HN::kAPIShowStories);
            idsAsk = HN::getStoriesIds(HN::kAPIAskStories);
            idsNew = HN::getStoriesIds(HN::kAPINewStories);

            lastStoriesPoll_s = ::t_s();
            hasChange = true;
        } else {
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                g_nextUpdate = lastStoriesPoll_s + 30 - now;
            }
        }

        if (timeout(now, lastUpdatePoll_s)) {
            auto ids = HN::getChangedItemsIds();
            for (auto id : ids) {
                items[id].needUpdate = true;
            }

            lastUpdatePoll_s = ::t_s();
            hasChange = true;
        }

        for (auto id : toRefresh) {
            if (items[id].needUpdate == false) continue;

            const auto data = getItemDataFromJSON(getJSONForURI(getItemURI(id)));
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
                            hasChange = true;
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
                            hasChange = true;
                            parseComment(data, story);
                            item.needUpdate = false;
                        }
                    }
                    break;
                case ItemType::Job:
                    {
                    }
                    break;
                case ItemType::Poll:
                    {
                    }
                    break;
                case ItemType::PollOpt:
                    {
                    }
                    break;
            };

            break;
        }

        return hasChange;
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
};

struct State {
    int hoveredWindowId = 0;
    int maxStories = 10;

    int statusWindowHeight = 4;
    bool showStatusWindow = true;

    int nWindows = 3;

    std::map<int, bool> collapsed;

    std::array<WindowData, 3> windows { {
        {
            WindowContent::Top,
            false,
            0, 0, 0,
        },
        {
            WindowContent::Show,
            false,
            0, 0, 0,
        },
        {
            WindowContent::New,
            false,
            0, 0, 0,
        },
    } };
};

}

int main(int argc, char ** argv) {
    auto argm = parseCmdArguments(argc, argv);
    int mouseSupport = argm.find("--mouse") != argm.end() || argm.find("m") != argm.end();
    if (argm.find("--help") != argm.end() || argm.find("-h") != argm.end()) {
        printf("Usage: %s [-m] [-h]\n", argv[0]);
        printf("    -m,--mouse : ncurses mouse support\n");
        printf("    -h,--help  : print this help\n");
        return -1;
    }

    if (initCurl() == false) {
        fprintf(stderr, "Failed to initialize CURL. Aborting\n");
        return -1;
    }

    std::mutex mutex;

    bool stateHNUpdated = false;
    HN::State stateHNBG;
    HN::State stateHNBuf;
    HN::State stateHNFG;

    HN::ItemIds toRefreshBG;
    HN::ItemIds toRefreshBuf;
    HN::ItemIds toRefreshFG;

    std::thread workerHN = std::thread([&]() {
        while (g_keepRunning) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                toRefreshBG = toRefreshBuf;
            }

            bool needUpdate = stateHNBG.update(toRefreshBG);

            if (needUpdate) {
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    stateHNUpdated = true;
                    stateHNBuf = stateHNBG;
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    VSync vsync;
    ImTui::TScreen screen;

    ImTui_ImplNcurses_Init(mouseSupport != 0);
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

    bool demo = true;

    UI::State stateUI;

    while (true) {
        if (stateHNUpdated) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                stateHNUpdated = false;
                stateHNFG = stateHNBuf;
            }
        }

        toRefreshFG.clear();

        ImTui_ImplNcurses_NewFrame(screen);
        ImTui_ImplText_NewFrame();

        ImGui::GetIO().DeltaTime = vsync.delta_s();

        ImGui::NewFrame();

        for (int windowId = 0; windowId < stateUI.nWindows; ++windowId) {
            const auto & items = stateHNFG.items;
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
                         ImGuiWindowFlags_NoMove);

            ImGui::Text("%s", "");
            if (window.showComments == false) {
                const auto & storyIds =
                    (window.content == UI::WindowContent::Top) ? stateHNFG.idsTop :
                    (window.content == UI::WindowContent::Show) ? stateHNFG.idsShow :
                    (window.content == UI::WindowContent::New) ? stateHNFG.idsNew :
                    stateHNFG.idsTop;

                int nShow = std::min(stateUI.maxStories, (int) storyIds.size());
                for (int i = 0; i < nShow; ++i) {
                    const auto & id = storyIds[i];

                    toRefreshFG.push_back(id);
                    if (items.find(id) == items.end() || std::holds_alternative<HN::Story>(items.at(id).data) == false) {
                        continue;
                    }

                    const auto & item = items.at(id);
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
                    }

                    bool isHovered = false;

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
                    ImGui::TextDisabled("    %d points by %s %s ago | %d comments", story.score, story.by.c_str(), ::timeSince(story.time).c_str(), story.descendants);
                    isHovered |= ImGui::IsItemHovered();

                    if (isHovered) {
                        stateUI.hoveredWindowId = windowId;
                        window.hoveredStoryId = i;
                    }

                    if (ImGui::GetCursorScreenPos().y > ImGui::GetWindowSize().y) {
                        stateUI.maxStories = i;
                    } else {
                        if ((i == stateUI.maxStories - 1) && (ImGui::GetCursorScreenPos().y + 2 < ImGui::GetWindowSize().y)) {
                            ++stateUI.maxStories;
                        }
                    }
                }

                if (windowId == stateUI.hoveredWindowId) {
                    if (ImGui::IsMouseClicked(0) ||
                        ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Enter], false)) {
                        window.showComments = true;
                        window.selectedStoryId = storyIds[window.hoveredStoryId];
                    }

                    if (ImGui::IsKeyPressed('k', true) ||
                        ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_UpArrow], true)) {
                        window.hoveredStoryId = std::max(0, window.hoveredStoryId - 1);
                    }

                    if (ImGui::IsKeyPressed('j', true) ||
                        ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_DownArrow], true)) {
                        window.hoveredStoryId = std::min(stateUI.maxStories - 1, window.hoveredStoryId + 1);
                    }

                    if (ImGui::IsKeyPressed('g', true)) {
                        window.hoveredStoryId = 0;
                    }

                    if (ImGui::IsKeyPressed('G', true)) {
                        window.hoveredStoryId = stateUI.maxStories - 1;
                    }

                    if (ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Tab])) {
                        stateUI.windows[stateUI.hoveredWindowId].content =
                            (UI::WindowContent) ((int(stateUI.windows[stateUI.hoveredWindowId].content) + 1)%int(UI::WindowContent::Count));
                    }
                }
            } else {
                const auto & story = std::get<HN::Story>(items.at(window.selectedStoryId).data);

                int curCommentId = 0;

                std::function<void(const HN::ItemIds & commentIds, int indent)> renderComments;
                renderComments = [&](const HN::ItemIds & commentIds, int indent) {
                    const int nComments = commentIds.size();
                    for (int i = 0; i < nComments; ++i) {
                        const auto & id = commentIds[i];

                        toRefreshFG.push_back(commentIds[i]);
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

                            if (ImGui::GetCursorScreenPos().y < 0) {
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
                            if (ImGui::GetCursorScreenPos().y > ImGui::GetWindowSize().y) {
                                ImGui::SetScrollHereY(1.0f);
                            }

                            if (ImGui::IsMouseClicked(0) ||
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

                renderComments(story.kids, 0);

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

                    if (ImGui::IsMouseClicked(1) ||
                        ImGui::IsKeyPressed('q', false) ||
                        ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_Backspace], false)) {
                        window.showComments = false;
                    }
                }

                window.hoveredCommentId = std::min(curCommentId - 1, window.hoveredCommentId);
            }

            ImGui::End();
        }

        if (ImGui::IsKeyPressed('k', true) ||
            ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_LeftArrow], true)) {
            stateUI.hoveredWindowId = std::max(0, stateUI.hoveredWindowId - 1);
        }

        if (ImGui::IsKeyPressed('l', true) ||
            ImGui::IsKeyPressed(ImGui::GetIO().KeyMap[ImGuiKey_RightArrow], true)) {
            stateUI.hoveredWindowId = std::min(stateUI.nWindows - 1, stateUI.hoveredWindowId + 1);
        }

        if (stateUI.showStatusWindow) {
            {
                auto wSize = ImGui::GetIO().DisplaySize;
                ImGui::SetNextWindowPos(ImVec2(0, wSize.y - stateUI.statusWindowHeight), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(wSize.x, stateUI.statusWindowHeight), ImGuiCond_Always);
            }
            ImGui::Begin("Status", nullptr,
                     ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove);
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                ImGui::Text("Last API request : %s", g_curURI);
                ImGui::Text("API requests     : %d (next in %d s)", g_nfetches, g_nextUpdate);
                ImGui::Text("Total traffic    : %ld B", g_totalBytesDownloaded);
            }
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

        if (ImGui::IsKeyPressed('Q', false)) {
            g_keepRunning = false;
            break;
        }

        stateUI.hoveredWindowId = std::min(stateUI.nWindows - 1, stateUI.hoveredWindowId);

        if (ImGui::IsKeyReleased('h') || ImGui::IsKeyReleased('H')) {
            ImGui::OpenPopup("Help");
            auto col = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
            col.w *= 0.9;
            ImGui::PushStyleColor(ImGuiCol_WindowBg, col);
        }

        if (ImGui::BeginPopupModal("Help", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("    h/H         - toggle Help window    ");
            ImGui::Text("    s           - toggle Status window    ");
            ImGui::Text("    g           - go to top    ");
            ImGui::Text("    G           - go to end    ");
            ImGui::Text("    up/down/j/k - navigate items    ");
            ImGui::Text("    left/right  - navigate windows    ");
            ImGui::Text("    Tab         - change current window content    ");
            ImGui::Text("    1/2/3       - change number of windows    ");
            ImGui::Text("    q/bkspc     - close comments    ");
            ImGui::Text("    Q           - quit    ");

            for (int i = 0; i < 512; ++i) {
                if (ImGui::IsKeyPressed(i)) {
                    ImGui::CloseCurrentPopup();
                    ImGui::PopStyleColor();
                    break;
                }
            }

            ImGui::EndPopup();
        }

        ImGui::Render();

        ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), screen);
        ImTui_ImplNcurses_DrawScreen(screen);

        {
            std::lock_guard<std::mutex> lock(mutex);
            toRefreshBuf = toRefreshFG;
        }

        vsync.wait();
    }

    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();

    workerHN.join();

    freeCurl();

    return 0;
}
