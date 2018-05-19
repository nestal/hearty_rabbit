/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#include "TimeIndex.hh"

#include <cassert>

namespace hrb {

const std::string_view TimeIndex::m_key{"timeidx:"};

TimeIndex::TimeIndex(redis::Connection& db) : m_db{db}
{

}

void TimeIndex::add(std::string_view user, const ObjectID& blob, TimeIndex::time_point tp)
{
	m_db.command(
		[](auto&& reply, auto err)
		{
			assert(!err);
		},
		"ZADD %b%b %d %b",
		m_key.data(), m_key.size(),
		user.data(), user.size(),
		tp.time_since_epoch().count(),
		blob.data(), blob.size()
	);
}

} // end of namespace hrb
