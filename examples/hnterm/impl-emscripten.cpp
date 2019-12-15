/*! \file impl-emscripten.cpp
 *  \brief Enter description here.
 */

#include <emscripten.h>
#include <emscripten/fetch.h>

#include <map>
#include <mutex>
#include <string>

// not sure if this mutex is needed, but just in case
static std::mutex g_mutex;
static std::map<std::string, std::string> g_fetchCache;

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
    //printf("Finished downloading %llu bytes from URL %s.\n", fetch->numBytes, fetch->url);
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_fetchCache[fetch->url] = std::string(fetch->data, fetch->numBytes-1);
    }
    emscripten_fetch_close(fetch);
}

void downloadFailed(emscripten_fetch_t *fetch) {
    printf("Downloading %s failed, HTTP failure status code: %d.\n", fetch->url, fetch->status);
    emscripten_fetch_close(fetch);
}

std::string getJSONForURI_cache(std::string uri) {
    std::string res;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        res = std::move(g_fetchCache[uri] );
        g_fetchCache[uri] = "";
    }
    return res;
}

std::string getJSONForURI_impl(std::string uri) {
    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = downloadSucceeded;
    attr.onerror = downloadFailed;
    emscripten_fetch(&attr, uri.c_str());

    // Synchronous version - requires -s USE_PTHREADS=1
    //emscripten_fetch_attr_t attr;
    //emscripten_fetch_attr_init(&attr);
    //strcpy(attr.requestMethod, "GET");
    //attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_WAITABLE;
    //emscripten_fetch_t *fetch = emscripten_fetch(&attr, uri.c_str());
    //assert(fetch != 0);
    //memset(&attr, 0, sizeof(attr));
    //emscripten_fetch_wait(fetch, 1000000);
    //assert(fetch->data != 0);
    //assert(fetch->numBytes > 0);
    //assert(fetch->totalBytes == fetch->numBytes);
    //assert(fetch->readyState == 4/*DONE*/);
    //assert(fetch->status == 200);
    //if (fetch->status == 200) {
    //    res = std::string(fetch->data, fetch->numBytes-1);
    //} else {
    //}
    //emscripten_fetch_close(fetch);
    return "";
}
