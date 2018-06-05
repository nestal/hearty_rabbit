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
#include "common/ObjectID.hh"

#include <json.hpp>

#include <boost/range/adaptors.hpp>

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
		// reset cookie state
		m_cookie = Cookie{};
		m_cookie.add("id", Cookie{req.response().at(http::field::set_cookie)}.field("id"));

		comp(ec);
	});
	req->run();
}

template <typename Complete>
void HRBClient::list(std::string_view coll, Complete&& comp)
{
	auto req = std::make_shared<GenericHTTPRequest<http::empty_body, http::string_body>>(m_ioc, m_ssl);
	req->init(m_host, m_port, URLIntent{URLIntent::Action::api, m_user, coll, ""}.str(), http::verb::get);
	req->request().set(http::field::cookie, m_cookie.str());

	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		using namespace boost::adaptors;

		auto json = nlohmann::json::parse(req.response().body());
		std::unordered_map<ObjectID, CollEntry> result;
		for (auto&& item : json["elements"].items())
		{
			nlohmann::json key   = item.key();
			nlohmann::json value = item.value();

			if (auto blob = hrb::hex_to_object_id(key.get<std::string>()); blob.has_value())
				result.emplace(*blob, value.get<CollEntry>());
		}

		comp(std::move(result));
	});
	req->run();
}

} // end of namespace