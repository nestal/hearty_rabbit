/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#include <catch.hpp>
#include <iostream>

#include "client/GenericHTTPRequest.hh"

using namespace hrb;

TEST_CASE("simple client test", "[normal]")
{
	std::string host = "localhost";
	std::string port = "4433";
	std::string target = "/api/sumsum";
	int version = 11;

	// The io_context is required for all I/O
	boost::asio::io_context ioc;

	// The SSL context is required, and holds certificates
	ssl::context ctx{ssl::context::sslv23_client};

	// Launch the asynchronous operation
	std::make_shared<GenericHTTPRequest<http::empty_body, http::string_body>>(ioc, ctx, [](auto& req)
	{
		std::cout << req.response() << std::endl;
	})->run(host, port, target, version);

	// Run the I/O service. The call will return when
	// the get operation is complete.
	ioc.run();
}
