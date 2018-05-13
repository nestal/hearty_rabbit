/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
    This file is subject to the terms and conditions of the GNU General Public
    License.  See the file COPYING in the main directory of the hearty_rabbit
    distribution for more details.
*/

//
// Created by nestal on 5/13/18.
//

#pragma once

#include "util/BufferView.hh"
#include "util/FS.hh"

#include <opencv2/core.hpp>

#include <cstdint>

namespace hrb {

class PHash
{
public:
	PHash() = default;
	explicit PHash(std::uint64_t value) : m_hash{value} {}
	explicit PHash(cv::OutputArray arr);

	double compare(const PHash& other) const;
	auto value() const {return m_hash;}

private:
	std::uint64_t m_hash{};
};

inline bool operator==(const PHash& p1, const PHash& p2) {return p1.value() == p2.value();}
inline bool operator!=(const PHash& p1, const PHash& p2) {return p1.value() != p2.value();}
inline bool operator>=(const PHash& p1, const PHash& p2) {return p1.value() >= p2.value();}
inline bool operator<=(const PHash& p1, const PHash& p2) {return p1.value() <= p2.value();}
inline bool operator>(const PHash& p1, const PHash& p2) {return p1.value() > p2.value();}
inline bool operator<(const PHash& p1, const PHash& p2) {return p1.value() < p2.value();}

PHash phash(BufferView image);
PHash phash(fs::path file);
PHash phash(const cv::Mat& input);

} // end of namespace
