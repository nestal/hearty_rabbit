/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#pragma once

#include "hrb/ObjectID.hh"

#include "image/PHash.hh"
#include "net/Redis.hh"
#include "util/Log.hh"

#include <boost/range/adaptor/transformed.hpp>

namespace hrb {
namespace redis {
class Connection;
}

/// \brief Encapsulate the redis sorted set for storing phashes
class PHashDb
{
public:
	explicit PHashDb(redis::Connection& db);

	void add(const ObjectID& blob, PHash phash);

	template <class Complete>
	void similar(const ObjectID& src, Complete&& comp) const
	{
		static const char lua[] = R"__(
			local rank = redis.call("ZRANK", KEYS[1], ARGV[1])
			local start, end_ = rank, rank+1
			if start > 0 then start = start - 1 end
			return redis.call("ZRANGE", KEYS[1], start, end_, "WITHSCORES")
		)__";

		m_db.command(
			[comp=std::forward<Complete>(comp)](auto&& reply, auto err)
			{
				if (!reply || err)
					Log(LOG_WARNING, "similar() script reply %1% %2%", reply.as_error(), err);

				struct Entry
				{
					ObjectID    id;
					PHash       phash;
				};

				auto make_entry = [](auto&& kv)
				{
					auto oid = raw_to_object_id(kv.key());
					return Entry{
						oid.has_value() ? *oid : ObjectID{},
						PHash{static_cast<std::uint64_t>(kv.value().to_int())}
					};
				};

				using namespace boost::adaptors;
				comp(reply.kv_pairs() | transformed(make_entry), err);
			},
			"EVAL %s 1 %b %b",
			lua,
			m_key.data(), m_key.size(),
			src.data(), src.size()
		);
	}

private:
	static const std::string_view m_key;

private:
	redis::Connection&  m_db;
};

} // end of namespace hrb
