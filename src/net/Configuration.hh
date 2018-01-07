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

#include <boost/asio/ip/tcp.hpp>
#include <boost/filesystem/path.hpp>

#include <iosfwd>

namespace hrb {

/// \brief  Parsing command line options and configuration file
class Configuration
{
public:
	Configuration(int argc, char **argv, const char *env);

	boost::asio::ip::tcp::endpoint listen_http() const { return m_listen_http;}
	boost::asio::ip::tcp::endpoint listen_https() const { return m_listen_https;}
	boost::filesystem::path cert_path() const {return m_cert_path;}
	boost::filesystem::path web_root() const {return m_root;}
	std::size_t thread_count() const {return m_thread_count;}

	bool help() const {return m_help;}
	static void usage(std::ostream& out);

private:
	static boost::filesystem::path choose_config_file();

private:
	bool m_help{false};

	boost::asio::ip::tcp::endpoint m_listen_http, m_listen_https;
	boost::filesystem::path m_cert_path;
	boost::filesystem::path m_root;
	std::size_t m_thread_count{0};
};

} // end of namespace