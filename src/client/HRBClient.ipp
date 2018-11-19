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

#include "common/Cookie.hh"
#include "common/CollEntry.hh"
#include "common/Collection.hh"
#include "common/CollectionList.hh"
#include "common/Error.hh"
#include "common/Escape.hh"
#include "common/ObjectID.hh"
#include "common/URLIntent.hh"

#include <boost/beast/http/file_body.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>
#include <iostream>

namespace hrb {

template <typename Complete>
void HRBClient::login(std::string_view user, std::string_view password, Complete&& comp)
{
	std::string username{user};

	// Launch the asynchronous operation
	auto req = std::make_shared<GenericHTTPRequest<http::string_body, http::string_body>>(m_ioc, m_ssl);
	req->init(m_host, m_port, "/login", http::verb::post);
	req->request().set(http::field::content_type, "application/x-www-form-urlencoded");
	req->request().body() = "username=" + username + "&password=" + std::string{password};

	req->on_load([this, username, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		if (req.response().result() == http::status::no_content)
			m_user = UserID{Cookie{req.response().at(http::field::set_cookie)}, username};
		else
			ec = Error::login_incorrect;

		comp(ec);
		req.shutdown();
	});
	req->run();
}

template <typename Complete>
void HRBClient::list_collection(std::string_view coll, Complete&& comp)
{
	auto req = request<http::empty_body, http::string_body>({URLIntent::Action::api, m_user.username(), coll, ""}, http::verb::get);
	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		req.shutdown();

		if (!ec && req.response().result() == http::status::ok)
		{
			if (auto json = nlohmann::json::parse(req.response().body(), nullptr, false); !json.is_discarded())
			{
				comp(json.template get<Collection>(), ec);
				return;
			}
		}

		comp(Collection{}, ec ? ec : make_error_code(Error::unknown_error));
	});
	req->run();
}

template <typename Complete>
void HRBClient::scan_collections(Complete&& comp)
{
	auto req = request<http::empty_body, http::string_body>({URLIntent::QueryTarget::collection, "json&user=" + m_user.username()}, http::verb::get);
	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		comp(nlohmann::json::parse(req.response().body()).template get<CollectionList>(), ec);
		req.shutdown();
	});
	req->run();
}

template <typename Complete>
void HRBClient::upload(std::string_view coll, const fs::path& file, Complete&& comp)
{
	auto req = request<http::file_body, http::string_body>({
		URLIntent::Action::upload, m_user.username(), coll, file.filename().string()
	}, http::verb::put);

	boost::system::error_code err;
	req->request().body().open(file.string().c_str(), boost::beast::file_mode::read, err);
	if (err)
		throw std::system_error{err.value(), err.category()};

	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req) mutable
	{
		handle_upload_response(req.response(), std::forward<Complete>(comp), ec);
		req.shutdown();
	});
	req->run();
}

template <typename Complete, typename ByteIterator>
void HRBClient::upload(std::string_view coll, std::string_view filename, ByteIterator first_byte, ByteIterator last_byte, Complete&& comp)
{
	auto req = request<http::string_body, http::string_body>({
		URLIntent::Action::upload, m_user.username(), coll, filename
	}, http::verb::put);
	req->request().body().assign(first_byte, last_byte);
	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req) mutable
	{
		handle_upload_response(req.response(), std::forward<Complete>(comp), ec);
		req.shutdown();
	});
	req->run();
}

template <typename RequestBody, typename ResponseBody>
auto HRBClient::request(const URLIntent& intent, boost::beast::http::verb method)
{
	auto req = std::make_shared<GenericHTTPRequest<RequestBody, ResponseBody>>(m_ioc, m_ssl);
	req->init(m_host, m_port, intent.str(), method);
	req->request().set(http::field::cookie, m_user.cookie().str());
	return req;
}

template <typename Complete>
void HRBClient::get_blob(std::string_view owner, std::string_view coll, const ObjectID& blob, Complete&& comp)
{
	auto req = request<http::empty_body, http::string_body>({
		URLIntent::Action::api, owner, coll, blob
	}, http::verb::get);
	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		comp(req.response().body(), ec);
		req.shutdown();
	});
	req->run();
}

template <typename Complete>
void HRBClient::get_blob_meta(std::string_view owner, std::string_view coll, const ObjectID& blob, Complete&& comp)
{
	auto req = request<http::empty_body, http::string_body>({
		URLIntent::Action::api, owner, coll, blob, "&json"
	}, http::verb::get);
	req->on_load([this, comp=std::forward<Complete>(comp)](auto ec, auto& req)
	{
		comp(nlohmann::json::parse(req.response().body()), ec);
		req.shutdown();
	});
	req->run();

}

template <typename Complete, typename Response>
void HRBClient::handle_upload_response(Response& response, Complete&& comp, std::error_code ec)
{
	// TODO: return better error code
	if (response.result() != http::status::created)
		ec = Error::unknown_error;

	comp(response.count(http::field::location) > 0 ? URLIntent{response.at(http::field::location)} : URLIntent{}, ec);
}

} // end of namespace
