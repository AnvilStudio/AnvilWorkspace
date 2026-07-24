#include "ScriptInspectorLayer.h"

#include "EditorLayer.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>

using namespace anv;

namespace
{
    std::string trim(std::string _value)
    {
        const auto first = std::find_if_not(
            _value.begin(),
            _value.end(),
            [](unsigned char _character)
            {
                return std::isspace(_character) != 0;
            });

        const auto last = std::find_if_not(
            _value.rbegin(),
            _value.rend(),
            [](unsigned char _character)
            {
                return std::isspace(_character) != 0;
            }).base();

        if (first >= last)
            return {};

        return std::string(first, last);
    }

    bool input_string(const char* _label, std::string& _value)
    {
        char buffer[512]{};
        std::snprintf(buffer, sizeof(buffer), "%s", _value.c_str());

        if (!ImGui::InputText(_label, buffer, sizeof(buffer)))
            return false;

        _value = buffer;
        return true;
    }
}

ScriptInspectorLayer::ScriptInspectorLayer(EditorLayer* _editorLayer)
    : Layer("Script Inspector Layer"),
      m_EditorLayer(_editorLayer)
{
}

void ScriptInspectorLayer::OnImGuiRender()
{
    if (!m_EditorLayer)
        return;

    Ref<Scene> scene =
        App::GetInstance()->GetSceneManager()->GetActive();

    if (!scene)
        return;

    const entt::entity entity = m_EditorLayer->GetSelectedEntity();
    if (entity == entt::null || !scene->Registry().valid(entity))
        return;

    ImGui::Begin("Inspector");
    ImGui::Separator();

    draw_add_component_menu(scene, entity);

    if (scene->HasComponent<Component::Script>(entity))
    {
        auto& script = scene->GetComponent<Component::Script>(entity);
        draw_script_component(scene, entity, script);
    }

    ImGui::End();
}

void ScriptInspectorLayer::draw_add_component_menu(
    Ref<Scene> _scene,
    entt::entity _entity)
{
    const float width = ImGui::GetContentRegionAvail().x;
    if (ImGui::Button("Add Component", ImVec2(width, 0.0f)))
        ImGui::OpenPopup("AddComponentPopup");

    if (!ImGui::BeginPopup("AddComponentPopup"))
        return;

    if (!_scene->HasComponent<Component::Transform2d>(_entity) &&
        ImGui::MenuItem("Transform 2D"))
    {
        _scene->AddComponent<Component::Transform2d>(_entity);
        _scene->Save();
        ImGui::CloseCurrentPopup();
    }

    if (!_scene->HasComponent<Component::SpriteRenderer>(_entity) &&
        ImGui::MenuItem("Sprite Renderer"))
    {
        _scene->AddComponent<Component::SpriteRenderer>(_entity);
        _scene->Save();
        ImGui::CloseCurrentPopup();
    }

    if (!_scene->HasComponent<Component::Script>(_entity) &&
        ImGui::MenuItem("Python Script"))
    {
        _scene->AddComponent<Component::Script>(_entity);
        refresh_script_modules(_scene);
        _scene->Save();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void ScriptInspectorLayer::draw_script_component(
    Ref<Scene> _scene,
    entt::entity _entity,
    Component::Script& _script)
{
    if (!ImGui::CollapsingHeader(
            "Python Script",
            ImGuiTreeNodeFlags_DefaultOpen))
    {
        return;
    }

    bool changed = false;

    if (ImGui::Checkbox("Enabled", &_script.enabled))
        changed = true;

    ImGui::SameLine();
    if (ImGui::Button("Reload") && !_script.modulePath.empty())
        PythonScriptEngine::RequestReload(_script.modulePath);

    ImGui::SameLine();
    if (ImGui::Button("Refresh Scripts"))
        refresh_script_modules(_scene);

    if (m_Modules.empty())
        refresh_script_modules(_scene);

    const char* modulePreview = _script.modulePath.empty()
        ? "Select a Python module"
        : _script.modulePath.c_str();

    if (ImGui::BeginCombo("Module", modulePreview))
    {
        for (const ScriptModuleInfo& module : m_Modules)
        {
            const bool selected = module.relativePath == _script.modulePath;
            if (ImGui::Selectable(module.relativePath.c_str(), selected))
            {
                _script.modulePath = module.relativePath;
                _script.className.clear();
                _script.fields.clear();
                changed = true;
            }

            if (selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    const ScriptModuleInfo* selectedModule = find_module(_script.modulePath);
    const char* classPreview = _script.className.empty()
        ? "Select an anvil.Script class"
        : _script.className.c_str();

    ImGui::BeginDisabled(selectedModule == nullptr);
    if (ImGui::BeginCombo("Class", classPreview))
    {
        if (selectedModule)
        {
            for (const std::string& className : selectedModule->classes)
            {
                const bool selected = className == _script.className;
                if (ImGui::Selectable(className.c_str(), selected))
                {
                    _script.className = className;
                    _script.fields.clear();
                    changed = true;
                }

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
    ImGui::EndDisabled();

    if (selectedModule && selectedModule->classes.empty())
        ImGui::TextDisabled("No classes deriving from anvil.Script were found.");

    if (!_script.fields.empty())
    {
        ImGui::SeparatorText("Fields");

        std::vector<std::string> fieldNames;
        fieldNames.reserve(_script.fields.size());

        for (const auto& [name, field] : _script.fields)
        {
            (void)field;
            fieldNames.push_back(name);
        }

        std::sort(fieldNames.begin(), fieldNames.end());

        for (const std::string& name : fieldNames)
        {
            ImGui::PushID(name.c_str());
            changed |= draw_script_field(name, _script.fields.at(name));
            ImGui::PopID();
        }
    }
    else if (!_script.modulePath.empty() && !_script.className.empty())
    {
        ImGui::TextDisabled(
            "Fields appear after the script instance is created during scene update.");
    }

    if (changed)
    {
        _scene->Save();

        if (!_script.modulePath.empty())
            PythonScriptEngine::RequestReload(_script.modulePath);
    }

    ImGui::Separator();
    if (ImGui::Button("Remove Python Script"))
    {
        if (!_script.modulePath.empty())
            PythonScriptEngine::RequestReload(_script.modulePath);

        _scene->RemoveComponent<Component::Script>(_entity);
        _scene->Save();
    }
}

void ScriptInspectorLayer::refresh_script_modules(Ref<Scene> _scene)
{
    m_Modules.clear();
    m_ScriptsDirectory = find_scripts_directory(_scene);

    if (m_ScriptsDirectory.empty() ||
        !std::filesystem::exists(m_ScriptsDirectory))
    {
        return;
    }

    std::error_code error;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(
             m_ScriptsDirectory,
             std::filesystem::directory_options::skip_permission_denied,
             error))
    {
        if (error || !entry.is_regular_file())
            continue;

        if (entry.path().extension() != ".py" ||
            entry.path().filename() == "__init__.py")
        {
            continue;
        }

        ScriptModuleInfo module;
        module.relativePath = std::filesystem::relative(
            entry.path(),
            m_ScriptsDirectory,
            error).generic_string();

        if (error)
        {
            error.clear();
            continue;
        }

        module.classes = discover_script_classes(entry.path());
        m_Modules.push_back(std::move(module));
    }

    std::sort(
        m_Modules.begin(),
        m_Modules.end(),
        [](const ScriptModuleInfo& _left, const ScriptModuleInfo& _right)
        {
            return _left.relativePath < _right.relativePath;
        });
}

std::filesystem::path ScriptInspectorLayer::find_scripts_directory(
    Ref<Scene> _scene) const
{
    std::filesystem::path current = _scene->GetPath();
    if (!current.empty() && current.has_filename())
        current = current.parent_path();

    if (current.empty())
        current = std::filesystem::current_path();

    for (int depth = 0; depth < 8 && !current.empty(); ++depth)
    {
        const std::filesystem::path candidate = current / "Scripts";
        if (std::filesystem::is_directory(candidate))
            return candidate;

        const std::filesystem::path parent = current.parent_path();
        if (parent == current)
            break;

        current = parent;
    }

    const std::filesystem::path workingCandidate =
        std::filesystem::current_path() / "Scripts";

    if (std::filesystem::is_directory(workingCandidate))
        return workingCandidate;

    return {};
}

const ScriptInspectorLayer::ScriptModuleInfo*
ScriptInspectorLayer::find_module(const std::string& _relativePath) const
{
    const auto iterator = std::find_if(
        m_Modules.begin(),
        m_Modules.end(),
        [&](const ScriptModuleInfo& _module)
        {
            return _module.relativePath == _relativePath;
        });

    return iterator == m_Modules.end() ? nullptr : &(*iterator);
}

std::vector<std::string> ScriptInspectorLayer::discover_script_classes(
    const std::filesystem::path& _filePath)
{
    std::ifstream stream(_filePath);
    if (!stream)
        return {};

    std::vector<std::string> classes;
    std::string line;

    while (std::getline(stream, line))
    {
        const std::string cleaned = trim(line);
        if (!cleaned.starts_with("class "))
            continue;

        const std::size_t nameStart = 6;
        const std::size_t parenthesis = cleaned.find('(', nameStart);
        const std::size_t colon = cleaned.find(':', nameStart);
        const std::size_t nameEnd = std::min(
            parenthesis == std::string::npos ? cleaned.size() : parenthesis,
            colon == std::string::npos ? cleaned.size() : colon);

        const std::string className = trim(
            cleaned.substr(nameStart, nameEnd - nameStart));

        if (className.empty() || parenthesis == std::string::npos)
            continue;

        const std::size_t closeParenthesis = cleaned.find(')', parenthesis);
        if (closeParenthesis == std::string::npos)
            continue;

        const std::string bases = cleaned.substr(
            parenthesis + 1,
            closeParenthesis - parenthesis - 1);

        if (bases.find("anvil.Script") != std::string::npos ||
            bases.find("Script") != std::string::npos)
        {
            classes.push_back(className);
        }
    }

    std::sort(classes.begin(), classes.end());
    classes.erase(std::unique(classes.begin(), classes.end()), classes.end());
    return classes;
}

bool ScriptInspectorLayer::draw_script_field(
    const std::string& _name,
    ScriptField& _field)
{
    switch (_field.type)
    {
        case ScriptFieldType::Bool:
        {
            bool value = _field.value == "true" || _field.value == "1";
            if (!ImGui::Checkbox(_name.c_str(), &value))
                return false;

            _field.value = value ? "true" : "false";
            return true;
        }

        case ScriptFieldType::Int:
        {
            int value = 0;
            try
            {
                value = std::stoi(_field.value);
            }
            catch (...)
            {
            }

            if (!ImGui::DragInt(_name.c_str(), &value, 1.0f))
                return false;

            _field.value = std::to_string(value);
            return true;
        }

        case ScriptFieldType::Float:
        {
            float value = 0.0f;
            try
            {
                value = std::stof(_field.value);
            }
            catch (...)
            {
            }

            if (!ImGui::DragFloat(_name.c_str(), &value, 0.1f))
                return false;

            _field.value = std::to_string(value);
            return true;
        }

        case ScriptFieldType::String:
            return input_string(_name.c_str(), _field.value);

        default:
            ImGui::TextDisabled("%s: unsupported field type", _name.c_str());
            return false;
    }
}
