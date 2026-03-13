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
		template<typename T>
		inline T* GetAs() const
		{
			return dynamic_cast<T*>(this);
		}

	protected:
		ANV_NO_DSCRD
			static Ref<Shader> Create(const std::string& _shaderPath, _shared<Context> _ctx);
		ANV_NO_DSCRD
			static Ref<Shader> Create(Deserialized& _dser);

		Shader(std::string& _name);
		Shader(Deserialized& _dser);
	
		friend class AssetManager;
	};
}

