/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 3/12/18.
//

#include "Ownership.hh"

#pragma once

namespace hrb {

template <typename Complete>
void Ownership::Collection::scan(
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

template <typename Complete>
void Ownership::Collection::is_owned(redis::Connection& db, const ObjectID& blob, Complete&& complete) const
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
void Ownership::Collection::serialize(
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

template <typename BlobDb>
std::string Ownership::Collection::serialize(const BlobDb& blobdb, redis::Reply& reply) const
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

template <typename Complete>
void Ownership::link(
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
		comp(ec);
	}, "EXEC");
}

} // end of namespace
