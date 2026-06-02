#pragma once
#include <string>
#include "../Core/Uuid.h"

namespace anv
{
	enum SceneContext
	{
		CTX_2D,
		CTX_3D
	};

	struct SceneRegInfo
	{
		std::string path;
		std::string name;
		uuid::AssetUUID uuid;
		SceneContext ctx;
	};
}