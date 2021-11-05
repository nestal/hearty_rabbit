/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

#pragma once

#include "RepeatingTuple.hh"

#include <boost/utility/string_view.hpp>
#include <boost/algorithm/hex.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <tuple>

namespace hrb {

template <std::size_t N>
std::string to_hex(const std::array<unsigned char, N>& arr)
{
	std::string result(arr.size()*2, '\0');
	boost::algorithm::hex_lower(arr.begin(), arr.end(), result.begin());
	return result;
}

template <std::size_t N>
std::string to_quoted_hex(const std::array<unsigned char, N>& id, char quote = '\"')
{
	std::string result(id.size()*2 + 2, quote);
	boost::algorithm::hex_lower(id.begin(), id.end(), result.begin()+1);
	return result;
}

template <std::size_t N>
std::optional<std::array<unsigned char, N>> hex_to_array(std::string_view hex)
{
	try
	{
		std::array<unsigned char, N> result;
		if (hex.size() == result.size()*2)
		{
			boost::algorithm::unhex(hex.begin(), hex.end(), result.begin());
			return result;
		}
	}
	catch (boost::algorithm::hex_decode_error&)
	{
	}
	return std::nullopt;
}

std::vector<unsigned char> hex_to_vector(std::string_view hex);

std::string url_encode(std::string_view in);

std::string url_decode(std::string_view in);

std::tuple<std::string_view, char> split_left(std::string_view& in, std::string_view value);
std::tuple<std::string_view, char> split_right(std::string_view& in, std::string_view value);
std::string_view split_front_substring(std::string_view& in, std::string_view substring);

template <std::size_t index, typename ResultTuple>
void parse_token(std::string_view& remain, std::string_view value, ResultTuple& tuple)
{
	static_assert(index < std::tuple_size<ResultTuple>::value);
	static_assert(std::is_same<std::tuple_element_t<index, ResultTuple>, std::string_view>::value);
	std::get<index>(tuple) = std::get<0>(split_left(remain, value));

	if constexpr (index + 1 < std::tuple_size<ResultTuple>::value)
		parse_token<index+1>(remain, value, tuple);
}

template <std::size_t count>
auto tokenize(std::string_view remain, std::string_view value)
{
	static_assert(count > 0);

	typename RepeatingTuple<std::string_view, count>::type result;
	parse_token<0>(remain, value, result);
	return result;
}

template <std::size_t count>
auto tokenize(boost::string_view remain, boost::string_view value)
{
	return tokenize<count>(
		std::string_view{remain.data(), remain.size()},
		std::string_view{value.data(), value.size()}
	);
}

} // end of namespace
template <std::size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<unsigned char, N>& id)
{
	return os << hrb::to_hex(id);
}
