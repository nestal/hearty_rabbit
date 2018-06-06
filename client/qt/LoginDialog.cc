/*
	Copyright © 2018 Wan Wai Ho <me@nestal.net>
    
	This file is subject to the terms and conditions of the GNU General Public
	License.  See the file COPYING in the main directory of the hearty_rabbit
	distribution for more details.
*/

//
// Created by nestal on 6/6/18.
//

#include "LoginDialog.hh"

#include <QtWidgets/QAction>

namespace hrb {

LoginDialog::LoginDialog(QWidget *parent) : QDialog{parent}
{
	m_.setupUi(this);
}

QString LoginDialog::site() const
{
	return m_.site->text();
}

QString LoginDialog::username() const
{
	return m_.username->text();
}

QString LoginDialog::password() const
{
	return m_.password->text();
}

} // end of namespace hrb
