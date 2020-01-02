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

#include "common/hrb/ObjectID.hh"
#include "net/Redis.hh"

#include <chrono>
#include <string_view>

namespace hrb {

class TimeIndex
{
public:
	using time_point = std::chrono::system_clock::time_point;

public:
	explicit TimeIndex(redis::Connection& db);

	template <typename TimePoint>
	void add(std::string_view user, const ObjectID& blob, TimePoint tp)
	{
		add(user, blob, std::chrono::time_point_cast<time_point>(tp));
	}

	void add(std::string_view user, const ObjectID& blob, time_point tp);

private:
	static const std::string_view m_key;

private:
	redis::Connection&  m_db;
};

} // end of namespace hrb
