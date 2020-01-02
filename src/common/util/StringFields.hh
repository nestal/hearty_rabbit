/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#pragma once

#include "Escape.hh"

namespace hrb {

class StringFields
{
public:
	constexpr StringFields(std::string_view all_delims, std::string_view kv_delims, bool trim_space=false) :
		m_all_delims{all_delims}, m_kv_delims{kv_delims}, m_trim_space{trim_space}
	{
	}

	template <typename OutType, typename... Fields>
	auto basic_find(std::string_view remain, Fields... fields) const
	{
		typename RepeatingTuple<OutType, sizeof...(fields)>::type result;
		while (!remain.empty())
		{
			// Don't remove the temporary variables because the order
			// of execution in function parameters is undefined.
			// i.e. don't change to match_field(result, split_front("=;&"), split_front(";&"), fields...)
			auto [name, match]  = split_left(remain, m_all_delims);
			auto value = (match == '=' ? std::get<0>(split_left(remain, m_kv_delims)) : std::string_view{});

			// only trim spaces for "name" before matching. never modify "value"
			while (m_trim_space && !name.empty() && std::isspace(name.front()))
				name.remove_prefix(1);
			while (m_trim_space && !name.empty() && std::isspace(name.back()))
				name.remove_suffix(1);

			match_field(result, name, value, fields...);
		}
		return result;
	}

	template <typename... Fields>
	auto find(std::string_view remain, Fields... fields) const
	{
		return basic_find<std::string_view>(remain, std::forward<Fields>(fields)...);
	}

	template <typename... Fields>
	auto find_optional(std::string_view remain, Fields... fields) const
	{
		return basic_find<std::optional<std::string_view>>(remain, std::forward<Fields>(fields)...);
	}

private:
	std::string_view m_all_delims;
	std::string_view m_kv_delims;
	bool m_trim_space;
};
constexpr StringFields urlform{"=;&", ";&"};

} // end of namespace
