#pragma once

#include <Anvil.h>

#include <filesystem>
#include <string>

class CodeEditorPanel
{
public:
    CodeEditorPanel();
    explicit CodeEditorPanel(const std::filesystem::path& path);
    ~CodeEditorPanel();

    void Draw(bool* open = nullptr);
    void OnImGuiRender(bool* open = nullptr)
    {
        Draw(open);
    }

    bool OpenFile(const std::filesystem::path& path);
    bool Save();
    void Close();

    static bool OpenInActiveEditor(
        const std::filesystem::path& path
    );

    const std::filesystem::path& GetOpenPath() const
    {
        return m_OpenPath;
    }

    bool IsDirty() const
    {
        return m_Dirty;
    }

private:
    bool is_text_file(
        const std::filesystem::path& path
    ) const;

    void configure_editor_for_file(
        const std::filesystem::path& path
    );

    void draw_menu_bar();
    void draw_status_bar();
    void set_error(std::string message);

private:
    static CodeEditorPanel* s_ActiveEditor;

    TextEditor m_Editor;

    std::filesystem::path m_OpenPath;

    bool m_Dirty = false;
    bool m_WasTextChanged = false;

    std::string m_ErrorMessage;
};