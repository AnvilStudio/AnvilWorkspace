#include "ConsolePanel.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

namespace anv
{
    namespace
    {
        std::string ToLower(std::string value)
        {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char character)
                {
                    return static_cast<char>(
                        std::tolower(character));
                });

            return value;
        }

        bool ContainsInsensitive(
            const std::string &value,
            const std::string &search)
        {
            if (search.empty())
                return true;

            return ToLower(value).find(ToLower(search)) != std::string::npos;
        }
    }

    void ConsolePanel::Draw(bool *open)
    {
        if (open && !*open)
            return;

        if (!ImGui::Begin("Console", open))
        {
            ImGui::End();
            return;
        }

        DrawToolbar();

        ImGui::Separator();

        DrawMessages();

        ImGui::End();
    }

    void ConsolePanel::AddMessage(
        ConsoleLogLevel level,
        std::string message,
        std::string source)
    {
        ConsoleMessage consoleMessage;

        consoleMessage.id =
            m_NextMessageId.fetch_add(
                1,
                std::memory_order_relaxed);

        consoleMessage.level = level;
        consoleMessage.timestamp =
            GetCurrentTimestamp();

        consoleMessage.message =
            std::move(message);

        consoleMessage.source =
            std::move(source);

        {
            std::scoped_lock lock(m_MessageMutex);

            m_Messages.push_back(
                std::move(consoleMessage));

            while (m_Messages.size() >
                   m_MaximumMessages)
            {
                m_Messages.pop_front();
            }
        }

        m_ScrollToBottom = true;
    }

    void ConsolePanel::Clear()
    {
        std::scoped_lock lock(m_MessageMutex);

        m_Messages.clear();
    }

    void ConsolePanel::SetMaximumMessages(
        size_t maximumMessages)
    {
        m_MaximumMessages =
            std::max<size_t>(1, maximumMessages);

        std::scoped_lock lock(m_MessageMutex);

        while (m_Messages.size() >
               m_MaximumMessages)
        {
            m_Messages.pop_front();
        }
    }

    void ConsolePanel::AddLogRecord(
        const anv_log::LogRecord &record)
    {
        ConsoleMessage message;

        message.id =
            m_NextMessageId.fetch_add(
                1,
                std::memory_order_relaxed);

        switch (record.level)
        {
        case anv_log::LogLevel::LL_DEBUG:
            message.level = ConsoleLogLevel::Trace;
            break;

        case anv_log::LogLevel::LL_INFO:
            message.level = ConsoleLogLevel::Info;
            break;

        case anv_log::LogLevel::LL_WARN:
            message.level = ConsoleLogLevel::Warning;
            break;

        case anv_log::LogLevel::LL_ERROR:
            message.level = ConsoleLogLevel::Error;
            break;

        case anv_log::LogLevel::LL_FATAL:
            message.level = ConsoleLogLevel::Critical;
            break;

        case anv_log::LogLevel::LL_NONE:
        default:
            message.level = ConsoleLogLevel::Info;
            break;
        }

        message.timestamp = record.timestamp;
        message.message = record.message;
        message.source = record.source;

        {
            std::scoped_lock lock(m_MessageMutex);

            m_Messages.push_back(std::move(message));

            while (m_Messages.size() > m_MaximumMessages)
                m_Messages.pop_front();
        }

        m_ScrollToBottom.store(
            true,
            std::memory_order_relaxed);
    }

    void ConsolePanel::DrawToolbar()
    {
        if (ImGui::Button("Clear"))
            Clear();

        ImGui::SameLine();

        if (ImGui::Button("Copy"))
        {
            std::vector<ConsoleMessage> snapshot;

            {
                std::scoped_lock lock(
                    m_MessageMutex);

                snapshot.assign(
                    m_Messages.begin(),
                    m_Messages.end());
            }

            std::ostringstream output;

            for (const auto &message : snapshot)
            {
                if (!PassesFilter(message))
                    continue;

                output
                    << "["
                    << message.timestamp
                    << "] ["
                    << GetLevelName(message.level)
                    << "] ";

                if (!message.source.empty())
                {
                    output
                        << "["
                        << message.source
                        << "] ";
                }

                output
                    << message.message
                    << '\n';
            }

            ImGui::SetClipboardText(
                output.str().c_str());
        }

        ImGui::SameLine();

        ImGui::Checkbox(
            "Auto-scroll",
            &m_AutoScroll);

        ImGui::SameLine();

        ImGui::SetNextItemWidth(260.0f);

        ImGui::InputTextWithHint(
            "##ConsoleSearch",
            "Search messages...",
            m_SearchBuffer,
            sizeof(m_SearchBuffer));

        ImGui::Spacing();

        ImGui::Checkbox(
            "Trace",
            &m_ShowTrace);

        ImGui::SameLine();

        ImGui::Checkbox(
            "Info",
            &m_ShowInfo);

        ImGui::SameLine();

        ImGui::Checkbox(
            "Warnings",
            &m_ShowWarnings);

        ImGui::SameLine();

        ImGui::Checkbox(
            "Errors",
            &m_ShowErrors);

        ImGui::SameLine();

        ImGui::Checkbox(
            "Critical",
            &m_ShowCritical);
    }

    void ConsolePanel::DrawMessages()
    {
        std::vector<ConsoleMessage> snapshot;

        {
            std::scoped_lock lock(m_MessageMutex);

            snapshot.assign(
                m_Messages.begin(),
                m_Messages.end());
        }

        constexpr ImGuiTableFlags tableFlags =
            ImGuiTableFlags_BordersInnerV |
            ImGuiTableFlags_RowBg |
            ImGuiTableFlags_Resizable |
            ImGuiTableFlags_ScrollY |
            ImGuiTableFlags_SizingStretchProp;

        if (!ImGui::BeginTable(
                "ConsoleMessages",
                4,
                tableFlags,
                ImVec2(0.0f, 0.0f)))
        {
            return;
        }

        ImGui::TableSetupScrollFreeze(0, 1);

        ImGui::TableSetupColumn(
            "Time",
            ImGuiTableColumnFlags_WidthFixed,
            85.0f);

        ImGui::TableSetupColumn(
            "Level",
            ImGuiTableColumnFlags_WidthFixed,
            80.0f);

        ImGui::TableSetupColumn(
            "Source",
            ImGuiTableColumnFlags_WidthFixed,
            150.0f);

        ImGui::TableSetupColumn(
            "Message",
            ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableHeadersRow();

        for (const auto &message : snapshot)
        {
            if (!PassesFilter(message))
                continue;

            ImGui::PushID(
                static_cast<int>(message.id));

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);

            ImGui::TextUnformatted(
                message.timestamp.c_str());

            ImGui::TableSetColumnIndex(1);

            ImGui::PushStyleColor(
                ImGuiCol_Text,
                GetLevelColor(message.level));

            ImGui::TextUnformatted(
                GetLevelName(message.level));

            ImGui::PopStyleColor();

            ImGui::TableSetColumnIndex(2);

            ImGui::TextUnformatted(
                message.source.empty()
                    ? "-"
                    : message.source.c_str());

            ImGui::TableSetColumnIndex(3);

            ImGui::TextWrapped(
                "%s",
                message.message.c_str());

            if (ImGui::BeginPopupContextItem(
                    "ConsoleMessageContext"))
            {
                if (ImGui::MenuItem(
                        "Copy Message"))
                {
                    ImGui::SetClipboardText(
                        message.message.c_str());
                }

                if (ImGui::MenuItem(
                        "Copy Full Entry"))
                {
                    std::ostringstream output;

                    output
                        << "["
                        << message.timestamp
                        << "] ["
                        << GetLevelName(
                               message.level)
                        << "] ";

                    if (!message.source.empty())
                    {
                        output
                            << "["
                            << message.source
                            << "] ";
                    }

                    output << message.message;

                    ImGui::SetClipboardText(
                        output.str().c_str());
                }

                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        if (m_AutoScroll &&
            m_ScrollToBottom.exchange(
                false,
                std::memory_order_relaxed))
        {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndTable();
    }

    bool ConsolePanel::PassesFilter(
        const ConsoleMessage &message) const
    {
        switch (message.level)
        {
        case ConsoleLogLevel::Trace:
            if (!m_ShowTrace)
                return false;
            break;

        case ConsoleLogLevel::Info:
            if (!m_ShowInfo)
                return false;
            break;

        case ConsoleLogLevel::Warning:
            if (!m_ShowWarnings)
                return false;
            break;

        case ConsoleLogLevel::Error:
            if (!m_ShowErrors)
                return false;
            break;

        case ConsoleLogLevel::Critical:
            if (!m_ShowCritical)
                return false;
            break;
        }

        const std::string search =
            m_SearchBuffer;

        return ContainsInsensitive(
                   message.message,
                   search) ||
               ContainsInsensitive(
                   message.source,
                   search) ||
               ContainsInsensitive(
                   GetLevelName(message.level),
                   search);
    }

    const char *ConsolePanel::GetLevelName(
        ConsoleLogLevel level)
    {
        switch (level)
        {
        case ConsoleLogLevel::Trace:
            return "Trace";

        case ConsoleLogLevel::Info:
            return "Info";

        case ConsoleLogLevel::Warning:
            return "Warning";

        case ConsoleLogLevel::Error:
            return "Error";

        case ConsoleLogLevel::Critical:
            return "Critical";
        }

        return "Unknown";
    }

    ImVec4 ConsolePanel::GetLevelColor(
        ConsoleLogLevel level)
    {
        switch (level)
        {
        case ConsoleLogLevel::Trace:
            return ImVec4(
                0.65f,
                0.65f,
                0.65f,
                1.0f);

        case ConsoleLogLevel::Info:
            return ImVec4(
                0.80f,
                0.85f,
                1.00f,
                1.0f);

        case ConsoleLogLevel::Warning:
            return ImVec4(
                1.00f,
                0.75f,
                0.20f,
                1.0f);

        case ConsoleLogLevel::Error:
            return ImVec4(
                1.00f,
                0.30f,
                0.30f,
                1.0f);

        case ConsoleLogLevel::Critical:
            return ImVec4(
                1.00f,
                0.15f,
                0.75f,
                1.0f);
        }

        return ImVec4(
            1.0f,
            1.0f,
            1.0f,
            1.0f);
    }

    std::string ConsolePanel::GetCurrentTimestamp()
    {
        const auto now =
            std::chrono::system_clock::now();

        const std::time_t time =
            std::chrono::system_clock::to_time_t(
                now);

        std::tm localTime{};

#ifdef PLATFORM_WIN64
        localtime_s(
            &localTime,
            &time);
#else
        localtime_r(
            &time,
            &localTime);
#endif

        std::ostringstream output;

        output
            << std::put_time(
                   &localTime,
                   "%H:%M:%S");

        return output.str();
    }
}