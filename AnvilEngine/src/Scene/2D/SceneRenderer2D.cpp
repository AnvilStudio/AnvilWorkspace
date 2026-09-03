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
            Component::Transform2d,
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

        for (auto entity : drawOrder)
        {
            auto& transform = view.get<Component::Transform2d>(entity);
            auto& sprite = view.get<Component::SpriteRenderer>(entity);
            Ref<Texture> texture = assetManager->GetAs<Texture>(sprite.texture);

            Renderer2D::DrawQuad(
                transform.position,
                transform.rotation,
                transform.scale,
                sprite.color,
                texture,
                sprite.drawLayer);
        }
    }
}
