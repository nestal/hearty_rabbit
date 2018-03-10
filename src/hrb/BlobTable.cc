/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/10/18.
//

#include "BlobTable.hh"

#include "net/Redis.hh"

namespace hrb {
namespace {
static const std::string_view redis_prefix{"blob:"};
}

BlobTable::BlobTable(std::string_view user, const ObjectID& blob) :
	m_user{user}, m_blob{blob}
{
}

void BlobTable::watch(redis::Connection& db) const
{
	db.command(
		"WATCH %b%b:%b",
		redis_prefix.data(), redis_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size()
	);
}

void BlobTable::add_link(redis::Connection& db, std::string_view mime, std::string_view path) const
{
	const char empty = '\0';
	db.command(
		"HMSET %b%b:%b mime %b link:%b %b",
		redis_prefix.data(), redis_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size(),
		mime.data(), mime.size(),
		path.data(), path.size(),
		&empty, 0
	);
}

} // end of namespace hrb
