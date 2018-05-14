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
		local oids = redis.call('HGET', KEYS[1], ARGV[1])
		local pos = 0
		if not oids then oids = ARGV[2] else
			pos = string.find(oids, ARGV[2], 1, true) -- use plain text search
			if not pos
			then
				oids = oids .. ARGV[2]
				pos = -1
			end
		end
		redis.call('HSET', KEYS[1], ARGV[1], oids)
		return oids
	)__";
	m_db.command(
		[](auto&& reply, auto err)
		{
			Log(LOG_DEBUG, "add() script reply integer: %1%", reply.as_int());
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
