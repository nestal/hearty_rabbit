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

#include <string_view>
#include <functional>
#include <unordered_map>
#include <sstream>

namespace hrb {

class Entry
{
public:
	Entry(std::string_view filename, std::string_view json);

	Entry(std::string_view filename, const ObjectID& blob, std::string_view mime) :
		m_filename{filename},
		m_blob{blob},
		m_mime{mime}
	{
	}

	std::string JSON() const;
	static std::string JSON(const ObjectID& blob, std::string_view mime, std::string_view filename);

	const ObjectID& blob() const {return m_blob;}
	const std::string& filename() const {return m_filename;}
	const std::string& mime() const {return m_mime;}

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
	explicit Container(std::string_view user, std::string_view path);

	template <typename Complete>
	static void add(
		redis::Connection& db,
		std::string_view user,
		std::string_view path,
		std::string_view filename,
		const ObjectID& blob,
		std::string_view mime,
		Complete&& complete
	)
	{
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&&, std::error_code&& ec) mutable
			{
				comp(std::move(ec));
			},
			"RPUSH %b%b:%b %b",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
			path.data(), path.size(),
			blob.data(), blob.size()
		);
	}

	template <typename Complete>
	static void load(
		redis::Connection& db,
		std::string_view user,
		std::string_view path,
		Complete&& complete
	)
	{
		db.command([
				comp=std::forward<Complete>(complete),
				user=std::string{user},
				path=std::string{path}
			](auto&& reply, std::error_code&& ec) mutable
			{
				Container con{std::move(user), std::move(path)};

				for (auto&& blob : reply)
					con.m_blobs.push_back(raw_to_object_id(blob.as_string()));

				comp(std::move(con), std::move(ec));
			},
			"LRANGE %b%b:%b 0 -1",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
			path.data(), path.size()
		);
	}

	template <typename Complete, typename BlobDb>
	static void serialize(
		redis::Connection& db,
		std::string_view user,
		std::string_view path,
		const BlobDb& blobdb,
		Complete&& complete
	)
	{
		db.command(
			[
				comp=std::forward<Complete>(complete),
				user=std::string{user},
				path=std::string{path},
				&blobdb
			](auto&& reply, std::error_code&& ec) mutable
			{
				comp(serialize(std::move(user), std::move(path), blobdb, reply), std::move(ec));
			},
			"LRANGE %b%b:%b 0 -1",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
			path.data(), path.size()
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

	using iterator = std::vector<ObjectID>::const_iterator;
	iterator begin() const {return m_blobs.begin();}
	iterator end() const {return m_blobs.end();}

private:
	template <typename BlobDb>
	static std::string serialize(std::string&& user, std::string&& path, const BlobDb& blobdb, redis::Reply& reply)
	{
		std::ostringstream ss;
		ss  << R"__({"name":")__"      << user
			<< R"__(", "path":")__"    << path
			<< R"__(", "elements":)__" << "[";

		bool first = true;
		for (auto&& blob : reply)
		{
			if (first)
				first = false;
			else
				ss << ",\n";

			ss << blobdb.load_meta_json(raw_to_object_id(blob.as_string()));
		}
		ss << "]}";
		return ss.str();
	}

private:
	std::string             m_user;
	std::string             m_path;
	std::vector<ObjectID>   m_blobs;
};

} // end of namespace hrb
