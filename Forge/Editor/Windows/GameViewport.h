#pragma once

#include <Anvil.h>

class GameViewport
{
public:
    GameViewport();
    void Draw();

private:
    void synchronize_scene_runtime();

private:
    anv::Ref<anv::RenderTarget> m_GameViewportTarget;

    bool m_OnPlayFocused = false;
    bool m_InputEnabled = false;
    bool m_WasPlaying = false;

    uint32_t m_LastTargetWidth = 0;
    uint32_t m_LastTargetHeight = 0;
};
