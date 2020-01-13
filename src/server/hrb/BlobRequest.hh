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

#include "hrb/ObjectID.hh"
#include "hrb/URLIntent.hh"
#include "net/Request.hh"

#include <string_view>

namespace hrb {

class UserID;

class BlobRequest
{
public:
	template <typename Request>
	BlobRequest(Request&& req, URLIntent&& intent) :
		m_url{std::move(intent)}, m_version{req.version()}, m_etag{req[http::field::if_none_match]}
	{
		if constexpr (std::is_same_v<std::decay_t<Request>, StringRequest>)
		{
			if (req[http::field::content_type] == "application/x-www-form-urlencoded")
				m_body = std::move(req.body());
		}
	}
	BlobRequest(BlobRequest&& other) = default;
	BlobRequest(const BlobRequest& other) = default;
	BlobRequest& operator=(BlobRequest&& other) = default;
	BlobRequest& operator=(const BlobRequest& other) = default;

	[[nodiscard]] std::optional<ObjectID> blob() const    {return ObjectID::from_hex(m_url.filename());}
	[[nodiscard]] std::string_view owner() const          {return m_url.user();}
	[[nodiscard]] std::string_view collection() const     {return m_url.collection();}
	[[nodiscard]] std::string_view option() const         {return m_url.option();}
	[[nodiscard]] std::string_view etag() const           {return m_etag;}
	[[nodiscard]] unsigned version() const                {return m_version;}

	[[nodiscard]] bool request_by_owner(const UserID& requester) const;

	[[nodiscard]] std::string_view body() const           {return m_body;}

	[[nodiscard]] const URLIntent& intent() const         {return m_url;}

private:
	URLIntent   m_url;
	unsigned    m_version;

	std::string m_etag;
	std::string m_body;
};

} // end of namespace
