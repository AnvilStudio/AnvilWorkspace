#pragma once
#include "../Core/Reference.h"
#include "RenderPass.h"
#include "Shader.h"

namespace anv
{

	struct VertexAttribute {
		std::string name;           // Attribute name (e.g., "position", "normal")
		uint32_t location;          // Binding location
		uint32_t offset;            // Byte offset in the vertex structure
		uint32_t size;              // Size of the attribute (e.g., sizeof(float) * 3)
		uint32_t stride;            // Stride between vertices
		bool normalized;            // Whether the data should be normalized
	};

	struct VertexInputLayout {
		_vec<VertexAttribute> attributes; // List of attributes
		uint32_t binding;                        // Vertex buffer binding index
		size_t stride;                    

		void AddAttribute(const std::string& name, uint32_t location, 
			uint32_t offset, uint32_t size, 
			uint32_t stride, bool normalized = false) 
		{
			attributes.push_back({ name, location, offset, size, stride, normalized });
		}
	};

	struct RasterizationSettings {
		bool depthClampEnable = false;      // Whether to clamp depth values
		bool rasterizerDiscardEnable = false; // Whether to disable rasterization
		uint32_t polygonMode = 0;          // Fill (0), line (1), or point (2)
		uint32_t cullMode = 1;             // None (0), front (1), back (2), or both (3)
		uint32_t frontFace = 0;            // Clockwise (0) or counter-clockwise (1)
		float lineWidth = 1.0f;            // Line width for wireframe rendering
	};

	struct BlendAttachment {
		bool blendEnable = false;          // Enable blending
		uint32_t srcColorBlendFactor = 1; // Source blend factor
		uint32_t dstColorBlendFactor = 0; // Destination blend factor
		uint32_t colorBlendOp = 0;        // Blend operation (add, subtract, etc.)
		uint32_t srcAlphaBlendFactor = 1; // Source alpha blend factor
		uint32_t dstAlphaBlendFactor = 0; // Destination alpha blend factor
		uint32_t alphaBlendOp = 0;        // Alpha blend operation
		uint32_t colorWriteMask = 0xF;    // RGBA write mask (0xF = all channels)
	};

	struct ColorBlendSettings {
		bool logicOpEnable = false;        // Enable logic operations
		uint32_t logicOp = 0;              // Logic operation (if enabled)
		float blendConstants[4] = { 0, 0, 0, 0 }; // Blend constants for equations
		_vec<BlendAttachment> attachments; // Blend settings per attachment

		void AddAttachment(const BlendAttachment& attachment) {
			attachments.push_back(attachment);
		}
	};

	struct PipelineCreateInfo
	{
		Ref<Shader> pShaderStages = nullptr;         // Ref to the shader containing the shader staged
		VertexInputLayout vertexInputLayout{};       // Vertex input Layout (vertex) settings
		RasterizationSettings rasterizeSettings{};   // Rasterization (fragment) settings
		ColorBlendSettings colorBlendSettings{};     // Color blending settings
	};

	class GraphicsPipeline : public Asset
	{
	public:
		virtual ~GraphicsPipeline() = default;

		virtual void SetShaderStages(const Ref<Shader> _shader)                     = 0;
		virtual void SetVertexInputLayout(const VertexInputLayout* _layout)         = 0;
		virtual void SetRasterizationSettings(const RasterizationSettings* _raster) = 0;
		virtual void SetColorBlendSettings(const ColorBlendSettings* _colbld)       = 0;
		virtual void SetRenderPass(const Ref<RenderPass> _rps) = 0;
		virtual void Build() = 0;
		virtual void Bind(Ref<CommandBuffer> _cmd) = 0;

		virtual void OnSave(Serializer& _ser) = 0;

	protected:
		GraphicsPipeline(_shared<Context> _ctx, std::string _dName);
		ANV_NO_DSCRD
		static Ref<GraphicsPipeline> create_pipeline_asset(_shared<Context> _ctx, std::string _dName);
		// static Ref<GraphicsPipeline> Load(Ref<File> _cache)
		
		std::string m_Name;

	private:
		_shared<Context> m_Context;
		friend class AssetManager;
	};
}
