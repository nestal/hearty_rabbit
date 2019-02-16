/*
	Copyright © 2019 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/16/19.
//

#include "RedisKeys.hh"
#include "common/ObjectID.hh"

namespace hrb::key {

using namespace std::literals;

std::string blob_refs(std::string_view user, const ObjectID& blob)
{
	std::string s{"blob-refs:"};
	s.append(user.data(), user.size());
	s.push_back(':');
	s.append(reinterpret_cast<const char*>(blob.data()), blob.size());
	return s;
}

std::string blob_owners(const ObjectID& blob)
{
	std::string s{"blob-owners:"};
	s.append(reinterpret_cast<const char*>(blob.data()), blob.size());
	return s;
}

std::string collection(std::string_view user, std::string_view coll)
{
	std::string s{"coll:"};
	s.append(user.data(), user.size());
	s.push_back(':');
	s.append(coll.data(), coll.size());
	return s;
}

std::string collection_list(std::string_view user)
{
	std::string s{"colls:"};
	s.append(user.data(), user.size());
	return s;
}

std::string public_blobs(std::string_view user)
{
	std::string s{"public_blobs:"};
	s.append(user.data(), user.size());
	return s;
}

std::string blob_meta(std::string user, const ObjectID& blob)
{
	std::string s{"blob-meta:"};
	s.append(user.data(), user.size());
	s.push_back(':');
	s.append(reinterpret_cast<const char*>(blob.data()), blob.size());
	return s;
}

} // end of namespace
