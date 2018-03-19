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
#include <rapidjson/stringbuffer.h>

#include <string_view>
#include <string>

namespace hrb {
namespace json {

struct Error : virtual Exception {};
struct NotString : virtual Error {};
using MissingField = boost::error_info<struct tag_missing_field, std::string>;

std::string_view string_view(const rapidjson::Value& value);
std::string string(const rapidjson::Value& value);
const rapidjson::Value& required(const rapidjson::Value& object, std::string_view field);

// Note that AddMember(StringRef) does not deep-copy the string.
// Use AddMember(std::string) to deep-copy
template <typename Ch>
rapidjson::GenericStringRef<Ch> string_ref(std::basic_string_view<Ch> sv)
{
	return {sv.data(), static_cast<unsigned>(sv.size())};
}

}} // end of namespace
