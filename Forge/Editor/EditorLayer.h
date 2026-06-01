#pragma once
#include "Anvil.h"

class EditorLayer : public anv::Layer
{
public:
	EditorLayer();

	void OnUpdate(float dt) override;
	void OnImGuiRender() override;

private:
	void begin_dock_space();
	void draw_scene_hierarchy();
	void draw_inspector();
	void draw_stats();

	entt::entity m_SelectedEntity = entt::null;
};

