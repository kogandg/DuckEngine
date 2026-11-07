#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

namespace Duck
{

    // a fixed set of worker threads that run ParallelFor loops. The calling thread works as well so ThreadCount()
    // includes it. Only one ParallelFor runs at a time per pool
    class ThreadPool
    {
    public:
        // threadCount is the total including the caller, 0 means one per hardware thread
        explicit ThreadPool(unsigned threadCount = 0);
        ~ThreadPool();

        ThreadPool(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(ThreadPool&&) = delete;

        unsigned ThreadCount() const noexcept { return static_cast<unsigned>(this->workers.size()) + 1; }

        // process-wide pool sized to the hardware, created on first use
        static ThreadPool& Shared();

        // Calls body(index) for every index in [0, count), spread across the threads, and returns when all
        // are done. indices are handed out one at a time, so each one should be a meaningful chunk of work.
        // Must not be called from inside a body.
        template <typename TFunc>
        void ParallelFor(int count, TFunc&& body)
        {
            using Fn = std::remove_reference_t<TFunc>;
            this->Run(
                count, [](void* context, int index) { (*static_cast<Fn*>(context))(index); },
                const_cast<std::remove_const_t<Fn>*>(std::addressof(body)));
        }

    private:
        using Invoker = void (*)(void*, int);

        struct Job
        {
            Invoker invoker = nullptr;
            void* context = nullptr;
            int count = 0;
        };

        void Run(int count, Invoker invoker, void* context);
        void WorkerLoop();
        void Work(const Job& job);

        std::mutex runMutex; // serializes ParallelFor calls
        std::mutex mutex;    // guards the fields below
        std::condition_variable wake;
        std::condition_variable done;
        Job job;                      // current work
        std::uint64_t generation = 0; // goes up for every new job so workers know there is new work
        unsigned workersBusy = 0;     // how many workers havent finished their current job
        bool stopping = false;        // set at shutdown

        std::atomic<int> nextIndex{0};

        std::vector<std::jthread>
            workers; // sleeping threads. last, so they are joined before anything above is destroyed
    };

}