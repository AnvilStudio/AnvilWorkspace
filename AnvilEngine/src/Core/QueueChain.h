#pragma once
#include <queue>
#include <functional>
#include <thread>
#include <atomic>
#include <memory>

namespace anv
{


class QueueChain {
public:
    using CmdQueue = std::queue<std::function<void()>>;

    QueueChain();
    ~QueueChain();

    void Start();
    void Stop();

    void WriteToBack(const std::function<void()>& task);
    void WaitForProcessComplete();
    void Swap();

private:
    std::unique_ptr<CmdQueue> m_Front;
    std::unique_ptr<CmdQueue> m_Back;

    std::thread m_ProcThread;
    std::atomic<bool> m_StopProc{false};
    std::atomic<bool> m_ThreadRunning{false};

    std::atomic<bool> m_WorkAvailable{false};
    std::atomic<bool> m_ProcComplete{true};

    void ProcessFrontQueue();
};
} // namespace anv