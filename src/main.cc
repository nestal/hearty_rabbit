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
#include "hrb/Server.hh"

#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <thread>
#include <iostream>
#include <cstdlib>

namespace hrb {

void migrate_blob_backlink(const Configuration& cfg)
{
	boost::asio::io_context ctx;
	auto db = redis::connect(ctx, cfg.redis());

	db->command(
		[db](auto&& reply, auto ec)
		{
			assert(!ec);
			assert(reply.array_size() == 2);
			assert(reply.as_array(0).as_int() == 0);

			std::cout << "# of blobs: " << reply.as_array(1).array_size() << std::endl;

			for (auto&& bl : reply.as_array(1))
			{
				std::string backlink{bl.as_string()};
				db->command(
					[backlink](auto&& reply, auto ec)
					{
						assert(!ec);
//						std::cout << backlink << " has " << reply.array_size() << " links" << std::endl;

						if (!boost::string_view{backlink}.starts_with("blob-backlink:nestal:"))
						{
							std::cout << backlink << " is owned by someone else" << std::endl;
						}
					},
					"SMEMBERS %b", backlink.data(), backlink.size()
				);
			}
		},
		"SCAN 0 MATCH blob-backlink:* COUNT 100000"
	);

	ctx.run();
}

int StartServer(const Configuration& cfg)
{
	Server server{cfg};
	if (cfg.add_user([&server](auto&& username)
	{
		std::cout << "Please input password of the new user " << username << ":\n";
		std::string password;
		if (std::getline(std::cin, password))
		{
			server.add_user(username, Password{std::string_view{password}}, [](std::error_code&& ec)
			{
				std::cout << "result = " << ec << " " << ec.message() << std::endl;
			});
		}
	})) {return EXIT_SUCCESS;}

	else
	{
		Log(LOG_NOTICE, "hearty_rabbit (version %1%) starting", constants::version);
		server.run();
		return EXIT_SUCCESS;
	}
}

} // end of namespace

int main(int argc, char *argv[])
{
	using namespace hrb;
	try
	{
		Configuration cfg{argc, argv, ::getenv("HEART_RABBIT_CONFIG")};
		if (cfg.help())
		{
			cfg.usage(std::cout);
			std::cout << "\n";
			return EXIT_SUCCESS;
		}
		else if (cfg.blob_id([&cfg](auto&&)
		{
			migrate_blob_backlink(cfg);
		})) { return EXIT_SUCCESS;}

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
