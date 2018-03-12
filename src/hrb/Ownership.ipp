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

} // end of namespace
