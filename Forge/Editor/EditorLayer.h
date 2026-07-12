#pragma once
#include <Anvil.h>
//#include "Windows/Viewport.h" // TODO: Get viewports back!
#include "Windows/DevNotes.h"

class EditorLayer : public anv::Layer
{
public:

	enum class SceneState
	{
		Edit,
		Play,
		Pause
	};

	EditorLayer();

	void OnAttach() override;
	void OnUpdate(float dt) override;
	void OnImGuiRender() override;

private:
	void begin_dock_space();
	void draw_menu_bar();
	void draw_viewport();
	void draw_scene_hierarchy();
	void draw_inspector();
	void draw_add_component_menu(
		anv::Ref<anv::Scene> scene,
		entt::entity entity);
	void draw_stats();
	void draw_filesys();

	entt::entity m_SelectedEntity = entt::null;
	entt::entity m_EntityToDelete = entt::null;

	SceneState m_SceneState = SceneState::Edit;

	//Viewport m_Viewport;
	DevNotesPanel m_DevNotes;
};

