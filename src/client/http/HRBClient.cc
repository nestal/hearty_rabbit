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

#include "../../URLIntent.hh"
#include "hrb/Blob.hh"

#include <regex>
#include <iostream>

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

Blob HRBClient::parse_response(const boost::beast::http::fields& response)
{
	// URL "parser"
	URLIntent intent{response[http::field::location]};

	// For /api/<user>/<coll>/<blobid> intents, the filename() will be a blob ID
	auto blobid = ObjectID::from_hex(response[http::field::etag]);
//	if (!blobid)
//		blobid = ObjectID::from_hex(intent.filename());

	BlobInode inode{};

	static const std::regex disposition_regex{"filename=(.+)"};
	std::string disposition{response[http::field::content_disposition]};
	std::smatch m;

	if (regex_search(disposition, m, disposition_regex) && m.size() == 2)
		inode.filename = m[1].str();

	return Blob{/*std::string{intent.user()}, std::string{intent.collection()}, blobid.value_or(ObjectID{}), inode*/};
}

} // end of namespace
