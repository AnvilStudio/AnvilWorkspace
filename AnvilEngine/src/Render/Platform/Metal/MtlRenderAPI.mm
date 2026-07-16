#include "MtlRenderAPI.h"

#include <Render/Renderer.h>
#include <Core/Window.h>
#include <Util/Time/Time.h>

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_metal.h>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include "MtlRenderTarget.h"

namespace anv
{
    struct MetalSpriteUniforms
    {
        glm::mat4 ViewProjection{ 1.0f };
        glm::mat4 Model{ 1.0f };
        glm::vec4 Color{ 1.0f };
    };

    static NSString* SpriteShaderSource = @R"(
        #include <metal_stdlib>
        using namespace metal;

        struct SpriteUniforms
        {
            float4x4 viewProjection;
            float4x4 model;
            float4 color;
        };

        struct VertexOutput
        {
            float4 position [[position]];
            float4 color;
        };

        vertex VertexOutput sprite_vertex(
            uint vertexID [[vertex_id]],
            constant SpriteUniforms& uniforms [[buffer(0)]]
        )
        {
            constexpr float2 positions[] =
            {
                float2(-0.5, -0.5),
                float2( 0.5, -0.5),
                float2( 0.5,  0.5),
                float2( 0.5,  0.5),
                float2(-0.5,  0.5),
                float2(-0.5, -0.5)
            };

            VertexOutput output;
            float4 localPosition =
                float4(positions[vertexID], 0.0, 1.0);

            output.position =
                uniforms.viewProjection *
                uniforms.model *
                localPosition;
            output.color = uniforms.color;

            return output;
        }

        fragment float4 sprite_fragment(
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

        create_sprite_pipeline();
        initialize_imgui();
    }

    MetalRenderAPI::~MetalRenderAPI()
    {
        OnShutdown();
    }

    void MetalRenderAPI::create_sprite_pipeline()
    {
        id<MTLDevice> device =
            (__bridge id<MTLDevice>)
                m_MetalContext->GetDevice();

        NSError* error = nil;

        id<MTLLibrary> library =
            [device newLibraryWithSource:SpriteShaderSource
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
            [library newFunctionWithName:@"sprite_vertex"];

        id<MTLFunction> fragmentFunction =
            [library newFunctionWithName:@"sprite_fragment"];

        if (!vertexFunction || !fragmentFunction)
        {
            ANV_LOG_FATAL(
                "Failed to retrieve Metal sprite shader functions!"
            );
            return;
        }

        MTLRenderPipelineDescriptor* descriptor =
            [[MTLRenderPipelineDescriptor alloc] init];

        descriptor.label = @"Anvil Sprite Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;

        descriptor.colorAttachments[0].pixelFormat =
            MTLPixelFormatBGRA8Unorm;

        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].sourceRGBBlendFactor =
            MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor =
            MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor =
            MTLBlendFactorOne;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor =
            MTLBlendFactorOneMinusSourceAlpha;

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

        m_SpritePipeline =
            (__bridge_retained void*)pipeline;

        ANV_LOG_INFO("Created Metal sprite pipeline");
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

        m_ImGuiGlfwInitialized = true;

        if (!ImGui_ImplMetal_Init(device))
        {
            ImGui_ImplGlfw_Shutdown();
            m_ImGuiGlfwInitialized = false;

            ANV_LOG_FATAL(
                "Failed to initialize ImGui Metal backend!"
            );
            return;
        }

        m_ImGuiMetalInitialized = true;

        ANV_LOG_INFO("Initialized ImGui Metal backend");
    }

    void MetalRenderAPI::shutdown_imgui()
    {
        if (m_ImGuiMetalInitialized)
        {
            ImGui_ImplMetal_Shutdown();
            m_ImGuiMetalInitialized = false;
        }

        if (m_ImGuiGlfwInitialized)
        {
            ImGui_ImplGlfw_Shutdown();
            m_ImGuiGlfwInitialized = false;
        }

        if (ImGui::GetCurrentContext())
            ImGui::DestroyContext();
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
            m_SpritePipeline;

    encoder.label = @"Anvil Main Encoder";

    std::stable_sort(
        m_QuadQueue.begin(),
        m_QuadQueue.end(),
        [](const QuadSubmission& _left, const QuadSubmission& _right)
        {
            return _left.Layer < _right.Layer;
        });

    [encoder setRenderPipelineState:pipeline];

    for (const QuadSubmission& quad : m_QuadQueue)
    {
        MetalSpriteUniforms uniforms{};

        uniforms.ViewProjection =
            m_Camera->GetCameraUBO().ViewProjection;

        uniforms.Model =
            glm::translate(
                glm::mat4(1.0f),
                glm::vec3(quad.Position, 0.0f))
            *
            glm::rotate(
                glm::mat4(1.0f),
                glm::radians(quad.Rotaion),
                glm::vec3(0.0f, 0.0f, 1.0f))
            *
            glm::scale(
                glm::mat4(1.0f),
                glm::vec3(quad.Size, 1.0f));

        uniforms.Color = quad.Color;

        [encoder setVertexBytes:&uniforms
                         length:sizeof(MetalSpriteUniforms)
                        atIndex:0];

        [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                    vertexStart:0
                    vertexCount:6];
    }

    // Draw ImGui into the same active render encoder.
    ImGui_ImplMetal_RenderDrawData(
        ImGui::GetDrawData(),
        commandBuffer,
        encoder
    );

    [encoder endEncoding];

    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];

    m_Stats.DrawCalls =
        static_cast<uint32_t>(m_QuadQueue.size());

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
        if (m_HasShutdown)
            return;

        m_HasShutdown = true;

        if (m_MetalContext)
            m_MetalContext->WaitIdle();

        shutdown_imgui();

        if (m_SpritePipeline)
        {
            CFBridgingRelease(m_SpritePipeline);
            m_SpritePipeline = nullptr;
        }

        m_CurrentTarget = nullptr;
        m_Camera.reset();
        m_MetalContext.reset();
        m_Context.reset();
    }

    void MetalRenderAPI::BeginScene()
    {
        if (!m_MetalContext || !m_SpritePipeline || !m_Camera)
            return;

        m_QuadQueue.clear();
        m_Stats.DrawCalls = 0;
        m_Stats.QuadCount = 0;

        m_MetalContext->CreateSwapchain();

        CAMetalLayer* layer =
            (__bridge CAMetalLayer*)m_MetalContext->GetLayer();

        id<CAMetalDrawable> drawable = [layer nextDrawable];

        if (!drawable)
            return;

        if (drawable.texture.height > 0)
        {
            m_Camera->SetAspectRatio(
                static_cast<float>(drawable.texture.width) /
                static_cast<float>(drawable.texture.height));
        }

        m_Camera->Update(Time::DeltaTime());

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
        if (!renderTarget ||
            !m_MetalContext ||
            !m_SpritePipeline ||
            !m_Camera)
        {
            return;
        }

        Ref<MetalRenderTarget> metalTarget =
            renderTarget.As<MetalRenderTarget>();

        if (!metalTarget)
        {
            ANV_LOG_ERROR(
                "MetalRenderAPI::DrawScene received a non-Metal RenderTarget"
            );
            return;
        }

        id<MTLTexture> targetTexture =
            (__bridge id<MTLTexture>)
                metalTarget->GetTexture();

        if (!targetTexture)
        {
            ANV_LOG_ERROR(
                "MetalRenderAPI::DrawScene RenderTarget has no color texture"
            );
            return;
        }

        id<MTLCommandQueue> commandQueue =
            (__bridge id<MTLCommandQueue>)
                m_MetalContext->GetCommandQueue();

        id<MTLRenderPipelineState> pipeline =
            (__bridge id<MTLRenderPipelineState>)
                m_SpritePipeline;

        if (!commandQueue || !pipeline)
            return;

        id<MTLCommandBuffer> commandBuffer =
            [commandQueue commandBuffer];

        if (!commandBuffer)
            return;

        commandBuffer.label =
            @"Anvil Viewport Command Buffer";

        MTLRenderPassDescriptor* renderPass =
            [MTLRenderPassDescriptor renderPassDescriptor];

        renderPass.colorAttachments[0].texture =
            targetTexture;

        renderPass.colorAttachments[0].loadAction =
            MTLLoadActionClear;

        renderPass.colorAttachments[0].storeAction =
            MTLStoreActionStore;

        renderPass.colorAttachments[0].clearColor =
            MTLClearColorMake(
                0.025,
                0.025,
                0.035,
                1.0
            );

        id<MTLRenderCommandEncoder> encoder =
            [commandBuffer
                renderCommandEncoderWithDescriptor:renderPass];

        if (!encoder)
        {
            ANV_LOG_ERROR(
                "Failed to create Metal viewport render encoder"
            );
            return;
        }

        encoder.label =
            @"Anvil Viewport Encoder";

        const NSUInteger width =
            targetTexture.width;

        const NSUInteger height =
            targetTexture.height;

        if (width == 0 || height == 0)
        {
            [encoder endEncoding];
            return;
        }

        MTLViewport viewport{};
        viewport.originX = 0.0;
        viewport.originY = 0.0;
        viewport.width =
            static_cast<double>(width);
        viewport.height =
            static_cast<double>(height);
        viewport.znear = 0.0;
        viewport.zfar = 1.0;

        [encoder setViewport:viewport];

        MTLScissorRect scissor{};
        scissor.x = 0;
        scissor.y = 0;
        scissor.width = width;
        scissor.height = height;

        [encoder setScissorRect:scissor];

        if (height > 0)
        {
            m_Camera->SetAspectRatio(
                static_cast<float>(width) /
                static_cast<float>(height)
            );
        }

        m_Camera->Update(Time::DeltaTime());

        std::stable_sort(
            m_QuadQueue.begin(),
            m_QuadQueue.end(),
            [](const QuadSubmission& left,
            const QuadSubmission& right)
            {
                return left.Layer < right.Layer;
            }
        );

        [encoder setRenderPipelineState:pipeline];

        for (const QuadSubmission& quad : m_QuadQueue)
        {
            MetalSpriteUniforms uniforms{};

            uniforms.ViewProjection =
                m_Camera->GetCameraUBO().ViewProjection;

            uniforms.Model =
                glm::translate(
                    glm::mat4(1.0f),
                    glm::vec3(
                        quad.Position,
                        static_cast<float>(quad.Layer)
                    )
                )
                *
                glm::rotate(
                    glm::mat4(1.0f),
                    glm::radians(quad.Rotaion),
                    glm::vec3(0.0f, 0.0f, 1.0f)
                )
                *
                glm::scale(
                    glm::mat4(1.0f),
                    glm::vec3(quad.Size, 1.0f)
                );

            uniforms.Color =
                quad.Color;

            [encoder setVertexBytes:&uniforms
                            length:sizeof(MetalSpriteUniforms)
                            atIndex:0];

            [encoder drawPrimitives:MTLPrimitiveTypeTriangle
                        vertexStart:0
                        vertexCount:6];
        }

        [encoder endEncoding];
        [commandBuffer commit];

        m_Stats.DrawCalls =
            static_cast<uint32_t>(m_QuadQueue.size());
    }

    void MetalRenderAPI::DrawQuad(
        const glm::vec2& position,
        float rotation,
        const glm::vec2& size,
        glm::vec4 color,
        int layer
    )
    {
        m_QuadQueue.push_back({
            position,
            rotation,
            size,
            color,
            layer
        });

        m_Stats.QuadCount++;
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