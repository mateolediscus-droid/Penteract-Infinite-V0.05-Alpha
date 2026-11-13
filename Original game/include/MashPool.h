/*
Copyright (c) 2012 Jakob Progsch, Václav Zeman

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/
#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <cassert>

class MashPool
{
public:

	MashPool(size_t threadCount = std::thread::hardware_concurrency());

	// joins all worker threads
	~MashPool();

	template<class F>
	void addTask(F&& f);

	template<class F, class... Args>
	void addTask(F&& f, Args&&... args);

	template<class F, class... Args>
	auto addTaskFuture(F&& f, Args&&... args)
		-> std::future<std::invoke_result_t<F, Args...>>;

	// waits until all tasks have been completed
	void wait();

	// cancels all tasks that aren't already running
	void cancelAllQueuedTasks();

private:

	std::vector<std::thread> workers;

	std::queue<std::move_only_function<void()>> tasks;
	std::atomic_uint taskCount;

	// synchronization
	std::mutex queueMutex;
	std::condition_variable condition;
	bool stop;
};

inline MashPool::MashPool(size_t threadCount)
	: stop{ false }
{
	for (size_t i = 0; i < threadCount; ++i)
	{
		workers.emplace_back(
			[this]
			{
				for (;;)
				{
					std::move_only_function<void()> task;

					{
						std::unique_lock<std::mutex> lock{ this->queueMutex };
						this->condition.wait(lock,
							[this] {
								return this->stop || !this->tasks.empty();
							}
						);

						if (this->stop && this->tasks.empty())
						{
							return;
						}

						task = std::move(this->tasks.front());
						this->tasks.pop();
					}

					task();

					// decrement taskCount only after completing a task
					--this->taskCount;
				}
			}
		);
	}
}

inline MashPool::~MashPool()
{
	{
		std::scoped_lock lock{ queueMutex };
		stop = true;
	}
	condition.notify_all();

	for (std::thread& worker : workers)
	{
		worker.join();
	}
}

template<class F>
inline void MashPool::addTask(F&& f)
{
	// increment "taskCount" before doing anything
	++taskCount;

	{
		std::scoped_lock lock{ queueMutex };

		// don't allow enqueueing after stopping the pool
		assert(!stop);

		tasks.emplace(std::move(f));
	}
	condition.notify_one();
}

template<class F, class... Args>
void MashPool::addTask(F&& f, Args&&... args)
{
	// increment "taskCount" before doing anything
	++taskCount;

	{
		std::scoped_lock lock{ queueMutex };

		// don't allow enqueueing after stopping the pool
		assert(!stop);

		tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
	}
	condition.notify_one();
}

template<class F, class... Args>
auto MashPool::addTaskFuture(F&& f, Args&&... args)
-> std::future<std::invoke_result_t<F, Args...>>
{
	// increment "taskCount" before doing anything
	++taskCount;

	using return_type = std::invoke_result_t<F, Args...>;

	// the packaged_task is created on the heap with std::make_unique(),
	// and the resulting unique_ptr is moved to the capture list of a lambda which the packaged_task is then called from,
	// which is apparently faster than just creating the packaged_task on the stack and then calling tasks.emplace(std::move(packaged_task)).
	auto task = std::make_unique<std::packaged_task<return_type()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	);

	std::future<return_type> res = task->get_future();
	{
		std::scoped_lock<std::mutex> lock{ queueMutex };

		// don't allow tasks to be queued when the pool has been stopped
		assert(!stop);

		tasks.emplace([task = std::move(task)]() { (*task)(); });
	}
	condition.notify_one();

	return res;
}

inline void MashPool::wait()
{
	while (taskCount > 0);
}

inline void MashPool::cancelAllQueuedTasks()
{
	std::scoped_lock lock{ queueMutex };
	decltype(tasks) tmp;
	std::swap(tasks, tmp);
	taskCount -= tmp.size();
}