/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

#include "RepeatingTuple.hh"

#include <boost/utility/string_view.hpp>

#include <string>
#include <string_view>
#include <tuple>

namespace hrb {

std::string url_encode(std::string_view in);

std::string url_decode(std::string_view in);

std::tuple<std::string_view, char> split_left(std::string_view& in, std::string_view value);
std::tuple<std::string_view, char> split_right(std::string_view& in, std::string_view value);
std::string_view split_front_substring(std::string_view& in, std::string_view substring);

template <typename OutType, typename... Fields>
auto basic_find_fields(std::string_view remain, Fields... fields)
{
	typename RepeatingTuple<OutType, sizeof...(fields)>::type result;
	while (!remain.empty())
	{
		// Don't remove the temporary variables because the order
		// of execution in function parameters is undefined.
		// i.e. don't need change to callback(split_front("=;&"), split_front(";&"))
		auto [name, match]  = split_left(remain, "=;&");
		auto value = (match == '=' ? std::get<0>(split_left(remain, ";&")) : std::string_view{});

		match_field(result, name, value, fields...);
	}
	return result;
}

template <typename... Fields>
auto find_fields(std::string_view remain, Fields... fields)
{
	return basic_find_fields<std::string_view>(remain, std::forward<Fields>(fields)...);
}

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
