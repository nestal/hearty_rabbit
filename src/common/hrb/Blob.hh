/*
	Copyright © 2020 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 9/1/2020.
//

#pragma once

#include "ObjectID.hh"
#include "BlobInode.hh"

#include <string>
#include <vector>

namespace hrb {

class Blob
{
public:
	Blob() = default;
	Blob(std::string owner, std::string coll, const ObjectID& id, BlobInode en) :
		m_owner{std::move(owner)}, m_coll{std::move(coll)}, m_id{id}, m_info{std::move(en)}
	{
	}

	[[nodiscard]] auto& owner() const noexcept {return m_owner;}
	[[nodiscard]] auto& collection() const noexcept {return m_coll;}
	[[nodiscard]] auto& id() const noexcept {return m_id;}
	[[nodiscard]] auto& info() const noexcept {return m_info;}

private:
	std::string m_owner;
	std::string m_coll;
	ObjectID    m_id;
	BlobInode   m_info;
};

using BlobElements = std::vector<Blob>;

void to_json(nlohmann::json& dest, const BlobElements& src);
void from_json(const nlohmann::json& src, BlobElements& dest);

} // end of namespace hrb
