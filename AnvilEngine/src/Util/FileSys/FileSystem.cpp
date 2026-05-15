#include "FileSystem.h"

#include <chrono>
#include <fstream>
#include <Util/UMacros.h>

namespace anv
{

    anv::FileSystem::FileSystem(std::string _rootDir)
        : m_RootDir(std::move(_rootDir))
        , m_WorkDir(m_RootDir)
    {
        std::error_code ec;
        std::filesystem::create_directories(m_RootDir, ec);

        // Normalize root/work best-effort
        m_RootDir = std::filesystem::weakly_canonical(m_RootDir, ec);
        if (ec) m_RootDir = m_RootDir.lexically_normal();
        m_WorkDir = m_RootDir;

        // Start delete worker
        m_DeleteWorker = std::thread(&FileSystem::DeleteWorkerLoop_, this);
    }

    FileSystem::~FileSystem()
    {
        m_StopWorker.store(true, std::memory_order_release);
        m_DeleteCv.notify_all();
        if (m_DeleteWorker.joinable())
            m_DeleteWorker.join();

        PumpDeletes();
    }

    bool anv::FileSystem::SetRoot(const std::string& _newRoot)
    {
        std::filesystem::path p(_newRoot);
        if (p.is_relative())
            p = m_RootDir / p; // relative to current root

        std::error_code ec;
        std::filesystem::create_directories(p, ec);
        if (ec) return false;

        auto canon = std::filesystem::weakly_canonical(p, ec);
        if (ec) canon = p.lexically_normal();

        if (!std::filesystem::exists(canon, ec) || ec) return false;
        if (!std::filesystem::is_directory(canon, ec) || ec) return false;

        // Clear state that is tied to the old root
        {
            std::scoped_lock lk(m_RegistryMutex);
            m_FilesByAbsPath.clear();
        }
        {
            std::scoped_lock lk(m_DeleteMutex);
            m_DeleteQueue.clear();
        }
        m_KeyMap.clear();
        m_DirStack.clear();

        m_RootDir = canon;
        m_WorkDir = m_RootDir;
        return true;
    }

    std::string FileSystem::GetCwd() const
    {
        return m_WorkDir.string();
    }

    void FileSystem::PushDir()
    {
        m_DirStack.push_back(m_WorkDir);
    }

    bool FileSystem::PopDir()
    {
        if (m_DirStack.empty())
            return false;

        m_WorkDir = m_DirStack.back();
        m_DirStack.pop_back();
        return true;
    }

    bool anv::FileSystem::IsWithinRoot_(const std::filesystem::path& _abs) const
    {
        std::error_code ec;

        // Compare via path-relative computation (more robust than string prefix)
        auto rel = std::filesystem::relative(_abs, m_RootDir, ec);
        if (ec) return false;

        // If relative() yields something that starts with "..", it escaped the root
        auto it = rel.begin();
        if (it != rel.end() && *it == "..")
            return false;

        // Also reject absolute rels (shouldn't happen, but belt+suspenders)
        if (rel.is_absolute())
            return false;

        return true;
    }

    std::filesystem::path FileSystem::ResolveKey(const std::string& _key)
    {
        if (_key.empty())
            return {};

        // If it doesn't start with '@', treat it as a normal path
        if (_key[0] != '@')
            return std::filesystem::path(_key);

        // Find the first slash/backslash after the '@Key'
        const size_t slashPos = _key.find_first_of("/\\", 1);

        // Extract key name WITHOUT '@'
        std::string keyName;
        std::string relativePath;

        if (slashPos == std::string::npos)
        {
            // "@Assets"
            keyName = _key.substr(1);
            relativePath = "";
        }
        else
        {
            // "@Assets/folder/file"
            keyName = _key.substr(1, slashPos - 1);
            relativePath = _key.substr(slashPos + 1);
        }

        // Lookup key
        auto it = m_KeyMap.find(keyName); // m_KeyMap: "Assets" -> base path
        if (it == m_KeyMap.end())
        {
            ANV_LOG_ERROR("FileSystem: Unknown key '{}'", keyName);
            return {};
        }

        std::filesystem::path resolved = it->second;
        if (!relativePath.empty())
            resolved /= std::filesystem::path(relativePath); // lets filesystem handle separators
        return resolved;
    }

    bool anv::FileSystem::ResolveSandboxed_(const std::string& _relOrAbs, std::filesystem::path& _outAbs) const
    {
        std::filesystem::path p(_relOrAbs);

        // Resolve relative paths against current working directory
        if (p.is_relative())
            p = m_WorkDir / p;

        std::error_code ec;
        // weakly_canonical works even if parts don't exist, which is what we want for create ops
        auto canon = std::filesystem::weakly_canonical(p, ec);
        if (ec) canon = p.lexically_normal();

        // Enforce sandbox
        if (!IsWithinRoot_(canon))
            return false;

        _outAbs = canon;
        return true;
    }

    bool anv::FileSystem::SwitchDir(const std::string& _dirRelOrAbs)
    {
        std::filesystem::path abs;
        if (!ResolveSandboxed_(_dirRelOrAbs, abs))
            return false;

        std::error_code ec;
        if (!std::filesystem::exists(abs, ec) || ec) return false;
        if (!std::filesystem::is_directory(abs, ec) || ec) return false;

        m_WorkDir = abs;
        return true;
    }

    bool anv::FileSystem::CreateDir(const std::string& _mkdirRelOrAbs)
    {
        std::filesystem::path abs;
        if (!ResolveSandboxed_(_mkdirRelOrAbs, abs))
            return false;

        std::error_code ec;
        const bool ok = std::filesystem::create_directories(abs, ec);
        if (ec)
        {
            ANV_LOG_WARN("CreateDir failed '%s': %s", abs.string().c_str(), ec.message().c_str());
            return false;
        }

        return ok || std::filesystem::exists(abs);
    }

    bool anv::FileSystem::DeleteDir(const std::string& _dltRelOrAbs)
    {
        std::filesystem::path abs;
        if (!ResolveSandboxed_(_dltRelOrAbs, abs))
            return false;

        std::error_code ec;
        const auto removed = std::filesystem::remove_all(abs, ec);
        if (ec)
        {
            ANV_LOG_WARN("DeleteDir failed '%s': %s", abs.string().c_str(), ec.message().c_str());
            return false;
        }
        return removed > 0;
    }

    Ref<File> anv::FileSystem::CreateFile(const std::string& _mkfileRelOrAbs)
    {
        std::filesystem::path abs;
        if (!ResolveSandboxed_(_mkfileRelOrAbs, abs))
        {
            ANV_LOG_ERROR("CreateFile blocked by sandbox: " + _mkfileRelOrAbs);
            return nullptr;
        }

        const std::string key = abs.string();

        // Return existing if already created
        {
            std::scoped_lock lk(m_RegistryMutex);
            auto it = m_FilesByAbsPath.find(key);
            if (it != m_FilesByAbsPath.end())
                return it->second;
        }

        // Ensure parent dir exists
        {
            std::error_code ec;
            std::filesystem::create_directories(abs.parent_path(), ec);
        }

        Ref<File> f = Ref<File>::Create(key);
        f->CreateIfMissing(true);

        // Store in registry (FS holds 1 persistent ref)
        {
            std::scoped_lock lk(m_RegistryMutex);
            m_FilesByAbsPath[key] = f;
        }

        return f;
    }

    bool anv::FileSystem::DeleteFile(Ref<File>& _dltfile)
    {
        if (!_dltfile)
            return false;

        // Enforce: only allow deleting files inside sandbox
        std::filesystem::path abs = std::filesystem::path(_dltfile->Path());
        if (!IsWithinRoot_(abs))
        {
            ANV_LOG_WARN("DeleteFile blocked (outside sandbox): '%s'", _dltfile->Path().c_str());
            return false;
        }

        _dltfile->MarkForDelete();
        EnqueueDelete_(_dltfile);
        return true;
    }

    std::filesystem::path FileSystem::ResolveDir(std::string _dir)
    {
        return std::filesystem::path();
    }

    void anv::FileSystem::MountKey(const std::string& _key, const std::string& _dirRelOrAbs)
    {
        if (_key.empty() || _dirRelOrAbs.empty())
        {
            ANV_LOG_WARN("MountKey: empty key or dir");
            return;
        }

        // 1) Resolve @Key paths into a real path; otherwise just treat as a path
        std::filesystem::path resolved =
            (_dirRelOrAbs[0] == '@')
            ? ResolveKey(_dirRelOrAbs)
            : std::filesystem::path(_dirRelOrAbs);

        if (resolved.empty())
        {
            ANV_LOG_WARN("MountKey: failed to resolve path for key='%s' input='%s'",
                _key.c_str(), _dirRelOrAbs.c_str());
            return;
        }

        // 2) Sandbox the RESOLVED path (not the raw input string)
        std::filesystem::path abs;
        if (resolved.is_absolute())
        {
            abs = resolved;
        }
        else
        {
            // If ResolveSandboxed_ expects a relative path, pass the resolved RELATIVE path
            if (!ResolveSandboxed_(resolved.string(), abs))
            {
                ANV_LOG_WARN("MountKey blocked by sandbox: key='%s' path='%s'",
                    _key.c_str(), resolved.string().c_str());
                return;
            }
        }

        // 3) Normalize and optionally ensure the directory exists
        std::error_code ec;
        abs = std::filesystem::weakly_canonical(abs, ec);
        if (ec)
        {
            ANV_LOG_WARN("MountKey: failed to canonicalize '%s' (ec=%d)", abs.string().c_str(), (int)ec.value());
            return;
        }

        // Optional: create if missing (choose what you want)
        if (!std::filesystem::exists(abs, ec) || ec)
        {
            std::filesystem::create_directories(abs, ec);
            if (ec)
            {
                ANV_LOG_WARN("MountKey: failed to create dir '%s' (ec=%d)", abs.string().c_str(), (int)ec.value());
                return;
            }
        }

        if (!std::filesystem::is_directory(abs, ec) || ec)
        {
            ANV_LOG_WARN("MountKey: not a directory '%s'", abs.string().c_str());
            return;
        }

        // 4) Store key WITHOUT '@' (based on your earlier rule)
        std::string cleanKey = _key;
        if (!cleanKey.empty() && cleanKey[0] == '@')
            cleanKey.erase(cleanKey.begin());

        m_KeyMap[cleanKey] = abs.string();
    }

    std::filesystem::path anv::FileSystem::GetKeyVal(const std::string& _key)
    {
        auto it = m_KeyMap.find(_key);
        if (it == m_KeyMap.end())
            return {};
        return it->second;
    }


    bool anv::FileSystem::MoveToKey(const std::string& _key)
    {
        const std::filesystem::path resolved = ResolveKey(_key);

        std::filesystem::path abs;
        if (!ResolveSandboxed_(resolved.string(), abs))
            return false;

        std::error_code ec;

        // Normalize the final directory (optional but recommended)
        abs = std::filesystem::weakly_canonical(abs, ec);
        if (ec) return false;

        if (!std::filesystem::exists(abs, ec) || ec) return false;
        if (!std::filesystem::is_directory(abs, ec) || ec) return false;

        m_WorkDir = abs;
        return true;
    }

    void FileSystem::ForEach(const std::filesystem::path _dirRelOrAbs, _ForEachFn _Fn)
    {
        ANV_ASSERT(_Fn, "ForEach function is null");

        // if using keys
        // lowk hate this
        // TODO: Fix all the ".string()"s
        std::filesystem::path resolved =
            (_dirRelOrAbs.string()[0] == '@')
            ? ResolveKey(_dirRelOrAbs.string())
            : std::filesystem::path(_dirRelOrAbs);

        if (resolved.empty())
        {
            ANV_LOG_WARN("MountKey: failed to resolve path for key='%s' input='%s'",
                resolved.c_str(), _dirRelOrAbs.c_str());
            return;
        }

        // Resolve without mutating working directory (no side effects)
        std::filesystem::path absDir;
        if (!ResolveSandboxed_(resolved.string(), absDir))
        {
            ANV_LOG_WARN("ForEach blocked by sandbox: '%s'", _dirRelOrAbs.string().c_str());
            return;
        }

        std::error_code ec;

        if (!std::filesystem::exists(absDir, ec) || ec)
        {
            ANV_LOG_WARN("ForEach: directory does not exist: '%s'", absDir.string().c_str());
            return;
        }

        if (!std::filesystem::is_directory(absDir, ec) || ec)
        {
            ANV_LOG_WARN("ForEach: path is not a directory: '%s'", absDir.string().c_str());
            return;
        }

        // Recursive scan, yielding only regular files
        for (auto it = std::filesystem::recursive_directory_iterator(absDir, ec);
            it != std::filesystem::recursive_directory_iterator();
            it.increment(ec))
        {
            if (ec)
            {
                ec.clear();
                continue;
            }

            if (!it->is_regular_file(ec) || ec)
            {
                ec.clear();
                continue;
            }

            // Use CreateFile to reuse registry refs (1 persistent ref per abs path)
            Ref<File> file = CreateFile(it->path().string());
            if (!file)
                continue;

            _Fn(file);
        }
    }

    void anv::FileSystem::EnqueueDelete_(const Ref<File>& _file)
    {
        {
            std::scoped_lock lk(m_DeleteMutex);
            m_DeleteQueue.push_back(_file);
        }
        m_DeleteCv.notify_one();
    }

    void FileSystem::PumpDeletes()
    {
        std::deque<Ref<File>> local;
        {
            std::scoped_lock lk(m_DeleteMutex);
            local.swap(m_DeleteQueue);
        }

        for (auto& f : local)
        {
            if (!f || !f->IsDeleteRequested())
                continue;

            // Safe delete when only registry ref remains
            const uint32_t rc = f->GetRefCount();
            if (rc == 1)
            {
                const std::string key = f->Path();
                f->DeleteNow();

                std::scoped_lock lk(m_RegistryMutex);
                m_FilesByAbsPath.erase(key);
            }
            else
            {
                // Still in use, requeue
                EnqueueDelete_(f);
            }
        }
    }

    void FileSystem::DeleteWorkerLoop_()
    {
        while (!m_StopWorker.load(std::memory_order_acquire))
        {
            Ref<File> f = nullptr;

            {
                std::unique_lock lk(m_DeleteMutex);
                m_DeleteCv.wait(lk, [&] {
                    return m_StopWorker.load(std::memory_order_acquire) || !m_DeleteQueue.empty();
                    });

                if (m_StopWorker.load(std::memory_order_acquire))
                    break;

                f = m_DeleteQueue.front();
                m_DeleteQueue.pop_front();
            }

            if (!f || !f->IsDeleteRequested())
                continue;

            const uint32_t rc = f->GetRefCount();
            if (rc == 1)
            {
                const std::string key = f->Path();
                f->DeleteNow();

                std::scoped_lock lk(m_RegistryMutex);
                m_FilesByAbsPath.erase(key);
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                EnqueueDelete_(f);
            }
        }
    }
}
