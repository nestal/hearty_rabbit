/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#include "Collection.hh"

namespace hrb {

const std::string_view Collection::redis_prefix = "dir:";

Collection::Collection(std::string_view user, std::string_view path) :
	m_user{user},
	m_path{path}
{
}

void Collection::watch(redis::Connection& db,
	std::string_view user,
	std::string_view path
)
{
	db.command("WATCH %b%b:%b",
		redis_prefix.data(), redis_prefix.size(),
		user.data(), user.size(), path.data(), path.size()
	);
}


} // end of namespace hrb
