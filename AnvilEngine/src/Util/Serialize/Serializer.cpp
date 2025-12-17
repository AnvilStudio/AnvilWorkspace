#include "Serializer.h"

#include <cstring>
#include <filesystem>

namespace anv
{
    Serializer::Serializer(std::string path, Mode mode, Direction dir)
        : m_Path(std::move(path)), m_Mode(mode), m_Dir(dir)
    {
        ANV_PROFILE_SCOPE()
        ANV_LOG_INFO("Opening Serializable File %s\n - In Mode %i\n - with direction %i", m_Path.c_str(), mode, dir)

        if (m_Mode == Mode::SER_MODE_TOML)
        {
            m_TomlStack.clear();
            m_TomlStack.push_back(&m_TomlRoot);

            if (IsReading())
                TomlLoad();
        }

        else
        {
            if (IsReading())
                BinLoad();
        }
    }

    Serializer::~Serializer()
    {
        try { Close(); }
        catch (...) { /* don’t throw in dtor */ }
        ANV_LOG_DEBUG("Closing Serializable File %s", m_Path.c_str())
    }

    void Serializer::Close()
    {
        if (m_Closed) return;
        m_Closed = true;

        if (IsWriting())
        {
            if (m_Mode == Mode::SER_MODE_TOML)
                TomlSave();
            else
                BinSave();
        }
    }

    // -------------------------
    // TOML backend
    // -------------------------

    void Serializer::TomlLoad()
    {
        // Read whole file and parse as TOML
        File f(m_Path);
        const auto bytes = f.ReadAs<uint8_t>();
        const std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());

        m_TomlRoot = toml::parse(text);
        m_TomlStack.clear();
        m_TomlStack.push_back(&m_TomlRoot);
    }

    void Serializer::TomlSave()
    {
        std::ostringstream oss;
        oss << m_TomlRoot;   // toml++ stream serializer

        File f(m_Path);
        f.Write(oss.str());
    }

    void Serializer::PushTomlTable(const std::string& name, toml::table& childOut)
    {
        // insert and then push pointer to it
        (*m_TomlStack.back()).insert_or_assign(name, childOut);

        auto* node = (*m_TomlStack.back()).get(name);
        if (!node || !node->is_table())
            ANV_LOG_ERROR("TOML failed to create table: " + name);

        m_TomlStack.push_back(node->as_table());
    }

    void Serializer::PushTomlTableForRead(const std::string& name)
    {
        auto* node = (*m_TomlStack.back()).get(name);
        if (!node || !node->is_table())
        ANV_LOG_ERROR("TOML missing table: " + name);

        m_TomlStack.push_back(node->as_table());
    }

    void Serializer::PopTomlTable()
    {
        if (m_TomlStack.size() <= 1)
            ANV_LOG_ERROR("TOML stack underflow");
        m_TomlStack.pop_back();
    }

    bool Serializer::TomlHasTable(const std::string& name) const
    {
        auto* node = (*m_TomlStack.back()).get(name);
        return node && node->is_table();
    }

    void anv::Serializer::PushOrCreateTomlTable(const std::string& name)
    {
        // current stack top is the table we’re writing into
        auto& cur = *m_TomlStack.back();

        auto* node = cur.get(name);
        if (!node)
        {
            cur.insert_or_assign(name, toml::table{});
            node = cur.get(name);
        }

        if (!node->is_table())
            throw std::runtime_error("TOML key exists but is not a table: " + name);

        m_TomlStack.push_back(node->as_table());
    }

    void anv::Serializer::PushOrCreateTomlKeyedTable(const std::string& key)
    {
        auto& cur = *m_TomlStack.back();

        auto* node = cur.get(key);
        if (!node)
        {
            cur.insert_or_assign(key, toml::table{});
            node = cur.get(key);
        }

        if (!node->is_table())
            throw std::runtime_error("TOML key exists but is not a table: " + key);

        m_TomlStack.push_back(node->as_table());
    }

    void anv::Serializer::PushTomlTableForReadKeyed(toml::table& parent, const std::string& key)
    {
        auto* node = parent.get(key);
        if (!node || !node->is_table())
            throw std::runtime_error("TOML missing keyed table: " + key);

        m_TomlStack.push_back(node->as_table());
    }


    // -------------------------
    // Binary backend
    // -------------------------

    void Serializer::BinLoad()
    {
        File f(m_Path);
        m_BinBuffer = f.ReadAs<uint8_t>();
        m_BinCursor = 0;
    }

    void Serializer::BinSave()
    {
        File f(m_Path);
        f.WriteAs<uint8_t>(m_BinBuffer);
    }

    void Serializer::BinWriteBytes(const void* data, size_t n)
    {
        const auto* b = reinterpret_cast<const uint8_t*>(data);
        m_BinBuffer.insert(m_BinBuffer.end(), b, b + n);
    }

    void Serializer::BinReadBytes(void* out, size_t n)
    {
        if (m_BinCursor + n > m_BinBuffer.size())
            ANV_LOG_ERROR("Binary read out of bounds");

        std::memcpy(out, m_BinBuffer.data() + m_BinCursor, n);
        m_BinCursor += n;
    }

    void Serializer::BinWriteU64(uint64_t v)
    {
        BinWriteBytes(&v, sizeof(uint64_t));
    }

    uint64_t Serializer::BinReadU64()
    {
        uint64_t v{};
        BinReadBytes(&v, sizeof(uint64_t));
        return v;
    }

    void Serializer::BinWriteTag(BinTag t)
    {
        uint8_t b = static_cast<uint8_t>(t);
        BinWriteBytes(&b, 1);
    }

    Serializer::BinTag Serializer::BinReadTag()
    {
        uint8_t b{};
        BinReadBytes(&b, 1);
        return static_cast<BinTag>(b);
    }

    void Serializer::BinWriteName(const std::string& name)
    {
        BinWriteU64(static_cast<uint64_t>(name.size()));
        if (!name.empty())
            BinWriteBytes(name.data(), name.size());
    }

    std::string Serializer::BinReadName()
    {
        const uint64_t len = BinReadU64();
        std::string s;
        s.resize(static_cast<size_t>(len));
        if (len > 0)
            BinReadBytes(s.data(), static_cast<size_t>(len));
        return s;
    }

    void Serializer::BinWriteObjectBegin(const std::string& name)
    {
        BinWriteTag(BinTag::ObjectBegin);
        BinWriteName(name);
    }

    void Serializer::BinWriteObjectEnd()
    {
        BinWriteTag(BinTag::ObjectEnd);
    }

    void Serializer::BinReadObjectBegin(const std::string& expectedName)
    {
        const BinTag tag = BinReadTag();
        if (tag != BinTag::ObjectBegin)
            ANV_LOG_ERROR("Binary expected ObjectBegin");

        const std::string got = BinReadName();
        if (got != expectedName)
            ANV_LOG_ERROR("Binary object name mismatch. expected=" + expectedName + " got=" + got);
    }

    void Serializer::BinReadObjectEnd()
    {
        const BinTag tag = BinReadTag();
        if (tag != BinTag::ObjectEnd)
            ANV_LOG_ERROR("Binary expected ObjectEnd");
    }

    // -------------------------
    // String + string vector
    // -------------------------

    void Serializer::FieldString(const std::string& name, std::string& value)
    {
        if (m_Mode == Mode::SER_MODE_TOML)
        {
            if (IsWriting()) TomlWriteValue(name, value);
            else TomlReadValue(name, value);
            return;
        }

        if (IsWriting())
        {
            BinWriteName(name);
            BinWriteTag(BinTag::String);
            BinWriteU64(static_cast<uint64_t>(value.size()));
            if (!value.empty())
                BinWriteBytes(value.data(), value.size());
        }
        else
        {
            const std::string got = BinReadName();
            if (got != name)
                ANV_LOG_ERROR("Binary field name mismatch. expected=" + name + " got=" + got);

            const BinTag tag = BinReadTag();
            if (tag != BinTag::String)
                ANV_LOG_ERROR("Binary type mismatch for: " + name);

            const uint64_t len = BinReadU64();
            value.resize(static_cast<size_t>(len));
            if (len > 0)
                BinReadBytes(value.data(), static_cast<size_t>(len));
        }
    }

    void Serializer::VectorString(const std::string& name, _vec<std::string>& v)
    {
        if (m_Mode == Mode::SER_MODE_TOML)
        {
            if (IsWriting())
            {
                toml::array arr;
                arr.reserve(v.size());
                for (auto& s : v) arr.push_back(s);
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
                    auto val = it.value<std::string>();
                    if (!val) ANV_LOG_ERROR("TOML array type mismatch: " + name);
                    v.push_back(*val);
                }
            }
            return;
        }

        if (IsWriting())
        {
            BinWriteName(name);
            BinWriteTag(BinTag::VecString);
            BinWriteU64(static_cast<uint64_t>(v.size()));
            for (auto& s : v)
            {
                BinWriteU64(static_cast<uint64_t>(s.size()));
                if (!s.empty())
                    BinWriteBytes(s.data(), s.size());
            }
        }
        else
        {
            const std::string got = BinReadName();
            if (got != name)
                ANV_LOG_ERROR("Binary vector name mismatch. expected=" + name + " got=" + got);

            const BinTag tag = BinReadTag();
            if (tag != BinTag::VecString)
                ANV_LOG_ERROR("Binary type mismatch for vector: " + name);

            const uint64_t count = BinReadU64();
            v.clear();
            v.reserve(static_cast<size_t>(count));

            for (uint64_t i = 0; i < count; ++i)
            {
                const uint64_t len = BinReadU64();
                std::string s;
                s.resize(static_cast<size_t>(len));
                if (len > 0)
                    BinReadBytes(s.data(), static_cast<size_t>(len));
                v.push_back(std::move(s));
            }
        }
    }
}
