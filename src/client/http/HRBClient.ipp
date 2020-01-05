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

#include "util/Cookie.hh"
#include "hrb/BlobInode.hh"
#include "hrb/Collection.hh"
#include "hrb/CollectionList.hh"
#include "util/Error.hh"
#include "util/Escape.hh"
#include "hrb/ObjectID.hh"
#include "hrb/URLIntent.hh"

#include <boost/beast/http/file_body.hpp>

#include <nlohmann/json.hpp>

#include <string>
#include <unordered_map>

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

	req->on_load(
		[this, username, comp = std::forward<Complete>(comp)](auto ec, auto& req)
		{
			finish_request(req.shared_from_this());

			if (req.response().result() == http::status::no_content)
				m_user = UserID{Cookie{req.response().at(http::field::set_cookie)}, username};
			else
				ec = Error::login_incorrect;

			comp(ec);
			req.shutdown();
		}
	);
	add_request(std::move(req));
}

template <typename Complete>
void HRBClient::list_collection(std::string_view coll, Complete&& comp)
{
	auto req = request<http::empty_body, http::string_body>(
		{URLIntent::Action::api, m_user.username(), coll, ""},
		http::verb::get
	);
	req->on_load(
		[this, comp = std::forward<Complete>(comp)](auto ec, auto& req)
		{
			finish_request(req.shared_from_this());
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
		}
	);
	add_request(std::move(req));
}

template <typename Complete>
void HRBClient::scan_collections(Complete&& comp)
{
	auto req = request<http::empty_body, http::string_body>(
		{
			URLIntent::QueryTarget::collection,
			"json&user=" + m_user.username()
		}, http::verb::get
	);
	req->on_load(
		[this, comp = std::forward<Complete>(comp)](auto ec, auto& req)
		{
			finish_request(req.shared_from_this());

			comp(nlohmann::json::parse(req.response().body()).template get<CollectionList>(), ec);
			req.shutdown();
		}
	);
	add_request(std::move(req));
}

template <typename Complete>
void HRBClient::upload(std::string_view coll, const fs::path& file, Complete&& comp)
{
	auto req = request<http::file_body, http::string_body>(
		{
			URLIntent::Action::upload, m_user.username(), coll, file.filename().string()
		}, http::verb::put
	);

	boost::system::error_code err;
	req->request().body().open(file.string().c_str(), boost::beast::file_mode::read, err);
	if (err)
		throw std::system_error{err.value(), err.category()};

	req->on_load(
		[this, comp = std::forward<Complete>(comp)](auto ec, auto& req) mutable
		{
			finish_request(req.shared_from_this());

			handle_upload_response(req.response(), std::forward<Complete>(comp), ec);
			req.shutdown();
		}
	);
	add_request(std::move(req));
}

template <typename Complete, typename ByteIterator>
void HRBClient::upload(
	std::string_view coll,
	std::string_view filename,
	ByteIterator first_byte,
	ByteIterator last_byte,
	Complete&& comp
)
{
	auto req = request<http::string_body, http::string_body>(
		{URLIntent::Action::upload, m_user.username(), coll, filename},
		http::verb::put
	);
	req->request().body().assign(first_byte, last_byte);
	req->on_load(
		[this, comp = std::forward<Complete>(comp)](auto ec, auto& req) mutable
		{
			finish_request(req.shared_from_this());
			handle_upload_response(req.response(), std::forward<Complete>(comp), ec);
			req.shutdown();
		}
	);
	add_request(std::move(req));
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
void HRBClient::get_blob(
	std::string_view owner,
	std::string_view coll,
	const ObjectID& blob,
	std::string_view rendition,
	Complete&& comp
)
{
	auto req = request<http::empty_body, http::string_body>(
		{
			URLIntent::Action::api, owner, coll, blob, "rendition=" + std::string{rendition}
		}, http::verb::get
	);
	req->on_load(
		[this, comp = std::forward<Complete>(comp)](auto ec, auto& req)
		{
			finish_request(req.shared_from_this());
			comp(req.response().body(), ec);
			req.shutdown();
		}
	);
	add_request(std::move(req));
}

template <typename Complete>
void HRBClient::download_blob(
	std::string_view owner,
	std::string_view coll,
	const ObjectID& blob,
	std::string_view rendition,
	const std::filesystem::path& dest,
	Complete&& comp
)
{
	auto req = request<http::empty_body, http::file_body>(
		{
			URLIntent::Action::api, owner, coll, blob, "rendition=" + std::string{rendition}
		}, http::verb::get
	);

	boost::system::error_code ec;
	req->response().body().open(dest.c_str(), boost::beast::file_mode::write, ec);
	req->set_body_limit(20 * 1024 * 1024);
	if (ec)
	{
		comp(req->response().body(), ec);
	}
	else
	{
		req->on_load(
			[this, comp = std::forward<Complete>(comp)](auto ec, auto& req)
			{
				finish_request(req.shared_from_this());
				comp(req.response().body(), ec);
				req.shutdown();
			}
		);
		add_request(std::move(req));
	}
}

template <typename Complete>
void HRBClient::get_blob_meta(std::string_view owner, std::string_view coll, const ObjectID& blob, Complete&& comp)
{
	auto req = request<http::empty_body, http::string_body>(
		{
			URLIntent::Action::api, owner, coll, blob, "&json"
		}, http::verb::get
	);
	req->on_load(
		[this, comp = std::forward<Complete>(comp)](auto ec, auto& req)
		{
			finish_request(req.shared_from_this());
			comp(nlohmann::json::parse(req.response().body()), ec);
			req.shutdown();
		}
	);
	add_request(std::move(req));
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
