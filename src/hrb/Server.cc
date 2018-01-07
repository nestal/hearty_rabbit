/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#include "Server.hh"

namespace hrb {

Server::Server(const Configuration& cfg) :
	m_cfg{cfg}
{
}

http::response<boost::beast::http::empty_body> Server::redirect(boost::beast::string_view where, unsigned version)
{
	http::response<http::empty_body> res{http::status::moved_permanently, version};
	res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
	res.set(http::field::location, where);
	res.keep_alive(true);
	return res;
}

} // end of namespace
