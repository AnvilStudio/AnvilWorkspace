#pragma once

#include "../UMacros.h"
#include "../../Core/Reference.h"
#include "../Serialize/Serializer.h"

#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

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

        void Write(const std::string & _str) const;

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

    class FileSystem
    {
    public:
        // Root directory for this sandbox FS (will be created if missing)
        explicit FileSystem(std::string _rootDir);
        ~FileSystem();

        FileSystem(const FileSystem&) = delete;
        FileSystem& operator=(const FileSystem&) = delete;

        // Sets sandbox root. Resets working dir to root and clears dir stack.
        bool SetRoot(const std::string & _newRoot);

        // Working-directory helpers (logical, NOT OS CWD)
        std::string GetCwd() const;
        bool SwitchDir(const std::string & _dirRelOrAbs); // relative to current working dir
        void PushDir();
        bool PopDir();

        bool CreateDir(const std::string & _mkdirRelOrAbs);
        bool DeleteDir(const std::string & _dltRelOrAbs);

        Ref<File> CreateFile(const std::string & _mkfileRelOrAbs);
        bool DeleteFile(Ref<File>& _dltfile); // request delete

        void CreateKeyDir(const std::string & _key, const std::string & _dirRelOrAbs);
        std::string AtKeyDir(const std::string & _key);
        bool MoveToKeyDir(const std::string & _key);

        // If you don’t want a background thread, you can stop it and call this manually.
        void PumpDeletes();

    private:
        // Resolve a path against working directory, normalize, and enforce sandbox.
        bool ResolveSandboxed_(const std::string & _relOrAbs, std::filesystem::path & _outAbs) const;
        bool IsWithinRoot_(const std::filesystem::path & _abs) const;

        void EnqueueDelete_(const Ref<File>& _file);
        void DeleteWorkerLoop_();

    private:
        std::unordered_map<std::string, std::string> m_KeyDirs;
        std::filesystem::path                                       m_RootDir; // sandbox root
        std::filesystem::path                                       m_WorkDir; // logical cwd inside root
        _vec<std::filesystem::path>                           m_DirStack;
        // Registry: FileSystem holds 1 persistent ref so we can safely decide "no external refs"
        mutable std::mutex                                        m_RegistryMutex;
        std::unordered_map<std::string, Ref<File>> m_FilesByAbsPath;

        // Delete queue
        std::mutex                     m_DeleteMutex;
        std::condition_variable   m_DeleteCv;
        std::deque<Ref<File>> m_DeleteQueue;

        std::atomic<bool> m_StopWorker{ false };
        std::thread             m_DeleteWorker;
    };
}
