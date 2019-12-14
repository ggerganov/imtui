/*! \file impl-emscripten.cpp
 *  \brief Enter description here.
 */

#include <emscripten.h>
#include <emscripten/fetch.h>

#include <string>

bool hnInit() {
    return true;
}

void hnFree() {
}

std::string getJSONForURI_impl(std::string uri) {
    std::string res = "";

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_SYNCHRONOUS | EMSCRIPTEN_FETCH_WAITABLE;
    emscripten_fetch_t *fetch = emscripten_fetch(&attr, uri.c_str());
    assert(fetch != 0);
    memset(&attr, 0, sizeof(attr));
    emscripten_fetch_wait(fetch, 1000000);
    assert(fetch->data != 0);
    assert(fetch->numBytes > 0);
    assert(fetch->totalBytes == fetch->numBytes);
    assert(fetch->readyState == 4/*DONE*/);
    assert(fetch->status == 200);
    if (fetch->status == 200) {
        res = std::string(fetch->data, fetch->numBytes-1);
    } else {
    }
    emscripten_fetch_close(fetch);
    return res;
}
