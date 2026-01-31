#pragma once
#include "../Util/UMacros.h"
#include "../Core/Reference.h"
#include "QueueChain.h"
#include "Context.h"

namespace anv
{
	struct RenderPassCreateInfo;

	class RenderPass
		: public RefCounter
	{
	public:

		struct Attachment
		{
		public:
			// Attachment type
			enum class Type {
				ATT_TY_UNDEF,
				ATT_TY_COLOR,
				ATT_TY_DEPTH, // depth stencil
			};

			// Before rendering operation
			enum class LoadOp
			{
				LOAD_OP_CLEAR,   // clear the image
				LOAD_OP_LOAD,    // preserve the images previous contents
				LOAD_OP_UNDEF,   // dont care about the previous contents
				LOAD_OP_MAX_ENUM // null/default value pretty much
			};

			// After rendering operation
			enum class StoreOp
			{
				STORE_OP_STORE,   // Store rendered contents in memory
				STORE_OP_UNDEF,   // contents will be undefined (dont care ab them)
				STORE_OP_MAX_ENUM // null/default value as well
			};

			enum class ImgLayout
			{
				IMG_LAYOUT_UNDEF,      // Dont care ab the images prev layout
				IMG_LAYOUT_COLOR_ATT,  // Used as a color att
				IMG_LAYOUT_PRES,       // Presented to the swapchain
				IMG_LAYOUT_MEMCPY_DST, // Used for a mem copy destination
				IMG_LAYOUT_MAX_ENUM    // null/default value
			};

			Type type;
			ImgLayout beginLayout = ImgLayout::IMG_LAYOUT_MAX_ENUM;
			ImgLayout endLayout   = ImgLayout::IMG_LAYOUT_MAX_ENUM;
			LoadOp  loadOp        = LoadOp::LOAD_OP_MAX_ENUM;
			StoreOp storeOp       = StoreOp::STORE_OP_MAX_ENUM;

			// debug index
			int d_index = -1;
			const char* d_rp_name;
		private:
			friend class RenderPass;
			friend class VulkanRenderPass;
		};

		ANV_NO_DSCRD
		static Ref<RenderPass> Create(RenderPassCreateInfo& _createinfo, _shared<Context> _ctx);

		RenderPass(RenderPassCreateInfo _info);
		RenderPass() = default;

		//virtual void AddAttachment(RenderPassCreateInfo::Attachment _new_att) = 0;
		//virtual void SetOutput(Framebuffer& _fb);
		virtual void Begin() = 0;
		virtual void End()   = 0;
		virtual void Build() = 0;

		std::string m_DName;

	protected:
		_shared<QueueChain> m_RenderQueue = nullptr;
	};

	struct RenderPassCreateInfo
	{
		_shared<QueueChain> commandQueue;
		struct SubpassInfo {
			std::vector<int> colorAttachments;       // Indices of color attachments
			int depthStencilAttachment = -1;         // Index of depth/stencil attachment
		};

		std::vector<RenderPass::Attachment> attachments;    // Attachments for the render pass
		std::vector<SubpassInfo> subpasses;        // Subpasses in the render pass
		const char* d_name;                        // debug name
	};

    
}
