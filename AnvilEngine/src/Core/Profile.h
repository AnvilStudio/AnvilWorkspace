// Profiler.h
#pragma once
#include <chrono>
#include <iostream>

#define ANV_PROFILE_SCOPE() Profiler _profile_scope(__FUNCTION__)

namespace anv
{
    class Profiler {
    public:
        Profiler(const char* scopeName)
            : m_ScopeName(scopeName), m_StartTime(std::chrono::high_resolution_clock::now()) {
            // Start timing
        }

        ~Profiler() {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_StartTime).count();
            // Output the timing or store it in a profiling system
            std::cout << "Profile [" << m_ScopeName << "] took " << duration << "us\n";
        }

    private:
        const char* m_ScopeName;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    };
}
