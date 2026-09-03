#include "FileBrowser.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <system_error>

namespace
{
    std::string ToLower(std::string _value)
    {
        std::transform(
            _value.begin(),
            _value.end(),
            _value.begin(),
            [](unsigned char _character)
            {
                return static_cast<char>(std::tolower(_character));
            });
        return _value;
    }
}

FileBrowser::FileBrowser(std::filesystem::path _rootDirectory)
{
    std::error_code error;
    const std::filesystem::path projectDirectory = _rootDirectory.parent_path();
    const std::filesystem::path scriptsDirectory = projectDirectory / "Scripts";

    if (_rootDirectory.filename() == "Assets" &&
        std::filesystem::is_directory(scriptsDirectory, error))
    {
        _rootDirectory = projectDirectory;
    }

    error.clear();
    m_RootDirectory = std::filesystem::weakly_canonical(_rootDirectory, error);
    if (error)
    {
        error.clear();
        m_RootDirectory = std::filesystem::absolute(_rootDirectory, error);
    }
    if (error)
        m_RootDirectory = std::move(_rootDirectory);

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
    DrawCreatePopup();
    DrawDeletePopup();
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

    std::sort(m_Entries.begin(), m_Entries.end(), [](const Entry& _left, const Entry& _right)
    {
        if (_left.isDirectory != _right.isDirectory)
            return _left.isDirectory > _right.isDirectory;
        return ToLower(_left.path.filename().string()) <
               ToLower(_right.path.filename().string());
    });
}

void FileBrowser::NavigateTo(const std::filesystem::path& _directory)
{
    std::error_code error;
    const auto canonicalDirectory = std::filesystem::weakly_canonical(_directory, error);
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
    if (ImGui::Button("Add"))
    {
        m_NewFileName[0] = '\0';
        m_OpenCreatePopup = true;
    }

    ImGui::SameLine();
    const bool canDelete = !m_SelectedPath.empty();
    ImGui::BeginDisabled(!canDelete);
    if (ImGui::Button("Delete"))
        m_OpenDeletePopup = true;
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
            if (MatchesSearch(entry))
                DrawEntry(entry);

        ImGui::EndTable();
    }

    if (!m_PendingDirectory.empty())
    {
        const auto directory = m_PendingDirectory;
        m_PendingDirectory.clear();
        NavigateTo(directory);
    }
}

void FileBrowser::DrawEntry(const Entry& _entry)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    const bool selected = m_SelectedPath == _entry.path;
    const std::string label = GetEntryLabel(_entry);

    ImGui::PushID(_entry.path.string().c_str());
    if (ImGui::Selectable(
            label.c_str(),
            selected,
            ImGuiSelectableFlags_SpanAllColumns |
                ImGuiSelectableFlags_AllowDoubleClick))
    {
        m_SelectedPath = _entry.path;
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (_entry.isDirectory)
                m_PendingDirectory = _entry.path;
            else if (m_OpenFileCallback)
                m_OpenFileCallback(_entry.path);
        }
    }

    if (!_entry.isDirectory && ImGui::BeginDragDropSource())
    {
        const std::string path = _entry.path.string();
        ImGui::SetDragDropPayload("ANV_ASSET_PATH", path.c_str(), path.size() + 1);
        ImGui::TextUnformatted(_entry.path.filename().string().c_str());
        ImGui::EndDragDropSource();
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (_entry.isDirectory)
        {
            if (ImGui::MenuItem("Open"))
                m_PendingDirectory = _entry.path;
        }
        else if (ImGui::MenuItem("Open in Code Editor"))
        {
            if (m_OpenFileCallback)
                m_OpenFileCallback(_entry.path);
        }

        if (ImGui::MenuItem("Copy Path"))
            ImGui::SetClipboardText(_entry.path.string().c_str());
        if (ImGui::MenuItem("Delete"))
        {
            m_SelectedPath = _entry.path;
            m_OpenDeletePopup = true;
        }
        ImGui::EndPopup();
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::TextUnformatted(_entry.isDirectory
        ? "Folder"
        : _entry.path.extension().string().c_str());

    ImGui::TableSetColumnIndex(2);
    if (!_entry.isDirectory)
    {
        const std::string size = FormatFileSize(_entry.size);
        ImGui::TextUnformatted(size.c_str());
    }
    ImGui::PopID();
}

void FileBrowser::DrawCreatePopup()
{
    if (m_OpenCreatePopup)
    {
        ImGui::OpenPopup("Create File");
        m_OpenCreatePopup = false;
    }

    if (!ImGui::BeginPopupModal("Create File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::InputTextWithHint(
        "##NewFileName",
        "File name, including extension",
        m_NewFileName,
        sizeof(m_NewFileName));

    if (ImGui::Button("Create"))
    {
        create_file();
        if (m_ErrorMessage.empty())
            ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

void FileBrowser::DrawDeletePopup()
{
    if (m_OpenDeletePopup)
    {
        ImGui::OpenPopup("Delete File");
        m_OpenDeletePopup = false;
    }

    if (!ImGui::BeginPopupModal("Delete File", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        return;

    ImGui::TextWrapped(
        "Delete '%s'? This cannot be undone.",
        m_SelectedPath.filename().string().c_str());

    if (ImGui::Button("Delete"))
    {
        delete_selected();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
        ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

void FileBrowser::create_file()
{
    m_ErrorMessage.clear();
    const std::filesystem::path name(m_NewFileName);
    if (name.empty() || name.has_parent_path() || name.filename() == "." || name.filename() == "..")
    {
        m_ErrorMessage = "Enter a valid file name without directory separators.";
        return;
    }

    const std::filesystem::path path = m_CurrentDirectory / name;
    if (!IsInsideRoot(path) || std::filesystem::exists(path))
    {
        m_ErrorMessage = "That file already exists or is outside the project root.";
        return;
    }

    std::ofstream stream(path, std::ios::binary);
    if (!stream)
    {
        m_ErrorMessage = "Unable to create file: " + path.string();
        return;
    }

    m_SelectedPath = path;
    Refresh();
    if (m_OpenFileCallback)
        m_OpenFileCallback(path);
}

void FileBrowser::delete_selected()
{
    m_ErrorMessage.clear();
    if (m_SelectedPath.empty() || !IsInsideRoot(m_SelectedPath))
        return;

    std::error_code error;
    if (std::filesystem::is_directory(m_SelectedPath, error))
        std::filesystem::remove_all(m_SelectedPath, error);
    else
        std::filesystem::remove(m_SelectedPath, error);

    if (error)
    {
        m_ErrorMessage = "Unable to delete: " + error.message();
        return;
    }

    m_SelectedPath.clear();
    Refresh();
}

bool FileBrowser::MatchesSearch(const Entry& _entry) const
{
    if (m_SearchBuffer[0] == '\0')
        return true;
    return ToLower(_entry.path.filename().string()).find(ToLower(m_SearchBuffer)) != std::string::npos;
}

bool FileBrowser::IsInsideRoot(const std::filesystem::path& _path) const
{
    std::error_code error;
    const auto normalized = std::filesystem::weakly_canonical(_path, error);
    const auto& candidate = error ? _path : normalized;

    auto rootIterator = m_RootDirectory.begin();
    auto pathIterator = candidate.begin();
    for (; rootIterator != m_RootDirectory.end(); ++rootIterator, ++pathIterator)
    {
        if (pathIterator == candidate.end() || *rootIterator != *pathIterator)
            return false;
    }
    return true;
}

std::string FileBrowser::GetEntryLabel(const Entry& _entry) const
{
    return (_entry.isDirectory ? "[DIR] " : "") + _entry.path.filename().string();
}

std::string FileBrowser::FormatFileSize(std::uintmax_t _bytes) const
{
    constexpr std::uintmax_t kilobyte = 1024;
    constexpr std::uintmax_t megabyte = kilobyte * 1024;
    constexpr std::uintmax_t gigabyte = megabyte * 1024;
    char buffer[64]{};

    if (_bytes >= gigabyte)
        std::snprintf(buffer, sizeof(buffer), "%.2f GB", static_cast<double>(_bytes) / gigabyte);
    else if (_bytes >= megabyte)
        std::snprintf(buffer, sizeof(buffer), "%.2f MB", static_cast<double>(_bytes) / megabyte);
    else if (_bytes >= kilobyte)
        std::snprintf(buffer, sizeof(buffer), "%.2f KB", static_cast<double>(_bytes) / kilobyte);
    else
        std::snprintf(buffer, sizeof(buffer), "%llu B", static_cast<unsigned long long>(_bytes));
    return buffer;
}
