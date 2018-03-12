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
#include "Ownership.hh"
#include "PathURL.hh"

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

#include <iostream>
#include <system_error>

namespace hrb {

// URL prefixes
namespace url {
const boost::string_view login{"/login"};
const boost::string_view logout{"/logout"};
const boost::string_view blob{"/blob"};
const boost::string_view view{"/view"};
const boost::string_view collection{"/coll"};
const boost::string_view upload{"/upload"};
}

class Configuration;
class Password;
class BlobFile;

/// The main application logic of hearty rabbit.
/// This is the class that handles HTTP requests from and produce response to clients. The unit test
/// this class calls handle_https().
class Server
{
public:
	explicit Server(const Configuration& cfg);

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
	unsigned short https_port() const;
	std::size_t upload_limit() const;

private:
	http::response<SplitBuffers> static_file_request(const EmptyRequest& req);
	http::response<SplitBuffers> on_login_incorrect(const EmptyRequest& req);

	template <class Send>
	void on_valid_session(EmptyRequest&& req, Send&& send, const Authentication& auth);

	static bool is_upload(const RequestHeader& header);
	static bool is_login(const RequestHeader& header);
	static void drop_privileges();
	static std::string user_view(std::string_view user, std::string_view path = {});

	using EmptyResponseSender  = std::function<void(http::response<http::empty_body>&&)>;
	using StringResponseSender = std::function<void(http::response<http::string_body>&&)>;
	using FileResponseSender   = std::function<void(http::response<SplitBuffers>&&)>;
	using BlobResponseSender   = std::function<void(http::response<MMapResponseBody>&&)>;

	void on_login(const StringRequest& req, EmptyResponseSender&& send);
	void on_logout(const EmptyRequest& req, EmptyResponseSender&& send, const Authentication& auth);
	void on_invalid_session(const RequestHeader& req, FileResponseSender&& send);
	void on_upload(UploadRequest&& req, EmptyResponseSender&& send, const Authentication& auth);
	void unlink(std::string_view user, std::string_view coll, const ObjectID& blobid, unsigned version, EmptyResponseSender&& send);
	bool is_static_resource(boost::string_view target) const;
	void serve_view(const EmptyRequest& req, FileResponseSender&& send, const Authentication& auth);
	void serve_collection(const EmptyRequest& req, StringResponseSender&& send, const Authentication& auth);

	template <typename Send>
	void handle_blob(const EmptyRequest& req, Send&& send, const Authentication& auth);

	template <typename Send>
	void get_blob(std::string_view user, std::string_view coll, const ObjectID& blobid, unsigned version, boost::string_view etag, Send&& send);

private:
	const Configuration&    m_cfg;
	boost::asio::io_context m_ioc;

	redis::Pool     m_db;
	WebResources    m_lib;
	BlobDatabase    m_blob_db;
};

} // end of namespace
