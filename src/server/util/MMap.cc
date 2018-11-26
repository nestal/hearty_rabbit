/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 1/17/18.
//

// Application headers
#include "MMap.hh"
#include "common/Error.hh"

// Boost library
#include <boost/beast/core/file_posix.hpp>

// Standard C++ library
#include <cassert>
#include <cstring>

// Linux/POSIX specific
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/mman.h>

namespace hrb {

MMap::MMap(int fd)
{
	std::error_code ec;
	open(fd, ec);
	if (ec)
		throw std::system_error(ec);
}

void MMap::cache() const
{
	// ignore error
	::madvise(m_mmap, m_size, MADV_SEQUENTIAL);
}

MMap MMap::open(int fd, std::error_code& ec)
{
	MMap result;

	struct stat s{};
	if (fstat(fd, &s) == 0)
		result.mmap(fd, static_cast<std::size_t>(s.st_size), PROT_READ, MAP_SHARED, ec);
	else
		ec.assign(errno, std::generic_category());

	return result;
}

MMap MMap::open(const fs::path& path, std::error_code& ec)
{
	MMap result;

	boost::system::error_code bec;
	boost::beast::file_posix file;
	file.open(path.string().c_str(), boost::beast::file_mode::read, bec);
	if (bec)
		ec.assign(bec.value(), bec.category());
	else
		result = open(file.native_handle(), ec);

	return result;
}

void MMap::mmap(int fd, std::size_t size, int prot, int flags, std::error_code& ec)
{
	assert(!is_opened());
	auto addr = ::mmap(nullptr, size, prot, flags, fd, 0);
	if (addr == MAP_FAILED)
	{
		m_mmap = nullptr;
		m_size = 0;
		ec.assign(errno, std::generic_category());
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
	m_mmap = nullptr;
	m_size = 0;
}

MMap MMap::create(int fd, const void *data, std::size_t size, std::error_code& ec)
{
	MMap result;
	result.mmap(fd, size, PROT_READ | PROT_WRITE, MAP_SHARED, ec);
	if (!ec)
		std::memcpy(result.m_mmap, data, size);

	return result;
}

void MMap::swap(MMap& target) noexcept
{
	std::swap(m_mmap, target.m_mmap);
	std::swap(m_size, target.m_size);
}

MMap MMap::allocate(std::size_t size, std::error_code& ec)
{
	MMap result;
	result.mmap(-1, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, ec);
	return result;
}

MMap::MMap(MMap&& m) noexcept
{
	swap(m);
	assert(!m.is_opened());
}

MMap& MMap::operator=(MMap&& rhs) noexcept
{
	MMap copy{std::move(rhs)};
	swap(copy);
	return *this;
}

} // end of namespace
