#pragma once
#include <Scene/Manager.h>
#include <Scene/Scene.h>
#include <Util/UMacros.h>
#include <Layer/Layer.h>

namespace anv
{
	class SceneLayer : public Layer
	{
	public:
		SceneLayer(_shared<SceneManager> _manager);

		virtual void OnAttach()  override;
		virtual void OnDetach() override;
		virtual void OnRender() override;
		virtual void OnUpdate(float dt) override;
		/*virtual void OnImGuiRender() {}*/

	private:
		_shared<SceneManager> m_ScnMgr;
		Ref<Scene> m_Active;
	};
}

