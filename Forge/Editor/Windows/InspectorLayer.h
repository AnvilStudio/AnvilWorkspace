#pragma once

#include <Anvil.h>

class InspectorLayer
{
public:
    void Draw(
        const anv::Ref<anv::Scene>& scene,
        entt::entity selectedEntity);

private:
    void draw_add_component_menu(
        const anv::Ref<anv::Scene>& scene,
        entt::entity entity);
};
