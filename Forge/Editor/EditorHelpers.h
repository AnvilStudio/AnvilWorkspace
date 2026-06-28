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

static void property_label(const char* label, float width = 90.0f)
{
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
}

static bool property_float2(const char* label, float* value, float speed = 0.1f)
{
    property_label(label);
    return ImGui::DragFloat2(("##" + std::string(label)).c_str(), value, speed);
}

static bool property_float(const char* label, float* value, float speed = 0.1f)
{
    property_label(label);
    return ImGui::DragFloat(("##" + std::string(label)).c_str(), value, speed);
}

static bool property_int(const char* label, int* value)
{
    property_label(label);
    return ImGui::DragInt(("##" + std::string(label)).c_str(), value);
}

static bool property_color4(const char* label, float* value)
{
    property_label(label);
    return ImGui::ColorEdit4(("##" + std::string(label)).c_str(), value);
}

static bool property_text(const char* label, char* buffer, size_t size)
{
    property_label(label);
    return ImGui::InputText(("##" + std::string(label)).c_str(), buffer, size);
}