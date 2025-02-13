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

	// Extends Asset for serialization
	class Shader : public Asset
	{
	public:
		ANV_NO_DSCRD
		static Ref<Shader> Create(const std::string& _shaderPath, _shared<Context> _ctx);

		template<typename T>
		inline T* GetAs() const
		{
			return dynamic_cast<T*>(this);
		}

	private:

		// Write the shaders compiled src 
		// to either a json fmt or bin fmt.
		void OnSerialize() override {};

	private:
		static inline std::unordered_map<std::string, Ref<Shader>> s_ShaderCache;
	};
}

