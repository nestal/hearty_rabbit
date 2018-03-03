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
#include <cstring>
#include <algorithm>
#include <random>

namespace hrb {

void secure_random(void *buf, std::size_t size);

/// Fill a buffer with random bytes generated by a
/// [UniformRandomBitGenerator](http://en.cppreference.com/w/cpp/concept/UniformRandomBitGenerator)
template <typename URBG>
void fill_buffer(void *buf, std::size_t size, URBG&& gen)
{
	for (std::size_t i = 0 ; i < size ; i += sizeof(gen()))
	{
		auto rand = gen();
		std::memcpy(
			static_cast<char*>(buf) + i,
			&rand,
			std::min(sizeof(rand), size-i)
		);
	}
}

/// Specialization for std::random_device that calls secure_random() directory
template <>
void fill_buffer(void *buf, std::size_t size, std::random_device&&);

template <typename T, typename Random, class=std::enable_if_t<std::is_standard_layout<T>::value, T>>
auto random_value(T& val, Random&& gen)
{
	fill_buffer(&val, sizeof(val), std::forward<Random>(gen));
	return val;
}

template <typename T, typename Random>
std::enable_if_t<std::is_standard_layout<T>::value, T> random_value(Random&& gen)
{
	T val;
	return random_value(val, std::forward<Random>(gen));
}

template <typename T>
std::enable_if_t<std::is_standard_layout<T>::value, T> secure_random()
{
	T val;
	secure_random(&val, sizeof(val));
	return val;
}

template <typename T, class=std::enable_if_t<std::is_standard_layout<T>::value, T>>
auto secure_random(T& t)
{
	secure_random(&t, sizeof(t));
	return t;
}

template <typename T, std::size_t size, typename Random>
std::enable_if_t<std::is_standard_layout<T>::value, std::array<T, size>> random_array(Random&& gen)
{
	return random_value<std::array<T, size>>(std::forward<Random>(gen));
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
	Blake2x() noexcept ;
	Blake2x(Blake2x&&) = default;
	Blake2x(const Blake2x&) = default;
	~Blake2x() = default;

	Blake2x& operator=(const Blake2x&) = default;
	Blake2x& operator=(Blake2x&&) = default;

	using result_type = std::uint64_t;
	static constexpr result_type min() {return std::numeric_limits<result_type>::min();}
	static constexpr result_type max() {return std::numeric_limits<result_type>::max();}

	result_type operator()();

	void reseed();

	// per-thread instance
	static Blake2x& instance();

private:
	::blake2b_param m_param{};
	std::uint64_t m_h0{};
};

void insecure_random(void *buf, std::size_t size);

template <typename T>
std::enable_if_t<std::is_standard_layout<T>::value, T> insecure_random()
{
	return random_value<T>(std::move(Blake2x::instance()));
}

template <typename T, class=std::enable_if_t<std::is_standard_layout<T>::value, T>>
auto insecure_random(T& t)
{
	return random_value<T>(t, std::move(Blake2x::instance()));
}

} // end of namespace hrb
