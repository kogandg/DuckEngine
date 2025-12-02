#pragma once

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

#include <iostream>
#include <print>

class ThreadPool
{
public:
	ThreadPool(int numThreads);
	~ThreadPool();

	void Enqueue(const std::function<void()>& job);

	void Cancel();

	void Shutdown();

private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> jobQueue;

	std::mutex queueMutex;
	std::condition_variable queueCV;
	std::atomic<bool> stop;
};

