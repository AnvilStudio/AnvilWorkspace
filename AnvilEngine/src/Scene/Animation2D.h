#pragma once

#include "../Util/Serialize/Serializer.h"
#include <glm/glm.hpp>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

// A reusable .aanim file contains an optional texture UUID and ordered UV frames.
// Runtime playback is entity-owned; clip data may be shared by many entities.
namespace anv
{
    struct SpriteAnimationFrame
    {
        glm::vec2 uvMin{0.0f};
        glm::vec2 uvMax{1.0f};
        float duration = 0.1f;
        std::string textureUUID;
    };

    struct SpriteAnimationClip
    {
        std::string textureUUID;
        bool loop = true;
        std::vector<SpriteAnimationFrame> frames;

        bool Load(const std::filesystem::path& path)
        {
            frames.clear();
            if (!std::filesystem::is_regular_file(path)) return false;
            try
            {
                Serializer ser(path, Serializer::Mode::SER_MODE_TOML,
                               Serializer::Direction::Read);
                if (!ser.HasObject("Animation")) return false;
                ser.Object("Animation", [&]
                {
                    ser.FieldOr<std::string>("TextureID", textureUUID, "");
                    ser.FieldOr<bool>("Loop", loop, true);
                });
                ser.ForEachTable("Frames", [&](const std::string& name)
                {
                    // Frames are keyed with zero-padded indices so TOML ordering is stable.
                    SpriteAnimationFrame frame;
                    ser.FieldOr<float>("UVMinX", frame.uvMin.x, 0.0f);
                    ser.FieldOr<float>("UVMinY", frame.uvMin.y, 0.0f);
                    ser.FieldOr<float>("UVMaxX", frame.uvMax.x, 1.0f);
                    ser.FieldOr<float>("UVMaxY", frame.uvMax.y, 1.0f);
                    ser.FieldOr<float>("Duration", frame.duration, 0.1f);
                    ser.FieldOr<std::string>("TextureID", frame.textureUUID, "");
                    frame.duration = std::max(frame.duration, 0.001f);
                    frames.push_back(frame);
                });
                return !frames.empty();
            }
            catch (...) { frames.clear(); return false; }
        }

        bool Save(const std::filesystem::path& path) const
        {
            if (frames.empty()) return false;
            try
            {
                Serializer ser(path, Serializer::Mode::SER_MODE_TOML,
                               Serializer::Direction::Write);
                std::string texture = textureUUID;
                bool repeating = loop;
                ser.Object("Animation", [&]
                {
                    ser.Field("TextureID", texture);
                    ser.Field("Loop", repeating);
                });
                for (size_t i = 0; i < frames.size(); ++i)
                {
                    const auto& frame = frames[i];
                    const std::string index = std::to_string(i);
                    const std::string name = std::string(index.size() < 8 ? 8 - index.size() : 0, '0') + index;
                    ser.ObjectKeyed("Frames", name, [&]
                    {
                        float minX = frame.uvMin.x, minY = frame.uvMin.y;
                        float maxX = frame.uvMax.x, maxY = frame.uvMax.y;
                        float duration = frame.duration;
                        ser.Field("UVMinX", minX);
                        ser.Field("UVMinY", minY);
                        ser.Field("UVMaxX", maxX);
                        ser.Field("UVMaxY", maxY);
                        ser.Field("Duration", duration);
                        std::string frameTexture = frame.textureUUID;
                        ser.Field("TextureID", frameTexture);
                    });
                }
                ser.Close();
                return true;
            }
            catch (...) { return false; }
        }
    };
}
