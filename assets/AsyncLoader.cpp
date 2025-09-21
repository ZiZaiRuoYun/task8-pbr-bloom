#include "AsyncLoader.h"

namespace assets {

    AsyncLoader::AsyncLoader() : worker_([this] { WorkerLoop(); }) {}

    AsyncLoader::~AsyncLoader() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        if (worker_.joinable()) worker_.join();
    }

    void AsyncLoader::Enqueue(Job job) {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            q_.push(std::move(job));
        }
        cv_.notify_one();
    }

    void AsyncLoader::WorkerLoop() {
        for (;;) {
            Job job;
            {
                std::unique_lock<std::mutex> lk(mtx_);
                cv_.wait(lk, [this] { return stop_ || !q_.empty(); });
                if (stop_ && q_.empty()) break;
                job = std::move(q_.front());
                q_.pop();
            }
            if (job) job();
        }
    }

} // namespace assets
