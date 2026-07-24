#pragma once

#include <Anvil.h>

#include <filesystem>
#include <string>
#include <vector>

class CodeEditorPanel
{
public:
    CodeEditorPanel();
    explicit CodeEditorPanel(const std::filesystem::path& _unusedPath);
    ~CodeEditorPanel();

    void Draw(bool* _open = nullptr);
    void OnImGuiRender(bool* _open = nullptr) { Draw(_open); }

    bool OpenFile(const std::filesystem::path& _path);
    bool Save();
    void Close();

    static bool OpenInActiveEditor(const std::filesystem::path& _path);

    const std::filesystem::path& GetOpenPath() const { return m_OpenPath; }
    bool IsDirty() const { return m_Dirty; }

private:
    static int input_callback(ImGuiInputTextCallbackData* _data);
    bool is_text_file(const std::filesystem::path& _path) const;
    void set_error(std::string _message);

private:
    static CodeEditorPanel* s_ActiveEditor;

    std::filesystem::path m_OpenPath;
    std::vector<char> m_Buffer;
    bool m_Dirty = false;
    std::string m_ErrorMessage;
};
