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
        for (auto& [id, asset] : m_AssetReg)
            asset->Save();
    }

    std::string AssetManager::NormalizeResource(const std::filesystem::path& resource)
    {
        std::error_code error;
        auto normalized = std::filesystem::weakly_canonical(resource, error);

        if (error)
        {
            error.clear();
            normalized = std::filesystem::absolute(resource, error);
        }

        if (error)
            normalized = resource.lexically_normal();

        return normalized.generic_string();
    }

    void AssetManager::Register(Ref<Asset> asset)
    {
        if (!asset)
            return;

        const uuid::AssetUUID id = asset->GetAssetID();
        asset->GenMetaFile();
        m_AssetReg.insert_or_assign(id, asset);

        if (!asset->GetResourcePath().empty())
        {
            m_ByResource.insert_or_assign(
                NormalizeResource(asset->GetResourcePath()),
                id);
        }
    }

    Ref<Asset> AssetManager::Get(const uuid::AssetUUID& id) const
    {
        auto it = m_AssetReg.find(id);
        return it == m_AssetReg.end() ? nullptr : it->second;
    }

    Ref<Asset> AssetManager::GetByResource(const std::filesystem::path& resource) const
    {
        auto resourceIt = m_ByResource.find(NormalizeResource(resource));
        return resourceIt == m_ByResource.end() ? nullptr : Get(resourceIt->second);
    }

    Ref<Texture> AssetManager::CreateTexture(const std::filesystem::path& resource)
    {
        auto texture = Texture::Create(resource);
        Register(texture.As<Asset>());
        return texture;
    }

    Ref<Texture> AssetManager::CreateTexture(Deserialized& deserialized)
    {
        auto texture = Texture::Create(deserialized);
        Register(texture.As<Asset>());
        return texture;
    }

    Ref<Texture> AssetManager::GetOrCreateTexture(const std::filesystem::path& resource)
    {
        if (auto existing = GetByResourceAs<Texture>(resource))
            return existing;

        return CreateTexture(resource);
    }

    void AssetManager::create(Deserialized& deserialized)
    {
        if (deserialized.type == "Shader")
        {
            auto shader = Shader::Create(deserialized);
            Register(shader.As<Asset>());
        }
        else if (deserialized.type == "Texture")
        {
            CreateTexture(deserialized);
        }
    }

    Ref<GraphicsPipeline> AssetManager::CreateGraphicsPipeline(
        _shared<Context> context,
        std::string displayName)
    {
        auto pipeline = GraphicsPipeline::create_pipeline_asset(context, displayName);
        Register(pipeline.As<Asset>());
        return pipeline;
    }

    Ref<Shader> AssetManager::CreateShader(
        const std::string& shaderPath,
        _shared<Context> context)
    {
        if (auto existing = GetByResourceAs<Shader>(shaderPath))
            return existing;

        auto shader = Shader::Create(shaderPath, context);
        Register(shader.As<Asset>());
        return shader;
    }

    void AssetManager::resolve_assets()
    {
        auto assets = m_Fs.GetKeyVal("AssetMeta");

        m_Fs.ForEach(assets, [&](Ref<File> file)
        {
            Serializer serializer(
                file,
                Serializer::Mode::SER_MODE_TOML,
                Serializer::Direction::Read);

            Deserialized deserialized;
            deserialized.ser = serializer;

            serializer.ObjectStrict("Asset", [&]
            {
                serializer.FieldStrict("Name", deserialized.name);
                serializer.FieldStrict("Resource", deserialized.resource);
                serializer.FieldStrict("UUID", deserialized.uuid);
                serializer.ObjectStrict("Spec", [&]
                {
                    serializer.FieldStrict("Type", deserialized.type);
                });
            });

            create(deserialized);
        });
    }
}
