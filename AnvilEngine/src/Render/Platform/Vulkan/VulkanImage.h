#pragma once
#include "Render/Image.h"
#include "VulkanContext.h"

#include <vulkan/vulkan.h>

namespace anv
{
	class VulkanImageView :
		public ImageView
	{
	public:

		VulkanImageView(_shared<Context> _dv, VkFormat _fmt, VkImage _img = nullptr);
		~VulkanImageView();

		void OnDestroy() override;

		VkImageView& Get() { return m_ImgView; }

	private:
		VkImageView   m_ImgView;
		VulkanContext* m_VkContext;
	};

	class VulkanImage2D
		: public Image2D
	{
	public:
		VulkanImage2D(_shared<Context> _ctx, Format _fmt, uint32_t _width, uint32_t _height);
		VulkanImage2D(_shared<Context> _ctx, VkImage _img, Format _fmt, uint32_t _width, uint32_t _height);
		~VulkanImage2D();

		// Builds an image view for the VkImage
		Ref<ImageView> MakeImageView() override;

		VkImage Get() { return m_Image; }
		void SetImage(VkImage _img) { m_Image = _img; };

	private:
		void create_image();

		VkFormat       m_Format;
		VkImage        m_Image = VK_NULL_HANDLE;
		Ref<VulkanImageView> m_ImageView = nullptr;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
		bool m_OwnsImage = true;
	};

}
