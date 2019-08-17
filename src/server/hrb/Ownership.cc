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

		CollEntryDB entry{perm.as_string()};
		auto blob_id = ObjectID::from_raw(blob);

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
	auto blob_meta  = key::blob_meta(m_user);
	db.command(
		"HSET %b %b %b",
		blob_meta.data(), blob_meta.size(),
		blob.data(), blob.size(),
		entry.data(), entry.size()
	);
}

redis::CommandString Ownership::link_command(std::string_view coll, const ObjectID& blob, std::optional<CollEntry> coll_entry) const
{
	auto blob_ref   = key::blob_refs(m_user, blob);
	auto blob_owner = key::blob_owners(blob);
	auto blob_meta  = key::blob_meta(m_user);
	auto coll_key   = key::collection(m_user, coll);
	auto coll_list  = key::collection_list(m_user);
	auto hex = to_hex(blob);
	auto entry = coll_entry ? CollEntryDB::create(*coll_entry) : "";

	static const char lua[] = R"__(
		local blob_ref, blob_owner, blob_meta, coll_key, coll_list = KEYS[1], KEYS[2], KEYS[3], KEYS[4], KEYS[5]
		local user, coll, blob, cover, entry = ARGV[1], ARGV[2], ARGV[3], ARGV[4], ARGV[5]

		redis.call('SADD',   blob_ref,   coll)
		redis.call('SADD',   blob_owner, user)

		if entry ~= nil and entry ~= '' then
			redis.call('HSETNX', blob_meta,  blob, entry)
		end

		redis.call('SADD',   coll_key,   blob)
		redis.call('HSETNX', coll_list, coll, cjson.encode({cover=cover}))
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
	auto blob_meta  = key::blob_meta(m_user);
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
			redis.call('HDEL', blob_meta, blob)
			redis.call('LREM', pub_list, 0, cmsgpack.pack(user, blob))
		end

		-- delete the blob in the collection set
		redis.call('SREM', coll_set, blob)

		-- if the collection has no more entries, delete the collection in the
		-- user's list of collections
		if redis.call('EXISTS', coll_set) == 0 then
			redis.call('HDEL', coll_list, coll)

		-- if the collection still exists, check if the blob we are removing
		-- is the cover of the collection
		else
			local album = cjson.decode(redis.call('HGET', coll_list, coll))

			-- The intent here is to select a random image as the cover
			-- as the original cover is removed.
			-- However, we can't use SRANDMEMBER to select a random image
			-- in the album because it is not deterministic, and non-deter-
			-- ministic commands may break replication. We have no choice
			-- but to use the slower SMEMBERS and take the first element.
			if album['cover'] == tohex(blob) then
				album['cover'] = tohex(redis.call('SMEMBERS', coll_set)[1])
				redis.call('HSET', coll_list, coll, cjson.encode(album))
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
	auto coll_set  = key::collection(m_user, coll);
	auto coll_list = key::collection_list(m_user);
	auto blob_meta = key::blob_meta(m_user);

	static const char lua[] = R"__(
		local coll_set, coll_list, blob_meta = KEYS[1], KEYS[2], KEYS[3]
		local coll = ARGV[1]
		local dirs = {}
		for k, blob in ipairs(redis.call('SMEMBERS', coll_set)) do
			table.insert(dirs, blob)
			table.insert(dirs, redis.call('HGET', blob_meta, blob))
		end
		return {dirs, redis.call('HGET', coll_list, coll)}
	)__";
	return redis::CommandString{
		"EVAL %s 3 %b %b %b   %b", lua,
		coll_set.data(), coll_set.size(),
		coll_list.data(), coll_list.size(),
		blob_meta.data(), blob_meta.size(),

		coll.data(), coll.size()
	};
}

redis::CommandString Ownership::set_permission_command(
	const ObjectID& blobid,
	const Permission& perm
) const
{
	auto blob_meta = key::blob_meta(m_user);
	auto pub_list  = key::public_blobs();

	static const char lua[] = R"__(
		local user, blob, perm = ARGV[1], ARGV[2], ARGV[3]

		local original = redis.call('HGET', KEYS[2], blob)
		local updated  = perm .. string.sub(original, 2, -1)
		redis.call('HSET', KEYS[2], blob, updated)

		local msgpack = cmsgpack.pack(user, blob)
		if perm == '*' then
			redis.call('LREM', KEYS[1], 0, msgpack)
			if redis.call('RPUSH', KEYS[1], msgpack) > 100 then
				redis.call('LPOP', KEYS[1])
			end
		else
			redis.call('LREM', KEYS[1], 0, msgpack)
		end
	)__";
	return redis::CommandString{
		"EVAL %s 2 %b %b  %b %b %b", lua,
		pub_list.data(), pub_list.size(),   // KEYS[1]: list of public blob IDs
		blob_meta.data(), blob_meta.size(), // KEYS[2]: blob meta

		m_user.data(), m_user.size(),       // ARGV[1]: user
		blobid.data(), blobid.size(),       // ARGV[2]: blob ID
		perm.data(), perm.size()            // ARGV[3]: permission string
	};
}

redis::CommandString Ownership::move_blob_command(std::string_view src, std::string_view dest, const ObjectID& blobid) const
{
	auto coll_list = key::collection_list(m_user);
	auto src_coll  = key::collection(m_user, src);
	auto dest_coll = key::collection(m_user, dest);
	auto blob_refs = key::blob_refs(m_user, blobid);

	static const char lua[] = R"__(
		local src_coll, dest_coll, blob_refs, coll_list = KEYS[1], KEYS[2], KEYS[3], KEYS[4]
		local src, dest, blob = ARGV[1], ARGV[2], ARGV[3]

		redis.call('SREM', blob_refs, src)
		redis.call('SADD', blob_refs, dest)

		redis.call('SREM', src_coll,  blob)
		redis.call('SADD', dest_coll, blob)
	)__";
	return redis::CommandString{
		"EVAL %s 4 %b %b %b %b    %b %b %b", lua,

		src_coll.data(), src_coll.size(),
		dest_coll.data(), dest_coll.size(),
		blob_refs.data(), blob_refs.size(),
		coll_list.data(), coll_list.size(),

		src.data(), src.size(),
		dest.data(), dest.size(),
		blobid.data(), blobid.size()
	};
}

redis::CommandString Ownership::set_cover_command(std::string_view coll, const ObjectID& cover) const
{
	auto coll_list = key::collection_list(m_user);
	auto coll_set  = key::collection(m_user, coll);
	auto cover_hex = to_hex(cover);

	// set the cover of the collection
	// only set the cover if the collection is already in the dirs:<user> hash
	// and only if the cover blob is already in the collection (i.e. dir:<user>:<collection> hash)
	static const char lua[] = R"__(
		local coll_list, coll_set = KEYS[1], KEYS[2]
		local coll, cover_hex, cover = ARGV[1], ARGV[2], ARGV[3]

		local json    = redis.call('HGET', coll_list, coll)
		local in_coll = redis.call('SISMEMBER', coll_set, cover)
		if json and in_coll == 1 then
			local album = cjson.decode(json)
			album['cover'] = cover_hex
			redis.call('HSET', coll_list, coll, cjson.encode(album))

			return 1
		else
			return 0
		end
	)__";
	return redis::CommandString{
	"EVAL %s 2 %b %b    %b %b %b", lua,
		coll_list.data(), coll_list.size(),
		coll_set.data(), coll_set.size(),

		coll.data(), coll.size(),
		cover_hex.data(), cover_hex.size(),
		cover.data(), cover.size()
	};
}

redis::CommandString Ownership::list_public_blob_command() const
{
		static const char lua[] = R"__(
		local elements = {}
		local pub_list = redis.call('LRANGE', KEYS[1], 0, -1)
		for i, msgpack in ipairs(pub_list) do
			local user, blob = cmsgpack.unpack(msgpack)
			if user and blob then
				table.insert(elements, {
					user, blob,
					redis.call('SRANDMEMBER', 'blob-refs:' .. user .. ':' .. blob),
					redis.call('HGET', 'blob-meta:' .. user, blob)
				})
			end
		end
		return elements
	)__";
	return redis::CommandString{
		"EVAL %s 1 %b",
		lua,
		key::public_blobs().data(), key::public_blobs().size()
	};
}

} // end of namespace hrb
