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

#include "Server.hh"

#include "Ownership.ipp"
#include "UploadFile.hh"
#include "URLIntent.hh"
#include "Ownership.hh"

#include "crypto/Authentication.hh"
#include "net/MMapResponseBody.hh"

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/version.hpp>

namespace hrb {

template <class Request, class Send>
void Server::handle_https(Request&& req, Send&& send, const Authentication& auth)
{
	if constexpr (std::is_same<std::remove_reference_t<Request>, EmptyRequest>::value)
	{
		if (is_static_resource(req.target()))
			return send(static_file_request(req));

		if (req.target() == "/login_incorrect.html")
			return send(on_login_incorrect(req));
	}

	if constexpr (std::is_same<std::remove_reference_t<Request>, StringRequest>::value)
	{
		if (is_login(req))
			return on_login(req, std::forward<Send>(send));
	}

	// Everything else require a valid session.
	return auth.valid() ?
		on_valid_session(std::move(req), std::forward<decltype(send)>(send), auth) :
		on_invalid_session(std::move(req), std::forward<decltype(send)>(send));
}

template <class Request, class Send>
void Server::handle_blob(Request&& req, Send&& send, const Authentication& auth)
{
	URLIntent path_url{req.target()};

	// Return 400 bad request if the blob ID is invalid
	auto object_id = hex_to_object_id(path_url.filename());
	if (!object_id)
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});

	if (req.method() == http::verb::delete_)
		return unlink(auth.user(), path_url.user(), path_url.collection(), *object_id, req.version(), std::move(send));

	else if (req.method() == http::verb::get)
		return get_blob(
			auth.user(), path_url.user(), path_url.collection(), *object_id, req.version(),
			req[http::field::if_none_match], std::move(send)
		);

	else if (req.method() == http::verb::post)
		return update_blob(
			auth.user(), path_url.user(), path_url.collection(), *object_id, req.version(),
			std::move(send)
		);

	else
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});
}

template <class Send>
void Server::get_blob(
	std::string_view requester,
	std::string_view owner,
	std::string_view coll,
	const ObjectID& object_id,
	unsigned version,
	boost::string_view etag,
	Send&& send
)
{
	// Check if the user owns the blob
	Ownership{owner}.allow(
		*m_db.alloc(), requester, coll, object_id,
		[
			object_id, version, etag=etag.to_string(), this,
			send=std::move(send)
		](bool can_access, auto ec) mutable
		{
			if (ec == Error::object_not_exist)
				return send(http::response<http::empty_body>{http::status::not_found, version});

			if (ec)
				return send(http::response<http::empty_body>{http::status::internal_server_error, version});

			if (!can_access)
				return send(http::response<http::empty_body>{http::status::forbidden, version});

			return send(m_blob_db.response(object_id, version, etag));
		}
	);
}

template <class Request, class Send>
void Server::on_valid_session(Request&& req, Send&& send, const Authentication& auth)
{
	// handle_blob() is a function template on the request type. It can work with all
	// request types so no need to check before calling.
	if (req.target().starts_with(url::blob))
		return handle_blob(std::forward<Request>(req), std::forward<Send>(send), auth);

	// The following URL only support EmptyRequests, i.e. requests without body.
	if constexpr (std::is_same<std::remove_reference_t<Request>, EmptyRequest>::value)
	{
		if (req.target() == "/")
			return send(see_other(URLIntent{"view", auth.user(), "", ""}.str(), req.version()));

		if (req.target().starts_with(url::view))
			return serve_view(std::forward<Request>(req), std::forward<Send>(send), auth);

		if (req.target().starts_with(url::collection))
			return serve_collection(std::forward<Request>(req), std::forward<Send>(send), auth);

		if (req.target() == url::logout)
			return on_logout(std::forward<Request>(req), std::forward<Send>(send), auth);
	}

	// Upload requests for /upload URL only
	if constexpr (std::is_same<std::remove_reference_t<Request>, UploadRequest>::value)
	{
		if (req.target().starts_with(url::upload))
			return on_upload(std::move(req), std::forward<Send>(send), auth);
	}

	return send(not_found(req.target(), req.version()));
}

/// \arg    header      The header we just received. This reference must be valid
///						until \a complete() is called.
/// \arg    src         The request_parser that produce \a header. It will be moved
///                     to \a dest. Must be valid until \a complete() is called.
template <class Complete>
void Server::on_request_header(
	const RequestHeader& header,
	EmptyRequestParser& src,
	RequestBodyParsers& dest,
	Complete&& complete
)
{
	// Use StringRequestParser to parser login requests.
	// The username/password will be stored in the string body.
	// No need to verify session.
	if (is_login(header))
	{
		dest.emplace<StringRequestParser>(std::move(src));
		return complete(Authentication{});
	}

	if (is_static_resource(header.target()))
	{
		dest.emplace<EmptyRequestParser>(std::move(src));
		return complete(Authentication{});
	}

	// Everything else require a valid session.
	auto cookie = header[http::field::cookie];
	auto session = parse_cookie({cookie.data(), cookie.size()});
	if (!session)
	{
		dest.emplace<EmptyRequestParser>(std::move(src));
		return complete(Authentication{});
	}

	Authentication::verify_session(
		*session,
		*m_db.alloc(),
		[
			this,
			&header,
			&dest,
			&src,
			complete=std::forward<Complete>(complete)
		](std::error_code ec, const Authentication& auth) mutable
		{
			// Use a UploadRequestParse to parser upload requests, only when the session is authenicated.
			if (!ec && is_upload(header))
				prepare_upload(dest.emplace<UploadRequestParser>(std::move(src)).get().body(), ec);

			// blobs support post request
			else if (!ec && header.target().starts_with(url::blob) && header.method() == http::verb::post)
				dest.emplace<StringRequestParser>(std::move(src));

			// Other requests use EmptyRequestParser, because they don't have a body.
			else
				dest.emplace<EmptyRequestParser>(std::move(src));

			complete(auth);
		}
	);
}

} // end of namespace
