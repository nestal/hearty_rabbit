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

#include "common/ObjectID.hh"
#include "CollEntryDB.hh"

#include "net/Redis.hh"

#include <string_view>
#include <functional>

namespace hrb {

class Authentication;
class Permission;
class CollEntry;

/// A set of blob objects represented by a redis set.
class Ownership
{
public:
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
	);

	void update(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blobid,
		const CollEntry& entry
	);

	void update(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blobid,
		const nlohmann::json& entry
	);

	template <typename Complete>
	void unlink(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blobid,
		Complete&& complete
	);

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
		const Authentication& requester,
		std::string_view coll,
		Complete&& complete
	) const;

	template <typename Complete>
	void list(
		redis::Connection& db,
		std::string_view coll,
		Complete&& complete
	) const;

	template <typename Complete>
	void move_blob(
		redis::Connection& db,
		std::string_view src_coll,
		std::string_view dest_coll,
		const ObjectID& blobid,
		Complete&& complete
	);

	template <typename Complete>
	void find(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blob,
		Complete&& complete
	) const;

	template <typename Complete>
	void set_cover(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blob,
		Complete&& complete
	) const;

	template <typename CollectionCallback, typename Complete>
	void scan_collections(
		redis::Connection& db,
		long cursor,
		CollectionCallback&& callback,
		Complete&& complete
	) const;

	template <typename Complete>
	void scan_all_collections(
		redis::Connection& db,
		Complete&& complete
	) const;

	template <typename Complete>
	void list_public_blobs(
		redis::Connection& db,
		Complete&& complete
	);

	template <typename Complete>
	void query_blob(
		redis::Connection& db,
		const ObjectID& blob,
		Complete&& complete
	);

	const std::string& user() const {return m_user;}

private:
	std::string m_user;
};

} // end of namespace hrb
