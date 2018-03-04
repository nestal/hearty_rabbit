/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
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

class Entry
{
public:
	Entry(std::string_view filename, const ObjectID& blob, std::string_view mime) :
		m_filename{filename},
		m_blob{blob},
		m_mime{mime}
	{
	}

	std::string JSON() const;
	static std::string JSON(const ObjectID& blob, std::string_view mime);

private:
	std::string m_filename;
	ObjectID    m_blob;
	std::string m_mime;
};

/// A set of blob objects represented by a redis set.
class Container
{
private:
	static const std::string_view redis_prefix;

public:
	explicit Container(std::string_view user, std::string_view name);

	template <typename Complete>
	static void add(
		redis::Connection& db,
		std::string_view user,
		std::string_view dir,
		std::string_view filename,
		const ObjectID& blob,
		std::string_view mime,
		Complete&& complete
	)
	{
		auto json = Entry::JSON(blob, mime);
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&&, std::error_code&& ec) mutable
			{
				comp(std::move(ec));
			},
			"HSET %b%b:%b %b %b",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
			dir.data(), dir.size(),
			filename.data(), filename.size(),
			json.data(), json.size()
		);
	}

private:
	std::string                 m_name;
	std::vector<std::string>    m_jsons;
};

} // end of namespace hrb
