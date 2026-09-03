#pragma once

#include "Scene.h"
#include "../Core/Uuid.h"
#include "../Core/Reference.h"
#include "../Util/Serialize/Serializer.h"
#include "SceneData.h"

namespace anv
{
    class SceneManager
    {
    public:
        SceneManager();
        ~SceneManager();

        Ref<Scene> Create(std::string _name);

        // Register is for "create a scene with externally supplied info"
        // e.g. from disk metadata, editor registry, etc.
        Ref<Scene> Register(std::string _path);
        void Register(Ref<Scene> _scene);

        Ref<Scene> GetActive();
        void SetActive(uuid::AssetUUID _sceneUUID);

        /**
         * @brief Opens an existing scene asset and makes it active.
         *
         * A scene already present in the registry is reused. The previous active
         * scene is saved before switching. When _createIfMissing is true, an empty
         * scene asset is created when the target file does not yet contain data.
         */
        Ref<Scene> OpenScene(
            const std::filesystem::path& _path,
            bool _createIfMissing = false);

        /** @brief Creates a new scene asset at _path and makes it active. */
        Ref<Scene> CreateScene(const std::filesystem::path& _path);

        /**
         * @brief Replaces the active in-memory scene with a freshly loaded copy.
         *
         * The current scene path is preserved. Runtime script instances are shut
         * down before the old scene is released. This is used by Forge to restore
         * the edit-time scene after a Play session.
         */
        Ref<Scene> ReloadActive();

        void Shutdown();
        //void Unload(uuid::AssetUUID _sceneUUID);

    private:
        bool m_HasShutdown = false;
        uuid::AssetUUID m_Active;
        std::unordered_map<uuid::AssetUUID, Ref<Scene>> m_Registry;
    };
}
