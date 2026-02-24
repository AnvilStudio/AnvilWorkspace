#pragma once
#include "../../Core/Reference.h"
#include "../UMacros.h"

#include <atomic>
#include <string>
#include <filesystem>

namespace anv
{
    class File final : public RefCounter
    {
    public:
        explicit File(std::string _path);
        ~File();

        File(const File&) = delete;
        File& operator=(const File&) = delete;

        // IMPORTANT: must be non-movable when managed by Ref<T>
        File(File&&) = delete;
        File& operator=(File&&) = delete;

        void MarkForDelete();
        bool DeleteNow();

        size_t Size() const;
        _vec<std::string> Read() const;

        bool Exists() const;
        bool CreateIfMissing(bool binary) const;

        template<typename T>
        _vec<T> ReadAs() const
        {
            static_assert(std::is_trivial_v<T>, "ReadAs<T> requires T to be trivial (POD-like).");

            FILE* f = std::fopen(m_Path.c_str(), "rb");
            if (!f)
            {
                ANV_LOG_ERROR("File::ReadAs - failed to open file: " + m_Path);
                return {};
            }

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
                return {};
            }

            const size_t count = bytes / sizeof(T);
            out.resize(count);

            const size_t readCount = std::fread(out.data(), sizeof(T), count, f);
            std::fclose(f);

            if (readCount != count)
            {
                ANV_LOG_ERROR("File::ReadAs - failed to read entire file: " + m_Path);
                return {};
            }

            return out;
        }

        void Write(const std::string& _str) const;

        template<typename T>
        void WriteAs(const _vec<T>& _data) const
        {
            static_assert(std::is_trivial_v<T>, "WriteAs<T> requires T to be trivial (POD-like).");

            FILE* f = std::fopen(m_Path.c_str(), "wb");
            if (!f)
            {
                ANV_LOG_ERROR("File::WriteAs - failed to open file: " + m_Path);
                return;
            }

            if (!_data.empty())
            {
                const size_t written = std::fwrite(_data.data(), sizeof(T), _data.size(), f);
                std::fclose(f);

                if (written != _data.size())
                    ANV_LOG_ERROR("File::WriteAs - failed to write entire buffer: " + m_Path);
            }
            else
            {
                std::fclose(f);
            }
        }

        const std::string& Path() const { return m_Path; }
        bool IsDeleteRequested() const { return m_DeleteRequested.load(std::memory_order_acquire); }

    private:
        static size_t GetFileSizeBytes(FILE* _f);

    private:
        std::string m_Path;
        std::atomic<bool> m_DeleteRequested{ false };
    };
}