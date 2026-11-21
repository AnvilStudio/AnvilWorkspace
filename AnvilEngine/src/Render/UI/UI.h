#pragma once
#include <SlateUI/slate.h>
namespace anv {
class UIEngine {
public:
 UIEngine();
 slate::Context& GetContext() { return m_Context; }
 slate::ElementHandle GetRoot() const { return m_Root; }
 slate::ElementHandle GetPanel() const { return m_Panel; }
 void Render(float width,float height);
private:
 slate::Context m_Context{};
 slate::ElementHandle m_Root=slate::null_handle;
 slate::ElementHandle m_Panel=slate::null_handle;
};
}
