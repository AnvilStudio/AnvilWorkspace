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

    // void AssetManager::Register(Ref<Asset> asset)
    // {
    //     if (!asset)
    //         return;

    //     const uuid::AssetUUID id = asset->GetAssetID();
    //     const std::filesystem::path resource =
    //         asset->GetResourcePath();

    //     if (!resource.empty())
    //     {
    //         const std::string normalized =
    //             NormalizeResource(resource);

    //         auto resourceIt = m_ByResource.find(normalized);

    //         if (resourceIt != m_ByResource.end())
    //         {
    //             const uuid::AssetUUID &existingID =
    //                 resourceIt->second;

    //             if (existingID != id)
    //             {
    //                 ANV_LOG_ERROR(
    //                     "Duplicate asset resource rejected: %s | "
    //                     "Existing UUID: %s | New UUID: %s",
    //                     normalized.c_str(),
    //                     existingID.uuid.c_str(),
    //                     id.uuid.c_str());

    //                 return;
    //             }
    //         }
    //     }

    //     auto idIt = m_AssetReg.find(id);

    //     if (idIt != m_AssetReg.end())
    //     {
    //         ANV_LOG_WARN(
    //             "Asset UUID already registered: %s",
    //             id.uuid.c_str());

    //         return;
    //     }

    //     asset->GenMetaFile();

    //     m_AssetReg.emplace(id, asset);

    //     if (!resource.empty())
    //     {
    //         m_ByResource.insert_or_assign(
    //             NormalizeResource(resource),
    //             id);
    //     }
    // }

    void AssetManager::Register(
        Ref<Asset> asset,
        bool generateMetadata)
    {
        if (!asset)
            return;

        const uuid::AssetUUID id = asset->GetAssetID();
        const std::filesystem::path resource =
            asset->GetResourcePath();

        if (!resource.empty())
        {
            const std::string normalized =
                NormalizeResource(resource);

            auto resourceIt = m_ByResource.find(normalized);

            if (resourceIt != m_ByResource.end())
            {
                if (resourceIt->second != id)
                {
                    ANV_LOG_ERROR(
                        "Duplicate asset resource rejected: %s | "
                        "Existing UUID: %s | New UUID: %s",
                        normalized.c_str(),
                        resourceIt->second.uuid.c_str(),
                        id.uuid.c_str());

                    return;
                }
            }
        }

        m_AssetReg.insert_or_assign(id, asset);

        if (!resource.empty())
        {
            const std::string key =
                NormalizeResource(asset->GetResourcePath());

            ANV_LOG_INFO(
                "Register asset resource='%s' normalized='%s' UUID='%s'",
                asset->GetResourcePath().c_str(),
                key.c_str(),
                id.uuid.c_str());

            m_ByResource.insert_or_assign(key, id);
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
        if (auto existing = GetByResourceAs<Texture>(resource))
        {
            ANV_LOG_WARN(
                "CreateTexture called for existing resource; "
                "returning existing texture: %s",
                resource.string().c_str());

            return existing;
        }

        auto texture = Texture::Create(resource);

        if (!texture)
        {
            ANV_LOG_ERROR(
                "Failed to create texture: %s",
                resource.string().c_str());

            return nullptr;
        }

        Register(texture.As<Asset>(), true);
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
            "GetOrCreateTexture input='%s' normalized='%s'",
            resource.string().c_str(),
            key.c_str());

        auto resourceIt = m_ByResource.find(key);

        if (resourceIt != m_ByResource.end())
        {
            ANV_LOG_INFO(
                "Reusing texture UUID: %s",
                resourceIt->second.uuid.c_str());

            return GetAs<Texture>(resourceIt->second);
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
