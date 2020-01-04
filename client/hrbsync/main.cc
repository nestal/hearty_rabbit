/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/1/2020.
//

#include "client/HRBClient.hh"
#include "client/HRBClient.ipp"

#include "common/util/MMap.hh"
#include "common/util/Magic.hh"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <iostream>
#include <fstream>

using namespace hrb;
using namespace std::chrono_literals;

Collection load_local(const std::filesystem::path& local)
{
	Magic magic;

	Collection coll;
	for (auto&& file : std::filesystem::directory_iterator{local})
	{
		std::error_code ec;
		auto mmap = MMap::open(file.path(), ec);
		if (!ec)
		{
			Blake2 hash;
			hash.update(mmap.data(), mmap.size());

			coll.add_blob(
				hash.finalize(),
				BlobInode{
					{}, file.path().filename(), std::string{magic.mime(mmap.buffer())}, {}
				}
			);
		}
	}

	return coll;
}

Collection compare_collection(Collection remote, Collection local)
{
	std::cout << "comparing remote " << remote.size() << " and local " << local.size() << std::endl;

	std::set<ObjectID> remote_blobids, local_blobids;
	for (auto&& [id, blob] : remote.blobs())
		remote_blobids.insert(id);

	for (auto&& [id, blob] : local.blobs())
		local_blobids.insert(id);

	std::vector<ObjectID> diff1, diff2;
	std::set_difference(
		remote_blobids.begin(), remote_blobids.end(),
		local_blobids.begin(), local_blobids.end(),
		std::back_inserter(diff1)
	);

	Collection download;
	for (auto&& id : diff1)
	{
		auto blob = remote.find(id);

//		std::cout << "blob " << blob->second.filename << " not found " << std::endl;
		for (auto i = 0U; i < blob->second.filename.size(); ++i)
		{
//			std::cout << "char " << i << " " << blob->second.filename[i] << " " << (int)blob->second.filename[i] << std::endl;
		}

		assert(remote.find(id) != remote.blobs().end());
		download.add_blob(id, remote.find(id)->second);
	}
	std::cout << download.size() << " differences" << std::endl;

	return download;
}

int main(int argc, char **argv)
{
	if (argc < 6)
	{
		std::cerr << "usage: " << argv[0] << " [server] [port] [collection] [username] [password]" << std::endl;
		return -1;
	}

	auto local = load_local(std::filesystem::current_path());

	boost::asio::io_context ioc;
	ssl::context ctx{ssl::context::sslv23_client};

	HRBClient client{ioc, ctx, argv[1], argv[2]};
	client.login(argv[4], argv[5], [&client, coll=argv[3], &local](std::error_code ec)
	{
		if (!ec)
			std::cout << "login success!" << std::endl;

		client.list_collection(coll, [&client, &local](Collection&& coll, std::error_code ec)
		{
			if (!ec)
			{
				auto diff = compare_collection(coll, local);
				for (auto&&[id, entry] : diff.blobs())
				{
					std::cout << "blob: " << to_hex(id) << " " << entry.filename << std::endl;

					client.get_blob(
						coll.owner(),
						coll.name(),
						id,
						"master",
						[fname = entry.filename](const std::string& body, std::error_code ec)
						{
							std::cout << "downloaded: " << fname << " " << body.size() << " " << ec << std::endl;

							Blake2 hash;
							hash.update(body.data(), body.size());
							std::cout << "object ID:" << to_hex(hash.finalize()) << std::endl;

							for (
								auto i = 0U; i < fname.size(); ++i
								)
							{
								std::cout << "char " << i << " " << fname[i] << " " << (int) fname[i] << std::endl;
							}

							try
							{
								std::ofstream out;
								out.exceptions(std::ios::failbit | std::ios::badbit);

								out.open(fname, std::ios::out | std::ios::trunc);

								std::cout
									<< "written: "
									<< out.rdbuf()->sputn(body.data(), body.size())
									<< " bytes"
									<< std::endl;
							}
							catch (std::ofstream::failure& e)
							{
								std::cout << "cannot write to file " << e.code().message() << std::endl;
							}
						}
					);
				}
			}
		});
	});
	ioc.run();

	return 0;
}
