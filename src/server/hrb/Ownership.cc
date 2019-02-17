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
#include "RedisKeys.hh"

#include "util/Log.hh"
#include "common/Escape.hh"

#include <nlohmann/json.hpp>
#include <sstream>

namespace hrb {

Ownership::Ownership(std::string_view name) : m_user{name}
{
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

hrb::Collection Ownership::from_reply(
	const redis::Reply& reply,
	std::string_view coll,
	const Authentication& requester,
	nlohmann::json&& meta
) const
{
	assert(meta.is_object());
	hrb::Collection result{coll, m_user, std::move(meta)};

	for (auto&& kv : reply.kv_pairs())
	{
		auto&& blob = kv.key();
		auto&& perm = kv.value();

		if (perm.as_string().empty())
			continue;

		auto blob_id = ObjectID::from_raw(blob);
		CollEntryDB entry{perm.as_string()};

		// check permission: allow allow owner (i.e. m_user)
		if (blob_id && entry.permission().allow(requester.id(), m_user))
		{
			if (auto fields = entry.fields(); fields.has_value())
				result.add_blob(*blob_id, *fields);
		}
	};
	return result;
}

void Ownership::update(redis::Connection& db, const ObjectID& blobid, const CollEntry& entry)
{
	// assume the blob is already in the collection, so there is no need to update
	// blob backlink
	auto s = CollEntryDB::create(entry);
	update(db, blobid, CollEntryDB{s});
}

void Ownership::update(redis::Connection& db, const ObjectID& blobid, const nlohmann::json& entry)
{
	// assume the blob is already in the collection, so there is no need to update
	// blob backlink
	auto en_str = CollEntryDB::create(Permission::from_description(entry["perm"].get<std::string>()), entry);
	update(db, blobid, CollEntryDB{en_str});
}

void Ownership::update(redis::Connection& db, const ObjectID& blob, const CollEntryDB& entry)
{
	auto blob_meta  = key::blob_meta(m_user, blob);

	static const char lua[] = R"__(
		local user, coll, blob, cover, entry = ARGV[1], ARGV[2], ARGV[3], ARGV[4], ARGV[5],
		redis.call('SADD', KEYS[1], coll)
		redis.call('SADD', KEYS[2], user)
		redis.call('SET',  KEYS[3], entry)
		redis.call('SADD', KEYS[4], blob)
		redis.call('HSETNX', KEYS[5], coll, cjson.encode({cover=cover}))
	)__";
	db.command(
		"SET %b %b",
		blob_meta.data(), blob_meta.size(),
		entry.data(), entry.size()
	);
}

redis::CommandString Ownership::link_command(std::string_view coll, const ObjectID& blob, const CollEntry& coll_entry) const
{
	auto blob_ref   = key::blob_refs(m_user, blob);
	auto blob_owner = key::blob_owners(blob);
	auto blob_meta  = key::blob_meta(m_user, blob);
	auto coll_key   = key::collection(m_user, coll);
	auto coll_list  = key::collection_list(m_user);
	auto hex = to_hex(blob);
	auto entry = CollEntryDB::create(coll_entry);

	static const char lua[] = R"__(
		local user, coll, blob, cover, entry = ARGV[1], ARGV[2], ARGV[3], ARGV[4], ARGV[5]
		redis.call('SADD', KEYS[1], coll)
		redis.call('SADD', KEYS[2], user)
		redis.call('SET',  KEYS[3], entry)
		redis.call('SADD', KEYS[4], blob)
		redis.call('HSETNX', KEYS[5], coll, cjson.encode({cover=cover}))
	)__";
	return redis::CommandString{
		"EVAL %s 5 %b %b %b %b %b   %b %b %b %b %b", lua,

		blob_ref.data(), blob_ref.size(),
		blob_owner.data(), blob_owner.size(),
		blob_meta.data(), blob_meta.size(),
		coll_key.data(), coll_key.size(),
		coll_list.data(), coll_list.size(),

		m_user.data(), m_user.size(),       // ARGV[1]: user name
		coll.data(), coll.size(),           // ARGV[2]: collection name
		blob.data(), blob.size(),           // ARGV[3]: blob
		hex.data(), hex.size(),             // ARGV[4]: blob in hex string
		entry.data(), entry.size()          // ARGV[5]: blob entry
	};
}

redis::CommandString Ownership::unlink_command(std::string_view coll, const ObjectID& blob) const
{
	auto blob_ref   = key::blob_refs(m_user, blob);
	auto blob_owner = key::blob_owners(blob);
	auto blob_meta  = key::blob_meta(m_user, blob);
	auto coll_key   = key::collection(m_user, coll);
	auto coll_list  = key::collection_list(m_user);
	auto public_blobs = key::public_blobs();

	static const char lua[] = R"__(
		-- convert binary to lowercase hex string
		local tohex = function(str)
			return (str:gsub('.', function (c)
				return string.format('%02x', string.byte(c))
			end))
		end

		local blob_ref, blob_owner, blob_meta, coll_set, coll_list, pub_list = KEYS[1], KEYS[2], KEYS[3], KEYS[4], KEYS[5], KEYS[6]
		local user, coll, blob = ARGV[1], ARGV[2], ARGV[3]

		-- delete the link from blob-refs
		redis.call('SREM', blob_ref, coll)

		-- if there is no more links to this blob to other collections, we can remove the blob
		-- for this user and remove the blob from the public list
		if redis.call('EXISTS', blob_ref) == 0 then
			redis.call('SREM', blob_owner, user)
			redis.call('DEL',  blob_meta, blob)
			redis.call('LREM', KEYS[4], 0, cmsgpack.pack(user, blob))
		end

		-- delete the blob in the collection set
		redis.call('SREM', KEYS[2], blob)

		-- if the collection has no more entries, delete the collection in the
		-- user's list of collections
		if redis.call('EXISTS', KEYS[2]) == 0 then
			redis.call('HDEL', KEYS[3], coll)

		-- if the collection still exists, check if the blob we are removing
		-- is the cover of the collection
		else
			local album = cjson.decode(redis.call('HGET', KEYS[3], coll))

			-- tohex() return upper case, so need to convert album[cover] to upper
			-- case before comparing
			if album['cover'] == tohex(blob) then
				album['cover'] = tohex(redis.call('SMEMBERS', KEYS[2])[1])
				redis.call('HSET', KEYS[3], coll, cjson.encode(album))
			end
		end
	)__";
	return redis::CommandString{
		"EVAL %s 6 %b %b %b %b %b %b   %b %b %b", lua,

		blob_ref.data(), blob_ref.size(),
		blob_owner.data(), blob_owner.size(),
		blob_meta.data(), blob_meta.size(),
		coll_key.data(), coll_key.size(),
		coll_list.data(), coll_list.size(),
		public_blobs.data(), public_blobs.size(),

		m_user.data(), m_user.size(),       // ARGV[1]: user name
		coll.data(), coll.size(),           // ARGV[2]: collection name
		blob.data(), blob.size()            // ARGV[3]: blob
	};
}

redis::CommandString Ownership::scan_collection_command(std::string_view coll) const
{
	auto coll_key  = key::collection(m_user, coll);
	auto coll_list = key::collection_list(m_user);
	static const char lua[] = R"__(
		local dirs = {}
		for k, blob in ipairs(redis.call('SMEMBERS', KEYS[1])) do
			table.insert(dirs, blob)
			table.insert(dirs, redis.call('GET', "blob-meta:" .. ARGV[2] .. ':' .. blob))
		end
		return {dirs, redis.call('HGET', KEYS[2], ARGV[1])}
	)__";
	return redis::CommandString{
		"EVAL %s 2 %b %b   %b %b", lua,
		coll_key.data(), coll_key.size(),
		coll_list.data(), coll_list.size(),

		coll.data(), coll.size(),
		m_user.data(), m_user.size()
	};
}

} // end of namespace hrb
