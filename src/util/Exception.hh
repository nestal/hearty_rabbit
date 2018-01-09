/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/7/18.
//

#pragma once

#include <boost/exception/exception.hpp>
#include <boost/exception/error_info.hpp>

#include <system_error>

namespace hrb {

struct Exception : virtual boost::exception, virtual std::exception
{
	const char* what() const noexcept override ;
};

struct SystemError : virtual Exception {};
using ErrorCode = boost::error_info<struct tag_error_code, std::error_code>;

} // end of namespace
