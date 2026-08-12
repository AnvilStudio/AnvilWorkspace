#include "Shader.h"
#include "RenderAPI.h"
#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
#include "Platform/Vulkan/VulkanShader.h"
#elif defined(PLATFORM_APPLE)
// #include "Platform/Metal/MtlShader.h"
#endif
namespace anv
{

	Ref<Shader> anv::Shader::Create(const std::string& _shaderPath, _shared<Context> _ctx)
	{
		// shader has not been loaded yet
		GraphicsAPI api = RenderAPI::GetAPI();
		switch (api)
		{
		case GraphicsAPI::VK:
		{
			#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
			auto shader = Ref<VulkanShader>::Create(_shaderPath, _ctx);
			shader->GenMetaFile();
			return shader;
			#else 
			ANV_LOG_FATAL("Graphics API missmatch!");
			return nullptr;
			#endif
			break;
		}
		case GraphicsAPI::MTL:
		{
			#if defined(PLATFORM_APPLE) && !defined(PLATFORM_APPLE_VK)
			// TODO: IMPL
			//auto shader = Ref<MetalShader>::Create(_shaderPath, _ctx);
			//shader->GenMetaFile();
			//return shader;
			return nullptr;
			#else
			ANV_LOG_FATAL("Graphics API missmatch!")
			return nullptr;
			#endif
			break;
		}
		default:
			ANV_LOG_FATAL("GraphicsAPI missmatch!")
			return nullptr;
			break;
		}
	}

	Ref<Shader> Shader::Create(Deserialized& _dser)
	{
		GraphicsAPI api = RenderAPI::GetAPI();
		if (api == GraphicsAPI::VK)
		{
			#if defined(PLATFORM_WIN64) || defined(PLATFORM_APPLE_VK)
			return Ref<VulkanShader>::Create(_dser);
			#else
			ANV_LOG_FATAL("Vulkan shader backend is unavailable on this platform!")
			return nullptr;
			#endif
		}
		else
		{
			ANV_LOG_FATAL("GraphicsAPI not supported!")
			return nullptr;
		}
	}

	Shader::Shader(std::string& _name)
		: Asset(_name)
	{
		m_Type = "Shader";
	}

	Shader::Shader(Deserialized& _dser)
		: Asset(_dser)
	{
		ANV_LOG_INFO("Deserializing shader: %s", _dser.name.c_str());
	}
}