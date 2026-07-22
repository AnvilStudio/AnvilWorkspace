#pragma once
#include <string>
#include <filesystem>

class DevNotesPanel
{
public:
    DevNotesPanel(const std::filesystem::path& savePath);

    void OnImGuiRender(bool* open=nullptr);

    void Load();
    void Save();

private:
    std::filesystem::path m_SavePath;
    std::string m_Text;
    bool m_Dirty = false;
};