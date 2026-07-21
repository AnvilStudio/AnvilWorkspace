#pragma once

#include <Anvil.h>

#include <filesystem>
#include <string>
#include <vector>

class FileBrowser
{
public:
    explicit FileBrowser(std::filesystem::path rootDirectory);

    void Draw();

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
    void NavigateTo(const std::filesystem::path& directory);
    void DrawToolbar();
    void DrawBreadcrumbs();
    void DrawEntries();
    void DrawEntry(const Entry& entry);

    bool MatchesSearch(const Entry& entry) const;
    bool IsInsideRoot(const std::filesystem::path& path) const;
    std::string GetEntryLabel(const Entry& entry) const;
    std::string FormatFileSize(std::uintmax_t bytes) const;

private:
    std::filesystem::path m_RootDirectory;
    std::filesystem::path m_CurrentDirectory;
    std::filesystem::path m_SelectedPath;

    std::vector<Entry> m_Entries;
    char m_SearchBuffer[256]{};
    std::string m_ErrorMessage;
};
