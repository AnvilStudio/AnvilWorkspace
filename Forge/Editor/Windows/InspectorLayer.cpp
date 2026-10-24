#include "InspectorLayer.h"
#include "../EditorHelpers.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

using namespace anv;

void InspectorLayer::Draw(
    Ref<Scene> &scene,
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
            [&](Component::Tag &tag)
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

        // Relationship is scene metadata rather than an add/remove component.
        const entt::entity parent = scene->GetParent(selectedEntity);
        if (parent != entt::null)
        {
            std::string parentName = "Entity";
            if (scene->HasComponent<Component::Tag>(parent))
                parentName = scene->GetComponent<Component::Tag>(parent).Get();
            ImGui::Text("Parent: %s", parentName.c_str());
            ImGui::SameLine();
            if (ImGui::SmallButton("Unparent"))
                if (scene->SetParent(selectedEntity, entt::null))
                    scene->Save();
        }
        else
        {
            ImGui::TextDisabled("Parent: None (root)");
        }
        ImGui::TextDisabled("Transforms are local to the parent");

        draw_component<Component::Transform2D>(
            "Transform2D",
            selectedEntity,
            scene,
            [&](Component::Transform2D &transform)
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

        draw_component<Component::Camera2D>(
            "Camera 2D",
            selectedEntity,
            scene,
            [&](Component::Camera2D &cameraComponent)
            {
                if (!cameraComponent.camera)
                    cameraComponent.camera = std::make_shared<anv::Camera2D>();

                if (ImGui::BeginTable("Camera2DProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 100.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    bool active = cameraComponent.isActive;
                    ImGui::TableNextRow();
                    if (property_bool("Is Active", &active))
                    {
                        if (active)
                            scene->SetActiveCamera(selectedEntity);
                        else
                            cameraComponent.isActive = false;

                        scene->Save();
                    }

                    float zoom = cameraComponent.camera->GetZoom();
                    ImGui::TableNextRow();
                    if (property_float("Zoom", &zoom))
                    {
                        cameraComponent.camera->SetZoom(std::max(0.01f, zoom));
                        scene->Save();
                    }

                    ImGui::EndTable();
                }
            });

        draw_component<Component::SpriteRenderer>(
            "Sprite Renderer",
            selectedEntity,
            scene,
            [&](Component::SpriteRenderer &sprite)
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

                    const char *textureLabel = currentTexture
                                                   ? currentTexture->GetName().c_str()
                                                   : "None — drop texture here";

                    ImGui::Button(textureLabel, ImVec2(-1.0f, 0.0f));

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload *payload =
                                ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
                        {
                            const char *pathData =
                                static_cast<const char *>(payload->Data);

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
                                else
                                {
                                    if (!texture)
                                    {
                                        ANV_LOG_ERROR("Failed Drag/Drop Texture, No Texture Found!");
                                    }
                                    else if (!texture->IsGPUReady())
                                    {
                                        ANV_LOG_ERROR("Failed Drag/Drop Texture, Not GPU ready!");
                                    }
                                }
                            }
                        }

                        ImGui::EndDragDropTarget();
                    }

                    ImGui::SetItemTooltip("%s", textureLabel);
                    ImGui::SameLine(0.0f, -ImGui::GetItemRectSize().x);
                    ImGui::TextUnformatted(textureLabel);

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

                    ImGui::TableNextRow();
                    if (property_float2("UV Min", &sprite.uvMin.x))
                    {
                        sprite.uvMin = glm::clamp(sprite.uvMin, glm::vec2(0.0f), glm::vec2(1.0f));
                        scene->Save();
                    }

                    ImGui::TableNextRow();
                    if (property_float2("UV Max", &sprite.uvMax.x))
                    {
                        sprite.uvMax = glm::clamp(sprite.uvMax, glm::vec2(0.0f), glm::vec2(1.0f));
                        scene->Save();
                    }
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Flip X");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Checkbox("##SpriteFlipX", &sprite.flipX)) scene->Save();
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Flip Y");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Checkbox("##SpriteFlipY", &sprite.flipY)) scene->Save();
                    ImGui::EndTable();
                }
            });

        draw_component<Component::Animator2D>(
            "Animator 2D", selectedEntity, scene,
            [&](Component::Animator2D& animator)
            {
                ImGui::SeparatorText("Animation States");
                if (ImGui::BeginTable("AnimatorStates", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    static char newStateName[96]{};
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("New State");
                    ImGui::TableSetColumnIndex(1);
                    const float addWidth = 100.0f;
                    ImGui::SetNextItemWidth(std::max(50.0f, ImGui::GetContentRegionAvail().x - addWidth - ImGui::GetStyle().ItemSpacing.x));
                    ImGui::InputText("##NewAnimatorState", newStateName, sizeof(newStateName));
                    ImGui::SameLine();
                    if (ImGui::Button("Add State", ImVec2(addWidth, 0)) && newStateName[0])
                    {
                        const std::string name(newStateName);
                        const bool exists = std::any_of(animator.states.begin(), animator.states.end(),
                            [&](const auto& state) { return state.name == name; });
                        if (!exists)
                        {
                            animator.states.push_back({name, {}});
                            if (animator.defaultState.empty()) animator.defaultState = name;
                            animator.currentState.clear();
                            animator.loadedPath.clear();
                            scene->Save();
                            newStateName[0] = '\0';
                        }
                    }

                    int removeIndex = -1;
                    for (size_t i = 0; i < animator.states.size(); ++i)
                    {
                        auto& state = animator.states[i];
                        ImGui::PushID(static_cast<int>(i));
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted(state.name.c_str());
                        ImGui::TableSetColumnIndex(1);
                        const std::string clipName = state.clip.empty()
                            ? "Drop .aanim here" : std::filesystem::path(state.clip).filename().string();
                        const float removeWidth = ImGui::GetFrameHeight();
                        ImGui::Button(clipName.c_str(), ImVec2(std::max(60.0f, ImGui::GetContentRegionAvail().x - removeWidth - ImGui::GetStyle().ItemSpacing.x), 0));
                        if (ImGui::BeginDragDropTarget())
                        {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
                            {
                                if (payload->Data && payload->DataSize > 1 &&
                                    static_cast<const char*>(payload->Data)[payload->DataSize-1] == '\0')
                                {
                                    const std::filesystem::path path(static_cast<const char*>(payload->Data));
                                    if (path.extension() == ".aanim" && std::filesystem::is_regular_file(path))
                                    {
                                        state.clip = path.string();
                                        animator.loadedPath.clear();
                                        scene->Save();
                                    }
                                }
                            }
                            ImGui::EndDragDropTarget();
                        }
                        if (ImGui::IsItemHovered() && !state.clip.empty())
                            ImGui::SetTooltip("%s", state.clip.c_str());
                        ImGui::SameLine();
                        if (ImGui::SmallButton("X")) removeIndex = static_cast<int>(i);
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted("Loop");
                        ImGui::TableSetColumnIndex(1);
                        if (ImGui::Checkbox("##StateLoop", &state.loop)) scene->Save();
                        ImGui::PopID();
                    }
                    if (removeIndex >= 0)
                    {
                        const auto removedName = animator.states[removeIndex].name;
                        animator.states.erase(animator.states.begin() + removeIndex);
                        if (animator.defaultState == removedName)
                            animator.defaultState = animator.states.empty() ? "" : animator.states.front().name;
                        if (animator.currentState == removedName) animator.currentState.clear();
                        animator.loadedPath.clear();
                        scene->Save();
                    }

                    if (!animator.states.empty())
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted("Default State");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-1);
                        const std::string preview = animator.defaultState.empty() ? "Select default" : animator.defaultState;
                        if (ImGui::BeginCombo("##DefaultState", preview.c_str()))
                        {
                            for (const auto& state : animator.states)
                                if (ImGui::Selectable(state.name.c_str(), animator.defaultState == state.name))
                                {
                                    animator.defaultState = state.name;
                                    animator.currentState.clear();
                                    animator.loadedPath.clear();
                                    scene->Save();
                                }
                            ImGui::EndCombo();
                        }

                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::TextUnformatted("Current State");
                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-1);
                        if (ImGui::BeginCombo("##CurrentState", animator.currentState.empty() ? "None" : animator.currentState.c_str()))
                        {
                            for (const auto& state : animator.states)
                                if (ImGui::Selectable(state.name.c_str(), animator.currentState == state.name))
                                {
                                    animator.PlayState(state.name);
                                    scene->Save();
                                }
                            ImGui::EndCombo();
                        }
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Autoplay");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Checkbox("##AnimatorAutoplay", &animator.autoplay)) scene->Save();

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("Enabled");
                    ImGui::TableSetColumnIndex(1);
                    if (ImGui::Checkbox("##AnimatorEnabled", &animator.enabled)) scene->Save();

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextUnformatted("Frame");
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextDisabled("%zu", animator.frame);
                    ImGui::EndTable();
                }
            });

                draw_component<Component::Rigidbody2D>(
            "Rigidbody 2D",
            selectedEntity,
            scene,
            [&](Component::Rigidbody2D &rigidbody)
            {
                if (ImGui::BeginTable("Rigidbody2DProps", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 110.0f);
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                    ImGui::TableNextRow();
                    property_label("Body Type");
                    int bodyType = static_cast<int>(rigidbody.type);
                    const char *bodyTypes[] = {"Static", "Kinematic", "Dynamic"};
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
            [&](Component::BoxCollider2D &collider)
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
    Ref<Scene> &scene,
    entt::entity entity)
{
    if (!scene || entity == entt::null || !scene->Registry().valid(entity))
        return;

    ImGui::PushID("InspectorAddComponent");

    if (ImGui::Button("Add Component", ImVec2(-1.0f, 0.0f)))
        ImGui::OpenPopup("Popup");

    if (ImGui::BeginPopup("Popup"))
    {
        if (!scene->HasComponent<Component::Camera2D>(entity) &&
            ImGui::MenuItem("Camera 2D"))
        {
            scene->AddComponent<Component::Camera2D>(entity);
            if (scene->GetActiveCameraEntity() == entt::null)
                scene->SetActiveCamera(entity);
            scene->Save();
            ImGui::CloseCurrentPopup();
        }

        if (!scene->HasComponent<Component::SpriteRenderer>(entity) &&
            ImGui::MenuItem("Sprite Renderer"))
        {
            scene->AddComponent<Component::SpriteRenderer>(entity);
            ImGui::CloseCurrentPopup();
        }

        if (!scene->HasComponent<Component::Transform2D>(entity) &&
            ImGui::MenuItem("Transform 2D"))
        {
            scene->AddComponent<Component::Transform2D>(entity);
            ImGui::CloseCurrentPopup();
        }

        if (!scene->HasComponent<Component::Animator2D>(entity) &&
            ImGui::MenuItem("Animator 2D"))
        {
            scene->AddComponent<Component::Animator2D>(entity);
            scene->Save();
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
