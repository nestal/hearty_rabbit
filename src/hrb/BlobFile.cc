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

#include "image/RotateImage.hh"
#include "image/JPEG.hh"
#include "util/MMap.hh"
#include "util/Log.hh"
#include "util/Magic.hh"

namespace hrb {

namespace {
const std::string master_rendition = "master";
}

BlobFile BlobFile::upload(
	UploadFile&& tmp,
	const Magic& magic,
	const Size2D& resize_img,
	std::string_view filename,
	int quality,
	std::error_code& ec
)
{
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
		RotateImage transform;
		auto rotated = transform.auto_rotate(master.buffer(), ec);

		if (ec)
		{
			// just keep the file as-is if we can't auto-rotate it
			Log(LOG_WARNING, "BlobFile::upload(): cannot rotate image %1% %2%", ec, ec.message());
			ec.clear();
		}

		else
		{
			JPEG img{
				rotated.empty() ? master.buffer().data() : rotated.data(),
				rotated.empty() ? master.size() : rotated.size(),
				resize_img
			};

			if (resize_img != img.size())
				rotated = img.compress(quality);

			std::ostringstream fn;
			fn << resize_img.width() << "x" << resize_img.height();

			result.m_rend.emplace(fn.str(), std::move(rotated));
		}
	}

	// commit result
	result.m_id     = tmp.ID();
	result.m_tmp    = std::move(tmp);
	result.m_master = std::move(master);
	result.m_meta   = CollEntry::create(Permission{}, filename, mime);

	return result;
}

BufferView BlobFile::blob() const
{
	return m_master.buffer();
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

void BlobFile::save(const fs::path& dir, std::error_code& ec) const
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
	m_tmp.move(dir/hrb::master_rendition, ec);

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

BlobFile::BlobFile(const fs::path& dir, const ObjectID& id, std::string_view rendition, std::error_code& ec) :
	m_id{id}
{
	auto resized = dir/std::string{rendition};
	m_master = MMap::open(exists(resized) ? resized : dir/hrb::master_rendition, ec);
}

CollEntry BlobFile::entry() const
{
	return CollEntry{m_meta};
}

std::string_view BlobFile::master_rendition()
{
	return hrb::master_rendition;
}

} // end of namespace hrb
