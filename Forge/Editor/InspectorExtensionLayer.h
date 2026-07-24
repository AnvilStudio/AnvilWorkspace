#pragma once

#include <Anvil.h>

#include <filesystem>
#include <string>
#include <vector>

class EditorLayer;

/**
 * @brief Appends Python script editing and component creation to Forge's
 * Inspector window.
 *
 * Python files may be dropped anywhere in the Inspector. Texture assignment is
 * intentionally owned by the Sprite Renderer texture slot and the Viewport.
 */
class InspectorExtensionLayer : public anv::Layer
{
public:
    explicit InspectorExtensionLayer(EditorLayer* _editorLayer);

    void OnImGuiRender() override;

private:
    struct ScriptModuleInfo
    {
        std::string relativePath;
        std::vector<std::string> classes;
    };

    void draw_script_component(
        anv::Ref<anv::Scene> _scene,
        entt::entity _entity,
        anv::Component::Script& _script);

    void draw_add_component_menu(
        anv::Ref<anv::Scene> _scene,
        entt::entity _entity);

    void draw_inspector_script_drop_target(
        anv::Ref<anv::Scene> _scene,
        entt::entity _entity);

    void assign_script(
        anv::Ref<anv::Scene> _scene,
        entt::entity _entity,
        const std::filesystem::path& _path);

    void refresh_script_modules();
    std::filesystem::path get_scripts_directory() const;
    const ScriptModuleInfo* find_module(const std::string& _relativePath) const;

    static std::vector<std::string> discover_script_classes(
        const std::filesystem::path& _filePath);

    static bool draw_script_field(
        const std::string& _name,
        anv::ScriptField& _field);

private:
    EditorLayer* m_EditorLayer = nullptr;
    std::filesystem::path m_ScriptsDirectory;
    std::vector<ScriptModuleInfo> m_Modules;
};