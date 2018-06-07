/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/12/18.
//

#include "Ownership.hh"
#include "BlobRef.hh"
#include "CollEntryDB.hh"

#include "common/Permission.hh"
#include "common/Collection.hh"

#include "crypto/Authentication.hh"
#include "common/Error.hh"
#include "util/Log.hh"
#include "common/Escape.hh"

#include <json.hpp>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <vector>

#pragma once

namespace hrb {

class Ownership::BlobBackLink
{
public:
	static const std::string_view m_prefix;

public:
	BlobBackLink(std::string_view user, std::string_view coll, const ObjectID& blob);

	// expect to be done inside a transaction
	void link(redis::Connection& db) const;
	void unlink(redis::Connection& db) const;

	std::string_view user() const {return m_user;}
	const ObjectID& blob() const {return m_blob;}
	std::string_view collection() const {return m_coll;}

private:
	std::string m_user;
	std::string m_coll;
	ObjectID    m_blob;
};

/// A set of blob objects represented by a redis set.
class Ownership::Collection
{
public:
	static const std::string_view m_dir_prefix;
	static const std::string_view m_list_prefix;
	static const std::string_view m_public_blobs;

public:
	Collection(std::string_view user, std::string_view path);
	explicit Collection(std::string_view redis_key);

	void link(redis::Connection& db, const ObjectID& id, const CollEntryDB& entry);
	void unlink(redis::Connection& db, const ObjectID& id);
	std::string redis_key() const;
	void update(redis::Connection& db, const ObjectID& id, const CollEntryDB& entry);
	void update(redis::Connection& db, const ObjectID& id, const nlohmann::json& entry);

	template <typename Complete>
	void set_cover(redis::Connection& db, const ObjectID& cover, Complete&& complete);

	template <typename Complete>
	void set_permission(redis::Connection& db, const ObjectID& blob, const Permission& perm, Complete&& complete) const;

	template <typename Complete>
	void find(redis::Connection& db, const ObjectID& blob, Complete&& complete) const;

	template <typename Complete>
	void list(
		redis::Connection& db,
		Complete&& complete
	) const;

	template <typename Complete>
	void serialize(
		redis::Connection& db,
		const Authentication& requester,
		Complete&& complete
	) const;

	template <typename CollectionCallback, typename Complete>
	static void scan(redis::Connection& db, std::string_view user, long cursor,
		CollectionCallback&& callback,
		Complete&& complete
	);

	template <typename Complete>
	static void scan_all(redis::Connection& db, std::string_view owner, Complete&& complete);

	const std::string& user() const {return m_user;}
	const std::string& path() const {return m_path;}

	hrb::Collection serialize(
		const redis::Reply& hash_getall_reply,
		const Authentication& requester,
		nlohmann::json&& meta
	) const;

	void post_unlink(redis::Connection& db, const ObjectID& blob);

private:
	std::string m_user;
	std::string m_path;
};

template <typename CollectionCallback, typename Complete>
void Ownership::Collection::scan(
	redis::Connection& db,
	std::string_view user,
	long cursor,
	CollectionCallback&& callback,
	Complete&& complete
)
{
	db.command(
		[
			comp=std::forward<Complete>(complete),
			cb=std::forward<CollectionCallback>(callback),
			&db, user=std::string{user}
		](redis::Reply&& reply, std::error_code&& ec) mutable
		{
			if (!ec)
			{
				auto [cursor_reply, dirs] = reply.as_tuple<2>(ec);
				if (!ec)
				{
					// Repeat scanning only when the cycle is not completed yet (i.e.
					// cursor != 0), and the completion callback return true.
					auto cursor = cursor_reply.to_int();

					// call the callback once to handle one collection
					for (auto&& p : dirs.kv_pairs())
					{
						auto sv = p.value().as_string();
						if (sv.empty())
							return;

						cb(p.key(), nlohmann::json::parse(sv));
					};

					// if comp return true, keep scanning with the same callback and
					// comp as completion routine
					if (comp(cursor, ec) && cursor != 0)
						scan(db, user, cursor, std::move(cb), std::move(comp));
					return;
				}
			}

			comp(0, ec);
		},
		"HSCAN %b%b %d",
		m_list_prefix.data(), m_list_prefix.size(),
		user.data(), user.size(),
		cursor
	);
}

template <typename Complete>
void Ownership::Collection::scan_all(
	redis::Connection& db,
	std::string_view owner,
	Complete&& complete
)
{
	auto jdoc = std::make_shared<nlohmann::json>();
	jdoc->emplace("owner",    std::string{owner});
	jdoc->emplace("colls",    nlohmann::json::object());

	scan(db, owner, 0,
		[&colls=(*jdoc)["colls"]](auto coll, auto&& json)
		{
			colls.emplace(coll, std::move(json));
		},
		[jdoc, comp=std::forward<Complete>(complete)](long cursor, auto ec)
		{
			if (cursor == 0)
				comp(std::move(*jdoc), ec);
			return true;
		}
	);
}

template <typename Complete>
void Ownership::Collection::find(
	redis::Connection& db,
	const ObjectID& blob,
	Complete&& complete
) const
{
	db.command(
		[
			comp=std::forward<Complete>(complete)
		](auto&& entry, std::error_code&& ec) mutable
		{
			if (!ec && entry.is_nil())
				ec = Error::object_not_exist;

			comp(
				CollEntryDB{entry.as_string()},
				std::move(ec)
			);
		},

		"HGET %b%b:%b %b",
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		blob.data(), blob.size()
	);
}


template <typename Complete>
void Ownership::Collection::set_permission(
	redis::Connection& db,
	const ObjectID& blob,
	const Permission& perm,
	Complete&& complete
) const
{
	static const char lua[] = R"__(
		local user, coll, blob, perm = ARGV[1], ARGV[2], ARGV[3], ARGV[4]
		local coll_key = 'dir:' .. user .. ':' .. coll

		local original = redis.call('HGET', coll_key, blob)
		local updated  = perm .. string.sub(original, 2, -1)
		redis.call('HSET', coll_key, blob, updated)

		local msgpack = cmsgpack.pack(user, coll, blob)
		if perm == '*' then
			redis.call('LREM', KEYS[1], 0, msgpack)
			if redis.call('RPUSH', KEYS[1], msgpack) > 100 then
				redis.call('LPOP', KEYS[1])
			end
		else
			redis.call('LREM', KEYS[1], 0, msgpack)
		end
	)__";
	db.command(
		[comp=std::forward<Complete>(complete)](auto&& reply, auto&& ec) mutable
		{
			if (!reply)
				Log(LOG_WARNING, "Collection::set_permission(): script error: %1%", reply.as_error());
			comp(std::move(ec));
		},
		"EVAL %s 1 %b %b %b %b %b", lua,

		// KEYS[1]: list of public blob IDs
		m_public_blobs.data(), m_public_blobs.size(),

		// ARGV[1]: user
		m_user.data(), m_user.size(),

		// ARGV[2]: collection
		m_path.data(), m_path.size(),

		// ARGV[3]: blob ID
		blob.data(), blob.size(),

		// ARGV[4]: permission string
		perm.data(), perm.size()
	);
}

template <typename Complete>
void Ownership::Collection::set_cover(redis::Connection& db, const ObjectID& cover, Complete&& complete)
{
	// set the cover of the collection
	// only set the cover if the collection is already in the dirs:<user> hash
	// and only if the cover blob is already in the collection (i.e. dir:<user>:<collection> hash)
	auto hex_id = to_hex(cover);
	static const char lua[] = R"__(
		local coll, blob_hex, blob = ARGV[1], ARGV[2], ARGV[3]
		local json  = redis.call('HGET', KEYS[1], coll)
		local entry = redis.call('HGET', KEYS[2], blob)
		if json and entry then
			local album = cjson.decode(json)
			album['cover'] = blob_hex
			redis.call('HSET', KEYS[1], coll, cjson.encode(album))

			return 1
		else
			return 0
		end
	)__";
	db.command(
		[comp=std::forward<Complete>(complete)](auto&& reply, auto ec)
		{
			if (!reply || ec)
				Log(LOG_WARNING, "set_cover(): reply %1% %2%", reply.as_error(), ec);

			comp(reply.as_int() == 1, ec);
		},
		"EVAL %s 2 %b%b %b%b:%b %b %b %b", lua,

		// KEYS[1]: dirs:<user>
		m_list_prefix.data(), m_list_prefix.size(),
		m_user.data(), m_user.size(),

		// KEYS[2]: dir:<user>:<collection>
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),

		m_path.data(), m_path.size(),
		hex_id.data(), hex_id.size(),
		cover.data(), cover.size()
	);
}

template <typename Complete>
void Ownership::Collection::list(
	redis::Connection& db,
	Complete&& complete
) const
{
	db.command(
		[
			comp=std::forward<Complete>(complete)
		](auto&& reply, std::error_code&& ec) mutable
		{
			if (!reply || ec)
				Log(LOG_WARNING, "list() reply %1% %2%", reply.as_error(), ec);

			using namespace boost::adaptors;
			comp(
				reply |
				transformed([](auto&& reply){return ObjectID::from_raw(reply.as_string());}) |
				filtered([](auto&& opt_oid){return opt_oid.has_value();}) |
				transformed([](auto&& opt_oid){return *opt_oid;}),
				ec
			);
		},
		"HKEYS %b%b:%b",
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size()
	);
}

template <typename Complete>
void Ownership::Collection::serialize(
	redis::Connection& db,
	const Authentication& requester,
	Complete&& complete
) const
{
	static const char lua[] = R"__(
		return {
			redis.call('HGETALL', KEYS[1]),
			redis.call('HGET', KEYS[2], ARGV[1])
		}
	)__";
	db.command(
		[
			comp=std::forward<Complete>(complete),
			*this, requester, path=m_path
		](auto&& reply, std::error_code&& ec) mutable
		{
			if (!reply || ec)
				Log(LOG_WARNING, "serialize() script reply %1% %2%", reply.as_error(), ec);

			if (reply.array_size() == 2)
			{
				// in some error cases created by unit tests, the string is not valid JSON
				// in the database
				auto meta = nlohmann::json::parse(reply[1].as_string(), nullptr, false);
				if (meta.is_discarded())
					meta = nlohmann::json::object();

				comp(serialize(reply[0], requester, std::move(meta)), std::move(ec));
			}

			// TODO: handle case where
		},
		"EVAL %s 2 %b%b:%b %b%b %b", lua,
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),

		m_list_prefix.data(), m_list_prefix.size(),
		m_user.data(), m_user.size(),

		m_path.data(), m_path.size()
	);
}

template <typename Complete>
void Ownership::link(
	redis::Connection& db,
	std::string_view coll_name,
	const ObjectID& blobid,
	const CollEntry& entry,
	Complete&& complete
)
{
	BlobBackLink blob{m_user, coll_name, blobid};
	Collection   coll{m_user, coll_name};

	auto en_str = CollEntryDB::create(entry);

	db.command("MULTI");
	blob.link(db);
	coll.link(db, blob.blob(), CollEntryDB{en_str});
	db.command([comp=std::forward<Complete>(complete)](auto&&, std::error_code ec)
	{
		comp(ec);
	}, "EXEC");
}

template <typename Complete>
void Ownership::unlink(
	redis::Connection& db,
	std::string_view coll_name,
	const ObjectID& blobid,
	Complete&& complete
)
{
	BlobBackLink  blob{m_user, coll_name, blobid};
	Collection coll{m_user, coll_name};

	db.command("MULTI");
	blob.unlink(db);
	coll.unlink(db, blob.blob());
	db.command(
		"LREM %b 1 %b",
		Collection::m_public_blobs.data(), Collection::m_public_blobs.size(),
		blobid.data(), blobid.size()
	);
	db.command([comp=std::forward<Complete>(complete)](auto&& reply, std::error_code ec)
	{
		assert(reply.array_size() == 3);
		comp(ec);
	}, "EXEC");

	coll.post_unlink(db, blob.blob());
}

template <typename Complete>
void Ownership::serialize(
	redis::Connection& db,
	const Authentication& requester,
	std::string_view coll,
	Complete&& complete
) const
{
	return Collection{m_user, coll}.serialize(db, requester, std::forward<Complete>(complete));
}

template <typename Complete>
void Ownership::list(
	redis::Connection& db,
	std::string_view coll,
	Complete&& complete
) const
{
	return Collection{m_user, coll}.list(db, std::forward<Complete>(complete));
}

template <typename Complete>
void Ownership::find(
	redis::Connection& db,
	std::string_view coll,
	const ObjectID& blob,
	Complete&& complete
) const
{
	return Collection{m_user, coll}.find(db, blob, std::forward<Complete>(complete));
}

template <typename CollectionCallback, typename Complete>
void Ownership::scan_collections(
	redis::Connection& db,
	long cursor,
	CollectionCallback&& callback,
	Complete&& complete
) const
{
	return Collection::scan(db, m_user, cursor,
		std::forward<CollectionCallback>(callback),
		std::forward<Complete>(complete)
	);
}

template <typename Complete>
void Ownership::scan_all_collections(
	redis::Connection& db,
	Complete&& complete
) const
{
	return Collection::scan_all(db, m_user, std::forward<Complete>(complete));
}

template <typename Complete>
void Ownership::set_permission(
	redis::Connection& db,
	std::string_view coll,
	const ObjectID& blobid,
	const Permission& perm,
	Complete&& complete
)
{
	return Collection{m_user, coll}.set_permission(db, blobid, perm, std::forward<Complete>(complete));
}

template <typename Complete>
void Ownership::move_blob(
	redis::Connection& db,
	std::string_view src_coll,
	std::string_view dest_coll,
	const ObjectID& blobid,
	Complete&& complete
)
{
	find(db, src_coll, blobid,
		[
			blobid,
			db=db.shared_from_this(),
			dest=Collection{m_user, dest_coll},
			src=Collection{m_user, src_coll},
			comp=std::forward<Complete>(complete)
		](CollEntryDB&& entry, auto ec) mutable
		{
			if (!ec)
			{
				db->command("MULTI");
				dest.link(*db, blobid, entry);
				src.unlink(*db, blobid);
				db->command([comp=std::move(comp)](auto&&, auto ec)
				{
					comp(ec);
				}, "EXEC");
			}
		}
	);
}

template <typename Complete>
void Ownership::list_public_blobs(
	redis::Connection& db,
	Complete&& complete
)
{
	static const char lua[] = R"__(
		local elements = {}
		local pub_list = redis.call('LRANGE', KEYS[1], 0, -1)
		for i, msgpack in ipairs(pub_list) do
			local user, coll, blob = cmsgpack.unpack(msgpack)
			if user and coll and blob then
				table.insert(elements, {user, coll, blob, redis.call('HGET', 'dir:' .. user .. ':' .. coll, blob)})
			end
		end
		return elements
	)__";

	db.command(
		[user=m_user, comp=std::forward<Complete>(complete)](auto&& reply, auto ec) mutable
		{
			if (!reply)
				Log(LOG_WARNING, "list_public_blobs() script return %1%", reply.as_error());

			using namespace boost::adaptors;
			comp(
				reply |
				transformed([](const redis::Reply& row)
				{
					std::error_code err;
					auto [owner, coll, blob, entry_str] = row.as_tuple<4>(err);

					std::optional<BlobRef> result;
					if (auto blob_id = ObjectID::from_raw(blob.as_string()); !err && blob_id.has_value())
						result = BlobRef{
							std::string{owner.as_string()},
							std::string{coll.as_string()},
							*blob_id,
							CollEntryDB{entry_str.as_string()}
	                    };

					return result;
				}) |
				filtered([](auto&& opt){return opt.has_value();}) |
				transformed([](auto&& opt){return *opt;}),
				ec
			);
		},
		"EVAL %s 1 %b",
		lua,
		Collection::m_public_blobs.data(), Collection::m_public_blobs.size()
	);
}


template <typename Complete>
void Ownership::query_blob(redis::Connection& db, const ObjectID& blob, Complete&& complete)
{
	static const char lua[] = R"__(
		local dirs = {}
		for k, mpack in ipairs(redis.call('SMEMBERS', KEYS[1])) do
			local dir, user, coll = cmsgpack.unpack(mpack)
			local coll_key = dir .. user .. ':' .. coll
			table.insert(dirs, coll_key)
			table.insert(dirs, redis.call('HGET', coll_key, ARGV[1]))
		end
		return dirs
	)__";

	db.command(
		[user=m_user, blob, comp=std::forward<Complete>(complete)](auto&& reply, auto ec)
		{
			if (!reply)
				Log(LOG_WARNING, "query_blob() script reply: %1%", reply.as_error());

			using namespace boost::adaptors;
			comp(reply.kv_pairs() |
				transformed([blob](auto&& kv)
				{
					Collection coll{kv.key()};
					return BlobRef{coll.user(), coll.path(), blob, CollEntryDB{kv.value().as_string()}};
				}) |
				filtered([&user](auto&& blob)
				{
					return user.empty() ? blob.entry.permission() == Permission::public_() : user == blob.user;
				}),
				ec
			);
		},
		"EVAL %s 1 %b%b %b",
		lua,
		BlobBackLink::m_prefix.data(), BlobBackLink::m_prefix.size(),
		blob.data(), blob.size(),
		blob.data(), blob.size()
	);
}

template <typename Complete>
void Ownership::set_cover(redis::Connection& db, std::string_view coll, const ObjectID& blob, Complete&& complete) const
{
	Collection{m_user, coll}.set_cover(db, blob, std::forward<Complete>(complete));
}

} // end of namespace
