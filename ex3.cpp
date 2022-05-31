#include <iostream>
#include <pthread.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
using namespace std;

class semaphore {
    mutex mutex;
    condition_variable condition;
    unsigned long count = 0;

public:

    unsigned long getCount() {
        return count;
    }

    void setCount(unsigned long initialCount) {
        count = initialCount;
    }

    void release() {
        lock_guard<decltype(mutex)> lock(mutex);
        ++count;
        condition.notify_one();
    }

    void acquire() {
        unique_lock<decltype(mutex)> lock(mutex);
        while(!count) // Handle spurious wake-ups.
            condition.wait(lock);
        --count;
    }

    bool try_acquire() {
        lock_guard<decltype(mutex)> lock(mutex);
        if(count) {
            --count;
            return true;
        }
        return false;
    }
};

class BoundedQueue
{
    queue<string> boundedQueue;
    //int queueSize;
    mutex mtx;
    semaphore full;
    semaphore empty;
public:

    BoundedQueue(int size)
    {
        //queueSize = size;
        full.setCount(0);
        empty.setCount(size);
    }
    void enqueue(string news)
    {
        empty.acquire();
        mtx.lock();
        boundedQueue.push(news);
        mtx.unlock();
        full.release();
    }
    string dequeue()
    {
        full.acquire();
        mtx.lock();
        string news = boundedQueue.front();
        boundedQueue.pop();
        mtx.unlock();
        empty.release();
        return news;
    }
};

class UnboundedQueue
{
    queue<string> unboundedQueue;
    //int queueSize;
    mutex mtx;
    semaphore full;
public:

    UnboundedQueue(int size)
    {
        //queueSize = size;
        full.setCount(0);
    }
    void enqueue(string news)
    {
        mtx.lock();
        unboundedQueue.push(news);
        mtx.unlock();
        full.release();
    }
    string dequeue()
    {
        full.acquire();
        mtx.lock();
        string news = unboundedQueue.front();
        unboundedQueue.pop();
        mtx.unlock();
        return news;
    }
};

