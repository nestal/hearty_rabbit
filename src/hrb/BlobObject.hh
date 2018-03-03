/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#pragma once

#include "ObjectID.hh"
#include "util/FS.hh"

namespace hrb {

class BlobMeta;

class BlobObject
{
public:
	BlobObject(const fs::path& base, const ObjectID& id);

	BlobMeta meta() const;
	std::string meta_string() const;

private:
	fs::path    m_base;
	ObjectID    m_id;
};

} // end of namespace hrb
