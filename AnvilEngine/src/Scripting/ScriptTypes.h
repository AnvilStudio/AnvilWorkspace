#pragma once

#include "Util/Serialize/Serializer.h"

#include <string>
#include <unordered_map>

namespace anv
{
    enum class ScriptFieldType
    {
        None = 0,
        Bool,
        Int,
        Float,
        String
    };

    inline const char* ScriptFieldTypeToString(ScriptFieldType _type)
    {
        switch (_type)
        {
            case ScriptFieldType::Bool:   return "Bool";
            case ScriptFieldType::Int:    return "Int";
            case ScriptFieldType::Float:  return "Float";
            case ScriptFieldType::String: return "String";
            default:                      return "None";
        }
    }

    inline bool ScriptFieldTypeFromString(const std::string& _value, ScriptFieldType& _out)
    {
        if (_value == "Bool")   { _out = ScriptFieldType::Bool; return true; }
        if (_value == "Int")    { _out = ScriptFieldType::Int; return true; }
        if (_value == "Float")  { _out = ScriptFieldType::Float; return true; }
        if (_value == "String") { _out = ScriptFieldType::String; return true; }
        if (_value == "None")   { _out = ScriptFieldType::None; return true; }
        return false;
    }

    struct ScriptField
    {
        ScriptFieldType type = ScriptFieldType::None;
        std::string value;

        void Serialize(Serializer& _ser)
        {
            _ser.EnumFieldOr(
                "Type",
                type,
                ScriptFieldType::None,
                ScriptFieldTypeToString,
                ScriptFieldTypeFromString);
            _ser.Field("Value", value);
        }

        void Deserialize(Serializer& _ser)
        {
            _ser.EnumFieldOr(
                "Type",
                type,
                ScriptFieldType::None,
                ScriptFieldTypeToString,
                ScriptFieldTypeFromString);
            _ser.FieldOr<std::string>("Value", value, "");
        }
    };

    using ScriptFieldMap = std::unordered_map<std::string, ScriptField>;
}
