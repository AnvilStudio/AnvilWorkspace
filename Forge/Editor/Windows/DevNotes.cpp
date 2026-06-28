#include "DevNotes.h"
#include "DevNotes.h"
#include <Anvil.h>
#include <fstream>

DevNotesPanel::DevNotesPanel(const std::filesystem::path& savePath)
    : m_SavePath(savePath)
{
    Load();
}

void DevNotesPanel::OnImGuiRender()
{
    ImGui::Begin("Dev Notes");

    if (ImGui::Button("Save"))
        Save();

    ImGui::SameLine();

    if (m_Dirty)
        ImGui::TextUnformatted("* unsaved");
    else
        ImGui::TextUnformatted("saved");

    ImGui::Separator();

    ImGuiInputTextFlags flags =
        ImGuiInputTextFlags_AllowTabInput;

    if (ImGui::InputTextMultiline(
        "##DevNotesEditor",
        &m_Text,
        ImVec2(-FLT_MIN, -FLT_MIN),
        flags))
    {
        m_Dirty = true;
    }

    ImGui::End();
}

void DevNotesPanel::Load()
{
    if (!std::filesystem::exists(m_SavePath))
    {
        m_Text.clear();
        m_Dirty = false;
        return;
    }

    toml::table data = toml::parse_file(m_SavePath.string());

    m_Text = data["DevNotes"]["Text"].value_or("");
    m_Dirty = false;
}

void DevNotesPanel::Save()
{
    std::filesystem::create_directories(m_SavePath.parent_path());

    toml::table data;
    data.insert_or_assign("DevNotes", toml::table{
        { "Text", m_Text }
        });

    std::ofstream out(m_SavePath);
    out << data;

    m_Dirty = false;
}