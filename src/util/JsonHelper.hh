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

#include "Exception.hh"

#include <rapidjson/document.h>

#include <string_view>
#include <string>

namespace hrb {
namespace json {

struct Error : virtual Exception {};
struct NotString : virtual Error {};
using MissingField = boost::error_info<struct tag_missing_field, std::string>;

std::string_view string_view(const rapidjson::Value& value);
std::string string(const rapidjson::Value& value);
std::uint64_t optional(const rapidjson::Value& object, std::string_view field, std::uint64_t default_value = 0);
const rapidjson::Value& field(const rapidjson::Value& object, std::string_view field);

}} // end of namespace
