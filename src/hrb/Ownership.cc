/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/23/18.
//

#include "Ownership.hh"
#include "BlobDatabase.hh"

#include <sstream>

namespace hrb {

Ownership::Ownership(std::string_view name) : m_user{name}
{
}

const std::string_view Ownership::Blob::m_prefix{"blob:"};

Ownership::Blob::Blob(std::string_view user, const ObjectID& blob) :
	m_user{user}, m_blob{blob}
{
}

void Ownership::Blob::watch(redis::Connection& db) const
{
	db.command(
		"WATCH %b%b:%b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size()
	);
}

void Ownership::Blob::link(redis::Connection& db, std::string_view path) const
{
	// If the blob link already exists, it should be pointing to an existing
	// permission string. We should not change it
	const char empty = '\0';
	db.command(
		"HSETNX %b%b:%b link:%b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size(),
		path.data(), path.size(),
		&empty, 0
	);
}

void Ownership::Blob::unlink(redis::Connection& db, std::string_view path) const
{
	db.command(
		"HDEL %b%b:%b link:%b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size(),
		path.data(), path.size()
	);
}

} // end of namespace hrb
