/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/28/18.
//

#pragma once

#include "util/BufferView.hh"
#include <cstddef>

namespace hrb {

class TurboBuffer
{
public:
	TurboBuffer() = default;
	TurboBuffer(void *data, std::size_t size);
	TurboBuffer(TurboBuffer&& other) noexcept ;
	TurboBuffer(const TurboBuffer& other);
	~TurboBuffer();

	TurboBuffer& operator=(TurboBuffer&& other) noexcept ;
	TurboBuffer& operator=(const TurboBuffer& other);

	void swap(TurboBuffer& other) noexcept ;

	const unsigned char* data() const {return m_data;}
	      unsigned char* data()       {return m_data;}
	std::size_t size() const {return m_size;}

	BufferView buffer() const {return {m_data, m_size};}

	bool empty() const {return m_size == 0;}

private:
	unsigned char *m_data{};
	std::size_t m_size{};
};

} // end of namespace hrb
