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

#include <blake2.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace hrb {

void system_random(void *buf, std::size_t size);
void user_random(void *buf, std::size_t size);
void insecure_random(void *buf, std::size_t size);

template <typename T, typename Generator>
T random_value(Generator gen)
{
	static_assert(std::is_standard_layout<T>::value);

	T val{};
	gen(&val, sizeof(val));
	return val;
}

template <typename T>
T random_value()
{
	return random_value<T>(user_random);
}

template <typename T, typename Generator>
T randomize(T& val, Generator gen)
{
	static_assert(std::is_standard_layout<T>::value);
	gen(&val, sizeof(val));
	return val;
}

template <typename T>
T randomize(T& val)
{
	static_assert(std::is_standard_layout<T>::value);
	return randomize(val, user_random);
}

template <typename T, std::size_t size> std::array<T,size> random_array()
{
	return random_value<std::array<T, size>>();
}

} // end of namespace hrb
