/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/7/18.
//

#pragma once

#include <QtCore/QAbstractListModel>
#include "common/Collection.hh"

namespace hrb {

class CollectionModel : public QAbstractListModel
{
public:
	CollectionModel(QObject *parent);

	void update(const Collection& coll);

	int rowCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;

private:
	Collection m_coll;
	std::vector<ObjectID>   m_blob_ids;
};

} // end of namespace hrb
