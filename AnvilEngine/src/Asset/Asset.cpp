#include "Asset.h"
#include "Core/App.h"

namespace anv {
    void Asset::SetResource(const std::string& _path)
    {
        m_ResourcePath = _path;

        size_t pos = _path.find_last_of("/\\");
        if (pos != std::string::npos)
            m_Name = _path.substr(pos + 1);
        else
            m_Name = _path;
    }

    void Asset::GenAssetFile()
    {
        auto& fs = App::GetInstance()->GetFS();

        const std::string metaDir = fs.AtKeyDir("AssetMeta");
        ANV_ASSERT(!metaDir.empty(), "AssetMeta directory not registered");

        m_Meta = metaDir + "/" + m_Name + ".aamta";

        Save(); // write initial meta
    }

    void Asset::Save()
    {
        ANV_ASSERT(!m_Meta.empty(), "Asset meta path not generated");

        Serializer ser(m_Meta,
            Serializer::Mode::SER_MODE_TOML,
            Serializer::Direction::Write);

        ser.Object("Asset", [&]
            {
                ser.Field("Name", m_Name);
                ser.Field("Resource", m_ResourcePath); // full file path now
                ser.Field("UUID", m_Uuid.uuid);
            });
    }
}