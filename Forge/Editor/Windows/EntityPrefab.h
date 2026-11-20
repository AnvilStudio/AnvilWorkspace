#pragma once

#include <Anvil.h>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <cstdio>

namespace forge
{
    struct EntitySnapshot
    {
        std::string name;
        int parent = -1;
        bool transform = false, camera = false, sprite = false, animator = false;
        bool rigidbody = false, collider = false, script = false;
        anv::Component::Transform2D transformData{};
        anv::Component::Camera2D cameraData{};
        anv::Component::SpriteRenderer spriteData{};
        anv::Component::Animator2D animatorData{};
        anv::Component::Rigidbody2D rigidbodyData{};
        anv::Component::BoxCollider2D colliderData{};
        anv::Component::Script scriptData{};
    };

    struct EntityPrefab
    {
        std::vector<EntitySnapshot> nodes;

        void Clear() { nodes.clear(); }
        bool Empty() const { return nodes.empty(); }

        void Capture(anv::Ref<anv::Scene> scene, const std::vector<entt::entity>& selection)
        {
            Clear();
            if (!scene) return;
            std::unordered_set<entt::entity> visited;
            auto collect = [&](auto&& self, entt::entity entity, int parent, int depth) -> void
            {
                if (depth > 256 || !scene->Registry().valid(entity) || !visited.insert(entity).second)
                    return;
                EntitySnapshot snap;
                snap.parent = parent;
                if (scene->HasComponent<anv::Component::Tag>(entity))
                    snap.name = scene->GetComponent<anv::Component::Tag>(entity).value;
                if ((snap.transform = scene->HasComponent<anv::Component::Transform2D>(entity)))
                    snap.transformData = scene->GetComponent<anv::Component::Transform2D>(entity);
                if ((snap.camera = scene->HasComponent<anv::Component::Camera2D>(entity)))
                    snap.cameraData = scene->GetComponent<anv::Component::Camera2D>(entity);
                if ((snap.sprite = scene->HasComponent<anv::Component::SpriteRenderer>(entity)))
                    snap.spriteData = scene->GetComponent<anv::Component::SpriteRenderer>(entity);
                if ((snap.animator = scene->HasComponent<anv::Component::Animator2D>(entity)))
                    snap.animatorData = scene->GetComponent<anv::Component::Animator2D>(entity);
                if ((snap.rigidbody = scene->HasComponent<anv::Component::Rigidbody2D>(entity)))
                    snap.rigidbodyData = scene->GetComponent<anv::Component::Rigidbody2D>(entity);
                if ((snap.collider = scene->HasComponent<anv::Component::BoxCollider2D>(entity)))
                    snap.colliderData = scene->GetComponent<anv::Component::BoxCollider2D>(entity);
                if ((snap.script = scene->HasComponent<anv::Component::Script>(entity)))
                    snap.scriptData = scene->GetComponent<anv::Component::Script>(entity);
                const int index = static_cast<int>(nodes.size());
                nodes.push_back(std::move(snap));
                for (auto child : scene->GetChildren(entity))
                    self(self, child, index, depth + 1);
            };
            for (auto entity : selection)
            {
                if (!scene->Registry().valid(entity)) continue;
                bool nested = false;
                for (auto other : selection)
                    if (entity != other && scene->Registry().valid(other) &&
                        scene->IsDescendant(entity, other)) { nested = true; break; }
                if (!nested) collect(collect, entity, -1, 0);
            }
        }

        std::vector<entt::entity> Instantiate(anv::Ref<anv::Scene> scene, const std::filesystem::path& source = {}) const
        {
            std::vector<entt::entity> roots;
            if (!scene || nodes.empty()) return roots;
            std::vector<entt::entity> created;
            created.reserve(nodes.size());
            for (const auto& snap : nodes)
            {
                const auto entity = scene->CreateEntity(snap.name);
                if (snap.transform)
                    scene->GetComponent<anv::Component::Transform2D>(entity) = snap.transformData;
                else scene->RemoveComponent<anv::Component::Transform2D>(entity);
                if (snap.camera) scene->AddComponent<anv::Component::Camera2D>(entity) = snap.cameraData;
                if (snap.sprite) scene->AddComponent<anv::Component::SpriteRenderer>(entity) = snap.spriteData;
                if (snap.animator)
                {
                    auto& a = scene->AddComponent<anv::Component::Animator2D>(entity);
                    a = snap.animatorData;
                    a.loadedPath.clear();
                    a.Stop();
                }
                if (snap.rigidbody) scene->AddComponent<anv::Component::Rigidbody2D>(entity) = snap.rigidbodyData;
                if (snap.collider) scene->AddComponent<anv::Component::BoxCollider2D>(entity) = snap.colliderData;
                if (snap.script) scene->AddComponent<anv::Component::Script>(entity) = snap.scriptData;
                created.push_back(entity);
            }
            for (size_t i = 0; i < nodes.size(); ++i)
            {
                const int parent = nodes[i].parent;
                if (parent >= 0 && static_cast<size_t>(parent) < i)
                    scene->SetParent(created[i], created[parent], false);
                else roots.push_back(created[i]);
            }
            if (!source.empty())
                for (auto root : roots)
                    scene->AddComponent<anv::Component::PrefabInstance>(root).sourcePath = source.string();
            scene->Save();
            return roots;
        }

        // Refresh an existing instance from its source asset. Preserve its root
        // handle, UUID, parent and transform so external references remain stable.
        // Descendants are reconstructed and receive fresh UUIDs in this v1 model.
        bool Refresh(anv::Ref<anv::Scene> scene, entt::entity root) const
        {
            if (!scene || nodes.empty() || !scene->Registry().valid(root) ||
                !scene->HasComponent<anv::Component::PrefabInstance>(root) ||
                nodes[0].parent != -1)
                return false;
            const auto source = scene->GetComponent<anv::Component::PrefabInstance>(root).sourcePath;
            const auto oldChildren = scene->GetChildren(root);
            // Destroy deepest nodes first to prevent orphaning/reparenting side effects.
            auto destroyTree = [&](auto&& self, entt::entity entity, int depth) -> void
            {
                if (depth > 256 || !scene->Registry().valid(entity)) return;
                for (auto child : scene->GetChildren(entity))
                    self(self, child, depth+1);
                scene->DestroyEntity(entity);
            };
            for (auto child : oldChildren) destroyTree(destroyTree, child, 0);

            const auto& n = nodes.front();
            scene->GetComponent<anv::Component::Tag>(root).value = n.name;
            // Keep root transform and root UUID across prefab updates.
            auto sync = [&](auto typeTag, bool present, const auto& value)
            {
                using T = decltype(typeTag);
                if (present) scene->AddComponent<T>(root) = value;
                else scene->RemoveComponent<T>(root);
            };
            sync(anv::Component::Camera2D{}, n.camera, n.cameraData);
            sync(anv::Component::SpriteRenderer{}, n.sprite, n.spriteData);
            sync(anv::Component::Animator2D{}, n.animator, n.animatorData);
            sync(anv::Component::Rigidbody2D{}, n.rigidbody, n.rigidbodyData);
            sync(anv::Component::BoxCollider2D{}, n.collider, n.colliderData);
            sync(anv::Component::Script{}, n.script, n.scriptData);

            std::vector<entt::entity> created(nodes.size());
            std::fill(created.begin(), created.end(), entt::entity{entt::null});
            created[0] = root;
            for (size_t i = 1; i < nodes.size(); ++i)
            {
                const auto& child = nodes[i];
                auto entity = scene->CreateEntity(child.name);
                if (child.transform)
                    scene->GetComponent<anv::Component::Transform2D>(entity) = child.transformData;
                if (child.camera) scene->AddComponent<anv::Component::Camera2D>(entity) = child.cameraData;
                if (child.sprite) scene->AddComponent<anv::Component::SpriteRenderer>(entity) = child.spriteData;
                if (child.animator) scene->AddComponent<anv::Component::Animator2D>(entity) = child.animatorData;
                if (child.rigidbody) scene->AddComponent<anv::Component::Rigidbody2D>(entity) = child.rigidbodyData;
                if (child.collider) scene->AddComponent<anv::Component::BoxCollider2D>(entity) = child.colliderData;
                if (child.script) scene->AddComponent<anv::Component::Script>(entity) = child.scriptData;
                created[i] = entity;
                const int parent = child.parent;
                scene->SetParent(entity,
                    parent >= 0 && static_cast<size_t>(parent) < i ? created[parent] : root,
                    false);
            }
            scene->GetComponent<anv::Component::PrefabInstance>(root).sourcePath = source;
            scene->Save();
            return true;
        }

        bool Save(const std::filesystem::path& path) const
        {
            if (nodes.empty() || path.extension() != ".aprefab") return false;
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec) return false;
            try
            {
                anv::Serializer ser(path, anv::Serializer::Mode::SER_MODE_TOML,
                                    anv::Serializer::Direction::Write);
                for (size_t i = 0; i < nodes.size(); ++i)
                {
                    const auto& n = nodes[i];
                    char key[24];
                    std::snprintf(key, sizeof(key), "%08zu", i);
                    ser.ObjectKeyed("Entities", key, [&]
                    {
                        std::string name = n.name;
                        int parent = n.parent;
                        ser.Field("Name", name);
                        ser.Field("ParentIndex", parent);
                        auto write = [&](const char* label, auto data, bool present)
                        {
                            if (present) ser.Object(label, [&] { data.Serialize(ser); });
                        };
                        write("Transform2d", n.transformData, n.transform);
                        write("Camera2D", n.cameraData, n.camera);
                        write("SpriteRenderer", n.spriteData, n.sprite);
                        write("Animator2D", n.animatorData, n.animator);
                        write("Rigidbody2D", n.rigidbodyData, n.rigidbody);
                        write("BoxCollider2D", n.colliderData, n.collider);
                        write("Script", n.scriptData, n.script);
                    });
                }
                ser.Close();
                return true;
            }
            catch (...) { return false; }
        }

        bool Load(const std::filesystem::path& path)
        {
            if (!std::filesystem::is_regular_file(path) || path.extension() != ".aprefab")
                return false;
            std::vector<EntitySnapshot> result;
            try
            {
                anv::Serializer ser(path, anv::Serializer::Mode::SER_MODE_TOML,
                                    anv::Serializer::Direction::Read);
                ser.ForEachTable("Entities", [&](const std::string&)
                {
                    EntitySnapshot n;
                    ser.FieldOr<std::string>("Name", n.name, "Entity");
                    ser.FieldOr<int>("ParentIndex", n.parent, -1);
                    auto read = [&](const char* label, auto& data, bool& present)
                    {
                        present = ser.HasObject(label);
                        if (present) ser.Object(label, [&] { data.Deserialize(ser); });
                    };
                    read("Transform2d", n.transformData, n.transform);
                    read("Camera2D", n.cameraData, n.camera);
                    read("SpriteRenderer", n.spriteData, n.sprite);
                    read("Animator2D", n.animatorData, n.animator);
                    read("Rigidbody2D", n.rigidbodyData, n.rigidbody);
                    read("BoxCollider2D", n.colliderData, n.collider);
                    read("Script", n.scriptData, n.script);
                    result.push_back(std::move(n));
                });
            }
            catch (...) { return false; }
            if (result.empty()) return false;
            for (size_t i = 0; i < result.size(); ++i)
                if (result[i].parent >= static_cast<int>(i) || result[i].parent < -1)
                    return false;
            nodes = std::move(result);
            return true;
        }
    };
}
