/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/18/18.
//

#include "UploadFile.hh"
#include "util/Log.hh"

namespace hrb {

UploadFile::~UploadFile()
{
	if (!m_path.empty())
		fs::remove(m_path);
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
	auto glibc_mkstemp = [parent_directory, this]
	{
		m_path = (parent_directory / "blob-XXXXXX").string();
		return ::mkstemp(&m_path[0]);
	};

#ifdef O_TMPFILE
	// Note that O_TMPFILE requires the "path" to be a directory.
	// See http://man7.org/linux/man-pages/man2/open.2.html
	auto fd = ::open(parent_directory.string().c_str(), O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);

	// Fallback to glibc mkstemp() if O_TMPFILE is not supported
	// by the Linux kernel.
	if (fd < 0 && errno == EISDIR)
		fd = glibc_mkstemp();
#else
	// CentOS 7 (i.e. Linux 3.10) does not support O_TMPFILE, so
	// don't bother even trying.
	auto fd = glibc_mkstemp();
#endif

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

void UploadFile::linkat(const fs::path& dest, std::error_code& ec) const
{
	std::ostringstream proc;
	proc << "/proc/self/fd/" << m_file.native_handle();

	if (::linkat(AT_FDCWD, proc.str().c_str(), AT_FDCWD, dest.string().c_str(), AT_SYMLINK_FOLLOW) != 0)
		ec.assign(errno, std::generic_category());
}

void UploadRequestBody::reader::init(const boost::optional<std::uint64_t>&, boost::system::error_code& ec)
{
	Log(LOG_NOTICE, "UploadRequestBody::reader::init()");

	if (!m_body.is_open())
		ec.assign(EBADF, boost::system::generic_category());
}

} // end of namespace hrb
