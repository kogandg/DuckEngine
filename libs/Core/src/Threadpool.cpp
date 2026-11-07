#include <Duck/Core/ThreadPool.h>

#include <Duck/Core/Assert.h>

#include <algorithm>

namespace Duck
{
    namespace
    {
        thread_local bool insideParallelFor = false;
    }

    ThreadPool::ThreadPool(unsigned threadCount)
    {
        if (threadCount == 0)
        {
            threadCount = std::max(1u, std::thread::hardware_concurrency());
        }

        this->workers.reserve(threadCount - 1); // caller works too but is not included in the list
        for (unsigned i = 1; i < threadCount; i++)
        {
            this->workers.emplace_back([this] { this->WorkerLoop(); });
        }
    }

    ThreadPool::~ThreadPool()
    {
        {
            std::lock_guard lock(this->mutex);
            this->stopping = true;
        }
        this->wake.notify_all();
        // jthreads join when 'workers' is destroyed.
    }

    ThreadPool& ThreadPool::Shared()
    {
        static ThreadPool pool;
        return pool;
    }

    void ThreadPool::Work(const Job& jobToRun)
    {
        insideParallelFor = true;
        while (true)
        {
            const int index = this->nextIndex.fetch_add(1, std::memory_order_relaxed);
            if (index >= jobToRun.count)
            {
                break;
            }
            jobToRun.invoker(jobToRun.context, index);
        }
        insideParallelFor = false;
    }

    void ThreadPool::WorkerLoop()
    {
        std::uint64_t seen = 0;
        while (true)
        {
            Job current;
            {
                std::unique_lock lock(this->mutex);
                this->wake.wait(lock, [&] { return this->stopping || this->generation != seen; });
                if (this->stopping)
                {
                    return;
                }
                seen = this->generation;
                current = this->job;
            }

            this->Work(current);

            {
                std::lock_guard lock(this->mutex);
                if (--this->workersBusy == 0)
                {
                    this->done.notify_one();
                }
            }
        }
    }

    void ThreadPool::Run(int count, Invoker invoker, void* context)
    {
        DUCK_ASSERT(!insideParallelFor, "ParallelFor cannot be nested");
        if (count == 0)
        {
            return;
        }

        const Job newJob{invoker, context, count};

        //just work on the caller thread if there are no workier or just 1 index
        if (this->workers.empty() || count == 1)
        {
            for (int i = 0; i < count; i++)
            {
                invoker(context, i);
            }
            return;
        }

        std::lock_guard runLock(this->runMutex);

        {
            std::lock_guard lock(this->mutex);
            this->job = newJob;
            this->nextIndex.store(0, std::memory_order_relaxed);
            this->workersBusy = static_cast<unsigned>(this->workers.size());
            this->generation++;
        }
        this->wake.notify_all();

        this->Work(newJob); // the calling thread pulls indices too

        // wait until every worker has finished with this job, not just until the indices run out
        std::unique_lock lock(this->mutex);
        this->done.wait(lock, [&] { return this->workersBusy == 0; });
    }
}