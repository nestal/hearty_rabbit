/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

#include "net/Listener.hh"
#include "crypto/Authenication.hh"
#include "crypto/Password.hh"
#include "util/Configuration.hh"
#include "util/Exception.hh"
#include "util/Log.hh"
#include "hrb/Server.hh"
#include "net/Redis.hh"

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <thread>
#include <iostream>
#include <cstdlib>

namespace hrb {

int Main(int argc, const char* const* argv)
{
	Configuration cfg{argc, argv, ::getenv("HEART_RABBIT_CONFIG")};
	OpenSSL_add_all_digests();

	if (cfg.help())
	{
		cfg.usage(std::cout);
		std::cout << "\n";
		return EXIT_SUCCESS;
	}
	else if (cfg.add_user([&cfg](auto&& username)
	{
		std::cout << "Please input password of the new user " << username << ":\n";
		std::string password;
		if (std::getline(std::cin, password))
		{
			boost::asio::io_context ioc;
			redis::Database db{ioc, cfg.redis_host(), cfg.redis_port()};
			add_user(username, Password{std::string_view{password}}, db, [&db](std::error_code&& ec)
			{
				std::cout << "result = " << ec << " " << ec.message() << std::endl;
				db.disconnect();
			});
			ioc.run();
		}
	}))
	{
		return EXIT_SUCCESS;
	}

	Log(LOG_NOTICE, "hearty_rabbit starting");
	Server server{cfg};
	server.run();
	return EXIT_SUCCESS;
}

} // end of namespace

int main(int argc, char *argv[])
{
	using namespace hrb;
	try
	{
		return Main(argc, argv);
	}
	catch (Exception& e)
	{
		Log(LOG_CRIT, "Uncaught boost::exception: %1%", boost::diagnostic_information(e));
		return EXIT_FAILURE;
	}
	catch (std::exception& e)
	{
		Log(LOG_CRIT, "Uncaught std::exception: %1%", e.what());
		return EXIT_FAILURE;
	}
	catch (...)
	{
		Log(LOG_CRIT, "Uncaught unknown exception");
		return EXIT_FAILURE;
	}
}
