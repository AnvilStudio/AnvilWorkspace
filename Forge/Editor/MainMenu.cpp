#include "MainMenu.h"
#include "EditorLayer.h"
#include <filesystem>
#include <system_error>
#include <string>

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
            build_project();
            ImGui::OpenPopup("Project Build");
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

    if (ImGui::BeginPopupModal("Project Build", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextWrapped("%s", m_BuildStatus.c_str());
        if (ImGui::Button("Close")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    m_PrjSettingsWindow.Draw();
    m_AboutWindow.Draw();
}
void MainMenu::build_project()
{
    namespace fs = std::filesystem;
    m_BuildFailed = false;
    m_BuildStatus.clear();
    std::error_code ec;

    auto* app = App::GetInstance();
    auto sceneManager = app->GetSceneManager();
    auto scene = sceneManager ? sceneManager->GetActive() : nullptr;
    if (scene) scene->Save();

    const fs::path assetsDir = app->GetFS().GetKeyVal("Assets");
    const fs::path projectDir = assetsDir.parent_path();
    const fs::path projectFile = projectDir / "Project.anv";
    const fs::path runtimeDir = app->GetFS().GetExecDir().parent_path() / "AnvRuntime";
#ifdef PLATFORM_WIN64
    const fs::path executable = runtimeDir / "AnvRuntime.exe";
#else
    const fs::path executable = runtimeDir / "AnvRuntime";
#endif
    auto fail = [&](const std::string& message)
    {
        m_BuildFailed = true;
        m_BuildStatus = message;
        ANV_LOG_ERROR("Project build: %s", message.c_str());
    };

    if (!fs::is_regular_file(projectFile, ec))
    {
        fail("Project.anv was not found at " + projectFile.string());
        return;
    }
    ec.clear();
    if (!fs::is_regular_file(executable, ec))
    {
        fail("AnvRuntime executable was not found at " + executable.string() +
             ". Compile the AnvRuntime target for this configuration and renderer before building.");
        return;
    }

    const fs::path destination = projectDir / "Build" / projectDir.filename();
    fs::create_directories(destination, ec);
    if (ec) { fail("Cannot create output directory: " + ec.message()); return; }

    const auto copyFile = [&](const fs::path& source, const fs::path& target) -> bool
    {
        ec.clear();
        fs::copy_file(source, target, fs::copy_options::overwrite_existing, ec);
        return !ec;
    };
    if (!copyFile(projectFile, destination / "Project.anv") ||
        !copyFile(executable, destination / executable.filename()))
    {
        fail("Failed to copy Project.anv or the runtime executable: " + ec.message());
        return;
    }

    // Copy only project-content directories; never recursively copy Build into itself.
    for (const char* folder : {"Assets", "Scripts"})
    {
        const fs::path source = projectDir / folder;
        ec.clear();
        if (!fs::is_directory(source, ec)) continue;
        fs::create_directories(destination / folder, ec);
        if (ec) { fail("Could not prepare " + std::string(folder) + ": " + ec.message()); return; }
        fs::copy(source, destination / folder,
                 fs::copy_options::recursive |
                 fs::copy_options::overwrite_existing, ec);
        if (ec) { fail("Could not package " + std::string(folder) + ": " + ec.message()); return; }
    }

    // Runtime-owned resources (including Python wrappers) are installed by its postbuild step.
    const fs::path runtimeResources = runtimeDir / "Anvil";
    ec.clear();
    if (fs::is_directory(runtimeResources, ec))
    {
        fs::create_directories(destination / "Anvil", ec);
        if (ec) { fail("Could not prepare runtime resources: " + ec.message()); return; }
        fs::copy(runtimeResources, destination / "Anvil",
                 fs::copy_options::recursive |
                 fs::copy_options::overwrite_existing, ec);
        if (ec) { fail("Could not copy runtime resources: " + ec.message()); return; }
    }
    else
    {
        fail("Runtime resources are missing: " + runtimeResources.string() +
             ". Rebuild AnvRuntime so its postbuild resource copy runs.");
        return;
    }

    // Copy companion libraries, where present (particularly Windows Python DLLs).
    for (fs::directory_iterator it(runtimeDir, ec), end; !ec && it != end; it.increment(ec))
    {
        if (!it->is_regular_file()) continue;
        const std::string ext = it->path().extension().string();
        if (ext == ".dll" || ext == ".dylib" || ext == ".so")
            if (!copyFile(it->path(), destination / it->path().filename()))
            {
                fail("Could not copy runtime library: " + ec.message());
                return;
            }
    }
    if (ec) { fail("Could not inspect runtime dependencies: " + ec.message()); return; }

    m_BuildStatus = "Build packaged successfully to:\n" + destination.string() +
        "\n\nThis is a folder build for the current platform/configuration. "
        "Third-party system dependencies (including Vulkan and Python on macOS) "
        "may still need to be installed on the target machine.";
    ANV_LOG_INFO("Project build packaged: %s", destination.string().c_str());
}
