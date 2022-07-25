#include "imtui/imtui.h"

#include "logs.h"

#ifdef __EMSCRIPTEN__

#include "imtui/imtui-impl-emscripten.h"

#include <emscripten.h>
#include <emscripten/html5.h>

#else

#define EMSCRIPTEN_KEEPALIVE

#include "imtui/imtui-impl-ncurses.h"

#include <fstream>

#endif

#include <cstdio>
#include <string>
#include <vector>

ImVec4 toVec4(int r, int g, int b) {
    return ImVec4(r/255.0f, g/255.0f, b/255.0f, 1.0f);
}

ImVec4 toVec4(int r, int g, int b, int a) {
    return ImVec4(r/255.0f, g/255.0f, b/255.0f, a/255.0f);
}

// convert seconds to [Friday, July 22]
std::string tToDate(int32_t t_s) {
    char buf[256];
    time_t t = t_s;
    struct tm *tm = localtime(&t);
    strftime(buf, sizeof(buf), "%A, %B %d", tm);
    return buf;
}

// convert seconds to [hh:mm]
std::string tToTime(int32_t t_s) {
    char buf[256];
    time_t t = t_s;
    struct tm *tm = localtime(&t);
    strftime(buf, sizeof(buf), "%H:%M", tm);
    return buf;
}

// get current timestamp in seconds
int32_t tGet() {
    return time(nullptr);
}

// data structures

struct ColorTheme {
    ImVec4 textFG  = toVec4(190, 190, 190);
    ImVec4 titleFG = toVec4(255, 255, 255);

    ImVec4 itemBG        = toVec4( 48,  48,  48);
    ImVec4 itemBGHovered = toVec4( 69,  69,  97);
    ImVec4 itemBGActive  = toVec4( 66, 150, 250);

    ImVec4 leftPanelBG      = toVec4( 48,  48,  48);
    ImVec4 leftPanelTextFG  = toVec4(190, 190, 190);
    ImVec4 leftPanelTitleFG = toVec4(255, 255, 255);

    ImVec4 mainWindowBG      = toVec4( 32,  32,  32);
    ImVec4 mainWindowTextFG  = toVec4(190, 190, 190);
    ImVec4 mainWindowTitleFG = toVec4(255, 255, 255);

    ImVec4 popupBG = toVec4( 20,  20,  20);

    ImVec4 listHeaderFG = toVec4(255, 255, 255);

    ImVec4 channelSelectedBG = toVec4( 39,  39,  60);
    ImVec4 channelActiveFG   = toVec4(255, 255, 255);
    ImVec4 channelMentionsFG = toVec4(255, 255, 255);
    ImVec4 channelMentionsBG = toVec4(192,   0,   0);

    ImVec4 userActiveFG = toVec4(255, 255, 255);

    ImVec4 messageUser    = toVec4(255, 255, 255);
    ImVec4 messageTime    = toVec4( 92,  92,  92);
    ImVec4 messageHovered = toVec4( 48,  48,  48);
    ImVec4 messageReact   = toVec4(255, 255,   0);
    ImVec4 messageReplies = toVec4(128, 128, 255);

    ImVec4 inputBG            = toVec4( 48,  48,  48);
    ImVec4 inputSendBG        = toVec4( 48,  92,  48);
    ImVec4 inputSendFG        = toVec4(255, 255, 255);
    ImVec4 inputSendBGHovered = toVec4( 48, 128,  48);
    ImVec4 inputSendBGActive  = toVec4( 48, 192,  48);
    ImVec4 inputTooltip       = toVec4(255, 255, 255);

    ImVec4 buttonActiveFG = toVec4(255, 255, 255);

    ImVec4 indicatorOnline  = toVec4(  0, 255,   0);
    ImVec4 indicatorOffline = toVec4(190, 190, 190);
};

struct Message {
    int32_t t_s; // timestamp in seconds
    int32_t uid; // userId

    std::string text;

    int32_t reactUp;
    int32_t reactDown;
};

struct MessageWithReplies {
    Message msg;

    std::vector<MessageWithReplies> replies;
};

struct User {
    int32_t id;

    bool isOnline;
    bool hasUnread;

    std::string username;
    std::string bio;

    std::vector<MessageWithReplies> messages;
};

struct Channel {
    bool hasUnread;

    int32_t mentions;

    std::string label;
    std::string description;

    std::vector<User> members;
    std::vector<MessageWithReplies> messages;

    bool isMember(int32_t uid) const {
        for (auto &member : members) {
            if (member.id == uid) {
                return true;
            }
        }
        return false;
    }
};

struct UI {
    enum EStyle {
        EStyle_Default,
        EStyle_Dark
    };

    int leftPanelW      = 30;
    int mainWindowW     = 30;
    int mainWindowH     = 30;
    int infoWindowW     = 60;
    int aboutWindowW    = 40;
    int messagesWindowH = 30;
    int workspaceInfoW  = 30;
    int mentionPanelW   = 30;
    int threadPanelW    = 30;
    int threadPanelH    = 30;

    bool isResizingLeftPanel = false;
    bool isHoveringLeftPanelResize = false;

    bool isResizingThreadPanel = false;
    bool isHoveringThreadPanelResize = false;

    bool showChannels       = true;
    bool showDirectMessages = true;
    bool showApps           = true;
    bool showStyleEditor    = false;
    bool showThreadPanel    = false;

    bool doScrollMain   = false;

    std::vector<Channel> channels;
    int selectedChannel = -1;

    std::vector<User> users;
    int selectedUser = -1;

    int threadChannel = -1;
    int threadMessage = -1;

    ColorTheme colors;

    void setStyle(EStyle style);

    void renderSeparator(const char * prefix, const char * ch, const char * suffix, int n, bool disabled);
    bool renderBeginMenu(const char * label);

    bool renderButton (const char * label, int width, bool active);
    bool renderList   (const char * label, int width, bool active);

    bool renderChannel(const Channel & channel, int width, bool selected);
    bool renderUser   (const User & user,       int width, bool selected);

    void renderOnlineIndicator(const char * symbol, bool online);

    bool renderMessages(const std::vector<MessageWithReplies> & messages, int width, bool isThread);
};

void UI::setStyle(EStyle styleId) {
    auto & style = ImGui::GetStyle();

    switch (styleId) {
        case EStyle_Default:
            colors = {
                .textFG             = { 0.745098, 0.745098, 0.745098, 1.000000 },
                .titleFG            = { 1.000000, 1.000000, 1.000000, 1.000000 },
                .itemBG             = { 0.223529, 0.074510, 0.235294, 1.000000 },
                .itemBGHovered      = { 0.270588, 0.270588, 0.380392, 1.000000 },
                .itemBGActive       = { 0.258824, 0.588235, 0.980392, 1.000000 },
                .leftPanelBG        = { 0.223529, 0.074510, 0.235294, 1.000000 },
                .leftPanelTextFG    = { 0.745098, 0.745098, 0.745098, 1.000000 },
                .leftPanelTitleFG   = { 1.000000, 1.000000, 1.000000, 1.000000 },
                .mainWindowBG       = { 1.000000, 1.000000, 1.000000, 1.000000 },
                .mainWindowTextFG   = { 0.125490, 0.125490, 0.125490, 1.000000 },
                .mainWindowTitleFG  = { 0.000000, 0.000000, 0.000000, 1.000000 },
                .popupBG            = { 0.831373, 0.831373, 0.831373, 1.000000 },
                .listHeaderFG       = { 1.000000, 1.000000, 1.000000, 1.000000 },
                .channelSelectedBG  = { 0.152941, 0.152941, 0.235294, 1.000000 },
                .channelActiveFG    = { 1.000000, 1.000000, 1.000000, 1.000000 },
                .channelMentionsFG  = { 1.000000, 1.000000, 1.000000, 1.000000 },
                .channelMentionsBG  = { 0.752941, 0.000000, 0.000000, 1.000000 },
                .userActiveFG       = { 1.000000, 1.000000, 1.000000, 1.000000 },
                .messageUser        = { 0.000000, 0.000000, 0.000000, 1.000000 },
                .messageTime        = { 0.360784, 0.360784, 0.360784, 1.000000 },
                .messageHovered     = { 0.949020, 0.949020, 0.949020, 1.000000 },
                .messageReact       = { 0.745098, 0.745098, 0.000000, 1.000000 },
                .messageReplies     = { 0.500000, 0.500000, 1.000000, 1.000000 },
                .inputBG            = { 0.949020, 0.949020, 0.949020, 1.000000 },
                .inputSendBG        = { 0.188235, 0.360784, 0.188235, 1.000000 },
                .inputSendFG        = { 1.000000, 1.000000, 1.000000, 1.000000 },
                .inputSendBGHovered = { 0.188235, 0.501961, 0.188235, 1.000000 },
                .inputSendBGActive  = { 0.188235, 0.752941, 0.188235, 1.000000 },
                .inputTooltip       = { 0.000000, 0.000000, 0.000000, 1.000000 },
                .buttonActiveFG     = { 1.000000, 1.000000, 1.000000, 1.000000 },
                .indicatorOnline    = { 0.000000, 1.000000, 0.000000, 1.000000 },
                .indicatorOffline   = { 0.745098, 0.745098, 0.745098, 1.000000 },
            };
            break;
        case EStyle_Dark:
            colors = ColorTheme();
            break;
    }

    style.Colors[ImGuiCol_Text]          = colors.textFG;
    style.Colors[ImGuiCol_Header]        = colors.itemBG;
    style.Colors[ImGuiCol_HeaderHovered] = colors.itemBGHovered;
    style.Colors[ImGuiCol_HeaderActive]  = colors.itemBGActive;
    style.Colors[ImGuiCol_Button]        = colors.itemBG;
    style.Colors[ImGuiCol_ButtonHovered] = colors.itemBGHovered;
    style.Colors[ImGuiCol_ButtonActive]  = colors.itemBGActive;
    style.Colors[ImGuiCol_PopupBg]       = colors.popupBG;
    style.Colors[ImGuiCol_Border]        = colors.itemBGActive;
    style.Colors[ImGuiCol_Tab]           = colors.itemBG;
    style.Colors[ImGuiCol_TabHovered]    = colors.itemBGHovered;
    style.Colors[ImGuiCol_TabActive]     = colors.itemBGActive;

    style.Colors[ImGuiCol_ModalWindowDimBg] = toVec4(0, 0, 0, 0);
}

void UI::renderSeparator(const char * prefix, const char * ch, const char * suffix, int n, bool disabled) {
    static char buf[256];
    if (n > 255) n = 255;
    if (n < 0) n = 0;
    for (int i = 0; i < n; i++) {
        buf[i] = ch[0];
    }
    buf[n] = 0;
    if (disabled) {
        ImGui::TextDisabled("%s%s%s", prefix, buf, suffix);
    } else {
        ImGui::Text("%s%s%s", prefix, buf, suffix);
    }
}

bool UI::renderBeginMenu(const char * label) {
    bool result = false;

    if (ImGui::BeginMenu(label)) {
        result = true;
    }

    return result;
}

bool UI::renderButton(const char * label, int width, bool active) {
    int npop = 0;
    bool result = false;

    ImGui::PushID(label);
    const auto p0 = ImGui::GetCursorScreenPos();
    if (ImGui::Button("##but", ImVec2(width, 1))) {
        result = true;
    }
    if (ImGui::IsItemHovered() || active) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors.buttonActiveFG);
        ++npop;
    }
    ImGui::SetCursorScreenPos(p0);
    ImGui::Text("%s", label);
    ImGui::PopStyleColor(npop);
    ImGui::PopID();

    return result;
}

bool UI::renderList(const char * label, int width, bool active) {
    int npop = 0;
    bool result = false;

    ImGui::PushID(label);
    const auto p0 = ImGui::GetCursorScreenPos();
    if (ImGui::Button("##but", ImVec2(width, 1))) {
        result = true;
    }
    if (ImGui::IsItemHovered() || active) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors.listHeaderFG);
        ++npop;
    }
    ImGui::SetCursorScreenPos(p0);
    ImGui::Text("%s %s", active ? "v" : ">", label);
    ImGui::PopStyleColor(npop);
    ImGui::PopID();

    return result;
}

bool UI::renderChannel(const Channel & channel, int width, bool selected) {
    int npop = 0;
    bool result = false;

    ImGui::PushID(channel.label.c_str());
    const auto p0 = ImGui::GetCursorScreenPos();
    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Button, colors.channelSelectedBG);
        ++npop;
    }
    if (ImGui::Button("##but", ImVec2(width, 1))) {
        result = true;
    }
    if (channel.mentions > 0) {
        ImGui::SetCursorScreenPos({ p0.x + width - 6, p0.y });
        ImGui::PushStyleColor(ImGuiCol_Text,          colors.channelMentionsFG);
        ImGui::PushStyleColor(ImGuiCol_Button,        colors.channelMentionsBG);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colors.channelMentionsBG);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  colors.channelMentionsBG);
        static char buf[16];
        sprintf(buf, " %d ", channel.mentions);
        ImGui::SmallButton(buf);
        ImGui::PopStyleColor(4);
    }
    if (ImGui::IsItemHovered() || selected || channel.hasUnread || channel.mentions > 0) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors.channelActiveFG);
        ++npop;
    }
    ImGui::SetCursorScreenPos(p0);
    ImGui::Text("# %s", channel.label.c_str());
    ImGui::PopStyleColor(npop);
    ImGui::PopID();

    return result;
}

bool UI::renderUser(const User & user, int width, bool selected) {
    int npop = 0;
    bool result = false;

    ImGui::PushID(user.username.c_str());
    const auto p0 = ImGui::GetCursorScreenPos();
    if (ImGui::Button("##but", ImVec2(width, 1))) {
        result = true;
    }
    if (ImGui::IsItemHovered() || selected || user.hasUnread) {
        ImGui::PushStyleColor(ImGuiCol_Text, colors.userActiveFG);
        ++npop;
    }
    ImGui::SetCursorScreenPos(p0);
    renderOnlineIndicator("o", user.isOnline);
    ImGui::SameLine();
    ImGui::Text("%s", user.username.c_str());
    ImGui::PopStyleColor(npop);
    ImGui::PopID();

    return result;
}

void UI::renderOnlineIndicator(const char * symbol, bool online) {
    if (online) {
        ImGui::TextColored(colors.indicatorOnline, "%s", symbol);
    } else {
        ImGui::TextColored(colors.indicatorOffline, "%s", symbol);
    }
}

bool UI::renderMessages(const std::vector<MessageWithReplies> & messages, int width, bool isThread) {
    int32_t lastUid = -1;
    std::string lastDate = "";

    ImGui::PushTextWrapPos(width - 4);

    for (int i = 0; i < (int) messages.size(); i++) {
        const auto & message = messages[i];
        const auto & uid = message.msg.uid;
        const auto & user = users[uid];

        ImGui::PushID(i);

        // render new date separator
        {
            const auto curDate = tToDate(message.msg.t_s);
            if (curDate != lastDate) {
                const auto l = curDate.size() + 6;

                ImGui::Text("%s", "");
                renderSeparator("", "-", "", width/2 - l/2 - 1, true);
                ImGui::SameLine();
                ImGui::Text("%s", curDate.c_str());
                ImGui::SameLine();
                renderSeparator("", "-", "", width/2 - l/2 - 1, true);
                lastDate = curDate;
            }
        }

        const auto p0 = ImVec2 { ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y };

        if (lastUid != uid) {
            ImGui::Text("%s", "");
            ImGui::TextColored(colors.messageUser, "%s", user.username.c_str());
            ImGui::SameLine();
            ImGui::TextColored(colors.messageTime, "%s", tToTime(message.msg.t_s).c_str());
        }
        ImGui::Text("%s", message.msg.text.c_str());
        if (message.msg.reactUp > 0) {
            ImGui::TextColored(colors.messageReact, "%s", "[+]");
            ImGui::SameLine();
            ImGui::Text("%d", message.msg.reactUp);
        }

        if (message.msg.reactDown > 0) {
            if (message.msg.reactUp > 0) {
                ImGui::SameLine();
            }
            ImGui::TextColored(colors.messageReact, "%s", "[-]");
            ImGui::SameLine();
            ImGui::Text("%d", message.msg.reactDown);
        }

        if (message.replies.size() > 0) {
            if (message.msg.reactUp > 0 || message.msg.reactDown > 0) {
                ImGui::SameLine();
                ImGui::Text("|");
                ImGui::SameLine();
            }
            ImGui::TextColored(colors.messageReplies, "%d replies", (int) message.replies.size());
        }

        const auto p1 = ImVec2 { ImGui::GetCursorScreenPos().x + width, ImGui::GetCursorScreenPos().y };

        bool doUpvote = false;
        bool doReplyInThead = false;

        if (ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(p0, p1)) {
            auto drawList = ImGui::GetWindowDrawList();

            drawList->AddRectFilled({ p0.x, p0.y + 1 }, { p1.x, p1.y - 1 }, ImGui::ColorConvertFloat4ToU32(colors.messageHovered));

            ImGui::SetCursorScreenPos(p0);

            if (lastUid != uid) {
                ImGui::Text("%s", "");
                ImGui::TextColored(colors.messageUser, "%s", user.username.c_str());
                ImGui::SameLine();
                ImGui::TextColored(colors.messageTime, "%s", tToTime(message.msg.t_s).c_str());
            }
            ImGui::Text("%s", message.msg.text.c_str());
            if (message.msg.reactUp > 0) {
                ImGui::TextColored(colors.messageReact, "%s", "[+]");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x - 10, p0.y - 4 });
                    ImGui::SetNextWindowSize({ 40, 0 });
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", "");
                    ImGui::TextColored(colors.messageReact, "%s", "[+]");
                    ImGui::Text("%s", "");
                    int w = 0;
                    for (int i = 0; i < message.msg.reactUp; i++) {
                        const auto & username = users[i%users.size()].username;
                        ImGui::TextColored(colors.messageUser, "%s%s", username.c_str(),
                                           i == message.msg.reactUp - 1 ? "" : i == message.msg.reactUp - 2 ? " and" : ",");
                        if (w += username.size(); w < 20) {
                            ImGui::SameLine();
                        } else {
                            w = 0;
                        }
                    }
                    ImGui::Text("reacted with :+1:");
                    ImGui::Text("%s", "");
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::Text("%d", message.msg.reactUp);
            }

            if (message.msg.reactDown > 0) {
                if (message.msg.reactUp > 0) {
                    ImGui::SameLine();
                }
                ImGui::TextColored(colors.messageReact, "%s", "[-]");
                if (ImGui::IsItemHovered()) {
                    ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x - 10, p0.y - 4 });
                    ImGui::SetNextWindowSize({ 40, 0 });
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", "");
                    ImGui::TextColored(colors.messageReact, "%s", "[-]");
                    ImGui::Text("%s", "");
                    int w = 0;
                    for (int i = 0; i < message.msg.reactDown; i++) {
                        const auto & username = users[i%users.size()].username;
                        ImGui::TextColored(colors.messageUser, "%s%s", username.c_str(),
                                             i == message.msg.reactDown - 1 ? "" : i == message.msg.reactDown - 2 ? " and" : ",");
                        if (w += username.size(); w < 20) {
                            ImGui::SameLine();
                        } else {
                            w = 0;
                        }
                    }
                    ImGui::Text("reacted with :-1:");
                    ImGui::Text("%s", "");
                    ImGui::EndTooltip();
                }
                ImGui::SameLine();
                ImGui::Text("%d", message.msg.reactDown);
            }

            if (message.replies.size() > 0) {
                if (message.msg.reactUp > 0 || message.msg.reactDown > 0) {
                    ImGui::SameLine();
                    ImGui::Text("|");
                    ImGui::SameLine();
                }
                ImGui::TextColored(colors.messageReplies, "%d replies", (int) message.replies.size());
                if (ImGui::IsItemHovered()) {
                    ImGui::SameLine();
                    ImGui::SmallButton("View thread");
                    if (ImGui::IsMouseDown(0)) {
                        doReplyInThead = true;
                    }
                }
            }

            auto savePos = ImGui::GetCursorScreenPos();

            const int offsetX = isThread ? 7 : 12;

            ImGui::SetCursorScreenPos({ p1.x - offsetX, p0.y });
            ImGui::SmallButton("[+]");
            if (ImGui::IsItemHovered()) {
                if (ImGui::IsMouseReleased(0)) {
                    doUpvote = true;
                }
                ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x - 10, ImGui::GetIO().MousePos.y - 2 });
                ImGui::SetTooltip("React with +1");
            }

            if (!isThread) {
                ImGui::SameLine();
                ImGui::SmallButton("[>]");
                if (ImGui::IsItemHovered()) {
                    if (ImGui::IsMouseDown(0)) {
                        doReplyInThead = true;
                    }
                    ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x - 16, ImGui::GetIO().MousePos.y - 2 });
                    ImGui::SetTooltip("Reply in thread");
                }
            }

            ImGui::SetCursorScreenPos(savePos);
        }

        if (doUpvote) {
            if (!isThread) {
                channels[selectedChannel].messages[i].msg.reactUp++;
            }
        }

        if (doReplyInThead) {
            showThreadPanel = true;
            threadChannel = selectedChannel;
            threadMessage = i;
        }

        ImGui::PopID();

        lastUid = uid;
    }

    ImGui::PopTextWrapPos();

    return true;
}

UI g_ui;
bool g_isRunning = true;
ImTui::TScreen * g_screen = nullptr;

extern "C" {

    EMSCRIPTEN_KEEPALIVE
        bool render_frame() {
#ifdef __EMSCRIPTEN__
            ImTui_ImplEmscripten_NewFrame();
#else
            bool isActive = false;
            isActive |= ImTui_ImplNcurses_NewFrame();
#endif
            ImTui_ImplText_NewFrame();

            ImGui::NewFrame();

            // full-screen window:
            ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(g_screen->nx, g_screen->ny), ImGuiCond_Always);

            ImGui::Begin("Main window", nullptr,
                         ImGuiWindowFlags_NoTitleBar |
                         ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoScrollbar |
                         ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_MenuBar);

            bool doOpenAbout = false;

            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("Slack ")) {
                    ImGui::TextDisabled("%s", "");

                    if (ImGui::MenuItem("About Slack")) {
                        doOpenAbout = true;
                    }

                    ImGui::TextDisabled("--------------");

                    ImGui::MenuItem("Preferences ...", "Cmd+.");
                    if (ImGui::BeginMenu("Settings & Administration")) {
                        ImGui::TextDisabled("%s", "");

                        ImGui::MenuItem("Manage Members");
                        ImGui::MenuItem("Manage Apps");

                        ImGui::TextDisabled("--------------");

                        ImGui::MenuItem("Customize Workspace");

                        ImGui::TextDisabled("--------------");

                        ImGui::MenuItem("Workspace Settings");

                        ImGui::TextDisabled("%s", "");
                        ImGui::EndMenu();
                    }

                    ImGui::TextDisabled("--------------");

                    ImGui::MenuItem("Services");

                    ImGui::TextDisabled("--------------");

                    ImGui::MenuItem("Sign Out");
                    ImGui::MenuItem("Quit Slack", "Cmd+Q");

                    ImGui::TextDisabled("%s", "");
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("File ")) {
                    ImGui::TextDisabled("%s", "");

                    ImGui::MenuItem("New Message", "Ctrl+N");
                    ImGui::MenuItem("New Channel");

                    ImGui::TextDisabled("--------------");

                    ImGui::MenuItem("Workspace");
                    ImGui::MenuItem("Invite People");

                    ImGui::TextDisabled("--------------");

                    ImGui::MenuItem("Close Window");

                    ImGui::TextDisabled("%s", "");
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("View ")) {
                    ImGui::TextDisabled("%s", "");

                    ImGui::MenuItem("Reload", "Cmd+R");
                    ImGui::MenuItem("Force Reload", "Shift+Cmd+R");

                    ImGui::TextDisabled("--------------");

                    ImGui::MenuItem("Toggle Fullscreen", "Shift+Cmd+F");

                    ImGui::TextDisabled("--------------");

                    ImGui::MenuItem("Hide Sidebar", "Shift+Cmd+D");

                    ImGui::TextDisabled("%s", "");
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Style ")) {
                    ImGui::TextDisabled("%s", "");

                    if (ImGui::MenuItem("Style editor", nullptr, g_ui.showStyleEditor)) {
                        g_ui.showStyleEditor = !g_ui.showStyleEditor;
                    }

                    ImGui::TextDisabled("--------------");

                    if (ImGui::MenuItem("Default style")) {
                        g_ui.setStyle(UI::EStyle_Default);
                    }

                    if (ImGui::MenuItem("Dark style")) {
                        g_ui.setStyle(UI::EStyle_Dark);
                    }

                    ImGui::TextDisabled("%s", "");
                    ImGui::EndMenu();
                }

                if (ImGui::BeginMenu("Help ")) {
                    ImGui::TextDisabled("%s", "");

                    ImGui::MenuItem("Keyboard Shortcuts");
                    ImGui::MenuItem("Contact Us");

                    ImGui::TextDisabled("%s", "");
                    ImGui::EndMenu();
                }

                ImGui::EndMenuBar();
            }

            // About Slack
            {
                if (doOpenAbout) {
                    ImGui::OpenPopup("About Slack");
                    doOpenAbout = false;
                }

                ImGui::SetNextWindowPos(ImVec2(g_screen->nx/2.0f - g_ui.aboutWindowW/2.0f, g_screen->ny/3.0f), ImGuiCond_Always);
                ImGui::SetNextWindowSize(ImVec2(g_ui.aboutWindowW, 0), ImGuiCond_Always);
                if (ImGui::BeginPopupModal("About Slack", NULL,
                                           ImGuiWindowFlags_AlwaysAutoResize |
                                           ImGuiWindowFlags_NoMove)) {
                    ImGui::PushStyleColor(ImGuiCol_Text,   g_ui.colors.mainWindowTextFG);
                    ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.mainWindowBG);

                    ImGui::Text("%s", "");
                    ImGui::Text("%s", "");

                    {
                        const auto p0 = ImGui::GetCursorScreenPos();
                        {
                            const char * txt = "#";
                            ImGui::SetCursorScreenPos({ p0.x + g_ui.aboutWindowW/2.0f - ImGui::CalcTextSize(txt).x/2.0f, p0.y });
                            auto col = g_ui.colors.mainWindowTitleFG;
                            const auto t = ImGui::GetTime();
                            const int it = (int(t/0.25f))%5;
                            if (it == 0) col = toVec4(255, 255, 255);
                            if (it == 1) col = toVec4(128, 128, 255);
                            if (it == 2) col = toVec4(  0, 255,   0);
                            if (it == 3) col = toVec4(255,   0,   0);
                            if (it == 4) col = toVec4(255, 255,   0);

                            ImGui::TextColored(col, "%s", txt);
                        }

                        ImGui::SetCursorScreenPos({ p0.x + g_ui.aboutWindowW - 6, p0.y - 1 });
                        if (ImGui::Button("|x|")) {
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    ImGui::Text("%s", "");
                    ImGui::Text("%s", "");

                    {
                        const char * txt = "Slack (TUI)";
                        const auto p0 = ImGui::GetCursorScreenPos();
                        ImGui::SetCursorScreenPos({ p0.x + g_ui.aboutWindowW/2.0f - ImGui::CalcTextSize(txt).x/2.0f, p0.y });
                        ImGui::TextColored(g_ui.colors.mainWindowTitleFG, "%s", txt);
                    }

                    ImGui::Text("%s", "");
                    ImGui::Text("%s", "");

                    {
                        const char * txt = "Version 0.1.0";
                        const auto p0 = ImGui::GetCursorScreenPos();
                        ImGui::SetCursorScreenPos({ p0.x + g_ui.aboutWindowW/2.0f - ImGui::CalcTextSize(txt).x/2.0f, p0.y });
                        ImGui::Text("%s", txt);
                    }

                    {
                        const char * txt = "https://github.com/ggerganov/imtui";
                        const auto p0 = ImGui::GetCursorScreenPos();
                        ImGui::SetCursorScreenPos({ p0.x + g_ui.aboutWindowW/2.0f - ImGui::CalcTextSize(txt).x/2.0f, p0.y });
                        ImGui::Text("%s", txt);
                    }

                    ImGui::Text("%s", "");

                    {
                        const char * txt = "@2022 Georgi Gerganov";
                        const auto p0 = ImGui::GetCursorScreenPos();
                        ImGui::SetCursorScreenPos({ p0.x + g_ui.aboutWindowW/2.0f - ImGui::CalcTextSize(txt).x/2.0f, p0.y });
                        ImGui::Text("%s", txt);
                    }

                    ImGui::Text("%s", "");

                    ImGui::PopStyleColor(2);
                    ImGui::EndPopup();
                }
            }

            // left panel
            {
                ImGui::PushStyleColor(ImGuiCol_Button,  g_ui.colors.leftPanelBG);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui.colors.leftPanelBG);

                ImGui::BeginChild("Child window", ImVec2(g_ui.leftPanelW - 1, 0), true);

                {
                    ImGui::PushStyleColor(ImGuiCol_Text,   g_ui.colors.leftPanelTitleFG);
                    const auto p0 = ImGui::GetCursorScreenPos();
                    if (ImGui::Button("## Workspace", ImVec2(g_ui.leftPanelW, 3))) {
                        ImGui::OpenPopup("Workspace Info");
                    }
                    ImGui::SetCursorScreenPos(p0);
                    ImGui::Text("%s", "");
                    ImGui::Text("V ImTui Workspace");
                    ImGui::PopStyleColor(1);

                    ImGui::SetNextWindowSize(ImVec2(g_ui.workspaceInfoW, 0), ImGuiCond_Always);
                    if (ImGui::BeginPopup("Workspace Info")) {
                        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + g_ui.workspaceInfoW - 4);
                        ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.popupBG);

                        ImGui::Text("%s", "");
                        ImGui::Text("ImTui");
                        ImGui::TextDisabled("imtui.slack.com");

                        ImGui::Text("%s", "");
                        g_ui.renderSeparator("", "-", "", g_ui.workspaceInfoW - 4, true);
                        ImGui::Text("%s", "");

                        ImGui::Text("Total Messages");
                        ImGui::TextDisabled("Upgrade to access your first 169k messages.");

                        ImGui::Text("%s", "");
                        g_ui.renderSeparator("", "-", "", g_ui.workspaceInfoW - 4, true);
                        ImGui::Text("%s", "");

                        g_ui.renderButton("Invite people to ImTui", g_ui.workspaceInfoW, false);
                        g_ui.renderButton("Create a channel",       g_ui.workspaceInfoW, false);

                        ImGui::Text("%s", "");
                        g_ui.renderSeparator("", "-", "", g_ui.workspaceInfoW - 4, true);
                        ImGui::Text("%s", "");

                        g_ui.renderButton("Preferences",               g_ui.workspaceInfoW, false);
                        g_ui.renderButton("Settings & administration", g_ui.workspaceInfoW, false);

                        ImGui::Text("%s", "");
                        g_ui.renderSeparator("", "-", "", g_ui.workspaceInfoW - 4, true);
                        ImGui::Text("%s", "");

                        if (g_ui.renderBeginMenu("Tools")) {
                            ImGui::Text("%s", "");

                            g_ui.renderButton("Workflow builder", g_ui.workspaceInfoW, false);
                            g_ui.renderButton("Analytics",        g_ui.workspaceInfoW, false);

                            ImGui::Text("%s", "");
                            ImGui::EndMenu();
                        }

                        ImGui::Text("%s", "");
                        g_ui.renderSeparator("", "-", "", g_ui.workspaceInfoW - 4, true);
                        ImGui::Text("%s", "");

                        g_ui.renderButton("Sign out of ImTui", g_ui.workspaceInfoW, false);
                        g_ui.renderButton("Add workspaces",    g_ui.workspaceInfoW, false);

                        ImGui::Text("%s", "");
                        ImGui::PopStyleColor();
                        ImGui::PopTextWrapPos();
                        ImGui::EndPopup();
                    }
                }

                ImGui::Text("%s", "");
                g_ui.renderSeparator("-", "-", "-", g_ui.leftPanelW - 4, true);
                ImGui::Text("%s", "");

                g_ui.renderButton("\\ Threads",             g_ui.leftPanelW, false);
                g_ui.renderButton("& Direct Messages",      g_ui.leftPanelW, false);
                g_ui.renderButton("@ Mentions & reactions", g_ui.leftPanelW, false);
                g_ui.renderButton("* Saved items",          g_ui.leftPanelW, false);
                g_ui.renderButton("| More",                 g_ui.leftPanelW, false);

                ImGui::Text("%s", "");
                g_ui.renderSeparator("-", "-", "-", g_ui.leftPanelW - 4, true);
                ImGui::Text("%s", "");

                if (g_ui.renderList("Channels", g_ui.leftPanelW, g_ui.showChannels)) {
                    g_ui.showChannels = !g_ui.showChannels;
                }

                if (g_ui.showChannels) {
                    ImGui::Text("%s", "");

                    for (int i = 0; i < (int) g_ui.channels.size(); ++i) {
                        const auto & channel = g_ui.channels[i];

                        if (g_ui.renderChannel(channel, g_ui.leftPanelW, i == g_ui.selectedChannel)) {
                            g_ui.selectedChannel = i;
                            g_ui.selectedUser    = -1;
                        }
                    }

                    ImGui::Text("%s", "");
                    g_ui.renderButton("+ Add channels", g_ui.leftPanelW, false);
                }

                ImGui::Text("%s", "");
                g_ui.renderSeparator("-", "-", "-", g_ui.leftPanelW - 4, true);
                ImGui::Text("%s", "");

                if (g_ui.renderList("Direct messages", g_ui.leftPanelW, g_ui.showDirectMessages)) {
                    g_ui.showDirectMessages = !g_ui.showDirectMessages;
                }

                if (g_ui.showDirectMessages) {
                    ImGui::Text("%s", "");

                    g_ui.renderButton("~ Slackbot",      g_ui.leftPanelW, false);

                    const auto & users = g_ui.users;
                    for (int i = 0; i < (int) g_ui.users.size(); ++i) {
                        const auto & user = users[i];
                        if (g_ui.renderUser(user, g_ui.leftPanelW, i == g_ui.selectedUser)) {
                            g_ui.selectedChannel = -1;
                            g_ui.selectedUser    = i;
                        }
                    }

                    ImGui::Text("%s", "");
                    g_ui.renderButton("+ Add teammates", g_ui.leftPanelW, false);
                }

                ImGui::Text("%s", "");
                g_ui.renderSeparator("-", "-", "-", g_ui.leftPanelW - 4, true);
                ImGui::Text("%s", "");

                if (g_ui.renderList("Apps", g_ui.leftPanelW, g_ui.showApps)) {
                    g_ui.showApps = !g_ui.showApps;
                }

                if (g_ui.showApps) {
                    ImGui::Text("%s", "");

                    g_ui.renderButton("+ Add apps", g_ui.leftPanelW, false);
                }

                ImGui::Text("%s", "");
                ImGui::Text("%s", "");
                ImGui::Text("%s", "");

                ImGui::TextDisabled("Debug:");
                ImGui::TextDisabled("%g %g %d %d", ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y, ImGui::GetIO().MouseDown[0], ImGui::GetIO().MouseDown[1]);
                ImGui::TextDisabled("%d %d", g_ui.threadChannel, g_ui.threadMessage);

                for (int i = 1; i < 128; ++i) {
                    if (ImGui::IsKeyPressed(i)) {
                        ImGui::Text("%d", i);
                    }
                }

                // left panel resizing
                if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootWindow) && abs(ImGui::GetIO().MousePos.x - g_ui.leftPanelW) < 2) {
                    g_ui.isHoveringLeftPanelResize = true;

                    if (ImGui::IsMouseDown(0) && !g_ui.isResizingLeftPanel) {
                        g_ui.isResizingLeftPanel = true;
                    } else {
                        if (!ImGui::IsMouseDown(0)) {
                            g_ui.isResizingLeftPanel = false;
                        }
                    }
                } else {
                    g_ui.isHoveringLeftPanelResize = false;
                }

                ImGui::EndChild();

                ImGui::PopStyleColor(2);
            }

            if (g_ui.isHoveringLeftPanelResize) {
                auto drawList = ImGui::GetWindowDrawList();

                drawList->AddRect(ImVec2(g_ui.leftPanelW + 0.5, 0), ImVec2(g_ui.leftPanelW + 1.1, g_screen->ny), ImGui::GetColorU32(ImGuiCol_Border));
            }

            // main window
            {
                g_ui.mainWindowW = g_screen->nx - g_ui.leftPanelW - 2;
                if (g_ui.showThreadPanel) {
                    g_ui.mainWindowW -= g_ui.threadPanelW;
                }
                g_ui.mainWindowH = g_screen->ny;
                g_ui.messagesWindowH = g_ui.mainWindowH - 16;

                ImGui::PushStyleColor(ImGuiCol_Text, g_ui.colors.mainWindowTextFG);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui.colors.mainWindowBG);
                ImGui::SetNextWindowPos(ImVec2(g_ui.leftPanelW + 1, 0), ImGuiCond_Always);

                ImGui::BeginChild("main", ImVec2(g_ui.mainWindowW, g_ui.mainWindowH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

                ImGui::Text("%s", "");

                if (g_ui.selectedChannel >= 0) {
                    const auto & channel = g_ui.channels[g_ui.selectedChannel];

                    {

                        ImGui::PushStyleColor(ImGuiCol_Text,   g_ui.colors.mainWindowTitleFG);
                        ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.mainWindowBG);
                        const auto p0 = ImGui::GetCursorScreenPos();
                        if (ImGui::Button("## Channel", ImVec2(channel.label.size() + 5, 3))) {
                            ImGui::OpenPopup("Channel Info");
                        }
                        ImGui::SetCursorScreenPos(p0);
                        ImGui::Text("%s", "");
                        ImGui::Text("# %s v", channel.label.c_str());
                        ImGui::SameLine();
                        ImGui::TextDisabled(" %s", channel.description.c_str());
                        ImGui::SetCursorScreenPos({ p0.x + g_ui.mainWindowW - 7, p0.y + 1 });
                        {
                            static char buf[16];
                            sprintf(buf, "[%2d]", (int) channel.members.size());
                            if (ImGui::Button(buf)) {
                                ImGui::OpenPopup("Channel Info");
                            }
                        }
                        ImGui::PopStyleColor(2);
                    }

                    ImGui::Text("%s", "");
                    g_ui.renderSeparator("-", "-", "-", g_ui.mainWindowW - 4, true);

                    ImGui::BeginChild("messages", ImVec2(g_ui.mainWindowW - 2, g_ui.messagesWindowH), true);

                    g_ui.renderMessages(channel.messages, g_ui.mainWindowW, false);

                    if (g_ui.doScrollMain) {
                        ImGui::SetScrollHereY(1.0f);
                        g_ui.doScrollMain = false;
                    }

                    ImGui::EndChild();

                    ImGui::SetNextWindowSize(ImVec2(g_ui.infoWindowW, 0), ImGuiCond_Always);
                    if (ImGui::BeginPopupModal("Channel Info", NULL,
                                               ImGuiWindowFlags_AlwaysAutoResize |
                                               ImGuiWindowFlags_NoMove)) {
                        ImGui::PushStyleColor(ImGuiCol_Text,   g_ui.colors.mainWindowTextFG);
                        ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.mainWindowBG);

                        ImGui::Text("%s", "");
                        {
                            const auto p0 = ImGui::GetCursorScreenPos();
                            ImGui::TextColored(g_ui.colors.mainWindowTitleFG, "# %s", channel.label.c_str());

                            ImGui::SetCursorScreenPos({ p0.x + g_ui.infoWindowW - 6, p0.y });
                            if (ImGui::Button("[X]")) {
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::Text("%s", "");

                        if (ImGui::BeginTabBar("##tabbar", ImGuiTabBarFlags_None)) {
                            ImGui::PushStyleColor(ImGuiCol_Text, g_ui.colors.mainWindowTitleFG);
                            if (ImGui::BeginTabItem("About")) {
                                ImGui::PopStyleColor();
                                ImGui::Text("%s", "");
                                ImGui::TextColored(g_ui.colors.mainWindowTitleFG, "Channel name");
                                ImGui::Text("# %s", channel.label.c_str());
                                ImGui::Text("%s", "");

                                ImGui::TextColored(g_ui.colors.mainWindowTitleFG, "Topic");
                                ImGui::Text("Some topic");
                                ImGui::Text("%s", "");

                                ImGui::TextColored(g_ui.colors.mainWindowTitleFG, "Description");
                                ImGui::Text("%s", channel.description.c_str());
                                ImGui::Text("%s", "");

                                ImGui::TextColored(g_ui.colors.mainWindowTitleFG, "Created by");
                                ImGui::Text("%s", "Georgi Gerganov on 24 July, 2022");
                                ImGui::Text("%s", "");

                                ImGui::EndTabItem();
                            } else {
                                ImGui::PopStyleColor();
                            }

                            {
                                static char buf[16];
                                snprintf(buf, sizeof(buf), "Members %d", (int) channel.members.size());
                                ImGui::PushStyleColor(ImGuiCol_Text, g_ui.colors.mainWindowTitleFG);
                                if (ImGui::BeginTabItem(buf)) {
                                    ImGui::PopStyleColor();
                                    ImGui::Text("%s", "");

                                    const auto & users = channel.members;
                                    for (int i = 0; i < (int) users.size(); ++i) {
                                        const auto & user = users[i];
                                        g_ui.renderUser(user, g_ui.infoWindowW, false);
                                    }

                                    ImGui::Text("%s", "");
                                    ImGui::EndTabItem();
                                } else {
                                    ImGui::PopStyleColor();
                                }
                            }

                            ImGui::PushStyleColor(ImGuiCol_Text, g_ui.colors.mainWindowTitleFG);
                            if (ImGui::BeginTabItem("Integrations")) {
                                ImGui::PopStyleColor();
                                ImGui::Text("%s", "");
                                ImGui::Text("%s", "Workflows");
                                ImGui::Text("%s", "");
                                ImGui::EndTabItem();
                            } else {
                                ImGui::PopStyleColor();
                            }

                            ImGui::PushStyleColor(ImGuiCol_Text, g_ui.colors.mainWindowTitleFG);
                            if (ImGui::BeginTabItem("Settings")) {
                                ImGui::PopStyleColor();
                                ImGui::Text("%s", "");
                                ImGui::TextColored(g_ui.colors.mainWindowTitleFG, "Channel name");
                                ImGui::Text("# %s", channel.label.c_str());
                                ImGui::Text("%s", "");
                                ImGui::EndTabItem();
                            } else {
                                ImGui::PopStyleColor();
                            }

                            ImGui::EndTabBar();
                        }

                        ImGui::Text("%s", "");

                        ImGui::PopStyleColor(2);
                        ImGui::EndPopup();
                    }
                }

                if (g_ui.selectedUser >= 0) {
                    const auto & user = g_ui.users[g_ui.selectedUser];

                    {
                        ImGui::PushStyleColor(ImGuiCol_Text,   g_ui.colors.mainWindowTitleFG);
                        ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.mainWindowBG);
                        const auto p0 = ImGui::GetCursorScreenPos();
                        if (ImGui::Button("## User", ImVec2(user.username.size() + 5, 3))) {
                            ImGui::OpenPopup("User Info");
                        }
                        ImGui::SetCursorScreenPos(p0);
                        ImGui::Text("%s", "");
                        g_ui.renderOnlineIndicator("o", user.isOnline);
                        ImGui::SameLine();
                        ImGui::Text("%s v", user.username.c_str());
                        ImGui::PopStyleColor(2);
                    }

                    ImGui::Text("%s", "");
                    g_ui.renderSeparator("-", "-", "-", g_ui.mainWindowW - 4, true);

                    ImGui::BeginChild("messages", ImVec2(g_ui.mainWindowW - 1, g_ui.messagesWindowH), true);

                    g_ui.renderMessages(user.messages, g_ui.mainWindowW, false);

                    if (g_ui.doScrollMain) {
                        ImGui::SetScrollHereY(1.0f);
                        g_ui.doScrollMain = false;
                    }

                    ImGui::EndChild();
                }

                {
                    ImGui::BeginChild("input", ImVec2(g_ui.mainWindowW - 1, g_ui.mainWindowH - g_ui.messagesWindowH), true);

                    ImGui::Text("%s", "");
                    g_ui.renderSeparator("-", "-", "-", g_ui.mainWindowW - 4, true);
                    ImGui::Text("%s", "");

                    bool doSend = false;
                    static char input[1024];

                    {
                        // text input
                        {
                            const auto p0 = ImGui::GetCursorScreenPos();
                            ImGui::SetCursorScreenPos(ImVec2(p0.x + 1, p0.y));
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, g_ui.colors.inputBG);
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1));
                            if (ImGui::InputTextMultiline("##input", input, sizeof(input), ImVec2(g_ui.mainWindowW - 3, 5),
                                                          ImGuiInputTextFlags_EnterReturnsTrue |
                                                          ImGuiInputTextFlags_AutoSelectAll)) {
                                doSend = true;
                            }
                            ImGui::PopStyleVar();
                            ImGui::PopStyleColor();
                        }

                        ImGui::Text("%s", "");

                        // toolbar
                        {
                            const auto p0 = ImGui::GetCursorScreenPos();

                            ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.mainWindowBG);

                            ImGui::SmallButton("[+]");
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x - 10, p0.y - 3 });
                                ImGui::BeginTooltip();
                                ImGui::Text("%s", "");
                                ImGui::TextColored(g_ui.colors.inputTooltip, "Attachments & shortcuts ");
                                ImGui::Text("%s", "");
                                ImGui::EndTooltip();
                            }
                            ImGui::SameLine();
                            if (ImGui::SmallButton("[@]")) {
                                ImGui::OpenPopup("@");
                            }
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x - 10, p0.y - 3 });
                                ImGui::BeginTooltip();
                                ImGui::Text("%s", "");
                                ImGui::TextColored(g_ui.colors.inputTooltip, "Mention someone ");
                                ImGui::Text("%s", "");
                                ImGui::EndTooltip();
                            }

                            ImGui::SetCursorScreenPos(p0);

                            ImGui::PopStyleColor();
                        }

                        // send button
                        {
                            const auto p0 = ImGui::GetCursorScreenPos();
                            ImGui::SetCursorScreenPos(ImVec2(p0.x + g_ui.mainWindowW - 12, p0.y));
                            ImGui::PushStyleColor(ImGuiCol_Text,          g_ui.colors.inputSendFG);
                            ImGui::PushStyleColor(ImGuiCol_Button,        g_ui.colors.inputSendBG);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_ui.colors.inputSendBGHovered);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_ui.colors.inputSendBGActive);
                            if (ImGui::Button(" Send > ")) {
                                doSend = true;
                            }
                            ImGui::PopStyleColor(4);
                        }

                        // context menus:
                        {
                            ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y - 14 }, ImGuiCond_Appearing);
                            if (ImGui::BeginPopup("@", ImGuiWindowFlags_NoScrollbar)) {
                                ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.popupBG);

                                ImGui::Text("%s", "");
                                ImGui::TextColored(g_ui.colors.inputTooltip, "Mention someone:");
                                ImGui::Text("%s", "");

                                ImGui::BeginChild("mention", ImVec2(g_ui.mentionPanelW, 8), true);

                                const auto & users = g_ui.users;
                                for (int i = 0; i < (int) users.size(); ++i) {
                                    const auto & user = users[i];
                                    if (g_ui.renderUser(user, g_ui.mentionPanelW, i == g_ui.selectedUser)) {
                                        strcat(input, "@");
                                        strcat(input, user.username.c_str());
                                        strcat(input, " ");
                                        ImGui::CloseCurrentPopup();
                                    }
                                }

                                ImGui::EndChild();

                                ImGui::Text("%s", "");

                                ImGui::PopStyleColor();

                                ImGui::EndPopup();
                            }
                        }
                    }

                    if (doSend) {
                        if (g_ui.selectedChannel > 0) {
                            g_ui.channels[g_ui.selectedChannel].messages.push_back({ { tGet(), 0, input, 0, 0, }, {} });
                        }
                        if (g_ui.selectedUser > 0) {
                            g_ui.users[g_ui.selectedUser].messages.push_back({ { tGet(), 0, input, 0, 0, }, {} });
                        }
                        g_ui.doScrollMain = true;
                        memset(input, 0, sizeof(input));
                    }


                    ImGui::EndChild();
                }

                // thread panel resizing
                if (ImGui::IsWindowHovered() && abs(ImGui::GetIO().MousePos.x - g_ui.leftPanelW - g_ui.mainWindowW) < 2) {
                    g_ui.isHoveringThreadPanelResize = true;

                    if (ImGui::IsMouseDown(0) && !g_ui.isResizingThreadPanel) {
                        g_ui.isResizingThreadPanel = true;
                    } else {
                        if (!ImGui::IsMouseDown(0)) {
                            g_ui.isResizingThreadPanel = false;
                        }
                    }
                } else {
                    g_ui.isHoveringThreadPanelResize = false;
                }

                ImGui::EndChild();

                ImGui::PopStyleColor(2);
            }

            if (g_ui.isHoveringThreadPanelResize) {
                auto drawList = ImGui::GetWindowDrawList();

                drawList->AddRect(ImVec2(g_ui.leftPanelW + g_ui.mainWindowW + 0.5, 0), ImVec2(g_ui.leftPanelW + g_ui.mainWindowW + 1.1, g_screen->ny), ImGui::GetColorU32(ImGuiCol_Border));
            }

            // thread panel
            if (g_ui.showThreadPanel) {
                g_ui.threadPanelH = g_screen->ny;

                ImGui::PushStyleColor(ImGuiCol_Text, g_ui.colors.mainWindowTextFG);
                ImGui::PushStyleColor(ImGuiCol_ChildBg, g_ui.colors.mainWindowBG);
                ImGui::SetNextWindowPos(ImVec2(g_ui.leftPanelW + g_ui.mainWindowW + 1, 0), ImGuiCond_Always);

                ImGui::BeginChild("thread", ImVec2(g_ui.threadPanelW, g_ui.threadPanelH), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

                ImGui::Text("%s", "");

                if (g_ui.threadChannel >= 0) {
                    const auto & channel = g_ui.channels[g_ui.threadChannel];

                    {
                        ImGui::PushStyleColor(ImGuiCol_Text,   g_ui.colors.mainWindowTitleFG);
                        ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.mainWindowBG);
                        const auto p0 = ImGui::GetCursorScreenPos();
                        ImGui::Text("%s", "");
                        ImGui::Text(" %s", "Thread");
                        ImGui::SameLine();
                        ImGui::TextDisabled(" # %s", channel.label.c_str());
                        ImGui::PopStyleColor(2);

                        ImGui::SetCursorScreenPos({ p0.x + g_ui.threadPanelW - 6, p0.y + 1 });
                        if (ImGui::Button("[X]")) {
                            g_ui.showThreadPanel = false;
                            g_ui.threadChannel = -1;
                        }
                    }

                    ImGui::Text("%s", "");
                    g_ui.renderSeparator("-", "-", "-", g_ui.threadPanelW - 4, true);

                    ImGui::BeginChild("messages", ImVec2(g_ui.threadPanelW - 1, g_ui.threadPanelH - 6), true);

                    g_ui.renderMessages({{ channel.messages[g_ui.threadMessage].msg, {} }}, g_ui.threadPanelW, true);

                    if (int nreplies = channel.messages[g_ui.threadMessage].replies.size(); nreplies > 0) {
                        ImGui::Text("%s", "");
                        {
                            static char buf[32];
                            sprintf(buf, "%d replies ", nreplies);
                            g_ui.renderSeparator(buf, "-", "-", g_ui.threadPanelW - 4, true);
                        }
                        ImGui::Text("%s", "");
                    }

                    g_ui.renderMessages(channel.messages[g_ui.threadMessage].replies, g_ui.threadPanelW, true);

                    // thread input

                    ImGui::Text("%s", "");
                    g_ui.renderSeparator("-", "-", "-", g_ui.threadPanelW - 4, true);
                    ImGui::Text("%s", "");

                    bool doSend = false;
                    static char input[1024];

                    {
                        // text input
                        {
                            const auto p0 = ImGui::GetCursorScreenPos();
                            ImGui::SetCursorScreenPos(ImVec2(p0.x + 1, p0.y));
                            ImGui::PushStyleColor(ImGuiCol_FrameBg, g_ui.colors.inputBG);
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 1));
                            if (ImGui::InputTextMultiline("##input", input, sizeof(input), ImVec2(g_ui.threadPanelW - 3, 5),
                                                          ImGuiInputTextFlags_EnterReturnsTrue |
                                                          ImGuiInputTextFlags_AutoSelectAll)) {
                                doSend = true;
                            }
                            ImGui::PopStyleVar();
                            ImGui::PopStyleColor();
                        }

                        ImGui::Text("%s", "");

                        // toolbar
                        {
                            const auto p0 = ImGui::GetCursorScreenPos();

                            ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.mainWindowBG);

                            ImGui::SmallButton("[+]");
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x - 10, p0.y - 3 });
                                ImGui::BeginTooltip();
                                ImGui::Text("%s", "");
                                ImGui::TextColored(g_ui.colors.inputTooltip, "Attachments & shortcuts ");
                                ImGui::Text("%s", "");
                                ImGui::EndTooltip();
                            }
                            ImGui::SameLine();
                            if (ImGui::SmallButton("[@]")) {
                                ImGui::OpenPopup("@");
                            }
                            if (ImGui::IsItemHovered()) {
                                ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x - 10, p0.y - 3 });
                                ImGui::BeginTooltip();
                                ImGui::Text("%s", "");
                                ImGui::TextColored(g_ui.colors.inputTooltip, "Mention someone ");
                                ImGui::Text("%s", "");
                                ImGui::EndTooltip();
                            }

                            ImGui::SetCursorScreenPos(p0);

                            ImGui::PopStyleColor();
                        }

                        // send button
                        {
                            const auto p0 = ImGui::GetCursorScreenPos();
                            ImGui::SetCursorScreenPos(ImVec2(p0.x + g_ui.threadPanelW - 12, p0.y));
                            ImGui::PushStyleColor(ImGuiCol_Text,          g_ui.colors.inputSendFG);
                            ImGui::PushStyleColor(ImGuiCol_Button,        g_ui.colors.inputSendBG);
                            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, g_ui.colors.inputSendBGHovered);
                            ImGui::PushStyleColor(ImGuiCol_ButtonActive,  g_ui.colors.inputSendBGActive);
                            if (ImGui::Button(" Send > ")) {
                                doSend = true;
                            }
                            ImGui::PopStyleColor(4);
                        }

                        // context menus:
                        {
                            ImGui::SetNextWindowPos({ ImGui::GetIO().MousePos.x, ImGui::GetIO().MousePos.y - 14 }, ImGuiCond_Appearing);
                            if (ImGui::BeginPopup("@", ImGuiWindowFlags_NoScrollbar)) {
                                ImGui::PushStyleColor(ImGuiCol_Button, g_ui.colors.popupBG);

                                ImGui::Text("%s", "");
                                ImGui::TextColored(g_ui.colors.inputTooltip, "Mention someone:");
                                ImGui::Text("%s", "");

                                ImGui::BeginChild("mention", ImVec2(g_ui.mentionPanelW, 8), true);

                                const auto & users = g_ui.users;
                                for (int i = 0; i < (int) users.size(); ++i) {
                                    const auto & user = users[i];
                                    if (g_ui.renderUser(user, g_ui.mentionPanelW, i == g_ui.selectedUser)) {
                                        strcat(input, "@");
                                        strcat(input, user.username.c_str());
                                        strcat(input, " ");
                                        ImGui::CloseCurrentPopup();
                                    }
                                }

                                ImGui::EndChild();

                                ImGui::Text("%s", "");

                                ImGui::PopStyleColor();

                                ImGui::EndPopup();
                            }
                        }
                    }

                    if (doSend) {
                        g_ui.channels[g_ui.threadChannel].messages[g_ui.threadMessage].replies.push_back({ { tGet(), 0, input, 0, 0, }, {} });
                        memset(input, 0, sizeof(input));
                    }


                    ImGui::EndChild();
                }

                ImGui::EndChild();

                ImGui::PopStyleColor(2);
            }

            if (g_ui.isResizingLeftPanel) {
                g_ui.leftPanelW = ImGui::GetIO().MousePos.x;
            }

            if (g_ui.isResizingThreadPanel) {
                g_ui.threadPanelW = g_screen->nx - ImGui::GetIO().MousePos.x - 2;
            }

            ImGui::End();

            if (g_ui.showStyleEditor) {
                ImGui::SetNextWindowPos({ 20, 10 }, ImGuiCond_Once);
                ImGui::SetNextWindowSize({ 50, 24 }, ImGuiCond_Once);
                ImGui::Begin("Style editor", &g_ui.showStyleEditor);

                ImGui::Text("%s", "");

                {
                    ImGui::BeginChild("##Colors", ImVec2(0, ImGui::GetWindowSize().y - ImGui::GetCursorPosY() - 3), true);

                    bool changed = false;

                    changed |= ImGui::ColorEdit3("textFG",  (float *) &g_ui.colors.textFG);
                    changed |= ImGui::ColorEdit3("titleFG", (float *) &g_ui.colors.titleFG);

                    changed |= ImGui::ColorEdit3("itemBG",        (float *) &g_ui.colors.itemBG);
                    changed |= ImGui::ColorEdit3("itemBGHovered", (float *) &g_ui.colors.itemBGHovered);
                    changed |= ImGui::ColorEdit3("itemBGActive",  (float *) &g_ui.colors.itemBGActive);

                    changed |= ImGui::ColorEdit3("leftPanelBG",      (float *) &g_ui.colors.leftPanelBG);
                    changed |= ImGui::ColorEdit3("leftPanelTextFG",  (float *) &g_ui.colors.leftPanelTextFG);
                    changed |= ImGui::ColorEdit3("leftPanelTitleFG", (float *) &g_ui.colors.leftPanelTitleFG);

                    changed |= ImGui::ColorEdit3("mainWindowBG",      (float *) &g_ui.colors.mainWindowBG);
                    changed |= ImGui::ColorEdit3("mainWindowTextFG",  (float *) &g_ui.colors.mainWindowTextFG);
                    changed |= ImGui::ColorEdit3("mainWindowTitleFG", (float *) &g_ui.colors.mainWindowTitleFG);

                    changed |= ImGui::ColorEdit3("popupBG", (float *) &g_ui.colors.popupBG);

                    changed |= ImGui::ColorEdit3("listHeaderFG", (float *) &g_ui.colors.listHeaderFG);

                    changed |= ImGui::ColorEdit3("channelSelectedBG", (float *) &g_ui.colors.channelSelectedBG);
                    changed |= ImGui::ColorEdit3("channelActiveFG",   (float *) &g_ui.colors.channelActiveFG);
                    changed |= ImGui::ColorEdit3("channelMentionsFG", (float *) &g_ui.colors.channelMentionsFG);
                    changed |= ImGui::ColorEdit3("channelMentionsBG", (float *) &g_ui.colors.channelMentionsBG);

                    changed |= ImGui::ColorEdit3("userActiveFG", (float *) &g_ui.colors.userActiveFG);

                    changed |= ImGui::ColorEdit3("messageUser",    (float *) &g_ui.colors.messageUser);
                    changed |= ImGui::ColorEdit3("messageTime",    (float *) &g_ui.colors.messageTime);
                    changed |= ImGui::ColorEdit3("messageHovered", (float *) &g_ui.colors.messageHovered);
                    changed |= ImGui::ColorEdit3("messageReact",   (float *) &g_ui.colors.messageReact);
                    changed |= ImGui::ColorEdit3("messageReplies", (float *) &g_ui.colors.messageReplies);

                    changed |= ImGui::ColorEdit3("inputBG",            (float *) &g_ui.colors.inputBG);
                    changed |= ImGui::ColorEdit3("inputSendBG",        (float *) &g_ui.colors.inputSendBG);
                    changed |= ImGui::ColorEdit3("inputSendFG",        (float *) &g_ui.colors.inputSendFG);
                    changed |= ImGui::ColorEdit3("inputSendBGHovered", (float *) &g_ui.colors.inputSendBGHovered);
                    changed |= ImGui::ColorEdit3("inputSendBGActive",  (float *) &g_ui.colors.inputSendBGActive);
                    changed |= ImGui::ColorEdit3("inputTooltip",       (float *) &g_ui.colors.inputTooltip);

                    changed |= ImGui::ColorEdit3("buttonActiveFG", (float *) &g_ui.colors.buttonActiveFG);

                    changed |= ImGui::ColorEdit3("indicatorOnline",  (float *) &g_ui.colors.indicatorOnline);
                    changed |= ImGui::ColorEdit3("indicatorOffline", (float *) &g_ui.colors.indicatorOffline);

                    if (changed) {
                        g_ui.setStyle(UI::EStyle(-1));
                    }

                    ImGui::EndChild();
                }

                ImGui::Text("%s", "");

                if (ImGui::Button(" To File ")) {
                    std::string res;

                    auto to_s = [](const ImVec4 & col) {
                        return std::to_string(col.x) + ", " + std::to_string(col.y) + ", " + std::to_string(col.z) + ", " + std::to_string(col.w);
                    };

                    res += ".textFG  = { " + to_s(g_ui.colors.textFG)  + " },\n";
                    res += ".titleFG = { " + to_s(g_ui.colors.titleFG) + " },\n";

                    res += ".itemBG        = { " + to_s(g_ui.colors.itemBG)        + " },\n";
                    res += ".itemBGHovered = { " + to_s(g_ui.colors.itemBGHovered) + " },\n";
                    res += ".itemBGActive  = { " + to_s(g_ui.colors.itemBGActive)  + " },\n";

                    res += ".leftPanelBG      = { " + to_s(g_ui.colors.leftPanelBG)      + " },\n";
                    res += ".leftPanelTextFG  = { " + to_s(g_ui.colors.leftPanelTextFG)  + " },\n";
                    res += ".leftPanelTitleFG = { " + to_s(g_ui.colors.leftPanelTitleFG) + " },\n";

                    res += ".mainWindowBG      = { " + to_s(g_ui.colors.mainWindowBG)      + " },\n";
                    res += ".mainWindowTextFG  = { " + to_s(g_ui.colors.mainWindowTextFG)  + " },\n";
                    res += ".mainWindowTitleFG = { " + to_s(g_ui.colors.mainWindowTitleFG) + " },\n";

                    res += ".popupBG = { " + to_s(g_ui.colors.popupBG) + " },\n";

                    res += ".listHeaderFG = { " + to_s(g_ui.colors.listHeaderFG) + " },\n";

                    res += ".channelSelectedBG = { " + to_s(g_ui.colors.channelSelectedBG) + " },\n";
                    res += ".channelActiveFG   = { " + to_s(g_ui.colors.channelActiveFG)   + " },\n";
                    res += ".channelMentionsFG = { " + to_s(g_ui.colors.channelMentionsFG) + " },\n";
                    res += ".channelMentionsBG = { " + to_s(g_ui.colors.channelMentionsBG) + " },\n";

                    res += ".userActiveFG = { " + to_s(g_ui.colors.userActiveFG) + " },\n";

                    res += ".messageUser    = { " + to_s(g_ui.colors.messageUser)    + " },\n";
                    res += ".messageTime    = { " + to_s(g_ui.colors.messageTime)    + " },\n";
                    res += ".messageHovered = { " + to_s(g_ui.colors.messageHovered) + " },\n";
                    res += ".messageReact   = { " + to_s(g_ui.colors.messageReact)   + " },\n";
                    res += ".messageReplies = { " + to_s(g_ui.colors.messageReplies) + " },\n";

                    res += ".inputBG            = { " + to_s(g_ui.colors.inputBG)            + " },\n";
                    res += ".inputSendBG        = { " + to_s(g_ui.colors.inputSendBG)        + " },\n";
                    res += ".inputSendFG        = { " + to_s(g_ui.colors.inputSendFG)        + " },\n";
                    res += ".inputSendBGHovered = { " + to_s(g_ui.colors.inputSendBGHovered) + " },\n";
                    res += ".inputSendBGActive  = { " + to_s(g_ui.colors.inputSendBGActive)  + " },\n";
                    res += ".inputTooltip       = { " + to_s(g_ui.colors.inputTooltip)       + " },\n";

                    res += ".buttonActiveFG = { " + to_s(g_ui.colors.buttonActiveFG) + " },\n";

                    res += ".indicatorOnline  = { " + to_s(g_ui.colors.indicatorOnline)  + " },\n";
                    res += ".indicatorOffline = { " + to_s(g_ui.colors.indicatorOffline) + " },\n";

#ifndef __EMSCRIPTEN__
                    std::ofstream fout("style.h");
                    fout << res;
                    fout.close();
#endif
                }

                ImGui::Text("%s", "");

                ImGui::End();

            }

#ifndef __EMSCRIPTEN__
            if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape))) {
                g_isRunning = false;
            }
#endif

            ImGui::Render();

            ImTui_ImplText_RenderDrawData(ImGui::GetDrawData(), g_screen);

#ifndef __EMSCRIPTEN__
            ImTui_ImplNcurses_DrawScreen(isActive);
#endif

            return true;
        }
}

int main() {
    // initialize some random workspace data
    {
        // channels
        {
            g_ui.channels = {
                { false, 0, "compiler", "A channel about the compiler",       {}, {} },
                { false, 0, "random",   "Random thoughts",                    {}, {} },
                { false, 0, "general",  "The main channel about ImTui",       {}, {} },
                { true,  0, "rust",     "Tell us how great this language is", {}, {} },
                { false, 0, "c++",      "The best language",                  {}, {} },
                { false, 2, "imtui",    "The best TUI library",               {}, {} },
                { false, 0, "reddit",   "Dive into anything",                 {}, {} },
                { false, 0, "hn",       "Hacker News",                        {}, {} },
                { true,  0, "news",     "Mainstream media news",              {}, {} },
                { false, 3, "builds",   "Build reports",                      {}, {} },
            };

            g_ui.selectedChannel = 2;
        }

        // users
        {
            g_ui.users = {
                {  0, true , false, "Georgi Gerganov", "", {}},
                {  1, false, false, "John Doe",        "", {}},
                {  2, false, false, "Betty Basil",     "", {}},
                {  3, true , true,  "Chace Jordana",   "", {}},
                {  4, true , false, "Elon Musk",       "", {}},
                {  5, false, false, "Vin Kennedi",     "", {}},
                {  6, true , false, "Kilie Katlyn",    "", {}},
                {  7, false, false, "Kaeden Gil",      "", {}},
                {  8, false, false, "Mary Jane",       "", {}},
                {  9, true , true,  "Adair Rigby",     "", {}},
                { 10, true , false, "Bryce Bekki",     "", {}},
            };

            g_ui.selectedUser = -1;

            // assign users to random set of channels
            for (auto & user : g_ui.users) {
                const int nMembership = rand()%g_ui.channels.size() + 1;

                for (int i = 0; i < nMembership; i++) {
                    int channelIndex = rand()%g_ui.channels.size();
                    while (g_ui.channels[channelIndex].isMember(user.id)) {
                        channelIndex = rand()%g_ui.channels.size();
                    }
                    g_ui.channels[channelIndex].members.push_back(user);
                }
            }
        }

        // filter the logs, because we only support single-byte strings
        std::vector<Log> logs;
        for (auto & cur : kLogs) {
            std::string text;
            for (auto & ch : cur.text) {
                if (ch < 32) {
                    text += "@";
                } else if (ch == '<') {
                    text += "@lt;";
                } else if (ch == '>') {
                    text += "@gt;";
                } else if (ch == '&') {
                    text += "@amp;";
                } else {
                    text += ch;
                }
            }

            logs.push_back({ cur.username, std::move(text) });
        }

        // generate random channel messages
        for (auto & channel : g_ui.channels) {
            auto t = tGet() - 24*3600*(rand()%100 + 10);

            int lastUid = -1;
            int logId = 1 + rand()%(logs.size() - 2);
            const int nMessages = rand()%100 + 10;

            for (int i = 0; i < nMessages; ++i) {
                const bool isSameUser = (logs[logId - 1].username == logs[logId].username);

                int uid = lastUid == -1 || !isSameUser ? rand()%g_ui.users.size() : lastUid;
                while (!isSameUser && uid == lastUid) {
                    uid = rand()%g_ui.users.size();
                }
                lastUid = uid;

                if (isSameUser) {
                    t += rand()%60;
                } else {
                    t += rand()%3600;
                }

                int nReactUp   = rand()%100 > 80 ? rand()%5 : 0;
                int nReactDown = rand()%100 > 90 ? rand()%5 : 0;

                if (logs[logId + 1].username == logs[logId].username) {
                    nReactUp = 0;
                    nReactDown = 0;
                }

                channel.messages.push_back({ {
                    t, uid, logs[logId].text, nReactUp, nReactDown, }, {}
                });

                // random threads
                const int nReplies = rand()%100 > 90 ? rand()%30 : 0;
                for (int j = 0; j < nReplies; ++j) {
                    t += rand()%60;
                    logId = std::max((int)((logId + 1)%(logs.size() - 1)), 1);

                    nReactUp   = rand()%100 > 80 ? rand()%5 : 0;
                    nReactDown = rand()%100 > 90 ? rand()%5 : 0;

                    channel.messages.back().replies.push_back({ {
                        t, (int) (rand()%g_ui.users.size()), logs[logId].text, 0, 0,
                    }, {} });
                }

                logId = std::max((int)((logId + 1)%(logs.size() - 1)), 1);
            }
        }

        // generate random DMs
        for (auto & user : g_ui.users) {
            if (user.id == 0) continue;

            auto t = tGet() - 24*3600*(rand()%100 + 10);

            int lastUid = -1;
            int logId = 1 + rand()%(logs.size() - 2);
            const int nMessages = rand()%100 + 10;

            for (int i = 0; i < nMessages; ++i) {
                const bool isSameUser = (logs[logId - 1].username == logs[logId].username);

                int uid = lastUid == -1 || !isSameUser ? rand()%g_ui.users.size() : lastUid;
                while (!isSameUser && (uid == lastUid || (uid != 0 && uid != user.id))) {
                    uid = rand()%g_ui.users.size();
                }
                lastUid = uid;

                if (isSameUser) {
                    t += rand()%60;
                } else {
                    t += rand()%3600;
                }

                int nReactUp   = rand()%100 > 80 ? rand()%5 : 0;
                int nReactDown = rand()%100 > 90 ? rand()%5 : 0;

                if (logs[logId + 1].username == logs[logId].username) {
                    nReactUp = 0;
                    nReactDown = 0;
                }

                user.messages.push_back({ {
                    t, uid, logs[logId].text, nReactUp, nReactDown, }, {}
                });

                logId = std::max((int)((logId + 1)%(logs.size() - 1)), 1);
            }
        }
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    g_ui.setStyle(UI::EStyle_Dark);

    ImGui::GetIO().IniFilename = nullptr;

#ifdef __EMSCRIPTEN__
    g_screen = ImTui_ImplEmscripten_Init(true);
#else
    // when no changes occured - limit frame rate to 3.0 fps to save CPU
    g_screen = ImTui_ImplNcurses_Init(true, 60.0, 3.0);
#endif

    ImTui_ImplText_Init();

#ifndef __EMSCRIPTEN__
    while (g_isRunning) {
        if (render_frame() == false) break;
    }

    ImTui_ImplText_Shutdown();
    ImTui_ImplNcurses_Shutdown();
#endif

    return 0;
}
