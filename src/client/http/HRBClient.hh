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

#include "RequestScheduler.hh"

#include "util/Cookie.hh"
#include "hrb/UserID.hh"
#include "util/FS.hh"

#include <boost/beast/http/verb.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/http/file_body.hpp>

#include <type_traits>

namespace hrb {

class Blob;
class Collection;
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
	void get_collection(std::string_view coll, Complete&& comp);

	template <typename Complete>
	void scan_collections(Complete&& comp);

	template <typename Complete>
	void upload(std::string_view coll, const fs::path& file, Complete&& comp);

	template <typename Complete, typename ByteIterator>
	void upload(std::string_view coll, std::string_view filename, ByteIterator first_byte, ByteIterator last_byte, Complete&& comp);

	template <typename Complete>
	void get_blob(std::string_view owner, std::string_view coll, const ObjectID& blob, std::string_view rendition, Complete&& comp);

	template <typename Complete, typename=std::enable_if_t<std::is_invocable_v<Complete, Blob, std::error_code>>>
	void download_blob(
		std::string_view owner,
		std::string_view coll,
		const ObjectID& blob,
		std::string_view rendition,
		const std::filesystem::path& dest,
		Complete&& comp
	);

	template <typename Complete>
	void download_collection(
		const Collection& coll,
		std::string_view rendition,
		const std::filesystem::path& dest_dir,
		Complete&& comp
	);

	template <typename Complete>
	void get_blob_meta(std::string_view owner, std::string_view coll, const ObjectID& blob, Complete&& comp);

private:
	template <typename RequestBody, typename ResponseBody>
	auto request(const URLIntent& intent, boost::beast::http::verb method);

	template <typename Complete, typename Response>
	void handle_upload_response(Response& response, Complete&& comp, std::error_code ec);

	static Blob parse_response(const boost::beast::http::fields& response);

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
	RequestScheduler m_outstanding;
};

}
