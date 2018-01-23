/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>

    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.

    Copyright (C) 1998 - 2017, Daniel Stenberg, <daniel@haxx.se>, et al.
*/

#include "Escape.hh"

namespace {

// Copied from libcurl
// See https://tools.ietf.org/html/rfc3986#section-2.3
bool is_unreserved(unsigned char in)
{
	switch (in)
	{
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case 'a': case 'b': case 'c': case 'd': case 'e':
		case 'f': case 'g': case 'h': case 'i': case 'j':
		case 'k': case 'l': case 'm': case 'n': case 'o':
		case 'p': case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
		case '-': case '.': case '_': case '~':
			return true;
		default:
			break;
	}
	return false;
}

} // end of local namespace

namespace hrb {

std::string url_encode(std::string_view in)
{
	return {};
}

std::string url_decode(std::string_view in)
{
	visit_form_string(in, [](auto, auto){});
	return {};
}

std::string_view split_front(std::string_view& in, std::string_view value)
{
	// substr() will not throw even if "in" is empty and location==npos
	auto location = in.find_first_of(value);
	auto result   = in.substr(0, location);

	in.remove_prefix(result.size());

	// Remove the matching character, if any
	if (location != in.npos)
		in.remove_prefix(1);

	return result;
}

} // end of hrb namespace