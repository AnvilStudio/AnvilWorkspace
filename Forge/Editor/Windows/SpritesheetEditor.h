#pragma once

#include <Anvil.h>
#include <algorithm>
#include <cmath>
#include <string>

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
            ImGui::TextDisabled("Select an entity with a Sprite Renderer.");
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
            ImGui::TextDisabled("Assign a valid texture in the Sprite Renderer inspector.");
            ImGui::End();
            return;
        }

        const int width = texture->Width();
        const int height = texture->Height();
        if (m_LastTexture != sprite.texture.uuid)
        {
            m_LastTexture = sprite.texture.uuid;
            m_SelectedColumn = 0;
            m_SelectedRow = 0;
        }

        // Header: asset information on the left, current entity on the right.
        ImGui::TextUnformatted(texture->GetName().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("| %d x %d px", width, height);
        ImGui::Separator();

        // Compact settings in an aligned two-column control layout.
        if (ImGui::BeginTable("##SheetSettings", 4, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("CELL WIDTH");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputInt("##CellWidth", &m_CellWidth, 0);
            ImGui::TableNextColumn();
            ImGui::TextDisabled("CELL HEIGHT");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputInt("##CellHeight", &m_CellHeight, 0);
            ImGui::TableNextColumn();
            ImGui::TextDisabled("MARGIN");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputInt("##Margin", &m_Margin, 0);
            ImGui::TableNextColumn();
            ImGui::TextDisabled("SPACING");
            ImGui::SetNextItemWidth(-1.0f);
            ImGui::InputInt("##Spacing", &m_Spacing, 0);
            ImGui::EndTable();
        }

        m_CellWidth = std::clamp(m_CellWidth, 1, width);
        m_CellHeight = std::clamp(m_CellHeight, 1, height);
        m_Margin = std::clamp(m_Margin, 0, std::min(width, height) - 1);
        m_Spacing = std::clamp(m_Spacing, 0, std::max(width, height));

        const int columns = std::max(0, (width - 2 * m_Margin + m_Spacing) / (m_CellWidth + m_Spacing));
        const int rows = std::max(0, (height - 2 * m_Margin + m_Spacing) / (m_CellHeight + m_Spacing));
        if (columns == 0 || rows == 0)
        {
            ImGui::TextDisabled("No cells fit these settings.");
            ImGui::End();
            return;
        }

        m_SelectedColumn = std::clamp(m_SelectedColumn, 0, columns - 1);
        m_SelectedRow = std::clamp(m_SelectedRow, 0, rows - 1);

        ImGui::Spacing();
        ImGui::TextDisabled("%d columns  /  %d rows  /  %d sprites", columns, rows, columns * rows);
        ImGui::SameLine();
        if (ImGui::SmallButton("Fit"))
        {
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            m_Zoom = std::clamp(std::min((avail.x - 20.0f) / width,
                (avail.y - 135.0f) / height), 0.1f, 8.0f);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(125.0f);
        ImGui::SliderFloat("##SheetZoom", &m_Zoom, 0.1f, 8.0f, "%.2fx");
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Spritesheet preview zoom");

        // Reserve an always-visible footer; only the image canvas scrolls.
        const float footerHeight = ImGui::GetFrameHeightWithSpacing() * 3.5f;
        const float canvasHeight = std::max(100.0f, ImGui::GetContentRegionAvail().y - footerHeight);
        ImGui::BeginChild("##SpritesheetCanvas", ImVec2(0.0f, canvasHeight),
            ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);

        void* imageID = texture->GetImGuiTextureID();
        if (imageID)
        {
            const ImVec2 topLeft = ImGui::GetCursorScreenPos();
            const ImVec2 imageSize(width * m_Zoom, height * m_Zoom);
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
            const ImU32 gridColor = IM_COL32(140, 165, 188, 85);
            // Clip all decoration to the canvas, including at high zoom.
            const ImVec2 clipMin = ImGui::GetWindowPos();
            const ImVec2 clipMax(clipMin.x + ImGui::GetWindowSize().x, clipMin.y + ImGui::GetWindowSize().y);
            draw->PushClipRect(clipMin, clipMax, true);
            for (int row = 0; row < rows && row < 1024; ++row)
                for (int col = 0; col < columns && col < 1024; ++col)
                {
                    const float x = topLeft.x + (m_Margin + col * (m_CellWidth + m_Spacing)) * m_Zoom;
                    const float y = topLeft.y + (m_Margin + row * (m_CellHeight + m_Spacing)) * m_Zoom;
                    const ImVec2 a(x, y);
                    const ImVec2 b(x + m_CellWidth * m_Zoom, y + m_CellHeight * m_Zoom);
                    const bool selected = col == m_SelectedColumn && row == m_SelectedRow;
                    if (selected)
                        draw->AddRectFilled(a, b, IM_COL32(90, 165, 235, 38));
                    draw->AddRect(a, b, selected ? IM_COL32(88, 179, 255, 255) : gridColor,
                        0.0f, 0, selected ? 2.0f : 1.0f);
                }
            draw->PopClipRect();
        }
        ImGui::EndChild();

        const int x = m_Margin + m_SelectedColumn * (m_CellWidth + m_Spacing);
        const int y = m_Margin + m_SelectedRow * (m_CellHeight + m_Spacing);
        const glm::vec2 uvMin(static_cast<float>(x) / width,
                              1.0f - static_cast<float>(y + m_CellHeight) / height);
        const glm::vec2 uvMax(static_cast<float>(x + m_CellWidth) / width,
                              1.0f - static_cast<float>(y) / height);

        ImGui::Separator();
        ImGui::Text("Selected  %d, %d", m_SelectedColumn, m_SelectedRow);
        ImGui::SameLine();
        ImGui::TextDisabled("|  UV  %.3f, %.3f  -  %.3f, %.3f",
            uvMin.x, uvMin.y, uvMax.x, uvMax.y);

        if (ImGui::Button("Apply Selected Cell"))
        {
            sprite.uvMin = uvMin;
            sprite.uvMax = uvMax;
            scene->Save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset to Full Texture"))
        {
            sprite.uvMin = glm::vec2(0.0f);
            sprite.uvMax = glm::vec2(1.0f);
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
