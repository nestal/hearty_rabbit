/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 1/Mar/18.
//

#include "EXIF2.hh"

#include <cassert>

namespace hrb {

EXIF2::EXIF2(BufferView jpeg) :
	m_data{::exif_data_new_from_data(jpeg.data(), jpeg.size())}
{
}

EXIF2::~EXIF2() = default;

std::optional<std::chrono::system_clock::time_point> EXIF2::date_time() const
{
	assert(m_data);
	if (auto entry = ::exif_content_get_entry(m_data->ifd[EXIF_IFD_0], EXIF_TAG_DATE_TIME); entry)
	{
		char buf[1024];
        ::exif_entry_get_value(entry, buf, sizeof(buf));

        struct std::tm result{};
        auto p = strptime(reinterpret_cast<const char*>(buf), "%Y:%m:%d %H:%M:%S", &result);
		if (p)
			return std::chrono::system_clock::from_time_t(::timegm(&result));
	}

	return std::nullopt;
}


void EXIF2::Unref::operator()(::ExifData *data) const
{
	if (data)
		::exif_data_unref(data);
}

} // end of namespace
