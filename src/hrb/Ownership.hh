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

#include "Collection.hh"
#include "net/Redis.hh"

#include <rapidjson/document.h>

#include <iostream>
#include <string_view>
#include <functional>
#include <vector>

namespace hrb {

class BlobDatabase;
class ContainerName;

namespace detail {
template <typename BlobOrDir>
struct Prefix {};

template <> struct Prefix<ObjectID> 		{static const std::string_view value;};
template <> struct Prefix<ContainerName>	{static const std::string_view value;};
}

/// A set of blob objects represented by a redis set.
class Ownership
{
private:
	class BlobTable
	{
	public:
		BlobTable(std::string_view user, const ObjectID& blob);

		void watch(redis::Connection& db) const;

		// expect to be done inside a transaction
		void link(redis::Connection& db, std::string_view path) const;
		void unlink(redis::Connection& db, std::string_view path) const;

		const std::string& user() const {return m_user;}
		const ObjectID& blob() const {return m_blob;}

	private:
		std::string m_user;
		ObjectID    m_blob;
	};
	static const std::string_view redis_prefix;

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
		BlobTable  blob{m_user, blobid};
		Collection coll{m_user, path};

		// watch everything that will be modified
		blob.watch(db);
		coll.watch(db);

		coll.has(
			db, blobid, [
				blob, coll, db=db.shared_from_this(), comp=std::forward<Complete>(complete)
			](bool present, std::error_code ec) mutable
			{
				if (!present)
				{
					db->command("MULTI");
					blob.link(*db, coll.path());
					coll.link(*db, blob.blob());

					db->command([comp=std::move(comp)](auto&&, std::error_code ec)
					{
						comp(ec);
					}, "EXEC");
				}
				else
				{
					db->command([comp = std::move(comp)](auto&&, std::error_code ec)
					{
						comp(ec);
					}, "DISCARD");
				}
			}
		);
	}

	template <typename Complete, typename BlobOrDir>
	static void add(
		redis::Connection& db,
		std::string_view user,
		const BlobOrDir& blob_or_dir,
		Complete&& complete
	)
	{
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&&, std::error_code&& ec) mutable
			{
				comp(std::move(ec));
			},
			"HINCRBY %b%b %b%b 1",

			// key is ownership:<user>
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),

			// hash field is blob:<blob ID>
			detail::Prefix<BlobOrDir>::value.data(), detail::Prefix<BlobOrDir>::value.size(),
			blob_or_dir.data(), blob_or_dir.size()
		);
	}

	template <typename Complete, typename BlobOrDir>
	static void unlink(
		redis::Connection& db,
		std::string_view user,
		const BlobOrDir& blob_or_dir,
		Complete&& complete
	)
	{
		// If only redis has a command to automatically delete hash field if
		// it's decremented to zero. But since there is no harm to keep these
		// zeroed fields in the hash, we will leave them.
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&& reply, std::error_code&& ec) mutable
			{
				comp(reply.as_int() == 0, std::move(ec));
			},
			"HINCRBY %b%b %b%b -1",
			// key is ownership:<user>
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),

			// hash field is blob:<blob ID>
			detail::Prefix<BlobOrDir>::value.data(), detail::Prefix<BlobOrDir>::value.size(),
			blob_or_dir.data(), blob_or_dir.size()
		);
	}

	template <typename Complete, typename BlobOrDir>
	static void is_owned(
		redis::Connection& db,
		std::string_view user,
		const BlobOrDir& blob_or_dir,
		Complete&& complete
	)
	{
		db.command(
			[comp=std::forward<Complete>(complete)](auto&& reply, std::error_code&& ec) mutable
			{
				comp(std::move(ec), reply.to_int() > 0);
			},
			"HGET %b%b %b%b",
			// key is ownership:<user>
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),

			// hash field is blob:<blob ID>
			detail::Prefix<BlobOrDir>::value.data(), detail::Prefix<BlobOrDir>::value.size(),
			blob_or_dir.data(), blob_or_dir.size()
		);
	}

	const std::string& user() const {return m_user;}

	using value_type = ObjectID;
	using iterator = std::vector<ObjectID>::const_iterator;
	iterator begin() const {return m_blobs.begin();}
	iterator end() const {return m_blobs.end();}
	std::size_t size() const {return m_blobs.size();}
	bool empty() const {return m_blobs.empty();}

	std::string serialize(const BlobDatabase& db) const;

private:
	std::string             m_user;
	std::vector<ObjectID>   m_blobs;
};


} // end of namespace hrb
