/*! \file impl-ncurses.cpp
 *  \brief Enter description here.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <curl/curl.h>

#include <map>
#include <deque>
#include <string>
#include <chrono>

#define MAX_PARALLEL 8

struct Data {
    bool running = false;
    std::string uri = "";
    std::string content = "";
};

static CURLM *g_cm;

static std::deque<std::string> g_fetchQueue;
static std::map<std::string, std::string> g_fetchCache;
static std::array<Data, MAX_PARALLEL> g_fetchData;

uint64_t t_s() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); // duh ..
}

size_t writeFunction(void *ptr, size_t size, size_t nmemb, Data* data) {
    data->content.append((char*) ptr, size * nmemb);
    g_fetchCache[data->uri] = std::move(data->content);
    data->content.clear();
    data->running = false;

    return size * nmemb;
}

static void add_transfer(CURLM *cm, int idx, std::string && uri) {
    CURL *eh = curl_easy_init();
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(eh, CURLOPT_URL, uri.c_str());
    curl_easy_setopt(eh, CURLOPT_PRIVATE, uri.c_str());
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, &g_fetchData[idx]);
    curl_multi_add_handle(cm, eh);
}

bool hnInit() {
    curl_global_init(CURL_GLOBAL_ALL);
    g_cm = curl_multi_init();

    curl_multi_setopt(g_cm, CURLMOPT_MAXCONNECTS, (long)MAX_PARALLEL);

    return true;
}

void hnFree() {
    curl_multi_cleanup(g_cm);
    curl_global_cleanup();
}

int openInBrowser(std::string uri) {
    // todo : on Mac OS use "open"
    std::string cmd = "xdg-open " + uri + " > /dev/null 2>&1";
    return system(cmd.c_str());
}

std::string getJSONForURI_impl(const std::string & uri) {
    if (auto it = g_fetchCache.find(uri); it != g_fetchCache.end()) {
        auto res = std::move(g_fetchCache[uri]);
        g_fetchCache.erase(it);

        return res;
    }

    return "";
}

void requestJSONForURI_impl(std::string uri) {
    g_fetchQueue.push_back(std::move(uri));
}

void updateRequests_impl() {
    CURLMsg *msg;
    int msgs_left = -1;

    while ((msg = curl_multi_info_read(g_cm, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            char *url;
            CURL *e = msg->easy_handle;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &url);
            //fprintf(stderr, "R: %d - %s <%s>\n", msg->data.result, curl_easy_strerror(msg->data.result), url);
            curl_multi_remove_handle(g_cm, e);
            curl_easy_cleanup(e);
        } else {
            fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
        }
    }

    int still_alive = 1;

    curl_multi_perform(g_cm, &still_alive);

    while (still_alive < MAX_PARALLEL && g_fetchQueue.size() > 0) {
        auto uri = std::move(g_fetchQueue.front());
        int idx = 0;
        while (g_fetchData[idx].running) ++idx;
        g_fetchData[idx].running = true;
        g_fetchData[idx].uri = uri;
        g_fetchQueue.pop_front();
        add_transfer(g_cm, idx, std::move(uri));
        ++still_alive;
    }
}
