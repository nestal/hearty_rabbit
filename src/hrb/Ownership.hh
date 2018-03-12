/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 2/23/18.
//

#pragma once

#include "ObjectID.hh"

#include "Collection.hh"
#include "net/Redis.hh"
#include "util/Log.hh"

#include <rapidjson/document.h>

#include <string_view>
#include <functional>
#include <vector>

namespace hrb {

class BlobDatabase;

/// A set of blob objects represented by a redis set.
class Ownership
{
private:
	class BlobBackLink
	{
	private:
		static const std::string_view m_prefix;

	public:
		BlobBackLink(std::string_view user, const ObjectID& blob);

		void watch(redis::Connection& db) const;

		// expect to be done inside a transaction
		void link(redis::Connection& db, std::string_view coll) const;
		void unlink(redis::Connection& db, std::string_view coll) const;

		const std::string& user() const {return m_user;}
		const ObjectID& blob() const {return m_blob;}

	private:
		std::string m_user;
		ObjectID    m_blob;
	};

public:
	explicit Ownership(std::string_view name);

	template <typename Complete>
	void link(
		redis::Connection& db,
		std::string_view path,
		const ObjectID& blobid,
		bool add,
		Complete&& complete
	)
	{
		BlobBackLink  blob{m_user, blobid};
		Collection coll{m_user, path};

		// watch everything that will be modified
		blob.watch(db);
		coll.watch(db);

		db.command("MULTI");
		if (add)
		{
			blob.link(db, coll.path());
			coll.link(db, blob.blob());
		}
		else
		{
			blob.unlink(db, coll.path());
			coll.unlink(db, blob.blob());
		}

		db.command([comp=std::forward<Complete>(complete)](auto&&, std::error_code ec)
		{
			Log(LOG_INFO, "transaction completed %1%", ec);
			comp(ec);
		}, "EXEC");
	}

	template <typename Complete>
	void is_owned(
		redis::Connection& db,
		std::string_view coll,
		const ObjectID& blob,
		Complete&& complete
	)
	{
		Collection{m_user, coll}.is_owned(db, blob, std::forward<Complete>(complete));
	}

	const std::string& user() const {return m_user;}

private:
	std::string             m_user;
};


} // end of namespace hrb
