/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

#include "crypto/Password.hh"
#include "util/Configuration.hh"
#include "util/Exception.hh"
#include "util/Log.hh"
#include "image/ImageContent.hh"
#include "hrb/Server.hh"
#include "net/Listener.hh"
#include "net/Session.hh"

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <openssl/evp.h>

#include <regex>

#include <thread>
#include <iostream>
#include <cstdlib>

namespace hrb {

void run(const Configuration& cfg)
{
	auto const threads = std::max(1UL, cfg.thread_count());

	Server server{cfg};
	server.listen();

	// make sure we load the certificates and listen before dropping root privileges
	server.drop_privileges();

	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for (auto i = threads - 1; i > 0; --i)
		v.emplace_back([&server]{server.get_io_context().run();});

	server.get_io_context().run();
}

int StartServer(const Configuration& cfg)
{
	if (cfg.add_user([&cfg](auto&& username)
	{
		std::cout << "Please input password of the new user " << username << ":\n";
		std::string password;
		if (std::getline(std::cin, password))
		{
			Server::add_user(cfg, username, Password{std::string_view{password}}, [](std::error_code&& ec)
			{
				std::cout << "result = " << ec << " " << ec.message() << std::endl;
			});
		}
	})) {return EXIT_SUCCESS;}

	else
	{
		Log(LOG_NOTICE, "hearty_rabbit (version %1%) starting", constants::version);
		run(cfg);
		return EXIT_SUCCESS;
	}
}

} // end of namespace

int main(int argc, char *argv[])
{
	using namespace hrb;
	try
	{
		OpenSSL_add_all_digests();
		Configuration cfg{argc, argv, ::getenv("HEARTY_RABBIT_CONFIG")};
		if (cfg.help())
		{
			cfg.usage(std::cout);
			std::cout << "\n";
			return EXIT_SUCCESS;
		}

		// check if HAAR model path in configuration is valid
		ImageContent::check_models(cfg.haar_path());

		return StartServer(cfg);
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
