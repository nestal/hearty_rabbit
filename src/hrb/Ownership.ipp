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
#include "util/JsonHelper.hh"

#include <rapidjson/document.h>

#include <vector>

#pragma once

namespace hrb {

class Ownership::BlobBackLink
{
private:
	static const std::string_view m_prefix;

public:
	BlobBackLink(std::string_view user, std::string_view coll, const ObjectID& blob);
	BlobBackLink(std::string_view db_str);

	// expect to be done inside a transaction
	void link(redis::Connection& db) const;
	void unlink(redis::Connection& db) const;

	const std::string& user() const {return m_user;}
	const ObjectID& blob() const {return m_blob;}

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

public:
	explicit Collection(std::string_view user, std::string_view path);

	void watch(redis::Connection& db);

	void link(redis::Connection& db, const ObjectID& id, const CollEntry& entry);
	void unlink(redis::Connection& db, const ObjectID& id);

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
	) const ;

	template <typename CollectionCallback, typename Complete>
	static void scan(redis::Connection& db, std::string_view user, long cursor,
		CollectionCallback&& callback,
		Complete&& complete
	);

	template <typename Complete>
	static void scan_all(redis::Connection& db, std::string_view owner, std::string_view requester, Complete&& complete);

	const std::string& user() const {return m_user;}
	const std::string& path() const {return m_path;}

private:
	std::string serialize(redis::Reply& reply, std::string_view requester) const;

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
					dirs.foreach_kv_pair([&cb](auto&& key, auto&& value)
					{
						auto sv = value.as_string();
						if (sv.empty())
							return;

						// although we can modify the string inside the redis::Reply "value",
						// it is not a null-terminated string and rapidjson does not support
						// insitu parsing with non-null-terminated strings, so we must
						// use the slower Parse() instead of ParseInsitu()
						rapidjson::Document mdoc;
						mdoc.Parse(sv.data(), sv.size());

						if (!mdoc.HasParseError())
							cb(key, std::move(mdoc));
					});

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
	std::string_view requester,
	Complete&& complete
)
{
	auto jdoc = std::make_shared<rapidjson::Document>();
	jdoc->SetObject();

	jdoc->AddMember("owner",    std::string{owner},     jdoc->GetAllocator());
	jdoc->AddMember("username", std::string{requester}, jdoc->GetAllocator());
	jdoc->AddMember("colls",    rapidjson::Value{}.SetObject(), jdoc->GetAllocator());

	scan(db, owner, 0,
		[&colls=(*jdoc)["colls"], jdoc](auto coll, auto&& json)
		{
			colls.AddMember(
				json::string_ref(coll),
				rapidjson::Value{}.CopyFrom(json, jdoc->GetAllocator()),
				jdoc->GetAllocator()
			);
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
	)__";
	db.command(
		[comp=std::forward<Complete>(complete)](auto&& reply, auto&& ec) mutable
		{
			comp(std::move(ec));
		},
		"EVAL %s 1 %b%b:%b %b %b",
		lua,

		// KEYS[1]
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),

		// ARGV[1]
		blob.data(), blob.size(),

		// ARGV2[]
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
	db.command([comp=std::forward<Complete>(complete)](auto&& reply, std::error_code ec)
	{
		assert(reply.array_size() == 2);
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
	std::string_view requester,
	Complete&& complete
) const
{
	return Collection::scan_all(db, m_user, requester, std::forward<Complete>(complete));
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

} // end of namespace
