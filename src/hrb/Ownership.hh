/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/23/18.
//

#pragma once

#include "ObjectID.hh"

#include "net/Redis.hh"
#include "util/Log.hh"

#include <rapidjson/document.h>

#include <string_view>
#include <functional>
#include <vector>

namespace hrb {

class BlobDatabase;

/// A set of blob objects represented by a redis set.
class Ownership
{
private:
	class BlobBackLink
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
	class Collection
	{
	private:
		static const std::string_view m_prefix;

	public:
		explicit Collection(std::string_view user, std::string_view path);

		void watch(redis::Connection& db);

		void link(redis::Connection& db, const ObjectID& id, std::string_view perm="");
		void unlink(redis::Connection& db, const ObjectID& id);

		template <typename Complete>
		void is_owned(redis::Connection& db, const ObjectID& blob, Complete&& complete) const;

		template <typename Complete, typename BlobDb>
		void serialize(
			redis::Connection& db,
			const BlobDb& blobdb,
			Complete&& complete
		) const ;

		template <typename Complete>
		static void scan(redis::Connection& db, std::string_view user, long cursor, Complete&& complete);

		const std::string& user() const {return m_user;}
		const std::string& path() const {return m_path;}

	private:
		template <typename BlobDb>
		std::string serialize(const BlobDb& blobdb, redis::Reply& reply) const;

	private:
		std::string m_user;
		std::string m_path;
	};

public:
	explicit Ownership(std::string_view name);

	template <typename Complete>
	void link(
		redis::Connection& db,
		std::string_view path,
		const ObjectID& blobid,
		Complete&& complete
	)
	{
		link(db, path, blobid, true, std::forward<Complete>(complete));
	}

	template <typename Complete>
	void unlink(
		redis::Connection& db,
		std::string_view path,
		const ObjectID& blobid,
		Complete&& complete
	)
	{
		link(db, path, blobid, false, std::forward<Complete>(complete));
	}

	template <typename Complete, typename BlobDb>
	void serialize(
		redis::Connection& db,
		std::string_view coll,
		const BlobDb& blobdb,
		Complete&& complete
	) const
	{
		return Collection{m_user, coll}.serialize(db, blobdb, std::forward<Complete>(complete));
	}

	template <typename Complete>
	void is_owned(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blob,
		Complete&& complete
	) const
	{
		Collection{m_user, coll}.is_owned(db, blob, std::forward<Complete>(complete));
	}

	template <typename Complete>
	void scan_collections(
		redis::Connection& db,
		long cursor,
		Complete&& complete
	) const
	{
		return Collection::scan(db, m_user, cursor, std::forward<Complete>(complete));
	}

	const std::string& user() const {return m_user;}

private:
	template <typename Complete>
	void link(
		redis::Connection& db,
		std::string_view path,
		const ObjectID& blobid,
		bool add,
		Complete&& complete
	);

private:
	std::string             m_user;
};

} // end of namespace hrb
