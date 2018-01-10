/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/10/18.
//

#include <boost/asio/io_context.hpp>

#include <hiredis/hiredis.h>
#include <iostream>


int main()
{
	auto c = redisConnect("127.0.0.1", 6379);
	if (!c || c->err)
	{
	    if (c)
	    {
	        printf("Error: %s\n", c->errstr);
	        // handle error
	    }
	    else
	    {
	        printf("Can't allocate redis context\n");
	    }
	}

	auto reply = static_cast<redisReply*>(redisCommand(c, "set mycounter 1"));
	switch (reply->type)
	{
		case REDIS_REPLY_STATUS:
			std::cout << "Return status: " << std::string_view(reply->str, reply->len) << std::endl;
			break;
		case REDIS_REPLY_STRING:
			std::cout << "Return string: " << std::string_view(reply->str, reply->len) << std::endl;
			break;
	}

	redisFree(c);

	return 0;
}