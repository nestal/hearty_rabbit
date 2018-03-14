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
#include "UploadFile.hh"

#include "image/TurboBuffer.hh"
#include "util/Size2D.hh"
#include "util/FS.hh"
#include "util/MMap.hh"

#include <unordered_map>
#include <system_error>

namespace hrb {

class UploadFile;
class BlobMeta;
class Magic;
class CollEntry;

class BlobFile
{
public:
	BlobFile() = default;
	BlobFile(const fs::path& dir, const ObjectID& id, const Size2D& resize_img, std::error_code& ec);

	static BlobFile upload(
		UploadFile&& tmp,
		const Magic& magic,
		const Size2D& resize_img,
		std::string_view filename,
		int quality,
		std::error_code& ec
	);
	static std::string meta_string(const fs::path& dir);

	BufferView blob() const;
	MMap& master() {return m_master;}

	const ObjectID& ID() const {return m_id;}
	CollEntry entry() const;

	void save(const fs::path& dir, std::error_code& ec) const;

private:
	ObjectID    m_id{};
	std::string m_meta;

	mutable UploadFile  m_tmp;
	MMap                m_master;
	std::unordered_map<std::string, TurboBuffer> m_rend;
};

} // end of namespace hrb
