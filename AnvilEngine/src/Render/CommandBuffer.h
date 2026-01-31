#pragma once
#include "../Core/Reference.h"
#include "Context.h"

namespace anv
{
    class CommandBuffer : public RefCounter
    {
    public:
        static Ref<CommandBuffer> Create(_shared<Context> _ctx);

        CommandBuffer(_shared<Context> _ctx);
        virtual ~CommandBuffer() = default;
        virtual void Begin() = 0;
        virtual void End() = 0;
        virtual void Submit() = 0;
        virtual void Reset() = 0;

    protected:
        // Command buffer state
        bool m_IsRecording = false;
        bool m_IsSubmitted = false;

        _shared<Context> m_Context;
    };
} // namespace anv
