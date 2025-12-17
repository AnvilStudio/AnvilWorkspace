#include "FileSystem.h"

#include <cstring>   // std::strlen
#include <utility>   // std::move

namespace anv
{
    File::File(std::string path)
        : m_Path(std::move(path))
    {
        // Keep a handle open (read-binary) mainly to validate existence and allow quick Size().
        // Read/Write functions open their own handles with correct modes.
        m_File = std::fopen(m_Path.c_str(), "rb");
        if (!m_File)
            ANV_LOG_ERROR("File::File - failed to open file: " + m_Path);
    }

    File::~File()
    {
        if (m_File)
        {
            std::fclose(m_File);
            m_File = nullptr;
        }
    }

    File::File(File&& other) noexcept
        : m_Path(std::move(other.m_Path)), m_File(other.m_File)
    {
        other.m_File = nullptr;
    }

    File& File::operator=(File&& other) noexcept
    {
        if (this == &other)
            return *this;

        if (m_File)
            std::fclose(m_File);

        m_Path = std::move(other.m_Path);
        m_File = other.m_File;
        other.m_File = nullptr;

        return *this;
    }

    size_t File::GetFileSizeBytes(FILE* f)
    {
        if (!f) return 0;

        const long cur = std::ftell(f);
        if (cur < 0)
            ANV_LOG_ERROR("File::GetFileSizeBytes - ftell failed");

        if (std::fseek(f, 0, SEEK_END) != 0)
            ANV_LOG_ERROR("File::GetFileSizeBytes - fseek(SEEK_END) failed");

        const long end = std::ftell(f);
        if (end < 0)
            ANV_LOG_ERROR("File::GetFileSizeBytes - ftell(end) failed");

        if (std::fseek(f, cur, SEEK_SET) != 0)
            ANV_LOG_ERROR("File::GetFileSizeBytes - fseek(SEEK_SET) failed");

        return static_cast<size_t>(end);
    }

    size_t File::Size() const
    {
        // Use the persistent handle if it exists; otherwise open a temp one.
        if (m_File)
            return GetFileSizeBytes(m_File);

        FILE* f = std::fopen(m_Path.c_str(), "rb");
        if (!f)
            ANV_LOG_ERROR("File::Size - failed to open file: " + m_Path);

        const size_t bytes = GetFileSizeBytes(f);
        std::fclose(f);
        return bytes;
    }

    _vec<std::string> File::Read() const
    {
        FILE* f = std::fopen(m_Path.c_str(), "rb");
        if (!f)
            ANV_LOG_ERROR("File::Read - failed to open file: " + m_Path);

        const size_t bytes = GetFileSizeBytes(f);
        std::string content;
        content.resize(bytes);

        if (bytes > 0)
        {
            const size_t readBytes = std::fread(content.data(), 1, bytes, f);
            std::fclose(f);

            if (readBytes != bytes)
                ANV_LOG_ERROR("File::Read - failed to read entire file: " + m_Path);
        }
        else
        {
            std::fclose(f);
        }

        _vec<std::string> lines;
        std::string line;
        line.reserve(256);

        for (size_t i = 0; i < content.size(); ++i)
        {
            const char c = content[i];

            if (c == '\n')
            {
                // strip optional '\r'
                if (!line.empty() && line.back() == '\r')
                    line.pop_back();

                lines.push_back(line);
                line.clear();
            }
            else
            {
                line.push_back(c);
            }
        }

        // last line (even if file doesn't end with \n)
        if (!line.empty() || (!content.empty() && content.back() == '\n' == false))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            lines.push_back(line);
        }

        return lines;
    }

    void File::Write(const std::string& str) const
    {
        FILE* f = std::fopen(m_Path.c_str(), "wb");
        if (!f)
            ANV_LOG_ERROR("File::Write - failed to open file: " + m_Path);

        if (!str.empty())
        {
            const size_t written = std::fwrite(str.data(), 1, str.size(), f);
            std::fclose(f);

            if (written != str.size())
                ANV_LOG_ERROR("File::Write - failed to write entire string: " + m_Path);
        }
        else
        {
            std::fclose(f);
        }
    }
}