/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/18/18.
//

#include <iostream>
#include "UploadFile.hh"

namespace hrb {

UploadFile::~UploadFile()
{
	if (!m_tmp_path.empty() && fs::exists(m_tmp_path))
		fs::remove(m_tmp_path);
}

bool UploadFile::is_open() const
{
	return m_file.is_open();
}

void UploadFile::close(boost::system::error_code& ec)
{
	m_file.close(ec);
}

void UploadFile::open(const fs::path& parent_directory, boost::system::error_code& ec)
{
	// The code used to call open(O_TMPFILE)/linkat(/proc/self/fd/) when the kernel supports
	// it, but it doesn't work when the parent_directory is a samba mount. Therefore we use
	// the old mkstemp()/rename() which should work everywhere.
	m_tmp_path = (parent_directory / "blob-XXXXXX").string();
	int fd = ::mkstemp(&m_tmp_path[0]);

	if (fd < 0)
		ec.assign(errno, boost::system::generic_category());
	else
		m_file.native_handle(fd);
}

std::uint64_t UploadFile::size(boost::system::error_code& ec) const
{
	return m_file.size(ec);
}

std::uint64_t UploadFile::pos(boost::system::error_code& ec) const
{
	return m_file.pos(ec);
}

void UploadFile::seek(std::uint64_t offset, boost::system::error_code& ec)
{
	return m_file.seek(offset, ec);
}

std::size_t UploadFile::read(void *buffer, std::size_t n, boost::system::error_code& ec) const
{
	return m_file.read(buffer, n, ec);
}

std::size_t UploadFile::write(void const *buffer, std::size_t n, boost::system::error_code& ec)
{
	m_hash.update(buffer, n);
	return m_file.write(buffer, n, ec);
}

/// Get the object ID (blake2 hash) of the file.
ObjectID UploadFile::ID() const
{
	return ObjectID{Blake2{m_hash}.finalize()};
}

UploadFile::native_handle_type UploadFile::native_handle() const
{
	return m_file.native_handle();
}

void UploadFile::move(const fs::path& dest, std::error_code& ec)
{
	assert(!m_tmp_path.empty());

	// Note that linkat() does not work in samba mount drive.
	if (!m_tmp_path.empty())
	{
		boost::system::error_code bec;

		// try moving the file instead of linking
		if (!bec)
			fs::rename(m_tmp_path, dest, bec);
		if (bec)
			ec.assign(bec.value(), bec.category());

		m_tmp_path.clear();
	}
	else
		ec.assign(ENOENT, std::generic_category());
}

std::size_t UploadFile::pread(void *buffer, std::size_t n, std::streamoff pos, std::error_code& ec) const
{
	auto r = ::pread(m_file.native_handle(), buffer, n, pos);
	if (r < 0)
		ec.assign(errno, std::generic_category());
	return static_cast<std::size_t>(r);
}

void UploadRequestBody::reader::init(const boost::optional<std::uint64_t>&, boost::system::error_code& ec)
{
	if (!m_body.is_open())
		ec.assign(EBADF, boost::system::generic_category());
}

} // end of namespace hrb
