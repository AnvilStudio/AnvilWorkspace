#include "EditorGrid.h"
#include <algorithm>
#include <glm/gtc/matrix_inverse.hpp>

namespace forge
{
    // 112 bytes. Fits Vulkan's guaranteed 128-byte minimum.
    // Must match layout(push_constant) in EditorGrid.glsl.
    struct alignas(16) GridPush
    {
        glm::mat4 inverseViewProjection{1.0f};
        glm::vec4 settings{1.0f, 5.0f, 1.0f, 0.0f};
        glm::vec4 minorColor{0.149f, 0.149f, 0.149f, 1.f};
        glm::vec4 majorColor{0.769f, 0.769f, 0.769f, 1.f};
    };
    static_assert(sizeof(GridPush) == 112);

    void EditorGrid::Initialize(anv::Ref<anv::RenderTarget> target)
    {
#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
        if (m_Pipeline || !target)
            return;

        auto context = anv::App::GetInstance()->GetMainWindow()->GetContext();
        const auto shaderPath =
            anv::App::GetInstance()->GetFS().GetExecDir() /
            "ForgeResources" / "Shaders" / "EditorGrid.glsl";

        if (!std::filesystem::exists(shaderPath))
        {
            ANV_LOG_ERROR("Forge grid shader not found: %s", shaderPath.string().c_str());
            return;
        }

        m_Shader = anv::Shader::CreateInternal(shaderPath.string(), context);
        if (!m_Shader)
            return;

        m_Pipeline = anv::GraphicsPipeline::CreateInternal(context, "Forge Editor Grid");
        if (!m_Pipeline)
            return;

        m_Pipeline->SetShaderStages(m_Shader);
        m_Pipeline->SetVertexInputLayout(nullptr);
        m_Pipeline->SetRasterizationSettings(nullptr);
        m_Pipeline->SetColorBlendSettings(nullptr);
        m_Pipeline->SetPushConstants({static_cast<uint32_t>(anv::ShaderStage::Fragment), sizeof(GridPush), 0});
        m_Pipeline->SetRenderPass(target->GetRenderPass());
        m_Pipeline->Build();
#else
        (void)target; // Generic Metal pipelines are not implemented yet.
#endif
    }

    void EditorGrid::Submit(anv::RenderOverlays& overlays,
                            const glm::mat4& viewProjection) const
    {
        if (!m_Settings.visible || !m_Pipeline)
            return;

        GridPush data;
        data.inverseViewProjection = glm::inverse(viewProjection);
        data.settings = glm::vec4(
            std::max(m_Settings.cellSize, 0.001f),
            std::max(m_Settings.majorInterval, 1.0f),
            std::max(m_Settings.lineWidth, 0.1f),
            0.0f);

        data.minorColor = m_Settings.minorColor;
        data.majorColor = m_Settings.majorColor;

        // Values are copied because Vulkan records scene commands on a worker.
        auto pipeline = m_Pipeline;
        overlays.push_back({
            .stage = anv::RenderOverlayStage::BeforeScene,
            .priority = 100,
            .callback = [pipeline, data](anv::RenderCommandEncoder& cmd)
            {
                cmd.BindPipeline(pipeline);
                cmd.PushConstants(anv::ShaderStage::Fragment, &data, sizeof(data));
                cmd.Draw(3);
            }
        });
    }
}
