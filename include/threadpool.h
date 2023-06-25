#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

class ThreadPool
{
public:
    ThreadPool(size_t numThreads) : stop(false)
    {
        for (size_t i = 0; i < numThreads; ++i)
        {
            threads.emplace_back([this]
                                 {
                while (true) {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        condition.wait(lock, [this] {
                            return stop || !tasks.empty();
                        });

                        if (stop && tasks.empty()) {
                            return;
                        }

                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    task();
                } });
        }
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            stop = true;
        }

        condition.notify_all();

        for (std::thread &thread : threads)
        {
            thread.join();
        }
    }

    template <class F, class... Args>
    void enqueue(F &&f, Args &&...args)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            tasks.emplace([f = std::function<void()>(std::bind(std::forward<F>(f), std::forward<Args>(args)...))]
                          { f(); });
        }

        condition.notify_one();
    }

    void clearTasks()
    {
        std::unique_lock<std::mutex> lock(mutex);
        while (!tasks.empty())
        {
            tasks.pop();
        }
    }

private:
    std::vector<std::thread> threads;
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable condition;
    bool stop;
};
