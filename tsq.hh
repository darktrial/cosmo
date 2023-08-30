#ifndef TSQ_HH
#define TSQ_HH
#include <string.h>
#include <iostream>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <deque>

template <typename T>
class threadSafeQueue
{
    std::mutex queueCondMutex;
    std::mutex processQueueMutex;
    std::deque<T> queue;
    bool condQueue;
    std::condition_variable handleQueueCond;
    void notifyNewData()
    {
        std::lock_guard<std::mutex> lock(queueCondMutex);
        condQueue = true;
        handleQueueCond.notify_all();
    }
    void waitNewData()
    {
        std::unique_lock<std::mutex> lock(queueCondMutex);
        while (!condQueue)
            handleQueueCond.wait(lock, [&]
                                 { return condQueue == true; });
        condQueue = false;
    }

public:
    threadSafeQueue()
    {
        condQueue = false;
    }
    ~threadSafeQueue()
    {
    }
    void push(T const &data)
    {
        std::lock_guard<std::mutex> lock(processQueueMutex);
        queue.push_back(data);
        notifyNewData();
    }
    bool try_pop(T &popped_value)
    {
        if (queue.empty())
        {
            return false;
        }
        std::lock_guard<std::mutex> lock(processQueueMutex);
        popped_value = queue.front();
        queue.pop_front();
        queue.shrink_to_fit();
        return true;
    }
    void wait_available()
    {
        waitNewData();
    }
};


#endif // TSQ_HH
