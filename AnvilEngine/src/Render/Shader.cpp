#include "Shader.h"
#include "RenderAPI.h"

#include "Platform/Vulkan/VulkanShader.h"

namespace anv
{
	Ref<Shader> anv::Shader::Create(const std::string& _shaderPath, _shared<Context> _ctx)
	{
		// Shader already loaded
		if (s_ShaderCache.find(_shaderPath) != s_ShaderCache.end())
		{
			return s_ShaderCache[_shaderPath];
		}

		// shader has not been loaded yet
		switch (RenderAPI::GetAPI())
		{
		case GraphicsAPI::VK:
			return Ref<VulkanShader>::Create(_shaderPath, _ctx);
			break;
		case GraphicsAPI::OGL:
			ANV_LOG_ERROR("OpenGL Not Supported!")
			return Ref<VulkanShader>::Create(_shaderPath, _ctx);
			break;
		case GraphicsAPI::DX:
			ANV_LOG_ERROR("DirectX Not Supported!")
			return Ref<VulkanShader>::Create(_shaderPath, _ctx);
			break;
		case GraphicsAPI::MTL:
			ANV_LOG_ERROR("Metal Not Supported!")
			return Ref<VulkanShader>::Create(_shaderPath, _ctx);
			break;
		default:
			break;
		}
	}
}