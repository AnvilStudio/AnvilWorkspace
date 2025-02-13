#pragma once
#include "../Util/UMacros.h"

namespace anv
{

	struct RenderAPICreateInfo
	{

	};


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
		static _shared<RenderAPI> Create(RenderAPICreateInfo _info);

		static GraphicsAPI GetAPI() { return s_API; }
		static void SetAPI(GraphicsAPI _api) { s_API = _api; };

		virtual ~RenderAPI() {};

	private:
		// TODO: impl API switch
		inline static GraphicsAPI s_API = GraphicsAPI::VK;
	};
}