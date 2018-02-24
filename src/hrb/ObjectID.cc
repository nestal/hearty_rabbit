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

#include <boost/algorithm/hex.hpp>

#include <openssl/evp.h>

#include <cstring>
#include <cassert>
#include <fstream>
#include <iomanip>

namespace hrb {

std::string to_hex(const ObjectID& id)
{
	std::string result(id.size()*2, '\0');
	boost::algorithm::hex_lower(id.begin(), id.end(), result.begin());
	return result;
}

ObjectID hex_to_object_id(std::string_view hex)
{
	ObjectID result{};
	if (hex.size() == result.size()*2)
		boost::algorithm::unhex(hex.begin(), hex.end(), result.begin());
	return result;
}

ObjectID raw_to_object_id(std::string_view raw)
{
	ObjectID result{};
	if (raw.size() == result.size())
		std::copy(raw.begin(), raw.end(), result.begin());
	return result;
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

} // end of namespace
