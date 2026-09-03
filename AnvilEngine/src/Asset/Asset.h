#pragma once

#include "../Core/Reference.h"
#include "../Core/Uuid.h"
#include "../Util/Serialize/Serializer.h"

#include <filesystem>
#include <string>

namespace anv
{
    /**
     * @brief Serialized metadata used to reconstruct an Asset.
     *
     * Asset loaders populate this structure from an Anvil asset metadata file
     * before handing it to the appropriate concrete Asset implementation.
     */
    struct Deserialized
    {
        std::string name;
        std::string resource;
        std::string uuid;
        std::string type;
        std::filesystem::path metaPath;
        Serializer ser;
    };

    /**
     * @brief Base class for resources tracked by Anvil's asset system.
     *
     * An Asset may represent either a file-backed resource or a metadata-only
     * engine object. Concrete asset types provide their type-specific serialized
     * fields through OnSave().
     */
    class Asset : public RefCounter
    {
    public:
        /**
         * @brief Creates a metadata-only asset.
         * @param _name Display name used by the asset registry and editor.
         */
        explicit Asset(const std::string& _name);

        /**
         * @brief Registers a file-backed resource as an engine asset.
         * @param _resource Path to the source resource.
         */
        explicit Asset(const std::filesystem::path& _resource);

        /**
         * @brief Reconstructs an asset from deserialized metadata.
         * @param _deserialized Parsed asset metadata.
         */
        explicit Asset(Deserialized& _deserialized);

        /** @return The persistent identifier assigned to this asset. */
        uuid::AssetUUID GetAssetID() const { return m_Uuid; }

        /** @return The asset's display name. */
        const std::string& GetName() const { return m_Name; }

        /**
         * @return The source resource path as a string, or an empty string for
         * metadata-only assets.
         */
        std::string GetResourcePath() const { return m_ResourcePath.string(); }

        /** @return The path to the asset's Anvil metadata file. */
        const std::filesystem::path& GetMetaPath() const { return m_Meta; }

        /** @return The serialized asset type identifier. */
        const std::string& GetAssetType() const { return m_Type; }

        /** @brief Generates the asset metadata file when one does not exist. */
        void GenMetaFile();

        /** @return True when the asset has an assigned metadata path. */
        bool HasMetaPath() const { return !m_Meta.empty(); }

        /** @brief Serializes the current asset state to its metadata file. */
        void Save();

    protected:
        /** @brief Configures this asset as a file-backed resource. */
        void set_resource(const std::filesystem::path& _path);

        /** @brief Configures this asset as a metadata-only resource. */
        void set_nonres_asset(const std::string& _name);

        /**
         * @brief Writes fields owned by a concrete asset type.
         * @param _serializer Serializer already populated with common Asset fields.
         */
        virtual void OnSave(Serializer& _serializer) = 0;

        uuid::AssetUUID m_Uuid{};
        std::filesystem::path m_ResourcePath;
        std::string m_Name;
        std::string m_Type;
        std::filesystem::path m_Meta;
    };
}
