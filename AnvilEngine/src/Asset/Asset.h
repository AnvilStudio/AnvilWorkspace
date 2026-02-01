#pragma once
#include "../Core/Reference.h"
#include "../Core/Uuid.h"

namespace anv{
    class Asset : public RefCounter
    {
    public:
        Asset(const std::string& _resource)
            : m_Uuid(uuid::uuid_GenAssetID())
        {
            SetResource(_resource);
            GenAssetFile(); // IMPORTANT
        }

        uuid::AssetUUID GetAssetID() const { return m_Uuid; }

        const std::string& GetName() const { return m_Name; }
        const std::string& GetResourcePath() const { return m_ResourcePath; }
        const std::string& GetMetaPath() const { return m_Meta; }

        void Save();

    protected:
        void SetResource(const std::string& _path);
        void GenAssetFile();

    protected:
        uuid::AssetUUID m_Uuid{};
        std::string m_ResourcePath; // full path to file
        std::string m_Name;         // filename
        std::string m_Meta;         // full path to .aamta
    };
}
