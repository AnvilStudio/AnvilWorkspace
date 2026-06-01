#pragma once
#include "../Util/UMacros.h"
#include "../Core/Reference.h"
#include "Camera.h"
#include "Framebuffer.h"
#include <glm/glm.hpp>
#include "RenderStats.h"

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

		virtual RendererStats GetStats() = 0;

		virtual void DrawFrame() = 0;
		virtual void OnShutdown() = 0;

		virtual void BeginScene() = 0;
		virtual void DrawQuad(const glm::vec2& position, const glm::vec2& size, glm::vec4 color) = 0;
		virtual void EndScene() = 0;

		virtual void SetMainCamera(_shared<Camera2D> camera) = 0;

	protected:
		// TODO: impl API switch
		inline static GraphicsAPI s_API = GraphicsAPI::VK;
		_shared<Context> m_Context = nullptr;
		_shared<Camera2D> m_Camera;
	};
}