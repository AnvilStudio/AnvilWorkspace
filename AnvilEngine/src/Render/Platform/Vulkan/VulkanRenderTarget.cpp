#include "VulkanRenderTarget.h"
#include "VulkanContext.h"
#include <Render/RenderPass.h>
#include "VulkanSwapChainTarget.h"
#include "VulkanUtil.h"
#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include "VulkanImage.h"

namespace anv
{
    VulkanRenderTarget::VulkanRenderTarget(_shared<Context> _ctx, RenderTargetType _type, uint32_t _width, uint32_t _height)
        : RenderTarget(_ctx, _type, _width, _height)
    {
        create_renderpass();
        create_image();
        create_framebuffer();
        create_sampler();
    }

    VulkanRenderTarget::~VulkanRenderTarget()
    {
        if (m_Sampler)
        {
            vkDestroySampler(
                m_Context->GetAs<VulkanContext>()->GetDevice(),
                m_Sampler,
                nullptr
            );
        }
    }

    RenderTargetType VulkanRenderTarget::GetType() 
    {
        return m_Type;
    }

    uint32_t VulkanRenderTarget::GetWidth() 
    {
        return m_Width;
    }

    uint32_t VulkanRenderTarget::GetHeight() 
    {
        return m_Height;
    }

    void VulkanRenderTarget::Resize(uint32_t _width, uint32_t _height)
    {
        m_Width = _width;
        m_Height = _height;

        create_image();
        create_framebuffer();
        create_sampler();

        // The image view changed, so any ImGui descriptor referring to the old
        // view must not be reused. It will be recreated lazily when requested.
        m_ImGuiDescriptor = VK_NULL_HANDLE;
    }

    Ref<Image2D> VulkanRenderTarget::GetImage()
    {
        return m_Image;
    }

    Ref<RenderPass> VulkanRenderTarget::GetRenderPass()
    {
        return m_Renderpass;
    }

    Ref<Framebuffer> VulkanRenderTarget::GetFrameBuffer()
    {
        return m_Framebuffer;
    }

    void VulkanRenderTarget::Begin(Ref<CommandBuffer> _cmd)
    {
        m_Renderpass->Begin(_cmd, m_Framebuffer, m_Width, m_Height);
    }

    void VulkanRenderTarget::End(Ref<CommandBuffer> _cmd)
    {
        m_Renderpass->End(_cmd);
    }

    void* VulkanRenderTarget::GetImGuiTextureID()
    {
        if (m_ImGuiDescriptor == VK_NULL_HANDLE)
        {
            // Render targets can be created before the ImGui Vulkan backend is
            // initialized. Register the texture only when the editor actually
            // asks ImGui to display it.
            if (ImGui::GetCurrentContext() == nullptr ||
                ImGui::GetIO().BackendRendererUserData == nullptr)
            {
                return nullptr;
            }

            auto vkImageView = m_ImageView.As<VulkanImageView>();
            if (!vkImageView || m_Sampler == VK_NULL_HANDLE)
                return nullptr;

            m_ImGuiDescriptor = ImGui_ImplVulkan_AddTexture(
                m_Sampler,
                vkImageView->Get(),
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );
        }

        return (void*)m_ImGuiDescriptor;
    }

    void VulkanRenderTarget::create_framebuffer()
    {
        m_ImageView = m_Image->MakeImageView();

        m_Framebuffer = Framebuffer::Create(
            m_Context,
            m_ImageView,
            m_Renderpass,
            m_Width,
            m_Height
        );
    }

    void VulkanRenderTarget::create_sampler()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;

        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        ANV_VK_CHECK_RESULT(
            vkCreateSampler(
                m_Context->GetAs<VulkanContext>()->GetDevice(),
                &samplerInfo,
                nullptr,
                &m_Sampler
            ),
            "Failed to create Vulkan render target sampler!"
        );
    }

    void VulkanRenderTarget::create_renderpass()
    {
        RenderPassCreateInfo rpinfo{};
        rpinfo.d_name = "RenderTargetPass";

        RenderPassAttachment colatt;
        colatt.type = RenderPassAttachment::Type::ATT_TY_COLOR;
        colatt.loadOp = RenderPassAttachment::LoadOp::LOAD_OP_CLEAR;
        colatt.storeOp = RenderPassAttachment::StoreOp::STORE_OP_STORE;
        colatt.beginLayout = RenderPassAttachment::ImgLayout::IMG_LAYOUT_UNDEF;
        colatt.endLayout = RenderPassAttachment::ImgLayout::IMG_LAYOUT_SHADER_READ_ONLY; // ready for a shader to sample
        rpinfo.attachments.push_back(colatt);

        RenderPassCreateInfo::SubpassInfo rpspinfo{
            .colorAttachments = {0},     // ref the first color attach
            .depthStencilAttachment = -1 // no depth att
        };

        rpinfo.subpasses.push_back(rpspinfo);
        m_Renderpass = RenderPass::Create(rpinfo, m_Context);
        m_Renderpass->Build();
    }

    void VulkanRenderTarget::create_image()
    {
        //m_Image = Image2D::Create(m_Context, Image2D::Format::R8G8B8A8_UNorm, m_Width, m_Height);
        m_Image = Ref<VulkanImage2D>::Create(m_Context, Image2D::Format::R8G8B8A8_UNorm, m_Width, m_Height);
    }

}
