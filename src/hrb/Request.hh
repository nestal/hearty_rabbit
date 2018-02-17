/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/14/18.
//

#pragma once

#include "BlobDatabase.hh"

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/file_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>

#include <boost/asio/ip/tcp.hpp>

namespace hrb {

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

using EndPoint = boost::asio::ip::tcp::endpoint;

using StringRequest = http::request<http::string_body>;
using FileRequest   = http::request<http::file_body>;
using EmptyRequest  = http::request<http::empty_body>;
using UploadRequest = http::request<http::basic_file_body<BlobDatabase::TempFile>>;

using StringRequestParser 	= http::request_parser<http::string_body>;
using FileRequestParser 	= http::request_parser<http::file_body>;
using EmptyRequestParser 	= http::request_parser<http::empty_body>;
using UploadRequestParser   = http::request_parser<http::basic_file_body<BlobDatabase::TempFile>>;

}
