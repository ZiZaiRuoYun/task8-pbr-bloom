#pragma once
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

namespace assets {

    class AsyncLoader {
    public:
        using Job = std::function<void()>;

        AsyncLoader();
        ~AsyncLoader();

        void Enqueue(Job job);

    private:
        void WorkerLoop();

        std::thread worker_;
        std::mutex mtx_;
        std::condition_variable cv_;
        std::queue<Job> q_;
        bool stop_{ false };
    };

} // namespace assets
