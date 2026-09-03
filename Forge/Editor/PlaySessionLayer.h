#pragma once

#include <Anvil.h>

#include <filesystem>

class EditorLayer;

/**
 * @brief Snapshots the edit scene when Play begins and restores it on Stop.
 */
class PlaySessionLayer : public anv::Layer
{
public:
    explicit PlaySessionLayer(EditorLayer* _editorLayer);

    void OnImGuiRender() override;
    void OnDetach() override;

private:
    void begin_play_session();
    void end_play_session();
    void discard_snapshot();

private:
    EditorLayer* m_EditorLayer = nullptr;
    bool m_WasPlaying = false;
    std::filesystem::path m_OriginalScenePath;
    std::filesystem::path m_ResolvedScenePath;
    std::filesystem::path m_SnapshotPath;
};
