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

#include <boost/core/demangle.hpp>

#ifndef LBUNWIND_FOUND
#include <execinfo.h>
#endif

#include <memory>
#include <iostream>
#include <iomanip>

namespace hrb {

Backtrace::Backtrace()
{
	update();
}

void Backtrace::update()
{
#ifdef LIBUNWIND_FOUND
	::unw_getcontext(&m_uctx);
#else
	m_count = ::backtrace(&m_stack[0], m_stack.size());
#endif
}

std::ostream& operator<<(std::ostream& os, const Backtrace& bs)
{
#ifdef LIBUNWIND_FOUND
	::unw_cursor_t cursor;
	::unw_context_t cx{bs.m_uctx};
	::unw_init_local(&cursor, &cx);

	// Unwind frames one by one, going up the frame stack.
	for (auto i = 0; unw_step(&cursor) > 0 ; i++)
	{
		::unw_word_t pc{};
		unw_get_reg(&cursor, UNW_REG_IP, &pc);
		if (pc == 0)
		{
			break;
		}
		os << std::dec << "#" << i << " " << std::hex << std::setw(sizeof(pc)*2) << std::setfill('0') << pc << ": ";

		char sym[1024] = {};
		::unw_word_t offset{};
		if (::unw_get_proc_name(&cursor, sym, sizeof(sym)-1, &offset) == 0)
		{
			auto demangled_sym = boost::core::demangle(sym);
			os << demangled_sym << "+0x" << std::hex << std::setw(sizeof(offset)*2) << std::setfill('0') << offset << "\n";
		}
		else
		{
			os << "Error getting symbol and offset from " << pc << "\n";
		}
	}

#else
	auto free_it = [](char **p) {::free(p);};
	std::unique_ptr<char*, decltype(free_it)> symbols{
		::backtrace_symbols(&bs.m_stack[0], bs.m_count),
		free_it
	};

	for (auto i = 0; i < bs.m_count; i++)
		os << symbols.get()[i] << '\n';
#endif
	return os;
}

} // end of namespace
