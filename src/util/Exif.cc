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

namespace hrb {

std::optional<Exif> Exif::load(const fs::path& path)
{
	std::optional<Exif> result;

	auto data = ::exif_data_new_from_file(path.string().c_str());
	if (data)
		result.emplace(Exif{data});

	return result;
}

std::optional<Exif> Exif::load(const void *raw, std::size_t size)
{
	std::optional<Exif> result;

	auto data = ::exif_data_new_from_data(static_cast<const unsigned char*>(raw), static_cast<unsigned>(size));
	if (data)
		result.emplace(Exif{data});

	return result;
}

Exif::Exif(ExifData *data) :
	m_exif{data}
{
	assert(m_exif);
}

Exif::~Exif()
{
	// default constructed will be null
	if (m_exif)
		::exif_data_unref(m_exif);
}

int Exif::ISO() const
{
	assert(m_exif);
	auto entry = exif_data_get_entry(m_exif, EXIF_TAG_ISO_SPEED_RATINGS);
	if (!entry)
		throw std::runtime_error("error: cannot load ISO speed ratings from image");

	char buf[1024] = {};
	exif_entry_get_value(entry, buf, sizeof(buf));

	return std::stoi(buf);
}

std::chrono::system_clock::time_point Exif::datetime() const
{
	assert(m_exif);
	auto str = datetime_str();

	struct tm tm{};
	strptime(str.c_str(), "%Y:%m:%d %H:%M:%S", &tm);

	// mktime() assume local time-zone. assume the camera also set to local timezone
	return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::string Exif::datetime_str() const
{
	assert(m_exif);
	auto entry = exif_data_get_entry(m_exif, EXIF_TAG_DATE_TIME);
	if (!entry)
		throw std::runtime_error("error: cannot load date/time from image");

	char buf[1024] = {};
	exif_entry_get_value(entry, buf, sizeof(buf));
	return buf;
}

std::optional<int> Exif::orientation() const
{
	assert(m_exif);
	auto entry = exif_data_get_entry(m_exif, EXIF_TAG_ORIENTATION);
	if (!entry)
		return std::nullopt;

	auto order = exif_data_get_byte_order(m_exif);
	return exif_get_short(entry->data, order);
}

Exif::Exif(Exif&& other)
{
	std::swap(m_exif, other.m_exif);
	assert(!other.m_exif);
}

Exif& Exif::operator=(Exif&& other)
{
	auto copy{std::move(other)};
	std::swap(m_exif, copy.m_exif);
	return *this;
}

} // end of namespace hrb
