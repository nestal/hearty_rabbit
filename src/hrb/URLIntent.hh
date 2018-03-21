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

#include <boost/utility/string_view.hpp>
#include <string_view>
#include <array>

namespace hrb {

// Deduce the intention of the user from the URL
class URLIntent
{
public:
	// TODO: narrow down the possibility of actions in these enum
	enum class Action {login, logout, blob, view, coll, upload, home, lib, listcolls, none};

private:
	// Required URL components
	static const std::array<bool, static_cast<int>(Action::none)> require_user, require_filename;

public:
	URLIntent() = default;
	explicit URLIntent(boost::string_view target);
	URLIntent(Action act, std::string_view user, std::string_view coll, std::string_view name);

	bool parse(boost::string_view target);

	Action action() const {return m_action;}
	std::string_view user() const {return m_user;}
	std::string_view collection() const {return m_coll;}
	std::string_view filename() const {return m_filename;}
	std::string_view option() const {return m_option;}

	std::string str() const;

	bool valid() const;

private:
	static std::string_view trim(std::string_view s);
	static Action parse_action(std::string_view str);

private:
	Action  m_action{Action::none};

	std::string_view m_user;

	// container name
	std::string_view m_coll;

	// filename
	std::string_view m_filename;

	// option
	std::string_view m_option;
};

} // end of namespace hrb
