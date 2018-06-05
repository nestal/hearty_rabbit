/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#pragma once

#include "HRBClient.hh"

namespace hrb {

template <typename Complete>
void HRBClient::login(std::string_view user, std::string_view password, Complete&& comp)
{
	// Launch the asynchronous operation
	auto req = std::make_shared<GenericHTTPRequest<http::string_body, http::string_body>>(m_ioc, m_ssl);
	req->init(m_host, m_port, "/login", http::verb::post);
	req->request().set(http::field::content_type, "application/x-www-form-urlencoded");
	req->request().body() = "username=" + std::string{user} + "&password=" + std::string{password};

	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		m_cookie = Cookie{req.response().at(http::field::set_cookie)};
		comp(ec);
	});
	req->run();
}

} // end of namespace
