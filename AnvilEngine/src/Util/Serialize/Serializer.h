#pragma once
#include "../FileSys/FileSystem.h"
#include "../../vendor/tomlplusplus/include/toml++/toml.hpp"

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace anv
{
    class Serializer
    {
    public:
        enum class Mode
        {
            SER_MODE_BINARY,
            SER_MODE_TOML
        };

        enum class Direction
        {
            Read,
            Write
        };

        Serializer(std::string path, Mode mode, Direction dir);
        ~Serializer();

        // For Write: flush to disk. For Read: no-op.
        void Close();

        Mode GetMode() const { return m_Mode; }
        Direction GetDirection() const { return m_Dir; }
        bool IsReading() const { return m_Dir == Direction::Read; }
        bool IsWriting() const { return m_Dir == Direction::Write; }

        // -------------------------
        // Core API (agnostic)
        // -------------------------

        // Primitive + string + trivially copyable structs
        template<typename T>
        void Field(const std::string& name, T& value)
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                FieldString(name, value);
            }
            else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
            {
                FieldArithmetic(name, value);
            }
            else if constexpr (std::is_trivially_copyable_v<T>)
            {
                // e.g. small POD structs (careful with endianness/padding across platforms)
                FieldTrivialBlob(name, value);
            }
            else
            {
                static_assert(sizeof(T) == 0, "Serializer::Field: unsupported type. Use Object(...) or add a specialization.");
            }
        }

        // Serialize a nested object using the same Field() calls inside.
        // Usage:
        //   ser.Object("Transform", [&] { ser.Field("X", t.x); ... });
        template<typename Fn>
        void Object(const std::string& name, Fn&& fn)
        {
            if (m_Mode == Mode::SER_MODE_TOML)
            {
                if (IsWriting())
                {
                    toml::table child;
                    PushTomlTable(name, child);
                    std::forward<Fn>(fn)();
                    PopTomlTable();
                }
                else
                {
                    PushTomlTableForRead(name);
                    std::forward<Fn>(fn)();
                    PopTomlTable();
                }
            }
            else
            {
                // Binary: write object markers + name, then fields inside.
                if (IsWriting())
                    BinWriteObjectBegin(name);
                else
                    BinReadObjectBegin(name);

                std::forward<Fn>(fn)();

                if (IsWriting())
                    BinWriteObjectEnd();
                else
                    BinReadObjectEnd();
            }
        }

        // Vector of arithmetic/enum/trivially-copyable elements
        template<typename T>
        void Vector(const std::string& name, _vec<T>& v)
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                VectorString(name, v);
            }
            else if constexpr (std::is_arithmetic_v<T> || std::is_enum_v<T>)
            {
                VectorArithmetic(name, v);
            }
            else if constexpr (std::is_trivially_copyable_v<T>)
            {
                VectorTrivialBlob(name, v);
            }
            else
            {
                static_assert(sizeof(T) == 0, "Serializer::Vector: unsupported vector element type.");
            }
        }

        // Non-throwing key read. Returns false if missing or wrong type.
        template<typename T>
        bool TryField(const std::string& name, T& out)
        {
            if (IsWriting())
            {
                Field(name, out);
                return true;
            }

            if (m_Mode == Mode::SER_MODE_TOML)
                return TomlTryReadValue(name, out);

            // Binary mode is ordered/strict in this design.
            Field(name, out);
            return true;
        }

        // Reads with fallback if missing/wrong type (TOML only). Mirrors value_or behavior.
        template<typename T>
        void FieldOr(const std::string& name, T& value, const T& fallback)
        {
            if (IsWriting())
            {
                Field(name, value);
                return;
            }

            if (m_Mode == Mode::SER_MODE_TOML)
            {
                if (!TomlTryReadValue(name, value))
                    value = fallback;
                return;
            }

            // Binary: strict ordered stream
            Field(name, value);
        }

        // Strict versions (will throw on missing/wrong type in TOML)
        template<typename T>
        void FieldStrict(const std::string& name, T& value)
        {
            if (m_Mode == Mode::SER_MODE_TOML)
            {
                if (IsWriting()) { Field(name, value); return; }
                TomlReadValue(name, value); // throws if missing/mismatch
                return;
            }

            Field(name, value);
        }

        // Optional object/table: only enters if table exists in TOML read.
        // In write mode, always creates the table.
        template<typename Fn>
        bool ObjectIf(const std::string& name, Fn&& fn)
        {
            if (m_Mode == Mode::SER_MODE_TOML)
            {
                if (IsWriting())
                {
                    toml::table child;
                    PushTomlTable(name, child);
                    std::forward<Fn>(fn)();
                    PopTomlTable();
                    return true;
                }
                else
                {
                    if (!TomlHasTable(name))
                        return false;

                    PushTomlTableForRead(name);
                    std::forward<Fn>(fn)();
                    PopTomlTable();
                    return true;
                }
            }

            // Binary: objects are strict/ordered here
            Object(name, std::forward<Fn>(fn));
            return true;
        }

        // Strict object/table: throws on missing table in TOML read.
        template<typename Fn>
        void ObjectStrict(const std::string& name, Fn&& fn)
        {
            if (m_Mode == Mode::SER_MODE_TOML)
            {
                if (IsWriting())
                {
                    toml::table child;
                    PushTomlTable(name, child);
                    std::forward<Fn>(fn)();
                    PopTomlTable();
                }
                else
                {
                    PushTomlTableForRead(name); // throws if missing
                    std::forward<Fn>(fn)();
                    PopTomlTable();
                }
                return;
            }

            Object(name, std::forward<Fn>(fn));
        }

    private:
        // -------------------------
        // TOML backend
        // -------------------------
        void TomlLoad();
        void TomlSave();

        void PushTomlTable(const std::string& name, toml::table& childOut);
        void PushTomlTableForRead(const std::string& name);
        void PopTomlTable();

        template<typename T>
        void TomlWriteValue(const std::string& name, const T& v)
        {
            (*m_TomlStack.back()).insert_or_assign(name, v);
        }

        template<typename T>
        void TomlReadValue(const std::string& name, T& out)
        {
            auto* node = (*m_TomlStack.back()).get(name);
            if (!node)
                ANV_LOG_ERROR("TOML missing key: " + name);

            if constexpr (std::is_enum_v<T>)
            {
                using U = std::underlying_type_t<T>;
                auto val = node->value<U>();
                if (!val) ANV_LOG_ERROR("TOML type mismatch for key: " + name);
                out = static_cast<T>(*val);
            }
            else
            {
                auto val = node->value<T>();
                if (!val) ANV_LOG_ERROR("TOML type mismatch for key: " + name);
                out = *val;
            }
        }

        bool TomlHasTable(const std::string& name) const;

        template<typename T>
        bool TomlTryReadValue(const std::string& name, T& out)
        {
            auto* node = (*m_TomlStack.back()).get(name);
            if (!node)
                return false;

            if constexpr (std::is_enum_v<T>)
            {
                using U = std::underlying_type_t<T>;
                auto val = node->value<U>();
                if (!val) return false;
                out = static_cast<T>(*val);
                return true;
            }
            else
            {
                auto val = node->value<T>();
                if (!val) return false;
                out = *val;
                return true;
            }
        }


        // -------------------------
        // Binary backend
        // -------------------------
        enum class BinTag : uint8_t
        {
            ObjectBegin = 1,
            ObjectEnd = 2,

            I64 = 10,
            U64 = 11,
            F64 = 12,
            Bool = 13,

            String = 20,

            Blob = 30,
            VecBlob = 31,
            VecString = 32
        };

        void BinLoad();
        void BinSave();

        void BinWriteObjectBegin(const std::string& name);
        void BinWriteObjectEnd();
        void BinReadObjectBegin(const std::string& expectedName);
        void BinReadObjectEnd();

        void BinWriteName(const std::string& name);
        std::string BinReadName();

        void BinWriteTag(BinTag t);
        BinTag BinReadTag();

        void BinWriteBytes(const void* data, size_t n);
        void BinReadBytes(void* out, size_t n);

        void BinWriteU64(uint64_t v);
        uint64_t BinReadU64();

        // -------------------------
        // Field implementations
        // -------------------------
        template<typename T>
        void FieldArithmetic(const std::string& name, T& value)
        {
            if (m_Mode == Mode::SER_MODE_TOML)
            {
                if (IsWriting())
                {
                    if constexpr (std::is_enum_v<T>)
                        TomlWriteValue(name, static_cast<std::underlying_type_t<T>>(value));
                    else
                        TomlWriteValue(name, value);
                }
                else
                {
                    if constexpr (std::is_enum_v<T>)
                        TomlReadValue(name, value);
                    else
                        TomlReadValue(name, value);
                }
                return;
            }

            // Binary
            if (IsWriting())
            {
                BinWriteName(name);

                if constexpr (std::is_same_v<T, bool>)
                {
                    BinWriteTag(BinTag::Bool);
                    uint8_t b = value ? 1 : 0;
                    BinWriteBytes(&b, 1);
                }
                else if constexpr (std::is_floating_point_v<T>)
                {
                    BinWriteTag(BinTag::F64);
                    double d = static_cast<double>(value);
                    BinWriteBytes(&d, sizeof(double));
                }
                else if constexpr (std::is_signed_v<T>)
                {
                    BinWriteTag(BinTag::I64);
                    int64_t i = static_cast<int64_t>(value);
                    BinWriteBytes(&i, sizeof(int64_t));
                }
                else
                {
                    BinWriteTag(BinTag::U64);
                    uint64_t u = static_cast<uint64_t>(value);
                    BinWriteBytes(&u, sizeof(uint64_t));
                }
            }
            else
            {
                // name is in stream; validate ordering by matching the expected name
                const std::string got = BinReadName();
                if (got != name)
                    ANV_LOG_ERROR("Binary field name mismatch. expected=" + name + " got=" + got);

                const BinTag tag = BinReadTag();

                if constexpr (std::is_same_v<T, bool>)
                {
                    if (tag != BinTag::Bool) ANV_LOG_ERROR("Binary type mismatch for: " + name);
                    uint8_t b{};
                    BinReadBytes(&b, 1);
                    value = (b != 0);
                }
                else if constexpr (std::is_floating_point_v<T>)
                {
                    if (tag != BinTag::F64) ANV_LOG_ERROR("Binary type mismatch for: " + name);
                    double d{};
                    BinReadBytes(&d, sizeof(double));
                    value = static_cast<T>(d);
                }
                else if constexpr (std::is_signed_v<T>)
                {
                    if (tag != BinTag::I64) ANV_LOG_ERROR("Binary type mismatch for: " + name);
                    int64_t i{};
                    BinReadBytes(&i, sizeof(int64_t));
                    value = static_cast<T>(i);
                }
                else
                {
                    if (tag != BinTag::U64) ANV_LOG_ERROR("Binary type mismatch for: " + name);
                    uint64_t u{};
                    BinReadBytes(&u, sizeof(uint64_t));
                    value = static_cast<T>(u);
                }
            }
        }

        void FieldString(const std::string& name, std::string& value);
        template<typename T>
        void FieldTrivialBlob(const std::string& name, T& value)
        {
            if (m_Mode == Mode::SER_MODE_TOML)
                ANV_LOG_ERROR("TOML cannot store arbitrary POD blobs directly: " + name + " (use Object/Fields instead)");

            if (IsWriting())
            {
                BinWriteName(name);
                BinWriteTag(BinTag::Blob);
                BinWriteU64(static_cast<uint64_t>(sizeof(T)));
                BinWriteBytes(&value, sizeof(T));
            }
            else
            {
                const std::string got = BinReadName();
                if (got != name)
                    ANV_LOG_ERROR("Binary field name mismatch. expected=" + name + " got=" + got);

                const BinTag tag = BinReadTag();
                if (tag != BinTag::Blob)
                    ANV_LOG_ERROR("Binary type mismatch for: " + name);

                const uint64_t sz = BinReadU64();
                if (sz != sizeof(T))
                    ANV_LOG_ERROR("Binary blob size mismatch for: " + name);

                BinReadBytes(&value, sizeof(T));
            }
        }

        template<typename T>
        void VectorArithmetic(const std::string& name, _vec<T>& v)
        {
            if (m_Mode == Mode::SER_MODE_TOML)
            {
                if (IsWriting())
                {
                    toml::array arr;
                    arr.reserve(v.size());
                    for (auto& e : v)
                    {
                        if constexpr (std::is_enum_v<T>)
                            arr.push_back(static_cast<std::underlying_type_t<T>>(e));
                        else
                            arr.push_back(e);
                    }
                    (*m_TomlStack.back()).insert_or_assign(name, std::move(arr));
                }
                else
                {
                    auto* node = (*m_TomlStack.back()).get(name);
                    if (!node || !node->is_array())
                        ANV_LOG_ERROR("TOML missing/invalid array: " + name);

                    auto& arr = *node->as_array();
                    v.clear();
                    v.reserve(arr.size());

                    for (auto& it : arr)
                    {
                        if constexpr (std::is_enum_v<T>)
                        {
                            using U = std::underlying_type_t<T>;
                            auto val = it.value<U>();
                            if (!val) ANV_LOG_ERROR("TOML array type mismatch: " + name);
                            v.push_back(static_cast<T>(*val));
                        }
                        else
                        {
                            auto val = it.value<T>();
                            if (!val) ANV_LOG_ERROR("TOML array type mismatch: " + name);
                            v.push_back(*val);
                        }
                    }
                }
                return;
            }

            // Binary: write as VecBlob of element bytes (works for arithmetic/enum)
            if (IsWriting())
            {
                BinWriteName(name);
                BinWriteTag(BinTag::VecBlob);
                BinWriteU64(static_cast<uint64_t>(sizeof(T)));
                BinWriteU64(static_cast<uint64_t>(v.size()));
                if (!v.empty())
                    BinWriteBytes(v.data(), sizeof(T) * v.size());
            }
            else
            {
                const std::string got = BinReadName();
                if (got != name)
                    ANV_LOG_ERROR("Binary vector name mismatch. expected=" + name + " got=" + got);

                const BinTag tag = BinReadTag();
                if (tag != BinTag::VecBlob)
                    ANV_LOG_ERROR("Binary type mismatch for vector: " + name);

                const uint64_t elemSz = BinReadU64();
                if (elemSz != sizeof(T))
                    ANV_LOG_ERROR("Binary vector element size mismatch for: " + name);

                const uint64_t count = BinReadU64();
                v.resize(static_cast<size_t>(count));
                if (count > 0)
                    BinReadBytes(v.data(), sizeof(T) * static_cast<size_t>(count));
            }
        }

        void VectorString(const std::string& name, _vec<std::string>& v);

        template<typename T>
        void VectorTrivialBlob(const std::string& name, _vec<T>& v)
        {
            // Binary only; for TOML, you should represent as Object/Fields or specific arrays.
            if (m_Mode == Mode::SER_MODE_TOML)
                ANV_LOG_ERROR("TOML cannot store arbitrary POD vector blobs directly: " + name);

            if (IsWriting())
            {
                BinWriteName(name);
                BinWriteTag(BinTag::VecBlob);
                BinWriteU64(static_cast<uint64_t>(sizeof(T)));
                BinWriteU64(static_cast<uint64_t>(v.size()));
                if (!v.empty())
                    BinWriteBytes(v.data(), sizeof(T) * v.size());
            }
            else
            {
                const std::string got = BinReadName();
                if (got != name)
                    ANV_LOG_ERROR("Binary vector name mismatch. expected=" + name + " got=" + got);

                const BinTag tag = BinReadTag();
                if (tag != BinTag::VecBlob)
                    ANV_LOG_ERROR("Binary type mismatch for vector: " + name);

                const uint64_t elemSz = BinReadU64();
                if (elemSz != sizeof(T))
                    ANV_LOG_ERROR("Binary vector element size mismatch for: " + name);

                const uint64_t count = BinReadU64();
                v.resize(static_cast<size_t>(count));
                if (count > 0)
                    BinReadBytes(v.data(), sizeof(T) * static_cast<size_t>(count));
            }
        }

    private:
        std::string m_Path;
        Mode m_Mode{};
        Direction m_Dir{};

        // TOML state
        toml::table m_TomlRoot{};
        _vec<toml::table*> m_TomlStack{};

        // Binary state
        _vec<uint8_t> m_BinBuffer{};
        size_t m_BinCursor = 0;

        bool m_Closed = false;
    };
}