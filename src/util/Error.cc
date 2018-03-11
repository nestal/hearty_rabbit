/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/20/18.
//

#include "Error.hh"

namespace hrb {

const std::error_category& hrb_error_category()
{
	struct Cat : std::error_category
	{
		Cat() = default;
		const char *name() const noexcept override { return "hrb"; }

		std::string message(int ev) const override
		{
			switch (static_cast<Error>(ev))
			{
				case Error::ok: return "no error";
				case Error::object_not_exist: return "object not exist";
				case Error::invalid_object: return "invalid object";
				case Error::mmap_already_opened: return "mmap already opened";
				case Error::redis_command_error: return "redis command error";
				case Error::redis_field_not_found: return "redis field not found";
				case Error::redis_transaction_aborted: return "redis transaction aborted";
				case Error::login_incorrect: return "login incorrect";
				default: return "unknown error " + std::to_string(ev);
			}
		}
	};
	static const Cat cat;
	return cat;
}

std::error_code make_error_code(Error err)
{
	return std::error_code(static_cast<int>(err), hrb_error_category());
}

} // end of namespace
