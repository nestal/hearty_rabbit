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

#include <rapidjson/document.h>

#include <vector>

#pragma once

namespace hrb {

class Ownership::BlobBackLink
{
private:
	static const std::string_view m_prefix;

public:
	BlobBackLink(std::string_view user, const ObjectID& blob);

	void watch(redis::Connection& db) const;

	// expect to be done inside a transaction
	void link(redis::Connection& db, std::string_view coll) const;
	void unlink(redis::Connection& db, std::string_view coll) const;

	const std::string& user() const {return m_user;}
	const ObjectID& blob() const {return m_blob;}

private:
	std::string m_user;
	ObjectID    m_blob;
};

/// A set of blob objects represented by a redis set.
class Ownership::Collection
{
private:
	static const std::string_view m_dir_prefix;
	static const std::string_view m_list_prefix;

public:
	explicit Collection(std::string_view user, std::string_view path);

	void watch(redis::Connection& db);

	void link(redis::Connection& db, const ObjectID& id, const CollEntry& entry);
	void unlink(redis::Connection& db, const ObjectID& id);

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

						// although we can modify the string inside value, rapidjson does not
						// support insitu parsing with non-null-terminated string, so we must
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
	// we can't lock a field in the hash table, so lock a dummy key instead
	db.command(
		"WATCH lock:%b%b:%b:%b",
		m_dir_prefix.data(), m_dir_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		blob.data(), blob.size()
	);
	db.command(
		[
			comp=std::forward<Complete>(complete),
			db=db.shared_from_this(), *this, blob, perm
		](auto&& reply, std::error_code&& ec) mutable
		{
			CollEntry entry{reply.as_string()};
			auto s = CollEntry::create(Permission{perm}, entry.filename(), entry.mime());

			db->command("MULTI");
			db->command(
				"SETEX lock:%b%b:%b:%b 3600 %b",
				m_dir_prefix.data(), m_dir_prefix.size(),
				m_user.data(), m_user.size(),
				m_path.data(), m_path.size(),
				blob.data(), blob.size(),
				s.data(), s.size()
			);
			db->command(
				"HSET %b%b:%b %b %b",
				m_dir_prefix.data(), m_dir_prefix.size(),
				m_user.data(), m_user.size(),
				m_path.data(), m_path.size(),
				blob.data(), blob.size(),
				s.data(), s.size()
			);
			db->command(
				[comp=std::forward<Complete>(comp)](auto&& reply, auto&& ec) mutable
				{
					comp(std::move(ec));
				},
				"EXEC"
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
	std::string_view path,
	const ObjectID& blobid,
	const CollEntry& entry,
	bool add,
	Complete&& complete
)
{
	BlobBackLink  blob{m_user, blobid};
	Collection coll{m_user, path};

	db.command("MULTI");
	if (add)
	{
		blob.link(db, coll.path());
		coll.link(db, blob.blob(), entry);
	}
	else
	{
		blob.unlink(db, coll.path());
		coll.unlink(db, blob.blob());
	}

	db.command([comp=std::forward<Complete>(complete)](auto&&, std::error_code ec)
	{
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

} // end of namespace
