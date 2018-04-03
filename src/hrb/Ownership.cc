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

#include "util/Log.hh"
#include "util/Escape.hh"

#include <sstream>

namespace hrb {

Ownership::Ownership(std::string_view name) : m_user{name}
{
}

// Parse the string that was put in the redis set, i.e. "%b%b:%b"
Ownership::BlobBackLink::BlobBackLink(std::string_view db_str, const ObjectID& blob) : m_blob{blob}
{
	auto [prefix, colon] = split_front(db_str, ":");
	if (colon != ':' || prefix != Collection::m_dir_prefix.substr(0, Collection::m_dir_prefix.size()-1))
		return;

	auto [user, colon2] = split_front(db_str, ":");
	if (colon2 != ':')
		return;

	m_user = user;
	m_coll = db_str;
}

const std::string_view Ownership::BlobBackLink::m_prefix{"blob-ref:"};

Ownership::BlobBackLink::BlobBackLink(std::string_view user, std::string_view coll, const ObjectID& blob) :
	m_user{user}, m_coll{coll}, m_blob{blob}
{
}

void Ownership::BlobBackLink::link(redis::Connection& db) const
{
	db.command(
		"SADD %b:%b %b%b:%b",
		m_prefix.data(), m_prefix.size(),
		m_blob.data(), m_blob.size(),
		Collection::m_dir_prefix.data(), Collection::m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_coll.data(), m_coll.size()
	);
}

void Ownership::BlobBackLink::unlink(redis::Connection& db) const
{
	db.command(
		"SREM %b:%b %b%b:%b",
		m_prefix.data(), m_prefix.size(),
		m_blob.data(), m_blob.size(),
		Collection::m_dir_prefix.data(), Collection::m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_coll.data(), m_coll.size()
	);
}

const std::string_view Ownership::Collection::m_dir_prefix = "dir:";
const std::string_view Ownership::Collection::m_list_prefix = "dirs:";
const std::string_view Ownership::Collection::m_public_blobs = "public-blobs";

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

	set_cover(db, id, [](auto&&, auto&&){});
}

void Ownership::Collection::unlink(redis::Connection& db, const ObjectID& id)
{
	// LUA script: delete the blob from the dir:<user>:<coll> hash table, and if
	// the hash table is empty, remove the entry in dirs:<user> hash table
	static const char cmd[] = R"__(
		redis.call('HDEL', KEYS[1], ARGV[1])
		if redis.call('EXISTS', KEYS[1]) == 0 then redis.call('HDEL', KEYS[2], ARGV[2]) else
			local album = cjson.decode(redis.call('HGET', KEYS[2], ARGV[2]))
			if album['cover'] == ARGV[3] then
				album['cover'] = nil
				redis.call('HSET', KEYS[2], ARGV[2], cjson.encode(album))
			end
		end
	)__";

	auto hex_id = to_hex(id);

	db.command(
		[](auto&& reply, auto ec)
		{
			if (!reply || ec)
				Log(LOG_WARNING, "unlink lua script failure: %1% (%2%)", reply.as_error(), ec);
		},
		"EVAL %s 2 %b%b:%b %b%b %b %b %b",

		cmd,

		// KEYS[1] (hash table that stores the blob in a collection)
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),

		// KEYS[2] (hash table that stores all collections owned by a user)
		m_list_prefix.data(), m_list_prefix.size(),
		m_user.data(), m_user.size(),

		// ARGV[1] (name of the blob to unlink)
		id.data(), id.size(),

		// ARGV[2] (collection name)
		m_path.data(), m_path.size(),

		// ARGV[3] (hex of blob ID)
		hex_id.data(), hex_id.size()
	);
}

std::string Ownership::Collection::serialize(redis::Reply& reply, std::string_view requester) const
{
	// TODO: get the cover here... where to find a redis::Connection?

	std::ostringstream ss;
	ss  << R"__({"owner":")__"         << m_user
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
			if (json.size() >= 2 && json.front() == '{' && json.back() == '}')
			{
				if (first)
					first = false;
				else
					ss << ",\n";

				json.remove_prefix(1);
				json.remove_suffix(1);
				ss  << to_quoted_hex(*blob_id)
					<< ":{ \"perm\":\"" << entry.permission().description() << "\"";
				if (!json.empty())
					ss << "," << json;
				ss << "}";
			}
		}
	});
	ss << "}}";
	return ss.str();
}

} // end of namespace hrb
