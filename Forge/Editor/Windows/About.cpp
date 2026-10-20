#include "About.h"

void About::Draw()
{
    if (m_DoDraw != true) return;
        

    if (ImGui::Begin("About", &m_DoDraw))
    {

        auto& info = App::GetInstance()->GetEngineInfo();
        ImGui::Text("Engine version ...");
        ImGui::SameLine();
        ImGui::TextUnformatted(info.version.c_str());
        ImGui::Separator();
        ImGui::Text("Graphics API ...");
        ImGui::SameLine();
        ImGui::TextUnformatted(info.graphicsAPI.c_str());
        ImGui::Separator();
        ImGui::Text("Authors ...");
        ImGui::Text("Cj0x7c00");
    }
    ImGui::End();
}

void About::SetOpen(bool open)
{
    m_DoDraw = open;
}