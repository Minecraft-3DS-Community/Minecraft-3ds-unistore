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
/** @file threadPool.h
 *  @brief Thread pool implementation
 */
#pragma once

#include <functional>
#include <future>
#include <queue>
#include <type_traits>

class ThreadPool
{
public:
	~ThreadPool ();

	template <typename F, class... Args>
	static auto enqueue (F &&f, Args &&... args) -> decltype (
	    std::async (std::launch::deferred, std::forward<F> (f), std::forward<Args> (args)...)
	        .share ())
	{
		auto future =
		    std::async (std::launch::deferred, std::forward<F> (f), std::forward<Args> (args)...)
		        .share ();

		pushJob (std::bind (&decltype (future)::wait, future));

		assert (future.valid ());
		return future;
	}

private:
	ThreadPool ();

	static void pushJob (std::function<void(void)> &&job);

	static ThreadPool pool;
};
