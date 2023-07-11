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
#include <cstring>
#include <algorithm>
#include <limits>
#include <cstdint>

namespace hrb {

void secure_random(void* buf, std::size_t size);
void insecure_random(void* buf, std::size_t size);

template <typename Function>
concept Randomizer = requires (Function function, void* buf, std::size_t size)
{
	function(buf, size);
};

// Simple wrapper for selecting secure and insecure version of random generation.
template <typename T>
requires std::is_standard_layout_v<T>
T secure_random()
{
	T t;
	secure_random(&t, sizeof(t));
	return t;
}

template <typename T>
requires std::is_standard_layout_v<T>
T insecure_random()
{
	T t;
	insecure_random(&t, sizeof(t));
	return t;
}

template <typename T, std::size_t size> std::array<T,size> secure_random_array()
{
	return secure_random<std::array<T,size>>();
}
template <typename T, std::size_t size> std::array<T,size> insecure_random_array()
{
	return insecure_random<std::array<T,size>>();
}

} // end of namespace hrb
