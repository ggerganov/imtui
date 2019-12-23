/*! \file hn-state.cpp
 * \brief HN::State
 */

#include "hn-state.h"

#include "json.h"

extern void requestJSONForURI_impl(std::string uri);
extern std::string getJSONForURI_impl(const std::string & uri);
extern uint64_t getTotalBytesDownloaded();
extern int getNFetches();
extern void updateRequests_impl();
extern uint64_t t_s();

namespace {

    void replaceAll(std::string & s, const std::string & search, const std::string & replace) {
        for (size_t pos = 0; ; pos += replace.length()) {
            pos = s.find(search, pos);
            if (pos == std::string::npos) break;
            s.erase(pos, search.length());
            s.insert(pos, replace);
        }
    }

    std::string getJSONForURI(const std::string & uri) {
        auto responseString = getJSONForURI_impl(uri);
        if (responseString == "") return "";

        return responseString;
    }

}

namespace HN {

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


    bool State::update(const ItemIds & toRefresh) {
        bool updated = false;

        auto now = ::t_s();

        if (timeout(now, lastUpdatePoll_s)) {
            requestJSONForURI(HN::kAPITopStories);
            //requestJSONForURI(HN::kAPIBestStories);
            requestJSONForURI(HN::kAPIShowStories);
            requestJSONForURI(HN::kAPIAskStories);
            requestJSONForURI(HN::kAPINewStories);
            requestJSONForURI(HN::kAPIUpdates);

            lastUpdatePoll_s = ::t_s();
            updated = true;
        } else {
            nextUpdate = lastUpdatePoll_s + 30 - now;
        }

        {
            {
                auto ids = HN::getStoriesIds(HN::kAPITopStories);
                if (ids.empty() == false) {
                    idsTop = std::move(ids);
                    updated = true;
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
                    updated = true;
                }
            }

            {
                auto ids = HN::getStoriesIds(HN::kAPIAskStories);
                if (ids.empty() == false) {
                    idsAsk = std::move(ids);
                    updated = true;
                }
            }

            {
                auto ids = HN::getStoriesIds(HN::kAPINewStories);
                if (ids.empty() == false) {
                    idsNew = std::move(ids);
                    updated = true;
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

            requestJSONForURI(getItemURI(id));
            items[id].needRequest = false;
            updated = true;
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
                            updated = true;
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
                            updated = true;
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
                            updated = true;
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

        nFetches = getNFetches();
        totalBytesDownloaded = getTotalBytesDownloaded();

        updateRequests_impl();

        return updated;
    }

    void State::forceUpdate(const ItemIds & toUpdate) {
        auto tNow_s = t_s();
        for (auto id : toUpdate) {
            if (items.find(id) == items.end()) continue;
            if (tNow_s - items[id].lastForceUpdate_s > 60) {
                items[id].needUpdate = true;
                items[id].needRequest = true;

                items[id].lastForceUpdate_s = tNow_s;
            }
        }
    }

    bool State::timeout(uint64_t now, uint64_t last) const {
        return now - last > 30;
    }

    std::string State::timeSince(uint64_t t) const {
        auto delta = t_s() - t;
        if (delta < 60) return std::to_string(delta) + " seconds";
        if (delta < 3600) return std::to_string(delta/60) + " minutes";
        if (delta < 24*3600) return std::to_string(delta/3600) + " hours";
        return std::to_string(delta/24/3600) + " days";
    }

    void State::requestJSONForURI(std::string uri) {
        snprintf(curURI, 512, "%s", uri.c_str());

        requestJSONForURI_impl(std::move(uri));
    }

}
