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

#include <json.hpp>

#include <system_error>
#include <utility>

namespace hrb {

class Authentication;
class Configuration;
class Password;
class BlobFile;
class BlobRequest;
class URLIntent;

/// The main application logic of hearty rabbit.
/// This is the class that handles HTTP requests from and produce response to clients. The unit test
/// this class calls handle_https().
class Server
{
public:
	explicit Server(const Configuration& cfg);

	template <class Complete>
	void on_request_header(
		const RequestHeader& header,
		EmptyRequestParser& src,
		RequestBodyParsers& dest,
		Complete&& complete
	);

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Request, class Send>
	void handle_request(Request&& req, Send&& send, const Authentication& auth);

	static http::response<http::string_body> bad_request(boost::string_view why, unsigned version);
	http::response<SplitBuffers> not_found(boost::string_view target, const std::optional<Authentication>& auth, unsigned version);
	static http::response<http::string_body> server_error(boost::string_view what, unsigned version);
	static http::response<http::empty_body> see_other(boost::beast::string_view where, unsigned version);

	boost::asio::io_context& get_io_context();

	// Administrative commands and configurations
	void add_user(std::string_view username, Password&& password, std::function<void(std::error_code)> complete);
	std::size_t upload_limit() const;
	std::chrono::seconds session_length() const;
	void drop_privileges() const;

private:
	http::response<SplitBuffers> file_request(const URLIntent& intent, boost::string_view etag, unsigned version);

	template <class Send>
	class SendJSON
	{
	public:
		SendJSON(Send&& send, unsigned version) : m_send{std::move(send)}, m_version{version} {}
		void operator()(const nlohmann::json& json, std::error_code ec) const
		{
			http::response<http::string_body> res{
				std::piecewise_construct,
				std::make_tuple(json.dump()),
				std::make_tuple(ec ? http::status::internal_server_error : http::status::ok, m_version)
			};
			res.set(http::field::content_type, "application/json");
			return m_send(std::move(res));
		}
	private:
		mutable Send m_send;
		unsigned m_version;
	};

	using EmptyResponseSender  = std::function<void(http::response<http::empty_body>&&)>;
	using StringResponseSender = std::function<void(http::response<http::string_body>&&)>;
	using FileResponseSender   = std::function<void(http::response<SplitBuffers>&&)>;
	using BlobResponseSender   = std::function<void(http::response<MMapResponseBody>&&)>;

	void on_login(const StringRequest& req, EmptyResponseSender&& send);
	void on_logout(const EmptyRequest& req, EmptyResponseSender&& send, const Authentication& auth);
	void on_upload(UploadRequest&& req, EmptyResponseSender&& send, const Authentication& auth);
	void unlink(BlobRequest&& req, EmptyResponseSender&& send);
	void update_blob(BlobRequest&& req, EmptyResponseSender&& send);
	void serve_view(const URLIntent& url, unsigned version, FileResponseSender&& send, const Authentication& auth);
	void serve_home(const EmptyRequest& req, FileResponseSender&& send, const Authentication& auth);
	void serve_collection(const URLIntent& intent, unsigned version, StringResponseSender&& send, const Authentication& auth);
	void scan_collection(const URLIntent& intent, unsigned version, StringResponseSender&& send, const Authentication& auth);
	void prepare_upload(UploadFile& result, std::error_code& ec);

	template <class Request, class Send>
	void on_request_view(Request&& req, Send&& send, const Authentication& auth);

	template <class Send>
	void get_blob(const BlobRequest& req, Send&& send);

private:
	const Configuration&    m_cfg;
	boost::asio::io_context m_ioc;

	redis::Pool     m_db;
	WebResources    m_lib;
	BlobDatabase    m_blob_db;
};

} // end of namespace
