#pragma once
#include "../../Util/UMacros.h"

namespace anv {

	class Window;
	
	class Context
	{
	public:
		static _unique(Context) Create(Window* _win);

	public:
		virtual void CreateBuffer() = 0;
	};
}
