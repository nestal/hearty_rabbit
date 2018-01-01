#include "Listener.hh"

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <boost/filesystem.hpp>

#include <thread>
#include <iostream>

int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 5)
    {
        std::cerr <<
            "Usage: http-server-async <address> <port> <doc_root> <threads>\n" <<
            "Example:\n" <<
            "    http-server-async 0.0.0.0 8080 . 1\n";
        return EXIT_FAILURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    std::string const doc_root = argv[3];
    auto const threads = std::max<int>(1, std::atoi(argv[4]));

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
