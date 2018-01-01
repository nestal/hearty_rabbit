#include "Listener.hh"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <thread>
#include <iostream>

namespace po = boost::program_options;

int main(int argc, char* argv[])
{
	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("address,a", po::value<std::string>()->default_value("0.0.0.0"),    "IP address of the network interface to listener to")
		("port,p",    po::value<unsigned short>()->default_value(80),        "Port number to listener to")
		("root,r",    po::value<std::string>()->default_value("."),          "Album directory")
		("threads",   po::value<int>()->default_value(1),                    "Number of threads")
	;

    po::variables_map config;
	po::store(po::parse_command_line(argc, argv, desc), config);
	po::notify(config);

	if (config.count("help"))
	{
		std::cout << desc << "\n";
		return 1;
	}

	auto const address = boost::asio::ip::make_address(config["address"].as<std::string>());
    auto const port = config["port"].as<unsigned short>();
    std::string const doc_root = config["root"].as<std::string>();
    auto const threads = std::max<int>(1, config["threads"].as<int>());

    std::cout << address << " " << port << std::endl;

    // The io_context is required for all I/O
    boost::asio::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<hrb::Listener>(
	    ioc,
        boost::asio::ip::tcp::endpoint{address, port},
        doc_root)->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(static_cast<std::size_t>(threads - 1));
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back([&ioc]{ ioc.run(); });
    ioc.run();

    return EXIT_SUCCESS;
}
