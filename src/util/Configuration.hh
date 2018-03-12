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
#include "Size.hh"

#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem/path.hpp>

#include <iosfwd>

namespace hrb {

/// \brief  Parsing command line options and configuration file
class Configuration
{
public:
	struct Error : virtual Exception {};
	struct FileError : virtual Error {};
	struct MissingUsername : virtual Error {};
	using Path      = boost::error_info<struct tag_path,    boost::filesystem::path>;
	using Message   = boost::error_info<struct tag_message, std::string>;
	using Offset    = boost::error_info<struct tag_offset,  std::size_t>;
	using ErrorCode = boost::error_info<struct tag_error_code,  std::error_code>;

public:
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
	Size image_dimension() const {return m_img_dim;}

	bool help() const {return m_args.count("help") > 0;}

	template <typename AddUser>
	bool add_user(AddUser&& func)
	{
		return m_args.count("add-user") > 0 ?
			(func(m_args["add-user"].as<std::string>()), true) :
			false;
	}
	template <typename Function>
	bool blob_id(Function&& func)
	{
		return m_args.count("blob-id") > 0 ?
			(func(m_args["blob-id"].as<std::string>()), true) :
			false;
	}

	void usage(std::ostream& out) const;

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
	std::size_t m_thread_count{1};
	std::size_t m_upload_limit{10 * 1024 * 1024};

	Size m_img_dim{2048, 2048};
};

} // end of namespace
