/*! \file impl-emscripten.cpp
 *  \brief Enter description here.
 */

#include <emscripten.h>
#include <emscripten/fetch.h>

#include <map>
#include <mutex>
#include <string>

static int g_nFetches;
static uint64_t g_totalBytesDownloaded = 0;

// not sure if this mutex is needed, but just in case
static std::mutex g_mutex;
static std::map<std::string, std::string> g_fetchCache;

uint64_t t_s() {
    return emscripten_date_now()*0.001f;
}

bool hnInit() {
    return true;
}

void hnFree() {
}

int openInBrowser(std::string uri) {
    std::string cmd = "window.open('" + uri + "');";
    emscripten_run_script(cmd.c_str());

    return 0;
}

void downloadSucceeded(emscripten_fetch_t *fetch) {
    g_totalBytesDownloaded += fetch->numBytes;

    //printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_fetchCache[fetch->url] = std::string(fetch->data, fetch->numBytes-1);
    }
    emscripten_fetch_close(fetch);
}

void downloadFailed(emscripten_fetch_t *fetch) {
    fprintf(stderr, "Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
    emscripten_fetch_close(fetch);
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

int getNFetches() {
    return g_nFetches;
}

void requestJSONForURI_impl(std::string uri) {
    ++g_nFetches;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    emscripten_fetch(&attr, uri.c_str());
}

void updateRequests_impl() {
}
