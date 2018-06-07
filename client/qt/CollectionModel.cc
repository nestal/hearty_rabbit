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
#include "QtClient.hh"

#include <iostream>

namespace hrb {

CollectionModel::CollectionModel(QObject *parent, QtClient *hrb) :
	QAbstractListModel{parent}, m_hrb{hrb}
{
	connect(m_hrb, &QtClient::on_get_blob, this, &CollectionModel::receive_blob);
}

int CollectionModel::rowCount(const QModelIndex& parent) const
{
	return parent.isValid() ? 0 : static_cast<int>(m_coll.size());
}

QVariant CollectionModel::data(const QModelIndex& index, int role) const
{
	assert(m_coll.size() == m_blob_ids.size());

	if (index.row() < m_blob_ids.size())
	{
		assert(index.row() < static_cast<int>(m_blob_ids.size()));

		if (auto it = m_coll.find(m_blob_ids[index.row()]); it != m_coll.blobs().end())
		{
			switch (role)
			{
				case Qt::DisplayRole: return QString::fromStdString(it->second.filename);
				case Qt::DecorationRole:
				{
					if (auto image = m_images.find(it->first); image != m_images.end())
						return image->second;
				}
			}
		}
	}
	return {};
}

void CollectionModel::update(const Collection& coll)
{
	emit layoutAboutToBeChanged();

	std::vector<ObjectID> ids;
	for (auto&& [id, en] : coll.blobs())
	{
		ids.push_back(id);
		m_hrb->get_blob(QString::fromStdString(std::string{coll.owner()}), QString::fromStdString(std::string{coll.name()}), id, "thumbnail");
	}

	m_coll = coll;
	m_blob_ids = std::move(ids);

	changePersistentIndex({}, {});
	emit layoutChanged();
}

void CollectionModel::receive_blob(const ObjectID& id, const QString& rendition, const QByteArray& blob)
{
	std::cout << "receiving " << blob.size() << " bytes " << blob.toStdString() << std::endl;

	QImage image;
	if (image.loadFromData(blob))
	{
		m_images.emplace(id, QIcon{QPixmap::fromImage(std::move(image))});

		auto row = std::find(m_blob_ids.begin(), m_blob_ids.end(), id);
		if (row != m_blob_ids.end())
		{
			auto idx = index(row-m_blob_ids.begin(), 0);
			emit dataChanged(idx, idx, {Qt::DecorationRole});
		}
	}
}

} // end of namespace hrb
