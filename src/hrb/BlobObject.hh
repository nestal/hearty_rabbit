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
#include "BlobMeta.hh"
#include "UploadFile.hh"

#include "image/TurboBuffer.hh"
#include "util/Size.hh"
#include "util/FS.hh"
#include "util/MMap.hh"

#include <unordered_map>
#include <system_error>

namespace hrb {

class UploadFile;
class Magic;

class BlobObject
{
public:
	BlobObject() = default;
	BlobObject(const fs::path& dir, const ObjectID& id);

	static BlobObject upload(UploadFile&& tmp, const Magic& magic, const Size& resize_img, std::error_code& ec);

	const BlobMeta& meta() const;

	BufferView blob() const;

	const ObjectID& ID() const {return m_id;}

	void save(const fs::path& dir, std::error_code& ec) const;

private:
	ObjectID    m_id{};
	BlobMeta    m_meta;

	mutable UploadFile  m_tmp;
	MMap        m_master;
	std::unordered_map<std::string, TurboBuffer> m_rend;
};

} // end of namespace hrb
