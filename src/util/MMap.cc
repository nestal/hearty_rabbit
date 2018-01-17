/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/17/18.
//

#include "MMap.hh"

#include <sys/mman.h>
#include <sys/stat.h>
#include <cstring>
#include <utility>
#include <cassert>

namespace hrb {

MMap::MMap(int fd)
{
	std::error_code ec;
	open(fd, ec);
	if (ec)
		throw std::system_error(ec);
}

void MMap::open(int fd, std::error_code& ec)
{
	assert(!is_opened());
	struct stat s{};
	if (fstat(fd, &s) == 0)
		map(fd, static_cast<std::size_t>(s.st_size), PROT_READ, MAP_SHARED, ec);
	else
		ec.assign(errno, std::system_category());
}

void MMap::map(int fd, std::size_t size, int prot, int flags, std::error_code& ec)
{
	assert(!is_opened());
	auto addr = ::mmap(nullptr, size, prot, flags, fd, 0);
	if (addr == MAP_FAILED)
	{
		m_mmap = nullptr;
		m_size = 0;
		ec.assign(errno, std::system_category());
	}
	else
	{
		m_mmap = addr;
		m_size = size;
		ec.clear();
	}
}

MMap::~MMap()
{
	if (is_opened())
		clear();
}

void MMap::clear()
{
	assert(is_opened());
	::munmap(m_mmap, m_size);
}

void MMap::create(int fd, const void *data, std::size_t size, std::error_code& ec)
{
	assert(!is_opened());
	map(fd, size, PROT_READ|PROT_WRITE, MAP_SHARED, ec);
	if (!ec)
		std::memcpy(m_mmap, data, size);
}

void MMap::swap(MMap& target)
{
	std::swap(m_mmap, target.m_mmap);
	std::swap(m_size, target.m_size);
}

void MMap::allocate(std::size_t size, std::error_code& ec)
{
	assert(!is_opened());
	map(-1, size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, ec);
}

} // end of namespace
