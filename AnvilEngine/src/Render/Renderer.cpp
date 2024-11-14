#include "Renderer.h"
#include <Core/App.h>
#include <Core/Profile.h>

namespace anv
{
	Renderer2D::Renderer2D(RenderCreateInfo _info)
		: m_RenderInfo(_info)
	{
		ANV_PROFILE_SCOPE();
	}

	Renderer2D::~Renderer2D()
	{
	}

	void Renderer2D::BeginFrame()
	{
		
	}

	void Renderer2D::EndFrame()
	{

	}
}