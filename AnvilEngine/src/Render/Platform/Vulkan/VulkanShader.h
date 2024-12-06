#pragma once
#include "Render/Shader.h"
#include "Util/UMacros.h"
#include "Render/Context.h"
#include "VulkanContext.h"

#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>

namespace anv
{

	class VulkanShader
		: public Shader
	{
	public:
		VulkanShader(std::string _path, _shared<Context> _ctx);
		~VulkanShader();
	private:
		void set_name();
		void load(std::string& _file);
		// Shaders consist of both vert and frag.
		void pre_process();
		void compile_to_spv();
		void create_module();

	private:
		std::string              m_Name;
		_vec<std::string>        m_SrcCode;
		std::pair<std::string, _vec<uint32_t>>
		                         m_VertCode;
		std::pair<std::string, _vec<uint32_t>>
		                         m_FragCode;
		VkShaderModule           m_VModule;
		VkShaderModule           m_FModule;
		VulkanContext*           m_VkContext;
	};
}
