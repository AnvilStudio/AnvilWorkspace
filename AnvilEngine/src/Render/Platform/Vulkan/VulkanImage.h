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

		VulkanImageView(_shared<Context> _dv, VkImage _img, VkFormat _fmt);
		~VulkanImageView();

		void OnDestroy() override;

		VkImageView GetRaw() { return m_ImgView; }

	private:
		VkImageView   m_ImgView;
		VulkanContext* m_VkContext;
	};

	class VulkanImage2D
		: public Image2D
	{
	public:
		VulkanImage2D(_shared<Context> _ctx, Format _fmt);
		VulkanImage2D(_shared<Context> _ctx, VkImage _img, Format _fmt);
		~VulkanImage2D();


		Ref<ImageView> MakeImageView() override;

		VkImage GetRaw() { return m_Image; }
		void SetImage(VkImage _img) { m_Image = _img; };

	private:
		VkFormat       m_Format;
		VkImage        m_Image;
	};

}
