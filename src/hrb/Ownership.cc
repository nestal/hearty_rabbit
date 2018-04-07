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

#include <json.hpp>
#include <sstream>

namespace hrb {

Ownership::Ownership(std::string_view name) : m_user{name}
{
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

Ownership::Collection::Collection(std::string_view redis_reply)
{
	auto [prefix, colon] = split_left(redis_reply, ":");
	if (colon != ':' || prefix != Collection::m_dir_prefix.substr(0, Collection::m_dir_prefix.size()-1))
		return;

	auto [user, colon2] = split_left(redis_reply, ":");
	if (colon2 != ':')
		return;

	m_user = user;
	m_path = redis_reply;
}

std::string Ownership::Collection::redis_key() const
{
	return std::string{m_dir_prefix} + m_user + ':' + m_path;
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
	// the hash table is empty, remove the entry in dirs:<user> hash table.
	// Also, remove the 'cover' field in the dirs:<user> hash table if the cover
	// image is the one being removed.
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

nlohmann::json Ownership::Collection::serialize(redis::Reply& reply, std::string_view requester) const
{
	// TODO: get the cover here... where to find a redis::Connection?
	auto jdoc = nlohmann::json::object();

	jdoc.emplace("owner", m_user);
	jdoc.emplace("collection", m_path);

	auto elements = nlohmann::json::object();
	reply.foreach_kv_pair([&elements, &jdoc, requester, this](auto&& blob, auto&& perm, auto)
	{
		auto blob_id = raw_to_object_id(blob);
		CollEntry entry{perm.as_string()};

		// check permission: allow allow owner (i.e. m_user)
		if (blob_id && (m_user == requester || entry.permission().allow(requester)))
		{
			try
			{
				auto entry_jdoc = nlohmann::json::parse(entry.json());
				entry_jdoc.emplace("perm", std::string{entry.permission().description()});
				elements.emplace(to_hex(*blob_id), std::move(entry_jdoc));
			}
			catch (std::exception& e)
			{
				Log(LOG_WARNING, "exception thrown when parsing CollEntry::json(): %1% %2%", e.what(), entry.json());
			}
		}
	});
	jdoc.emplace("elements", std::move(elements));

	return jdoc;
}

} // end of namespace hrb
