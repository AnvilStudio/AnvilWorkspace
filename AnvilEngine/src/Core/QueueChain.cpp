#include "QueueChain.h"

namespace anv {

    QueueChain::QueueChain() : m_StopProc(false), m_MainRdy(false), m_ProcRdy(false)
    {
        m_Front = new CmdQueue();
        m_Middle = new CmdQueue();
        m_Back = new CmdQueue();
    }

    QueueChain::~QueueChain() {
        Stop();
    }

    void QueueChain::Start() {
        m_StopProc = false;
        m_ProcThread = std::thread(&QueueChain::ProcessFrontQueue, this);
    }

    void QueueChain::Stop() {
        {
            std::unique_lock<std::mutex> lock(m_QueueMutex);
            m_StopProc = true;
            m_QueueCondition.notify_all();
        }
        if (m_ProcThread.joinable()) {
            m_ProcThread.join();
        }
    }

    void QueueChain::WriteToBack(const std::function<void()>& task) {
        std::lock_guard<std::mutex> lock(m_QueueMutex);
        m_Back->push(task);
    }

    void QueueChain::NotifyMainDone() {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_MainRdy = true;
        m_QueueCondition.notify_all();
    }

    void QueueChain::WaitForProcessComplete() {
        std::unique_lock<std::mutex> lock(m_QueueMutex);
        m_QueueCondition.wait(lock, [this]() { return m_ProcRdy; });
        m_ProcRdy = false;
    }

    void QueueChain::Swap() {
        std::unique_lock<std::mutex> lock(m_QueueMutex);



        if (m_Back->empty()) {
            return;
        }

        // Rotate queues
        std::swap(m_Front, m_Middle);
        std::swap(m_Middle, m_Back);



        m_MainRdy = true;  // Notify that the front queue is ready
        m_QueueCondition.notify_all();
    }


    // Worker thread
    void QueueChain::ProcessFrontQueue() {
        while (!m_StopProc) {
            std::unique_lock<std::mutex> lock(m_QueueMutex);


            m_QueueCondition.wait(lock, [this]() {
                return (this->m_MainRdy) || this->m_StopProc;
                });

            if (m_StopProc) {
                break;
            }


            if (m_Front->empty())
            {

            }

            else {
                while (!m_Front->empty()) {
                    auto task = m_Front->front();
                    m_Front->pop();
                    lock.unlock();
                    if (task) task();
                    lock.lock();
                }
            }


            m_ProcRdy = true;
            m_QueueCondition.notify_all();
        }
    }
}