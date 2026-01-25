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
                toml_load();
        }

        else
        {
            if (IsReading())
                bin_load();
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
                toml_save();
            else
                bin_save();
        }
    }

    // -------------------------
    // TOML backend
    // -------------------------

    void Serializer::toml_load()
    {
        // Read whole file and parse as TOML
        File f(m_Path);
        const auto bytes = f.ReadAs<uint8_t>();
        const std::string text(reinterpret_cast<const char*>(bytes.data()), bytes.size());

        m_TomlRoot = toml::parse(text);
        m_TomlStack.clear();
        m_TomlStack.push_back(&m_TomlRoot);
    }

    void Serializer::toml_save()
    {
        std::ostringstream oss;
        oss << m_TomlRoot;   // toml++ stream serializer

        File f(m_Path);
        f.Write(oss.str());
    }

    void Serializer::push_toml_table(const std::string& name, toml::table& childOut)
    {
        // insert and then push pointer to it
        (*m_TomlStack.back()).insert_or_assign(name, childOut);

        auto* node = (*m_TomlStack.back()).get(name);
        if (!node || !node->is_table())
            ANV_LOG_ERROR("TOML failed to create table: " + name);

        m_TomlStack.push_back(node->as_table());
    }

    void Serializer::push_toml_table_for_read(const std::string& name)
    {
        auto* node = (*m_TomlStack.back()).get(name);
        if (!node || !node->is_table())
        ANV_LOG_ERROR("TOML missing table: " + name);

        m_TomlStack.push_back(node->as_table());
    }

    void Serializer::pop_toml_table()
    {
        if (m_TomlStack.size() <= 1)
            ANV_LOG_ERROR("TOML stack underflow");
        m_TomlStack.pop_back();
    }

    bool Serializer::toml_has_table(const std::string& name) const
    {
        auto* node = (*m_TomlStack.back()).get(name);
        return node && node->is_table();
    }

    void anv::Serializer::push_or_create_toml_table(const std::string& name)
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

    void anv::Serializer::push_or_create_toml_keyed_table(const std::string& key)
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

    void anv::Serializer::push_toml_table_for_read_keyed(toml::table& parent, const std::string& key)
    {
        auto* node = parent.get(key);
        if (!node || !node->is_table())
            throw std::runtime_error("TOML missing keyed table: " + key);

        m_TomlStack.push_back(node->as_table());
    }


    // -------------------------
    // Binary backend
    // -------------------------

    void Serializer::bin_load()
    {
        File f(m_Path);
        m_BinBuffer = f.ReadAs<uint8_t>();
        m_BinCursor = 0;
    }

    void Serializer::bin_save()
    {
        File f(m_Path);
        f.WriteAs<uint8_t>(m_BinBuffer);
    }

    void Serializer::bin_write_bytes(const void* data, size_t n)
    {
        const auto* b = reinterpret_cast<const uint8_t*>(data);
        m_BinBuffer.insert(m_BinBuffer.end(), b, b + n);
    }

    void Serializer::bin_read_bytes(void* out, size_t n)
    {
        if (m_BinCursor + n > m_BinBuffer.size())
            ANV_LOG_ERROR("Binary read out of bounds");

        std::memcpy(out, m_BinBuffer.data() + m_BinCursor, n);
        m_BinCursor += n;
    }

    void Serializer::bin_write_u64(uint64_t v)
    {
        bin_write_bytes(&v, sizeof(uint64_t));
    }

    uint64_t Serializer::bin_read_u64()
    {
        uint64_t v{};
        bin_read_bytes(&v, sizeof(uint64_t));
        return v;
    }

    void Serializer::bin_write_tag(BinTag t)
    {
        uint8_t b = static_cast<uint8_t>(t);
        bin_write_bytes(&b, 1);
    }

    Serializer::BinTag Serializer::bin_read_tag()
    {
        uint8_t b{};
        bin_read_bytes(&b, 1);
        return static_cast<BinTag>(b);
    }

    void Serializer::bin_write_name(const std::string& name)
    {
        bin_write_u64(static_cast<uint64_t>(name.size()));
        if (!name.empty())
            bin_write_bytes(name.data(), name.size());
    }

    std::string Serializer::bin_read_name()
    {
        const uint64_t len = bin_read_u64();
        std::string s;
        s.resize(static_cast<size_t>(len));
        if (len > 0)
            bin_read_bytes(s.data(), static_cast<size_t>(len));
        return s;
    }

    void Serializer::bin_write_obj_begin(const std::string& name)
    {
        bin_write_tag(BinTag::ObjectBegin);
        bin_write_name(name);
    }

    void Serializer::bin_write_obj_end()
    {
        bin_write_tag(BinTag::ObjectEnd);
    }

    void Serializer::bin_read_obj_begin(const std::string& expectedName)
    {
        const BinTag tag = bin_read_tag();
        if (tag != BinTag::ObjectBegin)
            ANV_LOG_ERROR("Binary expected ObjectBegin");

        const std::string got = bin_read_name();
        if (got != expectedName)
            ANV_LOG_ERROR("Binary object name mismatch. expected=" + expectedName + " got=" + got);
    }

    void Serializer::bin_read_obj_end()
    {
        const BinTag tag = bin_read_tag();
        if (tag != BinTag::ObjectEnd)
            ANV_LOG_ERROR("Binary expected ObjectEnd");
    }

    // -------------------------
    // String + string vector
    // -------------------------

    void Serializer::field_str(const std::string& name, std::string& value)
    {
        if (m_Mode == Mode::SER_MODE_TOML)
        {
            if (IsWriting()) TomlWriteValue(name, value);
            else toml_read_value(name, value);
            return;
        }

        if (IsWriting())
        {
            bin_write_name(name);
            bin_write_tag(BinTag::String);
            bin_write_u64(static_cast<uint64_t>(value.size()));
            if (!value.empty())
                bin_write_bytes(value.data(), value.size());
        }
        else
        {
            const std::string got = bin_read_name();
            if (got != name)
                ANV_LOG_ERROR("Binary field name mismatch. expected=" + name + " got=" + got);

            const BinTag tag = bin_read_tag();
            if (tag != BinTag::String)
                ANV_LOG_ERROR("Binary type mismatch for: " + name);

            const uint64_t len = bin_read_u64();
            value.resize(static_cast<size_t>(len));
            if (len > 0)
                bin_read_bytes(value.data(), static_cast<size_t>(len));
        }
    }

    void Serializer::vec_string(const std::string& name, _vec<std::string>& v)
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
            bin_write_name(name);
            bin_write_tag(BinTag::VecString);
            bin_write_u64(static_cast<uint64_t>(v.size()));
            for (auto& s : v)
            {
                bin_write_u64(static_cast<uint64_t>(s.size()));
                if (!s.empty())
                    bin_write_bytes(s.data(), s.size());
            }
        }
        else
        {
            const std::string got = bin_read_name();
            if (got != name)
                ANV_LOG_ERROR("Binary vector name mismatch. expected=" + name + " got=" + got);

            const BinTag tag = bin_read_tag();
            if (tag != BinTag::VecString)
                ANV_LOG_ERROR("Binary type mismatch for vector: " + name);

            const uint64_t count = bin_read_u64();
            v.clear();
            v.reserve(static_cast<size_t>(count));

            for (uint64_t i = 0; i < count; ++i)
            {
                const uint64_t len = bin_read_u64();
                std::string s;
                s.resize(static_cast<size_t>(len));
                if (len > 0)
                    bin_read_bytes(s.data(), static_cast<size_t>(len));
                v.push_back(std::move(s));
            }
        }
    }
}
