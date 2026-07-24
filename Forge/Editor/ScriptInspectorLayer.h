#pragma once

#include <Anvil.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

class EditorLayer;

class ScriptInspectorLayer : public anv::Layer
{
public:
    explicit ScriptInspectorLayer(EditorLayer* _editorLayer);

    void OnImGuiRender() override;

private:
    struct ScriptModuleInfo
    {
        std::string relativePath;
        std::vector<std::string> classes;
    };

    void draw_add_component_menu(
        const anv::Ref<anv::Scene>& _scene,
        entt::entity _entity);

    void draw_script_component(
        const anv::Ref<anv::Scene>& _scene,
        entt::entity _entity,
        anv::Component::Script& _script);

    void refresh_script_modules(const anv::Ref<anv::Scene>& _scene);
    std::filesystem::path find_scripts_directory(const anv::Ref<anv::Scene>& _scene) const;
    const ScriptModuleInfo* find_module(const std::string& _relativePath) const;

    static std::vector<std::string> discover_script_classes(
        const std::filesystem::path& _filePath);

    static bool draw_script_field(
        const std::string& _name,
        anv::ScriptField& _field);

    EditorLayer* m_EditorLayer = nullptr;
    std::filesystem::path m_ScriptsDirectory;
    std::vector<ScriptModuleInfo> m_Modules;
};