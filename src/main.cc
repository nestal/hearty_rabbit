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
#include "net/Listener.hh"
#include "net/Session.hh"

#include <boost/asio/ssl/context.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <regex>

#include <thread>
#include <iostream>
#include <cstdlib>

namespace hrb {

void run(Server& server, const Configuration& cfg)
{
	OpenSSL_add_all_digests();
	auto const threads = std::max(1UL, cfg.thread_count());

	boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23};
	ctx.set_options(
		boost::asio::ssl::context::default_workarounds |
		boost::asio::ssl::context::no_sslv2
	);
	ctx.use_certificate_chain_file(cfg.cert_chain().string());
	ctx.use_private_key_file(cfg.private_key().string(), boost::asio::ssl::context::pem);

	// Create and launch a listening port for HTTP and HTTPS
	std::make_shared<Listener>(
		server.get_io_context(),
		cfg.listen_http(),
		Listener::SessionFactory{},
		nullptr,
		server.https_root()
	)->run();
	std::make_shared<Listener>(
		server.get_io_context(),
		cfg.listen_https(),
		[&server](auto&& sock, auto& ssl, auto nth)
		{
			return std::make_shared<Session>(server, std::move(sock), ssl, nth);
		},
		&ctx,
		server.https_root()
	)->run();

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
		run(server, cfg);
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
