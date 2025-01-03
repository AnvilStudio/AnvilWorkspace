///////////////////////////////////////////////////////////////////////
/// QueueChain: A class designed to handle high-volume tasks in a   ///
/// multi-threaded environment, similar to how swapchains are used. ///
///                                                                 ///
/// Usage:                                                          ///
/// The QueueChain is primarily used for rendering, but can be      ///
/// applied to other high-volume tasks, such as physics or AI.      ///
///                                                                 ///
/// This class manages a set of queues (Back Queue and Front Queue) ///
/// where the Back Queue is written to while the Front Queue is     ///
/// processed by a separate worker thread, allowing for parallel    ///
/// processing and reducing the burden on the main thread.          ///
///////////////////////////////////////////////////////////////////////


#include <iostream>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "../Util/UMacros.h"
namespace anv {
    class QueueChain {
    public:

        using CmdQueue = std::queue<std::function<void()>>;

        QueueChain();

        ~QueueChain();

        void Start();

        void Stop();

        void WriteToBack(const std::function<void()>& task);

        void NotifyMainDone();

        void WaitForProcessComplete();

        void Swap();


    private:
        CmdQueue* m_Front = nullptr;
        CmdQueue* m_Middle = nullptr;
        CmdQueue* m_Back = nullptr;

        std::mutex m_QueueMutex;
        std::condition_variable m_QueueCondition;

        std::thread m_ProcThread;
        std::atomic<bool> m_StopProc;

        bool m_MainRdy;
        bool m_ProcRdy;

        // Worker thread
        void ProcessFrontQueue();
    };
}