/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#pragma once

#include "util/Cookie.hh"
#include "hrb/UserID.hh"
#include "util/FS.hh"

#include <boost/beast/http/verb.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>

#include <unordered_set>
#include <deque>

namespace hrb {

class BaseRequest;
class URLIntent;
struct ObjectID;

class HRBClient
{
public:
	HRBClient(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx, std::string_view host, std::string_view port);

	template <typename Complete>
	void login(std::string_view user, std::string_view password, Complete&& comp);

	template <typename Complete>
	void list_collection(std::string_view coll, Complete&& comp);

	template <typename Complete>
	void scan_collections(Complete&& comp);

	template <typename Complete>
	void upload(std::string_view coll, const fs::path& file, Complete&& comp);

	template <typename Complete, typename ByteIterator>
	void upload(std::string_view coll, std::string_view filename, ByteIterator first_byte, ByteIterator last_byte, Complete&& comp);

	template <typename Complete>
	void get_blob(std::string_view owner, std::string_view coll, const ObjectID& blob, std::string_view rendition, Complete&& comp);

	template <typename Complete>
	void download_blob(
		std::string_view owner,
		std::string_view coll,
		const ObjectID& blob,
		std::string_view rendition,
		const std::filesystem::path& dest,
		Complete&& comp
	);

	template <typename Complete>
	void get_blob_meta(std::string_view owner, std::string_view coll, const ObjectID& blob, Complete&& comp);

private:
	template <typename RequestBody, typename ResponseBody>
	auto request(const URLIntent& intent, boost::beast::http::verb method);

	template <typename Complete, typename Response>
	void handle_upload_response(Response& response, Complete&& comp, std::error_code ec);

	void add_request(std::shared_ptr<BaseRequest>&& req);
	void try_start_requests();
	void finish_request(const std::shared_ptr<BaseRequest>& req);

private:
	// connection to the server
	boost::asio::io_context&    m_ioc;
	boost::asio::ssl::context&  m_ssl;

	// configurations
	std::string m_host;
	std::string m_port;

	// authenticated user
	UserID  m_user;

	// outstanding and pending requests
	std::size_t m_outstanding_limit{5};
	std::unordered_set<std::shared_ptr<BaseRequest>> m_outstanding;
	std::deque<std::shared_ptr<BaseRequest>> m_pending;
};

}
