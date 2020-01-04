/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/10/18.
//

#include "CollectionListModel.hh"

#include <cassert>
#include <iostream>

namespace hrb {

CollectionListModel::CollectionListModel(QObject *parent, QtClient *hrb) :
	QAbstractListModel{parent}, m_hrb{hrb}
{
	assert(m_hrb);
}

int CollectionListModel::rowCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

QVariant CollectionListModel::data(const QModelIndex& index, int role) const
{
	return index.row() < m_entries.size() && role == Qt::DisplayRole ?
		QString::fromStdString(std::string{m_entries[index.row()].name()}) :
		QVariant();
}

void CollectionListModel::update(const CollectionList& coll)
{
	Q_EMIT layoutAboutToBeChanged();

	auto en = coll.entries();
	m_entries.assign(en.begin(), en.end());

	changePersistentIndex({}, {});
	Q_EMIT layoutChanged();
}

Collection CollectionListModel::find(const QModelIndex& index) const
{
	return m_entries.at(static_cast<std::size_t>(index.row()));
}

} // end of namespace hrb
