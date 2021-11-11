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

#include "UploadFile.hh"
#include "WebResources.hh"
#include "URLIntent.hh"

#include "server/Authentication.hh"
#include "server/Authentication.ipp"
#include "net/MMapResponseBody.hh"

#include "util/Error.hh"
#include "util/StringFields.hh"
#include "util/Log.hh"
#include "util/Cookie.hh"
#include "util/Escape.hh"

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/version.hpp>

#include <iostream>

namespace hrb {

template <class Send>
class SessionHandler::SendJSON
{
public:
	SendJSON(
		Send&& send,
		unsigned version,
//		std::optional<ObjectID> blob,
		SessionHandler& parent,
		const WebResources *lib = nullptr
	) :
		m_send{std::move(send)},
		m_version{version},
//		m_blob{blob},
		m_parent{parent},
		m_lib{lib}
	{
	}

	/// Convert anything that is thrown into this function into JSON and send
	/// it out as a HTTP response.
	template <typename JSONSerializable>
	auto operator()(JSONSerializable&& serializable, std::error_code ec) const
	{
		try
		{
			nlohmann::json json(std::forward<JSONSerializable>(serializable));
			return send_json(std::move(json), ec);
		}
		catch (std::exception& e)
		{
			Log(LOG_WARNING, "exception throw in send_json(): %1%", e.what());
			return send_json(nlohmann::json::object(), Error::unknown_error);
		}
	}

	auto send_json(nlohmann::json&& json, std::error_code ec) const
	{
		// Ignore JSON if error occurs (i.e. internal server error) for security reasons.
		if (ec)
			return m_send(http::response<http::empty_body>{
				http::status::internal_server_error, m_version
			});

		assert(json.is_object());
		using namespace std::chrono;

		if (m_parent.m_server.user().is_valid())
			json.emplace("username", m_parent.m_server.user().username());
		if (m_parent.m_server.user().is_guest())
			json.emplace("auth", to_hex(m_parent.m_server.auth()));
//		if (m_blob)
//			json.emplace("blob", to_hex(*m_blob));
		if (m_parent.m_on_header != high_resolution_clock::time_point{})
			json.emplace(
				"elapse",
				duration_cast<
				    duration<double, microseconds::period>
				>(high_resolution_clock::now() - m_parent.m_on_header).count()
			);

		if (m_lib)
		{
			// TODO: catch exception here
			// refactor it so that we don't need to get these from JSON
			// better move these to SessionHandler
			using jptr = nlohmann::json::json_pointer;
			auto&& cover = json.value(jptr{"/blob"},       json.value(jptr{"/meta/cover"}, std::string{""}));
			auto&& owner = json.value(jptr{"/owner"},      std::string{""});
			auto&& coll  = json.value(jptr{"/collection"}, std::string{""});

			static const boost::format fmt{R"(<meta property="og:image" content="%1%%2%">
	<meta property="og:url" content="%1%%3%">
	<meta property="og:title" content="Hearty Rabbit: %4%">
	)"};
//			URLIntent cover_url{URLIntent::Action::api, owner, coll, cover};
//			URLIntent view_url{URLIntent::Action::view, owner, coll, "Hearty Rabbit"};
//			if (m_parent.m_server.auth().id().is_guest())
//			{
//				auto auth = "auth=" + to_hex(m_parent.m_server.auth().session());
//				cover_url.add_option(auth);
//				view_url.add_option(auth);
//			}
//			cover_url.add_option("rendition=thumbnail");
//
//			return m_send(m_lib->inject(
//				http::status::ok,
//				json.dump(),
//				(
//					boost::format{fmt} % m_parent.server_root() %
//					cover_url.str() %
//					view_url.str() %
//					coll
//				).str(),
//				m_version)
//			);
		}
		else
		{
			http::response<http::string_body> res{
				std::piecewise_construct,
				std::make_tuple(json.dump()),
				std::make_tuple(http::status::ok, m_version)
			};
			res.set(http::field::content_type, "application/json");
			return m_send(std::move(res));
		}
	}
private:
	mutable Send    m_send;
	unsigned        m_version;
//	std::optional<ObjectID> m_blob;
	SessionHandler& m_parent;
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
	assert(!m_server.user().is_valid());

	m_on_header = std::chrono::high_resolution_clock::now();
	URLIntent intent{header.target()};

	// Use StringRequestParser to parser login requests.
	// The username/password will be stored in the string body.
	// No need to verify session.
	if (intent.type() == URLIntent::Type::session && intent.session()->action == URLIntent::Session::Action::create)
		return complete(RequestBodyType::string, std::error_code{});

	// Everything else require a valid session.
	m_request_session_id = Authentication::parse_cookie(Cookie{header[http::field::cookie]});
	if (!m_request_session_id)
	{
//		auto [auth_str] = urlform.find(intent.option(), "auth");
//		auto auth_key = hex_to_array<Authentication::SessionID{}.size()>(auth_str);
//		if (header.method() == http::verb::get && auth_key)
		{
/*			Authentication auth{*auth_key};
			return auth.is_shared_resource(
				intent.user(),
				intent.collection(),
				*m_db,
				[complete = std::forward<Complete>(complete), auth, this, intent](bool shared, auto err)
				{
					assert(auth.id().is_guest());
					// TODO: support shared_resource()
//					if (!err && shared)
//						m_auth = auth;

					complete(RequestBodyType::empty, err);
				}
			);
*/
		}
//		else
		{
			return complete(RequestBodyType::empty, std::error_code{});
		}
	}

	m_server.verify_session(
		*m_request_session_id,
		[
			this,
			type=intent.type(),
			method=header.method(),
			complete=std::forward<Complete>(complete)
		](std::error_code ec) mutable
		{
			auto body_type = RequestBodyType::empty;

			// Use a UploadRequestParse to parser upload requests, only when the session is authenticated.
			if (!ec && type == URLIntent::Type::user && method == http::verb::put)
				body_type = RequestBodyType::upload;

			// blobs support post request
			else if (!ec && type == URLIntent::Type::user && method == http::verb::post)
				body_type = RequestBodyType::string;

			// If the cookie returned by verify_session() is different from the one we passed to it,
			// that mean it is going to expire and it's renewed.
			// In this case we want to tell Session to put it in the "Set-Cookie" header.
			complete(body_type, ec);
		}
	);
}

template <class Request, class Send>
void SessionHandler::on_request_body(Request&& req, Send&& send)
{
//	assert((m_auth.session() == Authentication::SessionID{}) == m_auth.id().is_anonymous());

	URLIntent intent{req.target()};
std::cout << "intent " << (int)intent.type() << std::endl;
	if (!intent.verb_supported(req.method()))
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});

	switch (intent.type())
	{
	case URLIntent::Type::session:
		assert(intent.session());
		if constexpr (std::is_same<std::remove_reference_t<Request>, StringRequest>::value)
		{
			if (intent.session()->action == URLIntent::Session::Action::create)
				return on_login(req, std::forward<Send>(send));
		}
		if constexpr (std::is_same_v<std::decay_t<Request>, EmptyRequest>)
		{
			if (intent.session()->action == URLIntent::Session::Action::destroy)
				return on_logout(std::forward<Request>(req), std::forward<Send>(send));
		}
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});

	case URLIntent::Type::user:
	case URLIntent::Type::lib:
	case URLIntent::Type::query:
	case URLIntent::Type::none:
		return send(file_request(intent, req[http::field::if_none_match], req.version()));
	}

	// on_request_view() is a function template on the request type. It can work with all
	// request types so no need to check before calling.
//	if (intent.action() == URLIntent::Action::view)
//		return on_request_view(std::forward<Request>(req), std::move(intent), std::forward<Send>(send));
//
//	if (intent.action() == URLIntent::Action::api)
//		return on_request_api(std::forward<Request>(req), std::move(intent), std::forward<Send>(send));

	// The following URL only support EmptyRequests, i.e. requests without body.
	if constexpr (std::is_same_v<std::decay_t<Request>, EmptyRequest>)
	{
		if (req.method() != http::verb::get)
			return send(bad_request("unsupported HTTP method", req.version()));

//		if (intent.action() == URLIntent::Action::lib)
//			return send(file_request(intent, req[http::field::if_none_match], req.version()));
/*
		if (intent.action() == URLIntent::Action::home)
			return m_auth.is_valid() ?
				Ownership{m_auth.id()}.scan_all_collections(
					*m_db,
					SendJSON{std::forward<Send>(send), req.version(), std::nullopt, *this, &m_lib}
				) :
				list_public_blobs(false, "", req.version(), std::forward<Send>(send));
*/
//		if (intent.action() == URLIntent::Action::query)
//			return on_query({std::forward<Request>(req), std::move(intent)}, std::forward<Send>(send));

//		if (intent.action() == URLIntent::Action::logout)
		if (auto session = intent.session(); session && session->action == URLIntent::Session::Action::destroy)
			return on_logout(std::forward<Request>(req), std::forward<Send>(send));
	}

	// Upload requests for /upload URL only
	if constexpr (std::is_same_v<std::decay_t<Request>, UploadRequest>)
	{
//		if (intent.action() == URLIntent::Action::upload)
//			return on_upload(std::forward<Request>(req), std::forward<Send>(send));
	}

	return send(not_found(req.target(), req.version()));
}

template <class Request, class Send>
void SessionHandler::on_request_api(Request&& req, URLIntent&& intent, Send&& send)
{
	BlobRequest breq{req, std::move(intent)};

	if (req.method() == http::verb::delete_)
	{
		return unlink(std::move(breq), std::forward<Send>(send));
	}
	else if (req.method() == http::verb::get)
	{
//		if (breq.blob())
			return get_blob(std::move(breq), std::forward<Send>(send));
/*		else
			return Ownership{breq.owner(), m_auth.id()}.get_collection(
				*m_db,
				breq.collection(),
				[
					send = SendJSON{std::forward<Send>(send), req.version(), std::nullopt, *this}, this
				](auto&& coll, std::error_code ec) mutable
				{
					validate_collection(coll);
					send(nlohmann::json(coll), ec);
				}
			);*/
	}
	else if (req.method() == http::verb::post)
	{
//		if (breq.blob())
//			return post_blob(std::move(breq), std::forward<Send>(send));
//		else
//			return post_view(std::move(breq), std::forward<Send>(send));
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
//		// view request always sends HTML: pass &m_lib to SendJSON
//		return Ownership{breq.owner(), m_auth.id()}.get_collection(
//			*m_db,
//			breq.collection(),
//			SendJSON{std::forward<Send>(send), breq.version(), breq.blob(), *this, &m_lib}
//		);
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
//	assert(req.blob());
//
//	// Return 304 if the etag is the same as the blob ID.
//	// No need to check permission because we don't need to provide anything.
//	if (req.etag() == to_quoted_hex(*req.blob()))
//	{
//		http::response<http::empty_body> res{http::status::not_modified, req.version()};
//		res.set(http::field::cache_control, "private, max-age=31536000, immutable");
//		res.set(http::field::etag, to_quoted_hex(*req.blob()));
//		return send(std::move(res));
//	}

	// Check if the user can access the blob
	// Note: do not move-construct "req" to the lambda because the arguments of find() uses it.
	// Otherwise, "req" will become dangled.
/*	Ownership{req.owner(), m_auth.id()}.get_blob(
		*m_db, req.collection(), *req.blob(),
		[
			req, this,
			send = std::forward<Send>(send)
		](const BlobDBEntry& entry, auto filename, auto ec) mutable
		{
			// Only allow the owner to know whether an object exists or not.
			// Always reply forbidden for everyone else.
			if (ec == Error::object_not_exist)
				return send(
					http::response<http::empty_body>{
						req.request_by_owner(m_auth.id()) ? http::status::not_found : http::status::forbidden,
						req.version()
					}
				);

			if (ec)
				return send(http::response<http::empty_body>{http::status::internal_server_error, req.version()});

			if (!entry.permission().allow(m_auth.id(), req.owner()))
				return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

			if (auto[json] = urlform.find_optional(req.option(), "json"); json)
			{
				return send(m_blob_db.meta(*req.blob(), req.version()));
			}
			else
			{
				auto[rendition] = urlform.find(req.option(), "rendition");
				auto response = m_blob_db.response(*req.blob(), req.version(), req.etag(), rendition);

				if (auto fields = entry.fields(); fields.has_value())
					set_header(*fields, response);
				response.set(http::field::location, req.intent().str());
				return send(std::move(response));
			}
		}
	);
*/
}

} // end of namespace
