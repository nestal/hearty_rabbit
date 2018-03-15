/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/15/18.
//

#include "BlobRequest.hh"

namespace hrb {

BlobRequest::BlobRequest(BlobRequest&& other) noexcept :
	m_requester{std::move(other.m_requester)}, m_target{std::move(other.m_target)}, m_url{m_target},
	m_version{other.m_version}, m_etag{std::move(other.m_etag)}, m_body{std::move(other.m_body)}
{
}

BlobRequest::BlobRequest(const BlobRequest& other) :
	m_requester{other.m_requester}, m_target{other.m_target}, m_url{m_target},
	m_version{other.m_version}, m_etag{other.m_etag}, m_body{other.m_body}
{
}

BlobRequest& BlobRequest::operator=(BlobRequest&& other) noexcept
{
	BlobRequest tmp{std::move(other)};
	swap(tmp);
	return *this;
}
BlobRequest& BlobRequest::operator=(const BlobRequest& other)
{
	BlobRequest tmp{other};
	swap(tmp);
	return *this;
}

void BlobRequest::swap(BlobRequest& tmp) noexcept
{
	m_requester.swap(tmp.m_requester);
	m_target.swap(tmp.m_target);
	m_url = URLIntent{m_target};
	m_version = tmp.m_version;
	m_etag.swap(tmp.m_etag);
	m_body.swap(tmp.m_body);
}

} // end of namespace
