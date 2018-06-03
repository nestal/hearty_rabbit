/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/13/18.
//

#pragma once

#include "config.hh"

#ifdef LIBUNWIND_FOUND
#define UNW_LOCAL_ONLY
#include <libunwind.h>
#endif

#include <array>
#include <iosfwd>

namespace hrb {

class Backtrace
{
public:
	Backtrace();

	void update();

	friend std::ostream& operator<<(std::ostream& os, const Backtrace& bs);

private:
#ifdef LIBUNWIND_FOUND
	unw_context_t m_uctx;
#else
	std::array<void*, 100> m_stack;
	int m_count{0};
#endif
};

} // end of namespace
