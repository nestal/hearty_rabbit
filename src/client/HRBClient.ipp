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

#include "GenericHTTPRequest.hh"
#include "common/URLIntent.hh"
#include "common/CollEntry.hh"
#include "common/Collection.hh"
#include "common/ObjectID.hh"
#include "common/Error.hh"

#include <nlohmann/json.hpp>

#include <iostream>
#include <string>
#include <unordered_map>

namespace hrb {

template <typename Complete>
void HRBClient::login(std::string_view user, std::string_view password, Complete&& comp)
{
	// Launch the asynchronous operation
	auto req = std::make_shared<GenericHTTPRequest<http::string_body, http::string_body>>(m_ioc, m_ssl);
	req->init(m_host, m_port, "/login", http::verb::post);
	req->request().set(http::field::content_type, "application/x-www-form-urlencoded");
	req->request().body() = "username=" + std::string{user} + "&password=" + std::string{password};

	m_user = std::string{user};

	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		if (req.response().result() == http::status::no_content)
		{
			// reset cookie state
			m_cookie = Cookie{};
			m_cookie.add("id", Cookie{req.response().at(http::field::set_cookie)}.field("id"));
		}
		else
			ec = hrb::Error::login_incorrect;

		comp(ec);
		req.shutdown();
	});
	req->run();
}

template <typename Complete>
void HRBClient::list_collection(std::string_view coll, Complete&& comp)
{
	auto req = std::make_shared<GenericHTTPRequest<http::empty_body, http::string_body>>(m_ioc, m_ssl);
	req->init(m_host, m_port, URLIntent{URLIntent::Action::api, m_user, coll, ""}.str(), http::verb::get);
	req->request().set(http::field::cookie, m_cookie.str());

	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		auto json = nlohmann::json::parse(req.response().body());
		comp(json.template get<Collection>(), ec);
		req.shutdown();
	});
	req->run();
}

template <typename Complete>
void HRBClient::scan_collections(Complete&& comp)
{
	auto req = std::make_shared<GenericHTTPRequest<http::empty_body, http::string_body>>(m_ioc, m_ssl);
	req->init(m_host, m_port, URLIntent{URLIntent::QueryTarget::collection, "user=" + m_user + "&json"}.str(), http::verb::get);
	req->request().set(http::field::cookie, m_cookie.str());

	std::cout << URLIntent{URLIntent::QueryTarget::collection, "user=" + m_user + "&json"}.str() << std::endl;

	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		comp(nlohmann::json::parse(req.response().body()), ec);
		req.shutdown();
	});
	req->run();
}

} // end of namespace