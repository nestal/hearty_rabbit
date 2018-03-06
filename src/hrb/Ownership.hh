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

public:
	explicit Ownership(std::string_view name);

	template <typename Complete>
	static void add(
		redis::Connection& db,
		std::string_view user,
		const ObjectID& blob,
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
			"HSET %b%b %b %b",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
			blob.data(), blob.size(),
			&empty_str, 0
		);
	}

	template <typename Complete>
	static void remove(
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
			"HDEL %b%b %b",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
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
			"HEXISTS %b%b %b",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
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

} // end of namespace hrb
