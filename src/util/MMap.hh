/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/17/18.
//

#pragma once

#include <cstddef>
#include <system_error>

namespace hrb {

class MMap
{
public:
	MMap() = default;
	explicit MMap(int fd);
	MMap(MMap&&) noexcept ;
	MMap(const MMap&) = delete;
	MMap& operator=(MMap&&) noexcept ;
	MMap& operator=(const MMap&) = delete;
	~MMap();

	void* data() const {return m_mmap;}
	std::size_t size() const {return m_size;}

	std::string_view string_view() const {return {static_cast<const char*>(m_mmap), m_size};}

	void clear();
	void open(int fd, std::error_code& ec);
	void create(int fd, const void *data, std::size_t size, std::error_code& ec);
	void allocate(std::size_t size, std::error_code& ec);
	bool is_opened() const {return m_mmap;}
	void swap(MMap& target);

private:
	void mmap(int fd, std::size_t size, int prot, int flags, std::error_code& ec);

private:
	void *m_mmap{};         //!< Pointer to memory mapped file
	std::size_t m_size{};   //!< File size in bytes.
};

} // end of namespace
