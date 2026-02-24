#include "Shader.h"
#include "RenderAPI.h"

#include "Platform/Vulkan/VulkanShader.h"

namespace anv
{

	Ref<Shader> anv::Shader::Create(const std::string& _shaderPath, _shared<Context> _ctx)
	{
		// shader has not been loaded yet
		GraphicsAPI api = RenderAPI::GetAPI();
		if (api == GraphicsAPI::VK)
		{
			auto shader = Ref<VulkanShader>::Create(_shaderPath, _ctx);
			shader->GenAssetFile();
			return shader;
		}
		else
		{
			ANV_LOG_FATAL("GraphicsAPI not supported!")
			return nullptr;
		}
	}

	Ref<Shader> Shader::Create(Deserialized& _dser)
	{
		GraphicsAPI api = RenderAPI::GetAPI();
		if (api == GraphicsAPI::VK)
		{
			return Ref<VulkanShader>::Create(_dser);
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
	}

	Shader::Shader(Deserialized& _dser)
		: Asset(_dser)
	{
		ANV_LOG_INFO("Deserializing shader: %s", _dser.name.c_str());
	}
}