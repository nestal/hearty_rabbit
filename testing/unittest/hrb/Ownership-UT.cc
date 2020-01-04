/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 2/23/18.
//

#include <catch2/catch.hpp>

#include "hrb/Ownership.hh"
#include "hrb/Ownership.ipp"
#include "hrb/BlobDatabase.hh"
#include "hrb/UploadFile.hh"
#include "common/hrb/Permission.hh"
#include "crypto/Random.hh"

#include <boost/algorithm/string.hpp>

using namespace hrb;
using namespace std::chrono_literals;
using namespace boost::algorithm;

TEST_CASE("list of collection owned by user", "[normal]")
{
	auto blobid = insecure_random<ObjectID>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	Ownership subject{"owner"};

	int tested = 0;
	subject.link_blob(
		*redis, "/", blobid, BlobInode{}, [&tested](std::error_code ec)
		{
			REQUIRE_FALSE(ec);
			tested++;
		}
	);

	// assert that the collection is added
	subject.scan_all_collections(*redis, [&tested](auto&& coll_list, auto ec)
	{
		REQUIRE_FALSE(ec);
		REQUIRE(coll_list.find("owner", "/") != coll_list.end());
		REQUIRE(coll_list.size() > 0);
		tested++;

		for (auto&& c : coll_list)
			REQUIRE(c.owner() == "owner");
	});

	// assert the blob backlink points back to the collection
	std::vector<std::string> refs;
	subject.query_blob(*redis, blobid, [&refs](auto&& range, auto ec)
	{
		for (auto&& ref : range)
			if (ref.user == "owner")
				refs.emplace_back(ref.coll);
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
	REQUIRE(std::find(refs.begin(), refs.end(), "/") != refs.end());
	ioc.restart();

	// remove all blobs in the collection
	subject.get_collection(
		*redis, Authentication{{}, "owner"}, "/",
		[&tested, redis](auto&& coll, std::error_code ec)
		{
			REQUIRE(coll.blobs().begin() != coll.blobs().end());
			for (
				auto&&[id, blob] : coll.blobs())
			{
				INFO("removing = " << to_hex(id));
				Ownership{"owner"}.unlink_blob(
					*redis, "/", id, [](auto&& ec)
					{
						REQUIRE(!ec);
					}
				);
			}

			tested++;
		}
	);

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 3);
	ioc.restart();

	// assert that the collection "/" does not exist anymore, because all its blobs are removed
	subject.scan_all_collections(*redis, [&tested](auto&& coll_list, auto ec)
	{
		REQUIRE_FALSE(ec);
		REQUIRE(coll_list.find("owner", "/") == coll_list.end());
		tested++;
	});
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 4);
}


TEST_CASE("add blob to Ownership", "[normal]")
{
	auto blobid = insecure_random<ObjectID>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	int tested = 0;

	Ownership subject{"owner"};

	subject.link_blob(
		*redis, "/", blobid, BlobInode{{}, "file.name"}, [&tested](std::error_code ec)
		{
			REQUIRE(!ec);
			tested++;
		}
	);

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 1);
	ioc.restart();

	subject.get_collection(*redis, {{}, "owner"}, "/", [&tested, blobid](Collection&& coll, std::error_code ec)
	{
		REQUIRE_FALSE(ec);
		REQUIRE(coll.find(blobid) != coll.blobs().end());
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
	ioc.restart();

	// owner access is allowed
	subject.get_blob(
		*redis, "/", blobid, [&tested](auto&& entry, auto filename, std::error_code ec)
		{
			REQUIRE(!ec);
			REQUIRE(filename == "file.name");
			REQUIRE(entry.filename() == "file.name");
			tested++;
		}
	);

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 3);
	ioc.restart();

	// anonymous access is not allowed
	subject.get_blob(
		*redis, "/", blobid, [&tested](auto&& entry, auto filename, std::error_code ec)
		{
			REQUIRE(!ec);
			REQUIRE(!entry.permission().allow({}, "owner"));
			REQUIRE(entry.filename() == "file.name");
			REQUIRE(filename == "file.name");
			tested++;
		}
	);

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 4);
	ioc.restart();

	// set permission to public
	subject.set_permission(*redis, blobid, Permission::public_(), [&tested](std::error_code ec)
	{
		REQUIRE(!ec);
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 5);
	ioc.restart();

	// change filename
	subject.rename_blob(
		*redis, "/", blobid, "something.else", [&tested](std::error_code ec)
		{
			REQUIRE(!ec);
			tested++;
		}
	);

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 6);
	ioc.restart();

	// anonymous access is now allowed
	subject.get_blob(
		*redis, "/", blobid, [&tested](auto&& entry, auto filename, std::error_code ec)
		{
			REQUIRE(!ec);
			REQUIRE(entry.permission().allow({}, "owner"));
			REQUIRE(entry.filename() == "file.name");
			REQUIRE(filename == "something.else");
			tested++;
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 7);
	ioc.restart();

	// verify that the newly added blob is in the public list
	bool found = false;
	subject.list_public_blobs(*redis, [&found, blobid](auto&& brefs, auto ec)
	{
		auto it = std::find_if(brefs.begin(), brefs.end(), [blobid](auto&& bref){return bref.blob == blobid;});
		found = (it != brefs.end());

		if (found)
			REQUIRE(it->coll == "/");
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(found);
	ioc.restart();

	// move to another new collection
	subject.move_blob(*redis, "/", "someother", blobid, [&tested](std::error_code ec)
	{
		REQUIRE(!ec);
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 8);
	ioc.restart();

	// check if it is in the new collection
	subject.get_blob(
		*redis, "someother", blobid, [&tested](auto&& entry, auto filename, std::error_code ec)
		{
			REQUIRE(!ec);
			REQUIRE(entry.filename() == "file.name");
			REQUIRE(filename == "something.else");
			REQUIRE(entry.permission().allow({}, "owner"));
			tested++;
		}
	);

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 9);
	ioc.restart();

	// query blob by owner
	subject.get_blob(*redis, {{}, "owner"}, blobid, [&tested](auto&& entry, std::error_code ec)
	{
		REQUIRE(!ec);
		REQUIRE(entry.filename() == "file.name");
		REQUIRE(entry.permission().allow({}, "owner"));
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 10);
	ioc.restart();

	// query blob by someone else
	subject.get_blob(*redis, {{}, "someone_else"}, blobid, [&tested](auto&& entry, std::error_code ec)
	{
		REQUIRE(!ec);
		REQUIRE(entry.filename() == "file.name");
		REQUIRE(entry.permission().allow({}, "someone_else"));
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 11);
	ioc.restart();
}

TEST_CASE("Load 3 images in json", "[normal]")
{
	const auto blobids = insecure_random<std::array<ObjectID, 3>>();

	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	// TODO: unlink all images left behind before testing

	int added = 0;

	Ownership subject{"testuser"};

	for (auto&& blobid : blobids)
	{
		BlobInode entry{Permission::private_(), "file.jpg", "image/jpeg", Timestamp::now()};
		subject.link_blob(
			*redis, "some/collection", blobid, entry, [&added](auto ec)
			{
				REQUIRE(!ec);
				added++;
			}
		);
	}

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(added == blobids.size());

	ioc.restart();

	// update BlobInode of the blobs
	BlobInode entry{Permission::public_(), "another_file.jpg", "application/json", Timestamp{std::chrono::milliseconds{100}}};
	for (auto&& blobid : blobids)
		subject.update_blob(*redis, blobid, entry);

	// rename all files
	int renamed = 0;
	for (auto&& blobid : blobids)
		subject.rename_blob(
			*redis, "some/collection", blobid, "renamed.jpg", [&renamed](auto ec)
			{
				REQUIRE(!ec);
				renamed++;
			}
		);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(renamed == added);

	ioc.restart();

	bool tested = false;
	subject.get_collection(
		*redis, {{}, "testuser"}, "some/collection", [&tested, &blobids](auto&& coll, auto ec)
		{
			using json = nlohmann::json;

			REQUIRE_FALSE(ec);
			REQUIRE(coll.owner() == "testuser");
			REQUIRE(coll.name() == "some/collection");

			for (
				auto&&[id, entry] : coll.blobs())
			{
				REQUIRE(entry.perm == Permission::public_());
				REQUIRE(entry.filename == "renamed.jpg");
				REQUIRE(entry.mime == "application/json");
				REQUIRE(entry.timestamp == Timestamp{std::chrono::milliseconds{100}});
			}

			tested = true;
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);

	ioc.restart();

	// delete all 3 image blobs
	for (auto&& blobid : blobids)
		subject.unlink_blob(
			*redis, "some/collection", blobid, [&added](auto ec)
			{
				REQUIRE(!ec);
				added--;
			}
		);

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(added == 0);
}

TEST_CASE("Query blob of testuser")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	Ownership subject{"testuser"};

	auto blobid = insecure_random<ObjectID>();

	BlobInode entry{Permission::public_(), "haha.jpeg", "image/jpeg", Timestamp::now()};

	int tested = 0;
	subject.link_blob(
		*redis, "somecoll", blobid, entry, [&tested](auto ec)
		{
			REQUIRE(!ec);
			tested++;
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 1);
	ioc.restart();

	subject.query_blob(*redis, blobid, [&tested](auto&& range, auto ec)
	{
		REQUIRE(!ec);
		auto first = range.begin();
		auto last  = range.end();
		REQUIRE(first != last);

		// There should be only one collection that owns the blob
		REQUIRE(first->user == "testuser");
		REQUIRE(first->coll == "somecoll");
		REQUIRE(first->entry.permission() == Permission::public_());
		REQUIRE(first->entry.filename() == "haha.jpeg");
		REQUIRE(first->entry.mime() == "image/jpeg");
		REQUIRE(++first == last);
		tested++;
	});

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested == 2);
}

TEST_CASE("set cover error cases", "[error]")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	Ownership subject{"testuser"};
	auto cover_blob = insecure_random<ObjectID>();

	SECTION("setting cover of inexist album")
	{
		std::string inexist_album{"inexist"};

		// concatenate all existing album name to create a album name that doesn't exist
		subject.scan_all_collections(*redis,
			[&inexist_album](auto&& coll_list, auto ec)
			{
				for (auto&& it : coll_list)
				{
					REQUIRE(it.owner() == "testuser");
					inexist_album += it.name();
				}
			}
		);

		bool tested = false;

		// setting the cover of an album that doesn't exists
		subject.set_cover(*redis, inexist_album, cover_blob, [&tested](bool ok, auto ec)
		{
			REQUIRE(!ec);
			REQUIRE(!ok);
			tested = true;
		});

		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(tested);
	}
	SECTION("setting cover of inexist blob in a valid album")
	{
		auto blob1 = insecure_random<ObjectID>();
		auto blob2 = insecure_random<ObjectID>();

		// add 2 blobs to an album
		int run = 0;
		subject.link_blob(
			*redis, "/", blob1, BlobInode{}, [&run](auto ec)
			{
				REQUIRE(!ec);
				++run;
			}
		);
		subject.link_blob(
			*redis, "/", blob2, BlobInode{}, [&run](auto ec)
			{
				REQUIRE(!ec);
				++run;
			}
		);
		// remove blob1 so that it doesn't exist in the album
		subject.unlink_blob(
			*redis, "/", blob1, [&run](auto ec)
			{
				REQUIRE(!ec);
				++run;
			}
		);
		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(run == 3);
		ioc.restart();

		// try to set blob1 as cover, it should fail because blob1 doesn't exist
		// in the album
		subject.set_cover(*redis, "/", blob1, [&run](bool ok, auto ec)
		{
			REQUIRE(!ok);
			++run;
		});
		REQUIRE(ioc.run_for(10s) > 0);
		REQUIRE(run == 4);
	}
}

TEST_CASE("setting and remove the cover of collection", "[normal]")
{
	boost::asio::io_context ioc;
	auto redis = redis::connect(ioc);

	Ownership subject{"testuser"};
	auto cover_blob = insecure_random<ObjectID>();

	bool added = false;
	subject.link_blob(
		*redis, "/", cover_blob, BlobInode{}, [&added](auto ec)
		{
			REQUIRE(!ec);
			added = true;
		}
	);

	// add another blob to the collection so that the collection will be
	// still here even after removing the first one
	subject.link_blob(
		*redis, "/", insecure_random<ObjectID>(), BlobInode{}, [&added](auto ec)
		{
			REQUIRE(!ec);
		}
	);

	std::vector<std::string> dirs;

	bool tested = false;
	subject.scan_all_collections(*redis,
		[&dirs, &tested](auto&& coll_list, auto ec)
		{
			INFO("scan() error: " << ec << " " << ec.message());
			REQUIRE_FALSE(ec);
			tested = true;

			for (auto&& entry : coll_list)
			{
				dirs.emplace_back(entry.name());
				REQUIRE(entry.owner() == "testuser");
			}
		}
	);

	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(added);
	REQUIRE(tested);
	REQUIRE(!dirs.empty());
	INFO("dirs.size() " << dirs.size());
	REQUIRE(std::find(dirs.begin(), dirs.end(), "/") != dirs.end());
	ioc.restart();
	tested = false;

	// set the cover to be the new generated blob
	subject.set_cover(*redis, "/", cover_blob, [&tested](bool ok, auto ec)
	{
		REQUIRE(!ec);
		REQUIRE(ok);
		tested = true;
	});
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
	tested = false;
	ioc.restart();

	// check if the cover is updated
	subject.scan_all_collections(*redis,
		[&cover_blob, &tested](auto&& coll_list, auto ec)
		{
			REQUIRE_FALSE(ec);
			auto def_coll = coll_list.find("testuser", "/");
			REQUIRE(def_coll != coll_list.end());
			REQUIRE(def_coll->cover() == cover_blob);
			tested = true;
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(tested);
	bool removed = false;
	ioc.restart();

	// remove the new blob
	subject.unlink_blob(
		*redis, "/", cover_blob, [&removed](auto ec)
		{
			REQUIRE(!ec);
			removed = true;
		}
	);

	// check if the cover is updated
	bool updated = false;
	subject.scan_all_collections(*redis,
		[&cover_blob, &updated, &removed](auto&& coll_list, auto ec)
		{
			REQUIRE(removed);
			REQUIRE_FALSE(ec);

			for (auto&& it : coll_list)
			{
				REQUIRE(it.owner() == "testuser");
				if (it.name() == "/" )
				{
					REQUIRE(it.cover() != cover_blob);
					updated = true;
				}
			}
		}
	);
	REQUIRE(ioc.run_for(10s) > 0);
	REQUIRE(removed);
	REQUIRE(updated);
}

TEST_CASE("collection entry", "[normal]")
{
	Authentication yung{insecure_random<UserID::SessionID>(), "yungyung"};
	Authentication sum{insecure_random<UserID::SessionID>(), "sumsum"};

	auto s = BlobInodeDB::create({}, "somepic.jpeg", "image/jpeg", Timestamp::now());
	BlobInodeDB subject{s};
	INFO("entry JSON = " << subject.json());

	REQUIRE(subject.filename() == "somepic.jpeg");
	REQUIRE(subject.mime() == "image/jpeg");
	REQUIRE_FALSE(subject.permission().allow(sum.id(), yung.id().username()));
	REQUIRE(subject.raw() == s);

	BlobInodeDB same{subject.raw()};
	REQUIRE(same.filename() == "somepic.jpeg");
	REQUIRE(same.mime() == "image/jpeg");
	REQUIRE_FALSE(same.permission().allow(yung.id(), sum.id().username()));
	REQUIRE(same.raw() == subject.raw());

	auto s2 = BlobInodeDB::create(Permission::shared(), nlohmann::json::parse(same.json()));
	BlobInodeDB same2{s2};
	REQUIRE(same2.filename() == "somepic.jpeg");
	REQUIRE(same2.mime() == "image/jpeg");
	REQUIRE(same2.permission().allow(yung.id(), sum.id().username()));
	REQUIRE(same2.raw().substr(1) == subject.raw().substr(1));

}
