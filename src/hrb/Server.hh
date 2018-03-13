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

#include "net/Request.hh"
#include "net/Redis.hh"

#include <boost/asio/io_context.hpp>

#include <system_error>

namespace hrb {

// URL prefixes
namespace url {
const boost::string_view view{"/view"};
const boost::string_view login{"/login"};
const boost::string_view logout{"/logout"};
const boost::string_view blob{"/blob"};
const boost::string_view collection{"/coll"};
const boost::string_view upload{"/upload"};
}

class Authentication;
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
	void handle_https(UploadRequest&& req, Send&& send, const Authentication& auth);

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Send>
	void handle_https(StringRequest&& req, Send&& send, const Authentication& auth);

	template <class Send>
	void handle_https(EmptyRequest&& req, Send&& send, const Authentication& auth);

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

	template <class Request, class Send>
	void on_valid_session(Request&& req, Send&& send, const Authentication& auth);

	static bool is_upload(const RequestHeader& header);
	static bool is_login(const RequestHeader& header);
	static void drop_privileges();

	using EmptyResponseSender  = std::function<void(http::response<http::empty_body>&&)>;
	using StringResponseSender = std::function<void(http::response<http::string_body>&&)>;
	using FileResponseSender   = std::function<void(http::response<SplitBuffers>&&)>;
	using BlobResponseSender   = std::function<void(http::response<MMapResponseBody>&&)>;

	void on_login(const StringRequest& req, EmptyResponseSender&& send);
	void on_logout(const EmptyRequest& req, EmptyResponseSender&& send, const Authentication& auth);
	void on_invalid_session(const RequestHeader& req, FileResponseSender&& send);
	void on_upload(UploadRequest&& req, EmptyResponseSender&& send, const Authentication& auth);
	void unlink(std::string_view requester, std::string_view owner, std::string_view coll, const ObjectID& blobid, unsigned version, EmptyResponseSender&& send);
	void update_blob(std::string_view requester, std::string_view owner, std::string_view coll, const ObjectID& blobid, unsigned version, EmptyResponseSender&& send);
	bool is_static_resource(boost::string_view target) const;
	void serve_view(const EmptyRequest& req, FileResponseSender&& send, const Authentication& auth);
	void serve_collection(const EmptyRequest& req, StringResponseSender&& send, const Authentication& auth);

	template <typename Request, typename Send>
	void handle_blob(Request&& req, Send&& send, const Authentication& auth);

	template <typename Send>
	void get_blob(
		std::string_view requester,
		std::string_view owner,
		std::string_view coll,
		const ObjectID& blobid,
		unsigned version,
		boost::string_view etag,
		Send&& send
	);

private:
	const Configuration&    m_cfg;
	boost::asio::io_context m_ioc;

	redis::Pool     m_db;
	WebResources    m_lib;
	BlobDatabase    m_blob_db;
};

} // end of namespace
