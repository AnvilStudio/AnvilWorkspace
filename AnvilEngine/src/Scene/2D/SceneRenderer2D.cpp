#include "SceneRenderer2D.h"
#include "../Component.h"
#include "../vendor/entt/single_include/entt/entt.hpp"
#include <Render/Renderer.h>
#include <Asset/AssetTypes/Texture.h>
#include <Core/App.h>
#include <algorithm>
#include <vector>
#include <chrono>

namespace anv
{
    void SceneRenderer2D::Render(Ref<Scene> scene)
    {
        auto view = scene->m_Registry.view<
            Component::Transform2D,
            Component::SpriteRenderer>();

        static thread_local std::vector<entt::entity> drawOrder;
        drawOrder.clear();
        drawOrder.reserve(view.size_hint());

        for (auto entity : view)
            drawOrder.push_back(entity);

        std::stable_sort(
            drawOrder.begin(),
            drawOrder.end(),
            [&](entt::entity left, entt::entity right)
            {
                return view.get<Component::SpriteRenderer>(left).drawLayer <
                    view.get<Component::SpriteRenderer>(right).drawLayer;
            });

        scene->RebuildWorldTransformSnapshot();
        auto assetManager = App::GetInstance()->GetAssetManager();
        if (!assetManager)
            return;

        for (auto entity : drawOrder)
        {
            auto& transform = view.get<Component::Transform2D>(entity);
            auto& sprite = view.get<Component::SpriteRenderer>(entity);
            if (!sprite.visibleInGame)
                continue;
            const auto t0 = std::chrono::steady_clock::now();
            Ref<Texture> texture = sprite.texture.uuid.empty() ? nullptr : assetManager->GetAs<Texture>(sprite.texture);
            const auto t1 = std::chrono::steady_clock::now();

            const Component::Transform2D* world = &transform;
            Component::Transform2D resolved;
            if (scene->HasComponent<Component::Relationship>(entity))
            {
                resolved.SetFromMatrix(scene->GetSnapshotWorldTransform(entity));
                world = &resolved;
            }
            const auto t2 = std::chrono::steady_clock::now();

            Renderer2D::DrawQuad(world->position, world->rotation,
                world->scale, 
                sprite.color, 
                texture, 
                sprite.drawLayer, 
                {sprite.flipX ? sprite.uvMax.x : sprite.uvMin.x,
                 sprite.flipY ? sprite.uvMax.y : sprite.uvMin.y},
                {sprite.flipX ? sprite.uvMin.x : sprite.uvMax.x,
                 sprite.flipY ? sprite.uvMin.y : sprite.uvMax.y}
            );

            const auto t3 = std::chrono::steady_clock::now();
            FrameProfiler::Record(FrameProfiler::Zone::SpriteLookup,
                std::chrono::duration<float, std::milli>(t1 - t0).count());
            FrameProfiler::Record(FrameProfiler::Zone::SpriteTransform,
                std::chrono::duration<float, std::milli>(t2 - t1).count());
            FrameProfiler::Record(FrameProfiler::Zone::SpriteSubmit,
                std::chrono::duration<float, std::milli>(t3 - t2).count());
        }
    }
}
