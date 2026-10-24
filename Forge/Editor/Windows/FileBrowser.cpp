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
    InitializeIcons();
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


void FileBrowser::InitializeIcons()
{
    if (m_IconsInitialized || !ImGui::GetCurrentContext() ||
        !ImGui::GetIO().BackendRendererUserData) return;
    const auto base = anv::App::GetInstance()->GetFS().GetExecDir()
        / "ForgeResources" / "Resources" / "Icons" / "Files";
    constexpr const char* names[] = {
        "folder.png", "file.png", "image.png", "python.png",
        "cpp.png", "header.png", "scene.png"
    };
    for (size_t i = 0; i < IconCount; ++i)
    {
        const auto path = base / names[i];
        if (!std::filesystem::exists(path)) continue;
        auto texture = anv::Texture::Create(path);
        if (texture && texture->IsGPUReady()) m_Icons[i] = texture;
    }
    m_IconsInitialized = true;
}

size_t FileBrowser::IconIndex(const Entry& e) const
{
    if (e.isDirectory) return Folder;
    const auto ext = ToLower(e.path.extension().string());
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
        ext == ".tga" || ext == ".bmp") return Image;
    if (ext == ".py") return Python;
    if (ext == ".cpp" || ext == ".cc" || ext == ".cxx") return Cpp;
    if (ext == ".h" || ext == ".hpp") return Header;
    if (ext == ".ascn") return Scene;
    return Generic;
}

void FileBrowser::DrawEntries()
{
    const float tileWidth = 112.0f;
    const float available = ImGui::GetContentRegionAvail().x;
    const int columns = std::max(1, (int)(available / (tileWidth + 8.0f)));
    ImGui::BeginChild("##FileGrid", ImVec2(0, 0));
    if (ImGui::BeginTable("##FileGridTable", columns, ImGuiTableFlags_SizingFixedFit))
    {
        int i = 0;
        for (const Entry& e : m_Entries)
        {
            if (!MatchesSearch(e)) continue;
            if (i % columns == 0) ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(i % columns);
            DrawEntry(e);
            ++i;
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();
    if (!m_PendingDirectory.empty())
    {
        const auto directory = m_PendingDirectory;
        m_PendingDirectory.clear();
        NavigateTo(directory);
    }
}

void FileBrowser::DrawEntry(const Entry& e)
{
    const std::string filename = e.path.filename().string();
    const bool selected = m_SelectedPath == e.path;
    const ImVec2 tile(112.0f, 110.0f);
    ImGui::PushID(e.path.string().c_str());
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##Tile", tile);
    const bool hover = ImGui::IsItemHovered();
    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
    {
        m_SelectedPath = e.path;
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            if (e.isDirectory) m_PendingDirectory = e.path;
            else if (m_OpenFileCallback) m_OpenFileCallback(e.path);
        }
    }

    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (selected || hover)
    {
        draw->AddRectFilled(pos, ImVec2(pos.x + tile.x, pos.y + tile.y),
            selected ? IM_COL32(65, 108, 155, 120) : IM_COL32(110, 120, 140, 35), 5.0f);
        if (selected) draw->AddRect(pos, ImVec2(pos.x + tile.x, pos.y + tile.y),
            IM_COL32(100, 180, 250, 255), 5.0f);
    }
    const ImVec2 iconMin(pos.x + 30.0f, pos.y + 9.0f);
    const ImVec2 iconMax(iconMin.x + 52.0f, iconMin.y + 52.0f);
    auto& icon = m_Icons[IconIndex(e)];
    void* texture = icon && icon->IsGPUReady() ? icon->GetImGuiTextureID() : nullptr;
    if (texture)
        draw->AddImage((ImTextureID)texture, iconMin, iconMax,
            ImVec2(0, 1), ImVec2(1, 0));
    else
    {
        const ImU32 color = e.isDirectory ? IM_COL32(206, 172, 104, 255)
                                          : IM_COL32(139, 164, 187, 255);
        draw->AddRectFilled(ImVec2(iconMin.x + 8, iconMin.y + 10),
            ImVec2(iconMax.x - 8, iconMax.y - 6), color, 3.0f);
    }
    std::string label = filename;
    while (label.size() > 1 && ImGui::CalcTextSize(label.c_str()).x > tile.x - 12)
        label.pop_back();
    if (label != filename)
    {
        while (!label.empty() && ImGui::CalcTextSize((label + "...").c_str()).x > tile.x - 12)
            label.pop_back();
        label += "...";
    }
    const float textWidth = ImGui::CalcTextSize(label.c_str()).x;
    draw->AddText(ImVec2(pos.x + (tile.x - textWidth) * 0.5f, pos.y + 77),
        ImGui::GetColorU32(ImGuiCol_Text), label.c_str());
    if (hover) ImGui::SetTooltip("%s", filename.c_str());

    if (!e.isDirectory && ImGui::BeginDragDropSource())
    {
        const std::string path = e.path.string();
        ImGui::SetDragDropPayload("ANV_ASSET_PATH", path.c_str(), path.size() + 1);
        ImGui::TextUnformatted(filename.c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginPopupContextItem())
    {
        if (e.isDirectory)
        {
            if (ImGui::MenuItem("Open")) m_PendingDirectory = e.path;
        }
        else if (ImGui::MenuItem("Open in Code Editor") && m_OpenFileCallback)
            m_OpenFileCallback(e.path);
        if (ImGui::MenuItem("Copy Path"))
            ImGui::SetClipboardText(e.path.string().c_str());
        if (ImGui::MenuItem("Delete"))
        {
            m_SelectedPath = e.path;
            m_OpenDeletePopup = true;
        }
        ImGui::EndPopup();
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
