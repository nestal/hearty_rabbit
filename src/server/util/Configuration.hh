/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/6/18.
//

#pragma once

#include "Exception.hh"
#include "Size2D.hh"
#include "common/FS.hh"

#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem/path.hpp>

#include "config.hh"

#include <chrono>
#include <iosfwd>

namespace hrb {

struct SquareCropSetting
{
	boost::filesystem::path model_path{std::string{constants::haarcascades_path}};

	double face_size_ratio{0.05};
	double eye_size_ratio{0.5};
};

struct JPEGRenditionSetting
{
	Size2D  dim;
	int     quality{70};

	std::optional<SquareCropSetting> square_crop;
};

class RenditionSetting
{
public:
	RenditionSetting() = default;

	Size2D dimension(std::string_view rend = {}) const;
	int quality(std::string_view rend = {}) const;

	const JPEGRenditionSetting& find(std::string_view rend) const;

	bool valid(std::string_view rend) const;
	const std::string& default_rendition() const {return m_default;}
	void default_rendition(std::string_view rend) {m_default = rend;}

	void add(std::string_view rend, const JPEGRenditionSetting& setting);

private:
	std::string m_default{"2048x2048"};
	std::unordered_map<std::string, JPEGRenditionSetting> m_renditions{
		{m_default, JPEGRenditionSetting{{2048, 2048}, 70, std::nullopt}}
	};
};

/// \brief  Parsing command line options and configuration file
class Configuration
{
public:
	struct Error : virtual Exception {};
	struct FileError : virtual Error {};
	struct MissingUsername : virtual Error {};
	struct InvalidUserOrGroup: virtual Error {};
	using Path      = boost::error_info<struct tag_path,    boost::filesystem::path>;
	using Message   = boost::error_info<struct tag_message, std::string>;
	using Offset    = boost::error_info<struct tag_offset,  std::size_t>;
	using ErrorCode = boost::error_info<struct tag_error_code,  std::error_code>;

public:
	Configuration() = default;
	Configuration(int argc, const char *const *argv, const char *env);

	boost::asio::ip::tcp::endpoint listen_http() const { return m_listen_http;}
	boost::asio::ip::tcp::endpoint listen_https() const { return m_listen_https;}
	boost::asio::ip::tcp::endpoint redis() const {return m_redis;}

	boost::filesystem::path cert_chain() const {return m_cert_chain;}
	boost::filesystem::path private_key() const {return m_private_key;}
	boost::filesystem::path web_root() const {return m_root;}
	boost::filesystem::path blob_path() const {return m_blob_path;}
	std::size_t thread_count() const {return m_thread_count;}
	std::size_t upload_limit() const {return m_upload_limit;}
	uid_t user_id() const {return m_user_id;}
	gid_t group_id() const {return m_group_id;}
	const RenditionSetting& renditions() const {return m_rendition;}
	const std::string& server_name() const {return m_server_name;}
	std::string https_root() const;
	std::chrono::seconds session_length() const {return m_session_length;}

	bool help() const {return m_args.count("help") > 0;}

	template <typename AddUser>
	bool add_user(AddUser&& func) const
	{
		return m_args.count("add-user") > 0 ?
			(func(m_args["add-user"].as<std::string>()), true) :
			false;
	}
	template <typename Function>
	bool blob_id(Function&& func) const
	{
		return m_args.count("blob-id") > 0 ?
			(func(m_args["blob-id"].as<std::string>()), true) :
			false;
	}

	void usage(std::ostream& out) const;

	// for unit tests
	void blob_path(boost::filesystem::path path) {m_blob_path = std::move(path);}
	void change_listen_ports(std::uint16_t https, std::uint16_t http);

private:
	void load_config(const boost::filesystem::path& path);

private:
	boost::program_options::options_description m_desc{"Allowed options"};
	boost::program_options::variables_map       m_args;

	boost::asio::ip::tcp::endpoint m_listen_http, m_listen_https;
	boost::asio::ip::tcp::endpoint m_redis{
		boost::asio::ip::make_address("127.0.0.1"),
		6379
	};

	boost::filesystem::path m_cert_chain, m_private_key;
	boost::filesystem::path m_root, m_blob_path;
	std::string m_server_name;
	std::size_t m_thread_count{1};
	std::size_t m_upload_limit{10 * 1024 * 1024};

	RenditionSetting m_rendition;
	uid_t m_user_id{65535};
	gid_t m_group_id{65535};

	std::chrono::seconds m_session_length{3600};
};

} // end of namespace
