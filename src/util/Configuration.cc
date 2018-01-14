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
#include "JsonHelper.hh"

#include "config.hh"

#include <boost/program_options.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>

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
			("cfg",  po::value<std::string>()->default_value(std::string{hrb::constants::config_filename}), "Configuration file")
		;
		return &self;
	};

	static const auto& result = *make_desc();
	return result;
}

boost::asio::ip::tcp::endpoint parse_endpoint(const rapidjson::Value& json)
{
	return {
		boost::asio::ip::make_address(hrb::json::string(json["address"])),
		static_cast<unsigned short>(json["port"].GetUint())
	};
}

} // end of local namespace

namespace hrb {

Configuration::Configuration(int argc, const char *const *argv, const char *env)
{
	po::variables_map config;
	store(po::parse_command_line(argc, argv, the_desc()), config);
	po::notify(config);

	m_help = config.count("help") > 0;

	// no need for other options when --help is specified
	if (!m_help)
		load_config(config["cfg"].as<std::string>());
}

void Configuration::usage(std::ostream &out)
{
	out << the_desc();
}

void Configuration::load_config(const boost::filesystem::path& path)
{
	try
	{
		using namespace rapidjson;
		std::ifstream config_file;
		config_file.open(path.string(), std::ios::in);
		if (!config_file)
		{
			BOOST_THROW_EXCEPTION(FileError()
				<< ErrorCode({errno, std::system_category()})
			);
		}

		IStreamWrapper wrapper{config_file};

		Document json;
		if (json.ParseStream(wrapper).HasParseError())
		{
			BOOST_THROW_EXCEPTION(Error()
				<< Offset{json.GetErrorOffset()}
				<< Message{GetParseError_En(json.GetParseError())}
			);
		}

		using namespace json;
		m_cert_chain    = string(field(json, "cert_chain"));
		m_private_key   = string(field(json, "private_key"));
		m_root          = absolute(string(field(json, "web_root")), path.parent_path()).lexically_normal();
		m_server_name   = string(field(json, "server_name"));

		m_listen_http  = parse_endpoint(field(json, "http"));
		m_listen_https = parse_endpoint(field(json, "https"));
	}
	catch (Exception& e)
	{
		e << Path{path};
		throw;
	}
	catch (std::exception& e)
	{
		throw boost::enable_error_info(e) << Path{path};
	}
}

} // end of namespace
