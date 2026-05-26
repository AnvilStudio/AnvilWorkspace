// Profiler.h
#pragma once
#include "AnvLog/AnvLog.h"
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

            anv_log::AnvLog::LOG_CUST(anv_log::TermColor::TC_CYAN, anv_log::LogLevel::LL_NONE, 
                "[PROFILE]: %s :: %.2f ms", m_ScopeName, duration);
        }

    private:
        const char* m_ScopeName;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
    };

    class FPSCounter
    {
    public:
        void Update(float dt)
        {
            m_DeltaHistory[m_CurrentIndex] = dt;

            m_CurrentIndex =
                (m_CurrentIndex + 1) % SAMPLE_COUNT;

            if (m_Filled < SAMPLE_COUNT)
                m_Filled++;

            float total = 0.0f;

            for (uint32_t i = 0; i < m_Filled; i++)
                total += m_DeltaHistory[i];

            float averageDT =
                total / (float)m_Filled;

            m_FPS =
                averageDT > 0.0f
                ? (1.0f / averageDT)
                : 0.0f;
        }

        uint32_t GetFPS() const
        {
            return (uint32_t)m_FPS;
        }

    private:
        static constexpr uint32_t SAMPLE_COUNT = 10;

        float m_DeltaHistory[SAMPLE_COUNT]{};

        uint32_t m_CurrentIndex = 0;
        uint32_t m_Filled = 0;

        float m_FPS = 0.0f;
    };
}
