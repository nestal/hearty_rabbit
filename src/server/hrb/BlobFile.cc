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
#include "util/Escape.hh"
#include "image/ImageContent.hh"
#include "image/PHash.hh"
#include "image/EXIF2.hh"

#include "util/Configuration.hh"
#include "util/Log.hh"
#include "util/Magic.hh"
#include "util/MMap.hh"

// JSON for saving meta data
#include <nlohmann/json.hpp>

// OpenCV for calculating phash
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <limits>
#include <fstream>

namespace hrb {

namespace {
const std::string master_rendition = "master";

cv::Mat square_crop(const cv::Mat& image, const fs::path& haar_path)
{
	try
	{
		ImageContent content{image, haar_path};
		return image(content.square_crop()).clone();
	}
	catch (cv::Exception& e)
	{
		Log(LOG_WARNING, "Exception at BlobFile::generate_image_rendition(): %1%", e.msg.c_str());
	}
	catch (std::exception& e)
	{
		Log(LOG_WARNING, "Exception at BlobFile::generate_image_rendition(): %1%", e.what());
	}
	catch (...)
	{
		Log(LOG_WARNING, "Unknown exception at BlobFile::generate_image_rendition()");
	}

	// fall back: crop at center
	auto window_length = std::min(image.cols, image.rows);
	cv::Rect roi = (image.cols > image.rows) ?
		cv::Rect{image.cols / 2 - window_length/2, 0, window_length, window_length} :
		cv::Rect{0, image.rows / 2 - window_length/2, window_length, window_length} ;
	return image(roi).clone();
}

} // end of local namespace

/// \brief Open an existing blob in its directory
BlobFile::BlobFile(const fs::path& dir, const ObjectID& id) : m_id{id}, m_dir{dir}
{
}

/// \brief Creates a new blob from a uploaded file
BlobFile::BlobFile(UploadFile&& tmp, const fs::path& dir, std::error_code& ec)  : m_id{tmp.ID()}, m_dir{dir}
{
	assert(!ec);
	assert(tmp.is_open());

	assert(tmp.native_handle() > 0);

	// Note: closing the file before munmap() is OK: the mapped memory will still be there.
	// Details: http://pubs.opengroup.org/onlinepubs/7908799/xsh/mmap.html
	auto master = MMap::open(tmp.native_handle(), ec);
	if (ec)
	{
		struct stat buf{};
		if (auto r = ::fstat(tmp.native_handle(), &buf); r == 0)
			Log(LOG_WARNING, "fstat %1%", buf.st_size);

		// TODO: give more information about the file
		Log(LOG_WARNING, "BlobFile::upload(): cannot mmap temporary file %1% %2%", ec, ec.message());
		return;
	}

	create_directories(dir, ec);
	if (ec)
	{
		Log(LOG_WARNING, "create create directory %1% (%2% %3%)", dir, ec, ec.message());
	}
	else
	{
		// Try moving the temp file to our destination first. If failed, use
		// deep copy instead.
		tmp.move(m_dir / hrb::master_rendition, ec);

		// deduce meta data from the uploaded file
		deduce_meta(std::move(master));
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

MMap BlobFile::rendition(std::string_view rendition, const RenditionSetting& cfg, const fs::path& haar_path, std::error_code& ec) const
{
	if (rendition == hrb::master_rendition)
		return load_master(ec);

	// check if rendition is allowed by config
	if (!cfg.valid(rendition))
		rendition = cfg.default_rendition();

	auto rend_path = m_dir/std::string{rendition};

	// generate the rendition if it doesn't exist
	if (!exists(rend_path) && is_image())
		generate_image_rendition(cfg.find(rendition), rend_path, haar_path, ec);

	return exists(rend_path) ? MMap::open(rend_path, ec) : load_master(ec);
}

void BlobFile::generate_image_rendition(const JPEGRenditionSetting& cfg, const fs::path& dest, const fs::path& haar_path, std::error_code& ec) const
{
	auto jpeg = cv::imread((m_dir/hrb::master_rendition).string(), cv::IMREAD_ANYCOLOR);
	if (!jpeg.empty())
	{
		auto ratio = std::min(
			cfg.dim.width() / static_cast<double>(jpeg.cols),
			cfg.dim.height() / static_cast<double>(jpeg.rows)
		);

		cv::Mat out;
		if (ratio < 1.0)
			cv::resize(jpeg, out, {}, ratio, ratio, cv::INTER_LINEAR);
		else
			out = jpeg;

		if (cfg.square_crop)
			out = square_crop(out, haar_path);

		std::vector<unsigned char> out_buf;
		cv::imencode(mime() == "image/png" ? ".png" : ".jpg", out, out_buf, {cv::IMWRITE_JPEG_QUALITY, cfg.quality});
		save_blob(out_buf, dest, ec);
	}
	else
	{
		Log(LOG_WARNING, "BlobFile::generate_image_rendition(): Cannot open master rendition at %1%", m_dir);
	}
}

MMap BlobFile::load_master(std::error_code& ec) const
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
	return is_image(m_meta->mime());
}

std::optional<PHash> BlobFile::phash() const
{
	update_meta();
	return m_meta->phash();
}

std::string_view BlobFile::mime() const
{
	update_meta();
	return m_meta->mime();
}

nlohmann::json BlobFile::meta_json() const
{
	update_meta();
	return nlohmann::json(*m_meta);
}

void BlobFile::update_meta() const
{
	if (!m_meta.has_value())
	{
		try
		{
			// try to load it from meta.json
			std::ifstream meta_file{m_dir/"meta.json"};
			nlohmann::json meta;

			if (meta_file)
				meta_file >> meta;

			if (meta_file && meta.is_object())
				m_meta.emplace(meta.get<ImageMeta>());
		}
		catch (nlohmann::json::exception& e)
		{
			Log(LOG_WARNING, "json parse error @ file %1%: %2%", m_dir, e.what());
		}

		// some meta field is still missing, we need to deduce the meta
		deduce_meta({});
	}
}

MMap BlobFile::deduce_meta(MMap&& master) const
{
	// load master rendition on-demand
	master = this->master(std::move(master));

	// emplace() will destroy the original object if any
	m_meta.emplace(master.buffer());

	// save the meta data to file
	std::ofstream meta_file{(m_dir/"meta.json").string()};
	meta_file << nlohmann::json(*m_meta);

	return std::move(master);
}

Timestamp BlobFile::original_datetime() const
{
	update_meta();
	return m_meta->original_timestamp().has_value() ? *m_meta->original_timestamp() : m_meta->upload_timestamp();
}

double BlobFile::compare(const BlobFile& other) const
{
	return phash() && other.phash() ?
		phash()->compare(*other.phash()) :
		std::numeric_limits<double>::max();
}

// Return the memory map of the master rendition. Load it from disk if it is not loaded yet.
MMap BlobFile::master(MMap&& master) const
{
	std::error_code ec;
	if (!master.is_opened())
	{
		master = this->load_master(ec);
		if (ec)
			Log(LOG_WARNING, "cannot load master rendition of blob %1% from %2% (%3%)", to_hex(m_id), m_dir, ec.message());
	}
	return std::move(master);
}

MMap BlobFile::load_meta() const
{
	std::error_code ec;
	auto mmap = MMap::open(m_dir/"meta.json", ec);
	if (ec)
	{
		deduce_meta({});
		mmap = MMap::open(m_dir/"meta.json", ec);
	}

	// log error because we are going to ignore it
	if (ec)
		Log(LOG_WARNING, "cannot open meta.json for %3%: %1% (%2%)", ec, ec.message(), to_hex(m_id));

	return std::move(mmap);
}

} // end of namespace hrb
