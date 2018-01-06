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

namespace po = boost::program_options;

namespace {

const po::options_description& the_desc()
{
	using namespace std::literals;
	auto make_desc = []
	{
		// Declare the supported options.
		static po::options_description self{"Allowed options"};
		self.add_options()
			("help", "produce help message")
			("address,a", po::value<std::string>()->default_value("0.0.0.0"),               "IP address of the network interface to listener to")
			("port,p",    po::value<unsigned short>()->default_value(80),                   "Port number to listener to")
			("root,r",    po::value<std::string>()->default_value(std::string{hrb::constants::install_prefix} + "/lib"s),   "Album directory")
			("threads",   po::value<int>()->default_value(1),                               "Number of threads")
			("cert-path", po::value<std::string>(),                                         "Path to certs")
		;
		return &self;
	};

	static const auto& result = *make_desc();
	return result;
}

}

namespace hrb {

Configuration::Configuration(int argc, char **argv, const char *env)
{
	auto& desc = the_desc();

	std::ifstream config_file{choose_config_file().string()};

    po::variables_map config;
	store(po::parse_command_line(argc, argv, desc), config);
	store(po::parse_config_file(config_file, desc), config);
	po::notify(config);

	m_help = config.count("help") > 0;

	// no need for other options when --help is specified
	if (!m_help)
	{
		m_cert_path = config["cert-path"].as<std::string>();
		m_root = config["root"].as<std::string>();
		m_listen.address(boost::asio::ip::make_address(config["address"].as<std::string>()));
		m_listen.port(config["port"].as<unsigned short>());
	}
}

boost::filesystem::path Configuration::choose_config_file()
{
	return boost::filesystem::path{std::string{constants::install_prefix}} / std::string{constants::config_filename};
}

void Configuration::usage(std::ostream &out)
{
	out << the_desc();
}

} // end of namespace
