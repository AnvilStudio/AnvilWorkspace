#pragma once
#include "../Util/UMacros.h"
#include "../Core/Macros.h"
#include "../Core/Reference.h"
#include "RenderTarget.h"
#include "Camera.h"
#include "Framebuffer.h"
#include "RenderStats.h"
#include <glm/glm.hpp>

namespace anv
{
    class Context;
    class Texture;
    struct Render2DCreateInfo;

    enum class GraphicsAPI
    {
        VK,
        DX,
        MTL
    };

    class RenderAPI
    {
    public:
        ANV_NO_DSCRD
        static _shared<RenderAPI> Create(Render2DCreateInfo info);

        virtual ~RenderAPI() = default;

        static GraphicsAPI GetAPI() { return s_API; }
        static void SetAPI(GraphicsAPI api) { s_API = api; }

        virtual RendererStats GetStats() = 0;
        virtual void DrawFrame() = 0;
        virtual void OnShutdown() = 0;
        virtual void BeginScene() = 0;
        virtual void BeginScene(Ref<RenderTarget> renderTarget) = 0;
        virtual void DrawScene(Ref<RenderTarget> renderTarget) = 0;
        virtual void DrawQuad(
            const glm::vec2& position,
            float rotation,
            const glm::vec2& size,
            glm::vec4 color,
            Ref<Texture> texture,
            int layer) = 0;
        virtual void EndScene() = 0;
        virtual void SetMainCamera(_shared<Camera2D> camera) = 0;

    protected:
#if defined(PLATFORM_WIN64) || defined(PLATFORM_LINUX)
        inline static GraphicsAPI s_API = GraphicsAPI::VK;
#endif
#ifdef PLATFORM_APPLE
        inline static GraphicsAPI s_API = GraphicsAPI::MTL;
#endif
        _shared<Context> m_Context = nullptr;
        Ref<RenderTarget> m_CurrentTarget = nullptr;
        _shared<Camera2D> m_Camera;
    };
}
