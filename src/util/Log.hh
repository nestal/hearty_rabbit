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

#ifdef SYSTEMD_FOUND
#include <systemd/sd-journal.h>
#endif

namespace hrb {

template <typename... Args>
int Log(int priority, const std::string& fmt, Args... args)
{
	// I love C++17
	auto&& line = (boost::format{fmt} % ... % std::forward<Args>(args)).str();

	return
#ifdef SYSTEMD_FOUND
		::sd_journal_print
#else
		::syslog
#endif
	(priority, line.c_str());
}

} // end of namespace

