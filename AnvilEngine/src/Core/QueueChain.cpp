#include "QueueChain.h"
#include "../Util/UMacros.h"
#include <iostream>

namespace anv
{

QueueChain::QueueChain() {
    m_Front = std::make_unique<CmdQueue>();
    m_Back = std::make_unique<CmdQueue>();
}

QueueChain::~QueueChain() {
    Stop(); // <-- Ensure thread is stopped before destruction
}

void QueueChain::Start() {
    m_StopProc = false;
    m_ThreadRunning = true;
    m_ProcThread = std::thread(&QueueChain::ProcessFrontQueue, this);
}

void QueueChain::Stop() {
    if (m_ThreadRunning) {
        m_StopProc = true;
        if (m_ProcThread.joinable()) {
            m_ProcThread.join();
        }
        m_ThreadRunning = false;
    }
}

void QueueChain::WriteToBack(const std::function<void()>& task) {
    m_Back->push(task);
}

void QueueChain::Swap() {
    if (!m_Back->empty()) {
        std::swap(m_Front, m_Back); // Swap buffer pointers
        m_WorkAvailable = true;
        m_ProcComplete = false; // Mark processing not yet done
    }
}

void QueueChain::WaitForProcessComplete() {
    // Spin-wait until processing is done
    while (!m_ProcComplete && !m_StopProc) {
        std::this_thread::yield();
    }
}

void QueueChain::ProcessFrontQueue() {
    while (!m_StopProc) {
        // Spin-wait for work
        if (!m_WorkAvailable) {
            std::this_thread::yield();
            continue;
        }

        // Process all tasks in strict FIFO order
        while (!m_Front->empty()) {
            auto task = m_Front->front();
            m_Front->pop();
            try {
                if (task) task();
            } catch (const std::exception& ex) {
                ANV_LOG_ERROR("[QueueChain] Task exception: %s", ex.what())
            } catch (...) {
                ANV_LOG_ERROR("[QueueChain] Task threw unknown exception!")
            }
        }

        m_WorkAvailable = false;
        m_ProcComplete = true;

        std::this_thread::yield();
    }

    m_ThreadRunning = false; // Mark thread as stopped
}
} // namespace anv