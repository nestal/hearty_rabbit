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

namespace hrb {

const std::string_view PHashDb::m_key{"phash-oid"};

PHashDb::PHashDb(redis::Connection& db) : m_db{db}
{
}

void PHashDb::add(const ObjectID& blob, PHash phash)
{
	static const char lua[] = R"__(
		local str = redis.call('HGET', KEYS[1], ARGV[1])
		if not str then str = ARGV[2] else
			local pos = string.find(str, ARGV[2])
			if pos == nil
			then
				str = str .. ARGV[2]
			end
		end
		return redis.call('HSET', KEYS[1], ARGV[1], str)
	)__";
	m_db.command(
		[](auto&& reply, auto err)
		{
			if (!reply)
				Log(LOG_WARNING, "add() script reply: %1%", reply.as_error());
		},
		"EVAL %s 1 %b %d %b",
		lua,
		m_key.data(), m_key.size(),
		phash.value(),
		blob.data(), blob.size()
	);
}

} // end of namespace hrb
