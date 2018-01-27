/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#include "Authenication.hh"

#include "net/Redis.hh"

#include <openssl/evp.h>

namespace hrb {

Authenication::Authenication(std::string_view username, std::string_view password, hrb::redis::Database& db)
{

}

void add_user(
	std::string_view username,
	std::string_view password,
	redis::Database& db,
	std::function<void(std::string_view cookie, std::error_code)> completion
)
{
/*	db.command([](auto reply, auto& ec)
	{

	}, "HSETNX user:%s field value ");*/
}

} // end of namespace hrb
