/*! \file impl-ncurses.cpp
 *  \brief Enter description here.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <curl/curl.h>

#include <map>
#include <array>
#include <deque>
#include <string>
#include <chrono>

#define MAX_PARALLEL 5

//#define DEBUG_SIGPIPE

#if defined(DEBUG_SIGPIPE) || defined(ENABLE_API_CACHE)
#include <fstream>
#endif

namespace {
#ifdef ENABLE_API_CACHE
    inline std::string getCacheFname(const std::string & uri) {
        std::string fname = "cache-" + uri;
        for (auto & ch : fname) {
            if ((ch >= 'a' && ch <= 'z') ||
                (ch >= 'A' && ch <= 'Z') ||
                (ch >= '0' && ch <= '9')) {
                continue;
            }
            ch = '-';
        }

        return fname;
    }
#endif
}

void sigpipe_handler([[maybe_unused]] int signal) {
#ifdef DEBUG_SIGPIPE
    std::ofstream fout("SIGPIPE.OCCURED");
    fout << signal << std::endl;
    fout.close();
#endif
}

struct Data {
    CURL *eh = NULL;
    bool running = false;
    std::string uri = "";
    std::string content = "";
};

static CURLM *g_cm;

static int g_nFetches = 0;
static uint64_t g_totalBytesDownloaded = 0;
static std::deque<std::string> g_fetchQueue;
static std::map<std::string, std::string> g_fetchCache;
static std::array<Data, MAX_PARALLEL> g_fetchData;

uint64_t t_s() {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count(); // duh ..
}

static size_t writeFunction(void *ptr, size_t size, size_t nmemb, Data* data) {
    size_t bytesDownloaded = size*nmemb;
    g_totalBytesDownloaded += bytesDownloaded;

    data->content.append((char*) ptr, bytesDownloaded);

#ifdef ENABLE_API_CACHE
    auto fname = ::getCacheFname(data->uri);

    std::ofstream fout(fname);
    fout.write(data->content.c_str(), data->content.size());
    fout.close();
#endif

    g_fetchCache[data->uri] = std::move(data->content);
    data->content.clear();

    return bytesDownloaded;
}

static void addTransfer(CURLM *cm, int idx, std::string && uri) {
    if (g_fetchData[idx].eh == NULL) {
        g_fetchData[idx].eh = curl_easy_init();
    }

    CURL *eh = g_fetchData[idx].eh;
    curl_easy_setopt(eh, CURLOPT_URL, uri.c_str());
    curl_easy_setopt(eh, CURLOPT_PRIVATE, &g_fetchData[idx]);
    curl_easy_setopt(eh, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(eh, CURLOPT_WRITEDATA, &g_fetchData[idx]);
    curl_multi_add_handle(cm, eh);
}

bool hnInit() {
#ifndef _WIN32
    struct sigaction sh;
    struct sigaction osh;

    sh.sa_handler = &sigpipe_handler; //Can set to SIG_IGN
    // Restart interrupted system calls
    sh.sa_flags = SA_RESTART;

    // Block every signal during the handler
    sigemptyset(&sh.sa_mask);

    if (sigaction(SIGPIPE, &sh, &osh) < 0) {
        return false;
    }
#endif

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
#ifdef __APPLE__
    std::string cmd = "open " + uri + " > /dev/null 2>&1";
#else
    std::string cmd = "xdg-open " + uri + " > /dev/null 2>&1";
#endif
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

uint64_t getTotalBytesDownloaded() {
    return g_totalBytesDownloaded;
}

uint64_t getNFetches() {
    return g_nFetches;
}

void requestJSONForURI_impl(std::string uri) {
    g_fetchQueue.push_back(std::move(uri));
}

void updateRequests_impl() {
    CURLMsg *msg;
    int msgs_left = -1;

    while ((msg = curl_multi_info_read(g_cm, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            Data* data;
            CURL *e = msg->easy_handle;
            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &data);
            data->running = false;
            curl_multi_remove_handle(g_cm, e);
            //curl_easy_cleanup(e);
            //data->eh = NULL;
        } else {
            fprintf(stderr, "E: CURLMsg (%d)\n", msg->msg);
        }
    }

    int still_alive = 1;

    curl_multi_perform(g_cm, &still_alive);

    while (still_alive < MAX_PARALLEL && g_fetchQueue.size() > 0) {
        long unsigned int idx = 0;
        while (g_fetchData[idx].running) {
            ++idx;
            if (idx == g_fetchData.size()) break;
        }
        if (idx == g_fetchData.size()) break;

        auto uri = std::move(g_fetchQueue.front());
        g_fetchQueue.pop_front();

#ifdef ENABLE_API_CACHE
        auto fname = ::getCacheFname(uri);

        std::ifstream fin(fname);
        if (fin.is_open() && fin.good()) {
            std::string str((std::istreambuf_iterator<char>(fin)), std::istreambuf_iterator<char>());
            fin.close();

            g_fetchCache[uri] = std::move(str);

            continue;
        }
#endif

        ++g_nFetches;

        g_fetchData[idx].running = true;
        g_fetchData[idx].uri = uri;
        addTransfer(g_cm, idx, std::move(uri));

        ++still_alive;
    }
}
