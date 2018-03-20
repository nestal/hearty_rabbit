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
#include "Ownership.ipp"
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

const std::string_view Ownership::Collection::m_dir_prefix = "dir:";
const std::string_view Ownership::Collection::m_list_prefix = "dirs:";

Ownership::Collection::Collection(std::string_view user, std::string_view path) :
	m_user{user},
	m_path{path}
{
}

void Ownership::Collection::watch(redis::Connection& db)
{
	db.command("WATCH %b%b:%b",
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(), m_path.data(), m_path.size()
	);
}

void Ownership::Collection::link(redis::Connection& db, const ObjectID& id, const CollEntry& entry)
{
	db.command(
		"HSET %b%b:%b %b %b",
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		id.data(), id.size(),
		entry.data(), entry.size()
	);

	auto cover = R"({"cover":)" + to_quoted_hex(id) + "}";
	db.command(
		R"(HSETNX %b%b %b %b)",
		m_list_prefix.data(), m_list_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		cover.data(), cover.size()
	);
}

void Ownership::Collection::unlink(redis::Connection& db, const ObjectID& id)
{
	db.command("HDEL %b%b:%b %b",
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		id.data(), id.size()
	);
}

std::string Ownership::Collection::serialize(redis::Reply& reply, std::string_view requester) const
{
	std::ostringstream ss;
	ss  << R"__({"owner":")__"          << m_user
		<< R"__(", "collection":")__"  << m_path
		<< R"__(", "username":")__"    << requester
		<< R"__(", "elements":)__"     << "{";

	bool first = true;
	reply.foreach_kv_pair([&ss, &first, requester, this](auto&& blob, auto&& perm)
	{
		auto blob_id = raw_to_object_id(blob);
		CollEntry entry{perm.as_string()};

		// check permission: allow allow owner (i.e. m_user)
		if (blob_id && (m_user == requester || entry.permission().allow(requester)))
		{
			// entry string must be json. skip the { in the front and } in the back
			// and prepend the "perm"="public"
			auto json = entry.json();
			if (json.size() > 2 && json.front() == '{' && json.back() == '}')
			{
				if (first)
					first = false;
				else
					ss << ",\n";

				json.remove_prefix(1);
				json.remove_suffix(1);
				ss  << to_quoted_hex(*blob_id)
					<< ":{ \"perm\":\"" << entry.permission().description() << "\","
					<< json << "}";
			}
		}
	});
	ss << "}}";
	return ss.str();
}

} // end of namespace hrb
