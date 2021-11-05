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
#include <chrono>

using namespace hrb;

class HeartyRabbitServerFixture
{
public:
	HeartyRabbitServerFixture() = default;

protected:
	boost::asio::io_context m_ios;
	hrb::HeartyRabbitServer m_subject{std::filesystem::current_path(), redis::connect(m_ios)};
};

using namespace std::chrono_literals;

TEST_CASE_METHOD(HeartyRabbitServerFixture, "HRB2 login and verify user", "[normal]")
{
	auto tested = 0;
	REQUIRE(m_subject.add_user("user", hrb::Password{"abc"}) == std::error_code());
	m_subject.login("user", hrb::Password{"abc"}, [this, &tested](auto ec)
	{
		INFO(ec);
		REQUIRE(!ec);
		REQUIRE(!m_subject.auth().id().is_guest());
		REQUIRE(!m_subject.auth().id().is_anonymous());
		REQUIRE(m_subject.auth().id().username() == "user");
		++tested;
	});
	REQUIRE(m_ios.run_for(10s) > 0);
	REQUIRE(tested == 1);
	m_ios.restart();

	// The second time when the same user connects it will be another HeartyRabbitServer to serve her.
	HeartyRabbitServer other{std::filesystem::current_path(), redis::connect(m_ios)};
	other.verify_session(m_subject.auth().session(), 3600s, [this, &tested, &other](auto ec)
	{
		INFO(ec);
		REQUIRE(!ec);
		REQUIRE(other.auth().id().username() == "user");
		REQUIRE(other.auth().session() == m_subject.auth().session());
		tested++;
	});
	REQUIRE(m_ios.run_for(10s) > 0);
	REQUIRE(tested == 2);
}
