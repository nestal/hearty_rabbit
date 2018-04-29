/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/14/18.
//

// This file is intended to be used as a precompile header.
// #pragma once doesn't work well for precompiled headers
// so we use an old-style include guard here
#ifndef HRB_NET_REQUEST_PRECOMPILED_HEADER_INCLUDED
#define HRB_NET_REQUEST_PRECOMPILED_HEADER_INCLUDED

#include <boost/beast/http/message.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/buffer_body.hpp>
#include <boost/beast/http/empty_body.hpp>
#include <boost/beast/http/parser.hpp>

#include <boost/asio/ip/tcp.hpp>

#include <variant>

namespace hrb {

class UploadRequestBody;

using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

using EndPoint = boost::asio::ip::tcp::endpoint;

using RequestHeader = http::header<true, http::fields>;

using HeaderRequest = http::request<http::buffer_body>;
using StringRequest = http::request<http::string_body>;
using EmptyRequest  = http::request<http::empty_body>;
using UploadRequest = http::request<UploadRequestBody>;

using HeaderRequestParser 	= http::request_parser<http::buffer_body>;
using StringRequestParser 	= http::request_parser<http::string_body>;
using EmptyRequestParser 	= http::request_parser<http::empty_body>;
using UploadRequestParser   = http::request_parser<UploadRequestBody>;

}

#endif
