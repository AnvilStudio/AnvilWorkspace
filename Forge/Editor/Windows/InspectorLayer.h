#pragma once

#include <Anvil.h>

class InspectorLayer
{
public:
    void Draw(
        anv::Ref<anv::Scene>& scene,
        entt::entity selectedEntity);

private:
    void draw_add_component_menu(
        anv::Ref<anv::Scene>& scene,
        entt::entity entity);
};
