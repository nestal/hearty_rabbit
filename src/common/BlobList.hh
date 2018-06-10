/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/10/18.
//

#pragma once

#include <json.hpp>

namespace hrb {

class BlobList
{
public:
	BlobList() = default;

private:
	nlohmann::json m_json;
};

} // end of namespace hrb
