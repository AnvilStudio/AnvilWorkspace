#pragma once
#include "../Util/UMacros.h"
#include "../Core/Reference.h"
#include "Framebuffer.h"

namespace anv
{
	class Context;
	struct Render2DCreateInfo;

	enum class GraphicsAPI {
		VK,
		OGL,
		DX,
		MTL
	};

	class RenderAPI
	{
	public:
		ANV_NO_DSCRD
		static _shared<RenderAPI> Create(Render2DCreateInfo _info);

		virtual ~RenderAPI() = default;
		
		static GraphicsAPI GetAPI() { return s_API; }
		static void SetAPI(GraphicsAPI _api) { s_API = _api; };

		virtual void DrawFrame() = 0;
		virtual void OnShutdown() = 0;

	protected:
		// TODO: impl API switch
		inline static GraphicsAPI s_API = GraphicsAPI::VK;
		_shared<Context> m_Context = nullptr;
	};
}