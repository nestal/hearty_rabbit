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

#include "net/Redis.hh"
#include "net/Request.hh"

#include <json.hpp>

#include <system_error>
#include <functional>
#include <utility>

namespace hrb {

class Authentication;
class BlobDatabase;
class BlobRequest;
class Configuration;
class MMapResponseBody;
class SplitBuffers;
class URLIntent;
class UploadFile;
class WebResources;

class SessionHandler
{
public:
	SessionHandler(std::shared_ptr<redis::Connection>&& db, WebResources& lib, BlobDatabase& blob_db, const Configuration& cfg) :
		m_db{db}, m_lib{lib}, m_blob_db{blob_db}, m_cfg{cfg}
	{
	}

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

	std::size_t upload_limit() const;
	std::chrono::seconds session_length() const;

private:
	http::response<SplitBuffers> file_request(const URLIntent& intent, boost::string_view etag, unsigned version);

	template <class Send>
	class SendJSON;

	using EmptyResponseSender  = std::function<void(http::response<http::empty_body>&&)>;
	using StringResponseSender = std::function<void(http::response<http::string_body>&&)>;
	using FileResponseSender   = std::function<void(http::response<SplitBuffers>&&)>;
	using BlobResponseSender   = std::function<void(http::response<MMapResponseBody>&&)>;

	void on_login(const StringRequest& req, EmptyResponseSender&& send);
	void on_logout(const EmptyRequest& req, EmptyResponseSender&& send, const Authentication& auth);
	void on_upload(UploadRequest&& req, EmptyResponseSender&& send, const Authentication& auth);
	void unlink(BlobRequest&& req, EmptyResponseSender&& send);
	void update_blob(BlobRequest&& req, EmptyResponseSender&& send);
	void prepare_upload(UploadFile& result, std::error_code& ec);

	template <class Send>
	void scan_collection(const URLIntent& intent, unsigned version, Send&& send, const Authentication& auth);

	template <class Request, class Send>
	void on_request_view(Request&& req, URLIntent&& intent, Send&& send, const Authentication& auth);

	template <class Send>
	void view_collection(const URLIntent& intent, unsigned version, Send&& send, const Authentication& auth);

	template <class Send>
	void view_blob(const BlobRequest& req, Send&& send);

	template <class Send>
	void on_query(const BlobRequest& req, Send&& send, const Authentication& auth);

	template <class Send>
	void query_blob(const BlobRequest& req, Send&& send, const Authentication& auth);

	template <class Send>
	void query_blob_set(const URLIntent& intent, unsigned version, Send&& send, const Authentication& auth);

private:
	std::shared_ptr<redis::Connection>     m_db;
	WebResources&           m_lib;
	BlobDatabase&           m_blob_db;
	const Configuration&    m_cfg;
};

} // end of namespace hrb
