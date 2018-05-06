/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/12/18.
//

#pragma once

#include "SessionHandler.hh"

#include "BlobRequest.hh"
#include "BlobDatabase.hh"
#include "Ownership.ipp"
#include "UploadFile.hh"
#include "URLIntent.hh"
#include "WebResources.hh"

#include "crypto/Authentication.hh"
#include "crypto/Authentication.ipp"
#include "net/MMapResponseBody.hh"
#include "util/Log.hh"
#include "util/Escape.hh"

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/version.hpp>

namespace hrb {

template <class Send>
class SessionHandler::SendJSON
{
public:
	SendJSON(
		Send&& send,
		const Authentication& auth,
		unsigned version,
		std::optional<ObjectID> blob,
		std::string&& server_root,
		const WebResources *lib = nullptr
	) :
		m_send{std::move(send)},
		m_auth{auth},
		m_version{version},
		m_blob{blob},
		m_server_root{std::move(server_root)},
		m_lib{lib}
	{
	}

	auto operator()(nlohmann::json&& json, std::error_code ec) const
	{
		if (!m_auth.is_guest() && m_auth.valid())
			json.emplace("username", m_auth.user());
		if (m_auth.is_guest())
			json.emplace("auth", to_hex(m_auth.cookie()));
		if (m_blob)
			json.emplace("blob", to_hex(*m_blob));

		auto result = ec ? http::status::internal_server_error : http::status::ok;
		if (m_lib)
		{
			using jptr = nlohmann::json::json_pointer;
			auto&& cover = json.value(jptr{"/blob"},       json.value(jptr{"/meta/cover"}, std::string{""}));
			auto&& owner = json.value(jptr{"/owner"},      std::string{""});
			auto&& coll  = json.value(jptr{"/collection"}, std::string{""});

			static const boost::format fmt{R"(<meta property="og:image" content="%1%%2%">
	<meta property="og:url" content="%1%%3%">
	<meta property="og:title" content="Hearty Rabbit: %4%">
	)"};
			URLIntent cover_url{URLIntent::Action::api, owner, coll, cover};
			URLIntent view_url{URLIntent::Action::view, owner, coll, "Hearty Rabbit"};
			if (m_auth.is_guest())
			{
				auto auth = "auth=" + to_hex(m_auth.cookie());
				cover_url.set_option(auth);
				view_url.set_option(auth);
			}

			return m_send(m_lib->inject(
				result,
				json.dump(),
				(
					boost::format{fmt} % m_server_root %
					cover_url.str() %
					view_url.str() %
					coll
				).str(),
				m_version)
			);
		}
		else
		{
			http::response<http::string_body> res{
				std::piecewise_construct,
				std::make_tuple(json.dump()),
				std::make_tuple(result, m_version)
			};
			res.set(http::field::content_type, "application/json");
			return m_send(std::move(res));
		}
	}
private:
	mutable Send    m_send;
	Authentication  m_auth;
	unsigned        m_version;
	std::optional<ObjectID> m_blob;
	std::string     m_server_root;
	const WebResources *m_lib;
};

/// \arg    header      The header we just received. This reference must be valid
///						until \a complete() is called.
/// \arg    src         The request_parser that produce \a header. It will be moved
///                     to \a dest. Must be valid until \a complete() is called.
template <class Complete>
void SessionHandler::on_request_header(
	const RequestHeader& header,
	Complete&& complete
)
{
	URLIntent intent{header.target()};

	// Use StringRequestParser to parser login requests.
	// The username/password will be stored in the string body.
	// No need to verify session.
	if (intent.action() == URLIntent::Action::login)
		return complete(RequestBodyType::string, std::error_code{});

	// Everything else require a valid session.
	auto cookie = header[http::field::cookie];
	m_request_cookie = parse_cookie({cookie.data(), cookie.size()});
	if (!m_request_cookie)
	{
		auto [auth_str] = find_fields(intent.option(), "auth");
		auto auth_key = hex_to_array<Authentication::Cookie{}.size()>(auth_str);
		if (header.method() == http::verb::get && auth_key)
		{
			Authentication auth{*auth_key, intent.user(), true};
			return auth.is_shared_resource(
				intent.collection(),
				*m_db,
				[complete = std::forward<Complete>(complete), auth, this, intent](bool shared, auto err)
				{
					assert(auth.is_guest());
					if (!err && shared)
						m_auth = auth;

					complete(RequestBodyType::empty, err);
				}
			);
		}
		else
		{
			return complete(RequestBodyType::empty, std::error_code{});
		}
	}

	Authentication::verify_session(
		*m_request_cookie,
		*m_db,
		session_length(),
		[
			this,
			action=intent.action(),
			method=header.method(),
			complete=std::forward<Complete>(complete)
		](std::error_code ec, const Authentication& auth) mutable
		{
			m_auth = auth;

			auto body_type = RequestBodyType::empty;

			// Use a UploadRequestParse to parser upload requests, only when the session is authenticated.
			if (!ec && action == URLIntent::Action::upload && method == http::verb::put)
				body_type = RequestBodyType::upload;

			// blobs support post request
			else if (!ec && action == URLIntent::Action::api && method == http::verb::post)
				body_type = RequestBodyType::string;

			// If the cookie returned by verify_session() is different from the one we passed to it,
			// that mean it is going to expired and it's renewed.
			// In this case we want to tell Session to put it in the "Set-Cookie" header.
			complete(body_type, ec);
		}
	);
}

template <class Request, class Send>
void SessionHandler::on_request_body(Request&& req, Send&& send)
{
	URLIntent intent{req.target()};
	if (intent.action() == URLIntent::Action::login)
	{
		if constexpr (std::is_same<std::remove_reference_t<Request>, StringRequest>::value)
		{
			if (req.method() == http::verb::post)
				return on_login(req, std::forward<Send>(send));
		}

		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});
	}

	// on_request_view() is a function template on the request type. It can work with all
	// request types so no need to check before calling.
	if (intent.action() == URLIntent::Action::view)
		return on_request_view(std::forward<Request>(req), std::move(intent), std::forward<Send>(send));

	if (intent.action() == URLIntent::Action::api)
		return on_request_api(std::forward<Request>(req), std::move(intent), std::forward<Send>(send));

	// The following URL only support EmptyRequests, i.e. requests without body.
	if constexpr (std::is_same<std::remove_reference_t<Request>, EmptyRequest>::value)
	{
		if (req.method() != http::verb::get)
			return send(bad_request("unsupported HTTP method", req.version()));

		if (intent.action() == URLIntent::Action::lib)
			return send(file_request(intent, req[http::field::if_none_match], req.version()));

		if (intent.action() == URLIntent::Action::home)
			return m_auth.valid() ?
				Ownership{m_auth.user()}.scan_all_collections(
					*m_db,
					SendJSON{std::move(send), m_auth, req.version(), std::nullopt, server_root(), &m_lib}
				) :
				Ownership{m_auth.user()}.list_public_blobs(
					*m_db,
					SendJSON{std::move(send), m_auth, req.version(), std::nullopt, server_root(), &m_lib}
				);

		if (intent.action() == URLIntent::Action::query)
			return on_query({std::move(req), std::move(intent)}, std::forward<Send>(send));

		if (intent.action() == URLIntent::Action::logout)
			return on_logout(std::forward<Request>(req), std::forward<Send>(send));
	}

	// Upload requests for /upload URL only
	if constexpr (std::is_same<std::remove_reference_t<Request>, UploadRequest>::value)
	{
		if (intent.action() == URLIntent::Action::upload)
			return on_upload(std::move(req), std::forward<Send>(send));
	}

	return send(not_found(req.target(), req.version()));
}

template <class Request, class Send>
void SessionHandler::on_request_api(Request&& req, URLIntent&& intent, Send&& send)
{
	BlobRequest breq{req, std::move(intent)};

	if (req.method() == http::verb::delete_)
	{
		return unlink(std::move(breq), std::move(send));
	}
	else if (req.method() == http::verb::get)
	{
		if (breq.blob())
			return get_blob(std::move(breq), std::move(send));
		else
			return Ownership{breq.owner()}.serialize(
				*m_db,
				m_auth,
				breq.collection(),
				SendJSON{std::move(send), m_auth, req.version(), std::nullopt, server_root()}
			);
	}
	else if (req.method() == http::verb::post)
	{
		if (breq.blob())
			return update_blob(std::move(breq), std::move(send));
		else
			return update_view(std::move(breq), std::move(send));
	}
	else
	{
		Log(LOG_WARNING, "/api request error: bad method %1%", req.method_string());
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});
	}
}

template <class Request, class Send>
void SessionHandler::on_request_view(Request&& req, URLIntent&& intent, Send&& send)
{
	BlobRequest breq{req, std::move(intent)};
	if (req.method() == http::verb::get)
	{
		// view request always sends HTML: pass &m_lib to SendJSON
		return Ownership{breq.owner()}.serialize(
			*m_db,
			m_auth,
			breq.collection(),
			SendJSON{std::move(send), m_auth, breq.version(), breq.blob(), server_root(), &m_lib}
		);
	}
	else
	{
		Log(LOG_WARNING, "/view request error: bad method %1%", req.method_string());
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});
	}
}

template <class Send>
void SessionHandler::get_blob(const BlobRequest& req, Send&& send)
{
	assert(req.blob());

	if (req.etag() == to_quoted_hex(*req.blob()))
	{
		http::response<http::empty_body> res{http::status::not_modified, req.version()};
		res.set(http::field::cache_control, "private, max-age=31536000, immutable");
		res.set(http::field::etag, to_quoted_hex(*req.blob()));
		return send(std::move(res));
	}

	// Check if the user owns the blob
	// Note: the arguments of find() uses req, so we can't move req into the lambda.
	// Otherwise, req will become dangled.
	Ownership{req.owner()}.find(
		*m_db, req.collection(), *req.blob(),
		[
			req, this,
			send=std::move(send)
		](CollEntry entry, auto ec) mutable
		{
			// Only allow the owner to know whether an object exists or not.
			// Always reply forbidden for everyone else.
			if (ec == Error::object_not_exist)
				return send(http::response<http::empty_body>{
					req.request_by_owner(m_auth) ? http::status::not_found : http::status::forbidden,
					req.version()
				});

			if (ec)
				return send(http::response<http::empty_body>{http::status::internal_server_error, req.version()});

			if (!entry.permission().allow(m_auth, req.owner()))
				return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

			auto [rendition] = find_fields(req.option(), "rendition");

			return send(m_blob_db.response(*req.blob(), req.version(), entry.mime(), req.etag(), rendition));
		}
	);
}

template <class Send>
void SessionHandler::on_query(const BlobRequest& req, Send&& send)
{
	switch (req.intent().query_target())
	{
		case URLIntent::QueryTarget::collection:
			return scan_collection(req.intent(), req.version(), std::forward<Send>(send));

		case URLIntent::QueryTarget::blob:
			return query_blob(req, std::forward<Send>(send));

		case URLIntent::QueryTarget::blob_set:
			return query_blob_set(req.intent(), req.version(), std::forward<Send>(send));

		default:
			return send(bad_request("unsupported query target", req.version()));
	}
}

template <class Send>
void SessionHandler::scan_collection(const URLIntent& intent, unsigned version, Send&& send)
{
	auto [user, json] = find_optional_fields(intent.option(), "user", "json");

	if (!user.has_value())
		return send(bad_request("invalid user in query", version));

	// TODO: allow other users to query another user's shared collections
	if (m_auth.user() != *user)
		return send(http::response<http::string_body>{http::status::forbidden, version});

	Ownership{*user}.scan_all_collections(
		*m_db,
		SendJSON{std::move(send), m_auth, version, std::nullopt, server_root(), json.has_value() ? nullptr : &m_lib}
	);
}

template <class Send>
void SessionHandler::query_blob(const BlobRequest& req, Send&& send)
{
	auto [blob_arg, rendition] = find_fields(req.option(), "id", "rendition");
	auto blob = hex_to_object_id(blob_arg);
	if (!blob)
		return send(bad_request("invalid blob ID", req.version()));

	Ownership{m_auth.user()}.query_blob(
		*m_db,
		*blob,
		[
			send=std::forward<Send>(send), req, blobid=*blob,
			rendition=std::string{rendition}, this
		](auto&& range, auto ec)
		{
			if (ec)
				return send(server_error("internal server error", req.version()));

			for (auto&& en : range)
				return send(m_blob_db.response(blobid, req.version(), en.entry.mime(), req.etag(), rendition));

			return send(not_found("blob not found", req.version()));
		}
	);
}

template <class Send>
void SessionHandler::query_blob_set(const URLIntent& intent, unsigned version, Send&& send)
{
	auto [pub, json] = find_optional_fields(intent.option(), "public", "json");

	Ownership{m_auth.user()}.list_public_blobs(
		*m_db,
		SendJSON{std::forward<Send>(send), m_auth, version, std::nullopt, server_root(), !json.has_value() ? &m_lib : nullptr}
	);
}

} // end of namespace
