/*! \file common.h
 *  \brief Enter description here.
 */

#pragma once

#include <thread>
#include <chrono>

struct VSync {
    VSync(double rate_fps = 60.0) : tStep_us(1000000.0/rate_fps) {}

    uint64_t tStep_us;
    uint64_t tLast_us = t_us();
    uint64_t tNext_us = tLast_us + tStep_us;

    inline uint64_t t_us() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count(); // duh ..
    }

    inline void wait() {
        uint64_t tNow_us = t_us();
        while (tNow_us < tNext_us - 100) {
            std::this_thread::sleep_for(std::chrono::microseconds((uint64_t) (0.9*(tNext_us - tNow_us))));
            tNow_us = t_us();
        }

        tNext_us += tStep_us;
    }

    inline float delta_s() {
        uint64_t tNow_us = t_us();
        uint64_t res = tNow_us - tLast_us;
        tLast_us = tNow_us;
        return float(res)/1e6f;
    }
};
