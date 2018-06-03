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

#include <boost/format.hpp>

#include "config.hh"
#include <syslog.h>


namespace hrb {

namespace detail {
void DetailLog(int priority, std::string&& line);
}

template <typename... Args>
void Log(int priority, const std::string& fmt, Args... args)
{
	boost::format bfmt{fmt};
	bfmt.exceptions(boost::io::no_error_bits);

	// I love C++17
	return detail::DetailLog(priority, (bfmt % ... % std::forward<Args>(args)).str());
}

} // end of namespace

