#pragma once
#include "Render/Shader.h"
#include "Util/UMacros.h"
#include "Render/Context.h"
#include "VulkanContext.h"

#include <shaderc/shaderc.h>
#include <vulkan/vulkan.h>

namespace anv
{
	static uint32_t kShaderCacheMagic = 0x414E5653; // 'ANVS'
	static uint32_t kShaderCacheVersion = 1;

	class VulkanShader
		: public Shader
	{
	public:
		VulkanShader(std::string _path, _shared<Context> _ctx);
		VulkanShader(Deserialized & _dser);
		~VulkanShader();

		_vec<VkPipelineShaderStageCreateInfo> GetShaderStages();

	private:
		void set_name();
		void load(std::string& _file);
		// Shaders consist of both vert and frag.
		void pre_process();
		void compile_to_spv();
		void write_spv_cache_file(); // save the compiled output
		void create_module();

	protected:
		void OnSave(Serializer& _ser) override;

	private:
		std::string                   m_Name;
		std::string				       m_FilePath;
		std::filesystem::path		m_Cache;
		_vec<std::string>        m_SrcCode;
		std::pair<std::string, _vec<uint32_t>>
		                                   m_VertCode;
		std::pair<std::string, _vec<uint32_t>>
		                                    m_FragCode;
		VkShaderModule           m_VModule;
		VkShaderModule           m_FModule;
		VulkanContext*             m_VkContext;
	};
}
