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

const std::string_view Ownership::redis_prefix{"ownership:"};

namespace detail {
const std::string_view Prefix<ObjectID>::value{"blob:"};
const std::string_view Prefix<ContainerName>::value{"dir:"};
}

Ownership::Ownership(std::string_view name) : m_user{name}
{
}

std::string Ownership::serialize(const BlobDatabase& db) const
{
	bool first = true;
	std::ostringstream json;
	json << R"({"name":")" << m_user << R"(", "elements":{)";
	for (auto&& blob : m_blobs)
	{
		auto meta = db.load_meta_json(blob);
		if (!meta.empty())
		{
			if (first)
				first = false;
			else
				json << ",\n";

			json << '\"' << blob << R"(": )" << meta;
		}
	}
	json << "}}";

	return json.str();
}

Ownership::BlobTable::BlobTable(std::string_view user, const ObjectID& blob) :
	m_user{user}, m_blob{blob}
{
}

void Ownership::BlobTable::watch(redis::Connection& db) const
{
	db.command(
		"WATCH %b%b:%b",
		detail::Prefix<ObjectID>::value.data(), detail::Prefix<ObjectID>::value.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size()
	);
}

void Ownership::BlobTable::link(redis::Connection& db, std::string_view path) const
{
	const char empty = '\0';
	db.command(
		"HMSET %b%b:%b link:%b %b",
		detail::Prefix<ObjectID>::value.data(), detail::Prefix<ObjectID>::value.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size(),
		path.data(), path.size(),
		&empty, 0
	);
}

void Ownership::BlobTable::unlink(redis::Connection& db, std::string_view path) const
{
	db.command(
		"HDEL %b%b:%b link:%b",
		detail::Prefix<ObjectID>::value.data(), detail::Prefix<ObjectID>::value.size(),
		m_user.data(), m_user.size(),
		m_blob.data(), m_blob.size(),
		path.data(), path.size()
	);
}

} // end of namespace hrb
