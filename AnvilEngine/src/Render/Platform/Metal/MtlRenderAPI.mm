#include "MtlRenderAPI.h"

#include <Render/Renderer.h>
#include <Core/Window.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_metal.h>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace anv
{
    static NSString* TriangleShaderSource = @R"(
        #include <metal_stdlib>
        using namespace metal;

        struct VertexOutput
        {
            float4 position [[position]];
            float4 color;
        };

        vertex VertexOutput triangle_vertex(
            uint vertexID [[vertex_id]]
        )
        {
            constexpr float2 positions[] =
            {
                float2( 0.0,  0.65),
                float2(-0.65, -0.65),
                float2( 0.65, -0.65)
            };

            constexpr float4 colors[] =
            {
                float4(1.0, 0.15, 0.10, 1.0),
                float4(0.10, 1.0, 0.25, 1.0),
                float4(0.10, 0.35, 1.0, 1.0)
            };

            VertexOutput output;
            output.position =
                float4(positions[vertexID], 0.0, 1.0);
            output.color = colors[vertexID];

            return output;
        }

        fragment float4 triangle_fragment(
            VertexOutput input [[stage_in]]
        )
        {
            return input.color;
        }
    )";

    MetalRenderAPI::MetalRenderAPI(Render2DCreateInfo info)
    {
        m_Context = info.pTarget->GetContext();

        m_MetalContext =
            std::dynamic_pointer_cast<MetalContext>(m_Context);

        if (!m_MetalContext)
        {
            ANV_LOG_FATAL(
                "MetalRenderAPI received a non-Metal context!"
            );
            return;
        }

        create_triangle_pipeline();
        initialize_imgui();
    }

    MetalRenderAPI::~MetalRenderAPI()
    {
        OnShutdown();
    }

    void MetalRenderAPI::create_triangle_pipeline()
    {
        id<MTLDevice> device =
            (__bridge id<MTLDevice>)
                m_MetalContext->GetDevice();

        NSError* error = nil;

        id<MTLLibrary> library =
            [device newLibraryWithSource:TriangleShaderSource
                                options:nil
                                  error:&error];

        if (!library)
        {
            ANV_LOG_FATAL(
                "Metal shader compilation failed: %s",
                error.localizedDescription.UTF8String
            );
            return;
        }

        id<MTLFunction> vertexFunction =
            [library newFunctionWithName:@"triangle_vertex"];

        id<MTLFunction> fragmentFunction =
            [library newFunctionWithName:@"triangle_fragment"];

        if (!vertexFunction || !fragmentFunction)
        {
            ANV_LOG_FATAL(
                "Failed to retrieve Metal triangle shader functions!"
            );
            return;
        }

        MTLRenderPipelineDescriptor* descriptor =
            [[MTLRenderPipelineDescriptor alloc] init];

        descriptor.label = @"Anvil Triangle Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;

        descriptor.colorAttachments[0].pixelFormat =
            MTLPixelFormatBGRA8Unorm;

        id<MTLRenderPipelineState> pipeline =
            [device
                newRenderPipelineStateWithDescriptor:descriptor
                error:&error];

        if (!pipeline)
        {
            ANV_LOG_FATAL(
                "Metal pipeline creation failed: %s",
                error.localizedDescription.UTF8String
            );
            return;
        }

        m_TrianglePipeline =
            (__bridge_retained void*)pipeline;

        ANV_LOG_INFO("Created Metal triangle pipeline");
    }

    void MetalRenderAPI::initialize_imgui()
    {

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        // Leave multi-viewport disabled for now.
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        GLFWwindow* window =
            m_MetalContext->GetWindowHandle();

        id<MTLDevice> device =
            (__bridge id<MTLDevice>)
                m_MetalContext->GetDevice();

        if (!ImGui_ImplGlfw_InitForOther(window, true))
        {
            ANV_LOG_FATAL(
                "Failed to initialize ImGui GLFW backend!"
            );
            return;
        }

        if (!ImGui_ImplMetal_Init(device))
        {
            ImGui_ImplGlfw_Shutdown();

            ANV_LOG_FATAL(
                "Failed to initialize ImGui Metal backend!"
            );
            return;
        }

        ANV_LOG_INFO("Initialized ImGui Metal backend");
    }

    RendererStats MetalRenderAPI::GetStats()
    {
        return m_Stats;
    }

void MetalRenderAPI::DrawFrame()
{
    if (!m_CurrentDrawable ||
        !m_CurrentCommandBuffer ||
        !m_CurrentEncoder)
    {
        return;
    }

    id<CAMetalDrawable> drawable =
        (__bridge id<CAMetalDrawable>)
            m_CurrentDrawable;

    id<MTLCommandBuffer> commandBuffer =
        (__bridge id<MTLCommandBuffer>)
            m_CurrentCommandBuffer;

    id<MTLRenderCommandEncoder> encoder =
        (__bridge id<MTLRenderCommandEncoder>)
            m_CurrentEncoder;

    id<MTLRenderPipelineState> pipeline =
        (__bridge id<MTLRenderPipelineState>)
            m_TrianglePipeline;

    encoder.label = @"Anvil Main Encoder";

    // Draw the test triangle.
    [encoder setRenderPipelineState:pipeline];

    [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                vertexStart:0
                vertexCount:3];

    // Draw ImGui into the same active render encoder.
    ImGui_ImplMetal_RenderDrawData(
        ImGui::GetDrawData(),
        commandBuffer,
        encoder
    );

    [encoder endEncoding];

    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];

    m_Stats.DrawCalls = 2;

    CFBridgingRelease(m_CurrentEncoder);
    CFBridgingRelease(m_CurrentRenderPass);
    CFBridgingRelease(m_CurrentCommandBuffer);
    CFBridgingRelease(m_CurrentDrawable);

    m_CurrentEncoder = nullptr;
    m_CurrentRenderPass = nullptr;
    m_CurrentCommandBuffer = nullptr;
    m_CurrentDrawable = nullptr;
}

    void MetalRenderAPI::OnShutdown()
    {
        if (m_TrianglePipeline)
        {
            CFBridgingRelease(m_TrianglePipeline);
            m_TrianglePipeline = nullptr;
        }

        // m_CurrentTarget.reset();
        // m_Camera.reset();
        // m_MetalContext.reset();
        // m_Context.reset();
    }

    void MetalRenderAPI::BeginScene()
    {
        if (!m_MetalContext || !m_TrianglePipeline)
            return;

        m_MetalContext->CreateSwapchain();

        CAMetalLayer* layer =
            (__bridge CAMetalLayer*)m_MetalContext->GetLayer();

        id<CAMetalDrawable> drawable = [layer nextDrawable];

        if (!drawable)
            return;

        id<MTLCommandQueue> commandQueue =
            (__bridge id<MTLCommandQueue>)
                m_MetalContext->GetCommandQueue();

        id<MTLCommandBuffer> commandBuffer =
            [commandQueue commandBuffer];

        MTLRenderPassDescriptor* renderPass =
            [MTLRenderPassDescriptor renderPassDescriptor];

        renderPass.colorAttachments[0].texture =
            drawable.texture;

        renderPass.colorAttachments[0].loadAction =
            MTLLoadActionClear;

        renderPass.colorAttachments[0].storeAction =
            MTLStoreActionStore;

        renderPass.colorAttachments[0].clearColor =
            MTLClearColorMake(0.025, 0.025, 0.035, 1.0);

        id<MTLRenderCommandEncoder> encoder =
            [commandBuffer
                renderCommandEncoderWithDescriptor:renderPass];

        m_CurrentDrawable =
            (__bridge_retained void*)drawable;

        m_CurrentCommandBuffer =
            (__bridge_retained void*)commandBuffer;

        m_CurrentRenderPass =
            (__bridge_retained void*)renderPass;

        m_CurrentEncoder =
            (__bridge_retained void*)encoder;

        // Backend frame setup must happen before ImGui::NewFrame().
        ImGui_ImplMetal_NewFrame(renderPass);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void MetalRenderAPI::BeginScene(
        Ref<RenderTarget> renderTarget
    )
    {
        m_CurrentTarget = renderTarget;
    }

    void MetalRenderAPI::DrawScene(
        Ref<RenderTarget> renderTarget
    )
    {
        (void)renderTarget;
    }

    void MetalRenderAPI::DrawQuad(
        const glm::vec2& position,
        float rotation,
        const glm::vec2& size,
        glm::vec4 color,
        int layer
    )
    {
        (void)position;
        (void)rotation;
        (void)size;
        (void)color;
        (void)layer;
    }

    void MetalRenderAPI::EndScene()
    {
        ImGui::Render();
    }

    void MetalRenderAPI::SetMainCamera(
        _shared<Camera2D> camera
    )
    {
        m_Camera = camera;
    }
}