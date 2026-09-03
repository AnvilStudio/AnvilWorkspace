#pragma once
#include "Asset.h"
#include "../Render/GraphicsPipeline.h"
#include "../Core/Reference.h"
#include <type_traits>
#include <utility>
#include <unordered_map>

namespace anv
{
    class FileSystem;
    class Texture;

    struct AssetRegistryEntry
    {
        uuid::AssetUUID uuid;

        std::string name;
        std::string type;

        std::filesystem::path resource;
        std::filesystem::path metadata;

        uint32_t refCount;

        bool hasResource = false;
        bool resourceExists = false;

        bool hasMetadata = false;
        bool metadataExists = false;

        bool indexedByResource = false;
    };

    struct ResourceIndexEntry
    {
        std::string normalizedResource;
        uuid::AssetUUID uuid;

        std::string assetName;
        std::string assetType;

        bool resolvesToAsset = false;
        bool resourceExists = false;
    };

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        Ref<Asset> Get(const uuid::AssetUUID &id) const;
        Ref<Asset> GetByResource(const std::filesystem::path &resource) const;

        std::vector<AssetRegistryEntry>
        GetRegistrySnapshot() const
        {
            std::vector<AssetRegistryEntry> snapshot;
            snapshot.reserve(m_AssetReg.size());

            for (const auto &[assetId, asset] : m_AssetReg)
            {
                if (!asset)
                    continue;

                AssetRegistryEntry entry;

                entry.uuid = assetId;
                entry.name = asset->GetName();
                entry.type = asset->GetAssetType();
                entry.resource = asset->GetResourcePath();
                entry.metadata = asset->GetMetaPath();

                entry.hasResource =
                    !entry.resource.empty();

                entry.hasMetadata =
                    !entry.metadata.empty();

                std::error_code error;

                if (entry.hasResource)
                {
                    entry.resourceExists =
                        std::filesystem::exists(
                            entry.resource,
                            error);

                    const std::string normalized =
                        NormalizeResource(entry.resource);

                    auto resourceIt =
                        m_ByResource.find(normalized);

                    entry.indexedByResource =
                        resourceIt != m_ByResource.end() &&
                        resourceIt->second == assetId;
                }

                error.clear();

                if (entry.hasMetadata)
                {
                    entry.metadataExists =
                        std::filesystem::exists(
                            entry.metadata,
                            error);
                }

                snapshot.push_back(
                    std::move(entry));
            }

            std::sort(
                snapshot.begin(),
                snapshot.end(),
                [](const AssetRegistryEntry &left,
                   const AssetRegistryEntry &right)
                {
                    if (left.type != right.type)
                        return left.type < right.type;

                    return left.name < right.name;
                });

            return snapshot;
        }

        std::vector<ResourceIndexEntry>
        GetResourceIndexSnapshot() const
        {
            std::vector<ResourceIndexEntry> snapshot;
            snapshot.reserve(m_ByResource.size());

            for (const auto &[resourceKey, assetId] : m_ByResource)
            {
                ResourceIndexEntry entry;

                entry.normalizedResource = resourceKey;
                entry.uuid = assetId;

                std::error_code error;

                entry.resourceExists =
                    !resourceKey.empty() &&
                    std::filesystem::exists(
                        std::filesystem::path(resourceKey),
                        error);

                auto assetIt = m_AssetReg.find(assetId);

                if (assetIt != m_AssetReg.end() &&
                    assetIt->second)
                {
                    entry.resolvesToAsset = true;
                    entry.assetName =
                        assetIt->second->GetName();

                    entry.assetType =
                        assetIt->second->GetAssetType();
                }
                else
                {
                    entry.resolvesToAsset = false;
                    entry.assetName = "<unresolved>";
                    entry.assetType = "<unknown>";
                }

                snapshot.push_back(
                    std::move(entry));
            }

            std::sort(
                snapshot.begin(),
                snapshot.end(),
                [](const ResourceIndexEntry &left,
                   const ResourceIndexEntry &right)
                {
                    return left.normalizedResource <
                           right.normalizedResource;
                });

            return snapshot;
        }

        template <class TAsset>
        Ref<TAsset> GetAs(const uuid::AssetUUID &id) const
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);
            auto asset = Get(id);
            return asset ? asset.Cast<TAsset>() : nullptr;
        }

        template <class TAsset>
        Ref<TAsset> GetByResourceAs(const std::filesystem::path &resource) const
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);
            auto asset = GetByResource(resource);
            return asset ? asset.Cast<TAsset>() : nullptr;
        }

        template <class TAsset, class... Args>
        Ref<TAsset> Create(Args &&...args)
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);

            Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<Args>(args)...);
            Register(asset.template As<Asset>());
            return asset;
        }

        template <class TAsset, class... Args>
        Ref<TAsset> GetOrCreate(const std::filesystem::path &resource, Args &&...args)
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);

            if (auto existing = GetByResourceAs<TAsset>(resource))
                return existing;

            return Create<TAsset>(resource, std::forward<Args>(args)...);
        }

        Ref<Texture> CreateTexture(const std::filesystem::path &resource);
        Ref<Texture> CreateTexture(Deserialized &deserialized);
        Ref<Texture> GetOrCreateTexture(const std::filesystem::path &resource);

        Ref<GraphicsPipeline> CreateGraphicsPipeline(_shared<Context> context, std::string displayName);
        Ref<Shader> CreateShader(const std::string &shaderPath, _shared<Context> context);

    private:
        std::string NormalizeResource(const std::filesystem::path &resource) const;
        // void Register(Ref<Asset> asset);
        void Register(
            Ref<Asset> asset,
            bool generateMetadata = true);
        void resolve_assets();
        void create(Deserialized &deserialized);

    private:
        FileSystem &m_Fs;
        std::unordered_map<uuid::AssetUUID, Ref<Asset>> m_AssetReg;
        std::unordered_map<std::string, uuid::AssetUUID> m_ByResource;
    };
}
