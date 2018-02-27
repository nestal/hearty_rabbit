/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#pragma once

#include "BlobDatabase.hh"
#include "WebResources.hh"
#include "UploadFile.hh"
#include "Container.hh"

#include "crypto/Authentication.hh"
#include "net/SplitBuffers.hh"
#include "net/Request.hh"
#include "net/Redis.hh"
#include "net/MMapResponseBody.hh"
#include "util/Escape.hh"

#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/io_context.hpp>

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/member.hpp>

#include <system_error>

namespace hrb {

// URL prefixes
namespace url {
const boost::string_view login{"/login"};
const boost::string_view logout{"/logout"};
const boost::string_view blob{"/blob"};
const boost::string_view dir{"/dir"};
const boost::string_view upload{"/upload"};
}

class Configuration;
class Password;
class BlobObject;

/// The main application logic of hearty rabbit.
/// This is the class that handles HTTP requests from and produce response to clients. The unit test
/// this class calls handle_https().
class Server
{
public:
	explicit Server(const Configuration& cfg);

	static std::tuple<
		std::string_view,
		std::string_view
	> extract_prefix(const RequestHeader& req);

	void on_request_header(
		const RequestHeader& header,
		EmptyRequestParser& src,
		RequestBodyParsers& dest,
		std::function<void(const Authentication&)>&& complete
	);

	template<class Send>
	void handle_https(UploadRequest&& req, Send&& send, const Authentication& auth)
	{
		assert(is_upload(req));

		if (auth.valid())
			return req.target().starts_with(url::upload) ?
				on_upload(std::move(req), std::forward<Send>(send), auth) :
				send(not_found(req.target(), req.version()));

		else
			return on_invalid_session(std::move(req), std::forward<Send>(send));
	}

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Send>
	void handle_https(StringRequest&& req, Send&& send, const Authentication& auth)
	{
		assert(is_login(req));
		return on_login(req, std::forward<Send>(send));
	}

	template <class Send>
	void handle_https(EmptyRequest&& req, Send&& send, const Authentication& auth)
	{
		if (is_static_resource(req.target()))
			return send(static_file_request(req));

		if (req.target() == "/login_incorrect.html")
			return send(on_login_incorrect(req));

		// Everything else require a valid session.
		return auth.valid() ?
			on_valid_session(std::move(req), std::forward<decltype(send)>(send), auth) :
			on_invalid_session(std::move(req), std::forward<decltype(send)>(send));
	}

	static http::response<http::string_body> bad_request(boost::string_view why, unsigned version);
	static http::response<http::string_body> not_found(boost::string_view target, unsigned version);
	static http::response<http::string_body> server_error(boost::string_view what, unsigned version);
	static http::response<http::empty_body> see_other(boost::beast::string_view where, unsigned version);

	void run();
	boost::asio::io_context& get_io_context();

	// Administrative commands and configurations
	void add_user(std::string_view username, Password&& password, std::function<void(std::error_code)> complete);
	std::string https_root() const;
	std::size_t upload_limit() const;

private:
	http::response<SplitBuffers> static_file_request(const EmptyRequest& req);
	http::response<SplitBuffers> on_login_incorrect(const EmptyRequest& req);

	template <class Send>
	void on_valid_session(EmptyRequest&& req, Send&& send, const Authentication& auth)
	{
		const RequestHeader& header = req;

		if (req.target() == "/")
			return serve_home(std::forward<decltype(send)>(send), req.version(), auth);

		if (req.target().starts_with(url::blob))
			return get_blob(req, std::forward<decltype(send)>(send), auth);

		if (req.target().starts_with(url::dir))
			return send(get_dir(req));

		if (req.target() == url::logout)
			return on_logout(req, auth, std::forward<Send>(send));

		return send(not_found(req.target(), req.version()));
	}

	static bool is_upload(const RequestHeader& header);
	static bool is_login(const RequestHeader& header);
	static void drop_privileges();

	using EmptyResponseSender  = std::function<void(http::response<http::empty_body>&&)>;
	using StringResponseSender = std::function<void(http::response<http::string_body>&&)>;
	using FileResponseSender   = std::function<void(http::response<SplitBuffers>&&)>;
	using BlobResponseSender   = std::function<void(http::response<MMapResponseBody>&&)>;

	void on_login(const StringRequest& req, EmptyResponseSender&& send);
	void on_logout(const EmptyRequest& req, const Authentication& auth, EmptyResponseSender&& send);
	void on_invalid_session(const RequestHeader& req, FileResponseSender&& send);
	void on_upload(UploadRequest&& req, EmptyResponseSender&& send, const Authentication& auth);
	http::response<http::string_body> get_dir(const EmptyRequest& req);
	static bool is_static_resource(boost::string_view target);
	void serve_home(FileResponseSender&& send, unsigned version, const Authentication& auth);

	template <typename Send>
	void get_blob(const EmptyRequest& req, Send&& send, const Authentication& auth);

private:
	const Configuration&    m_cfg;
	boost::asio::io_context m_ioc;

	redis::Pool     m_db;
	WebResources    m_lib;
	BlobDatabase    m_blob_db;
};

template <typename Send>
void Server::get_blob(const EmptyRequest& req, Send&& send, const Authentication& auth)
{
	// blob ID is fixed size (40 characters, i.e., Blake hash size * 2)
	auto [empty, blob, blob_id, rendition] = tokenize<4>(req.target(), "/");
	assert(empty.empty());
	assert(blob == url::blob.substr(1).to_string());

	// Return 404 not_found if the blob ID is invalid
	auto object_id = hex_to_object_id(std::string_view{blob_id.data(), blob_id.size()});
	if (object_id == ObjectID{})
		return send(http::response<http::empty_body>{http::status::not_found, req.version()});

	// Check if the user has permission to read the blob
	Container::is_member(*m_db.alloc(), auth.user(), object_id,
		[
			object_id,
			rendition=std::string{rendition},
			send=std::move(send),
			version=req.version(),
			etag=req[http::field::if_none_match].to_string(),
			this
		](auto ec, bool is_member) mutable
		{
			if (ec)
				return send(http::response<http::empty_body>{http::status::internal_server_error, version});

			if (!is_member)
				return send(http::response<http::empty_body>{http::status::forbidden, version});

			if (rendition == "as_svg")
			{
				static const boost::format svg{R"___(<?xml version="1.0" standalone="no"?>
					<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1//EN"
					  "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd">
					<svg version="1.1"
					     xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
					    <image xlink:href="/blob/%1%" x="0" y="0" height="100%%" width="100%%"/>
					</svg>)___"
				};

				http::response<http::string_body> res{
					std::piecewise_construct,
					std::make_tuple((boost::format{svg} % to_hex(object_id)).str()),
					std::make_tuple(http::status::ok, version)
				};
				res.set(http::field::content_type, "image/svg+xml");
				return send(std::move(res));
			}

			return send(m_blob_db.response(object_id, version, etag));
		}
	);
}

} // end of namespace
