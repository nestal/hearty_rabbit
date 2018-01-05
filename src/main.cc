#include "Listener.hh"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <systemd/sd-journal.h>

#include <thread>
#include <iostream>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("address,a", po::value<std::string>()->default_value("0.0.0.0"),               "IP address of the network interface to listener to")
		("port,p",    po::value<unsigned short>()->default_value(80),                   "Port number to listener to")
		("root,r",    po::value<std::string>()->default_value(INSTALL_PREFIX "/lib"),   "Album directory")
		("threads",   po::value<int>()->default_value(1),                               "Number of threads")
		("cert-path", po::value<std::string>(),                                         "Path to certs")
	;

	std::ifstream config_file{INSTALL_PREFIX "/etc/hearty_rabbit.conf"};

    po::variables_map config;
	store(po::parse_command_line(argc, argv, desc), config);
	store(po::parse_config_file(config_file, desc), config);
	po::notify(config);

	if (config.count("help"))
	{
		std::cout << desc << "\n";
		return 1;
	}

	sd_journal_print(LOG_NOTICE, "hearty_rabbit starting");

    auto const threads = std::max<int>(1, config["threads"].as<int>());

	boost::filesystem::path cert_path{config["cert-path"].as<std::string>()};

	boost::asio::ssl::context ctx{boost::asio::ssl::context::sslv23};
	ctx.set_options(
        boost::asio::ssl::context::default_workarounds |
	    boost::asio::ssl::context::no_sslv2
	);
	ctx.use_certificate_chain_file((cert_path/"fullchain.pem").string());
	ctx.use_private_key_file((cert_path/"privkey.pem").string(), boost::asio::ssl::context::pem);

    // The io_context is required for all I/O
    boost::asio::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<hrb::Listener>(
	    ioc,
        boost::asio::ip::tcp::endpoint{
	        boost::asio::ip::make_address(config["address"].as<std::string>()),
	        config["port"].as<unsigned short>()
        },
        config["root"].as<std::string>(),
	    ctx
    )->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(static_cast<std::size_t>(threads - 1));
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back([&ioc]{ ioc.run(); });
    ioc.run();

    return EXIT_SUCCESS;
}
