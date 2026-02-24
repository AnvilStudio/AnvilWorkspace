#pragma once
#include "../Util/Serialize/Serializer.h"
#include "../Core/Reference.h"
#include "../Core/Uuid.h"

namespace anv{

    struct Deserialized
    {
        std::string name;
        std::string resource;
        std::string uuid;
        std::string type;
        Serializer ser;
    };

    class Asset : public RefCounter
    {
    public:
        Asset(const std::string& _resource)
            : m_Uuid(uuid::uuid_GenAssetID())
        {
            set_resource(_resource);
        }

        Asset(Deserialized& _dser);

        uuid::AssetUUID GetAssetID() const { return m_Uuid; }

        const std::string& GetName() const { return m_Name; }
        const std::string& GetResourcePath() const { return m_ResourcePath; }
        const std::filesystem::path& GetMetaPath() const { return m_Meta; }
        void GenAssetFile();

        void Save();

    protected:
        void set_resource(const std::string& _path);
        virtual void OnSave(Serializer& _ser) = 0;

    protected:
        uuid::AssetUUID m_Uuid{};
        std::string m_ResourcePath; // full path to file
        std::string m_Name;         // filename
        std::filesystem::path m_Meta;         // full path to .aamta
    };
}
