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

class BlobDatabase;

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
		auto json = Entry::JSON(blob, mime, filename);
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&&, std::error_code&& ec) mutable
			{
				comp(std::move(ec));
			},
			"HSET %b%b:%b %b %b",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
			path.data(), path.size(),
			filename.data(), filename.size(),
			json.data(), json.size()
		);
	}

	template <typename Complete>
	static void find_entry(
		redis::Connection& db,
		std::string_view user,
		std::string_view path,
		std::string_view filename,
		Complete&& complete
	)
	{
		db.command([
				comp=std::forward<Complete>(complete)
			](auto&& reply, std::error_code&& ec) mutable
			{
				comp(reply.as_string(), std::move(ec));
			},
			"HGET %b%b:%b %b",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
			path.data(), path.size(),
			filename.data(), filename.size()
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

				reply.foreach_kv_pair([&con](auto&& filename, auto&& json)
				{
					con.m_jsons.emplace(filename, json.as_string());
				});

				comp(std::move(con), std::move(ec));
			},
			"HGETALL %b%b:%b",
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size(),
			path.data(), path.size()
		);
	}

	std::string find(const std::string& filename) const;
	std::optional<Entry> find_entry(const std::string& filename) const;

	template <typename Complete>
	static void serialize(
		redis::Connection& db,
		std::string_view user,
		std::string_view path,
		Complete&& complete
	)
	{
		db.command(
			[
				comp=std::forward<Complete>(complete),
				user=std::string{user},
				path=std::string{path}
			](auto&& reply, std::error_code&& ec) mutable
			{
				comp(serialize(std::move(user), std::move(path), reply), std::move(ec));
			},
			"HGETALL %b%b:%b",
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
				comp=std::forward<Complete>(complete)
			](redis::Reply&& reply, std::error_code&& ec) mutable
			{
				if (!ec)
				{
					auto [cursor, dirs] = reply.as_tuple<2>(ec);
					if (!ec)
						return comp(dirs.begin(), dirs.end(), cursor.to_int(), ec);
				}

				redis::Reply empty{};
				return comp(empty.begin(), empty.end(), 0, ec);
			},
			"SCAN %d MATCH %b%b:*",
			cursor,
			redis_prefix.data(), redis_prefix.size(),
			user.data(), user.size()
		);
	}

private:
	static std::string serialize(std::string&& user, std::string&& path, redis::Reply& reply);

private:
	std::string                         m_user;
	std::string                         m_path;
	std::unordered_map<std::string, std::string>  m_jsons;
};

} // end of namespace hrb
