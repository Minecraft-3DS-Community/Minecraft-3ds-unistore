/*------------------------------------------------------------------------------
 * Copyright (c) 2019
 *     Michael Theall (mtheall)
 *
 * This file is part of tex3ds.
 *
 * tex3ds is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tex3ds is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tex3ds.  If not, see <http://www.gnu.org/licenses/>.
 *----------------------------------------------------------------------------*/
/** @file threadPool.cpp
 *  @brief Thread pool implementation
 */

#include "threadPool.h"

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace
{
std::vector<std::thread> threads;
std::queue<std::function<void(void)>> jobs;
std::mutex mutex;
std::condition_variable newJob;
std::condition_variable jobTaken;
bool quit = false;

void worker ()
{
	std::unique_lock<std::mutex> lock (mutex);

	while (!quit)
	{
		while (!quit && jobs.empty ())
			newJob.wait (lock);

		if (quit)
			return;

		auto job = std::move (jobs.front ());
		jobs.pop ();
		jobTaken.notify_one ();
		lock.unlock ();

		job ();

		lock.lock ();
	}
};

std::once_flag initOnce;
void init ()
{
	const auto numThreads = std::max (1u, std::thread::hardware_concurrency ());
	for (unsigned i = 0; i < numThreads; ++i)
		threads.emplace_back (worker);
}
}

ThreadPool ThreadPool::pool;

ThreadPool::~ThreadPool ()
{
	{
		std::unique_lock<std::mutex> lock (mutex);
		quit = true;
		newJob.notify_all ();
	}

	for (auto &thread : threads)
		thread.join ();
}

ThreadPool::ThreadPool ()
{
}

void ThreadPool::pushJob (std::function<void(void)> &&job)
{
	std::call_once (initOnce, init);

	std::unique_lock<std::mutex> lock (mutex);

	// block while there's many outstanding jobs
	while (jobs.size () > threads.size () * 2)
		jobTaken.wait (lock);

	jobs.emplace (std::move (job));
	newJob.notify_one ();
}
