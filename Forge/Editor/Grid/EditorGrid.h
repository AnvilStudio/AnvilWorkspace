#pragma once
#include <Anvil.h>
#include <filesystem>

namespace forge
{
    class EditorGrid
    {
    public:
        struct Settings
        {
            bool visible = true;
            float cellSize = 1.0f;
            float majorInterval = 5.0f;
            float lineWidth = 1.0f;
        };

        void Initialize(anv::Ref<anv::RenderTarget> target);
        void Submit(anv::RenderOverlays& overlays, const glm::mat4& viewProjection) const;

        Settings& GetSettings() { return m_Settings; }

    private:
        Settings m_Settings;
        anv::Ref<anv::Shader> m_Shader;
        anv::Ref<anv::GraphicsPipeline> m_Pipeline;
    };
}
