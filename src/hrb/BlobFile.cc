/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/3/18.
//

#include "BlobFile.hh"
#include "Ownership.hh"
#include "Permission.hh"

// HeartyRabbit headers
#include "image/RotateImage.hh"
#include "image/JPEG.hh"
#include "image/PHash.hh"
#include "util/MMap.hh"
#include "util/Log.hh"
#include "util/Magic.hh"
#include "util/Configuration.hh"

namespace hrb {

namespace {
const std::string master_rendition = "master";
}

BlobFile::BlobFile(
	UploadFile&& tmp,
	const fs::path& dir,
	const Magic& magic,
	std::error_code& ec
) : m_id{tmp.ID()}, m_dir{dir}
{
	// Note: closing the file before munmap() is OK: the mapped memory will still be there.
	// Details: http://pubs.opengroup.org/onlinepubs/7908799/xsh/mmap.html
	auto master = MMap::open(tmp.native_handle(), ec);
	if (ec)
	{
		Log(LOG_WARNING, "BlobFile::upload(): cannot mmap temporary file %1% %2%", ec, ec.message());
		return;
	}

	// commit result
	m_mime   = magic.mime(master.blob());
	m_phash  = hrb::phash(master.buffer());

	boost::system::error_code bec;
	fs::create_directories(dir, bec);
	if (bec)
	{
		Log(LOG_WARNING, "create create directory %1% (%2% %3%)", dir, ec, ec.message());
		return;
	}

	// Try moving the temp file to our destination first. If failed, use
	// deep copy instead.
	tmp.move(m_dir/hrb::master_rendition, ec);
}

template <typename Blob>
void save_blob(const Blob& blob, const fs::path& dest, std::error_code& ec)
{
	assert(blob.data());
	assert(blob.size() > 0);

	boost::system::error_code bec;
	boost::beast::file file;
	file.open(dest.string().c_str(), boost::beast::file_mode::write, bec);
	if (bec)
	{
		Log(LOG_WARNING, "save_blob(): cannot open file %1% (%2% %3%)", dest, bec, bec.message());
	}
	file.write(blob.data(), blob.size(), bec);
	if (bec)
	{
		Log(LOG_WARNING, "save_blob(): cannot write to file %1% (%2% %3%)", dest, bec, bec.message());
		ec.assign(bec.value(), bec.category());
	}
	else
		ec.assign(0, ec.category());
}

BlobFile::BlobFile(const fs::path& dir, const ObjectID& id) : m_id{id}, m_dir{dir}
{
}

MMap BlobFile::rendition(std::string_view rendition, const RenditionSetting& cfg, std::error_code& ec) const
{
	// check if rendition is allowed by config
	if (!cfg.valid(rendition) && rendition != hrb::master_rendition)
		rendition = cfg.default_rendition();

	auto rend_path = m_dir/std::string{rendition};

	// generate the rendition if it doesn't exist
	if (rendition != hrb::master_rendition && !exists(rend_path) && m_mime == "image/jpeg")
	{
		auto master = MMap::open(m_dir/hrb::master_rendition, ec);
		if (!ec)
		{
			auto tb = generate_rendition_from_jpeg(
				master.buffer(),
				cfg.dimension(rendition),
				cfg.quality(rendition),
				ec
			);
			if (!tb.empty())
				save_blob(tb, rend_path, ec);
		}
	}

	return MMap::open(exists(rend_path) ? rend_path : m_dir/hrb::master_rendition, ec);
}

TurboBuffer BlobFile::generate_rendition_from_jpeg(BufferView jpeg_master, Size2D dim, int quality, std::error_code& ec)
{
	RotateImage transform;
	auto rotated = transform.auto_rotate(jpeg_master, ec);

	if (!ec)
	{
		JPEG img{
			rotated.empty() ? jpeg_master.data() : rotated.data(),
			rotated.empty() ? jpeg_master.size() : rotated.size(),
			dim
		};

		if (dim != img.size())
			rotated = img.compress(quality);
	}

	return rotated;
}

} // end of namespace hrb
