#pragma once
#include <chrono>

namespace anv
{
    class Time
    {
    public:
        static void Init()
        {
            s_StartTime = Clock::now();
            s_LastFrame = s_StartTime;
            s_DeltaTime = 0.0f;
        }

        static void Update()
        {
            auto now = Clock::now();

            s_DeltaTime =
                std::chrono::duration<float>(now - s_LastFrame).count();

            s_LastFrame = now;
        }

        // Seconds since app launch
        static float GetTime()
        {
            return std::chrono::duration<float>(
                Clock::now() - s_StartTime
            ).count();
        }

        // Seconds between frames
        static float DeltaTime()
        {
            return s_DeltaTime;
        }

    private:
        using Clock = std::chrono::steady_clock;

        inline static Clock::time_point s_StartTime;
        inline static Clock::time_point s_LastFrame;

        inline static float s_DeltaTime = 0.0f;
    };
}