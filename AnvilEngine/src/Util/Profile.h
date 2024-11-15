// Profiler.h
#pragma once
#include <chrono>
#include <iostream>

namespace anv
{
    class Profiler {
    public:
        Profiler(const char* scopeName)
            : m_ScopeName(scopeName), m_StartTime(std::chrono::high_resolution_clock::now()) {
            std::cout << "\033[38;5;128mStarting Profile [ "<< m_ScopeName <<"]\033[0m\n";
        }

        ~Profiler() {
            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - m_StartTime).count();
            // Output the timing or store it in a profiling system
            std::cout << "\033[38;5;128mProfile [" << m_ScopeName << "] took " << duration * .001 << "ms\033[0m\n";
        }

    private:
        const char* m_ScopeName;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    };
}
