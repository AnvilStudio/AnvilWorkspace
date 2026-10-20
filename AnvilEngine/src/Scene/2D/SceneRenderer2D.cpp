#include "SceneRenderer2D.h"
#include "../Component.h"
#include "../vendor/entt/single_include/entt/entt.hpp"
#include <Render/Renderer.h>
#include <Asset/AssetTypes/Texture.h>
#include <Core/App.h>
#include <algorithm>
#include <vector>

namespace anv
{
    void SceneRenderer2D::Render(Ref<Scene> scene)
    {
        auto view = scene->m_Registry.view<
            Component::Transform2D,
            Component::SpriteRenderer>();

        std::vector<entt::entity> drawOrder;
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

        auto assetManager = App::GetInstance()->GetAssetManager();
        if (!assetManager)
            return;

        for (auto entity : drawOrder)
        {
            auto& transform = view.get<Component::Transform2D>(entity);
            auto& sprite = view.get<Component::SpriteRenderer>(entity);
            if (!sprite.visibleInGame)
                continue;
            Ref<Texture> texture = assetManager->GetAs<Texture>(sprite.texture);

            if (!scene->HasComponent<Component::Relationship>(entity))
                Renderer2D::DrawQuad(transform.position, transform.rotation,
                    transform.scale, sprite.color, texture, sprite.drawLayer);
            else
            {
                Component::Transform2D worldTransform;
                worldTransform.SetFromMatrix(scene->GetWorldTransform(entity));
                Renderer2D::DrawQuad(worldTransform.position, worldTransform.rotation,
                    worldTransform.scale, sprite.color, texture, sprite.drawLayer);
            }
        }
    }
}
