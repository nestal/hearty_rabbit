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

#include <boost/asio/ssl.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>

#include <thread>
#include <iostream>
#include <cstdlib>

namespace hrb {
void drop_privileges()
{
	// drop privileges if run as root
	if (::getuid() == 0)
	{
		if (::setuid(65535) != 0)
			BOOST_THROW_EXCEPTION(hrb::SystemError()
				<< ErrorCode(std::error_code(errno, std::system_category()))
				<< boost::errinfo_api_function("setuid")
			);
	}

	if (::getuid() == 0)
		throw std::runtime_error("cannot run as root");
}

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
			redis::Database db{ioc, cfg.redis_addr(), cfg.redis_port()};
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
	auto const threads = std::max(1UL, cfg.thread_count());

	boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23};
	ctx.set_options(
		boost::asio::ssl::context::default_workarounds |
		boost::asio::ssl::context::no_sslv2
	);
	ctx.use_certificate_chain_file(cfg.cert_chain().string());
	ctx.use_private_key_file(cfg.private_key().string(), boost::asio::ssl::context::pem);

	Server server{cfg};

	// The io_context is required for all I/O
	boost::asio::io_context ioc{static_cast<int>(threads)};

	// Create and launch a listening port for HTTP and HTTPS
	std::make_shared<Listener>(
		ioc,
		cfg.listen_http(),
		server,
		nullptr
	)->run();
	std::make_shared<Listener>(
		ioc,
		cfg.listen_https(),
		server,
		&ctx
	)->run();

	// make sure we load the certificates and listen before dropping root privileges
	drop_privileges();

	// Run the I/O service on the requested number of threads
	std::vector<std::thread> v;
	v.reserve(threads - 1);
	for (auto i = threads - 1; i > 0; --i)
		v.emplace_back(
			[&ioc]
			{ ioc.run(); }
		);
	ioc.run();
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
