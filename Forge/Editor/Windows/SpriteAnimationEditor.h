#pragma once

#include <Anvil.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

// Forge-only authoring UI. The runtime reads the same SpriteAnimationClip format.
class SpriteAnimationEditor
{
public:
    void Draw(bool* open, anv::Ref<anv::Scene> scene, entt::entity entity)
    {
        if (!open || !*open) return;
        if (!ImGui::Begin("Sprite Animation Editor", open))
        {
            ImGui::End();
            return;
        }
        if (!scene || entity == entt::null || !scene->Registry().valid(entity) ||
            !scene->HasComponent<anv::Component::SpriteRenderer>(entity))
        {
            ImGui::TextDisabled("Select an entity with a Sprite Renderer.");
            ImGui::End();
            return;
        }

        auto& sprite = scene->GetComponent<anv::Component::SpriteRenderer>(entity);
        auto assets = anv::App::GetInstance()->GetAssetManager();
        auto texture = assets && !sprite.texture.uuid.empty()
            ? assets->GetAs<anv::Texture>(sprite.texture) : nullptr;
        if (!texture || !texture->IsGPUReady() || texture->Width() <= 0 || texture->Height() <= 0)
        {
            ImGui::TextDisabled("Assign a texture to the entity's Sprite Renderer first.");
            ImGui::End();
            return;
        }

        if (m_TextureID != sprite.texture.uuid)
        {
            m_TextureID = sprite.texture.uuid;
            m_Clip.textureUUID = m_TextureID;
            m_Clip.frames.clear();
            m_SelectedFrame = 0;
            m_PreviewFrame = 0;
            m_PreviewTime = 0;
            m_Playing = false;
            std::snprintf(m_ClipPath, sizeof(m_ClipPath), "%s", "Animations/NewAnimation.aanim");
            m_Status.clear();
        }

        ImGui::TextUnformatted(texture->GetName().c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("| %d x %d", texture->Width(), texture->Height());
        ImGui::Separator();

        if (ImGui::Button("New Clip"))
        {
            m_Clip = anv::SpriteAnimationClip{};
            m_Clip.textureUUID = sprite.texture.uuid;
            m_SelectedFrame = m_PreviewFrame = 0;
            m_PreviewTime = 0.0f;
            m_Playing = false;
            std::snprintf(m_ClipPath, sizeof(m_ClipPath), "%s", "Animations/NewAnimation.aanim");
            m_Status = "New clip ready. Add frames and click Save to create it.";
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(265);
        ImGui::InputTextWithHint("##AnimationPath", "Assets/Animations/Run.aanim",
                                 m_ClipPath, sizeof(m_ClipPath));
        ImGui::SameLine();
        if (ImGui::Button("Load"))
        {
            anv::SpriteAnimationClip loaded;
            if (loaded.Load(ResolvePath(scene, m_ClipPath)))
            {
                if (!loaded.textureUUID.empty() && loaded.textureUUID != sprite.texture.uuid)
                    m_Status = "Clip texture differs from selected Sprite Renderer.";
                else
                {
                    m_Clip = std::move(loaded);
                    m_Clip.textureUUID = sprite.texture.uuid;
                    m_SelectedFrame = m_PreviewFrame = 0;
                    m_PreviewTime = 0;
                    m_Playing = false;
                    m_Status = "Animation loaded.";
                }
            }
            else m_Status = "Could not read an animation clip.";
        }
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            std::filesystem::path requested(m_ClipPath);
            if (!requested.empty() && requested.extension().empty())
            {
                requested += ".aanim";
                std::snprintf(m_ClipPath, sizeof(m_ClipPath), "%s", requested.string().c_str());
            }
            const auto path = ResolvePath(scene, m_ClipPath);
            if (path.extension() != ".aanim" || m_Clip.frames.empty())
                m_Status = "Enter a .aanim path and add at least one frame.";
            else if (std::error_code ec; !std::filesystem::create_directories(path.parent_path(), ec) && ec)
                m_Status = "Could not create animation directory: " + ec.message();
            else
            {
                m_Clip.textureUUID = sprite.texture.uuid;
                m_Status = m_Clip.Save(path) ? "Animation saved." : "Could not save animation.";
                if (m_Status == "Animation saved." &&
                    scene->HasComponent<anv::Component::Animator2D>(entity))
                {
                    auto& animator = scene->GetComponent<anv::Component::Animator2D>(entity);
                    animator.clipPath = path.string();
                    animator.loadedPath.clear();
                    animator.Stop();
                    scene->Save();
                }
            }
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
            {
                if (payload->Data && payload->DataSize > 1)
                {
                    const char* bytes = static_cast<const char*>(payload->Data);
                    if (bytes[payload->DataSize - 1] == '\0')
                    {
                        const std::filesystem::path path(bytes);
                        if (path.extension() == ".aanim")
                            std::snprintf(m_ClipPath, sizeof(m_ClipPath), "%s", path.string().c_str());
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        if (!m_Status.empty()) ImGui::TextDisabled("%s", m_Status.c_str());

        ImGui::SeparatorText("Spritesheet");
        if (ImGui::BeginTable("##AnimSheetSettings", 4, ImGuiTableFlags_SizingStretchSame))
        {
            const char* names[] = {"Cell Width", "Cell Height", "Margin", "Spacing"};
            int* values[] = {&m_CellWidth, &m_CellHeight, &m_Margin, &m_Spacing};
            for (int i = 0; i < 4; ++i)
            {
                ImGui::TableNextColumn();
                ImGui::TextDisabled("%s", names[i]);
                ImGui::SetNextItemWidth(-1);
                ImGui::InputInt((std::string("##AnimDim") + std::to_string(i)).c_str(), values[i], 0);
            }
            ImGui::EndTable();
        }
        const int width = texture->Width(), height = texture->Height();
        m_CellWidth = std::clamp(m_CellWidth, 1, width);
        m_CellHeight = std::clamp(m_CellHeight, 1, height);
        m_Margin = std::clamp(m_Margin, 0, std::min(width, height) - 1);
        m_Spacing = std::clamp(m_Spacing, 0, std::max(width, height));
        const int columns = std::max(0, (width - 2*m_Margin + m_Spacing)/(m_CellWidth + m_Spacing));
        const int rows = std::max(0, (height - 2*m_Margin + m_Spacing)/(m_CellHeight + m_Spacing));
        ImGui::TextDisabled("Click cells to append frames  |  %d x %d grid", columns, rows);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(95);
        ImGui::SliderFloat("##AnimZoom", &m_Zoom, 0.2f, 5.0f, "%.2fx");

        void* imageID = texture->GetImGuiTextureID();
        if (imageID && rows > 0 && columns > 0)
        {
            ImGui::BeginChild("##AnimationSheetCanvas", ImVec2(0, std::max(120.0f, ImGui::GetContentRegionAvail().y * 0.48f)),
                ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
            const ImVec2 size(width*m_Zoom, height*m_Zoom);
            const ImVec2 avail = ImGui::GetContentRegionAvail();
            if (avail.x > size.x) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (avail.x-size.x)*0.5f);
            if (avail.y > size.y) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (avail.y-size.y)*0.5f);
            const ImVec2 origin = ImGui::GetCursorScreenPos();
            ImGui::Image(imageID, size, ImVec2(0,1), ImVec2(1,0));
            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
            {
                const ImVec2 mouse = ImGui::GetIO().MousePos;
                const float px = (mouse.x-origin.x)/m_Zoom, py = (mouse.y-origin.y)/m_Zoom;
                const int cx = static_cast<int>(std::floor((px-m_Margin)/(m_CellWidth+m_Spacing)));
                const int cy = static_cast<int>(std::floor((py-m_Margin)/(m_CellHeight+m_Spacing)));
                const float lx = px - m_Margin - cx*(m_CellWidth+m_Spacing);
                const float ly = py - m_Margin - cy*(m_CellHeight+m_Spacing);
                if (cx >= 0 && cx < columns && cy >= 0 && cy < rows &&
                    lx >= 0 && lx < m_CellWidth && ly >= 0 && ly < m_CellHeight)
                {
                    const int x = m_Margin + cx*(m_CellWidth+m_Spacing);
                    const int y = m_Margin + cy*(m_CellHeight+m_Spacing);
                    anv::SpriteAnimationFrame frame;
                    frame.uvMin = {float(x)/width, 1.0f-float(y+m_CellHeight)/height};
                    frame.uvMax = {float(x+m_CellWidth)/width, 1.0f-float(y)/height};
                    frame.duration = std::max(0.001f, m_DefaultDuration);
                    m_Clip.frames.push_back(frame);
                    m_SelectedFrame = m_Clip.frames.size()-1;
                    m_Status.clear();
                }
            }
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->PushClipRect(origin, ImVec2(origin.x+size.x, origin.y+size.y), true);
            for (int row = 0; row < rows && row < 512; ++row)
                for (int col = 0; col < columns && col < 512; ++col)
                {
                    const ImVec2 a(origin.x+(m_Margin+col*(m_CellWidth+m_Spacing))*m_Zoom,
                                   origin.y+(m_Margin+row*(m_CellHeight+m_Spacing))*m_Zoom);
                    const ImVec2 b(a.x+m_CellWidth*m_Zoom, a.y+m_CellHeight*m_Zoom);
                    draw->AddRect(a,b,IM_COL32(140,165,188,85));
                }
            draw->PopClipRect();
            ImGui::EndChild();
        }

        ImGui::SeparatorText("Timeline");
        ImGui::Checkbox("Loop", &m_Clip.loop);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::DragFloat("New frame seconds", &m_DefaultDuration, 0.01f, 0.001f, 10.0f, "%.3f");
        m_DefaultDuration = std::clamp(m_DefaultDuration, 0.001f, 10.0f);
        if (ImGui::Button(m_Playing ? "Pause" : "Play"))
        {
            if (!m_Clip.frames.empty()) m_Playing = !m_Playing;
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            m_Playing = false;
            m_PreviewFrame = 0;
            m_PreviewTime = 0;
        }
        if (m_Playing && !m_Clip.frames.empty())
        {
            m_PreviewTime += std::min(ImGui::GetIO().DeltaTime, 0.1f);
            // Limit catch-up work on stalls.
            for (int i = 0; i < 256 &&
                m_PreviewTime >= m_Clip.frames[m_PreviewFrame].duration; ++i)
            {
                m_PreviewTime -= m_Clip.frames[m_PreviewFrame].duration;
                if (++m_PreviewFrame == m_Clip.frames.size())
                {
                    if (m_Clip.loop) m_PreviewFrame = 0;
                    else { m_PreviewFrame = m_Clip.frames.size()-1; m_Playing = false; break; }
                }
            }
        }
        if (!m_Clip.frames.empty() && imageID)
        {
            m_PreviewFrame = std::min(m_PreviewFrame, m_Clip.frames.size()-1);
            const auto& frame = m_Clip.frames[m_PreviewFrame];
            ImGui::SameLine();
            ImGui::Image(imageID, ImVec2(54,54),
                ImVec2(frame.uvMin.x, frame.uvMax.y),
                ImVec2(frame.uvMax.x, frame.uvMin.y));
        }

        ImGui::BeginChild("##AnimationTimeline", ImVec2(0, 100), ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_HorizontalScrollbar);
        for (size_t i = 0; i < m_Clip.frames.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            const auto& frame = m_Clip.frames[i];
            if (i) ImGui::SameLine();
            ImGui::BeginGroup();
            if (imageID)
            {
                const bool selected = i == m_SelectedFrame;
                ImGui::PushStyleColor(ImGuiCol_Button, selected ?
                    ImGui::GetStyleColorVec4(ImGuiCol_TabActive) :
                    ImGui::GetStyleColorVec4(ImGuiCol_Button));
                if (ImGui::ImageButton("##AnimFrame", (ImTextureID)imageID, ImVec2(56,56),
                    ImVec2(frame.uvMin.x, frame.uvMax.y),
                    ImVec2(frame.uvMax.x, frame.uvMin.y)))
                {
                    m_SelectedFrame = i;
                    m_PreviewFrame = i;
                    m_PreviewTime = 0;
                    m_Playing = false;
                }
                ImGui::PopStyleColor();
            }
            ImGui::TextDisabled("%zu  %.2fs", i+1, frame.duration);
            ImGui::EndGroup();
            ImGui::PopID();
        }
        ImGui::EndChild();
        if (!m_Clip.frames.empty())
        {
            m_SelectedFrame = std::min(m_SelectedFrame, m_Clip.frames.size()-1);
            ImGui::Text("Selected frame %zu / %zu", m_SelectedFrame+1, m_Clip.frames.size());
            auto& selected = m_Clip.frames[m_SelectedFrame];
            ImGui::SetNextItemWidth(130);
            ImGui::DragFloat("Duration (s)", &selected.duration, 0.01f, 0.001f, 10.0f, "%.3f");
            selected.duration = std::clamp(selected.duration, 0.001f, 10.0f);
            if (ImGui::Button("Move Left") && m_SelectedFrame > 0)
            {
                std::swap(m_Clip.frames[m_SelectedFrame], m_Clip.frames[m_SelectedFrame-1]);
                --m_SelectedFrame;
            }
            ImGui::SameLine();
            if (ImGui::Button("Move Right") && m_SelectedFrame+1 < m_Clip.frames.size())
            {
                std::swap(m_Clip.frames[m_SelectedFrame], m_Clip.frames[m_SelectedFrame+1]);
                ++m_SelectedFrame;
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove Frame"))
            {
                m_Clip.frames.erase(m_Clip.frames.begin()+m_SelectedFrame);
                m_SelectedFrame = m_Clip.frames.empty() ? 0 : std::min(m_SelectedFrame, m_Clip.frames.size()-1);
                m_PreviewFrame = 0;
                m_Playing = false;
            }
        }
        ImGui::End();
    }

private:
    static std::filesystem::path ResolvePath(const anv::Ref<anv::Scene>& scene, const char* raw)
    {
        std::filesystem::path path(raw ? raw : "");
        if (path.empty()) return {};
        if (path.is_relative())
        {
            // Clips are project assets, not children of the active scene folder.
            const auto assetsRoot = anv::App::GetInstance()->GetFS().GetKeyVal("Assets");
            path = assetsRoot / (path.begin()->string() == "Assets" ?
                std::filesystem::path(path.string().substr(7)) : path);
        }
        return path.lexically_normal();
    }
    anv::SpriteAnimationClip m_Clip;
    std::string m_TextureID;
    char m_ClipPath[512]{};
    std::string m_Status;
    int m_CellWidth = 32, m_CellHeight = 32, m_Margin = 0, m_Spacing = 0;
    float m_DefaultDuration = 0.1f, m_Zoom = 1.0f, m_PreviewTime = 0.0f;
    size_t m_SelectedFrame = 0, m_PreviewFrame = 0;
    bool m_Playing = false;
};
