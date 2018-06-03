/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/3/18.
//

#pragma once

#include "ui_MainWindow.h"

#include <QtWidgets/QMainWindow>
#include <QtNetwork/QNetworkAccessManager>

class QFileSystemModel;

namespace hrb {

class MainWindow : public QMainWindow
{
public:
	MainWindow();

private:
	void list_hrb();

private:
	Ui::MainWindow      m_;
	QFileSystemModel    *m_fs_model{};
	QNetworkAccessManager   m_nam;
};

} // end of namespace hrb
