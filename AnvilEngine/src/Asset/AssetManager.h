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

        template<class TAsset>
        Ref<TAsset> GetAs(const uuid::AssetUUID& _id) const
        {
            static_assert(std::is_base_of_v<Asset, TAsset>);
            return Get(_id).Cast<TAsset>();
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
            return asset;
        }

        Ref<GraphicsPipeline> CreateGraphicsPipeline(_shared<Context> _ctx, std::string _dName);
        Ref<Shader> CreateShader(const std::string& _shaderPath, _shared<Context> _ctx);

    private:
        void resolve_assets();
        void create(Deserialized& _dser);

    private:
        FileSystem& m_Fs;
        std::unordered_map<uuid::AssetUUID, Ref<Asset>> m_AssetReg;
        std::unordered_map<std::string, uuid::AssetUUID> m_ByResource; // de-dupe
    };
}

