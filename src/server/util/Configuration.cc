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

#include <json.hpp>

#include <boost/program_options.hpp>
#include <boost/exception/info.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>

#include <fstream>

namespace po = boost::program_options;
namespace ip = boost::asio::ip;

namespace hrb {
namespace {

ip::tcp::endpoint parse_endpoint(const nlohmann::json& json)
{
	return {
		ip::make_address(json["address"].get<std::string>()),
		json["port"].get<unsigned short>()
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
		)->value_name("path"), "Configuration file. Use environment variable HEARTY_RABBIT_CONFIG to set default path.")
	;

	if (argc > 0)
	{
		store(po::parse_command_line(argc, argv, m_desc), m_args);
		po::notify(m_args);

	}

	// no need for other options when --help is specified
	if (!help())
		load_config(
			m_args.count("cfg") > 0 ? m_args["cfg"].as<std::string>() : env
	    );
}

void Configuration::usage(std::ostream &out) const
{
	out << m_desc;
}

void Configuration::load_config(const boost::filesystem::path& path)
{
	try
	{
		std::ifstream config_file;
		config_file.open(path.string(), std::ios::in);
		if (!config_file)
		{
			BOOST_THROW_EXCEPTION(FileError()
				<< ErrorCode({errno, std::system_category()})
			);
		}

		auto json = nlohmann::json::parse(config_file);
		using jptr = nlohmann::json::json_pointer;

		// Paths are relative to the configuration file
		m_cert_chain    = weakly_canonical(absolute(json.at(jptr{"/cert_chain"}).get<std::string>(),  path.parent_path()));
		m_private_key   = weakly_canonical(absolute(json.at(jptr{"/private_key"}).get<std::string>(), path.parent_path()));
		m_root          = weakly_canonical(absolute(json.at(jptr{"/web_root"}).get<std::string>(),    path.parent_path()));
		m_blob_path     = weakly_canonical(absolute(json.at(jptr{"/blob_path"}).get<std::string>(),   path.parent_path()));
		m_server_name   = json.at(jptr{"/server_name"});
		m_thread_count  = json.value(jptr{"/thread_count"}, m_thread_count);
		m_rendition.default_rendition(
			json.value(jptr{"/default_rendition"}, m_rendition.default_rendition())
		);
		m_upload_limit  = static_cast<std::size_t>(
			json.value(jptr{"/upload_limit_mb"}, m_upload_limit/1024.0/1024.0) * 1024 * 1024
		);
		if (json.find("rendition") != json.end())
		{
			for (auto&& rend : json["rendition"].items())
			{
				auto width  = rend.value().value("width", 0);
				auto height = rend.value().value("height", 0);
				if (width > 0 && height > 0)
					m_rendition.add(rend.key(), {width, height});
			}
		}
		m_session_length = std::chrono::seconds{json.value(jptr{"/session_length_in_sec"}, 3600L)};

		m_user_id  = json.value(jptr{"/uid"}, m_user_id);
		m_group_id = json.value(jptr{"/gid"}, m_group_id);
		if (m_user_id == 0 || m_group_id == 0)
			BOOST_THROW_EXCEPTION(InvalidUserOrGroup());

		m_listen_http   = parse_endpoint(json.at(jptr{"/http"}));
		m_listen_https  = parse_endpoint(json.at(jptr{"/https"}));

		if (auto redis = json.value(jptr{"/redis"}, nlohmann::json::object_t{}); !redis.empty())
			m_redis = parse_endpoint(redis);
	}
	catch (nlohmann::json::exception& e)
	{
		throw;
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

std::string Configuration::https_root() const
{
	using namespace std::literals;
	return "https://" + m_server_name
		+ (listen_https().port() == 443 ? ""s : (":"s + std::to_string(listen_https().port())));
}

void Configuration::change_https_port(std::uint16_t port)
{
	m_listen_https.port(port);
}

const JPEGRenditionSetting& RenditionSetting::find(std::string_view rend) const
{
	assert(m_renditions.find(std::string{m_default}) != m_renditions.end());

	if (rend.empty())
		rend = m_default;

	auto it = m_renditions.find(std::string{rend});
	return it != m_renditions.end() ? it->second : find(m_default);
}

Size2D RenditionSetting::dimension(std::string_view rend) const
{
	return find(rend).dim;
}

int RenditionSetting::quality(std::string_view rend) const
{
	return find(rend).quality;
}

bool RenditionSetting::valid(std::string_view rend) const
{
	return m_renditions.find(std::string{rend}) != m_renditions.end();
}

void RenditionSetting::add(std::string_view rend, Size2D dim, int quality)
{
	m_renditions.insert_or_assign(std::string{rend}, JPEGRenditionSetting{dim, quality});
}

} // end of namespace
