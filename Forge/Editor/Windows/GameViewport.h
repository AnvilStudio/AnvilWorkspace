#pragma once
#include <Anvil.h>

class GameViewport
{
public:
    GameViewport();
    void Draw();

private:
    anv::Ref<anv::RenderTarget> m_GameViewportTarget;

    uint32_t m_LastTargetWidth = 0;
    uint32_t m_LastTargetHeight = 0;
};