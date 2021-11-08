/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 3/4/18.
//

#pragma once

#include <array>
#include <optional>
#include <iosfwd>
#include <string_view>
#include <string>
#include <filesystem>
#include <variant>

namespace hrb {

/// \brief  Deduce the intention of the user from the URL.
/// REST API:
/// GET "/" login screen
/// POST "/session?create" with username and password to login, redirect to "/~bunny" (where "rabbit" is username)
/// POST "/session?destroy" logout
/// GET "/~bunny" home directory of bunny
/// GET "/~bunny/subdir/" sub-directory under the bunny's home
/// GET "/~bunny/subdir/?rend=json" list files under sub-directory as JSON
/// PUT "/~bunny/subdir/somefile" upload file to "/~user/subdir/somefile". Overwriting if exists.
/// GET "/~bunny/subdir/somefile" download file
/// HEAD "/~bunny/subdir/somefile" HTTP headers for GET but without the payload
/// DELETE "/~bunny/subdir/somefile" delete file
/// GET "/~bunny/subdir/somefile?rend=1024x1024" download specific rendition of file
/// GET "/~bunny/subdir/?show=somefile" view file in directory
/// GET "/~bunny/subdir/?show=somefile&rend=1024x1024" view rendition of file in directory
/// POST "/~bunny/subdir/somefile" update meta-data of file, e.g. permission, user comments and tags.
/// GET "/query?tags=carrot" show page with list of files tagged as "carrot"
/// GET "/query?tags=carrot&rend=json" show list of files tagged as "sunny" as JSON
/// GET "/lib/background.js" get files from WebResource.
class URLIntent
{
public:
	enum class Type {none, session, user, query, lib};

	struct Session
	{
		enum class Action {create, destroy};
		Action action;
	};

	struct User
	{
		std::string username;      	//!< "bunny" in the above example
		std::filesystem::path path;	//!< "/subdir/somefile" in the above example
		std::string rendition;     	//!< "1024x1024" or "json" in the above example

		enum class Action {none, show};
	};

	struct Query
	{
		std::string tags;
		std::string rendition;
	};

	struct Lib
	{
		std::string filename;
	};

public:
	URLIntent() = default;
	URLIntent(URLIntent&&) = default;
	URLIntent(const URLIntent&) = default;
	URLIntent& operator=(URLIntent&&) = default;
	URLIntent& operator=(const URLIntent&) = default;

	explicit URLIntent(std::string_view target);

	explicit URLIntent(const Session& s) : m_var{s} {}
	explicit URLIntent(const User& s)   : m_var{s} {}
	explicit URLIntent(const Query& s)  : m_var{s} {}
	explicit URLIntent(const Lib& s)    : m_var{s} {}

	[[nodiscard]] Type type() const {return static_cast<Type>(m_var.index());}

	Session*    session()   {return std::get_if<Session>(&m_var);}
	User*       user()      {return std::get_if<User>(&m_var);}
	Query*      query()     {return std::get_if<Query>(&m_var);}
	Lib*        lib()       {return std::get_if<Lib>(&m_var);}
	[[nodiscard]] const Session*    session()   const {return std::get_if<Session>(&m_var);}
	[[nodiscard]] const User*       user()      const {return std::get_if<User>(&m_var);}
	[[nodiscard]] const Query*      query()     const {return std::get_if<Query>(&m_var);}
	[[nodiscard]] const Lib*        lib()       const {return std::get_if<Lib>(&m_var);}

private:
	std::variant<std::monostate, Session, User, Query, Lib> m_var;
};

} // end of namespace hrb
