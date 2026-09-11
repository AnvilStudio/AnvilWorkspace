#include "ProjectSettings.h"

void ProjectSettings::Draw()
{
    if (!m_DoDraw) return;

    if(ImGui::Begin("Project Settings", &m_DoDraw))
    {

        ImGui::End();
    }
    
}

void ProjectSettings::SetOpen(bool open)
{
    m_DoDraw = open;
}