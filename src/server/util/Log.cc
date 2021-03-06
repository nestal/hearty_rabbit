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

namespace hrb {
namespace detail {

void DetailLog(int priority, std::string &&line)
{
	::syslog(priority, "%s", line.c_str());
	::printf("%s\n", line.c_str());
}

}} // end of namespace
