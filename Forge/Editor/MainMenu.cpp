#include "MainMenu.h"
#include "EditorLayer.h"

void MainMenu::Draw()
{
    Windows& windows = EditorLayer::GetInstance()->GetWindowSettings();
    if (!ImGui::BeginMainMenuBar())
        return;

    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("About"))
        {
            m_AboutWindow.SetOpen(true);
        }


        ImGui::Separator();

        if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
        {
            auto sceneManager = App::GetInstance()->GetSceneManager();
            auto scene = sceneManager ? sceneManager->GetActive() : nullptr;
            if (scene)
                scene->Save();
        }

        if (ImGui::MenuItem("Reload Scene"))
            App::GetInstance()->GetSceneManager()->ReloadActive();

        ImGui::Separator();

        if (ImGui::MenuItem("Build"))
        {
            //anv::ProjectBuilder::Build();
            ANV_LOG_DEBUG("Hello!")
        }

        ImGui::Separator();

        if (ImGui::MenuItem("Project Settings"))
        {
            m_PrjSettingsWindow.SetOpen(true);
        }


        ImGui::Separator();

        if (ImGui::MenuItem("Exit"))
            App::GetInstance()->Close();

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::BeginMenu("Scene"))
        {
            ImGui::MenuItem("Scene Viewport", nullptr, &windows.showViewport);
            ImGui::MenuItem("Game Viewport", nullptr, &windows.showGameViewport);
            ImGui::MenuItem("Scene Hierarchy", nullptr, &windows.showHierarchy);
            ImGui::MenuItem("Inspector", nullptr, &windows.showInspector);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools"))
        {
            ImGui::MenuItem("Files", nullptr, &windows.showFiles);
            ImGui::MenuItem("Asset Registry", nullptr, &windows.showAssetRegistry);
            ImGui::MenuItem("Spritesheet Editor", nullptr, &windows.showSpritesheetEditor);
            ImGui::MenuItem("Sprite Animation Editor", nullptr, &windows.showSpriteAnimationEditor);
            ImGui::MenuItem("Code Editor", nullptr, &windows.showCodeEditor);
            ImGui::MenuItem("Console", nullptr, &windows.showConsole);
            ImGui::MenuItem("Performance Stats", nullptr, &windows.showStats);
            ImGui::EndMenu();
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Show All Windows"))
            windows.ShowAll();
        if (ImGui::MenuItem("Restore Default Windows"))
            windows.RestoreDefaults();
        ImGui::EndMenu();
    }

    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - 80.0f) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.55f, 0.27f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.35f, 0.18f, 1.0f));

    if (EditorLayer::GetSceneState() == EditorLayer::SceneState::Edit)
    {
        if (ImGui::Button("Play"))
            EditorLayer::GetInstance()->SetSceneState(EditorLayer::SceneState::Play);
    }
    else
    {
        if (ImGui::Button("Stop"))
            EditorLayer::GetInstance()->SetSceneState(EditorLayer::SceneState::Edit);
    }

    ImGui::PopStyleColor(3);
    ImGui::EndMainMenuBar();

    m_PrjSettingsWindow.Draw();
    m_AboutWindow.Draw();
}