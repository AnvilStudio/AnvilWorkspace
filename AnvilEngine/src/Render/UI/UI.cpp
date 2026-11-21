#include "UI.h"
#include <Render/Renderer.h>
#include <imgui.h>

namespace anv {

UIEngine::UIEngine()
{
    m_Root = slate::CreateElement(m_Context.pool);
    m_Panel = slate::CreateElement(m_Context.pool);
    m_Label = slate::CreateElement(m_Context.pool);

    auto& panel = m_Context.pool.slots[m_Panel.index].transform;
    panel.anchor = slate::Anchor::TOP_LEFT;
    panel.offset = {24.f, 24.f};
    panel.pivot = {0.f, 0.f};
    panel.size = {300.f, 120.f};

    auto& label = m_Context.pool.slots[m_Label.index];
    label.transform.anchor = slate::Anchor::TOP_LEFT;
    label.transform.offset = {12.f, 12.f};
    label.transform.pivot = {0.f, 0.f};
    label.transform.size = {276.f, 28.f};
    label.text = "SlateUI text rendering works!";
    label.font_size = 18.f;

    slate::AddChild(m_Root, m_Panel, m_Context.pool);
    slate::AddChild(m_Panel, m_Label, m_Context.pool);
}

void UIEngine::Render(float width, float height)
{
    if (width <= 0.f || height <= 0.f)
        return;

    slate::ResolveTree(m_Root, {{0.f, 0.f}, {width, height}}, m_Context.pool);

    // ImGui 1.92+ stores glyph metrics in ImFontBaked, not ImFont.
    // Bake the test label's characters before copying the atlas pixels.
    ImFontBaked* baked = nullptr;
    if (ImGui::GetCurrentContext())
    {
        auto* fonts = ImGui::GetIO().Fonts;
        if (fonts && !fonts->Fonts.empty())
        {
            ImFont* font = fonts->Fonts[0];
            const auto& label = m_Context.pool.slots[m_Label.index];
            baked = font->GetFontBaked(label.font_size);
            if (baked)
            {
                for (unsigned char c : label.text)
                    if (c >= 32) baked->FindGlyph(c);
                if (!m_FontAtlas)
                {
                    unsigned char* pixels = nullptr;
                    int atlasWidth = 0, atlasHeight = 0;
                    fonts->GetTexDataAsRGBA32(&pixels, &atlasWidth, &atlasHeight);
                    if (pixels && atlasWidth > 0 && atlasHeight > 0)
                        m_FontAtlas = Texture::CreateRGBA(pixels, atlasWidth, atlasHeight, "SlateUI Font Atlas");
                }
            }
        }
    }

    auto drawText = [&](const slate::ElementSlot& slot)
    {
        if (!baked || !m_FontAtlas || !m_FontAtlas->IsGPUReady() ||
            slot.text.empty() || slot.font_size <= 0.f || baked->Size <= 0.f)
            return;

        const float scale = slot.font_size / baked->Size;
        const auto& rect = slot.resolved;
        const float originX = rect.position.x;
        const float originY = rect.position.y;
        float penX = originX;
        float baselineY = originY;

        // ASCII first milestone; UTF-8 decoding and shaping come later.
        for (unsigned char c : slot.text)
        {
            if (c == '\n')
            {
                penX = originX;
                baselineY += baked->Size * scale;
                continue;
            }
            const ImFontGlyph* glyph = baked->FindGlyph(c);
            if (!glyph) continue;

            const float x0 = penX + glyph->X0 * scale;
            const float y0 = baselineY + glyph->Y0 * scale;
            const float x1 = penX + glyph->X1 * scale;
            const float y1 = baselineY + glyph->Y1 * scale;

            if (x1 > x0 && y1 > y0)
            {
                Renderer2D::DrawUIRect({
                    {x0, y0},
                    {x1 - x0, y1 - y0},
                    {1.f, 1.f, 1.f, 1.f},
                    m_FontAtlas,
                    {glyph->U0, glyph->V0},
                    {glyph->U1, glyph->V1}
                });
            }
            penX += glyph->AdvanceX * scale;
        }
    };

    auto visit = [&](auto&& self, slate::ElementHandle parent) -> void
    {
        for (auto child = m_Context.pool.slots[parent.index].relationship.first_child;
             slate::IsValid(child, m_Context.pool);
             child = m_Context.pool.slots[child.index].relationship.next_sibling)
        {
            const auto& slot = m_Context.pool.slots[child.index];
            const auto& rect = slot.resolved;
            if (child == m_Panel)
            {
                Renderer2D::DrawUIRect({
                    {rect.position.x, rect.position.y},
                    {rect.size.x, rect.size.y},
                    {0.14f, 0.22f, 0.34f, 0.94f}
                });
            }
            drawText(slot);
            self(self, child);
        }
    };
    visit(visit, m_Root);
}
} // namespace anv
