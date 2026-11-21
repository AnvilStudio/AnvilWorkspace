#include "UI.h"
#include <Render/Renderer.h>
namespace anv {
UIEngine::UIEngine() {
 m_Root=slate::CreateElement(m_Context.pool);
 m_Panel=slate::CreateElement(m_Context.pool);
 auto& t=m_Context.pool.slots[m_Panel.index].transform;
 t.anchor=slate::Anchor::TOP_LEFT;
 t.offset={24.f,24.f};
 t.pivot={0.f,0.f};
 t.size={300.f,120.f};
 slate::AddChild(m_Root,m_Panel,m_Context.pool);
}
void UIEngine::Render(float width,float height) {
 if(width<=0.f||height<=0.f) return;
 slate::ResolveTree(m_Root,{{0.f,0.f},{width,height}},m_Context.pool);
 auto visit=[&](auto&& self,slate::ElementHandle parent)->void {
   for(auto child=m_Context.pool.slots[parent.index].relationship.first_child;
       slate::IsValid(child,m_Context.pool);
       child=m_Context.pool.slots[child.index].relationship.next_sibling) {
     const auto& r=m_Context.pool.slots[child.index].resolved;
     Renderer2D::DrawUIRect({{r.position.x,r.position.y},{r.size.x,r.size.y},
                            {0.14f,0.22f,0.34f,0.94f}});
     self(self,child);
   }
 };
 visit(visit,m_Root);
}
} // namespace anv
