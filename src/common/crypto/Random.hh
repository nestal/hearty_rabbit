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
#include <limits>
#include <cstdint>

namespace hrb {

void secure_random(void *buf, std::size_t size);

template <typename T>
struct SecureRandom
{
	using result_type = T;
	constexpr result_type min() const {return std::numeric_limits<T>::min();}
	constexpr result_type max() const {return std::numeric_limits<T>::max();}
	result_type operator()() const
	{
		result_type val;
		secure_random(&val, sizeof(val));
		return val;
	}
};

/// Fill a buffer with random bytes generated by a
/// [UniformRandomBitGenerator](http://en.cppreference.com/w/cpp/concept/UniformRandomBitGenerator)
template <typename URBG>
void random_buffer(void *buf, std::size_t size, URBG&& gen)
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

/// Specialization for SecureRandom that calls secure_random() directly.
template <typename T>
void random_buffer(void *buf, std::size_t size, SecureRandom<T>&&)
{
	secure_random(buf, size);
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

	template <typename URBG>
	void reseed(URBG&& gen);
	void reseed();

	// per-thread instance
	static Blake2x& instance();

private:
	::blake2b_param m_param{};
	std::uint64_t m_h0{};
};

template <typename URBG, typename T>
class RandomAdaptor
{
	static_assert(std::is_standard_layout<T>::value);

public:
	explicit RandomAdaptor(URBG&& gen) : m_gen{gen} {}

	using result_type = T;
	static constexpr result_type min() {return std::numeric_limits<result_type>::min();}
	static constexpr result_type max() {return std::numeric_limits<result_type>::max();}

	/// Fill a buffer with random bytes generated by a
	/// [UniformRandomBitGenerator](http://en.cppreference.com/w/cpp/concept/UniformRandomBitGenerator)
	result_type operator()() const
	{
		result_type result{};
		random_buffer(&result, sizeof(result), std::move(m_gen));
		return result;
	}

	result_type& operator()(result_type& v) const
	{
		random_buffer(&v, sizeof(v), std::move(m_gen));
		return v;
	}

private:
	URBG&  m_gen;
};

template <typename T, typename URBG>
T random_value(URBG&& gen)
{
	return RandomAdaptor<URBG, T>{std::forward<URBG>(gen)}();
}

template <typename T, typename URBG>
T random_value(T& t, URBG&& gen)
{
	return RandomAdaptor<URBG, T>{std::forward<URBG>(gen)}(t);
}

template <typename URBG>
void Blake2x::reseed(URBG&& gen)
{
	random_buffer(m_param.salt, sizeof(m_param.salt), std::forward<URBG>(gen));
	m_h0 = random_value(m_h0, std::forward<URBG>(gen));
}

// Simple wrapper for selecting secure and insecure version of random generation.
template <typename T> T   secure_random()     {return random_value<T>(SecureRandom<T>{});}
template <typename T> T   secure_random(T& t) {return random_value(t, SecureRandom<T>{});}
template <typename T> T insecure_random()     {return random_value<T>(Blake2x::instance());}
template <typename T> T insecure_random(T& t) {return random_value(t, Blake2x::instance());}

template <typename T, std::size_t size> std::array<T,size> secure_random_array()
{
	return random_value<std::array<T, size>>(SecureRandom<T>{});
}
template <typename T, std::size_t size> std::array<T,size> insecure_random_array()
{
	return random_value<std::array<T, size>>(Blake2x::instance());
}


inline void insecure_random(void *buf, std::size_t size) {random_buffer(buf, size, Blake2x::instance());}

} // end of namespace hrb
