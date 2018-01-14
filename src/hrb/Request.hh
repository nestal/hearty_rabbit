/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/14/18.
//

#pragma once

#include <boost/beast/http/message.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http/string_body.hpp>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

using Request = http::request<http::string_body>;
using EndPoint = boost::asio::ip::tcp::endpoint;

}
