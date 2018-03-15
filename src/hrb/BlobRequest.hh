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

#include "ObjectID.hh"
#include "URLIntent.hh"
#include "net/Request.hh"

#include <string_view>

namespace hrb {

class BlobRequest
{
public:
	template <typename Request>
	BlobRequest(Request&& req, std::string_view requester) :
		m_requester{requester}, m_target{req.target()}, m_url{m_target}, m_version{req.version()},
		m_etag{req[http::field::if_none_match]}
	{
		if constexpr (std::is_same<std::remove_reference_t<Request>, StringRequest>::value)
		{
			if (req[http::field::content_type] == "application/x-www-form-urlencoded")
				m_body = std::move(req.body());
		}
	}
	BlobRequest(BlobRequest&& other);
	BlobRequest(const BlobRequest& other);
	BlobRequest& operator=(BlobRequest&& other);
	BlobRequest& operator=(const BlobRequest& other);

	void swap(BlobRequest& tmp);

	std::optional<ObjectID> blob() const {return hex_to_object_id(m_url.filename());}
	std::string_view requester() const {return m_requester;}
	std::string_view owner() const {return m_url.user();}
	std::string_view collection() const {return m_url.collection();}
	std::string_view etag() const {return m_etag;}
	unsigned version() const {return m_version;}

	bool request_by_owner() const {return m_requester == owner();}

	std::string_view body() const {return m_body;}

private:
	std::string m_requester;
	std::string m_target;
	URLIntent   m_url;
	unsigned    m_version;

	std::string m_etag;
	std::string m_body;
};

} // end of namespace
