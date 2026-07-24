#pragma once

#include <Anvil.h>

#include <filesystem>
#include <functional>
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
    void DrawEntry(const Entry& _entry);
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

    std::vector<Entry> m_Entries;
    OpenFileCallback m_OpenFileCallback;

    char m_SearchBuffer[256]{};
    char m_NewFileName[256]{};
    bool m_OpenCreatePopup = false;
    bool m_OpenDeletePopup = false;
    std::string m_ErrorMessage;
};
