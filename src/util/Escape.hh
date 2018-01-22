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

template <typename Callback>
void visit_form_string(std::string_view remain, Callback&& callback)
{
	while (!remain.empty())
	{
		auto end  = remain.find_first_of("=;&");
		auto name = remain.substr(0, end);
		std::string_view value{};

		if (end != remain.npos)
		{
			auto sep = remain[end];
			end++;

			if (sep == '=' && end < remain.size())
			{
				auto end2 = remain.find_first_of(";&", end);
				value =  remain.substr(end, end2-end);
				end   += value.size();

				if (end2 != remain.npos)
					end++;
			}
		}
		else
			end = name.size();

		callback(name, value);
		remain.remove_prefix(end);
	}

}

} // end of namespace
