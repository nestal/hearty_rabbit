/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/14/18.
//

#pragma once

#include "common/ObjectID.hh"
#include "common/URLIntent.hh"
#include "net/Request.hh"

#include <string_view>

namespace hrb {

class Authentication;

class BlobRequest
{
public:
	template <typename Request>
	BlobRequest(Request&& req, URLIntent&& intent) :
		m_url{std::move(intent)}, m_version{req.version()}, m_etag{req[http::field::if_none_match]}
	{
		if constexpr (std::is_same<std::remove_reference_t<Request>, StringRequest>::value)
		{
			if (req[http::field::content_type] == "application/x-www-form-urlencoded")
				m_body = std::move(req.body());
		}
	}
	BlobRequest(BlobRequest&& other) = default;
	BlobRequest(const BlobRequest& other) = default;
	BlobRequest& operator=(BlobRequest&& other) = default;
	BlobRequest& operator=(const BlobRequest& other) = default;

	std::optional<ObjectID> blob() const    {return ObjectID::from_hex(m_url.filename());}
	std::string_view owner() const          {return m_url.user();}
	std::string_view collection() const     {return m_url.collection();}
	std::string_view option() const         {return m_url.option();}
	std::string_view etag() const           {return m_etag;}
	unsigned version() const                {return m_version;}

	bool request_by_owner(const Authentication& requester) const;

	std::string_view body() const           {return m_body;}

	const URLIntent& intent() const         {return m_url;}

private:
	URLIntent   m_url;
	unsigned    m_version;

	std::string m_etag;
	std::string m_body;
};

} // end of namespace
