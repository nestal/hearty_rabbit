/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/7/18.
//

#pragma once

#include "Request.hh"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/string_body.hpp>

#include <functional>

namespace hrb {

// A mapping from HTTP requests and their handlers.

template <typename Body>
using ResponseSender  = std::function<void(http::response<Body>&&)>;

enum class SessionState {anonymous, user};

template <typename Body>
struct SiteEntry
{
	boost::string_view      url_prefix;
	http::verb              method;
	SessionState            session;
	std::function<void (Request&&, ResponseSender<Body>&&)> handler;
};

// Generic handler
void not_found(Request&& req, ResponseSender<http::string_body>&& send);

SiteEntry<http::string_body> string_map[] = {
	{"/blob", http::verb::get, SessionState::anonymous, not_found}
};

} // end of namespace
