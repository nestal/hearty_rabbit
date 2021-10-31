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

#include "BufferView.hh"
#include "FS.hh"

#include <boost/asio/buffer.hpp>

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

	static MMap open(int fd, std::error_code& ec);
	static MMap open(const fs::path& path, std::error_code& ec);
	static MMap create(int fd, const void *data, std::size_t size, std::error_code& ec);
	static MMap allocate(std::size_t size, std::error_code& ec);

	[[nodiscard]] void* data() const {return m_mmap;}
	[[nodiscard]] std::size_t size() const {return m_size;}

	[[nodiscard]] boost::asio::mutable_buffer blob() noexcept {return {m_mmap, m_size};}
	[[nodiscard]] boost::asio::const_buffer blob() const noexcept {return {m_mmap, m_size};}
	[[nodiscard]] std::string_view string() const {return {static_cast<const char*>(m_mmap), m_size};}
	[[nodiscard]] BufferView buffer() const {return {static_cast<const unsigned char*>(m_mmap), m_size};}

	[[nodiscard]] bool is_opened() const {return m_mmap != nullptr;}
	void clear();
	void swap(MMap& target) noexcept ;
	void cache() const;

private:
	void mmap(int fd, std::size_t size, int prot, int flags, std::error_code& ec);

private:
	void *m_mmap{};         //!< Pointer to memory mapped file
	std::size_t m_size{};   //!< File size in bytes.
};

} // end of namespace
