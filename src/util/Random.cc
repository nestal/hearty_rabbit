/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#include "Random.hh"

#include <system_error>

// C++17 is doing cmake's job
#if __has_include(<sys/random.h>)
#include <sys/random.h>
#else
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/random.h>
#endif

namespace {

#if ! __has_include(<sys/random.h>)
ssize_t getrandom(void *buf, size_t size, unsigned int flags)
{
	return syscall(SYS_getrandom, buf, size, flags);
}
#endif

}

namespace hrb {

void random(void *buf, std::size_t size)
{
	if (::getrandom(buf, size, 0) != size)
		throw std::system_error(errno, std::generic_category());
}

} // end of namespace hrb
