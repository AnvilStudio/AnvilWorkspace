#include "Asset.h"
#include "Core/App.h"

namespace anv
{

    Asset::Asset(const std::string &_name)
        : m_Uuid(uuid::uuid_GenAssetID())
    {
        ANV_LOG_DEBUG("Asset Path: %s", _name.c_str())
        set_resource(_name);
    }

    Asset::Asset(const std::filesystem::path &_resource)
        : m_Uuid(uuid::uuid_GenAssetID())
    {
        ANV_LOG_DEBUG("Asset Path: %s", _resource.string().c_str())
        set_resource(_resource);
    }

    Asset::Asset(Deserialized &deserialized)
        : m_Uuid(deserialized.uuid),
          m_ResourcePath(deserialized.resource),
          m_Name(deserialized.name),
          m_Meta(deserialized.metaPath)
    {
    }

    // helper to find the name and set resource path of a source file
    void Asset::set_resource(const std::filesystem::path &_path)
    {
        m_ResourcePath = _path;

        m_Name = _path.filename().string();
    }

    void Asset::set_nonres_asset(const std::string &_name)
    {
        m_Name = _name;
    }

    void Asset::GenMetaFile()
    {
        auto &fs = App::GetInstance()->GetFS();

        const std::filesystem::path metaDir = fs.GetKeyVal("AssetMeta");
        ANV_ASSERT(!metaDir.empty(), "AssetMeta directory not registered");

        std::string fname = m_Name;
        std::replace(fname.begin(), fname.end(), ' ', '_');
        m_Meta = metaDir / (fname + ".aamta");

        Save(); // write initial meta
    }

    void Asset::Save()
    {
        ANV_ASSERT(
            !m_Meta.empty(),
            "Asset meta path not generated");

        Serializer ser(
            m_Meta,
            Serializer::Mode::SER_MODE_TOML,
            Serializer::Direction::Write);

        std::string resource =
            m_ResourcePath.generic_string();

        ser.Object("Asset", [&]
                   {
        ser.Field("Name", m_Name);
        ser.Field("Resource", resource);
        ser.Field("UUID", m_Uuid.uuid);

        OnSave(ser); });
    }
}