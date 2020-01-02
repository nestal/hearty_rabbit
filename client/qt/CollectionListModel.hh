/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/10/18.
//

#pragma once


#include <QtCore/QAbstractListModel>
#include <QtGui/QIcon>

#include "common/hrb/CollectionList.hh"

namespace hrb {

class QtClient;

class CollectionListModel : public QAbstractListModel
{
	Q_OBJECT

public:
	CollectionListModel(QObject *parent, QtClient *hrb);

	int rowCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;

	CollectionList::Entry find(const QModelIndex& index) const;

public Q_SLOTS:
	void update(const CollectionList& coll);

private:
	QtClient    *m_hrb{};

	std::vector<CollectionList::Entry> m_entries;
};

} // end of namespace hrb
