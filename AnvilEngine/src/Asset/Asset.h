#pragma once
#include "../Util/Serialize/Serializer.h"
#include "../Core/Reference.h"
#include "../Core/Uuid.h"

namespace anv
{

    struct Deserialized
    {
        std::string name;
        std::string resource;
        std::string uuid;
        std::string type;
        std::filesystem::path metaPath;
        Serializer ser;
    };

    class Asset : public RefCounter
    {
    public:
        // Asset has no resource bound
        Asset(const std::string &_name);
        // register a resource as an engine asset for use
        Asset(const std::filesystem::path &_resource);
        // Seserialize from a file
        Asset(Deserialized &_dser);

        uuid::AssetUUID GetAssetID() const { return m_Uuid; }

        const std::string &GetName() const { return m_Name; }
        const std::string GetResourcePath() const { return m_ResourcePath.string(); }
        const std::filesystem::path &GetMetaPath() const { return m_Meta; }
        const std::string GetAssetType() const { return m_Type; }
        
        void GenMetaFile();
        
        bool HasMetaPath() const
        {
            return !m_Meta.empty();
        }

        void Save();

    protected:
        void set_resource(const std::filesystem::path &_path);
        void set_nonres_asset(const std::string &_name);
        virtual void OnSave(Serializer &_ser) = 0;

    protected:
        uuid::AssetUUID m_Uuid{};
        std::filesystem::path m_ResourcePath; // full path to file, empty if meta-only assets
        std::string m_Name;                   // filename
        std::string m_Type;                   // filename
        std::filesystem::path m_Meta;         // full path to .aamta
    };
}
