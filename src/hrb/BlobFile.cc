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
#include "CollEntry.hh"
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

BlobFile BlobFile::upload(
	UploadFile&& tmp,
	const fs::path& dir,
	const Magic& magic,
	const RenditionSetting& cfg,
	std::string_view filename,
	std::error_code& ec
)
{
	// Note: closing the file before munmap() is OK: the mapped memory will still be there.
	// Details: http://pubs.opengroup.org/onlinepubs/7908799/xsh/mmap.html
	BlobFile result;
	auto master = MMap::open(tmp.native_handle(), ec);
	if (ec)
	{
		Log(LOG_WARNING, "BlobFile::upload(): cannot mmap temporary file %1% %2%", ec, ec.message());
		return result;
	}

	auto mime = magic.mime(master.blob());

	if (mime == "image/jpeg")
	{
		// generate default rendition
		auto rotated = generate_rendition_from_jpeg(
			master.buffer(),
			cfg.dimension(cfg.default_rendition()),
			cfg.quality(cfg.default_rendition()),
			ec
		);

		if (ec)
		{
			// just keep the file as-is if we can't auto-rotate it
			Log(LOG_WARNING, "BlobFile::upload(): cannot rotate image %1% %2%", ec, ec.message());
			ec.clear();
		}

		else if (!rotated.empty())
		{
			result.m_rend.emplace(cfg.default_rendition(), std::move(rotated));
		}
	}

	// commit result
	result.m_id     = tmp.ID();
	result.m_phash  = hrb::phash(master.buffer());
	result.m_mmap   = std::move(master);
	result.m_coll_entry   = CollEntry::create(Permission{}, filename, mime);

	if (!ec)
		result.save(tmp, dir, ec);

	return result;
}

BufferView BlobFile::buffer() const
{
	return m_mmap.buffer();
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

void BlobFile::save(UploadFile& tmp, const fs::path& dir, std::error_code& ec) const
{
	boost::system::error_code bec;
	fs::create_directories(dir, bec);
	if (bec)
	{
		Log(LOG_WARNING, "create create directory %1% (%2% %3%)", dir, ec, ec.message());
		return;
	}

	// Try moving the temp file to our destination first. If failed, use
	// deep copy instead.
	tmp.move(dir/hrb::master_rendition, ec);

	// Save the renditions, if any.
	for (auto&& [name, rend] : m_rend)
	{
		save_blob(rend, dir / name, ec);
		if (ec)
		{
			Log(LOG_WARNING, "cannot save blob rendition %1% (%2% %3%)", name, ec, ec.message());
			break;
		}
	}
}

BlobFile::BlobFile(
	const fs::path& dir,
	const ObjectID& id,
	std::string_view rendition,
	const RenditionSetting& cfg,
	std::error_code& ec
) :
	m_id{id}
{
	// check if rendition is allowed by config
	if (!cfg.valid(rendition) && rendition != hrb::master_rendition)
		rendition = cfg.default_rendition();

	// generate the rendition if it doesn't exist
	if (rendition != hrb::master_rendition && !exists(dir/std::string{rendition}))
	{
		auto master = MMap::open(dir/hrb::master_rendition, ec);
		if (!ec)
		{
			auto tb = generate_rendition_from_jpeg(
				master.buffer(),
				cfg.dimension(rendition),
				cfg.quality(rendition),
				ec
			);
			if (!tb.empty())
				save_blob(tb, dir / std::string{rendition}, ec);
		}
	}

	auto resized = dir/std::string{rendition};
	m_mmap = MMap::open(exists(resized) ? resized : dir/hrb::master_rendition, ec);
}

CollEntry BlobFile::entry() const
{
	return CollEntry{m_coll_entry};
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
