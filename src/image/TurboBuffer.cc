/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/28/18.
//

#include "TurboBuffer.hh"

#include <turbojpeg.h>

#include <utility>
#include <cstring>

namespace hrb {

TurboBuffer::TurboBuffer(void *data, std::size_t size) :
	m_data{static_cast<unsigned char*>(data)}, m_size{size}
{

}

TurboBuffer::TurboBuffer(TurboBuffer&& other) noexcept
{
	swap(other);
}

TurboBuffer::TurboBuffer(const TurboBuffer& other) :
	m_data{other.m_data ? tjAlloc(other.m_size) : nullptr},
	m_size{other.m_size}
{
	if (m_data)
		std::memcpy(m_data, other.m_data, m_size);
}

TurboBuffer::~TurboBuffer()
{
	if (m_data)
		tjFree(m_data);
}

TurboBuffer& TurboBuffer::operator=(TurboBuffer&& other) noexcept
{
	auto tmp{std::move(other)};
	swap(tmp);
	return *this;
}

TurboBuffer& TurboBuffer::operator=(const TurboBuffer& other)
{
	auto tmp{other};
	swap(tmp);
	return *this;
}

void TurboBuffer::swap(TurboBuffer& other) noexcept
{
	std::swap(m_data, other.m_data);
	std::swap(m_size, other.m_size);
}

} // end of namespace hrb
