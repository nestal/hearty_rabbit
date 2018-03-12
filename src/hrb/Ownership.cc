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

const std::string_view Ownership::BlobBackLink::m_prefix{"blob-backlink:"};

Ownership::BlobBackLink::BlobBackLink(std::string_view user, const ObjectID& blob) :
	m_user{user}, m_blob{blob}
{
}

void Ownership::BlobBackLink::watch(redis::Connection& db) const
{
	db.command(
		"WATCH %b%b:%b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size()
	);
}

void Ownership::BlobBackLink::link(redis::Connection& db, std::string_view coll) const
{
	// If the blob link already exists, it should be pointing to an existing
	// permission string. We should not change it
	const char empty = '\0';
	db.command(
		"SADD %b%b:%b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size(),
		coll.data(), coll.size()
	);
}

void Ownership::BlobBackLink::unlink(redis::Connection& db, std::string_view coll) const
{
	db.command(
		"SREM %b%b:%b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size(),
		coll.data(), coll.size()
	);
}

const std::string_view Ownership::Collection::m_prefix = "dir:";

Ownership::Collection::Collection(std::string_view user, std::string_view path) :
	m_user{user},
	m_path{path}
{
}

void Ownership::Collection::watch(redis::Connection& db)
{
	db.command("WATCH %b%b:%b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(), m_path.data(), m_path.size()
	);
}

void Ownership::Collection::link(redis::Connection& db, const ObjectID& id, std::string_view perm)
{
	db.command("HSET %b%b:%b %b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		id.data(), id.size(),
		perm.data(), perm.size()
	);
}

void Ownership::Collection::unlink(redis::Connection& db, const ObjectID& id)
{
	db.command("HDEL %b%b:%b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		id.data(), id.size()
	);
}

} // end of namespace hrb
