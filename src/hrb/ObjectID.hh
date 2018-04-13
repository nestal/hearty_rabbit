/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 12 Feb 2018.
//

#pragma once

#include "crypto/Blake2.hh"

#include <msgpack.hpp>

#include <array>
#include <iosfwd>
#include <string_view>
#include <type_traits>
#include <optional>

namespace hrb {

// If use a typedef (or using), then the argument-dependent lookup (ADL) will not
// work for operator<<
struct ObjectID : std::array<unsigned char, Blake2::size>
{
	using array::array;
	explicit ObjectID(const array& ar) : array{ar} {}
};

static_assert(std::is_standard_layout<ObjectID>::value);

std::string to_hex(const ObjectID& id);
std::string to_quoted_hex(const ObjectID& id, char quote = '\"');
std::optional<ObjectID> hex_to_object_id(std::string_view hex);
bool is_valid_blob_id(std::string_view hex);

std::optional<ObjectID> raw_to_object_id(std::string_view raw);

std::ostream& operator<<(std::ostream& os, const ObjectID& id);

} // end of namespace

namespace msgpack {
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS) {
namespace adaptor {

// Place class template specialization here
template<>
struct convert<hrb::ObjectID> {
	msgpack::object const& operator()(msgpack::object const& o, hrb::ObjectID& v) const {
		if (o.type != msgpack::type::STR)
			throw msgpack::type_error();
		if (o.via.str.size != v.size())
			throw msgpack::type_error();

		std::memcpy(&v[0], o.via.str.ptr, v.size());
		return o;
	}
};

template<>
struct pack<hrb::ObjectID>
{
	template <typename Stream>
	packer<Stream>& operator()(msgpack::packer<Stream>& o, const hrb::ObjectID& v) const
    {
		// packing member variables as an array.
	    o.pack_str(v.size());
	    o.pack_str_body(reinterpret_cast<const char*>(&v[0]), v.size());
		return o;
	}
};

}}}
