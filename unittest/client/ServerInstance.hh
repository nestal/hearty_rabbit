/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/5/18.
//

#pragma once

#include <memory>

namespace hrb {

class ServerInstance
{
public:
	ServerInstance();
	~ServerInstance();

	void run();

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};

} // end of namespace hrb
