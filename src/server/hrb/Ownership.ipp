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
#include "RedisKeys.hh"

#include "net/Redis.hh"

#include "common/hrb/Permission.hh"
#include "common/hrb/Collection.hh"
#include "common/hrb/CollectionList.hh"
#include "common/util/Error.hh"
#include "common/util/Escape.hh"

#include "crypto/Authentication.hh"
#include "util/Log.hh"

#include <nlohmann/json.hpp>

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <vector>
#include <iostream>

#pragma once

namespace hrb {

template <typename Complete>
void Ownership::link(
	redis::Connection& db,
	std::string_view coll_name,
	const ObjectID& blobid,
	const CollEntry& entry,
	Complete&& complete
)
{
	db.command(
		[comp=std::forward<Complete>(complete)](auto&& r, std::error_code ec)
		{
			comp(ec);
		},
		link_command(coll_name, blobid, entry)
	);
}

template <typename Complete>
void Ownership::unlink(
	redis::Connection& db,
	std::string_view coll_name,
	const ObjectID& blobid,
	Complete&& complete
)
{
	db.do_write(
		unlink_command(coll_name, blobid),
		[comp=std::forward<Complete>(complete)](auto&& reply, std::error_code ec)
		{
			if (!reply || ec)
				Log(LOG_WARNING, "Ownership::unlink() script reply %1% %2%", reply.as_error(), ec);
			comp(ec);
		}
	);
}

template <typename Complete>
void Ownership::find_collection(
	redis::Connection& db,
	const Authentication& requester,
	std::string_view coll,
	Complete&& complete
) const
{
	db.command(
		[
			comp=std::forward<Complete>(complete), *this,
			requester, coll=std::string{coll}
		](auto&& reply, std::error_code&& ec) mutable
		{
			if (!reply || ec)
				Log(LOG_WARNING, "Ownership::find_collection() script reply %1% %2%", reply.as_error(), ec);

			if (reply.array_size() == 2)
			{
				// in some error cases created by unit tests, the string is not valid JSON
				// in the database
				auto meta = nlohmann::json::parse(reply[1].as_string(), nullptr, false);
				if (meta.is_discarded())
					meta = nlohmann::json::object();

				if (!meta.is_object())
				{
					Log(LOG_WARNING, "invalid meta data for collection %1%: %2%", coll, reply[1].as_string());
					meta = nlohmann::json::object();
				}

				comp(from_reply(reply[0], coll, requester, std::move(meta)), ec);
			}

			// This should never happen unless the redis server has bugs.
			else
				comp(no_collection(), hrb::make_error_code(Error::redis_command_error));
		},
		scan_collection_command(coll)
	);

}

template <typename Complete>
void Ownership::list(
	redis::Connection& db,
	std::string_view coll,
	Complete&& complete
) const
{
	auto coll_hash = key::collection(m_user, coll);
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
		"HKEYS %b", coll_hash.data(), coll_hash.size()
	);
}

template <typename Complete>
void Ownership::find(
	redis::Connection& db,
	std::string_view coll,
	const ObjectID& blob,
	Complete&& complete
) const
{
	auto coll_hash = key::collection(m_user, coll);
	auto blob_meta = key::blob_meta(m_user);
	static const char lua[] = R"__(
		if redis.call('HEXISTS', KEYS[1], ARGV[1]) == 1 then
			return {redis.call('HGET', KEYS[2], ARGV[1]), redis.call('HGET', KEYS[1], ARGV[1])}
		else
			return false
		end
	)__";
	db.command(
		[
			comp=std::forward<Complete>(complete)
		](auto&& entry, std::error_code ec) mutable
		{
			if (!ec && entry.is_nil())
				ec = Error::object_not_exist;

			if (entry.array_size() == 2)
				comp(CollEntryDB{entry[0].as_string()}, entry[1].as_string(), ec);
		},
		"EVAL %s 2 %b %b %b", lua,
		coll_hash.data(), coll_hash.size(),
		blob_meta.data(), blob_meta.size(),
		blob.data(), blob.size()
	);
}

template <typename CollectionCallback, typename Complete>
void Ownership::scan_collections(
	redis::Connection& db,
	long cursor,
	CollectionCallback&& callback,
	Complete&& complete
) const
{
	auto coll_list = key::collection_list(m_user);
	db.command(
		[
			comp=std::forward<Complete>(complete),
			callback=std::forward<CollectionCallback>(callback),
			&db, *this
		](redis::Reply&& reply, std::error_code ec) mutable
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
						if (auto sv = p.value().as_string(); !sv.empty())
							callback(p.key(), nlohmann::json::parse(sv));

					// if comp return true, keep scanning with the same callback and
					// comp as completion routine
					if (comp(cursor, ec) && cursor != 0)
						scan_collections(db, cursor, std::move(callback), std::move(comp));
					return;
				}
			}

			comp(0, ec);
		},
		"HSCAN %b %d",
		coll_list.data(), coll_list.size(),
		cursor
	);
}

template <typename Complete>
void Ownership::scan_all_collections(
	redis::Connection& db,
	Complete&& complete
) const
{
	auto coll_list = std::make_shared<CollectionList>();

	scan_collections(db, 0,
		[coll_list, user=m_user](auto coll, auto&& json) mutable
		{
			coll_list->add(user, coll, std::move(json));
		},
		[coll_list, comp=std::forward<Complete>(complete)](long cursor, auto ec) mutable
		{
			if (cursor == 0)
				comp(std::move(*coll_list), ec);
			return true;
		}
	);
}

template <typename Complete>
void Ownership::set_permission(
	redis::Connection& db,
	const ObjectID& blobid,
	Permission perm,
	Complete&& complete
)
{
	db.command(
		[comp=std::forward<Complete>(complete)](auto&& reply, auto&& ec) mutable
		{
			if (!reply)
				Log(LOG_WARNING, "Collection::set_permission(): script error: %1%", reply.as_error());
			comp(std::move(ec));
		},
		set_permission_command(blobid, perm)
	);
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
	db.command(
		[
			comp=std::forward<Complete>(complete),
		    blobid
        ](auto&& reply, auto ec) mutable
		{
			if (!reply)
				Log(LOG_WARNING, "Collection::move_blob(): script error: %1%", reply.as_error());
			comp(ec);
		},
		move_command(src_coll, dest_coll, blobid)
	);
}

template <typename Complete>
void Ownership::list_public_blobs(
	redis::Connection& db,
	Complete&& complete
)
{
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
					auto [owner, blob, coll, entry_str] = row.as_tuple<4>(err);

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
		list_public_blob_command()
	);
}


template <typename Complete>
void Ownership::query_blob(redis::Connection& db, const ObjectID& blob, Complete&& complete)
{
	auto blob_ref   = key::blob_refs(m_user, blob);
	auto blob_meta  = key::blob_meta(m_user);

	static const char lua[] = R"__(
		local dirs = {}
		for k, coll in ipairs(redis.call('SMEMBERS', KEYS[1])) do
			table.insert(dirs, coll)
			table.insert(dirs, redis.call('HGET', KEYS[2], ARGV[1]))
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
				transformed([blob, &user](auto&& kv)
				{
					return BlobRef{user, std::string{kv.key()}, blob, CollEntryDB{kv.value().as_string()}};
				}),
				ec
			);
		},
		"EVAL %s 2 %b %b  %b",
		lua,
		blob_ref.data(),  blob_ref.size(),      // KEYS[1]
		blob_meta.data(), blob_meta.size(),     // KEYS[2]
		blob.data(), blob.size()
	);
}

template <typename Complete>
void Ownership::set_cover(redis::Connection& db, std::string_view coll, const ObjectID& blob, Complete&& complete) const
{
	db.command(
		[comp=std::forward<Complete>(complete)](auto&& reply, auto ec)
		{
			if (!reply || ec)
				Log(LOG_WARNING, "set_cover(): reply %1% %2%", reply.as_error(), ec);

			comp(reply.as_int() == 1, ec);
		},
		set_cover_command(coll, blob)
	);
}

} // end of namespace
