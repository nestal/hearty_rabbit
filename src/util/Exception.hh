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
#include <system_error>

namespace hrb {

struct Exception : virtual boost::exception, virtual std::exception
{
	const char* what() const noexcept override ;
};
struct SystemError : virtual boost::exception, virtual std::system_error
{
	using system_error::system_error;
};

} // end of namespace
