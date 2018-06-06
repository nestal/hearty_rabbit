/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/6/18.
//

#pragma once

#include <QtWidgets/QDialog>

#include "ui_LoginDialog.h"

class QWidget;

namespace hrb {

class LoginDialog : public QDialog
{
public:
	explicit LoginDialog(QWidget *parent);

private:
	Ui::LoginDialog m_;
};

} // end of namespace hrb
