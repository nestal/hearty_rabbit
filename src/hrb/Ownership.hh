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
#include "util/Log.hh"

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
	class BlobBackLink
	{
	private:
		static const std::string_view m_prefix;

	public:
		BlobBackLink(std::string_view user, const ObjectID& blob);

		void watch(redis::Connection& db) const;

		// expect to be done inside a transaction
		void link(redis::Connection& db, std::string_view coll) const;
		void unlink(redis::Connection& db, std::string_view coll) const;

		const std::string& user() const {return m_user;}
		const ObjectID& blob() const {return m_blob;}

	private:
		std::string m_user;
		ObjectID    m_blob;
	};

	/// A set of blob objects represented by a redis set.
	class Collection
	{
	private:
		static const std::string_view m_prefix;

	public:
		explicit Collection(std::string_view user, std::string_view path);

		void watch(redis::Connection& db);

		void link(redis::Connection& db, const ObjectID& id, std::string_view perm="");
		void unlink(redis::Connection& db, const ObjectID& id);

		template <typename Complete>
		void is_owned(redis::Connection& db, const ObjectID& blob, Complete&& complete) const
		{
			db.command(
				[comp=std::forward<Complete>(complete)](redis::Reply&& reply, std::error_code&& ec) mutable
				{
					comp(!reply.is_nil(), std::move(ec));
				},

				"HGET %b%b:%b %b",
				m_prefix.data(), m_prefix.size(),
				m_user.data(), m_user.size(),
				m_path.data(), m_path.size(),
				blob.data(), blob.size()
			);
		}

		template <typename Complete, typename BlobDb>
		void serialize(
			redis::Connection& db,
			const BlobDb& blobdb,
			Complete&& complete
		) const
		{
			db.command(
				[
					comp=std::forward<Complete>(complete),
					user=std::string{m_user},
					path=std::string{m_path},
					&blobdb
				](auto&& reply, std::error_code&& ec) mutable
				{
					comp(Collection{user,path}.serialize(blobdb, reply), std::move(ec));
				},
				"HGETALL %b%b:%b",
				m_prefix.data(), m_prefix.size(),
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
				m_prefix.data(), m_prefix.size(),
				user.data(), user.size()
			);
		}

		const std::string& user() const {return m_user;}
		const std::string& path() const {return m_path;}

	private:
		template <typename BlobDb>
		std::string serialize(const BlobDb& blobdb, redis::Reply& reply) const
		{
			std::ostringstream ss;
			ss  << R"__({"username":")__"      << m_user
				<< R"__(", "collection":")__"  << m_path
				<< R"__(", "elements":)__" << "{";

			bool first = true;
			reply.foreach_kv_pair([&ss, &blobdb, &first](auto&& blob, auto&& perm)
			{
				// TODO: check perm

				if (first)
					first = false;
				else
					ss << ",\n";

				auto blob_id = raw_to_object_id(blob);

				ss  << to_quoted_hex(blob_id) << ":"
					<< blobdb.load_meta_json(blob_id);
			});
			ss << "}}";
			return ss.str();
		}

	private:
		std::string m_user;
		std::string m_path;
	};

public:
	explicit Ownership(std::string_view name);

	template <typename Complete>
	void link(
		redis::Connection& db,
		std::string_view path,
		const ObjectID& blobid,
		Complete&& complete
	)
	{
		link(db, path, blobid, true, std::forward<Complete>(complete));
	}

	template <typename Complete>
	void unlink(
		redis::Connection& db,
		std::string_view path,
		const ObjectID& blobid,
		Complete&& complete
	)
	{
		link(db, path, blobid, false, std::forward<Complete>(complete));
	}

	template <typename Complete, typename BlobDb>
	void serialize(
		redis::Connection& db,
		std::string_view coll,
		const BlobDb& blobdb,
		Complete&& complete
	) const
	{
		return Collection{m_user, coll}.serialize(db, blobdb, std::forward<Complete>(complete));
	}

	template <typename Complete>
	void is_owned(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blob,
		Complete&& complete
	) const
	{
		Collection{m_user, coll}.is_owned(db, blob, std::forward<Complete>(complete));
	}

	template <typename Complete>
	void scan_collections(
		redis::Connection& db,
		long cursor,
		Complete&& complete
	) const
	{
		return Collection::scan(db, m_user, cursor, std::forward<Complete>(complete));
	}

	const std::string& user() const {return m_user;}

private:
	template <typename Complete>
	void link(
		redis::Connection& db,
		std::string_view path,
		const ObjectID& blobid,
		bool add,
		Complete&& complete
	)
	{
		BlobBackLink  blob{m_user, blobid};
		Collection coll{m_user, path};

		db.command("MULTI");
		if (add)
		{
			blob.link(db, coll.path());
			coll.link(db, blob.blob());
		}
		else
		{
			blob.unlink(db, coll.path());
			coll.unlink(db, blob.blob());
		}

		db.command([comp=std::forward<Complete>(complete)](auto&&, std::error_code ec)
		{
			Log(LOG_INFO, "transaction completed %1%", ec);
			comp(ec);
		}, "EXEC");
	}

private:
	std::string             m_user;
};


} // end of namespace hrb
