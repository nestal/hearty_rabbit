/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/10/18.
//

#pragma once

#include "ObjectID.hh"

#include <string_view>

namespace hrb {
namespace redis {
class Connection;
}

class BlobTable
{
public:
	BlobTable(std::string_view user, const ObjectID& blob);

	void watch(redis::Connection& db) const;

	// expect to be done inside a transaction
	void add_link(redis::Connection& db, std::string_view mime, std::string_view path) const;

	const std::string& user() const {return m_user;}
	const ObjectID& blob() const {return m_blob;}

private:
	std::string m_user;
	ObjectID    m_blob;
};

} // end of namespace hrb
