/*! \file hn-state.h
 *  \brief HN::State
 */

#pragma once

#include <map>
#include <string>
#include <vector>
#include <variant>
#include <cstdint>

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

    uint64_t lastForceUpdate_s = 0;

    std::variant<Story, Comment, Job, Poll, PollOpt> data;
};

struct State {
    bool update(const ItemIds & toRefresh);
    void forceUpdate(const ItemIds & toUpdate);

    bool timeout(uint64_t now, uint64_t last) const;
    std::string timeSince(uint64_t t) const;

    ItemIds idsTop;
    //ItemIds idsBest;
    ItemIds idsShow;
    ItemIds idsAsk;
    ItemIds idsNew;

    std::map<ItemId, Item> items;

    int nFetches = 0;
    uint64_t totalBytesDownloaded = 0;

    uint64_t lastUpdatePoll_s = 0;

    char curURI[512];
    int nextUpdate = 0;

    private:
    void requestJSONForURI(std::string uri);
};

}

