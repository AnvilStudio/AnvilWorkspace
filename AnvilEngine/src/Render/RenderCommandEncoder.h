#pragma once
#include "GraphicsPipeline.h"
#include <cstdint>
namespace anv {
enum class ShaderStage : uint32_t { Vertex = 1, Fragment = 2, VertexFragment = 3 };
// Valid only inside the active render-pass callback.
class RenderCommandEncoder {
public:
    virtual ~RenderCommandEncoder() = default;
    virtual void BindPipeline(Ref<GraphicsPipeline> pipeline) = 0;
    virtual void PushConstants(ShaderStage stages, const void* data, uint32_t size, uint32_t offset = 0) = 0;
    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
};
}
