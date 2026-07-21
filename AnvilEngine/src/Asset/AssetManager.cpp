#include "AssetManager.h"
#include "Util/FileSys/FileSystem.h"
#include "AssetTypes/Texture.h"
#include "Render/RenderAPI.h"
#include "Render/Platform/Vulkan/VulkanPipeline.h"
#include <Core/App.h>
#include <system_error>

namespace anv
{
	AssetManager::AssetManager()
		: m_Fs(App::GetInstance()->GetFS())
	{
		m_Fs.MountKey("AssetMeta", "Assets/com.anvstu.engine/AssetMeta/");
		resolve_assets();
	}

	AssetManager::~AssetManager()
	{
		// Ensure all assets are saved
		for (auto a : m_AssetReg)
		{
			a.second->Save();
		}
	}

	std::string AssetManager::NormalizeResource(const std::filesystem::path& _resource)
	{
		std::error_code error;
		auto normalized = std::filesystem::weakly_canonical(_resource, error);

		if (error)
		{
			error.clear();
			normalized = std::filesystem::absolute(_resource, error);
		}

		if (error)
			normalized = _resource.lexically_normal();

		return normalized.generic_string();
	}

	Ref<Asset> AssetManager::Get(const uuid::AssetUUID& _id) const
	{
		auto it = m_AssetReg.find(_id);
		if (it == m_AssetReg.end())
			return nullptr;
		return it->second;
	}

	Ref<Asset> AssetManager::GetByResource(const std::filesystem::path& _resource) const
	{
		auto resourceIt = m_ByResource.find(NormalizeResource(_resource));
		if (resourceIt == m_ByResource.end())
			return nullptr;

		return Get(resourceIt->second);
	}

	void AssetManager::create(Deserialized& _dser)
	{
		if (_dser.type == "Shader")
		{
			auto shader = Shader::Create(_dser);
			shader->GenMetaFile();
			m_AssetReg.try_emplace(shader->GetAssetID(), shader);

			if (!shader->GetResourcePath().empty())
				m_ByResource.insert_or_assign(
					NormalizeResource(shader->GetResourcePath()),
					shader->GetAssetID());
		}

		if (_dser.type == "Texture")
		{
			ANV_LOG_DEBUG("Texture");
			Create<Texture>(_dser);
		}
	}

	Ref<GraphicsPipeline> AssetManager::CreateGraphicsPipeline(_shared<Context> _ctx, std::string _dName)
	{
		auto p = GraphicsPipeline::create_pipeline_asset(_ctx, _dName);

		const uuid::AssetUUID id = p->GetAssetID();
		p->GenMetaFile();
		m_AssetReg.try_emplace(id, p.As<Asset>());
		return p;
	}

	Ref<Shader> AssetManager::CreateShader(const std::string& _shaderPath, _shared<Context> _ctx)
	{
		if (auto existing = GetByResourceAs<Shader>(_shaderPath))
			return existing;

		auto s = Shader::Create(_shaderPath, _ctx);

		const uuid::AssetUUID id = s->GetAssetID();
		s->GenMetaFile();
		m_AssetReg.try_emplace(id, s.As<Asset>());
		m_ByResource.insert_or_assign(NormalizeResource(s->GetResourcePath()), id);
		return s;
	}

	void AssetManager::resolve_assets()
	{
		auto assets = m_Fs.GetKeyVal("AssetMeta");

		m_Fs.ForEach(assets, [&](Ref<File> _file)
		{
				Serializer ser(_file, Serializer::Mode::SER_MODE_TOML, Serializer::Direction::Read);
				Deserialized dser;
				dser.ser = ser;

				ser.ObjectStrict("Asset", [&] {
					ser.FieldStrict("Name", dser.name);
					ser.FieldStrict("Resource", dser.resource);
					ser.FieldStrict("UUID", dser.uuid);
					ser.ObjectStrict("Spec", [&] {
					ser.FieldStrict("Type", dser.type);
						});
					});

				create(dser);
		});
	}
}