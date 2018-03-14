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

#include <string_view>
#include <functional>

namespace hrb {

class Permission;

// TODO: move to a separate header file
class CollEntry
{
public:
	CollEntry() = default;
	explicit CollEntry(std::string_view redis_reply);

	static std::string create(std::string_view perm, std::string_view filename, std::string_view mime);

	bool allow(std::string_view user) const;

	std::string filename() const;
	std::string mime() 	const;

	std::string_view json() const;

	std::string_view raw() const {return m_raw;}
	auto data() const {return m_raw.data();}
	auto size() const {return m_raw.size();}

private:
	std::string_view	m_raw{" {}"};
};

/// A set of blob objects represented by a redis set.
class Ownership
{
private:
	class BlobBackLink;

	/// A set of blob objects represented by a redis set.
	class Collection;

public:
	explicit Ownership(std::string_view name);

	template <typename Complete>
	void link(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blobid,
		const CollEntry& entry,
		Complete&& complete
	)
	{
		link(db, coll, blobid, entry, true, std::forward<Complete>(complete));
	}

	template <typename Complete>
	void unlink(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blobid,
		Complete&& complete
	)
	{
		link(db, coll, blobid, CollEntry{}, false, std::forward<Complete>(complete));
	}

	template <typename Complete>
	void set_permission(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blobid,
		const Permission& perm,
		Complete&& complete
	);

	template <typename Complete>
	void serialize(
		redis::Connection& db,
		std::string_view coll,
		Complete&& complete
	) const;

	template <typename Complete>
	void allow(
		redis::Connection& db,
		std::string_view requester,
		std::string_view coll,
		const ObjectID& blob,
		Complete&& complete
	) const;

	template <typename Complete>
	void scan_collections(
		redis::Connection& db,
		long cursor,
		Complete&& complete
	) const;

	const std::string& user() const {return m_user;}

private:
	template <typename Complete>
	void link(
		redis::Connection& db,
		std::string_view path,
		const ObjectID& blobid,
		const CollEntry& entry,
		bool add,
		Complete&& complete
	);

private:
	std::string             m_user;
};

} // end of namespace hrb
