/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

#include <string>
#include <string_view>
#include <tuple>
#include <iostream>

namespace hrb {

std::string url_encode(std::string_view in);

std::string url_decode(std::string_view in);

std::string_view split_front(std::string_view& in, std::string_view value);

template <typename Callback>
void visit_form_string(std::string_view remain, Callback&& callback)
{
	while (!remain.empty())
	{
		// Don't remove the temporary variables because the order
		// of execution in function parameters is undefined.
		// i.e. don't need change to callback(split_front("=;&"), split_front(";&"))
		auto name  = split_front(remain, "=;&");
		auto value = split_front(remain, ";&");

		if (!callback(name, value))
			break;
	}
}

template <typename Field, typename ResultTuple>
void match_field(ResultTuple& result, std::string_view name, std::string_view value, Field field)
{
	static_assert(std::tuple_size<ResultTuple>::value > 0);
	if (name == field)
		std::get<std::tuple_size<ResultTuple>::value-1>(result) = value;
}

template <typename... Fields, typename Field, typename ResultTuple>
void match_field(ResultTuple& result, std::string_view name, std::string_view value, Field field, Fields... remain)
{
	static_assert(sizeof...(remain) < std::tuple_size<ResultTuple>::value);
	if (name == field)
		std::get<std::tuple_size<ResultTuple>::value - sizeof...(remain) - 1>(result) = value;
	else
		match_field(result, name, value, remain...);
}

template<typename Dependent, std::size_t>
using DependOn = Dependent;

template<typename T, std::size_t N, typename Indices = std::make_index_sequence<N>>
struct Repeat;

template<typename T, std::size_t N, std::size_t... Indices>
struct Repeat<T, N, std::index_sequence<Indices...>>
{
    using type = std::tuple<DependOn<T, Indices>...>;
};

template <typename... Fields>
auto find_fields(std::string_view remain, Fields... fields)
{
	typename Repeat<std::string_view, sizeof...(fields)>::type result;
	while (!remain.empty())
	{
		// Don't remove the temporary variables because the order
		// of execution in function parameters is undefined.
		// i.e. don't need change to callback(split_front("=;&"), split_front(";&"))
		auto name  = split_front(remain, "=;&");
		auto value = split_front(remain, ";&");

		match_field(result, name, value, fields...);
	}
	return result;
}

} // end of namespace
