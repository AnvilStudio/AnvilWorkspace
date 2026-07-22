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
        for (auto &[id, asset] : m_AssetReg)
        {
            if (!asset)
                continue;

            if (!asset->HasMetaPath())
            {
                ANV_LOG_ERROR(
                    "Skipping save for asset without metadata path: %s | UUID: %s",
                    asset->GetName().c_str(),
                    id.uuid.c_str());

                continue;
            }

            asset->Save();
        }
    }

    std::string AssetManager::NormalizeResource(
        const std::filesystem::path &resource) const
    {
        if (resource.empty())
            return {};

        std::filesystem::path resolved = resource;
        const std::string raw = resource.generic_string();

        if (!raw.empty() && raw.front() == '@')
        {
            resolved = m_Fs.ResolveKey(raw);

            if (resolved.empty())
            {
                ANV_LOG_ERROR(
                    "Could not resolve virtual asset path: %s",
                    raw.c_str());

                return {};
            }
        }
        else if (resolved.is_relative())
        {
            auto first = resolved.begin();

            if (first != resolved.end() && *first == "Assets")
            {
                resolved =
                    m_Fs.GetKeyVal("Assets").parent_path() /
                    resolved;
            }
            else
            {
                resolved =
                    m_Fs.GetKeyVal("Assets") /
                    resolved;
            }
        }

        std::error_code error;

        auto normalized =
            std::filesystem::weakly_canonical(
                resolved,
                error);

        if (error)
        {
            error.clear();

            normalized =
                std::filesystem::absolute(
                    resolved,
                    error);
        }

        if (error)
            normalized = resolved.lexically_normal();

        return normalized.generic_string();
    }

    void AssetManager::Register(
        Ref<Asset> asset,
        bool generateMetadata)
    {
        if (!asset)
        {
            ANV_LOG_ERROR("AssetManager::Register received null asset");
            return;
        }

        const uuid::AssetUUID id = asset->GetAssetID();
        const std::filesystem::path resource =
            asset->GetResourcePath();

        ANV_LOG_INFO(
            "Register manager=%p asset='%s' UUID='%s' resource='%s'",
            static_cast<void *>(this),
            asset->GetName().c_str(),
            id.uuid.c_str(),
            resource.string().c_str());

        if (resource.empty())
        {
            ANV_LOG_WARN(
                "Asset '%s' has no resource path; it cannot be indexed by resource",
                asset->GetName().c_str());
        }

        // Register by UUID.
        m_AssetReg.insert_or_assign(id, asset);

        // Register by normalized resource.
        if (!resource.empty())
        {
            const std::string key = NormalizeResource(resource);

            if (key.empty())
            {
                ANV_LOG_ERROR(
                    "Failed to normalize resource path for asset '%s'",
                    asset->GetName().c_str());
            }
            else
            {
                m_ByResource.insert_or_assign(key, id);

                ANV_LOG_INFO(
                    "Indexed resource manager=%p key='%s' UUID='%s' "
                    "resourceCount=%zu",
                    static_cast<void *>(this),
                    key.c_str(),
                    id.uuid.c_str(),
                    m_ByResource.size());
            }
        }

        if (generateMetadata)
            asset->GenMetaFile();
    }

    Ref<Asset> AssetManager::Get(const uuid::AssetUUID &id) const
    {
        auto it = m_AssetReg.find(id);
        return it == m_AssetReg.end() ? nullptr : it->second;
    }

    Ref<Asset> AssetManager::GetByResource(const std::filesystem::path &resource) const
    {
        auto resourceIt = m_ByResource.find(NormalizeResource(resource));
        return resourceIt == m_ByResource.end() ? nullptr : Get(resourceIt->second);
    }

    Ref<Texture> AssetManager::CreateTexture(
        const std::filesystem::path &resource)
    {
        const std::string key = NormalizeResource(resource);

        if (auto existing = GetByResourceAs<Texture>(resource))
        {
            ANV_LOG_INFO(
                "CreateTexture reused existing texture UUID='%s'",
                existing->GetAssetID().uuid.c_str());

            return existing;
        }

        Ref<Texture> texture = Texture::Create(resource);

        if (!texture)
        {
            ANV_LOG_ERROR(
                "Texture::Create failed for '%s'",
                resource.string().c_str());

            return nullptr;
        }

        Register(texture.As<Asset>(), true);

        auto registeredIt = m_ByResource.find(key);

        if (registeredIt == m_ByResource.end())
        {
            ANV_LOG_ERROR(
                "Texture was created but not indexed: manager=%p key='%s'",
                static_cast<void *>(this),
                key.c_str());
        }
        else
        {
            ANV_LOG_INFO(
                "Texture creation registered key='%s' UUID='%s'",
                key.c_str(),
                registeredIt->second.uuid.c_str());
        }

        return texture;
    }

    Ref<Texture> AssetManager::CreateTexture(Deserialized &deserialized)
    {
        Ref<Texture> texture = Texture::Create(deserialized);

        if (!texture)
        {
            ANV_LOG_ERROR(
                "Failed to restore texture '%s' with UUID '%s'",
                deserialized.resource.c_str(),
                deserialized.uuid.c_str());

            return nullptr;
        }

        Register(texture.As<Asset>(), false);

        ANV_LOG_INFO(
            "Restored texture '%s' with UUID '%s'",
            deserialized.resource.c_str(),
            texture->GetAssetID().uuid.c_str());

        return texture;
    }

    Ref<Texture> AssetManager::GetOrCreateTexture(
        const std::filesystem::path &resource)
    {
        const std::string key = NormalizeResource(resource);

        ANV_LOG_INFO(
            "GetOrCreateTexture manager=%p input='%s' normalized='%s' "
            "resourceCount=%zu assetCount=%zu",
            static_cast<void *>(this),
            resource.string().c_str(),
            key.c_str(),
            m_ByResource.size(),
            m_AssetReg.size());

        auto resourceIt = m_ByResource.find(key);

        if (resourceIt != m_ByResource.end())
        {
            Ref<Texture> existing =
                GetAs<Texture>(resourceIt->second);

            if (existing)
            {
                ANV_LOG_INFO(
                    "Reusing texture UUID='%s'",
                    resourceIt->second.uuid.c_str());

                return existing;
            }

            ANV_LOG_ERROR(
                "Resource index contains UUID='%s', but UUID registry does not "
                "contain a Texture",
                resourceIt->second.uuid.c_str());

            // Remove a broken index entry.
            m_ByResource.erase(resourceIt);
        }

        ANV_LOG_WARN(
            "No texture registered for normalized path: %s",
            key.c_str());

        return CreateTexture(resource);
    }

    void AssetManager::create(Deserialized &deserialized)
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
        const std::string &shaderPath,
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
            deserialized.metaPath = file->Path();

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

            create(deserialized); });
    }
}
