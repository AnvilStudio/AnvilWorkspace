#pragma once
#include "Render/RenderAPI.h"
#include "Render/Renderer.h"

namespace anv
{
	class VulkanRenderAPI
		: public RenderAPI
	{
	public:
		VulkanRenderAPI(RenderAPICreateInfo _info);
		~VulkanRenderAPI() override {};

	private:
		Render3DCreateInfo m_RenderInfo;
	};
}

