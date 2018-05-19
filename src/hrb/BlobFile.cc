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
#include "UploadFile.hh"

// HeartyRabbit headers
#include "image/PHash.hh"
#include "image/EXIF2.hh"
#include "util/MMap.hh"
#include "util/Log.hh"
#include "util/Magic.hh"
#include "util/Configuration.hh"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

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
		Log(LOG_WARNING, "create create directory %1% (%2% %3%)", dir, bec, bec.message());
		ec.assign(bec.value(), bec.category());
	}
	else
	{
		// Try moving the temp file to our destination first. If failed, use
		// deep copy instead.
		tmp.move(m_dir / hrb::master_rendition, ec);
	}
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

BlobFile::BlobFile(const fs::path& dir, const ObjectID& id) :
	m_id{id}, m_dir{dir}
{
}

MMap BlobFile::rendition(std::string_view rendition, const RenditionSetting& cfg, std::error_code& ec)
{
	// check if rendition is allowed by config
	if (!cfg.valid(rendition) && rendition != hrb::master_rendition)
		rendition = cfg.default_rendition();

	auto rend_path = m_dir/std::string{rendition};

	// generate the rendition if it doesn't exist
	if (rendition != hrb::master_rendition && !exists(rend_path))
	{
		if (m_mime.empty())
			m_mime = Magic{}.mime(m_dir/hrb::master_rendition);

		if (!m_mime.empty() && m_mime.substr(0, 5) == "image")
			generate_rendition_from_image(cfg.find(rendition), rend_path, ec);
	}

	return MMap::open(exists(rend_path) ? rend_path : m_dir/hrb::master_rendition, ec);
}

void BlobFile::generate_rendition_from_image(const JPEGRenditionSetting& cfg, const fs::path& dest, std::error_code& ec) const
{
	try
	{
		auto jpeg = cv::imread((m_dir/hrb::master_rendition).string(), cv::IMREAD_ANYCOLOR);
		if (!jpeg.empty())
		{
			auto xratio = cfg.dim.width() / static_cast<double>(jpeg.cols);
			auto yratio = cfg.dim.height() / static_cast<double>(jpeg.rows);

			cv::Mat out;
			if (xratio < 1.0 || yratio < 1.0)
				cv::resize(jpeg, out, {}, std::min(xratio, yratio), std::min(xratio, yratio), cv::INTER_LINEAR);
			else
				out = jpeg;

			std::vector<unsigned char> out_buf;
			cv::imencode(m_mime == "image/png" ? ".png" : ".jpg", out, out_buf, {cv::IMWRITE_JPEG_QUALITY, cfg.quality});
			save_blob(out_buf, dest, ec);
		}
	}
	catch (...)
	{
	}
}

MMap BlobFile::master(std::error_code& ec) const
{
	return MMap::open(m_dir/hrb::master_rendition, ec);
}

} // end of namespace hrb
