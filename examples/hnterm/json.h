/*! \file json.h
 *  \brief Json parsing
 */

#pragma once

#include <map>
#include <vector>
#include <string>

namespace JSON {

std::vector<int> parseIntArray(const std::string & json) {
    std::vector<int> res;
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

std::map<std::string, std::string> parseJSONMap(const std::string & json) {
    std::map<std::string, std::string> res;
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

}
