/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/11/2021.
//

#include "HeartyRabbit.hh"
#include "net/Redis.hh"

#include <boost/asio/io_context.hpp>

#include <coroutine>
#include <iostream>

namespace hrb {

template <typename T>
struct Task
{
	struct promise_type
	{
		std::coroutine_handle<> precursor;
		T result;

		Task get_return_object() { return {std::coroutine_handle<promise_type>::from_promise(*this)}; }
		std::suspend_never initial_suspend() { return {}; }

		auto final_suspend() noexcept
		{
			struct AWaiter
			{
				// Return false here to return control to the thread's event loop. Remember that we're
				// running on some async thread at this point.
				[[nodiscard]] bool await_ready() const noexcept
				{
					return false;
				}

				void await_resume() const noexcept {}

				// Returning a coroutine handle here resumes the coroutine it refers to (needed for
				// continuation handling). If we wanted, we could instead enqueue that coroutine handle
				// instead of immediately resuming it by enqueuing it and returning void.
				std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h)
				{
					if (auto p = h.promise().precursor; p)
						return p;

					return std::noop_coroutine();
				}
			};
            return AWaiter{};
		}

		void unhandled_exception() {}

		// When the coroutine co_returns a value, this method is used to publish the result
		void return_value(T value) noexcept
		{
			result = std::move(value);
		}
	};

	// The following methods make our task type conform to the awaitable concept, so we can
	// co_await for a task to complete
	[[nodiscard]] bool await_ready() const noexcept
	{
		// No need to suspend if this task has no outstanding work
		return handle.done();
	}

	T await_resume() const noexcept
	{
		// The returned value here is what `co_await our_task` evaluates to
		return std::move(handle.promise().data);
	}

	void await_suspend(std::coroutine_handle<> coroutine) const noexcept
	{
		// The coroutine itself is being suspended (async work can beget other async work)
		// Record the argument as the continuation point when this is resumed later. See
		// the final_suspend awaiter on the promise_type above for where this gets used
		handle.promise().precursor = coroutine;
	}

	// This handle is assigned to when the coroutine itself is suspended (see await_suspend above)
	std::coroutine_handle<promise_type> handle;
};

struct AsyncOperation
{
	boost::asio::io_context& m_ios;
	[[nodiscard]] constexpr bool await_ready() const noexcept { return false; }

	void await_suspend(std::coroutine_handle<> h) const noexcept
	{
		std::thread thread{[&ios=m_ios, h]
		{
			std::cout << "before 1 seconds" << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds{1});

			std::cout << "after 1 seconds" << std::endl;
			ios.post([h]{h.resume();});
		}};
		thread.detach();
	}

	constexpr void await_resume() const noexcept {}
};

Task<int> keys_coroutine(boost::asio::io_context& ios)
{
	boost::asio::executor_work_guard<decltype(ios.get_executor())> work{ios.get_executor()};

	AsyncOperation a{ios};
	co_await a;

	std::cout << "resumed" << std::endl;
}

} // end of namespace hrb

int main(int argc, char** argv)
{
	boost::asio::io_context ios;

	hrb::keys_coroutine(ios);
//	std::cout << co_await hrb::keys_coroutine() << std::endl;

	ios.run();

	return -1;
}