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

    // Full-width, square-cornered component header flush with Inspector edges.
    // The entire bar (including the settings control) shares one background.
    const ImGuiStyle& style = ImGui::GetStyle();
    const float sidePadding = style.WindowPadding.x;
    const ImVec2 headerStart = ImGui::GetCursorScreenPos();
    const float width = ImGui::GetContentRegionAvail().x;
    const float height = ImGui::GetFrameHeight() + 7.0f;
    const ImVec2 barMin(headerStart.x - sidePadding, headerStart.y);
    const ImVec2 barMax(headerStart.x + width + sidePadding, headerStart.y + height);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    const ImU32 barColor = IM_COL32(48, 53, 60, 255);
    const ImU32 edgeColor = IM_COL32(82, 96, 111, 255);
    drawList->AddRectFilled(barMin, barMax, barColor);
    drawList->AddRectFilled(
        ImVec2(barMin.x, barMax.y - 2.0f), barMax, edgeColor);

    // The collapsing arrow/title occupies the left; settings is pinned right.
    const float buttonWidth = 36.0f;
    const float buttonHeight = height - 6.0f;
    const float headerWidth = std::max(1.0f, barMax.x - barMin.x - buttonWidth - 12.0f);

    ImGui::SetCursorScreenPos(ImVec2(barMin.x + 6.0f, barMin.y + 3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.31f, 0.38f, 0.65f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.30f, 0.40f, 0.51f, 0.65f));
    ImGui::SetNextItemWidth(headerWidth);
    const bool open = ImGui::CollapsingHeader(
        name, ImGuiTreeNodeFlags_DefaultOpen |
              ImGuiTreeNodeFlags_AllowOverlap);
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    ImGui::SetCursorScreenPos(ImVec2(barMax.x - buttonWidth - 6.0f, barMin.y + 3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.31f, 0.38f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.30f, 0.40f, 0.51f, 1.0f));
    if (ImGui::Button("...", ImVec2(buttonWidth, buttonHeight)))
        ImGui::OpenPopup("Settings");
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // Reserve the full header height even when the collapsing arrow is closed.
    ImGui::SetCursorScreenPos(ImVec2(headerStart.x, barMax.y + 3.0f));
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

    ImGui::Dummy(ImVec2(0.0f, 4.0f));
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
