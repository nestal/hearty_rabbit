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
#include <boost/exception/info.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>

#include <fstream>
#include <iostream>

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
			("cfg",  po::value<std::string>()->default_value(std::string{hrb::constants::config_filename}), "Configuration file")
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
	po::variables_map config;
	store(po::parse_command_line(argc, argv, the_desc()), config);
	po::notify(config);

	auto config_path = boost::filesystem::path{config["cfg"].as<std::string>()};

	using namespace rapidjson;
	std::ifstream config_file{config_path.string()};
	IStreamWrapper wrapper{config_file};

	Document json;
	if (json.ParseStream(wrapper).HasParseError())
	{
		BOOST_THROW_EXCEPTION(Error()
			<< Path{config_path}
			<< Offset{json.GetErrorOffset()}
			<< Message{GetParseError_En(json.GetParseError())}
		);
	}

	m_help = config.count("help") > 0;

	// no need for other options when --help is specified
	if (!m_help)
	{
		m_cert_path = json["cert_path"].GetString();
		m_root      = json["web_root"].GetString();

		m_listen_http.address(boost::asio::ip::make_address(json["http"]["address"].GetString()));
		m_listen_http.port(static_cast<unsigned short>(json["http"]["port"].GetUint()));

		m_listen_https.address(boost::asio::ip::make_address(json["https"]["address"].GetString()));
		m_listen_https.port(static_cast<unsigned short>(json["https"]["port"].GetUint()));
	}
}

void Configuration::usage(std::ostream &out)
{
	out << the_desc();
}

} // end of namespace
