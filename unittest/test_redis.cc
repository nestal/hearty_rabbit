/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/10/18.
//


#include "net/RedisDriver.hh"

#include <boost/asio/io_context.hpp>

#include <iostream>
#include <cassert>


int main()
{
	boost::asio::io_context ic;

	std::cout << "main thread: " << std::this_thread::get_id() << std::endl;
	hrb::RedisDriver redis{ic, "localhost", 6379};

	redis.command("SET key 100", [](auto) {});
	redis.command("GET key", [](auto reply)
	{
	    if (reply)
		    std::cout << "key: " << std::string_view{reply->str, (unsigned)reply->len} << std::endl;
	});

	ic.run();

	return 0;
}