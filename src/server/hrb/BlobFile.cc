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
#include "image/EXIF2.hh"
#include "image/PHash.hh"
#include "util/Configuration.hh"
#include "common/Escape.hh"
#include "util/Log.hh"
#include "util/Magic.hh"
#include "util/MMap.hh"

// JSON for saving meta data
#include <nlohmann/json.hpp>

// OpenCV for calculating phash
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <limits>

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
			cv::imencode(mime() == "image/png" ? ".png" : ".jpg", out, out_buf, {cv::IMWRITE_JPEG_QUALITY, cfg.quality});
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
	return is_image(m_meta->mime);
}

std::optional<PHash> BlobFile::phash() const
{
	update_meta();
	return m_meta->phash;
}

std::string_view BlobFile::mime() const
{
	update_meta();
	return m_meta->mime;
}

void BlobFile::update_meta() const
{
	if (!m_meta.has_value())
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

		if (meta_file && meta.is_object())
			m_meta.emplace(meta.get<Meta>());

		// some meta field is still missing, we need to deduce the meta
		deduce_meta({});
	}
}

MMap BlobFile::deduce_meta(MMap&& master) const
{
	if (!m_meta.has_value())
		m_meta.emplace();

	master = deduce_mime(std::move(master));
	master = deduce_phash(std::move(master));
	master = deduce_original(std::move(master));
	master = deduce_uploaded(std::move(master));

	// save the meta data to file
	std::ofstream meta_file{(m_dir/"meta.json").string()};
	meta_file << nlohmann::json(*m_meta);

	return std::move(master);
}

MMap BlobFile::deduce_mime(MMap&& master) const
{
	if (m_meta->mime.empty())
	{
		master = this->master(std::move(master));
		m_meta->mime = Magic::instance().mime(master.buffer());
	}
	return std::move(master);
}

MMap BlobFile::deduce_phash(MMap&& master) const
{
	if (!m_meta->phash.has_value() && is_image(m_meta->mime))
	{
		master = this->master(std::move(master));
		m_meta->phash = hrb::phash(master.buffer());
	}
	return std::move(master);
}

MMap BlobFile::deduce_original(MMap&& master) const
{
	// deduce original time from EXIF2
	if (!m_meta->original.has_value() && m_meta->mime == "image/jpeg")
	{
		master = this->master(std::move(master));

		std::error_code ec;
		EXIF2 exif{master.buffer(), ec};
		if (!ec)
		{
			auto field = exif.get(master.buffer(), EXIF2::Tag::date_time);
			if (field.has_value())
				m_meta->original = std::chrono::time_point_cast<Timestamp::duration>(
					EXIF2::parse_datetime(exif.get_value(master.buffer(), *field))
				);
		}
	}

	return std::move(master);
}

Timestamp BlobFile::original_datetime() const
{
	update_meta();
	return m_meta->original.has_value() ? *m_meta->original : m_meta->uploaded;
}

double BlobFile::compare(const BlobFile& other) const
{
	return phash() && other.phash() ?
		phash()->compare(*other.phash()) :
		std::numeric_limits<double>::max();
}

MMap BlobFile::master(MMap&& master) const
{
	std::error_code ec;
	if (!master.is_opened())
	{
		master = this->master(ec);
		if (ec)
			Log(LOG_WARNING, "cannot load master rendition of blob %1% from %2% (%3%)", to_hex(m_id), m_dir, ec.message());
	}
	return std::move(master);
}

MMap BlobFile::deduce_uploaded(MMap&& master) const
{
	if (m_meta->uploaded == Timestamp{})
		m_meta->uploaded = std::chrono::time_point_cast<Timestamp::duration>(Timestamp::clock::now());
	return std::move(master);
}

nlohmann::json BlobFile::meta_json() const
{
	update_meta();
	return nlohmann::json(*m_meta);
}

void to_json(nlohmann::json& dest, const BlobFile::Meta& src)
{
	// save the meta data to file
	using namespace std::chrono;
	nlohmann::json meta{
		{"mime", src.mime}
	};

	// just to be sure
	assert(meta.is_object());

	if (src.original)
		meta.emplace("original_datetime", *src.original);
	if (src.phash)
		meta.emplace("phash", src.phash->value());

	dest = std::move(meta);
}

void from_json(const nlohmann::json& src, BlobFile::Meta& dest)
{
	if (src.is_object())
	{
		dest.mime = src["mime"];

		if (src.count("original_datetime") > 0)
			dest.original = src["original_datetime"].get<Timestamp>();
		else
			dest.original = std::nullopt;

		if (src.count("phash") > 0)
			dest.phash = PHash{src["phash"].get<std::uint64_t>()};
		else
			dest.phash = std::nullopt;
	}
}

MMap BlobFile::meta() const
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
