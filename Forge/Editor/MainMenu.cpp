#include "MainMenu.h"
#include "EditorLayer.h"

void MainMenu::Draw()
{
    static Windows& windows = EditorLayer::GetInstance()->GetWindowSettings();
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
        ImGui::MenuItem("Asset Registry", nullptr, windows.showAssetRegistry);
        ImGui::MenuItem("Dev Notes", nullptr, windows.showCodeEditor);
        ImGui::MenuItem("Stats", nullptr, windows.showStats);
        ImGui::MenuItem("Console", nullptr, windows.showConsole);
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