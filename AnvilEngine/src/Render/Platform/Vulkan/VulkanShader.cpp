#include "VulkanShader.h"
#include "VulkanUtil.h"
#include <shaderc/shaderc.hpp>

namespace anv
{
	VulkanShader::VulkanShader(std::string _path, _shared<Context> _ctx)
		: m_Name(_path)
	{
		ANV_PROFILE_SCOPE()
		m_VkContext = _ctx->GetNativeContextAs<VulkanContext>();
		load(_path);
		pre_process();
		compile_to_spv();
		create_module();
	}

	VulkanShader::~VulkanShader()
	{
		vkDestroyShaderModule(m_VkContext->GetDevice(), m_VModule, nullptr);
		vkDestroyShaderModule(m_VkContext->GetDevice(), m_FModule, nullptr);
	}

	void VulkanShader::set_name()
	{
		size_t lastSlash = m_Name.find_last_of("/\\");
		std::string fileName = m_Name.substr(lastSlash + 1);
		m_Name = fileName;
	}

	void VulkanShader::load(std::string& _file)
	{
		ANV_LOG_DEBUG("Loading Shader: %s", m_Name.c_str())
		std::ifstream shader_file(_file);

		if (!shader_file.is_open())
		{
			ANV_LOG_ERROR("Failed to open shader file: %s", _file)
			return;
		}
		
		m_SrcCode = {};
		std::string line{};
		while (std::getline(shader_file, line))
		{
			m_SrcCode.push_back(line);
		}

		shader_file.close();
	}

	void VulkanShader::pre_process()
	{
		ANV_LOG_DEBUG("Pre-Processing Shader: % s", m_Name.c_str())

		ShaderType ty = ShaderType::NONE;
		std::unordered_map<ShaderType, std::string> shaders = {};
		std::stringstream shade_s{};

		for (auto& str : m_SrcCode)
		{
			/// separate vert/frag shaders
			if (str.find("#type") != std::string::npos)
			{
				if (ty != ShaderType::NONE) {
					shaders[ty] = shade_s.str();
					shade_s.str(""); // Clear the stream
					shade_s.clear();
				}

				if (str == "#type vert")
				{
					ty = ShaderType::VERT;
				}
				if (str == "#type frag")
				{
					ty = ShaderType::FRAG;
				}
			}
			else
			{
				shade_s << str + '\n';
			}
		}

		if (ty != ShaderType::NONE) {
			shaders[ty] = shade_s.str();
		}

		// set the src code of both shaders
		m_VertCode.first = shaders.at(ShaderType::VERT);
		m_FragCode.first = shaders.at(ShaderType::FRAG);
	}

	void VulkanShader::compile_to_spv()
	{
		ANV_PROFILE_SCOPE()
		ANV_LOG_INFO("Compiling Shader: %s", m_Name.c_str())
		shaderc::Compiler compiler;
		shaderc::CompileOptions options;

		options.SetOptimizationLevel(shaderc_optimization_level_performance);

		// compile and profile
		shaderc::SpvCompilationResult vresult = {};
		{
			ANV_PROFILE_SCOPE_NAME("\tVertex Shader")
			vresult = compiler.CompileGlslToSpv(m_VertCode.first, shaderc_vertex_shader, m_Name.c_str(), options);
		}
		shaderc::SpvCompilationResult fresult = {};
		{
			ANV_PROFILE_SCOPE_NAME("\tFragment Shader")
			fresult = compiler.CompileGlslToSpv(m_FragCode.first, shaderc_vertex_shader, m_Name.c_str(), options);
		}

		// Check comp status
		if (vresult.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			ANV_LOG_ERROR("Failed to compile vertex shader: %s\nMessage: %s", m_Name.c_str(), vresult.GetErrorMessage().c_str())
		}
		else
		{
			ANV_LOG_INFO("Compiled Vertex Shader: %s", m_Name.c_str())
			m_VertCode.second = { vresult.begin(), vresult.end() };
		}

		if (fresult.GetCompilationStatus() != shaderc_compilation_status_success)
		{
			ANV_LOG_ERROR("Failed to compile fragment shader: %s\nMessage: %s", m_Name.c_str(), fresult.GetErrorMessage().c_str())
		}
		else
		{
			ANV_LOG_INFO("Compiled Fragment Shader: %s", m_Name.c_str())
			m_FragCode.second = { fresult.begin(), fresult.end() };
		}
	}

	void VulkanShader::create_module()
	{
		VkShaderModuleCreateInfo vcreateInfo{};
		vcreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vcreateInfo.codeSize = m_VertCode.second.size() * sizeof(uint32_t); // multiply by u32t for alignment
		vcreateInfo.pCode = m_VertCode.second.data();

		ANV_VK_CHECK_RESULT(vkCreateShaderModule(m_VkContext->GetDevice(), &vcreateInfo, nullptr, &m_VModule), std::string("Failed to create vert shader module for: ") + m_Name);

		VkShaderModuleCreateInfo fcreateInfo = {};
		fcreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fcreateInfo.codeSize = m_FragCode.second.size() * sizeof(uint32_t); // multiply by u32t for alignment
		fcreateInfo.pCode = m_FragCode.second.data();

		ANV_VK_CHECK_RESULT(vkCreateShaderModule(m_VkContext->GetDevice(), &fcreateInfo, nullptr, &m_FModule), std::string("Failed to create frag shader module for: ") + m_Name);
	}
}