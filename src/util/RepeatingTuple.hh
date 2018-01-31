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

template <typename ResultTuple, std::size_t remain>
constexpr std::size_t match_index()
{
	static_assert(remain < std::tuple_size<ResultTuple>::value);
	return std::tuple_size<ResultTuple>::value - remain - 1;
}

template <typename Field, typename Value, typename ResultTuple>
void match_field(ResultTuple& result, std::string_view name, Value&& value, Field field)
{
	if (name == field)
		std::get<match_index<ResultTuple, 0>()>(result) = std::forward<Value>(value);
}

template <typename... Fields, typename Field, typename Value, typename ResultTuple>
void match_field(ResultTuple& result, std::string_view name, Value&& value, Field field, Fields... remain)
{
	if (name == field)
		std::get<match_index<ResultTuple, sizeof...(remain)>()>(result) = std::forward<Value>(value);
	else
		match_field(result, name, value, remain...);
}

} // end of namespace
