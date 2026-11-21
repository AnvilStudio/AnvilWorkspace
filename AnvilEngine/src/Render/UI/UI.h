#pragma once
#include <SlateUI/slate.h>

namespace anv
{
    class UIEngine
    {
    public:
        UIEngine();
        ~UIEngine();

    private:
        slate::Context m_Context;
    };
}