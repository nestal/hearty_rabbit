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
#include "util/Configuration.hh"
#include "util/Escape.hh"
#include "util/Log.hh"
#include "util/Magic.hh"
#include "util/MMap.hh"

// JSON for saving meta data
#include <json.hpp>

// OpenCV for calculating phash
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace hrb {

namespace {
const std::string master_rendition = "master";
}

/// \brief Open an existing blob in its directory
BlobFile::BlobFile(const fs::path& dir, const ObjectID& id) : m_id{id}, m_dir{dir}
{
}

/// \brief Creates a new blob from a uploaded file
BlobFile::BlobFile(UploadFile&& tmp, const fs::path& dir, std::error_code& ec)  : m_id{tmp.ID()}, m_dir{dir}
{
	// Note: closing the file before munmap() is OK: the mapped memory will still be there.
	// Details: http://pubs.opengroup.org/onlinepubs/7908799/xsh/mmap.html
	auto master = MMap::open(tmp.native_handle(), ec);
	if (ec)
	{
		Log(LOG_WARNING, "BlobFile::upload(): cannot mmap temporary file %1% %2%", ec, ec.message());
		return;
	}

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

		// deduce meta data from the uploaded file
		deduce_meta(master.buffer());
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

MMap BlobFile::rendition(std::string_view rendition, const RenditionSetting& cfg, std::error_code& ec) const
{
	if (rendition == hrb::master_rendition)
		return master(ec);

	// check if rendition is allowed by config
	if (!cfg.valid(rendition))
		rendition = cfg.default_rendition();

	auto rend_path = m_dir/std::string{rendition};

	// generate the rendition if it doesn't exist
	if (!exists(rend_path) && is_image())
		generate_image_rendition(cfg.find(rendition), rend_path, ec);

	return exists(rend_path) ? MMap::open(rend_path, ec) : master(ec);
}

void BlobFile::generate_image_rendition(const JPEGRenditionSetting& cfg, const fs::path& dest, std::error_code& ec) const
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

bool BlobFile::is_image(std::string_view mime)
{
	std::string_view image{"image"};
	return !mime.empty() && mime.substr(0, image.size()) == image;
}

bool BlobFile::is_image() const
{
	// Must update meta before using the meta data
	update_meta();
	return is_image(m_mime);
}

std::optional<PHash> BlobFile::phash() const
{
	update_meta();
	return m_phash;
}

std::string_view BlobFile::mime() const
{
	update_meta();
	return m_mime;
}

void BlobFile::update_meta() const
{
	// m_mime is empty means the meta data is missing.
	// if we don't know what the mime type, it will be application/octets-stream
	if (m_mime.empty())
	{
		// try to load it from meta.json
		std::ifstream meta_file{(m_dir/"meta.json").string()};
		nlohmann::json meta;

		try
		{
			if (meta_file)
				meta_file >> meta;
		}
		catch (nlohmann::json::exception& e)
		{
			Log(LOG_WARNING, "json parse error @ file %1%: %2%", m_dir, e.what());
			meta_file.setstate(std::ios::failbit);
		}

		if (meta_file)
		{
			m_mime = meta["mime"];
			if (meta.find("phash") != meta.end())
				m_phash = PHash{meta["phash"].get<std::uint64_t>()};
			else
				m_phash = std::nullopt;
		}
		// json file missing, we need to deduce the meta
		else
		{
			std::error_code ec;
			auto master = this->master(ec);
			if (ec)
				Log(LOG_WARNING, "cannot load master rendition from blob %1%", to_hex(m_id));
			else
				deduce_meta(master.buffer());
		}
	}
}

void BlobFile::deduce_meta(BufferView master) const
{
	m_mime = Magic::instance().mime(master);
	if (is_image(m_mime))
		m_phash  = hrb::phash(master);

	// save the meta data to file
	nlohmann::json meta{
		{"mime", m_mime}
	};
	if (m_phash)
		meta.emplace("phash", m_phash->value());

	Log(LOG_WARNING, "writing meta to file %1%", m_dir);

	std::ofstream meta_file{(m_dir/"meta.json").string()};
	meta_file << meta;
}

} // end of namespace hrb
