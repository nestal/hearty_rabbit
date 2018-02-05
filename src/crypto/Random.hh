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

// Blake2x random number generator: https://blake2.net/blake2x.pdf
class Blake2x
{
public:
	Blake2x();

	using result_type = std::uint64_t;
	static constexpr result_type min() {return std::numeric_limits<result_type>::min();}
	static constexpr result_type max() {return std::numeric_limits<result_type>::max();}

	result_type operator()();

private:
	::blake2b_param m_param{};
	std::uint64_t m_h0{secure_random<decltype(m_h0)>()};
};

} // end of namespace hrb
