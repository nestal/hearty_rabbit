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
	static const std::string_view m_prefix;

public:
	explicit Collection(std::string_view user, std::string_view path);

	void watch(redis::Connection& db);

	void link(redis::Connection& db, const ObjectID& id, const CollEntry& entry);
	void unlink(redis::Connection& db, const ObjectID& id);

	template <typename Complete>
	void set_permission(redis::Connection& db, const ObjectID& blob, const Permission& perm, Complete&& complete) const;

	template <typename Complete>
	void allow(redis::Connection& db, std::string_view requester, const ObjectID& blob, Complete&& complete) const;

	template <typename Complete>
	void serialize(
		redis::Connection& db,
		Complete&& complete
	) const ;

	template <typename Complete>
	static void scan(redis::Connection& db, std::string_view user, long cursor, Complete&& complete);

	const std::string& user() const {return m_user;}
	const std::string& path() const {return m_path;}

private:
	std::string serialize(redis::Reply& reply) const;

private:
	std::string m_user;
	std::string m_path;
};

template <typename Complete>
void Ownership::Collection::scan(
	redis::Connection& db,
	std::string_view user,
	long cursor,
	Complete&& complete
)
{
	db.command(
		[
			comp=std::forward<Complete>(complete), &db, user=std::string{user}
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
					if (comp(dirs.begin(), dirs.end(), cursor, ec) && cursor != 0)
						scan(db, user, cursor, std::move(comp));
					return;
				}
			}

			redis::Reply empty{};
			comp(empty.begin(), empty.end(), 0, ec);
		},
		"SCAN %d MATCH %b%b:*",
		cursor,
		m_prefix.data(), m_prefix.size(),
		user.data(), user.size()
	);
}

template <typename Complete>
void Ownership::Collection::allow(
	redis::Connection& db,
	std::string_view requester,
	const ObjectID& blob,
	Complete&& complete
) const
{
	db.command(
		[
			comp=std::forward<Complete>(complete),
			requester=std::string{requester},
			requested_by_owner = (m_user == requester)
		](auto&& perm, std::error_code&& ec) mutable
		{
			// Only owner is allow to know whether an object exists or not
			if (!ec && requested_by_owner && perm.is_nil())
				ec = Error::object_not_exist;

			comp(
				// true if granted access, false if deny
				// always allow access to owner
				!perm.is_nil() && (requested_by_owner || CollEntry{perm.as_string()}.allow(requester)),
				std::move(ec)
			);
		},

		"HGET %b%b:%b %b",
		m_prefix.data(), m_prefix.size(),
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
	db.command(
		[
			comp=std::forward<Complete>(complete),
			perm
		](auto&&, std::error_code&& ec) mutable
		{
			comp(std::move(ec));
		},

		"HSET %b%b:%b %b %b",
		m_prefix.data(), m_prefix.size(),
		m_user.data(), m_user.size(),
		m_path.data(), m_path.size(),
		blob.data(), blob.size(),
		perm.data(), perm.size()
	);
}

template <typename Complete>
void Ownership::Collection::serialize(
	redis::Connection& db,
	Complete&& complete
) const
{
	db.command(
		[
			comp=std::forward<Complete>(complete),
			user=std::string{m_user},
			path=std::string{m_path}
		](auto&& reply, std::error_code&& ec) mutable
		{
			comp(Collection{user,path}.serialize(reply), std::move(ec));
		},
		"HGETALL %b%b:%b",
		m_prefix.data(), m_prefix.size(),
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
	std::string_view coll,
	Complete&& complete
) const
{
	return Collection{m_user, coll}.serialize(db, std::forward<Complete>(complete));
}

template <typename Complete>
void Ownership::allow(
	redis::Connection& db,
	std::string_view requester,
	std::string_view coll,
	const ObjectID& blob,
	Complete&& complete
) const
{
	return Collection{m_user, coll}.allow(db, requester, blob, std::forward<Complete>(complete));
}

template <typename Complete>
void Ownership::scan_collections(
	redis::Connection& db,
	long cursor,
	Complete&& complete
) const
{
	return Collection::scan(db, m_user, cursor, std::forward<Complete>(complete));
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
