/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

#include <string>
#include <string_view>

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

		callback(name, value);
	}
}

} // end of namespace
