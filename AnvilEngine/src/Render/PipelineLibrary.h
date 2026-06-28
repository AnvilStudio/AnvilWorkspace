#pragma once
#include "GraphicsPipeline.h"
#include "RenderTarget.h"
#include "RenderPass.h"

#include <unordered_map>
#include <functional>
#include <string>

namespace anv
{
    struct PipelineKey
    {
        std::string Name;

        Image2D::Format ColorFormat;
        Image2D::Format DepthFormat;

        uint32_t Samples = 1;
        bool DepthEnabled = false;
        bool BlendingEnabled = true;

        bool operator==(const PipelineKey& other) const
        {
            return Name == other.Name &&
                ColorFormat == other.ColorFormat &&
                DepthFormat == other.DepthFormat &&
                Samples == other.Samples &&
                DepthEnabled == other.DepthEnabled &&
                BlendingEnabled == other.BlendingEnabled;
        }
    };

    struct PipelineKeyHasher
    {
        size_t operator()(const PipelineKey& key) const
        {
            size_t h = std::hash<std::string>{}(key.Name);

            auto combine = [&](size_t v)
                {
                    h ^= v + 0x9e3779b9 + (h << 6) + (h >> 2);
                };

            combine(std::hash<int>{}((int)key.ColorFormat));
            combine(std::hash<int>{}((int)key.DepthFormat));
            combine(std::hash<uint32_t>{}(key.Samples));
            combine(std::hash<bool>{}(key.DepthEnabled));
            combine(std::hash<bool>{}(key.BlendingEnabled));

            return h;
        }
    };

    class PipelineLibrary
    {
    public:
        using PipelineBuilderFn =
            std::function<Ref<GraphicsPipeline>(Ref<RenderPass>)>;

        void Register(
            const std::string& name,
            PipelineBuilderFn builder
        );

        Ref<GraphicsPipeline> Get(
            const std::string& name,
            Ref<RenderTarget> target
        );

        void Clear();

    private:
        PipelineKey MakeKey(
            const std::string& name,
            Ref<RenderTarget> target
        );

    private:
        std::unordered_map<
            std::string,
            PipelineBuilderFn
        > m_Builders;

        std::unordered_map<
            PipelineKey,
            Ref<GraphicsPipeline>,
            PipelineKeyHasher
        > m_Pipelines;
    };
}