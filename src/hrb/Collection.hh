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
#include "util/Error.hh"

#include <string_view>
#include <functional>
#include <unordered_map>
#include <sstream>
#include <iostream>

namespace hrb {

/// A set of blob objects represented by a redis set.
class Collection
{
private:
	static const std::string_view redis_prefix;

public:
	explicit Collection(std::string_view user, std::string_view path);

	void watch(redis::Connection& db);

	void link(redis::Connection& db, const ObjectID& id);
	void unlink(redis::Connection& db, const ObjectID& id);

	template <typename Complete, typename BlobDb>
	void serialize(
		redis::Connection& db,
		const BlobDb& blobdb,
		Complete&& complete
	)
	{
		db.command(
			[
				comp=std::forward<Complete>(complete),
				user=std::string{m_user},
				path=std::string{m_path},
				&blobdb
			](auto&& reply, std::error_code&& ec) mutable
			{
				comp(Collection::serialize(std::move(user), std::move(path), blobdb, reply), std::move(ec));
			},
			"SMEMBERS %b%b:%b",
			redis_prefix.data(), redis_prefix.size(),
			m_user.data(), m_user.size(),
			m_path.data(), m_path.size()
		);
	}

	template <typename Complete>
	static void scan(
		redis::Connection& db,
		std::string_view user,
		long cursor,
		Complete&& complete
	)
	{
		db.command(
			[
				comp=std::forward<Complete>(complete), &db, user=std::string{user}
			](redis::Reply&& reply, std::error_code&& ec) mutable
			{
				if (!ec)
				{
					auto [cursor_reply, dirs] = reply.as_tuple<2>(ec);
					if (!ec)
					{
						// Repeat scanning only when the cycle is not completed yet (i.e.
						// cursor != 0), and the completion callback return true.
						auto cursor = cursor_reply.to_int();
						if (comp(dirs.begin(), dirs.end(), cursor, ec) && cursor != 0)
							scan(db, user, cursor, std::move(comp));
						return;
					}
				}

				redis::Reply empty{};
				comp(empty.begin(), empty.end(), 0, ec);
			},
			"SCAN %d MATCH %b%b:*",
			cursor,
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size()
		);
	}

	const std::string& user() const {return m_user;}
	const std::string& path() const {return m_path;}

private:
	template <typename BlobDb>
	static std::string serialize(std::string&& user, std::string&& path, const BlobDb& blobdb, redis::Reply& reply)
	{
		std::ostringstream ss;
		ss  << R"__({"username":")__"      << user
			<< R"__(", "collection":")__"    << path
			<< R"__(", "elements":)__" << "{";

		bool first = true;
		for (auto&& blob : reply)
		{
			if (first)
				first = false;
			else
				ss << ",\n";

			auto blob_id = raw_to_object_id(blob.as_string());

			ss  << to_quoted_hex(blob_id) << ":"
				<< blobdb.load_meta_json(blob_id);
		}
		ss << "}}";
		return ss.str();
	}

private:
	std::string m_user;
	std::string m_path;
};

} // end of namespace hrb
