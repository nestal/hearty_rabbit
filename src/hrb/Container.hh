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
#include <vector>

namespace hrb {

/// A set of blob objects represented by a redis set.
class Container
{
public:
	Container(std::string_view name);

	template <typename Complete>
	static void load(
		redis::Connection& db,
		std::string_view container,
		Complete&& complete
	)
	{
		complete(Container{container});
	}

	template <typename Complete>
	static void add(
		redis::Connection& db,
		std::string_view container,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		db.command([comp=std::forward<Complete>(complete)](auto&&, std::error_code&& ec)
		{
			comp(std::move(ec));
		}, "SADD dir:%b %b", container.data(), container.size(), blob.data(), blob.size());
	}

	template <typename Complete>
	static void is_member(
		redis::Connection& db,
		std::string_view container,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		db.command([comp=std::forward<Complete>(complete)](auto&& reply, std::error_code&& ec)
		{
			comp(std::move(ec), reply.as_int() == 1);
		}, "SISMEMBER dir:%b %b", container.data(), container.size(), blob.data(), blob.size());
	}

private:
	std::string             m_name;
	std::vector<ObjectID>   m_blobs;
};

} // end of namespace hrb
