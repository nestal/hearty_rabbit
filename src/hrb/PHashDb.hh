/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#pragma once

#include "ObjectID.hh"

namespace hrb {
namespace redis {
class Connection;
}

class PHash;

/// \brief Encapsulate the redis sorted set for storing phashes
class PHashDb
{
public:
	explicit PHashDb(redis::Connection& db);

	void add(const ObjectID& blob, PHash phash);

private:
	static const std::string_view m_key;

private:
	redis::Connection&  m_db;
};

} // end of namespace hrb
