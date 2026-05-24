#pragma once

#include "../Core/Reference.h"
#include "../Input/Input.h"
#include "Camera.h"

namespace anv
{
	class CameraController
	{
	public:
		CameraController(_shared<InputSystem>_is, _shared<Camera2D> _cam);
		~CameraController();

		void Update(float _deltaTime);

	private:
			_shared<InputSystem> m_InputSystem;
			_shared<Camera2D>    m_Camera;
			float m_Speed = 2.f;
	};
}

