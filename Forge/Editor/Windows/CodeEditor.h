#pragma once

#include <Anvil.h>

#include <filesystem>
#include <string>
#include <vector>

class CodeEditorPanel
{
public:
    CodeEditorPanel();

    void Draw(bool* _open = nullptr);
    bool OpenFile(const std::filesystem::path& _path);
    bool Save();
    void Close();

    const std::filesystem::path& GetOpenPath() const { return m_OpenPath; }
    bool IsDirty() const { return m_Dirty; }

private:
    static int input_callback(ImGuiInputTextCallbackData* _data);
    bool is_text_file(const std::filesystem::path& _path) const;
    void set_error(std::string _message);

private:
    std::filesystem::path m_OpenPath;
    std::vector<char> m_Buffer;
    bool m_Dirty = false;
    std::string m_ErrorMessage;
};
