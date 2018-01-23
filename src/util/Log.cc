/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/8/18.
//

#include "Log.hh"

#ifdef SYSTEMD_FOUND
#include <systemd/sd-journal.h>
#endif

namespace hrb {
namespace detail {

void DetailLog(int priority, std::string &&line)
{
	// preprocessor is bad
#ifdef SYSTEMD_FOUND
	::sd_journal_print
#else
	syslog
#endif
	(priority, "%s", line.c_str());
}

}} // end of namespace

