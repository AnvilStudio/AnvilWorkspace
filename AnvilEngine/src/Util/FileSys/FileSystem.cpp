#include "FileSystem.h"

#include <chrono>
#include <fstream>
#include <Util/UMacros.h>

namespace anv
{
    // -------------------- File --------------------

    File::File(std::string path)
        : m_Path(std::move(path))
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

    size_t File::GetFileSizeBytes(FILE* f)
    {
        if (!f) return 0;

        const long cur = std::ftell(f);
        std::fseek(f, 0, SEEK_END);
        const long end = std::ftell(f);
        std::fseek(f, cur, SEEK_SET);

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

    void File::Write(const std::string& str) const
    {
        std::ofstream out(m_Path, std::ios::trunc);
        if (!out.is_open())
        {
            ANV_LOG_ERROR("File::Write - failed to open: " + m_Path);
            return;
        }
        out << str;
    }

    // -------------------- FileSystem --------------------

    FileSystem::FileSystem(std::string rootDir)
        : m_RootDir(std::move(rootDir))
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

    bool FileSystem::SetRoot(const std::string& newRoot)
    {
        std::filesystem::path p(newRoot);
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
        m_KeyDirs.clear();
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

    bool FileSystem::IsWithinRoot_(const std::filesystem::path& abs) const
    {
        std::error_code ec;

        // Compare via path-relative computation (more robust than string prefix)
        auto rel = std::filesystem::relative(abs, m_RootDir, ec);
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

    bool FileSystem::ResolveSandboxed_(const std::string& relOrAbs, std::filesystem::path& outAbs) const
    {
        std::filesystem::path p(relOrAbs);

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

        outAbs = canon;
        return true;
    }

    bool FileSystem::SwitchDir(const std::string& dirRelOrAbs)
    {
        std::filesystem::path abs;
        if (!ResolveSandboxed_(dirRelOrAbs, abs))
            return false;

        std::error_code ec;
        if (!std::filesystem::exists(abs, ec) || ec) return false;
        if (!std::filesystem::is_directory(abs, ec) || ec) return false;

        m_WorkDir = abs;
        return true;
    }

    bool FileSystem::CreateDir(const std::string& mkdirRelOrAbs)
    {
        std::filesystem::path abs;
        if (!ResolveSandboxed_(mkdirRelOrAbs, abs))
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

    bool FileSystem::DeleteDir(const std::string& dltRelOrAbs)
    {
        std::filesystem::path abs;
        if (!ResolveSandboxed_(dltRelOrAbs, abs))
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

    Ref<File> FileSystem::CreateFile(const std::string& mkfileRelOrAbs)
    {
        std::filesystem::path abs;
        if (!ResolveSandboxed_(mkfileRelOrAbs, abs))
        {
            ANV_LOG_ERROR("CreateFile blocked by sandbox: " + mkfileRelOrAbs);
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

    bool FileSystem::DeleteFile(Ref<File>& dltfile)
    {
        if (!dltfile)
            return false;

        // Enforce: only allow deleting files inside sandbox
        std::filesystem::path abs = std::filesystem::path(dltfile->Path());
        if (!IsWithinRoot_(abs))
        {
            ANV_LOG_WARN("DeleteFile blocked (outside sandbox): '%s'", dltfile->Path().c_str());
            return false;
        }

        dltfile->MarkForDelete();
        EnqueueDelete_(dltfile);
        return true;
    }

    void FileSystem::CreateKeyDir(const std::string& key, const std::string& dirRelOrAbs)
    {
        std::filesystem::path abs;
        if (!ResolveSandboxed_(dirRelOrAbs, abs))
        {
            ANV_LOG_WARN("CreateKeyDir blocked by sandbox: key='%s'", key.c_str());
            return;
        }

        // Create immediately (optional but handy)
        std::error_code ec;
        std::filesystem::create_directories(abs, ec);

        m_KeyDirs[key] = abs.string();
    }

    std::string FileSystem::AtKeyDir(const std::string& key)
    {
        auto it = m_KeyDirs.find(key);
        if (it == m_KeyDirs.end())
            return {};
        return it->second;
    }


    bool FileSystem::MoveToKeyDir(const std::string& key)
    {
        auto it = m_KeyDirs.find(key);
        if (it == m_KeyDirs.end())
            return false;

        const std::string& absStr = it->second;

        std::filesystem::path abs;
        if (!ResolveSandboxed_(absStr, abs))
            return false;

        std::error_code ec;
        if (!std::filesystem::exists(abs, ec) || ec) return false;
        if (!std::filesystem::is_directory(abs, ec) || ec) return false;

        m_WorkDir = abs;
        return true;
    }

    void FileSystem::EnqueueDelete_(const Ref<File>& file)
    {
        {
            std::scoped_lock lk(m_DeleteMutex);
            m_DeleteQueue.push_back(file);
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
