/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/7/18.
//

#include "SiteMap.hh"

namespace hrb {

void not_found(Request&& req, ResponseSender<http::string_body>&& send)
{
	using namespace std::literals;
	http::response<http::string_body> res{
		std::piecewise_construct,
		std::make_tuple("The resource '"s + req.target().to_string() + "' was not found."),
		std::make_tuple(http::status::not_found, req.version())
	};
	res.set(http::field::content_type, "text/plain");
	res.prepare_payload();
	return send(std::move(res));
}

} // end of namespace
