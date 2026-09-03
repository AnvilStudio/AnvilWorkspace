#include "MtlRenderAPI.h"
#include "MtlRenderTarget.h"
#include "MtlTexture.h"

#include <Render/Renderer.h>
#include <Core/Window.h>
#include <Core/App.h>
#include <Util/Time/Time.h>

#include <algorithm>
#include <cstdint>
#include <glm/gtc/matrix_transform.hpp>

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>


namespace anv
{
    struct MetalSpriteUniforms
    {
        glm::mat4 ViewProjection{1.0f};
        glm::mat4 Model{1.0f};
        glm::vec4 Color{1.0f};
        uint32_t UseTexture = 0;
        uint32_t Padding[3]{};
    };

    static NSString* SpriteShaderSource = @R"(
        #include <metal_stdlib>
        using namespace metal;

        struct SpriteUniforms
        {
            float4x4 viewProjection;
            float4x4 model;
            float4 color;
            uint useTexture;
            uint3 padding;
        };

        struct VertexOutput
        {
            float4 position [[position]];
            float4 color;
            float2 texCoord;
            uint useTexture [[flat]];
        };

        vertex VertexOutput sprite_vertex(
            uint vertexID [[vertex_id]],
            constant SpriteUniforms& uniforms [[buffer(0)]])
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

            constexpr float2 texCoords[] =
            {
                float2(0.0, 0.0),
                float2(1.0, 0.0),
                float2(1.0, 1.0),
                float2(1.0, 1.0),
                float2(0.0, 1.0),
                float2(0.0, 0.0)
            };

            VertexOutput output;
            output.position = uniforms.viewProjection * uniforms.model *
                float4(positions[vertexID], 0.0, 1.0);
            output.color = uniforms.color;
            output.texCoord = texCoords[vertexID];
            output.useTexture = uniforms.useTexture;
            return output;
        }

        fragment float4 sprite_fragment(
            VertexOutput input [[stage_in]],
            texture2d<float> spriteTexture [[texture(0)]],
            sampler spriteSampler [[sampler(0)]])
        {
            if (input.useTexture == 0)
                return input.color;

            return spriteTexture.sample(spriteSampler, input.texCoord) * input.color;
        }
    )";

    MetalRenderAPI::MetalRenderAPI(Render2DCreateInfo info)
    {
        m_Context = info.pTarget->GetContext();
        m_MetalContext = std::dynamic_pointer_cast<MetalContext>(m_Context);

        if (!m_MetalContext)
        {
            ANV_LOG_FATAL("MetalRenderAPI received a non-Metal context!");
            return;
        }

        create_sprite_pipeline();
        m_ImGuiLayer = ImGuiLayer::Create(m_Context, nullptr, info.imguiIniPath);
        ANV_ASSERT(m_ImGuiLayer, "Failed to initialize ImGui layer");
    }

    MetalRenderAPI::~MetalRenderAPI()
    {
        OnShutdown();
    }

    void MetalRenderAPI::create_sprite_pipeline()
    {
        id<MTLDevice> device = (__bridge id<MTLDevice>)m_MetalContext->GetDevice();
        NSError* error = nil;
        id<MTLLibrary> library = [device newLibraryWithSource:SpriteShaderSource options:nil error:&error];

        if (!library)
        {
            ANV_LOG_FATAL("Metal shader compilation failed: %s", error.localizedDescription.UTF8String);
            return;
        }

        id<MTLFunction> vertexFunction = [library newFunctionWithName:@"sprite_vertex"];
        id<MTLFunction> fragmentFunction = [library newFunctionWithName:@"sprite_fragment"];

        MTLRenderPipelineDescriptor* descriptor = [[MTLRenderPipelineDescriptor alloc] init];
        descriptor.label = @"Anvil Sprite Pipeline";
        descriptor.vertexFunction = vertexFunction;
        descriptor.fragmentFunction = fragmentFunction;
        descriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
        descriptor.colorAttachments[0].blendingEnabled = YES;
        descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;

        id<MTLRenderPipelineState> pipeline =
            [device newRenderPipelineStateWithDescriptor:descriptor error:&error];

        if (!pipeline)
        {
            ANV_LOG_FATAL("Metal pipeline creation failed: %s", error.localizedDescription.UTF8String);
            return;
        }

        MTLSamplerDescriptor* samplerDescriptor = [[MTLSamplerDescriptor alloc] init];
        samplerDescriptor.minFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.magFilter = MTLSamplerMinMagFilterLinear;
        samplerDescriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
        samplerDescriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;

        id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:samplerDescriptor];
        m_SpritePipeline = (__bridge_retained void*)pipeline;
        m_Sampler = (__bridge_retained void*)sampler;
    }

    RendererStats MetalRenderAPI::GetStats()
    {
        return m_Stats;
    }

    void MetalRenderAPI::encode_quads(void* encoderHandle, _shared<Camera2D> camera)
    {
        id<MTLRenderCommandEncoder> encoder =
            (__bridge id<MTLRenderCommandEncoder>)encoderHandle;
        id<MTLRenderPipelineState> pipeline =
            (__bridge id<MTLRenderPipelineState>)m_SpritePipeline;
        id<MTLSamplerState> sampler =
            (__bridge id<MTLSamplerState>)m_Sampler;

        std::stable_sort(
            m_QuadQueue.begin(),
            m_QuadQueue.end(),
            [](const QuadSubmission& left, const QuadSubmission& right)
            {
                return left.Layer < right.Layer;
            });

        [encoder setRenderPipelineState:pipeline];
        [encoder setFragmentSamplerState:sampler atIndex:0];

        for (const QuadSubmission& quad : m_QuadQueue)
        {
            MetalSpriteUniforms uniforms{};
            uniforms.ViewProjection = camera->GetCameraUBO().ViewProjection;
            uniforms.Model =
                glm::translate(glm::mat4(1.0f), glm::vec3(quad.Position, 0.0f)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(quad.Rotation), glm::vec3(0.0f, 0.0f, 1.0f)) *
                glm::scale(glm::mat4(1.0f), glm::vec3(quad.Size, 1.0f));
            uniforms.Color = quad.Color;

            id<MTLTexture> texture = nil;
            if (quad.TextureAsset && quad.TextureAsset->IsGPUReady())
            {
                texture = (__bridge id<MTLTexture>)quad.TextureAsset->GetNativeHandle();
                uniforms.UseTexture = texture ? 1u : 0u;
            }

            [encoder setVertexBytes:&uniforms length:sizeof(uniforms) atIndex:0];
            [encoder setFragmentTexture:texture atIndex:0];
            [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
        }
    }

    void MetalRenderAPI::DrawFrame()
    {
        if (!m_CurrentDrawable || !m_CurrentCommandBuffer || !m_CurrentEncoder)
            return;

        id<CAMetalDrawable> drawable = (__bridge id<CAMetalDrawable>)m_CurrentDrawable;
        id<MTLCommandBuffer> commandBuffer = (__bridge id<MTLCommandBuffer>)m_CurrentCommandBuffer;
        id<MTLRenderCommandEncoder> encoder = (__bridge id<MTLRenderCommandEncoder>)m_CurrentEncoder;

        encode_quads(m_CurrentEncoder, m_Camera);
        m_ImGuiLayer->Render({
            .nativeCommandBuffer = (__bridge void*)commandBuffer,
            .nativeRenderEncoder = (__bridge void*)encoder
        });
        [encoder endEncoding];
        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];

        m_Stats.DrawCalls = static_cast<uint32_t>(m_QuadQueue.size());

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

        m_ImGuiLayer.reset();

        if (m_Sampler)
            CFBridgingRelease(m_Sampler);
        if (m_SpritePipeline)
            CFBridgingRelease(m_SpritePipeline);

        m_Sampler = nullptr;
        m_SpritePipeline = nullptr;
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
        m_Stats = {};
        m_MetalContext->CreateSwapchain();

        CAMetalLayer* layer = (__bridge CAMetalLayer*)m_MetalContext->GetLayer();
        id<CAMetalDrawable> drawable = [layer nextDrawable];
        if (!drawable)
            return;

        m_Camera->Update(Time::DeltaTime());
        id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)m_MetalContext->GetCommandQueue();
        id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
        MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
        renderPass.colorAttachments[0].texture = drawable.texture;
        renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPass.colorAttachments[0].storeAction = MTLStoreActionStore;
        renderPass.colorAttachments[0].clearColor = MTLClearColorMake(0.025, 0.025, 0.035, 1.0);
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPass];

        m_CurrentDrawable = (__bridge_retained void*)drawable;
        m_CurrentCommandBuffer = (__bridge_retained void*)commandBuffer;
        m_CurrentRenderPass = (__bridge_retained void*)renderPass;
        m_CurrentEncoder = (__bridge_retained void*)encoder;

        m_ImGuiLayer->BeginFrame((__bridge void*)renderPass);
    }

    void MetalRenderAPI::BeginScene(Ref<RenderTarget> renderTarget)
    {
        m_CurrentTarget = renderTarget;
    }

    void MetalRenderAPI::DrawScene(Ref<RenderTarget> renderTarget, _shared<Camera2D> camera)
    {
        if (!renderTarget || !m_MetalContext || !m_SpritePipeline || !camera)
            return;

        Ref<MetalRenderTarget> target = renderTarget.As<MetalRenderTarget>();
        if (!target)
            return;

        id<MTLTexture> targetTexture = (__bridge id<MTLTexture>)target->GetTexture();
        id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)m_MetalContext->GetCommandQueue();
        id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
        MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
        renderPass.colorAttachments[0].texture = targetTexture;
        renderPass.colorAttachments[0].loadAction = MTLLoadActionClear;
        renderPass.colorAttachments[0].storeAction = MTLStoreActionStore;
        renderPass.colorAttachments[0].clearColor = MTLClearColorMake(0.025, 0.025, 0.035, 1.0);
        id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPass];

        if (targetTexture.height > 0)
            m_Camera->SetAspectRatio(static_cast<float>(targetTexture.width) / static_cast<float>(targetTexture.height));

        MTLViewport viewport{0.0, 0.0, static_cast<double>(targetTexture.width), static_cast<double>(targetTexture.height), 0.0, 1.0};
        [encoder setViewport:viewport];
        encode_quads((__bridge void*)encoder, camera);
        [encoder endEncoding];
        [commandBuffer commit];
        m_Stats.DrawCalls = static_cast<uint32_t>(m_QuadQueue.size());
    }

    void MetalRenderAPI::DrawQuad(
        const glm::vec2& position,
        float rotation,
        const glm::vec2& size,
        glm::vec4 color,
        Ref<Texture> texture,
        int layer)
    {
        m_QuadQueue.push_back({position, rotation, size, color, texture, layer});
        m_Stats.QuadCount++;
    }

    void MetalRenderAPI::EndScene()
    {
        m_ImGuiLayer->EndFrame();
    }

    void MetalRenderAPI::SetMainCamera(_shared<Camera2D> camera)
    {
        m_Camera = camera;
    }
}
