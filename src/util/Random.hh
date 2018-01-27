/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/27/18.
//

#pragma once

#include <array>

namespace hrb {

void random(void *buf, std::size_t size);

template <std::size_t size>
auto random()
{
	std::array<unsigned char, size> buf;
	random(buf.data(), size);
	return buf;
}

} // end of namespace hrb