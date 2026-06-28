#pragma once
#include "Anvil.h"

template<typename Comp, typename UIFunc>
void draw_component(
    const char* name,
    entt::entity entity,
    anv::Ref<anv::Scene> scene,
    UIFunc&& uiFunc)
{
    if (!scene->HasComponent<Comp>(entity))
        return;

    auto& component = scene->GetComponent<Comp>(entity);

    bool removeComponent = false;

    ImGui::PushID(name);

    if (ImGui::Button("..."))
        ImGui::OpenPopup("Settings");

    ImGui::SameLine();

    bool open = ImGui::CollapsingHeader(
        name,
        ImGuiTreeNodeFlags_DefaultOpen
    );

    if (ImGui::BeginPopup("Settings"))
    {
        if (ImGui::MenuItem("Remove Component"))
            removeComponent = true;

        ImGui::EndPopup();
    }

    if (open)
    {
        ImGui::Indent();

        uiFunc(component);

        ImGui::Unindent();
    }

    ImGui::PopID();

    if (removeComponent)
        scene->RemoveComponent<Comp>(entity);

    ImGui::Separator();
}