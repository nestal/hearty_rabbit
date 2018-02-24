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

#include "crypto/Authentication.hh"
#include "net/SplitBuffers.hh"
#include "net/Request.hh"
#include "net/Redis.hh"

#include <boost/filesystem/path.hpp>
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
		if (allow_anonymous(req.target()))
			return send(static_file_request(req));

		// Everything else require a valid session.
		return auth.valid() ?
			on_valid_session(std::move(req), std::forward<decltype(send)>(send), auth) :
			on_invalid_session(std::move(req), std::forward<decltype(send)>(send));
	}

	static http::response<http::string_body> bad_request(boost::string_view why, unsigned version);
	static http::response<http::string_body> not_found(boost::string_view target, unsigned version);
	static http::response<http::string_body> server_error(boost::string_view what, unsigned version);
	static http::response<http::empty_body> redirect(boost::beast::string_view where, unsigned version);

	void disconnect_db();
	void run();
	boost::asio::io_context& get_io_context();

	// Administrative commands
	void add_user(std::string_view username, Password&& password, std::function<void(std::error_code)> complete);
	std::string https_root() const;

private:
	http::response<SplitBuffers> static_file_request(const EmptyRequest& req);
	http::response<SplitBuffers> serve_home(unsigned version);

	template <class Send>
	void on_valid_session(EmptyRequest&& req, Send&& send, const Authentication& auth)
	{
		const RequestHeader& header = req;

		if (req.target() == "/")
			return send(serve_home(req.version()));

		if (req.target().starts_with(url::blob))
			return get_blob(req, std::forward<decltype(send)>(send), auth);

		if (req.target().starts_with(url::dir))
			return send(get_dir(req));

		if (req.target().starts_with(url::logout))
			return on_logout(req, auth, std::forward<Send>(send));

		return send(not_found(req.target(), req.version()));
	}

	static bool is_upload(const RequestHeader& header);
	static bool is_login(const RequestHeader& header);
	static void drop_privileges();

	using EmptyResponseSender  = std::function<void(http::response<http::empty_body>&&)>;
	using StringResponseSender = std::function<void(http::response<http::string_body>&&)>;
	using FileResponseSender   = std::function<void(http::response<SplitBuffers>&&)>;
	using BlobResponseSender   = std::function<void(http::response<http::file_body>&&)>;

	void on_login(const StringRequest& req, EmptyResponseSender&& send);
	void on_logout(const EmptyRequest& req, const Authentication& auth, EmptyResponseSender&& send);
	void on_invalid_session(const RequestHeader& req, FileResponseSender&& send);
	void on_upload(UploadRequest&& req, EmptyResponseSender&& send, const Authentication& auth);
	void get_blob(const EmptyRequest& req, BlobResponseSender&& send, const Authentication& auth);
	http::response<http::string_body> get_dir(const EmptyRequest& req);
	static bool allow_anonymous(boost::string_view target);

private:
	const Configuration&    m_cfg;
	boost::asio::io_context m_ioc;

	redis::Pool     m_db;
	WebResources    m_lib;
	BlobDatabase    m_blob_db;
};

} // end of namespace
