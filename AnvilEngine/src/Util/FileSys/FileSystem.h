#pragma once

#include "../UMacros.h"
#include <cstdio>
#include <string>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <cstdint>

namespace anv
{
    class File
    {
    public:
        explicit File(std::string path);
        ~File();

        // Non-copyable (FILE* ownership)
        File(const File&) = delete;
        File& operator=(const File&) = delete;

        // Movable
        File(File&& other) noexcept;
        File& operator=(File&& other) noexcept;

        size_t Size() const;

        // Reads text file as lines (strips '\n' and optional '\r')
        _vec<std::string> Read() const;

        // Reads entire file as raw bytes into a vector of T.
        // Common uses: T=uint8_t, char, std::byte, etc.
        template<typename T>
        _vec<T> ReadAs() const
        {
            static_assert(std::is_trivial_v<T>, "ReadAs<T> requires T to be trivial (POD-like).");

            FILE* f = std::fopen(m_Path.c_str(), "rb");
            if (!f)
                ANV_LOG_ERROR("File::ReadAs - failed to open file: " + m_Path);

            const size_t bytes = GetFileSizeBytes(f);
            _vec<T> out;

            if (bytes == 0)
            {
                std::fclose(f);
                return out;
            }

            if (bytes % sizeof(T) != 0)
            {
                std::fclose(f);
                ANV_LOG_ERROR("File::ReadAs - file size not aligned to sizeof(T): " + m_Path);
            }

            const size_t count = bytes / sizeof(T);
            out.resize(count);

            const size_t readCount = std::fread(out.data(), sizeof(T), count, f);
            std::fclose(f);

            if (readCount != count)
                ANV_LOG_ERROR("File::ReadAs - failed to read entire file: " + m_Path);

            return out;
        }

        // Overwrites file with string (text write)
        void Write(const std::string& str) const;

        // Writes raw bytes from vector<T> (binary write)
        template<typename T>
        void WriteAs(const _vec<T>& data) const
        {
            static_assert(std::is_trivial_v<T>, "WriteAs<T> requires T to be trivial (POD-like).");

            FILE* f = std::fopen(m_Path.c_str(), "wb");
            if (!f)
                ANV_LOG_ERROR("File::WriteAs - failed to open file: " + m_Path);

            if (!data.empty())
            {
                const size_t written = std::fwrite(data.data(), sizeof(T), data.size(), f);
                std::fclose(f);

                if (written != data.size())
                    ANV_LOG_ERROR("File::WriteAs - failed to write entire buffer: " + m_Path);
            }
            else
            {
                std::fclose(f);
            }
        }

        const std::string& Path() const { return m_Path; }

    private:
        static size_t GetFileSizeBytes(FILE* f);

    private:
        std::string m_Path;
        FILE* m_File = nullptr; // optional "handle" kept open by ctor; used by Size() if you want
    };

}