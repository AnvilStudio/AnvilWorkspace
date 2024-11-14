#include "Context.h"
#include "Render/Platform/Vulkan/VulkanContext.h"
#include "Core/Window.h"

namespace anv {
    _unique(Context) Context::Create(Window* _win)
    {
        // TODO: user API switch
        return std::make_unique<VulkanContext>(_win);
    }
}