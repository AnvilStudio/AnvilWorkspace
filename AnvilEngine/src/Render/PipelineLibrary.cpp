#include "PipelineLibrary.h"

namespace anv
{
    void PipelineLibrary::Register(
        const std::string& name,
        PipelineBuilderFn builder)
    {
        m_Builders[name] = builder;
    }

    Ref<GraphicsPipeline> PipelineLibrary::Get(
        const std::string& name,
        Ref<RenderTarget> target)
    {
        PipelineKey key = MakeKey(name, target);

        auto found = m_Pipelines.find(key);
        if (found != m_Pipelines.end())
            return found->second;

        auto builder = m_Builders.find(name);
        ANV_ASSERT(
            builder != m_Builders.end(),
            "Pipeline is not registered!"
        );

        Ref<GraphicsPipeline> pipeline =
            builder->second(target->GetRenderPass());

        m_Pipelines[key] = pipeline;
        return pipeline;
    }

    void PipelineLibrary::Clear()
    {
        m_Pipelines.clear();
    }

    PipelineKey PipelineLibrary::MakeKey(
        const std::string& name,
        Ref<RenderTarget> target)
    {
        const auto& sig =
            target->GetRenderPass()->GetSignature();

        PipelineKey key{};
        key.Name = name;
        key.ColorFormat = sig.ColorFormat;
        key.DepthFormat = sig.DepthFormat;
        key.Samples = sig.Samples;
        key.DepthEnabled = sig.HasDepth;
        key.BlendingEnabled = true;

        return key;
    }
}