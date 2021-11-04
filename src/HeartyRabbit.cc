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
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/coroutine.hpp>

#include <coroutine>
#include <iostream>

namespace hrb {

template <typename T>
struct Future
{
	class promise_type
	{
	public:
		T m_result{};

	public:
		promise_type() = default;

		Future get_return_object()
		{
			return {std::coroutine_handle<promise_type>::from_promise(*this)};
		}
		std::suspend_never initial_suspend() { return {}; }

		std::suspend_always final_suspend() noexcept {return {};}

		void unhandled_exception() noexcept {}

		// When the coroutine co_returns a value, this method is used to publish the result
		void return_value(T value) noexcept
		{
			m_result = std::move(value);
		}
	};

	Future(std::coroutine_handle<promise_type> handle) : m_handle{handle}
	{
	}
	~Future()
	{
		if (m_handle)
			m_handle.destroy();
	}

	struct AwaitableFuture
	{
		Future& m_future;
		[[nodiscard]] bool await_ready() const noexcept {return false;}
		void await_suspend(std::coroutine_handle<> handle)
		{
			std::thread{[this, handle]
			{
				std::cout << "before resume all" << std::endl;
				m_future.m_handle.resume();
				std::cout << "after resume future" << std::endl;
				handle.resume();
				std::cout << "after resume all" << std::endl;
			}}.detach();
		}

		T await_resume()
		{
            return m_future.m_handle.promise().m_result;
        }
	};
	friend struct AwaitableFuture;

	AwaitableFuture operator co_await()
	{
		return {*this};
	}

private:
	std::coroutine_handle<promise_type> m_handle;
};

struct Task
{
	struct promise_type
	{
		Task get_return_object() { return {}; }
		std::suspend_never initial_suspend() noexcept { return {}; }
		std::suspend_never final_suspend() noexcept { return {}; }
		void return_void() {}
		void unhandled_exception() noexcept	{}
	};
};

template <typename F, typename... Args>
Future<std::invoke_result_t<F, Args...>> async(F f, Args... args)
{
	std::cout << "async" << std::endl;
	co_return f(args...);
}

Task keys_coroutine(boost::asio::io_context& ios, std::shared_ptr<redis::Connection> redis)
{
	boost::asio::executor_work_guard<decltype(ios.get_executor())> work{ios.get_executor()};



	auto result = co_await async([]
	{
		std::cout << "before sleep" << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds{1});
		std::cout << "after sleep" << std::endl;
		return 100;
	});
	std::cout << "result = " << result << std::endl;
}

} // end of namespace hrb

int main(int argc, char** argv)
{
	boost::asio::io_context ios;
	auto redis = hrb::redis::connect(ios);

	hrb::keys_coroutine(ios, redis);
//	std::cout << co_await hrb::keys_coroutine() << std::endl;

	std::thread worker{[&ios]
	{
		ios.run();
	}};
	worker.join();

	return -1;
}