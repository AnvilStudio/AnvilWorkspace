#pragma once

#include <Anvil.h>

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>

namespace anv
{
    enum class ConsoleLogLevel : uint8_t
    {
        Trace = 0,
        Info,
        Warning,
        Error,
        Critical
    };

    struct ConsoleMessage
    {
        uint64_t id = 0;
        ConsoleLogLevel level = ConsoleLogLevel::Info;

        std::string timestamp;
        std::string message;
        std::string source;
    };

    class ConsolePanel
    {
    public:
        ConsolePanel() = default;

        void Draw(bool *open = nullptr);

        void AddMessage(
            ConsoleLogLevel level,
            std::string message,
            std::string source = {});

        void Clear();

        void SetMaximumMessages(size_t maximumMessages);

        void AddLogRecord(
            const anv_log::LogRecord &record);

    private:
        void DrawToolbar();
        void DrawMessages();

        bool PassesFilter(
            const ConsoleMessage &message) const;

        static const char *GetLevelName(
            ConsoleLogLevel level);

        static ImVec4 GetLevelColor(
            ConsoleLogLevel level);

        static std::string GetCurrentTimestamp();

    private:
        mutable std::mutex m_MessageMutex;
        std::deque<ConsoleMessage> m_Messages;

        std::atomic<uint64_t> m_NextMessageId = 1;

        size_t m_MaximumMessages = 2000;

        char m_SearchBuffer[256]{};

        bool m_ShowTrace = true;
        bool m_ShowInfo = true;
        bool m_ShowWarnings = true;
        bool m_ShowErrors = true;
        bool m_ShowCritical = true;

        bool m_AutoScroll = true;
        std::atomic<bool> m_ScrollToBottom = false;
    };
}