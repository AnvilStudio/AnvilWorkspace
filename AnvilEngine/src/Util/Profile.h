// Profiler.h
#pragma once
#include "AnvLog/include/AnvLog.h"
#include <chrono>
#include <iostream>

namespace anv
{
    class Profiler {
    public:
        Profiler(const char* scopeName)
            : m_ScopeName(scopeName), m_StartTime(std::chrono::high_resolution_clock::now()) {
        }

        ~Profiler() {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_StartTime).count() * .001;
            // Output the timing or store it in a profiling system
            anv_log::AnvLog::LOG_DEBUG("[PROFILE]: %s :: %.2f ms", m_ScopeName, duration);
        }

    private:
        const char* m_ScopeName;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    };
}
