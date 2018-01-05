/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/6/18.
//

#include "Configuration.hh"
#include "config.hh"

#include <boost/program_options.hpp>

#include <fstream>

namespace hrb {

Configuration::Configuration(int argc, char **argv, const char *env)
{
	namespace po = boost::program_options;
	using namespace std::literals;

	// Declare the supported options.
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("address,a", po::value<std::string>()->default_value("0.0.0.0"),               "IP address of the network interface to listener to")
		("port,p",    po::value<unsigned short>()->default_value(80),                   "Port number to listener to")
		("root,r",    po::value<std::string>()->default_value(std::string{hrb::constants::install_prefix} + "/lib"s),   "Album directory")
		("threads",   po::value<int>()->default_value(1),                               "Number of threads")
		("cert-path", po::value<std::string>(),                                         "Path to certs")
	;

	std::ifstream config_file{choose_config_file().string()};

    po::variables_map config;
	store(po::parse_command_line(argc, argv, desc), config);
	store(po::parse_config_file(config_file, desc), config);
	po::notify(config);

	m_help = config.count("help") > 0;

	m_cert_path = config["cert-path"].as<std::string>();
	m_root      = config["root"].as<std::string>();
	m_listen.address(boost::asio::ip::make_address(config["address"].as<std::string>()));
	m_listen.port(config["port"].as<unsigned short>());
}

boost::filesystem::path Configuration::choose_config_file()
{
	return boost::filesystem::path{std::string{constants::install_prefix}} / std::string{constants::config_filename};
}

} // end of namespace
