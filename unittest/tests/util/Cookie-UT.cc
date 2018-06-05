/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#include "common/Cookie.hh"

#include <catch.hpp>

using namespace hrb;

TEST_CASE("parse expire time from cookie", "[normal]")
{
	hrb::Cookie subject{" id=a3fWa; Expires=Wed, 21 Oct 2015 07:28:00 GMT; Secure; HttpOnly"};
	subject.expires();
}
