#pragma once

namespace anv
{
    class CommandBuffer
    {
    public:
        CommandBuffer() = default;
        virtual ~CommandBuffer() = default;
        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void Submit() = 0;
        virtual void Reset() = 0;

    private:
        // Command buffer state
        bool m_IsRecording = false;
        bool m_IsSubmitted = false;
    };
} // namespace anv
