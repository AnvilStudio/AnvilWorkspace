#include "CodeEditor.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <system_error>

CodeEditorPanel* CodeEditorPanel::s_ActiveEditor = nullptr;

namespace
{
    std::string to_lower(std::string _value)
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

CodeEditorPanel::CodeEditorPanel()
{
    m_Buffer.resize(4096, '\0');
    s_ActiveEditor = this;
}

CodeEditorPanel::CodeEditorPanel(const std::filesystem::path& _unusedPath)
    : CodeEditorPanel()
{
    (void)_unusedPath;
}

CodeEditorPanel::~CodeEditorPanel()
{
    if (s_ActiveEditor == this)
        s_ActiveEditor = nullptr;
}

void CodeEditorPanel::Draw(bool* _open)
{
    if (_open && !*_open)
        return;

    if (!ImGui::Begin("Code Editor", _open))
    {
        ImGui::End();
        return;
    }

    const std::string title = m_OpenPath.empty()
        ? "No file open"
        : m_OpenPath.filename().string() + (m_Dirty ? " *" : "");
    ImGui::TextUnformatted(title.c_str());

    if (!m_OpenPath.empty())
    {
        ImGui::SameLine();
        ImGui::BeginDisabled(!m_Dirty);
        if (ImGui::Button("Save"))
            Save();
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Close"))
            Close();
    }

    if (!m_ErrorMessage.empty())
    {
        ImGui::TextWrapped("%s", m_ErrorMessage.c_str());
        ImGui::Separator();
    }

    if (m_OpenPath.empty())
    {
        ImGui::TextDisabled("Double-click a text file in Files to open it.");
        ImGui::End();
        return;
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S))
        Save();

    if (ImGui::InputTextMultiline(
            "##CodeEditorText",
            m_Buffer.data(),
            m_Buffer.size(),
            ImGui::GetContentRegionAvail(),
            ImGuiInputTextFlags_AllowTabInput |
                ImGuiInputTextFlags_CallbackResize,
            input_callback,
            this))
    {
        m_Dirty = true;
    }

    ImGui::End();
}

bool CodeEditorPanel::OpenFile(const std::filesystem::path& _path)
{
    m_ErrorMessage.clear();
    if (!is_text_file(_path))
    {
        set_error("This file type is not supported by the text editor.");
        return false;
    }

    std::ifstream stream(_path, std::ios::binary);
    if (!stream)
    {
        set_error("Unable to open file: " + _path.string());
        return false;
    }

    std::string contents{
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()};

    m_Buffer.assign(contents.begin(), contents.end());
    m_Buffer.push_back('\0');
    m_Buffer.resize(std::max<std::size_t>(m_Buffer.size() + 1024, 4096), '\0');
    m_OpenPath = _path;
    m_Dirty = false;
    return true;
}

bool CodeEditorPanel::Save()
{
    if (m_OpenPath.empty())
        return false;

    std::ofstream stream(m_OpenPath, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        set_error("Unable to save file: " + m_OpenPath.string());
        return false;
    }

    stream.write(
        m_Buffer.data(),
        static_cast<std::streamsize>(std::char_traits<char>::length(m_Buffer.data())));
    if (!stream)
    {
        set_error("Failed while writing file: " + m_OpenPath.string());
        return false;
    }

    m_Dirty = false;
    m_ErrorMessage.clear();
    return true;
}

void CodeEditorPanel::Close()
{
    m_OpenPath.clear();
    m_Buffer.assign(4096, '\0');
    m_Dirty = false;
    m_ErrorMessage.clear();
}

bool CodeEditorPanel::OpenInActiveEditor(const std::filesystem::path& _path)
{
    return s_ActiveEditor && s_ActiveEditor->OpenFile(_path);
}

int CodeEditorPanel::input_callback(ImGuiInputTextCallbackData* _data)
{
    if (_data->EventFlag != ImGuiInputTextFlags_CallbackResize)
        return 0;

    auto* editor = static_cast<CodeEditorPanel*>(_data->UserData);
    editor->m_Buffer.resize(static_cast<std::size_t>(_data->BufTextLen) + 1024, '\0');
    _data->Buf = editor->m_Buffer.data();
    return 0;
}

bool CodeEditorPanel::is_text_file(const std::filesystem::path& _path) const
{
    if (!std::filesystem::is_regular_file(_path))
        return false;

    const std::string extension = to_lower(_path.extension().string());
    if (extension.empty())
        return true;

    return extension == ".txt" || extension == ".md" || extension == ".py" ||
           extension == ".cpp" || extension == ".c" || extension == ".h" ||
           extension == ".hpp" || extension == ".inl" || extension == ".glsl" ||
           extension == ".vert" || extension == ".frag" || extension == ".metal" ||
           extension == ".json" || extension == ".toml" || extension == ".yaml" ||
           extension == ".yml" || extension == ".ini" || extension == ".cfg" ||
           extension == ".cmake" || extension == ".lua" || extension == ".sh";
}

void CodeEditorPanel::set_error(std::string _message)
{
    m_ErrorMessage = std::move(_message);
    ANV_LOG_ERROR("%s", m_ErrorMessage.c_str());
}
