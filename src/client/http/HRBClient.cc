/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#include "HRBClient.hh"

#include "GenericHTTPRequest.hh"

#include "hrb/URLIntent.hh"
#include "hrb/BlobInode.hh"

#include <regex>

namespace hrb {

HRBClient::HRBClient(
	boost::asio::io_context& ioc,
	boost::asio::ssl::context& ctx,
	std::string_view host,
	std::string_view port
) :
	m_ioc{ioc}, m_ssl{ctx}, m_host{host}, m_port{port}
{
}

BlobInode HRBClient::parse_response(const boost::beast::http::fields& response)
{
	// URL "parser"
	URLIntent intent{response.at(http::field::location)};

	BlobInode result{};

	static const std::regex disposition_regex{"filename=(.+)"};
	std::string disposition{response.at(http::field::content_disposition)};
	std::smatch m;

	if (regex_match(disposition, m, disposition_regex) && m.size() == 2)
		result.filename = m[1].str();

	return result;
}

} // end of namespace
