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
#include "BlobRequest.hh"

#include "crypto/Authentication.hh"
#include "net/MMapResponseBody.hh"

#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/version.hpp>

namespace hrb {

/// \arg    header      The header we just received. This reference must be valid
///						until \a complete() is called.
/// \arg    src         The request_parser that produce \a header. It will be moved
///                     to \a dest. Must be valid until \a complete() is called.
template <class Complete>
void Server::on_request_header(
	const RequestHeader& header,
	const Authentication& existing_auth,
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
		return complete(Authentication{}, false);
	}

	if (is_static_resource(header.target()))
	{
		dest.emplace<EmptyRequestParser>(std::move(src));
		return complete(Authentication{}, false);
	}

	// Everything else require a valid session.
	auto cookie = header[http::field::cookie];
	auto session = parse_cookie({cookie.data(), cookie.size()});
	if (!session)
	{
		dest.emplace<EmptyRequestParser>(std::move(src));
		return complete(Authentication{}, false);
	}

	auto on_verify_session = [
		this,
		&header,
		&dest,
		&src,
		old_session=*session,
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

		// If the cookie returned by verify_session() is different from the one we passed to it,
		// that mean it is going to expired and it's renewed.
		// In this case we want to tell Session to put it in the "Set-Cookie" header.
		complete(auth, old_session != auth.cookie());
	};

	// no need to verify the session again if it is the same as previous one
	if (existing_auth.cookie() == *session)
		on_verify_session(std::error_code{}, existing_auth);
	else
		Authentication::verify_session(
			*session,
			*m_db.alloc(),
			session_length(),
			std::move(on_verify_session)
		);
}

template <class Request, class Send>
void Server::handle_request(Request&& req, Send&& send, const Authentication& auth)
{
	if constexpr (std::is_same<std::remove_reference_t<Request>, EmptyRequest>::value)
	{
		if (is_static_resource(req.target()))
			return send(static_file_request(req));

		if (req.target() == url::login_incorrect)
			return send(on_login_incorrect(req));
	}

	if (req.target() == hrb::url::login)
	{
		if constexpr (std::is_same<std::remove_reference_t<Request>, StringRequest>::value)
		{
			if (req.method() == http::verb::post)
				return on_login(req, std::forward<Send>(send));
		}

		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});
	}

	// handle_blob() is a function template on the request type. It can work with all
	// request types so no need to check before calling.
	if (req.target().starts_with(url::blob))
		return handle_blob(std::forward<Request>(req), std::forward<Send>(send), auth);

	// The following URL only support EmptyRequests, i.e. requests without body.
	if constexpr (std::is_same<std::remove_reference_t<Request>, EmptyRequest>::value)
	{
		if (req.target() == "/")
			return serve_home(std::forward<Request>(req), std::forward<Send>(send), auth);

		if (req.target().starts_with(url::view))
			return serve_view(std::forward<Request>(req), std::forward<Send>(send), auth);

		if (req.target().starts_with(url::collection))
			return serve_collection(std::forward<Request>(req), std::forward<Send>(send), auth);

		if (req.target().starts_with(url::list_coll))
			return scan_collection(std::forward<Request>(req), std::forward<Send>(send), auth);

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

template <class Request, class Send>
void Server::handle_blob(Request&& req, Send&& send, const Authentication& auth)
{
	BlobRequest breq{req, auth.user()};

	// Return 400 bad request if the blob ID is invalid
	auto object_id = breq.blob();
	if (!object_id)
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});

	if (req.method() == http::verb::delete_)
		return unlink(std::move(breq), std::move(send));

	else if (req.method() == http::verb::get)
		return get_blob(std::move(breq), std::move(send));

	else if (req.method() == http::verb::post)
		return update_blob(std::move(breq),std::move(send));

	else
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});
}

template <class Send>
void Server::get_blob(BlobRequest&& req, Send&& send)
{
	assert(req.blob());

	// Check if the user owns the blob
	Ownership{req.owner()}.find(
		*m_db.alloc(), req.collection(), *req.blob(),
		[
			req=std::move(req),
			send=std::move(send),
			this
		](CollEntry entry, auto ec) mutable
		{
			// Only allow the owner to know whether an object exists or not.
			// Always reply forbidden for everyone else.
			if (ec == Error::object_not_exist)
				return send(http::response<http::empty_body>{
					req.request_by_owner() ? http::status::not_found : http::status::forbidden,
					req.version()
				});

			if (ec)
				return send(http::response<http::empty_body>{http::status::internal_server_error, req.version()});

			if (!req.request_by_owner() && !entry.permission().allow(req.requester()))
				return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

			return send(m_blob_db.response(*req.blob(), req.version(), entry.mime(), req.etag()));
		}
	);
}

} // end of namespace
