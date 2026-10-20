#pragma once
#include "Anvil.h"
#include <algorithm>

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
    (void)width;
    ImGui::TableSetColumnIndex(0);
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(label);

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-1.0f);
}

static bool property_float2(const char* label, float* value, float speed = 0.1f)
{
    property_label(label);
    // Keep spatial axes recognizable: X red, Y green (Z blue for 3D).
    const ImVec4 axisColors[2] = {
        ImVec4(0.80f, 0.29f, 0.31f, 1.0f),
        ImVec4(0.32f, 0.68f, 0.39f, 1.0f)
    };
    const char* axisLabels[2] = {"X", "Y"};
    const float spacing = ImGui::GetStyle().ItemInnerSpacing.x;
    const float available = ImGui::GetContentRegionAvail().x;
    const float fieldWidth = (available - spacing) * 0.5f;
    const float axisWidth = 19.0f;
    bool changed = false;

    ImGui::PushID(label);
    for (int axis = 0; axis < 2; ++axis)
    {
        if (axis != 0) ImGui::SameLine(0.0f, spacing);
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, axisColors[axis]);
        ImGui::TextUnformatted(axisLabels[axis]);
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 3.0f);
        ImGui::SetNextItemWidth(std::max(25.0f, fieldWidth - axisWidth));
        ImGui::PushID(axis);
        changed |= ImGui::DragFloat("##Axis", &value[axis], speed);
        ImGui::PopID();
        ImGui::EndGroup();
    }
    ImGui::PopID();
    return changed;
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

static bool property_bool(const char* label, bool* value)
{
    property_label(label);
    return ImGui::Checkbox(("##" + std::string(label)).c_str(), value);
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
