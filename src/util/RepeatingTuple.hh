/////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Thomson Reuters Limited
// This document contains information proprietary to Thomson Reuters Limited, and
// may not be reproduced, disclosed or used in whole or in part without
// the express written permission of Thomson Reuters Limited.
//
/////////////////////////////////////////////////////////////////////////

#pragma once

#include <tuple>
#include <utility>

namespace hrb {

template<typename Dependent, std::size_t>
using DependOn = Dependent;

template<typename T, std::size_t N, typename Indices = std::make_index_sequence<N>>
struct RepeatingTuple;

template<typename T, std::size_t N, std::size_t... Indices>
struct RepeatingTuple<T, N, std::index_sequence<Indices...>>
{
    using type = std::tuple<DependOn<T, Indices>...>;
};

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

} // end of namespace
