/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/8/18.
//

#include "JsonHelper.hh"

namespace hrb {
namespace json {

std::string_view string_view(const rapidjson::Value& value)
{
	return {value.GetString(), value.GetStringLength()};
}

std::string string(const rapidjson::Value& value)
{
	return std::string{string_view(value)};
}

}} // end of namespace
