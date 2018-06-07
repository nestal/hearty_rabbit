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
#include "common/Escape.hh"

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
	static const char lua[] = R"__(
		redis.call('SADD', KEYS[1], cmsgpack.pack(ARGV[1], ARGV[2], ARGV[3]))
	)__";
	db.command(
		"EVAL %s 1 %b%b %b %b %b", lua,
		m_prefix.data(), m_prefix.size(),
		m_blob.data(), m_blob.size(),

		Collection::m_dir_prefix.data(), Collection::m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_coll.data(), m_coll.size()
	);
}

void Ownership::BlobBackLink::unlink(redis::Connection& db) const
{
	static const char lua[] = R"__(
		redis.call('SREM', KEYS[1], cmsgpack.pack(ARGV[1], ARGV[2], ARGV[3]))
	)__";
	db.command(
		"EVAL %s 1 %b%b %b %b %b", lua,

		// KEYS[1]
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

void Ownership::Collection::link(redis::Connection& db, const ObjectID& id, const CollEntryDB& entry)
{
	auto hex = to_hex(id);

	static const char lua[] = R"__(
		local blob, entry, cover, coll = ARGV[1], ARGV[2], ARGV[3], ARGV[4]
		redis.call('HSET',   KEYS[1], blob, entry)
		redis.call('HSETNX', KEYS[2], coll, cjson.encode({cover=cover}))
	)__";
	db.command(
		[](auto&& reply, auto ec)
		{
			if (!reply || ec)
				Log(LOG_WARNING, "Collection::link() returns %1% %2%", reply.as_error(), ec);
		},
		"EVAL %s 2 %b%b:%b %b%b %b %b %b %b", lua,

		// KEYS[1]
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),

		// KEYS[2]
		m_list_prefix.data(), m_list_prefix.size(),
		m_user.data(), m_user.size(),

		id.data(), id.size(),        // ARGV[1]
		entry.data(), entry.size(),  // ARGV[2]
		hex.data(), hex.size(),      // ARGV[3]
		m_path.data(), m_path.size()
	);
}

void Ownership::Collection::unlink(redis::Connection& db, const ObjectID& id)
{
	// LUA script: delete the blob from the dir:<user>:<coll> hash table, and if
	// the hash table is empty, remove the entry in dirs:<user> hash table.
	// Also, remove the 'cover' field in the dirs:<user> hash table if the cover
	// image is the one being removed.
	static const char cmd[] = R"__(
		-- convert binary to lowercase hex string
		local tohex = function(str)
			return (str:gsub('.', function (c)
				return string.format('%02x', string.byte(c))
			end))
		end

		local blob, coll = ARGV[1], ARGV[2]

		-- delete the CollEntry in the collection hash
		redis.call('HDEL', KEYS[1], blob)

		-- if the collection has no more entries, delete the collection in the
		-- user's list of collections
		if redis.call('EXISTS', KEYS[1]) == 0 then
			redis.call('HDEL', KEYS[2], coll)

		-- if the collection still exists, check if the blob we are removing
		-- is the cover of the collection
		else
			local album = cjson.decode(redis.call('HGET', KEYS[2], coll))

			-- tohex() return upper case, so need to convert album[cover] to upper
			-- case before comparing
			if album['cover'] == tohex(blob) then
				album['cover'] = tohex(redis.call('HKEYS', KEYS[1])[1])
				redis.call('HSET', KEYS[2], coll, cjson.encode(album))
			end
		end
	)__";

	db.command(
		[](auto&& reply, auto ec)
		{
			if (!reply || ec)
				Log(LOG_WARNING, "Collection::unlink() lua script failure: %1% (%2%)", reply.as_error(), ec);
		},
		"EVAL %s 2 %b%b:%b %b%b %b %b",

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
		m_path.data(), m_path.size()
	);
}

hrb::Collection Ownership::Collection::serialize(const redis::Reply& reply, const Authentication& requester) const
{
	hrb::Collection result{m_path, m_user};

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
			auto entry_jdoc = nlohmann::json::parse(entry.json(), nullptr, false);
			if (!entry_jdoc.is_discarded())
				result.add_blob(*blob_id, entry.fields());
		}
	};
	return result;
}

/// \brief  Called after a blob is deleted.
/// This function will remove all references to the blob from all the indexes in the database
/// after a blob is unlinked. These references are weak references so they don't need to be
/// kept up-to-date at all times. Therefore it is not absolutely need to be updated atomically
/// at the same time as unlinking them from the collection.
void Ownership::Collection::post_unlink(redis::Connection& db, const ObjectID& blob)
{
	static const char lua[] = R"__(
		local user, coll, blob = ARGV[1], ARGV[2], ARGV[3]

		-- must check if the blob is really unlinked from the collection before
		-- removing the references, because it may be possible that the same blob
		-- is re-linked to the same collection again
		if redis.call('HEXISTS', KEYS[1], blob) == 1 then

			-- remove the blob from the public list
			redis.call('LREM', KEYS[2], 0, cmsgpack.pack(user, coll, blob))
		end
	)__";
	db.command(
		[](auto&& reply, auto ec)
		{
			if (!reply || ec)
				Log(LOG_WARNING, "Collection::unlink() lua script failure: %1% (%2%)", reply.as_error(), ec);
		},
		"EVAL %s 2 %b%b:%b %b %b %b %b",

		lua,

		// KEYS[1] (hash table that stores the blob in a collection)
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),

		// KEYS[2] (list that stores public blobs)
		m_public_blobs.data(), m_public_blobs.size(),
		m_user.data(), m_user.size(),

		// ARGV[1] (user name)
		m_user.data(), m_user.size(),

		// ARGV[2] (collection name)
		m_path.data(), m_path.size(),

		// ARGV[3] (blob ID)
		blob.data(), blob.size()
	);
}

void Ownership::update(
	redis::Connection& db,
	std::string_view coll,
	const ObjectID& blobid,
	const CollEntry& entry
)
{
	// assume the blob is already in the collection, so there is no need to update
	// blob backlink
	auto s = CollEntryDB::create(entry);
	Collection{m_user, coll}.update(db, blobid, CollEntryDB{s});
}

void Ownership::update(
	redis::Connection& db,
	std::string_view coll,
	const ObjectID& blobid,
	const nlohmann::json& entry
)
{
	// assume the blob is already in the collection, so there is no need to update
	// blob backlink
	Collection{m_user, coll}.update(db, blobid, entry);
}

void Ownership::Collection::update(
	redis::Connection& db,
	const ObjectID& id,
	const CollEntryDB& entry
)
{
	static const char hsetex[] = R"__(
		if redis.call('HEXISTS', KEYS[1], ARGV[1]) then
			redis.call('HSET', KEYS[1], ARGV[1], ARGV[2])
		end
	)__";
	db.command(
		[](auto&& reply, auto ec)
		{
			if (!reply || ec)
				Log(LOG_WARNING, "Collection::update() lua script failure: %1% (%2%)", reply.as_error(), ec);
		},
		"EVAL %s 1 %b%b:%b %b %b",
		hsetex,

		// KEYS[1] (hash table that stores the blob in a collection)
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),

		id.data(), id.size(), // ARGV[1]
		entry.data(),  entry.size()   // ARGV[2]
	);
}

void Ownership::Collection::update(
	redis::Connection& db,
	const ObjectID& id,
	const nlohmann::json& entry
)
{
	auto en_str = CollEntryDB::create(Permission::from_description(entry["perm"].get<std::string>()), entry);
	update(db, id, CollEntryDB{en_str});
}

} // end of namespace hrb
