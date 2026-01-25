#include "VulkanShader.h"
#include "VulkanUtil.h"
#include <Core/App.h>
#include <Util/FileSys/FileSystem.h>
#include <shaderc/shaderc.hpp>


// TODO: Add a settings file to cache the already compiled shader src code path.
// That way, next time the app opens, the shader wont have to re-compile

namespace anv
{
	VulkanShader::VulkanShader(std::string _path, _shared<Context> _ctx)
		: Shader(_path), m_Name(_path)
	{
		ANV_PROFILE_SCOPE()
		App::GetInstance()->GetFS().CreateKeyDir("ShaderCache", 
			"Assets/com.anvstu.engine/Cache/ShaderCache/");

		m_VkContext = _ctx->GetAs<VulkanContext>();
		load(_path);
		pre_process();
		compile_to_spv();
		create_module();
	}

	VulkanShader::~VulkanShader()
	{
		ANV_PROFILE_SCOPE()

		vkDestroyShaderModule(m_VkContext->GetDevice(), m_VModule, nullptr);
		vkDestroyShaderModule(m_VkContext->GetDevice(), m_FModule, nullptr);
	}

	_vec<VkPipelineShaderStageCreateInfo> VulkanShader::GetShaderStages()
	{
		VkPipelineShaderStageCreateInfo vstageInfo{};
		vstageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vstageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vstageInfo.module = m_VModule;
		vstageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fstageInfo{};
		fstageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fstageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fstageInfo.module = m_FModule;
		fstageInfo.pName = "main";

		return { vstageInfo, fstageInfo };
	}

	void VulkanShader::set_name()
	{

		// Get the parent path to store the SPIR-V
		std::filesystem::path path(m_Name);
		m_FilePath = path.parent_path().string(); 

		// Set name
		size_t lastSlash = m_Name.find_last_of("/\\");
		std::string fileName = m_Name.substr(lastSlash + 1);
		m_Name = fileName;

	}

	void VulkanShader::load(std::string& _file)
	{
		ANV_LOG_DEBUG("Loading Shader: %s", m_Name.c_str())
		auto& fs = App::GetInstance()->GetFS();
		// No need to call "close" as it happens automatically
		auto shader_file = fs.CreateFile(_file);
		m_SrcCode = shader_file->Read();
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
			fresult = compiler.CompileGlslToSpv(m_FragCode.first, shaderc_fragment_shader, m_Name.c_str(), options);
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

		save_files();
	}
	
	void VulkanShader::save_files()
	{
		auto& fs = App::GetInstance()->GetFS();
		//fs.Save(
		//	fs.AtKeyDir("ShaderCache") + m_Name + ".shade",
		//	Serializer::Mode::SER_MODE_BINARY,
		//	[&](Serializer& ser)
		//	{

		//		auto vert = m_VertCode.second;
		//		auto frag = m_FragCode.second;

		//		ser.Object("ShaderCache", [&]
		//			{
		//				ser.Field("Magic", kShaderCacheMagic);
		//				ser.Field("Version", kShaderCacheVersion);
		//				ser.Field("Name", m_Name);
		//				ser.Vector("VertSpv", vert);
		//				ser.Vector("FragSpv", frag);
		//			});
		//	}
		//);
		std::string path;
		path = fs.AtKeyDir("ShaderCache") + m_Name.substr(m_Name.find_last_of("\\/")) + ".shade";
		auto file = fs.CreateFile(path);
		Serializer ser(path, Serializer::Mode::SER_MODE_TOML, Serializer::Direction::Write);
		ser.Object(m_Name, [&]()
			{
				ser.Field("Magic", kShaderCacheMagic);
				ser.Field("Version", kShaderCacheVersion);
				ser.Vector("Vertex", m_VertCode.second);
				ser.Vector("Fragment", m_FragCode.second);
			});
	}


	void VulkanShader::create_module()
	{
		VkShaderModuleCreateInfo vcreateInfo{};
		vcreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vcreateInfo.codeSize = m_VertCode.second.size() * sizeof(uint32_t); // multiply by u32t for alignment
		vcreateInfo.pCode = m_VertCode.second.data();

		ANV_VK_CHECK_RESULT(vkCreateShaderModule(m_VkContext->GetDevice(), &vcreateInfo, nullptr, &m_VModule), 
			std::string("Failed to create vert shader module for: ") + m_Name);

		VkShaderModuleCreateInfo fcreateInfo = {};
		fcreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fcreateInfo.codeSize = m_FragCode.second.size() * sizeof(uint32_t); // multiply by u32t for alignment
		fcreateInfo.pCode = m_FragCode.second.data();

		ANV_VK_CHECK_RESULT(vkCreateShaderModule(m_VkContext->GetDevice(), &fcreateInfo, nullptr, &m_FModule), 
			std::string("Failed to create frag shader module for: ") + m_Name);
	}
}