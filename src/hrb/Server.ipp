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

namespace hrb {

template <typename Send>
void Server::handle_blob(const EmptyRequest& req, Send&& send, const Authentication& auth)
{
	URLIntent path_url{req.target()};

	// Return 404 not_found if the blob ID is invalid
	auto object_id = hex_to_object_id(path_url.filename());
	if (object_id == ObjectID{})
		return send(http::response<http::empty_body>{http::status::not_found, req.version()});

	if (auth.user() != path_url.user())
		return send(http::response<http::empty_body>{http::status::forbidden, req.version()});

	if (req.method() == http::verb::delete_)
		return unlink(path_url.user(), path_url.collection(), object_id, req.version(), std::move(send));

	else if (req.method() == http::verb::get)
		return get_blob(
			path_url.user(), path_url.collection(), object_id, req.version(),
			req[http::field::if_none_match], std::move(send)
		);

	else
		return send(http::response<http::empty_body>{http::status::bad_request, req.version()});
}

template <typename Send>
void Server::get_blob(std::string_view user, std::string_view coll, const ObjectID& object_id, unsigned version, boost::string_view etag, Send&& send)
{
	// Check if the user owns the blob
	Ownership{user}.is_owned(
		*m_db.alloc(), coll, object_id,
		[
			object_id, version, etag=etag.to_string(), this,
			user=std::string{user},
			send=std::move(send)
		](bool is_member, auto ec) mutable
		{
			if (ec)
				return send(http::response<http::empty_body>{http::status::internal_server_error, version});

			if (!is_member)
				return send(http::response<http::empty_body>{http::status::forbidden, version});

			return send(m_blob_db.response(object_id, version, etag));
		}
	);
}

template <class Send>
void Server::on_valid_session(EmptyRequest&& req, Send&& send, const Authentication& auth)
{
	const RequestHeader& header = req;

	if (req.target() == "/")
		return send(see_other(URLIntent{"view", auth.user(), "", ""}.str(), req.version()));

	if (req.target().starts_with(url::blob))
		return handle_blob(req, std::forward<decltype(send)>(send), auth);

	if (req.target().starts_with(url::view))
		return serve_view(req, std::forward<decltype(send)>(send), auth);

	if (req.target().starts_with(url::collection))
		return serve_collection(req, std::forward<decltype(send)>(send), auth);

	if (req.target() == url::logout)
		return on_logout(req, std::forward<Send>(send), auth);

	return send(not_found(req.target(), req.version()));
}

} // end of namespace
