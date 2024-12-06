#pragma once
#include "../Util/UMacros.h"
#include "Context.h"

namespace anv
{
	struct RenderPassCreateInfo
	{
		struct RPAttachmentDes
		{
			enum class Format {
				Undefined,
				Color,
				DepthStencil,
			};

			Format format;
			bool isLoadOpClear;
			bool isStoreOpClear;
		};

		struct RPSubpassInfo {
			std::vector<int> colorAttachments;       // Indices of color attachments
			int depthStencilAttachment = -1;         // Index of depth/stencil attachment
		};

		std::vector<RPAttachmentDes> attachments;    // Attachments for the render pass
		std::vector<RPSubpassInfo> subpasses;        // Subpasses in the render pass
	};

	// need a way to set inputs and outputs
	class RenderPass
	{
	public:
		static _shared<RenderPass> Create(RenderPassCreateInfo& _createinfo, _shared<Context> _ctx);

		virtual void SetInput(int _index, void* _resource);
		virtual void SetOutput();
		virtual void Build() = 0;
	};
}
