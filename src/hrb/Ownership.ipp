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
#include "Permission.hh"
#include "CollEntry.hh"

#include "util/Error.hh"
#include "util/Log.hh"

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

	void link(redis::Connection& db, const ObjectID& id, const CollEntry& entry);
	void unlink(redis::Connection& db, const ObjectID& id);
	std::string redis_key() const;

	template <typename Complete>
	void set_cover(redis::Connection& db, const ObjectID& cover, Complete&& complete);

	template <typename Complete>
	void set_permission(redis::Connection& db, const ObjectID& blob, const Permission& perm, Complete&& complete) const;

	template <typename Complete>
	void find(redis::Connection& db, const ObjectID& blob, Complete&& complete) const;

	template <typename Complete>
	void serialize(
		redis::Connection& db,
		std::string_view requester,
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

private:
	nlohmann::json serialize(redis::Reply& reply, std::string_view requester) const;

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
				CollEntry{entry.as_string()},
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
		local original = redis.call('HGET', KEYS[1], ARGV[1])
		local updated  = ARGV[2] .. string.sub(original, 2, -1)
		redis.call('HSET', KEYS[1], ARGV[1], updated)
		if ARGV[2] == '*' then
			if redis.call('LPUSH', KEYS[2], ARGV[1]) > 100 then
				redis.call('RPOP', KEYS[2])
			end
		else
			redis.call('LREM', KEYS[2], 1, ARGV[1])
		end
	)__";
	db.command(
		[comp=std::forward<Complete>(complete)](auto&& reply, auto&& ec) mutable
		{
			if (!reply)
				Log(LOG_WARNING, "Collection::set_permission(): script error: %1%", reply.as_error());
			comp(std::move(ec));
		},
		"EVAL %s 2 %b%b:%b %b %b %b",
		lua,

		// KEYS[1]: dir:<user>:<collection>
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),

		// KEYS[2]: public list of blob references
		m_public_blobs.data(), m_public_blobs.size(),

		// ARGV[1]: blob ID
		blob.data(), blob.size(),

		// ARGV[2]: permission string
		perm.data(), perm.size()
	);
}

template <typename Complete>
void Ownership::Collection::set_cover(redis::Connection& db, const ObjectID& cover, Complete&& complete)
{
	// set the cover of the collection
	auto json = R"({"cover":)" + to_quoted_hex(cover) + "}";
	db.command(
		std::forward<Complete>(complete),
		R"(HSETNX %b%b %b %b)",
		m_list_prefix.data(), m_list_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		json.data(), json.size()
	);
}

template <typename Complete>
void Ownership::Collection::serialize(
	redis::Connection& db,
	std::string_view requester,
	Complete&& complete
) const
{
	db.command(
		[
			comp=std::forward<Complete>(complete), *this,
			requester=std::string{requester}
		](auto&& reply, std::error_code&& ec) mutable
		{
			comp(serialize(reply, requester), std::move(ec));
		},
		"HGETALL %b%b:%b",
		m_dir_prefix.data(), m_dir_prefix.size(),
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

	db.command("MULTI");
	blob.link(db);
	coll.link(db, blob.blob(), entry);
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
}

template <typename Complete>
void Ownership::serialize(
	redis::Connection& db,
	std::string_view requester,
	std::string_view coll,
	Complete&& complete
) const
{
	return Collection{m_user, coll}.serialize(db, requester, std::forward<Complete>(complete));
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
		](CollEntry&& entry, auto ec) mutable
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
void Ownership::find_reference(redis::Connection& db, const ObjectID& blob, Complete&& complete) const
{
	db.command(
		[comp=std::forward<Complete>(complete), blob, user=m_user](auto&& reply, auto ec)
		{
			for (auto&& entry : reply)
			{
				Collection ref{entry.as_string()};
				if (user == ref.user())
					comp(ref.path());
			}
		},
		"SMEMBERS %b:%b",
		BlobBackLink::m_prefix.data(), BlobBackLink::m_prefix.size(),
		blob.data(), blob.size()
	);
}

template <typename Complete>
void Ownership::list_public_blobs(
	redis::Connection& db,
	Complete&& complete
)
{
	db.command(
		[comp=std::forward<Complete>(complete)](auto&& reply, auto ec)
		{
			for (auto&& en : reply)
			{
				// redis::Reply can be used as a boost range because it has begin()/end() (i.e.
				// treated as an array). Each redis::Reply in the "reply" array contains a
				// 20-byte string that should be ObjectIDs.

				// First, use raw_to_object_id() to convert the byte strings into
				// optional<ObjectID>. If the conversion failed (maybe the byte string is too
				// short), the optional will be empty.
				auto reply_to_oid = [](const redis::Reply& en)
				{
					return raw_to_object_id(en.as_string().substr(0, ObjectID{}.size()));
				};

				// Then we filter out all optionals that doesn't contain a ObjectID.
				auto valid = [](const std::optional<ObjectID>& opt){return opt.has_value();};

				// Finally, all optionals remained should has_value(), so we extract the
				// ObjectID they contain.
				auto extract = [](const std::optional<ObjectID>& opt) {return opt.value();};

				using namespace boost::adaptors;
				comp(
					reply | transformed(reply_to_oid) | filtered(valid) | transformed(extract),
					ec
				);
			}
		},
		"LRANGE %b 0 -1",
		Collection::m_public_blobs.data(), Collection::m_public_blobs.size()
	);
}

template <typename Complete>
void Ownership::query_blob(redis::Connection& db, const ObjectID& blob, Complete&& complete)
{
	static const char lua[] = R"__(
		local dirs = {}
		for k, coll in pairs(redis.call('SMEMBERS', KEYS[1])) do
			table.insert(dirs, coll)
			table.insert(dirs, redis.call('HGET', coll, ARGV[1]))
		end
		return dirs;
	)__";

	db.command(
		[user=m_user, comp=std::forward<Complete>(complete)](auto&& reply, auto ec)
		{
			if (!reply)
				Log(LOG_WARNING, "script reply: %1%", reply.as_error());

			struct Blob
			{
				std::string user;
				std::string coll;
				CollEntry   entry;
			};

			auto kv2blob = [](auto&& kv)
			{
				Collection coll{kv.key()};
				return Blob{coll.user(), coll.path(), CollEntry{kv.value().as_string()}};
			};
			auto owned = [&user](const Blob& blob) {return user == blob.user;};

			using namespace boost::adaptors;
			comp(reply.kv_pairs() | transformed(kv2blob) | filtered(owned), ec);
		},
		"EVAL %s 1 %b:%b %b",
		lua,
		BlobBackLink::m_prefix.data(), BlobBackLink::m_prefix.size(),
		blob.data(), blob.size(),
		blob.data(), blob.size()
	);
}

} // end of namespace
