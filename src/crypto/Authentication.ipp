/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/1/18.
//

#pragma once

#include "Authentication.hh"
#include "Random.hh"

#include "net/Redis.hh"

namespace hrb {

template <typename Complete>
void Authentication::share_resource(
	std::string_view owner,
	std::string_view resource,
	redis::Connection& db,
	Complete&& comp
)
{
	auto auth = insecure_random<Cookie>();

	db.command(
		[
			comp=std::forward<Complete>(comp),
			owner=std::string{owner},
			auth
	    ](auto&&, auto ec)
		{
			comp(Authentication{auth, owner, true}, ec);
		},
		"SADD %b%b:%b %b",
		m_shared_auth_prefix.data(), m_shared_auth_prefix.size(),
		owner.data(), owner.size(),
		resource.data(), resource.size(),
		auth.data(), auth.size()
	);
}

template <typename Complete>
void Authentication::is_shared_resource(
	std::string_view resource,
	redis::Connection& db,
	Complete&& comp
)
{
	db.command(
		[comp=std::forward<Complete>(comp)](auto&& reply, auto ec)
		{
			comp(reply.as_int() == 1, ec);
		},
		"SISMEMBER %b%b:%b %b",
		m_shared_auth_prefix.data(), m_shared_auth_prefix.size(),
		m_user.data(), m_user.size(),
		resource.data(), resource.size(),
		m_cookie.data(), m_cookie.size()
	);
}

} // end of namespace
