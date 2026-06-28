#pragma once
#include <Anvil.h>

class Viewport
{
public:
	Viewport();
	void Draw();
private:
	anv::Ref<anv::RenderTarget> m_ViewportTarget;
	anv::_shared<anv::Camera2D> m_Camera;
};

