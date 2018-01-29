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

#include <boost/exception/info.hpp>
#include <boost/throw_exception.hpp>
#include <rapidjson/pointer.h>

namespace hrb {
namespace json {

const rapidjson::Value& required(const rapidjson::Value& object, std::string_view field)
{
	auto val = rapidjson::Pointer{field.data(), field.size()}.Get(object);
	if (!val)
		BOOST_THROW_EXCEPTION(Error() << MissingField{std::string{field}});

	return *val;
}

std::string_view string_view(const rapidjson::Value& value)
{
	if (!value.IsString())
		BOOST_THROW_EXCEPTION(NotString());

	return {value.GetString(), value.GetStringLength()};
}

std::string string(const rapidjson::Value& value)
{
	return std::string{string_view(value)};
}

}} // end of namespace
