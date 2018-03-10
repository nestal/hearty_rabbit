/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include "Collection.hh"

namespace hrb {

const std::string_view Collection::redis_prefix = "dir:";

Collection::Collection(std::string_view user, std::string_view path) :
	m_user{user},
	m_path{path}
{
}

void Collection::watch(redis::Connection& db)
{
	db.command("WATCH %b%b:%b",
		redis_prefix.data(), redis_prefix.size(),
		m_user.data(), m_user.size(), m_path.data(), m_path.size()
	);
}

void Collection::link(redis::Connection& db, const ObjectID& id)
{
	db.command("SADD %b%b:%b %b",
		redis_prefix.data(), redis_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		id.data(), id.size()
	);
}

void Collection::unlink(redis::Connection& db, const ObjectID& id)
{
	db.command("SREM %b%b:%b %b",
		redis_prefix.data(), redis_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		id.data(), id.size()
	);
}


} // end of namespace hrb
