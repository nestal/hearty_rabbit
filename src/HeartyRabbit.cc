/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/11/2021.
//

#include "HeartyRabbit.hh"
#include "crypto/Password.hh"
#include "net/Redis.hh"
#include "util/MMap.hh"

#include <nlohmann/json.hpp>
#include <iostream>

namespace hrb {

void HeartyRabbitServer::login(
	std::string_view user_name, const Password& password,
	std::function<void(std::error_code)> on_complete
)
{
	auto user_dir = m_root / user_name;

	std::error_code ec;
	auto passwd = MMap::open(user_dir/"passwd.json", ec);
	if (ec)
	{
		on_complete(ec);
		return;
	}

	auto json = nlohmann::json::parse(passwd.string());
	if (json["password"].get<std::string>() == password.get())
	{
		on_complete({});
	}
}

} // end of namespace hrb

int main(int argc, char** argv)
{
	hrb::HeartyRabbitServer srv{std::filesystem::current_path()};
	srv.login("user", hrb::Password{"abc"}, [](auto ec)
	{
		std::cout << ec.message() << std::endl;
	});

	boost::asio::io_context ios;

	ios.run();
	return -1;
}
