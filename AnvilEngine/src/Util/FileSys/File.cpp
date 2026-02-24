#include "File.h"

namespace anv
{
    anv::File::File(std::string _path)
        : m_Path(_path)
    {
    }

    File::~File() = default;

    void File::MarkForDelete()
    {
        m_DeleteRequested.store(true, std::memory_order_release);
    }

    bool File::DeleteNow()
    {
        std::error_code ec;
        const bool removed = std::filesystem::remove(std::filesystem::path(m_Path), ec);

        if (!removed)
        {
            if (ec)
                ANV_LOG_WARN("Failed to delete file '%s': %s", m_Path.c_str(), ec.message().c_str());
            return false;
        }
        return true;
    }

    size_t anv::File::GetFileSizeBytes(FILE* _f)
    {
        if (!_f) return 0;

        const long cur = std::ftell(_f);
        std::fseek(_f, 0, SEEK_END);
        const long end = std::ftell(_f);
        std::fseek(_f, cur, SEEK_SET);

        return (end < 0) ? 0u : static_cast<size_t>(end);
    }

    size_t File::Size() const
    {
        std::error_code ec;
        const auto sz = std::filesystem::file_size(std::filesystem::path(m_Path), ec);
        if (ec) return 0;
        return static_cast<size_t>(sz);
    }

    bool File::Exists() const
    {
        std::error_code ec;
        return std::filesystem::exists(std::filesystem::path(m_Path), ec) && !ec;
    }

    bool File::CreateIfMissing(bool binary) const
    {
        if (Exists())
            return true;

        // Ensure parent exists
        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(m_Path).parent_path(), ec);

        FILE* f = std::fopen(m_Path.c_str(), binary ? "wb" : "w");
        if (!f)
        {
            ANV_LOG_ERROR("File::CreateIfMissing - failed to create: " + m_Path);
            return false;
        }

        std::fclose(f);
        return true;
    }

    _vec<std::string> File::Read() const
    {
        _vec<std::string> lines;

        std::ifstream in(m_Path);
        if (!in.is_open())
        {
            ANV_LOG_ERROR("File::Read - failed to open: " + m_Path);
            return lines;
        }

        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(std::move(line));
        }

        return lines;
    }

    void anv::File::Write(const std::string& _str) const
    {
        std::ofstream out(m_Path, std::ios::trunc);
        if (!out.is_open())
        {
            ANV_LOG_ERROR("File::Write - failed to open: " + m_Path);
            return;
        }
        out << _str;
    }
}