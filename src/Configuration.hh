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

namespace hrb {

class Configuration
{
public:
	Configuration(int argc, char **argv, const char *env);

	boost::asio::ip::tcp::endpoint address() const { return m_address;}
	std::uint16_t port() const {return m_port;}
	boost::filesystem::path cert_path() const {return m_cert_path;}
	std::size_t thread_count() const {return m_thread_count;}

private:
	static boost::filesystem::path choose_config_file();

private:
	boost::asio::ip::tcp::endpoint m_address;
	std::uint16_t m_port{0};
	boost::filesystem::path m_cert_path;
	std::size_t m_thread_count{0};
};

} // end of namespace
