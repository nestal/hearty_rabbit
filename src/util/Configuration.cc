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
#include <rapidjson/pointer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>

#include <fstream>

namespace po = boost::program_options;
namespace ip = boost::asio::ip;

namespace hrb {
namespace {

ip::tcp::endpoint parse_endpoint(const rapidjson::Value& json)
{
	return {
		ip::make_address(json::string(json["address"])),
		static_cast<unsigned short>(json["port"].GetUint())
	};
}

} // end of local namespace

Configuration::Configuration(int argc, const char *const *argv, const char *env)
{
	using namespace std::literals;
	m_desc.add_options()
		("help",      "produce help message")
		("add-user",  po::value<std::string>()->value_name("username"), "add a new user given a user name")
		("blob-id",   po::value<std::string>()->value_name("filename"), "calculate the blob object ID of a given file")
		("cfg",       po::value<std::string>()->default_value(
			env ? std::string{env} : std::string{hrb::constants::config_filename}
		)->value_name("path"), "Configuration file. Use environment variable HEART_RABBIT_CONFIG to set default path.")
	;

	store(po::parse_command_line(argc, argv, m_desc), m_args);
	po::notify(m_args);

	// no need for other options when --help is specified
	if (!help())
		load_config(m_args["cfg"].as<std::string>());
}

void Configuration::usage(std::ostream &out) const
{
	out << m_desc;
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

		// Paths are relative to the configuration file
		m_cert_chain    = weakly_canonical(absolute(string(required(json, "/cert_chain")),  path.parent_path()));
		m_private_key   = weakly_canonical(absolute(string(required(json, "/private_key")), path.parent_path()));
		m_root          = weakly_canonical(absolute(string(required(json, "/web_root")),    path.parent_path()));
		m_blob_path     = weakly_canonical(absolute(string(required(json, "/blob_path")),   path.parent_path()));
		m_server_name   = string(required(json, "/server_name"));
		m_thread_count  = GetValueByPointerWithDefault(json, "/thread_count", m_thread_count).GetUint64();
		m_upload_limit  = static_cast<std::size_t>(
			GetValueByPointerWithDefault(json, "/upload_limit_mb", m_upload_limit/1024.0/1024.0).GetDouble() *
				1024 * 1024
		);
		m_img_dim.assign(
			GetValueByPointerWithDefault(json, "/image_dimension/width", m_img_dim.width()).GetInt(),
			GetValueByPointerWithDefault(json, "/image_dimension/height", m_img_dim.height()).GetInt()
		);

		m_listen_http   = parse_endpoint(required(json, "/http"));
		m_listen_https  = parse_endpoint(required(json, "/https"));

		if (auto redis = Pointer{"/redis"}.Get(json))
			m_redis = parse_endpoint(*redis);
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
