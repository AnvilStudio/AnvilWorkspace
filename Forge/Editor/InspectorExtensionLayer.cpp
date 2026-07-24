#include "InspectorExtensionLayer.h"

#include "EditorHelpers.h"
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

    std::string to_lower(std::string _value)
    {
        std::transform(
            _value.begin(),
            _value.end(),
            _value.begin(),
            [](unsigned char _character)
            {
                return static_cast<char>(std::tolower(_character));
            });

        return _value;
    }

    bool is_python_path(const std::filesystem::path& _path)
    {
        return to_lower(_path.extension().string()) == ".py";
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

InspectorExtensionLayer::InspectorExtensionLayer(EditorLayer* _editorLayer)
    : Layer("Inspector Extension Layer"),
      m_EditorLayer(_editorLayer)
{
    refresh_script_modules();
}

void InspectorExtensionLayer::OnImGuiRender()
{
    if (!m_EditorLayer)
        return;

    Ref<Scene> scene =
        App::GetInstance()->GetSceneManager()->GetActive();

    if (!scene)
        return;

    const bool playMode =
        m_EditorLayer->GetSceneState() == EditorLayer::SceneState::Play;
    scene->SetScriptExecutionEnabled(playMode);

    const entt::entity entity = m_EditorLayer->GetSelectedEntity();
    if (entity == entt::null || !scene->Registry().valid(entity))
        return;

    ImGui::Begin("Inspector");

    if (scene->HasComponent<Component::Script>(entity))
    {
        const std::string previousModule =
            scene->GetComponent<Component::Script>(entity).modulePath;

        draw_component<Component::Script>(
            "Python Script",
            entity,
            scene,
            [&](Component::Script& _script)
            {
                draw_script_component(scene, entity, _script);
            });

        if (!scene->HasComponent<Component::Script>(entity))
        {
            if (!previousModule.empty())
                PythonScriptEngine::RequestReload(previousModule);

            scene->Save();
        }
    }

    draw_add_component_menu(scene, entity);
    draw_inspector_script_drop_target(scene, entity);

    ImGui::End();
}

void InspectorExtensionLayer::draw_script_component(
    Ref<Scene> _scene,
    entt::entity _entity,
    Component::Script& _script)
{
    (void)_entity;

    bool changed = false;

    if (m_Modules.empty())
        refresh_script_modules();

    if (ImGui::BeginTable(
            "PythonScriptProps",
            2,
            ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn(
            "Label",
            ImGuiTableColumnFlags_WidthFixed,
            90.0f);
        ImGui::TableSetupColumn(
            "Value",
            ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Script");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);

        const char* modulePreview = _script.modulePath.empty()
            ? "Select Python script"
            : _script.modulePath.c_str();

        if (ImGui::BeginCombo("##ScriptModule", modulePreview))
        {
            for (const ScriptModuleInfo& module : m_Modules)
            {
                const bool selected = module.relativePath == _script.modulePath;
                if (ImGui::Selectable(module.relativePath.c_str(), selected))
                {
                    _script.modulePath = module.relativePath;
                    _script.className.clear();
                    _script.fields.clear();

                    if (module.classes.size() == 1)
                        _script.className = module.classes.front();

                    changed = true;
                }

                if (selected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        const ScriptModuleInfo* selectedModule = find_module(_script.modulePath);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Class");
        ImGui::TableSetColumnIndex(1);
        ImGui::SetNextItemWidth(-1.0f);

        const char* classPreview = _script.className.empty()
            ? "Select anvil.Script class"
            : _script.className.c_str();

        ImGui::BeginDisabled(selectedModule == nullptr);
        if (ImGui::BeginCombo("##ScriptClass", classPreview))
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

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Enabled");
        ImGui::TableSetColumnIndex(1);
        changed |= ImGui::Checkbox("##ScriptEnabled", &_script.enabled);

        if (!_script.fields.empty())
        {
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
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(name.c_str());
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-1.0f);

                ImGui::PushID(name.c_str());
                changed |= draw_script_field("##Value", _script.fields.at(name));
                ImGui::PopID();
            }
        }

        ImGui::EndTable();
    }

    const ScriptModuleInfo* selectedModule = find_module(_script.modulePath);
    if (selectedModule && selectedModule->classes.empty())
    {
        ImGui::TextDisabled(
            "No classes deriving from anvil.Script were found in this file.");
    }

    if (m_Modules.empty())
    {
        ImGui::TextDisabled(
            "No Python files found in: %s",
            m_ScriptsDirectory.string().c_str());
    }

    if (ImGui::Button("Reload"))
        PythonScriptEngine::RequestReload(_script.modulePath);

    ImGui::SameLine();
    if (ImGui::Button("Refresh Scripts"))
        refresh_script_modules();

    if (changed)
    {
        _scene->Save();
        PythonScriptEngine::RequestReload(_script.modulePath);
    }
}

void InspectorExtensionLayer::draw_add_component_menu(
    Ref<Scene> _scene,
    entt::entity _entity)
{
    const float width = ImGui::GetContentRegionAvail().x;
    if (ImGui::Button("Add Component", ImVec2(width, 0.0f)))
        ImGui::OpenPopup("InspectorExtensionAddComponentPopup");

    if (!ImGui::BeginPopup("InspectorExtensionAddComponentPopup"))
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
        refresh_script_modules();
        _scene->Save();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void InspectorExtensionLayer::draw_inspector_script_drop_target(
    Ref<Scene> _scene,
    entt::entity _entity)
{
    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (!activePayload || !activePayload->IsDataType("ANV_ASSET_PATH"))
        return;

    const char* activePathData =
        static_cast<const char*>(activePayload->Data);
    if (!activePathData || !is_python_path(std::filesystem::path(activePathData)))
        return;

    const ImVec2 previousCursor = ImGui::GetCursorScreenPos();
    const ImVec2 windowPosition = ImGui::GetWindowPos();
    const ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    const ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

    const ImVec2 targetPosition(
        windowPosition.x + contentMin.x,
        windowPosition.y + contentMin.y);
    const ImVec2 targetSize(
        contentMax.x - contentMin.x,
        contentMax.y - contentMin.y);

    ImGui::SetCursorScreenPos(targetPosition);
    ImGui::InvisibleButton("##InspectorPythonDropTarget", targetSize);

    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload =
                ImGui::AcceptDragDropPayload("ANV_ASSET_PATH"))
        {
            const char* pathData = static_cast<const char*>(payload->Data);
            if (pathData)
                assign_script(_scene, _entity, std::filesystem::path(pathData));
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::SetCursorScreenPos(previousCursor);
}

void InspectorExtensionLayer::assign_script(
    Ref<Scene> _scene,
    entt::entity _entity,
    const std::filesystem::path& _path)
{
    if (!is_python_path(_path))
        return;

    refresh_script_modules();

    std::error_code error;
    const std::filesystem::path relativePath =
        std::filesystem::relative(_path, m_ScriptsDirectory, error);

    if (error || relativePath.empty() || relativePath.string().starts_with(".."))
    {
        ANV_LOG_ERROR(
            "Python scripts must be inside the project Scripts directory: %s",
            _path.string().c_str());
        return;
    }

    auto& script = _scene->AddComponent<Component::Script>(_entity);
    script.modulePath = relativePath.generic_string();
    script.className.clear();
    script.fields.clear();

    if (const ScriptModuleInfo* module = find_module(script.modulePath))
    {
        if (module->classes.size() == 1)
            script.className = module->classes.front();
    }

    _scene->Save();
    PythonScriptEngine::RequestReload(script.modulePath);
}

void InspectorExtensionLayer::refresh_script_modules()
{
    m_Modules.clear();
    m_ScriptsDirectory = get_scripts_directory();

    if (m_ScriptsDirectory.empty() ||
        !std::filesystem::is_directory(m_ScriptsDirectory))
    {
        return;
    }

    std::error_code error;
    std::filesystem::recursive_directory_iterator iterator(
        m_ScriptsDirectory,
        std::filesystem::directory_options::skip_permission_denied,
        error);

    for (const auto& entry : iterator)
    {
        if (error)
        {
            error.clear();
            continue;
        }

        if (!entry.is_regular_file() || !is_python_path(entry.path()) ||
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

std::filesystem::path InspectorExtensionLayer::get_scripts_directory() const
{
    const std::filesystem::path assetsDirectory =
        App::GetInstance()->GetFS().GetKeyVal("Assets");

    if (!assetsDirectory.empty())
    {
        const std::filesystem::path scriptsDirectory =
            assetsDirectory.parent_path() / "Scripts";

        if (std::filesystem::is_directory(scriptsDirectory))
            return scriptsDirectory;
    }

    const std::filesystem::path workingDirectory =
        std::filesystem::current_path() / "Scripts";

    if (std::filesystem::is_directory(workingDirectory))
        return workingDirectory;

    return {};
}

const InspectorExtensionLayer::ScriptModuleInfo*
InspectorExtensionLayer::find_module(const std::string& _relativePath) const
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

std::vector<std::string> InspectorExtensionLayer::discover_script_classes(
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

bool InspectorExtensionLayer::draw_script_field(
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
            ImGui::TextDisabled("Unsupported field type");
            return false;
    }
}