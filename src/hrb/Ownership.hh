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

/// A set of blob objects represented by a redis set.
class Ownership
{
private:
	static const std::string_view redis_prefix;
	static const std::string_view blob_prefix;
	static const std::string_view dir_prefix;

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
		const char empty_str = '\0';
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&&, std::error_code&& ec) mutable
			{
				comp(std::move(ec));
			},
			"HSET %b%b %b%b %b",

			// key is ownership:<user>
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),

			// hash field is blob:<blob ID>
			Prefix<BlobOrDir>::value.data(), Prefix<BlobOrDir>::value.size(),
			blob_or_dir.data(), blob_or_dir.size(),

			// value is empty
			&empty_str, 0
		);
	}

	template <typename Complete>
	static void remove_blob(
		redis::Connection& db,
		std::string_view user,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&&, std::error_code&& ec) mutable
			{
				comp(std::move(ec));
			},
			"HDEL %b%b %b%b",
			// key is ownership:<user>
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),

			// hash field is blob:<blob ID>
			blob_prefix.data(), blob_prefix.size(),
			blob.data(), blob.size()
		);
	}

	template <typename Complete>
	static void is_owned(
		redis::Connection& db,
		std::string_view user,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		db.command(
			[comp=std::forward<Complete>(complete)](auto&& reply, std::error_code&& ec) mutable
			{
				comp(std::move(ec), reply.as_int() == 1);
			},
			"HEXISTS %b%b %b%b",
			// key is ownership:<user>
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),

			// hash field is blob:<blob ID>
			blob_prefix.data(), blob_prefix.size(),
			blob.data(), blob.size()
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

template <> struct Ownership::Prefix<ObjectID> 			{static const std::string_view value;};
template <> struct Ownership::Prefix<std::string_view>	{static const std::string_view value;};


} // end of namespace hrb
