#pragma once
#include "Context.h"
#include "../Asset/Asset.h"
#include <unordered_map>

namespace anv
{
	enum class ShaderType
	{
		VERT,
		FRAG,
		NONE
	};

	class Shader : public Asset
	{
	public:
		static Ref<Shader> Create(const std::string& _shaderPath, _shared<Context> _ctx);

		template<typename T>
		inline T* GetAs()
		{
			return dynamic_cast<T*>(this);
		}

	private:
		static inline std::unordered_map<std::string, Ref<Shader>> s_ShaderCache;
	};
}

