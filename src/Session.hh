/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/1/18.
//

#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/http/message.hpp>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>

namespace hrb {

// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session>
{
	// This is the C++11 equivalent of a generic lambda.
	// The function object is used to send an HTTP message.
	struct send_lambda
	{
		Session &self_;

		explicit send_lambda(Session &self)
			: self_(self)
		{
		}

		template<bool isRequest, class Body, class Fields>
		void
		operator()(boost::beast::http::message <isRequest, Body, Fields> &&msg) const
		{
			// The lifetime of the message has to extend
			// for the duration of the async operation so
			// we use a shared_ptr to manage it.
			auto sp = std::make_shared<
			    boost::beast::http::message<isRequest, Body, Fields>
			>(std::move(msg));

			// Store a type-erased version of the shared
			// pointer in the class to keep it alive.
			self_.res_ = sp;

			// Write the response
			boost::beast::http::async_write(
				self_.socket_,
				*sp,
				boost::asio::bind_executor(
					self_.strand_,
					std::bind(
						&Session::on_write,
						self_.shared_from_this(),
						std::placeholders::_1,
						std::placeholders::_2,
						sp->need_eof())));
		}
	};

	boost::asio::ip::tcp::socket socket_;
	boost::asio::strand <
	boost::asio::io_context::executor_type> strand_;
	boost::beast::flat_buffer buffer_;
	std::string doc_root_;
	boost::beast::http::request <boost::beast::http::string_body> req_;
	std::shared_ptr<void> res_;
	send_lambda lambda_;

public:
	// Take ownership of the socket
	explicit
	Session(
		boost::asio::ip::tcp::socket socket,
		std::string const &doc_root);

	// Start the asynchronous operation
	void run();
	void do_read();
	void on_read(boost::system::error_code ec, std::size_t bytes_transferred);
	void on_write(boost::system::error_code ec, std::size_t bytes_transferred, bool close);
	void do_close();
};

} // end of namespace
