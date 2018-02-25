/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/25/18.
//

#include "Exif.hh"
#include <libexif/exif-data.h>

namespace hrb {

Exif::Exif(const fs::path& path) :
	m_exif{::exif_data_new_from_file(path.string().c_str())}
{
}

Exif::Exif(const void *data, std::size_t size) :
	m_exif{::exif_data_new_from_data(static_cast<const unsigned char*>(data), static_cast<unsigned>(size))}
{
}


Exif::~Exif()
{
	if (m_exif)
		::exif_data_unref(m_exif);
}

int Exif::ISO() const
{
	if (m_exif)
	{
		auto entry = ::exif_content_get_entry(m_exif->ifd[EXIF_IFD_EXIF], EXIF_TAG_ISO_SPEED_RATINGS);
		if (!entry)
			throw std::runtime_error("error: cannot load ISO speed ratings from image");

		char buf[1024] = {};
		exif_entry_get_value(entry, buf, sizeof(buf));

		return std::stoi(buf);
	}
	else
		return 100;
}

std::chrono::system_clock::time_point Exif::datetime() const
{
	auto str = datetime_str();

	struct tm tm;
	strptime(str.c_str(), "%Y:%m:%d %H:%M:%S", &tm);

	// mktime() assume local time-zone. assume the camera also set to local timezone
	return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string Exif::datetime_str() const
{
	if (m_exif)
	{
		auto entry = ::exif_content_get_entry(m_exif->ifd[EXIF_IFD_0], EXIF_TAG_DATE_TIME);
		if (!entry)
			throw std::runtime_error("error: cannot load date/time from image");

		char buf[1024] = {};
		exif_entry_get_value(entry, buf, sizeof(buf));
		return buf;
	}
	else
		return {};
}

std::optional<int> Exif::orientation() const
{
	if (m_exif)
	{
		auto entry = exif_data_get_entry(m_exif, EXIF_TAG_ORIENTATION);
		if (!entry)
			return std::nullopt;

		auto order = exif_data_get_byte_order(m_exif);
		return exif_get_short(entry->data, order);
	}
	else
		return std::nullopt;
}

} // end of namespace hrb
