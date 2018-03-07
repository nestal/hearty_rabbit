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

#include <rapidjson/document.h>

#include <string_view>
#include <functional>
#include <vector>

namespace hrb {

class BlobDatabase;
class ContainerName;

/// A set of blob objects represented by a redis set.
class Ownership
{
private:
	static const std::string_view redis_prefix;

	template <typename BlobOrDir>
	struct Prefix {};

public:
	explicit Ownership(std::string_view name);

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
			Prefix<BlobOrDir>::value.data(), Prefix<BlobOrDir>::value.size(),
			blob_or_dir.data(), blob_or_dir.size()
		);
	}

	template <typename Complete, typename BlobOrDir>
	static void remove(
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
			](auto&&, std::error_code&& ec) mutable
			{
				comp(std::move(ec));
			},
			"HINCRBY %b%b %b%b -1",
			// key is ownership:<user>
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),

			// hash field is blob:<blob ID>
			Prefix<BlobOrDir>::value.data(), Prefix<BlobOrDir>::value.size(),
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
			Prefix<BlobOrDir>::value.data(), Prefix<BlobOrDir>::value.size(),
			blob_or_dir.data(), blob_or_dir.size()
		);
	}

	const std::string& name() const {return m_name;}

	using value_type = ObjectID;
	using iterator = std::vector<ObjectID>::const_iterator;
	iterator begin() const {return m_blobs.begin();}
	iterator end() const {return m_blobs.end();}
	std::size_t size() const {return m_blobs.size();}
	bool empty() const {return m_blobs.empty();}

	std::string serialize(const BlobDatabase& db) const;

private:
	std::string             m_name;
	std::vector<ObjectID>   m_blobs;
};

template <> struct Ownership::Prefix<ObjectID> 		{static const std::string_view value;};
template <> struct Ownership::Prefix<ContainerName>	{static const std::string_view value;};


} // end of namespace hrb
