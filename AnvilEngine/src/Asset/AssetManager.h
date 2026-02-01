#pragma once
#include "Asset.h"
#include "../Core/Reference.h"

#include <unordered_map>

namespace anv
{
    class AssetManager
    {
    public:
        AssetManager();
        ~AssetManager();

        Ref<Asset> GetOrCreate(const std::string& _resource);

    private:
        std::unordered_map<uuid::AssetUUID, Ref<Asset>> m_AssetReg;
        std::unordered_map<std::string, uuid::AssetUUID> m_ByResource; // de-dupe
    };
}

