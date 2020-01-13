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
#include "RedisKeys.hh"

#include "util/Escape.hh"

#include <nlohmann/json.hpp>

namespace hrb {

// Request by owner
Ownership::Ownership(UserID owner) : m_user{owner.username()}, m_requester{std::move(owner)}
{
}

Ownership::Ownership(std::string_view name, UserID requester) : m_user{name}, m_requester{std::move(requester)}
{
}

Collection Ownership::from_reply(
	const redis::Reply& reply,
	std::string_view coll,
	nlohmann::json&& meta
) const
{
	assert(meta.is_object());
	assert(reply.array_size() % 3 == 0);

	hrb::Collection result{coll, m_user, std::move(meta)};

	for (auto i = 0U; i+2 < reply.array_size(); i += 3)
	{
		auto blob = reply[i].as_string();
		auto perm = reply[i+1];
		auto filename = reply[i+2].as_string();

		if (perm.as_string().empty())
			continue;

		BlobDBEntry entry{perm.as_string()};
		if (auto blob_id = ObjectID::from_raw(blob); blob_id && can_access(entry.permission()))
		{
			if (auto fields = entry.fields(); fields.has_value())
			{
				// Overwrite filename field with the filename in directory.
				// The filename in the BlobDBEntry of owner:blob_meta is the original filename of the blob
				// when it was uploaded. The actual filename in the collection may be different
				if (!filename.empty())
					fields->filename = filename;

				result.add_blob(*blob_id, *fields);
			}
		}
	};
	return result;
}

void Ownership::update_blob(redis::Connection& db, const ObjectID& blobid, const BlobInode& entry)
{
	// assume the blob is already in the collection, so there is no need to update
	// blob backlink
	auto s = BlobDBEntry::create(entry);
	update(db, blobid, BlobDBEntry{s});
}

void Ownership::update_blob(redis::Connection& db, const ObjectID& blobid, const nlohmann::json& entry)
{
	// assume the blob is already in the collection, so there is no need to update
	// blob backlink
	auto en_str = BlobDBEntry::create(Permission::from_description(entry["perm"].get<std::string>()), entry);
	update(db, blobid, BlobDBEntry{en_str});
}

void Ownership::update(redis::Connection& db, const ObjectID& blob, const BlobDBEntry& entry)
{
	auto blob_meta  = key::blob_inode(m_user);
	db.command(
		"HSET %b %b %b",
		blob_meta.data(), blob_meta.size(),
		blob.data(), blob.size(),
		entry.data(), entry.size()
	);
}

redis::CommandString Ownership::link_command(std::string_view coll, const ObjectID& blob, const BlobInode& coll_entry) const
{
	auto blob_ref   = key::blob_refs(m_user, blob);
	auto blob_owner = key::blob_owners(blob);
	auto blob_meta  = key::blob_inode(m_user);
	auto coll_key   = key::collection(m_user, coll);
	auto coll_list  = key::collection_list(m_user);
	auto hex = to_hex(blob);
	auto entry = BlobDBEntry::create(coll_entry);

	auto filename = coll_entry.filename.empty() ? "hello" : coll_entry.filename;

	static const char lua[] = R"__(
		local blob_ref, blob_owner, blob_meta, coll_key, coll_list = KEYS[1], KEYS[2], KEYS[3], KEYS[4], KEYS[5]
		local user, coll, blob, cover, entry, filename = ARGV[1], ARGV[2], ARGV[3], ARGV[4], ARGV[5], ARGV[6]

		redis.call('SADD',   blob_ref,   coll)
		redis.call('SADD',   blob_owner, user)

		if entry ~= nil and entry ~= '' then
			redis.call('HSETNX', blob_meta,  blob, entry)
		end

		redis.call('HSET',   coll_key,  blob, filename)
		redis.call('HSETNX', coll_list, coll, cjson.encode({cover=cover}))
	)__";
	return redis::CommandString{
		"EVAL %s 5 %b %b %b %b %b   %b %b %b %b %b %b", lua,

		blob_ref.data(), blob_ref.size(),
		blob_owner.data(), blob_owner.size(),
		blob_meta.data(), blob_meta.size(),
		coll_key.data(), coll_key.size(),
		coll_list.data(), coll_list.size(),

		m_user.data(), m_user.size(),       // ARGV[1]: user name
		coll.data(), coll.size(),           // ARGV[2]: collection name
		blob.data(), blob.size(),           // ARGV[3]: blob
		hex.data(), hex.size(),             // ARGV[4]: blob in hex string
		entry.data(), entry.size(),         // ARGV[5]: blob entry
		filename.data(), filename.size()    // ARGV[6]: filename
	};
}

redis::CommandString Ownership::move_command(std::string_view src, std::string_view dest, const ObjectID& blob) const
{
	auto blob_ref   = key::blob_refs(m_user, blob);
	auto src_key    = key::collection(m_user, src);
	auto dest_key   = key::collection(m_user, dest);
	auto coll_list  = key::collection_list(m_user);
	auto hex = to_hex(blob);

	static const char lua[] = R"__(
		local blob_ref, src_key, dest_key, coll_list = KEYS[1], KEYS[2], KEYS[3], KEYS[4]
		local src_coll, dest_coll, blob, cover = ARGV[1], ARGV[2], ARGV[3], ARGV[4]

		redis.call('SADD',   blob_ref,   dest_coll)
		redis.call('SREM',   blob_ref,   src_coll)

		local filename = redis.call('HGET', src_key, blob)

		redis.call('HSET',   dest_key,   blob,  filename)
		redis.call('HDEL',   src_key,    blob)

		redis.call('HSETNX', coll_list, dest_coll, cjson.encode({cover=cover}))
	)__";
	return redis::CommandString{
		"EVAL %s 4 %b %b %b %b   %b %b %b %b", lua,

		blob_ref.data(), blob_ref.size(),
		src_key.data(), src_key.size(),
		dest_key.data(), dest_key.size(),
		coll_list.data(), coll_list.size(),

		src.data(), src.size(),             // ARGV[1]: source collection name
		dest.data(), dest.size(),           // ARGV[2]: destination collection name
		blob.data(), blob.size(),           // ARGV[3]: blob ID
		hex.data(), hex.size(),             // ARGV[4]: blob ID in hex string (for setting cover)
	};
}

redis::CommandString Ownership::unlink_command(std::string_view coll, const ObjectID& blob) const
{
	auto blob_ref   = key::blob_refs(m_user, blob);
	auto blob_owner = key::blob_owners(blob);
	auto blob_meta  = key::blob_inode(m_user);
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

		local blob_ref, blob_owner, blob_meta, coll_hash, coll_list, pub_list = KEYS[1], KEYS[2], KEYS[3], KEYS[4], KEYS[5], KEYS[6]
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

		-- delete the blob in the collection hash
		redis.call('HDEL', coll_hash, blob)

		-- if the collection has no more entries, delete the collection in the
		-- user's list of collections
		if redis.call('EXISTS', coll_hash) == 0 then
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
				album['cover'] = tohex(redis.call('HKEYS', coll_hash)[1])
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
	auto coll_hash = key::collection(m_user, coll);
	auto coll_list = key::collection_list(m_user);
	auto blob_meta = key::blob_inode(m_user);

	static const char lua[] = R"__(
		local coll_hash, coll_list, blob_meta = KEYS[1], KEYS[2], KEYS[3]
		local coll = ARGV[1]
		local dirs = {}
		for i, v in ipairs(redis.call('HGETALL', coll_hash)) do
			if i % 2 == 1 then
				-- v is the blob ID
				table.insert(dirs, v)
				table.insert(dirs, redis.call('HGET', blob_meta, v))
			else
				-- v is the filename
				table.insert(dirs, v)
			end
		end
		return {dirs, redis.call('HGET', coll_list, coll)}
	)__";
	return redis::CommandString{
		"EVAL %s 3 %b %b %b   %b", lua,
		coll_hash.data(), coll_hash.size(),
		coll_list.data(), coll_list.size(),
		blob_meta.data(), blob_meta.size(),

		coll.data(), coll.size()
	};
}

redis::CommandString Ownership::set_permission_command(const ObjectID& blobid, Permission perm) const
{
	auto blob_meta = key::blob_inode(m_user);
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
		blob_meta.data(), blob_meta.size(), // KEYS[2]: blob inode

		m_user.data(), m_user.size(),       // ARGV[1]: user
		blobid.data(), blobid.size(),       // ARGV[2]: blob ID
		perm.data(), perm.size()            // ARGV[3]: permission string
	};
}

redis::CommandString Ownership::set_cover_command(std::string_view coll, const ObjectID& cover) const
{
	auto coll_list = key::collection_list(m_user);
	auto coll_hash = key::collection(m_user, coll);
	auto cover_hex = to_hex(cover);

	// set the cover of the collection
	// only set the cover if the collection is already in the dirs:<user> hash
	// and only if the cover blob is already in the collection (i.e. dir:<user>:<collection> hash)
	static const char lua[] = R"__(
		local coll_list, coll_hash = KEYS[1], KEYS[2]
		local coll, cover_hex, cover = ARGV[1], ARGV[2], ARGV[3]

		local json    = redis.call('HGET', coll_list, coll)
		local in_coll = redis.call('HEXISTS', coll_hash, cover)
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
		coll_hash.data(), coll_hash.size(),

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
					redis.call('HGET', 'blob-inodes:' .. user, blob)
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

bool Ownership::can_access(Permission perm) const
{
	// Don't swap these two!
	return perm.allow(m_requester, m_user);
}

} // end of namespace hrb
