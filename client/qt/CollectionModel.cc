/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/7/18.
//

#include "CollectionModel.hh"

namespace hrb {

CollectionModel::CollectionModel(QObject *parent) :
	QAbstractListModel{parent}
{
}

int CollectionModel::rowCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : static_cast<int>(m_coll.size());
}

QVariant CollectionModel::data(const QModelIndex& index, int role) const
{
	assert(m_coll.size() == m_blob_ids.size());

	if (index.row() < rowCount(index))
	{
		if (auto it = m_coll.find(m_blob_ids[index.row()]); it != m_coll.blobs().end())
			return QString::fromStdString(it->second.filename);
	}
	return {};
}

void CollectionModel::update(const Collection& coll)
{
	emit layoutAboutToBeChanged();

	std::vector<ObjectID> ids;
	for (auto&& [id, en] : coll.blobs())
		ids.push_back(id);

	m_coll = coll;
	m_blob_ids = std::move(ids);

	changePersistentIndex({}, {});
	emit layoutChanged();
}

} // end of namespace hrb
