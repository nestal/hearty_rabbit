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
#include <type_traits>

namespace hrb {

void secure_random(void *buf, std::size_t size);

template <typename T>
std::enable_if_t<std::is_standard_layout<T>::value, T> secure_random()
{
	T val;
	secure_random(&val, sizeof(val));
	return val;
}

template <typename T, std::size_t size>
std::enable_if_t<std::is_standard_layout<T>::value, std::array<T, size>> secure_random_array()
{
	return secure_random<std::array<T, size>>();
}

} // end of namespace hrb
