/*! \file impl-ncurses.cpp
 *  \brief Enter description here.
 */

#include <curl/curl.h>

#include <string>

void * g_curl = nullptr;

bool hnInit() {
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

void hnFree() {
    if (g_curl) {
        curl_easy_cleanup(g_curl);
    }
    g_curl = NULL;
}

int openInBrowser(std::string uri) {
    std::string cmd = "xdg-open " + uri;
    return system(cmd.c_str());
}

size_t writeFunction(void *ptr, size_t size, size_t nmemb, std::string* data) {
    data->append((char*) ptr, size * nmemb);
    return size * nmemb;
}

std::string getJSONForURI_impl(std::string uri) {
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

    return response_string;
}
