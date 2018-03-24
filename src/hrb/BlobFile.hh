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
class CollEntry;
class Magic;
class CollEntry;
class RenditionSetting;

class BlobFile
{
public:
	BlobFile() = default;
	BlobFile(const fs::path& dir, const ObjectID& id, std::string_view rendition, const RenditionSetting& cfg, std::error_code& ec);

	static BlobFile upload(
		UploadFile&& tmp,
		const Magic& magic,
		const RenditionSetting& cfg,
		std::string_view filename,
		std::error_code& ec
	);

	BufferView blob() const;
	MMap& mmap() {return m_mmap;}

	const ObjectID& ID() const {return m_id;}
	CollEntry entry() const;

	void save(const fs::path& dir, std::error_code& ec) const;

private:
	static TurboBuffer generate_rendition(BufferView master, std::string_view rend, Size2D dim, int quality, std::error_code& ec);

private:
	ObjectID    m_id{};
	std::string m_meta;

	mutable UploadFile  m_tmp;
	MMap                m_mmap;
	std::unordered_map<std::string, TurboBuffer> m_rend;
};

} // end of namespace hrb
