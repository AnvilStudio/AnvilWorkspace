#pragma once

#include "Context.h"
#include "../Core/Reference.h"

namespace anv
{
	enum class BufferUsage
	{
		Vertex,
		Index,
		Uniform,
		Storage
	};


	struct BufferCreateInfo
	{
		BufferUsage Usage;
		uint64_t Size = 0;
		const void* InitialData = nullptr;
		bool Dynamic = false;
	};

	class Buffer : public RefCounter
	{

	public:
		Buffer(_shared<Context> _ctx);

		static Ref<Buffer> Create(_shared<Context> _ctx, BufferCreateInfo _info);
		
		virtual void SetData(const void* data, uint64_t size, uint64_t offset = 0) = 0;
		virtual void Bind() = 0;

		virtual uint64_t GetSize() const = 0;
		virtual BufferUsage GetUsage() const = 0;

	protected:
		_shared<Context> m_Context;
	};
}