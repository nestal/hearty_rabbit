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
#include <QtGui/QIcon>

#include "common/Collection.hh"

namespace hrb {

class QtClient;

class CollectionModel : public QAbstractListModel
{
	Q_OBJECT

public:
	CollectionModel(QObject *parent, QtClient *hrb);

	int rowCount(const QModelIndex& parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;

public slots:
	void update(const Collection& coll);
	void receive_blob(const ObjectID& id, const QString& rendition, const QByteArray& blob);

private:
	QtClient    *m_hrb{};

	std::vector<ObjectID>   m_blob_ids;

	Collection m_coll;
	std::unordered_map<ObjectID, QIcon>    m_images;
};

} // end of namespace hrb
