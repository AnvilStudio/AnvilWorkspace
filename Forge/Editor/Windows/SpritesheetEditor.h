#pragma once

#include <Anvil.h>
#include <algorithm>
#include <cmath>

class SpritesheetEditor
{
public:
    void Draw(bool* open, anv::Ref<anv::Scene> scene, entt::entity selectedEntity)
    {
        if (!open || !*open)
            return;

        if (!ImGui::Begin("Spritesheet Editor", open))
        {
            ImGui::End();
            return;
        }

        if (!scene || selectedEntity == entt::null ||
            !scene->Registry().valid(selectedEntity) ||
            !scene->HasComponent<anv::Component::SpriteRenderer>(selectedEntity))
        {
            ImGui::TextDisabled("Select an entity with a Sprite Renderer to edit its spritesheet.");
            ImGui::End();
            return;
        }

        auto& sprite = scene->GetComponent<anv::Component::SpriteRenderer>(selectedEntity);
        auto assets = anv::App::GetInstance()->GetAssetManager();
        auto texture = assets && !sprite.texture.uuid.empty()
            ? assets->GetAs<anv::Texture>(sprite.texture) : nullptr;

        if (!texture || !texture->IsGPUReady() ||
            texture->Width() <= 0 || texture->Height() <= 0)
        {
            ImGui::TextDisabled("Assign a valid texture to the selected Sprite Renderer first.");
            ImGui::End();
            return;
        }

        const int width = texture->Width();
        const int height = texture->Height();
        const std::string assetID = sprite.texture.uuid;
        if (assetID != m_LastTexture)
        {
            m_LastTexture = assetID;
            m_SelectedColumn = 0;
            m_SelectedRow = 0;
        }

        ImGui::Text("%s (%d x %d)", texture->GetName().c_str(), width, height);
        ImGui::SetNextItemWidth(95.0f);
        ImGui::InputInt("Cell Width", &m_CellWidth);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(95.0f);
        ImGui::InputInt("Cell Height", &m_CellHeight);

        ImGui::SetNextItemWidth(95.0f);
        ImGui::InputInt("Margin", &m_Margin);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(95.0f);
        ImGui::InputInt("Spacing", &m_Spacing);

        m_CellWidth = std::clamp(m_CellWidth, 1, width);
        m_CellHeight = std::clamp(m_CellHeight, 1, height);
        m_Margin = std::clamp(m_Margin, 0, std::min(width, height) - 1);
        m_Spacing = std::max(0, m_Spacing);

        const int columns = std::max(0, (width - 2 * m_Margin + m_Spacing) / (m_CellWidth + m_Spacing));
        const int rows = std::max(0, (height - 2 * m_Margin + m_Spacing) / (m_CellHeight + m_Spacing));
        if (columns == 0 || rows == 0)
        {
            ImGui::TextDisabled("The current grid settings do not fit any cells.");
            ImGui::End();
            return;
        }

        m_SelectedColumn = std::clamp(m_SelectedColumn, 0, columns - 1);
        m_SelectedRow = std::clamp(m_SelectedRow, 0, rows - 1);
        ImGui::Text("%d columns x %d rows (%d sprites)", columns, rows, columns * rows);

        ImGui::SetNextItemWidth(150.0f);
        ImGui::SliderFloat("Zoom", &m_Zoom, 0.25f, 8.0f, "%.2fx");

        // The preview uses top-left pixel coordinates; convert the selection to
        // Anvil's bottom-left UV convention when applying it to SpriteRenderer.
        const ImVec2 imageSize(width * m_Zoom, height * m_Zoom);
        ImGui::BeginChild("##SpritesheetCanvas", ImVec2(0.0f, -ImGui::GetFrameHeightWithSpacing() * 2.5f),
            ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

        void* imageID = texture->GetImGuiTextureID();
        if (imageID)
        {
            const ImVec2 topLeft = ImGui::GetCursorScreenPos();
            ImGui::Image(imageID, imageSize, ImVec2(0, 1), ImVec2(1, 0));
            const ImVec2 mouse = ImGui::GetIO().MousePos;
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                const float px = (mouse.x - topLeft.x) / m_Zoom;
                const float py = (mouse.y - topLeft.y) / m_Zoom;
                const int strideX = m_CellWidth + m_Spacing;
                const int strideY = m_CellHeight + m_Spacing;
                const int ix = static_cast<int>(std::floor((px - m_Margin) / strideX));
                const int iy = static_cast<int>(std::floor((py - m_Margin) / strideY));
                const float localX = px - m_Margin - ix * strideX;
                const float localY = py - m_Margin - iy * strideY;
                if (ix >= 0 && ix < columns && iy >= 0 && iy < rows &&
                    localX >= 0.0f && localX < m_CellWidth &&
                    localY >= 0.0f && localY < m_CellHeight)
                {
                    m_SelectedColumn = ix;
                    m_SelectedRow = iy;
                }
            }

            ImDrawList* draw = ImGui::GetWindowDrawList();
            const ImU32 gridColor = IM_COL32(230, 235, 240, 100);
            for (int row = 0; row < rows && row < 1024; ++row)
                for (int col = 0; col < columns && col < 1024; ++col)
                {
                    const float x = topLeft.x + (m_Margin + col * (m_CellWidth + m_Spacing)) * m_Zoom;
                    const float y = topLeft.y + (m_Margin + row * (m_CellHeight + m_Spacing)) * m_Zoom;
                    const ImVec2 a(x, y);
                    const ImVec2 b(x + m_CellWidth * m_Zoom, y + m_CellHeight * m_Zoom);
                    const bool selected = col == m_SelectedColumn && row == m_SelectedRow;
                    draw->AddRect(a, b, selected ? IM_COL32(100, 190, 255, 255) : gridColor,
                        0.0f, 0, selected ? 2.0f : 1.0f);
                }
        }
        ImGui::EndChild();

        const int x = m_Margin + m_SelectedColumn * (m_CellWidth + m_Spacing);
        const int y = m_Margin + m_SelectedRow * (m_CellHeight + m_Spacing);
        const glm::vec2 uvMin(static_cast<float>(x) / width,
                              1.0f - static_cast<float>(y + m_CellHeight) / height);
        const glm::vec2 uvMax(static_cast<float>(x + m_CellWidth) / width,
                              1.0f - static_cast<float>(y) / height);

        ImGui::Text("Cell (%d, %d)  UV Min (%.3f, %.3f)  Max (%.3f, %.3f)",
            m_SelectedColumn, m_SelectedRow, uvMin.x, uvMin.y, uvMax.x, uvMax.y);
        if (ImGui::Button("Apply to Selected Sprite"))
        {
            sprite.uvMin = uvMin;
            sprite.uvMax = uvMax;
            scene->Save();
        }

        ImGui::End();
    }

private:
    std::string m_LastTexture;
    int m_CellWidth = 32;
    int m_CellHeight = 32;
    int m_Margin = 0;
    int m_Spacing = 0;
    int m_SelectedColumn = 0;
    int m_SelectedRow = 0;
    float m_Zoom = 2.0f;
};
