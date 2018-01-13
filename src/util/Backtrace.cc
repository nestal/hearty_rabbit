/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/13/18.
//

#include "Backtrace.hh"

#include <execinfo.h>
#include <memory>
#include <iostream>

namespace hrb {

Backtrace::Backtrace()
{
	update();
}

void Backtrace::update()
{
	m_count = ::backtrace(&m_stack[0], m_stack.size());
}

std::ostream& operator<<(std::ostream& os, const Backtrace& bs)
{
	auto free_it = [](char **p) {::free(p);};
	std::unique_ptr<char*, decltype(free_it)> symbols{
		::backtrace_symbols(&bs.m_stack[0], bs.m_count),
		free_it
	};

	for (auto i = 0; i < bs.m_count; i++)
		os << symbols.get()[i] << '\n';

	return os;
}

} // end of namespace
