#pragma once

#include "../UMacros.h"
#include "../../Core/Reference.h"
#include "../Serialize/Serializer.h"
#include "File.h"

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
#include <functional>

namespace anv
{
    class FileSystem
    {
    public:
        using _ForEachFn = std::function<void(Ref<File>)>;

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
        bool         DeleteFile(Ref<File>& _dltfile); // request delete

        // Stores a reference to the directory as a key for fast movement
        void          MountKey(const std::string & _key, const std::string & _dirRelOrAbs);
        std::filesystem::path GetKeyVal(const std::string & _key);
        bool          MoveToKey(const std::string & _key);

        void ForEach(const const std::filesystem::path _dirRelOrAbs, _ForEachFn _Fn);

        // If you don’t want a background thread, you can stop it and call this manually.
        void PumpDeletes();

    private:
        // Resolve a path against working directory, normalize, and enforce sandbox.
        bool ResolveSandboxed_(const std::string & _relOrAbs, std::filesystem::path & _outAbs) const;
        bool IsWithinRoot_(const std::filesystem::path & _abs) const;
        std::filesystem::path ResolveKey_(const std::string& _key);
        void EnqueueDelete_(const Ref<File>& _file);
        void DeleteWorkerLoop_();

    private:
        std::unordered_map<std::string, std::string> m_KeyMap;
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
