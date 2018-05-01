/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 1/15/18.
//

#include "ObjectID.hh"

#include "crypto/Blake2.hh"
#include "util/Error.hh"
#include "util/Escape.hh"

#include <boost/algorithm/hex.hpp>
#include <openssl/evp.h>

#include <cstring>
#include <cassert>
#include <fstream>
#include <iomanip>

namespace hrb {

std::optional<ObjectID> hex_to_object_id(std::string_view hex)
{
	ObjectID result{};
	try
	{
		if (hex.size() == result.size()*2)
		{
			boost::algorithm::unhex(hex.begin(), hex.end(), result.begin());
			return result;
		}
	}
	catch (boost::algorithm::hex_decode_error&)
	{
	}
	return std::nullopt;
}

std::optional<ObjectID> raw_to_object_id(std::string_view raw)
{
	ObjectID result{};
	if (raw.size() == result.size())
	{
		std::copy(raw.begin(), raw.end(), result.begin());
		return result;
	}
	else
	{
		return std::nullopt;
	}
}

bool operator==(const ObjectID& id1, const ObjectID& id2)
{
	static_assert(id1.size() == id2.size());    // isn't it obvious?
	return std::memcmp(id1.data(), id2.data(), id1.size()) == 0;
}

bool operator!=(const ObjectID& id1, const ObjectID& id2)
{
	static_assert(id1.size() == id2.size());    // isn't it obvious?
	return std::memcmp(id1.data(), id2.data(), id1.size()) != 0;
}

std::ostream& operator<<(std::ostream& os, const ObjectID& id)
{
	return os << to_hex(id);
}

bool is_valid_blob_id(std::string_view hex)
{
	return hex.size() == Blake2::size*2 && hex.find_first_not_of("01234567890ABCDEFabcdef") == hex.npos;
}

} // end of namespace
