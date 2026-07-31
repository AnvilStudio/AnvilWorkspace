#include "InspectorLayer.h"
#include "../EditorHelpers.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

using namespace anv;

void InspectorLayer::Draw(
    Ref<Scene>& scene,
    entt::entity selectedEntity)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
    ImGui::Begin("Inspector");

    if (scene &&
        selectedEntity != entt::null &&
        scene->Registry().valid(selectedEntity))
    {
        ImGui::PushID(static_cast<uint32_t>(selectedEntity));

        draw_component<Component::Tag>(
            "Tag",
            selectedEntity,
            scene,
            [&](Component::Tag& tag)
            {
                char buffer[256]{};
                std::strncpy(buffer, tag.Get().c_str(), sizeof(buffer) - 1);

                if (ImGui::BeginTable("TagProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableNextRow();

                    if (property_text("Name", buffer, sizeof(buffer)))
                        tag.value = buffer;

                    ImGui::EndTable();
                }
            });

        draw_component<Component::Transform2d>(
            "Transform2D",
            selectedEntity,
            scene,
            [&](Component::Transform2d& transform)
            {
                if (ImGui::BeginTable("Transform2DProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableNextRow();
                    property_float2("Position", &transform.position.x);
                    ImGui::TableNextRow();
                    property_float("Rotation", &transform.rotation);
                    ImGui::TableNextRow();
                    property_float2("Scale", &transform.scale.x);
                    ImGui::EndTable();
                }
            });

        draw_component<Component::SpriteRenderer>(
            "Sprite Renderer",
            selectedEntity,
            scene,
            [&](Component::SpriteRenderer& sprite)
            {
                auto assetManager = App::GetInstance()->GetAssetManager();
                Ref<Texture> currentTexture = assetManager
                    ? assetManager->GetAs<Texture>(sprite.texture)
                    : nullptr;

                if (ImGui::BeginTable("SpriteProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Texture");
                    ImGui::TableSetColumnIndex(1);

                    const char* textureLabel = currentTexture
                        ? currentTexture->GetName().c_str()
                        : "None — drop texture here";

                    ImGui::Button("##TextureSlot", ImVec2(-1.0f, 0.0f));
                    ImGui::SetItemTooltip("%s", textureLabel);
                    ImGui::SameLine(0.0f, -ImGui::GetItemRectSize().x);
                    ImGui::TextUnformatted(textureLabel);

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload =
                                ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
                        {
                            const char* pathData =
                                static_cast<const char*>(payload->Data);

                            std::filesystem::path texturePath(pathData ? pathData : "");
                            std::string extension = texturePath.extension().string();
                            std::transform(
                                extension.begin(),
                                extension.end(),
                                extension.begin(),
                                [](unsigned char character)
                                {
                                    return static_cast<char>(std::tolower(character));
                                });

                            const bool isTexture =
                                extension == ".png" ||
                                extension == ".jpg" ||
                                extension == ".jpeg" ||
                                extension == ".bmp" ||
                                extension == ".tga";

                            if (isTexture && assetManager)
                            {
                                Ref<Texture> texture =
                                    assetManager->GetOrCreateTexture(texturePath);

                                if (texture && texture->IsGPUReady())
                                {
                                    sprite.texture = texture->GetAssetID();
                                    currentTexture = texture;
                                    scene->Save();
                                }
                            }
                        }

                        ImGui::EndDragDropTarget();
                    }

                    if (currentTexture)
                    {
                        ImGui::TextDisabled(
                            "%dx%d | %s",
                            currentTexture->Width(),
                            currentTexture->Height(),
                            currentTexture->IsGPUReady()
                                ? "GPU ready"
                                : "GPU unavailable");

                        if (ImGui::Button("Clear Texture"))
                        {
                            sprite.texture = {};
                            scene->Save();
                        }
                    }
                    else if (!sprite.texture.uuid.empty())
                    {
                        ImGui::TextDisabled(
                            "Missing asset: %s",
                            sprite.texture.uuid.c_str());
                    }

                    ImGui::TableNextRow();
                    property_color4("Color", &sprite.color.x);
                    ImGui::TableNextRow();
                    property_int("Draw Layer", &sprite.drawLayer);
                    ImGui::EndTable();
                }
            });

        draw_component<Component::Rigidbody2D>(
            "Rigidbody 2D",
            selectedEntity,
            scene,
            [&](Component::Rigidbody2D& rigidbody)
            {
                if (ImGui::BeginTable("Rigidbody2DProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    property_label("Body Type");
                    int bodyType = static_cast<int>(rigidbody.type);
                    const char* bodyTypes[] = {"Static", "Kinematic", "Dynamic"};
                    if (ImGui::Combo(
                            "##BodyType",
                            &bodyType,
                            bodyTypes,
                            IM_ARRAYSIZE(bodyTypes)))
                    {
                        rigidbody.type =
                            static_cast<Component::Rigidbody2DType>(bodyType);
                    }

                    ImGui::TableNextRow();
                    if (property_float("Gravity Scale", &rigidbody.gravityScale))
                        rigidbody.gravityScale = std::max(0.0f, rigidbody.gravityScale);

                    ImGui::TableNextRow();
                    if (property_float("Linear Damping", &rigidbody.linearDamping))
                        rigidbody.linearDamping = std::max(0.0f, rigidbody.linearDamping);

                    ImGui::TableNextRow();
                    if (property_float("Angular Damping", &rigidbody.angularDamping))
                        rigidbody.angularDamping = std::max(0.0f, rigidbody.angularDamping);

                    ImGui::TableNextRow();
                    property_bool("Fixed Rotation", &rigidbody.fixedRotation);
                    ImGui::TableNextRow();
                    property_bool("Bullet", &rigidbody.bullet);
                    ImGui::TableNextRow();
                    property_bool("Enabled", &rigidbody.enabled);

                    ImGui::EndTable();
                }
            });

        draw_component<Component::BoxCollider2D>(
            "Box Collider 2D",
            selectedEntity,
            scene,
            [&](Component::BoxCollider2D& collider)
            {
                if (ImGui::BeginTable("BoxCollider2DProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    if (property_float2("Size", &collider.size.x))
                    {
                        collider.size.x = std::max(0.001f, collider.size.x);
                        collider.size.y = std::max(0.001f, collider.size.y);
                    }

                    ImGui::TableNextRow();
                    property_float2("Offset", &collider.offset.x);

                    ImGui::TableNextRow();
                    if (property_float("Density", &collider.density))
                        collider.density = std::max(0.0f, collider.density);

                    ImGui::TableNextRow();
                    if (property_float("Friction", &collider.friction))
                        collider.friction = std::max(0.0f, collider.friction);

                    ImGui::TableNextRow();
                    if (property_float("Restitution", &collider.restitution))
                        collider.restitution = std::clamp(collider.restitution, 0.0f, 1.0f);

                    ImGui::TableNextRow();
                    property_bool("Sensor", &collider.sensor);

                    ImGui::EndTable();
                }
            });

        ImGui::PopID();
    }
    else
    {
        ImGui::TextDisabled("Select an entity to inspect");
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void InspectorLayer::draw_add_component_menu(
    Ref<Scene>& scene,
    entt::entity entity)
{
    if (!scene || entity == entt::null || !scene->Registry().valid(entity))
        return;

    ImGui::PushID("InspectorAddComponent");

    if (ImGui::Button("Add Component", ImVec2(-1.0f, 0.0f)))
        ImGui::OpenPopup("Popup");

    if (ImGui::BeginPopup("Popup"))
    {
        if (!scene->HasComponent<Component::SpriteRenderer>(entity) &&
            ImGui::MenuItem("Sprite Renderer"))
        {
            scene->AddComponent<Component::SpriteRenderer>(entity);
            ImGui::CloseCurrentPopup();
        }

        if (!scene->HasComponent<Component::Transform2d>(entity) &&
            ImGui::MenuItem("Transform 2D"))
        {
            scene->AddComponent<Component::Transform2d>(entity);
            ImGui::CloseCurrentPopup();
        }

        if (!scene->HasComponent<Component::Rigidbody2D>(entity) &&
            ImGui::MenuItem("Rigidbody 2D"))
        {
            scene->AddComponent<Component::Rigidbody2D>(entity);
            ImGui::CloseCurrentPopup();
        }

        if (!scene->HasComponent<Component::BoxCollider2D>(entity) &&
            ImGui::MenuItem("Box Collider 2D"))
        {
            scene->AddComponent<Component::BoxCollider2D>(entity);
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopID();
}
