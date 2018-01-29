/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

#include <string>
#include <string_view>
#include <tuple>

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

inline std::string_view find_field(std::string_view form_string, std::string_view field)
{
	std::string_view result;
	visit_form_string(form_string, [field, &result](auto name, auto value)
	{
		if (name == field)
		{
			result = value;
			return false;
		}
		else
			return true;
	});
	return result;
}

template <typename... Field>
auto find_fields(std::string_view form_string, Field... fields)
{
	return std::make_tuple(find_field(form_string, fields)...);
}


} // end of namespace
