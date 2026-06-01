#pragma once
#include <chrono>

namespace anv
{
    class ScopedTimer
    {
    public:
        ScopedTimer(float& result)
            : m_Result(result)
        {
            m_Start = std::chrono::high_resolution_clock::now();
        }

        ~ScopedTimer()
        {
            auto end =
                std::chrono::high_resolution_clock::now();

            m_Result =
                std::chrono::duration<float, std::milli>(
                    end - m_Start
                ).count();
        }

    private:
        std::chrono::high_resolution_clock::time_point m_Start;
        float& m_Result;
    };
}