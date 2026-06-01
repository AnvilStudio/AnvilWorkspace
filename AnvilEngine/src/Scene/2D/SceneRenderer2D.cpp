#include "SceneRenderer2D.h"
#include "../Component.h"
#include "../vendor/entt/single_include/entt/entt.hpp"
#include <Render/Renderer.h>

namespace anv
{
	void SceneRenderer2D::Render(Ref<Scene> _scene)
	{
        auto view = _scene->m_Registry.view<
            Component::Transform2d,
            Component::SpriteRenderer>();

        view.each([](
            auto entity,
            Component::Transform2d& transform,
            Component::SpriteRenderer& sprite)
            {
                Renderer2D::DrawQuad(
                    transform.Position,
                    transform.Scale,
                    sprite.Color
                );
            });
	}
}