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

#include "hrb/ObjectID.hh"
#include "hrb/UserID.hh"

#include "BlobDBEntry.hh"

#include <string_view>
#include <functional>

namespace hrb {
namespace redis {
class Connection;
class CommandString;
class Reply;
}

class Authentication;
class Permission;
class BlobInode;
class Collection;
class CollectionList;

/// Encapsulate all blobs owned by a user.
class Ownership
{
private:
	[[nodiscard]] redis::CommandString link_command(std::string_view coll, const ObjectID& blob, const BlobInode& entry) const;
	[[nodiscard]] redis::CommandString unlink_command(std::string_view coll, const ObjectID& blob) const;
	[[nodiscard]] redis::CommandString move_command(std::string_view src, std::string_view dest, const ObjectID& blob) const;
	[[nodiscard]] redis::CommandString scan_collection_command(std::string_view coll) const;
	[[nodiscard]] redis::CommandString set_permission_command(const ObjectID& blobid, Permission perm) const;
	[[nodiscard]] redis::CommandString set_cover_command(std::string_view coll, const ObjectID& cover) const;
	[[nodiscard]] redis::CommandString list_public_blob_command() const;
	void update(redis::Connection& db, const ObjectID& blobid, const BlobDBEntry& entry);

	[[nodiscard]] Collection from_reply(
		const redis::Reply& hash_getall_reply,
		std::string_view coll,
		const Authentication& requester,
		nlohmann::json&& meta
	) const;

public:
	explicit Ownership(std::string_view name);

	template <typename Complete, typename=std::enable_if_t<std::is_invocable_v<Complete, std::error_code>>>
	void link_blob(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blobid,
		const BlobInode& entry,
		Complete&& complete
	);

	void update_blob(
		redis::Connection& db,
		const ObjectID& blobid,
		const BlobInode& entry
	);

	void update_blob(
		redis::Connection& db,
		const ObjectID& blobid,
		const nlohmann::json& entry
	);

	template <typename Complete, typename=std::enable_if_t<std::is_invocable_v<Complete, std::error_code>>>
	void unlink_blob(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blobid,
		Complete&& complete
	);

	template <typename Complete, typename=std::enable_if_t<std::is_invocable_v<Complete, std::error_code>>>
	void rename_blob(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blobid,
		std::string_view filename,
		Complete&& complete
	);

	template <typename Complete, typename=std::enable_if_t<std::is_invocable_v<Complete, std::error_code>>>
	void set_permission(
		redis::Connection& db,
		const ObjectID& blobid,
		Permission perm,
		Complete&& complete
	);

	template <typename Complete, typename=std::enable_if_t<std::is_invocable_v<Complete, Collection&&, std::error_code>>>
	void get_collection(
		redis::Connection& db,
		const Authentication& requester,
		std::string_view coll,
		Complete&& complete
	) const;

	template <typename Complete, typename=std::enable_if_t<std::is_invocable_v<Complete, std::error_code>>>
	void move_blob(
		redis::Connection& db,
		std::string_view src_coll,
		std::string_view dest_coll,
		const ObjectID& blobid,
		Complete&& complete
	);

	template <
	    typename Complete,
	    typename=std::enable_if_t<std::is_invocable_v<Complete, BlobDBEntry, std::string_view, std::error_code>>
    >
	void get_blob(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blob,
		Complete&& complete
	) const;

	template <
	    typename Complete,
	    typename=std::enable_if_t<std::is_invocable_v<Complete, BlobDBEntry, std::error_code>>
    >
	void get_blob(
		redis::Connection& db,
		const Authentication& requester,
		const ObjectID& blob,
		Complete&& complete
	) const;

	template <typename Complete, typename=std::enable_if_t<std::is_invocable_v<Complete, bool, std::error_code>>>
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

	template <
	    typename Complete,
	    typename=std::enable_if_t<std::is_invocable_v<Complete, CollectionList, std::error_code>>
	    >
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

	[[nodiscard]] auto& user() const {return m_user;}

private:
	std::string m_user;
	UserID m_requester;
};

} // end of namespace hrb
