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
	using Path      = boost::error_info<struct tag_path,    boost::filesystem::path>;
	using Message   = boost::error_info<struct tag_message, std::string>;
	using Offset    = boost::error_info<struct tag_offset,  std::size_t>;
	using ErrorCode = boost::error_info<struct tag_error_code,  std::error_code>;

public:
	Configuration(int argc, const char *const *argv, const char *env);

	boost::asio::ip::tcp::endpoint listen_http() const { return m_listen_http;}
	boost::asio::ip::tcp::endpoint listen_https() const { return m_listen_https;}
	boost::filesystem::path cert_chain() const {return m_cert_chain;}
	boost::filesystem::path private_key() const {return m_private_key;}
	boost::filesystem::path web_root() const {return m_root;}
	std::size_t thread_count() const {return m_thread_count;}
	std::string server_name() const {return m_server_name;}

	bool help() const {return m_help;}
	static void usage(std::ostream& out);

private:
	void load_config(const boost::filesystem::path& path);


private:
	bool m_help{false};

	boost::asio::ip::tcp::endpoint m_listen_http, m_listen_https;
	boost::filesystem::path m_cert_chain, m_private_key;
	boost::filesystem::path m_root;
	std::string m_server_name;
	std::size_t m_thread_count{0};
};

} // end of namespace
