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

    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        // -------- GET (never creates) --------
        Ref<Asset> Get(const uuid::AssetUUID& _id) const;
        Ref<Asset> GetByResource(const std::filesystem::path& _resource) const;

        template<class TAsset>
        Ref<TAsset> GetAs(const uuid::AssetUUID& _id) const
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);
            return Get(_id).Cast<TAsset>();
        }

        template<class TAsset>
        Ref<TAsset> GetByResourceAs(const std::filesystem::path& _resource) const
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);
            return GetByResource(_resource).Cast<TAsset>();
        }

        // -------- CREATE (never searches) --------
        template<class TAsset, class... Args>
        Ref<TAsset> Create(Args&&... _args)
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);

            Ref<TAsset> asset = Ref<TAsset>::Create(std::forward<Args>(_args)...);
            const uuid::AssetUUID id = asset->GetAssetID();

            asset->GenMetaFile();

            m_AssetReg.try_emplace(id, asset.template As<Asset>());

            if (!asset->GetResourcePath().empty())
                m_ByResource.insert_or_assign(NormalizeResource(asset->GetResourcePath()), id);

            return asset;
        }

        template<class TAsset, class... Args>
        Ref<TAsset> GetOrCreate(const std::filesystem::path& _resource, Args&&... _args)
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);

            if (auto existing = GetByResourceAs<TAsset>(_resource))
                return existing;

            return Create<TAsset>(_resource, std::forward<Args>(_args)...);
        }

        Ref<GraphicsPipeline> CreateGraphicsPipeline(_shared<Context> _ctx, std::string _dName);
        Ref<Shader> CreateShader(const std::string& _shaderPath, _shared<Context> _ctx);

    private:
        static std::string NormalizeResource(const std::filesystem::path& _resource);
        void resolve_assets();
        void create(Deserialized& _dser);

    private:
        FileSystem& m_Fs;
        std::unordered_map<uuid::AssetUUID, Ref<Asset>> m_AssetReg;
        std::unordered_map<std::string, uuid::AssetUUID> m_ByResource;
    };
}