/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 4/8/18.
//

#pragma once

#include "hrb/UserID.hh"
#include "net/Redis.hh"
#include "net/Request.hh"

// JSON library
#include <nlohmann/json.hpp>

// standard library
#include <chrono>
#include <functional>
#include <optional>
#include <system_error>
#include <utility>

namespace hrb {

class Authentication;
class BlobDatabase;
class BlobRequest;
class Collection;
class Configuration;
class MMapResponseBody;
class SplitBuffers;
class URLIntent;
class UploadFile;
class WebResources;

class SessionHandler
{
public:
	enum class RequestBodyType {string, upload, empty};

public:
	SessionHandler(
		std::shared_ptr<redis::Connection>&& db,
		WebResources& lib,
		BlobDatabase& blob_db,
		const Configuration& cfg
	);

	template <class Complete>
	void on_request_header(
		const RequestHeader& header,
		Complete&& complete
	);

	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template<class Request, class Send>
	void on_request_body(Request&& req, Send&& send);

	void prepare_upload(UploadFile& result, std::error_code& ec);

	// ugly hack for unit test
	template <class Request, class Send>
	void handle_request(Request&& req, Send&& send, const UserID& auth)
	{
		m_auth = auth;
		on_request_body(std::forward<Request>(req), std::forward<Send>(send));
	};

	static http::response<http::string_body> bad_request(std::string_view why, unsigned version);
	http::response<SplitBuffers> not_found(std::string_view target, unsigned version);
	static http::response<http::string_body> server_error(std::string_view what, unsigned version);
	static http::response<http::empty_body> see_other(std::string_view where, unsigned version);

	[[nodiscard]] std::chrono::seconds session_length() const;

	[[nodiscard]] const UserID& auth() const {return m_auth;}
	[[nodiscard]] bool renewed_auth() const;

private:
	http::response<SplitBuffers> file_request(const URLIntent& intent, std::string_view etag, unsigned version);

	template <class Send>
	class SendJSON;

	using EmptyResponseSender  = std::function<void(http::response<http::empty_body>&&)>;
	using StringResponseSender = std::function<void(http::response<http::string_body>&&)>;
	using FileResponseSender   = std::function<void(http::response<SplitBuffers>&&)>;
	using BlobResponseSender   = std::function<void(http::response<MMapResponseBody>&&)>;

	void on_login(const StringRequest& req, EmptyResponseSender&& send);
	void on_logout(const EmptyRequest& req, EmptyResponseSender&& send);
	void on_upload(UploadRequest&& req, EmptyResponseSender&& send);
	void unlink(BlobRequest&& req, EmptyResponseSender&& send);
	void post_blob(BlobRequest&& req, EmptyResponseSender&& send);

	template <class Send>
	void post_view(BlobRequest&& req, Send&& send);

	template <class Send>
	void scan_collection(const URLIntent& intent, unsigned version, Send&& send);

	template <class Request, class Send>
	void on_request_view(Request&& req, URLIntent&& intent, Send&& send);

	template <class Request, class Send>
	void on_request_api(Request&& req, URLIntent&& intent, Send&& send);

	template <class Send>
	void get_blob(const BlobRequest& req, Send&& send);

	template <class Send>
	void on_query(const BlobRequest& req, Send&& send);

	template <class Send>
	void query_blob(const BlobRequest& req, Send&& send);

	template <class Send>
	void query_blob_set(const URLIntent& intent, unsigned version, Send&& send);

	template <class Send>
	void list_public_blobs(bool is_json, std::string_view user, unsigned version, Send&& send);

	std::string server_root() const;
	void validate_collection(Collection& json);

private:
	std::shared_ptr<redis::Connection>              m_db;
	std::optional<UserID::SessionID>                m_request_session_id;
	std::chrono::high_resolution_clock::time_point  m_on_header;

	UserID                  m_auth;
	WebResources&           m_lib;
	BlobDatabase&           m_blob_db;
	const Configuration&    m_cfg;
};

} // end of namespace hrb
