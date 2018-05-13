/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#include "PHashDb.hh"

#include "image/PHash.hh"
#include "net/Redis.hh"

namespace hrb {

const std::string_view PHashDb::m_key{"phashdb:"};

PHashDb::PHashDb(redis::Connection& db) : m_db{db}
{
}

void PHashDb::add(const ObjectID& blob, PHash phash)
{
	m_db.command(
		"ZADD %b %d %b",
		m_key.data(), m_key.size(),
		phash.value(),
		blob.data(), blob.size()
	);
}

} // end of namespace hrb
