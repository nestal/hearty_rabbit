/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

#include "util/Listener.hh"
#include "net/Configuration.hh"

#include <boost/asio/ssl.hpp>
#include <boost/asio/bind_executor.hpp>

#include <systemd/sd-journal.h>

#include <thread>
#include <iostream>
#include <cstdlib>

int main(int argc, char *argv[])
{
	using namespace hrb;
	Configuration cfg{argc, argv, ::getenv("HEART_RABBIT_CONFIG")};

	if (cfg.help())
	{
		cfg.usage(std::cout);
		std::cout << "\n";
	}
	else
	{
		sd_journal_print(LOG_NOTICE, "hearty_rabbit starting");

		auto const threads = std::max(1UL, cfg.thread_count());

		boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23};
		ctx.set_options(
			boost::asio::ssl::context::default_workarounds |
			boost::asio::ssl::context::no_sslv2
		);
		ctx.use_certificate_chain_file((cfg.cert_path() / "fullchain.pem").string());
		ctx.use_private_key_file((cfg.cert_path() / "privkey.pem").string(), boost::asio::ssl::context::pem);

		// The io_context is required for all I/O
		boost::asio::io_context ioc{static_cast<int>(threads)};

		// Create and launch a listening port for HTTP and HTTPS
		std::make_shared<Listener>(
			ioc,
			cfg.listen_http(),
			cfg.web_root(),
			nullptr
		)->run();
		std::make_shared<Listener>(
			ioc,
			cfg.listen_https(),
			cfg.web_root(),
			&ctx
		)->run();

		// Run the I/O service on the requested number of threads
		std::vector<std::thread> v;
		v.reserve(threads - 1);
		for (auto i = threads - 1; i > 0; --i)
			v.emplace_back(
				[&ioc]{ ioc.run(); }
			);
		ioc.run();
	}
	return EXIT_SUCCESS;
}

