#include "CodeEditor.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <system_error>

CodeEditorPanel* CodeEditorPanel::s_ActiveEditor = nullptr;

static TextEditor::LanguageDefinition
CreatePythonLanguageDefinition()
{
    TextEditor::LanguageDefinition language;

    language.mName = "Python";

    language.mKeywords =
    {
        "and",
        "as",
        "assert",
        "async",
        "await",
        "break",
        "class",
        "continue",
        "def",
        "del",
        "elif",
        "else",
        "except",
        "False",
        "finally",
        "for",
        "from",
        "global",
        "if",
        "import",
        "in",
        "is",
        "lambda",
        "None",
        "nonlocal",
        "not",
        "or",
        "pass",
        "raise",
        "return",
        "True",
        "try",
        "while",
        "with",
        "yield"
    };

    language.mTokenRegexStrings =
    {
        {
            R"(#[^\n]*)",
            TextEditor::PaletteIndex::Comment
        },
        {
            R"([ \t]*#[ \t]*[a-zA-Z_]+)",
            TextEditor::PaletteIndex::Preprocessor
        },
        {
            R"(L?\"(\\.|[^"])*\")",
            TextEditor::PaletteIndex::String
        },
        {
            R"(L?'(\\.|[^'])*')",
            TextEditor::PaletteIndex::String
        },
        {
            R"([+-]?[0-9]+[.]?[0-9]*)",
            TextEditor::PaletteIndex::Number
        },
        {
            R"([a-zA-Z_][a-zA-Z0-9_]*)",
            TextEditor::PaletteIndex::Identifier
        },
        {
            R"([\[\]{}!%^&*()\-=+~|<>?/;:,.])",
            TextEditor::PaletteIndex::Punctuation
        }
    };

    language.mCommentStart = "\"\"\"";
    language.mCommentEnd = "\"\"\"";
    language.mSingleLineComment = "#";

    language.mCaseSensitive = true;
    language.mAutoIndentation = true;

    return language;
}

namespace
{
    std::string to_lower(std::string value)
    {
        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(
                    std::tolower(character)
                );
            });

        return value;
    }

    std::string read_file(
        const std::filesystem::path& path
    )
    {
        std::ifstream stream(
            path,
            std::ios::binary
        );

        if (!stream)
            return {};

        return std::string(
            std::istreambuf_iterator<char>(stream),
            std::istreambuf_iterator<char>()
        );
    }
}

CodeEditorPanel::CodeEditorPanel()
{
    s_ActiveEditor = this;

    m_Editor.SetPalette(
        TextEditor::GetDarkPalette()
    );

    m_Editor.SetTabSize(4);
    m_Editor.SetShowWhitespaces(false);
    m_Editor.SetReadOnly(false);

    // Optional: prevents overwrite-mode surprises.
    m_Editor.SetHandleKeyboardInputs(true);
    m_Editor.SetHandleMouseInputs(true);
}

CodeEditorPanel::CodeEditorPanel(
    const std::filesystem::path& path
)
    : CodeEditorPanel()
{
    if (!path.empty())
        OpenFile(path);
}

CodeEditorPanel::~CodeEditorPanel()
{
    if (s_ActiveEditor == this)
        s_ActiveEditor = nullptr;
}

void CodeEditorPanel::Draw(bool* open)
{
    if (open && !*open)
        return;

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (!ImGui::Begin(
            "Code Editor",
            open,
            flags))
    {
        ImGui::End();
        return;
    }

    draw_menu_bar();

    if (!m_ErrorMessage.empty())
    {
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            ImVec4(
                1.0f,
                0.35f,
                0.35f,
                1.0f
            )
        );

        ImGui::TextWrapped(
            "%s",
            m_ErrorMessage.c_str()
        );

        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    if (m_OpenPath.empty())
    {
        const ImVec2 available =
            ImGui::GetContentRegionAvail();

        const char* message =
            "Double-click a source file in Files to open it.";

        const ImVec2 textSize =
            ImGui::CalcTextSize(message);

        ImGui::SetCursorPos({
            ImGui::GetCursorPosX() +
                std::max(
                    0.0f,
                    (available.x - textSize.x) * 0.5f
                ),

            ImGui::GetCursorPosY() +
                std::max(
                    0.0f,
                    (available.y - textSize.y) * 0.5f
                )
        });

        ImGui::TextDisabled("%s", message);

        ImGui::End();
        return;
    }

    const bool commandModifier =
        ImGui::GetIO().KeyCtrl ||
        ImGui::GetIO().KeySuper;

    if (commandModifier &&
        ImGui::IsKeyPressed(
            ImGuiKey_S,
            false))
    {
        Save();
    }

    /*
     * Reserve room for the status line so the editor does not
     * overlap it.
     */
    const float statusBarHeight =
        ImGui::GetFrameHeightWithSpacing();

    ImVec2 editorSize =
        ImGui::GetContentRegionAvail();

    editorSize.y =
        std::max(
            0.0f,
            editorSize.y - statusBarHeight
        );

    m_Editor.Render(
        "##AnvilCodeEditor",
        editorSize,
        true
    );

    /*
     * IsTextChanged() reports edits performed during the current
     * editor update.
     */
    if (m_Editor.IsTextChanged())
        m_Dirty = true;

    draw_status_bar();

    ImGui::End();
}

void CodeEditorPanel::draw_menu_bar()
{
    if (!ImGui::BeginMenuBar())
        return;

    if (ImGui::BeginMenu("File"))
    {
        const bool hasFile =
            !m_OpenPath.empty();

        if (ImGui::MenuItem(
                "Save",
#ifdef PLATFORM_APPLE
                "Cmd+S",
#else
                "Ctrl+S",
#endif
                false,
                hasFile && m_Dirty))
        {
            Save();
        }

        if (ImGui::MenuItem(
                "Close",
                nullptr,
                false,
                hasFile))
        {
            Close();
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit"))
    {
        const bool readOnly =
            m_Editor.IsReadOnly();

        if (ImGui::MenuItem(
                "Undo",
#ifdef PLATFORM_APPLE
                "Cmd+Z",
#else
                "Ctrl+Z",
#endif
                false,
                !readOnly &&
                    m_Editor.CanUndo()))
        {
            m_Editor.Undo();
        }

        if (ImGui::MenuItem(
                "Redo",
#ifdef PLATFORM_APPLE
                "Cmd+Shift+Z",
#else
                "Ctrl+Y",
#endif
                false,
                !readOnly &&
                    m_Editor.CanRedo()))
        {
            m_Editor.Redo();
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
                "Copy",
#ifdef PLATFORM_APPLE
                "Cmd+C",
#else
                "Ctrl+C",
#endif
                false,
                m_Editor.HasSelection()))
        {
            m_Editor.Copy();
        }

        if (ImGui::MenuItem(
                "Cut",
#ifdef PLATFORM_APPLE
                "Cmd+X",
#else
                "Ctrl+X",
#endif
                false,
                !readOnly &&
                    m_Editor.HasSelection()))
        {
            m_Editor.Cut();
        }

        if (ImGui::MenuItem(
                "Paste",
#ifdef PLATFORM_APPLE
                "Cmd+V",
#else
                "Ctrl+V",
#endif
                false,
                !readOnly))
        {
            m_Editor.Paste();
        }

        ImGui::Separator();

        if (ImGui::MenuItem(
                "Select All",
#ifdef PLATFORM_APPLE
                "Cmd+A"
#else
                "Ctrl+A"
#endif
            ))
        {
            m_Editor.SetSelection(
                TextEditor::Coordinates(0, 0),
                TextEditor::Coordinates(
                    m_Editor.GetTotalLines(),
                    0
                )
            );
        }

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View"))
    {
        bool showWhitespace =
            m_Editor.IsShowingWhitespaces();

        if (ImGui::MenuItem(
                "Show Whitespace",
                nullptr,
                &showWhitespace))
        {
            m_Editor.SetShowWhitespaces(
                showWhitespace
            );
        }

        ImGui::EndMenu();
    }

    ImGui::EndMenuBar();
}

void CodeEditorPanel::draw_status_bar()
{
    const TextEditor::Coordinates cursor =
        m_Editor.GetCursorPosition();

    ImGui::Separator();

    ImGui::TextDisabled(
        "Ln %d, Col %d",
        cursor.mLine + 1,
        cursor.mColumn + 1
    );

    ImGui::SameLine();

    ImGui::TextDisabled(
        " |  %d lines",
        m_Editor.GetTotalLines()
    );

    ImGui::SameLine();

    ImGui::TextDisabled(
        " |  Spaces: %d",
        m_Editor.GetTabSize()
    );

    if (m_Dirty)
    {
        ImGui::SameLine();
        ImGui::TextDisabled(" |  Modified");
    }

    const std::string path =
        m_OpenPath.string();

    if (!path.empty())
    {
        const ImVec2 size =
            ImGui::CalcTextSize(path.c_str());

        const float rightEdge =
            ImGui::GetWindowContentRegionMax().x;

        const float targetX =
            rightEdge - size.x;

        if (targetX > ImGui::GetCursorPosX())
        {
            ImGui::SameLine();
            ImGui::SetCursorPosX(targetX);
            ImGui::TextDisabled(
                "%s",
                path.c_str()
            );
        }
    }
}

bool CodeEditorPanel::OpenFile(
    const std::filesystem::path& path
)
{
    m_ErrorMessage.clear();

    if (!is_text_file(path))
    {
        set_error(
            "This file type is not supported by "
            "the code editor."
        );

        return false;
    }

    std::ifstream stream(
        path,
        std::ios::binary
    );

    if (!stream)
    {
        set_error(
            "Unable to open file: " +
            path.string()
        );

        return false;
    }

    const std::string contents{
        std::istreambuf_iterator<char>(stream),
        std::istreambuf_iterator<char>()
    };

    configure_editor_for_file(path);

    /*
     * SetText clears and replaces the editor document.
     */
    m_Editor.SetText(contents);

    m_OpenPath = path;
    m_Dirty = false;
    m_ErrorMessage.clear();

    return true;
}

bool CodeEditorPanel::Save()
{
    if (m_OpenPath.empty())
        return false;

    std::ofstream stream(
        m_OpenPath,
        std::ios::binary |
        std::ios::trunc
    );

    if (!stream)
    {
        set_error(
            "Unable to save file: " +
            m_OpenPath.string()
        );

        return false;
    }

    const std::string text =
        m_Editor.GetText();

    stream.write(
        text.data(),
        static_cast<std::streamsize>(
            text.size()
        )
    );

    if (!stream)
    {
        set_error(
            "Failed while writing file: " +
            m_OpenPath.string()
        );

        return false;
    }

    m_Dirty = false;
    m_ErrorMessage.clear();

    return true;
}

void CodeEditorPanel::Close()
{
    m_Editor.SetText("");

    m_OpenPath.clear();
    m_Dirty = false;
    m_ErrorMessage.clear();
}

bool CodeEditorPanel::OpenInActiveEditor(
    const std::filesystem::path& path
)
{
    const std::string extension =
        to_lower(path.extension().string());

    if (extension == ".ascn")
    {
        auto sceneManager =
            anv::App::GetInstance()
                ->GetSceneManager();

        if (!sceneManager)
            return false;

        const bool opened =
            static_cast<bool>(
                sceneManager->OpenScene(
                    path,
                    true
                )
            );

        if (opened &&
            s_ActiveEditor &&
            s_ActiveEditor->m_OpenPath == path)
        {
            s_ActiveEditor->Close();
        }

        return opened;
    }

    return s_ActiveEditor &&
           s_ActiveEditor->OpenFile(path);
}

void CodeEditorPanel::configure_editor_for_file(
    const std::filesystem::path& path
)
{
    const std::string extension =
        to_lower(path.extension().string());

    /*
     * C, C++, headers, Metal and CMake-like files can use the
     * C++ highlighter as a reasonable default.
     */
    if (extension == ".cpp" ||
        extension == ".cc" ||
        extension == ".cxx" ||
        extension == ".c" ||
        extension == ".h" ||
        extension == ".hpp" ||
        extension == ".hh" ||
        extension == ".inl" ||
        extension == ".metal")
    {
        m_Editor.SetLanguageDefinition(
            TextEditor::LanguageDefinition::CPlusPlus()
        );

        return;
    }

    if (extension == ".glsl" ||
        extension == ".vert" ||
        extension == ".frag")
    {
        m_Editor.SetLanguageDefinition(
            TextEditor::LanguageDefinition::GLSL()
        );

        return;
    }

    if (extension == ".py")
    {
        /*
         * Some ImGuiColorTextEdit revisions include Python(),
         * while older revisions do not.
         */
        m_Editor.SetLanguageDefinition(
            CreatePythonLanguageDefinition()
        );

        return;
    }

    if (extension == ".lua")
    {
        m_Editor.SetLanguageDefinition(
            TextEditor::LanguageDefinition::Lua()
        );

        return;
    }

    /*
     * PlainText is not present in every fork. A default-constructed
     * definition disables language-specific keyword coloring.
     */
    m_Editor.SetLanguageDefinition(
        TextEditor::LanguageDefinition()
    );
}

bool CodeEditorPanel::is_text_file(
    const std::filesystem::path& path
) const
{
    std::error_code error;

    if (!std::filesystem::is_regular_file(
            path,
            error) ||
        error)
    {
        return false;
    }

    const std::string extension =
        to_lower(path.extension().string());

    if (extension.empty())
        return true;

    return
        extension == ".txt"   ||
        extension == ".md"    ||
        extension == ".py"    ||
        extension == ".cpp"   ||
        extension == ".cc"    ||
        extension == ".cxx"   ||
        extension == ".c"     ||
        extension == ".h"     ||
        extension == ".hpp"   ||
        extension == ".hh"    ||
        extension == ".inl"   ||
        extension == ".glsl"  ||
        extension == ".vert"  ||
        extension == ".frag"  ||
        extension == ".metal" ||
        extension == ".json"  ||
        extension == ".toml"  ||
        extension == ".yaml"  ||
        extension == ".yml"   ||
        extension == ".ini"   ||
        extension == ".cfg"   ||
        extension == ".cmake" ||
        extension == ".lua"   ||
        extension == ".sh";
}

void CodeEditorPanel::set_error(
    std::string message
)
{
    m_ErrorMessage = std::move(message);

    ANV_LOG_ERROR(
        "%s",
        m_ErrorMessage.c_str()
    );
}