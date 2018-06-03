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

#include "../../../boost_1_67_0/include/boost/utility/string_view.hpp"

#include "../../../../../../usr/include/c++/8/array"
#include "../../../../../../usr/include/c++/8/string_view"
#include "../../../../../../usr/include/c++/8/vector"

namespace hrb {

// Deduce the intention of the user from the URL
class URLIntent
{
public:
	enum class Action {login, logout, view, upload, home, lib, query, api, none};

	enum class QueryTarget {blob, collection, blob_set, none};

	//! Possible parameters specified by a URL
	enum class Parameter {
		user,           //!< Name of a user whom this request is applied to. Note this may not
						//!< be the user who send this request. Only occupy one segment in the URL.
		collection,     //!< Name of the collection which the request is applied to. It may span
						//!< across multiple segments.
		filename,       //!< Filename of a resource. Single segment only.
		blob,           //!< 40-character hex string for blob ID. Obviously single segment.
		query_target    //!< Target of the query: blob or collection
	};

private:
	using Parameters = std::vector<Parameter>;

	static constexpr std::array<bool, static_cast<int>(Action::none)> require_user =
	//   login, logout, view, upload, home,  lib,   query, api, none
		{false, false,  true, true,   false, false, false, true};

	static constexpr std::array<bool, static_cast<int>(Action::none)> require_filename =
	//   login, logout, view,  upload, home,  lib,  query, api, none
		{false, false,  false, true,   false, true, false, false};

	static const std::array<Parameters, static_cast<int>(Action::none)> intent_defintions;
	static const Parameters separator_fields;

public:
	URLIntent() = default;
	URLIntent(URLIntent&&) = default;
	URLIntent(const URLIntent&) = default;
	URLIntent& operator=(URLIntent&&) = default;
	URLIntent& operator=(const URLIntent&) = default;

	explicit URLIntent(boost::string_view target);
	URLIntent(Action act, std::string_view user, std::string_view coll, std::string_view name, std::string_view option = "");

	Action action() const {return m_action;}
	std::string_view user() const {return m_user;}
	std::string_view collection() const {return m_coll;}
	std::string_view filename() const {return m_filename;}
	std::string_view option() const {return m_option;}
	QueryTarget query_target() const {return m_query_target;}

	void set_option(std::string_view option) {m_option = option;}
	void add_option(std::string_view option);

	std::string str() const;
	bool valid() const;
	explicit operator bool() const {return valid();}

private:
	static std::string_view trim(std::string_view s);
	static Action parse_action(std::string_view str);
	void parse_field_from_left(std::string_view& target, Parameter p);
	void parse_field_from_right(std::string_view& target, Parameter p);
	static QueryTarget parse_query_target(std::string_view str);
	static std::string_view to_string(QueryTarget query_target);

private:
	Action  m_action{Action::none};

	std::string m_user;

	// container name
	std::string m_coll;

	// filename
	std::string m_filename;

	// option
	std::string m_option;

	QueryTarget m_query_target{QueryTarget::none};

	bool m_valid{false};
};

} // end of namespace hrb
