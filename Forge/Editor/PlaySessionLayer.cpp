#include "PlaySessionLayer.h"

#include "EditorLayer.h"

#include <system_error>

using namespace anv;

PlaySessionLayer::PlaySessionLayer(EditorLayer* _editorLayer)
    : Layer("Play Session Layer"),
      m_EditorLayer(_editorLayer)
{
}

void PlaySessionLayer::OnImGuiRender()
{
    if (!m_EditorLayer)
        return;

    const bool isPlaying =
        m_EditorLayer->GetSceneState() == EditorLayer::SceneState::Play;

    if (isPlaying && !m_WasPlaying)
        begin_play_session();
    else if (!isPlaying && m_WasPlaying)
        end_play_session();

    m_WasPlaying = isPlaying;
}

void PlaySessionLayer::OnDetach()
{
    if (m_WasPlaying)
        end_play_session();
    else
        discard_snapshot();
}

void PlaySessionLayer::begin_play_session()
{
    auto sceneManager = App::GetInstance()->GetSceneManager();
    Ref<Scene> scene = sceneManager ? sceneManager->GetActive() : nullptr;
    if (!scene)
        return;

    m_OriginalScenePath = scene->GetPath();
    if (m_OriginalScenePath.empty())
    {
        ANV_LOG_ERROR("Cannot start a restorable Play session: the active scene has no path.");
        return;
    }

    scene->Save();

    std::error_code error;
    const std::filesystem::path tempDirectory =
        std::filesystem::temp_directory_path(error);

    if (error)
    {
        ANV_LOG_ERROR("Unable to locate the temporary directory for the Play snapshot: %s", error.message().c_str());
        return;
    }

    m_SnapshotPath = tempDirectory /
        ("anvil-play-" + scene->GetUUID().uuid + ".scene-backup");

    std::filesystem::copy_file(
        m_OriginalScenePath,
        m_SnapshotPath,
        std::filesystem::copy_options::overwrite_existing,
        error);

    if (error)
    {
        ANV_LOG_ERROR(
            "Unable to snapshot scene '%s' before Play: %s",
            m_OriginalScenePath.string().c_str(),
            error.message().c_str());
        m_SnapshotPath.clear();
        return;
    }

    ANV_LOG_INFO("Created Play snapshot for '%s'.", m_OriginalScenePath.string().c_str());
}

void PlaySessionLayer::end_play_session()
{
    auto sceneManager = App::GetInstance()->GetSceneManager();
    Ref<Scene> scene = sceneManager ? sceneManager->GetActive() : nullptr;

    if (scene)
        scene->SetScriptExecutionEnabled(false);

    if (m_SnapshotPath.empty() || m_OriginalScenePath.empty())
    {
        discard_snapshot();
        return;
    }

    std::error_code error;
    std::filesystem::copy_file(
        m_SnapshotPath,
        m_OriginalScenePath,
        std::filesystem::copy_options::overwrite_existing,
        error);

    if (error)
    {
        ANV_LOG_ERROR(
            "Unable to restore scene '%s' after Play: %s",
            m_OriginalScenePath.string().c_str(),
            error.message().c_str());
        discard_snapshot();
        return;
    }

    Ref<Scene> restored = sceneManager ? sceneManager->ReloadActive() : nullptr;
    if (!restored)
    {
        ANV_LOG_ERROR("The Play snapshot was restored on disk, but the active scene could not be reloaded.");
    }
    else
    {
        restored->SetScriptExecutionEnabled(false);
        m_EditorLayer->ClearSelection();
        ANV_LOG_INFO("Restored edit scene after Play.");
    }

    discard_snapshot();
}

void PlaySessionLayer::discard_snapshot()
{
    if (!m_SnapshotPath.empty())
    {
        std::error_code error;
        std::filesystem::remove(m_SnapshotPath, error);
    }

    m_OriginalScenePath.clear();
    m_SnapshotPath.clear();
}
