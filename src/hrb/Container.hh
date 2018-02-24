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

/// A set of blob objects represented by a redis set.
class Container
{
private:
	static const std::string_view redis_prefix;

public:
	Container(std::string_view name);

	template <typename Complete>
	static void load(
		redis::Connection& db,
		std::string_view container,
		Complete&& complete
	)
	{
		db.command([comp=std::forward<Complete>(complete), name=std::string{container}](auto&& reply, std::error_code&& ec) mutable
		{
			Container result{name};
			for (auto&& element : reply)
			{
				if (ObjectID oid = raw_to_object_id(element.as_string()); oid != ObjectID{})
					result.m_blobs.push_back(oid);
			}

			comp(std::move(ec), std::move(result));
		}, "SMEMBERS %b%b", redis_prefix.data(), redis_prefix.size(), container.data(), container.size());
	}

	template <typename Complete>
	static void add(
		redis::Connection& db,
		std::string_view container,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		db.command([comp=std::forward<Complete>(complete)](auto&&, std::error_code&& ec) mutable
		{
			comp(std::move(ec));
		}, "SADD %b%b %b", redis_prefix.data(), redis_prefix.size(), container.data(), container.size(), blob.data(), blob.size());
	}

	template <typename Complete>
	static void is_member(
		redis::Connection& db,
		std::string_view container,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		db.command(
			[comp=std::forward<Complete>(complete)](auto&& reply, std::error_code&& ec) mutable
			{
				comp(std::move(ec), reply.as_int() == 1);
			},
			"SISMEMBER %b%b %b",
			redis_prefix.data(), redis_prefix.size(),
			container.data(), container.size(),
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

	rapidjson::Document serialize() const;

private:
	std::string             m_name;
	std::vector<ObjectID>   m_blobs;
};

} // end of namespace hrb
