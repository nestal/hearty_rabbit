/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/18/18.
//

#pragma once

#include "common/hrb/ObjectID.hh"
#include "common/util/FS.hh"
#include "net/Request.hh"

#include <boost/beast/core/file_posix.hpp>
#include <system_error>

namespace hrb {

class UploadFile
{
public:
	using native_handle_type = boost::beast::file_posix::native_handle_type ;

public:
	UploadFile() = default;
	UploadFile(const UploadFile&) = delete;
	UploadFile(UploadFile&&) = default;
	~UploadFile();

	UploadFile& operator=(const UploadFile&) = delete;
	UploadFile& operator=(UploadFile&&) = default;

	/// Returns `true` if the file is open
	bool is_open() const;

	/// Close the file if open
	void close(boost::system::error_code& ec);

	/// Open a file at the given path with the specified mode
	void open(const fs::path& parent_directory, boost::system::error_code& ec);

	/// Return the size of the open file
	std::uint64_t size(boost::system::error_code& ec) const;

	/// Return the current position in the open file
	std::uint64_t pos(boost::system::error_code& ec) const;

	/// Adjust the current position in the open file
	void seek(std::uint64_t offset, boost::system::error_code& ec);

	/// Read from the open file
	std::size_t read(void* buffer, std::size_t n, boost::system::error_code& ec) const;

	std::size_t pread(void* buffer, std::size_t n, std::streamoff pos, std::error_code& ec) const;

	/// Write to the open file
	std::size_t write(void const* buffer, std::size_t n, boost::system::error_code& ec);

	[[nodiscard]] ObjectID ID() const;

	[[nodiscard]] native_handle_type native_handle() const;

	void move(const fs::path& dest, std::error_code& ec);

	[[nodiscard]] const std::string& path() const {return m_tmp_path;}

private:
	boost::beast::file_posix m_file{};
	std::string m_tmp_path{};
	Blake2      m_hash{};
};

class UploadRequestBody
{
public:
	using value_type = UploadFile;

	class reader
	{
	public:
		template<bool is_request, class Fields>
		explicit reader(http::header<is_request, Fields>&, value_type& body) : m_body{body}
		{
		}

		void init(const boost::optional<std::uint64_t>&, boost::system::error_code& ec);

		template<class ConstBufferSequence>
		std::size_t put(const ConstBufferSequence& buffers, boost::system::error_code& ec)
		{
			std::size_t total = 0;

			for (auto&& buf : buffers)
			{
				total += m_body.write(buf.data(), buf.size(), ec);
				if (ec)
					return total;
			}

			ec.assign(0, ec.category());
			return total;
		}

		void finish(boost::system::error_code&)
		{
		}

	private:
		value_type& m_body;
	};
};

} // end of namespace hrb
