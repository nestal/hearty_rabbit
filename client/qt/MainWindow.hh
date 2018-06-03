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
#include <QMainWindow>

namespace hrb {

class MainWindow : public QMainWindow
{
public:
	MainWindow();

private:
	Ui::MainWindow  m_;
};

} // end of namespace hrb
