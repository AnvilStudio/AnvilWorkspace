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

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        Ref<Asset> Get(const uuid::AssetUUID& id) const;
        Ref<Asset> GetByResource(const std::filesystem::path& resource) const;

        template<class TAsset>
        Ref<TAsset> GetAs(const uuid::AssetUUID& id) const
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);
            auto asset = Get(id);
            return asset ? asset.Cast<TAsset>() : nullptr;
        }

        template<class TAsset>
        Ref<TAsset> GetByResourceAs(const std::filesystem::path& resource) const
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);
            auto asset = GetByResource(resource);
            return asset ? asset.Cast<TAsset>() : nullptr;
        }

        template<class TAsset, class... Args>
        Ref<TAsset> Create(Args&&... args)
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);

            Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<Args>(args)...);
            Register(asset.template As<Asset>());
            return asset;
        }

        template<class TAsset, class... Args>
        Ref<TAsset> GetOrCreate(const std::filesystem::path& resource, Args&&... args)
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);

            if (auto existing = GetByResourceAs<TAsset>(resource))
                return existing;

            return Create<TAsset>(resource, std::forward<Args>(args)...);
        }

        Ref<Texture> CreateTexture(const std::filesystem::path& resource);
        Ref<Texture> CreateTexture(Deserialized& deserialized);
        Ref<Texture> GetOrCreateTexture(const std::filesystem::path& resource);

        Ref<GraphicsPipeline> CreateGraphicsPipeline(_shared<Context> context, std::string displayName);
        Ref<Shader> CreateShader(const std::string& shaderPath, _shared<Context> context);

    private:
        std::string NormalizeResource(const std::filesystem::path& resource) const;
        // void Register(Ref<Asset> asset);
        void Register(
            Ref<Asset> asset,
            bool generateMetadata = true);
        void resolve_assets();
        void create(Deserialized& deserialized);

    private:
        FileSystem& m_Fs;
        std::unordered_map<uuid::AssetUUID, Ref<Asset>> m_AssetReg;
        std::unordered_map<std::string, uuid::AssetUUID> m_ByResource;
    };
}
