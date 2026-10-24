#pragma once

#include <Anvil.h>
#include "CodeEditor.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <functional>
#include <array>
#include <unordered_map>
#include <string>
#include <vector>

class FileBrowser
{
public:
    using OpenFileCallback = std::function<void(const std::filesystem::path&)>;

    explicit FileBrowser(std::filesystem::path _rootDirectory);

    void Draw();
    void SetOpenFileCallback(OpenFileCallback _callback)
    {
        m_OpenFileCallback = std::move(_callback);
    }

    const std::filesystem::path& GetCurrentDirectory() const
    {
        return m_CurrentDirectory;
    }

    const std::filesystem::path& GetSelectedPath() const
    {
        return m_SelectedPath;
    }

private:
    struct Entry
    {
        std::filesystem::path path;
        bool isDirectory = false;
        std::uintmax_t size = 0;
    };

    void Refresh();
    void NavigateTo(const std::filesystem::path& _directory);
    void DrawToolbar();
    void DrawBreadcrumbs();
    void DrawEntries();
    void InitializeIcons();
    size_t IconIndex(const Entry&) const;
    anv::Ref<anv::Texture> GetThumbnail(const Entry& entry);
    bool IsImage(const Entry& entry) const;
    void DrawEntry(const Entry& _entry);
    void AcceptMoveDrop(const std::filesystem::path& destination);
    void MoveEntry(const std::filesystem::path& source, const std::filesystem::path& destination);
    void DrawCreateMenu();
    void BeginCreate(int kind, const std::filesystem::path& directory);
    void DrawCreatePopup();
    void DrawDeletePopup();

    void create_file();
    void delete_selected();

    bool MatchesSearch(const Entry& _entry) const;
    bool IsInsideRoot(const std::filesystem::path& _path) const;
    std::string GetEntryLabel(const Entry& _entry) const;
    std::string FormatFileSize(std::uintmax_t _bytes) const;

private:
    std::filesystem::path m_RootDirectory;
    std::filesystem::path m_CurrentDirectory;
    std::filesystem::path m_SelectedPath;
    std::filesystem::path m_PendingDirectory;

    enum IconKind : size_t { Folder, Generic, Image, Python, Cpp, Header, Scene, IconCount };
    std::array<anv::Ref<anv::Texture>, IconCount> m_Icons{};
    bool m_IconsInitialized = false;
    float m_TileWidth = 112.0f;
    std::unordered_map<std::string, anv::Ref<anv::Texture>> m_ThumbnailCache;
    std::vector<Entry> m_Entries;
    OpenFileCallback m_OpenFileCallback = [](const std::filesystem::path& _path)
    {
        std::string extension = _path.extension().string();
        std::transform(
            extension.begin(),
            extension.end(),
            extension.begin(),
            [](unsigned char _character)
            {
                return static_cast<char>(std::tolower(_character));
            });

        if (extension == ".ascn")
        {
            auto* app = anv::App::GetInstance();
            auto sceneManager = app ? app->GetSceneManager() : nullptr;
            if (!sceneManager)
            {
                ANV_LOG_ERROR("Unable to open scene '%s': scene manager is unavailable.", _path.string().c_str());
                return;
            }

            std::error_code error;
            const bool isEmpty = std::filesystem::exists(_path, error) &&
                !error &&
                std::filesystem::file_size(_path, error) == 0;

            if (isEmpty)
                sceneManager->CreateScene(_path);
            else
                sceneManager->OpenScene(_path);

            return;
        }

        CodeEditorPanel::OpenInActiveEditor(_path);
    };

    char m_SearchBuffer[256]{};
    char m_NewFileName[256]{};
    bool m_OpenCreatePopup = false;
    int m_CreateKind = 0; // 0 generic file, 1 folder, 2 scene, 3 Python, 4 C++, 5 header
    std::filesystem::path m_CreateDirectory;
    bool m_OpenDeletePopup = false;
    std::string m_ErrorMessage;
};