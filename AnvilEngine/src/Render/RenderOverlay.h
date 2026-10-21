#pragma once
#include "RenderCommandEncoder.h"
#include <cstdint>
#include <functional>
#include <vector>
namespace anv {
enum class RenderOverlayStage { BeforeScene, AfterScene };
// Vulkan callbacks execute later on its render thread. Capture immutable frame
// data by value and retain resource handles for the callback's lifetime.
struct RenderOverlay {
    RenderOverlayStage stage = RenderOverlayStage::AfterScene;
    int32_t priority = 0;
    std::function<void(RenderCommandEncoder&)> callback;
};
using RenderOverlays = std::vector<RenderOverlay>;
}
