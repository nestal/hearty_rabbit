/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/17/18.
//

#pragma once

#include "ObjectID.hh"
#include "util/FS.hh"

#include <boost/beast/core/file_posix.hpp>

namespace hrb {

class ObjectID;
class BlobObject;
class UploadFile;

class BlobDatabase
{
public:
	explicit BlobDatabase(const fs::path& base);

	void prepare_upload(UploadFile& result, std::error_code& ec) const;
	ObjectID save(UploadFile&& tmp, std::error_code& ec);

	fs::path dest(ObjectID id) const;

private:
	fs::path    m_base;
};

} // end of namespace hrb
