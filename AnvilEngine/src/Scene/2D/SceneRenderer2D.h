#pragma once

#include <Scene/Scene.h>
#include <Util/UMacros.h>
#include <Core/Reference.h>
#include <Render/RenderData.h>

namespace anv
{
	class SceneRenderer2D
	{
	public:
		static void Render(Ref<Scene> _scene);
	};
}
