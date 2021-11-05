/*
	Copyright © 2021 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/11/2021.
//

#include "HeartyRabbit.hh"
#include "crypto/Password.hh"

#include <catch2/catch.hpp>
#include <iostream>

using namespace hrb;

class HeartyRabbitServerFixture
{
public:
	HeartyRabbitServerFixture() = default;

protected:
	boost::asio::io_context m_ios;
	hrb::HeartyRabbitServer m_subject{std::filesystem::current_path(), redis::connect(m_ios)};
};

TEST_CASE_METHOD(HeartyRabbitServerFixture, "HRB2 test cases", "[normal]")
{
	REQUIRE(m_subject.add_user("user", hrb::Password{"abc"}) == std::error_code());

	m_subject.login("user", hrb::Password{"abc"}, [this](auto ec)
	{
		INFO(ec);
		REQUIRE(!ec);
		REQUIRE(!m_subject.auth().id().is_guest());
		REQUIRE(!m_subject.auth().id().is_anonymous());
		REQUIRE(m_subject.auth().id().username() == "user");
	});

	m_ios.run();
}
