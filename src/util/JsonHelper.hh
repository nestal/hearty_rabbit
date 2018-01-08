/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/8/18.
//

#pragma once

#include <rapidjson/document.h>

#include <string_view>
#include <string>

namespace hrb {
namespace json {

std::string_view string_view(const rapidjson::Value& value);
std::string string(const rapidjson::Value& value);

}} // end of namespace
