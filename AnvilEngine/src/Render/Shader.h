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

		virtual void OnSave(Serializer& _ser) = 0;

		// Renderer-owned shaders should not depend on the application AssetManager.
		// This avoids startup cycles while still keeping normal project shaders
		// registered through AssetManager::CreateShader().
		ANV_NO_DSCRD
		static Ref<Shader> CreateInternal(const std::string& _shaderPath, _shared<Context> _ctx)
		{
			return Create(_shaderPath, _ctx);
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
