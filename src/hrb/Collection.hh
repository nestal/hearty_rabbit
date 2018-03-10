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

class ContainerName
{
public:
	ContainerName(std::string_view user, std::string_view path) :
		m_name{std::string{user} + std::string{path}}
	{
	}

	const std::string& str() const {return m_name;}

	auto data() const {return m_name.data();}
	auto size() const {return m_name.size();}

private:
	std::string m_name;
};

/// A set of blob objects represented by a redis set.
class Collection
{
private:
	static const std::string_view redis_prefix;

public:
	explicit Collection(std::string_view user, std::string_view path);

	void watch(redis::Connection& db);

	template <typename Complete>
	void has(
		redis::Connection& db,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		db.command(
			[comp=std::forward<Complete>(complete)](redis::Reply&& reply, std::error_code ec) mutable
			{
				std::cout << "Collection::has() " << reply.type() << " " << reply.as_int() << std::endl;
				comp(reply.as_int() == 1, std::move(ec));
			},
			"SISMEMBER %b%b:%b %b",
			redis_prefix.data(), redis_prefix.size(),
			m_user.data(), m_user.size(),
			m_path.data(), m_path.size(),
			blob.data(), blob.size()
		);
	}

	void link(redis::Connection& db, const ObjectID& id);
	void unlink(redis::Connection& db, const ObjectID& id);

	template <typename Complete>
	static void add(
		redis::Connection& db,
		std::string_view user,
		std::string_view path,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&& reply, std::error_code&& ec) mutable
			{
				comp(reply.as_int() == 1, std::move(ec));
			},
			"SADD %b%b:%b %b",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
			path.data(), path.size(),
			blob.data(), blob.size()
		);
	}

	template <typename Complete>
	static void remove(
		redis::Connection& db,
		std::string_view user,
		std::string_view path,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&& reply, std::error_code&& ec) mutable
			{
				// redis replies with the number of object deleted
				if (!ec && reply.as_int() < 1)
					ec = Error::object_not_exist;

				comp(std::move(ec));
			},
			"SREM %b%b:%b %b",
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
				Collection con{std::move(user), std::move(path)};

				for (auto&& blob : reply)
					con.m_blobs.push_back(raw_to_object_id(blob.as_string()));

				comp(std::move(con), std::move(ec));
			},
			"SMEMBERS %b%b:%b",
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
			"SMEMBERS %b%b:%b",
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

	const std::string& user() const {return m_user;}
	const std::string& path() const {return m_path;}

private:
	template <typename BlobDb>
	static std::string serialize(std::string&& user, std::string&& path, const BlobDb& blobdb, redis::Reply& reply)
	{
		std::ostringstream ss;
		ss  << R"__({"name":")__"      << user
			<< R"__(", "path":")__"    << path
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
	std::string             m_user;
	std::string             m_path;
	std::vector<ObjectID>   m_blobs;
};

} // end of namespace hrb
