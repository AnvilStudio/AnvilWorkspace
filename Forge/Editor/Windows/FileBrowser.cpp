#include "FileBrowser.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <system_error>

namespace
{
    std::string ToLower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(std::tolower(character));
            });

        return value;
    }
}

FileBrowser::FileBrowser(std::filesystem::path rootDirectory)
{
    std::error_code error;

    // Forge historically passed the Assets directory as the browser root.
    // Use the project directory instead when a sibling Scripts directory exists
    // so both project content roots are visible in the same browser.
    const std::filesystem::path projectDirectory = rootDirectory.parent_path();
    const std::filesystem::path scriptsDirectory = projectDirectory / "Scripts";

    if (rootDirectory.filename() == "Assets" &&
        std::filesystem::is_directory(scriptsDirectory, error))
    {
        rootDirectory = projectDirectory;
    }

    error.clear();
    m_RootDirectory = std::filesystem::weakly_canonical(rootDirectory, error);

    if (error)
    {
        error.clear();
        m_RootDirectory = std::filesystem::absolute(rootDirectory, error);
    }

    if (error)
        m_RootDirectory = std::move(rootDirectory);

    m_CurrentDirectory = m_RootDirectory;
    Refresh();
}

void FileBrowser::Draw()
{
    ImGui::Begin("Files");

    DrawToolbar();
    DrawBreadcrumbs();
    ImGui::Separator();

    if (!m_ErrorMessage.empty())
        ImGui::TextWrapped("%s", m_ErrorMessage.c_str());

    DrawEntries();

    ImGui::End();
}

void FileBrowser::Refresh()
{
    m_Entries.clear();
    m_ErrorMessage.clear();

    std::error_code error;
    std::filesystem::directory_iterator iterator(
        m_CurrentDirectory,
        std::filesystem::directory_options::skip_permission_denied,
        error);

    if (error)
    {
        m_ErrorMessage = "Unable to read directory: " + error.message();
        return;
    }

    for (const auto& directoryEntry : iterator)
    {
        Entry entry;
        entry.path = directoryEntry.path();
        entry.isDirectory = directoryEntry.is_directory(error);

        if (error)
            error.clear();

        if (!entry.isDirectory)
        {
            entry.size = directoryEntry.file_size(error);
            if (error)
            {
                entry.size = 0;
                error.clear();
            }
        }

        m_Entries.push_back(std::move(entry));
    }

    std::sort(
        m_Entries.begin(),
        m_Entries.end(),
        [](const Entry& left, const Entry& right)
        {
            if (left.isDirectory != right.isDirectory)
                return left.isDirectory > right.isDirectory;

            return ToLower(left.path.filename().string()) <
                ToLower(right.path.filename().string());
        });
}

void FileBrowser::NavigateTo(const std::filesystem::path& directory)
{
    std::error_code error;
    const auto canonicalDirectory =
        std::filesystem::weakly_canonical(directory, error);

    if (error || !std::filesystem::is_directory(canonicalDirectory, error))
    {
        m_ErrorMessage = "Unable to open directory.";
        return;
    }

    if (!IsInsideRoot(canonicalDirectory))
        return;

    m_CurrentDirectory = canonicalDirectory;
    m_SelectedPath.clear();
    Refresh();
}

void FileBrowser::DrawToolbar()
{
    const bool canGoUp = m_CurrentDirectory != m_RootDirectory;

    ImGui::BeginDisabled(!canGoUp);
    if (ImGui::Button("Up"))
        NavigateTo(m_CurrentDirectory.parent_path());
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Refresh"))
        Refresh();

    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint(
        "##FileBrowserSearch",
        "Search project files...",
        m_SearchBuffer,
        sizeof(m_SearchBuffer));
}

void FileBrowser::DrawBreadcrumbs()
{
    std::string rootLabel = m_RootDirectory.filename().string();
    if (rootLabel.empty())
        rootLabel = "Project";

    if (ImGui::SmallButton(rootLabel.c_str()))
        NavigateTo(m_RootDirectory);

    std::error_code error;
    const auto relativePath = std::filesystem::relative(
        m_CurrentDirectory,
        m_RootDirectory,
        error);

    if (error || relativePath.empty() || relativePath == ".")
        return;

    auto accumulatedPath = m_RootDirectory;
    for (const auto& component : relativePath)
    {
        accumulatedPath /= component;
        ImGui::SameLine();
        ImGui::TextUnformatted(">");
        ImGui::SameLine();

        const std::string label = component.string();
        ImGui::PushID(accumulatedPath.string().c_str());
        if (ImGui::SmallButton(label.c_str()))
            NavigateTo(accumulatedPath);
        ImGui::PopID();
    }
}

void FileBrowser::DrawEntries()
{
    if (ImGui::BeginTable(
            "FileBrowserTable",
            3,
            ImGuiTableFlags_RowBg |
                ImGuiTableFlags_BordersInnerV |
                ImGuiTableFlags_Resizable |
                ImGuiTableFlags_ScrollY,
            ImVec2(0.0f, 0.0f)))
    {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableHeadersRow();

        for (const Entry& entry : m_Entries)
        {
            if (MatchesSearch(entry))
                DrawEntry(entry);
        }

        ImGui::EndTable();
    }

    if (!m_PendingDirectory.empty())
    {
        const auto directory = m_PendingDirectory;
        m_PendingDirectory.clear();
        NavigateTo(directory);
    }
}

void FileBrowser::DrawEntry(const Entry& entry)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    const bool selected = m_SelectedPath == entry.path;
    const std::string label = GetEntryLabel(entry);

    ImGui::PushID(entry.path.string().c_str());
    if (ImGui::Selectable(
            label.c_str(),
            selected,
            ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowDoubleClick))
    {
        m_SelectedPath = entry.path;

        if (entry.isDirectory &&
            ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            m_PendingDirectory = entry.path;
        }
    }

    if (!entry.isDirectory && ImGui::BeginDragDropSource())
    {
        const std::string path = entry.path.string();
        ImGui::SetDragDropPayload(
            "ANV_ASSET_PATH",
            path.c_str(),
            path.size() + 1);
        ImGui::TextUnformatted(entry.path.filename().string().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (entry.isDirectory && ImGui::MenuItem("Open"))
            m_PendingDirectory = entry.path;

        if (ImGui::MenuItem("Copy Path"))
            ImGui::SetClipboardText(entry.path.string().c_str());

        ImGui::EndPopup();
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(
        entry.isDirectory
            ? "Folder"
            : entry.path.extension().string().c_str());

    ImGui::TableSetColumnIndex(2);
    if (!entry.isDirectory)
    {
        const std::string size = FormatFileSize(entry.size);
        ImGui::TextUnformatted(size.c_str());
    }

    ImGui::PopID();
}

bool FileBrowser::MatchesSearch(const Entry& entry) const
{
    if (m_SearchBuffer[0] == '\0')
        return true;

    const std::string fileName =
        ToLower(entry.path.filename().string());
    const std::string searchText = ToLower(m_SearchBuffer);

    return fileName.find(searchText) != std::string::npos;
}

bool FileBrowser::IsInsideRoot(const std::filesystem::path& path) const
{
    auto rootIterator = m_RootDirectory.begin();
    auto pathIterator = path.begin();

    for (; rootIterator != m_RootDirectory.end();
         ++rootIterator, ++pathIterator)
    {
        if (pathIterator == path.end() || *rootIterator != *pathIterator)
            return false;
    }

    return true;
}

std::string FileBrowser::GetEntryLabel(const Entry& entry) const
{
    return (entry.isDirectory ? "[DIR] " : "") +
        entry.path.filename().string();
}

std::string FileBrowser::FormatFileSize(std::uintmax_t bytes) const
{
    constexpr std::uintmax_t kilobyte = 1024;
    constexpr std::uintmax_t megabyte = kilobyte * 1024;
    constexpr std::uintmax_t gigabyte = megabyte * 1024;

    char buffer[64]{};

    if (bytes >= gigabyte)
    {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "%.2f GB",
            static_cast<double>(bytes) / gigabyte);
    }
    else if (bytes >= megabyte)
    {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "%.2f MB",
            static_cast<double>(bytes) / megabyte);
    }
    else if (bytes >= kilobyte)
    {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "%.2f KB",
            static_cast<double>(bytes) / kilobyte);
    }
    else
    {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "%llu B",
            static_cast<unsigned long long>(bytes));
    }

    return buffer;
}
