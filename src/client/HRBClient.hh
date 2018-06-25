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

#include "common/Cookie.hh"
#include "common/UserID.hh"
#include "common/FS.hh"

#include <boost/beast/http/verb.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>

#include <memory>

namespace hrb {

class URLIntent;
struct ObjectID;

template <typename RequestBody, typename ResponseBody>
class GenericHTTPRequest;

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

	template <typename Complete>
	void get_blob(std::string_view owner, std::string_view coll, const ObjectID& blob, Complete&& comp);

	template <typename Complete>
	void get_blob_meta(std::string_view owner, std::string_view coll, const ObjectID& blob, Complete&& comp);

private:
	template <typename RequestBody, typename ResponseBody>
	auto request(const URLIntent& intent, boost::beast::http::verb method);

private:
	// connection to the server
	boost::asio::io_context&    m_ioc;
	boost::asio::ssl::context&  m_ssl;

	// configurations
	std::string m_host;
	std::string m_port;

	// authenticated user
	UserID  m_user;
};

}
