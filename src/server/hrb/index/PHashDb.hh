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
	void exact_match(PHash phash, Complete&& comp) const
	{
		m_db.command(
			[comp=std::forward<Complete>(comp)](auto&& reply, auto err)
			{
				std::vector<ObjectID> result;

				auto oids = reply.as_string();
				while (oids.size() >= ObjectID{}.size())
				{
					if (auto oid = ObjectID::from_raw(oids.substr(0, ObjectID{}.size())); oid.has_value())
						result.push_back(*oid);

					oids.remove_prefix(ObjectID{}.size());
				}

				comp(std::move(result), err);
			},
			"HGET %b %d",
			m_key.data(), m_key.size(),
			phash
		);
	}

private:
	static const std::string_view m_key;

private:
	redis::Connection&  m_db;
};

} // end of namespace hrb
