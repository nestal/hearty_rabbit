/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/20/18.
//

#pragma once

#include <system_error>

namespace hrb {

enum class Error
{
	ok,
	object_not_exist,
	redis_command_error,
	redis_field_not_found,
	redis_transaction_aborted,
	login_incorrect,

	unknown_error
};

const std::error_category& hrb_error_category();
std::error_code make_error_code(Error err);

} // end of namespace hrb

namespace std
{
	template <> struct is_error_code_enum<hrb::Error> : true_type {};
}
